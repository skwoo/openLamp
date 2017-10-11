/* 
   This file is part of openML, mobile and embedded DBMS.

   Copyright (C) 2012 Inervit Co., Ltd.
   support@inervit.com

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Less General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Less General Public License for more details.

   You should have received a copy of the GNU Less General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mdb_config.h"
#include "dbport.h"

#include "mdb_compat.h"

#include "sql_datast.h"
#include "sql_decimal.h"
#include "mdb_er.h"
#include "ErrorCode.h"
#include "sql_main.h"

extern int evaluate_expression(T_EXPRESSIONDESC * expr, MDB_BOOL is_orexpr);
int evaluate_conjunct(EXPRDESC_LIST * list);
void set_exprvalue(T_RTTABLE * rttable, T_EXPRESSIONDESC * expr, OID rid);

int open_cursor(int connection, T_RTTABLE * rttable,
        T_NESTING * nest, EXPRDESC_LIST * expr_list, MDB_UINT8 mode);
int close_cursor(int connection, T_RTTABLE * rttable);

int datetime2char(char *str, char *datetime);
int char2datetime(char *datetime, char *str, int str_len);
int timestamp2char(char *str, t_astime * timestamp);
int char2timestamp(t_astime * timestamp, char *str, int str_len);
int calc_func_convert(T_VALUEDESC * base, T_VALUEDESC * src, T_VALUEDESC * res,
        MDB_INT32 is_init);
int make_single_keyvalue(T_NESTING * nest, T_KEYDESC * key);
int delete_single_keyvalue(T_KEYDESC * key);

extern int check_numeric_bound(T_VALUEDESC * value, DataType type, int length);

/*****************************************************************************/
//! set_updaterecord

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param rttable()
 * \param col_list(): 
 * \param val_list():
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *  - 첫번째 레코드의 칼럼이 NULL인 경우, 2번째 레코드도 덩달아 null로 바뀜
 *  - BYTE/VARBYTE 지원
 *
 *****************************************************************************/
