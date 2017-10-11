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
#include "mdb_Server.h"
#include "mdb_compat.h"
#include "sql_main.h"

#include "sql_parser.h"
#include "sql_plan.h"
#include "sql_execute.h"
#include "sql_norm.h"

#include "sql_func_timeutil.h"
#include "sql_mclient.h"

extern char *get_plan_string(T_STATEMENT * select);
extern int drop_temp_index(int handle, char *index);

extern void free_scan_info(T_NESTING * nest);
static void close_cursor_select(int handle, T_TRANSDESC * trans_info,
        T_SELECT * select);

char *__systables[] = {
    "systables",
    "sysfields",
    "sysindexes",
    "sysindexfields",
    "sysstatus",
    "sysdummy",
    "sysviews",
    "syssequences",
    "#",
    NULL
};

void
EXPRDESC_CLEANUP(T_PARSING_MEMORY * chunk, T_EXPRESSIONDESC * expr)
{
    int i;

    for (i = 0; i < expr->len; i++)
    {
        POSTFIXELEM_CLEANUP(chunk, expr->list[i]);
        sql_mem_set_free_postfixelem(chunk, expr->list[i]);
        expr->list[i] = NULL;
    }
    sql_mem_set_free_postfixelem_list(chunk, expr->list);
    expr->list = NULL;
    expr->max = 0;
    expr->len = 0;
}

void
EXPRDESC_LIST_CLEANUP(T_PARSING_MEMORY * chunk, EXPRDESC_LIST * list)
{
    EXPRDESC_LIST *cur, *prev;
    T_EXPRESSIONDESC *expr;

    for (cur = list; cur;)
    {
        prev = cur;
        cur = cur->next;
        expr = (T_EXPRESSIONDESC *) prev->ptr;

        EXPRDESC_CLEANUP(chunk, expr);
        sql_mem_freenul(chunk, expr);
        sql_mem_freenul(chunk, prev);
    }
    list = NULL;
}

void
EXPRDESC_LIST_FREENUL(T_PARSING_MEMORY * chunk, EXPRDESC_LIST * list)
{
    EXPRDESC_LIST *cur, *prev;

    for (cur = list; cur;)
    {
        prev = cur;
        cur = cur->next;
        sql_mem_freenul(chunk, prev);
    }
    list = NULL;
}

void
ATTRED_EXPRDESC_LIST_CLEANUP(T_PARSING_MEMORY * chunk,
        ATTRED_EXPRDESC_LIST * list)
{
    ATTRED_EXPRDESC_LIST *cur, *prev;

    for (cur = list; cur;)
    {
        prev = cur;
        cur = cur->next;

        sql_mem_freenul(chunk, prev->ptr);
        sql_mem_freenul(chunk, prev);
    }
    list = NULL;
}

/*****************************************************************************/
//! POSTFIXELEM_CLEANUP

/*! \breif 
 ************************************
 * \param memory(in)    :
 * \param elem_(in/out) :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *      
 *****************************************************************************/
void
POSTFIXELEM_CLEANUP(T_PARSING_MEMORY * memory, T_POSTFIXELEM * elem_)
{
    if ((elem_)->elemtype == SPT_VALUE)
    {
        if (IS_ALLOCATED_TYPE((elem_)->u.value.valuetype))
            sql_value_ptrFree(&((elem_)->u.value));

        if ((elem_)->u.value.valueclass == SVC_VARIABLE)
        {
            sql_mem_freenul(memory, (elem_)->u.value.attrdesc.table.tablename);
            sql_mem_freenul(memory, (elem_)->u.value.attrdesc.table.aliasname);
            sql_mem_freenul(memory, (elem_)->u.value.attrdesc.attrname);
        }

        if ((elem_)->u.value.attrdesc.defvalue.defined &&
                (elem_)->u.value.attrdesc.defvalue.not_null &&
                (IS_ALLOCATED_TYPE((elem_)->u.value.attrdesc.type)))
            sql_mem_freenul(memory, (elem_)->u.value.attrdesc.defvalue.u.ptr);
    }
    else if ((elem_)->elemtype == SPT_OPERATOR)
        PMEM_FREENUL((elem_)->u.op.likeStr);
}

/*****************************************************************************/
//! cleanup_indexfieldinfo 

/*! \breif 
 *
 ************************************
 * \param nest(in) :
 * \param chunk_memory(in) : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *  - cleanup시 parsing_memory의 주소가 인자로 넘어온다.
 *  - 지금은 BYTE TYPE을 INDEX로 사용하는 일이 없지만,
 *      확장을 위해서 MACRO를 변경한다
 *      
 *****************************************************************************/
void
cleanup_indexfieldinfo(T_NESTING * nest, T_PARSING_MEMORY * chunk_memory)
{
    T_EXPRESSIONDESC *expr;
    T_ATTRDESC *attr;
    int i, j;
    T_POSTFIXELEM *elem;

    for (i = 0; i < nest->index_field_num; i++)
    {
        expr = &nest->min[i].expr;
        if (expr == NULL)
            break;

        for (j = 0; j < expr->len; j++)
        {
            elem = expr->list[j];
            if (elem->elemtype == SPT_VALUE)
            {
                elem->u.value.attrdesc.table.tablename = NULL;
                elem->u.value.attrdesc.table.aliasname = NULL;
                elem->u.value.attrdesc.attrname = NULL;
                if (IS_ALLOCATED_TYPE(elem->u.value.valuetype))
                {
                    if (elem->u.value.call_free)
                    {
                        sql_mem_freenul(chunk_memory, elem->u.value.u.ptr);
                        elem->u.value.call_free = 0;
                    }
                }
            }
            sql_mem_set_free_postfixelem(chunk_memory, elem);
            expr->list[j] = NULL;
        }
        sql_mem_set_free_postfixelem_list(chunk_memory, expr->list);
        expr->list = NULL;
        expr->max = 0;
        expr->len = 0;

        expr = &(nest->max[i].expr);
        if (expr == NULL)
            break;

        for (j = 0; j < expr->len; j++)
        {
            elem = expr->list[j];
            if (elem->elemtype == SPT_VALUE)
            {
                elem->u.value.attrdesc.table.tablename = NULL;
                elem->u.value.attrdesc.table.aliasname = NULL;
                elem->u.value.attrdesc.attrname = NULL;
                if (IS_ALLOCATED_TYPE(elem->u.value.valuetype))
                {
                    if (elem->u.value.call_free)
                    {
                        sql_mem_freenul(chunk_memory, elem->u.value.u.ptr);
                        elem->u.value.call_free = 0;
                    }
                }
            }
            sql_mem_set_free_postfixelem(chunk_memory, elem);
            expr->list[j] = NULL;
        }
        sql_mem_set_free_postfixelem_list(chunk_memory, expr->list);
        expr->list = NULL;
        expr->max = 0;
        expr->len = 0;

        attr = &nest->index_field_info[i];
        attr->table.tablename = NULL;
        attr->table.aliasname = NULL;
        attr->attrname = NULL;

        if (attr->defvalue.defined && attr->defvalue.not_null &&
                IS_ALLOCATED_TYPE(attr->type))
            PMEM_FREENUL(attr->defvalue.u.ptr);
    }

    nest->index_field_num = 0;
    nest->op_like = 0;

    sql_mem_freenul(chunk_memory, nest->indexname);
    sql_mem_freenul(chunk_memory, nest->index_field_info);

    nest->min = NULL;
    nest->max = NULL;
}

