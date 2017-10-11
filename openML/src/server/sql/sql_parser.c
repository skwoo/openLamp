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

#include "sql_parser.h"
#include "sql_util.h"
#include "sql_datast.h"
#include "mdb_Server.h"
#include "sql_mclient.h"
#include "sql_func_timeutil.h"
#include "mdb_er.h"

#include "sql_mclient.h"

int SQL_prepare(int *handle, T_STATEMENT * stmt, int flag);
MDB_INLINE char *sql_strdup(char *str, DataType type);


extern int calc_func_convert(T_VALUEDESC * base, T_VALUEDESC * src,
        T_VALUEDESC * res, MDB_INT32 is_init);
extern int create_subquery_table(int *, T_SELECT *, char *);

//////////////////////// Util Functions Start ////////////////////////

/*****************************************************************************/
//! make_columnlist

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param selection() :
 * \param table() :
 * \param tablenum(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static int
make_columnlist(T_LIST_SELECTION * selection,
        T_JOINTABLEDESC * p_table, int tablenum)
{
    T_POSTFIXELEM *elem;
    int i, j, k;
    int size;
    T_JOINTABLEDESC *table = p_table;

    for (i = 0, size = 0; i < tablenum; i++)
        size += table[i].tableinfo.numFields;
    if (selection->list == NULL)
    {
        selection->list = PMEM_XCALLOC(sizeof(T_SELECTION) * (size + 1));
        selection->len = 0;
        selection->max = size;
    }
    else if (selection->len != size)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTMATCHCOLUMNCOUNT, 0);
        return RET_ERROR;
    }

    // foreach table
    for (i = 0, j = 0; i < tablenum; i++)
    {
        table = p_table + i;
        // foreach field, allocate and fill the field
        for (k = 0; k < table->tableinfo.numFields; k++)
        {
            if (selection->aliased_len != -1)
            {
                sc_strcpy(selection->list[j].res_name,
                        table->fieldinfo[k].fieldName);
            }

            selection->list[j].expr.len = 1;
            selection->list[j].expr.max = 1;
            selection->list[j].expr.list =
                    sql_mem_get_free_postfixelem_list(NULL, 1);
            if (selection->list[j].expr.list == NULL)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return RET_ERROR;
            }
            selection->list[j].expr.list[0] =
                    sql_mem_get_free_postfixelem(NULL);
            if (selection->list[j].expr.list[0] == NULL)
            {
                sql_mem_set_free_postfixelem_list(NULL,
                        selection->list[j].expr.list);
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return RET_ERROR;
            }
            sc_memset(selection->list[j].expr.list[0], 0x00,
                    sizeof(T_POSTFIXELEM));


            elem = selection->list[j++].expr.list[0];
            elem->elemtype = SPT_VALUE;
            elem->u.value.valueclass = SVC_VARIABLE;
            elem->u.value.valuetype = table->fieldinfo[k].type;

            if (IS_ALLOCATED_TYPE(elem->u.value.valuetype))
            {
                elem->u.value.u.ptr = NULL;
            }

            elem->u.value.attrdesc.table.tablename =
                    PMEM_STRDUP(table->table.tablename);
            if (!elem->u.value.attrdesc.table.tablename)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return RET_ERROR;
            }
            if (table->table.aliasname)
            {
                elem->u.value.attrdesc.table.aliasname =
                        PMEM_STRDUP(table->table.aliasname);
                if (!elem->u.value.attrdesc.table.aliasname)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_OUTOFMEMORY, 0);
                    return RET_ERROR;
                }
            }
            else
                elem->u.value.attrdesc.table.aliasname = NULL;

            elem->u.value.attrdesc.attrname
                    = PMEM_STRDUP(table->fieldinfo[k].fieldName);
            if (!elem->u.value.attrdesc.attrname)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return RET_ERROR;
            }
            if (set_attrinfobyname(-1, &elem->u.value.attrdesc,
                            &(table->fieldinfo[k])) == RET_ERROR)
                return SQL_E_ERROR;

        }
    }

    selection->len = j;

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
void
set_super_tbl_correlated(T_SELECT * select)
{
    T_SELECT *cur = select;

    while (cur->super_query && cur->super_query->super_query)
    {
        cur->super_query->kindofwTable |= iTABLE_CORRELATED;
        cur = cur->super_query;
    }
}

static int check_attr_correlation(int handle, T_QUERYDESC * qdesc,
        T_ATTRDESC * attr, T_EXPRESSIONDESC * expr, int *expr_pos,
        T_SELECT * select);

/*****************************************************************************/
//! check_selection

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)        :
 * \param select(in/out)    :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  check result columns :
 *  1. if '*', fill selection list.
 *  2. all columns exist in tables ?
 *  3. set attribute of every postfix element in the "selection.list"
 *  4. group by checking : is "i" a member of group by argument in the
 *     "select i from tbl group by arg1, arg2" ?
 *
 *  - T_EXPRESSIONDESC 변경
 *
 *****************************************************************************/
static int
check_selection(int handle, T_SELECT * select)
{
    T_QUERYDESC *qdesc;
    T_LIST_SELECTION *selection;
    T_POSTFIXELEM *elem;
    T_ATTRDESC *attr;

    int is_aggregation = 0;
    int has_dcount = 0;
    int i, j, k, l;
    int ret;

    qdesc = &select->planquerydesc.querydesc;
    selection = &qdesc->selection;

    // in case of "select *" query, selection list is built.
    // check col_refs and save it into selection.list[].expr.list[]
    if (selection->len == 0 /* all columns */  ||
            selection->aliased_len == -1 /* all columns & aliases */ )
    {
        ret = make_columnlist(selection,
                qdesc->fromlist.list, qdesc->fromlist.len);
        if (ret < 0)
        {
            return RET_ERROR;
        }
        if (qdesc->groupby.len)
        {
            is_aggregation = 1;
        }
    }
    else
    {       // if specify column list;
        // for each selection list
        for (i = 0; i < selection->len; i++)
        {
            // check attributes
            // "selection.list[].expr.list[].attr_infomations"
            for (l = 0; l < selection->list[i].expr.len; l++)
            {
                elem = selection->list[i].expr.list[l];
                if (elem->elemtype == SPT_AGGREGATION)
                {
                    ++qdesc->aggr_info.len;
                    if (elem->u.aggr.type == SAT_AVG)
                        ++qdesc->aggr_info.n_sel_avg;
                    if (qdesc->aggr_info.n_expr < elem->u.aggr.significant)
                        qdesc->aggr_info.n_expr = elem->u.aggr.significant;
                    if (elem->is_distinct)
                    {
                        qdesc->is_distinct = 1;
                        selection->list[i].is_distinct = 1;
                        ++qdesc->distinct.len;
                    }
                    is_aggregation = 1;

                    if (elem->u.aggr.type == SAT_DCOUNT)
                        has_dcount += 1;

                    if (has_dcount == 1)
                    {
                        if (qdesc->fromlist.len != 1 ||
                                qdesc->condition.len != 0)
                        {
                            /* dirty_count는 단일 테이블에 대해서,
                             * 조건절이 없는 경우 사용할 수 있음 */
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_INVALIDDIRTYCOUNT, 0);

                            return RET_ERROR;
                        }

                    }
                }
                if (elem->elemtype == SPT_FUNCTION &&
                        elem->u.func.type == SFT_COPYFROM)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_INVALIDEXPRESSION, 0);
                    return RET_ERROR;
                }

                if ((elem->elemtype != SPT_VALUE && elem->elemtype != SPT_NONE)
                        || elem->u.value.valueclass == SVC_CONSTANT)
                    continue;

                // now, elem->elemtype is SPT_VALUE .
                // let's find attr.
                ret = check_attr_correlation(handle, qdesc,
                        &elem->u.value.attrdesc, NULL, NULL, select);
                if (ret != RET_SUCCESS)
                    return ret;
                elem->u.value.valuetype = elem->u.value.attrdesc.type;
                // TODO : correl_attr과 subquery를 가리키는 T_SELECT를 free
                //        하는곳 검사
                if (elem->u.value.attrdesc.table.correlated_tbl != select)
                {
                    CORRELATED_ATTR *cl = PMEM_ALLOC(sizeof(CORRELATED_ATTR));

                    if (cl == NULL)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_OUTOFMEMORY, 0);
                        return RET_ERROR;
                    }
                    cl->next = qdesc->correl_attr;
                    cl->ptr = (void *) elem;
                    qdesc->correl_attr = cl;
                    select->kindofwTable |= iTABLE_CORRELATED;
                    set_super_tbl_correlated(select);
                }

#ifdef MDB_DEBUG
                sc_assert(elem->u.value.attrdesc.table.correlated_tbl != NULL,
                        __FILE__, __LINE__);
#endif
                // set_result_info() valuetype을 access하므로
                // 여기서 미리 valuetype을 assign해놓자.
                elem->u.value.valuetype = elem->u.value.attrdesc.type;
                elem->u.value.is_null = 1;
            }   // // for each node(element ?) of a result column
        }   // for each column of a result tuple
        if (!is_aggregation && qdesc->groupby.len)
            is_aggregation = 1;

        if (qdesc->distinct.len >= MAX_DISTINCT_TABLE_NUM)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_TOOMANYDISTINCT, 0);
            return RET_ERROR;
        }

        // group by and aggregation ... checking
        for (i = 0; i < selection->len; i++)
        {
            T_EXPRESSIONDESC *expr = &selection->list[i].expr;
            int start, checked = 0;

            // foreach postfix element
            for (j = 0, start = -1; j < expr->len; j++)
            {
                if (start < 0)
                    start = j;

                elem = expr->list[j];
                if (elem->elemtype == SPT_AGGREGATION)
                {
                    checked = 1;
                    if (j - elem->u.aggr.significant == start)
                    {
                        start = -1;
                        continue;
                    }
                    if (elem->u.aggr.significant > j - start)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_NESTEDAGGREGATION, 0);
                        return RET_ERROR;
                    }

                    // in case of "j + avg(i)" or " ... "
                    // Every postfix element of aggregation expression should be a
                    // memeber of group by argument.
                    //  ex) "select i + avg(j) from T group by arg_list"
                    //      => 'i' is a member of "arg_list" list.
                    for (k = start; k < j - elem->u.aggr.significant; k++)
                    {
                        if (expr->list[k]->elemtype == SPT_VALUE &&
                                expr->list[k]->u.value.valueclass
                                == SVC_VARIABLE)
                        {
                            for (l = 0; l < qdesc->groupby.len; l++)
                            {
                                attr = &qdesc->groupby.list[l].attr;
                                if (!mdb_strcmp(attr->attrname,
                                                expr->list[k]->u.value.
                                                attrdesc.attrname))
                                {
                                    if (expr->list[k]->u.value.attrdesc.table.
                                            aliasname)
                                    {
                                        if (!attr->table.tablename ||
                                                !mdb_strcmp(attr->table.
                                                        tablename,
                                                        expr->list[k]->u.value.
                                                        attrdesc.table.
                                                        aliasname))
                                        {
                                            break;
                                        }
                                    }
                                    else if (!attr->table.tablename ||
                                            !mdb_strcmp(attr->table.tablename,
                                                    expr->list[k]->u.value.
                                                    attrdesc.table.tablename))
                                    {
                                        break;
                                    }
                                }
                            }
                            if (l == qdesc->groupby.len)
                            {
                                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                        SQL_E_NOTGROUPBYEXPRESSION, 0);
                                return RET_ERROR;
                            }
                        }
                    }
                    start = -1;
                }
            }   // for(selection.list[i]의 각 postfix element)

            if (is_aggregation)
            {
                if (!checked)
                {
                    if (!qdesc->groupby.len)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_NOTGROUPBYEXPRESSION, 0);
                        return RET_ERROR;
                    }
                }
                else if (start >= 0)
                {
                    for (j = start; j < expr->len; j++)
                    {
                        elem = expr->list[j];
                        if (elem->elemtype == SPT_VALUE &&
                                elem->u.value.valueclass == SVC_VARIABLE)
                        {
                            for (k = 0; k < qdesc->groupby.len; k++)
                            {
                                attr = &qdesc->groupby.list[k].attr;
                                if (!mdb_strcmp(attr->attrname,
                                                elem->u.value.attrdesc.
                                                attrname))
                                {
                                    if (elem->u.value.attrdesc.table.aliasname)
                                    {
                                        if (!attr->table.tablename ||
                                                !mdb_strcmp(attr->table.
                                                        tablename,
                                                        elem->u.value.attrdesc.
                                                        table.aliasname))
                                        {
                                            break;
                                        }
                                    }
                                    else if (!attr->table.tablename ||
                                            !mdb_strcmp(attr->table.tablename,
                                                    elem->u.value.attrdesc.
                                                    table.tablename))
                                    {
                                        break;
                                    }
                                }
                            }
                            if (k == qdesc->groupby.len)
                            {
                                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                        SQL_E_NOTGROUPBYEXPRESSION, 0);
                                return RET_ERROR;
                            }
                        }
                    }
                }
            }
        }

        /* check if an argument of function is numeric type */
        for (i = 0; i < selection->len; i++)
        {
            T_POSTFIXELEM **agg_list = selection->list[i].expr.list;
            int agg_size = selection->list[i].expr.len;
            int numeric_ok;

            for (j = 0, numeric_ok = 1; j < agg_size; j++)
            {
                switch (agg_list[j]->elemtype)
                {
                case SPT_VALUE:
                    switch (agg_list[j]->u.value.valuetype)
                    {
                    case DT_BOOL:
                    case DT_CHAR:
                    case DT_NCHAR:
                    case DT_VARCHAR:
                    case DT_NVARCHAR:
                        if (j + 1 < agg_size &&
                                agg_list[j + 1]->elemtype
                                == SPT_FUNCTION &&
                                agg_list[j + 1]->u.func.type == SFT_OBJECTSIZE)
                        {       /* ... */
                        }
                        else
                        {
                            numeric_ok = 0;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                case SPT_AGGREGATION:
                    switch (agg_list[j]->u.aggr.type)
                    {
                    case SAT_SUM:
                    case SAT_AVG:
                        if (!numeric_ok)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_INVALIDTYPE, 0);
                            return RET_ERROR;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                case SPT_FUNCTION:
                    /* we do not consider now what returned data types of
                       CONVERT(), DECODE() functions are */
                    break;
                default:
                    break;
                }
            }
        }
    }
    qdesc->querytype = (is_aggregation) ? SQT_AGGREGATION : SQT_NORMAL;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_attr_correlation

/*! \breif attr가 query와 그 super query에 명시된
 *      질의 대상 테이블중 하나에 속하는지 확인한다.
 ************************************
 * \param handle(in)    : handle
 * \param select(in)    :
 * \param attr(in/out)  : Fields info
 ************************************
 * \return RET_SUCCESS or some errors
 ************************************
 * \note
 * 현재 T_SELECT에서 주어진 attr이 있는지 확인하고 있다면
 * attr.[][]를 ...
 * 없다면 parent T_SELECT에 대해 반복적으로 수행해서 ..
 * TODO: group by, having같은 경우는 correlated column이 올 수 없으므로
 *          super_query node를 찾아갈 필요가 없다. super_query node를 쫓아갈
 *          필요가 있는지를 나타내는 인자 추가.
 *****************************************************************************/
/* correlated column에 대한 설정 정리 update, delete에 대하여 처리되도록 수정 */
/* alias name과 동일한 expression을 alias의 원본 expression으로 교체 */
static int
set_alias_expr(T_EXPRESSIONDESC * expr, int expr_idx,
        T_EXPRESSIONDESC * alias_expr, T_PARSING_MEMORY * chunk)
{
    T_POSTFIXELEM *elem;
    T_ATTRDESC *expr_attr, *alias_attr;
    int i;

    if (expr->max < (expr->len + alias_expr->len - 1))
    {
        T_POSTFIXELEM **list = NULL;

        list = sql_mem_get_free_postfixelem_list(chunk,
                (expr->len + alias_expr->len - 1));
        if (list == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return RET_ERROR;
        }

        for (i = 0; i < expr->len; ++i)
        {
            list[i] = expr->list[i];
        }

        sql_mem_set_free_postfixelem_list(chunk, expr->list);

        expr->list = list;
        expr->max = expr->len + alias_expr->len - 1;
    }

    for (i = expr_idx + 1; i < expr->len; ++i)
    {
        expr->list[i + alias_expr->len - 1] = expr->list[i];
    }

    for (i = 0; i < alias_expr->len; ++i)
    {
        if (i > 0)
        {
            expr->list[i + expr_idx] = sql_mem_get_free_postfixelem(chunk);
        }
        sc_memcpy(expr->list[i + expr_idx], alias_expr->list[i],
                sizeof(T_POSTFIXELEM));
        elem = expr->list[i + expr_idx];

        if (elem->elemtype == SPT_VALUE &&
                elem->u.value.valueclass == SVC_VARIABLE)
        {
            expr_attr = &elem->u.value.attrdesc;
            alias_attr = &alias_expr->list[i]->u.value.attrdesc;

            expr_attr->attrname = PMEM_STRDUP(alias_attr->attrname);
            if (!expr_attr->attrname)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return RET_ERROR;
            }
            if (alias_attr->table.aliasname)
            {
                expr_attr->table.tablename =
                        PMEM_STRDUP(alias_attr->table.aliasname);
                if (!expr_attr->table.tablename)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_OUTOFMEMORY, 0);
                    return RET_ERROR;
                }
                expr_attr->table.aliasname = NULL;
            }
            else if (alias_attr->table.tablename)
            {
                expr_attr->table.tablename =
                        PMEM_STRDUP(alias_attr->table.tablename);
                if (!expr_attr->table.tablename)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_OUTOFMEMORY, 0);
                    return RET_ERROR;
                }
            }
        }
    }

    expr->len += (alias_expr->len - 1);

    return RET_SUCCESS;
}

