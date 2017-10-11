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

#include "sql_main.h"
#include "sql_decimal.h"
#include "sql_execute.h"
#include "mdb_ppthread.h"

#include "sql_execute.h"
#include "sql_norm.h"

extern void set_exprvalue(T_RTTABLE * rttable, T_EXPRESSIONDESC * expr,
        OID rid);
extern void convert_attr2value(T_ATTRDESC * attr, T_VALUEDESC * val,
        char *rec);
extern int exec_insert_subquery(int handle, T_SELECT * select,
        T_RTTABLE * rttable, T_PARSING_MEMORY * main_parsing_chunk);

extern void convert_rec_attr2value(T_ATTRDESC * attr,
        FIELDVALUES_T * rec_values, int position, T_VALUEDESC * val);
extern FIELDVALUES_T *init_rec_values(T_RTTABLE * rttable,
        SYSTABLE_T * tableinfo, SYSFIELD_T * fieldsinfo, int select_star);

typedef struct
{
    ihandle Htable;
    char table[MAX_TABLE_NAME_LEN];
    SYSFIELD_T *fieldinfo;
    int fieldnum;
    char *rec_buf;
    int rec_buf_len;
    int nullflag_offset;
    int column_index;
    int field_index;
} t_distinctinfo;

typedef struct
{
    int posi_in_having;
    int posi_in_fields;
    int idx_of_count;
} t_havinginfo;

static int copy_ptr_to_value(T_VALUEDESC * val, char *ptr,
        SYSFIELD_T * fieldinfo);
double convert_numeric_val_to_double(T_VALUEDESC * value);

extern int set_rec_values_info_select(int handle, T_SELECT * select);

extern int create_subquery_table(int *, T_SELECT *, char *);

/*****************************************************************************/
//! function_name

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name :
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
set_field_as_value(char *rec_buf, SYSFIELD_T * fieldinfo,
        int nullflag_offset, T_POSTFIXELEM * value)
{
    char *target;
    int ret = SQL_E_NOERROR;

    target = rec_buf + fieldinfo->offset;

    if (value->u.value.is_null)
    {
        DB_VALUE_SETNULL(rec_buf, fieldinfo->position, nullflag_offset);
    }
    else
    {
        ret = copy_value_into_record(-1, target, &value->u.value, fieldinfo,
                0);
        if (ret != RET_SUCCESS)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTYPE, 0);
            return SQL_E_INVALIDTYPE;
        }

        DB_VALUE_SETNOTNULL(rec_buf, fieldinfo->position, nullflag_offset);
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! process_elem_value

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param elem()    :
 * \param selelem() :
 * \param rec_buf() :
 * \param rec_buf_len() :
 * \param nullflag_offset() :
 * \param fieldinfo()   :
 * \param fpos():
 * \param j:
 ************************************
 * \return  return_value
 ************************************
 * \note 집계 함수와 GROUP BY 칼럼을 임시 테이블에서 읽어 \n
 *        SELECT절 표현식에 바인딩하고 SELECT절 표현식을 계산한다.
 *  - BYTE/VARBYTE TYPE 지원하기 위해서...
 *  - collation도 설정함
 *****************************************************************************/
static MDB_INT32
process_elem_value(T_POSTFIXELEM * elem, T_POSTFIXELEM * selelem,
        char *rec_buf, int nullflag_offset,
        SYSFIELD_T * fieldinfo, int *fpos, int *j)
{
    if (elem->elemtype == SPT_VALUE)
    {
        if (elem->u.value.valueclass == SVC_VARIABLE)
        {
            T_ATTRDESC *attr = &elem->u.value.attrdesc;

            if (DB_VALUE_ISNULL(rec_buf, attr->posi.ordinal, nullflag_offset))
                selelem->u.value.is_null = 1;
            else
            {
                selelem->u.value.is_null = 0;
                convert_attr2value(attr, &selelem->u.value, rec_buf);
            }

            selelem->elemtype = elem->elemtype;
            selelem->u.value.valueclass = elem->u.value.valueclass;
        }
        else    /* elem->u.value.valueclass == SVC_CONSTANT */
            sc_memcpy(selelem, elem, sizeof(T_POSTFIXELEM));
    }
    else if (elem->elemtype == SPT_AGGREGATION)
    {
        if (elem->u.aggr.type == SAT_AVG)
        {
            double dividend;
            int divisor;
            char *target;

            selelem->u.value.valuetype = DT_DOUBLE;
            divisor =
                    *(int *) (rec_buf +
                    fieldinfo[elem->u.aggr.cnt_idx].offset);

            if (DB_VALUE_ISNULL(rec_buf, fieldinfo[*fpos].position,
                            nullflag_offset) || divisor == 0)
            {
                selelem->u.value.is_null = 1;
            }
            else
            {
                selelem->u.value.is_null = 0;
                target = rec_buf + fieldinfo[*fpos].offset;
                switch (fieldinfo[*fpos].type)
                {
                case DT_TINYINT:
                    dividend = *(tinyint *) target;
                    break;
                case DT_SMALLINT:
                    dividend = *(smallint *) target;
                    break;
                case DT_INTEGER:
                    dividend = *(int *) target;
                    break;
                case DT_BIGINT:
                    dividend = (double) (*(bigint *) target);
                    break;
                case DT_FLOAT:
                    dividend = *(float *) target;
                    break;
                case DT_DOUBLE:
                    dividend = *(double *) target;
                    break;
                case DT_DECIMAL:
                    char2decimal(&dividend, target, sc_strlen(target));
                    break;
                default:
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTYPE,
                            0);
                    return RET_ERROR;
                }
                selelem->u.value.u.f32 = dividend / divisor;
            }
        }
        else
        {   /* SAT_COUNT, SAT_MIN, SAT_MAX, SAT_SUM */

            selelem->u.value.attrdesc.len = fieldinfo[*fpos].length;
            selelem->u.value.valuetype = DT_NULL_TYPE;
            if (DB_VALUE_ISNULL(rec_buf, fieldinfo[*fpos].position,
                            nullflag_offset))
            {
                selelem->u.value.is_null = 1;
            }
            else
            {
                selelem->u.value.is_null = 0;
                selelem->u.value.attrdesc.posi.offset =
                        fieldinfo[*fpos].offset;
                selelem->u.value.attrdesc.type = fieldinfo[*fpos].type;
                selelem->u.value.attrdesc.len = fieldinfo[*fpos].length;
                selelem->u.value.attrdesc.collation =
                        fieldinfo[*fpos].collation;
                convert_attr2value(&selelem->u.value.attrdesc,
                        &selelem->u.value, rec_buf);
            }
            if (IS_ALLOCATED_TYPE(selelem->u.value.valuetype))
                selelem->u.value.call_free = 1;
            else
                selelem->u.value.call_free = 0;
        }

        (*j) += elem->u.aggr.significant;
        (*fpos)--;
    }
    else
    {
        sc_memcpy(selelem, elem, sizeof(T_POSTFIXELEM));
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! make_result_for_aggr

/*! \breif  집계 함수의 결과 값을 계산
 ************************************
 * \param handle()          :
 * \param stmt()            :
 * \param wTable()          :
 * \param rec_buf()         :
 * \param rec_buf_len()     :
 * \param nullflag_offset() :
 * \param having_info()     :
 * \param having_info_cnt() :
 * \param fieldinfo()       :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static int
make_result_for_aggr(int handle, T_SELECT * select, char *wTable,
        char *rec_buf, int rec_buf_len, int nullflag_offset,
        SYSFIELD_T * fieldinfo)
{
    T_LIST_SELECTION *selection;
    T_GROUPBYDESC *groupby;
    T_EXPRESSIONDESC *having;
    T_EXPRESSIONDESC *expr, selexpr;
    T_POSTFIXELEM *elem, *selelem;
    T_POSTFIXELEM result_org;
    T_POSTFIXELEM *result;
    T_VALUEDESC value;
    T_ATTRDESC *attr;
    int divisor;
    int fpos, selpos;
    int ref, Htable;
    int n_hv_avg;
    int n_aggr;
    int next_fpos;
    int i, j, k;
    int ret = RET_SUCCESS;

    int max_expr = -1;

    result = &result_org;

    sc_memset(&selexpr, 0, sizeof(selexpr));

    selection = &select->planquerydesc.querydesc.selection;
    groupby = &select->planquerydesc.querydesc.groupby;
    having = &select->planquerydesc.querydesc.groupby.having;

    Htable = dbi_Cursor_Open(handle, wTable, NULL, NULL, NULL, NULL,
            LK_EXCLUSIVE, 0);
    if (Htable < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, Htable, 0);
        return RET_ERROR;
    }

    for (i = 0; i < selection->len; ++i)
    {
        expr = &(selection->list[i].expr);
        if (max_expr < expr->len)
            max_expr = expr->len;
    }
    selexpr.list = sql_mem_get_free_postfixelem_list(NULL, max_expr);
    if (selexpr.list == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto RETURN_LABEL;
    }
    elem = PMEM_ALLOC(sizeof(T_POSTFIXELEM) * max_expr);
    if (!elem)
    {
        sql_mem_set_free_postfixelem_list(NULL, selexpr.list);
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto RETURN_LABEL;
    }

    for (i = 0; i < max_expr; ++i)
    {
        selexpr.list[i] = &(elem[i]);
    }
    selexpr.max = max_expr;

    sc_memset(result, 0, sizeof(T_POSTFIXELEM));
    result->elemtype = SPT_VALUE;
    result->u.value.valueclass = SVC_CONSTANT;

    /* reset schema information for each selection column */
    for (i = 0; i < selection->len; i++)
    {
        expr = &selection->list[i].expr;

        for (j = 0; j < expr->len; j++)
        {
            elem = expr->list[(expr->len - 1) - j];
            if (elem->elemtype == SPT_AGGREGATION)
                j += elem->u.aggr.significant;
            else if (elem->elemtype == SPT_VALUE &&
                    elem->u.value.valueclass == SVC_VARIABLE)
            {
                /* find the same column in group by clause */
                attr = &elem->u.value.attrdesc;
                for (k = 0; k < groupby->len; k++)
                {
                    if (mdb_strcmp(attr->attrname,
                                    groupby->list[k].attr.attrname))
                        continue;
                    if (mdb_strcmp(attr->table.tablename,
                                    groupby->list[k].attr.table.tablename))
                        continue;

                    if (attr->table.aliasname != NULL &&
                            groupby->list[k].attr.table.aliasname != NULL &&
                            mdb_strcmp(attr->table.aliasname,
                                    groupby->list[k].attr.table.aliasname))
                    {
                        continue;
                    }
                    else
                    {
                        break;
                    }

                }

                if (k >= groupby->len)
                {
                    /* GROUPBY_ANY */
                    continue;
                }
                else
                {
                    ref = groupby->list[k].i_ref;
                    attr->posi.ordinal = fieldinfo[ref].position;
                    attr->posi.offset = fieldinfo[ref].offset;
                    attr->type = fieldinfo[ref].type;
                    attr->len = fieldinfo[ref].length;
                }
            }
        }
    }

    n_hv_avg = select->planquerydesc.querydesc.aggr_info.n_hv_avg;

    // sexpr의 list는 n_hv_avg 만큼 할당?
    // TEMP TABLE의 RECORD를 읽어오는 부분...

    /* ret : SUCCESS, FAIL, DB_E_NOMORERECORDS, DB_E_TERMINATED */
    while (1)
    {
        ret = dbi_Cursor_Read(handle, Htable, rec_buf, &rec_buf_len,
                NULL, 0, 0);
        if (ret != DB_SUCCESS)
        {
            break;
        }

        fpos = selection->len - 1;

        /* SELECT절에서 GROUP BY 칼럼과 집계 함수를 바인딩하여
           표현식을 계산한 결과를 레코드에 설정한다. */
        for (i = 0; i < selection->len; i++)
        {
            expr = &selection->list[i].expr;

            /* expr이 표현식이면 바인딩후 계산하고 그렇지 않으면 스킵한다.
               그렇다면 표현식이 아닌 경우, 언제 그 값을 임시 테이블의
               레코드에 설정하나? */
            if (expr->len == 1 &&
                    expr->list[0]->elemtype == SPT_VALUE &&
                    expr->list[0]->u.value.valueclass == SVC_VARIABLE)
                continue;

            n_aggr = 0;
            for (j = 0, selpos = expr->len - 1; j < expr->len; j++)
            {
                elem = expr->list[j];

                if (elem->elemtype == SPT_AGGREGATION)
                {
                    selpos -= elem->u.aggr.significant;
                    n_aggr++;
                }
            }

            if (!n_aggr)
                continue;

            selexpr.len = selpos + 1;
            fpos += n_aggr;
            next_fpos = fpos;
            for (j = 0; j < expr->len; j++, selpos--)
            {
                elem = expr->list[(expr->len - 1) - j];
                selelem = selexpr.list[selpos];

                ret = process_elem_value(elem, selelem, rec_buf,
                        nullflag_offset, fieldinfo, &fpos, &j);

                if (ret == RET_ERROR)
                    goto RETURN_LABEL;
            }
            fpos = next_fpos;
            ret = calc_expression(&selexpr, &result->u.value, MDB_FALSE);
            if (ret < 0)
                goto RETURN_LABEL;

            if (set_field_as_value(rec_buf, &fieldinfo[i], nullflag_offset,
                            result) != RET_SUCCESS)
                goto RETURN_LABEL;

            sql_value_ptrFree(&result->u.value);

            for (j = 0; j < selexpr.len; j++)
            {
                if (selexpr.list[j]->elemtype == SPT_NONE ||
                        (selexpr.list[j]->elemtype == SPT_VALUE &&
                                selexpr.list[j]->u.value.valueclass ==
                                SVC_VARIABLE))
                {
                    sql_value_ptrFree(&selexpr.list[j]->u.value);
                }
            }
            for (j = 0; j < selexpr.len; j++)
                sc_memset(selexpr.list[j], 0, sizeof(T_POSTFIXELEM));
        }

        /* HAVING 절에서 GROUP BY 칼럼과 집계 함수를 바인딩하여
           표현식을 계산한 결과를 레코드에 설정한다. */
        if (n_hv_avg > 0)
        {
            int hv_cnt = n_hv_avg;
            T_QUERYDESC *qdesc = &select->planquerydesc.querydesc;

            for (i = 0; i < qdesc->having.len; i++)
            {

                elem = having->list[qdesc->having.hvinfo[i].posi_in_having];

                if (elem->elemtype == SPT_AGGREGATION &&
                        elem->u.aggr.type == SAT_AVG)
                {
                    ref = qdesc->having.hvinfo[i].posi_in_fields;
                    divisor = *(int *) (rec_buf +
                            fieldinfo[qdesc->having.hvinfo[i].idx_of_count].
                            offset);
                    if (divisor)
                    {
                        if (DB_VALUE_ISNULL(rec_buf, fieldinfo[ref].position,
                                        nullflag_offset))
                        {
                            DB_VALUE_SETNULL(rec_buf, fieldinfo[ref].position,
                                    nullflag_offset);
                        }
                        else
                        {
                            *(double *) (rec_buf + fieldinfo[ref].offset) /=
                                    divisor;
                            DB_VALUE_SETNOTNULL(rec_buf,
                                    fieldinfo[ref].position, nullflag_offset);
                        }

                    }
                    else
                    {   /* NULL로 설정 */
                        set_maxvalue(fieldinfo[ref].type,
                                fieldinfo[ref].length, &value);
                        *(double *) (rec_buf + fieldinfo[ref].offset) =
                                value.u.f32;
                        DB_VALUE_SETNULL(rec_buf, fieldinfo[ref].position,
                                nullflag_offset);

                    }
                    if ((hv_cnt--) <= 0)
                        break;
                }
            }   /* for */
        }   /* if n_hv_arg */

        ret = dbi_Cursor_Update(handle, Htable, rec_buf, rec_buf_len, NULL);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            break;
        }
    }

  RETURN_LABEL:
    if (selexpr.list)
    {
        sql_mem_set_free_postfixelem(NULL, selexpr.list[0]);
        sql_mem_set_free_postfixelem_list(NULL, selexpr.list);
    }

    dbi_Cursor_Close(handle, Htable);

    return ret;
}

/*****************************************************************************/
//! function_name

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name :
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
set_distinct_table_info(int handle,
        int index, char *table, T_DISTINCTINFO * distinct_table)
{
    SYSTABLE_T tableinfo;

    distinct_table[index].Htable =
            dbi_Cursor_Open(handle, table, NULL, NULL, NULL, NULL,
            LK_EXCLUSIVE, 0);

    if (distinct_table[index].Htable < 0)
        return RET_ERROR;

    if (dbi_Schema_GetTableInfo(handle, table, &tableinfo) < 0)
        goto process_error;

    distinct_table[index].rec_buf_len
            = get_recordlen(handle, table, &tableinfo);
    if (distinct_table[index].rec_buf_len < 0)
        goto process_error;

    distinct_table[index].rec_buf
            = PMEM_ALLOC(distinct_table[index].rec_buf_len);
    if (!distinct_table[index].rec_buf)
        goto process_error;

    distinct_table[index].nullflag_offset
            = get_nullflag_offset(handle, table, &tableinfo);
    if (distinct_table[index].nullflag_offset < 0)
        goto process_error;

    distinct_table[index].fieldnum = dbi_Schema_GetFieldsInfo(handle,
            table, &distinct_table[index].fieldinfo);
    if (distinct_table[index].fieldnum < 0)
        goto process_error;

    return RET_SUCCESS;

  process_error:
    PMEM_FREENUL(distinct_table[index].rec_buf);
    PMEM_FREENUL(distinct_table[index].fieldinfo);
    dbi_Cursor_Close(handle, distinct_table[index].Htable);
    distinct_table[index].Htable = -1;
    return RET_ERROR;
}

/*****************************************************************************/
//! get_value_len_n_scale

/*! \breif  temp table or temp index를 생성하기 위해서 특정 필드의 정보를 얻음
 ************************************
 * \param list(in)          : resultset
 * \param orderby(in)       : orderby
 * \param groupby(in)       : groupby
 * \param value_len(in)     : field's length(정의)
 * \param value_scale(in)   : decimal's scale
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - BYTE/VARBYTE 추가
 *  - 함수 추가
 *      get_value_len_n_scale를 대처하기 위해 함수 생성
 *****************************************************************************/
