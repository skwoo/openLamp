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
#include "sql_main.h"

#include "mdb_er.h"
#include "ErrorCode.h"

extern int evaluate_expression(T_EXPRESSIONDESC * expr, MDB_BOOL is_orexpr);
int evaluate_conjunct(EXPRDESC_LIST * list);
void set_exprvalue(T_RTTABLE * rttable, T_EXPRESSIONDESC * expr, OID rid);

int open_cursor(int connection, T_RTTABLE * rttable,
        T_NESTING * nest, EXPRDESC_LIST * expr_list, MDB_UINT8 mode);
void close_cursor(int connection, T_RTTABLE * rttable);
int make_single_keyvalue(T_NESTING * nest, T_KEYDESC * key);
int delete_single_keyvalue(T_KEYDESC * key);

extern FIELDVALUES_T *init_rec_values(T_RTTABLE * rttable,
        SYSTABLE_T * tableinfo, SYSFIELD_T * fieldsinfo, int select_star);
extern int set_rec_values_info_delete(int handle, T_DELETE * delete);

#define DELETE_HAS_SQLFILTER(stmt)                                  \
         ((stmt)->u._delete.rttable[0].sql_expr_list != NULL ||     \
         ((stmt)->u._delete.condition.list && stmt->u._delete.expr_list == NULL))

/*****************************************************************************/
//! exec_delete