static int
check_attr_correlation(int handle, T_QUERYDESC * qdesc,
        T_ATTRDESC * attr, T_EXPRESSIONDESC * expr, int *expr_pos,
        T_SELECT * select)
{
    T_JOINTABLEDESC *join;
    SYSFIELD_T *fieldinfo = NULL;
    int i, j, ret, found = 0;

    join = qdesc->fromlist.list;        // from 절 alias

    if (attr->table.tablename == NULL)
    {       // not specify table
        //  check existance of attribute name in given tables
        for (i = 0; i < qdesc->fromlist.len; i++)
        {
            if (attr->Hattr == -1)
            {
                if (qdesc->fromlist.len > 1)
                {       // ambiguous
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_AMBIGUOUSCOLUMN, 0);
                    return RET_ERROR;
                }

                attr->Htable = join[i].tableinfo.tableId;
                attr->table.tablename = PMEM_STRDUP(join[i].table.tablename);
                if (!attr->table.tablename)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_OUTOFMEMORY, 0);
                    return RET_ERROR;
                }
                if (join[i].table.aliasname)
                {
                    attr->table.aliasname =
                            PMEM_STRDUP(join[i].table.aliasname);
                    if (!attr->table.aliasname)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_OUTOFMEMORY, 0);
                        return RET_ERROR;
                    }
                }
                attr->table.correlated_tbl = select;
                return RET_SUCCESS;
            }

            for (j = 0; j < join[i].tableinfo.numFields; j++)
            {
                if (!mdb_strcmp(join[i].fieldinfo[j].fieldName,
                                attr->attrname))
                {
                    if (found)
                    {   // ambiguous
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_AMBIGUOUSCOLUMN, 0);
                        return RET_ERROR;
                    }

                    found = 1;

                    attr->table.tablename =
                            PMEM_STRDUP(join[i].table.tablename);
                    if (!attr->table.tablename)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_OUTOFMEMORY, 0);
                        return RET_ERROR;
                    }
                    if (join[i].table.aliasname)
                    {
                        attr->table.aliasname =
                                PMEM_STRDUP(join[i].table.aliasname);
                        if (!attr->table.aliasname)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_OUTOFMEMORY, 0);
                            return RET_ERROR;
                        }
                    }

                    fieldinfo = &join[i].fieldinfo[j];
                    break;
                }
            }
        }   // for(fromlist table)

        if (!found && expr_pos)
        {
            /* selection의 res(alias)와 비교하여 동일한 경우 alias 처리 */
            for (i = 0; i < qdesc->selection.len; ++i)
            {
                if (!mdb_strcmp(qdesc->selection.list[i].res_name,
                                attr->attrname))
                {
                    found = 1;
                    break;
                }
            }

            if (found)
            {
#if defined(MDB_DEBUG)
                /* alias의 expression은 parsing memory로 구성 */
                if (!select->main_parsing_chunk)
                {
                    sc_assert(0, __FILE__, __LINE__);
                }
#endif

                ret = set_alias_expr(expr, *expr_pos,
                        &qdesc->selection.list[i].expr,
                        select->main_parsing_chunk);
                if (ret < 0)
                {
                    return RET_ERROR;
                }

                /* expression이 alias로 교체되었기 때문에 expression을 재검사
                   하기 위해 expression의 현재 position을 유지 */
                --(*expr_pos);

                return RET_SUCCESS;
            }
        }
    }
    else
    {       // specify table name : tablename.attr
        for (i = 0; i < qdesc->fromlist.len; i++)
        {
            if (join[i].table.aliasname == NULL &&
                    !mdb_strcmp(attr->table.tablename,
                            join[i].table.tablename)
                    && join[i].table.correlated_tbl == NULL)
            {
                if (attr->Hattr == -1)
                {
                    attr->Htable = join[i].tableinfo.tableId;
                    attr->table.correlated_tbl = select;
                    return RET_SUCCESS;
                }

                // now, we've found the table
                for (j = 0; j < join[i].tableinfo.numFields; j++)
                {
                    if (!mdb_strcmp(join[i].fieldinfo[j].fieldName,
                                    attr->attrname))
                    {
                        found = 1;
                        fieldinfo = &join[i].fieldinfo[j];
                        break;
                    }
                }
            }
            else if (join[i].table.aliasname != NULL
                    && !mdb_strcmp(attr->table.tablename,
                            join[i].table.aliasname))
            {
                if (attr->Hattr == -1)
                {
                    attr->Htable = join[i].tableinfo.tableId;
                    attr->table.aliasname = attr->table.tablename;
                    attr->table.tablename =
                            PMEM_STRDUP(join[i].table.tablename);
                    if (!attr->table.tablename)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_OUTOFMEMORY, 0);
                        return RET_ERROR;
                    }
                    attr->table.correlated_tbl = select;

                    return RET_SUCCESS;
                }

                // now, we've found the table
                for (j = 0; j < join[i].tableinfo.numFields; j++)
                {
                    if (!mdb_strcmp(join[i].fieldinfo[j].fieldName,
                                    attr->attrname))
                    {
                        found = 1;
                        fieldinfo = &join[i].fieldinfo[j];
                        attr->table.aliasname = attr->table.tablename;
                        attr->table.tablename =
                                PMEM_STRDUP(join[i].table.tablename);
                        if (!attr->table.tablename)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_OUTOFMEMORY, 0);
                            return RET_ERROR;
                        }
                        break;
                    }
                }
            }
        }
    }       // table_name.attr

    if (found)
    {
        ret = set_attrinfobyname(handle, attr, fieldinfo);

        attr->table.correlated_tbl = select;
    }
    else if (select)
    {
        if (select->kindofwTable & iTABLE_SUBWHERE)
        {
            ret = check_attr_correlation(handle,
                    &select->super_query->planquerydesc.querydesc,
                    attr, expr, expr_pos, select->super_query);
        }
        else if (select->kindofwTable & iTABLE_SUBSET
                && select->super_query->super_query)
        {
            ret = check_attr_correlation(handle,
                    &select->super_query->super_query->planquerydesc.querydesc,
                    attr, expr, expr_pos, select->super_query->super_query);
        }
        else if (select->born_stmt)
        {
            if (select->born_stmt->sqltype == ST_DELETE)
            {
                ret = check_attr_correlation(handle,
                        &select->born_stmt->u._delete.planquerydesc.querydesc,
                        attr, NULL, NULL, NULL);
            }
            else if (select->born_stmt->sqltype == ST_UPDATE)
            {
                ret = check_attr_correlation(handle,
                        &select->born_stmt->u._update.planquerydesc.querydesc,
                        attr, NULL, NULL, NULL);
            }
            else
            {
                sc_assert(0, __FILE__, __LINE__);
            }

            attr->table.correlated_tbl = select;
        }
        else
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDCOLUMN, 0);
            ret = RET_ERROR;
        }
    }
    else
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDCOLUMN, 0);
        ret = RET_ERROR;
    }

    if (ret == RET_ERROR)
    {
        return RET_ERROR;
    }

    return RET_SUCCESS;
}

static void set_resultordinal_switch1(char *name, T_POSTFIXELEM * elem);

/*****************************************************************************/
//! set_resultordinal

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param select(in/out) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
set_resultordinal(T_SELECT * select)
{
    T_LIST_SELECTION *selection;
    T_QUERYRESULT *queryresult;
    T_POSTFIXELEM *elem;

    char buf[max(MAX_DATETIME_STRING_LEN, MAX_TIMESTAMP_STRING_LEN)];
    char *pp;
    char name[64];
    int i;

    queryresult = &select->queryresult;
    selection = &select->planquerydesc.querydesc.selection;

    if (queryresult->len)
    {
        if (queryresult->len > selection->len)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDCOLUMN, 0);
            return RET_ERROR;
        }

        return RET_SUCCESS;
    }

    queryresult->list = (T_RESULTCOLUMN *)
            PMEM_XCALLOC(sizeof(T_RESULTCOLUMN) * (selection->len + 1));
    if (queryresult->list == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return RET_ERROR;
    }

    if (select->planquerydesc.querydesc.querytype == SQT_NORMAL)
    {
        for (i = 0; i < selection->len; i++)
        {
            // set res_name of result
            if (selection->list[i].res_name[0] != '\0')
            {
                sc_memcpy(queryresult->list[i].res_name,
                        selection->list[i].res_name, FIELD_NAME_LENG - 1);
                selection->list[i].res_name[0] = '\0';
                queryresult->list[i].res_name[FIELD_NAME_LENG - 1] = '\0';
            }
            else
            {
                if (selection->list[i].expr.len == 1)
                {
                    pp = queryresult->list[i].res_name;
                    elem = selection->list[i].expr.list[0];
                    if (elem->elemtype == SPT_SUBQUERY
                            || elem->elemtype == SPT_FUNCTION)
                    {
                        sc_strcpy(pp, "?column?");
                    }
                    else if (elem->u.value.valueclass == SVC_VARIABLE)
                    {
                        sc_strncpy(pp, elem->u.value.attrdesc.attrname,
                                FIELD_NAME_LENG - 1);
                        pp[FIELD_NAME_LENG - 1] = '\0';
                        if (IS_BS_OR_NBS(elem->u.value.valuetype))
                            queryresult->list[i].value.attrdesc.len =
                                    elem->u.value.attrdesc.len;
                    }
                    else
                    {
                        set_resultordinal_switch1(name, elem);
                        sc_strncpy(pp, name, FIELD_NAME_LENG - 1);
                        pp[FIELD_NAME_LENG - 1] = '\0';
                    }
                }
                else if (selection->list[i].expr.list[selection->list[i].expr.
                                len - 1]->u.func.type == SFT_CONVERT
                        && selection->list[i].expr.list[0]->u.value.attrdesc.
                        attrname != NULL)
                    sc_strcpy(queryresult->list[i].res_name,
                            selection->list[i].expr.list[0]->u.value.attrdesc.
                            attrname);
                else
                    sc_strcpy(queryresult->list[i].res_name, "?column?");
            }   // selection.list[i].res_name == NULL
        }   // foreach selection list
    }
    else
    {       /* SQT_AGGREGATION */
        char distinct_str[10] = "distinct ";
        char empty_str[1] = "";
        char *d_ptr;

        for (i = 0; i < selection->len; i++)
        {
            if (selection->list[i].res_name[0] != '\0')
            {
                sc_memcpy(queryresult->list[i].res_name,
                        selection->list[i].res_name, FIELD_NAME_LENG - 1);
                selection->list[i].res_name[0] = '\0';
                queryresult->list[i].res_name[FIELD_NAME_LENG - 1] = '\0';
            }
            else
            {
                elem = selection->list[i].expr.list[selection->list[i].expr.
                        len - 1];

                if (selection->list[i].is_distinct)
                    d_ptr = distinct_str;
                else
                    d_ptr = empty_str;

                if (elem->elemtype == SPT_AGGREGATION &&
                        (elem->u.aggr.type == SAT_COUNT ||
                                elem->u.aggr.type == SAT_DCOUNT))
                {
                    if (selection->list[i].expr.len > 1)
                    {
                        if (selection->list[i].expr.len == 2)
                        {
                            elem = selection->list[i].expr.list[0];
                            if (elem->u.value.valueclass == SVC_VARIABLE)
                            {
                                sc_snprintf(name, 64, "count(%s%s)",
                                        d_ptr,
                                        elem->u.value.attrdesc.attrname);
                                sc_memcpy(queryresult->list[i].res_name, name,
                                        FIELD_NAME_LENG - 1);
                                queryresult->list[i].res_name[FIELD_NAME_LENG -
                                        1] = '\0';
                            }
                            else
                            {
                                switch (elem->u.value.valuetype)
                                {
                                case DT_TINYINT:
                                    sc_snprintf(name, 64, "count(%s%d)",
                                            d_ptr, (int) elem->u.value.u.i8);
                                    break;
                                case DT_SMALLINT:
                                    sc_snprintf(name, 64, "count(%s%hd)",
                                            d_ptr, elem->u.value.u.i16);
                                    break;
                                case DT_INTEGER:
                                    sc_snprintf(name, 64, "count(%s%d)",
                                            d_ptr, elem->u.value.u.i32);
                                    break;
                                case DT_BIGINT:
#if defined(WIN32)
                                    sc_snprintf(name, 64, "count(%s%I64d)",
                                            d_ptr, elem->u.value.u.i64);
#else
                                    sc_snprintf(name, 64, "count(%s%lld)",
                                            d_ptr, elem->u.value.u.i64);

#endif
                                    break;
                                case DT_FLOAT:
                                    sc_snprintf(name, 64, "count(%s%g)",
                                            d_ptr, elem->u.value.u.f16);
                                    break;
                                case DT_DOUBLE:
                                    sc_snprintf(name, 64, "count(%s%g)",
                                            d_ptr, elem->u.value.u.f32);
                                    break;
                                case DT_DECIMAL:
                                    sc_snprintf(name, 64, "count(%s%g)",
                                            d_ptr, elem->u.value.u.dec);
                                    break;
                                case DT_CHAR:
                                    sc_snprintf(name, 64, "count(%s%s)",
                                            d_ptr, elem->u.value.u.ptr);
                                    break;
                                case DT_VARCHAR:
                                    sc_snprintf(name, 64, "count(%s%s)",
                                            d_ptr, elem->u.value.u.ptr);
                                    break;
                                case DT_DATETIME:
                                    datetime2char(buf,
                                            elem->u.value.u.datetime);
                                    sc_snprintf(name, 64, "count(%s%s)",
                                            d_ptr, buf);
                                    break;
                                case DT_DATE:
                                    date2char(buf, elem->u.value.u.date);
                                    sc_snprintf(name, 64, "count(%s%s)",
                                            d_ptr, buf);
                                    break;
                                case DT_TIME:
                                    time2char(buf, elem->u.value.u.time);
                                    sc_snprintf(name, 64, "count(%s%s)",
                                            d_ptr, buf);
                                    break;
                                case DT_TIMESTAMP:
                                    timestamp2char(buf,
                                            &elem->u.value.u.timestamp);
                                    sc_snprintf(name, 64, "count(%s%s)",
                                            d_ptr, buf);
                                    break;
                                default:
                                    sc_snprintf(name, 64,
                                            "count(%s?column?)", d_ptr);
                                    break;
                                }
                                sc_memcpy(queryresult->list[i].res_name, name,
                                        FIELD_NAME_LENG - 1);
                                queryresult->list[i].res_name[FIELD_NAME_LENG -
                                        1] = '\0';
                            }
                        }
                        else
                        {
                            sc_snprintf(name, 64, "count(%s?column?)", d_ptr);
                            sc_memcpy(queryresult->list[i].res_name, name,
                                    FIELD_NAME_LENG - 1);
                            queryresult->list[i].res_name[FIELD_NAME_LENG -
                                    1] = '\0';
                        }
                    }
                    else
                        sc_strcpy(queryresult->list[i].res_name, "count(*)");
                }
                else
                {
                    if (selection->list[i].expr.len == 1 ||
                            selection->list[i].expr.len == 2)
                    {
                        elem = selection->list[i].expr.list[0];
                        if (elem->elemtype == SPT_SUBQUERY)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_NOTIMPLEMENTED, 0);
                            return RET_ERROR;
                        }
                        else if (elem->u.value.valueclass == SVC_VARIABLE)
                        {
                            sc_snprintf(name, 64, "%s%s",
                                    d_ptr, elem->u.value.attrdesc.attrname);
                        }
                        else
                        {
                            switch (elem->u.value.valuetype)
                            {
                            case DT_TINYINT:
                                sc_snprintf(name, 64, "%s%d",
                                        d_ptr, (int) elem->u.value.u.i8);
                                break;
                            case DT_SMALLINT:
                                sc_snprintf(name, 64, "%s%hd",
                                        d_ptr, elem->u.value.u.i16);
                                break;
                            case DT_INTEGER:
                                sc_snprintf(name, 64, "%s%d",
                                        d_ptr, elem->u.value.u.i32);
                                break;
                            case DT_BIGINT:
#if defined(WIN32)
                                sc_snprintf(name, 64, "%s%I64d",
                                        d_ptr, elem->u.value.u.i64);
#else
                                sc_snprintf(name, 64, "%s%lld",
                                        d_ptr, elem->u.value.u.i64);

#endif
                                break;
                            case DT_FLOAT:
                                sc_snprintf(name, 64, "%s%g",
                                        d_ptr, elem->u.value.u.f16);
                                break;
                            case DT_DOUBLE:
                                sc_snprintf(name, 64, "%s%g",
                                        d_ptr, elem->u.value.u.f32);
                                break;
                            case DT_DECIMAL:
                                sc_snprintf(name, 64, "%s%g",
                                        d_ptr, elem->u.value.u.dec);
                                break;
                            case DT_CHAR:
                                sc_snprintf(name, 64, "%s%s",
                                        d_ptr, elem->u.value.u.ptr);
                                break;
                            case DT_VARCHAR:
                                sc_snprintf(name, 64, "%s%s",
                                        d_ptr, elem->u.value.u.ptr);
                                break;
                            case DT_DATETIME:
                                datetime2char(buf, elem->u.value.u.datetime);
                                sc_snprintf(name, 64, "%s%s", d_ptr, buf);
                                break;
                            case DT_DATE:
                                date2char(buf, elem->u.value.u.date);
                                sc_snprintf(name, 64, "%s%s", d_ptr, buf);
                                break;
                            case DT_TIME:
                                time2char(buf, elem->u.value.u.time);
                                sc_snprintf(name, 64, "%s%s", d_ptr, buf);
                                break;
                            case DT_TIMESTAMP:
                                timestamp2char(buf,
                                        &elem->u.value.u.timestamp);
                                sc_snprintf(name, 64, "%s%s", d_ptr, buf);
                                break;
                            default:
                                sc_snprintf(name, 64, "%s?column?", d_ptr);
                                break;
                            }
                        }
                    }
                    else
                    {
                        sc_snprintf(name, 64, "%s?column?", d_ptr);
                    }
                    sc_memcpy(queryresult->list[i].res_name, name,
                            FIELD_NAME_LENG - 1);
                    queryresult->list[i].res_name[FIELD_NAME_LENG - 1] = '\0';
                    elem = selection->list[i].expr.list[selection->list[i].
                            expr.len - 1];

                    if (elem->elemtype == SPT_AGGREGATION)
                    {
                        switch (elem->u.aggr.type)
                        {
                        case SAT_MIN:
                            sc_sprintf(queryresult->list[i].res_name,
                                    "min(%s)", name);
                            break;
                        case SAT_MAX:
                            sc_sprintf(queryresult->list[i].res_name,
                                    "max(%s)", name);
                            break;
                        case SAT_SUM:
                            sc_sprintf(queryresult->list[i].res_name,
                                    "sum(%s)", name);
                            break;
                        case SAT_AVG:
                            sc_sprintf(queryresult->list[i].res_name,
                                    "avg(%s)", name);
                            break;
                        default:
                            sc_sprintf(queryresult->list[i].res_name, name);
                            break;
                        }
                    }
                    else
                        sc_sprintf(queryresult->list[i].res_name, name);
                }
            }   // res_name == NULL
        }   // foreach selectoin list item
    }
    queryresult->len = i;

    return RET_SUCCESS;
}

static void
set_resultordinal_switch1(char *name, T_POSTFIXELEM * elem)
{
    switch (elem->u.value.valuetype)
    {
    case DT_TINYINT:
    case DT_SMALLINT:
    case DT_INTEGER:
    case DT_BIGINT:
    case DT_FLOAT:
    case DT_DOUBLE:
    case DT_DECIMAL:
        print_value_as_string(&elem->u.value, name, 64);
        break;
    case DT_CHAR:
    case DT_VARCHAR:
        sc_snprintf(name, 64, "\'%s\'", elem->u.value.u.ptr);
        break;
    default:
        sc_strcpy(name, "?column?");
        break;
    }
}

/*****************************************************************************/
//! expr_has_function

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param expr(in)  :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *
 *****************************************************************************/
static MDB_BOOL
expr_has_function(T_EXPRESSIONDESC * expr)
{
    MDB_INT32 i;

    for (i = 0; i < expr->len; i++)
    {
        if (expr->list[i]->elemtype == SPT_FUNCTION)
            return MDB_TRUE;
        if (expr->list[i]->elemtype == SPT_OPERATOR)
        {
            if (expr->list[i]->u.op.type == SOT_MERGE)  /* how about others? */
                return MDB_TRUE;
        }
    }

    return MDB_FALSE;
}

/*****************************************************************************/
//! set_result_info

/*! \breif  SELECT 문이 수행될때 RESULT COLUMN들을 만드는 곳
 ************************************
 * \param select(in/out)  :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *  - SELECTION LIST에 BYTE/VARBYTE TYPE 추가
 *  - RESULTSET INFO에 COLLATION TYPE을 추가함(TEMP TABLE 때문에)
 *  - CODE에서 주석 제거함
 *
 *****************************************************************************/