static MDB_INT32
get_fields_info_for_temp(T_RESULTCOLUMN * list, T_ORDERBYITEM * orderby,
        T_GROUPBYITEM * groupby, MDB_INT32 * value_len,
        MDB_INT32 * value_scale, MDB_COL_TYPE * collation,
        MDB_INT32 * value_fixedlen)
{
    MDB_INT32 length, scale, fixedlen;
    DataType type;

    if (list)
    {
        if (orderby || groupby)
            return RET_ERROR;

        type = list->value.valuetype;
        length = list->value.attrdesc.len;
        scale = list->value.attrdesc.dec;
        fixedlen = list->value.attrdesc.fixedlen;
        *collation = list->value.attrdesc.collation;
    }
    else if (orderby)
    {
        if (groupby)
            return RET_ERROR;

        type = orderby->attr.type;
        length = orderby->attr.len;
        scale = orderby->attr.dec;
        fixedlen = orderby->attr.fixedlen;
        *collation = orderby->attr.collation;
    }
    else if (groupby)
    {
        type = groupby->attr.type;
        length = groupby->attr.len;
        scale = groupby->attr.dec;
        fixedlen = groupby->attr.fixedlen;
        *collation = groupby->attr.collation;
    }
    else
        return RET_ERROR;

    switch (type)
    {
    case DT_TINYINT:
    case DT_SMALLINT:
    case DT_INTEGER:
    case DT_BIGINT:
    case DT_FLOAT:
    case DT_DOUBLE:
    case DT_OID:
        break;
    case DT_DECIMAL:
        *value_scale = scale;
    case DT_CHAR:
    case DT_NCHAR:
    case DT_VARCHAR:
    case DT_NVARCHAR:
        *value_fixedlen = fixedlen;
    case DT_DATETIME:
    case DT_DATE:
    case DT_TIME:
    case DT_TIMESTAMP:
    case DT_BYTE:
    case DT_VARBYTE:
        *value_len = length;
        break;
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
        return RET_ERROR;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! init_distinct_table

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)    :
 * \param stmt()      :
 * \param having_info()   :
 * \param having_info_cnt() :
 * \param distinct_table() :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - called : make_aggregative_query()
 *  - T_EXPRESSIONDESC 구조체 변경
 *  - A. COLLATION 지원에 따른 dbi_FieldDesc 인터페이스 변경
 *      B. get_value_len_n_scale()을 get_fields_info_for_temp()로 대처
 *
 *****************************************************************************/
static int
set_distinct_table(int *handle, T_SELECT * select)
{
    T_PLANNEDQUERYDESC *plan;
    T_LIST_SELECTION *selection;

    T_EXPRESSIONDESC *expr;
    T_POSTFIXELEM *elem;

    FIELDDESC_T *fielddesc = NULL, *dist_posi;
    T_DISTINCTINFO *distinct_table;
    T_HAVINGINFO *having_info;

    char col_name[FIELD_NAME_LENG];

    int distinct_num = 0;
    int i, j, ret = RET_ERROR;
    MDB_INT32 value_len, value_scale, value_fixedlen;
    MDB_COL_TYPE collation;

    plan = &select->planquerydesc;
    selection = &plan->querydesc.selection;
    distinct_table = plan->querydesc.distinct.distinct_table;

    fielddesc =
            PMEM_ALLOC(sizeof(FIELDDESC_T) * (plan->querydesc.groupby.len +
                    1));

    for (i = 0; i < plan->querydesc.groupby.len; i++)
    {
        sc_snprintf(col_name, FIELD_NAME_LENG,
                "%s_distinct_%d", INTERNAL_FIELD_PREFIX, i);

        value_len = value_scale = value_fixedlen = 0;

        ret = get_fields_info_for_temp(NULL, NULL,
                &plan->querydesc.groupby.list[i], &value_len,
                &value_scale, &collation, &value_fixedlen);

        if (ret == RET_ERROR)
        {
            goto process_error;
        }

        dbi_FieldDesc(col_name, plan->querydesc.groupby.list[i].attr.type,
                value_len, value_scale, NULL_BIT, 0, value_fixedlen,
                &(fielddesc[i]), collation);
    }

    dist_posi = &fielddesc[i];

    sc_snprintf(col_name, FIELD_NAME_LENG, "%s_distinct_%d",
            INTERNAL_FIELD_PREFIX, i);

    for (i = 0; i < selection->len; i++)
    {
        if (!selection->list[i].is_distinct)
            continue;

        expr = &selection->list[i].expr;

        for (j = 0; j < expr->len; j++)
        {
            elem = expr->list[j];

            if (elem->elemtype == SPT_AGGREGATION && elem->is_distinct)
            {
                dbi_FieldDesc(col_name, elem->u.aggr.valtype,
                        elem->u.aggr.length, (MDB_INT8) elem->u.aggr.decimals,
                        NULL_BIT, 0, FIXED_VARIABLE, dist_posi,
                        elem->u.aggr.collation);
            }
            else
                continue;

            ret = dbi_Relation_Create(*handle,
                    distinct_table[distinct_num].table,
                    plan->querydesc.groupby.len + 1, fielddesc,
                    _CONT_TEMPTABLE, _USER_USER, 0, NULL);
            if (ret < 0)
            {
                if (ret == DB_E_DUPTABLENAME)
                {
                    Truncate_Table(distinct_table[distinct_num].table);
                }
                else
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    goto process_error;
                }
            }

            /* distict using unique index */
            if (plan->querydesc.groupby.len + 1 < MAX_INDEX_FIELD_NUM
                    && ret != DB_E_DUPTABLENAME)
            {
                ret = dbi_Index_Create(*handle,
                        distinct_table[distinct_num].table,
                        distinct_table[distinct_num].index,
                        plan->querydesc.groupby.len + 1, fielddesc,
                        UNIQUE_INDEX_BIT);
                if (ret < 0)
                {
                    if (ret != DB_E_INDEX_HAS_BYTE_OR_VARBYTE)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_ERROR, 0);
                        goto process_error;
                    }
                }
            }

            if (set_distinct_table_info(*handle,
                            distinct_num, distinct_table[distinct_num].table,
                            distinct_table) == RET_ERROR)
            {
                int k;

                for (k = 0; k <= distinct_num; k++)
                    dbi_Relation_Drop(*handle, distinct_table[k].table);

                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
                goto process_error;
            }
            ++distinct_num;
        }
    }

    having_info = plan->querydesc.having.hvinfo;

    if (plan->querydesc.having.len > 0)
    {
        expr = &plan->querydesc.groupby.having;

        for (i = 0; i < plan->querydesc.having.len; i++)
        {
            elem = expr->list[having_info[i].posi_in_having];

            if (elem->elemtype != SPT_AGGREGATION || !elem->is_distinct)
                continue;

            dbi_FieldDesc(col_name, elem->u.aggr.valtype, elem->u.aggr.length,
                    (MDB_INT8) elem->u.aggr.decimals, NULL_BIT, 0,
                    FIXED_VARIABLE, dist_posi, elem->u.aggr.collation);

            if (distinct_num >= MAX_DISTINCT_TABLE_NUM)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_TOOMANYDISTINCT,
                        0);
                goto process_error;
            }

            distinct_table[distinct_num].column_index = MDB_INT_MAX;    // means having item
            distinct_table[distinct_num].field_index = i;       // having_info index

            ret = dbi_Relation_Create(*handle,
                    distinct_table[distinct_num].table,
                    plan->querydesc.groupby.len + 1, fielddesc,
                    _CONT_TEMPTABLE, _USER_USER, 0, NULL);
            if (ret < 0)
            {
                if (ret == DB_E_DUPTABLENAME)
                {
                    Truncate_Table(distinct_table[distinct_num].table);
                }
                else
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    goto process_error;
                }
            }


            if (set_distinct_table_info(*handle, distinct_num,
                            distinct_table[distinct_num].table,
                            distinct_table) == RET_ERROR)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
                goto process_error;
            }

            ++distinct_num;
        }   /* for */

    }       /* if (having_info) */

    PMEM_FREENUL(fielddesc);
    return RET_SUCCESS;

  process_error:

    PMEM_FREENUL(fielddesc);
    for (i = 0; i <= distinct_num; i++)
        dbi_Relation_Drop(*handle, distinct_table[i].table);

    return RET_ERROR;
}

/*****************************************************************************/
//! insert_into_distinct_table

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)          :
 * \param stmt()            :
 * \param having_info()     :
 * \param having_info_cnt() :
 * \param col_index()       :
 * \param f_index()         :
 * \param distinct_table()  :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static int
insert_into_distinct_table(int handle, T_SELECT * select,
        int col_index, int f_index)
{
    T_QUERYDESC *qdesc;
    T_RTTABLE *rttable;

    T_EXPRESSIONDESC *expr, texpr;
    T_POSTFIXELEM *elem;
    T_VALUEDESC tmpval;
    T_DISTINCTINFO *distinct_table;

    SYSFIELD_T *fieldinfo = NULL;

    char *target, *rec_buf = NULL;
    int nullflag_offset = 0, rec_buf_len = 0;
    int idx, findex, fixed_size = 0;
    int i, j, k;
    int ret = RET_SUCCESS;

    int max_expr = 0;

    qdesc = &select->planquerydesc.querydesc;
    rttable = select->rttable;

    if (col_index < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
        return -1;
    }

    distinct_table = qdesc->distinct.distinct_table;

    for (idx = 0; idx < MAX_DISTINCT_TABLE_NUM; idx++)
    {
        if (distinct_table[idx].Htable < 0)
            continue;

        if (distinct_table[idx].column_index == col_index &&
                distinct_table[idx].field_index == f_index)
            break;
    }

    if (idx == MAX_DISTINCT_TABLE_NUM)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
        goto ERROR_LABEL;
    }

    // selection item의 distinct_table record의 field index
    findex = qdesc->groupby.len;

    rec_buf = distinct_table[idx].rec_buf;
    rec_buf_len = distinct_table[idx].rec_buf_len;
    nullflag_offset = distinct_table[idx].nullflag_offset;
    fieldinfo = distinct_table[idx].fieldinfo;
    sc_memset(rec_buf, 0, rec_buf_len);

    for (i = 0; i < qdesc->groupby.len; i++)
    {
        for (j = 0; j < qdesc->fromlist.len; j++)
            if (!mdb_strcmp(rttable[j].table.tablename,
                            qdesc->groupby.list[i].attr.table.tablename))
                break;

        if (j == qdesc->fromlist.len)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
            goto ERROR_LABEL;
        }

        target = rec_buf + fieldinfo[i].offset;

        convert_rec_attr2value(&qdesc->groupby.list[i].attr,
                rttable[j].rec_values,
                qdesc->groupby.list[i].attr.posi.ordinal, &tmpval);

        if (tmpval.is_null)
        {
            DB_VALUE_SETNULL(rec_buf, fieldinfo[i].position, nullflag_offset);
        }
        else
        {
            ret = copy_value_into_record(-1, target, &tmpval, &fieldinfo[i],
                    0);
            DB_VALUE_SETNOTNULL(rec_buf, fieldinfo[i].position,
                    nullflag_offset);
        }

        if (ret != RET_SUCCESS)
        {
            goto ERROR_LABEL;
        }

        fixed_size += fieldinfo[i].length;
    }

    target = rec_buf + fieldinfo[findex].offset;

    // TEMP
    // max_expr 계산
    if (col_index < MDB_INT_MAX)
    {       // if selection item
        i = distinct_table[idx].column_index;

        expr = &qdesc->selection.list[i].expr;
        for (j = 0; j < expr->len; ++j)
        {
            elem = expr->list[j];
            if (elem->elemtype != SPT_AGGREGATION ||
                    !elem->is_distinct || f_index-- > 0)
                continue;

            if (max_expr < elem->u.aggr.significant)
                max_expr = elem->u.aggr.significant;
        }
    }
    else
    {       // if having item
        expr = &qdesc->groupby.having;
        elem = expr->list[qdesc->having.hvinfo[f_index].posi_in_having];

        if (max_expr < elem->u.aggr.significant)
            max_expr = elem->u.aggr.significant;
    }

    if (max_expr > 0)
    {
        texpr.list = sql_mem_get_free_postfixelem_list(NULL, max_expr);
        if (texpr.list == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return -1;
        }

        elem = PMEM_ALLOC(sizeof(T_POSTFIXELEM) * max_expr);
        if (elem == NULL)
        {
            sql_mem_set_free_postfixelem_list(NULL, texpr.list);
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return -1;
        }

        for (i = 0; i < max_expr; ++i)
            texpr.list[i] = &(elem[i]);
        texpr.len = 0;
        texpr.max = max_expr;
    }
    else
    {
        sc_assert(0, __FILE__, __LINE__);
    }

    if (col_index < MDB_INT_MAX)
    {       // if selection item
        i = distinct_table[idx].column_index;

        expr = &qdesc->selection.list[i].expr;

        for (j = 0; j < expr->len; j++)
        {
            elem = expr->list[j];

            if (elem->elemtype != SPT_AGGREGATION || !elem->is_distinct ||
                    f_index-- > 0)
                continue;

            for (k = 0; k < elem->u.aggr.significant; ++k)
                sc_memcpy(texpr.list[k],
                        expr->list[j - elem->u.aggr.significant + k],
                        sizeof(T_POSTFIXELEM));

            texpr.len = elem->u.aggr.significant;

            ret = calc_expression(&texpr, &tmpval, MDB_FALSE);
            if (ret < 0)
            {
                goto ERROR_LABEL;
            }

            ret = RET_SUCCESS;

            if (tmpval.is_null)
            {
                DB_VALUE_SETNULL(rec_buf, fieldinfo[findex].position,
                        nullflag_offset);
            }
            else
            {
                ret = copy_value_into_record(-1, target, &tmpval,
                        &fieldinfo[findex], 0);
                if (ret != RET_SUCCESS)
                {
                    goto ERROR_LABEL;
                }
                DB_VALUE_SETNOTNULL(rec_buf, fieldinfo[findex].position,
                        nullflag_offset);
            }
            sql_value_ptrFree(&tmpval);
        }
    }
    else
    {       // if having item
        expr = &qdesc->groupby.having;

        elem = expr->list[qdesc->having.hvinfo[f_index].posi_in_having];

        for (k = 0; k < elem->u.aggr.significant; ++k)
            sc_memcpy(texpr.list[k],
                    expr->list[qdesc->having.hvinfo[f_index].posi_in_having -
                            elem->u.aggr.significant + k],
                    sizeof(T_POSTFIXELEM));

        texpr.len = elem->u.aggr.significant;

        ret = calc_expression(&texpr, &tmpval, MDB_FALSE);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
            goto ERROR_LABEL;
        }

        ret = RET_SUCCESS;

        if (tmpval.is_null)
        {
            DB_VALUE_SETNULL(rec_buf, fieldinfo[findex].position,
                    nullflag_offset);
        }
        else
        {
            ret = copy_value_into_record(-1, target, &tmpval,
                    &fieldinfo[findex], 0);
            if (ret != RET_SUCCESS)
                goto ERROR_LABEL;

            DB_VALUE_SETNOTNULL(rec_buf, fieldinfo[findex].position,
                    nullflag_offset);
        }

        sql_value_ptrFree(&tmpval);
    }

    sql_mem_set_free_postfixelem(NULL, texpr.list[0]);
    sql_mem_set_free_postfixelem_list(NULL, texpr.list);

    ret = dbi_Cursor_Distinct_Insert(handle, distinct_table[idx].Htable,
            rec_buf, rec_buf_len);
    if (ret == DB_E_DUPDISTRECORD)
    {
        return 0;
    }
    else if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        return -1;
    }

    return 1;

  ERROR_LABEL:
    sql_mem_set_free_postfixelem(NULL, texpr.list[0]);
    sql_mem_set_free_postfixelem_list(NULL, texpr.list);
    return -1;

}

/*****************************************************************************/
//! find_rttable_idx

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param nfromlist(in) :
 * \param rttable(in)   :
 * \param tablename(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static MDB_INT32
find_rttable_idx(MDB_INT32 nfromlist, T_RTTABLE * rttable, char *tablename)
{
    MDB_INT32 j;

    for (j = 0; j < nfromlist; j++)
    {
        if (rttable[j].table.aliasname)
        {
            if (!mdb_strcmp(rttable[j].table.aliasname, tablename))
                break;
        }
        else if (!mdb_strcmp(rttable[j].table.tablename, tablename))
            break;
    }

    if (j == nfromlist)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
        return RET_ERROR;
    }

    return j;
}

/*****************************************************************************/
//! make_fieldval_from_fieldinfo

/*! \breif  FIELD'S VALUE를 설정한다
 ************************************
 * \param fieldvalue(in/out)    : field's value
 * \param fieldinfo(in)         : field's info
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *  - COLLATION INTERFACE 지원
 *****************************************************************************/
static FIELDVALUE_T *
make_fieldval_from_fieldinfo(FIELDVALUE_T * fieldvalue, SYSFIELD_T * fieldinfo)
{
    int ret = -1;

    ret = dbi_FieldValue(MDB_NU, fieldinfo->type, fieldinfo->position,
            fieldinfo->offset, fieldinfo->length, fieldinfo->scale,
            fieldinfo->collation, 0, fieldinfo->type, fieldinfo->length,
            fieldvalue);
    if (ret != DB_SUCCESS)
        return NULL;

    return fieldvalue;
}

/*****************************************************************************/
//! insert_aggr_val_to_tmp_rcrd

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param expr_list()       : 
 * \param expr()            :
 * \param elem()            :
 * \param rec_buf(in/out)   : memory record
 * \param nullflag_offset() :
 * \param fieldinfo()       :
 * \param ref()             :
 * \param is_updated()      :
 * \param for_insert()      :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *  - BYTE/VARBYTE 지원
 *
 *****************************************************************************/