/*****************************************************************************/
//! free_rec_values 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param chunk_memory(in)      :
 * \param rec_values(in/out)    : 
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *
 *****************************************************************************/
void
free_rec_values(T_PARSING_MEMORY * chunk_memory, FIELDVALUES_T * rec_values)
{
    FIELDVALUE_T *fld_val;
    int i;

    if (rec_values == NULL)
        return;

    for (i = 0; i < rec_values->fld_cnt; i++)
    {
        fld_val = &rec_values->fld_value[i];
        if (IS_ALLOCATED_TYPE(fld_val->type_) || fld_val->type_ == DT_DECIMAL)
            sql_mem_freenul(chunk_memory, fld_val->value_.pointer_);
    }

    sql_mem_freenul(chunk_memory, rec_values->sel_fld_pos);
    sql_mem_freenul(chunk_memory, rec_values->eval_fld_pos);

    sql_mem_freenul(chunk_memory, rec_values->fld_value);
    rec_values->rec_oid = NULL_OID;
    rec_values->fld_cnt = 0;
}

/*****************************************************************************/
//! cleanup_select 

/*! \breif  T_STATEMENT 구조체의 u._select 의 정보를 cleanup 시키기 위한 함수 
 ************************************
 * \param handle(in) :
 * \param select(in) : 
 * \param chunk_memory(in)  :
 ************************************
 * \return  return_value
 ************************************
  * \note 
  *
 *****************************************************************************/