static int
set_result_info(T_SELECT * select)
{
    T_LIST_SELECTION *selection;
    T_QUERYRESULT *qresult;

    T_EXPRESSIONDESC *expr;
    T_VALUEDESC resval;
    T_VALUEDESC *resvalptr = NULL;
    register T_RESULTCOLUMN *q_list;

    int i, j, ret = RET_ERROR;

    selection = &select->planquerydesc.querydesc.selection;
    qresult = &select->queryresult;

    for (i = 0; i < qresult->len; i++)
    {
        expr = &selection->list[i].expr;
        q_list = &qresult->list[i];

        if (expr->len)
        {
            if (expr->len == 1 && expr->list[0]->elemtype == SPT_VALUE)
            {
                resvalptr = &(expr->list[0]->u.value);
                ret = 1;
            }
            else
            {
                /* resval structure is initailized within calc_expression(). */
                ret = calc_expression(expr, &resval, MDB_TRUE);
                if (ret < 0)
                {
                    goto RETURN_LABEL;
                }
                resvalptr = &resval;
            }
        }
        else
        {
            sc_memset(&resval, 0, sizeof(T_VALUEDESC));
            resvalptr = &resval;
            ret = 1;
        }
        {
            q_list->value.valueclass = SVC_CONSTANT;
            q_list->value.valuetype = resvalptr->valuetype;

            q_list->value.attrdesc.len = SizeOfType[q_list->value.valuetype];

            switch (q_list->value.valuetype)
            {
            case DT_TINYINT:
                q_list->len = CSIZE_TINYINT;
                break;
            case DT_SMALLINT:
                q_list->len = CSIZE_SMALLINT;
                break;
            case DT_INTEGER:
                q_list->len = CSIZE_INT;
                break;
            case DT_BIGINT:
                q_list->len = CSIZE_BIGINT;
                break;
            case DT_FLOAT:
                q_list->len = CSIZE_FLOAT;
                break;
            case DT_DOUBLE:
                q_list->len = CSIZE_DOUBLE;
                break;
            case DT_DECIMAL:
                /* set DECIMAL display size */
                q_list->len = CSIZE_DECIMAL;
                q_list->value.attrdesc.len = resvalptr->attrdesc.len;
                q_list->value.attrdesc.dec = resvalptr->attrdesc.dec;
                break;
            case DT_CHAR:
            case DT_NCHAR:
            case DT_VARCHAR:
            case DT_NVARCHAR:
                if (expr_has_function(expr))
                {
                    int column_len, title_len;

                    for (j = 0, column_len = 0; j < expr->len; j++)
                    {
                        if (expr->list[j]->elemtype == SPT_VALUE &&
                                IS_BS_OR_NBS(expr->list[j]->u.value.valuetype))
                        {
                            column_len += expr->list[j]->u.value.attrdesc.len;
                        }
                        else if (expr->list[j]->elemtype == SPT_VALUE &&
                                IS_BYTE(expr->list[j]->u.value.valuetype) &&
                                j + 1 < expr->len &&
                                expr->list[j + 1]->elemtype == SPT_FUNCTION)
                        {
                            if (expr->list[j + 1]->u.func.type ==
                                    SFT_HEXASTRING)
                            {
                                if (column_len <
                                        expr->list[j]->u.value.attrdesc.len *
                                        2)
                                    column_len =
                                            expr->list[j]->u.value.attrdesc.
                                            len * 2;
                            }
                            if (expr->list[j + 1]->u.func.type ==
                                    SFT_BINARYSTRING)
                            {
                                if (column_len <
                                        expr->list[j]->u.value.attrdesc.len *
                                        8)
                                    column_len =
                                            expr->list[j]->u.value.attrdesc.
                                            len * 8;
                            }
                        }
                    }
                    title_len = sc_strlen(q_list->res_name);
                    if (resvalptr->attrdesc.len < column_len)
                        resvalptr->attrdesc.len = column_len;
                    if (resvalptr->attrdesc.len < title_len)
                        resvalptr->attrdesc.len = title_len;
                }

                q_list->len = resvalptr->attrdesc.len;
                q_list->value.attrdesc.len = resvalptr->attrdesc.len;
                q_list->value.attrdesc.fixedlen = resvalptr->attrdesc.fixedlen;

                q_list->value.call_free = 0;
                break;

            case DT_BYTE:
            case DT_VARBYTE:
                if (expr_has_function(expr))
                {
                    int column_len, title_len;

                    for (j = 0, column_len = 0; j < expr->len; j++)
                    {
                        if (expr->list[j]->elemtype == SPT_VALUE &&
                                expr->list[j]->u.value.valueclass
                                == SVC_VARIABLE &&
                                IS_BYTE(expr->list[j]->u.value.valuetype))
                        {
                            if (column_len
                                    < expr->list[j]->u.value.attrdesc.len)
                                column_len =
                                        expr->list[j]->u.value.attrdesc.len;
                        }
                    }
                    title_len = sc_strlen(q_list->res_name);
                    if (resvalptr->attrdesc.len < column_len)
                        resvalptr->attrdesc.len = column_len;
                    if (resvalptr->attrdesc.len < title_len)
                        resvalptr->attrdesc.len = title_len;
                }

                q_list->len = resvalptr->attrdesc.len;
                q_list->value.attrdesc.len = resvalptr->attrdesc.len;
                q_list->value.call_free = 0;
                break;
            case DT_DATETIME:
                q_list->len = MAX_DATETIME_STRING_LEN - 1;
                q_list->value.attrdesc.len = resvalptr->attrdesc.len;
                break;
            case DT_DATE:
                q_list->len = MAX_DATE_STRING_LEN - 1;
                q_list->value.attrdesc.len = resvalptr->attrdesc.len;
                break;
            case DT_TIME:
                q_list->len = MAX_TIME_STRING_LEN - 1;
                q_list->value.attrdesc.len = resvalptr->attrdesc.len;
                break;
            case DT_TIMESTAMP:
                q_list->len = MAX_TIMESTAMP_STRING_LEN - 1;
                q_list->value.attrdesc.len = resvalptr->attrdesc.len;
                break;
            case DT_OID:
                q_list->len = CSIZE_INT;
                q_list->value.attrdesc.len = sizeof(OID);
                break;
            case DT_NULL_TYPE:
                q_list->len = 0;
                q_list->value.attrdesc.len = 0;
                break;
            default:
#ifdef MDB_DEBUG
                __SYSLOG("set_result_info:%d\n", q_list->value.valuetype);
#endif
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTYPE, 0);
                ret = RET_ERROR;
                goto RETURN_LABEL;
            }

            q_list->value.attrdesc.collation = resvalptr->attrdesc.collation;
        }

        if (resvalptr == &resval)
            sql_value_ptrFree(&resval);

        for (j = 0; j < expr->len; j++)
        {
            if (expr->list[j]->elemtype == SPT_VALUE &&
                    expr->list[j]->u.value.valueclass == SVC_VARIABLE &&
                    IS_ALLOCATED_TYPE(expr->list[j]->u.value.valuetype))
            {
                sql_value_ptrFree(&expr->list[j]->u.value);
            }
        }
    }

    ret = RET_SUCCESS;

  RETURN_LABEL:
    if (ret < 0)
        ++i;
    for (--i; i >= 0; --i)
    {
        expr = &selection->list[i].expr;

        for (j = 0; j < expr->len; j++)
        {
            if (expr->list[j]->elemtype == SPT_VALUE &&
                    expr->list[j]->u.value.valueclass == SVC_VARIABLE)
            {
                sql_value_ptrFree(&expr->list[j]->u.value);
            }
        }
    }

    if (resvalptr == &resval)
        sql_value_ptrFree(&resval);

    return ret;
}


/*****************************************************************************/
//! check_subselect_from

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
 *
 *****************************************************************************/
static int
check_subselect_from(int handle, T_SELECT * select)
{
    T_QUERYDESC *qdesc;
    T_JOINTABLEDESC *join;
    char *left_table, *right_table;
    int i, j;

    qdesc = &select->planquerydesc.querydesc;
    join = qdesc->fromlist.list;

    /***************************************************************
    Following fields must be initialized in the parsing process.
        join[i].tableinfo, join[i].fieldinfo
    ***************************************************************/

    if (select->msync_flag)
    {
        if ((join[0].tableinfo.flag & _CONT_MSYNC) == 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_INVALID_MSYNCTABLE, 0);
            return RET_ERROR;
        }
    }

    for (i = 0; i < qdesc->fromlist.len; i++)
    {
        if (join[i].table.aliasname)
        {
            if (sc_strlen(join[i].table.aliasname) > REL_NAME_LENG)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_MAX_ALIAS_NAME_LEN_EXEEDED, 0);
                return RET_ERROR;
            }
        }
    }

    for (i = 0; i < qdesc->fromlist.len; i++)
    {
        // 1.1 table(alias) name conflict checking
        if (join[i].table.aliasname)
            right_table = join[i].table.aliasname;
        else
            right_table = join[i].table.tablename;

        for (j = 0; j < i; j++)
        {
            if (join[j].table.aliasname)
                left_table = join[j].table.aliasname;
            else
                left_table = join[j].table.tablename;

            if (!mdb_strcmp(left_table, right_table))
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_NOTUNIQUETABLEALIAS, 0);
                return RET_ERROR;
            }
        }

        // 1.2 if tmp table from a subquery, create tmp table ...
        if (join[i].table.correlated_tbl != NULL)
        {
            char *tmp_table_nm = PMEM_XCALLOC(REL_NAME_LENG);

            if (dbi_Trans_NextTempName(handle, 1, tmp_table_nm,
                            join[i].table.tablename) < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
                return RET_ERROR;
            }
            if (((join[i].tableinfo.flag & _CONT_VIEW) == 0)
                    || (join[i].table.aliasname == NULL))
                join[i].table.aliasname = join[i].table.tablename;
            join[i].table.tablename = tmp_table_nm;
            if (create_subquery_table(&handle, join[i].table.correlated_tbl,
                            join[i].table.tablename) != RET_SUCCESS)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
                return RET_ERROR;
            }
        }

        // 1.3 given tbl exist ? : get field informations of givem table
        if (join[i].fieldinfo == NULL)
        {
            if (dbi_Schema_GetTableFieldsInfo(handle, join[i].table.tablename,
                            &(join[i].tableinfo), &(join[i].fieldinfo)) <= 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTABLE,
                        0);
                return RET_ERROR;
            }
        }
    }
    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_subselect_join

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)    :
 * \param select(in)    :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static int
check_subselect_join(int handle, T_SELECT * select)
{
    T_QUERYDESC *qdesc;
    T_POSTFIXELEM *elem;
    int i, j;
    int expr_pos, ret;

    qdesc = &select->planquerydesc.querydesc;

    for (i = 0; i < qdesc->fromlist.len; i++)
    {
        for (j = 0; j < qdesc->fromlist.list[i].condition.len; j++)
        {
            elem = qdesc->fromlist.list[i].condition.list[j];

            if ((elem->elemtype != SPT_VALUE && elem->elemtype != SPT_NONE)
                    || elem->u.value.valueclass == SVC_CONSTANT)
                continue;

            // now elemtype is SPT_VALUE (or SPT_NONE ??)
            // check existance of attr and it's ambiguity
            expr_pos = j;
            ret = check_attr_correlation(handle, qdesc,
                    &elem->u.value.attrdesc,
                    &qdesc->fromlist.list[i].condition, &expr_pos, select);
            if (ret != RET_SUCCESS)
            {
                return RET_ERROR;
            }

            /* expression의 position이 변경된 경우 alias 적용됨 */
            if (expr_pos != j)
            {
                j = expr_pos;
                continue;
            }

            elem->u.value.valuetype = elem->u.value.attrdesc.type;

            // TODO : correl_attr과 subquery를 가리키는 T_SELECT를 free
            //            하는곳 검사

            // join 조건에는 correlated column이 오면 안 된 다.
            if (elem->u.value.attrdesc.table.correlated_tbl != select)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDCOLUMN, 0);
                return RET_ERROR;
            }
        }
    }
    return RET_SUCCESS;
}

static int
check_condition(int handle, T_QUERYDESC * qdesc, T_SELECT * select)
{
    T_POSTFIXELEM *elem;
    int i, ret;
    int expr_pos;

    for (i = 0; i < qdesc->condition.len; i++)
    {
        elem = qdesc->condition.list[i];
        if (elem->elemtype == SPT_VALUE && IS_BYTE(elem->u.value.valuetype))
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_RESTRICT_WHERE_CLAUSE, 0);
            return RET_ERROR;
        }

        if (elem->elemtype == SPT_OPERATOR)
        {
            /* null 처리를 위해 prepare시 in 처리 type convert skip */
            if (elem->u.op.type == SOT_LIKE
                    || elem->u.op.type == SOT_ILIKE
                    || elem->u.op.type == SOT_IN || elem->u.op.type == SOT_NIN)
            {
                continue;
            }

            if (elem->u.op.argcnt == 2)
            {
                ret = type_convert(&(qdesc->condition.list[i - 1]->u.value),
                        &(qdesc->condition.list[i - 2]->u.value));
                if (ret < 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    return RET_ERROR;
                }
            }
            if (elem->u.op.type == SOT_PLUS ||
                    elem->u.op.type == SOT_MINUS ||
                    elem->u.op.type == SOT_TIMES ||
                    elem->u.op.type == SOT_DIVIDE ||
                    elem->u.op.type == SOT_EXPONENT ||
                    elem->u.op.type == SOT_MODULA ||
                    elem->u.op.type == SOT_MERGE)
            {

                ret = preconvert_expression(elem->u.op.argcnt,
                        &(qdesc->condition), &i);

                if (ret < 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    return RET_ERROR;
                }
            }
        }

        /* condition에서 subquery를 사용하는 경우 exists, not exists를
           제외한 subquery의 selection 개수는 1개이며
           1 개보다 많은 경우 error 처리 */
        if (elem->elemtype == SPT_SUBQUERY)
        {
            if (elem->u.subq->planquerydesc.querydesc.selection.len > 1)
            {
                if (qdesc->condition.list[i + 1]->elemtype == SPT_SUBQ_OP &&
                        (qdesc->condition.list[i + 1]->u.op.type == SOT_EXISTS
                                || qdesc->condition.list[i + 1]->u.op.type ==
                                SOT_NEXISTS))
                {
                }
                else
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_NOTMATCHCOLUMNCOUNT, 0);
                    return RET_ERROR;
                }
            }

            continue;
        }

        if (elem->elemtype == SPT_AGGREGATION)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTIMPLEMENTED, 0);
            return RET_ERROR;

        }

        if (elem->elemtype != SPT_VALUE && elem->elemtype != SPT_NONE &&
                elem->elemtype != SPT_FUNCTION)
            continue;

        if (elem->elemtype == SPT_VALUE &&
                elem->u.value.valueclass == SVC_CONSTANT)
        {
            if (IS_BYTE(elem->u.value.valuetype))
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDEXPRESSION, 0);
                return RET_ERROR;
            }

            continue;
        }

        if (elem->elemtype == SPT_FUNCTION)
        {
            if (elem->u.func.type == SFT_CHARTOHEX)
            {
                MDB_SYSLOG(("sql error string : Special function CHARTOHEX() "
                                "shouldn't be used in WHERE clause"));
                return RET_ERROR;
            }

            ret = preconvert_expression(elem->u.func.argcnt,
                    &(qdesc->condition), &i);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                return RET_ERROR;
            }
            continue;
        }

        if (elem->u.value.valueclass == SVC_VARIABLE
                && elem->u.value.attrdesc.type == DT_OID
                && select->msync_flag == DELETE_SLOT)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_NOTALLOW_MSYNC_RID, 0);
            return RET_ERROR;
        }

        // what if elem->elemtype == SPT_NONE ?
        expr_pos = i;
        ret = check_attr_correlation(handle, qdesc, &elem->u.value.attrdesc,
                &qdesc->condition, &expr_pos, select);
        if (ret != RET_SUCCESS)
        {
            return ret;
        }

        /* expression의 position이 변경된 경우 alias 적용됨 */
        if (expr_pos != i)
        {
            i = expr_pos;
            continue;
        }

        elem->u.value.valuetype = elem->u.value.attrdesc.type;
        if (elem->elemtype == SPT_VALUE && (IS_BYTE(elem->u.value.valuetype) ||
                        IS_BYTE(elem->u.value.attrdesc.type)))
        {
            int j, arg_cnt = 0, is_func_arg = 0;

            for (j = i + 1; j < qdesc->condition.len; j++)
            {
                if (qdesc->condition.list[j]->elemtype == SPT_OPERATOR)
                    break;

                if (qdesc->condition.list[j]->elemtype == SPT_FUNCTION)
                {
                    arg_cnt += qdesc->condition.list[j]->u.func.argcnt;
                    if (j - i <= arg_cnt)
                    {
                        is_func_arg = 1;
                        break;
                    }
                }
            }

            if (!is_func_arg && !(i + 1 < qdesc->condition.len &&
                            (qdesc->condition.list[i + 1]->elemtype ==
                                    SPT_OPERATOR)
                            && (qdesc->condition.list[i + 1]->u.op.type ==
                                    SOT_ISNULL
                                    || qdesc->condition.list[i +
                                            1]->u.op.type == SOT_ISNOTNULL)))
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_RESTRICT_WHERE_CLAUSE, 0);
                return RET_ERROR;
            }
        }

        // TODO : correl_attr과 subquery를 가리키는 T_SELECT를 free 하는곳 검사
        if (elem->u.value.attrdesc.table.correlated_tbl != select)
        {
            select->kindofwTable |= iTABLE_CORRELATED;
            set_super_tbl_correlated(select);
        }
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_subselect_condition

/*! \breif  where clause의 column들을 검사함
 ************************************
 * \param handle(in)    :
 * \param select(in/out)    :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 자료구조 변경
 *  - byte/nbyte type은 where 절에 올 수 없다
 *
 *****************************************************************************/
static int
check_subselect_condition(int handle, T_SELECT * select)
{
    T_QUERYDESC *qdesc;
    int ret;

    qdesc = &select->planquerydesc.querydesc;

    ret = check_condition(handle, qdesc, select);
    if (ret != RET_SUCCESS)
    {
        return RET_ERROR;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_subselect_groupby

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)    :
 * \param select(in)    :
 ************************************
 * \return  return_value
 ************************************
 *  \note
 *  - T_EXPRESSIONDESC 자료구조 변경
 *  - BYTE/VARBYTE의 경우 GROUPBY CLAUSE에 사용 불가
 *****************************************************************************/
static int
check_subselect_groupby(int handle, T_SELECT * select)
{
    T_QUERYDESC *qdesc;
    T_GROUPBYITEM *gitem;
    T_POSTFIXELEM *elem;
    int i, j, ret;

    qdesc = &select->planquerydesc.querydesc;

    if (MAX_INDEX_FIELD_NUM < qdesc->groupby.len)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                SQL_E_EXCEED_GROUPBY_FILEDS, 0);
        return RET_ERROR;
    }
    for (i = 0; i < qdesc->groupby.len; i++)
    {
        gitem = &qdesc->groupby.list[i];

        ret = check_attr_correlation(handle, qdesc, &gitem->attr, NULL, NULL,
                select);
        if (ret != RET_SUCCESS)
        {
            return ret;
        }

        if (IS_BYTE(gitem->attr.type))
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_RESTRICT_GROUPBY_CLAUSE, 0);
            return RET_ERROR;
        }


        if (gitem->attr.table.correlated_tbl != select)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDCOLUMN, 0);
            return RET_ERROR;
        }

        if (gitem->s_ref < 0)
        {
            for (j = 0; j < qdesc->selection.len; j++)
            {
                if (qdesc->selection.list[j].expr.len > 1)
                    continue;

                elem = qdesc->selection.list[j].expr.list[0];
                if (elem->elemtype == SPT_VALUE &&
                        elem->u.value.valueclass == SVC_VARIABLE)
                {
                    if (!mdb_strcmp(gitem->attr.attrname,
                                    elem->u.value.attrdesc.attrname))
                    {
                        if (gitem->attr.table.aliasname &&
                                elem->u.value.attrdesc.table.aliasname)
                        {
                            if (!mdb_strcmp(gitem->attr.table.aliasname,
                                            elem->u.value.attrdesc.table.
                                            aliasname))
                            {
                                gitem->s_ref = j;
                                break;
                            }
                        }
                        else if (!mdb_strcmp(gitem->attr.table.tablename,
                                        elem->u.value.attrdesc.table.
                                        tablename))
                        {
                            gitem->s_ref = j;
                            break;
                        }
                    }
                }
            }
            if (gitem->s_ref < 0)
                ++qdesc->aggr_info.n_not_sref_gby;
        }
        else
        {
            for (j = 0; j < qdesc->selection.list[gitem->s_ref].expr.len; j++)
            {
                elem = qdesc->selection.list[gitem->s_ref].expr.list[j];
                if (elem->elemtype == SPT_AGGREGATION)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_NOTGROUPBYEXPRESSION, 0);
                    return RET_ERROR;
                }
            }
        }
    }
    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_subselect_having

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)    :
 * \param select(in/out):
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 자료구조 변경
 *  - BYTE/VARBYTE TYPE인 경우 HAVING CLAUSE에 나올수 없다
 *****************************************************************************/