static MDB_INT32
insert_aggr_val_to_tmp_rcrd(T_EXPRESSIONDESC * expr,
        T_POSTFIXELEM * elem, char *rec_buf, int nullflag_offset,
        SYSFIELD_T * fieldinfo, MDB_INT32 ref,
        MDB_BOOL * is_updated, MDB_BOOL for_insert)
{
    T_RESULTCOLUMN res;
    T_VALUEDESC current_val, prev_val;
    char *p = NULL;
    char *cnt_ptr = NULL;

    sc_memset(&prev_val, 0, sizeof(T_VALUEDESC));
    sc_memset(&res, 0, sizeof(T_RESULTCOLUMN));

    if (calc_expression(expr, &current_val, MDB_FALSE) < 0)
    {
        return RET_ERROR;
    }

    res.value.valuetype = current_val.valuetype;
    prev_val.valuetype = current_val.valuetype;

    if (IS_ALLOCATED_TYPE(res.value.valuetype))
        res.len = current_val.attrdesc.len;

    p = rec_buf + fieldinfo[ref].offset;

    if (elem->u.aggr.type == SAT_COUNT)
    {
        if (for_insert)
        {
            if (!current_val.is_null)
                *(int *) p = 1;
            else
                *(int *) p = 0;
        }
        else    /* for_update */
        {
            if (!current_val.is_null)
            {
                *is_updated = MDB_TRUE;
                ++(*(int *) p);
            }
        }
    }

    if (current_val.is_null)    /* current_vall is a null value */
    {
        if (for_insert && elem->u.aggr.type == SAT_AVG)
        {
            cnt_ptr = rec_buf + fieldinfo[elem->u.aggr.cnt_idx].offset;
            (*(int *) cnt_ptr) = 0;
        }
        if (for_insert && (elem->u.aggr.type == SAT_SUM ||
                        elem->u.aggr.type == SAT_MIN ||
                        elem->u.aggr.type == SAT_MAX))
        {
            DB_VALUE_SETNULL(rec_buf, fieldinfo[ref].position,
                    nullflag_offset);
        }
    }
    else    /* current_vall is a non-null value */
    {
        if (!for_insert
                && DB_VALUE_ISNULL(rec_buf, fieldinfo[ref].position,
                        nullflag_offset))
        {
            DB_VALUE_SETNOTNULL(rec_buf, fieldinfo[ref].position,
                    nullflag_offset);

            switch (elem->u.aggr.type)
            {
            case SAT_MIN:
            case SAT_MAX:
                copy_value_into_record(-1, p, &current_val,
                        &fieldinfo[ref], 0);
                *is_updated = MDB_TRUE;
                break;
            case SAT_SUM:
            case SAT_AVG:
                {
                    T_VALUEDESC temp_val;

                    temp_val.is_null = 0;
                    temp_val.valuetype = DT_DOUBLE;
                    GET_FLOAT_VALUE(temp_val.u.f32, double, &current_val);

                    copy_value_into_record(-1, p, &temp_val,
                            &fieldinfo[ref], 0);
                }
                *is_updated = MDB_TRUE;
                break;
            default:
                break;
            }
        }
        else    /* current value is not NULL value */
        {

            copy_ptr_to_value(&res.value, p, &fieldinfo[ref]);
            res.value.is_null = 0;

            switch (elem->u.aggr.type)
            {
            case SAT_MIN:
                if (for_insert || get_minvalue(&res, &current_val) > 0)
                {
                    copy_value_into_record(-1, p, &current_val,
                            &fieldinfo[ref], 0);
                    *is_updated = MDB_TRUE;
                }
                break;
            case SAT_MAX:
                if (for_insert || get_maxvalue(&res, &current_val) > 0)
                {
                    copy_value_into_record(-1, p, &current_val,
                            &fieldinfo[ref], 0);
                    *is_updated = MDB_TRUE;
                }
                break;
            case SAT_SUM:
                res.value.valuetype = fieldinfo[ref].type;
                prev_val.valuetype = fieldinfo[ref].type;

                GET_FLOAT_VALUE(res.value.u.f32, double, &current_val);

                res.value.is_null = 0;

                if (for_insert)
                {
                }
                else
                {
                    copy_ptr_to_value(&prev_val, p, &fieldinfo[ref]);
                    prev_val.is_null = 0;
                    get_sumvalue(&res, &prev_val);
                }

                copy_value_into_record(-1, p, &res.value, &fieldinfo[ref], 0);
                *is_updated = MDB_TRUE;
                break;
            case SAT_AVG:
                res.value.valuetype = fieldinfo[ref].type;
                prev_val.valuetype = fieldinfo[ref].type;

                GET_FLOAT_VALUE(res.value.u.f32, double, &current_val);

                res.value.is_null = 0;

                if (for_insert)
                {
                    cnt_ptr = rec_buf + fieldinfo[elem->u.aggr.cnt_idx].offset;
                    (*(int *) cnt_ptr) = 1;
                }
                else
                {
                    copy_ptr_to_value(&prev_val, p, &fieldinfo[ref]);
                    prev_val.is_null = 0;
                    get_sumvalue(&res, &prev_val);

                    cnt_ptr = rec_buf + fieldinfo[elem->u.aggr.cnt_idx].offset;
                    ++(*(int *) cnt_ptr);
                }

                copy_value_into_record(-1, p, &res.value, &fieldinfo[ref], 0);
                *is_updated = MDB_TRUE;
                break;
            default:
                break;
            }
        }
    }

    sql_value_ptrFree(&current_val);
    sql_value_ptrFree(&prev_val);
    sql_value_ptrFree(&res.value);

    return RET_SUCCESS;
}

/*****************************************************************************/
//! make_temp_record

/*! \breif  TEMP TABLE을 만들때, 이때 TEMP TABLE의 RECORD값을 할당
 ************************************
 * \param handle(in)     :
 * \param stmt(in)       :
 * \param rec_buf(in/out)   : memory record
 * \param nullflag_offset :
 * \param expr :
 * \param numfields :
 * \param fieldinfo :
 * \param distinct_table :
 * \param having_info :
 * \param having_info_cnt :
 * \param is_updated :
 * \param for_insert :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - called : GROUPBY용 TEMP TABLE에서만 사용됨
 *  - referenced : _Schema_CheckFieldDesc()
 *      heap record : DB Engine에서 저장하는 record format
 *      memory record : DB API에서 사용하는 record format
 *
 *  - TEMP TALBE'S record format
 * <selection items,aggr items,groupby items,having items,avg cnts,rid item>
 *
 *  - T_EXPRESSIONDESC 구조체 변경
 *  - BYTE/VARBYTE 추가
 *
 *****************************************************************************/
static MDB_INT32
make_temp_record(MDB_INT32 handle, T_SELECT * select,
        char *rec_buf, int nullflag_offset,
        T_EXPRESSIONDESC * expr,
        int numfields,
        SYSFIELD_T * fieldinfo, MDB_BOOL * is_updated, MDB_BOOL for_insert)
{
    T_EXPRESSIONDESC *pexpr = NULL;
    T_POSTFIXELEM *elem;
    MDB_INT32 is_validcolumn = 0;
    MDB_INT32 ref, start, findex;
    MDB_INT32 i, j, k;
    int ret;
    int is_aggr = 0;
    T_VALUEDESC value;

    T_QUERYDESC *query;

    query = &select->planquerydesc.querydesc;

    ref = query->selection.len;
    // 1. SELECTION-ITEMS FIELDS
    for (i = 0; i < query->selection.len; i++)
    {
        pexpr = &query->selection.list[i].expr;
        for (j = findex = 0; j < pexpr->len; j++)
        {
            elem = pexpr->list[j];
            if (elem->elemtype == SPT_AGGREGATION)
            {
                is_aggr = 1;
                if (elem->is_distinct)
                {
                    is_validcolumn = insert_into_distinct_table(handle,
                            select, i, findex++);
                    if (is_validcolumn < 0)
                        return RET_ERROR;

                    if (!is_validcolumn)
                    {
                        ref++;
                        continue;
                    }
                }

                expr->len = elem->u.aggr.significant;
                for (k = 0; k < elem->u.aggr.significant; ++k)
                    sc_memcpy(expr->list[k],
                            pexpr->list[j - elem->u.aggr.significant + k],
                            sizeof(T_POSTFIXELEM));
                if (insert_aggr_val_to_tmp_rcrd(expr, elem, rec_buf,
                                nullflag_offset, fieldinfo, ref, is_updated,
                                for_insert) == RET_ERROR)
                {
                    return RET_ERROR;
                }
                ref++;
            }
            else if (pexpr->len == 1 && elem->elemtype == SPT_VALUE &&
                    elem->u.value.valueclass == SVC_VARIABLE)
            {
                if (elem->u.value.is_null)
                    DB_VALUE_SETNULL(rec_buf, fieldinfo[i].position,
                            nullflag_offset);
                else
                    copy_value_into_record(-1, rec_buf + fieldinfo[i].offset,
                            &elem->u.value, &fieldinfo[i], 0);
            }
        }
        if (!is_aggr)
        {
            if (pexpr->len > 1)
            {
                value.call_free = 0;
                ret = calc_expression(pexpr, &value, MDB_FALSE);
                if (ret < 0)
                {
                    return RET_ERROR;
                }

                ret = copy_value_into_record(-1, rec_buf + fieldinfo[i].offset,
                        &value, &fieldinfo[i], 0);
                if (ret < 0)
                {
                    sql_value_ptrFree(&value);
                    return RET_ERROR;
                }

                sql_value_ptrFree(&value);
            }
        }
        is_aggr = 0;
    }       /* for(query->selection.len) */

    if (for_insert)
    {
        // group-by item에 대해서도 처리하자.
        for (i = 0; i < query->groupby.len; i++)
        {
            ref = query->groupby.list[i].i_ref;
            if (query->groupby.list[i].attr.table.aliasname)
                j = find_rttable_idx(query->fromlist.len, select->rttable,
                        query->groupby.list[i].attr.table.aliasname);
            else
                j = find_rttable_idx(query->fromlist.len, select->rttable,
                        query->groupby.list[i].attr.table.tablename);

            if (j == RET_ERROR)
                return RET_ERROR;

            if (1)
            {
                T_VALUEDESC val;

                if (query->groupby.list[i].s_ref > -1)
                {
                    pexpr = &query->selection.list[query->groupby.list[i].
                            s_ref].expr;
                    if (pexpr->len == 1
                            && pexpr->list[0]->elemtype == SPT_VALUE)
                    {
                        sc_memcpy(&val, &pexpr->list[0]->u.value,
                                sizeof(T_VALUEDESC));
                    }
                    else
                    {
                        if ((calc_expression(pexpr, &val, MDB_FALSE)) < 0)
                            return RET_ERROR;
                    }
                }
                else
                    convert_rec_attr2value(&query->groupby.list[i].attr,
                            select->rttable[j].rec_values,
                            query->groupby.list[i].attr.posi.ordinal, &val);

                if (val.is_null)
                {
                    DB_VALUE_SETNULL(rec_buf, fieldinfo[ref].position,
                            nullflag_offset);
                }
                else
                {
                    copy_value_into_record(-1, rec_buf + fieldinfo[ref].offset,
                            &val, &fieldinfo[ref], 0);
                    DB_VALUE_SETNOTNULL(rec_buf, fieldinfo[ref].position,
                            nullflag_offset);
                }
                if (query->groupby.list[i].s_ref > -1)
                    sql_value_ptrFree(&val);
            }
        }
    }       /* if (for_insert) */

    // HAVING item에 대해서도 처리하자.
    if (query->having.len > 0)
    {
        for (i = 0; i < query->having.len; i++)
        {
            elem = query->groupby.having.list[query->having.hvinfo[i].
                    posi_in_having];

            if (elem->elemtype == SPT_AGGREGATION)
            {
                if (!for_insert && elem->is_distinct)
                {
                    is_validcolumn = insert_into_distinct_table(handle,
                            select, MDB_INT_MAX, i);
                    if (is_validcolumn < 0)
                    {
                        return RET_ERROR;
                    }

                    if (!is_validcolumn)
                        continue;
                }

                ref = query->having.hvinfo[i].posi_in_fields;

                if (elem->u.aggr.type == SAT_COUNT
                        && !elem->u.aggr.significant)
                {
                    *is_updated = MDB_TRUE;
                    ++(*(int *) (rec_buf + fieldinfo[ref].offset));
                    continue;
                }
                else
                {
                    start = query->having.hvinfo[i].posi_in_having -
                            elem->u.aggr.significant;

                    expr->len = query->having.hvinfo[i].posi_in_having - start;
                    for (j = 0;
                            j < query->having.hvinfo[i].posi_in_having - start;
                            ++j)
                        sc_memcpy(expr->list[j],
                                query->groupby.having.list[start + j],
                                sizeof(T_POSTFIXELEM));

                    for (j = 0; j < query->fromlist.len; j++)
                        if (expr->len)
                            set_exprvalue(&select->rttable[j], expr, NULL_OID);

                    if (insert_aggr_val_to_tmp_rcrd(expr, elem, rec_buf,
                                    nullflag_offset, fieldinfo, ref,
                                    is_updated, for_insert) == RET_ERROR)
                    {
                        return RET_ERROR;
                    }
                }
            }   /* if (SPT_AGGREGATION) */
        }   /* for (having_info_cnt) */
    }       /* if (having_info_cnt)  */

    {
        T_VALUEDESC *_tmpval =
                &select->queryresult.list[query->selection.len].value;

        return copy_value_into_record(-1,
                rec_buf + fieldinfo[numfields - 1].offset, _tmpval,
                &fieldinfo[numfields - 1], 0);
    }
}

static int
Ins_or_Upd_row_for_aggr(int handle, T_SELECT * select,
        char *wTable, char *wIndex, char *rec_buf, int rec_buf_len,
        int nullflag_offset, int numfields,
        SYSFIELD_T * fieldinfo, int *wTable_cursor)
{
    T_QUERYDESC *query;
    T_EXPRESSIONDESC expr = { 0, 0, NULL };
    T_VALUEDESC val;
    T_KEYDESC minkey, maxkey;
    FIELDVALUE_T *min_fieldval = NULL;
    FIELDVALUE_T *max_fieldval = NULL;

    MDB_UINT8 bound;
    int i, j, ret = RET_SUCCESS;
    MDB_BOOL is_updated = MDB_FALSE;
    OID inserted_rid;

    T_POSTFIXELEM *elem;
    T_EXPRESSIONDESC *tmp_expr = NULL;
    int n_expr = 0;

    min_fieldval = (FIELDVALUE_T *)
            PMEM_ALLOC(sizeof(FIELDVALUE_T) * MAX_INDEX_FIELD_NUM);
    if (min_fieldval == NULL)
        return RET_ERROR;

    max_fieldval = (FIELDVALUE_T *)
            PMEM_ALLOC(sizeof(FIELDVALUE_T) * MAX_INDEX_FIELD_NUM);
    if (max_fieldval == NULL)
    {
        PMEM_FREE(min_fieldval);
        return RET_ERROR;
    }

    sc_memset(&val, 0, sizeof(val));

    query = &select->planquerydesc.querydesc;

    // index key를 generate 하자.
    for (i = 0; i < query->groupby.len; i++)
    {
        if (query->groupby.list[i].attr.table.aliasname)
            j = find_rttable_idx(query->fromlist.len, select->rttable,
                    query->groupby.list[i].attr.table.aliasname);
        else
            j = find_rttable_idx(query->fromlist.len, select->rttable,
                    query->groupby.list[i].attr.table.tablename);
        if (j == RET_ERROR)
        {
            PMEM_FREE(min_fieldval);
            PMEM_FREE(max_fieldval);
            return RET_ERROR;
        }

        if (query->groupby.list[i].s_ref > -1)
        {
            tmp_expr =
                    &query->selection.list[query->groupby.list[i].s_ref].expr;
            if (tmp_expr->len == 1 && tmp_expr->list[0]->elemtype == SPT_VALUE)
            {
                sc_memcpy(&val, &tmp_expr->list[0]->u.value,
                        sizeof(T_VALUEDESC));
            }
            else
            {
                ret = calc_expression(tmp_expr, &val, MDB_FALSE);
                if (ret < 0)
                    return ret;
            }
        }
        sc_memcpy(&val.attrdesc, &query->groupby.list[i].attr,
                sizeof(T_ATTRDESC));
        val.attrdesc.posi.ordinal =
                fieldinfo[query->groupby.list[i].i_ref].position;

        val.attrdesc.posi.offset =
                fieldinfo[query->groupby.list[i].i_ref].offset;
        val.valuetype = val.attrdesc.type;

        if (query->groupby.list[i].s_ref < 0)
            convert_rec_attr2value(&query->groupby.list[i].attr,
                    select->rttable[j].rec_values,
                    query->groupby.list[i].attr.posi.ordinal, &val);

        if (val.is_null)
        {
            if (make_fieldval_from_fieldinfo(&min_fieldval[i],
                            &fieldinfo[query->groupby.list[i].i_ref]) == NULL)
            {
                PMEM_FREE(min_fieldval);
                PMEM_FREE(max_fieldval);
                return RET_ERROR;
            }
            if (make_fieldval_from_fieldinfo(&max_fieldval[i],
                            &fieldinfo[query->groupby.list[i].i_ref]) == NULL)
            {
                PMEM_FREE(min_fieldval);
                PMEM_FREE(max_fieldval);
                return RET_ERROR;
            }
        }
        else
        {
            bound = MDB_GE;
            if (make_fieldvalue(&val, &min_fieldval[i], &bound) == RET_ERROR)
            {
                PMEM_FREE(min_fieldval);
                PMEM_FREE(max_fieldval);
                return RET_ERROR;
            }

            bound = MDB_LE;
            if (make_fieldvalue(&val, &max_fieldval[i], &bound) == RET_ERROR)
            {
                PMEM_FREE(min_fieldval);
                PMEM_FREE(max_fieldval);
                return RET_ERROR;
            }
            sql_value_ptrFree(&val);
        }
    }

    if (i > 0)
    {
        dbi_KeyValue(i, min_fieldval, &minkey);
        dbi_KeyValue(i, max_fieldval, &maxkey);
    }
    else
    {
        sc_memset(&minkey, 0, sizeof(T_KEYDESC));
        sc_memset(&maxkey, 0, sizeof(T_KEYDESC));
    }

    if (*wTable_cursor < 0)
        *wTable_cursor = dbi_Cursor_Open(handle, wTable, wIndex,
                &minkey, &maxkey, NULL, LK_EXCLUSIVE, 0);
    else
        ret = dbi_Cursor_Reopen(handle, *wTable_cursor, &minkey, &maxkey,
                NULL);

    if (*wTable_cursor < 0 || ret < 0)
    {
        PMEM_FREE(min_fieldval);
        PMEM_FREE(max_fieldval);
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, *wTable_cursor, 0);
        return RET_ERROR;
    }

    // (A+B) 한 갯수를 계산해서 할당한다.
    // A. query->selection.list[i].expr.u.aggr.significant
    // B. query->groupby.having.list[having_info[i].posi_in_having].expr.u.aggr.significant
    n_expr = select->planquerydesc.querydesc.aggr_info.n_expr;
    if (n_expr > 0)
    {
        expr.list = sql_mem_get_free_postfixelem_list(NULL, n_expr);
        if (expr.list == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }

        elem = PMEM_ALLOC(sizeof(T_POSTFIXELEM) * n_expr);
        if (elem == NULL)
        {
            sql_mem_set_free_postfixelem_list(NULL, expr.list);
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }

        for (i = 0; i < n_expr; ++i)
            expr.list[i] = &(elem[i]);
        expr.len = 0;
        expr.max = n_expr;
    }

    /* ret : SUCCESS, FAIL, DB_E_NOMORERECORDS, DB_E_TERMINATED */
    ret = dbi_Cursor_Read(handle, *wTable_cursor, rec_buf, &rec_buf_len,
            NULL, 0, 0);

    if (ret == DB_SUCCESS)
    {
        if (make_temp_record(handle, select, rec_buf, nullflag_offset, &expr,
                        numfields, fieldinfo, &is_updated,
                        MDB_FALSE) == RET_ERROR)
        {
            goto RETURN_LABEL;
        }

        if (is_updated == 1)
        {
            ret = dbi_Cursor_Update(handle, *wTable_cursor, rec_buf,
                    rec_buf_len, NULL);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                goto RETURN_LABEL;
            }
        }
    }
    else if (ret == DB_E_NOMORERECORDS) // 같은 데이타가 없는 경우.. 걍 insert하자...
    {
        if (make_temp_record(handle, select, rec_buf, nullflag_offset, &expr,
                        numfields, fieldinfo, &is_updated,
                        MDB_TRUE) == RET_ERROR)
        {
            goto RETURN_LABEL;
        }

        ret = dbi_Cursor_Insert(handle, *wTable_cursor, rec_buf,
                rec_buf_len, 0, &inserted_rid);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            goto RETURN_LABEL;
        }
    }
    else
    {
        goto RETURN_LABEL;
    }

    if (n_expr > 0)
    {
        sql_mem_set_free_postfixelem(NULL, expr.list[0]);
        sql_mem_set_free_postfixelem_list(NULL, expr.list);
    }
    PMEM_FREE(min_fieldval);
    PMEM_FREE(max_fieldval);
    return RET_SUCCESS;


  RETURN_LABEL:
    if (expr.list)
    {
        sql_mem_set_free_postfixelem(NULL, expr.list[0]);
        sql_mem_set_free_postfixelem_list(NULL, expr.list);
    }
    PMEM_FREENUL(min_fieldval);
    PMEM_FREENUL(max_fieldval);
    return RET_ERROR;
}