int
set_updaterecord(T_RTTABLE * rttable, T_LIST_COLDESC * col_list,
        T_LIST_VALUE * val_list)
{
    T_EXPRESSIONDESC *expr = NULL;
    SYSFIELD_T *field;
    T_VALUEDESC *val, res;
    char *buf, *target;

    ibool need_free_mem = 0;
    int i, j = 0, ret = RET_SUCCESS;

    sc_memset(&res, 0, sizeof(res));

    res.is_null = 1;

    buf = PMEM_ALLOC(sizeof(char) * rttable->rec_len);
    sc_memcpy(buf, rttable->rec_buf, rttable->rec_len);

    for (i = 0; i < col_list->len; i++)
    {
        field = (col_list->list[i]).fieldinfo;
        expr = &val_list->list[i];
        target = rttable->rec_buf + field->offset;
        if (!expr->len)
        {   // update NULL
            DB_VALUE_SETNULL(rttable->rec_buf,
                    field->position, rttable->nullflag_offset);
        }
        else
        {
            for (j = 0; j < expr->len; j++)
            {
                if (expr->list[j]->elemtype == SPT_OPERATOR ||
                        expr->list[j]->elemtype == SPT_FUNCTION ||
                        expr->list[j]->elemtype == SPT_AGGREGATION ||
                        expr->list[j]->elemtype == SPT_SUBQUERY ||
                        expr->list[j]->elemtype == SPT_SUBQ_OP)
                    continue;
                if (expr->list[j]->elemtype == SPT_VALUE)
                    expr->list[j]->u.value.attrdesc.collation =
                            field->collation;
                val = &(expr->list[j]->u.value);
                if (val->valueclass == SVC_CONSTANT)
                    continue;

                if (DB_VALUE_ISNULL(buf,
                                val->attrdesc.posi.ordinal,
                                rttable->nullflag_offset))
                {
                    val->valuetype = val->attrdesc.type;
                    val->is_null = 1;
                }
                else
                {       // if variable

                    if (val->valueclass == SVC_NONE &&
                            val->attrdesc.type == DT_NULL_TYPE)
                        continue;

                    val->is_null = 0;
                    val->valuetype = val->attrdesc.type;

                    switch (val->attrdesc.type)
                    {
                    case DT_VARCHAR:   // string
                    case DT_NVARCHAR:
                        if (!val->call_free &&
                                sql_value_ptrAlloc(val,
                                        val->attrdesc.len + 1) < 0)
                            return RET_ERROR;

                        strncpy_func[val->attrdesc.type] (val->u.ptr,
                                buf + val->attrdesc.posi.offset,
                                val->attrdesc.len);
                        val->value_len =
                                strlen_func[val->attrdesc.type] (val->u.ptr);
                        break;

                    case DT_CHAR:      // bytes
                    case DT_NCHAR:
                        if (!val->call_free &&
                                sql_value_ptrXcalloc(val,
                                        val->attrdesc.len + 1) < 0)
                            return RET_ERROR;

                        memcpy_func[val->attrdesc.type] (val->u.ptr,
                                buf + val->attrdesc.posi.offset,
                                val->attrdesc.len);
                        val->value_len =
                                strlen_func[val->attrdesc.type] (val->u.ptr);
                        break;
                    case DT_SMALLINT:  // smallint
                        val->u.i16 =
                                *(smallint *) (buf +
                                val->attrdesc.posi.offset);
                        break;
                    case DT_INTEGER:   // int
                        val->u.i32 =
                                *(int *) (buf + val->attrdesc.posi.offset);
                        break;
                    case DT_FLOAT:     // float
                        val->u.f16 =
                                *(float *) (buf + val->attrdesc.posi.offset);
                        break;
                    case DT_DOUBLE:    // double
                        val->u.f32 =
                                *(double *) (buf + val->attrdesc.posi.offset);
                        break;
                    case DT_TINYINT:   // tinyint
                        val->u.i8 =
                                *(tinyint *) (buf + val->attrdesc.posi.offset);
                        break;
                    case DT_BIGINT:    // bigint
                        val->u.i64 =
                                *(bigint *) (buf + val->attrdesc.posi.offset);
                        break;
                    case DT_DATETIME:  // datetime
                        sc_memcpy(val->u.datetime,
                                buf + val->attrdesc.posi.offset,
                                MAX_DATETIME_FIELD_LEN);
                        break;
                    case DT_DATE:      // date
                        sc_memcpy(val->u.date,
                                buf + val->attrdesc.posi.offset,
                                MAX_DATE_FIELD_LEN);
                        break;
                    case DT_TIME:      // time
                        sc_memcpy(val->u.time,
                                buf + val->attrdesc.posi.offset,
                                MAX_TIME_FIELD_LEN);
                        break;
                    case DT_TIMESTAMP: // timestamp
                        sc_memcpy(&val->u.timestamp,
                                buf + val->attrdesc.posi.offset,
                                MAX_TIMESTAMP_FIELD_LEN);
                        break;
                    case DT_DECIMAL:   // decimal
                        char2decimal(&val->u.dec,
                                buf + val->attrdesc.posi.offset,
                                sc_strlen(buf + val->attrdesc.posi.offset));
                        break;
                    case DT_BYTE:      // bytes
                    case DT_VARBYTE:
                        if (!val->call_free &&
                                sql_value_ptrXcalloc(val,
                                        val->attrdesc.len) < 0)
                            return RET_ERROR;

                        DB_GET_BYTE_LENGTH(buf + val->attrdesc.posi.offset,
                                val->value_len);
                        sc_memcpy(val->u.ptr,
                                buf + INT_SIZE + val->attrdesc.posi.offset,
                                val->value_len);
                        break;
                    default:   // vchar, blob,...
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_ERROR, 0);
                        need_free_mem = 1;
                        ret = RET_ERROR;
                        goto RETURN_LABEL;
                    }
                }
            }   // for loop

            ret = calc_expression(expr, &res, MDB_FALSE);
            if (ret < 0)
            {
                need_free_mem = 1;
                ret = RET_ERROR;
                goto RETURN_LABEL;
            }

            if (check_numeric_bound(&res, field->type, field->length)
                    == RET_FALSE)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_TOOLARGEVALUE, 0);
                need_free_mem = 1;
                ret = RET_ERROR;
                goto RETURN_LABEL;
            }

            if (res.is_null)
            {
                if (field->flag & NULL_BIT)
                {
                    DB_VALUE_SETNULL(rttable->rec_buf,
                            field->position, rttable->nullflag_offset);
                }
                else
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_NOTALLOWEDNULLVALUE, 0);
                    need_free_mem = 1;
                    ret = RET_ERROR;
                    goto RETURN_LABEL;
                }
            }
            else
            {
                ret = copy_value_into_record(rttable->Hcursor, target, &res,
                        field, 0);
                if (ret < 0)
                {
                    ret = RET_ERROR;
                    goto RETURN_LABEL;
                }
                DB_VALUE_SETNOTNULL(rttable->rec_buf,
                        field->position, rttable->nullflag_offset);
            }

            if (!res.is_null)
                sql_value_ptrFree(&res);
        }
    }

  RETURN_LABEL:

    if (need_free_mem)
    {
        for (j--; j >= 0; j--)
        {
            if (expr->list[j]->elemtype == SPT_OPERATOR)
                continue;
            if (expr->list[j]->elemtype == SPT_FUNCTION)
                continue;
            if (expr->list[j]->elemtype == SPT_AGGREGATION)
                continue;

            val = &(expr->list[j]->u.value);
            if (val->valueclass == SVC_VARIABLE)
                sql_value_ptrFree(val);

        }

    }

    if (!res.is_null)
        sql_value_ptrFree(&res);

    PMEM_FREENUL(buf);

    if (ret < 0)
        return RET_ERROR;
    else
        return RET_SUCCESS;
}