static void
cleanup_select(int handle, T_SELECT * select, T_PARSING_MEMORY * chunk_memory)
{
    T_PLANNEDQUERYDESC *plan;
    T_LIST_SELECTION *selection;
    T_JOINTABLEDESC *jointable;
    T_POSTFIXELEM *elem;
    T_VALUEDESC *val;
    T_ATTRDESC *attr;
    T_EXPRESSIONDESC *expr;
    T_RESULTCOLUMN *res;
    iSQL_ROWS *result_rows;
    CORRELATED_ATTR *cl;

    int i, j;

    if (chunk_memory == NULL)
        chunk_memory = select->main_parsing_chunk;

    if (select->first_sub)
    {
        cleanup_select(handle, select->first_sub, chunk_memory);
        sql_mem_freenul(chunk_memory, select->first_sub);
    }

    if (select->sibling_query)
    {
        cleanup_select(handle, select->sibling_query, chunk_memory);
        sql_mem_freenul(chunk_memory, select->sibling_query);
    }

    if (select->tmp_sub)
    {
        cleanup_select(handle, select->tmp_sub, chunk_memory);
        sql_mem_freenul(chunk_memory, select->tmp_sub);
    }

    select->super_query = NULL;

    plan = &select->planquerydesc;
    for (i = 0; i < plan->querydesc.setlist.len; i++)
    {
        elem = plan->querydesc.setlist.list[i];

        if (elem->elemtype == SPT_SUBQUERY)
        {
            cleanup_select(handle, elem->u.subq, chunk_memory);
            sql_mem_freenul(chunk_memory, elem->u.subq);
        }

        elem->elemtype = SPT_NONE;
    }
    sql_mem_freenul(chunk_memory, plan->querydesc.setlist.list);
    plan->querydesc.setlist.len = 0;
    plan->querydesc.setlist.max = 0;
    plan->querydesc.is_distinct = 0;
    plan->querydesc.querytype = 0;
    selection = &(plan->querydesc.selection);

    result_rows = select->subq_result_rows;
    while (result_rows)
    {
        select->subq_result_rows = result_rows->next;

        if (result_rows->data)
            sql_mem_freenul(chunk_memory, result_rows->data);
        sql_mem_freenul(chunk_memory, result_rows);
        result_rows = select->subq_result_rows;
    }

    cl = plan->querydesc.correl_attr;
    while (cl)
    {
        plan->querydesc.correl_attr = cl->next;
        sql_mem_freenul(chunk_memory, cl);
        cl = plan->querydesc.correl_attr;
    }
    plan->querydesc.correl_attr = NULL;


    for (i = 0; i < selection->len; i++)
    {
        expr = &(selection->list[i].expr);

        if (expr == NULL)
            break;
        EXPRDESC_CLEANUP(chunk_memory, expr);
    }
    selection->len = 0;
    selection->max = 0;

    sql_mem_freenul(chunk_memory, selection->list);

    expr = &plan->querydesc.condition;
    EXPRDESC_CLEANUP(chunk_memory, expr);
    EXPRDESC_LIST_CLEANUP(chunk_memory, plan->querydesc.expr_list);

    for (i = 0; i < plan->querydesc.groupby.len; i++)
    {
        attr = &plan->querydesc.groupby.list[i].attr;
        sql_mem_freenul(chunk_memory, attr->table.tablename);
        sql_mem_freenul(chunk_memory, attr->table.aliasname);
        sql_mem_freenul(chunk_memory, attr->attrname);
        if (attr->defvalue.defined && attr->defvalue.not_null
                && IS_ALLOCATED_TYPE(attr->type))
            sql_mem_freenul(chunk_memory, attr->defvalue.u.ptr);
    }

    if (plan->querydesc.groupby.len)
    {       // having check
        expr = &plan->querydesc.groupby.having;
        for (i = 0; i < expr->len; i++)
        {
            elem = expr->list[i];
            if (elem->elemtype == SPT_VALUE)
            {
                sql_value_ptrFree(&expr->list[i]->u.value);

                if (elem->u.value.valueclass == SVC_VARIABLE)
                {
                    sql_mem_freenul(chunk_memory,
                            elem->u.value.attrdesc.table.tablename);
                    sql_mem_freenul(chunk_memory,
                            elem->u.value.attrdesc.table.aliasname);
                    sql_mem_freenul(chunk_memory,
                            elem->u.value.attrdesc.attrname);
                    if (elem->u.value.attrdesc.defvalue.defined
                            && elem->u.value.attrdesc.defvalue.not_null &&
                            IS_ALLOCATED_TYPE(elem->u.value.attrdesc.type))
                        sql_mem_freenul(chunk_memory,
                                elem->u.value.attrdesc.defvalue.u.ptr);
                }
            }
            sql_mem_set_free_postfixelem(chunk_memory, elem);
        }
        expr->len = 0;
        expr->max = 0;
        sql_mem_set_free_postfixelem_list(chunk_memory, expr->list);
    }

    plan->querydesc.groupby.len = 0;
    plan->querydesc.groupby.max = 0;
    sql_mem_freenul(chunk_memory, plan->querydesc.groupby.list);

    for (i = 0; i < plan->querydesc.orderby.len; i++)
    {
        attr = &plan->querydesc.orderby.list[i].attr;
        sql_mem_freenul(chunk_memory, attr->table.tablename);
        sql_mem_freenul(chunk_memory, attr->table.aliasname);
        sql_mem_freenul(chunk_memory, attr->attrname);

        if (attr->defvalue.defined && attr->defvalue.not_null
                && IS_ALLOCATED_TYPE(attr->type))
            sql_mem_freenul(chunk_memory, attr->defvalue.u.ptr);
    }
    plan->querydesc.orderby.len = 0;
    plan->querydesc.orderby.max = 0;
    sql_mem_freenul(chunk_memory, plan->querydesc.orderby.list);

    select->rows -= plan->querydesc.limit.offset;
    select->param_count = 0;

    plan->querydesc.limit.offset = 0;
    plan->querydesc.limit.rows = 0;

    for (i = 0; i < plan->querydesc.fromlist.len; i++)
    {
        jointable = &plan->querydesc.fromlist.list[i];
        sql_mem_freenul(chunk_memory, jointable->table.tablename);
        sql_mem_freenul(chunk_memory, jointable->table.aliasname);

        sql_mem_freenul(chunk_memory, jointable->scan_hint.indexname);

        if (jointable->scan_hint.fields.list)
        {
            for (j = 0; j < jointable->scan_hint.fields.len; j++)
                sql_mem_freenul(chunk_memory,
                        jointable->scan_hint.fields.list[j].name);
            sql_mem_freenul(chunk_memory, jointable->scan_hint.fields.list);
        }

        sql_mem_freenul(chunk_memory, jointable->fieldinfo);

        jointable->join_type = SJT_NONE;

        expr = &jointable->condition;
        EXPRDESC_CLEANUP(chunk_memory, expr);
        EXPRDESC_LIST_CLEANUP(chunk_memory, jointable->expr_list);

        plan->nest[i].table.tablename = NULL;
        plan->nest[i].table.aliasname = NULL;

        sql_mem_freenul(chunk_memory, plan->nest[i].indexname);

        if (plan->nest[i].scandesc != NULL)
        {
            PMEM_FREENUL(plan->nest[i].scandesc->idxname);
            KeyValue_Delete(&plan->nest[i].scandesc->minkey);
            KeyValue_Delete(&plan->nest[i].scandesc->maxkey);
            KeyValue_Delete((struct KeyValue *) &plan->nest[i].scandesc->
                    filter);
            PMEM_FREENUL(plan->nest[i].scandesc);
        }
        plan->nest[i].keyins_flag = 0;

        sql_mem_freenul(chunk_memory, plan->nest[i].filter);

        plan->nest[i].is_temp_index = 0;
        plan->nest[i].is_subq_orderby_index = 0;
        plan->nest[i].cnt_merge_item = 0;
        cleanup_indexfieldinfo(&plan->nest[i], chunk_memory);

        ATTRED_EXPRDESC_LIST_CLEANUP(chunk_memory,
                plan->nest[i].attr_expr_list);
        free_scan_info(&plan->nest[i]);

        for (j = 0; j < select->rttable[i].cnt_merge_item; j++)
        {
            val = &select->rttable[i].merge_key_values[j];
            if (IS_ALLOCATED_TYPE(val->valuetype))
                sql_mem_freenul(chunk_memory, val->u.ptr);
        }
        select->rttable[i].cnt_merge_item = 0;

        sql_mem_freenul(chunk_memory, select->rttable[i].before_cursor_posi);
        sql_mem_freenul(chunk_memory, select->rttable[i].merge_key_values);

        for (j = 0; j < select->rttable[i].prev_rec_max_cnt; j++)
        {
            free_rec_values(chunk_memory,
                    &select->rttable[i].prev_rec_values[j]);
        }

        sql_mem_freenul(chunk_memory, select->rttable[i].prev_rec_values);
        select->rttable[i].table.tablename = NULL;
        select->rttable[i].table.aliasname = NULL;
        select->rttable[i].Hcursor = 0;
        select->rttable[i].status = 0;
        select->rttable[i].nullflag_offset = 0;
        select->rttable[i].rec_len = 0;
        sql_mem_freenul(chunk_memory, select->rttable[i].rec_buf);
        select->rttable[i].table_ordinal = 0;

        free_rec_values(chunk_memory, select->rttable[i].rec_values);
        sql_mem_freenul(chunk_memory, select->rttable[i].rec_values);


        EXPRDESC_LIST_FREENUL(chunk_memory, select->rttable[i].db_expr_list);
        EXPRDESC_LIST_FREENUL(chunk_memory, select->rttable[i].sql_expr_list);
    }

    sql_mem_freenul(chunk_memory, plan->querydesc.fromlist.list);
    plan->querydesc.fromlist.len = 0;
    plan->querydesc.fromlist.max = 0;
    plan->querydesc.fromlist.outer_join_exists = 0;

    for (i = 0; i < plan->querydesc.distinct.len
            && plan->querydesc.distinct.distinct_table; i++)
    {
        PMEM_FREENUL(plan->querydesc.distinct.distinct_table[i].rec_buf);
        PMEM_FREENUL(plan->querydesc.distinct.distinct_table[i].fieldinfo);
        PMEM_FREENUL(plan->querydesc.distinct.distinct_table[i].table);
        PMEM_FREENUL(plan->querydesc.distinct.distinct_table[i].index);
    }
    PMEM_FREENUL(plan->querydesc.distinct.distinct_table);
    plan->querydesc.distinct.len = 0;

    if (plan->querydesc.having.len > 0)
        PMEM_FREENUL(plan->querydesc.having.hvinfo);
    plan->querydesc.distinct.len = 0;

    if (plan->querydesc.aggr_info.len > 0)
        PMEM_FREENUL(plan->querydesc.aggr_info.aggr);
    plan->querydesc.aggr_info.len = 0;

    for (i = 0; i < select->queryresult.len; i++)
    {
        res = &(select->queryresult.list[i]);

        if (IS_ALLOCATED_TYPE(res->value.valuetype))
            sql_value_ptrFree(&(res->value));
    }

    sql_mem_freenul(chunk_memory, select->queryresult.list);
    select->queryresult.len = 0;

    sql_mem_freenul(chunk_memory, select->wTable);
    select->kindofwTable = 0;
    select->tstatus = TSS_RAW;
}

/*****************************************************************************/
//! cleanup_alter 

/*! \breif T_SQL의 ALTER 구조체에 할당한 memory를 free 시킨다.(메모리 릭을 없애기 위해서)
 *
 ************************************
 * \param handle(in)        :
 * \param alter(in/out)     : 
 * \param chunk_memory(in)  :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - 함수 인자 추가(chunk_memory)
 *  - IS_BS_OR_NBS -> IS_ALLOCATED_BYTE : BYTE 추가
 *  - index rebuild 관련
 *
 *****************************************************************************/