/*****************************************************************************/
//!  check_having_and_delete
//
/*! \breif  HAVING 절을 적용하여, HAVING 조건에 거짓인 레코드는 삭제
 ************************************
 * \param handle() :
 * \param stmt() :
 * \param wTable() :
 * \param rec_buf() :
 * \param rec_buf_len() :
 * \param nullflag_offset() :
 * \param fieldinfo() :
 * \param having_info() :
 * \param having_info_cnt() :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *  - attrdesc에 collation 설정
 *
 *****************************************************************************/
static int
check_having_and_delete(int handle, T_SELECT * select, char *wTable,
        char *rec_buf, int rec_buf_len,
        int nullflag_offset, SYSFIELD_T * fieldinfo)
{
    T_EXPRESSIONDESC *having, expr = { 0, 0, NULL };
    T_POSTFIXELEM *elem;
    T_VALUEDESC val;
    T_ATTRDESC attr;
    T_HAVINGINFO *having_info;

    int Htable;
    int ref, start;
    int expr_posi, having_info_posi;
    int i, j, ret = RET_ERROR;

    having_info = select->planquerydesc.querydesc.having.hvinfo;
    if (select->planquerydesc.querydesc.having.len == 0)
        return RET_SUCCESS;

    Htable = dbi_Cursor_Open(handle, wTable, NULL, NULL, NULL, NULL,
            LK_EXCLUSIVE, 0);

    if (Htable < 0)
        return RET_ERROR;

    expr.len = 0;
    expr.max = select->planquerydesc.querydesc.aggr_info.n_expr;

    expr.list = sql_mem_get_free_postfixelem_list(NULL, expr.max);
    if (expr.list == NULL)
        return RET_ERROR;

    elem = PMEM_ALLOC(sizeof(T_POSTFIXELEM) * expr.max);
    if (elem == NULL)
    {
        sql_mem_set_free_postfixelem_list(NULL, expr.list);
        return RET_ERROR;
    }

    for (i = 0; i < expr.max; ++i)
        expr.list[i] = &(elem[i]);

    having = &select->planquerydesc.querydesc.groupby.having;
    start = -1;

    for (i = 0; i < having->len; i++)
    {
        if (start < 0)
            start = i;

        elem = having->list[i];

        if (elem->elemtype == SPT_AGGREGATION)
        {
            if (i - elem->u.aggr.significant != start)
            {
                for (j = start; j < i - elem->u.aggr.significant; j++)
                {
                    sc_memcpy(expr.list[expr.len], having->list[j],
                            sizeof(T_POSTFIXELEM));
                    ++expr.len;
                }
            }

            expr.list[expr.len]->elemtype = SPT_VALUE;
            expr.list[expr.len]->u.value.valueclass = SVC_CONSTANT;

            if (elem->u.aggr.type == SAT_COUNT)
                expr.list[expr.len]->u.value.valuetype = DT_INTEGER;
            else if (elem->u.aggr.type == SAT_AVG)
                expr.list[expr.len]->u.value.valuetype = DT_DOUBLE;
            else if (elem->u.aggr.type == SAT_SUM)
                expr.list[expr.len]->u.value.valuetype = DT_DOUBLE;
            else
                expr.list[expr.len]->u.value.valuetype = elem->u.aggr.valtype;
            ++expr.len;
            start = -1;
        }
    }

    if (start >= 0)
    {
        for (j = 0; j < having->len - start; ++j)
        {
            sc_memcpy(expr.list[expr.len + j], having->list[start + j],
                    sizeof(T_POSTFIXELEM));
            sql_value_ptrInit(&expr.list[expr.len + j]->u.value);
        }
        expr.len += having->len - start;
    }

    while (1)
    {
        /* ret : SUCCESS, FAIL, DB_E_NOMORERECORDS, DB_E_TERMINATED */
        ret = dbi_Cursor_Read(handle, Htable, rec_buf, &rec_buf_len,
                NULL, 0, 0);
        if (ret < 0L)
            goto RETURN_LABEL;

        expr_posi = having_info_posi = 0;

        for (i = 0, start = -1; i < having->len; i++)
        {
            if (start < 0)
                start = i;

            elem = having->list[i];

            if (elem->elemtype == SPT_AGGREGATION)
            {
                if (i - elem->u.aggr.significant != start)
                {
                    for (j = start;
                            j < i - having->list[i]->u.aggr.significant; j++)
                    {
                        elem = having->list[j];

                        if (elem->elemtype == SPT_VALUE
                                && elem->u.value.valueclass == SVC_VARIABLE)
                        {
                            ref = having_info[having_info_posi++].
                                    posi_in_fields;

                            // 더이상 사용안하는 코드인것 같아서 처리 안함

                            if (DB_VALUE_ISNULL(rec_buf,
                                            fieldinfo[ref].position,
                                            nullflag_offset))
                            {
                                expr.list[expr_posi]->u.value.is_null = 1;
                            }
                            else
                            {
                                attr.posi.offset = fieldinfo[ref].offset;
                                attr.type = fieldinfo[ref].type;
                                attr.len = fieldinfo[ref].length;
                                attr.collation = fieldinfo[ref].collation;
                                convert_attr2value(&attr,
                                        &expr.list[expr_posi]->u.value,
                                        rec_buf);
                                expr.list[expr_posi]->u.value.is_null = 0;
                            }
                        }
                        ++expr_posi;
                    }
                }

                ref = having_info[having_info_posi++].posi_in_fields;

                if (DB_VALUE_ISNULL(rec_buf, fieldinfo[ref].position,
                                nullflag_offset))
                {
                    expr.list[expr_posi]->u.value.is_null = 1;
                }
                else
                {
                    attr.posi.offset = fieldinfo[ref].offset;
                    attr.type = fieldinfo[ref].type;
                    attr.len = fieldinfo[ref].length;
                    attr.collation = fieldinfo[ref].collation;
                    convert_attr2value(&attr, &expr.list[expr_posi]->u.value,
                            rec_buf);
                    expr.list[expr_posi]->u.value.is_null = 0;
                }

                ++expr_posi;
                start = -1;
            }
        }

        if (start >= 0)
        {
            for (i = start; i < having->len; i++)
            {
                elem = having->list[i];
                if (elem->elemtype == SPT_VALUE
                        && elem->u.value.valueclass == SVC_VARIABLE)
                {
                    ref = having_info[having_info_posi++].posi_in_fields;

                    if (DB_VALUE_ISNULL(rec_buf, fieldinfo[ref].position,
                                    nullflag_offset))
                    {
                        expr.list[expr_posi]->u.value.is_null = 1;
                    }
                    else
                    {
                        attr.posi.offset = fieldinfo[ref].offset;
                        attr.type = fieldinfo[ref].type;
                        attr.len = fieldinfo[ref].length;
                        attr.collation = fieldinfo[ref].collation;

                        // 아래.. len 설정 부분은.. 빠져도 되지 않나?
                        expr.list[expr_posi]->u.value.attrdesc.len = attr.len;
                        convert_attr2value(&attr,
                                &expr.list[expr_posi]->u.value, rec_buf);
                        expr.list[expr_posi]->u.value.is_null = 0;
                    }
                }
                ++expr_posi;
            }
        }

        if (calc_expression(&expr, &val, MDB_FALSE) < 0
                || val.valuetype != DT_BOOL)
            goto RETURN_LABEL;

        if (val.u.bl)
            dbi_Cursor_Read(handle, Htable, rec_buf, &rec_buf_len, NULL, 1, 0);
        else
            dbi_Cursor_Remove(handle, Htable);

        sql_value_ptrFree(&val);

    }       /* while(1) */

  RETURN_LABEL:
    dbi_Cursor_Close(handle, Htable);

    for (i = 0; i < expr.len; i++)
    {
        sql_value_ptrFree(&expr.list[i]->u.value);
    }

    sql_mem_set_free_postfixelem(NULL, expr.list[0]);
    sql_mem_set_free_postfixelem_list(NULL, expr.list);

    return ret;
}

/*****************************************************************************/
//! create_index_on_worktable

/*! \breif  임시 테이블에 대해 GROUP BY,ORDER BY에서 명시한 칼럼으로
 *          색인을 생성한다.
 ************************************
 * \param handle(in)        :
 * \param wTable(in)        :
 * \param wIndex(in/out)    :
 * \param select(in/out)    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *  - STEP : 1. GROUP BY를 위한 TEMP INDEX를 생성한다
 *      STEP : 2. ORDER BY를 위한 TEMP INDEX를 생성한다
 *  - if the orderby and groupby expression are same,
 *      create only groupby temp index
 *  - COLLATION 지원에 따른 dbi_FieldDesc 인터페이스 변경
 *****************************************************************************/
int
create_index_on_worktable(int *handle, char *wTable, char *wIndex,
        T_SELECT * select)
{
    T_QUERYDESC *qdesc;
    FIELDDESC_T *fielddesc = NULL;

    char col_name[FIELD_NAME_LENG];
    int n_index_fd;
    int ret;

    MDB_UINT8 _type = 0;
    int use_groupby_idx = 1;

    qdesc = &select->planquerydesc.querydesc;
    if (qdesc->groupby.len <= 0)
        goto TEMP_INDEX_STEP_1;

    // STEP : 1. GROUP BY를 위한 TEMP INDEX를 생성한다
    if (dbi_Trans_NextTempName(*handle, 0, wIndex, NULL) < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
        return RET_ERROR;
    }

    fielddesc = PMEM_ALLOC(sizeof(FIELDDESC_T) * qdesc->groupby.len);

    if (fielddesc == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return RET_ERROR;
    }

    if (qdesc->groupby.len < qdesc->orderby.len)
        use_groupby_idx = 0;

    for (n_index_fd = 0; n_index_fd < qdesc->groupby.len; n_index_fd++)
    {
        sc_snprintf(col_name, FIELD_NAME_LENG, "%s%d", INTERNAL_FIELD_PREFIX,
                qdesc->groupby.list[n_index_fd].i_ref);

        _type = 0;

        if (use_groupby_idx && (n_index_fd < qdesc->orderby.len))
        {
            if (qdesc->orderby.list[n_index_fd].i_ref
                    != qdesc->groupby.list[n_index_fd].i_ref)
                use_groupby_idx = 0;
            else if (qdesc->orderby.list[n_index_fd].ordertype == 'D')
                _type |= FD_DESC;
        }

        // DT_NULL_TYPE 이니깐..
        dbi_FieldDesc(col_name, 0, 0, 0, _type, 0, FIXED_VARIABLE,
                &(fielddesc[n_index_fd]), MDB_COL_NONE);
    }

    /* 임시 테이블에 대해 GROUP BY절에 명시한 칼럼으로 색인을 생성한다. */
    ret = dbi_Index_Create(*handle, wTable, wIndex, n_index_fd, fielddesc, 0);
    if (ret < 0)
    {
        PMEM_FREENUL(fielddesc);
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        return RET_ERROR;
    }

    PMEM_FREENUL(fielddesc);

    if (use_groupby_idx)
        return RET_SUCCESS;


    // STEP : 2. ORDER BY를 위한 TEMP INDEX를 생성한다
  TEMP_INDEX_STEP_1:
    if (qdesc->orderby.len > 0)
    {
        char wOrderByIdx[INDEX_NAME_LENG];      // GROUP BY 때 사용하는 INDEX

        if (dbi_Trans_NextTempName(*handle, 0, wOrderByIdx, NULL) < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
            return RET_ERROR;
        }

        fielddesc = PMEM_ALLOC(sizeof(FIELDDESC_T) * qdesc->orderby.len);

        if (fielddesc == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return RET_ERROR;
        }

        for (n_index_fd = 0; n_index_fd < qdesc->orderby.len; n_index_fd++)
        {
            sc_snprintf(col_name, FIELD_NAME_LENG, "%s%d",
                    INTERNAL_FIELD_PREFIX,
                    qdesc->orderby.list[n_index_fd].i_ref);

            _type = 0;

            if (qdesc->orderby.list[n_index_fd].ordertype == 'D')
                _type |= FD_DESC;

            // DT_NULL_TYPE 이므로..
            dbi_FieldDesc(col_name, 0, 0, 0, _type, 0, FIXED_VARIABLE,
                    &(fielddesc[n_index_fd]), MDB_COL_NONE);
        }

        ret = dbi_Index_Create(*handle, wTable, wOrderByIdx,
                n_index_fd, fielddesc, 0);
        if (ret < 0)
        {
            PMEM_FREENUL(fielddesc);
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return RET_ERROR;
        }

    }

    PMEM_FREENUL(fielddesc);
    return RET_SUCCESS;

}

/*****************************************************************************/
//! init_value_attrdesc

/*! \breif value's attribute info에 값을 설정
 ************************************
 * \param val(in/out)   :
 * \param fieldinfo(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  called : make_new_select(), make_groupby
 *  - 이 경우 이 함수 대신
 *****************************************************************************/
void
init_value_attrdesc(T_ATTRDESC * attrdesc, SYSFIELD_T * fieldinfo)
{

    attrdesc->posi.ordinal = fieldinfo->position;
    attrdesc->posi.offset = fieldinfo->offset;
    attrdesc->type = fieldinfo->type;
    attrdesc->len = fieldinfo->length;
    attrdesc->Htable = fieldinfo->tableId;
    attrdesc->Hattr = fieldinfo->fieldId;
    attrdesc->collation = fieldinfo->collation;
}

static int
make_distinct_for_aggr(int handle, T_SELECT * select, char *wTable,
        char *rec_buf, int rec_buf_len)
{
    T_QUERYDESC *qdesc;
    T_JOINTABLEDESC *join;

    SYSTABLE_T *tableinfo = NULL;
    SYSFIELD_T *fieldinfo = NULL;

    int ret;
    int read_cursor = -1, insert_cursor = -1;

    qdesc = &select->tmp_sub->planquerydesc.querydesc;
    join = qdesc->fromlist.list;
    tableinfo = &join->tableinfo;
    fieldinfo = join->fieldinfo;

    read_cursor = dbi_Cursor_Open(handle, wTable,
            NULL, NULL, NULL, NULL, LK_SHARED, 0);
    if (read_cursor < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, read_cursor, 0);
        return RET_ERROR;
    }

    insert_cursor = dbi_Cursor_Open(handle, tableinfo->tableName,
            NULL, NULL, NULL, NULL, LK_EXCLUSIVE, 0);
    if (insert_cursor < 0)
    {
        ret = insert_cursor;
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, insert_cursor, 0);
        goto RETURN_LABEL;
    }

    while (1)
    {
        ret = dbi_Cursor_Read(handle, read_cursor, rec_buf, &rec_buf_len,
                NULL, 1, 0);
        if (ret < 0)
        {
            if (ret == DB_E_NOMORERECORDS)
                break;
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            goto RETURN_LABEL;
        }

        sc_memset(rec_buf + fieldinfo[select->tmp_sub->queryresult.len].offset,
                0,
                rec_buf_len -
                fieldinfo[select->tmp_sub->queryresult.len].offset);

        ret = dbi_Cursor_Distinct_Insert(handle, insert_cursor, rec_buf,
                rec_buf_len);
        if (ret == DB_E_DUPDISTRECORD)
        {
            --select->rows;
        }
        else if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            goto RETURN_LABEL;
        }
    }

  RETURN_LABEL:

    if (insert_cursor >= 0)
        dbi_Cursor_Close(handle, insert_cursor);
    if (read_cursor >= 0)
        dbi_Cursor_Close(handle, read_cursor);

    return ret;
}

static int
make_groupby(int *handle, T_SELECT * select)
{
    T_QUERYDESC *qdesc;
    T_QUERYRESULT *qresult;
    T_JOINTABLEDESC *join;

    SYSTABLE_T tableinfo;
    SYSFIELD_T *fieldinfo = NULL;

    char *wTable;
    char wIndex[INDEX_NAME_LENG];       // GROUP BY 때 사용하는 INDEX
    char *rec_buf = NULL;
    int nullflag_offset, rec_buf_len;
    int wTable_cursor = -1;
    int i, ret;
    T_EXPRESSIONDESC *expr;
    T_ATTRDESC *attr;
    int j, k, l;

    qdesc = &select->tmp_sub->planquerydesc.querydesc;
    join = qdesc->fromlist.list;

    if (select->planquerydesc.querydesc.is_distinct)
        wTable = select->planquerydesc.querydesc.groupby.distinct_table;
    else
        wTable = join->tableinfo.tableName;

    if (create_index_on_worktable(handle, wTable, wIndex,
                    select) != RET_SUCCESS)
        goto ERROR_LABEL;

    ret = dbi_Schema_GetTableFieldsInfo(*handle, wTable, &tableinfo,
            &fieldinfo);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        goto ERROR_LABEL;
    }

    nullflag_offset = get_nullflag_offset(*handle, wTable, &tableinfo);
    rec_buf_len = get_recordlen(*handle, wTable, &tableinfo);
    if (rec_buf_len <= 0)
    {
        goto ERROR_LABEL;
    }

    rec_buf = PMEM_XCALLOC(rec_buf_len);
    if (rec_buf == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto ERROR_LABEL;
    }

    qdesc = &select->planquerydesc.querydesc;
    qresult = &select->queryresult;

    for (i = 0; i < qresult->len; i++)
    {
        init_value_attrdesc(&qresult->list[i].value.attrdesc, &fieldinfo[i]);
    }

    init_value_attrdesc(&qresult->list[i].value.attrdesc, &fieldinfo[i]);

    if (qdesc->is_distinct
            && set_distinct_table(handle, select) != RET_SUCCESS)
        goto ERROR_LABEL;

    /* 결과 레코드 하나를 인출하고 이를 임시 테이블에 삽입 또는 갱신한다. */

    while (1)
    {
        ret = SQL_fetch(handle, select, NULL);
        if (ret == RET_END)
            break;

        if (ret == RET_ERROR)
            goto ERROR_LABEL;

        sc_memset(rec_buf, 0, rec_buf_len);

        if (Ins_or_Upd_row_for_aggr(*handle,
                        select, wTable, wIndex, rec_buf, rec_buf_len,
                        nullflag_offset, tableinfo.numFields,
                        fieldinfo, &wTable_cursor) == RET_ERROR)
        {
            goto ERROR_LABEL;
        }

        if (ret == RET_END_USED_DIRECT_API)
            break;
    }

    if (wTable_cursor >= 0)
        dbi_Cursor_Close(*handle, wTable_cursor);

    /* 집계 함수의 결과 값을 계산한다. */
    if (make_result_for_aggr(*handle, select, wTable, rec_buf,
                    rec_buf_len, nullflag_offset, fieldinfo) == RET_ERROR)
    {
        goto ERROR_LABEL;
    }

    /* HAVING절을 적용하여 거짓인 레코드는 삭제한다. */
    if (check_having_and_delete(*handle, select, wTable, rec_buf,
                    rec_buf_len, nullflag_offset, fieldinfo) == RET_ERROR)
    {
        goto ERROR_LABEL;
    }

    if (select->planquerydesc.querydesc.is_distinct)
    {
        if (make_distinct_for_aggr(*handle, select, wTable, rec_buf,
                        rec_buf_len) == RET_ERROR)
        {
            goto ERROR_LABEL;
        }
    }

    PMEM_FREENUL(rec_buf);
    PMEM_FREENUL(fieldinfo);

    qdesc = &select->planquerydesc.querydesc;
    for (i = 0; i < qdesc->selection.len; i++)
    {
        expr = &qdesc->selection.list[i].expr;
        for (j = 0; j < expr->len; j++)
        {
            if (expr->list[j]->elemtype == SPT_VALUE
                    && expr->list[j]->u.value.valueclass == SVC_VARIABLE)
            {
                attr = &expr->list[j]->u.value.attrdesc;
                for (k = 0; k < qdesc->fromlist.len; k++)
                {
                    if (mdb_strcmp(attr->table.tablename,
                                    qdesc->fromlist.list[k].tableinfo.
                                    tableName))
                    {
                        continue;
                    }

                    fieldinfo = qdesc->fromlist.list[k].fieldinfo;
                    for (l = 0;
                            l < qdesc->fromlist.list[k].tableinfo.numFields;
                            l++)
                    {
                        if (mdb_strcmp(attr->attrname, fieldinfo[l].fieldName))
                        {
                            continue;
                        }

                        attr->posi.ordinal = fieldinfo[l].position;
                        attr->posi.offset = fieldinfo[l].offset;
                        attr->type = fieldinfo[l].type;
                        attr->len = fieldinfo[l].length;
                        break;
                    }

                    if (l != qdesc->fromlist.list[k].tableinfo.numFields)
                    {
                        break;
                    }
                }
            }
        }
    }

    return RET_SUCCESS;

  ERROR_LABEL:

    if (wTable_cursor >= 0)
        dbi_Cursor_Close(*handle, wTable_cursor);
    dbi_Relation_Drop(*handle, wTable);
    PMEM_FREENUL(rec_buf);
    PMEM_FREENUL(fieldinfo);

    return RET_ERROR;
}