/*****************************************************************************/
//! update_fieldvalue

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param upd_value_p(in)   :
 * \param fldidx()      : 
 * \param field()       :
 * \param fval()        :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - called : set_updatevalues()
 *  - BYTE, VARBYTE 추가
 *****************************************************************************/
int
update_fieldvalue(struct UpdateValue *upd_value_p, int fldidx,
        SYSFIELD_T * field, T_VALUEDESC * fval)
{
    char str_decimal[MAX_DECIMAL_STRING_LEN];
    UPDATEFIELDVALUE_T *update_fieldval;
    int l, bl;

    if (fldidx >= upd_value_p->update_field_count)
    {
        update_fieldval = NULL;
    }
    else
    {
        update_fieldval = &(upd_value_p->update_field_value[fldidx]);
    }

    if (update_fieldval == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDARGUMENT, 0);
        return RET_ERROR;
    }

    update_fieldval->pos = (DB_UINT16) field->position;
    update_fieldval->isnull = (DB_UINT8) fval->is_null;
    update_fieldval->Blength = 0;
    update_fieldval->data = &(update_fieldval->space[0]);

    switch (field->type)
    {
    case DT_TINYINT:
        *(char *) update_fieldval->data = fval->u.i8;
        break;
    case DT_SMALLINT:
        *(short *) update_fieldval->data = fval->u.i16;
        break;
    case DT_INTEGER:
        *(int *) update_fieldval->data = fval->u.i32;
        break;
    case DT_BIGINT:
        *(DB_INT64 *) update_fieldval->data = fval->u.i64;
        break;
    case DT_FLOAT:
        *(float *) update_fieldval->data = fval->u.f16;
        break;
    case DT_DOUBLE:
        *(double *) update_fieldval->data = fval->u.f32;
        break;
    case DT_DECIMAL:
        (void) decimal2char(str_decimal, fval->u.dec, field->length,
                field->scale);

        l = sc_strlen(str_decimal) + 1;
        if (l == 0 || str_decimal == NULL)
        {
            update_fieldval->isnull = 1;
            update_fieldval->data = NULL;
        }
        else
        {
            update_fieldval->Blength = l;
            if (update_fieldval->Blength > UPDATE_FLDVAL_FIXEDSIZE)
            {
                update_fieldval->data =
                        (char *) PMEM_ALLOC(update_fieldval->Blength);
                if (update_fieldval->data == NULL)
                {
                    ;   /* what should we do ? */
                }
            }
            sc_memcpy(update_fieldval->data, str_decimal,
                    update_fieldval->Blength);
        }

        break;
    case DT_CHAR:
        l = field->length;
        if (l == 0 || fval->u.ptr == NULL)
        {
            update_fieldval->isnull = 1;
            update_fieldval->data = NULL;
        }
        else
        {
            update_fieldval->Blength = l + 1;
            if (update_fieldval->Blength > UPDATE_FLDVAL_FIXEDSIZE)
            {
                update_fieldval->data =
                        (char *) PMEM_ALLOC(update_fieldval->Blength);
                if (update_fieldval->data == NULL)
                {
                    ;   /* what should we do ? */
                }
            }
            sc_memcpy(update_fieldval->data, fval->u.ptr,
                    update_fieldval->Blength);
        }
        break;
    case DT_NCHAR:
        l = field->length;
        if (l == 0 || fval->u.Wptr == NULL)
        {
            update_fieldval->isnull = 1;
            update_fieldval->data = NULL;
        }
        else
        {
            update_fieldval->Blength = (l + 1) * sizeof(DB_WCHAR);
            if (update_fieldval->Blength > UPDATE_FLDVAL_FIXEDSIZE)
            {
                update_fieldval->data = PMEM_WALLOC(l + 1);
                if (update_fieldval->data == NULL)
                {
                    ;   /* what should we do ? */
                }
            }
            sc_wmemcpy((DB_WCHAR *) update_fieldval->data, fval->u.Wptr,
                    l + 1);
        }
        break;
    case DT_VARCHAR:
        l = field->length;
        if (l == 0 || fval->u.ptr == NULL)
        {
            update_fieldval->isnull = 1;
            update_fieldval->data = NULL;
        }
        else
        {
            update_fieldval->Blength = sc_strlen(fval->u.ptr) + 1;
            if (update_fieldval->Blength > UPDATE_FLDVAL_FIXEDSIZE)
            {
                int len = l + 1;

                if (len < (int) update_fieldval->Blength)
                    len = update_fieldval->Blength;
                update_fieldval->data = (char *) PMEM_ALLOC(len);
                if (update_fieldval->data == NULL)
                {
                    ;   /* what should we do ? */
                }
            }
            sc_memcpy(update_fieldval->data, fval->u.ptr,
                    update_fieldval->Blength);
        }
        break;
    case DT_NVARCHAR:
        l = field->length;
        if (l == 0 || fval->u.Wptr == NULL)
        {
            update_fieldval->isnull = 1;
            update_fieldval->data = NULL;
        }
        else
        {
            int wstrlen;

            wstrlen = sc_wcslen(fval->u.Wptr);
            update_fieldval->Blength = (wstrlen + 1) * sizeof(DB_WCHAR);
            if (update_fieldval->Blength > UPDATE_FLDVAL_FIXEDSIZE)
            {
                update_fieldval->data = PMEM_WALLOC(wstrlen + 1);
                if (update_fieldval->data == NULL)
                {
                    ;   /* what should we do ? */
                }
            }
            sc_wmemcpy((DB_WCHAR *) update_fieldval->data, fval->u.Wptr,
                    wstrlen + 1);
        }
        break;
    case DT_DATETIME:
        if (fval->u.datetime == NULL)
        {
            update_fieldval->isnull = 1;
            update_fieldval->data = NULL;
        }
        else
        {
            sc_memcpy(update_fieldval->data, fval->u.datetime, 8);
        }
        break;
    case DT_DATE:
        if (fval->u.date == NULL)
        {
            update_fieldval->isnull = 1;
            update_fieldval->data = NULL;
        }
        else
        {
            sc_memcpy(update_fieldval->data, fval->u.date, 8);
        }
        break;
    case DT_TIME:
        if (fval->u.time == NULL)
        {
            update_fieldval->isnull = 1;
            update_fieldval->data = NULL;
        }
        else
        {
            sc_memcpy(update_fieldval->data, fval->u.time, 8);
        }
        break;
    case DT_TIMESTAMP:
        *(DB_INT64 *) update_fieldval->data = fval->u.timestamp;
        break;
    case DT_BYTE:
        l = field->length;
        bl = fval->value_len;

        if (l == 0 || fval->u.ptr == NULL || bl == 0)
        {
            update_fieldval->isnull = 1;
            update_fieldval->data = NULL;
        }
        else
        {
            update_fieldval->Blength = bl + INT_SIZE;
            if (update_fieldval->Blength > UPDATE_FLDVAL_FIXEDSIZE)
            {
                update_fieldval->data =
                        (char *) PMEM_XCALLOC(update_fieldval->Blength);
                if (update_fieldval->data == NULL)
                {
                    ;   /* what should we do ? */
                }
            }
            sc_memcpy(update_fieldval->data, &bl, INT_SIZE);
            sc_memcpy(update_fieldval->data + INT_SIZE, fval->u.ptr, bl);
        }

        break;
    case DT_VARBYTE:
        l = field->length;
        bl = fval->value_len;

        if (l == 0 || fval->u.ptr == NULL || bl == 0)
        {
            update_fieldval->isnull = 1;
            update_fieldval->data = NULL;
        }
        else
        {
            update_fieldval->Blength = bl + INT_SIZE;
            if (update_fieldval->Blength > UPDATE_FLDVAL_FIXEDSIZE)
            {
                update_fieldval->data =
                        (char *) PMEM_XCALLOC(update_fieldval->Blength);
                if (update_fieldval->data == NULL)
                {
                    ;   /* what should we do ? */
                }
            }
            sc_memcpy(update_fieldval->data, &bl, INT_SIZE);
            sc_memcpy(update_fieldval->data + INT_SIZE, fval->u.ptr, bl);
        }

        break;
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
        return RET_ERROR;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! set_updatevalues

/*! \breif  UPDATE 문장을 수행
 ************************************
 * \param upd_value_p(in)   :
 * \param col_list(in)      : 
 * \param val_list(in/out)  : 
 ************************************
 * \return  return_value
 ************************************
 * \note
 * - called : exec_update()
 *****************************************************************************/
int
set_updatevalues(struct UpdateValue *upd_value_p, T_LIST_COLDESC * col_list,
        T_LIST_VALUE * val_list)
{
    T_EXPRESSIONDESC *expr;
    SYSFIELD_T *field;
    T_VALUEDESC val, fval, fdesc;
    int i;

    for (i = 0; i < col_list->len; i++)
    {
        field = (col_list->list[i]).fieldinfo;
        expr = &val_list->list[i];

        if (expr->len == 0)     /* update null */
        {
            sc_memset(&val, 0, sizeof(T_VALUEDESC));
            val.is_null = 1;
        }
        else if (expr->len > 1)
        {
            return RET_ERROR;   /* do record-update routine not field-update */
        }
        else
        {
            if (expr->list[0]->u.value.valueclass == SVC_VARIABLE)
                return RET_ERROR;       /* do record-update routine not field-update */

            if (calc_expression(expr, &val, MDB_FALSE) < 0)
            {
                sql_value_ptrFree(&val);
                return er_errid();
            }

            if (check_numeric_bound(&val, field->type,
                            field->length) == RET_FALSE)
            {
                sql_value_ptrFree(&val);
                return SQL_E_TOOLARGEVALUE;
            }
        }

        if (val.is_null != 1)
        {
            if ((IS_BS(field->type) && IS_BS(val.valuetype)) ||
                    (IS_NBS(field->type) && IS_NBS(val.valuetype)))
            {
                if (strlen_func[val.valuetype] (val.u.ptr) > field->length)
                {
                    sql_value_ptrFree(&val);
                    return SQL_E_TOOLARGEVALUE;
                }
            }
            else if (IS_BYTE(field->type) && IS_BYTE(val.valuetype))
            {
                if (val.value_len > field->length)
                {
                    sql_value_ptrFree(&val);
                    return SQL_E_TOOLARGEVALUE;
                }
            }
        }

        fdesc.valueclass = SVC_NONE;
        fdesc.valuetype = field->type;
        fdesc.attrdesc.len = field->length;
        if (field->type == DT_DECIMAL)
        {
            fdesc.attrdesc.dec = field->scale;
        }

        sc_memset(&fval, 0, sizeof(T_VALUEDESC));

        if (calc_func_convert(&fdesc, &val, &fval, MDB_FALSE) < 0)
        {
            sql_value_ptrFree(&val);
            return er_errid();
        }

        if (fval.is_null != 1)
        {
            if (IS_BS_OR_NBS(fval.valuetype))
            {
                if (strlen_func[field->type] (fval.u.ptr) > field->length)
                {
                    sql_value_ptrFree(&fval);
                    sql_value_ptrFree(&val);
                    return SQL_E_TOOLARGEVALUE;
                }
            }
            else if (IS_BYTE(fval.valuetype))
            {
                if (fval.value_len > field->length)
                {
                    sql_value_ptrFree(&fval);
                    sql_value_ptrFree(&val);
                    return SQL_E_TOOLARGEVALUE;
                }
            }
        }

        if (!(field->flag & NULL_BIT) && fval.is_null)
        {
            sql_value_ptrFree(&fval);
            sql_value_ptrFree(&val);
            return SQL_E_NOTALLOWEDNULLVALUE;
        }

        if (update_fieldvalue(upd_value_p, i, field, &fval) < RET_SUCCESS)
            return RET_ERROR;
        sql_value_ptrFree(&val);
        sql_value_ptrFree(&fval);
    }
    return RET_SUCCESS;
}

/*****************************************************************************/
//!  exec_update

/*! \breif  UPDATE 문장을 수행
 ************************************
 * \param handle(in)    : DB Handle
 * \param stmt(in/out)  : T_STATEMENT
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
exec_update(int handle, T_STATEMENT * stmt)
{
    T_LIST_COLDESC *col_list;
    T_LIST_VALUE *val_list;
    EXPRDESC_LIST *cur;
    T_RTTABLE *rttable;
    T_NESTING *nest;
    int ret = RET_SUCCESS;
    int i = 0;
    int do_field_update = 1;
    int already_updated = 0;
    struct UpdateValue upd_values;
    T_KEYDESC key;
    T_QUERYDESC *qdesc;
    T_EXPRESSIONDESC *condition;
    int cursor = -1;
    DB_BOOL has_index = 0;
    DB_BOOL create_col_pos = 0;
    int *col_pos = NULL;

    col_list = &stmt->u._update.columns;
    val_list = &stmt->u._update.values;
    rttable = stmt->u._update.rttable;
    nest = stmt->u._update.planquerydesc.nest;
    qdesc = &stmt->u._update.planquerydesc.querydesc;
    condition = &qdesc->condition;

    upd_values.update_field_value = NULL;
    upd_values.update_field_count = 0;

    if (IS_SINGLE_OP(nest) &&
            (rttable[0].db_expr_list || rttable[0].sql_expr_list))
    {
        nest->cnt_single_item = 0;
    }

    if (rttable[0].sql_expr_list)
        do_field_update = 0;
    else if (condition->list && qdesc->expr_list == NULL)
        do_field_update = 0;
    if (stmt->u._update.columns.len >= 8)
        do_field_update = 0;
    if (stmt->u._update.subselect.first_sub
            || stmt->u._update.subselect.sibling_query)
        do_field_update = 0;

    if (do_field_update || stmt->u._update.rid != NULL_OID
            || nest->scanrid != NULL_OID)
    {
        upd_values = dbi_UpdateValue_Create(col_list->len);
        ret = set_updatevalues(&upd_values, col_list, val_list);

        if (ret < 0 && ret != RET_ERROR)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            ret = RET_ERROR;

            goto RETURN_PROCESS;
        }

        if (ret < 0)
            do_field_update = 0;

        /* method 1/2 : update by rid or direct api */
        if (stmt->u._update.rid != NULL_OID)
        {
            ret = dbi_Rid_Update(handle, stmt->u._update.rid, &upd_values);
            already_updated = 1;
        }
        else if (do_field_update && !stmt->u._update.has_variable &&
                IS_SINGLE_OP(nest) && isPrimaryIndex_name(nest->indexname))
        {
            ret = make_single_keyvalue(nest, &key);
            if (ret < 0)
                goto RETURN_PROCESS;

            ret = dbi_Direct_Update_Field(handle, nest->table.tablename,
                    nest->indexname, &key, &upd_values, LK_EXCLUSIVE);

            delete_single_keyvalue(&key);
            already_updated = 1;
        }

        if (already_updated)
        {
            if (ret >= 0)
            {
                stmt->u._update.rows++;
                ret = RET_SUCCESS;
            }
            else if (ret == DB_E_NOMORERECORDS)
            {
                ret = RET_SUCCESS;
            }
            else if (ret != DB_E_INVALIDPARAM)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                ret = RET_ERROR;
            }
            goto RETURN_PROCESS;
        }
    }

    if (!rttable->table.tablename)
    {       // 전에 prepare & execute를 수행했다면..
        rttable->table.tablename = nest->table.tablename;
        rttable->table.aliasname = nest->table.aliasname;
        rttable->nullflag_offset = get_nullflag_offset(handle,
                rttable->table.tablename, &qdesc->fromlist.list[0].tableinfo);
        rttable->rec_len = get_recordlen(handle, rttable->table.tablename,
                &qdesc->fromlist.list[0].tableinfo);
        if (do_field_update && nest->scanrid == NULL_OID)
            PMEM_FREENUL(rttable->rec_buf);
        else
        {
            int rec_len;

            rec_len = rttable->rec_len;

            rttable->rec_buf = PMEM_ALLOC(sizeof(char) * rec_len);
        }
    }

    if (rttable->rec_len < 0)
    {
        ret = RET_ERROR;
        goto RETURN_PROCESS;
    }

    if (nest->scanrid != NULL_OID && do_field_update)
    {
        int row = 1;
        int in_pos = 0;

        if (nest->ridin_cnt > 0)
        {
            row = nest->ridin_cnt;

            for (i = 0; i < condition->len; i++)
            {
                if (condition->list[i]->u.op.type == SOT_IN)
                {
                    in_pos = i - nest[0].ridin_cnt;
                    break;
                }
            }
        }
        else
            ret = 1;

        for (i = 0; i < row; i++)
        {
            if (nest->ridin_cnt > 0)
            {
                nest->scanrid = condition->list[in_pos + i]->u.value.u.oid;
            }

            if ((nest->ridin_cnt == 0 && condition->len > 3) ||
                    nest->ridin_cnt != 0)
            {
                ret = dbi_Direct_Read_ByRid(handle,
                        nest->table.tablename,
                        nest->scanrid,
                        &(qdesc->fromlist.list[0].tableinfo),
                        LK_SHARED, rttable->rec_buf,
                        (DB_UINT32 *) & (rttable->rec_len), NULL);

                if (ret >= 0)
                {
                    set_exprvalue(rttable, condition, nest->scanrid);
                    ret = evaluate_expression(condition, 0);
                    if (ret < 0)
                    {
                        ret = RET_ERROR;
                        goto RETURN_PROCESS;
                    }
                }
            }

            if (ret == RET_SUCCESS)
            {
                ret = dbi_Rid_Update(handle, nest->scanrid, &upd_values);
            }
            if (ret >= 0)
            {
                stmt->u._update.rows++;
            }
            else if (ret == DB_E_NOMORERECORDS)
            {
                ret = RET_END;
                goto RETURN_PROCESS;
            }
            else if (ret != DB_E_INVALIDPARAM)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                ret = RET_ERROR;
                goto RETURN_PROCESS;
            }
        }

        ret = RET_SUCCESS;
        goto RETURN_PROCESS;
    }

    /* method 3/4 field update or record update */
    rttable[0].in_expr_idx = -1;

    ret = dbi_Schema_GetTableIndexesInfo(handle, rttable->table.tablename,
            SCHEMA_REGU_INDEX, NULL);
    if (ret < 0)
    {
        goto RETURN_PROCESS;
    }
    else if (ret > 0)
    {
        has_index = 1;
        create_col_pos = 1;
    }

    if (!do_field_update && qdesc->fromlist.list[0].tableinfo.has_variabletype)
    {
        create_col_pos = 1;
    }

    if (create_col_pos)
    {
        col_pos = (int *) PMEM_ALLOC(sizeof(int) * (col_list->len + 1));
        if (col_pos == NULL)
        {
            ret = SQL_E_OUTOFMEMORY;
            goto RETURN_PROCESS;
        }

        col_pos[0] = col_list->len;

        for (i = 0; i < col_list->len; ++i)
        {
            col_pos[i + 1] = col_list->list[i].fieldinfo->position;
        }

        i = 0;
    }

    do
    {
        if (nest->op_in)
        {
            if (rttable[0].in_expr_idx == -1 && nest->index_field_num > 0)
            {
                cursor = dbi_Cursor_Open(handle, rttable->table.tablename,
                        NULL, NULL, NULL, NULL, LK_EXCLUSIVE, 0);
                if (cursor < 0)
                {
                    ret = RET_ERROR;
                    goto RETURN_PROCESS;
                }
            }

            rttable[0].in_expr_idx += 1;
            if (rttable[0].in_expr_idx == nest->in_exprs)
            {
                ret = RET_SUCCESS;
                goto RETURN_PROCESS;
            }
        }

        ret = open_cursor(handle, &(rttable[0]),
                nest, rttable[0].db_expr_list, LK_EXCLUSIVE);
        if (ret < 0)
        {
            ret = RET_ERROR;
            goto RETURN_PROCESS;
        }
        else if (ret == RET_FALSE)
        {
            close_cursor(handle, rttable);

            if (i == nest->in_exprs)
            {
                ret = RET_SUCCESS;
                goto RETURN_PROCESS;
            }
        }

        if (has_index)
        {
            ret = dbi_Cursor_Set_updidx(handle, rttable->Hcursor, col_pos);
            if (ret < 0)
            {
                ret = RET_ERROR;
                goto RETURN_PROCESS;
            }
        }

        /* ret : SUCCESS, FAIL, DB_E_NOMORERECORDS, DB_E_TERMINATED */
        while (1)
        {
            if (do_field_update)
            {
                /* skip evaluation */
                ret = RET_SUCCESS;
            }
            else
            {
                OID cur_rid = NULL_OID;

                ret = dbi_Cursor_GetRid(handle, rttable->Hcursor, &cur_rid);
                if (ret < 0)
                {
                    if (ret != DB_E_NOMORERECORDS)
                        ret = RET_ERROR;
                    break;
                }
                sc_memset(rttable->rec_buf, 0, rttable->rec_len);
                ret = dbi_Cursor_Read(handle, rttable->Hcursor,
                        rttable->rec_buf, &(rttable->rec_len), NULL, 0, 0);
                if (ret != DB_SUCCESS)
                    break;
                if (rttable[0].sql_expr_list == NULL)
                {
                    if ((condition->list && qdesc->expr_list == NULL)
                            || nest->scanrid != NULL_OID)
                    {
                        set_exprvalue(&rttable[0], condition, cur_rid);
                        ret = evaluate_expression(condition, 0);
                    }
                    else
                        ret = RET_SUCCESS;
                }
                else
                {
                    for (cur = rttable[0].sql_expr_list; cur; cur = cur->next)
                        set_exprvalue(&rttable[0], cur->ptr, cur_rid);
                    ret = evaluate_conjunct(rttable[0].sql_expr_list);
                }
            }

            if (ret < 0)
            {
                ret = RET_ERROR;
                break;
            }
            else if (ret)       // success
            {
                if (stmt->u._update.rows == qdesc->limit.rows)
                {
                    break;
                }

                if (do_field_update)
                {
                    ret = dbi_Cursor_Update_Field(handle, rttable->Hcursor,
                            &upd_values);
                }
                else
                {
                    ret = set_updaterecord(rttable, col_list, val_list);
                    if (ret == RET_SUCCESS)
                    {
                        ret = dbi_Cursor_Update(handle, rttable->Hcursor,
                                rttable->rec_buf, rttable->rec_len, col_pos);
                    }
                }

                if (ret < 0)
                {
                    if (ret != DB_E_NOMORERECORDS)
                    {
                        if (ret == DB_E_NOTNULLFIELD)
                            ret = SQL_E_NOTALLOWEDNULLVALUE;
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                        ret = RET_ERROR;
                    }
                    break;
                }
                stmt->u._update.rows++;
            }
            else
            {   /* fetch next in record update */
                ret = dbi_Cursor_Read(handle, rttable->Hcursor,
                        rttable->rec_buf, &(rttable->rec_len), NULL, 1, 0);
                if (ret < 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    ret = RET_ERROR;
                    break;
                }

            }
        }

        close_cursor(handle, rttable);

        if (ret == RET_ERROR)
            break;

        i += 1;

    }
    while (i < nest->in_exprs);

  RETURN_PROCESS:

    if (col_pos)
    {
        PMEM_FREE(col_pos);
    }

    if (cursor > -1)
        dbi_Cursor_Close(handle, cursor);

    dbi_UpdateValue_Destroy(&upd_values);

    close_cursor(handle, rttable);

    return ret;
}