static int
check_subselect_having(int handle, T_SELECT * select)
{
    T_QUERYDESC *qdesc;
    T_EXPRESSIONDESC *expr;
    T_POSTFIXELEM *elem;
    T_ATTRDESC *attr;
    int agg_arg_scope;
    int i, j;
    int start = 0;
    int n_expr = 0;
    int ret, expr_pos, expr_len;

    qdesc = &select->planquerydesc.querydesc;

    if (qdesc->groupby.having.len)
    {
        expr = &qdesc->groupby.having;
        agg_arg_scope = expr->len;
        for (i = expr->len - 1; i >= 0; i--)
        {
            elem = expr->list[i];

            if (elem->elemtype == SPT_VALUE &&
                    IS_BYTE(elem->u.value.valuetype))
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_RESTRICT_HAVING_CLAUSE, 0);
                return RET_ERROR;
            }

            if (elem->elemtype == SPT_AGGREGATION)
            {
                if (start == -1)
                {
                    start = i + 1;
                }

                n_expr += (i - elem->u.aggr.significant + 1);
                ++qdesc->having.len;
                ++qdesc->aggr_info.len;

                if (elem->u.aggr.type == SAT_AVG)
                    ++qdesc->aggr_info.n_hv_avg;

                if (elem->is_distinct)
                {
                    qdesc->is_distinct = 1;
                    ++qdesc->distinct.len;
                }
                agg_arg_scope = i - elem->u.aggr.significant;

            }
            else if (elem->elemtype == SPT_SUBQUERY ||
                    elem->elemtype == SPT_SUBQ_OP)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_NOTIMPLEMENTED, 0);
                return RET_ERROR;

            }
            else if (elem->elemtype == SPT_VALUE &&
                    elem->u.value.valueclass == SVC_VARIABLE)
            {
                attr = &elem->u.value.attrdesc;

                expr_pos = i;
                expr_len = expr->len;
                ret = check_attr_correlation(handle, qdesc, attr, expr,
                        &expr_pos, select);
                if (ret != RET_SUCCESS)
                {
                    return RET_ERROR;
                }

                /* expression의 position이 변경된 경우 alias 적용됨 */
                if (expr_pos != i)
                {
                    /* having의 경우 뒤에서 부터 처리되기 때문에 expression
                       전체 길이 변화를 이용하여 현재 position 계산 */
                    i += expr->len - expr_len + 2;
                    continue;
                }

                elem->u.value.valuetype = attr->type;
                /* check if current item is not a correlated column */
                if (attr->table.correlated_tbl != select)
                {
                    if (attr->table.correlated_tbl != select->super_query)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_INVALIDCOLUMN, 0);
                        return RET_ERROR;
                    }
                }

                elem->u.value.valuetype = attr->type;

                if (i >= agg_arg_scope)
                    continue;

                ++qdesc->having.len;

                /* check if current item is a group by item */
                for (j = 0; j < qdesc->groupby.len; j++)
                {
                    if (!mdb_strcmp(qdesc->groupby.list[j].attr.attrname,
                                    attr->attrname))
                        break;
                }
                if (j >= qdesc->groupby.len)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_NOTGROUPBYEXPRESSION, 0);
                    return RET_ERROR;
                }
            }
        }
        n_expr += qdesc->groupby.having.len - start;
        if (qdesc->distinct.len >= MAX_DISTINCT_TABLE_NUM)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_TOOMANYDISTINCT, 0);
            return RET_ERROR;
        }
    }

    if (qdesc->aggr_info.n_expr < n_expr)
    {
        qdesc->aggr_info.n_expr = n_expr;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_subselect_orderby

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)    :
 * \param select(in/out):
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 자료구조 변경
 *  - ORDER BY 절에서 ALIAS가 사용된 경우
 *  - BYTE/VARBYTE TYPE은 ORDER BY CLAUSE에는 나올수 없다
 *  - group by 절에 나오지 않은 칼럼을 order by에 넣었을 때
 *      ERROR 리턴하도록 수정
 *  - orderby 절이 selection list에 존재하는 경우
 *      아래의 루틴 처럼 groupby를 체크 할 필요없다
 *  - caution
 *      orderby, groupby가 나오는 경우, orderby의 column은 
 *      반드시 selection list or groupby list 둘중 하나에 존재해야 함
 *
 *****************************************************************************/
static int
check_subselect_orderby(int handle, T_SELECT * select)
{
    T_QUERYDESC *qdesc;
    T_ORDERBYITEM *oitem;
    T_JOINTABLEDESC *join;
    T_POSTFIXELEM *elem = NULL;
    int tPosi = 0, fPosi = 0;
    int ambiguity;
    int i, j, k;
    T_ATTRDESC *attr;

    qdesc = &select->planquerydesc.querydesc;
    join = qdesc->fromlist.list;

    if (MAX_INDEX_FIELD_NUM < qdesc->orderby.len)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                SQL_E_EXCEED_ORDERBY_FILEDS, 0);
        return RET_ERROR;
    }
    for (i = 0; i < qdesc->orderby.len; i++)
    {
        oitem = &qdesc->orderby.list[i];

        if (oitem->param_idx > 0)
            continue;
        if (qdesc->select_star && oitem->s_ref != -1)
        {
            if (qdesc->selection.len > 0 &&
                    qdesc->selection.len <= oitem->s_ref)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDITEMINDEX, 0);
                return RET_ERROR;
            }

            elem = qdesc->selection.list[oitem->s_ref].expr.list[0];
            if (qdesc->selection.list[oitem->s_ref].expr.len == 1 &&
                    elem->elemtype == SPT_VALUE &&
                    elem->u.value.valueclass == SVC_VARIABLE &&
                    !oitem->attr.attrname)
            {
                attr = &elem->u.value.attrdesc;
                if (attr->attrname && attr->table.tablename)
                {
                    oitem->attr.attrname = PMEM_STRDUP(attr->attrname);
                    if (!oitem->attr.attrname)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_OUTOFMEMORY, 0);
                        return RET_ERROR;
                    }
                    oitem->attr.table.tablename =
                            PMEM_STRDUP(attr->table.tablename);
                    if (!oitem->attr.table.tablename)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_OUTOFMEMORY, 0);
                        return RET_ERROR;
                    }
                }
            }

            continue;
        }

        if (oitem->attr.table.tablename)
        {
            for (j = 0; j < qdesc->fromlist.len; j++)
            {
                if (!mdb_strcmp(oitem->attr.table.tablename,
                                join[j].table.tablename))
                {
                    for (k = 0; k < join[j].tableinfo.numFields; k++)
                    {
                        if (!mdb_strcmp(join[j].fieldinfo[k].fieldName,
                                        oitem->attr.attrname))
                            break;
                    }
                    if (k == join[j].tableinfo.numFields)
                    {   // not found
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_INVALIDCOLUMN, 0);
                        return RET_ERROR;
                    }
                    tPosi = j;
                    fPosi = k;
                    break;
                }
                else if (join[j].table.aliasname != NULL)
                {
                    if (!mdb_strcmp(oitem->attr.table.tablename,
                                    join[j].table.aliasname))
                    {
                        for (k = 0; k < join[j].tableinfo.numFields; k++)
                        {
                            if (!mdb_strcmp(join[j].fieldinfo[k].fieldName,
                                            oitem->attr.attrname))
                                break;
                        }
                        if (k == join[j].tableinfo.numFields)
                        {       // not found
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_INVALIDCOLUMN, 0);
                            return RET_ERROR;
                        }
                        oitem->attr.table.aliasname =
                                oitem->attr.table.tablename;
                        oitem->attr.table.tablename =
                                PMEM_STRDUP(join[j].table.tablename);
                        if (!oitem->attr.table.tablename)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_OUTOFMEMORY, 0);
                            return RET_ERROR;
                        }
                        tPosi = j;
                        fPosi = k;
                        break;
                    }
                }
            }
            if (j == qdesc->fromlist.len)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDCOLUMN, 0);
                return RET_ERROR;
            }
        }
        else
        {
            ambiguity = 0;
            attr = NULL;
            if (oitem->s_ref > -1)
            {
                j = oitem->s_ref;
                for (k = 0; k < qdesc->selection.list[j].expr.len; ++k)
                {
                    elem = qdesc->selection.list[j].expr.list[k];
                    if ((elem->elemtype == SPT_VALUE) &&
                            (elem->u.value.valueclass == SVC_VARIABLE))
                    {
                        attr = &elem->u.value.attrdesc;
                        ambiguity = 1;
                        break;
                    }
                }
                if (ambiguity == 0)
                    continue;
            }
            else
            {
                for (j = 0; j < qdesc->selection.len; ++j)
                {
                    if (qdesc->selection.list[j].res_name[0] == '\0')
                    {
                        elem = qdesc->selection.list[j].expr.list[0];

                        if (qdesc->selection.list[j].expr.len != 1 ||
                                !(elem->elemtype == SPT_VALUE &&
                                        elem->u.value.valueclass ==
                                        SVC_VARIABLE))
                            continue;

                        if (!mdb_strcmp(oitem->attr.attrname,
                                        elem->u.value.attrdesc.attrname))
                        {
                            if (ambiguity)
                            {
                                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                        SQL_E_AMBIGUOUSCOLUMN, 0);

                                return RET_ERROR;
                            }
                            attr = &elem->u.value.attrdesc;
                            oitem->s_ref = j;
                            ambiguity = 1;
                        }
                    }
                    else if (!mdb_strcmp(qdesc->selection.list[j].res_name,
                                    oitem->attr.attrname))
                    {
                        if (ambiguity)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_AMBIGUOUSCOLUMN, 0);

                            return RET_ERROR;
                        }

                        ambiguity = 1;

                        for (k = 0; k < qdesc->selection.list[j].expr.len; ++k)
                        {
                            elem = qdesc->selection.list[j].expr.list[k];
                            if ((elem->elemtype == SPT_VALUE) &&
                                    (elem->u.value.valueclass == SVC_VARIABLE))
                                break;
                        }
                        if (k == qdesc->selection.list[j].expr.len)
                        {
                            oitem->s_ref = j;
                            attr = NULL;
                            continue;
                        }

                        attr = &elem->u.value.attrdesc;
                        oitem->s_ref = j;
                    }
                }
            }

            if (ambiguity)
            {
                if (attr == NULL)
                    continue;

                if (IS_BYTE(attr->type))
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_RESTRICT_ORDERBY_CLAUSE, 0);
                    return RET_ERROR;
                }

                oitem->attr.attrname = PMEM_STRDUP(attr->attrname);
                if (!oitem->attr.attrname)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_OUTOFMEMORY, 0);
                    return RET_ERROR;
                }
                oitem->attr.table.tablename =
                        PMEM_STRDUP(attr->table.tablename);
                if (!oitem->attr.table.tablename)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_OUTOFMEMORY, 0);
                    return RET_ERROR;
                }

                oitem->attr.posi.ordinal = attr->posi.ordinal;
                oitem->attr.posi.offset = attr->posi.offset;
                oitem->attr.flag = attr->flag;
                oitem->attr.type = attr->type;
                oitem->attr.len = attr->len;
                oitem->attr.fixedlen = attr->fixedlen;
                oitem->attr.dec = attr->dec;
                if (!IS_BS_OR_NBS(attr->type)
                        || oitem->attr.collation == MDB_COL_NONE)
                {
                    oitem->attr.collation = attr->collation;
                }
                oitem->attr.Htable = attr->Htable;
                oitem->attr.Hattr = attr->Hattr;

                continue;
            }
            else
            {
                for (j = 0; j < qdesc->fromlist.len; j++)
                {
                    for (k = 0; k < join[j].tableinfo.numFields; k++)
                    {
                        if (!mdb_strcmp(join[j].fieldinfo[k].fieldName,
                                        oitem->attr.attrname))
                            break;
                    }
                    if (k < join[j].tableinfo.numFields)
                    {   // found it
                        if (ambiguity == 0)
                        {
                            oitem->attr.table.tablename =
                                    PMEM_STRDUP(join[j].table.tablename);
                            if (!oitem->attr.table.tablename)
                            {
                                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                        SQL_E_OUTOFMEMORY, 0);
                                return RET_ERROR;
                            }
                            oitem->attr.table.aliasname = NULL;
                            ambiguity = 1;
                            tPosi = j;
                            fPosi = k;
                        }
                        else
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_AMBIGUOUSCOLUMN, 0);

                            return RET_ERROR;
                        }
                    }
                }

                if (ambiguity == 0 && j == qdesc->fromlist.len)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_INVALIDCOLUMN, 0);
                }
            }
            if (ambiguity == 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDCOLUMN, 0);
                return RET_ERROR;
            }
        }

        if (set_attrinfobyname(handle, &oitem->attr,
                        &(join[tPosi].fieldinfo[fPosi])) == RET_ERROR)
        {
            return RET_ERROR;
        }
        if (oitem->s_ref < 0)
        {
            for (j = 0; j < qdesc->selection.len; j++)
            {
                if (qdesc->selection.list[j].expr.len > 1)
                    continue;

                elem = qdesc->selection.list[j].expr.list[0];
                if (elem->elemtype == SPT_VALUE &&
                        elem->u.value.valueclass == SVC_VARIABLE)
                {
                    if (!mdb_strcmp(oitem->attr.attrname,
                                    elem->u.value.attrdesc.attrname) &&
                            !mdb_strcmp(oitem->attr.table.tablename,
                                    elem->u.value.attrdesc.table.tablename))
                    {
                        oitem->s_ref = j;
                        break;
                    }
                }
            }
        }

        if (IS_BYTE(oitem->attr.type))
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_RESTRICT_ORDERBY_CLAUSE, 0);
            return RET_ERROR;
        }
    }
    return RET_SUCCESS;
}

static int
check_subselect_set_orderby(int handle, T_SELECT * select)
{
    T_QUERYDESC *qdesc;
    T_ORDERBYITEM *oitem;
    T_JOINTABLEDESC *join;
    T_POSTFIXELEM *elem = NULL;
    T_ORDERBYDESC *orderby;
    T_QUERYRESULT *queryresult;
    int ambiguity;
    int i, j, k;
    T_ATTRDESC *attr;

    qdesc = &select->planquerydesc.querydesc;
    orderby = &qdesc->orderby;
    queryresult = &qdesc->setlist.list[0]->u.subq->queryresult;
    qdesc = &qdesc->setlist.list[0]->u.subq->planquerydesc.querydesc;

    if (MAX_INDEX_FIELD_NUM < orderby->len)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                SQL_E_EXCEED_ORDERBY_FILEDS, 0);
        return RET_ERROR;
    }

    for (i = 0; i < orderby->len; i++)
    {
        oitem = &orderby->list[i];

        if (oitem->s_ref != -1)
        {
            if (oitem->s_ref >= qdesc->selection.len)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDITEMINDEX, 0);
                return RET_ERROR;
            }
        }
        else if (oitem->attr.table.tablename)
        {
            for (j = 0; j < qdesc->fromlist.len; j++)
            {
                join = qdesc->fromlist.list + j;
                if (!mdb_strcmp(oitem->attr.table.tablename,
                                join->table.tablename))
                {
                    for (k = 0; k < join->tableinfo.numFields; k++)
                    {
                        if (!mdb_strcmp(join->fieldinfo[k].fieldName,
                                        oitem->attr.attrname))
                            break;
                    }
                    if (k == join->tableinfo.numFields)
                    {   // not found
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_INVALIDCOLUMN, 0);
                        return RET_ERROR;
                    }
                    break;
                }
                else if (join->table.aliasname != NULL)
                {
                    if (!mdb_strcmp(oitem->attr.table.tablename,
                                    join->table.aliasname))
                    {
                        for (k = 0; k < join->tableinfo.numFields; k++)
                        {
                            if (!mdb_strcmp(join->fieldinfo[k].fieldName,
                                            oitem->attr.attrname))
                                break;
                        }
                        if (k == join->tableinfo.numFields)
                        {       // not found
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_INVALIDCOLUMN, 0);
                            return RET_ERROR;
                        }
                        oitem->attr.table.aliasname =
                                oitem->attr.table.tablename;
                        oitem->attr.table.tablename =
                                PMEM_STRDUP(join->table.tablename);
                        if (!oitem->attr.table.tablename)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_OUTOFMEMORY, 0);
                            return RET_ERROR;
                        }
                        break;
                    }
                }
            }
            if (j == qdesc->fromlist.len)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDCOLUMN, 0);
                return RET_ERROR;
            }
        }
        else
        {
            ambiguity = 0;
            for (j = 0; j < qdesc->fromlist.len; j++)
            {
                join = qdesc->fromlist.list + j;
                for (k = 0; k < join->tableinfo.numFields; k++)
                {
                    if (!mdb_strcmp(join->fieldinfo[k].fieldName,
                                    oitem->attr.attrname))
                        break;
                }
                if (k < join->tableinfo.numFields)
                {       // found it
                    if (ambiguity == 0)
                    {
                        oitem->attr.table.tablename =
                                PMEM_STRDUP(join->table.tablename);
                        if (!oitem->attr.table.tablename)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_OUTOFMEMORY, 0);
                            return RET_ERROR;
                        }
                        oitem->attr.table.aliasname = NULL;
                        ambiguity = 1;
                    }
                    else
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_AMBIGUOUSCOLUMN, 0);

                        return RET_ERROR;
                    }
                }
            }

            if (ambiguity == 0)
            {
                for (j = 0; j < qdesc->selection.len; ++j)
                {
                    if (!mdb_strcmp(queryresult->list[j].res_name,
                                    oitem->attr.attrname))
                    {
                        for (k = 0; k < qdesc->selection.list[j].expr.len; ++k)
                        {
                            elem = qdesc->selection.list[j].expr.list[k];
                            if ((elem->elemtype == SPT_VALUE) &&
                                    (elem->u.value.valueclass == SVC_VARIABLE))
                            {
                                break;
                            }
                        }
                        if (k == qdesc->selection.list[j].expr.len)
                        {
                            continue;   // j로 복귀
                        }

                        attr = &elem->u.value.attrdesc;

                        oitem->attr.attrname = PMEM_STRDUP(attr->attrname);
                        if (!oitem->attr.attrname)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_OUTOFMEMORY, 0);
                            return RET_ERROR;
                        }
                        oitem->attr.table.tablename =
                                PMEM_STRDUP(attr->table.tablename);
                        if (!oitem->attr.table.tablename)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_OUTOFMEMORY, 0);
                            return RET_ERROR;
                        }

                        oitem->attr.posi.ordinal = attr->posi.ordinal;
                        oitem->attr.posi.offset = attr->posi.offset;
                        oitem->attr.flag = attr->flag;
                        oitem->attr.type = attr->type;
                        oitem->attr.len = attr->len;
                        oitem->attr.fixedlen = attr->fixedlen;
                        oitem->attr.dec = attr->dec;
                        if (!IS_BS_OR_NBS(attr->type)
                                || oitem->attr.collation == MDB_COL_NONE)
                        {
                            oitem->attr.collation = attr->collation;
                        }
                        oitem->attr.Htable = attr->Htable;
                        oitem->attr.Hattr = attr->Hattr;

                        oitem->s_ref = j;
                        break;
                    }
                }

                if (IS_BYTE(oitem->attr.type))
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_RESTRICT_ORDERBY_CLAUSE, 0);
                    return RET_ERROR;
                }

                if (j < qdesc->selection.len)
                {
                    continue;   // main loop로 복귀
                }
            }
            if (ambiguity == 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDCOLUMN, 0);
                return RET_ERROR;
            }
        }

        if (oitem->s_ref < 0)
        {
            for (j = 0; j < qdesc->selection.len; j++)
            {
                if (qdesc->selection.list[j].expr.len > 1)
                {
                    continue;
                }

                elem = qdesc->selection.list[j].expr.list[0];
                if (elem->elemtype == SPT_VALUE &&
                        elem->u.value.valueclass == SVC_VARIABLE)
                {
                    if (!mdb_strcmp(oitem->attr.attrname,
                                    elem->u.value.attrdesc.attrname) &&
                            !mdb_strcmp(oitem->attr.table.tablename,
                                    elem->u.value.attrdesc.table.tablename))
                    {
                        oitem->s_ref = j;
                        break;
                    }
                }
            }
        }

        if (oitem->s_ref < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDCOLUMN, 0);
            return RET_ERROR;
        }

        if (IS_BYTE(oitem->attr.type))
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_RESTRICT_ORDERBY_CLAUSE, 0);
            return RET_ERROR;
        }
    }

    return RET_SUCCESS;
}

static int check_subselect(int handle, T_SELECT * tsptr);

