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

#include "sql_main.h"
#include "sql_execute.h"
#include "sql_decimal.h"
#include "sql_execute.h"
#include "sql_func_timeutil.h"
#include "mdb_er.h"
#include "ErrorCode.h"

#include "mdb_syslog.h"

extern int make_work_table(int *handle, T_SELECT * select);
extern int evaluate_conjunct(EXPRDESC_LIST * list);
extern int evaluate_expression(T_EXPRESSIONDESC * expr, MDB_BOOL is_orexpr);
extern void set_exprvalue(T_RTTABLE * rttable, T_EXPRESSIONDESC * expr,
        OID rid);
extern void set_exprvalue_for_rttable(T_RTTABLE * rttable,
        T_EXPRESSIONDESC * expr);

extern int calc_simpleexpr(T_OPDESC * op, T_POSTFIXELEM * res,
        T_POSTFIXELEM ** args);

extern int get_avgvalue(T_RESULTCOLUMN * res);

extern void set_correl_attr_values(CORRELATED_ATTR *);
extern int bin_result_record_len(T_QUERYRESULT *, int);
extern int copy_one_binary_column(char *, T_RESULTCOLUMN *, int);
extern int make_single_keyvalue(T_NESTING * nest, T_KEYDESC * key);
extern int delete_single_keyvalue(T_KEYDESC * key);

extern void convert_rec_attr2value(T_ATTRDESC * attr,
        FIELDVALUES_T * rec_values, int position, T_VALUEDESC * val);
extern int copy_rec_values(FIELDVALUES_T * dest_rec_values,
        FIELDVALUES_T * src_rec_values);

static int proceed_nextrecord(int, T_RTTABLE *, int, T_PLANNEDQUERYDESC *);

extern int (*sc_ncollate_func[]) (const char *s1, const char *s2, int n);

/*****************************************************************************/
//!set_keyvalue 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param outer_rttable(in/out) :
 * \param inner_nest(in/out)    : 
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static int
set_keyvalue(T_RTTABLE * outer_rttable, T_NESTING * inner_nest)
{
    T_EXPRESSIONDESC *expr = NULL;
    T_KEY_EXPRDESC *key = NULL;
    T_POSTFIXELEM *elem;
    T_ATTRDESC *attr;
    char *fval;                 /* field value pointer */
    int j, k;
    int stage = 0;

    // NOTES
    // key value가 존재하는 경우, 이미 where condition은 "AND" operator로만
    // 구성되어져있다는 것을 의미한다.
    // 따라서, key member중에 NULL assign되어지는 member가 있다면
    // Cursor를 열필요도 없다.
    // 이 경우, RET_ERROR를 return한다.

  Again:
    stage += 1;
    switch (stage)
    {
    case 1:
        key = inner_nest->min;
        if (key == NULL)
            goto Again;
        break;
    case 2:
        key = inner_nest->max;
        if (key == NULL)
            goto Again;
        break;
    case 3:
        return RET_SUCCESS;
    }

    for (j = 0; j < inner_nest->index_field_num; j++)
    {
        expr = &key[j].expr;
        if (expr->len == 0)
        {
            break;
        }
        for (k = 0; k < expr->len; k++)
        {
            elem = expr->list[k];

            if (elem->elemtype == SPT_OPERATOR)
                continue;
            if (elem->elemtype == SPT_FUNCTION)
                continue;
            if (elem->elemtype == SPT_AGGREGATION)
                continue;
            if (elem->u.value.valueclass == SVC_CONSTANT)
                continue;
            if (elem->u.value.valueclass == SVC_NONE)
                continue;
            if (mdb_strcmp(elem->u.value.attrdesc.table.tablename,
                            outer_rttable->table.tablename))
            {
                continue;
            }

            attr = &elem->u.value.attrdesc;

            if (outer_rttable->rec_values)
            {
                convert_rec_attr2value(attr, outer_rttable->rec_values,
                        attr->posi.ordinal, &elem->u.value);
                continue;
            }
            if (IS_ALLOCATED_TYPE(attr->type))
            {
                if (elem->u.value.valuetype == DT_NULL_TYPE)
                {
                    elem->u.value.valuetype = attr->type;
                    if (sql_value_ptrAlloc(&elem->u.value, attr->len + 1) < 0)
                        return RET_ERROR;
                }
            }
            else
            {   /* Not IS_BS_OR_NBS(attr->type) */
                elem->u.value.valuetype = attr->type;
            }

#ifdef MDB_DEBUG
            if (IS_BYTE(attr->type))
                sc_assert(0, __FILE__, __LINE__);
#endif

            if (DB_VALUE_ISNULL(outer_rttable->rec_buf, attr->posi.ordinal,
                            outer_rttable->nullflag_offset))
            {
                elem->u.value.is_null = 1;
            }
            else
            {
                /* get field value pointer */
                fval = outer_rttable->rec_buf + attr->posi.offset;

                if (elem->u.value.valuetype == DT_DECIMAL)
                {
                    char2decimal(&elem->u.value.u.dec, fval, sc_strlen(fval));
                }
                else if (IS_N_BYTES(elem->u.value.attrdesc.type))
                {
                    memcpy_func[attr->type] (elem->u.value.u.ptr,
                            fval, attr->len);
                }
                else if (IS_N_STRING(elem->u.value.attrdesc.type))
                {
                    strncpy_func[attr->type] (elem->u.value.u.ptr,
                            fval, attr->len);
                }
                else
                {
                    sc_memcpy(&elem->u.value.u, fval, attr->len);
                }

                elem->u.value.is_null = 0;
            }
        }
    }

    goto Again;
}