/*! \breif  DELETE 문장을 수행한다
 ************************************
 * \param handle(in)    :
 * \param stmt(in/out)  : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
exec_delete(int handle, T_STATEMENT * stmt)
{
    T_RTTABLE *rttable;
    T_NESTING *nest;
    EXPRDESC_LIST *cur;
    T_QUERYDESC *qdesc;
    T_EXPRESSIONDESC *condition;
    int ret = RET_SUCCESS;
    int i = 0;

    rttable = stmt->u._delete.rttable;
    nest = stmt->u._delete.planquerydesc.nest;
    qdesc = &stmt->u._delete.planquerydesc.querydesc;
    condition = &qdesc->condition;

    if (IS_SINGLE_OP(nest) &&
            (rttable[0].db_expr_list || rttable[0].sql_expr_list))
    {
        nest->cnt_single_item = 0;
    }

    if (IS_SINGLE_OP(nest) && isPrimaryIndex_name(nest->indexname))
    {
        T_KEYDESC key;

        ret = make_single_keyvalue(nest, &key);
        if (ret < 0)
        {
            return RET_ERROR;
        }

        ret = dbi_Direct_Remove(handle, nest->table.tablename,
                nest->indexname, &key, NULL_OID, LK_EXCLUSIVE);
        delete_single_keyvalue(&key);
        if (ret >= 0)
        {
            stmt->u._delete.rows++;
            return RET_SUCCESS;
        }
        else if (ret == DB_E_NOMORERECORDS)
        {
            return RET_SUCCESS;
        }
        else if (ret != DB_E_INVALIDPARAM)
        {
            return RET_ERROR;
        }
    }

    if (nest->scanrid != NULL_OID)
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
                if (!rttable->table.tablename)
                {
                    rttable->table.tablename = nest->table.tablename;
                    rttable->table.aliasname = nest->table.aliasname;
                    rttable->rec_values = init_rec_values(rttable,
                            &qdesc->fromlist.list[0].tableinfo,
                            qdesc->fromlist.list[0].fieldinfo, 0);
                    ret = set_rec_values_info_delete(handle, &stmt->u._delete);
                    if (ret < 0)
                    {
                        return RET_ERROR;
                    }
                }
                ret = dbi_Direct_Read_ByRid(handle,
                        nest->table.tablename, nest->scanrid,
                        &(qdesc->fromlist.list[0].tableinfo),
                        LK_SHARED, rttable->rec_buf,
                        (DB_UINT32 *) & (rttable->rec_len),
                        rttable->rec_values);
                if (ret >= 0)
                {
                    set_exprvalue(rttable, condition, NULL_OID);
                    ret = evaluate_expression(condition, 0);
                    if (ret < 0)
                    {
                        return RET_ERROR;
                    }
                }
            }

            if (ret > 0)
            {
                ret = dbi_Direct_Remove_ByRid(handle, nest->table.tablename,
                        nest->scanrid,
                        &(qdesc->fromlist.list[0].tableinfo), LK_EXCLUSIVE);
                if (ret >= 0)
                {
                    stmt->u._delete.rows++;
                }
                else if (ret == DB_E_NOMORERECORDS)
                {
                    return RET_END;
                }
                else if (ret != DB_E_INVALIDPARAM)
                {
                    return RET_ERROR;
                }
            }
        }

        return RET_SUCCESS;
    }

    rttable[0].in_expr_idx = -1;

    if (!rttable->table.tablename)      // 이미 전에 prepare & execute된적이 있다면
    {
        rttable->table.tablename = nest->table.tablename;
        rttable->table.aliasname = nest->table.aliasname;
    }

    if (!rttable[0].sql_expr_list
            && !(condition->list && qdesc->expr_list == NULL)
            && !nest->op_in
            && qdesc->limit.offset <= 0
            && qdesc->limit.rows < 0
            && nest->index_field_num && nest->indexname)
    {
        if (nest->op_in)
        {
            rttable[0].in_expr_idx += 1;
            if (rttable[0].in_expr_idx == nest->in_exprs)
            {
                ret = RET_SUCCESS;
                goto RETURN_LABEL;
            }
        }

        ret = open_cursor(handle, &(rttable[0]), nest,
                rttable[0].db_expr_list, LK_EXCLUSIVE);
        if (ret < 0)
            return ret;

        ret = dbi_Direct_Remove2_byUser(handle, rttable[0].Hcursor,
                NULL, NULL, NULL, NULL, NULL, LK_EXCLUSIVE, _USER_USER);
        close_cursor(handle, rttable);

        if (ret >= 0)
        {
            stmt->u._delete.rows += ret;
            return RET_SUCCESS;

        }
        else if (ret == DB_E_NOMORERECORDS)
        {
            return RET_SUCCESS;
        }
        else if (ret != DB_E_INVALIDPARAM)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return RET_ERROR;
        }
    }

    if (rttable->rec_values == NULL)
    {
        rttable->rec_values = init_rec_values(rttable,
                &(qdesc->fromlist.list[0].tableinfo),
                qdesc->fromlist.list[0].fieldinfo, 0);
    }

    if (rttable->rec_values == NULL)
    {
        return RET_ERROR;
    }


    ret = set_rec_values_info_delete(handle, &stmt->u._delete);
    if (ret < 0)
    {
        return RET_ERROR;
    }

    do
    {
        if (nest->op_in)
        {
            rttable[0].in_expr_idx += 1;
            if (rttable[0].in_expr_idx == nest->in_exprs)
            {
                ret = RET_SUCCESS;
                goto RETURN_LABEL;
            }
        }

        ret = open_cursor(handle, &(rttable[0]),
                nest, rttable[0].db_expr_list, LK_EXCLUSIVE);
        if (ret < 0)
        {
            return RET_ERROR;
        }
        else if (ret == RET_FALSE)
        {
            close_cursor(handle, rttable);

            if (i == nest->in_exprs)
            {
                return RET_SUCCESS;
            }
        }

        /* ret : SUCCESS, FAIL, DB_E_NOMORERECORDS, DB_E_TERMINATED */
        while (1)
        {
            ret = dbi_Cursor_Read(handle, rttable->Hcursor, NULL,
                    &(rttable->rec_len), rttable->rec_values, 0, 0);
            if (ret != DB_SUCCESS)
            {
                break;
            }

            if (rttable[0].sql_expr_list == NULL)
            {
                if (condition->list && qdesc->expr_list == NULL)
                {
                    set_exprvalue(&rttable[0], condition, NULL_OID);
                    ret = evaluate_expression(condition, 0);
                }
                else
                {
                    ret = RET_SUCCESS;
                }
            }
            else
            {
                for (cur = rttable[0].sql_expr_list; cur; cur = cur->next)
                {
                    set_exprvalue(&rttable[0], cur->ptr, NULL_OID);
                }
                ret = evaluate_conjunct(rttable[0].sql_expr_list);
            }
            if (ret < 0)
            {
                ret = RET_ERROR;
                goto RETURN_LABEL;
            }
            else if (ret)
            {
                if (stmt->u._delete.rows == qdesc->limit.rows)
                {
                    break;
                }

                ret = dbi_Cursor_Remove(handle, rttable->Hcursor);
                if (ret < 0)
                {
                    SQL_trans_implicit_partial_rollback(handle, stmt);
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    ret = RET_ERROR;
                    break;
                }
                stmt->u._delete.rows++;
            }
            else
            {
                ret = dbi_Cursor_Read(handle, rttable->Hcursor, NULL,
                        &(rttable->rec_len), rttable->rec_values, 1, 0);
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
        {
            break;
        }

        i += 1;
    }
    while (i < nest->in_exprs);

  RETURN_LABEL:

    if (ret == DB_E_TERMINATED)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        ret = RET_ERROR;
    }

    return ret;
}