static int
check_subselect_set(int handle, T_SELECT * select)
{
    T_QUERYDESC *qdesc;
    T_POSTFIXELEM *elem;
    int i;
    T_QUERYRESULT *qresult = NULL;
    int j;

    qdesc = &select->planquerydesc.querydesc;

    if (select->kindofwTable & iTABLE_SUBSELECT)
    {
        T_LIST_SELECTION *selection;

        elem = qdesc->setlist.list[0];
        selection = &elem->u.subq->planquerydesc.querydesc.selection;
        j = (qdesc->selection.len > selection->len) ?
                selection->len : qdesc->selection.len;
        for (i = 0; i < j; i++)
        {
            sc_strncpy(selection->list[i].res_name,
                    qdesc->selection.list[i].res_name, FIELD_NAME_LENG);
        }
    }

    for (i = 0; i < qdesc->setlist.len; i++)
    {
        elem = qdesc->setlist.list[i];

        if (elem->elemtype == SPT_SUBQUERY)
        {
            if (check_subselect(handle, elem->u.subq) != RET_SUCCESS)
            {
                return RET_ERROR;
            }

            if (i == 0)
            {
                qresult = &elem->u.subq->queryresult;
            }
            else
            {
                if (qresult->len != elem->u.subq->queryresult.len)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_NOTMATCHCOLUMNCOUNT, 0);
                    return RET_ERROR;
                }

                for (j = 0; j < qresult->len; j++)
                {
                    if (qresult->list[j].value.attrdesc.len <
                            elem->u.subq->queryresult.list[j].value.attrdesc.
                            len)
                    {
                        qresult->list[j].value.attrdesc.len =
                                elem->u.subq->queryresult.list[j].value.
                                attrdesc.len;
                        qresult->list[j].len =
                                qresult->list[j].value.attrdesc.len;
                    }
                }
            }
        }
    }

    for (i = 0; i < qresult->len; i++)
    {
        if (qresult->list[i].value.valuetype == DT_NULL_TYPE)
        {
            qresult->list[i].value.valuetype = DT_CHAR;
            qresult->list[i].value.attrdesc.len = 1;
            qresult->list[i].value.attrdesc.collation = MDB_COL_DEFAULT_CHAR;
        }
    }

    if (qdesc->orderby.len > 0)
    {
        if ((check_subselect_set_orderby(handle, select)) != RET_SUCCESS)
        {
            return RET_ERROR;
        }
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_select

/*! \breif  SELECT 질의문의 문법 체크하는 부분, 문법도 체크하면서
  *            결과에 대한 정보도 같이 만든다.

 ************************************
 * \param handle(in) :
 * \param stmt(in/out) :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  not only check semantics, but also set result informations
 *
 *****************************************************************************/
static int
check_select(int handle, T_STATEMENT * stmt)
{
    stmt->u._select.main_parsing_chunk = stmt->parsing_memory;
    return check_subselect(handle, &stmt->u._select);
}

/*****************************************************************************/
//! check_subselect

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in) :
 * \param tsptr(in/out) :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - SUPER QUERY의 경우, TABLE INFO를 SET하지 않고 들어오는 경우가 있다.
 *      dbi_Schema_GetTableInfo를 이용해서 값을 가져올것
 *  - temp table인 경우.. table 정보가 없으므로. Error 처리를 하면 안된다
 *****************************************************************************/
static int
check_subselect(int handle, T_SELECT * tsptr)
{
    T_QUERYDESC *qdesc = &tsptr->planquerydesc.querydesc;
    T_JOINTABLEDESC *join;
    int ret = RET_SUCCESS;
    int i;

    tsptr->handle = handle;

    for (i = 0; i < qdesc->fromlist.len; i++)
    {
        join = qdesc->fromlist.list + i;

        if (join->table.correlated_tbl == NULL && join->fieldinfo == 0x00)
        {
            dbi_Schema_GetTableFieldsInfo(handle,
                    join->table.tablename, &(join->tableinfo),
                    &(join->fieldinfo));
        }
    }

    if (qdesc->fromlist.len > 1 &&
            (qdesc->limit.start_at != NULL_OID ||
                    qdesc->limit.startat_param_idx > -1))
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_LIMITRID_SINGLETABLE, 0);
        return DB_E_LIMITRID_SINGLETABLE;
    }

    if (qdesc->limit.start_at != NULL_OID && tsptr->msync_flag == DELETE_SLOT)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTALLOW_MSYNC_RID, 0);
        return RET_ERROR;
    }

    if (tsptr->first_sub &&
            check_subselect(handle, tsptr->first_sub) != RET_SUCCESS)
        return RET_ERROR;

    if (tsptr->sibling_query &&
            check_subselect(handle, tsptr->sibling_query) != RET_SUCCESS)
        return RET_ERROR;

    if (qdesc->setlist.len > 0)
    {
        ret = check_subselect_set(handle, tsptr);
        if (ret != RET_SUCCESS)
        {
            goto RETURN_LABEL;
        }

        return RET_SUCCESS;
    }

    ret = check_subselect_from(handle, tsptr);
    if (ret != RET_SUCCESS)
        goto RETURN_LABEL;

    ret = check_selection(handle, tsptr);
    if (ret != RET_SUCCESS)
        goto RETURN_LABEL;

    ret = check_subselect_join(handle, tsptr);
    if (ret != RET_SUCCESS)
        goto RETURN_LABEL;

    ret = check_subselect_condition(handle, tsptr);
    if (ret != RET_SUCCESS)
        goto RETURN_LABEL;

    if (qdesc->groupby.len > 0)
    {
        ret = check_subselect_groupby(handle, tsptr);
        if (ret != RET_SUCCESS)
            goto RETURN_LABEL;
    }
    if (qdesc->groupby.having.len > 0)
    {
        ret = check_subselect_having(handle, tsptr);
        if (ret != RET_SUCCESS)
            goto RETURN_LABEL;
    }
    if (qdesc->orderby.len > 0)
    {
        ret = check_subselect_orderby(handle, tsptr);
        if (ret != RET_SUCCESS)
            goto RETURN_LABEL;
    }

    ret = set_resultordinal(tsptr);
    if (ret != RET_SUCCESS)
        goto RETURN_LABEL;

    ret = set_result_info(tsptr);
    if (ret != RET_SUCCESS)
        goto RETURN_LABEL;

  RETURN_LABEL:

    return ret;
}

/*****************************************************************************/
//! set_defaultvalue

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param columns() :
 * \param values() :
 * \param fields() :
 * \param fieldnum() :
 * \param chunk_memory(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *  - CHAR/NCHAR/BYTE의 경우.. LENGTH를 설정한다
 *  - default를 나타내는 구조체에도 length를 나타내는 멤버변수가 필요하다.
 *      그래야지만.. byte/varbyte도 지원 가능하다..
 *
 *****************************************************************************/
static int
set_defaultvalue(T_LIST_COLDESC * columns,
        T_LIST_VALUE * values, SYSFIELD_T * fields, int fieldnum,
        T_PARSING_MEMORY * chunk_memory)
{
    T_COLDESC *col_list;
    T_EXPRESSIONDESC *expr;
    T_POSTFIXELEM *elem;
    int i, j;
    int ret;

    for (i = 0; i < fieldnum; i++)
    {
        for (j = 0; j < columns->len; j++)
        {
            if (!mdb_strcmp(fields[i].fieldName, columns->list[j].name))
                break;
        }
        if (j < columns->len)
            continue;

        if (columns->max <= columns->len)
        {   // size가 작다면 reallocation..
            col_list = PMEM_ALLOC(sizeof(T_COLDESC) *
                    (columns->len + MAX_NEXT_LIST_NUM));
            if (!col_list)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return SQL_E_OUTOFMEMORY;
            }
            expr = PMEM_ALLOC(sizeof(T_EXPRESSIONDESC) *
                    (columns->len + MAX_NEXT_LIST_NUM));
            if (!expr)
            {
                PMEM_FREENUL(col_list);
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return SQL_E_OUTOFMEMORY;
            }
            sc_memcpy(col_list, columns->list,
                    sizeof(T_COLDESC) * columns->len);
            sc_memcpy(expr, values->list,
                    sizeof(T_EXPRESSIONDESC) * columns->len);

            sql_mem_freenul(chunk_memory, columns->list);
            sql_mem_freenul(chunk_memory, values->list);

            columns->list = col_list;
            columns->max = columns->len + MAX_NEXT_LIST_NUM;
            values->list = expr;
            values->max = columns->len + MAX_NEXT_LIST_NUM;
        }

        // default value로 채우자...
        col_list = &columns->list[columns->len];
        expr = &values->list[columns->len];

        col_list->name = PMEM_STRDUP(fields[i].fieldName);
        if (!col_list->name)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return RET_ERROR;
        }
        col_list->fieldinfo = &fields[i];

        if (!DB_DEFAULT_ISNULL(fields[i].defaultValue))
        {
            expr->list = sql_mem_get_free_postfixelem_list(NULL, 1);
            if (expr->list == NULL)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return SQL_E_OUTOFMEMORY;
            }

            expr->list[0] = sql_mem_get_free_postfixelem(NULL);
            if (expr->list[0] == NULL)
            {
                sql_mem_set_free_postfixelem_list(NULL, expr->list);
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return SQL_E_OUTOFMEMORY;
            }
            sc_memset(expr->list[0], 0x00, sizeof(T_POSTFIXELEM));

            expr->len = 1;
            expr->max = 1;

            elem = expr->list[0];
            elem->elemtype = SPT_VALUE;
            elem->u.value.valueclass = SVC_CONSTANT;
            elem->u.value.valuetype = fields[i].type;

            ret = get_defaultvalue(&elem->u.value.attrdesc, &fields[i]);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                return ret;
            }
            elem->u.value.u = elem->u.value.attrdesc.defvalue.u;
            if (IS_BS_OR_NBS(elem->u.value.valuetype))
                elem->u.value.value_len =
                        strlen_func[fields[i].type] (elem->u.value.u);
            else if (IS_BYTE(elem->u.value.valuetype))
                elem->u.value.value_len =
                        elem->u.value.attrdesc.defvalue.value_len;

            if (elem->u.value.attrdesc.defvalue.defined &&
                    elem->u.value.attrdesc.defvalue.not_null &&
                    (IS_ALLOCATED_TYPE(fields[i].type)))
                elem->u.value.call_free = 1;
            sc_memset(&elem->u.value.attrdesc, 0, sizeof(T_ATTRDESC));
        }

        columns->len++;
        values->len++;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_numeric_bound

/*! \breif 주어진 값이 숫자형 자료의 표현 범위내에 있는지 검사한다.
 ************************************
 * \param type      :
 * \param length    :
 * \param value     :
 ************************************
 * \return RET_TRUE or RET_FALSE
 ************************************
 * \note
 *****************************************************************************/
int
check_numeric_bound(T_VALUEDESC * value, DataType type, int length)
{
    bigint val4int = 0;
    double val4flt = 0.0;
    double val4str;
    int ret;

    if (value->is_null)
        return RET_TRUE;

    /* 동일한 type 인 경우 check 필요 없음 */
    if (value->valuetype == type)
    {
        return RET_TRUE;
    }

    switch (value->valuetype)
    {
    case DT_TINYINT:
    case DT_SMALLINT:
    case DT_INTEGER:
    case DT_BIGINT:
        GET_INT_VALUE(val4int, bigint, value);
        CHECK_OVERFLOW(val4int, type, length, ret);
        if (ret == RET_TRUE)
            return RET_FALSE;
        break;
    case DT_FLOAT:
    case DT_DOUBLE:
        GET_FLOAT_VALUE(val4flt, double, value);

        CHECK_OVERFLOW(val4flt, type, length, ret);
        if (ret == RET_TRUE)
            return RET_FALSE;
        CHECK_UNDERFLOW(val4flt, type, length, ret);
        if (ret == RET_TRUE)
            return RET_FALSE;
        break;
    case DT_CHAR:
    case DT_VARCHAR:
        val4str = sc_atof(value->u.ptr);
        CHECK_OVERFLOW(val4str, type, length, ret);
        if (ret == RET_TRUE)
            return RET_FALSE;
        CHECK_UNDERFLOW(val4str, type, length, ret);
        if (ret == RET_TRUE)
            return RET_FALSE;
        break;
    default:
        break;
    }
    return RET_TRUE;
}

/*****************************************************************************/
//! check_insert_values

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)        :
 * \param insert(in/out)    :
 * \param chunk_memory(in)  :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_PARSING_MEMORY가 함수의 인자로 넘어옴
 *  - DATETIME과 TIMESTAMP의 범위를 CHECK한다
 *
 *****************************************************************************/
static int
check_insert_values(int handle, T_INSERT * insert,
        T_PARSING_MEMORY * chunk_memory)
{
    T_LIST_COLDESC *col_list;
    T_COLDESC *column;
    T_LIST_VALUE *val_list;
    SYSFIELD_T *fieldinfo;
    int fieldnum;

    T_EXPRESSIONDESC *expr;
    T_VALUEDESC chk_value;
    int ins_msize;
    int i, j;

    val_list = &insert->u.values;
    col_list = &insert->columns;

    fieldinfo = insert->fieldinfo;
    fieldnum = insert->tableinfo.numFields;

    if (insert->is_upsert_msync)
    {
        if ((insert->tableinfo.flag & _CONT_MSYNC) == 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_INVALID_MSYNCTABLE, 0);
            return RET_ERROR;
        }
    }

    if (col_list->len == 0)
    {
        if (fieldnum != val_list->len)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_NOTMATCHCOLUMNCOUNT, 0);
            return RET_ERROR;
        }
        for (i = 0; i < fieldnum; i++)
        {
            expr = &val_list->list[i];

            if (expr->len == 1 && expr->list[0]->elemtype == SPT_VALUE
                    && expr->list[0]->u.value.is_null
                    && (expr->list[0]->u.value.param_idx == 0))
            {   /* NULL value insertion */
                if (!(fieldinfo[i].flag & AUTO_BIT) &&
                        !(fieldinfo[i].flag & NULL_BIT))
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_NOTALLOWEDNULLVALUE, 0);
                    return RET_ERROR;
                }
            }
            else
            {
                if (calc_expression(expr, &chk_value, MDB_TRUE) < 0)
                    return RET_ERROR;

/* if not numeric type, avoid check */
                if (fieldinfo[i].type < DT_CHAR &&
                        check_numeric_bound(&chk_value,
                                fieldinfo[i].type, fieldinfo[i].length)
                        == RET_FALSE)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_TOOLARGEVALUE, 0);
                    sql_value_ptrFree(&chk_value);
                    return RET_ERROR;
                }
                sql_value_ptrFree(&chk_value);
            }
            if (expr->len == 1)
            {
                int ret;
                T_VALUEDESC temp_value;

                temp_value.valueclass = SVC_VARIABLE;
                temp_value.valuetype = fieldinfo[i].type;
                temp_value.attrdesc.type = fieldinfo[i].type;
                temp_value.attrdesc.len = fieldinfo[i].length;

                ret = type_convert(&(expr->list[0]->u.value), &temp_value);
                if (ret < 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    return RET_ERROR;
                }
            }
        }
    }
    else
    {
        if (fieldnum > col_list->len)
        {
            ins_msize = fieldnum * sizeof(int);
            col_list->ins_cols = (int *) PMEM_ALLOC(ins_msize);
        }
        for (i = 0; i < col_list->len; i++)
        {

            column = &col_list->list[i];
            expr = &val_list->list[i];

            for (j = i + 1; j < col_list->len; j++)
            {
                if (!mdb_strcmp(column->name, col_list->list[j].name))
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_DUPLICATECOLUMNNAME, 0);
                    return RET_ERROR;
                }
            }

            for (j = 0; j < fieldnum; j++)
            {
                if (!mdb_strcmp(column->name, fieldinfo[j].fieldName))
                {
                    /* if hidden columns exist, set flag of insert column */
                    column->fieldinfo = &fieldinfo[j];
                    if (col_list->ins_cols)
                        col_list->ins_cols[fieldinfo[j].position] = 1;
                    break;
                }
            }
            if (j == fieldnum)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDCOLUMN, 0);
                return RET_ERROR;
            }
            if (expr->len == 1 && expr->list[0]->elemtype == SPT_VALUE
                    && expr->list[0]->u.value.is_null
                    && (expr->list[0]->u.value.param_idx == 0))
            {   /* NULL value insert */
                if (!(fieldinfo[j].flag & AUTO_BIT) &&
                        !(fieldinfo[j].flag & NULL_BIT))
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_NOTALLOWEDNULLVALUE, 0);
                    return RET_ERROR;
                }
            }
            else
            {
                if (IS_BS_OR_NBS(fieldinfo[j].type))
                    ;
                else if (IS_BYTE(fieldinfo[j].type))
                    ;
                else
                {
                    if (calc_expression(expr, &chk_value, MDB_TRUE) < 0)
                    {
                        return RET_ERROR;
                    }
/* if not numeric type, avoid check */
                    if (fieldinfo[j].type < DT_CHAR &&
                            check_numeric_bound(&chk_value, fieldinfo[j].type,
                                    fieldinfo[j].length) == RET_FALSE)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_TOOLARGEVALUE, 0);
                        sql_value_ptrFree(&chk_value);

                        return RET_ERROR;
                    }

                    sql_value_ptrFree(&chk_value);
                }
            }
            if (expr->len == 1)
            {
                int ret;
                T_VALUEDESC temp_value;

                temp_value.valueclass = SVC_VARIABLE;
                temp_value.valuetype = fieldinfo[j].type;
                temp_value.attrdesc.type = fieldinfo[j].type;
                temp_value.attrdesc.len = fieldinfo[j].length;

                ret = type_convert(&(expr->list[0]->u.value), &temp_value);
                if (ret < 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    return RET_ERROR;
                }
            }
        }   /* for (i = 0; i < col_list->len; i++)  */

        if (set_defaultvalue(col_list, val_list,
                        fieldinfo, fieldnum, chunk_memory) < 0)
        {
            return RET_ERROR;
        }

        /* if hidden columns exist, check not null flags */
        if (col_list->ins_cols)
        {
            for (j = 0; j < fieldnum; j++)
            {
                if (!(fieldinfo[j].flag & AUTO_BIT) &&
                        !col_list->ins_cols[fieldinfo[j].position] &&
                        !(fieldinfo[j].flag & NULL_BIT) &&
                        DB_DEFAULT_ISNULL(fieldinfo[j].defaultValue))
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_NOTALLOWEDNULLVALUE, 0);
                    return RET_ERROR;
                }
            }
        }
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_insert_query

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)    :
 * \param stmt(in/out)      :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *  - INSERT 부분에 대한 질의가 SELECT 되는 부분이 있다..
 *      STATEMENT 전체가 바뀐다면, 새로 SQL_PARSING_MEMORY를 할당해야 하나
 *      일부분만 바꾸므로 기존에 사용하면 stmt의 memory를 사용하도록 수정
 *
 *****************************************************************************/