static int
inner_table_column_binding(T_RTTABLE * rttable, int cur_run,
        T_PLANNEDQUERYDESC * plan)
{
    int n_tables;
    int i, idx, ret;
    T_RTTABLE *cur_rttable;
    EXPRDESC_LIST *expr;

    n_tables = plan->querydesc.fromlist.len;
    cur_rttable = &rttable[cur_run];

    for (i = (cur_run + 1); i < n_tables; i++)
    {
        idx = rttable[i].table_ordinal;
        ret = set_keyvalue(cur_rttable, &plan->nest[idx]);
        if (ret < 0)
            return ret;
        for (expr = rttable[i].db_expr_list; expr; expr = expr->next)
            if (((T_EXPRESSIONDESC *) expr->ptr)->len)
                set_exprvalue(cur_rttable, expr->ptr, NULL_OID);
        for (expr = rttable[i].sql_expr_list; expr; expr = expr->next)
            if (((T_EXPRESSIONDESC *) expr->ptr)->len)
                set_exprvalue(cur_rttable, expr->ptr, NULL_OID);
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_sortmergecondition 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param rttable :
 * \param nest : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - BYTE/VARBYTE 처리 아직 못함.. 
 *  - collation 지원
 *  - collation이 다른 경우.. assert 시켜 버릴것..
 *****************************************************************************/
static int
check_sortmergecondition(T_RTTABLE * rttable, T_NESTING * nest)
{
    T_VALUEDESC *val;
    T_VALUEDESC fld_val;
    int i;
    bigint comp_ret;

    for (i = 0; i < rttable->cnt_merge_item; i++)
    {
        val = &rttable->merge_key_values[i];
        sc_memset(&fld_val, 0, sizeof(T_VALUEDESC));
        convert_rec_attr2value(NULL, rttable->rec_values,
                nest->index_field_info[i].posi.ordinal, &fld_val);
        fld_val.valueclass = SVC_VARIABLE;
        fld_val.attrdesc = nest->index_field_info[i];

        if (sql_compare_values(&fld_val, val, &comp_ret) < 0)
            return -2;

        if (comp_ret != 0)
            return comp_ret;
    }

    return 0;
}

void
make_null_record(FIELDVALUES_T * rec_values)
{
    int fld_idx;

    rec_values->rec_oid = NULL_OID;

    for (fld_idx = 0; fld_idx < rec_values->fld_cnt; fld_idx++)
    {
        rec_values->fld_value[fld_idx].is_null = 1;
    }
}

/*****************************************************************************/
//! use_nestedloop

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)        :
 * \param rttable(in/out)   :
 * \param cur_run(in)       :
 * \param plan(in/out)      :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - BYTE/VARBYTE 처리 아직 못함.. 
 *  - collation 지원
 *  - collation이 다른 경우.. assert 시켜 버릴것..
 *****************************************************************************/
static int
use_nestedloop(int handle, T_RTTABLE * rttable, int cur_run,
        T_PLANNEDQUERYDESC * plan)
{
    register int n_tables, i;
    int next_run = cur_run + 1;
    int r_crs, ret;
    T_RTTABLE *cur_rttable;
    T_NESTING *cur_nest;
    T_JOINTABLEDESC *cur_jointab;
    EXPRDESC_LIST *expr;

    n_tables = plan->querydesc.fromlist.len;
    if (cur_run >= n_tables)
        return RET_END;

    cur_rttable = &rttable[cur_run];
    cur_nest = &plan->nest[cur_rttable->table_ordinal];
    cur_jointab = &plan->querydesc.fromlist.list[cur_rttable->table_ordinal];

  Open_Curosr_Again:

    switch (cur_rttable->status)
    {
    case SCS_OPEN:
        r_crs = RET_SUCCESS;
        break;

    case SCS_FORWARD:
        cur_rttable->status = SCS_OPEN;
        r_crs = RET_START;      /* read next record */
        break;

    case SCS_CLOSE:
        cur_rttable->prev_rec_cnt = 0;
        if (cur_nest->op_in)
            cur_rttable->in_expr_idx += 1;
        r_crs = open_cursor(handle, cur_rttable, cur_nest,
                cur_rttable->db_expr_list, LK_SHARED);
        if (r_crs <= RET_ERROR)
            return r_crs;

        if (r_crs == RET_FALSE) /* no satisfying records */
            r_crs = RET_END;
        else
            r_crs = RET_START;

        if (cur_rttable->jump2offset && r_crs == RET_START)
        {
            if (dbi_Cursor_Seek(handle, cur_rttable->Hcursor,
                            cur_rttable->jump2offset) >= 0)
            {
                cur_rttable->jump2offset = 0;
            }

            while (cur_rttable->jump2offset--)
            {
                if (cur_rttable->rec_values)
                    ret = dbi_Cursor_Read(handle, cur_rttable->Hcursor,
                            NULL,
                            &cur_rttable->rec_len,
                            cur_rttable->rec_values, 1, 0);
                else
                    ret = dbi_Cursor_Read(handle, cur_rttable->Hcursor,
                            cur_rttable->rec_buf,
                            &cur_rttable->rec_len, NULL, 1, 0);
                if (ret < 0)
                {
                    if (ret != DB_E_NOMORERECORDS && ret != DB_E_INDEXCHANGED)
                        return ret;
                    cur_rttable->status = SCS_REOPEN;
                    r_crs = RET_END;
                    break;
                }
            }
        }

        if (cur_rttable->start_at != NULL_OID && r_crs == RET_START)
        {
            ret = dbi_does_table_has_rid(cur_rttable->table.tablename,
                    cur_rttable->start_at);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                return DB_E_INVALID_RID;
            }

            ret = dbi_Cursor_SetPosition_UnderRid(handle,
                    cur_rttable->Hcursor, cur_rttable->start_at);
            if (ret < 0)
            {
                if (ret != DB_E_NOMORERECORDS && ret != DB_E_INDEXCHANGED)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    return ret;
                }
                cur_rttable->status = SCS_REOPEN;
                r_crs = RET_END;
            }
        }
        break;

    case SCS_REOPEN:
        if (!cur_nest->op_in && next_run < n_tables)
        {
            for (i = next_run; i < n_tables; i++)
            {
                if (rttable[i].cnt_merge_item == 0)
                    break;
                if (rttable[i].status != SCS_CLOSE &&
                        rttable[i].status != SCS_REOPEN)
                {
                    /* 마지막 테이블이 아니면서 sortmerge를
                     * 수행한다면, 현재 테이블에 인출할 레코드가
                     * 없으므로 다음 테이블의 수행시 커서를 다시
                     * 열도록 SCS_REOPEN 으로 설정한다. */
                    rttable[i].status = SCS_REOPEN;
                }
            }
        }
        if (cur_nest->op_in)
        {
            if (cur_rttable->in_expr_idx == -1)
                cur_rttable->prev_rec_cnt = 0;
            cur_rttable->in_expr_idx += 1;

            if (cur_rttable->in_expr_idx == cur_nest->in_exprs)
            {
                cur_rttable->in_expr_idx = -1;
                r_crs = RET_END;
                break;
            }
        }
        else
            cur_rttable->prev_rec_cnt = 0;
        r_crs = reopen_cursor(handle, cur_rttable, cur_nest,
                cur_rttable->db_expr_list, LK_SHARED);
        if (r_crs <= RET_ERROR)
            return r_crs;

        if (r_crs == RET_FALSE) /* no satisfying records */
            r_crs = RET_END;
        else
            r_crs = RET_START;
        break;

    default:
        MDB_SYSLOG(("Nested Loop Cursor Status Fail. %d\n",
                        cur_rttable->status));
        return RET_ERROR;
    }

  Read_Record_Again:
    if (r_crs == RET_START || (r_crs == RET_SUCCESS && next_run >= n_tables))
    {
        while (1)
        {
            if (cur_rttable->rec_values)
                ret = dbi_Cursor_Read(handle, cur_rttable->Hcursor,
                        NULL, &cur_rttable->rec_len, cur_rttable->rec_values,
                        1, 0);
            else
                ret = dbi_Cursor_Read(handle, cur_rttable->Hcursor,
                        cur_rttable->rec_buf, &cur_rttable->rec_len, NULL, 1,
                        0);
            if (ret < 0)
            {
                if (ret != DB_E_NOMORERECORDS && ret != DB_E_INDEXCHANGED)
                    return ret;
                cur_rttable->status = SCS_REOPEN;
                if ((ret == DB_E_NOMORERECORDS || ret == DB_E_INDEXCHANGED)
                        && cur_nest->op_in)
                    goto Open_Curosr_Again;
                r_crs = RET_END;
                break;
            }

            if (cur_rttable->sql_expr_list)
            {
                for (expr = cur_rttable->sql_expr_list; expr;
                        expr = expr->next)
                    if (((T_EXPRESSIONDESC *) expr->ptr)->len)
                        set_exprvalue(cur_rttable, expr->ptr, NULL_OID);
                /* sql filter 확인 시 error 발생에 대한 처리를 추가 */
                ret = evaluate_conjunct(cur_rttable->sql_expr_list);
                if (ret < 0)
                {
                    return ret;
                }
                else if (ret == 0)
                {
                    continue;
                }
            }

            /* do column binding */
            if (next_run < n_tables)
            {
                ret = inner_table_column_binding(rttable, cur_run, plan);
                if (ret < 0)
                    return ret;
            }

            cur_rttable->prev_rec_cnt += 1;
            r_crs = RET_SUCCESS;
            break;
        }
    }

    /* r_crs : RET_SUCCESS or RET_END */
    if (r_crs == RET_END)
    {
        if (cur_jointab->join_type == SJT_LEFT_OUTER_JOIN &&
                cur_rttable->prev_rec_cnt == 0)
        {
            /* make null record */
            make_null_record(cur_rttable->rec_values);

            if (next_run < n_tables)
            {
                ret = inner_table_column_binding(rttable, cur_run, plan);
                if (ret < 0)
                    return ret;
            }

            r_crs = RET_NEXT;
            goto Read_Record_Done;
        }

        if (1)
        {
        }

        return RET_END;
    }

  Read_Record_Done:
    /* r_crs : RET_SUCCESS or RET_NEXT */
    if (next_run < n_tables)
    {
        int inner_r_crs;

        inner_r_crs = proceed_nextrecord(handle, rttable, next_run, plan);
        if (inner_r_crs < 0)
        {
            return inner_r_crs;
        }
        if (inner_r_crs == RET_END || inner_r_crs == RET_REPOSITION)
        {
            if (r_crs == RET_SUCCESS)
            {
                r_crs = RET_START;
                goto Read_Record_Again;
            }
            else
            {
                return RET_END;
            }
        }

        if (inner_r_crs == RET_NEXT)
        {
            if (r_crs == RET_SUCCESS)
            {
                cur_rttable->status = SCS_FORWARD;
            }
        }

        if (inner_r_crs == RET_SUCCESS && r_crs == RET_NEXT)
        {
            r_crs = RET_SUCCESS;
        }
    }


    return r_crs;
}