/*****************************************************************************/
//!do_simple_count_query

/*! \breif  SELECT COUNT(*).. 의 쿼리인 경우 사용되는 함수
 ************************************
 * \param param_name :
 * \param param_name :
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
extern int make_filter(EXPRDESC_LIST * expr_list, T_NESTING * nest);
extern int make_keyrange_for_scandesc(T_NESTING * nest, SCANDESC_T * scandesc);

int
do_simple_count_query(int handle, T_SELECT * select)
{
    T_QUERYDESC *qdesc;
    T_RTTABLE *rttable;
    T_TABLEDESC *table;
    int cnt, total_cnt;
    int i;
    int ret = RET_SUCCESS;

    qdesc = &select->planquerydesc.querydesc;
    rttable = select->rttable;

    total_cnt = 1;

    for (i = 0; i < qdesc->fromlist.len; i++)
    {
        table = &qdesc->fromlist.list[i].table;

        if (qdesc->fromlist.len == 1 && !rttable->sql_expr_list &&
                qdesc->condition.len)
        {
            T_NESTING *nest = &select->planquerydesc.nest[0];
            SCANDESC_T scandesc;

            sc_memset(&scandesc, 0, sizeof(SCANDESC_T));
            if (nest && rttable->db_expr_list)
            {
                ret = make_filter(rttable->db_expr_list, nest);
                if (ret < 0)
                    return RET_ERROR;
            }

            if (nest && nest->index_field_num > 0)
            {
                ret = make_keyrange_for_scandesc(nest, &scandesc);
                if (ret < 0)
                {
                    if (nest && rttable->db_expr_list)
                    {
                        PMEM_FREENUL(nest->filter);
                    }

                    return RET_ERROR;
                }
            }
            total_cnt = dbi_Cursor_Count2(handle, table->tablename,
                    nest->indexname, &scandesc.minkey, &scandesc.maxkey,
                    nest->filter);
            if (nest && rttable->db_expr_list)
                PMEM_FREENUL(nest->filter);
            /* if total_cnt < 0 then error */
            if (total_cnt < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, total_cnt, 0);
                return RET_ERROR;
            }
            break;
        }

        if (qdesc->selection.list[0].expr.list[0]->u.aggr.type == SAT_DCOUNT)
        {
            cnt = dbi_Cursor_DirtyCount(handle, table->tablename);
        }
        else
        {
            cnt = dbi_Cursor_Count(handle, table->tablename,
                    NULL, NULL, NULL, NULL);
        }

        if (cnt < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, cnt, 0);
            return RET_ERROR;
        }
        else
            total_cnt *= cnt;
    }

    select->planquerydesc.querydesc.aggr_info.aggr[0].value.valuetype =
            DT_INTEGER;
    select->planquerydesc.querydesc.aggr_info.aggr[0].value.u.i32 = total_cnt;
    select->planquerydesc.querydesc.aggr_info.aggr[0].value.is_null = 0;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! do_simple_minmax_query

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)  :
 * \param select()  :
 * \param index()   :
 * \param aggr(in/out)    :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
int
do_simple_minmax_query(int handle, T_SELECT * select)
{
    T_QUERYDESC *qdesc;
    T_POSTFIXELEM *agg_elem, *arg_elem;
    T_ATTRDESC *attr;
    T_NESTING *nest;
    char *rec_buf;
    int rec_buf_len;
    int nullflag_offset;
    int Htable, ret;
    SYSTABLE_T tableinfo;
    int open_ascending_order;
    struct KeyValue *minkey = NULL;
    struct KeyValue *maxkey = NULL;
    struct Filter *filter = NULL;
    T_RTTABLE *rttable;
    SCANDESC_T scandesc;

    qdesc = &select->planquerydesc.querydesc;
    arg_elem = qdesc->selection.list[0].expr.list[0];
    agg_elem = qdesc->selection.list[0].expr.list[1];
    attr = &arg_elem->u.value.attrdesc;
    nest = select->planquerydesc.nest;

    rttable = select->rttable;

    if (qdesc->fromlist.len == 1 &&
            !rttable->sql_expr_list && qdesc->condition.len)
    {
        T_NESTING *nest = &select->planquerydesc.nest[0];

        if (nest->indexname == NULL)
        {
            return SQL_E_INVALIDINDEX;
        }

        sc_memset(&scandesc, 0, sizeof(SCANDESC_T));
        if (nest && rttable->db_expr_list)
            ret = make_filter(rttable->db_expr_list, nest);

        if (nest && nest->index_field_num > 0)
        {
            ret = make_keyrange_for_scandesc(nest, &scandesc);
        }

        minkey = &scandesc.minkey;
        maxkey = &scandesc.maxkey;
        filter = nest->filter;

    }


    if (agg_elem->u.aggr.type == SAT_MIN)
    {
        if (nest->index_field_info[0].order == 'A')
            open_ascending_order = 1;
        else
            open_ascending_order = 0;
    }
    else    /* SAT_MAX */
    {
        if (nest->index_field_info[0].order == 'D')
            open_ascending_order = 1;
        else
            open_ascending_order = 0;
    }

    if (open_ascending_order)
        Htable = dbi_Cursor_Open(handle, attr->table.tablename,
                nest->indexname, minkey, maxkey, filter, LK_SHARED, 0);
    else    /* agg_elem->u.aggr.type == SAT_MAX */
        Htable = dbi_Cursor_Open_Desc(handle, attr->table.tablename,
                nest->indexname, minkey, maxkey, filter, LK_SHARED);
    if (Htable < 0)
        return RET_ERROR;

    ret = dbi_Schema_GetTableInfo(handle, attr->table.tablename, &tableinfo);
    if (ret < 0)
    {
        dbi_Cursor_Close(handle, Htable);
        return RET_ERROR;
    }

    nullflag_offset = get_nullflag_offset(handle, attr->table.tablename,
            &tableinfo);

    rec_buf_len = get_recordlen(handle, attr->table.tablename, &tableinfo);
    if (rec_buf_len <= 0)
    {
        dbi_Cursor_Close(handle, Htable);
        return RET_ERROR;
    }

    rec_buf = PMEM_ALLOC(rec_buf_len);
    if (rec_buf == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        dbi_Cursor_Close(handle, Htable);
        return RET_ERROR;
    }

    while (1)
    {
        ret = dbi_Cursor_Read(handle, Htable, rec_buf, &rec_buf_len,
                NULL, 1, 0);

        if (ret == DB_E_NOMORERECORDS)
            break;

        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return RET_ERROR;
        }

        if (!DB_VALUE_ISNULL(rec_buf, attr->posi.ordinal, nullflag_offset))
        {
            qdesc->aggr_info.aggr[0].value.valuetype =
                    arg_elem->u.value.valuetype;
            convert_attr2value(attr, &qdesc->aggr_info.aggr[0].value, rec_buf);
            qdesc->aggr_info.aggr[0].value.is_null = 0;
            break;
        }
    }

    dbi_Cursor_Close(handle, Htable);
    PMEM_FREENUL(rec_buf);

    return RET_SUCCESS;
}

/*****************************************************************************/
//! make_result_select

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)    :
 * \param stmt(in/out)  :
 * \param aggr(in)      :
 * \param n_aggr(in)    :
 * \param sub_aggr(in)  :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      STACK SIZE를 5K로 맞추기 위한 작업임
 *  - parsing하는 부분은 없으나.. 마지막에 stmt와 new_stmt를 바꿔치기
 *      하기 때문에 stmt->parsing_memory를 할당
 *  - T_EXPRESSIONDESC의 max와 len을 수정
 *
 *****************************************************************************/
int
make_result_select(int *handle, T_SELECT * select)
{
    T_SELECT *new_select;
    T_LIST_SELECTION *selection, *new_selection;
    T_EXPRESSIONDESC *expr, *new_expr;
    T_POSTFIXELEM *elem;
    int pos, e_index;
    int i, j, n;

    int n_aggr = select->planquerydesc.querydesc.aggr_info.len;
    T_RESULTCOLUMN *aggr = select->planquerydesc.querydesc.aggr_info.aggr;

    new_select = select->tmp_sub;

    new_selection = &new_select->planquerydesc.querydesc.selection;
    selection = &select->planquerydesc.querydesc.selection;

    for (i = 0, pos = n_aggr; i < new_selection->len; i++)
    {
        e_index = (selection->len - 1) - i;

        new_expr = &new_selection->list[e_index].expr;
        expr = &selection->list[e_index].expr;

        for (j = 0, n = expr->len; j < expr->len; j++)
        {
            elem = expr->list[j];
            if (elem->elemtype == SPT_AGGREGATION)
                n -= elem->u.aggr.significant;
        }

        for (j = 0; j < expr->len; j++)
        {
            elem = expr->list[(expr->len - 1) - j];
            if (elem->elemtype == SPT_AGGREGATION)
            {
                sc_memcpy(&new_expr->list[--n]->u.value, &aggr[--pos].value,
                        sizeof(T_VALUEDESC));
                new_expr->list[n]->elemtype = SPT_VALUE;
                new_expr->list[n]->is_distinct = 0;
                j += elem->u.aggr.significant;
            }
            else
            {
                sc_memcpy(new_expr->list[--n], elem, sizeof(T_POSTFIXELEM));
            }
        }
    }

    new_select->rows = 0;

    if (new_select->rowsp)
        *new_select->rowsp = 0;

    for (i = 0; i < n_aggr; i++)
    {
        if (aggr[i].value.is_null == 0)
            new_select->rows = 1;
    }

    new_select->tstatus = TSS_EXECUTED;

    /* I don't know this function really needed. */
    new_select->values = select->values;

    return RET_SUCCESS;
}

int
init_aggregates_result(T_RESULTCOLUMN * aggr, T_LIST_SELECTION * selection)
{
    T_EXPRESSIONDESC *expr, aggr_expr;
    T_VALUEDESC value;
    int pos, i, j;

    for (i = 0, pos = 0; i < selection->len; i++)
    {
        expr = &selection->list[i].expr;
        for (j = 0; j < expr->len; j++)
        {
            if (expr->list[j]->elemtype == SPT_AGGREGATION)
            {
                aggr_expr.list =
                        &(expr->list[j - (expr->list[j]->u.aggr.significant)]);
                aggr_expr.len = expr->list[j]->u.aggr.significant;

                if (IS_ALLOCATED_TYPE(aggr[pos].value.valuetype))
                {
                    sql_value_ptrFree(&aggr[pos].value);
                }

                aggr[pos].value.is_null = 1;
                if (expr->list[j]->u.aggr.type != SAT_COUNT)
                {       /* SAT_MIN, SAT_MAX, SAT_SUM, SAT_AVG */
                    if (expr->list[j]->u.aggr.type == SAT_SUM ||
                            expr->list[j]->u.aggr.type == SAT_AVG)
                    {
                        aggr[pos].value.valuetype = DT_DOUBLE;
                    }
                    else
                    {
                        if (calc_expression(&aggr_expr, &value, MDB_TRUE) < 0)
                            continue;   /* error handling or preceed ? */

                        aggr[pos].value.valuetype = value.valuetype;
                        aggr[pos].value.attrdesc.len = value.attrdesc.len;

                        sql_value_ptrFree(&value);
                    }

                    aggr[pos].value.valueclass = SVC_CONSTANT;
                }
                pos++;
            }   /* if (SPT_AGGREGATION) */
        }   /* for(expr->len) */
    }       /* for (selection->len) */

    return RET_SUCCESS;
}

/*****************************************************************************/
//!accumulate_aggregates

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param aggr()        :
 * \param selection()   :
 * \param handle()      :
 * \param stmt()        :
 * \param distinct_table()  :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
int
accumulate_aggregates(T_RESULTCOLUMN * aggr, T_LIST_SELECTION * selection,
        int handle, T_SELECT * select)
{
    T_EXPRESSIONDESC *expr, aggr_expr;
    T_VALUEDESC value;
    int findex, isvalid;

    register int pos, i, j;

    for (i = 0, pos = 0; i < selection->len; i++)
    {
        expr = &selection->list[i].expr;

        for (j = 0, findex = 0; j < expr->len; j++)
        {
            if (expr->list[j]->elemtype == SPT_AGGREGATION)
            {
                aggr_expr.list =
                        &(expr->list[j - (expr->list[j]->u.aggr.significant)]);
                aggr_expr.len = expr->list[j]->u.aggr.significant;

                if (calc_expression(&aggr_expr, &value, MDB_FALSE) < 0)
                {
                    goto ERROR_LABEL;
                }
                if (value.is_null)
                {
                    pos++;
                    continue;
                }

                if (expr->list[j]->is_distinct)
                {
                    isvalid =
                            insert_into_distinct_table(handle, select, i,
                            findex++);
                    if (isvalid < 0)
                    {
                        goto ERROR_LABEL;
                    }
                    else if (isvalid == 0)
                    {
                        pos++;
                        sql_value_ptrFree(&value);
                        continue;
                    }
                }

                if (aggr[pos].value.is_null)
                {
                    aggr[pos].value.is_null = 0;

                    if (expr->list[j]->u.aggr.type == SAT_COUNT)
                    {
                        aggr[pos].value.valuetype = DT_INTEGER;
                        aggr[pos].value.valueclass = SVC_CONSTANT;
                        aggr[pos].value.u.i32 = 1;
                    }
                    else        /* SAT_MIN, SAT_MAX, SAT_SUM, SAT_AVG */
                    {
                        if (expr->list[j]->u.aggr.type == SAT_SUM ||
                                expr->list[j]->u.aggr.type == SAT_AVG)
                        {
                            aggr[pos].value.valuetype = DT_DOUBLE;
                            GET_FLOAT_VALUE(aggr[pos].value.u.f32,
                                    double, &value);
                        }

                        sql_value_ptrInit(&aggr[pos].value);

                        aggr[pos].value.valuetype = value.valuetype;
                        aggr[pos].value.u = value.u;
                        aggr[pos].value.attrdesc.len = value.attrdesc.len;
                        aggr[pos].value.valueclass = SVC_CONSTANT;

                        if (IS_BS_OR_NBS(value.valuetype))
                        {
                            if (sql_value_ptrStrdup(&aggr[pos].value,
                                            aggr[pos].value.u.ptr,
                                            aggr[pos].value.attrdesc.len + 1) <
                                    0)
                            {
                                goto ERROR_LABEL;
                            }
                        }
#ifdef MDB_DEBUG
                        else if (IS_BS_OR_NBS(value.valuetype))
                        {
                            sc_assert(0, __FILE__, __LINE__);
                        }
#endif

                        if (expr->list[j]->u.aggr.type == SAT_AVG)
                        {
                            aggr[pos].len = 1;
                        }
                    }
                }
                else    /* not null */
                {
                    switch (expr->list[j]->u.aggr.type)
                    {
                    case SAT_COUNT:
                        aggr[pos].value.u.i32++;
                        break;
                    case SAT_MIN:
                        get_minvalue(&aggr[pos], &value);
                        break;
                    case SAT_MAX:
                        get_maxvalue(&aggr[pos], &value);
                        break;
                    case SAT_SUM:
                        get_sumvalue(&aggr[pos], &value);
                        break;
                    case SAT_AVG:
                        get_sumvalue(&aggr[pos], &value);
                        aggr[pos].len++;
                        break;
                    default:   /* unknown aggregate function */
                        sql_value_ptrFree(&value);
                        goto ERROR_LABEL;
                    }
                }

                pos++;

                sql_value_ptrFree(&value);
            }   /* if (SPT_AGGREGATION) */
        }   /* for(expr->len) */
    }       /* for (selection->len) */

    return RET_SUCCESS;

  ERROR_LABEL:

    sql_value_ptrFree(&value);

    return RET_ERROR;   // 메모리가 부족한 경우..
}

/*****************************************************************************/
//! calculate_aggregates

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param aggr()        :
 * \param selection()   :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static int
calculate_aggregates(T_RESULTCOLUMN * aggr, T_LIST_SELECTION * selection)
{
    T_EXPRESSIONDESC *expr;
    double value;
    int pos;
    int i, j;

    /* calculate COUNT(), AVG() */
    for (i = 0, pos = 0; i < selection->len; i++)
    {
        expr = &selection->list[i].expr;

        for (j = 0; j < expr->len; j++)
        {
            if (expr->list[j]->elemtype == SPT_AGGREGATION)
            {
                switch (expr->list[j]->u.aggr.type)
                {
                case SAT_COUNT:
                    if (aggr[pos].value.is_null)
                    {
                        aggr[pos].value.u.i32 = 0;
                        aggr[pos].value.valuetype = DT_INTEGER;
                        aggr[pos].value.is_null = 0;
                    }
                    break;
                case SAT_MIN:
                case SAT_MAX:
                    break;
                case SAT_SUM:
                    if (aggr[pos].value.is_null)
                        break;

                    if (!IS_NUMERIC(aggr[pos].value.valuetype))
                        return RET_ERROR;

                    GET_FLOAT_VALUE(value, double, &aggr[pos].value);
                    aggr[pos].value.u.f32 = value;
                    aggr[pos].value.valuetype = DT_DOUBLE;
                    break;
                case SAT_AVG:
                    if (aggr[pos].value.is_null)
                        break;

                    if (!IS_NUMERIC(aggr[pos].value.valuetype))
                        return RET_ERROR;

                    GET_FLOAT_VALUE(value, double, &aggr[pos].value);
                    aggr[pos].value.u.f32 = value / aggr[pos].len;
                    aggr[pos].value.valuetype = DT_DOUBLE;
                    break;
                default:       /* unknown aggregate function */
                    return RET_ERROR;
                }
                pos++;
            }
        }
    }
    return RET_SUCCESS;
}