static int
check_insert_query(int handle, T_STATEMENT * stmt)
{
    T_LIST_COLDESC *col_list;
    T_COLDESC *column;
    T_STATEMENT *tmp_stmt = NULL;
    T_QUERYRESULT *qresult;
    SYSFIELD_T *fieldinfo;
    int fieldnum;

    int *ins_cols = NULL, ins_msize;
    int index = THREAD_HANDLE;
    int ret, i, j;

    tmp_stmt = (T_STATEMENT *) PMEM_ALLOC(sizeof(T_STATEMENT));
    if (tmp_stmt == NULL)
        return RET_ERROR;

    tmp_stmt->sqltype = ST_SELECT;

    // main stmt's parsing memory를 넘겨주어야..
    // sub stmt가 SQL_cleanup_memory를 호출할때..
    // main stmt's parsing memory를 free하지 않음
    tmp_stmt->parsing_memory = stmt->parsing_memory;
    sc_memcpy(&tmp_stmt->u._select, &(stmt->u._insert.u.query),
            sizeof(T_SELECT));


    ret = SQL_prepare(&handle, tmp_stmt, gClients[index].flags);
    sc_memcpy(&(stmt->u._insert.u.query), &tmp_stmt->u._select,
            sizeof(T_SELECT));
    if (ret == RET_ERROR)
    {
        PMEM_FREE(tmp_stmt);
        return RET_ERROR;
    }

    if (tmp_stmt->u._select.planquerydesc.querydesc.setlist.len > 0)
    {
        T_SELECT *tmp_select;

        tmp_select =
                tmp_stmt->u._select.planquerydesc.querydesc.setlist.list[0]->u.
                subq;
        qresult = &tmp_select->queryresult;
    }
    else
        qresult = &tmp_stmt->u._select.queryresult;
    col_list = &(stmt->u._insert.columns);

    fieldnum = stmt->u._insert.tableinfo.numFields;
    fieldinfo = stmt->u._insert.fieldinfo;

    if (col_list->len == 0)
    {
        if (fieldnum != qresult->len)
        {
            PMEM_FREE(tmp_stmt);
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_NOTMATCHCOLUMNCOUNT, 0);
            return RET_ERROR;
        }
    }
    else
    {
        if (fieldnum > col_list->len)
        {   /* check if hidden columns exist */
            ins_msize = fieldnum * sizeof(int);
            ins_cols = (int *) PMEM_ALLOC(ins_msize);
        }
        for (i = 0; i < col_list->len; i++)
        {
            column = &col_list->list[i];

            for (j = i + 1; j < col_list->len; j++)
            {
                if (!mdb_strcmp(column->name, col_list->list[j].name))
                {
                    PMEM_FREE(tmp_stmt);
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_DUPLICATEINDEXNAME, 0);
                    PMEM_FREENUL(ins_cols);
                    return RET_ERROR;
                }
            }

            for (j = 0; j < fieldnum; j++)
            {
                if (!mdb_strcmp(column->name, fieldinfo[j].fieldName))
                {
                    column->fieldinfo = &fieldinfo[j];
                    if (ins_cols)
                        ins_cols[j] = 1;
                    break;
                }
            }
            if (j == fieldnum)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDCOLUMN, 0);
                PMEM_FREE(tmp_stmt);
                PMEM_FREENUL(ins_cols);
                return RET_ERROR;
            }

        }
        /* check not null flags */
        if (ins_cols)
        {
            for (j = 0; j < fieldnum; j++)
            {
                if (!(fieldinfo[j].flag & AUTO_BIT) &&
                        !ins_cols[j] && !(fieldinfo[j].flag & NULL_BIT) &&
                        DB_DEFAULT_ISNULL(fieldinfo[j].defaultValue))
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_NOTALLOWEDNULLVALUE, 0);
                    PMEM_FREE(tmp_stmt);
                    PMEM_FREENUL(ins_cols);
                    return RET_ERROR;
                }
            }
        }
    }

    PMEM_FREENUL(ins_cols);
    PMEM_FREENUL(tmp_stmt);
    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_insert

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)    :
 * \param stmt(in/out)  :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *
 *****************************************************************************/
static int
check_insert(int handle, T_STATEMENT * stmt)
{
    T_TABLEDESC *table;

    table = &(stmt->u._insert.table);

    if (check_systable(table->tablename) >= 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTALLOWED, 0);
        return RET_ERROR;
    }

    {
        int rtnVal = check_valid_basetable(handle, table->tablename);

        if (rtnVal != 1)
        {
            if (rtnVal == 0)    // case: viewtable.
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALID_OPERATION_VIEW, 0);
            }
            else
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDBASETABLE, 0);
            }

            return RET_ERROR;
        }
    }

    if (dbi_Schema_GetTableFieldsInfo(handle, table->tablename,
                    &(stmt->u._insert.tableinfo),
                    &(stmt->u._insert.fieldinfo)) < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDBASETABLE, 0);
        return RET_ERROR;
    }

    if (stmt->u._insert.type == SIT_VALUES)
        return check_insert_values(handle, &stmt->u._insert,
                stmt->parsing_memory);

    else    /* stmt->u._insert.type == SIT_QUERY */
        return check_insert_query(handle, stmt);
}

/*****************************************************************************/
//! check_delete

/*! \breif DELETE SQL문장을 검사하고, 필드 정보를 할당한다
 ************************************
 * \param handle(in)    : DB Handle
 * \param stmt(in/out)  : Statement
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static int
check_delete(int handle, T_STATEMENT * stmt)
{
    T_TABLEDESC *table;
    int ret = RET_SUCCESS;
    T_JOINTABLEDESC *join;

    join = stmt->u._delete.planquerydesc.querydesc.fromlist.list;
    table = &(join->table);
    if (check_systable(table->tablename) >= 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTALLOWED, 0);
        return RET_ERROR;
    }

    {
        int rtnVal = check_valid_basetable(handle, table->tablename);

        if (rtnVal != 1)
        {
            if (rtnVal == 0)    // case: viewtable.
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALID_OPERATION_VIEW, 0);
            }
            else
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDBASETABLE, 0);
            }

            return RET_ERROR;
        }
    }

    if (dbi_Schema_GetTableFieldsInfo(handle, table->tablename,
                    &(join->tableinfo), &(join->fieldinfo)) < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDBASETABLE, 0);
        ret = RET_ERROR;
        goto RETURN_LABEL;
    }

    ret = check_condition(handle, &stmt->u._delete.planquerydesc.querydesc,
            NULL);
    if (ret != RET_SUCCESS)
    {
        ret = RET_ERROR;
    }

  RETURN_LABEL:
    return ret;
}

/*****************************************************************************/
//! check_update

/*! \breif  UPDATE 문장을 검사하고, 필드 정보를 설정
 ************************************
 * \param handle(in)    : DB Handle
 * \param stmt(in/out)  : Statement
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 자료구조 변경
 *  - BYTE/VARBYTE TYPE은 WHERE CLAUSE에서 사용할 수 없음
 *
 *****************************************************************************/
static int
check_update(int handle, T_STATEMENT * stmt)
{
    SYSFIELD_T *fieldinfo = NULL;
    T_TABLEDESC *table;
    T_LIST_COLDESC *col_list;
    T_LIST_VALUE *val_list;
    T_COLDESC *column;
    T_ATTRDESC *attr;
    T_EXPRESSIONDESC *expr;
    T_POSTFIXELEM *elem;

    int i, j = 0, k, fieldnum;
    int ret = RET_SUCCESS;
    T_JOINTABLEDESC *join;

    join = stmt->u._update.planquerydesc.querydesc.fromlist.list;
    table = &(join->table);

    if (stmt->u._update.rid != NULL_OID)
    {
        if (table->tablename == NULL)
            table->tablename = sql_malloc(MAX_TABLE_NAME_LEN + 1, 0);

        if (table->tablename == NULL)
            return RET_ERROR;

        if (dbi_Rid_Tablename(handle, stmt->u._update.rid,
                        table->tablename) < 0)
            return RET_ERROR;
    }

    if (check_systable(table->tablename) >= 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTALLOWED, 0);
        return RET_ERROR;
    }

    {
        int rtnVal = check_valid_basetable(handle, table->tablename);

        if (rtnVal != 1)
        {
            if (rtnVal == 0)    // case: viewtable.
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALID_OPERATION_VIEW, 0);
            }
            else
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDBASETABLE, 0);
            }

            return RET_ERROR;
        }
    }

    fieldnum = dbi_Schema_GetTableFieldsInfo(handle, table->tablename,
            &(join->tableinfo), &(join->fieldinfo));
    if (fieldnum < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, fieldnum, 0);
        return RET_ERROR;
    }

    fieldnum = join->tableinfo.numFields;
    fieldinfo = join->fieldinfo;

    stmt->u._update.has_variable = 0;
    col_list = &stmt->u._update.columns;
    val_list = &stmt->u._update.values;
    for (i = 0; i < col_list->len; i++)
    {
        column = &col_list->list[i];
        expr = &val_list->list[i];
        for (j = 0; j < fieldnum; j++)
        {
            if (!mdb_strcmp(column->name, fieldinfo[j].fieldName))
                break;
        }
        if (j == fieldnum)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDCOLUMN, 0);
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }
        column->fieldinfo = &fieldinfo[j];

        if (expr->len == 1 && expr->list[0]->elemtype == SPT_VALUE
                && expr->list[0]->u.value.is_null
                && (expr->list[0]->u.value.param_idx == 0))
        {
            if (!(fieldinfo[j].flag & NULL_BIT) &&
                    DB_DEFAULT_ISNULL(fieldinfo[j].defaultValue))
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_NOTALLOWEDNULLVALUE, 0);
                ret = RET_ERROR;
                goto RETURN_LABEL;
            }
        }
        for (j = 0; j < expr->len; j++)
        {
            elem = expr->list[j];
            if (elem->elemtype == SPT_OPERATOR)
            {
                /* null 처리를 위해 prepare시 in 처리 type convert skip */
                if (elem->u.op.type == SOT_LIKE
                        || elem->u.op.type == SOT_ILIKE
                        || elem->u.op.type == SOT_IN
                        || elem->u.op.type == SOT_NIN)
                {
                    continue;
                }

                if (elem->u.op.argcnt == 2)
                {
                    ret = type_convert(&(expr->list[j - 1]->u.value),
                            &(expr->list[j - 2]->u.value));
                    if (ret < 0)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                        ret = RET_ERROR;
                        goto RETURN_LABEL;
                    }
                }

                if (elem->u.op.type == SOT_PLUS ||
                        elem->u.op.type == SOT_MINUS ||
                        elem->u.op.type == SOT_TIMES ||
                        elem->u.op.type == SOT_DIVIDE ||
                        elem->u.op.type == SOT_EXPONENT ||
                        elem->u.op.type == SOT_MODULA ||
                        elem->u.op.type == SOT_MERGE)
                {
                    ret = preconvert_expression(elem->u.op.argcnt, expr, &j);

                    if (ret < 0)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                        ret = RET_ERROR;
                        goto RETURN_LABEL;
                    }
                }
                continue;
            }

            if (elem->elemtype == SPT_FUNCTION)
            {
                ret = preconvert_expression(elem->u.func.argcnt, expr, &j);
                if (ret < 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    ret = RET_ERROR;
                    goto RETURN_LABEL;
                }
                continue;
            }
            if (elem->elemtype == SPT_AGGREGATION)
                continue;
            if (elem->elemtype == SPT_SUBQUERY)
                continue;
            if (elem->elemtype == SPT_SUBQ_OP)
                continue;

            if (elem->u.value.valueclass == SVC_VARIABLE)
            {
                stmt->u._update.has_variable = 1;
                attr = &(elem->u.value.attrdesc);
                if (attr->table.tablename == NULL)
                {
                    for (k = 0; k < fieldnum; k++)
                        if (!mdb_strcmp(attr->attrname,
                                        fieldinfo[k].fieldName))
                        {
                            attr->table.tablename =
                                    PMEM_STRDUP(table->tablename);
                            if (!attr->table.tablename)
                            {
                                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                        SQL_E_OUTOFMEMORY, 0);
                                ret = RET_ERROR;
                                goto RETURN_LABEL;
                            }
                            if (table->aliasname)
                            {
                                attr->table.aliasname =
                                        PMEM_STRDUP(table->aliasname);
                                if (!attr->table.aliasname)
                                {
                                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                            SQL_E_OUTOFMEMORY, 0);
                                    ret = RET_ERROR;
                                    goto RETURN_LABEL;
                                }
                            }
                            else
                                attr->table.aliasname = NULL;
                            break;
                        }
                    if (k == fieldnum)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_INVALIDCOLUMN, 0);
                        ret = RET_ERROR;
                        goto RETURN_LABEL;
                    }
                }
                else
                {       // specify table name
                    k = 0;
                    if (!mdb_strcmp(attr->table.tablename, table->tablename))
                    {
                        for (k = 0; k < fieldnum; k++)
                            if (!mdb_strcmp(attr->attrname,
                                            fieldinfo[k].fieldName))
                                break;
                        if (k == fieldnum)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_INVALIDCOLUMN, 0);
                            ret = RET_ERROR;
                            goto RETURN_LABEL;
                        }
                    }
                    else if (table->aliasname != NULL)
                    {
                        if (!mdb_strcmp(attr->table.tablename,
                                        table->aliasname))
                        {
                            attr->table.aliasname = attr->table.tablename;
                            attr->table.tablename =
                                    PMEM_STRDUP(table->tablename);
                            if (!attr->table.tablename)
                            {
                                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                        SQL_E_OUTOFMEMORY, 0);
                                ret = RET_ERROR;
                                goto RETURN_LABEL;
                            }
                            for (k = 0; k < fieldnum; k++)
                                if (!mdb_strcmp(attr->attrname,
                                                fieldinfo[k].fieldName))
                                    break;
                            if (k == fieldnum)
                            {
                                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                        SQL_E_INVALIDCOLUMN, 0);
                                ret = RET_ERROR;
                                goto RETURN_LABEL;
                            }
                        }
                    }
                }

                if (set_attrinfobyname(handle,
                                attr, &fieldinfo[k]) == RET_ERROR)
                {
                    ret = RET_ERROR;
                    goto RETURN_LABEL;
                }
                elem->u.value.valuetype = attr->type;
            }
        }
    }       // for loop

    for (i = 0; i < col_list->len; i++)
    {
        column = &col_list->list[i];
        expr = &val_list->list[i];
        for (j = 0; j < fieldnum; j++)
        {
            if (!mdb_strcmp(column->name, fieldinfo[j].fieldName))
            {
                if (expr->len > 0 && expr->list[0]->u.value.is_null != 1
                        && expr->list[0]->u.value.value_len > 0)
                {
                    if ((IS_BS(fieldinfo[j].type) &&
                                    IS_BS(expr->list[0]->u.value.valuetype)) ||
                            (IS_NBS(fieldinfo[j].type) &&
                                    IS_NBS(expr->list[0]->u.value.valuetype)))
                    {
                        if (strlen_func[expr->list[0]->u.value.
                                        valuetype] (expr->list[0]->u.value.u.
                                        ptr) > fieldinfo[j].length)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_TOOLARGEVALUE, 0);
                            ret = RET_ERROR;
                            goto RETURN_LABEL;
                        }
                    }
                    else if (IS_BYTE(fieldinfo[j].type) &&
                            IS_BYTE(expr->list[0]->u.value.valuetype))
                    {
                        if (expr->list[0]->u.value.value_len >
                                fieldinfo[j].length)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_TOOLARGEVALUE, 0);
                            ret = RET_ERROR;
                            goto RETURN_LABEL;
                        }
                    }
                }
                break;
            }
        }
    }

    ret = check_condition(handle, &stmt->u._update.planquerydesc.querydesc,
            NULL);
    if (ret != RET_SUCCESS)
    {
        ret = RET_ERROR;
    }

  RETURN_LABEL:
    return ret;
}

/*****************************************************************************/
//! check_create_view

/*! \breif CREATE VIEW 문에 대한 의미 검사를 한다.
 ************************************
 * \param handle     :
 * \param view       :
 * \param chunk(in) :
 ************************************
 * \return RET_SUCCESS or RET_ERROR
 ************************************
 * \note
 *  - field 정보를 설정하는 부분을 함수로 변경한다
 *
 *****************************************************************************/
static int
check_create_view(int handle, T_CREATEVIEW * view)
{
    T_ATTRDESC *attr;
    T_RESULTCOLUMN *rescol;
    int i, j;
    T_QUERYRESULT *qresult;

    /* check view table existence */
    if (check_validtable(handle, view->name) == 1)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ALREADYEXISTTABLE, 0);
        return RET_ERROR;
    }
    /* check size of view definition string */
    if (sc_strlen(view->qstring) > MAX_SYSVIEW_DEF)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_TOOLONGVIEW, 0);
        return RET_ERROR;
    }
    /* check view definition */
    if (check_subselect(handle, &view->query) < 0)
    {
        return RET_ERROR;
    }
    if (view->query.planquerydesc.querydesc.setlist.len > 0)
        qresult =
                &view->query.planquerydesc.querydesc.setlist.list[0]->u.subq->
                queryresult;
    else
        qresult = &view->query.queryresult;
    /* check column match if view column is defined */
    if (view->columns.len == 0)
    {
        view->columns.len = qresult->len;
        view->columns.max = view->columns.len;
        view->columns.list = (T_ATTRDESC *)
                PMEM_ALLOC(sizeof(T_ATTRDESC) * view->columns.max);
        if (view->columns.list == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return RET_ERROR;
        }
    }
    else if (view->columns.len != qresult->len)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTMATCHCOLUMNCOUNT, 0);
        return RET_ERROR;
    }
    /* bind column schema information */
    for (i = 0; i < view->columns.len; i++)
    {
        attr = &view->columns.list[i];
        rescol = &qresult->list[i];

        /* bind column schema information */
        if (attr->attrname == NULL)
        {
            attr->attrname = sql_strdup(rescol->res_name, 0);
            if (!attr->attrname)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFMEMORY, 0);
                return RET_ERROR;
            }
        }
        attr->type = rescol->value.valuetype;
        attr->len = rescol->value.attrdesc.len;
        attr->dec = rescol->value.attrdesc.dec;
        attr->flag = NULL_BIT;
        attr->fixedlen = rescol->value.attrdesc.fixedlen;
        attr->collation = rescol->value.attrdesc.collation;
    }
    /* check if view column name is duplicated */
    for (i = 0; i < view->columns.len; i++)
    {
        attr = &view->columns.list[i];
        for (j = i + 1; j < view->columns.len; j++)
        {
            if (!mdb_strcmp(attr->attrname, view->columns.list[j].attrname))
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_DUPLICATECOLUMNNAME, 0);
                return RET_ERROR;
            }
        }
    }
    return RET_SUCCESS;
}

int
check_strtype_overflow(DataType type, int length, int fixedlen)
{
    if (IS_ALLOCATED_TYPE(type))
    {
        int _max_size_char = MAX_OF_CHAR / SizeOfType[type];
        int _max_size_varchar = MAX_OF_VARCHAR / SizeOfType[type];

        if (IS_BYTE(type))
            _max_size_char -= 4;

        if (fixedlen <= FIXED_VARIABLE ||
                server->shparams.default_fixed_part == FIXED_VARIABLE)
        {
            if (length > _max_size_char)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTYPE, 0);
                return SQL_E_INVALIDTYPE;
            }

        }
        else
        {
            if ((length > _max_size_varchar) || (fixedlen > MAX_OF_CHAR))
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTYPE, 0);
                return SQL_E_INVALIDTYPE;
            }
        }
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_create

/*! \breif  CREATE 문장인 경우 SQL 문법을 CHECK하고, 테이블의 필드 정보를 설정
 ************************************
 * \param handle(in)    :
 * \param stmt(in/out)      :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *  - STATEMENT 전체가 바뀐다면, 새로 SQL_PARSING_MEMORY를 할당해야 하나
 *      일부분만 바꾸므로 기존에 사용하면 stmt의 memory를 사용하도록 수정
 *  - CREATE TABLE시 TABLE NAME의 최대 길이를 32 BYTES까지 제약한다
 *  - CREATE TABLE시 FIELDS의 최대 길이를 32 BYTES까지 제약한다
 *  - CREATE INDEX시 INDEX의 최대 길이를 32 BYTES까지 제약한다
 *  - A. INDEX 생성시 COLLATION TYPE을 설정하지 않은 경우(MDB_COL_NONE로 설정)
 *         TABLE의 FIELD에 설정된 COLLATION을 가지고 간다
 *      B. 중복된 index인지를 검사할 때.. 기존에는 이름만을 가지고 비교했지만,
 *      이름과 collation type 동시에 2개를 검사해야한다
 *
 *****************************************************************************/