static int
use_sortmerge(int handle, T_RTTABLE * rttable, int cur_run,
        T_PLANNEDQUERYDESC * plan)
{
    int n_tables;
    int next_run = cur_run + 1;
    int r_crs, ret;
    T_RTTABLE *cur_rttable;
    T_NESTING *cur_nest;
    T_JOINTABLEDESC *cur_jointab;
    EXPRDESC_LIST *expr;

    n_tables = plan->querydesc.fromlist.len;
    if (cur_run >= n_tables)
        return RET_END;

    cur_rttable = &rttable[cur_run];
    cur_nest = &plan->nest[cur_rttable->table_ordinal];
    cur_jointab = &plan->querydesc.fromlist.list[cur_rttable->table_ordinal];

    /* open sort merge cursor */
  Open_Cursor_Again:
    switch (cur_rttable->status)
    {
    case SCS_OPEN:
        r_crs = RET_SUCCESS;
        break;

    case SCS_FORWARD:
        cur_rttable->status = SCS_OPEN;
        r_crs = RET_START;
        break;

    case SCS_CLOSE:
        r_crs = open_cursor(handle, cur_rttable, cur_nest, NULL,        /*cur_rttable->db_expr_list */
                LK_SHARED);
        if (r_crs <= RET_ERROR)
            return r_crs;

        if (r_crs == RET_FALSE) /* no satisfying records */
            r_crs = RET_END;
        else
            r_crs = RET_START;
        break;

    case SCS_REOPEN:
        /* clear next and prev record buffer */
        cur_rttable->save_rec_values = NULL;
        cur_rttable->prev_rec_cnt = 0;
        cur_rttable->has_prev_cursor_posi = 0;
        cur_rttable->has_next_record_read = 0;

        r_crs = reopen_cursor(handle, cur_rttable, cur_nest, NULL,      /*cur_rttable->db_expr_list */
                LK_SHARED);
        if (r_crs <= RET_ERROR)
            return r_crs;

        if (r_crs == RET_FALSE) /* no satisfying records */
            r_crs = RET_END;
        else
            r_crs = RET_START;
        break;

    case SCS_REPOSITION:
    case SCS_SM_LAST:
        r_crs = reposition_cursor(handle, cur_rttable, cur_nest);
        if (r_crs <= RET_ERROR)
            return r_crs;

        if (r_crs == RET_FALSE) /* no satisfying records */
        {
            cur_rttable->status = SCS_REOPEN;
            r_crs = RET_END;
        }
        else
            r_crs = RET_START;
        break;

    default:
        MDB_SYSLOG(("Sort Merge Cursor Status Fail. %d\n",
                        cur_rttable->status));
        return RET_ERROR;
    }

  Read_Record_Again:
    if (r_crs == RET_START || (r_crs == RET_SUCCESS && next_run >= n_tables))
    {
        if (cur_rttable->prev_rec_idx >= 0)     /* read prev record */
        {
            if (cur_rttable->prev_rec_idx < cur_rttable->prev_rec_cnt)
            {
                cur_rttable->rec_values =
                        &cur_rttable->prev_rec_values[cur_rttable->
                        prev_rec_idx];
                cur_rttable->prev_rec_idx += 1;
                r_crs = RET_SUCCESS;
                goto Read_Record_Done;
            }

            /* All prev records were accessed */
            cur_rttable->prev_rec_idx = -1;

            /* restore record buffer */
            if (cur_rttable->save_rec_values != NULL &&
                    cur_rttable->save_rec_values != cur_rttable->rec_values)
            {
                cur_rttable->rec_values = cur_rttable->save_rec_values;
                cur_rttable->save_rec_values = NULL;
            }


            if (cur_rttable->has_prev_cursor_posi)
            {
                ret = dbi_Cursor_SetPosition(handle, cur_rttable->Hcursor,
                        cur_rttable->before_cursor_posi);
                if (ret < 0)
                {
                    return ret; /* error handling */
                }

                /* clear next_record_read flag */
                cur_rttable->has_next_record_read = 0;
            }
        }

        while (1)
        {
            if (cur_rttable->has_next_record_read)
            {
                /* restore record buffer */
                if (cur_rttable->save_rec_values != NULL &&
                        cur_rttable->save_rec_values !=
                        cur_rttable->rec_values)
                {
                    cur_rttable->rec_values = cur_rttable->save_rec_values;
                    cur_rttable->save_rec_values = NULL;
                }

                if (cur_jointab->join_type == SJT_LEFT_OUTER_JOIN &&
                        cur_rttable->prev_rec_cnt == 1)
                {
                    ret = copy_rec_values(cur_rttable->rec_values,
                            cur_rttable->prev_rec_values);
                }

                /* clear next_record_read flag */
                cur_rttable->has_next_record_read = 0;
            }
            else        /* The next record does not exist */
            {
                ret = dbi_Cursor_Read(handle, cur_rttable->Hcursor,
                        NULL, &cur_rttable->rec_len,
                        cur_rttable->rec_values, 1, 0);
                if (ret < 0)
                {
                    if (ret != DB_E_NOMORERECORDS && ret != DB_E_INDEXCHANGED)
                        return ret;

                    /* reached to the last point */
                    cur_rttable->status = SCS_SM_LAST;
                    goto Open_Cursor_Again;
                }
            }

            ret = check_sortmergecondition(cur_rttable, cur_nest);
            if (ret < 0)
            {
                /* clear previous record buffer */
                if (rttable->prev_rec_cnt > 0)
                {
                    rttable->prev_rec_cnt = 0;
                    rttable->has_prev_cursor_posi = 0;
                }
                continue;
            }
            else if (ret > 0)
            {
                /* The next record has been read */
                cur_rttable->has_next_record_read = 1;
                cur_rttable->save_rec_values = cur_rttable->rec_values;

                if (cur_jointab->join_type == SJT_LEFT_OUTER_JOIN &&
                        cur_rttable->prev_rec_cnt == 0)
                {
                    /* make null record */
                    ret = copy_rec_values(cur_rttable->prev_rec_values,
                            cur_rttable->rec_values);

                    if (ret < 0)
                        return ret;

                    make_null_record(cur_rttable->rec_values);
                    cur_rttable->prev_rec_cnt = 1;

                    if (next_run < n_tables)
                    {
                        ret = inner_table_column_binding(rttable, cur_run,
                                plan);
                        if (ret < 0)
                            return ret;
                    }

                    r_crs = RET_SUCCESS;
                    goto Read_Record_Done;
                }
                if (cur_rttable->prev_rec_cnt == 0)
                    cur_rttable->status = SCS_REOPEN;
                else
                    cur_rttable->status = SCS_REPOSITION;
                return RET_REPOSITION;
            }

            /* evaluate DB Filters */
            if (cur_rttable->db_expr_list)
            {
                for (expr = cur_rttable->db_expr_list; expr; expr = expr->next)
                    if (((T_EXPRESSIONDESC *) expr->ptr)->len)
                        set_exprvalue(cur_rttable, expr->ptr, NULL_OID);
                if (evaluate_conjunct(cur_rttable->db_expr_list) <= 0)
                    continue;
            }

            /* evaluate SQL Filters */
            if (cur_rttable->sql_expr_list)
            {
                for (expr = cur_rttable->sql_expr_list; expr;
                        expr = expr->next)
                    if (((T_EXPRESSIONDESC *) expr->ptr)->len)
                        set_exprvalue(cur_rttable, expr->ptr, NULL_OID);
                if (evaluate_conjunct(cur_rttable->sql_expr_list) <= 0)
                    continue;
            }

            /* do column binding */
            if (next_run < n_tables)
            {
                ret = inner_table_column_binding(rttable, cur_run, plan);
                if (ret < 0)
                    return ret;
            }

            /* save the current record into prev record buffer */
            if (cur_rttable->prev_rec_cnt < cur_rttable->prev_rec_max_cnt)
            {
                ret = copy_rec_values(&cur_rttable->
                        prev_rec_values[cur_rttable->prev_rec_cnt],
                        cur_rttable->rec_values);

                if (ret < 0)
                    return ret;
                cur_rttable->prev_rec_cnt += 1;
                if (cur_rttable->prev_rec_cnt == cur_rttable->prev_rec_max_cnt)
                {
                    ret = dbi_Cursor_GetPosition(handle, cur_rttable->Hcursor,
                            cur_rttable->before_cursor_posi);
                    if (ret < 0)
                    {
                        return ret;     /* error handling */
                    }
                }
            }
            else
            {
                if (cur_rttable->has_prev_cursor_posi == 0)
                    cur_rttable->has_prev_cursor_posi = 1;
            }

            r_crs = RET_SUCCESS;
            break;
        }
    }

    /* r_crs : RET_SUCCESS or RET_END */
    if (r_crs == RET_END)
    {
        if (cur_jointab->join_type == SJT_LEFT_OUTER_JOIN &&
                cur_rttable->prev_rec_cnt == 0)
        {
            /* make null record */
            make_null_record(cur_rttable->rec_values);

            if (next_run < n_tables)
            {
                ret = inner_table_column_binding(rttable, cur_run, plan);
                if (ret < 0)
                    return ret;
            }

            r_crs = RET_NEXT;
            goto Read_Record_Done;
        }

        return RET_END;
    }

  Read_Record_Done:
    /* r_crs : RET_SUCCESS or RET_NEXT */
    if (next_run < n_tables)
    {
        int inner_r_crs;

        inner_r_crs = proceed_nextrecord(handle, rttable, next_run, plan);
        if (inner_r_crs < 0)
        {
            return inner_r_crs;
        }

        if (inner_r_crs == RET_END || inner_r_crs == RET_REPOSITION)
        {
            if (r_crs == RET_SUCCESS)
            {
                r_crs = RET_START;
                goto Read_Record_Again;
            }
            else
            {
                return RET_END;
            }
        }

        if (inner_r_crs == RET_NEXT)
        {
            if (r_crs == RET_SUCCESS)
                cur_rttable->status = SCS_FORWARD;
        }

        if (inner_r_crs == RET_SUCCESS && r_crs == RET_NEXT)
        {
            r_crs = RET_SUCCESS;
        }
    }


    return r_crs;
}