static int
make_aggregative_query(int *handle, T_SELECT * select)
{
    T_QUERYDESC *qdesc;
    T_LIST_SELECTION *selection;
    T_EXPRESSIONDESC *expr;
    int ret;
    int result = RET_SUCCESS;

    qdesc = &select->planquerydesc.querydesc;
    selection = &qdesc->selection;

    if (qdesc->limit.rows == 0)
        goto RETURN_LABEL;
    else
    {
        qdesc->limit.offset = 0;
        qdesc->limit.rows = -1;
    }

    if (qdesc->aggr_info.use_dosimple)
    {
        expr = &selection->list[0].expr;
        if (expr->len == 1)
        {
            ret = do_simple_count_query(*handle, select);
            if (ret != RET_SUCCESS)
                return RET_ERROR;
            goto RETURN_LABEL;
        }
        else if (expr->len == 2)
        {
            ret = do_simple_minmax_query(*handle, select);
            if (ret != RET_SUCCESS)
                return RET_ERROR;
            goto RETURN_LABEL;
        }
    }

    if (qdesc->is_distinct && set_distinct_table(handle, select)
            != RET_SUCCESS)
        return RET_ERROR;

    init_aggregates_result(qdesc->aggr_info.aggr, selection);
    /* execute original query, and calculate aggregates */
    while (1)
    {
        ret = SQL_fetch(handle, select, NULL);
        if (ret == RET_END)
            break;
        else if (ret == RET_ERROR)
        {
            result = RET_ERROR;
            goto RETURN_LABEL;

        }
        if (accumulate_aggregates(qdesc->aggr_info.aggr, selection,
                        *handle, select) != RET_SUCCESS)
        {
            result = RET_ERROR;
            goto RETURN_LABEL;

        }
        if (ret == RET_END_USED_DIRECT_API)
            break;

    }
    if (calculate_aggregates(qdesc->aggr_info.aggr, selection) != RET_SUCCESS)
    {
        result = RET_ERROR;
        goto RETURN_LABEL;
    }

  RETURN_LABEL:

    if (result == RET_SUCCESS && (make_result_select(handle, select)
                    != RET_SUCCESS))
    {
        result = RET_ERROR;
    }

    return result;
}

extern int init_rec_buf(T_RTTABLE * rttable, SYSTABLE_T * tableinfo);

static int
make_distinct_or_orderby(int handle, T_SELECT * select)
{
    T_PLANNEDQUERYDESC *plan;
    T_QUERYRESULT *qresult;
    T_RTTABLE *rttable = NULL;
    SYSTABLE_T *tableinfo;
    SYSFIELD_T *fieldinfo;
    T_VALUEDESC *val, tmpval;
    T_ATTRDESC *attr;
    char *wTable;
    int i, ret_fetch, ref;
    int ret = RET_SUCCESS;
    OID insert_rid;

    wTable = select->tmp_sub->rttable[0].table.tablename;
    plan = &select->tmp_sub->planquerydesc;
    tableinfo = &plan->querydesc.fromlist.list[0].tableinfo;
    fieldinfo = plan->querydesc.fromlist.list[0].fieldinfo;

    rttable = select->tmp_sub->rttable;
    rttable->status = SCS_CLOSE;
    if (rttable->rec_buf == NULL)
    {
        if (init_rec_buf(rttable, tableinfo) < 0)
            return RET_ERROR;
    }

    rttable->nullflag_offset = get_nullflag_offset(handle, wTable, tableinfo);

    ret = open_cursor(handle, &rttable[0], NULL, NULL, LK_EXCLUSIVE);
    if (ret < 0)
        return RET_ERROR;

    qresult = &select->queryresult;

    while (1)
    {
        ret_fetch = SQL_fetch(&handle, select, NULL);
        if (ret_fetch == RET_ERROR)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }
        else if (ret_fetch == RET_END)
        {
            goto RETURN_LABEL;
        }

        sc_memset(rttable->rec_buf, 0, rttable->rec_len);

        for (i = 0; i < qresult->len; i++)
        {
            val = &qresult->list[i].value;

            if (val->is_null)
            {
                DB_VALUE_SETNULL(rttable->rec_buf, fieldinfo[i].position,
                        rttable->nullflag_offset);
            }
            else
            {
                ret = copy_value_into_record(-1,
                        rttable->rec_buf + fieldinfo[i].offset, val,
                        &fieldinfo[i], 0);
                if (ret == RET_ERROR)
                    goto RETURN_LABEL;
            }
        }

        for (i = 0; i < plan->querydesc.orderby.len; i++)
        {
            if (plan->querydesc.orderby.list[i].s_ref >= 0)
                continue;

            ref = plan->querydesc.orderby.list[i].i_ref;

            attr = &select->planquerydesc.querydesc.orderby.list[i].attr;

            convert_rec_attr2value(attr, select->rttable->rec_values,
                    attr->posi.ordinal, &tmpval);

            if (tmpval.is_null)
            {
                DB_VALUE_SETNULL(rttable->rec_buf,
                        fieldinfo[ref].position, rttable->nullflag_offset);
            }
            else
            {
                ret = copy_value_into_record(-1,
                        rttable->rec_buf + fieldinfo[ref].offset, &tmpval,
                        &fieldinfo[ref], 0);
                DB_VALUE_SETNOTNULL(rttable->rec_buf,
                        fieldinfo[ref].position, rttable->nullflag_offset);
            }
        }   /* for loop */

        if (select->tmp_sub->kindofwTable & iTABLE_DISTINCT)
            ret = dbi_Cursor_Distinct_Insert(handle, rttable->Hcursor,
                    rttable->rec_buf, rttable->rec_len);
        else
            ret = dbi_Cursor_Insert(handle, rttable->Hcursor,
                    rttable->rec_buf, rttable->rec_len, 0, &insert_rid);
        if (ret == DB_E_DUPDISTRECORD)
        {
            --select->rows;
            ret = RET_SUCCESS;
        }
        else if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }

        if (ret_fetch == RET_END_USED_DIRECT_API)
            break;

    }

  RETURN_LABEL:
    if (rttable)
        close_cursor(handle, rttable);

    return ret;

}

int
recreate_subquery_table(int *handle, T_JOINTABLEDESC * join, char *table_nm)
{
    FIELDDESC_T *fielddesc;
    int i;

    fielddesc = PMEM_ALLOC(join->tableinfo.numFields * sizeof(FIELDDESC_T));
    if (fielddesc == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return RET_ERROR;
    }

    for (i = 0; i < join->tableinfo.numFields; i++)
    {
        dbi_FieldDesc(join->fieldinfo[i].fieldName,
                join->fieldinfo[i].type, join->fieldinfo[i].length,
                join->fieldinfo[i].scale, NULL_BIT, 0,
                join->fieldinfo[i].type == DT_NVARCHAR ?
                join->fieldinfo[i].fixed_part / WCHAR_SIZE :
                join->fieldinfo[i].fixed_part,
                &(fielddesc[i]), join->fieldinfo[i].collation);
    }

    if (dbi_Relation_Create(*handle, table_nm,
                    join->tableinfo.numFields, fielddesc, _CONT_TEMPTABLE,
                    _USER_USER, 0, NULL) < 0)
    {
        PMEM_FREENUL(fielddesc);
        return SQL_E_ERROR;
    }

    PMEM_FREENUL(fielddesc);

    return RET_SUCCESS;
}

int
make_work_temp_select(int handle, T_SELECT * select)
{
    T_PLANNEDQUERYDESC *plan = NULL;    /* fix warning */
    SYSTABLE_T tableinfo;
    SYSFIELD_T *fieldinfo = NULL;

    char *wTable = NULL;
    char *wIndex = NULL;

    int fieldnum;
    int i, ret = RET_SUCCESS;
    FIELDDESC_T *fielddesc;
    int is_temptable_create = 0;

    select->tmp_sub->rows = 0;

    if (select->tmp_sub->planquerydesc.querydesc.fromlist.len > 0)
    {
        wTable = select->tmp_sub->rttable[0].table.tablename;
        wIndex = select->tmp_sub->planquerydesc.nest[0].indexname;
        fieldnum = dbi_Schema_GetTableFieldsInfo(handle, wTable, &tableinfo,
                &fieldinfo);
        if (fieldnum < 0 && fieldnum != DB_E_UNKNOWNTABLE)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return RET_ERROR;
        }

        if (fieldnum == DB_E_UNKNOWNTABLE)
        {
            plan = &select->tmp_sub->planquerydesc;
            if (select->planquerydesc.querydesc.groupby.len > 0 &&
                    select->planquerydesc.querydesc.is_distinct)
            {
                ret = recreate_subquery_table(&handle,
                        plan->querydesc.fromlist.list,
                        select->planquerydesc.querydesc.groupby.
                        distinct_table);
                if (ret != RET_SUCCESS)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
                    return RET_ERROR;
                }
            }

            ret = recreate_subquery_table(&handle,
                    plan->querydesc.fromlist.list, wTable);
            if (ret != RET_SUCCESS)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
                return RET_ERROR;
            }

            fieldnum =
                    dbi_Schema_GetTableFieldsInfo(handle, wTable, &tableinfo,
                    &fieldinfo);
            if (fieldnum < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, fieldnum, 0);
                return RET_ERROR;
            }
            sc_memcpy(&(plan->querydesc.fromlist.list[0].tableinfo),
                    &tableinfo, sizeof(SYSTABLE_T));
            sc_memcpy(plan->querydesc.fromlist.list[0].fieldinfo, fieldinfo,
                    sizeof(SYSFIELD_T) * tableinfo.numFields);

            for (i = 0; i < tableinfo.numFields; i++)
            {
                select->tmp_sub->rttable->rec_values->fld_value[i].offset_ =
                        fieldinfo[i].offset;
                select->tmp_sub->rttable->rec_values->fld_value[i].h_offset_ =
                        fieldinfo[i].h_offset;
                select->tmp_sub->rttable->rec_values->fld_value[i].fixed_part =
                        fieldinfo[i].fixed_part;
            }

            if (select->tmp_sub->kindofwTable == iTABLE_DISTINCT
                    && select->queryresult.len <= MAX_KEY_FIELD_NO)
            {
                T_QUERYRESULT *qresult = &select->queryresult;

                FIELDDESC_T *fielddesc;

                fielddesc = PMEM_ALLOC(sizeof(FIELDDESC_T) * (qresult->len));
                if (fielddesc == NULL)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_OUTOFMEMORY, 0);
                    return RET_ERROR;
                }

                for (i = 0; i < qresult->len; i++)
                {
                    dbi_FieldDesc(fieldinfo[i].fieldName, 0, 0, 0, 0, 0, -1,
                            &(fielddesc[i]), fieldinfo[i].collation);
                }

                if (plan->querydesc.orderby.len)
                {
                    T_ORDERBYDESC *orderby;
                    int orderby_idx = 0;

                    orderby = &plan->querydesc.orderby;

                    for (orderby_idx = 0; orderby_idx < orderby->len;
                            orderby_idx++)
                    {
                        if (orderby->list[orderby_idx].s_ref < 0)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_DISTINCTORDERBY, 0);
                            PMEM_FREENUL(fielddesc);
                            return RET_ERROR;
                        }

                        if (orderby_idx != orderby->list[orderby_idx].s_ref)
                        {
                            fielddesc[orderby_idx] =
                                    fielddesc[orderby->list[orderby_idx].
                                    s_ref];
                        }

                        if (orderby->list[orderby_idx].ordertype == 'D')
                        {
                            fielddesc[orderby_idx].flag |= FD_DESC;
                        }

                        if (orderby->list[orderby_idx].attr.collation !=
                                MDB_COL_NONE)
                        {
                            fielddesc[orderby_idx].collation =
                                    orderby->list[orderby_idx].attr.collation;
                        }
                    }
                }

                ret = dbi_Index_Create(handle, wTable, wIndex, i, fielddesc,
                        0);
                PMEM_FREENUL(fielddesc);
                if (ret < 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    return RET_ERROR;
                }
            }
            else
                is_temptable_create = 1;
        }
        /* binding이거나 원본 table이 변경 된 경우 temp table을
           truncate 후에 data 재입력 */
        else if (select->param_count > 0 || select->tmp_sub->is_retemp)
        {
            Truncate_Table(wTable);

            /* temp table 재생성 값 초기화 */
            select->tmp_sub->is_retemp = MDB_FALSE;
        }
        else
        {
            if (select->tmp_sub->kindofwTable & iTABLE_AGGREGATION &&
                    select->planquerydesc.querydesc.groupby.len == 0)
            {
                select->tmp_sub->tstatus = TSS_EXECUTED;
            }

            PMEM_FREENUL(fieldinfo);
            return RET_SUCCESS;
        }
    }

    if (select->tmp_sub->kindofwTable & iTABLE_AGGREGATION)
    {
        if (select->planquerydesc.querydesc.groupby.len > 0)
            ret = make_groupby(&handle, select);
        else
            ret = make_aggregative_query(&handle, select);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }
    }
    else
    {
        ret = make_distinct_or_orderby(handle, select);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }
    }

    if (is_temptable_create && plan->querydesc.orderby.len)
    {
        fielddesc =
                PMEM_ALLOC(sizeof(FIELDDESC_T) *
                (plan->querydesc.orderby.len));
        if (fielddesc == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }
        for (i = 0; i < plan->querydesc.orderby.len; i++)
        {
            if (plan->querydesc.orderby.list[i].ordertype == 'D')
            {
                dbi_FieldDesc(plan->querydesc.orderby.list[i].attr.attrname,
                        0, 0, 0, FD_DESC, 0, FIXED_VARIABLE, &(fielddesc[i]),
                        plan->querydesc.orderby.list[i].attr.collation);
            }
            else
            {
                dbi_FieldDesc(plan->querydesc.orderby.list[i].attr.attrname,
                        0, 0, 0, 0, 0, FIXED_VARIABLE, &(fielddesc[i]),
                        plan->querydesc.orderby.list[i].attr.collation);
            }
        }

        ret = dbi_Index_Create(handle, wTable, wIndex, i, fielddesc, 0);
        PMEM_FREENUL(fielddesc);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }
    }

  RETURN_LABEL:
    PMEM_FREENUL(fieldinfo);

    if (select->tmp_sub->kindofwTable & iTABLE_AGGREGATION &&
            select->planquerydesc.querydesc.groupby.len == 0)
        select->tmp_sub->tstatus = TSS_EXECUTED;

    return ret;
}

int
make_tmp_select(int *handle, T_SELECT * select, char *wTable)
{
    T_PLANNEDQUERYDESC *plan;
    T_LIST_SELECTION *selection;
    T_POSTFIXELEM *elem;

    SYSTABLE_T tableinfo;
    SYSFIELD_T *fieldinfo = NULL;

    int ret = RET_SUCCESS;
    int i, j;

    plan = &select->planquerydesc;
    plan->querydesc.querytype = SQT_NORMAL;

    plan->querydesc.fromlist.list = (T_JOINTABLEDESC *)
            PMEM_ALLOC(sizeof(T_JOINTABLEDESC));
    if (plan->querydesc.fromlist.list == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto process_error;
    }

    ret = dbi_Schema_GetTableFieldsInfo(*handle, wTable, &tableinfo,
            &fieldinfo);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        goto process_error;
    }

    plan->querydesc.fromlist.max = 1;
    plan->querydesc.fromlist.len = 1;
    plan->querydesc.fromlist.list[0].table.tablename = PMEM_STRDUP(wTable);
    if (!plan->querydesc.fromlist.list[0].table.tablename)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto process_error;
    }
    plan->querydesc.fromlist.list[0].table.aliasname = NULL;
    plan->querydesc.fromlist.list[0].table.correlated_tbl = NULL;
    plan->querydesc.fromlist.list[0].join_type = SJT_NONE;
    plan->querydesc.fromlist.list[0].condition.len = 0;
    plan->querydesc.fromlist.list[0].condition.max = 0;
    plan->querydesc.fromlist.list[0].condition.list = NULL;
    plan->querydesc.fromlist.list[0].scan_hint.scan_type = SMT_NONE;
    plan->querydesc.fromlist.list[0].scan_hint.indexname = NULL;
    plan->querydesc.fromlist.list[0].scan_hint.fields.max = 0;
    plan->querydesc.fromlist.list[0].scan_hint.fields.len = 0;
    plan->querydesc.fromlist.list[0].scan_hint.fields.list = NULL;
    sc_memcpy(&(plan->querydesc.fromlist.list[0].tableinfo), &tableinfo,
            sizeof(SYSTABLE_T));
    plan->querydesc.fromlist.list[0].fieldinfo = NULL;
    plan->querydesc.fromlist.list[0].expr_list = NULL;

    selection = &plan->querydesc.selection;
    selection->list = PMEM_XCALLOC(sizeof(T_SELECTION) * tableinfo.numFields);
    selection->len = tableinfo.numFields;
    selection->max = tableinfo.numFields;
    for (i = 0; i < tableinfo.numFields; i++)
    {
        sc_strcpy(selection->list[i].res_name, fieldinfo[i].fieldName);
        selection->list[i].expr.len = 1;
        selection->list[i].expr.max = 1;
        selection->list[i].expr.list =
                sql_mem_get_free_postfixelem_list(NULL, 1);
        if (selection->list[i].expr.list == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return RET_ERROR;
        }
        selection->list[i].expr.list[0] = sql_mem_get_free_postfixelem(NULL);
        if (selection->list[i].expr.list[0] == NULL)
        {
            sql_mem_set_free_postfixelem_list(NULL,
                    selection->list[i].expr.list);
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return RET_ERROR;
        }

        sc_memset(selection->list[i].expr.list[0], 0x00,
                sizeof(T_POSTFIXELEM));

        elem = selection->list[i].expr.list[0];
        elem->elemtype = SPT_VALUE;
        elem->u.value.valueclass = SVC_VARIABLE;
        elem->u.value.valuetype = fieldinfo[i].type;

        if (IS_ALLOCATED_TYPE(elem->u.value.valuetype))
        {
            elem->u.value.u.ptr = NULL;
        }

        elem->u.value.attrdesc.table.tablename = PMEM_STRDUP(wTable);
        if (!elem->u.value.attrdesc.table.tablename)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            goto process_error;
        }
        elem->u.value.attrdesc.table.aliasname = NULL;

        elem->u.value.attrdesc.attrname = PMEM_STRDUP(fieldinfo[i].fieldName);
        if (!elem->u.value.attrdesc.attrname)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            goto process_error;
        }
        if (set_attrinfobyname(-1, &elem->u.value.attrdesc, &fieldinfo[i]) ==
                RET_ERROR)
        {
            return SQL_E_ERROR;
        }
    }

    for (i = 0; i < plan->querydesc.orderby.len; ++i)
    {
        if (plan->querydesc.orderby.list[i].s_ref > -1)
        {
            j = plan->querydesc.orderby.list[i].s_ref;
            PMEM_FREE(plan->querydesc.orderby.list[i].attr.table.tablename);
            PMEM_FREENUL(plan->querydesc.orderby.list[i].attr.table.aliasname);

            plan->querydesc.orderby.list[i].attr.table.tablename =
                    PMEM_STRDUP(wTable);
            if (!plan->querydesc.orderby.list[i].attr.table.tablename)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                goto process_error;
            }
            plan->querydesc.orderby.list[i].attr.attrname =
                    PMEM_STRDUP(fieldinfo[j].fieldName);
            if (!plan->querydesc.orderby.list[i].attr.attrname)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                goto process_error;
            }
            plan->querydesc.orderby.list[i].attr.posi.ordinal =
                    fieldinfo[j].position;
            plan->querydesc.orderby.list[i].attr.posi.offset =
                    fieldinfo[j].offset;
            plan->querydesc.orderby.list[i].attr.type = fieldinfo[j].type;
            plan->querydesc.orderby.list[i].attr.len = fieldinfo[j].length;
            plan->querydesc.orderby.list[i].attr.Htable = fieldinfo[j].tableId;
            plan->querydesc.orderby.list[i].attr.Hattr = fieldinfo[j].fieldId;
            plan->querydesc.orderby.list[i].attr.collation =
                    fieldinfo[j].collation;
        }

    }

    plan->nest[0].table.tablename =
            plan->querydesc.fromlist.list[0].table.tablename;
    plan->nest[0].table.aliasname = NULL;
    plan->nest[0].indexname = NULL;

    select->rttable[0].table.tablename =
            plan->querydesc.fromlist.list[0].table.tablename;
    select->rttable[0].table.aliasname = NULL;

    select->rttable[0].rec_values =
            init_rec_values(&select->rttable[0], &tableinfo, fieldinfo, 1);

    if (select->rttable[0].rec_values == NULL)
    {
        goto process_error;
    }

    select->queryresult.include_rid = NULL_OID;

    select->queryresult.list = (T_RESULTCOLUMN *)
            PMEM_XCALLOC(sizeof(T_RESULTCOLUMN) * (tableinfo.numFields + 1));
    if (select->queryresult.list == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto process_error;
    }

    select->queryresult.len = tableinfo.numFields;

    for (i = 0; i < select->queryresult.len; i++)
    {
        init_value_attrdesc(&select->queryresult.list[i].value.attrdesc,
                &fieldinfo[i]);

        sc_memcpy(select->queryresult.list[i].res_name,
                fieldinfo[i].fieldName, FIELD_NAME_LENG - 1);
        if (fieldinfo[i].type == DT_DECIMAL)
        {
            select->queryresult.list[i].len = CSIZE_DECIMAL;
        }
        else if (fieldinfo[i].type == DT_DATETIME)
        {
            select->queryresult.list[i].len = MAX_DATETIME_STRING_LEN - 1;
        }
        else if (fieldinfo[i].type == DT_DATE)
        {
            select->queryresult.list[i].len = MAX_DATE_STRING_LEN - 1;
        }
        else if (fieldinfo[i].type == DT_TIME)
        {
            select->queryresult.list[i].len = MAX_TIME_STRING_LEN - 1;
        }
        else if (fieldinfo[i].type == DT_TIMESTAMP)
        {
            select->queryresult.list[i].len = MAX_TIMESTAMP_STRING_LEN - 1;
        }
        else
        {
            select->queryresult.list[i].len = fieldinfo[i].length;
        }
        select->queryresult.list[i].value.valueclass = SVC_VARIABLE;
        select->queryresult.list[i].value.valuetype = fieldinfo[i].type;
        select->queryresult.list[i].value.call_free = 0;
    }

    plan->querydesc.limit.offset = 0;
    plan->querydesc.limit.rows = -1;

    plan->querydesc.limit.start_at = NULL_OID;

    select->wTable = PMEM_STRDUP(wTable);
    if (!select->wTable)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto process_error;
    }

    set_rec_values_info_select(*handle, select);

    PMEM_FREE(fieldinfo);

    return RET_SUCCESS;

  process_error:

    PMEM_FREE(fieldinfo);

    return RET_ERROR;
}