static int
check_create(int handle, T_STATEMENT * stmt)
{
    T_CREATETABLE *table_elements;
    SYSFIELD_T *fieldinfo = NULL;
    SYSINDEX_T *indexinfo = NULL;
    SYSINDEXFIELD_T *indexfields;
    char *ptr;
    int fieldnum, indexnum;
    int i, j, k, ret = RET_SUCCESS;
    MDB_UINT8 *flag;
    T_TABLEDESC *table_info;
    int auto_cnt = 0;

    if (stmt->u._create.type == SCT_TABLE_ELEMENTS)
    {
        table_elements = &stmt->u._create.u.table_elements;
        if (sc_strlen(table_elements->table.tablename) >= REL_NAME_LENG)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_MAX_TABLE_NAME_LEN_EXEEDED, 0);
            return RET_ERROR;
        }
        for (i = 0; i < table_elements->col_list.len; i++)
        {
            if (!sql_collation_compatible_check(table_elements->col_list.
                            list[i].type,
                            table_elements->col_list.list[i].collation))
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_MISMATCH_COLLATION, 0);
                return RET_ERROR;
            }
            if (sc_strlen(table_elements->col_list.list[i].attrname)
                    >= FIELD_NAME_LENG)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_MAX_FIELD_NAME_LEN_EXEEDED, 0);
                return RET_ERROR;
            }
        }

        if (table_elements->table_type == 7 && table_elements->fields.len == 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    DB_E_NOTFOUND_PRIMARYKEY, 0);
            return RET_ERROR;
        }

        // primary key checking
        for (i = 0; i < table_elements->fields.len; i++)
        {

            for (j = 0; j < table_elements->col_list.len; j++)
                if (!mdb_strcmp(table_elements->fields.list[i],
                                table_elements->col_list.list[j].attrname))
                    break;

            if (j == table_elements->col_list.len)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDKEYCOLUMN, 0);
                return RET_ERROR;
            }
            else
            {
                flag = &table_elements->col_list.list[j].flag;
                if (*flag & NULL_BIT)
                {
                    if (*flag & EXPLICIT_NULL_BIT)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_NOTALLOWEDNULLKEY, 0);
                        return RET_ERROR;
                    }
                    else
                        *flag &= ~NULL_BIT;
                }

                if (*flag & NOLOGGING_BIT)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_NOLOGGING_COLUMN_INDEX, 0);
                    return RET_ERROR;
                }

                *flag |= PRI_KEY_BIT;
            }
        }

        /* check for max_records index */
        if (stmt->u._create.u.table_elements.table.max_records > 0)
        {
            if (stmt->u._create.u.table_elements.table.column_name[0] != '#')
            {
                for (i = 0; i < table_elements->col_list.len; i++)
                {
                    if (!mdb_strcmp(stmt->u._create.u.table_elements.table.
                                    column_name,
                                    table_elements->col_list.list[i].attrname))
                        break;
                }
                if (i == table_elements->col_list.len)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_INVALIDKEYCOLUMN, 0);
                    return RET_ERROR;
                }
            }
        }

        for (i = 0; i < table_elements->col_list.len; i++)
        {
            // default value checking
            if (!(table_elements->col_list.list[i].flag & NULL_BIT) &&
                    table_elements->col_list.list[i].defvalue.defined &&
                    !table_elements->col_list.list[i].defvalue.not_null)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDDEFAULTVALUE, 0);
                return RET_ERROR;
            }
            /* check if the column name is duplicated */
            for (j = i + 1; j < table_elements->col_list.len; j++)
            {
                if (!mdb_strcmp(table_elements->col_list.list[i].attrname,
                                table_elements->col_list.list[j].attrname))
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_DUPLICATECOLUMNNAME, 0);
                    return RET_ERROR;
                }
            }
            /* check the limitation for n size of char(varchar) */
            if (check_strtype_overflow(table_elements->col_list.list[i].type,
                            table_elements->col_list.list[i].len,
                            table_elements->col_list.list[i].fixedlen) < 0)
                return RET_ERROR;
            if (table_elements->col_list.list[i].flag & AUTO_BIT)
            {
                auto_cnt++;
                if (auto_cnt > 1 ||
                        !(table_elements->col_list.list[i].flag & PRI_KEY_BIT)
                        || table_elements->col_list.list[i].type != DT_INTEGER
                        || table_elements->fields.len > 1)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_DUPLICATECOLUMNNAME, 0);
                    return RET_ERROR;
                }
            }
        }
        if (check_validtable(handle, table_elements->table.tablename) == 1)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_ALREADYEXISTTABLE, 0);
            return RET_ERROR;
        }
    }
    else if (stmt->u._create.type == SCT_TABLE_QUERY)
    {
        int index = THREAD_HANDLE;
        T_STATEMENT *tmp_stmt;

        tmp_stmt = (T_STATEMENT *) PMEM_ALLOC(sizeof(T_STATEMENT));
        if (tmp_stmt == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return RET_ERROR;
        }

        if (check_validtable(handle,
                        stmt->u._create.u.table_query.table.tablename) == 1)
        {
            PMEM_FREE(tmp_stmt);
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_ALREADYEXISTTABLE, 0);
            return RET_ERROR;
        }

        tmp_stmt->sqltype = ST_SELECT;
        sc_memcpy(&tmp_stmt->u._select,
                &stmt->u._create.u.table_query.query, sizeof(T_SELECT));
        // PREPARE에서 parsing하는 부분이 있으므로.. 이 메모리는 free되면 안됨
        tmp_stmt->parsing_memory = stmt->parsing_memory;
        ret = SQL_prepare(&handle, tmp_stmt, gClients[index].flags);
        sc_memcpy(&stmt->u._create.u.table_query.query,
                &tmp_stmt->u._select, sizeof(T_SELECT));


        PMEM_FREE(tmp_stmt);
        if (ret == RET_ERROR)
        {
            return RET_ERROR;
        }
    }
    else if (stmt->u._create.type == SCT_VIEW)
    {
        return check_create_view(handle, &(stmt->u._create.u.view));
    }
    else if (stmt->u._create.type == SCT_SEQUENCE)
    {
    }
    else if (stmt->u._create.type == SCT_DUMMY)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDSYNTAX, 0);
        return RET_ERROR;
    }
    else
    {       /* stmt->u._create.type == SCT_INDEX */
        ptr = stmt->u._create.u.index.table.tablename;

        if (sc_strlen(stmt->u._create.u.index.indexname) >= INDEX_NAME_LENG)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_MAX_INDEX_NAME_LEN_EXEEDED, 0);
            return RET_ERROR;
        }
        {
            int rtnVal = check_valid_basetable(handle, ptr);

            if (rtnVal != 1)
            {
                if (rtnVal == 0)        // case: viewtable.
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_INVALID_OPERATION_VIEW, 0);
                }
                else
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_INVALIDBASETABLE, 0);
                }

                ret = RET_ERROR;
                goto RETURN_LABEL;
            }
        }

        fieldnum = dbi_Schema_GetFieldsInfo(handle, ptr, &fieldinfo);

        if (fieldnum < 0L)
        {
            ret = RET_ERROR;
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, fieldnum, 0);
            goto RETURN_LABEL;
        }

        for (i = 0; i < stmt->u._create.u.index.fields.len; i++)
        {
            for (j = 0; j < fieldnum; j++)
                if (!mdb_strcmp(stmt->u._create.u.index.fields.list[i].name,
                                fieldinfo[j].fieldName))
                {
                    if (stmt->u._create.u.index.fields.list[i].collation
                            == MDB_COL_NONE)
                        stmt->u._create.u.index.fields.list[i].collation
                                = fieldinfo[j].collation;
                    break;
                }
            if (j == fieldnum)
            {
                ret = RET_ERROR;
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDCOLUMN, 0);
                goto RETURN_LABEL;
            }

            if (fieldinfo[j].flag & NOLOGGING_BIT)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_NOLOGGING_COLUMN_INDEX, 0);
                goto RETURN_LABEL;
            }

            if (!sql_collation_compatible_check(fieldinfo[j].type,
                            stmt->u._create.u.index.fields.list[i].collation))
            {
                ret = RET_ERROR;
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_MISMATCH_COLLATION, 0);
                goto RETURN_LABEL;
            }

            /* check if the key colume is duplicated */
            for (j = i + 1; j < stmt->u._create.u.index.fields.len; j++)
            {
                if (!mdb_strcmp(stmt->u._create.u.index.fields.list[i].name,
                                stmt->u._create.u.index.fields.list[j].name))
                {
                    ret = RET_ERROR;
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_DUPLICATECOLUMNNAME, 0);
                    goto RETURN_LABEL;
                }
            }
        }

        // ENHANCEMENT
        // 전체 index list를 가지고 같은 이름을 가지는 index가 있는 지
        // 체크를 해야 하는데, 특정 테이블에 걸려있는 index를 넘겨주는
        // 함수만 있고 전체 index list를 넘겨주는 함수는 없으므로
        // 여기서의 비교는 큰 의미가 없다.
        indexnum = dbi_Schema_GetTableIndexesInfo(handle, ptr,
                SCHEMA_REGU_INDEX, &indexinfo);

        if (indexnum < 0)
        {
            ret = RET_ERROR;
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, indexnum, 0);
            goto RETURN_LABEL;
        }
        for (i = 0; i < indexnum; i++)
        {
            if (!mdb_strcmp(stmt->u._create.u.index.indexname,
                            indexinfo[i].indexName))
            {
                ret = RET_ERROR;
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_DUPLICATEINDEXNAME, 0);
#ifdef MDB_DEBUG
                __SYSLOG("\t%s\n", stmt->u._create.u.index.indexname);
#endif
                goto RETURN_LABEL;
            }
            if (indexinfo[i].numFields == stmt->u._create.u.index.fields.len)
            {
                ret = dbi_Schema_GetIndexInfo(handle, indexinfo[i].indexName,
                        NULL, &indexfields, NULL);
                if (ret < 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    ret = RET_ERROR;
                    goto RETURN_LABEL;
                }

                for (j = 0; j < stmt->u._create.u.index.fields.len; j++)
                {
                    /* check if the same ordertype of the field */
                    if (stmt->u._create.u.index.fields.list[j].ordertype
                            != indexfields[j].order)
                        break;

                    /* find fieldid by given name */
                    for (k = 0; k < fieldnum; k++)
                        if (!mdb_strcmp(stmt->u._create.u.index.fields.list[j].
                                        name, fieldinfo[k].fieldName))
                            break;
                    /* and then check if the same field */
                    if (indexfields[j].fieldId != fieldinfo[k].fieldId)
                        break;
                    if (stmt->u._create.u.index.fields.list[j].collation
                            != (MDB_COL_TYPE) indexfields[j].collation)
                        break;
                }
                PMEM_FREENUL(indexfields);
                if (j == indexinfo[i].numFields)
                {       // 똑같은 index가 이미 존재
                    ret = RET_ERROR;

                    if (!mdb_strncmp(indexinfo[i].indexName, MAXREC_KEY_PREFIX,
                                    sc_strlen(MAXREC_KEY_PREFIX)))
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_DUPIDXFIXEDNUMTABLE, 0);
                    }
                    else
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_DUPLICATEINDEX, 0);
                    goto RETURN_LABEL;
                }
            }
        }

      RETURN_LABEL:
        PMEM_FREENUL(fieldinfo);
        PMEM_FREENUL(indexinfo);
        if (ret < 0)
            return ret;
    }

    table_info = NULL;

    if (stmt->u._create.type == SCT_TABLE_ELEMENTS)
        table_info = &stmt->u._create.u.table_elements.table;
    if (stmt->u._create.type == SCT_TABLE_QUERY)
        table_info = &stmt->u._create.u.table_query.table;

    if (table_info != NULL)
    {
        /* check for existed column */
    }

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
static int
check_drop(int handle, T_STATEMENT * stmt)
{
    if (stmt->u._drop.type == SDT_TABLE)
    {
        if (check_systable(stmt->u._drop.target) >= 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTALLOWED, 0);
            return RET_ERROR;
        }
        if (check_valid_basetable(handle, stmt->u._drop.target) != 1)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDBASETABLE,
                    0);
            return RET_ERROR;
        }
    }
    else if (stmt->u._drop.type == SDT_VIEW)
    {
        if (check_valid_basetable(handle, stmt->u._drop.target) != 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDVIEW, 0);
            return RET_ERROR;
        }
    }
    else if (stmt->u._drop.type == SDT_SEQUENCE)
    {
        if (stmt->u._drop.target == NULL || stmt->u._drop.target[0] == '\0')
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    DB_E_SEQUENCE_INVALID_NAME, 0);
            return RET_ERROR;
        }
    }
    else
    {
        // ENHANCEMENT
        // index name이 valid한 것인지에 대한 체크가 필요하다.
    }

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
static int
check_rename(int handle, T_STATEMENT * stmt)
{
    char *old_name, *new_name;

    old_name = stmt->u._rename.oldname;
    new_name = stmt->u._rename.newname;

    if (stmt->u._rename.type == SRT_TABLE)
    {
        if (check_systable(old_name) >= 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTALLOWED, 0);
            return RET_ERROR;
        }
        if (check_validtable(handle, old_name) != 1)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTABLE, 0);
            return RET_ERROR;
        }
        if (check_validtable(handle, new_name) == 1)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_ALREADYEXISTTABLE, 0);
            return RET_ERROR;
        }
    }
    else
    {       /* stmt->u._rename.type == SRT_INDEX */
        ;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_datatype_conversion

/*! \breif 자료형을 변경할 수 있는지 검사한다.
 ************************************
 * \param attr   : 변경할 자료형
 * \param field  : 기존 자료형
 ************************************
 * \return RET_SUCCESS or some errors
 ************************************
 * \note
 *****************************************************************************/
int
check_datatype_conversion(T_ATTRDESC * attr, SYSFIELD_T * field)
{
    /* check compatibility */
    if ((IS_STRING(attr->type) && !IS_STRING(field->type)) ||
            (!IS_STRING(attr->type) && IS_STRING(field->type)))
    {
        return SQL_E_SETTYPE_COMPATIBILITY;
    }
    else
    {
        /* check precision & scale */
        switch (field->type)
        {
        case DT_FLOAT:
            field->scale = 9;
            break;
        case DT_DOUBLE:
            field->scale = 16;
            break;
        default:
            break;
        }
        switch (attr->type)
        {
        case DT_FLOAT:
            attr->dec = 9;
            break;
        case DT_DOUBLE:
            attr->dec = 16;
            break;
        default:
            break;
        }
        if ((attr->len < field->length) || (attr->dec < field->scale))
        {
            return SQL_E_SETTYPE_PRECSCALE;
        }
    }
    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_defaultvalue

/*! \breif 주어진 DEFAULT값이 자료형의 표현 범위내에 있는지 검사한다.
 ************************************
 * \param value      :
 * \param type       :
 * \param length     :
 ************************************
 * \return RET_SUCCESS or SQL_E_INVALIDDEFAULTVALUE
 ************************************
 * \note DEFAULT값의 자료형은 DT_VARCHAR, DT_BIGINT, DT_DOUBLE 중 하나이다.
 *****************************************************************************/
int
check_defaultvalue(T_VALUEDESC * value, DataType type, int length)
{
    if ((IS_STRING(type) && (value->valuetype != DT_VARCHAR)) ||
            (!IS_STRING(type) && (value->valuetype == DT_VARCHAR)) ||
            (IS_NBS(type) && !IS_NBS(value->valuetype)) ||
            (IS_BYTE(type) && !IS_BYTE(value->valuetype)))
    {
        return SQL_E_INVALIDDEFAULTVALUE;
    }

    switch (type)
    {
    case DT_CHAR:
    case DT_VARCHAR:
        if (length < (int) sc_strlen(value->u.ptr))
        {
            return SQL_E_INVALIDDEFAULTVALUE;
        }
        break;
    case DT_NCHAR:
    case DT_NVARCHAR:
        if (length < sc_wcslen(value->u.Wptr))
        {
            return SQL_E_INVALIDDEFAULTVALUE;
        }
        break;
    case DT_DATETIME:
    case DT_DATE:
    case DT_TIME:
    case DT_TIMESTAMP:
        break;

    case DT_BYTE:
    case DT_VARBYTE:
        if (length < value->value_len)
            return SQL_E_INVALIDDEFAULTVALUE;

        break;
    default:
        if (check_numeric_bound(value, type, length) == RET_FALSE)
        {
            return SQL_E_INVALIDDEFAULTVALUE;
        }
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_alter_add

/*! \breif ALTER TABLE ... ADD 에 대한 의미검사를 한다.
 ************************************
 * \param handle    :
 * \param alter     :
 ************************************
 * \return RET_SUCCESS or RET_ERROR
 ************************************
 * \note 추가할 각 칼럼에 대해 다음을 체크한다.\n
 *        - 칼럼명이 중복되는가\n
 *        - 테이블내 칼럼의 최대 정의 개수를 넘어서는가\n
 *        - Primary Key를 설정할 경우, 이미 Key가 설정되어 있나\n
 *        - Primary Key를 설정할 경우, 키 칼럼이 존재하는가
 *****************************************************************************/
int
check_alter_add(int handle, T_ALTER * alter)
{
    SYSFIELD_T *fields;
    T_ATTRDESC *column_attr;
    int n_fields, i, j;

    n_fields = dbi_Schema_GetFieldsInfo(handle, alter->tablename, &fields);
    if (n_fields < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDBASETABLE, 0);
        return RET_ERROR;
    }
    if (alter->scope == SAT_COLUMN)
    {
        for (i = 0; i < alter->column.attr_list.len; i++)
        {
            column_attr = &alter->column.attr_list.list[i];
            for (j = 0; j < n_fields; j++)
            {
                if (!mdb_strcmp(column_attr->attrname, fields[j].fieldName))
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_DUPLICATECOLUMNNAME, 0);
                    PMEM_FREENUL(fields);
                    return RET_ERROR;
                }
                if (alter->constraint.type == SAT_PRIMARY_KEY &&
                        (fields[j].flag & PRI_KEY_BIT) != 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_MULTIPRIMARYKEY, 0);
                    PMEM_FREENUL(fields);
                    return RET_ERROR;
                }

                if ((column_attr->flag & NOLOGGING_BIT)
                        && alter->constraint.type == SAT_PRIMARY_KEY)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_NOLOGGING_COLUMN_INDEX, 0);
                    PMEM_FREENUL(fields);
                    return RET_ERROR;
                }
            }
            if (check_strtype_overflow(column_attr->type, column_attr->len,
                            column_attr->fixedlen) < 0)
            {
                PMEM_FREENUL(fields);
                return RET_ERROR;
            }
        }
    }
    else
    {       /* alter->scope == SAT_CONSTRAINT */
        for (i = 0; i < alter->constraint.list.len; i++)
        {
            for (j = 0; j < n_fields; j++)
            {
                if (!mdb_strcmp(alter->constraint.list.list[i],
                                fields[j].fieldName))
                {
                    fields[j].fieldId = -1;
                    break;
                }
            }
            if (j == n_fields)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDCOLUMN, 0);
                PMEM_FREENUL(fields);
                return RET_ERROR;
            }

            if ((fields[j].flag & NOLOGGING_BIT)
                    && alter->constraint.type == SAT_PRIMARY_KEY)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_NOLOGGING_COLUMN_INDEX, 0);
                PMEM_FREENUL(fields);
                return RET_ERROR;
            }
        }
        for (j = 0; j < n_fields; j++)
        {
            if (alter->constraint.type == SAT_PRIMARY_KEY &&
                    (fields[j].flag & PRI_KEY_BIT) != 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_MULTIPRIMARYKEY, 0);
                PMEM_FREENUL(fields);
                return RET_ERROR;
            }
        }
    }
    alter->old_fields = fields;
    alter->old_fieldnum = n_fields;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_alter_drop

/*! \breif ALTER TABLE ... DROP 에 대한 의미검사를 한다.
 ************************************
 * \param handle      :
 * \param alter         :
 ************************************
 * \return RET_SUCCESS or RET_ERROR
 ************************************
 * \note 삭제할 각 칼럼에 대해 다음을 체크한다.\n
 *        - 칼럼명이 존재하는가\n
 *        - 칼럼 삭제후 테이블에 정의된 칼럼이 없는가
 *****************************************************************************/
int
check_alter_drop(int handle, T_ALTER * alter)
{
    SYSFIELD_T *fields;
    char *column_name;
    int n_fields, i, j;

    n_fields = dbi_Schema_GetFieldsInfo(handle, alter->tablename, &fields);
    if (n_fields < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDBASETABLE, 0);
        return RET_ERROR;
    }
    if (alter->scope == SAT_COLUMN)
    {
        for (i = 0; i < alter->column.name_list.len; i++)
        {
            column_name = alter->column.name_list.list[i];
            for (j = 0; j < n_fields; j++)
            {
                if (!mdb_strcmp(column_name, fields[j].fieldName))
                {
                    fields[j].fieldId = -1;
                    break;
                }
            }
            if (j == n_fields)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDCOLUMN, 0);
                PMEM_FREENUL(fields);
                return RET_ERROR;
            }
        }
        if (alter->column.name_list.len == n_fields)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ALLCOLUMNDROP, 0);
            PMEM_FREENUL(fields);
            return RET_ERROR;
        }
    }       /* else alter->scope == SAT_CONSTRAINT */

    alter->old_fields = fields;
    alter->old_fieldnum = n_fields;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_alter_alter