// 하나의 결과 투플을 만들어 내기 위해 주어진 테이블에서 사용할 레코드를
// 결정한다.
// join조건 검사를 수행한다.
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
proceed_nextrecord(int handle, T_RTTABLE * rttable, int cur_run,
        T_PLANNEDQUERYDESC * plan)
{
    if (rttable[cur_run].cnt_merge_item)
        return use_sortmerge(handle, rttable, cur_run, plan);
    else
        return use_nestedloop(handle, rttable, cur_run, plan);
}

/*****************************************************************************/
//! set_normalresult 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param selection() :
 * \param return()    : 
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - T_EXPRESSIONDESC 구조체 변경
 *  - length를 할당한다.
 *      코드 정리도 같이..
 *
 *****************************************************************************/
static int
set_normalresult(T_LIST_SELECTION * selection, T_QUERYRESULT * result)
{
    T_VALUEDESC value;
    T_VALUEDESC *valptr;
    T_EXPRESSIONDESC *expr;
    int i, ret;
    T_RESULTCOLUMN *res_list;

    for (i = 0; i < selection->len; i++)
    {
        expr = &selection->list[i].expr;
        if (expr->len == 1 && expr->list[0]->elemtype == SPT_VALUE)
        {
            valptr = &(expr->list[0]->u.value);
        }
        else
        {
            valptr = &value;
            ret = calc_expression(expr, valptr, MDB_FALSE);
            if (ret < 0)
                return ret;
        }

        res_list = result->list + i;

        if (valptr->is_null)
            res_list->value.is_null = 1;
        else
        {
            res_list->value.valuetype = valptr->valuetype;
            res_list->value.is_null = 0;

            switch (valptr->valuetype)
            {
            case DT_TINYINT:
                res_list->value.u.i8 = valptr->u.i8;
                break;
            case DT_SMALLINT:
                res_list->value.u.i16 = valptr->u.i16;
                break;
            case DT_INTEGER:
                res_list->value.u.i32 = valptr->u.i32;
                break;
            case DT_BIGINT:
                res_list->value.u.i64 = valptr->u.i64;
                break;
            case DT_FLOAT:
                res_list->value.u.f16 = valptr->u.f16;
                break;
            case DT_DOUBLE:
                res_list->value.u.f32 = valptr->u.f32;
                break;
            case DT_DECIMAL:
                res_list->value.u.dec = valptr->u.dec;
                break;
            case DT_CHAR:
            case DT_NCHAR:
            case DT_VARCHAR:
            case DT_NVARCHAR:
            case DT_BYTE:
            case DT_VARBYTE:
                if (valptr->call_free)
                {
                    if (!res_list->value.call_free)
                    {
                        res_list->value.u.ptr =
                                sql_malloc(res_list->len + 1,
                                valptr->valuetype);

                        if (!res_list->value.u.ptr)
                            break;

                        res_list->value.call_free = 1;
                    }
                    if (IS_BYTE(valptr->valuetype))
                        strncpy_func[valptr->valuetype] (res_list->value.u.ptr,
                                valptr->u.ptr, valptr->value_len);
                    else
                        strncpy_func[valptr->valuetype] (res_list->value.u.ptr,
                                valptr->u.ptr, res_list->len);
                }
                else
                {
                    res_list->value.u.ptr = valptr->u.ptr;
                }
                // 위에서.. value_len만큼 할당하면 될것을.. 너무 많이 할당함
                res_list->value.value_len = valptr->value_len;
                break;
            case DT_DATETIME:
                sc_memcpy(res_list->value.u.datetime,
                        valptr->u.datetime, MAX_DATETIME_FIELD_LEN);
                break;
            case DT_DATE:
                sc_memcpy(res_list->value.u.date,
                        valptr->u.date, MAX_DATE_FIELD_LEN);
                break;
            case DT_TIME:
                sc_memcpy(res_list->value.u.time,
                        valptr->u.time, MAX_TIME_FIELD_LEN);
                break;
            case DT_TIMESTAMP:
                sc_memcpy(&res_list->value.u.timestamp,
                        &valptr->u.timestamp, MAX_TIMESTAMP_FIELD_LEN);
                break;
            case DT_OID:
                res_list->value.u.oid = valptr->u.oid;
                break;

            default:
                break;
            }
        }

        if (valptr == &value)
            sql_value_ptrFree(&value);
    }
    return RET_SUCCESS;
}