extern int close_all_open_tables(int, T_SELECT *);
extern int make_work_table(int *handle, T_SELECT * select);

static int
insert_into_settable(int *handle, T_SELECT * select, int Htable,
        int comp_Htable, SYSTABLE_T * tableinfo, SYSFIELD_T * fieldinfo,
        T_OPTYPE type)
{
    T_SELECT *fetch_select;
    T_VALUEDESC *val;

    char *rec_buf = NULL;
    int i, ret_fetch;
    int ret = RET_SUCCESS;
    int nullflag_offset, rec_buf_len = 0;
    OID inserted_rid;

    rec_buf_len = get_recordlen(*handle, NULL, tableinfo);
    if (rec_buf_len < 0)
    {
        goto process_error;
    }

    rec_buf = PMEM_ALLOC(rec_buf_len);
    if (rec_buf == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto process_error;
    }

    nullflag_offset = get_nullflag_offset(*handle, NULL, tableinfo);

    ret = make_work_table(handle, select);
    if (ret == RET_ERROR)
    {
        goto process_error;
    }

    fetch_select = (select->tmp_sub) ? select->tmp_sub : select;

    while (1)
    {
        ret_fetch = SQL_fetch(handle, fetch_select, NULL);
        if (ret_fetch == RET_END)
        {
            break;
        }
        else if (ret_fetch == RET_ERROR)
        {
            ret = ret_fetch;
            goto process_error;
        }

        sc_memset(rec_buf, 0, rec_buf_len);

        for (i = 0; i < fetch_select->queryresult.len; i++)
        {
            val = &fetch_select->queryresult.list[i].value;

            if (val->is_null)
            {
                DB_VALUE_SETNULL(rec_buf, fieldinfo[i].position,
                        nullflag_offset);
            }
            else
            {
                ret = copy_value_into_record(*handle,
                        rec_buf + fieldinfo[i].offset, val, &fieldinfo[i], 0);
                if (ret == RET_ERROR)
                {
                    goto process_error;
                }

                DB_VALUE_SETNOTNULL(rec_buf, fieldinfo[i].position,
                        nullflag_offset);
            }
        }

        if (type == SOT_UNION_ALL)
        {
            ret = dbi_Cursor_Insert(*handle, Htable, rec_buf, rec_buf_len,
                    0, &inserted_rid);
            if (ret < 0)
            {
                if (ret != DB_E_DUPUNIQUEKEY)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    goto process_error;
                }
                ret = RET_SUCCESS;
            }

        }
        else
        {
            /* except와 intersect는 select된 결과가 비교 select 구문에
               존재 여부에 따라 record가 temp table에 insert에 됨 */
            if (comp_Htable > -1 &&
                    (type == SOT_INTERSECT || type == SOT_EXCEPT))
            {
                ret = dbi_Cursor_Exist_by_Record(*handle, comp_Htable, rec_buf,
                        rec_buf_len);
                if (ret < 0)
                {
                    if (ret != DB_E_DUPDISTRECORD)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                        goto process_error;
                    }

                    ret = DB_SUCCESS;

                    if (type == SOT_EXCEPT)
                    {
                        continue;
                    }
                }
                else if (type == SOT_INTERSECT)
                {
                    continue;
                }
            }

            ret = dbi_Cursor_Distinct_Insert(*handle, Htable, rec_buf,
                    rec_buf_len);
            if (ret < 0)
            {
                if (ret != DB_E_DUPDISTRECORD)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    goto process_error;
                }
                ret = RET_SUCCESS;
            }
        }

        if (ret_fetch == RET_END_USED_DIRECT_API)
        {
            break;
        }
    }

  process_error:

    PMEM_FREENUL(rec_buf);

    return ret;
}

static int
create_set_temp_table(int *handle, char *wTable, int numFields,
        FIELDDESC_T * fielddesc)
{
    char wIndex[INDEX_NAME_LENG];
    int ret;

    ret = dbi_Trans_NextTempName(*handle, 1, wTable, NULL);
    if (ret < 0)
    {
        return ret;
    }

    ret = dbi_Relation_Create(*handle, wTable, numFields, fielddesc,
            _CONT_TEMPTABLE, _USER_USER, 0, NULL);
    if (ret < 0)
    {
        return ret;
    }

    if (numFields <= MAX_KEY_FIELD_NO)
    {
        ret = dbi_Trans_NextTempName(*handle, 0, wIndex, NULL);
        if (ret < 0)
        {
            return ret;
        }

        ret = dbi_Index_Create(*handle, wTable, wIndex, numFields,
                fielddesc, UNIQUE_INDEX_BIT);
        if (ret < 0)
        {
            return ret;
        }
    }

    return RET_SUCCESS;
}

static int
make_settree_result(int *handle, T_NODE * node,
        T_EXPRESSIONDESC * set_list, int Htable, SYSTABLE_T * tableinfo,
        SYSFIELD_T * fieldinfo, FIELDDESC_T * fielddesc,
        T_PARSING_MEMORY * parsing_memory, T_OPTYPE type)
{
    T_SELECT *select = NULL;

    char comp_wTable[REL_NAME_LENG];
    char wTable[REL_NAME_LENG];

    int ret = RET_SUCCESS;
    int temp_Htable = -1;
    int check_create_tmp = 0;
    int comp_Htable = -1;
    int check_create_comp = 0;
    int set_Htable = -1;
    T_POSTFIXELEM *elem;

    elem = set_list->list[node->start];

    if ((type == SOT_UNION_ALL && node->type != SOT_UNION_ALL)
            || (node->right &&
                    set_list->list[node->right->start]->elemtype !=
                    SPT_SUBQUERY))
    {
        check_create_tmp = 1;

        ret = create_set_temp_table(handle, wTable, tableinfo->numFields,
                fielddesc);
        if (ret < 0)
        {
            goto process_error;
        }

        temp_Htable = dbi_Cursor_Open(*handle, wTable, NULL, NULL,
                NULL, NULL, LK_EXCLUSIVE, 0);
        if (temp_Htable < 0)
        {
            ret = temp_Htable;
            goto process_error;
        }

        set_Htable = temp_Htable;
    }
    else
    {
        set_Htable = Htable;
    }

    /* intersect와 except의 경우 기준 select와 비교 select의 위치가
       변경 되었으며 비교 select 구문이 먼저 처리되어야 한다. */
    if (node->type == SOT_INTERSECT || node->type == SOT_EXCEPT)
    {
        check_create_comp = 1;

        ret = create_set_temp_table(handle, comp_wTable, tableinfo->numFields,
                fielddesc);
        if (ret < 0)
        {
            goto process_error;
        }

        comp_Htable = dbi_Cursor_Open(*handle, comp_wTable, NULL, NULL,
                NULL, NULL, LK_EXCLUSIVE, 0);
        if (comp_Htable < 0)
        {
            ret = comp_Htable;
            goto process_error;
        }

        set_Htable = comp_Htable;
    }

    if (node->left)
    {
        elem = set_list->list[node->left->start];
        if (elem->elemtype == SPT_SUBQUERY)
        {
            ret = insert_into_settable(handle, elem->u.subq, set_Htable, -1,
                    tableinfo, fieldinfo, node->type);
        }
        else
        {
            ret = make_settree_result(handle, node->left, set_list,
                    set_Htable, tableinfo, fieldinfo, fielddesc,
                    parsing_memory, node->type);
        }
        if (ret < 0)
        {
            goto process_error;
        }
    }

    if (check_create_tmp)
    {
        set_Htable = temp_Htable;
    }
    else
    {
        set_Htable = Htable;
    }

    if (node->right)
    {
        elem = set_list->list[node->right->start];
        if (elem->elemtype == SPT_SUBQUERY)
        {
            ret = insert_into_settable(handle, elem->u.subq, set_Htable,
                    comp_Htable, tableinfo, fieldinfo, node->type);
        }
        else
        {
            ret = make_settree_result(handle, node->right, set_list,
                    set_Htable, tableinfo, fieldinfo, fielddesc,
                    parsing_memory, node->type);
        }
        if (ret < 0)
        {
            goto process_error;
        }

    }

    if (check_create_tmp)
    {
        dbi_Cursor_Close(*handle, temp_Htable);

        select = (T_SELECT *) PMEM_ALLOC(sizeof(T_SELECT));
        if (select == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            goto process_error;
        }

        ret = make_tmp_select(handle, select, wTable);
        if (ret != RET_SUCCESS)
        {
            goto process_error;
        }

        ret = insert_into_settable(handle, select, Htable, comp_Htable,
                tableinfo, fieldinfo, node->type);
    }

  process_error:

    if (check_create_comp)
    {
        dbi_Cursor_Close(*handle, comp_Htable);
    }

    if (check_create_tmp)
    {
        if (temp_Htable >= 0)
        {
            dbi_Cursor_Close(*handle, temp_Htable);
        }

        if (select)
        {
            close_all_open_tables(*handle, select);

            if (select->wTable)
            {
                dbi_Relation_Drop(*handle, select->wTable);
            }

            SQL_cleanup_subselect(*handle, select, parsing_memory);
            PMEM_FREENUL(select);
        }
    }

    return ret;
}

/*****************************************************************************/
//! function_name

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name :
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
make_set_work_table(int *handle, T_SELECT * select)
{
    T_SELECT *new_select = NULL;
    T_PLANNEDQUERYDESC *plan;
    T_PLANNEDQUERYDESC *newplan;
    T_QUERYRESULT *qres;

    FIELDDESC_T *fielddesc = NULL;
    FIELDDESC_T *fielddesc2 = NULL;
    SYSTABLE_T tableinfo;
    SYSFIELD_T *fieldinfo = NULL;

    char *wTable;
    char tmp_Table[REL_NAME_LENG];
    char uIndex[INDEX_NAME_LENG];
    char col_name[FIELD_NAME_LENG];

    int ret;
    int i;
    int Htable = -1;
    int value_len, value_scale, value_fixedlen;
    MDB_COL_TYPE collation;
    T_NODE *tree = NULL;
    T_POSTFIXELEM *elem;
    DB_BOOL is_retemp = DB_FALSE;
    DB_BOOL is_recreate = DB_FALSE;

    plan = &select->planquerydesc;
    qres = &plan->querydesc.setlist.list[0]->u.subq->queryresult;

    for (i = 0; i < plan->querydesc.setlist.len; ++i)
    {
        elem = plan->querydesc.setlist.list[i];
        if (elem->elemtype == SPT_SUBQUERY)
        {
            ret = make_work_table(handle, elem->u.subq);
            if (ret < 0)
            {
                return RET_ERROR;
            }

            if (elem->u.subq->tmp_sub && elem->u.subq->tmp_sub->is_retemp)
            {
                is_retemp = DB_TRUE;
            }
        }
    }

    if (select->tmp_sub)
    {
        ret = dbi_Schema_GetTableInfo(*handle, select->tmp_sub->wTable,
                &tableinfo);
        if (ret < 0)
        {
            if (ret != DB_E_UNKNOWNTABLE)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                return RET_ERROR;
            }

            is_recreate = DB_TRUE;
        }
        else if (ret == DB_SUCCESS)
        {
            if (select->param_count == 0 && is_retemp == DB_FALSE)
            {
                return RET_SUCCESS;
            }

            Truncate_Table(select->tmp_sub->wTable);
            is_recreate = DB_FALSE;
        }
    }

    tree = trans_setlist2tree(&plan->querydesc.setlist);
    if (tree == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, er_errid(), 0);
        return RET_ERROR;
    }

    if (select->tmp_sub && !is_recreate)
    {
        wTable = select->tmp_sub->wTable;
    }
    else
    {
        if (is_recreate)
        {
            wTable = select->tmp_sub->wTable;
        }
        else
        {
            wTable = tmp_Table;

            ret = dbi_Trans_NextTempName(*handle, 1, wTable, NULL);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
                goto process_error;
            }
        }

        fielddesc = PMEM_ALLOC(sizeof(FIELDDESC_T) * (qres->len));
        if (fielddesc == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return SQL_E_OUTOFMEMORY;
        }

        /* result field information generation */
        for (i = 0; i < qres->len; i++)
        {
            sc_snprintf(col_name, FIELD_NAME_LENG, "%s%d",
                    INTERNAL_FIELD_PREFIX, i);
            value_len = value_scale = value_fixedlen = 0;
            ret = get_fields_info_for_temp(&qres->list[i], NULL, NULL,
                    &value_len, &value_scale, &collation, &value_fixedlen);
            if (ret == RET_ERROR)
            {
                goto process_error;
            }

            dbi_FieldDesc(col_name, qres->list[i].value.valuetype,
                    value_len, value_scale, NULL_BIT, 0, value_fixedlen,
                    &(fielddesc[i]), collation);
        }

        ret = dbi_Relation_Create(*handle, wTable, i, fielddesc,
                _CONT_TEMPTABLE, _USER_USER, 0, NULL);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            goto process_error;
        }

        if (tree->type != SOT_UNION_ALL && qres->len <= MAX_KEY_FIELD_NO)
        {
            /* create distinct index */
            ret = dbi_Trans_NextTempName(*handle, 0, uIndex, NULL);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                goto process_error;
            }

            ret = dbi_Index_Create(*handle, wTable, uIndex, qres->len,
                    fielddesc, UNIQUE_INDEX_BIT);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                goto process_error;
            }
        }

        if (plan->querydesc.orderby.len > 0)
        {
            T_ORDERBYDESC *orderby = &plan->querydesc.orderby;

            ret = dbi_Trans_NextTempName(*handle, 0, uIndex, NULL);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                goto process_error;
            }

            fielddesc2 = PMEM_ALLOC(sizeof(FIELDDESC_T) * (orderby->len));
            if (fielddesc2 == NULL)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return SQL_E_OUTOFMEMORY;
            }

            for (i = 0; i < orderby->len; i++)
            {
                sc_snprintf(col_name, FIELD_NAME_LENG, "%s%d",
                        INTERNAL_FIELD_PREFIX, orderby->list[i].s_ref);

                if (orderby->list[i].ordertype == 'D')
                {
                    dbi_FieldDesc(col_name, 0, 0, 0, FD_DESC, 0,
                            FIXED_VARIABLE, &(fielddesc2[i]),
                            orderby->list[i].attr.collation);
                }
                else
                {
                    dbi_FieldDesc(col_name, 0, 0, 0, 0, 0, FIXED_VARIABLE,
                            &(fielddesc2[i]), orderby->list[i].attr.collation);
                }
            }

            ret = dbi_Index_Create(*handle, wTable, uIndex, orderby->len,
                    fielddesc2, 0);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                goto process_error;
            }
        }

    }

    ret = dbi_Schema_GetTableFieldsInfo(*handle, wTable, &tableinfo,
            &fieldinfo);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        goto process_error;
    }

    Htable = dbi_Cursor_Open(*handle, wTable, NULL, NULL, NULL, NULL,
            LK_EXCLUSIVE, 0);

    ret = make_settree_result(handle, tree, &plan->querydesc.setlist, Htable,
            &tableinfo, fieldinfo, fielddesc, select->main_parsing_chunk,
            tree->type);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        goto process_error;
    }

    dbi_Cursor_Close(*handle, Htable);

    if (select->tmp_sub)
    {
        new_select = select->tmp_sub;
    }
    else
    {
        new_select = (T_SELECT *) PMEM_ALLOC(sizeof(T_SELECT));
        if (new_select == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return SQL_E_OUTOFMEMORY;
        }
        select->tmp_sub = new_select;

        sc_memset(new_select, 0, sizeof(T_SELECT));

        ret = make_tmp_select(handle, new_select, wTable);
        if (ret != RET_SUCCESS)
        {
            goto process_error;
        }

        newplan = &new_select->planquerydesc;

        if (plan->querydesc.orderby.len > 0)
        {
            newplan->nest[0].indexname = PMEM_STRDUP(uIndex);
            if (!newplan->nest[0].indexname)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                goto process_error;
            }
        }

        newplan->querydesc.limit.rows = plan->querydesc.limit.rows;
        newplan->querydesc.limit.offset = plan->querydesc.limit.offset;
    }

    PMEM_FREENUL(fieldinfo);
    PMEM_FREENUL(fielddesc2);
    PMEM_FREENUL(fielddesc);

    free_tree(tree);

    return RET_SUCCESS;

  process_error:

    PMEM_FREENUL(fieldinfo);
    PMEM_FREENUL(fielddesc2);
    PMEM_FREENUL(fielddesc);

    if (Htable >= 0)
    {
        dbi_Cursor_Close(*handle, Htable);
    }

    PMEM_FREENUL(fieldinfo);

    if (tree)
    {
        free_tree(tree);
    }

    return RET_ERROR;
}