/*! \breif ALTER TABLE ... ALTER 에 대한 의미검사를 한다.
 ************************************
 * \param handle     :
 * \param alter         :
 ************************************
 * \return RET_SUCCESS or RET_ERROR
 ************************************
 * \note 변경할 각 칼럼에 대해 다음을 체크한다.\n
 *        - 칼럼명이 존재하는가\n
 *        - 칼럼명을 변경할 경우, 새 칼럼명이 중복되는가\n
 *        - 키를 설정하는 경우, 기존에 키가 있는가\n
 *        - Primary Key를 설정할 경우, 이미 Key가 설정되어 있나\n
 *        - Primary Key를 설정할 경우, 키 칼럼이 존재하는가
 *****************************************************************************/

int
check_alter_alter(int handle, T_ALTER * alter)
{
    SYSFIELD_T *fields;
    T_ATTRDESC *column_attr;
    T_VALUEDESC colval, defval;
    char *column_name;
    int n_fields;
    int i, j, ret;

    n_fields = dbi_Schema_GetFieldsInfo(handle, alter->tablename, &fields);
    if (n_fields < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTABLE, 0);
        return RET_ERROR;
    }
    if (alter->scope == SAT_COLUMN)
    {
        if (alter->column.name_list.list)
        {   /* rename column */
            column_name = alter->column.name_list.list[0];
            column_attr = &alter->column.attr_list.list[0];
            for (j = 0; j < n_fields; j++)
            {
                if (!mdb_strcmp(column_name, fields[j].fieldName))
                {
                    fields[j].fieldId = -1;
                    break;
                }
            }
            if (j == n_fields)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDCOLUMN, 0);
                PMEM_FREENUL(fields);
                return RET_ERROR;
            }
            for (j = 0; j < n_fields; j++)
            {
                if (!mdb_strcmp(column_attr->attrname, fields[j].fieldName))
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_DUPLICATECOLUMNNAME, 0);
                    PMEM_FREENUL(fields);
                    return RET_ERROR;
                }
            }
        }
        else
        {   /* alter column attribute */
            for (i = 0; i < alter->column.attr_list.len; i++)
            {
                column_attr = &alter->column.attr_list.list[i];
                for (j = 0; j < n_fields; j++)
                {
                    if (!mdb_strcmp(column_attr->attrname,
                                    fields[j].fieldName))
                    {
                        fields[j].fieldId = -1;
                        break;
                    }
                }
                if (j == n_fields)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_INVALIDCOLUMN, 0);
                    PMEM_FREENUL(fields);
                    return RET_ERROR;
                }
                /* check alternation of datatype */
                if (column_attr->len > 0 &&     /* that means data type is defined */
                        alter->having_data != 0)
                {
                    ret = check_datatype_conversion(column_attr, &fields[j]);
                    if (ret < 0)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                        PMEM_FREENUL(fields);
                        return RET_ERROR;
                    }
                }

                if ((fields[j].flag & AUTO_BIT) &&
                        column_attr->type != DT_INTEGER)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_NOTALLOW_AUTO, 0);
                    PMEM_FREENUL(fields);
                    return RET_ERROR;
                }

                if ((column_attr->flag & NOLOGGING_BIT)
                        && (column_attr->flag & PRI_KEY_BIT))
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_NOLOGGING_COLUMN_INDEX, 0);
                    PMEM_FREENUL(fields);
                    return RET_ERROR;
                }

                if (check_strtype_overflow(column_attr->type, column_attr->len,
                                column_attr->fixedlen) < 0)
                {
                    PMEM_FREENUL(fields);
                    return RET_ERROR;
                }

                /* check alternation of default value */
                if (column_attr->len > 0)
                {
                    /* if the data type of default value is defined,
                       this is already done at syntax checking time */
                    ;
                }
                else
                {
                    if (column_attr->defvalue.defined == 1 &&
                            column_attr->defvalue.not_null == 1)
                    {

                        defval.call_free = 0;
                        defval.is_null = 0;
                        defval.valuetype = column_attr->type;
                        defval.attrdesc.len = column_attr->defvalue.value_len;
                        sc_memcpy(&defval.u, &column_attr->defvalue.u,
                                sizeof(defval.u));
                        ret = check_defaultvalue(&defval,
                                fields[j].type, fields[j].length);
                        if (ret < 0)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                            PMEM_FREENUL(fields);
                            return RET_ERROR;
                        }
                        sc_memset(&colval, 0, sizeof(T_VALUEDESC));
                        colval.valuetype = fields[j].type;
                        colval.attrdesc.len = fields[j].length;
                        if (calc_func_convert(&colval, &defval, &colval,
                                        MDB_FALSE) != RET_SUCCESS)
                        {
                            PMEM_FREENUL(fields);
                            return RET_ERROR;
                        }
                        /* set data type and converted default value */

                        column_attr->type = fields[j].type;
                        column_attr->len = fields[j].length;
                        column_attr->dec = fields[j].scale;
                        column_attr->fixedlen = fields[j].fixed_part;
                        if (column_attr->type == DT_NVARCHAR)
                            column_attr->fixedlen /= WCHAR_SIZE;
                        column_attr->collation = fields[j].collation;
                        sc_memcpy(&column_attr->defvalue.u, &colval.u,
                                sizeof(colval.u));
                    }
                }
            }
        }
    }
    else
    {       /* alter->scope == SAT_CONSTRAINT */
        for (i = 0; i < alter->constraint.list.len; i++)
        {
            for (j = 0; j < n_fields; j++)
            {
                if (!mdb_strcmp(alter->constraint.list.list[i],
                                fields[j].fieldName))
                {
                    fields[j].fieldId = -1;
                    break;
                }
            }
            if (j == n_fields)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDCOLUMN, 0);
                PMEM_FREENUL(fields);
                return RET_ERROR;
            }

            if ((fields[j].flag & NOLOGGING_BIT)
                    && alter->constraint.type == SAT_PRIMARY_KEY)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_NOLOGGING_COLUMN_INDEX, 0);
                PMEM_FREENUL(fields);
                return RET_ERROR;
            }
        }
    }
    alter->old_fields = fields;
    alter->old_fieldnum = n_fields;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_alter

/*! \breif ALTER TABLE 문에 대한 의미 검사를 수행한다.
 *
 ************************************
 * \param handle    :
 * \param stmt      :
 ************************************
 * \return RET_SUCCESS or RET_ERROR
 ************************************
 * \note
 *  - index rebuild 추가
 *****************************************************************************/
static int
check_alter(int handle, T_STATEMENT * stmt)
{
    T_ALTER *alter;
    char *tablename;

    alter = &stmt->u._alter;
    tablename = alter->tablename;

    if (alter->type == SAT_SEQUENCE)
    {
        /* do not thing */
        return RET_SUCCESS;
    }
    else if (alter->type == SAT_REBUILD)
    {
        /* do not thing */
        return RET_SUCCESS;
    }
    /* check table validity */
    if (check_systable(tablename) >= 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTALLOWED, 0);
        return RET_ERROR;
    }

    {
        int rtnVal = check_valid_basetable(handle, tablename);

        if (rtnVal != 1)
        {
            if (rtnVal == 0)    // case: viewtable.
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALID_OPERATION_VIEW, 0);
            }
            else
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDBASETABLE, 0);
            }

            return RET_ERROR;
        }
    }


    if (alter->type == SAT_MSYNC)
    {
        SYSTABLE_T tableinfo;
        int ret;

        ret = dbi_Schema_GetTableInfo(handle, tablename, &tableinfo);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return RET_ERROR;
        }

        if (alter->msync_alter_type == 1)
        {
            if (tableinfo.flag & _CONT_MSYNC || tableinfo.flag & _CONT_VIEW)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_NOTPERMITTED, 0);
                return RET_ERROR;
            }
        }
        else
        {
            if ((tableinfo.flag & _CONT_MSYNC) == 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_NOTPERMITTED, 0);
                return RET_ERROR;
            }
        }

        return RET_SUCCESS;
    }

    if (alter->type == SAT_LOGGING)
        return RET_SUCCESS;

    alter->having_data =
            (dbi_Cursor_DirtyCount(handle, alter->tablename) > 0) ? 1 : 0;
    switch (alter->type)
    {
    case SAT_ADD:
        return check_alter_add(handle, alter);
    case SAT_DROP:
        return check_alter_drop(handle, alter);
    case SAT_ALTER:
        return check_alter_alter(handle, alter);
    default:
        return RET_ERROR;
    }
}

/*****************************************************************************/
//! check_truncate

/*! \breif TRUNCATE TABLE 문에 대한 의미 검사를 한다.
 ************************************
 * \param handle(in)    :
 * \param stmt(in)      :
 ************************************
 * \return RET_SUCCESS or RET_ERROR
 ************************************
 * \note
 *  - truncate 구문 확장
 *****************************************************************************/
static int
check_truncate(int handle, T_STATEMENT * stmt)
{
    int ret;

    if (check_systable(stmt->u._truncate.objectname) >= 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTALLOWED, 0);
        return RET_ERROR;
    }
    ret = check_valid_basetable(handle, stmt->u._truncate.objectname);
    if (ret != 1)
    {
        if (ret == 0)   // case: viewtable.
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_INVALID_OPERATION_VIEW, 0);
        }
        else
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDBASETABLE,
                    0);
        }

        return RET_ERROR;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_describe

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)    :
 * \param stmt(in)      :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - desc 명령어만 들어오면 user table을 보여준다
 *****************************************************************************/
static int
check_describe(int handle, T_STATEMENT * stmt)
{
    if (stmt->u._desc.describe_table)
    {
        if (check_validtable(handle, stmt->u._desc.describe_table) != 1)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTABLE, 0);
            return RET_ERROR;
        }
    }
    return RET_SUCCESS;
}

/*****************************************************************************/
//! check_admin

/*! \breif administrator SQL 문장 검사
 ************************************
 * \param handle(in)    :
 * \param stmt(in)      :
 ************************************
 * \return RET_SUCCESS or RET_ERROR
 ************************************
 * \note
 *  - ADMIN 문장 추가(EXPORT & IMPORT)
 *****************************************************************************/
static int
check_admin(int handle, T_STATEMENT * stmt)
{
    T_ADMIN *admin = &(stmt->u._admin);

    switch (admin->type)
    {
    case SADM_EXPORT_ALL:
    case SADM_EXPORT_ONE:
        {
            // 파일 정보 설정할 것
            break;
        }
    case SADM_IMPORT:
        {
            // 파일 정보 설정할 것
            break;
        }
    default:
        return RET_ERROR;
    }

    return RET_SUCCESS;
}

////////////////////////// Util Functions End /////////////////////////



///////////////////// Main Helper Functions Start /////////////////////

/*****************************************************************************/
//! sql_validate_stmt

/*! \breif  STMT의 COLUMN들이 문법을 CHECK하고, TABLE의 필드 정보를 설정
 ************************************
 * \param handle(in)    : DB Handle
 * \param stmt(in/out)  : statement
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - ADMIN 문장 추가(EXPORT & IMPORT)
 *****************************************************************************/
int
sql_validating(int handle, T_STATEMENT * stmt)
{
    switch (stmt->sqltype)
    {
    case ST_SELECT:
        if (check_select(handle, stmt) != RET_SUCCESS)
            return RET_ERROR;
        break;
    case ST_UPSERT:
    case ST_INSERT:
        if (check_insert(handle, stmt) == RET_ERROR)
            return RET_ERROR;
        break;
    case ST_DELETE:
        if (check_delete(handle, stmt) == RET_ERROR)
            return RET_ERROR;
        if (stmt->u._delete.subselect.first_sub &&
                check_subselect(handle,
                        &stmt->u._delete.subselect) != RET_SUCCESS)
            return RET_ERROR;
        break;
    case ST_UPDATE:
        if (check_update(handle, stmt) == RET_ERROR)
            return RET_ERROR;
        if (stmt->u._update.subselect.first_sub &&
                check_subselect(handle,
                        &stmt->u._update.subselect) != RET_SUCCESS)
            return RET_ERROR;
        break;
    case ST_CREATE:
        if (check_create(handle, stmt) == RET_ERROR)
            return RET_ERROR;
        break;
    case ST_DROP:
        if (check_drop(handle, stmt) == RET_ERROR)
            return RET_ERROR;
        break;
    case ST_RENAME:
        if (check_rename(handle, stmt) == RET_ERROR)
            return RET_ERROR;
        break;
    case ST_ALTER:
        if (check_alter(handle, stmt) == RET_ERROR)
            return RET_ERROR;
        break;
    case ST_TRUNCATE:
        if (check_truncate(handle, stmt) == RET_ERROR)
            return RET_ERROR;
        break;
    case ST_DESCRIBE:
        if (check_describe(handle, stmt) == RET_ERROR)
            return RET_ERROR;
        break;
    case ST_ADMIN:
        if (check_admin(handle, stmt) == RET_ERROR)
            return RET_ERROR;
        break;
    default:
        break;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! view_transform

/*! \breif  create view를 할때 사용한다
 ************************************
 * \param handle(in)    :
 * \param select(in)    :
 * \param sinfo(in)     :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *
 *****************************************************************************/
int
view_transform(int handle, T_SELECT * select, T_STATEMENT * sinfo)
{
    T_SELECT *sub_select, *cur_select;
    T_LIST_SELECTION *selection;
    T_QUERYDESC *qdesc;
    T_JOINTABLEDESC *join;
    SYSVIEW_T *viewinfo = NULL;
    T_STATEMENT *stmt = NULL;
    int i, j, ret = RET_SUCCESS;

    /* find view tables and rewrite as query */
    qdesc = &select->planquerydesc.querydesc;
    for (i = 0; i < qdesc->fromlist.len; i++)
    {
        join = &qdesc->fromlist.list[i];
        if (join->table.correlated_tbl != NULL)
        {
            /* initailize tableinfo and fieldinfo */
            join->tableinfo.numFields = 0;
            join->fieldinfo = NULL;
            continue;
        }

        ret = dbi_Schema_GetTableFieldsInfo(handle, join->table.tablename,
                &join->tableinfo, &join->fieldinfo);
        if (ret <= 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTABLE, 0);
            ret = RET_ERROR;
            goto end_label;
        }

        if ((join->tableinfo.flag & _CONT_VIEW) == 0)
        {
            continue;
        }

        ret = dbi_Schema_GetViewDefinition(handle, join->table.tablename,
                &viewinfo);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDVIEW, 0);
            ret = RET_ERROR;
            goto end_label;
        }

        if (stmt == NULL)
        {
            stmt = (T_STATEMENT *) PMEM_ALLOC(sizeof(T_STATEMENT));
            if (stmt == NULL)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                ret = RET_ERROR;
                goto end_label;
            }
        }

        sc_memset(stmt, 0, sizeof(T_STATEMENT));
        stmt->id = sinfo->id;
        stmt->status = sinfo->status;
        stmt->trans_info = sinfo->trans_info;
        stmt->query = viewinfo->definition;
        stmt->qlen = MAX_SYSVIEW_DEF;
        stmt->qlen = sc_strlen(viewinfo->definition);
        stmt->parsing_memory = sinfo->parsing_memory;

        ret = sql_yyparse(redirect_yyinput(stmt, stmt->query, stmt->qlen));
        if (ret < 0)
        {
            ret = RET_ERROR;
            goto end_label;
        }

        selection = &stmt->u._select.planquerydesc.querydesc.selection;
        if (selection->len > 0)
        {
            selection->aliased_len = selection->len;
        }
        else
        {   /* selection->len == 0 */
            selection->aliased_len = -1;
            selection->list = (T_SELECTION *)
                    PMEM_ALLOC(sizeof(T_SELECTION) *
                    join->tableinfo.numFields);
            if (selection->list == NULL)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                ret = RET_ERROR;
                goto end_label;
            }
            selection->max = selection->len = join->tableinfo.numFields;
        }

        for (j = 0; j < join->tableinfo.numFields; j++)
        {
            sc_memcpy(selection->list[j].res_name,
                    join->fieldinfo[j].fieldName, FIELD_NAME_LENG - 1);
            selection->list[j].res_name[FIELD_NAME_LENG - 1] = '\0';
        }

        sub_select = PMEM_ALLOC(sizeof(T_SELECT));
        if (sub_select == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            ret = RET_ERROR;
            goto end_label;
        }
        sc_memcpy(sub_select, &stmt->u._select, sizeof(T_SELECT));

        /* relink super query */
        for (cur_select = sub_select->first_sub;
                cur_select != NULL; cur_select = cur_select->sibling_query)
        {
            if (cur_select->super_query == &stmt->u._select)
            {
                cur_select->super_query = sub_select;
            }
        }
        qdesc = &sub_select->planquerydesc.querydesc;
        for (j = 0; j < qdesc->setlist.len; ++j)
        {
            if (qdesc->setlist.list[j]->elemtype == SPT_SUBQUERY)
            {
                cur_select = qdesc->setlist.list[j]->u.subq;
                if (cur_select->super_query == &stmt->u._select)
                {
                    cur_select->super_query = sub_select;
                }
            }
        }
        qdesc = &select->planquerydesc.querydesc;

        sub_select->super_query = select;
        join->table.correlated_tbl = sub_select;
        join->table.correlated_tbl->kindofwTable = iTABLE_SUBSELECT;

        if (select->first_sub == NULL)
        {
            select->first_sub = sub_select;
        }
        else
        {
            cur_select = select->first_sub;
            while (cur_select->sibling_query)
            {
                cur_select = cur_select->sibling_query;
            }
            cur_select->sibling_query = sub_select;
            /* we must keep sibling order ?
               sub_select->sibling_query = select->first_sub;
               select->first_sub = sub_select;
             */
        }
        PMEM_FREENUL(viewinfo);

        /* check_subselect_from()에서 view table인 경우
           temp table 정보로 재설정하기 위해 free */
        PMEM_FREENUL(join->fieldinfo);
    }

    if (select->first_sub)
    {
        ret = view_transform(handle, select->first_sub, sinfo);
        if (ret != RET_SUCCESS)
        {
            goto end_label;
        }
    }

    if (select->sibling_query)
    {
        ret = view_transform(handle, select->sibling_query, sinfo);
        if (ret != RET_SUCCESS)
        {
            goto end_label;
        }
    }

    for (i = 0; i < qdesc->setlist.len; ++i)
    {
        if (qdesc->setlist.list[i]->elemtype == SPT_SUBQUERY)
        {
            ret = view_transform(handle, qdesc->setlist.list[i]->u.subq,
                    sinfo);
            if (ret != RET_SUCCESS)
            {
                goto end_label;
            }
        }
    }

  end_label:

    PMEM_FREENUL(viewinfo);
    PMEM_FREENUL(stmt);

    if (ret < 0)
    {
        return RET_ERROR;
    }

    return RET_SUCCESS;
}

///////////////////// Main Helper Functions End ///////////////////////

//////////////////////////// Main Function ////////////////////////////

// 각 T_SELECT의 queryresult에는 그 subquery의 schema가 들어간다.
// kindofwTable은 subquery의 종류(from/where, correlated/uncorrelated)를.
/*****************************************************************************/
//! sql_parser

/*! \breif  SQL 문장을 PARSING
 ************************************
 * \param handle(in)    :
 * \param stmt(in/out)  : STATEMENT
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
sql_parsing(int handle, T_STATEMENT * stmt)
{
    int ret = RET_SUCCESS;

    if (stmt->sqltype == ST_NONE ||
            (stmt->sqltype == ST_SELECT && stmt->u._select.tstatus == TSS_RAW))
    {
        void *ptr = redirect_yyinput(stmt, stmt->query, stmt->qlen);

        if (sql_yyparse(ptr) < 0)
        {
            return RET_ERROR;
        }
    }

    if (stmt->sqltype == ST_SELECT)
    {
        ret = view_transform(handle, &stmt->u._select, stmt);
        if (ret != RET_SUCCESS)
            return ret;
    }
    else if (stmt->sqltype == ST_UPDATE)
    {
        if (stmt->u._update.subselect.first_sub &&
                (ret = view_transform(handle, &stmt->u._update.subselect,
                                stmt)) != RET_SUCCESS)
            return ret;
    }
    else if (stmt->sqltype == ST_DELETE)
    {
        if (stmt->u._delete.subselect.first_sub &&
                (ret = view_transform(handle, &stmt->u._delete.subselect,
                                stmt)) != RET_SUCCESS)
            return ret;
    }

    return ret;
}