extern int exec_insert_subquery(int handle, T_SELECT * select,
        T_RTTABLE * rttable, T_PARSING_MEMORY * main_parsing_chunk);

// fetch a result tuple
// from 절 subquery에 오는  correlated column을 지원하지 않는다.
// tmp tbl인지 확인은 rttable[]의 correlated_tbl을 이용.
// algorithm
// 1. (uncorrelated) subquery수행
// 2. 기존 루틴 수행.
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
MDB_BOOL
is_single_table_without_condition(T_SELECT * select)
{
    T_QUERYDESC *querydesc;
    T_RTTABLE *rttable;

    querydesc = &(select->planquerydesc.querydesc);
    rttable = select->rttable;

    if (querydesc->fromlist.len > 1)
        return FALSE;

    if (querydesc->condition.len > 0)
        return FALSE;

    if (rttable->db_expr_list || rttable->sql_expr_list)
        return FALSE;

    if (rttable->status != SCS_CLOSE)
        return FALSE;

    if (select->first_sub)
        return FALSE;

    return TRUE;
}

extern int
dbi_Cursor_Sel_Read(int connid, int cursorId, FIELDVALUES_T * rec_values);
int
fetch_cursor(int handle, T_SELECT * select, T_RESULT_INFO * result)
{
    T_PLANNEDQUERYDESC *planquery;
    T_QUERYDESC *querydesc;
    T_LIST_SELECTION *selection;
    T_QUERYRESULT *qresult;
    T_RTTABLE *rttable;
    T_KEYDESC key;
    int i, j, ret;
    int r_eval = 0;
    int table_index = -1, n_tables;
    int skip_fetching = 0;
    int prev_er_id = er_errid();
    int jump2offset = 0;
    OID selection_rid = NULL_OID;

    planquery = &(select->planquerydesc);
    querydesc = &(planquery->querydesc);
    selection = &(select->planquerydesc.querydesc.selection);
    rttable = select->rttable;
    qresult = &(select->queryresult);

    n_tables = querydesc->fromlist.len;

    if (select->rows == 0)
    {
        if (is_single_table_without_condition(select))
        {
            jump2offset = querydesc->limit.offset;
            rttable[0].jump2offset = jump2offset;
        }
        if (querydesc->limit.start_at != NULL_OID &&
                !isTempTable_name(rttable[0].table.tablename))
        {
            rttable[0].start_at = querydesc->limit.start_at;
        }

        for (i = 0; i < n_tables; i++)
        {
            rttable[i].in_expr_idx = -1;
        }
    }
    if (!querydesc->limit.rows ||
            (select->rows - querydesc->limit.offset) == querydesc->limit.rows)
        return RET_END;

    table_index = 0;
    while (1)
    {
        switch (select->tstatus)
        {
        case TSS_EXECUTED:
            ret = set_normalresult(selection, qresult);
            if (ret < 0)
                return RET_ERROR;
            select->tstatus = TSS_END;
            return RET_SUCCESS;
        case TSS_END:
            return RET_END;
        default:
            break;
        }
        if (n_tables == 1 &&
                IS_SINGLE_OP(planquery->nest) &&
                isPrimaryIndex_name(planquery->nest[0].indexname))
        {
            sc_memset(&key, 0, sizeof(T_KEYDESC));
            ret = make_single_keyvalue(planquery->nest, &key);
            if (ret < 0)
                return RET_ERROR;

            ret = dbi_Direct_Read(handle, rttable[0].table.tablename,
                    planquery->nest[0].indexname, &key,
                    NULL_OID, LK_SHARED,
                    rttable[0].rec_buf, (DB_UINT32 *) & rttable[0].rec_len,
                    rttable[0].rec_values);

            delete_single_keyvalue(&key);
            if (ret >= 0)
            {
                skip_fetching = 1;

            }
            else if (ret == DB_E_NOMORERECORDS)
                return RET_END;
            else if (ret != DB_E_INVALIDPARAM)
                return RET_ERROR;
        }

        if (planquery->nest[0].scanrid != NULL_OID)
        {
            int in_pos = 0;

            if (planquery->nest[0].ridin_cnt)
            {
                if (planquery->nest[0].ridin_row ==
                        planquery->nest[0].ridin_cnt)
                    return RET_END;

                for (i = 0; i < querydesc->condition.len; i++)
                {
                    if (querydesc->condition.list[i]->u.op.type == SOT_IN)
                    {
                        in_pos = i - planquery->nest[0].ridin_cnt;
                        break;
                    }
                }

                planquery->nest[0].scanrid =
                        querydesc->condition.list[planquery->nest[0].
                        ridin_row + in_pos]->u.value.u.oid;
            }

            if (planquery->nest[0].ridin_cnt != 0
                    && querydesc->condition.len >
                    planquery->nest[0].ridin_cnt + 2)
            {
                i = planquery->nest[0].ridin_row;
                j = planquery->nest[0].ridin_cnt;
            }
            else
            {
                i = 0;
                j = 1;
            }

            for (; i < j; i++)
            {
                ret = dbi_Direct_Read_ByRid(handle, rttable[0].table.tablename,
                        planquery->nest[0].scanrid,
                        &(querydesc->fromlist.list[0].tableinfo),
                        LK_SHARED, rttable[0].rec_buf,
                        (DB_UINT32 *) & rttable[0].rec_len,
                        rttable[0].rec_values);

                if (ret >= 0)
                {
                    if (querydesc->condition.len)
                        set_exprvalue(&rttable[0], &querydesc->condition,
                                NULL_OID);
                    r_eval = evaluate_expression(&querydesc->condition, 0);
                    if (r_eval <= 0 && j == 1)
                        return RET_ERROR;

                    if (!planquery->nest[0].ridin_cnt)
                        skip_fetching = 1;
                }
                else if (ret == DB_E_NOMORERECORDS)
                    return RET_END;
                else if (ret != DB_E_INVALIDPARAM)
                    return RET_ERROR;

                if (j != 1 && i != (planquery->nest[0].ridin_cnt - 1))
                    planquery->nest[0].scanrid =
                            querydesc->condition.list[i + 1 +
                            in_pos]->u.value.u.oid;
                planquery->nest[0].ridin_row++;

                if (r_eval > 0)
                    break;
            }
            if (i == j)
                return RET_END;
        }

        select->rows += 1;
        if (select->rowsp)
            (*select->rowsp) = select->rows;

        if (!skip_fetching && planquery->nest[0].scanrid == NULL_OID)
        {
            ret = proceed_nextrecord(handle, rttable, table_index, planquery);
            if (ret < 0)
            {
                return RET_ERROR;
            }
            else if (ret == RET_END)
            {
                if (prev_er_id != er_errid())
                    return RET_ERROR;
                break;
            }
        }
        if (jump2offset)
        {
            select->rows += jump2offset;
            if (select->rowsp)
                (*select->rowsp) = select->rows;
        }

        if (querydesc->condition.list
                && (querydesc->fromlist.outer_join_exists
                        || querydesc->expr_list == NULL))
        {
            for (i = 0; i < n_tables; i++)
            {
                if (rttable[i].table.tablename == NULL)
                    break;

                if (querydesc->condition.len)
                    set_exprvalue(&rttable[i], &querydesc->condition,
                            NULL_OID);
            }
            r_eval = evaluate_expression(&querydesc->condition, 0);
            if (r_eval <= 0)
            {
                select->rows -= 1;
                if (select->rowsp)
                    (*select->rowsp) = select->rows;
                continue;
            }
        }

        if (select->rows - 1 < querydesc->limit.offset)
            continue;

        for (i = 0; i < n_tables; ++i)
        {   // leftouter join인 경우 1번 더 않읽도록 수정함
            if (rttable[i].rec_values->rec_oid &&
                    rttable[i].rec_values->sel_fld_cnt
                    && planquery->nest[0].scanrid == NULL_OID)
                dbi_Cursor_Sel_Read(handle, rttable[i].Hcursor,
                        rttable[i].rec_values);
        }

        if (n_tables == 1)
        {
            for (j = 0; j < selection->len; j++)
            {
                if (selection->list[j].expr.len)
                    set_exprvalue_for_rttable(rttable,
                            &selection->list[j].expr);
            }
            if (querydesc->groupby.having.len)
            {   // having 조건이 있는 경우.. 값을 가지고 온다??
                set_exprvalue_for_rttable(rttable, &querydesc->groupby.having);
            }

            if (qresult->include_rid)
            {
                if (isTempTable_name(rttable[0].table.tablename))
                {
                    T_VALUEDESC *_tmpval;

                    _tmpval = (T_VALUEDESC *) PMEM_ALLOC(sizeof(T_VALUEDESC));
                    if (_tmpval == NULL)
                        return RET_ERROR;
                    convert_rec_attr2value(NULL, rttable->rec_values,
                            rttable->rec_values->fld_cnt - 1, _tmpval);
                    selection_rid = _tmpval->u.oid;
                    PMEM_FREE(_tmpval);

                    if (querydesc->limit.start_at != NULL_OID)
                    {
                        if (querydesc->limit.start_at == selection_rid)
                        {
                            querydesc->limit.start_at = NULL_OID;
                        }

                        select->rows -= 1;
                        if (select->rowsp)
                            (*select->rowsp) = select->rows;

                        continue;
                    }
                }
                else
                    selection_rid = rttable->rec_values->rec_oid;
            }
        }
        else
        {
            for (i = 0; i < n_tables; i++)
            {
                for (j = 0; j < selection->len; j++)
                {
                    if (selection->list[j].expr.len)
                        set_exprvalue(&rttable[i], &selection->list[j].expr,
                                NULL_OID);
                }
                if (querydesc->groupby.having.len)
                {
                    set_exprvalue(&rttable[i], &querydesc->groupby.having,
                            NULL_OID);
                }
            }
        }

        qresult->list[selection->len].value.u.oid = selection_rid;
        qresult->list[selection->len].value.valuetype = DT_OID;

        if (querydesc->querytype == SQT_NORMAL)
        {
            ret = set_normalresult(selection, qresult);
            if (ret < 0)
            {
                return RET_ERROR;
            }
        }

        if (skip_fetching)
            return RET_END_USED_DIRECT_API;
        return RET_SUCCESS;     // OK!! We've found one.
    }

    return RET_END;
}