/* select 구조체 내 fromlist에 해당하는 table의 변경 정보확인 */
static int
check_table_modifiedtimes(T_SELECT * select)
{
    int is_modified = 0;
    int m_times;
    int i;
    T_JOINTABLEDESC *join;

    join = select->planquerydesc.querydesc.fromlist.list;
    for (i = 0; i < select->planquerydesc.querydesc.fromlist.len; ++i)
    {
        if (join[i].table.tablename == NULL)
        {
            continue;
        }

        /* temp table의 경우 원본 table의 변경 정보 확인 */
        if (isTempTable_name(join[i].table.tablename))
        {
            if (!join[i].table.correlated_tbl)
            {
                continue;
            }

            is_modified =
                    check_table_modifiedtimes(join[i].table.correlated_tbl);
            if (is_modified < 0)
            {
                return is_modified;
            }
        }
        else
        {
            /* 원본 table의 변경 정보 확인 */
            m_times = _Schema_GetTable_modifiedtimes(join[i].table.tablename);
            if (m_times < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_UNKNOWNTABLE, 0);
                return RET_ERROR;
            }

            if (join[i].modified_times != m_times)
            {
                is_modified = 1;
            }
        }

        if (is_modified)
        {
            break;
        }

    }

    return is_modified;
}

/* 변경된 원본 table의 변경 정보를 새로 설정 */
static int
set_table_modifiedtimes(T_SELECT * select)
{
    int m_times;
    int i;
    T_JOINTABLEDESC *join;

    join = select->planquerydesc.querydesc.fromlist.list;
    for (i = 0; i < select->planquerydesc.querydesc.fromlist.len; ++i)
    {
        if (join[i].table.tablename != NULL &&
                !isTempTable_name(join[i].table.tablename))
        {
            m_times = _Schema_GetTable_modifiedtimes(join[i].table.tablename);
            if (m_times < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_UNKNOWNTABLE, 0);
                return RET_ERROR;
            }

            join[i].modified_times = m_times;
        }
    }

    return RET_SUCCESS;
}

static int
make_work_temp_table_or_index(int *handle, T_SELECT * select)
{
    int i, j, ret;
    T_JOINTABLEDESC *join;
    T_PLANNEDQUERYDESC *plan;
    T_RTTABLE *rttable;
    T_SELECT *sub_select;
    int ord;
    DB_BOOL is_modified = DB_FALSE;

    if (select->tmp_sub)
    {
        select->tmp_sub->is_retemp = 0;
    }

    plan = &(select->planquerydesc);
    rttable = select->rttable;
    join = plan->querydesc.fromlist.list;
    for (i = 0; i < plan->querydesc.fromlist.len; i++)
    {
        ord = rttable[i].table_ordinal;
        /* from 절이 없는 select인 경우 skip */
        if (join[ord].table.tablename == NULL)
        {
            continue;
        }

        if (isTempTable_name(join[ord].table.tablename))
        {
            SYSTABLE_T tableinfo;
            SYSFIELD_T *fieldinfo;
            int fieldnum;

            ret = dbi_Schema_GetTableInfo(*handle,
                    join[ord].table.tablename, &tableinfo);
            /* temp table이 존재하는 경우 변경 여부를 확인하여 재사용 결정 */
            if (ret != DB_E_UNKNOWNTABLE)
            {
                /* union에 의한 temp table인 경우 skip */
                if (!join[ord].table.correlated_tbl)
                {
                    continue;
                }

                /* subquery 또는 view인 경우 원본 table의 변경 정보 확인 */
                ret = check_table_modifiedtimes(join[ord].table.
                        correlated_tbl);
                if (ret < 0)
                {
                    return RET_ERROR;
                }

                if (!ret)
                {
                    continue;
                }

                /* table이 변경된 경우 temp table truncate */
                Truncate_Table(join[ord].table.tablename);

                is_modified = DB_TRUE;
            }
            else
            {
                ret = recreate_subquery_table(handle, &join[ord],
                        join[ord].table.tablename);
                if (ret != RET_SUCCESS)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
                    return RET_ERROR;
                }
                fieldnum = dbi_Schema_GetTableFieldsInfo(*handle,
                        join[ord].table.tablename, &tableinfo, &fieldinfo);
                if (fieldnum < 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, fieldnum, 0);
                    return RET_ERROR;
                }
                sc_memcpy(&(plan->querydesc.fromlist.list[ord].tableinfo),
                        &tableinfo, sizeof(SYSTABLE_T));
                sc_memcpy(plan->querydesc.fromlist.list[ord].fieldinfo,
                        fieldinfo, sizeof(SYSFIELD_T) * tableinfo.numFields);

                for (j = 0; j < tableinfo.numFields; j++)
                {
                    rttable[i].rec_values->fld_value[j].offset_ =
                            fieldinfo[j].offset;
                    rttable[i].rec_values->fld_value[j].h_offset_ =
                            fieldinfo[j].h_offset;
                    rttable[i].rec_values->fld_value[j].fixed_part =
                            fieldinfo[j].fixed_part;
                }
                PMEM_FREENUL(fieldinfo);
            }

            sub_select = select->first_sub;
            while (sub_select)
            {
                if (sub_select->kindofwTable & iTABLE_SUBSELECT &&
                        rttable[sub_select->rttable_idx].status == SCS_CLOSE
                        && !mdb_strcmp(join[ord].table.tablename,
                                rttable[sub_select->rttable_idx].table.
                                tablename))
                {
                    rttable[sub_select->rttable_idx].status =
                            SCS_INITIAL_VACANCY;
                    sub_select->tstatus = TSS_RAW;
                    break;
                }
                sub_select = sub_select->sibling_query;
            }

        }
        else
        {
            /* temp table이 아닌 원본 table 역시 변경 여부 확인 */
            ret = _Schema_GetTable_modifiedtimes(join[ord].table.tablename);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_UNKNOWNTABLE, 0);
                return RET_ERROR;
            }

            if (join[ord].modified_times != ret)
            {
                is_modified = DB_TRUE;
            }
        }
        if (plan->nest[ord].indexname != NULL &&
                isTempTable_name(plan->nest[ord].indexname))
        {
            SYSINDEX_T indexsinfo;
            FIELDDESC_T *fieldDesc;
            MDB_UINT8 _type = 0;

            ret = dbi_Schema_GetIndexInfo(*handle, plan->nest[ord].indexname,
                    &indexsinfo, NULL, NULL);
            if (ret != DB_E_UNKNOWNINDEX)
            {
                continue;
            }

            fieldDesc =
                    PMEM_ALLOC(sizeof(FIELDDESC_T) *
                    plan->nest[ord].index_field_num);
            if (fieldDesc == NULL)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return RET_ERROR;
            }

            for (j = 0; j < plan->nest[ord].index_field_num; j++)
            {
                if (plan->nest[ord].index_field_info[j].order == 'D')
                {
                    _type |= FD_DESC;
                }
                else
                {
                    _type = 0;
                }

                dbi_FieldDesc(plan->nest[ord].index_field_info[j].attrname,
                        0, 0, 0, _type, 0, -1, &(fieldDesc[j]),
                        plan->nest[ord].index_field_info[j].collation);
            }
            ret = dbi_Index_Create_With_KeyInsCond(*handle,
                    plan->nest[ord].table.tablename, plan->nest[ord].indexname,
                    plan->nest[ord].index_field_num,
                    fieldDesc, 0, plan->nest[ord].keyins_flag,
                    plan->nest[ord].scandesc);

            PMEM_FREENUL(fieldDesc);

            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                return RET_ERROR;
            }
        }
    }

    sub_select = select->first_sub;
    while (sub_select)
    {
        if (sub_select->kindofwTable & iTABLE_SUBSELECT &&
                rttable[sub_select->rttable_idx].status != SCS_INITIAL_VACANCY
                && select->param_count > 0)
        {
            rttable[sub_select->rttable_idx].status = SCS_INITIAL_VACANCY;
            sub_select->tstatus = TSS_RAW;
            Truncate_Table(rttable[sub_select->rttable_idx].table.tablename);

            if (exec_insert_subquery(*handle, sub_select,
                            rttable + sub_select->rttable_idx,
                            select->main_parsing_chunk) < 0)
                return RET_ERROR;
        }
        else if (sub_select->kindofwTable & iTABLE_SUBSELECT &&
                rttable[sub_select->rttable_idx].status == SCS_INITIAL_VACANCY)
        {
            if (sub_select->tstatus == TSS_EXECUTED)
            {
                rttable[sub_select->rttable_idx].status = SCS_CLOSE;
            }
            else if (exec_insert_subquery(*handle, sub_select,
                            rttable + sub_select->rttable_idx,
                            select->main_parsing_chunk) < 0)
            {
                return RET_ERROR;
            }
        }
        else if (sub_select->kindofwTable & iTABLE_SUBWHERE &&
                sub_select->tstatus == TSS_EXECUTED && select->param_count > 0)
        {
            sub_select->tstatus = TSS_RAW;
            if (sub_select->subq_result_rows)
            {
                iSQL_ROWS *result_rows = sub_select->subq_result_rows;

                while (result_rows)
                {
                    sub_select->subq_result_rows = result_rows->next;

                    if (result_rows->data)
                        PMEM_FREENUL(result_rows->data);

                    PMEM_FREENUL(result_rows);
                    result_rows = sub_select->subq_result_rows;
                }
            }
            sub_select->subq_result_rows = NULL;
        }
        sub_select = sub_select->sibling_query;
    }

    /* fromlist에 포함된 table중 하나라도 변경된 경우 새로 설정 */
    if (is_modified)
    {
        ret = set_table_modifiedtimes(select);
        if (ret < 0)
        {
            return RET_ERROR;
        }

        /* tmp_sub가 존재하는 경우(orderby,groupby,aggregation)
           tmp_sub의 temp table의 data를 재생성하기 위해 설정 */
        if (select->tmp_sub)
        {
            select->tmp_sub->is_retemp = 1;
        }
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! make_work_table

/*! \breif  SELECT 구문을 수행할 TABLE을 결정한다
 ************************************
 * \param handle(in):
 * \param stmt(in/out):
 * \param subq_wt():
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - TEMP TABLE 생성
 *      TEMP-TABLE은 2가지 경우에 생성된다.
 *      1. GROUP_BY가 존재하는 경우
 *      2. ORDER_BY가 존재하는 경우
 *  - GROUP_BY AND ORDER_BY인 경우 지금은 TEMP TABLE을 2번 생성한다
 *
 *
 *****************************************************************************/
int
make_work_table(int *handle, T_SELECT * select)
{
    int ret;

    ret = make_work_temp_table_or_index(handle, select);
    if (ret < 0)
    {
        return RET_ERROR;
    }

    if (select->planquerydesc.querydesc.setlist.len > 0)
    {
        ret = make_set_work_table(handle, select);
        if (ret < 0)
        {
            return RET_ERROR;
        }
    }
    else if (select->tmp_sub)
    {
        ret = make_work_temp_select(*handle, select);
        if (ret < 0)
        {
            return RET_ERROR;
        }
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! create_subquery_table

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)    :
 * \param tsptr() :
 * \param table_nm():
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *  - A. COLLATION 지원에 따른 dbi_FieldDesc 인터페이스 변경
 *      B. get_value_len_n_scale()을 get_fields_info_for_temp()로 대처
 *****************************************************************************/
int
create_subquery_table(int *handle, T_SELECT * tsptr, char *table_nm)
{
    T_QUERYRESULT *qresult;
    T_RESULTCOLUMN *res_list;
    FIELDDESC_T *fielddesc;
    int fcnt;
    int value_len, value_scale, value_fixedlen;
    int i, j;
    MDB_COL_TYPE collation;

    if (tsptr->planquerydesc.querydesc.setlist.len > 0)
    {
        qresult =
                &tsptr->planquerydesc.querydesc.setlist.list[0]->u.subq->
                queryresult;
        if (tsptr->queryresult.len == qresult->len)
        {
            res_list = tsptr->queryresult.list;
        }
        else
        {
            res_list = qresult->list;
        }
    }
    else
    {
        qresult = &tsptr->queryresult;
        res_list = qresult->list;
    }

    fielddesc = PMEM_ALLOC(sizeof(FIELDDESC_T) * qresult->len);
    if (fielddesc == NULL)
        return SQL_E_OUTOFMEMORY;

    /* result field information generation */
    for (fcnt = 0; fcnt < qresult->len; fcnt++)
    {
        value_len = value_scale = value_fixedlen = 0;

        if (get_fields_info_for_temp(&qresult->list[fcnt], NULL, NULL,
                        &value_len, &value_scale, &collation, &value_fixedlen)
                == RET_ERROR)
        {
            PMEM_FREENUL(fielddesc);
            return RET_ERROR;
        }

        dbi_FieldDesc(res_list[fcnt].res_name,
                qresult->list[fcnt].value.valuetype, value_len, value_scale,
                NULL_BIT, 0, value_fixedlen, &(fielddesc[fcnt]), collation);
    }
    /* check column duplication */
    for (i = 0; i < fcnt - 1; i++)
    {
        for (j = i + 1; j < fcnt; j++)
        {
            if (!mdb_strcmp(fielddesc[i].fieldName, fielddesc[j].fieldName))
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_DUPLICATECOLUMNNAME, 0);
                PMEM_FREENUL(fielddesc);
                return RET_ERROR;
            }
        }
    }
    if (dbi_Relation_Create(*handle, table_nm, fcnt, fielddesc,
                    _CONT_TEMPTABLE, _USER_USER, 0, NULL) < 0)
    {
        PMEM_FREENUL(fielddesc);
        return SQL_E_ERROR;
    }

    PMEM_FREENUL(fielddesc);

    return RET_SUCCESS;
}

/*****************************************************************************/
//! function_name

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name :
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
close_all_open_tables(int handle, T_SELECT * select)
{
    T_RTTABLE *rttable;
    T_NESTING *nest;
    int n_tables;
    int i, ret;

    if (select->tmp_sub)
        close_all_open_tables(handle, select->tmp_sub);
    rttable = select->rttable;
    nest = select->planquerydesc.nest;
    n_tables = select->planquerydesc.querydesc.fromlist.len;

    for (i = 0; i < n_tables; i++)
    {
        if (rttable[i].Hcursor > 0)
        {
            ret = dbi_Cursor_Close(handle, rttable[i].Hcursor);
#ifdef MDB_DEBUG
            if (ret < 0)        // !SUCCESS, SUCCESS == 0, RET_SUCCESS = 1

                printf("%s:%d dbi_Cursor_Close failed\n", __FILE__, __LINE__);
#endif
            rttable[i].status = 0;
        }

        PMEM_FREENUL(nest[i].filter);

        if (rttable[i].before_cursor_posi)
        {
            PMEM_FREENUL(rttable[i].before_cursor_posi);
        }
    }

    return RET_SUCCESS;
}


/*****************************************************************************/
//!  copy_ptr_to_value

/*! \breif  ptr의 값을 val로 설정한다
 ************************************
 * \param val(in/out)   :
 * \param ptr(in)       : memory record's field pointer
 * \param fieldinfo(in) : fields info
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - called : insert_aggr_val_to_tmp_rcrd
 *  - BYTE/VARBYTE TYPE 추가
 *      value length
 *
 *****************************************************************************/
static int
copy_ptr_to_value(T_VALUEDESC * val, char *ptr, SYSFIELD_T * fieldinfo)
{
    switch (fieldinfo->type)
    {
    case DT_TINYINT:
        val->u.i8 = *(tinyint *) ptr;
        break;
    case DT_SMALLINT:
        val->u.i16 = *(smallint *) ptr;
        break;
    case DT_INTEGER:
        val->u.i32 = *(int *) ptr;
        break;
    case DT_BIGINT:
        val->u.i64 = *(bigint *) ptr;
        break;
    case DT_FLOAT:
        val->u.f16 = *(float *) ptr;
        break;
    case DT_DOUBLE:
        val->u.f32 = *(double *) ptr;
        break;
    case DT_DECIMAL:
        char2decimal(&val->u.dec, ptr, fieldinfo->length);
        break;
    case DT_CHAR:
    case DT_NCHAR:
        val->valuetype = fieldinfo->type;

        if (sql_value_ptrXcalloc(val, fieldinfo->length + 1) < 0)
            return SQL_E_OUTOFMEMORY;

        strncpy_func[fieldinfo->type] (val->u.ptr, ptr, fieldinfo->length);
        val->value_len = strlen_func[fieldinfo->type] (val->u.ptr);
        break;
    case DT_VARCHAR:
    case DT_NVARCHAR:
        /* just assign pointer */
        val->u.ptr = ptr;
        val->value_len = strlen_func[fieldinfo->type] (ptr);
        break;

    case DT_BYTE:
    case DT_VARBYTE:   /* just assign pointer */
        val->valuetype = fieldinfo->type;

        if (sql_value_ptrXcalloc(val, fieldinfo->length) < 0)
            return SQL_E_OUTOFMEMORY;

        DB_GET_BYTE(ptr, val->u.ptr, val->value_len);
        break;

    case DT_DATETIME:
        sc_memcpy(&val->u.datetime, ptr, MAX_DATETIME_FIELD_LEN);
        break;
    case DT_DATE:
        sc_memcpy(&val->u.date, ptr, MAX_DATE_FIELD_LEN);
        break;
    case DT_TIME:
        sc_memcpy(&val->u.time, ptr, MAX_TIME_FIELD_LEN);
        break;
    case DT_TIMESTAMP:
        sc_memcpy(&val->u.timestamp, ptr, MAX_TIMESTAMP_FIELD_LEN);
        break;
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
        return SQL_E_ERROR;
    }

    return SQL_E_NOERROR;
}