static void
cleanup_alter(int handle, T_ALTER * alter, T_PARSING_MEMORY * chunk_memory)
{
    T_ATTRDESC *column_attr;
    int i;

    if (alter->type == SAT_SEQUENCE)
    {
        sql_mem_freenul(chunk_memory, alter->u.sequence.name);
        return;
    }
    else if (alter->type == SAT_REBUILD)
    {
        for (i = 0; i < alter->u.rebuild_idx.num_object; ++i)
            sql_mem_freenul(chunk_memory, alter->u.rebuild_idx.objectname[i]);
        sql_mem_freenul(chunk_memory, alter->u.rebuild_idx.objectname);
        return;
    }

    sql_mem_freenul(chunk_memory, alter->tablename);
    if (alter->scope == SAT_COLUMN)
    {
        for (i = 0; i < alter->column.attr_list.len; i++)
        {
            column_attr = &alter->column.attr_list.list[i];
            sql_mem_freenul(chunk_memory, column_attr->attrname);
            if (column_attr->defvalue.defined
                    && IS_ALLOCATED_TYPE(column_attr->type))
                sql_mem_freenul(chunk_memory, column_attr->defvalue.u.ptr);
        }
        for (i = 0; i < alter->column.name_list.len; i++)
        {
            sql_mem_freenul(chunk_memory, alter->column.name_list.list[i]);
        }
        sql_mem_freenul(chunk_memory, alter->column.attr_list.list);
        sql_mem_freenul(chunk_memory, alter->column.name_list.list);

    }
    else
    {       /* alter->scope == SAT_CONSTRAINT */
        for (i = 0; i < alter->constraint.list.len; i++)
        {
            sql_mem_freenul(chunk_memory, alter->constraint.list.list[i]);
        }
        sql_mem_freenul(chunk_memory, alter->constraint.list.list);
    }
    sql_mem_freenul(chunk_memory, alter->old_fields);
    sql_mem_freenul(chunk_memory, alter->fielddesc);
    sql_mem_freenul(chunk_memory, alter->real_tablename);
}

/*****************************************************************************/
//!  SQL_cleanup_memory 
/*! \breif  SQL module에서 사용한 메모리를 free 시킨다
 ************************************
 * \param handle(in)    :
 * \param stmt(in/out)  : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *  - SQL MODULE에서 사용하는 잦은 메모리 할당과 해제를 
 *      제거하기 위한 모듈 추가
 *      어차피 아래에서 PARSING으로 들어가는 부분은 없다.
 *      고로.. memory assign을 SKIP해도 무관하나..
 *      memory를 free하는 과정중에서 chunk의 내부인지를 판단하기 위해서
 *      tmp_stmt->parsing_memory에 stmt->parsing_memory를 할당
 *  - truncate 구문 확장
 *  - ADMIN 문장 추가(EXPORT & IMPORT)
 *  
 *****************************************************************************/