/*****************************************************************************/
//! exec_select 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle :
 * \param stmt : 
 * \param result_type :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
exec_select(int handle, T_STATEMENT * stmt, int result_type)
{
    int ret;

    stmt->u._select.main_parsing_chunk = stmt->parsing_memory;
    ret = make_work_table(&handle, &stmt->u._select);
    if (ret == RET_ERROR)
    {
        return RET_ERROR;
    }

    return RET_SUCCESS;
}

// condition에 나오는 subquery
// fetch한 레코드 수를 리턴
// correlated subq의 경우는실행 결과를 가지고 있는것이 의미가 없고, 메모리
// 낭비다. 그렇지만 일단 가지고 있는 것으로 구현하자. free는 언제 ? 
/*****************************************************************************/
//! exec_singleton_subq 
  /*! \breif  condition에 나오는 subquery fetch한 레코드 수를 리턴
   ************************************
   * \param handle(in) :
   * \param select(in/out) : 
   * \param param_name :
   * \param param_name :
   ************************************
   * \return  condition에 나오는 subquery fetch한 레코드 수를 리턴
   ************************************
   * \note 내부 알고리즘\n
   *  - parsing 정보가 없기 때문에.. 공간을 할당할 필요는 없으나..
   *      새로 생성한 stmt가 cleanup 될때 main_parsing_chunk의 공간을
   *      free 해버리는 문제가 있으므로..
   *      main_stmt의 공간을 받아온다.
   *     
   *****************************************************************************/