void
SQL_cleanup_memory(int handle, T_STATEMENT * stmt)
{
    T_LIST_COLDESC *col_list;
    T_LIST_VALUE *values;
    T_ATTRDESC *attr;
    T_EXPRESSIONDESC *expr;
    int i;
    register T_PARSING_MEMORY *parsingM = stmt->parsing_memory;


    switch (stmt->sqltype)
    {
    case ST_CREATE:
        if (stmt->u._create.type == SCT_TABLE_ELEMENTS)
        {
            sql_mem_freenul(parsingM,
                    stmt->u._create.u.table_elements.table.tablename);
            sql_mem_freenul(parsingM,
                    stmt->u._create.u.table_elements.table.aliasname);
            sql_mem_freenul(parsingM,
                    stmt->u._create.u.table_elements.table.column_name);

            for (i = 0; i < stmt->u._create.u.table_elements.fields.len; i++)
            {
                sql_mem_freenul(parsingM,
                        stmt->u._create.u.table_elements.fields.list[i]);
            }
            sql_mem_freenul(parsingM,
                    stmt->u._create.u.table_elements.fields.list);

            for (i = 0; i < stmt->u._create.u.table_elements.col_list.len; i++)
            {
                attr = &stmt->u._create.u.table_elements.col_list.list[i];
                sql_mem_freenul(parsingM, attr->table.tablename);
                sql_mem_freenul(parsingM, attr->table.aliasname);

                sql_mem_freenul(parsingM, attr->attrname);
                if (attr->defvalue.defined && attr->defvalue.not_null &&
                        IS_BS(attr->type))
                    sql_mem_freenul(parsingM, attr->defvalue.u.ptr);
            }
            sql_mem_freenul(parsingM,
                    stmt->u._create.u.table_elements.col_list.list);
        }
        else if (stmt->u._create.type == SCT_TABLE_QUERY)
        {
            T_SELECT *select;

            sql_mem_freenul(parsingM,
                    stmt->u._create.u.table_query.table.tablename);
            sql_mem_freenul(parsingM,
                    stmt->u._create.u.table_query.table.aliasname);
            sql_mem_freenul(parsingM,
                    stmt->u._create.u.table_query.table.column_name);

            select = &stmt->u._create.u.table_query.query;

            if (stmt->trans_info && stmt->trans_info->opened > 0)
            {
                close_cursor_select(handle, stmt->trans_info, select);
            }

            if (select->wTable)
            {
                dbi_Relation_Drop(handle, select->wTable);
            }

            cleanup_select(handle, select, parsingM);
            sc_memset(&stmt->u._create.u.table_query.query, 0,
                    sizeof(T_SELECT));
        }
        else if (stmt->u._create.type == SCT_INDEX)
        {
            sql_mem_freenul(parsingM, stmt->u._create.u.index.table.tablename);
            sql_mem_freenul(parsingM, stmt->u._create.u.index.table.aliasname);
            sql_mem_freenul(parsingM,
                    stmt->u._create.u.index.table.column_name);

            sql_mem_freenul(parsingM, stmt->u._create.u.index.indexname);

            for (i = 0; i < stmt->u._create.u.index.fields.len; i++)
            {
                sql_mem_freenul(parsingM,
                        stmt->u._create.u.index.fields.list[i].name);
            }
            sql_mem_freenul(parsingM, stmt->u._create.u.index.fields.list);
        }
        else if (stmt->u._create.type == SCT_VIEW)
        {
            sql_mem_freenul(parsingM, stmt->u._create.u.view.name);
            sql_mem_freenul(parsingM, stmt->u._create.u.view.qstring);

            for (i = 0; i < stmt->u._create.u.view.columns.len; i++)
            {
                sql_mem_freenul(parsingM,
                        stmt->u._create.u.view.columns.list[i].attrname);
            }
            sql_mem_freenul(parsingM, stmt->u._create.u.view.columns.list);

            cleanup_select(handle, &(stmt->u._create.u.view.query), parsingM);
        }
        else if (stmt->u._create.type == SCT_SEQUENCE)
        {
            sql_mem_freenul(parsingM, stmt->u._create.u.sequence.name);
        }

        sc_memset(&stmt->u._create, 0, sizeof(T_CREATE));
        break;
    case ST_DELETE:
        if (stmt->u._delete.planquerydesc.querydesc.fromlist.len > 0)
        {
            sql_mem_freenul(parsingM,
                    stmt->u._delete.planquerydesc.querydesc.fromlist.list[0].
                    table.tablename);
            sql_mem_freenul(parsingM,
                    stmt->u._delete.planquerydesc.querydesc.fromlist.list[0].
                    table.aliasname);
            sql_mem_freenul(parsingM,
                    stmt->u._delete.planquerydesc.querydesc.fromlist.list[0].
                    fieldinfo);
            sql_mem_freenul(parsingM,
                    stmt->u._delete.planquerydesc.querydesc.fromlist.list[0].
                    scan_hint.indexname);
            for (i = 0;
                    i <
                    stmt->u._delete.planquerydesc.querydesc.fromlist.list[0].
                    scan_hint.fields.len; i++)
                sql_mem_freenul(parsingM,
                        stmt->u._delete.planquerydesc.querydesc.fromlist.
                        list[0].scan_hint.fields.list[i].name);
            sql_mem_freenul(parsingM,
                    stmt->u._delete.planquerydesc.querydesc.fromlist.list[0].
                    scan_hint.fields.list);
        }

        expr = &(stmt->u._delete.planquerydesc.querydesc.condition);
        EXPRDESC_CLEANUP(parsingM, expr);
        EXPRDESC_LIST_CLEANUP(parsingM,
                stmt->u._delete.planquerydesc.querydesc.expr_list);

        sql_mem_freenul(parsingM,
                stmt->u._delete.planquerydesc.nest->indexname);
        if (stmt->u._delete.planquerydesc.nest->filter)
        {
            sql_mem_freenul(parsingM,
                    stmt->u._delete.planquerydesc.nest->filter);
        }

        cleanup_indexfieldinfo(stmt->u._delete.planquerydesc.nest, parsingM);

        ATTRED_EXPRDESC_LIST_CLEANUP(parsingM,
                stmt->u._delete.planquerydesc.nest->attr_expr_list);
        free_scan_info(stmt->u._update.planquerydesc.nest);

        if (stmt->u._delete.planquerydesc.nest->scandesc != NULL)
        {
            PMEM_FREENUL(stmt->u._delete.planquerydesc.nest->scandesc->
                    idxname);
            KeyValue_Delete(&stmt->u._delete.planquerydesc.nest->scandesc->
                    minkey);
            KeyValue_Delete(&stmt->u._delete.planquerydesc.nest->scandesc->
                    maxkey);
            KeyValue_Delete((struct KeyValue *) &stmt->u._delete.planquerydesc.
                    nest->scandesc->filter);
            PMEM_FREENUL(stmt->u._delete.planquerydesc.nest->scandesc);
        }

        free_rec_values(parsingM, stmt->u._delete.rttable[0].rec_values);
        sql_mem_freenul(parsingM, stmt->u._delete.rttable[0].rec_values);
        sql_mem_freenul(parsingM, stmt->u._delete.rttable[0].rec_buf);

        EXPRDESC_LIST_FREENUL(parsingM,
                stmt->u._delete.rttable[0].db_expr_list);
        EXPRDESC_LIST_FREENUL(parsingM,
                stmt->u._delete.rttable[0].sql_expr_list);

        if (stmt->u._delete.subselect.first_sub)
        {
            cleanup_select(handle, &stmt->u._delete.subselect, parsingM);
        }

        sc_memset(&stmt->u._delete, 0, sizeof(T_DELETE));
        break;
    case ST_DESCRIBE:
        sql_mem_freenul(parsingM, stmt->u._desc.describe_table);
        sql_mem_freenul(parsingM, stmt->u._desc.describe_column);

        sc_memset(&stmt->u._desc, 0, sizeof(T_DESCRIBE));
        break;
    case ST_DROP:
        sql_mem_freenul(parsingM, stmt->u._drop.target);

        sc_memset(&stmt->u._drop, 0, sizeof(T_DROP));
        break;
    case ST_RENAME:
        sql_mem_freenul(parsingM, stmt->u._rename.oldname);
        sql_mem_freenul(parsingM, stmt->u._rename.newname);

        sc_memset(&stmt->u._rename, 0, sizeof(T_RENAME));
        break;
    case ST_ALTER:
        cleanup_alter(handle, &stmt->u._alter, parsingM);

        sc_memset(&stmt->u._alter, 0, sizeof(T_ALTER));
        break;

    case ST_TRUNCATE:
        sql_mem_freenul(parsingM, stmt->u._truncate.objectname);

        sc_memset(&stmt->u._truncate, 0, sizeof(T_TRUNCATE));
        break;
    case ST_ADMIN:
        if (stmt->u._admin.type == SADM_EXPORT_ALL)
        {
            sql_mem_freenul(parsingM, stmt->u._admin.u.export.table_name);
            sql_mem_freenul(parsingM, stmt->u._admin.u.export.export_file);
        }
        else if (stmt->u._admin.type == SADM_EXPORT_ONE)
        {
            sql_mem_freenul(parsingM, stmt->u._admin.u.export.table_name);
            sql_mem_freenul(parsingM, stmt->u._admin.u.export.export_file);
        }
        else if (stmt->u._admin.type == SADM_IMPORT)
        {
            sql_mem_freenul(parsingM, stmt->u._admin.u.import.import_file);
        }

        sc_memset(&stmt->u._admin, 0, sizeof(T_ADMIN));
        break;

    case ST_UPSERT:
    case ST_INSERT:
        sql_mem_freenul(parsingM, stmt->u._insert.fieldinfo);
        col_list = &stmt->u._insert.columns;
        for (i = 0; i < col_list->len; i++)
            sql_mem_freenul(parsingM, col_list->list[i].name);
        sql_mem_freenul(parsingM, col_list->list);
        sql_mem_freenul(parsingM, col_list->ins_cols);

        if (stmt->u._insert.type == SIT_VALUES)
        {
            values = &stmt->u._insert.u.values;
            for (i = 0; i < values->len; i++)
            {
                expr = &values->list[i];
                EXPRDESC_CLEANUP(parsingM, expr);
            }

            sql_mem_freenul(parsingM, values->list);
        }
        else
        {
            T_SELECT *select;

            select = &stmt->u._insert.u.query;

            if (stmt->trans_info && stmt->trans_info->opened > 0)
            {
                close_cursor_select(handle, stmt->trans_info, select);
            }

            if (select->wTable)
            {
                dbi_Relation_Drop(handle, select->wTable);
            }

            cleanup_select(handle, select, parsingM);
            sc_memset(&stmt->u._insert.u.query, 0, sizeof(T_SELECT));
        }

        sql_mem_freenul(parsingM, stmt->u._insert.table.tablename);
        sql_mem_freenul(parsingM, stmt->u._insert.table.aliasname);

        sql_mem_freenul(parsingM, stmt->u._insert.rttable[0].rec_buf);

        sc_memset(&stmt->u._insert, 0, sizeof(T_INSERT));
        stmt->sqltype = ST_NONE;
        break;
    case ST_SELECT:
        cleanup_select(handle, &stmt->u._select, parsingM);
        if (stmt->is_main_stmt)
        {
            values = &stmt->u._select.values;
            for (i = 0; i < values->len; i++)
            {
                expr = &values->list[i];
                EXPRDESC_CLEANUP(parsingM, expr);
            }
            sql_mem_freenul(parsingM, values->list);
        }

        sc_memset(&stmt->u._select, 0, sizeof(T_SELECT));
        break;
    case ST_SET:
        sc_memset(&stmt->u._set, 0, sizeof(T_SET));
        break;
    case ST_UPDATE:
        if (stmt->u._update.planquerydesc.querydesc.fromlist.len > 0)
        {
            sql_mem_freenul(parsingM,
                    stmt->u._update.planquerydesc.querydesc.fromlist.list[0].
                    table.tablename);
            sql_mem_freenul(parsingM,
                    stmt->u._update.planquerydesc.querydesc.fromlist.list[0].
                    table.aliasname);
            sql_mem_freenul(parsingM,
                    stmt->u._update.planquerydesc.querydesc.fromlist.list[0].
                    fieldinfo);
            sql_mem_freenul(parsingM,
                    stmt->u._update.planquerydesc.querydesc.fromlist.list[0].
                    scan_hint.indexname);
            for (i = 0;
                    i <
                    stmt->u._update.planquerydesc.querydesc.fromlist.list[0].
                    scan_hint.fields.len; i++)
            {
                sql_mem_freenul(parsingM,
                        stmt->u._update.planquerydesc.querydesc.fromlist.
                        list[0].scan_hint.fields.list[i].name);
            }
            sql_mem_freenul(parsingM,
                    stmt->u._update.planquerydesc.querydesc.fromlist.list[0].
                    scan_hint.fields.list);
        }

        col_list = &stmt->u._update.columns;
        for (i = 0; i < col_list->len; i++)
        {
            sql_mem_freenul(parsingM, col_list->list[i].name);
        }
        sql_mem_freenul(parsingM, col_list->list);

        values = &stmt->u._update.values;
        for (i = 0; i < values->len; i++)
        {
            expr = &values->list[i];
            EXPRDESC_CLEANUP(parsingM, expr);
        }
        sql_mem_freenul(parsingM, values->list);

        expr = &(stmt->u._update.planquerydesc.querydesc.condition);
        EXPRDESC_CLEANUP(parsingM, expr);
        EXPRDESC_LIST_CLEANUP(parsingM,
                stmt->u._update.planquerydesc.querydesc.expr_list);

        sql_mem_freenul(parsingM,
                stmt->u._update.planquerydesc.nest->indexname);
        if (stmt->u._update.planquerydesc.nest->filter)
        {
            sql_mem_freenul(parsingM,
                    stmt->u._update.planquerydesc.nest->filter);
        }

        cleanup_indexfieldinfo(stmt->u._update.planquerydesc.nest, parsingM);

        ATTRED_EXPRDESC_LIST_CLEANUP(parsingM,
                stmt->u._update.planquerydesc.nest->attr_expr_list);
        free_scan_info(stmt->u._update.planquerydesc.nest);

        if (stmt->u._update.planquerydesc.nest->scandesc != NULL)
        {
            PMEM_FREENUL(stmt->u._update.planquerydesc.nest->scandesc->
                    idxname);
            KeyValue_Delete(&stmt->u._update.planquerydesc.nest->scandesc->
                    minkey);
            KeyValue_Delete(&stmt->u._update.planquerydesc.nest->scandesc->
                    maxkey);
            KeyValue_Delete((struct KeyValue *) &stmt->u._update.planquerydesc.
                    nest->scandesc->filter);
            PMEM_FREENUL(stmt->u._update.planquerydesc.nest->scandesc);
        }

        sql_mem_freenul(parsingM, stmt->u._update.rttable[0].rec_buf);

        EXPRDESC_LIST_FREENUL(parsingM,
                stmt->u._update.rttable[0].db_expr_list);
        EXPRDESC_LIST_FREENUL(parsingM,
                stmt->u._update.rttable[0].sql_expr_list);

        if (stmt->u._update.subselect.first_sub)
        {
            cleanup_select(handle, &stmt->u._update.subselect, parsingM);
        }

        sc_memset(&stmt->u._update, 0, sizeof(T_UPDATE));
        break;
    default:
        break;
    }

    sql_mem_freenul(parsingM, stmt->query);

    if (stmt->is_main_stmt)
        sql_mem_cleanup_chunk(parsingM);
}

void
SQL_cleanup_subselect(int handle, T_SELECT * select,
        T_PARSING_MEMORY * chunk_memory)
{
    cleanup_select(handle, select, chunk_memory);
}

/*****************************************************************************/
//! SQL_trans_start 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)    :
 * \param stmt(in/out)  : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
SQL_trans_start(int handle, T_STATEMENT * stmt)
{
    int ret;

    if (stmt->trans_info)
    {
        ret = dbi_Trans_Begin(handle);

        if (ret < 0 && ret != DB_E_NOTCOMMITTED)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return RET_ERROR;
        }

        if (stmt->trans_info->opened == 0)
        {   /* trans already started by another layer */
            stmt->trans_info->opened = 1;
            stmt->trans_info->when_to_open = -1;
            stmt->trans_info->query_timeout = 0;
        }

        if (stmt->query_timeout)
        {
            ret = dbi_Trans_Set_Query_Timeout(handle, stmt->query_timeout);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                return RET_ERROR;
            }
        }
        else
        {   /* stmt->query_timeout == 0 */
            if (stmt->trans_info->query_timeout != 0)
            {
                ret = dbi_Trans_Set_Query_Timeout(handle, stmt->query_timeout);
                if (ret < 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    return RET_ERROR;
                }
            }
        }
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
int
SQL_trans_commit(int handle, T_STATEMENT * stmt)
{
    int ret;

    if (stmt->sqltype != ST_ROLLBACK)
        if (stmt->trans_info)
        {
            if (stmt->trans_info->opened > 0)
            {
                SQL_close_all_cursors(handle, stmt);
                ret = dbi_Trans_Commit(handle);
                if (ret < 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    return RET_ERROR;
                }
                stmt->trans_info->opened = 0;
                stmt->trans_info->when_to_open = -1;
                stmt->trans_info->query_timeout = 0;
            }
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
int
SQL_trans_rollback(int handle, T_STATEMENT * stmt)
{
    int ret;

    if (stmt->trans_info)
    {
        if (stmt->trans_info->opened > 0)
        {
            SQL_close_all_cursors(handle, stmt);
            ret = dbi_Trans_Abort(handle);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                return RET_ERROR;
            }
            stmt->trans_info->opened = 0;
            stmt->trans_info->when_to_open = -1;
            stmt->trans_info->query_timeout = 0;
        }
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
int
SQL_trans_implicit_savepoint_release(int handle, T_STATEMENT * stmt)
{
    int ret;

    if (stmt->trans_info)
    {
        if (stmt->trans_info->opened > 0)
        {
            ret = dbi_Trans_Implicit_Savepoint_Release(handle);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                return RET_ERROR;
            }
        }
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
int
SQL_trans_implicit_partial_rollback(int handle, T_STATEMENT * stmt)
{
    int ret;

    if (stmt->trans_info)
    {
        if (stmt->trans_info->opened > 0)
        {
            ret = dbi_Trans_Implicit_Partial_Rollback(handle);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                return RET_ERROR;
            }
        }
    }
    return RET_SUCCESS;
}

/*****************************************************************************/
//! SQL_prepare 

/*! \breif  SQL 문장을 PREPARE 시키는 함수
 *          (parsing/normalize/plan이 이루어짐)
 ************************************
 * \param handle(in/out) :  DB Handle
 * \param stmt(in/out) :    Statement
 * \param flag(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
_sql_prepare_error(int handle, T_STATEMENT * stmt, int rollback_flag)
{
    SQL_cleanup_memory(handle, stmt);

    if (rollback_flag & OPT_AUTOCOMMIT)
    {
        if (er_errid() != DB_E_DEADLOCKABORT)
            SQL_trans_rollback(handle, stmt);
    }

    return RET_ERROR;
}

int
SQL_prepare(int *handle, T_STATEMENT * stmt, int flag)
{
    if (SQL_trans_start(*handle, stmt) == RET_ERROR)
        return _sql_prepare_error(*handle, stmt, 0);

    if (sql_parsing(*handle, stmt) == RET_ERROR)
        return _sql_prepare_error(*handle, stmt, flag);

    if (!(flag & OPT_AUTOCOMMIT) && SQL_IS_DDL(stmt->sqltype))
    {
        if (SQL_trans_commit(*handle, stmt) == RET_ERROR)
        {
            return _sql_prepare_error(*handle, stmt, 0);
        }
        if (SQL_trans_start(*handle, stmt) == RET_ERROR)
        {
            return _sql_prepare_error(*handle, stmt, 0);
        }
    }

    if (sql_validating(*handle, stmt) == RET_ERROR)
        return _sql_prepare_error(*handle, stmt, flag);

    if (sql_normalizing(handle, stmt) == RET_ERROR)
        return _sql_prepare_error(*handle, stmt, flag);

    if (sql_planning(*handle, stmt) == RET_ERROR)
        return _sql_prepare_error(*handle, stmt, flag);

    return RET_SUCCESS;
}

/*****************************************************************************/
//! SQL_execute

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)        :
 * \param stmt(in/out)      : 
 * \param flag(in)          :
 * \param result_type(in)   :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
SQL_execute(int *handle, T_STATEMENT * stmt, int flag, int result_type)
{
    int rows;
    int implicit_savepoint_flag = 0;

    if (stmt->sqltype >= ST_COMMIT)
    {
        /* ST_COMMIT, ST_ROLLBACK, ST_SET */
        rows = exec_others(*handle, stmt, result_type);
        goto end;
    }

    /* Is the following code really needed ? */
    if (SQL_trans_start(*handle, stmt) == RET_ERROR)
    {
        rows = RET_ERROR;
        goto end;
    }

    if (stmt->sqltype >= ST_CREATE && stmt->sqltype <= ST_DROP)
    {
        if (!(flag & OPT_AUTOCOMMIT))
        {
            SQL_trans_commit(*handle, stmt);
            SQL_trans_start(*handle, stmt);
        }
        rows = exec_ddl(*handle, stmt);
        if (rows == RET_ERROR)
        {
            if (er_errid() != DB_E_DEADLOCKABORT)
            {
                /* In case of deadlock abort,
                   the current transaction has already been aborted.
                 */
                SQL_trans_rollback(*handle, stmt);
            }
            rows = RET_ERROR;
            goto end;
        }
        SQL_trans_commit(*handle, stmt);

        {
            rows = 0;
            goto end;
        }
    }


    switch (stmt->sqltype)
    {
    case ST_DELETE:
    case ST_INSERT:
    case ST_UPSERT:
    case ST_UPDATE:
        if (!(flag & OPT_AUTOCOMMIT))
        {   /* for statement atomicity */
            dbi_Trans_Implicit_Savepoint(*handle);
            implicit_savepoint_flag = 1;
        }
        /* fallthrough */

    case ST_SELECT:
        rows = exec_dml(*handle, stmt, result_type);

        break;

    case ST_DESCRIBE:
        rows = 0;
        break;

    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
        rows = RET_ERROR;
        break;
    }

    if (rows >= 0)
    {
        if (stmt->sqltype != ST_SELECT && stmt->sqltype != ST_DESCRIBE)
        {
            if (flag & OPT_AUTOCOMMIT)
                SQL_trans_commit(*handle, stmt);
            else
            {
                if (implicit_savepoint_flag)
                {
                    SQL_trans_implicit_savepoint_release(*handle, stmt);
                }
            }
        }
#if defined(MDB_DEBUG)
        /* Execute 성공시 error가 설정되어 있는 경우 assert 발생 */
        if (er_errid() != 0)
        {
            __SYSLOG("Invalid Error(Execute Success) = %d\n", er_errid());
            sc_assert(0, NULL, 0);
        }
#endif
    }
    else
    {
        /* partial rollback may be more accurate. */
        if (er_errid() != DB_E_DEADLOCKABORT)
        {
            /* In case of deadlock abort,
               the current transaction has already been aborted.
             */
            if (flag & OPT_AUTOCOMMIT)
                SQL_trans_rollback(*handle, stmt);
            else
            {
                if (implicit_savepoint_flag)
                    SQL_trans_implicit_partial_rollback(*handle, stmt);
            }
        }
    }

  end:

    return rows;
}

int
SQL_store_result(int handle, T_STATEMENT * stmt, int flag, int result_type)
{
    if (!store_result(handle, stmt, result_type))
        return RET_ERROR;

    if (stmt->result->row_num == 0 || stmt->result->fetch_cnt == 0 ||
            stmt->result->tmp_rec_len == 0)
    {
        if (flag & OPT_AUTOCOMMIT)
            SQL_trans_commit(handle, stmt);
    }

    return stmt->result->row_num;
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
SQL_fetch(int *handle, T_SELECT * select, T_RESULT_INFO * result)
{
#if defined(MDB_DEBUG)
    /* Fetch 성공시 error가 설정되어 있는 경우 assert 발생 */
    int ret;

    ret = fetch_cursor(*handle, select, result);

    if (ret >= 0 && er_errid() != 0)
    {
        __SYSLOG("Invalid Error(Fetch Success) = %d\n", er_errid());
        sc_assert(0, NULL, 0);
    }

    return ret;
#else
    return fetch_cursor(*handle, select, result);
#endif
}

/*****************************************************************************/
//! function_name 

/*! \breif   SQL MODULE에서 할당한 MEMORY를 FREE 시킨다.
 ************************************
 * \param handle(in)        :
 * \param stmt(in)          : 
 * \param commit_mode(in)   :
 * \param free_memory(in)   :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  (subquery에서도 동작함)
 *      
 *****************************************************************************/
int
SQL_cleanup(int *handle, T_STATEMENT * stmt, int commit_mode, int free_memory)
{
    if (stmt->sqltype == ST_SELECT)
    {
        if (stmt->trans_info && stmt->trans_info->opened > 0)
            close_cursor_select(*handle, stmt->trans_info, &stmt->u._select);
        if (stmt->u._select.wTable)
            dbi_Relation_Drop(*handle, stmt->u._select.wTable);
        if (free_memory)
            SQL_cleanup_memory(*handle, stmt);
    }
    else if (stmt->sqltype == ST_UPDATE)
    {
        if (stmt->trans_info && stmt->trans_info->opened > 0 &&
                stmt->u._update.subselect.first_sub)
            close_cursor_select(*handle, stmt->trans_info,
                    &stmt->u._update.subselect);

        if (free_memory)
            SQL_cleanup_memory(*handle, stmt);
    }
    else if (stmt->sqltype == ST_DELETE)
    {
        if (stmt->trans_info && stmt->trans_info->opened > 0 &&
                stmt->u._delete.subselect.first_sub)
            close_cursor_select(*handle, stmt->trans_info,
                    &stmt->u._delete.subselect);

        if (free_memory)
            SQL_cleanup_memory(*handle, stmt);
    }
    /* one CursorOpen, multi CursorInsret */
    else if (stmt->sqltype == ST_INSERT || stmt->sqltype == ST_UPSERT)
    {
        T_RTTABLE *rttable = stmt->u._insert.rttable;

        if (rttable[0].status)
        {
            dbi_Cursor_Close(*handle, rttable[0].Hcursor);
            rttable[0].status = SCS_CLOSE;
        }

        if (free_memory)
            SQL_cleanup_memory(*handle, stmt);
    }
    else if (free_memory)
        SQL_cleanup_memory(*handle, stmt);

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
char *
SQL_get_plan_string(int *handle, T_STATEMENT * stmt)
{
    return get_plan_string(stmt);
}

static void
close_cursor_select(int handle, T_TRANSDESC * trans_info, T_SELECT * select)
{
    T_QUERYDESC *qdesc = NULL;
    T_RTTABLE *rttable = NULL;
    int i;

    if (select->first_sub)
        close_cursor_select(handle, trans_info, select->first_sub);

    if (select->sibling_query)
        close_cursor_select(handle, trans_info, select->sibling_query);

    if (select->tmp_sub)
        close_cursor_select(handle, trans_info, select->tmp_sub);

    qdesc = &select->planquerydesc.querydesc;
    rttable = select->rttable;

    for (i = 0; i < qdesc->setlist.len; i++)
    {
        if (qdesc->setlist.list[i]->elemtype == SPT_SUBQUERY)
        {
            close_cursor_select(handle, trans_info,
                    qdesc->setlist.list[i]->u.subq);
        }
    }

    for (i = 0; i < qdesc->fromlist.len; i++)
    {

        if (rttable[i].status)
        {
            /* some part called this function does not set trans_fino.... */
            dbi_Cursor_Close(handle, rttable[i].Hcursor);

            rttable[i].status = 0;
        }

        if (rttable[i].before_cursor_posi)
            PMEM_FREENUL(rttable[i].before_cursor_posi);

        if (select->planquerydesc.nest[i].indexname &&
                isPartialIndex_name(select->planquerydesc.nest[i].indexname))
            dbi_Index_Drop(handle, select->planquerydesc.nest[i].indexname);

        rttable[i].prev_rec_cnt = 0;
        rttable[i].prev_rec_idx = -1;
        rttable[i].has_prev_cursor_posi = 0;
        rttable[i].has_next_record_read = 0;
    }
    if (qdesc->distinct.len > 0)
    {
        for (i = 0; i < qdesc->distinct.len
                && qdesc->distinct.distinct_table; i++)
        {
            if (qdesc->distinct.distinct_table[i].Htable < 0)
                continue;

            PMEM_FREENUL(qdesc->distinct.distinct_table[i].rec_buf);
            PMEM_FREENUL(qdesc->distinct.distinct_table[i].fieldinfo);
            dbi_Cursor_Close(handle, qdesc->distinct.distinct_table[i].Htable);
            dbi_Relation_Drop(handle, qdesc->distinct.distinct_table[i].table);
            qdesc->distinct.distinct_table[i].Htable = -1;
        }
    }
}

void
SQL_close_all_cursors(int handle, T_STATEMENT * stmt)
{

    switch (stmt->sqltype)
    {
    case ST_SELECT:
        close_cursor_select(handle, stmt->trans_info, &stmt->u._select);
        break;
    case ST_UPSERT:
    case ST_INSERT:
        {
            T_RTTABLE *rttable = stmt->u._insert.rttable;

            if (rttable[0].status)
            {
                dbi_Cursor_Close(handle, rttable[0].Hcursor);
                rttable[0].status = 0;
            }

            if (stmt->u._insert.type == SIT_QUERY)
            {
                T_SELECT *query = &stmt->u._insert.u.query;
                T_QUERYDESC *qdesc = &query->planquerydesc.querydesc;
                int i;

                rttable = query->rttable;

                for (i = 0; i < qdesc->fromlist.len; i++)
                {
                    if (rttable[i].status)
                    {
                        dbi_Cursor_Close(handle, rttable[i].Hcursor);
                        rttable[i].status = 0;
                    }
                }
            }
        }
        break;
    case ST_UPDATE:
        {
            T_RTTABLE *rttable = stmt->u._update.rttable;

            if (rttable[0].status)
            {
                dbi_Cursor_Close(handle, rttable[0].Hcursor);
                rttable[0].status = 0;
            }
        }
        break;
    case ST_DELETE:
        {
            T_RTTABLE *rttable = stmt->u._delete.rttable;

            if (rttable[0].status)
            {
                dbi_Cursor_Close(handle, rttable[0].Hcursor);
                rttable[0].status = 0;
            }
        }
        break;
    case ST_CREATE:
        if (stmt->u._create.type == SCT_TABLE_QUERY)
        {
            T_SELECT *query = &stmt->u._create.u.table_query.query;
            T_QUERYDESC *qdesc = &query->planquerydesc.querydesc;
            T_RTTABLE *rttable = query->rttable;
            int i;

            for (i = 0; i < qdesc->fromlist.len; i++)
            {
                if (rttable[i].status)
                {
                    dbi_Cursor_Close(handle, rttable[i].Hcursor);
                    rttable[i].status = 0;
                }
            }
        }
        break;
    default:
        break;
    }
}

int
SQL_destroy_ResultSet(iSQL_ROWS * result_rows)
{
    iSQL_ROWS *curr_row = NULL;

    while ((curr_row = result_rows) != NULL)
    {
        result_rows = result_rows->next;

        PMEM_FREENUL(curr_row->data);
        PMEM_FREENUL(curr_row);
    }

    return RET_SUCCESS;
}