extern int one_binary_column_length(T_RESULTCOLUMN * rescol, int is_subq);
extern int close_all_open_tables(int, T_SELECT *);

int
exec_singleton_subq(int handle, T_SELECT * select)
{
    T_SELECT *fetch_select;
    T_QUERYRESULT *qres = NULL;
    T_QUERYDESC *qdesc;
    T_RESULTCOLUMN *rescol;
    int i, ret = RET_SUCCESS, row_num = 0;
    int *rec_len, bi_rec_len, col_len;
    char *target, *null_ptr, bit;

#ifdef MDB_DEBUG
    sc_assert(select->kindofwTable & iTABLE_SUBWHERE, __FILE__, __LINE__);
#endif
    if ((select->kindofwTable & iTABLE_CORRELATED) == 0 &&
            select->tstatus == TSS_EXECUTED)
    {
        return RET_SUCCESS;
    }
#ifdef MDB_DEBUG
    if (select->tstatus == TSS_EXECUTED)
    {
        sc_assert((select->kindofwTable & iTABLE_CORRELATED) == 0,
                __FILE__, __LINE__);
    }
#endif

    qdesc = &select->planquerydesc.querydesc;

    set_correl_attr_values(qdesc->correl_attr);

    // 2. subquery execution
    // TODO: memory check. 계속해서 tmp tbl을 만드는 문제.
    // BUG !!!!

    ret = make_work_table(&handle, select);
    if (ret == RET_ERROR)
    {
        return RET_ERROR;
    }

    if (select->tmp_sub)
    {
        fetch_select = select->tmp_sub;
    }
    else
    {
        fetch_select = select;
    }

    qres = &fetch_select->queryresult;

    bi_rec_len = bin_result_record_len(qres, 1);        // 결과 레코드 길이

    if (select->subq_result_rows == NULL)
    {
        select->subq_result_rows = PMEM_XCALLOC(sizeof(iSQL_ROWS));
        if (select->subq_result_rows == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }
        select->subq_result_rows->next = NULL;
        select->subq_result_rows->data = NULL;
    }

    fetch_select->rows = 0;

    while (1)
    {
        ret = SQL_fetch(&handle, fetch_select, NULL);
        if (ret == RET_END)
        {
            break;
        }
        else if (ret == RET_ERROR)
        {
            goto RETURN_LABEL;
        }

        if (++row_num > 1)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTSINGLETONSUBQ,
                    0);
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }

        if (select->subq_result_rows->data == NULL)     // max length로 할당
        {
            select->subq_result_rows->data = PMEM_ALLOC(bi_rec_len);
        }
        if (!select->subq_result_rows->data)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }

        sc_memset(select->subq_result_rows->data, 0,
                sizeof(int) + (((qres->len - 1) >> 3) + 1));

        rec_len = (int *) select->subq_result_rows->data;
        null_ptr = (char *) select->subq_result_rows->data + sizeof(int);
        target = (char *) select->subq_result_rows->data + sizeof(int)
                + (((qres->len - 1) >> 3) + 1);
        *rec_len = sizeof(int) + ((qres->len - 1) >> 3) + 1;

        for (i = 0, bit = 1; i < qres->len; i++)
        {
            rescol = &qres->list[i];
            if (rescol->value.is_null)
            {
                col_len = one_binary_column_length(rescol, 1);
                *null_ptr |= bit;
            }
            else
            {
                col_len = copy_one_binary_column(target, rescol, 1);
            }

            target += col_len;
            (*rec_len) += col_len;

            if (!((bit <<= 1) & 0xFF))
            {
                bit = 1;
                null_ptr++;
            }
        }   // END: for()

        if (ret == RET_END_USED_DIRECT_API)
        {
            break;
        }
    }       // END: while()

  RETURN_LABEL:

    close_all_open_tables(handle, select);
    if (qdesc->setlist.len > 0)
    {
        SQL_cleanup_subselect(handle, select->tmp_sub, NULL);
        PMEM_FREENUL(select->tmp_sub);
    }

    if (select->wTable)
    {
        dbi_Relation_Drop(handle, select->wTable);
    }

    if (ret == RET_ERROR)
    {
        if (select->subq_result_rows)
        {
            iSQL_ROWS *result_rows = select->subq_result_rows;

            while (result_rows)
            {
                select->subq_result_rows = result_rows->next;
                if (result_rows->data)
                {
                    PMEM_FREENUL(result_rows->data);
                }
                PMEM_FREENUL(result_rows);
                result_rows = select->subq_result_rows;
            }
        }
    }
    /* singleton subquery의 결과가 없는 경우 result buffer를 초기화
       result buffer의 재사용에 대한 문제를 해결하기 위함 */
    else if (row_num == 0)
    {
        if (select->subq_result_rows)
        {
            iSQL_ROWS *result_rows = select->subq_result_rows;

            while (result_rows)
            {
                if (result_rows->data)
                {
                    PMEM_FREENUL(result_rows->data);
                }
                result_rows = result_rows->next;
            }
        }
    }

    for (i = 0; i < qdesc->fromlist.len; i++)
    {
        if (select->planquerydesc.nest[i].indexname &&
                select->planquerydesc.nest[i].is_subq_orderby_index)
        {
            PMEM_FREENUL(select->planquerydesc.nest[i].indexname);
            select->planquerydesc.nest[i].is_subq_orderby_index = 0;
        }
    }

    return ret;
}
