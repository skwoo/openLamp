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

#include "sql_plan.h"
#include "sql_execute.h"
#include "sql_util.h"

#include "mdb_er.h"
#include "ErrorCode.h"

#include "mdb_syslog.h"

#define IS_EQUAL_TABLE(a_, b_) \
    (((a_)->aliasname == NULL && (b_)->aliasname == NULL && \
              !mdb_strcmp((a_)->tablename, (b_)->tablename)) || \
      ((a_)->aliasname && (b_)->aliasname && \
            !mdb_strcmp((a_)->aliasname, (b_)->aliasname)))

#define IS_SSCAN(iscan)         \
    (((iscan) == NULL) ||       \
     ((iscan)->match_ifield_count == 0 && (iscan)->chosen_by_hint_flag == 0))
#define IS_SSCAN_FIXED(iscan)   \
    (((iscan) == NULL) ||       \
     ((iscan)->match_ifield_count == 0 && (iscan)->chosen_by_hint_flag == 0 \
      && (iscan)->recompute_cost_flag == 0))
#define IS_ISCAN(iscan)         \
    ((iscan) != NULL &&         \
     ((iscan)->match_ifield_count > 0 || (iscan)->chosen_by_hint_flag == 1))

#define ISCAN_USE_ALLFLD_EQUAL(iscan)   \
    ((iscan)->match_ifield_count == (iscan)->indexinfo.numFields && \
     (iscan)->index_equal_op_flag)
#define ISCAN_USE_UNIQUE_INDEX(iscan)   ((iscan)->indexinfo.type & 0x80)

extern void append_expression_string(_T_STRING * sbuf,
        T_EXPRESSIONDESC * expr);
extern int create_temp_index(int handle, T_TABLEDESC * table,
        T_IDX_LIST_FIELDS * indexfields, char *indexname, int keyins_flag,
        void *scandesc);
extern int drop_temp_index(int handle, char *index);

extern void cleanup_indexfieldinfo(T_NESTING * nest,
        T_PARSING_MEMORY * chunk_memory);
extern int make_filter_for_scandesc(T_NESTING * nest, SCANDESC_T * scandesc);
extern int make_keyrange_for_scandesc(T_NESTING * nest, SCANDESC_T * scandesc);

static int build_iscan_list(int connid, T_NESTING * nest,
        int index_count, SYSINDEX_T * indexes, int chosen_by_hint);

static int make_scan_info(int connid, T_NESTING * nest,
        T_JOINTABLEDESC * join);
static void compute_iscan_cost(T_NESTING * nest);
static int set_indexrange(int connid, T_NESTING * nest,
        int numfields, SYSFIELD_T * fieldinfo);
void free_scan_info(T_NESTING * nest);

int set_rec_values_info_select(int handle, T_SELECT * select);

int set_scanrid_in_nest(T_NESTING * nest, T_EXPRESSIONDESC * condition);

//////////////////////// Util Functions Start ////////////////////////
/*****************************************************************************/
//! make_valueexpr

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param dest(in/out)  :
 * \param src(in/out)   :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 자료구조 변경
 *
 *****************************************************************************/
static int
make_valueexpr(T_EXPRESSIONDESC * dest, T_EXPRESSIONDESC * src)
{
    int i, len;

    dest->max = src->len;
    dest->list = sql_mem_get_free_postfixelem_list(NULL, dest->max);
    if (dest->list == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return SQL_E_OUTOFMEMORY;
    }

    for (i = 0; i < src->len; ++i)
    {
        dest->list[i] = sql_mem_get_free_postfixelem(NULL);
        if (dest->list[i] == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return SQL_E_OUTOFMEMORY;
        }
        sc_memset(dest->list[i], 0x00, sizeof(T_POSTFIXELEM));
    }

    for (i = 0; i < src->len; i++)
    {
        if (src->list[i]->elemtype == SPT_VALUE)
        {

            DataType _type = src->list[i]->u.value.valuetype;

            dest->list[i]->m_name = src->list[i]->m_name;
            dest->list[i]->elemtype = src->list[i]->elemtype;
            dest->list[i]->u.value.valueclass =
                    src->list[i]->u.value.valueclass;
            dest->list[i]->u.value.valuetype = src->list[i]->u.value.valuetype;
            dest->list[i]->u.value.param_idx = src->list[i]->u.value.param_idx;

            if (src->list[i]->u.value.is_null)
            {
                dest->list[i]->u.value.is_null = src->list[i]->u.value.is_null;
            }
            else
                switch (_type)
                {
                case DT_VARCHAR:
                case DT_CHAR:
                case DT_NVARCHAR:
                case DT_NCHAR:
                    sc_memcpy(&dest->list[i]->u.value.attrdesc,
                            &src->list[i]->u.value.attrdesc,
                            sizeof(T_ATTRDESC));

                    if (src->list[i]->u.value.u.ptr == NULL)
                        break;
                    len = strlen_func[_type] (src->list[i]->u.value.u.ptr) + 1;

                    if (sql_value_ptrAlloc(&dest->list[i]->u.value, len) < 0)
                        return RET_ERROR;

                    strncpy_func[_type] (dest->list[i]->u.value.u.ptr,
                            src->list[i]->u.value.u.ptr, len);
                    break;
                default:
                    sc_memcpy(&(dest->list[i]->u.value),
                            &(src->list[i]->u.value), sizeof(T_VALUEDESC));
                    break;
                }

            dest->list[i]->u.value.attrdesc.table.tablename
                    = src->list[i]->u.value.attrdesc.table.tablename;
            dest->list[i]->u.value.attrdesc.table.aliasname
                    = src->list[i]->u.value.attrdesc.table.aliasname;
            dest->list[i]->u.value.attrdesc.attrname
                    = src->list[i]->u.value.attrdesc.attrname;
        }
        else if (src->list[i]->elemtype == SPT_OPERATOR)
        {
            dest->list[i]->elemtype = SPT_OPERATOR;
            dest->list[i]->u.op = src->list[i]->u.op;
        }
        else if (src->list[i]->elemtype == SPT_FUNCTION)
        {
            dest->list[i]->elemtype = SPT_FUNCTION;
            dest->list[i]->u.func = src->list[i]->u.func;
        }
        else if (src->list[i]->elemtype == SPT_AGGREGATION)
        {
            dest->list[i]->elemtype = SPT_AGGREGATION;
            dest->list[i]->u.aggr = src->list[i]->u.aggr;
        }
    }
    dest->len = src->len;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! initialize_nest

/*! \breif  table에 대한 접근 정보를 초기화 하는 부분(nest 정보)
 ************************************
 * \param nest(in/out) : TABLE에 접근하기 위한 정보
 * \param table(in) : table에 실제 정보 정보
 ************************************
 * \return  void
 ************************************
 * \note
 *      SQL Module 중 PLAN 정보를 만드는 부분에 해당함
 *
 *****************************************************************************/
static void
initialize_nest(T_NESTING * nest, T_TABLEDESC * table)
{
    if (table)
    {
        nest->table.tablename = table->tablename;
        nest->table.aliasname = table->aliasname;
        nest->table.correlated_tbl = table->correlated_tbl;
    }
    else
    {
        nest->table.tablename = NULL;
        nest->table.aliasname = NULL;
        nest->table.correlated_tbl = NULL;
    }
    nest->indexname = NULL;
    nest->is_temp_index = 0;
    nest->is_subq_orderby_index = 0;
    nest->index_field_info = NULL;
    nest->index_field_num = 0;
    nest->min = NULL;
    nest->max = NULL;
    nest->filter = NULL;
    nest->cnt_merge_item = 0;
    nest->cnt_single_item = -1;
    nest->attr_expr_list = NULL;
    nest->scanrid = NULL_OID;
    nest->ridin_cnt = 0;
    nest->ridin_row = 0;
    nest->is_param_scanrid = 0;
}

/*****************************************************************************/
//! set_indexinfo_for_nest

/*! \breif  T_NESTING 구조체 안쪽의 index 관련 정보를 설정
 ************************************
 * \param connid(in)    : connection id
 * \param nest(in/out)  : nesting info
 * \param index(in)     : index info
 * \param istemp(in)    :
 * \param numfields(in) : table's field number
 * \param fieldinfo(in) : table's field info
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - called : set_indexrange_for_select()
 *  - 정보를 설정하는 부분을 함수로 대체
 *  - index에 collation 정보가 있으면.. 고 넘으로 넣을것..
 *****************************************************************************/
static int
set_indexinfo_for_nest(int connid, T_NESTING * nest,
        T_INDEXINFO * index, int istemp, int numfields, SYSFIELD_T * fieldinfo)
{
    T_ATTRDESC *index_field;
    int i, j;
    T_KEY_EXPRDESC *temp;
    int ret;

    nest->indexname = PMEM_STRDUP(index->info.indexName);
    if (!nest->indexname)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return SQL_E_OUTOFMEMORY;
    }
    nest->index_field_num = index->info.numFields;
    nest->is_temp_index = istemp;

    nest->index_field_info = (T_ATTRDESC *) PMEM_ALLOC(index->info.numFields *
            (sizeof(T_ATTRDESC) + (2 * sizeof(T_KEY_EXPRDESC))));
    if (nest->index_field_info == NULL)
    {
        PMEM_FREENUL(nest->indexname);
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return SQL_E_OUTOFMEMORY;
    }
    temp = (T_KEY_EXPRDESC *) & nest->index_field_info[index->info.numFields];
    nest->min = &temp[0];
    nest->max = &temp[index->info.numFields];

    for (i = 0; i < index->info.numFields; i++)
    {
        index_field = &nest->index_field_info[i];
        index_field->table.tablename = nest->table.tablename;
        index_field->table.aliasname = nest->table.aliasname;
        index_field->table.correlated_tbl = NULL;
        index_field->Hattr = index->fields[i].fieldId;
        for (j = 0; j < numfields; j++)
        {
            if (index_field->Hattr == fieldinfo[j].fieldId)
                break;
        }
        if (j == numfields)
        {
            PMEM_FREENUL(nest->indexname);
            PMEM_FREENUL(nest->index_field_info);
            return RET_ERROR;
        }

        index_field->attrname = fieldinfo[j].fieldName;

        index_field->posi.ordinal = fieldinfo[j].position;
        index_field->posi.offset = fieldinfo[j].offset;
        index_field->flag = fieldinfo[j].flag;
        index_field->type = fieldinfo[j].type;
        index_field->len = fieldinfo[j].length;
        index_field->fixedlen = fieldinfo[j].fixed_part;
        if (index_field->type == DT_NVARCHAR)
            index_field->fixedlen /= WCHAR_SIZE;
        index_field->dec = fieldinfo[j].scale;
        index_field->Htable = fieldinfo[j].tableId;
        index_field->collation = index->fields[i].collation;
        index_field->order = index->fields[i].order;
        ret = get_defaultvalue(index_field, &fieldinfo[j]);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return RET_ERROR;
        }

        nest->min[i].expr.len = 0;
        nest->min[i].expr.max = 0;
        nest->min[i].expr.list = NULL;
        nest->max[i].expr.len = 0;
        nest->max[i].expr.max = 0;
        nest->max[i].expr.list = NULL;
    }
    return RET_SUCCESS;
}

/*****************************************************************************/
//!set_lu_expr

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param expr(in/out)  :
 * \param kc_attr(in) :
 * \param type(in)    :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *  - expr의 T_POSTFIXELEM 공간을 이 안에서 할당 받는다
 *
 *****************************************************************************/
static int
set_lu_expr(T_EXPRESSIONDESC * expr, T_ATTRDESC * kc_attr, T_BOUND type)
{
    T_POSTFIXELEM *elem;
    T_ATTRDESC *elem_attr;

    expr->len = 1;
    expr->max = 1;

    expr->list = sql_mem_get_free_postfixelem_list(NULL, expr->max);
    if (expr->list == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return SQL_E_OUTOFMEMORY;
    }

    expr->list[0] = sql_mem_get_free_postfixelem(NULL);
    if (expr->list[0] == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return SQL_E_OUTOFMEMORY;
    }
    sc_memset(expr->list[0], 0x00, sizeof(T_POSTFIXELEM));

    elem = expr->list[0];
    elem_attr = &elem->u.value.attrdesc;

    elem->elemtype = SPT_VALUE;
    sc_memcpy(elem_attr, kc_attr, sizeof(T_ATTRDESC));
    elem_attr->table.tablename = NULL;
    elem_attr->table.aliasname = NULL;
    elem_attr->attrname = NULL;

    elem->u.value.valuetype = elem_attr->type;

    if (IS_ALLOCATED_TYPE(kc_attr->type))
    {
        if (sql_value_ptrAlloc(&elem->u.value, elem_attr->len + 1) < 0)
            return SQL_E_OUTOFMEMORY;
    }
    elem->u.value.is_null = 0;
    switch (type)
    {
    case UPPER_BOUND:
        set_maxvalue(elem_attr->type, elem_attr->len, &elem->u.value);
        break;
    case LOWER_BOUND:
        set_minvalue(elem_attr->type, elem_attr->len, &elem->u.value);
        break;
    default:
        break;
    }
    return RET_SUCCESS;
}

/*****************************************************************************/
//!make_lu_limit

/*! \breif  T_NESTING의 min/max 값을 설정한다.
 ************************************
 * \param connid(in) :
 * \param fieldnum(in) :
 * \param nest(in/out) :
 * \param is_like(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *
 *  - T_EXPRESSIONDESC 구조체 변경
 *  - valexpr에 (T_POSTFIXELEM**)을 할당 해줘야 하는데...
 *
 *****************************************************************************/
static int
make_lu_limit(int connid, int fieldnum, T_NESTING * nest, ibool * is_like)
{
    T_ATTRDESC *index_field;
    T_OR_EXPRDESC *min_orexpr = NULL;
    T_OR_EXPRDESC *max_orexpr = NULL;
    T_OR_EXPRDESC *orexpr;
    T_KEY_EXPRDESC *min, *max;
    T_EXPRESSIONDESC *expr, valexpr;
    T_POSTFIXELEM *kc, *op;
    T_OPTYPE op_type;
    T_ORTYPE *type;
    int *cnt_merge_item;
    int *cnt_single_item;

    valexpr.list = NULL;

    index_field = &nest->index_field_info[fieldnum];
    min = &nest->min[fieldnum];
    max = &nest->max[fieldnum];

    cnt_merge_item = (nest->cnt_merge_item == fieldnum ?
            &nest->cnt_merge_item : NULL);
    cnt_single_item = (nest->cnt_single_item == fieldnum ?
            &nest->cnt_single_item : NULL);

    if (nest->indexable_orexpr_count > 0)
    {
        int i;

        for (i = 0; i < nest->indexable_orexpr_count; i++)
        {
            orexpr = nest->indexable_orexprs[i];
            if (!(orexpr->type & ORT_INDEXABLE))
                continue;

            expr = orexpr->expr;
            if (orexpr->type & ORT_REVERSED)
            {
                kc = expr->list[expr->len - 2];
                op = expr->list[expr->len - 1];
                op_type = get_commute_op(op->u.op.type);
            }
            else
            {
                kc = expr->list[0];
                op = expr->list[expr->len - 1];
                op_type = op->u.op.type;
            }

            if (kc->u.value.attrdesc.Hattr != index_field->Hattr)
                continue;

            switch (op_type)
            {
            case SOT_EQ:
                min_orexpr = orexpr;
                max_orexpr = orexpr;
                if (cnt_merge_item && (orexpr->type & ORT_JOINABLE))
                    (*cnt_merge_item)++;
                if (cnt_single_item && (orexpr->type & ORT_SCANNABLE))
                    (*cnt_single_item)++;
                break;
            case SOT_GT:
            case SOT_GE:
                min_orexpr = orexpr;
                break;
            case SOT_LT:
            case SOT_LE:
                max_orexpr = orexpr;
                break;
            case SOT_ISNULL:
            case SOT_ISNOTNULL:
                if (min_orexpr || max_orexpr)
                    continue;
                min_orexpr = orexpr;
                max_orexpr = orexpr;
                break;
            case SOT_LIKE:
                if (min_orexpr || max_orexpr)
                    continue;
                min_orexpr = orexpr;
                max_orexpr = orexpr;
                *is_like = 1;
                break;
            case SOT_IN:
                min_orexpr = orexpr;
                max_orexpr = orexpr;
                break;
            default:
                break;
            }
            if (min_orexpr && max_orexpr)
                break;
        }
    }
    else    /* nest->indexable_orexpr_count == 0 */
    {
        ATTRED_EXPRDESC_LIST *cur;

        for (cur = nest->attr_expr_list; cur; cur = cur->next)
        {
            orexpr = (T_OR_EXPRDESC *) cur->ptr;
            if (!(orexpr->type & ORT_INDEXABLE))
                continue;

            expr = orexpr->expr;
            if (orexpr->type & ORT_REVERSED)
            {
                kc = expr->list[expr->len - 2];
                op = expr->list[expr->len - 1];
                op_type = get_commute_op(op->u.op.type);
            }
            else
            {
                kc = expr->list[0];
                op = expr->list[expr->len - 1];
                op_type = op->u.op.type;
            }

            if (kc->u.value.attrdesc.Hattr != index_field->Hattr)
                continue;

            switch (op_type)
            {
            case SOT_EQ:
                min_orexpr = orexpr;
                max_orexpr = orexpr;
                if (cnt_merge_item && (orexpr->type & ORT_JOINABLE))
                    (*cnt_merge_item)++;
                if (cnt_single_item && (orexpr->type & ORT_SCANNABLE))
                    (*cnt_single_item)++;
                break;
            case SOT_GT:
            case SOT_GE:
                min_orexpr = orexpr;
                break;
            case SOT_LT:
            case SOT_LE:
                max_orexpr = orexpr;
                break;
            case SOT_ISNULL:
            case SOT_ISNOTNULL:
                if (min_orexpr || max_orexpr)
                    continue;
                min_orexpr = orexpr;
                max_orexpr = orexpr;
                break;
            case SOT_LIKE:
                if (min_orexpr || max_orexpr)
                    continue;
                min_orexpr = orexpr;
                max_orexpr = orexpr;
                *is_like = 1;
                break;
            case SOT_IN:
                min_orexpr = orexpr;
                max_orexpr = orexpr;
                break;
            default:
                break;
            }
            if (min_orexpr && max_orexpr)
                break;
        }
    }

    if (min_orexpr == NULL && max_orexpr == NULL)
        return RET_END;

    if (min_orexpr == NULL)
    {
        if (set_lu_expr(&min->expr, index_field, LOWER_BOUND) != RET_SUCCESS)
        {
            return RET_ERROR;
        }
    }
    else
    {
        expr = min_orexpr->expr;
        type = &min_orexpr->type;
        op = expr->list[expr->len - 1];
        op_type = ((*type & ORT_REVERSED) ?
                get_commute_op(op->u.op.type) : op->u.op.type);
        switch (op_type)
        {
        case SOT_ISNULL:
        case SOT_ISNOTNULL:
            valexpr.max = 1;
            valexpr.len = 1;
            valexpr.list = &(expr->list[expr->len - 1]);

            if (make_valueexpr(&min->expr, &valexpr) != RET_SUCCESS)
            {
                return RET_ERROR;
            }
            break;

        default:
            valexpr.max = expr->max - 2;
            valexpr.len = expr->len - 2;
            if (*type & ORT_REVERSED)
                valexpr.list = &(expr->list[0]);
            else
                valexpr.list = &(expr->list[1]);
            if (make_valueexpr(&min->expr, &valexpr) != RET_SUCCESS)
            {
                return RET_ERROR;
            }
            break;
        }
        min->type = op_type;
        /* 기존 SOT_GT인 경우 index range에 포함되지 않았으나
           SOT_GT 역시 포함되어 조건 변경 */
        if (op_type != SOT_LIKE)
        {
            *type |= ORT_INDEXRANGED;
        }
    }
    if (max_orexpr == NULL)
    {
        if (set_lu_expr(&max->expr, index_field, UPPER_BOUND) != RET_SUCCESS)
        {
            return RET_ERROR;
        }
    }
    else
    {
        expr = max_orexpr->expr;
        type = &max_orexpr->type;
        op = expr->list[expr->len - 1];

        op_type = ((*type & ORT_REVERSED) ?
                get_commute_op(op->u.op.type) : op->u.op.type);
        switch (op_type)
        {
        case SOT_ISNULL:
        case SOT_ISNOTNULL:
            valexpr.max = 1;
            valexpr.len = 1;
            valexpr.list = &(expr->list[expr->len - 1]);

            if (make_valueexpr(&max->expr, &valexpr) != RET_SUCCESS)
            {
                return RET_ERROR;
            }
            break;
        default:
            valexpr.max = expr->max - 2;
            valexpr.len = expr->len - 2;
            if (*type & ORT_REVERSED)
                valexpr.list = &(expr->list[0]);
            else
                valexpr.list = &(expr->list[1]);

            if (make_valueexpr(&max->expr, &valexpr) != RET_SUCCESS)
            {
                return RET_ERROR;
            }
            break;
        }

        max->type = op_type;
        /* 기존 SOT_GT인 경우 index range에 포함되지 않았으나
           SOT_GT 역시 포함되어 조건 변경 */
        if (op_type != SOT_LIKE)
        {
            *type |= ORT_INDEXRANGED;
        }

        if (op_type == SOT_IN)
        {
            nest->in_exprs = expr->len - 2;
            nest->op_in = TRUE;
        }
        else
        {
            nest->op_in = FALSE;
        }
    }

    return RET_SUCCESS;
}

#define INC_ATTR_COUNT 8

/*****************************************************************************/
//! check_predicate_type

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param expr(in/out)  :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *
 * we check the followings
 *   1. predicate is "col op vexpr"
 *   2. columns in "vexpr" are all bindable at runtime, i.e
 *      there are no table of "col" among tables refered by "vexpr"
 *   3. type of predicate, i.e scan or join ?
 *
 *  - T_EXPRESSIONDESC 구조체 변경
 *  - tmp를 ALLOCTE
 *
 *****************************************************************************/
static T_PREDTYPE
check_predicate_type(T_EXPRESSIONDESC * expr)
{
    T_POSTFIXELEM *col, *op;
    T_POSTFIXELEM *elem;
    static T_POSTFIXELEM tmp;
    T_TABLEDESC *col_table, *elem_table;
    t_stack_template *stack;
    int n_scan_columns;
    int n_join_columns;
    int i, j;

    /* "expr" is a predicate? */
    op = expr->list[expr->len - 1];

    if (is_logicalop(op->u.op.type))
        return PRED_NONE;

    /* "expr" is "col op vexpr" or "vexpr op col"? */
    tmp.elemtype = SPT_VALUE;
    tmp.u.value.valueclass = SVC_CONSTANT;
    tmp.u.value.valuetype = DT_BOOL;
    tmp.u.value.u.bl = 1;

    init_stack_template(&stack);
    for (i = 0; i < expr->len - 1; i++)
    {
        elem = expr->list[i];

        switch (elem->elemtype)
        {
        case SPT_VALUE:
            push_stack_template(elem, &stack);
            break;
        case SPT_OPERATOR:
            for (j = 0; j < elem->u.op.argcnt; j++)
                (void) pop_stack_template(&stack);
            push_stack_template(&tmp, &stack);
            break;
        case SPT_FUNCTION:
            for (j = 0; j < elem->u.func.argcnt; j++)
                (void) pop_stack_template(&stack);
            push_stack_template(&tmp, &stack);
            break;
        case SPT_AGGREGATION:
        case SPT_SUBQUERY:
        case SPT_SUBQ_OP:
            remove_stack_template(&stack);
            return PRED_NONE;
        default:
            remove_stack_template(&stack);
            return PRED_NONE;
        }
    }
    for (i = 0, col = NULL; i < op->u.op.argcnt; i++)
    {
        elem = pop_stack_template(&stack);
        if (!elem)
        {
            remove_stack_template(&stack);
            return PRED_NONE;
        }
        if (elem->elemtype == SPT_VALUE &&
                elem->u.value.valueclass == SVC_VARIABLE)
        {
            col = elem;
        }
    }
    if (!is_emptystack_template(&stack) || col == NULL)
    {
        remove_stack_template(&stack);
        return PRED_NONE;
    }

    /* which type of columns in "vexpr"? */
    n_scan_columns = 0;
    n_join_columns = 0;
    col_table = &col->u.value.attrdesc.table;
    for (i = 0; i < expr->len - 1; i++)
    {
        elem = expr->list[i];

        if (elem == col)
            continue;
        if (elem->elemtype == SPT_VALUE &&
                elem->u.value.valueclass == SVC_VARIABLE)
        {
            elem_table = &elem->u.value.attrdesc.table;
            if (IS_EQUAL_TABLE(elem_table, col_table))
                n_scan_columns++;
            else
                n_join_columns++;
        }
    }
    if (n_scan_columns > 0)
    {
        remove_stack_template(&stack);
        return PRED_NONE;
    }

    /* normalize "expr" like "col vexpr op" */
    if (col != expr->list[0])
    {
        sc_memcpy(&tmp, col, sizeof(T_POSTFIXELEM));

        // 혹시 몰라서..
        for (i = (expr->len - 2); i > 0; --i)
            sc_memcpy(expr->list[i], expr->list[i - 1], sizeof(T_POSTFIXELEM));

        sc_memcpy(expr->list[0], &tmp, sizeof(T_POSTFIXELEM));
        op->u.op.type = get_commute_op(op->u.op.type);
    }

    remove_stack_template(&stack);
    if (n_join_columns == 0)
        return PRED_SCAN;
    else if (n_join_columns == 1)
        return PRED_JOIN;
    else
        return PRED_MULTIJOIN;
}


#define IS_LIKESPECIAL(_likechar) \
                    ((_likechar) == '\\' || (_likechar) == '%' ||(_likechar) == '_')
static int
_use_reverse_index(T_EXPRESSIONDESC * expr, T_SCANHINT * scan_hint)
{
    int index_count;
    SYSINDEX_T *indexes = NULL;

    SYSINDEX_T index_info;
    SYSINDEXFIELD_T *idxflds_info = NULL;

    SYSTABLE_T tableinfo;
    SYSFIELD_T *fieldinfo = NULL;

    T_POSTFIXELEM *valleft = expr->list[0];

    char *relationName = NULL;
    char *indexName = NULL;
    int i, j, k;

    if (expr->len != 3 || scan_hint == NULL)
        return DB_FALSE;

    if (valleft->u.value.valueclass != SVC_VARIABLE)
        return DB_FALSE;

    relationName = valleft->u.value.attrdesc.table.tablename;
    if (relationName == NULL)
        relationName = valleft->u.value.attrdesc.table.aliasname;

    if (relationName == NULL)
        return DB_FALSE;

    if (scan_hint->scan_type != SMT_ISCAN)
        return DB_FALSE;

    /* <1> index name is given */
    if (scan_hint->indexname)
    {
        indexName = scan_hint->indexname;
        goto FOUND_INDEX;
    }

    /* get all indexes information */
    index_count = dbi_Schema_GetTableIndexesInfo(-1, relationName,
            SCHEMA_ALL_INDEX, &indexes);
    if (index_count < 0)
        return DB_FALSE;

    /* <2> primary index */
    if (scan_hint->fields.len == 0)
    {
        for (i = 0; i < index_count; i++)
        {
            if (isPrimaryIndex_name(indexes[i].indexName))
            {   /* primary index found */
                indexName = indexes[i].indexName;
                goto FOUND_INDEX;
            }
        }

        goto NOT_FOUND_INDEX;
    }
    else    /* <3> index columns are given */
    {       /* scan_hint->indexname == NULL && scan_hint->fields.len > 0 */
        if (dbi_Schema_GetTableFieldsInfo(-1, relationName,
                        &tableinfo, &fieldinfo) < 0)
        {
            goto NOT_FOUND_INDEX;
        }

        for (i = 0; i < index_count; i++)
        {
            if (indexes[i].numFields < scan_hint->fields.len)
                continue;       /* # of index fields Not match */
            if (dbi_Schema_GetIndexInfo(-1, indexes[i].indexName,
                            NULL, &idxflds_info, NULL) < 0)
                continue;

            for (j = 0; j < scan_hint->fields.len; j++)
            {
                for (k = 0; k < tableinfo.numFields; k++)
                {
                    if (idxflds_info[j].fieldId == fieldinfo->fieldId)
                        break;
                }
                if (k >= tableinfo.numFields)   /* FieldID Mismatch.." */
                    goto NOT_FOUND_INDEX;
                /* found field position : k */
                if (mdb_strcmp(fieldinfo[k].fieldName,
                                scan_hint->fields.list[j].name))
                    break;      /* different field */
            }

            PMEM_FREENUL(idxflds_info);

            if (j == scan_hint->fields.len)     /* index found */
            {
                indexName = indexes[i].indexName;
                goto FOUND_INDEX;
            }
        }

        goto NOT_FOUND_INDEX;
    }

  FOUND_INDEX:
    if (dbi_Schema_GetIndexInfo(-1, indexName, &index_info,
                    &idxflds_info, relationName) < 0)
    {
        /* ... */
    }
    else
    {
        for (i = 0; i < index_info.numFields; i++)
        {
            if (valleft->u.value.attrdesc.Hattr == idxflds_info[i].fieldId
                    && idxflds_info[i].collation == MDB_COL_CHAR_REVERSE)
            {
                PMEM_FREENUL(fieldinfo);
                PMEM_FREENUL(idxflds_info);
                PMEM_FREENUL(indexes);
                return DB_TRUE;
            }
        }
    }

  NOT_FOUND_INDEX:
    PMEM_FREENUL(indexes);
    PMEM_FREENUL(idxflds_info);
    PMEM_FREENUL(fieldinfo);

    return DB_FALSE;
}

/*****************************************************************************/
//!set_expr_attribute

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param expr_list(in/out)   :
 * \param table(in)       :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *      PLAN 관련된 함수
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static int
set_expr_attribute(EXPRDESC_LIST * expr_list, T_TABLEDESC * table,
        T_SCANHINT * scan_hint)
{
    EXPRDESC_LIST *cur;
    T_EXPRESSIONDESC *expr;
    T_POSTFIXELEM *col, *op;
    T_ORTYPE *type;
    T_TABLEDESC *col_table;
    T_PREDTYPE pred_type;
    int col_match;

    for (cur = expr_list; cur; cur = cur->next)
    {
        expr = ((T_OR_EXPRDESC *) cur->ptr)->expr;
        type = &((T_OR_EXPRDESC *) cur->ptr)->type;

        pred_type = check_predicate_type(expr);
        if (pred_type == PRED_NONE)
            continue;

        col = expr->list[0];
        col_table = &col->u.value.attrdesc.table;
        if (IS_EQUAL_TABLE(col_table, table))
            col_match = 1;
        else
            col_match = 0;
        if (!col_match && pred_type == PRED_JOIN && expr->len <= 3)
        {
            col = expr->list[1];
            col_table = &col->u.value.attrdesc.table;
            if (IS_EQUAL_TABLE(col_table, table))
            {
                col_match = 1;
                *type |= ORT_REVERSED;
            }
        }
        if (col_match)
        {
            op = expr->list[expr->len - 1];
            if (is_filterop(op->u.op.type))
                *type |= ORT_FILTERABLE;
            if (is_indexop(op->u.op.type))
                *type |= ORT_INDEXABLE;
            if (pred_type == PRED_SCAN)
                *type |= ORT_SCANNABLE;
            if (pred_type == PRED_JOIN)
                *type |= ORT_JOINABLE;

            if (op->u.op.type == SOT_LIKE && pred_type == PRED_SCAN)
            {
                T_POSTFIXELEM *val = expr->list[1];

                if (!val->u.value.is_null && val->u.value.value_len > 0)
                {
                    if (IS_BS(val->u.value.valuetype))
                    {
                        char ch = val->u.value.u.ptr[0];
                        char ch_end =
                                val->u.value.u.ptr[val->u.value.value_len - 1];

                        if (!IS_LIKESPECIAL(ch) &&
                                (ch_end == '%' || !IS_LIKESPECIAL(ch_end)))
                        {       /* right cutted like */
                        }
                        else if ((ch == '%' || !IS_LIKESPECIAL(ch)) &&
                                !IS_LIKESPECIAL(ch_end))
                        {       /* left cutted like */
                            if (!_use_reverse_index(expr, scan_hint))
                                *type &= ~ORT_INDEXABLE;
                        }
                        else
                        {
                            *type &= ~ORT_INDEXABLE;
                        }
                    }
                    else if (IS_NBS(val->u.value.valuetype))
                    {
                        DB_WCHAR ch = val->u.value.u.Wptr[0];

                        if (ch == '\\' || ch == '%' || ch == '_')
                            *type &= ~ORT_INDEXABLE;
                    }
                }
            }
        }
    }
    return RET_SUCCESS;
}

/*****************************************************************************/
//! reorder_exprs_by_attr

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param expr_list(in/out):
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
reorder_exprs_by_attr(EXPRDESC_LIST * expr_list)
/* move constant index exprs ahead of variable index exprs */
{
    ATTRED_EXPRDESC_LIST *cur, *target;
    T_OR_EXPRDESC *ptr;
    T_ORTYPE type;

    for (cur = expr_list, target = NULL; cur; cur = cur->next)
    {
        type = ((T_OR_EXPRDESC *) cur->ptr)->type;
        if ((type & ORT_INDEXABLE) && (type & ORT_SCANNABLE))
        {
            if (target == NULL)
                continue;
            else
            {   /* swapping */
                ptr = cur->ptr;
                cur->ptr = target->ptr;
                target->ptr = ptr;
                if (target->next == cur)
                    target = cur;
                else
                    target = target->next;
            }
        }
        else
        {
            if (target == NULL)
                target = cur;
        }
    }
    return RET_SUCCESS;
}


/*****************************************************************************/
//!choose_exprs_for_nest

/*! \breif  check the composition of expr and choose the exprs
 *          nest->attr_expr_list 정보 설정
 ************************************
 * \param nest(in/out) : SELECT시 테이블에 대한 접근 정보
 * \param expr_list(in) :
 * \param select(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note
 * check the composition of expr and choose the exprs
 *   ---------------------
 *   sc \ jc | Yes |  No
 *   --------+-----+------
 *      Yes  |  O  |  O
 *   --------+-----+------
 *      No   |  X  |  O
 *   ---------------------
 *  *) sc: scan column, jc: join column
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static int
choose_exprs_for_nest(T_NESTING * nest, EXPRDESC_LIST * expr_list,
        T_SELECT * select, int is_join_on)
{
    EXPRDESC_LIST *cur;
    T_EXPRESSIONDESC *expr;
    T_POSTFIXELEM *elem;
    T_OR_EXPRDESC *attr_expr;
    T_TABLEDESC *table, *elem_table;
    int n_scan_cols;
    int n_join_cols;
    int i;
    int is_join_on_expr = 0;
    int ret;

    table = &nest->table;
    for (cur = expr_list; cur; cur = cur->next)
    {
        expr = (T_EXPRESSIONDESC *) cur->ptr;
        n_scan_cols = 0;
        n_join_cols = 0;
        if (expr->len == 3 || expr->list[expr->len - 1]->u.op.type == SOT_IN)
        {
            if (set_scanrid_in_nest(nest, expr) < 0)
                return RET_ERROR;
        }
        for (i = 0; i < expr->len; i++)
        {
            elem = expr->list[i];
            if (elem->elemtype == SPT_VALUE &&
                    elem->u.value.valueclass == SVC_VARIABLE)
            {
                elem_table = &elem->u.value.attrdesc.table;
                if (elem_table->correlated_tbl != select)
                    continue;   /* subq expr, so no scan, no join col */
                if (IS_EQUAL_TABLE(elem_table, table))
                {
                    is_join_on_expr = 1;
                    n_scan_cols++;
                    break;
                }
                else
                    n_join_cols++;
            }
        }
        if (n_scan_cols || !n_join_cols)
        {
            if (!is_join_on
                    || (is_join_on && is_join_on_expr
                            && is_filterop(expr->list[expr->len -
                                            1]->u.op.type)))
            {
                attr_expr =
                        (T_OR_EXPRDESC *) PMEM_ALLOC(sizeof(T_OR_EXPRDESC));
                if (attr_expr == NULL)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY,
                            0);
                    return RET_ERROR;
                }

                attr_expr->type = ORT_NONE;
                attr_expr->expr = expr;

                LINK_ATTRED_EXPRDESC_LIST(nest->attr_expr_list, attr_expr,
                        ret);
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
//! put_exprs_for_nest

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param nest(in/out) :
 * \param expr_list(out) :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *      PLAN 정보를 생성시 사용
 *****************************************************************************/
static int
put_exprs_for_nest(T_NESTING * nest, EXPRDESC_LIST * expr_list)
/* put all exprs in nest */
{
    EXPRDESC_LIST *cur;
    T_EXPRESSIONDESC *expr;
    T_OR_EXPRDESC *attr_expr;
    int ret;

    for (cur = expr_list; cur; cur = cur->next)
    {
        expr = (T_EXPRESSIONDESC *) cur->ptr;
        if (expr->len == 3 || expr->list[expr->len - 1]->u.op.type == SOT_IN)
        {
            if (set_scanrid_in_nest(nest, expr) < 0)
                return RET_ERROR;
        }
        attr_expr = (T_OR_EXPRDESC *) PMEM_ALLOC(sizeof(T_OR_EXPRDESC));
        if (attr_expr == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return RET_ERROR;
        }
        attr_expr->type = ORT_NONE;
        attr_expr->expr = expr;
        LINK_ATTRED_EXPRDESC_LIST(nest->attr_expr_list, attr_expr, ret);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return RET_ERROR;
        }
    }
    return RET_SUCCESS;
}

/*****************************************************************************/
//! make_nestorder

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid(in) :
 * \param select(in/out) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
make_nestorder(int connid, T_SELECT * select)
{
    T_NESTING *nest;
    T_QUERYDESC *qdesc;
    T_JOINTABLEDESC *tables;
    int n_tables;
    int i, ret;
    int is_join_on = 0;

#ifdef MDB_DEBUG
    sc_assert(select != NULL, __FILE__, __LINE__);
#endif
    if (select->first_sub &&
            (ret = make_nestorder(connid, select->first_sub)) != RET_SUCCESS)
        return ret;
    if (select->sibling_query &&
            (ret = make_nestorder(connid, select->sibling_query))
            != RET_SUCCESS)
        return ret;

    nest = select->planquerydesc.nest;
    qdesc = &select->planquerydesc.querydesc;
    tables = qdesc->fromlist.list;
    n_tables = qdesc->fromlist.len;
    is_join_on = qdesc->fromlist.outer_join_exists;

    if (qdesc->setlist.len > 0)
    {
        for (i = 0; i < qdesc->setlist.len; i++)
        {
            if (qdesc->setlist.list[i]->elemtype == SPT_SUBQUERY)
            {
                ret = make_nestorder(connid, qdesc->setlist.list[i]->u.subq);
                if (ret != RET_SUCCESS)
                {
                    return ret;
                }
            }
        }

        return RET_SUCCESS;
    }

    if (n_tables > MAX_JOIN_TABLE_NUM)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_TOOMANYJOINTABLE, 0);
        return SQL_E_TOOMANYJOINTABLE;
    }

    for (i = 0; i < n_tables; i++)
    {
        initialize_nest(&nest[i], &tables[i].table);

        if (qdesc->expr_list != NULL)
        {
            /* process where-condition */
            ret = choose_exprs_for_nest(&nest[i], qdesc->expr_list,
                    select, is_join_on);
            if (ret != RET_SUCCESS)
                return ret;
            if (n_tables > 1 || tables[i].scan_hint.scan_type != SMT_NONE)
            {
                nest[i].scanrid = NULL_OID;
                nest[i].ridin_cnt = 0;
                nest[i].is_param_scanrid = 0;
            }
        }

        if (tables[i].expr_list != NULL)
        {
            /* process from-condition */
            ret = put_exprs_for_nest(&nest[i], tables[i].expr_list);
            if (ret != RET_SUCCESS)
                return ret;
        }

        if (nest[i].attr_expr_list != NULL)
        {
            ret = set_expr_attribute(nest[i].attr_expr_list,
                    &nest[i].table, &tables[i].scan_hint);
            if (ret != RET_SUCCESS)
                return ret;
            ret = reorder_exprs_by_attr(nest[i].attr_expr_list);
            if (ret != RET_SUCCESS)
                return ret;
        }
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! init_rec_buf

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param rttable :
 * \param tableinfo :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
init_rec_buf(T_RTTABLE * rttable, SYSTABLE_T * tableinfo)
{
    SYSTABLE_T tableinfo_space;

    if (tableinfo == NULL)
    {
        tableinfo = &tableinfo_space;

        if (_Schema_GetTableInfo(rttable->table.tablename, tableinfo) < 0)
            return RET_ERROR;
    }

    rttable->nullflag_offset =
            get_nullflag_offset(-1, rttable->table.tablename, tableinfo);

    rttable->rec_len = get_recordlen(-1, rttable->table.tablename, tableinfo);

    if (rttable->rec_len < 0)
        return RET_ERROR;

    /* make rec_buf */
    rttable->rec_buf = sql_malloc(rttable->rec_len, DT_NULL_TYPE);

    if (rttable->rec_buf == NULL)
        return RET_ERROR;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! init_rec_values

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param rttable()    :
 * \param tableinfo()  :
 * \param fieldinfo()    :
 * \param select_star()  :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *  - called : exec_delete(), make_rttableorder(),
 *             make_new_select(), make_orderby_using_temptable()
 *
 *****************************************************************************/
FIELDVALUES_T *
init_rec_values(T_RTTABLE * rttable, SYSTABLE_T * tableinfo,
        SYSFIELD_T * fieldsinfo, int select_star)
{
    int i;
    FIELDVALUES_T *rec_values = NULL;
    int numFields = 0;
    SYSTABLE_T tableinfo_space;
    int free_fieldsinfo = 0;

    if (tableinfo == NULL)
    {
        tableinfo = &tableinfo_space;

        if (_Schema_GetTableInfo(rttable->table.tablename, tableinfo) < 0)
            return NULL;
    }

    numFields = tableinfo->numFields;

    if (fieldsinfo == NULL)
    {
        fieldsinfo = sql_malloc(numFields * sizeof(SYSFIELD_T), DT_NULL_TYPE);
        if (fieldsinfo == NULL)
            return NULL;

        free_fieldsinfo = 1;

        if (_Schema_GetFieldsInfo(tableinfo->tableName, fieldsinfo) < 0)
            goto end;
    }

    rec_values = sql_malloc(sizeof(FIELDVALUES_T), DT_NULL_TYPE);
    if (rec_values == NULL)
        goto end;

    rec_values->fld_cnt = tableinfo->numFields;
    rec_values->rec_oid = NULL_OID;
    rec_values->fld_value = sql_xcalloc(numFields * sizeof(FIELDVALUE_T),
            DT_NULL_TYPE);
    rec_values->sel_fld_cnt = 0;
    rec_values->sel_fld_pos = sql_malloc(INT_SIZE * tableinfo->numFields,
            DT_NULL_TYPE);
    if (rec_values->sel_fld_pos == NULL)
        goto end;
    rec_values->eval_fld_cnt = 0;
    rec_values->eval_fld_pos = sql_malloc(INT_SIZE * tableinfo->numFields,
            DT_NULL_TYPE);
    if (rec_values->eval_fld_pos == NULL)
        goto end;

    if (!rec_values->fld_value)
    {
        PMEM_FREENUL(rec_values);
        goto end;
    }

    for (i = 0; i < tableinfo->numFields; i++)
    {
        rec_values->fld_value[i].position_ = fieldsinfo[i].position;
        rec_values->fld_value[i].type_ = fieldsinfo[i].type;
        rec_values->fld_value[i].offset_ = fieldsinfo[i].offset;
        rec_values->fld_value[i].h_offset_ = fieldsinfo[i].h_offset;
        rec_values->fld_value[i].scale_ = fieldsinfo[i].scale;
        rec_values->fld_value[i].length_ = fieldsinfo[i].length;
        rec_values->fld_value[i].fixed_part = fieldsinfo[i].fixed_part;
        rec_values->fld_value[i].collation = fieldsinfo[i].collation;
    }

  end:
    if (free_fieldsinfo)
        PMEM_FREENUL(fieldsinfo);

    return rec_values;
}

/*****************************************************************************/
//! copy_rec_values

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param dest_rec_values()    :
 * \param src_rec_values()  :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *  - called : exec_delete()
 *  - 메모리 TEST..
 *****************************************************************************/
int
copy_rec_values(FIELDVALUES_T * dest_rec_values,
        FIELDVALUES_T * src_rec_values)
{
    int i;
    char *ptr;

    FIELDVALUE_T *src_fld;

    dest_rec_values->fld_cnt = src_rec_values->fld_cnt;
    dest_rec_values->rec_oid = src_rec_values->rec_oid;

    if (dest_rec_values->fld_value == NULL)
    {
        dest_rec_values->fld_value =
                sql_xcalloc(src_rec_values->fld_cnt * sizeof(FIELDVALUE_T),
                DT_NULL_TYPE);
    }

    if (dest_rec_values->fld_value == NULL)
    {
        return RET_ERROR;
    }

    dest_rec_values->sel_fld_cnt = src_rec_values->sel_fld_cnt;
    if (dest_rec_values->sel_fld_pos == NULL)
    {
        dest_rec_values->sel_fld_pos =
                sql_xcalloc(src_rec_values->fld_cnt * INT_SIZE, DT_NULL_TYPE);
    }
    if (dest_rec_values->sel_fld_pos == NULL)
    {
        return RET_ERROR;
    }

    dest_rec_values->eval_fld_cnt = src_rec_values->eval_fld_cnt;
    if (dest_rec_values->eval_fld_pos == NULL)
    {
        dest_rec_values->eval_fld_pos =
                sql_xcalloc(src_rec_values->fld_cnt * INT_SIZE, DT_NULL_TYPE);
    }
    if (dest_rec_values->eval_fld_pos == NULL)
    {
        return RET_ERROR;
    }

    for (i = 0; i < src_rec_values->fld_cnt; i++)
    {
        src_fld = &src_rec_values->fld_value[i];

        if ((IS_ALLOCATED_TYPE(src_fld->type_) || src_fld->type_ == DT_DECIMAL)
                && src_fld->value_.pointer_ != NULL)
        {
            if (dest_rec_values->fld_value[i].value_.pointer_ == NULL)
            {
                ptr = sql_malloc(src_fld->length_ + 1, src_fld->type_);
                if (ptr == NULL)
                {
                    return RET_ERROR;
                }
            }
            else
            {
                ptr = dest_rec_values->fld_value[i].value_.pointer_;
            }
        }
        else
        {
            ptr = NULL;
        }

        dest_rec_values->fld_value[i] = *src_fld;

        if (ptr)
        {
            dest_rec_values->fld_value[i].value_.pointer_ = ptr;
            memcpy_func[src_fld->type_] (ptr, src_fld->value_.pointer_,
                    src_fld->length_);
        }
    }

    sc_memcpy(dest_rec_values->sel_fld_pos, src_rec_values->sel_fld_pos,
            sizeof(dest_rec_values->sel_fld_pos[0])
            * dest_rec_values->sel_fld_cnt);
    sc_memcpy(dest_rec_values->eval_fld_pos, src_rec_values->eval_fld_pos,
            sizeof(dest_rec_values->eval_fld_pos[0])
            * dest_rec_values->eval_fld_cnt);

    return RET_SUCCESS;
}

/*****************************************************************************/
//! make_rttableorder

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle()    :
 * \param rttable()  :
 * \param nest()  :
 * \param fromtables()  :
 * \param from_index_forrun()  :
 * \param select_star()  :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *
 *****************************************************************************/
static int
make_rttableorder(int handle, T_RTTABLE * rttable, T_NESTING * nest,
        T_LIST_JOINTABLEDESC * fromtables, int *from_index_forrun,
        int select_star)
{
    int run;
    int ord;                    /* table ordinal */

    SYSTABLE_T *tableinfo;
    SYSFIELD_T *fieldsinfo;

    for (run = 0; run < fromtables->len; run++)
    {
        /* set table_ordinal */
        ord = from_index_forrun[run];
        rttable[run].table_ordinal = ord;

        tableinfo = &fromtables->list[ord].tableinfo;
        fieldsinfo = fromtables->list[ord].fieldinfo;
        /* set table information */
        rttable[run].table.tablename = fromtables->list[ord].table.tablename;
        rttable[run].table.aliasname = fromtables->list[ord].table.aliasname;

        rttable[run].table.correlated_tbl =
                fromtables->list[ord].table.correlated_tbl;
        if (rttable[run].table.correlated_tbl)
        {
            rttable[run].status = SCS_INITIAL_VACANCY;
            rttable[run].table.correlated_tbl->rttable_idx = run;
        }
        else
        {
            rttable[run].status = SCS_CLOSE;
        }

        rttable[run].jump2offset = 0;

        /* set nullflag_offset and rec_len */

        if (rttable[run].table.correlated_tbl)
        {
            rttable[run].nullflag_offset =
                    get_nullflag_offset(handle, rttable[run].table.tablename,
                    &fromtables->list[ord].tableinfo);
            rttable[run].rec_len =
                    get_recordlen(handle, rttable[run].table.tablename,
                    &fromtables->list[ord].tableinfo);
        }

#ifdef MDB_DEBUG
        if (mdb_strcmp(rttable[run].table.tablename,
                        fromtables->list[ord].tableinfo.tableName))
        {
            MDB_SYSLOG(("%s:%d %d %s %d %s\n", __FILE__, __LINE__,
                            run, rttable[run].table.tablename,
                            ord, fromtables->list[ord].tableinfo.tableName));
        }
#endif

        rttable[run].rec_values = init_rec_values(&rttable[run],
                tableinfo, fieldsinfo, select_star);

        if (rttable[run].rec_values == NULL)
        {
            return RET_ERROR;
        }

        if (rttable[run].table.correlated_tbl)
        {
            if (rttable[run].rec_len < 0)
            {
                return RET_ERROR;
            }

            /* make rec_buf */
            rttable[run].rec_buf =
                    PMEM_ALLOC(sizeof(char) * rttable[run].rec_len);
            if (!rttable[run].rec_buf)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return RET_ERROR;
            }
        }

        /* initialize */
        rttable[run].save_rec_values = NULL;
        rttable[run].prev_rec_values = NULL;
        rttable[run].prev_rec_max_cnt = 0;
        rttable[run].prev_rec_cnt = 0;
        rttable[run].prev_rec_idx = -1;
        rttable[run].has_prev_cursor_posi = 0;
        rttable[run].has_next_record_read = 0;
        rttable[run].msync_flag = 0;

    }

    return RET_SUCCESS;
}

/*
 * runtime table의 순서를 정한다.
 */
/*****************************************************************************/
//! free_scan_info

/*! \breif  테이블의 접근 정보(nest)에 할당된 메모리를 헤제
 ************************************
 * \param nest(in/ou) : 테이블의 접근 정보
 ************************************
 * \return  return_value
 ************************************
 * \note
 *      SQL_PLAN을 생성시 사용된다.
 *
 *****************************************************************************/
void
free_scan_info(T_NESTING * nest)
{
    T_ISCANITEM *iscan;

    while (nest->iscan_list)
    {
        iscan = nest->iscan_list;
        nest->iscan_list = iscan->next;
        PMEM_FREENUL(iscan->indexfields);
        PMEM_FREENUL(iscan->orexprs);
        PMEM_FREENUL(iscan);
    }

    PMEM_FREENUL(nest->indexable_orexprs);
    nest->indexable_orexpr_count = 0;
    nest->joinable_orexpr_count = 0;
    nest->total_orexpr_count = 0;
}

/*****************************************************************************/
//! make_scandesc_for_index_create

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle() :
 * \param join() :
 * \param nest() :
 * \param index_count() :
 * \param indexes() :
 * \param ppScanDesc() :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘
 *  called : check_and_make_iscan
 * Build indexname, minkey, maxkey, and filter
 * for selecting records whose keys will be inserted
 * into the to-be-created temp index.
 *
 *****************************************************************************/
static int
make_scandesc_for_index_create(int handle,
        T_JOINTABLEDESC * join, T_NESTING * nest,
        int index_count, SYSINDEX_T * indexes, SCANDESC_T ** ppScanDesc)
{
    T_NESTING dummy_nest;
    int ret = RET_SUCCESS;
    int orexpr_count, i, j;
    T_OR_EXPRDESC **orexpr_array;
    T_OR_EXPRDESC *orexpr;
    T_EXPRESSIONDESC *expr;
    T_POSTFIXELEM *elem;

    orexpr_count = 0;
    for (i = 0; i < nest->indexable_orexpr_count; i++)
    {
        orexpr = nest->indexable_orexprs[i];
        if (orexpr->type & ORT_JOINABLE)
            continue;
        if (orexpr->type & ORT_FILTERABLE)
            orexpr_count++;
    }
    if (orexpr_count == 0)
    {
        *ppScanDesc = NULL;
        return RET_SUCCESS;
    }

    /* prepare dummy nest */
    sc_memcpy(&dummy_nest, nest, sizeof(T_NESTING));
    dummy_nest.attr_expr_list = NULL;

    /* extract scannable && indexable && filterable expression list */
    dummy_nest.indexable_orexpr_count = orexpr_count;
    dummy_nest.indexable_orexprs = (T_OR_EXPRDESC **)
            PMEM_ALLOC(orexpr_count * sizeof(T_OR_EXPRDESC *));
    if (dummy_nest.indexable_orexprs == NULL)
    {
        *ppScanDesc = NULL;
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return SQL_E_OUTOFMEMORY;
    }
    orexpr_count = 0;
    for (i = 0; i < nest->indexable_orexpr_count; i++)
    {
        orexpr = nest->indexable_orexprs[i];
        if (orexpr->type & ORT_JOINABLE)
            continue;
        if (orexpr->type & ORT_FILTERABLE)
        {
            /* find NOT-determined element, that is, parameter element */
            expr = orexpr->expr;
            for (j = 0; j < expr->len; j++)
            {
                elem = expr->list[j];
                if (elem->elemtype == SPT_VALUE &&
                        elem->u.value.valueclass == SVC_CONSTANT &&
                        elem->u.value.param_idx)
                    break;
            }
            if (j == expr->len)
            {   /* Determined element */
                dummy_nest.indexable_orexprs[orexpr_count] = orexpr;
                orexpr_count++;
            }
        }
    }
    if (orexpr_count == 0)
    {
        PMEM_FREENUL(dummy_nest.indexable_orexprs);
        *ppScanDesc = NULL;
        return RET_SUCCESS;
    }

    if (orexpr_count != dummy_nest.indexable_orexpr_count)
    {
        dummy_nest.indexable_orexpr_count = orexpr_count;
    }

    ret = build_iscan_list(handle, &dummy_nest, index_count, indexes, 0);
    if (ret < 0)
    {
        *ppScanDesc = NULL;
        goto end;
    }
    compute_iscan_cost(&dummy_nest);

    /* build scan descriptor */
    *ppScanDesc = (SCANDESC_T *) PMEM_ALLOC(sizeof(SCANDESC_T));
    if (*ppScanDesc == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        ret = SQL_E_OUTOFMEMORY;
        goto end;
    }

    if (IS_ISCAN(dummy_nest.iscan_list))
    {
        ret = set_indexrange(handle, &dummy_nest,
                join->tableinfo.numFields, join->fieldinfo);
        if (ret != RET_SUCCESS)
        {
            PMEM_FREENUL(*ppScanDesc);
            goto end;
        }

        /* remove ORT_INDEXRANGED expressions */
        orexpr_count = dummy_nest.indexable_orexpr_count;
        orexpr_array = dummy_nest.indexable_orexprs;
        for (i = 0; i < orexpr_count; i++)
        {
            if (orexpr_array[i]->type & ORT_INDEXRANGED)
            {
                orexpr_array[i]->type &= ~(ORT_INDEXRANGED);    /* clear it */
                orexpr_array[i] = orexpr_array[orexpr_count - 1];
                orexpr_array[orexpr_count - 1] = NULL;
                orexpr_count--;
                i--;
            }
        }
        dummy_nest.indexable_orexpr_count = orexpr_count;

        ret = make_keyrange_for_scandesc(&dummy_nest, *ppScanDesc);
        if (ret < 0 || ret == RET_FALSE)
        {
            PMEM_FREENUL(*ppScanDesc);
            goto end;
        }
    }
    else    /* sequential scan */
    {
        (*ppScanDesc)->idxname = NULL;
    }

    ret = make_filter_for_scandesc(&dummy_nest, *ppScanDesc);
    if (ret < 0 || ret == RET_FALSE)
    {
        KeyValue_Delete((struct KeyValue *) &(*ppScanDesc)->minkey);
        KeyValue_Delete((struct KeyValue *) &(*ppScanDesc)->maxkey);
        PMEM_FREENUL(*ppScanDesc);
        goto end;
    }

    ret = RET_SUCCESS;

  end:
    /* free dummy_nest */
    free_scan_info(&dummy_nest);
    cleanup_indexfieldinfo(&dummy_nest, NULL);
    nest->iscan_list = NULL;
    return ret;
}

/*****************************************************************************/
//! make_scandesc_for_index_create

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid(in)                :
 * \param join(in)                  :
 * \param nest(in)                  :
 * \param num_usable_indexes(in/out):
 * \param usable_indexes(in/out)    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘
 *
 *****************************************************************************/
static int
extract_usable_indexes(int connid, T_JOINTABLEDESC * join, T_NESTING * nest,
        int *num_usable_indexes, SYSINDEX_T ** usable_indexes)
{
    T_SCANHINT *scan_hint = &join->scan_hint;
    int i, j, k;
    int index_count;
    SYSINDEX_T *indexes;
    SYSINDEXFIELD_T *indexfields;

    int check_open_desc = 0;

    /* when a sequential scan hint is given */
    if (scan_hint->scan_type == SMT_SSCAN)
    {
        *num_usable_indexes = 0;
        *usable_indexes = NULL;
        return 1;       /* chosen by scan hint */
    }

    /* get all indexes information */
    index_count = dbi_Schema_GetTableIndexesInfo(connid, join->table.tablename,
            SCHEMA_ALL_INDEX, &indexes);
    if (index_count < 0)
    {
        MDB_SYSLOG(("Getting All IndexesInfo of Table(%s) Fail.\n",
                        join->table.tablename));
        return index_count;
    }

    /* when an index scan hint is given */
    if (scan_hint->scan_type == SMT_ISCAN)
    {
        /* 1) Find an index that matches with scan hint */
        if (scan_hint->indexname != NULL)       /* index name is given */
        {
            for (i = 0; i < index_count; i++)
            {
                if (!mdb_strcmp(scan_hint->indexname, indexes[i].indexName))
                {
                    /* index found */
                    if (i > 0)
                    {
                        sc_memcpy(&indexes[0], &indexes[i],
                                sizeof(SYSINDEX_T));
                    }
                    *num_usable_indexes = 1;
                    *usable_indexes = indexes;
                    return 1;   /* chosen by scan hint */
                }
            }

            /* There is no matching index */
            MDB_SYSLOG(("Scan Hint IndexName(%s) Not Found in Table(%s).\n",
                            scan_hint->indexname, join->table.tablename));
        }
        else if (scan_hint->fields.len == 0)
        {
            for (i = 0; i < index_count; i++)
            {
                if (isPrimaryIndex_name(indexes[i].indexName))
                {
                    /* primary index found */
                    if (i > 0)
                    {
                        sc_memcpy(&indexes[0], &indexes[i],
                                sizeof(SYSINDEX_T));
                    }
                    *num_usable_indexes = 1;
                    *usable_indexes = indexes;
                    return 1;   /* chosen by scan hint */
                }
            }

            /* There is no primary index */
            MDB_SYSLOG(("Scan Hint Primary Index Not Found in Table(%s).\n",
                            join->table.tablename));
        }
        else    /* scan_hint->indexname == NULL && scan_hint->fields.len > 0 :
                   index columns are given */
        {
            int temp_serial_num;
            char indexname[REL_NAME_LENG];
            SYSINDEX_T indexinfo;

            for (i = 0; i < index_count; i++)
            {
                if (isTempIndex_name(indexes[i].indexName))
                    continue;
                if (indexes[i].numFields < scan_hint->fields.len)
                    continue;   /* # of index fields Not match */

                if (dbi_Schema_GetIndexInfo(connid, indexes[i].indexName,
                                NULL, &indexfields, NULL) < 0)
                {
                    MDB_SYSLOG(("Getting Index(%s) Information FAIL.\n",
                                    indexes[i].indexName));
                    continue;   /* ignore the current index */
                }

                if (indexfields[0].order !=
                        scan_hint->fields.list[0].ordertype)
                    check_open_desc = 1;
                else
                    check_open_desc = 0;
                for (j = 0; j < scan_hint->fields.len; j++)
                {
                    for (k = 0; k < join->tableinfo.numFields; k++)
                    {
                        if (indexfields[j].fieldId ==
                                join->fieldinfo[k].fieldId)
                            break;
                    }
                    if (k >= join->tableinfo.numFields)
                    {
                        MDB_SYSLOG(("FieldID Mismatch..\n"));
                        PMEM_FREENUL(indexfields);
                        PMEM_FREENUL(indexes);
                        return RET_ERROR;
                    }

                    /* found field position : k */
                    if (indexfields[j].order ==
                            scan_hint->fields.list[j].ordertype)
                    {
                        if (check_open_desc)    /* must be the different order */
                            break;
                    }
                    else
                    {
                        if (!check_open_desc)   /* must be the same order */
                            break;
                    }

                    if (mdb_strcmp(join->fieldinfo[k].fieldName,
                                    scan_hint->fields.list[j].name))
                        break;  /* different field */

                    if (scan_hint->fields.list[j].collation == MDB_COL_NONE)
                    {
                        if (indexfields[j].collation !=
                                join->fieldinfo[k].collation)
                            break;
                    }
                    else if ((MDB_COL_TYPE) indexfields[j].collation !=
                            scan_hint->fields.list[j].collation)
                    {
                        break;  /* different collation */
                    }
                }

                PMEM_FREENUL(indexfields);

                if (j == scan_hint->fields.len)
                {       /* index found */
                    if (i > 0)
                    {
                        sc_memcpy(&indexes[0], &indexes[i],
                                sizeof(SYSINDEX_T));
                    }
                    *num_usable_indexes = 1;
                    *usable_indexes = indexes;
                    return 1;   /* chosen by scan hint */
                }
            }

            /* Following Two cases can be reach to this place:
               1) there is no existent index. (index_count == 0)
               2) there is no match index.
             */
            /* Make temp index using the index fields */

            /* check if the given index fields are valid */
            for (j = 0; j < scan_hint->fields.len; j++)
            {
                for (k = 0; k < join->tableinfo.numFields; k++)
                {
                    if (!mdb_strcmp(scan_hint->fields.list[j].name,
                                    join->fieldinfo[k].fieldName))
                        break;
                }
                if (k >= join->tableinfo.numFields)
                {
                    MDB_SYSLOG(
                            ("Scan Hint IField(%s) Not Exist in Table(%s).\n",
                                    scan_hint->fields.list[j].name,
                                    join->table.tablename));
                    goto end;   /* ignore the scan hint */
                }
            }

            temp_serial_num =
                    dbi_Trans_NextTempName(connid, 0, indexname, NULL);
            if (temp_serial_num < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
                goto end;
            }

            if (1)
            {
                int result;
                int keyins_flag = 1;
                SCANDESC_T *scandesc = NULL;

                if (temp_serial_num <= 5)
                {
                    /* temp partial index might be created */
                    /* Build indexname, minkey, maxkey, and filter
                       for selecting records whose keys will be inserted
                       into the to-be-created temp index.
                     */
                    result = make_scandesc_for_index_create(connid, join, nest,
                            index_count, indexes, &scandesc);
                    if (result < 0)
                    {
                        MDB_SYSLOG(("Make ScanDesc For Index Create FAIL \n"));
                        /* go downward */
                        /* keyins_flag == 1 && scandesc == NULL */
                        /* create complete index */
                    }
                    else if (result == RET_FALSE)
                    {
                        keyins_flag = 0;
                        /* go downward */
                        /* keyins_flag == 0 && scandesc == NULL */
                        /* create empty index (a sort of partial index) */
                    }
                }

                if (keyins_flag == 0 || (keyins_flag && scandesc))
                {
                    /* create temp partial index */
                    /* change indexname : #TI_XXX => #PI_XXX */
                    indexname[1] = 'p';
                }

                /* make temp index using the index fields */
                result = create_temp_index(connid,
                        &join->table, &scan_hint->fields,
                        indexname, keyins_flag, scandesc);

                nest->keyins_flag = keyins_flag;
                nest->scandesc = scandesc;

                if (result < 0)
                {
                    MDB_SYSLOG(("Create Temp Table Of Scan Hint Fail.\n"));
                    goto end;   /* ignore the scan hint */
                }
            }

            if (dbi_Schema_GetIndexInfo(connid, indexname, &indexinfo,
                            NULL, NULL) < 0)
            {
                MDB_SYSLOG(("Getting Index(%s) Info Fail.\n", indexname);
                        (void) drop_temp_index(connid, indexname));
                goto end;       /* ignore the scan hint */
            }

            if (index_count == 0)
            {
                indexes = PMEM_ALLOC(sizeof(SYSINDEX_T));
                if (indexes == NULL)
                {
                    (void) drop_temp_index(connid, indexname);
                    goto end;   /* ignore the scan hint */
                }
            }

            /* move the index info to the first position */
            sc_memcpy(&indexes[0], &indexinfo, sizeof(SYSINDEX_T));
            *num_usable_indexes = 1;
            *usable_indexes = indexes;
            return 1;   /* chosen by scan hint */
        }
    }

  end:
    *num_usable_indexes = index_count;
    *usable_indexes = indexes;
    return 0;   /* chosen by query optimizer */
}

/*****************************************************************************/
//!build_iscan_list

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid(in) :
 * \param nest(in/out) :
 * \param index_count(in) :
 * \param indexes(in/out) :
 * \param chosen_by_hint(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static int
build_iscan_list(int connid, T_NESTING * nest,
        int index_count, SYSINDEX_T * indexes, int chosen_by_hint)
{
    SYSINDEXFIELD_T *indexfields;
    T_OR_EXPRDESC *orexpr;
    T_OR_EXPRDESC *temp_orexpr;
    T_ATTRDESC *attr;
    T_ISCANITEM *iscan;
    int m_size;
    int i, j, idx;
    int target, start;
    int attr_index;

    for (i = 0; i < index_count; i++)
    {
        if (chosen_by_hint == -1 && isTempIndex_name(indexes[i].indexName))
            continue;
        /* get index fields information of an index */
        if (dbi_Schema_GetIndexInfo(connid, indexes[i].indexName, NULL,
                        &indexfields, NULL) < 0)
        {
            MDB_SYSLOG(("Getting Index(%s) Information FAIL.\n",
                            indexes[i].indexName));
            continue;   /* some problem exist */
        }

        target = 0;
        for (j = 0; j < indexes[i].numFields; j++)
        {
            start = target;
            target = -1;
            for (idx = start; idx < nest->indexable_orexpr_count; idx++)
            {
                orexpr = nest->indexable_orexprs[idx];
                if (orexpr->type & ORT_REVERSED)
                {
                    attr_index = orexpr->expr->len - 2;
                }
                else
                {
                    attr_index = 0;
                }

                attr = &orexpr->expr->list[attr_index]->u.value.attrdesc;

/* without HINT, 
         collation of tableColumn and indexColumn must be the same */
                if (indexfields[j].fieldId != attr->Hattr ||
                        (chosen_by_hint <= 0 &&
                                (MDB_COL_TYPE) indexfields[j].collation !=
                                attr->collation))
                {
                    if (target < 0)
                        target = idx;
                }
                else
                {
                    if (target >= 0)
                    {   /* swapping */
                        temp_orexpr = nest->indexable_orexprs[target];
                        nest->indexable_orexprs[target]
                                = nest->indexable_orexprs[idx];
                        nest->indexable_orexprs[idx] = temp_orexpr;
                        target++;
                    }
                }
            }
            if (target == -1)
            {
                target = nest->indexable_orexpr_count;
                break;
            }
            if (target == start)
            {   /* No OR exprs matched */
                break;
            }
        }
        if ((target > 0 /* there are some index OR expressions */  ||
                        chosen_by_hint > 0 /* chosen by scan hint */ )
                && nest->scanrid == NULL_OID)
        {
            iscan = (T_ISCANITEM *) PMEM_ALLOC(sizeof(T_ISCANITEM));
            if (iscan == NULL)
            {
                PMEM_FREENUL(indexfields);
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return SQL_E_OUTOFMEMORY;
            }

            sc_memcpy(&iscan->indexinfo, &indexes[i], sizeof(SYSINDEX_T));
            iscan->indexfields = indexfields;

            if (nest->indexable_orexpr_count > 0)
            {
                m_size = (nest->indexable_orexpr_count
                        * sizeof(T_OR_EXPRDESC *));
                iscan->orexprs = (T_OR_EXPRDESC **) PMEM_ALLOC(m_size);
                if (iscan->orexprs == NULL)
                {
                    PMEM_FREENUL(indexfields);
                    PMEM_FREENUL(iscan);
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_OUTOFMEMORY, 0);
                    return SQL_E_OUTOFMEMORY;
                }
                sc_memcpy((char *) iscan->orexprs,
                        (char *) nest->indexable_orexprs, m_size);
            }
            else
            {
                iscan->orexprs = NULL;
            }

            iscan->match_ifield_count = 0;
            iscan->index_orexpr_count = (target > 0 ? target : 0);
            iscan->match_orexpr_count = 0;
            iscan->recompute_cost_flag = 1;
            iscan->index_equal_op_flag = 0;
            if (chosen_by_hint == -1)
                chosen_by_hint = 0;
            iscan->chosen_by_hint_flag = (char) chosen_by_hint;

            iscan->unmatch_scan_count = 0;
            for (j = target; j < nest->indexable_orexpr_count; j++)
            {
                orexpr = iscan->orexprs[j];
                if ((orexpr->type & ORT_INDEXABLE) &&
                        (orexpr->type & ORT_SCANNABLE))
                    iscan->unmatch_scan_count += 1;
            }
            iscan->filter_orexpr_count = iscan->unmatch_scan_count;
            for (j = 0; j < target; j++)
            {
                orexpr = iscan->orexprs[j];
                if ((orexpr->type & ORT_INDEXABLE) &&
                        (orexpr->type & ORT_SCANNABLE))
                    iscan->filter_orexpr_count += 1;
            }

            /* connect iscan into the iscan_list */
            iscan->next = nest->iscan_list;
            nest->iscan_list = iscan;
        }
        else    /* there is no index OR expressions */
        {
            PMEM_FREENUL(indexfields);
        }
    }

    if (IS_SSCAN_FIXED(nest->iscan_list))
    {
        nest->joinable_orexpr_count = 0;
        for (i = 0; i < nest->indexable_orexpr_count; i++)
        {
            orexpr = nest->indexable_orexprs[i];
            if (orexpr->type & ORT_JOINABLE)
                nest->joinable_orexpr_count++;
        }
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! make_scan_info

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid(in) :
 * \param nest(in/out) :
 * \param join(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *
 *****************************************************************************/
static int
make_scan_info(int connid, T_NESTING * nest, T_JOINTABLEDESC * join)
{
    EXPRDESC_LIST *cur;
    T_OR_EXPRDESC *orexpr;
    SYSINDEX_T *indexes;
    int index_count;
    int chosen_by_hint;
    int m_size, i;
    int result;

    /* initialize scan info */
    nest->total_orexpr_count = 0;
    nest->joinable_orexpr_count = 0;
    nest->indexable_orexpr_count = 0;
    nest->indexable_orexprs = NULL;
    nest->iscan_list = NULL;

    /* count total & indexable OR expressions */
    for (cur = nest->attr_expr_list; cur; cur = cur->next)
    {
        nest->total_orexpr_count += 1;
        orexpr = (T_OR_EXPRDESC *) cur->ptr;
        if (orexpr->type & ORT_INDEXABLE)
            nest->indexable_orexpr_count += 1;
    }

    if (nest->indexable_orexpr_count == 0)
    {
        /* No indexable OR expressions */
        if (join->scan_hint.scan_type != SMT_ISCAN)
            return RET_SUCCESS;
        /* join->scan_hint.scan_type == SMT_ISCAN */
        nest->indexable_orexprs = NULL;
    }
    else
    {
        /* get indexable OR expressions */
        m_size = (nest->indexable_orexpr_count * sizeof(T_OR_EXPRDESC *));
        nest->indexable_orexprs = (T_OR_EXPRDESC **) PMEM_ALLOC(m_size);
        if (nest->indexable_orexprs == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return SQL_E_OUTOFMEMORY;
        }
        for (i = 0, cur = nest->attr_expr_list; cur; cur = cur->next)
        {
            orexpr = (T_OR_EXPRDESC *) cur->ptr;
            if (orexpr->type & ORT_INDEXABLE)
            {
                nest->indexable_orexprs[i] = orexpr;
                i++;
            }
        }
    }

    /* extract index information */
    chosen_by_hint = extract_usable_indexes(connid, join, nest,
            &index_count, &indexes);
    if (chosen_by_hint < 0)
    {
        free_scan_info(nest);
        return chosen_by_hint;
    }

    /* build index scan list */
    if (chosen_by_hint == 0 && indexes)
        chosen_by_hint = -1;
    result = build_iscan_list(connid, nest, index_count, indexes,
            chosen_by_hint);
    if (result < 0)
    {
        free_scan_info(nest);
        PMEM_FREENUL(indexes);
        return result;
    }

    PMEM_FREENUL(indexes);
    return RET_SUCCESS;
}

/*****************************************************************************/
//! set_scannable_super_query_ref_orexprs

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param nest(in/out):
 * \param select(in/out):
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static int
set_scannable_super_query_ref_orexprs(T_NESTING * nest, T_SELECT * select)
{
    T_OR_EXPRDESC *orexpr;
    T_POSTFIXELEM *elem;
    T_TABLEDESC *ref = NULL;
    int i, j, start, end;
    int type_change_count = 0;

    for (i = 0; i < nest->indexable_orexpr_count; i++)
    {
        orexpr = nest->indexable_orexprs[i];

        if (orexpr->type & ORT_SCANNABLE)
            continue;

        if (orexpr->type & ORT_JOINABLE)
        {
            if (orexpr->type & ORT_REVERSED)
            {
                start = 0;
                end = orexpr->expr->len - 2;
            }
            else
            {
                start = 1;
                end = orexpr->expr->len - 1;
            }
            for (j = start; j < end; j++)
            {
                elem = orexpr->expr->list[j];
                if (elem->elemtype == SPT_VALUE &&
                        elem->u.value.valueclass == SVC_VARIABLE)
                {
                    ref = &elem->u.value.attrdesc.table;
                    break;
                }
            }
            if (j < end)
            {
                if (ref->correlated_tbl == NULL ||
                        ref->correlated_tbl == select)
                    continue;
                orexpr->type |= ORT_SCANNABLE;
                type_change_count++;
            }
            else
            {
                /* No referenced table */
                MDB_SYSLOG(("NOTE: No referenced table.\n"));
            }
        }
    }

    return type_change_count;
}

/*****************************************************************************/
//! set_scannable_fixed_table_ref_orexprs

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param nest(in):
 * \param table(in):
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static int
set_scannable_fixed_table_ref_orexprs(T_NESTING * nest, T_TABLEDESC * table)
{
    T_OR_EXPRDESC *orexpr;
    T_POSTFIXELEM *elem;
    T_TABLEDESC *ref = NULL;
    int i, j, start, end;
    int type_change_count = 0;

    for (i = 0; i < nest->indexable_orexpr_count; i++)
    {
        orexpr = nest->indexable_orexprs[i];

        if (orexpr->type & ORT_SCANNABLE)
            continue;

        if (orexpr->type & ORT_JOINABLE)
        {
            /* find referenced table */
            if (orexpr->type & ORT_REVERSED)
            {
                start = 0;
                end = orexpr->expr->len - 2;
            }
            else
            {
                start = 1;
                end = orexpr->expr->len - 1;
            }
            for (j = start; j < end; j++)
            {
                elem = orexpr->expr->list[j];
                if (elem->elemtype == SPT_VALUE &&
                        elem->u.value.valueclass == SVC_VARIABLE)
                {
                    ref = &elem->u.value.attrdesc.table;
                    break;
                }
            }
            if (j < end)
            {
                if (mdb_strcmp(ref->tablename, table->tablename))
                {
                    continue;
                }
                if (ref->aliasname && table->aliasname &&
                        mdb_strcmp(ref->aliasname, table->aliasname))
                {
                    continue;
                }
                orexpr->type |= ORT_SCANNABLE;
                type_change_count++;
            }
            else
            {
                /* No referenced table */
                MDB_SYSLOG(("NOTE: No referenced table.\n"));
            }
        }
    }

    return type_change_count;
}

/*****************************************************************************/
//! clear_joinonly_indexable_orexprs

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param nest():
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static void
clear_joinonly_indexable_orexprs(T_NESTING * nest)
{
    T_OR_EXPRDESC *orexpr;
    int i;

    for (i = 0; i < nest->indexable_orexpr_count; i++)
    {
        orexpr = nest->indexable_orexprs[i];

        if (orexpr->type & ORT_SCANNABLE)
            continue;

        if (orexpr->type & ORT_JOINABLE)
            orexpr->type &= ~ORT_INDEXABLE;     /* clear ORT_INDEXABLE */
    }
}


static int
compare_iscan_cost(T_ISCANITEM * iscan1, T_ISCANITEM * iscan2)
{
    /* iscan1 != NULL && iscan2 != NULL */
    /* compare cost of iscans contained in same nest */

    /* check match_ifield_count */
    if (iscan1->match_ifield_count != iscan2->match_ifield_count)
    {
        if (iscan1->match_ifield_count > iscan2->match_ifield_count)
            return -1;
        else
            return 1;
    }

    /* check if all index fields have equal operation */
    if (ISCAN_USE_ALLFLD_EQUAL(iscan1))
    {
        if (!ISCAN_USE_ALLFLD_EQUAL(iscan2))
            return -1;

        /* check unique index */
        if (ISCAN_USE_UNIQUE_INDEX(iscan1))
        {
            if (ISCAN_USE_UNIQUE_INDEX(iscan2))
                return 0;       /* similar cost */
            else
                return -1;
        }
        else
        {
            if (ISCAN_USE_UNIQUE_INDEX(iscan2))
                return 1;
            else
                return 0;       /* similar cost */
        }
    }
    else
    {
        if (ISCAN_USE_ALLFLD_EQUAL(iscan2))
            return 1;

        /* check filter_orexpr_count */
        if (iscan1->filter_orexpr_count != iscan2->filter_orexpr_count)
        {
            if (iscan1->filter_orexpr_count > iscan2->filter_orexpr_count)
                return -1;
            else
                return 1;
        }

        return 0;       /* similar cost */
    }
}

static int
choose_join_order(T_NESTING * l_nest, T_NESTING * r_nest)
{
    T_ISCANITEM *l_iscan = l_nest->iscan_list;
    T_ISCANITEM *r_iscan = r_nest->iscan_list;
    int l_count, r_count;
    int comp_res;

    /* Basic Principles:
       1) Position a table having smaller scan cost ahead.
       2) If the difference of two scan costs is not too large and
       the scan cost of the latter table becomes smaller by recomputing,
       then position the latter table ahead.
     */

    /* case 1) table scan : table scan */
    if (IS_SSCAN_FIXED(l_iscan) && IS_SSCAN_FIXED(r_iscan))
    {
        /* compare # of scannable orexprs */
        l_count = l_nest->total_orexpr_count - l_nest->joinable_orexpr_count;
        r_count = r_nest->total_orexpr_count - r_nest->joinable_orexpr_count;
        if (l_count != r_count)
        {
            if (l_count > r_count)
                return LEFT_TABLE_AHEAD;        /* -1 */
            else
                return RIGHT_TABLE_AHEAD;       /* 1 */
        }
        /* compare # of joinnable orexprs */
        if (l_nest->joinable_orexpr_count != r_nest->joinable_orexpr_count)
        {
            if (l_nest->joinable_orexpr_count > r_nest->joinable_orexpr_count)
                return LEFT_TABLE_AHEAD;        /* -1 */
            else
                return RIGHT_TABLE_AHEAD;       /* 1 */
        }
        return ANY_TABLE_AHEAD; /* 0 */
    }

    /* case 2) table scan : index scan */
    if (IS_SSCAN_FIXED(l_iscan))
    {
        if (l_nest->total_orexpr_count == 0)
            return RIGHT_TABLE_AHEAD;

        if (l_nest->joinable_orexpr_count == 0)
            return LEFT_TABLE_AHEAD;

        /*****************
        if (ISCAN_USE_ALLFLD_EQUAL(r_iscan))
            return RIGHT_TABLE_AHEAD;

        if (r_iscan->recompute_cost_flag == 0)
            return RIGHT_TABLE_AHEAD;
        *****************/

        if (r_iscan->match_ifield_count > 0)
            return RIGHT_TABLE_AHEAD;
        else
            return LEFT_TABLE_AHEAD;
    }

    /* case 3) index scan : table_scan */
    if (IS_SSCAN_FIXED(r_iscan))
    {
        if (r_nest->total_orexpr_count == 0)
            return LEFT_TABLE_AHEAD;

        if (r_nest->joinable_orexpr_count == 0)
            return RIGHT_TABLE_AHEAD;

        /*****************
        if (ISCAN_USE_ALLFLD_EQUAL(l_iscan))
            return LEFT_TABLE_AHEAD;

        if (l_iscan->recompute_cost_flag == 0)
            return LEFT_TABLE_AHEAD;
        *****************/

        if (l_iscan->match_ifield_count > 0)
            return LEFT_TABLE_AHEAD;
        else
            return RIGHT_TABLE_AHEAD;
    }

    /* case 4) index scan : index scan */
    comp_res = compare_iscan_cost(l_iscan, r_iscan);
    if (comp_res == 0)
    {
        /* compare the number of other filters */
        l_count = l_nest->total_orexpr_count - l_nest->indexable_orexpr_count;
        r_count = r_nest->total_orexpr_count - r_nest->indexable_orexpr_count;
        if (l_count == r_count)
        {
            if (l_iscan->recompute_cost_flag == r_iscan->recompute_cost_flag)
                return ANY_TABLE_AHEAD;

            if (l_iscan->recompute_cost_flag == 0)
                return LEFT_TABLE_AHEAD;
            else
                return RIGHT_TABLE_AHEAD;
        }
        if (l_count > r_count)
            comp_res = -1;
        else
            comp_res = 1;
    }

    if (comp_res == -1)
    {
        /* l_nest's iscan cost < r_nest's iscan_cost */

        if (l_iscan->recompute_cost_flag == r_iscan->recompute_cost_flag)
            return LEFT_TABLE_AHEAD;

        if (l_iscan->recompute_cost_flag == 0)
            return LEFT_TABLE_AHEAD;

        if (ISCAN_USE_ALLFLD_EQUAL(l_iscan))
            return LEFT_TABLE_AHEAD;
        else
            return RIGHT_TABLE_AHEAD;
    }
    else    /* comp_res == 1 */
    {
        /* l_nest's iscan cost > r_nest's iscan_cost */

        if (r_iscan->recompute_cost_flag == l_iscan->recompute_cost_flag)
            return RIGHT_TABLE_AHEAD;

        if (r_iscan->recompute_cost_flag == 0)
            return RIGHT_TABLE_AHEAD;

        if (ISCAN_USE_ALLFLD_EQUAL(r_iscan))
            return RIGHT_TABLE_AHEAD;
        else
            return LEFT_TABLE_AHEAD;
    }
}

/*****************************************************************************/
//! is_in_predicate

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param orexpr(in):
 ************************************
 * \return  TRUE/FALSE
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
int
is_in_predicate(T_OR_EXPRDESC * orexpr)
{
    T_POSTFIXELEM *op;

    op = orexpr->expr->list[orexpr->expr->len - 1];

    if (op->u.op.type == SOT_IN)
        return TRUE;

    return FALSE;
}

/*****************************************************************************/
//! compute_iscan_cost

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param nest(in/out):
 ************************************
 * \return  TRUE/FALSE
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static void
compute_iscan_cost(T_NESTING * nest)
{
    T_ISCANITEM *prev_iscan, *iscan;
    T_OR_EXPRDESC *orexpr, *temp_orexpr;
    T_ATTRDESC *attr;
    T_POSTFIXELEM *op;
    short match_orexpr_count = 0;
    short match_ifield_count = 0;
    short recompute_cost_flag = 0;
    char index_equal_op_flag = 0;
    int target, start;
    int j, idx;
    int attr_index;
    int first_ifield_index;

    if (nest->iscan_list == NULL)
        return;

    prev_iscan = NULL;
    iscan = nest->iscan_list;
    while (iscan)
    {
        if (iscan->recompute_cost_flag == 0)
        {
            prev_iscan = iscan;
            iscan = iscan->next;
            continue;
        }

        if (iscan->match_ifield_count == 0)
        {
            first_ifield_index = 0;
        }
        else
        {
            if (iscan->index_equal_op_flag)
                first_ifield_index = iscan->match_ifield_count;
            else
                first_ifield_index = iscan->match_ifield_count - 1;
        }

        target = iscan->match_orexpr_count;
        for (j = first_ifield_index; j < iscan->indexinfo.numFields; j++)
        {
            index_equal_op_flag = 0;
            recompute_cost_flag = 0;

            start = target;
            target = -1;
            for (idx = start; idx < iscan->index_orexpr_count; idx++)
            {
                orexpr = iscan->orexprs[idx];
                if (orexpr->type & ORT_REVERSED)
                {
                    attr_index = orexpr->expr->len - 2;
                }
                else
                {
                    attr_index = 0;
                }
                attr = &orexpr->expr->list[attr_index]->u.value.attrdesc;

                if (iscan->indexfields[j].fieldId != attr->Hattr
                        || (is_in_predicate(orexpr) && j > 0))
                {
                    if (target < 0)
                        target = idx;
                    continue;
                }

                /* iscan->indexfields[j].fieldId == attr->Hattr */
                if (orexpr->type & ORT_SCANNABLE)
                {
                    op = orexpr->expr->list[orexpr->expr->len - 1];
                    if (op->u.op.type == SOT_EQ)
                        index_equal_op_flag = 1;
                    if (target >= 0)
                    {   /* swapping */
                        temp_orexpr = iscan->orexprs[target];
                        iscan->orexprs[target] = iscan->orexprs[idx];
                        iscan->orexprs[idx] = temp_orexpr;
                        target++;
                    }
                }
                else
                {
                    if (target < 0)
                        target = idx;
                    recompute_cost_flag = 1;
                }
            }

            if (target == -1)
            {   /* All OR exprs matched */
                match_orexpr_count = iscan->index_orexpr_count;
                match_ifield_count = j + 1;
                break;
            }
            if (target == start)
            {   /* No OR exprs matched */
                match_orexpr_count = start;
                match_ifield_count = j;
                break;
            }
            /* target > start: Some OR exprs matched */
            if (index_equal_op_flag == 0)
            {
                match_orexpr_count = target;
                match_ifield_count = j + 1;
                break;
            }
            if (j == (iscan->indexinfo.numFields - 1))
            {
                match_orexpr_count = target;
                match_ifield_count = j + 1;
                break;
            }
        }

        if (match_orexpr_count > iscan->match_orexpr_count)
        {
            iscan->match_ifield_count = match_ifield_count;
            iscan->match_orexpr_count = match_orexpr_count;
            iscan->recompute_cost_flag = (int) recompute_cost_flag;
            iscan->index_equal_op_flag = index_equal_op_flag;

            iscan->filter_orexpr_count = iscan->unmatch_scan_count;
            for (j = match_orexpr_count; j < iscan->index_orexpr_count; j++)
            {
                orexpr = iscan->orexprs[j];
                if ((orexpr->type & ORT_INDEXABLE) &&
                        (orexpr->type & ORT_SCANNABLE))
                    iscan->filter_orexpr_count += 1;
            }

            if (iscan != nest->iscan_list &&
                    compare_iscan_cost(iscan, nest->iscan_list) < 0)
            {
                prev_iscan->next = iscan->next;
                iscan->next = nest->iscan_list;
                nest->iscan_list = iscan;

                iscan = prev_iscan->next;
                continue;
            }
        }
        else    /* match_orexpr_count == iscan->match_orexpr_count */
        {
            if (recompute_cost_flag == 0 && match_ifield_count == 0 &&
                    iscan->chosen_by_hint_flag == 0)
            {
                if (prev_iscan == NULL)
                    nest->iscan_list = iscan->next;
                else
                    prev_iscan->next = iscan->next;

                PMEM_FREENUL(iscan->indexfields);
                PMEM_FREENUL(iscan->orexprs);
                PMEM_FREENUL(iscan);

                if (prev_iscan == NULL)
                    iscan = nest->iscan_list;
                else
                    iscan = prev_iscan->next;

                continue;
            }
        }

        prev_iscan = iscan;
        iscan = iscan->next;
    }

    return;
}

/*****************************************************************************/
//!check_and_make_iscan

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param   connid(in):
 * \param   nest(in/out):
 * \param   join(in):
 ************************************
 * \return  TRUE/FALSE
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static int
check_and_make_iscan(int connid, T_NESTING * nest, T_JOINTABLEDESC * join)
{
    int result, i;
    T_OR_EXPRDESC *orexpr = NULL;
    T_POSTFIXELEM *op;
    T_POSTFIXELEM *kc;

    if (nest->indexable_orexpr_count == 0)
    {
        /* iscan cannot be possible. */
        return RET_SUCCESS;
    }

    /* check if iscan can be used. */
    for (i = 0; i < nest->indexable_orexpr_count; i++)
    {
        orexpr = nest->indexable_orexprs[i];
        if ((orexpr->type & ORT_JOINABLE) && (orexpr->type & ORT_SCANNABLE))
        {
            op = orexpr->expr->list[orexpr->expr->len - 1];
            if (op->u.op.type == SOT_EQ)
                break;
        }
    }

    if (i < nest->indexable_orexpr_count)
    {
        T_IDX_LIST_FIELDS indexfieldsinfo;
        T_IDX_FIELD indexfield;
        char indexname[REL_NAME_LENG];
        SYSINDEX_T indexinfo;
        int temp_serial_num;

        temp_serial_num = dbi_Trans_NextTempName(connid, 0, indexname, NULL);
        if (temp_serial_num < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
            return temp_serial_num;
        }

        if (orexpr->type & ORT_REVERSED)
            kc = orexpr->expr->list[orexpr->expr->len - 2];
        else
            kc = orexpr->expr->list[0];

        indexfieldsinfo.max = 1;
        indexfieldsinfo.len = 1;
        indexfieldsinfo.list = &indexfield;
        indexfieldsinfo.list[0].name = kc->u.value.attrdesc.attrname;
        indexfieldsinfo.list[0].ordertype = 'A';
        indexfieldsinfo.list[0].collation = kc->u.value.attrdesc.collation;

        if (1)
        {
            int keyins_flag = 1;
            SCANDESC_T *scandesc = NULL;
            int index_count;
            SYSINDEX_T *indexes;

            if (temp_serial_num <= 5)
            {
                /* get all indexes information */
                index_count = dbi_Schema_GetTableIndexesInfo(connid,
                        join->table.tablename, SCHEMA_ALL_INDEX, &indexes);
                if (index_count < 0)
                {
                    MDB_SYSLOG(("Getting All IndexesInfo of Table(%s) Fail.\n",
                                    join->table.tablename));
                    return index_count;
                }

                /* Build indexname, minkey, maxkey, and filter
                   for selecting records whose keys will be inserted
                   into the to-be-created temp index.
                 */
                result = make_scandesc_for_index_create(connid, join, nest,
                        index_count, indexes, &scandesc);

                PMEM_FREENUL(indexes);

                if (result < 0)
                {
                    MDB_SYSLOG(("Make ScanDesc For Index Create FAIL \n"));
                    /* go downward */
                    /* keyins_flag == 1 && scandesc == NULL */
                    /* create complete index */
                }
                else if (result == RET_FALSE)
                {
                    keyins_flag = 0;
                    /* go downward */
                    /* keyins_flag == 0 && scandesc == NULL */
                    /* create empty index (a sort of partial index) */
                }
            }

            if (keyins_flag == 0 || (keyins_flag && scandesc))
            {
                /* create temp partial index */
                /* change indexname : #TI_XXX => #PI_XXX */
                indexname[1] = 'p';
            }

            result = create_temp_index(connid, &nest->table, &indexfieldsinfo,
                    indexname, keyins_flag, scandesc);

            nest->keyins_flag = keyins_flag;
            nest->scandesc = scandesc;
        }
        if (result != SQL_E_NOERROR)
        {
            return result;
        }

        result = dbi_Schema_GetIndexInfo(connid, indexname, &indexinfo,
                NULL, NULL);
        if (result < 0)
        {
            (void) drop_temp_index(connid, indexname);
            return result;
        }

        /* last argument, chosen_by_hint, is 0. */
        result = build_iscan_list(connid, nest, 1, &indexinfo, 0);
        if (result < 0)
        {
            (void) drop_temp_index(connid, indexname);
            return result;
        }

        compute_iscan_cost(nest);
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! swap_minmax

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param indexfield(in): index_field info
 * \param min(in/out)   : min keyvalue
 * \param max(in/out)   : max keyvalue
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
swap_minmax(SYSINDEXFIELD_T * indexfield,
        T_KEY_EXPRDESC * min, T_KEY_EXPRDESC * max)
{
    T_KEY_EXPRDESC temp_desc;

    if (indexfield->order == 'D')
    {
        temp_desc = *min;
        *min = *max;
        *max = temp_desc;
    }
    return RET_SUCCESS;
}

/*****************************************************************************/
//! set_opentype_for_select

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param select(in/out)    : index_field info
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
set_opentype_for_select(T_SELECT * select)
{
    select->planquerydesc.nest[0].opentype = ASCENDING;

    if (select->planquerydesc.querydesc.fromlist.len != 1)
        return RET_SUCCESS;

    if (select->planquerydesc.querydesc.orderby.len > 0)
    {
        T_ORDERBYDESC *orderby = &select->planquerydesc.querydesc.orderby;
        T_ISCANITEM *iscan = select->planquerydesc.nest[0].iscan_list;

        if (IS_ISCAN(iscan)
                && orderby->list[0].ordertype != iscan->indexfields[0].order)
            select->planquerydesc.nest[0].opentype = DESCENDING;
    }

    return RET_SUCCESS;
}


/*****************************************************************************/
//! set_indexrange_for_select

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid(in)    :
 * \param nest(in/out)  : table로의 접근 정보
 * \param numfields(in) : 접근 table의 필드 갯수
 * \param fieldinfo(in) : 접근 table의 필드 정보
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - called : arrange_rttableorder()
 *****************************************************************************/
static int
set_indexrange(int connid, T_NESTING * nest, int numfields,
        SYSFIELD_T * fieldinfo)
{
    T_ISCANITEM *iscan = nest->iscan_list;
    int ret, i;
    ibool is_temp = 0;
    ibool is_like;
    unsigned char *op_like, bit = 0x01;

    if (IS_ISCAN(iscan))        /* index scan */
    {
        if (isTempIndex_name(iscan->indexinfo.indexName))
            is_temp = 1;
        ret = set_indexinfo_for_nest(connid, nest, (T_INDEXINFO *) iscan,
                is_temp, numfields, fieldinfo);
        if (ret != RET_SUCCESS)
        {
            return ret;
        }

        /* for single row operation, check index */
        if ((iscan->indexinfo.type & UNIQUE_INDEX_BIT) != 0)
            nest->cnt_single_item = 0;

        op_like = (unsigned char *) &nest->op_like;

        for (i = 0; i < iscan->match_ifield_count; i++)
        {
            is_like = 0;

            ret = make_lu_limit(connid, i, nest, &is_like);
            if (ret <= RET_ERROR)
                return ret;
            else if (ret == RET_END)
                break;
            if (is_like)
                *op_like |= bit;
            if (!((bit <<= 1) & 0xFF))
            {
                bit = 0x01;
                op_like++;
            }
            swap_minmax(&iscan->indexfields[i], &nest->min[i], &nest->max[i]);
        }
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! arrange_rttableorder

/*! \breif  Scan 정보를 설정함
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in) : DB HANDLE
 * \param select(in/out) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
arrange_rttableorder(int handle, T_SELECT * select)
{
    T_RTTABLE *rttable;
    T_NESTING *nest;
    T_LIST_JOINTABLEDESC *fromtables;
    int from_index_forrun[MAX_JOIN_TABLE_NUM];
    int n_tables, nj_num, fj_num;
    int i, j, ret;
    int temp_idx, count;

#ifdef MDB_DEBUG
    sc_assert(select != NULL, __FILE__, __LINE__);
#endif

    if (select->first_sub)
    {
        ret = arrange_rttableorder(handle, select->first_sub);
        if (ret != RET_SUCCESS)
            return ret;
    }
    if (select->sibling_query)
    {
        ret = arrange_rttableorder(handle, select->sibling_query);
        if (ret != RET_SUCCESS)
            return ret;
    }

    if (select->planquerydesc.querydesc.setlist.len > 0)
    {
        T_EXPRESSIONDESC *setlist;

        setlist = &select->planquerydesc.querydesc.setlist;

        for (i = 0; i < setlist->len; i++)
        {
            if (setlist->list[i]->elemtype == SPT_SUBQUERY)
            {
                ret = arrange_rttableorder(handle, setlist->list[i]->u.subq);
                if (ret != RET_SUCCESS)
                {
                    return ret;
                }
            }
        }
        return RET_SUCCESS;
    }

    rttable = select->rttable;
    nest = select->planquerydesc.nest;
    fromtables = &select->planquerydesc.querydesc.fromlist;
    n_tables = fromtables->len;

    select->queryresult.include_rid = 0;

    if (n_tables == 1)  /* OrderBy Optimization */
    {
        T_QUERYDESC *qdesc = &select->planquerydesc.querydesc;
        T_SCANHINT *scan_hint = &fromtables->list[0].scan_hint;

        /*********************************************
        qdesc->orderby.len > 0           : ORDER BY is used
        scan_hint->scan_type == SMT_NONE : no scan hint
        nest[0].attr_expr_list == NULL   : no search predicate
        *********************************************/

        while (qdesc->orderby.len > 0 && scan_hint->scan_type == SMT_NONE
                && nest[0].attr_expr_list == NULL)
        {
            for (i = 0; i < qdesc->orderby.len; i++)
            {
                if (qdesc->orderby.list[i].s_ref >= 0)
                {
                    if (qdesc->selection.list[qdesc->orderby.list[i].s_ref].
                            expr.len > 1)
                        break;
                }
            }

            if (i < qdesc->orderby.len)
                break;

            scan_hint->fields.list =
                    PMEM_XCALLOC(sizeof(T_IDX_FIELD) * qdesc->orderby.len);
            if (scan_hint->fields.list == NULL)
            {
                /* Memory Allocation Fail */
                break;  /* cannot make scan hint */
            }

            for (i = 0; i < qdesc->orderby.len; i++)
            {
#ifdef MDB_DEBUG
                if (qdesc->orderby.list[i].ordertype != 'A'
                        && qdesc->orderby.list[i].ordertype != 'D')
                    sc_assert(0, __FILE__, __LINE__);
#endif

                scan_hint->fields.list[i].ordertype =
                        qdesc->orderby.list[i].ordertype;
                scan_hint->fields.list[i].collation =
                        qdesc->orderby.list[i].attr.collation;

                scan_hint->fields.list[i].name
                        = PMEM_STRDUP(qdesc->orderby.list[i].attr.attrname);
                if (scan_hint->fields.list[i].name == NULL)
                    break;

            }
            if (i < qdesc->orderby.len)
            {
                /* Memory Allocation Fail */
                for (j = 0; j < i; j++)
                    PMEM_FREENUL(scan_hint->fields.list[j].name);
                PMEM_FREENUL(scan_hint->fields.list);
                break;  /* cannot make scan hint */
            }

            /* set scan hint */
            scan_hint->fields.len = qdesc->orderby.len;
            scan_hint->fields.max = qdesc->orderby.len;
            scan_hint->indexname = NULL;
            scan_hint->scan_type = SMT_ISCAN;
        }

        /* just query about single table can be allowed for rid */
        if ((fromtables[0].list[0].tableinfo.flag & _CONT_VIEW) == 0)
            select->queryresult.include_rid = 1;
    }

    /*******************************************************************
    Definition)
      - from join table   : join-cond is specified in FROM clause.
      - natural join table: join-cond is specified in WHERE clause.

    Procedure)
      1. Compute scan cost using scannable OR exprs for each table.
      2. Position natural join(nj) tables ahead than from join(fj) tables.
      3. Make Join order of all nj tables
         while finding minimum scan method of each nj tables.
         For each nj table, the scan cost of each scan method
         is recomputed considering the fixed partial join order.
      4. For each fj table, the scan cost of each scan method
         is recomputed considering the fixed partial join order.
      5. Build minimum scan method for each table.
    ********************************************************************/

    /* (step 1)
     * Compute scan cost using scannable OR exprs for each table.
     */
    for (i = 0; i < n_tables; i++)
    {
        ret = make_scan_info(handle, &nest[i], &fromtables->list[i]);
        if (ret != RET_SUCCESS)
        {
            goto end;
        }
        if (select->super_query)
        {
            set_scannable_super_query_ref_orexprs(&nest[i], select);
        }
        compute_iscan_cost(&nest[i]);
    }

    /* (step 2)
     * Position natural join(nj) tables ahead than from join(fj) tables.
     */
    for (nj_num = 0, i = 0; i < n_tables; i++)
    {
        if (IS_FROM_JOIN_TABLE(fromtables->list[i].join_type))
            continue;
        if ((i + 1 < n_tables) &&
                IS_FROM_JOIN_TABLE(fromtables->list[i + 1].join_type))
            continue;

        from_index_forrun[nj_num++] = i;
    }
    if (nj_num < n_tables)
    {
        for (fj_num = nj_num, i = 0; i < n_tables; i++)
        {
            if (i + 1 < n_tables)
            {
                if (!IS_FROM_JOIN_TABLE(fromtables->list[i].join_type) &&
                        !IS_FROM_JOIN_TABLE(fromtables->list[i + 1].join_type))
                    continue;
            }
            else
            {
                if (!IS_FROM_JOIN_TABLE(fromtables->list[i].join_type))
                    continue;
            }
            from_index_forrun[fj_num++] = i;
        }
    }

    /* (step 3)
     * Make Join order of all nj tables
     * while finding minimum scan method of each nj tables.
     * For each nj table, the scan cost of each scan method
     * is recomputed considering the fixed partial join order.
     */
    for (i = 0; i < nj_num; i++)
    {
        for (j = i + 1; j < nj_num; j++)
        {
            if (choose_join_order(&nest[from_index_forrun[j]],
                            &nest[from_index_forrun[i]]) == LEFT_TABLE_AHEAD)
            {   /* swapping */
                temp_idx = from_index_forrun[j];
                from_index_forrun[j] = from_index_forrun[i];
                from_index_forrun[i] = temp_idx;
            }
        }

        if (IS_SSCAN(nest[from_index_forrun[i]].iscan_list))
            (void) check_and_make_iscan(handle, &nest[from_index_forrun[i]],
                    &fromtables->list[from_index_forrun[i]]);

        for (j = i + 1; j < n_tables; j++)
        {
            count = set_scannable_fixed_table_ref_orexprs(&nest
                    [from_index_forrun[j]], &nest[from_index_forrun[i]].table);
            if (count > 0)
                compute_iscan_cost(&nest[from_index_forrun[j]]);
        }

        clear_joinonly_indexable_orexprs(&nest[from_index_forrun[i]]);
    }

    /* (step 4)
     * For each fj table, the scan cost of each scan method
     * is recomputed considering the fixed partial join order.
     */
    for (i = nj_num; i < n_tables; i++)
    {
        if (IS_SSCAN(nest[from_index_forrun[i]].iscan_list))
            (void) check_and_make_iscan(handle, &nest[from_index_forrun[i]],
                    &fromtables->list[from_index_forrun[i]]);

        for (j = i + 1; j < n_tables; j++)
        {
            count = set_scannable_fixed_table_ref_orexprs(&nest
                    [from_index_forrun[j]], &nest[from_index_forrun[i]].table);
            if (count > 0)
                compute_iscan_cost(&nest[from_index_forrun[j]]);
        }

        clear_joinonly_indexable_orexprs(&nest[from_index_forrun[i]]);
    }

    /* (step 5)
     * Build minimum scan method for each table.
     */
    for (i = 0; i < n_tables; i++)
    {
        ret = set_indexrange(handle, &nest[i],
                fromtables->list[i].tableinfo.numFields,
                fromtables->list[i].fieldinfo);
        if (ret != RET_SUCCESS)
        {
            goto end;
        }
    }

    if (n_tables == 1)
        set_opentype_for_select(select);

    ret = make_rttableorder(handle, rttable, nest, fromtables,
            from_index_forrun, select->planquerydesc.querydesc.select_star);

    if (select->msync_flag)
    {
        rttable->msync_flag = select->msync_flag;
    }

  end:
    for (i = 0; i < n_tables; i++)
    {
        free_scan_info(&nest[i]);
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
void
drop_unbindable_expr(T_NESTING * nest, T_EXPRESSIONDESC * unbindable_expr)
{
    EXPRDESC_LIST *cur, *prev;
    T_EXPRESSIONDESC *expr;

    for (cur = nest->attr_expr_list, prev = cur; cur;)
    {
        expr = ((T_OR_EXPRDESC *) cur->ptr)->expr;
        if (expr == unbindable_expr)
        {
            if (prev == cur)
            {
                nest->attr_expr_list = cur->next;
                PMEM_FREENUL(cur->ptr);
                PMEM_FREENUL(cur);
                cur = nest->attr_expr_list;
                prev = cur;
            }
            else
            {
                prev->next = cur->next;
                PMEM_FREENUL(cur->ptr);
                PMEM_FREENUL(cur);
                cur = prev->next;
            }
        }
        else
        {
            prev = cur;
            cur = cur->next;
        }
    }
}

/*****************************************************************************/
//! ok_likefilterable

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param expr(in)  :
 ************************************
 * \return  TRUE/FALSE
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
int
ok_likefilterable(T_EXPRESSIONDESC * expr)
{
    if (expr->len == 3 &&
            expr->list[2]->elemtype == SPT_OPERATOR &&
            (expr->list[2]->u.op.type == SOT_LIKE
                    || expr->list[2]->u.op.type == SOT_ILIKE) &&
            !IS_BS_OR_NBS(expr->list[0]->u.value.attrdesc.type))
    {
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************/
//! ok_ridfilterable

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param expr(in)  :
 ************************************
 * \return  TRUE/FALSE
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
int
ok_ridfilterable(T_EXPRESSIONDESC * expr)
{
    if (expr->len == 3 &&
            expr->list[2]->elemtype == SPT_OPERATOR &&
            expr->list[2]->u.op.type == SOT_EQ &&
            (expr->list[0]->u.value.attrdesc.type == DT_OID ||
                    expr->list[1]->u.value.attrdesc.type == DT_OID))
    {
        return TRUE;
    }

    return FALSE;
}

/*****************************************************************************/
//! separate_exprs_by_attr

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param nest(in/out)      :
 * \param rrtable(in)   :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
int
separate_exprs_by_attr(T_NESTING * nest, T_RTTABLE * rttable)
{
    ATTRED_EXPRDESC_LIST *cur;
    T_EXPRESSIONDESC *expr;
    T_POSTFIXELEM *op, *elem = NULL;
    T_ORTYPE type;
    int is_selected_rid = 0;
    int ret;

    for (cur = nest->attr_expr_list; cur; cur = cur->next)
    {
        expr = ((T_OR_EXPRDESC *) cur->ptr)->expr;
        type = ((T_OR_EXPRDESC *) cur->ptr)->type;
        if (type & ORT_INDEXRANGED)
            continue;
        if (type & ORT_REVERSED)
        {
            elem = expr->list[0];
            expr->list[0] = expr->list[1];
            expr->list[1] = elem;
            op = expr->list[2];
            op->u.op.type = get_commute_op(op->u.op.type);
        }

        if (nest->scanrid != NULL_OID && ok_ridfilterable(expr)
                && !is_selected_rid)
        {
            is_selected_rid = 1;
            continue;
        }
        else if ((type & ORT_FILTERABLE) && ok_likefilterable(expr)
                && !ok_ridfilterable(expr))
            LINK_EXPRDESC_LIST(rttable->db_expr_list, expr, ret);
        else
            LINK_EXPRDESC_LIST(rttable->sql_expr_list, expr, ret);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return RET_ERROR;
        }
    }

    return RET_SUCCESS;
}

extern void set_super_tbl_correlated(T_SELECT * select);

/*****************************************************************************/
//! link_correl_attr_values

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param select():
 * \param expr():
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
int
link_correl_attr_values(T_SELECT * select, T_EXPRESSIONDESC * expr)
{
    T_POSTFIXELEM *elem;
    CORRELATED_ATTR *cl;
    int i;

    for (i = 0; i < expr->len; i++)
    {
        elem = expr->list[i];

        if (elem->elemtype == SPT_VALUE &&
                elem->u.value.attrdesc.table.correlated_tbl != NULL &&
                elem->u.value.attrdesc.table.correlated_tbl != select)
        {
            cl = (CORRELATED_ATTR *) PMEM_ALLOC(sizeof(CORRELATED_ATTR));
            if (cl == NULL)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return SQL_E_OUTOFMEMORY;
            }
            cl->next = select->planquerydesc.querydesc.correl_attr;
            cl->ptr = (void *) elem;
            select->planquerydesc.querydesc.correl_attr = cl;
            select->kindofwTable |= iTABLE_CORRELATED;
            set_super_tbl_correlated(select);
        }
    }
    return RET_SUCCESS;
}           /* link_correl_attr_values */

/*****************************************************************************/
//! determine_eval_expr

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)        :
 * \param select(in/out)    :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
int
determine_eval_expr(int handle, T_SELECT * select)
{
    T_RTTABLE *rttable;
    T_NESTING *nest;
    T_QUERYDESC *qdesc;
    EXPRDESC_LIST *cur;
    T_EXPRESSIONDESC *expr;
    int n_tables;
    int f_index, r_index;
    int i, j, ret;
    int run, ord;
    int rec_cnt;

#ifdef MDB_DEBUG
    sc_assert(select != NULL, __FILE__, __LINE__);
#endif
    if (select->first_sub &&
            (ret = determine_eval_expr(handle, select->first_sub))
            != RET_SUCCESS)
        return ret;
    if (select->sibling_query &&
            (ret = determine_eval_expr(handle, select->sibling_query))
            != RET_SUCCESS)
        return ret;

    qdesc = &select->planquerydesc.querydesc;
    n_tables = qdesc->fromlist.len;
    nest = select->planquerydesc.nest;
    rttable = select->rttable;

    if (qdesc->setlist.len > 0)
    {
        for (i = 0; i < qdesc->setlist.len; i++)
        {
            if (qdesc->setlist.list[i]->elemtype == SPT_SUBQUERY)
            {
                ret = determine_eval_expr(handle,
                        qdesc->setlist.list[i]->u.subq);
                if (ret != RET_SUCCESS)
                {
                    return ret;
                }
            }
        }
        return RET_SUCCESS;
    }

    for (i = 0; i < n_tables; i++)
    {
        f_index = rttable[n_tables - i - 1].table_ordinal;
        for (cur = nest[f_index].attr_expr_list; cur; cur = cur->next)
        {
            expr = ((T_OR_EXPRDESC *) cur->ptr)->expr;
            for (j = 0; j < n_tables - i - 1; j++)
            {
                r_index = rttable[j].table_ordinal;
                drop_unbindable_expr(&nest[r_index], expr);
            }
        }
    }
    for (i = 0; i < n_tables; i++)
    {
        f_index = rttable[i].table_ordinal;
        ret = separate_exprs_by_attr(&nest[f_index], &rttable[i]);
        if (ret != RET_SUCCESS)
            return ret;
    }
    if (select->super_query)
    {
        for (i = 0; i < n_tables; i++)
        {
            for (j = 0; j < nest[i].index_field_num; j++)
            {
                ret = link_correl_attr_values(select, &nest[i].min[j].expr);
                if (ret < 0)
                {
                    return RET_ERROR;
                }
                ret = link_correl_attr_values(select, &nest[i].max[j].expr);
                if (ret < 0)
                {
                    return RET_ERROR;
                }
            }
            for (cur = nest[i].attr_expr_list; cur; cur = cur->next)
            {
                expr = ((T_OR_EXPRDESC *) cur->ptr)->expr;
                if (link_correl_attr_values(select, expr) < 0)
                    return RET_ERROR;
            }
        }
    }

    for (run = 0; run < n_tables; run++)
    {
        ord = rttable[run].table_ordinal;

        /* make cnt_merge_item and merge_key_values */
        if (run == 0)   /* no sort merge for first run table */
            rttable[run].cnt_merge_item = 0;
        else
            rttable[run].cnt_merge_item = nest[ord].cnt_merge_item;

        if (rttable[run].cnt_merge_item)
        {
            T_EXPRESSIONDESC *key;
            T_VALUEDESC *val;
            T_TABLEDESC *ref;
            T_TABLEDESC *tab;
            int kc;
            int outer_run = 0;

            /* sort-merge를 사용해야 하는 경우라면,
             * key를 구성하는 expression내의 variable이
             * index member인지 확인하자.
             */
            for (kc = 0; kc < rttable[run].cnt_merge_item; kc++)
            {
                key = &nest[ord].min[kc].expr;

                if (key->len > 1)
                {
                    /* must have only one attribute reference */
                    /* for example, A.a = B.b */
                    /* In case of A.a = B.b + 100, do not use sort merge join */
                    rttable[run].cnt_merge_item = 0;
                    break;
                }

                if (key->list[0]->elemtype != SPT_VALUE ||
                        key->list[0]->u.value.valueclass != SVC_VARIABLE)
                {
                    rttable[run].cnt_merge_item = 0;
                    break;
                }

                /* find the outer table to be merged with */
                val = &key->list[0]->u.value;
                ref = &val->attrdesc.table;

                for (j = 0; j < run; j++)
                {
                    f_index = rttable[j].table_ordinal;
                    if (!nest[f_index].index_field_info)
                        continue;
                    tab = &nest[f_index].table;
                    if (mdb_strcmp(ref->tablename, tab->tablename))
                    {
                        continue;
                    }
                    if (ref->aliasname && tab->aliasname)
                    {
                        if (mdb_strcmp(ref->aliasname, tab->aliasname))
                        {
                            continue;
                        }
                    }
                    else
                    {
                        if (ref->aliasname || tab->aliasname)
                            continue;
                    }

                    if (kc == 0)
                    {
                        if (nest[f_index].index_field_num
                                != rttable[run].cnt_merge_item)
                        {
                            rttable[run].cnt_merge_item = 0;
                            break;
                        }

                        if (rttable[j].db_expr_list
                                || rttable[j].sql_expr_list)
                        {
                            /* outer table must have no filters */
                            rttable[run].cnt_merge_item = 0;
                            break;
                        }

                        outer_run = j;
                    }
                    else
                    {
                        if (j != outer_run)
                        {
                            /* outer table is not same table */
                            rttable[run].cnt_merge_item = 0;
                            break;
                        }
                    }

                    if (val->attrdesc.Hattr
                            != nest[f_index].index_field_info[kc].Hattr)
                    {
                        /* 해당 table index의 첫번째 member가 아니라면,
                         * nested-loop로 변경한다. */
                        rttable[run].cnt_merge_item = 0;
                        break;
                    }

                    break;
                }
                if (j == run)
                    rttable[run].cnt_merge_item = 0;
                if (!rttable[run].cnt_merge_item)
                    break;
            }

            if (rttable[run].cnt_merge_item)
            {
                /* make merge_key_values */
                rttable[run].merge_key_values = (T_VALUEDESC *)
                        PMEM_ALLOC(sizeof(T_VALUEDESC) *
                        rttable[run].cnt_merge_item);
                if (rttable[run].merge_key_values == NULL)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_OUTOFMEMORY, 0);
                    return RET_ERROR;
                }

                rttable[run].prev_rec_max_cnt = PREV_REC_MAX_CNT;

                rttable[run].prev_rec_values =
                        sql_xcalloc(sizeof(FIELDVALUES_T) * PREV_REC_MAX_CNT,
                        DT_NULL_TYPE);

                if (rttable[run].prev_rec_values == NULL)
                    return RET_ERROR;

                for (rec_cnt = 0; rec_cnt < PREV_REC_MAX_CNT; rec_cnt++)
                {
                    if (copy_rec_values(&rttable[run].prev_rec_values[rec_cnt],
                                    rttable[run].rec_values) != RET_SUCCESS)
                        return RET_ERROR;

                }

            }
        }
    }

    if (IS_SINGLE_OP(nest) &&
            (rttable->db_expr_list || rttable->sql_expr_list))
        nest->cnt_single_item = 0;
    return RET_SUCCESS;

}           /* determine_eval_expr */

///////////////////////// Util Functions End /////////////////////////


//////////////////////////// Main Function ////////////////////////////
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
append_subselect_string(_T_STRING * p, T_SELECT * select)
{
    T_NESTING *nest, *curnest;
    T_RTTABLE *rttable;
    T_ATTRDESC *attr;
    EXPRDESC_LIST *cur;
    char tbuf[STRING_BLOCK_SIZE];
    int n_tables, ord, i, j;

    nest = select->planquerydesc.nest;
    rttable = select->rttable;
    n_tables = select->planquerydesc.querydesc.fromlist.len;

    for (i = 0; i < n_tables; i++)
    {
        if (i > 0)
        {
            if (!rttable[i].cnt_merge_item)
            {
                append_nullstring(p, MDB_NEWLINE);
                append_nullstring(p, "\t\tNESTED LOOP");
            }
            else
            {
                append_nullstring(p, MDB_NEWLINE);
                append_nullstring(p, "\t\tSORT MERGE (< ");
                ord = rttable[i - 1].table_ordinal;
                for (j = 0; j < nest[ord].index_field_num; j++)
                {
                    if (j > 0)
                        append_nullstring(p, ", ");
                    attr = &nest[ord].index_field_info[j];
                    sc_sprintf(tbuf, "%s.%s", (attr->table.aliasname ?
                                    attr->table.aliasname : attr->table.
                                    tablename), attr->attrname);
                    append_nullstring(p, tbuf);
                }
                append_nullstring(p, " >, < ");
                ord = rttable[i].table_ordinal;
                for (j = 0; j < nest[ord].index_field_num; j++)
                {
                    if (j > 0)
                        append_nullstring(p, ", ");
                    attr = &nest[ord].index_field_info[j];
                    sc_sprintf(tbuf, "%s.%s", (attr->table.aliasname ?
                                    attr->table.aliasname : attr->table.
                                    tablename), attr->attrname);
                    append_nullstring(p, tbuf);
                }
                append_nullstring(p, " >)");
            }
        }
        sc_sprintf(tbuf, "%s[%d] %s %s ", MDB_NEWLINE, i + 1,
                rttable[i].table.tablename,
                (rttable[i].table.aliasname ? rttable[i].table.
                        aliasname : ""));
        append_nullstring(p, tbuf);

        curnest = &nest[rttable[i].table_ordinal];
        if (curnest->scanrid != NULL_OID)
        {
            if (curnest->ridin_cnt)
            {
                append_nullstring(p, MDB_NEWLINE);
                append_nullstring(p, "\tRSCAN");
            }
            else
            {
                sc_sprintf(tbuf, "%s\tRSCAN %s.rid = %d", MDB_NEWLINE,
                        rttable[i].table.tablename, curnest->scanrid);
                append_nullstring(p, tbuf);
            }
        }
        else if (curnest->indexname == NULL)
        {
            append_nullstring(p, MDB_NEWLINE);
            append_nullstring(p, "\tSSCAN");
        }
        else
        {
            append_nullstring(p, MDB_NEWLINE);
            append_nullstring(p, "\tISCAN RANGE FROM <");
            for (j = 0; j < curnest->index_field_num; j++)
            {
                if (j > 0)
                    append_nullstring(p, ",");
                if (curnest->min[j].expr.len == 0)
                {
                    append_nullstring(p, " ?");
                }
                else
                {
                    append_expression_string(p, &curnest->min[j].expr);
                }
            }
            append_nullstring(p, " > To <");
            for (j = 0; j < curnest->index_field_num; j++)
            {
                if (j > 0)
                    append_nullstring(p, ",");
                if (curnest->max[j].expr.len == 0)
                {
                    append_nullstring(p, " ?");
                }
                else
                {
                    append_expression_string(p, &curnest->max[j].expr);
                }
            }
            sc_sprintf(tbuf, " > ON INDEX %s", curnest->indexname);
            append_nullstring(p, tbuf);
        }
        if (rttable[i].db_expr_list)
        {
            append_nullstring(p, MDB_NEWLINE);
            append_nullstring(p, "\t\tFILTER");
            for (cur = rttable[i].db_expr_list; cur; cur = cur->next)
            {
                if (cur != rttable[i].db_expr_list)
                    append_nullstring(p, " and ");
                append_expression_string(p, cur->ptr);
            }
        }
        if (rttable[i].sql_expr_list)
        {
            append_nullstring(p, MDB_NEWLINE);
            append_nullstring(p, "\tCOND");
            for (cur = rttable[i].sql_expr_list; cur; cur = cur->next)
            {
                if (cur != rttable[i].sql_expr_list)
                    append_nullstring(p, " and ");
                append_expression_string(p, cur->ptr);
            }
        }
    }

    if (select->planquerydesc.querydesc.fromlist.outer_join_exists &&
            select->planquerydesc.querydesc.expr_list)
    {
        sc_sprintf(tbuf, "%s[%d] cond ", MDB_NEWLINE, n_tables + 1);
        append_nullstring(p, tbuf);
        for (cur = select->planquerydesc.querydesc.expr_list;
                cur; cur = cur->next)
        {
            if (cur != select->planquerydesc.querydesc.expr_list)
                append_nullstring(p, " and ");
            append_expression_string(p, cur->ptr);
        }
    }

    if (select->kindofwTable & iTABLE_SUBSELECT)
    {
        char *label =
                select->super_query->rttable[select->rttable_idx].table.
                tablename;
        append_nullstring(p, MDB_NEWLINE);
        append_nullstring(p, "\tRESULT ");
        append_nullstring(p, (label ? label : "?"));
    }
    else if (select->kindofwTable & iTABLE_SUBWHERE)
    {
        append_nullstring(p, MDB_NEWLINE);
        append_nullstring(p, "\tRESULT ?");
    }
    else if (select->tmp_sub)
    {
        if (select->tmp_sub->rttable->table.tablename ||
                select->planquerydesc.querydesc.distinct.len > 0)
        {
            append_nullstring(p, MDB_NEWLINE);
            append_nullstring(p, "\tRESULT ?");
        }

        if (select->planquerydesc.querydesc.distinct.len > 0)
        {
            for (j = 0; j < select->planquerydesc.querydesc.distinct.len
                    && select->planquerydesc.querydesc.distinct.distinct_table;
                    j++)
            {
                append_nullstring(p, MDB_NEWLINE);
                sc_sprintf(tbuf, "%s[%d] %s ", MDB_NEWLINE, i + j + 1,
                        select->planquerydesc.querydesc.distinct.
                        distinct_table[j].table);
                append_nullstring(p, tbuf);
                if (select->planquerydesc.querydesc.distinct.distinct_table[j].
                        index)
                {
                    append_nullstring(p, MDB_NEWLINE);
                    append_nullstring(p,
                            "\tISCAN RANGE FROM < > To < > ON INDEX ");
                    sc_sprintf(tbuf, "%s ",
                            select->planquerydesc.querydesc.distinct.
                            distinct_table[j].index);
                    append_nullstring(p, tbuf);
                }
            }
        }

    }

    append_nullstring(p, MDB_NEWLINE);
    append_nullstring(p, MDB_NEWLINE);
}

void
append_subselect_temptable(_T_STRING * p, T_SELECT * select)
{
    if (select && select->wTable)
    {
        append_nullstring(p, select->wTable);
        append_nullstring(p, "\t");
    }
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
append_select_string(_T_STRING * p, T_SELECT * select)
{
    T_SELECT *sub_select;

    sub_select = select->first_sub;
    while (sub_select)
    {
        append_select_string(p, sub_select);
        sub_select = sub_select->sibling_query;
    }
    append_subselect_string(p, select);
    if (select->tmp_sub && select->tmp_sub->rttable->table.tablename)
        append_select_string(p, select->tmp_sub);
}

void
append_delete_string(_T_STRING * p, T_DELETE * delete)
{
    T_NESTING *nest, *curnest;
    T_RTTABLE *rttable;
    EXPRDESC_LIST *cur;
    char tbuf[STRING_BLOCK_SIZE];
    int j;
    T_SELECT *sub_select;

    sub_select = &delete->subselect;

    while (sub_select)
    {
        append_select_string(p, sub_select);
        sub_select = sub_select->sibling_query;
    }

    rttable = &delete->rttable[0];
    nest = delete->planquerydesc.nest;

    sc_sprintf(tbuf, "%s[%d] %s", MDB_NEWLINE, 1, nest->table.tablename);
    append_nullstring(p, tbuf);

    curnest = &nest[0];
    if (curnest->scanrid != NULL_OID)
    {
        if (curnest->ridin_cnt)
        {
            append_nullstring(p, MDB_NEWLINE);
            append_nullstring(p, "\tRSCAN");
        }
        else
        {
            sc_sprintf(tbuf, "%s\tRSCAN %s.rid = %d", MDB_NEWLINE,
                    rttable->table.tablename, curnest->scanrid);
            append_nullstring(p, tbuf);
        }
    }
    else if (curnest->indexname == NULL)
    {
        append_nullstring(p, MDB_NEWLINE);
        append_nullstring(p, "\tSSCAN");
    }
    else
    {
        append_nullstring(p, MDB_NEWLINE);
        append_nullstring(p, "\tISCAN RANGE FROM <");
        for (j = 0; j < curnest->index_field_num; j++)
        {
            if (j > 0)
                append_nullstring(p, ",");
            if (curnest->min[j].expr.len == 0)
            {
                append_nullstring(p, " ?");
            }
            else
            {
                append_expression_string(p, &curnest->min[j].expr);
            }
        }
        append_nullstring(p, " > To <");
        for (j = 0; j < curnest->index_field_num; j++)
        {
            if (j > 0)
                append_nullstring(p, ",");
            if (curnest->max[j].expr.len == 0)
            {
                append_nullstring(p, " ?");
            }
            else
            {
                append_expression_string(p, &curnest->max[j].expr);
            }
        }
        sc_sprintf(tbuf, " > ON INDEX %s", curnest->indexname);
        append_nullstring(p, tbuf);
    }

    if (rttable->db_expr_list)
    {
        append_nullstring(p, MDB_NEWLINE);
        append_nullstring(p, "\t\tFILTER");
        for (cur = rttable->db_expr_list; cur; cur = cur->next)
        {
            if (cur != rttable->db_expr_list)
                append_nullstring(p, " and ");
            append_expression_string(p, cur->ptr);
        }
    }
    if (rttable->sql_expr_list)
    {
        append_nullstring(p, MDB_NEWLINE);
        append_nullstring(p, "\tCOND");
        for (cur = rttable->sql_expr_list; cur; cur = cur->next)
        {
            if (cur != rttable->sql_expr_list)
                append_nullstring(p, " and ");
            append_expression_string(p, cur->ptr);
        }
    }

    append_nullstring(p, MDB_NEWLINE);
    append_nullstring(p, MDB_NEWLINE);
}

void
append_update_string(_T_STRING * p, T_UPDATE * update)
{
    T_NESTING *nest, *curnest;
    T_RTTABLE *rttable;
    EXPRDESC_LIST *cur;
    char tbuf[STRING_BLOCK_SIZE];
    int j;
    T_SELECT *sub_select;

    sub_select = &update->subselect;

    while (sub_select)
    {
        append_select_string(p, sub_select);
        sub_select = sub_select->sibling_query;
    }

    rttable = &update->rttable[0];
    nest = update->planquerydesc.nest;

    sc_sprintf(tbuf, "%s[%d] %s", MDB_NEWLINE, 1,
            update->planquerydesc.querydesc.fromlist.list[0].table.tablename);
    append_nullstring(p, tbuf);

    curnest = &nest[0];
    if (curnest->scanrid != NULL_OID)
    {
        if (curnest->ridin_cnt)
        {
            append_nullstring(p, MDB_NEWLINE);
            append_nullstring(p, "\tRSCAN");
        }
        else
        {
            sc_sprintf(tbuf, "%s\tRSCAN %s.rid = %d", MDB_NEWLINE,
                    rttable->table.tablename, curnest->scanrid);
            append_nullstring(p, tbuf);
        }
    }
    else if (curnest->indexname == NULL)
    {
        append_nullstring(p, MDB_NEWLINE);
        append_nullstring(p, "\tSSCAN");
    }
    else
    {
        append_nullstring(p, MDB_NEWLINE);
        append_nullstring(p, "\tISCAN RANGE FROM <");
        for (j = 0; j < curnest->index_field_num; j++)
        {
            if (j > 0)
                append_nullstring(p, ",");
            if (curnest->min[j].expr.len == 0)
            {
                append_nullstring(p, " ?");
            }
            else
            {
                append_expression_string(p, &curnest->min[j].expr);
            }
        }
        append_nullstring(p, " > To <");
        for (j = 0; j < curnest->index_field_num; j++)
        {
            if (j > 0)
                append_nullstring(p, ",");
            if (curnest->max[j].expr.len == 0)
            {
                append_nullstring(p, " ?");
            }
            else
            {
                append_expression_string(p, &curnest->max[j].expr);
            }
        }
        sc_sprintf(tbuf, " > ON INDEX %s", curnest->indexname);
        append_nullstring(p, tbuf);
    }

    if (rttable->db_expr_list)
    {
        append_nullstring(p, MDB_NEWLINE);
        append_nullstring(p, "\t\tFILTER");
        for (cur = rttable->db_expr_list; cur; cur = cur->next)
        {
            if (cur != rttable->db_expr_list)
                append_nullstring(p, " and ");
            append_expression_string(p, cur->ptr);
        }
    }
    if (rttable->sql_expr_list)
    {
        append_nullstring(p, MDB_NEWLINE);
        append_nullstring(p, "\tCOND");
        for (cur = rttable->sql_expr_list; cur; cur = cur->next)
        {
            if (cur != rttable->sql_expr_list)
                append_nullstring(p, " and ");
            append_expression_string(p, cur->ptr);
        }
    }

    append_nullstring(p, MDB_NEWLINE);
    append_nullstring(p, MDB_NEWLINE);
}

void
append_select_temptable(_T_STRING * p, T_SELECT * select)
{
    T_SELECT *sub_select;

    sub_select = select->first_sub;
    while (sub_select)
    {
        append_select_temptable(p, sub_select);
        sub_select = sub_select->sibling_query;
    }
    append_subselect_temptable(p, select);
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
get_plan_string(T_STATEMENT * stmt)
{
    _T_STRING qstring = { NULL, 0, 0 };

    switch (stmt->sqltype)
    {
    case ST_SELECT:
        append_select_string(&qstring, &stmt->u._select);
        break;
    case ST_UPDATE:
        append_update_string(&qstring, &stmt->u._update);
        break;
    case ST_DELETE:
        append_delete_string(&qstring, &stmt->u._delete);
        break;
    default:
        append_nullstring(&qstring, "\n\tcurrently not supported\n");
        break;
    }
    return ((char *) qstring.bytes);
}

/*****************************************************************************/
//! collation_of_field

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param fld(in)           :
 * \param idx(in)           :
 * \param num_idx_fld(in)   :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static MDB_COL_TYPE
collation_of_field(T_ATTRDESC * fld, T_ATTRDESC * idx, int num_idx_fld)
{
    int i = 0;

    for (i = 0; i < num_idx_fld; ++i)
    {
        if (fld->Htable == idx[i].Htable && fld->Hattr == idx[i].Hattr)
            return idx[i].collation;
    }

    return MDB_COL_NONE;
}

/*****************************************************************************/
//! match_collation

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param nest(in)      :
 * \param ntable(in)    :
 * \param expr(in/out)  :   out은 오늘 collation으로 설정할 것이므로..
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
match_collation(T_NESTING * nest, int ntable, T_EXPRESSIONDESC * expr)
{
    int i, j;
    T_POSTFIXELEM *elem;
    T_POSTFIXELEM *kc, *kl;
    MDB_COL_TYPE kc_col = MDB_COL_NONE, kl_col = MDB_COL_NONE;

    for (i = 0; i < expr->len; ++i)
    {
        elem = expr->list[i];

        if (elem->elemtype != SPT_OPERATOR)
            continue;

        // GET ON
        kc = expr->list[i - 2];
        kl = expr->list[i - 1];

        if (kc->u.value.valueclass == SVC_CONSTANT ||
                kl->u.value.valueclass == SVC_CONSTANT)
            return RET_SUCCESS;

        for (j = 0; j < ntable; ++j)
        {
            kc_col = collation_of_field(&kc->u.value.attrdesc,
                    nest[j].index_field_info, nest[j].index_field_num);
            if (kc_col != MDB_COL_NONE)
                break;
        }
        /* SET INDEX'S COLLATION */
        if (j == ntable)        // not index
            kc_col = kc->u.value.attrdesc.collation;
        else
            kc->u.value.attrdesc.collation = kc_col;


        for (j = 0; j < ntable; ++j)
        {
            kl_col = collation_of_field(&kl->u.value.attrdesc,
                    nest[j].index_field_info, nest[j].index_field_num);
            if (kl_col != MDB_COL_NONE)
                break;
        }
        /* SET INDEX'S COLLATION */
        if (j == ntable)        // not index
            kl_col = kl->u.value.attrdesc.collation;
        else
            kl->u.value.attrdesc.collation = kl_col;



        if (kc_col != kl_col)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_MISMATCH_COLLATION,
                    0);
            return RET_ERROR;
        }
    }

    return RET_SUCCESS;
}


static int
align_collation_in_nest(int handle, T_NESTING * nest, int n_table)
{
    ATTRED_EXPRDESC_LIST *cur_alist;
    T_EXPRESSIONDESC *expr;
    T_POSTFIXELEM *elem;
    int i, j, k;
    MDB_COL_TYPE collation = MDB_COL_NONE;

    for (i = 0; i < n_table; i++)
    {
        for (cur_alist = nest[i].attr_expr_list; cur_alist;
                cur_alist = cur_alist->next)
        {
            expr = ((T_OR_EXPRDESC *) cur_alist->ptr)->expr;
            for (j = 0; j < expr->len; ++j)
            {
                elem = expr->list[j];

                if (elem->elemtype != SPT_VALUE)
                    continue;

                for (k = 0; k < n_table; ++k)
                {
                    collation = collation_of_field(&elem->u.value.attrdesc,
                            nest[k].index_field_info, nest[k].index_field_num);
                    if (collation != MDB_COL_NONE)
                        break;
                }

                if (k != n_table)
                {
                    elem->u.value.attrdesc.collation = collation;
                }
                else
                {
                    // index가 존재하는 경우에만
                    if (collation != MDB_COL_NONE)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_MISMATCH_COLLATION, 0);
                        return RET_ERROR;
                    }
                }
            }

        }
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! align_collation_of_nest_expr

/*! \breif  index가 존재하는 경우
 *          key, filter가 index fields의 collation을 따라가도록 수정
 ************************************
 * \param handle(in)    :
 * \param select(in/out):
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - join의 경우가 존재하므로 이때의 경우에는
 *    left/right collation의 값을 비교해서 error 처리할 것인지 아닌지를
 *    결정한다
 *****************************************************************************/
static int
align_collation_of_nest_expr(int handle, T_SELECT * select)
{
    T_NESTING *nest;
    T_QUERYDESC *qdesc;
    T_EXPRESSIONDESC *expr;
    int ret, i;
    T_ORTYPE type;
    ATTRED_EXPRDESC_LIST *cur_alist;

#ifdef MDB_DEBUG
    sc_assert(select != NULL, __FILE__, __LINE__);
#endif
    if (select->first_sub &&
            (ret = align_collation_of_nest_expr(handle, select->first_sub))
            != RET_SUCCESS)
        return ret;
    if (select->sibling_query &&
            (ret = align_collation_of_nest_expr(handle, select->sibling_query))
            != RET_SUCCESS)
        return ret;

    nest = select->planquerydesc.nest;
    qdesc = &select->planquerydesc.querydesc;

    if (qdesc->setlist.len > 0)
    {
        for (i = 0; i < qdesc->setlist.len; i++)
        {
            if (qdesc->setlist.list[i]->elemtype == SPT_SUBQUERY)
            {
                ret = align_collation_of_nest_expr(handle,
                        qdesc->setlist.list[i]->u.subq);
                if (ret != RET_SUCCESS)
                {
                    return ret;
                }
            }
        }
        return RET_SUCCESS;
    }

    for (i = 0; i < qdesc->fromlist.len; i++)
    {
        for (cur_alist = nest[i].attr_expr_list; cur_alist;
                cur_alist = cur_alist->next)
        {

            type = ((T_OR_EXPRDESC *) cur_alist->ptr)->type;
            expr = ((T_OR_EXPRDESC *) cur_alist->ptr)->expr;

            if (!(type & ORT_JOINABLE))
                continue;

            ret = match_collation(nest, qdesc->fromlist.len, expr);
            if (ret != RET_SUCCESS)
                return ret;
        }
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! align_collation_in_rttable

/*! \breif  index가 존재하는 경우
 *          rttable의 db filter, sql filter의 collation을
 *          index fields의 collation을 따라가도록 수정
 ************************************
 * \param handle(in)        :
 * \param nest(in/out)      :
 * \param rttable(in/out)   :
 * \param n_table(in/out)   :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
align_collation_in_rttable(int handle, T_NESTING * nest, T_RTTABLE * rttable,
        int n_table)
{
    EXPRDESC_LIST *cur;
    T_EXPRESSIONDESC *expr;
    T_POSTFIXELEM *elem;
    int i, j, k;
    MDB_COL_TYPE collation = MDB_COL_NONE;

    for (i = 0; i < n_table; ++i)
    {
        for (cur = rttable[i].db_expr_list; cur; cur = cur->next)
        {
            expr = (T_EXPRESSIONDESC *) cur->ptr;
            for (j = 0; j < expr->len; ++j)
            {
                elem = expr->list[j];

                if (elem->elemtype != SPT_VALUE)
                    continue;

                for (k = 0; k < n_table; ++k)
                {

                    collation = collation_of_field(&elem->u.value.attrdesc,
                            nest[k].index_field_info, nest[k].index_field_num);
                    if (collation != MDB_COL_NONE)
                        break;
                }

                if (k != n_table)
                {
                    elem->u.value.attrdesc.collation = collation;
                }
                else
                {
                    // index가 존재하는 경우에만
                    if (collation != MDB_COL_NONE)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_MISMATCH_COLLATION, 0);
                        return RET_ERROR;
                    }
                }
            }
        }
    }

    for (i = 0; i < n_table; ++i)
    {
        for (cur = rttable[i].sql_expr_list; cur; cur = cur->next)
        {
            expr = (T_EXPRESSIONDESC *) cur->ptr;
            for (j = 0; j < expr->len; ++j)
            {
                elem = expr->list[j];

                if (elem->elemtype != SPT_VALUE)
                    continue;

                for (k = 0; k < n_table; ++k)
                {

                    collation = collation_of_field(&elem->u.value.attrdesc,
                            nest[k].index_field_info, nest[k].index_field_num);
                    if (collation != MDB_COL_NONE)
                        break;
                }

                if (k != n_table)
                {
                    elem->u.value.attrdesc.collation = collation;
                }
                else
                {
                    // index가 존재하는 경우에만
                    if (collation != MDB_COL_NONE)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_MISMATCH_COLLATION, 0);
                        return RET_ERROR;
                    }
                }
            }
        }
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! align_collation_of_rttalbe_expr

/*! \breif  index가 존재하는 경우
 *          rttable의 db filter, sql filter의 collation을
 *          index fields의 collation을 따라가도록 수정
 ************************************
 * \param handle(in)    :
 * \param select(in/out):
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *      select 에서만 호출된다
 *****************************************************************************/
static int
align_collation_of_rttalbe_expr(int handle, T_SELECT * select)
{
    int ret;
    int i;

#ifdef MDB_DEBUG
    sc_assert(select != NULL, __FILE__, __LINE__);
#endif
    if (select->first_sub &&
            (ret = align_collation_of_rttalbe_expr(handle, select->first_sub))
            != RET_SUCCESS)
        return ret;
    if (select->sibling_query &&
            (ret = align_collation_of_rttalbe_expr(handle,
                            select->sibling_query)) != RET_SUCCESS)
        return ret;

    if (select->planquerydesc.querydesc.setlist.len > 0)
    {
        T_EXPRESSIONDESC *setlist;

        setlist = &select->planquerydesc.querydesc.setlist;

        for (i = 0; i < setlist->len; i++)
        {
            if (setlist->list[i]->elemtype == SPT_SUBQUERY)
            {
                ret = align_collation_of_rttalbe_expr(handle,
                        setlist->list[i]->u.subq);
                if (ret != RET_SUCCESS)
                {
                    return ret;
                }
            }
        }
        return RET_SUCCESS;
    }

    return align_collation_in_rttable(handle,
            select->planquerydesc.nest,
            select->rttable, select->planquerydesc.querydesc.fromlist.len);
}

static int
init_distinct_table(int *handle, T_SELECT * select)
{
    T_PLANNEDQUERYDESC *plan;
    T_LIST_SELECTION *selection;

    T_EXPRESSIONDESC *expr;
    T_POSTFIXELEM *elem;
    T_DISTINCTINFO *distinct_table;

    char wTable[REL_NAME_LENG];
    char wIndex[INDEX_NAME_LENG];

    int findex, distinct_num = 0;
    int i, j;

    distinct_table = (T_DISTINCTINFO *)
            PMEM_ALLOC(sizeof(T_DISTINCTINFO) * MAX_DISTINCT_TABLE_NUM);
    if (distinct_table == NULL)
        return RET_ERROR;

    plan = &select->planquerydesc;
    selection = &plan->querydesc.selection;

    for (i = 0; i < MAX_DISTINCT_TABLE_NUM; i++)
        distinct_table[i].Htable = -1;

    for (i = 0; i < selection->len; i++)
    {
        if (!selection->list[i].is_distinct)
            continue;

        findex = 0;

        expr = &selection->list[i].expr;
        for (j = 0; j < expr->len; j++)
        {
            elem = expr->list[j];
            if (elem->elemtype != SPT_AGGREGATION || !elem->is_distinct)
                continue;

            if (distinct_num >= MAX_DISTINCT_TABLE_NUM)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_TOOMANYDISTINCT, 0);
                goto process_error;
            }

            distinct_table[distinct_num].column_index = i;
            distinct_table[distinct_num].field_index = findex++;

            if (dbi_Trans_NextTempName(*handle, 1, wTable, NULL) < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
                goto process_error;
            }
            distinct_table[distinct_num].table = PMEM_STRDUP(wTable);
            if (!distinct_table[distinct_num].table)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                goto process_error;
            }

            if (plan->querydesc.groupby.len + 1 < MAX_INDEX_FIELD_NUM)
            {
                if (dbi_Trans_NextTempName(*handle, 0, wIndex, NULL) < 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
                    goto process_error;
                }
                distinct_table[distinct_num].index = PMEM_STRDUP(wIndex);
                if (!distinct_table[distinct_num].index)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_OUTOFMEMORY, 0);
                    goto process_error;
                }
            }
            else
                distinct_table[distinct_num].index = NULL;

            ++distinct_num;
        }
    }

    if (plan->querydesc.having.len > 0)
    {
        expr = &plan->querydesc.groupby.having;
        for (i = 0; i < plan->querydesc.having.len; i++)
        {
            elem = expr->list[plan->querydesc.having.hvinfo[i].posi_in_having];
            if (elem->elemtype != SPT_AGGREGATION || !elem->is_distinct)
                continue;

            if (distinct_num >= MAX_DISTINCT_TABLE_NUM)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_TOOMANYDISTINCT,
                        0);
                goto process_error;
            }
            distinct_table[distinct_num].column_index = MDB_INT_MAX;
            distinct_table[distinct_num].field_index = i;

            if (dbi_Trans_NextTempName(*handle, 1, wTable, NULL) < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
                goto process_error;
            }
            distinct_table[distinct_num].table = PMEM_STRDUP(wTable);
            if (!distinct_table[distinct_num].table)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                goto process_error;
            }

            ++distinct_num;
        }
    }

    plan->querydesc.distinct.len = distinct_num;
    plan->querydesc.distinct.distinct_table = distinct_table;

    return RET_SUCCESS;

  process_error:

    return RET_ERROR;
}

static int
set_aggregation_info(T_SELECT * select)
{
    T_QUERYDESC *qdesc;
    T_QUERYRESULT *qresult;
    T_JOINTABLEDESC *join;
    T_EXPRESSIONDESC *expr, argexpr;
    T_POSTFIXELEM *elem;
    T_VALUEDESC arg;
    SYSFIELD_T *fieldinfo;

    int n_fields, n_sel, n_aggr, n_avg, n_gby;
    int p_field, p_avg, p_hv;
    int i, j;

    int having_pos = 0;

    qdesc = &select->planquerydesc.querydesc;
    qresult = &select->queryresult;

    // STEP A.1 :  SELECT절의 항목 개수를 구한다.
    n_sel = qresult->len;

    if (qdesc->having.len > 0)
    {
        qdesc->having.hvinfo = (T_HAVINGINFO *)
                PMEM_ALLOC(sizeof(T_HAVINGINFO) * qdesc->having.len);
        if (qdesc->having.hvinfo == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return RET_ERROR;
        }

        // 끝부터 traverse하기 때문에 calc_expression할때 안맞는거 같다
        expr = &qdesc->groupby.having;
        having_pos = qdesc->having.len;
        for (i = 0; i < expr->len; i++)
        {
            elem = expr->list[(expr->len - 1) - i];
            if (elem->elemtype == SPT_AGGREGATION)
            {
                /* posi_in_having은 HAVING절에서 집계함수 또는 GROUP BY 칼럼이
                   있는 위치이다. */
                qdesc->having.hvinfo[--having_pos].posi_in_having =
                        (expr->len - 1) - i;
                i += elem->u.aggr.significant;

            }
            else if (elem->elemtype == SPT_VALUE &&
                    elem->u.value.valueclass == SVC_VARIABLE)
            {
                qdesc->having.hvinfo[--having_pos].posi_in_having =
                        (expr->len - 1) - i;
            }
        }
    }

    n_aggr = qdesc->aggr_info.len;
    n_gby = qdesc->aggr_info.n_not_sref_gby;
    n_avg = qdesc->aggr_info.n_sel_avg + qdesc->aggr_info.n_hv_avg;

    if (qdesc->groupby.len > 0)
        n_fields = n_sel + n_aggr + n_gby + n_avg;
    else
    {
        n_fields = n_sel;
        for (i = 0; i < qdesc->orderby.len; i++)
        {
            if (qdesc->orderby.list[i].s_ref < 0)
            {
                if (qdesc->is_distinct)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_DISTINCTORDERBY, 0);
                    return RET_ERROR;
                }

                n_fields++;
            }
        }
    }

    join = select->tmp_sub->planquerydesc.querydesc.fromlist.list;
    join->tableinfo.numFields = n_fields + 1;
    join->fieldinfo = PMEM_ALLOC(sizeof(SYSFIELD_T) * (n_fields + 1));
    if (join->fieldinfo == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return RET_ERROR;
    }

    for (p_field = 0; p_field < n_sel; p_field++)
    {
        fieldinfo = &join->fieldinfo[p_field];
        sc_snprintf(fieldinfo->fieldName, FIELD_NAME_LENG, "%s%d",
                INTERNAL_FIELD_PREFIX, p_field);

        fieldinfo->tableId = join->tableinfo.tableId;
        fieldinfo->fieldId = p_field;
        fieldinfo->position = p_field;
        fieldinfo->length = qresult->list[p_field].value.attrdesc.len;
        fieldinfo->scale = qresult->list[p_field].value.attrdesc.dec;
        fieldinfo->type = qresult->list[p_field].value.valuetype;
        fieldinfo->collation = qresult->list[p_field].value.attrdesc.collation;
        fieldinfo->fixed_part = FIXED_VARIABLE;
    }

    if (qdesc->groupby.len <= 0)
    {
        for (i = 0; i < qdesc->orderby.len; i++)
        {
            if (qdesc->orderby.list[i].s_ref < 0)
            {
                fieldinfo = &join->fieldinfo[p_field];
                sc_snprintf(fieldinfo->fieldName, FIELD_NAME_LENG,
                        "%s%d", INTERNAL_FIELD_PREFIX, p_field);

                fieldinfo->tableId = join->tableinfo.tableId;
                fieldinfo->fieldId = p_field;
                fieldinfo->position = p_field;
                fieldinfo->length = qdesc->orderby.list[i].attr.len;
                fieldinfo->scale = qdesc->orderby.list[i].attr.dec;
                fieldinfo->type = qdesc->orderby.list[i].attr.type;
                fieldinfo->collation = qdesc->orderby.list[i].attr.collation;
                fieldinfo->fixed_part = FIXED_VARIABLE;
                p_field++;
            }
        }

        goto RETURN_LABEL;
    }

    // STEP B.2 : SELECT절에서 집계함수에 대한 필드정보를 생성하고
    //            레코드에서 AVG 함수의 개수를 저장하는 offset을 구한다.
    p_avg = n_fields - n_avg;

    for (i = 0; i < qdesc->selection.len; i++)
    {
        expr = &qdesc->selection.list[i].expr;

        for (j = 0; j < expr->len; j++)
        {
            if (expr->list[j]->elemtype == SPT_AGGREGATION)
            {
                fieldinfo = &join->fieldinfo[p_field];

                sc_snprintf(fieldinfo->fieldName, FIELD_NAME_LENG,
                        "%s%d", INTERNAL_FIELD_PREFIX, p_field);

                if (expr->list[j]->u.aggr.type == SAT_COUNT)
                {
                    fieldinfo->length = 4;
                    fieldinfo->scale = 0;
                    fieldinfo->type = DT_INTEGER;
                    fieldinfo->collation = MDB_COL_NUMERIC;

                    p_field++;
                }
                else if (expr->list[j]->u.aggr.type == SAT_SUM ||
                        expr->list[j]->u.aggr.type == SAT_AVG)
                {
                    fieldinfo->length = sizeof(double);
                    fieldinfo->scale = 0;
                    fieldinfo->type = DT_DOUBLE;
                    fieldinfo->collation = MDB_COL_NUMERIC;

                    p_field++;
                }
                else    /* SAT_MIN, SAT_MAX, SAT_SUM, SAT_AVG */
                {
                    fieldinfo->length = expr->list[j]->u.aggr.length;
                    fieldinfo->scale = expr->list[j]->u.aggr.decimals;
                    fieldinfo->type = expr->list[j]->u.aggr.valtype;
                    fieldinfo->collation = expr->list[j]->u.aggr.collation;

                    p_field++;
                }

                fieldinfo->tableId = join->tableinfo.tableId;
                fieldinfo->fieldId = p_field;
                fieldinfo->position = p_field;
                fieldinfo->fixed_part = FIXED_VARIABLE;

                /* AVG 함수를 위한 count의 필드 정보를 생성하고
                   그 offset을 파스 트리의 표현식에 유지한다. */
                if (expr->list[j]->u.aggr.type == SAT_AVG)
                {
                    fieldinfo = &join->fieldinfo[p_avg];

                    sc_snprintf(fieldinfo->fieldName,
                            FIELD_NAME_LENG, "%s%d",
                            INTERNAL_FIELD_PREFIX, p_avg);

                    fieldinfo->tableId = join->tableinfo.tableId;
                    fieldinfo->fieldId = p_avg;
                    fieldinfo->position = p_avg;
                    fieldinfo->length = 4;
                    fieldinfo->scale = 0;
                    fieldinfo->type = DT_INTEGER;
                    fieldinfo->collation = MDB_COL_NUMERIC;
                    fieldinfo->fixed_part = FIXED_VARIABLE;

                    expr->list[j]->u.aggr.cnt_idx = p_avg;
                    p_avg++;
                }
            }
        }
    }

    if (qdesc->groupby.len <= 0)
        return RET_SUCCESS;

    //STEP B.3 : GROUP BY절에서 SELECT절에 없는 항목을 위해 필드 정보를 생성한다.
    for (i = 0; i < qdesc->groupby.len; i++)
    {
        if (qdesc->groupby.list[i].s_ref >= 0)
        {
            qdesc->groupby.list[i].i_ref = qdesc->groupby.list[i].s_ref;
        }
        else
        {
            qdesc->groupby.list[i].i_ref = p_field;

            fieldinfo = &join->fieldinfo[p_field];

            sc_snprintf(fieldinfo->fieldName, FIELD_NAME_LENG,
                    "%s%d", INTERNAL_FIELD_PREFIX, p_field);

            fieldinfo->tableId = join->tableinfo.tableId;
            fieldinfo->fieldId = p_field;
            fieldinfo->position = p_field;
            fieldinfo->length = qdesc->groupby.list[i].attr.len;
            fieldinfo->scale = qdesc->groupby.list[i].attr.dec;
            fieldinfo->type = qdesc->groupby.list[i].attr.type;
            fieldinfo->collation = qdesc->groupby.list[i].attr.collation;
            fieldinfo->fixed_part = FIXED_VARIABLE;

            p_field++;
        }
    }

    // STEP B.4  HAVING절을 위한 필드 정보를 생성하고
    //          HAVING절에서 집계 함수에 대한 필드 정보도 생성한다.
    expr = &qdesc->groupby.having;
    for (i = 0; i < qdesc->having.len; i++)
    {
        p_hv = qdesc->having.hvinfo[i].posi_in_having;
        elem = expr->list[p_hv];
        if (elem->elemtype == SPT_VALUE
                && elem->u.value.valueclass == SVC_VARIABLE)
        {
            for (j = 0; j < qdesc->groupby.len; j++)
            {
                if (!mdb_strcmp(qdesc->groupby.list[j].attr.attrname,
                                elem->u.value.attrdesc.attrname))
                    break;
            }

            if (j >= qdesc->groupby.len)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
                return RET_ERROR;
            }

            qdesc->having.hvinfo[i].posi_in_fields =
                    qdesc->groupby.list[j].i_ref;
        }
        else if (elem->elemtype == SPT_AGGREGATION)
        {
            fieldinfo = &join->fieldinfo[p_field];

            sc_snprintf(fieldinfo->fieldName, FIELD_NAME_LENG,
                    "%s%d", INTERNAL_FIELD_PREFIX, p_field);

            argexpr.len = elem->u.aggr.significant;
            argexpr.list = &(expr->list[p_hv - argexpr.len]);

            for (j = 0; j < argexpr.len; j++)
            {
                if (argexpr.list[j]->elemtype == SPT_VALUE &&
                        argexpr.list[j]->u.value.valueclass == SVC_VARIABLE)
                {
                    if (IS_ALLOCATED_TYPE(argexpr.list[j]->u.value.valuetype))
                        sql_value_ptrXcalloc(&argexpr.list[j]->u.value, 1);
                }
            }
            /* 집계 함수의 인자에 대한 최종 자료형을 알아낸다. */
            arg.call_free = 0;
            if (calc_expression(&argexpr, &arg, MDB_FALSE) < 0)
            {
                return RET_ERROR;
            }

            for (j = 0; j < argexpr.len; j++)
            {
                if (argexpr.list[j]->elemtype == SPT_VALUE &&
                        argexpr.list[j]->u.value.valueclass == SVC_VARIABLE)
                {
                    if (IS_BS(argexpr.list[j]->u.value.valuetype) &&
                            argexpr.list[j]->u.value.call_free)
                    {
                        argexpr.list[j]->u.value.call_free = 0;
                        PMEM_FREENUL(argexpr.list[j]->u.value.u.ptr);
                    }
                    else if (IS_NBS(argexpr.list[j]->u.value.valuetype) &&
                            argexpr.list[j]->u.value.call_free)
                    {
                        argexpr.list[j]->u.value.call_free = 0;
                        PMEM_FREENUL(argexpr.list[j]->u.value.u.Wptr);
                    }
                }
            }

            elem->u.aggr.valtype = arg.valuetype;
            elem->u.aggr.length = arg.attrdesc.len;
            elem->u.aggr.decimals = arg.attrdesc.dec;
            elem->u.aggr.collation = arg.attrdesc.collation;

            fieldinfo->tableId = join->tableinfo.tableId;
            fieldinfo->fieldId = p_field;
            fieldinfo->position = p_field;
            fieldinfo->fixed_part = FIXED_VARIABLE;

            switch (elem->u.aggr.type)
            {
            case SAT_COUNT:
                fieldinfo->length = 4;
                fieldinfo->scale = 0;
                fieldinfo->type = DT_INTEGER;
                fieldinfo->collation = MDB_COL_NUMERIC;
                break;
            case SAT_MIN:
            case SAT_MAX:
            case SAT_SUM:
                fieldinfo->length = sizeof(double);
                fieldinfo->scale = 0;
                fieldinfo->type = DT_DOUBLE;
                fieldinfo->collation = MDB_COL_NUMERIC;
                break;
            case SAT_AVG:
                fieldinfo->length = sizeof(double);
                fieldinfo->scale = 0;
                fieldinfo->type = DT_DOUBLE;
                fieldinfo->collation = MDB_COL_NUMERIC;

                /* count에 대한 필드 정보를 생성한다. */
                fieldinfo = &join->fieldinfo[p_avg];

                sc_snprintf(fieldinfo->fieldName,
                        FIELD_NAME_LENG, "%s%d", INTERNAL_FIELD_PREFIX, p_avg);
                fieldinfo->tableId = join->tableinfo.tableId;
                fieldinfo->fieldId = p_avg;
                fieldinfo->position = p_avg;
                fieldinfo->length = 4;
                fieldinfo->scale = 0;
                fieldinfo->type = DT_INTEGER;
                fieldinfo->collation = MDB_COL_NUMERIC;
                fieldinfo->fixed_part = FIXED_VARIABLE;
                elem->u.aggr.cnt_idx = p_avg;
                qdesc->having.hvinfo[i].idx_of_count = p_avg;
                p_avg++;
                break;
            default:
                sql_value_ptrFree(&arg);

                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
                return RET_ERROR;
            }
            sql_value_ptrFree(&arg);

            qdesc->having.hvinfo[i].posi_in_fields = p_field++;
        }
    }

    //n_fields = n_sel + n_aggr + n_gby + n_avg + n_order;
    // STEP B.5 : ORDER BY절에서 SELECT절에 없는 항목은 GROUPBY의 fileds와 동일하므로
    //            GROUP BY를 위해 생성된 TEMP TALBE'S FIELD를 REFERENCE한다.
    for (i = 0; i < qdesc->orderby.len; ++i)
    {
        if (qdesc->orderby.list[i].s_ref >= 0)
        {
            qdesc->orderby.list[i].i_ref = qdesc->orderby.list[i].s_ref;
        }
        else
        {
            for (j = 0; j < qdesc->groupby.len; ++j)
            {
                if (!mdb_strcmp(qdesc->orderby.list[i].attr.table.tablename,
                                qdesc->groupby.list[j].attr.table.tablename) &&
                        !mdb_strcmp(qdesc->orderby.list[i].attr.attrname,
                                qdesc->groupby.list[j].attr.attrname))
                    break;
            }
            qdesc->orderby.list[i].i_ref = qdesc->groupby.list[j].i_ref;
        }
    }

  RETURN_LABEL:
    fieldinfo = &join->fieldinfo[n_fields];

    sc_snprintf(fieldinfo->fieldName, FIELD_NAME_LENG, "%s%d",
            INTERNAL_FIELD_PREFIX, n_fields);
    fieldinfo->tableId = join->tableinfo.tableId;
    fieldinfo->fieldId = n_fields;
    fieldinfo->position = n_fields;
    fieldinfo->length = sizeof(OID);
    fieldinfo->scale = 0;
    fieldinfo->type = DT_OID;
    fieldinfo->collation = MDB_COL_NUMERIC;
    fieldinfo->fixed_part = FIXED_VARIABLE;

    return RET_SUCCESS;
}

static int
make_fromlist_for_temp_select(int handle, T_SELECT * select, char *wTable)
{
    T_QUERYDESC *old_qdesc;
    T_LIST_JOINTABLEDESC *fromlist;
    T_LIST_SELECTION *selection;
    T_EXPRESSIONDESC *expr;
    T_AGGRINFO *aggr_info;

    int tmp_tableid;
    int i;

    aggr_info = &select->planquerydesc.querydesc.aggr_info;

    if (aggr_info->len > 0)
    {
        int from_condition_len = 0;
        int use_dosimple;

        old_qdesc = &select->planquerydesc.querydesc;

        aggr_info->aggr = (T_RESULTCOLUMN *)
                PMEM_ALLOC(sizeof(T_RESULTCOLUMN) * aggr_info->len);
        if (aggr_info->aggr == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return RET_ERROR;
        }
        for (i = 0; i < aggr_info->len; i++)
        {
            aggr_info->aggr[i].value.valueclass = SVC_CONSTANT;
            aggr_info->aggr[i].value.is_null = 1;
        }

        for (i = 0, from_condition_len = 0; i < old_qdesc->fromlist.len; i++)
        {
            from_condition_len += old_qdesc->fromlist.list[i].condition.len;
        }

        use_dosimple = 1;

        if (old_qdesc->fromlist.len > 1 || from_condition_len > 0
                || old_qdesc->orderby.len > 0 || old_qdesc->groupby.len > 0)
        {
            use_dosimple = 0;
        }
        else if (old_qdesc->is_distinct != 0 || aggr_info->len != 1
                || select->planquerydesc.nest->op_in)
        {
            use_dosimple = 0;
        }
        else if (select->rttable->sql_expr_list)
        {
            use_dosimple = 0;
        }
        else if (old_qdesc->condition.list && old_qdesc->expr_list == NULL)
        {
            use_dosimple = 0;
        }

        selection = &select->planquerydesc.querydesc.selection;

        if (use_dosimple)
        {
            T_INDEXINFO index;

            sc_memset(&index, 0, sizeof(T_INDEXINFO));
            expr = &selection->list[0].expr;

            if (expr->len == 1 &&
                    expr->list[0]->elemtype == SPT_AGGREGATION &&
                    (expr->list[0]->u.aggr.type == SAT_COUNT ||
                            expr->list[0]->u.aggr.type == SAT_DCOUNT))
            {
                use_dosimple = 1;
            }
            else if (expr->len == 2 &&
                    expr->list[1]->elemtype == SPT_AGGREGATION &&
                    (expr->list[1]->u.aggr.type == SAT_MIN ||
                            expr->list[1]->u.aggr.type == SAT_MAX) &&
                    expr->list[0]->elemtype == SPT_VALUE &&
                    expr->list[0]->u.value.valueclass == SVC_VARIABLE &&
                    select->planquerydesc.nest[0].indexname &&
                    is_firstmemberofindex(handle,
                            &expr->list[0]->u.value.attrdesc,
                            select->planquerydesc.nest[0].indexname, &index))
            {
                use_dosimple = 1;
            }
            else
                use_dosimple = 0;
            PMEM_FREENUL(index.fields);
        }

        aggr_info->use_dosimple = use_dosimple;

        if (aggr_info->use_dosimple || old_qdesc->groupby.len == 0)
        {
            return RET_SUCCESS;
        }
    }

    tmp_tableid = dbi_Trans_NextTempName(handle, 1, wTable, NULL);
    if (tmp_tableid < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
        return RET_ERROR;
    }

    fromlist = &select->tmp_sub->planquerydesc.querydesc.fromlist;
    fromlist->max = 1;
    fromlist->len = 1;

    fromlist->list = (T_JOINTABLEDESC *) PMEM_ALLOC(sizeof(T_JOINTABLEDESC));
    if (fromlist->list == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return RET_ERROR;
    }

    fromlist->list->table.tablename = PMEM_STRDUP(wTable);
    if (!fromlist->list->table.tablename)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return RET_ERROR;
    }
    fromlist->list->tableinfo.tableId = -1;
    sc_strncpy(fromlist->list->tableinfo.tableName, wTable, REL_NAME_LENG);

    if (set_aggregation_info(select) != RET_SUCCESS)
        return RET_ERROR;

    if (select->planquerydesc.querydesc.groupby.len > 0 &&
            select->planquerydesc.querydesc.is_distinct)
    {
        if (dbi_Trans_NextTempName(handle, 1,
                        select->planquerydesc.querydesc.groupby.distinct_table,
                        NULL) < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
            return RET_ERROR;
        }
    }

    return RET_SUCCESS;
}

static int
make_selection_for_temp_select(int handle, T_SELECT * select, char *wTable)
{
    T_JOINTABLEDESC *join;
    T_LIST_SELECTION *selection, *old_selection;
    T_EXPRESSIONDESC *expr, *old_expr;
    T_POSTFIXELEM *elem;
    T_ATTRDESC *attr;
    T_AGGRINFO *aggr_info;

    char col_name[FIELD_NAME_LENG];
    int i, j, k, n, e_index;

    SYSFIELD_T *fieldinfo = NULL;

    join = select->tmp_sub->planquerydesc.querydesc.fromlist.list;
    selection = &select->tmp_sub->planquerydesc.querydesc.selection;
    aggr_info = &select->planquerydesc.querydesc.aggr_info;

    selection->list = (T_SELECTION *)
            PMEM_ALLOC(sizeof(T_SELECTION) * select->queryresult.len);
    if (selection->list == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return RET_ERROR;
    }

    selection->len = select->queryresult.len;
    selection->max = selection->len;

    old_selection = &select->planquerydesc.querydesc.selection;

    for (i = 0; i < selection->len; i++)
    {
        if (select->planquerydesc.querydesc.aggr_info.use_dosimple
                || wTable[0] == '\0')
        {
            selection->list[i].res_name[0] = '\0';
            selection->list[i].is_distinct = 0;
        }
        else
        {
            fieldinfo = join->fieldinfo + i;

            sc_snprintf(col_name, FIELD_NAME_LENG, "%s%d",
                    INTERNAL_FIELD_PREFIX, i);
            sc_memcpy(selection->list[i].res_name, fieldinfo->fieldName,
                    FIELD_NAME_LENG - 1);
            selection->list[i].res_name[FIELD_NAME_LENG - 1] = '\0';
        }

        if (!select->planquerydesc.querydesc.groupby.len && aggr_info->len > 0)
        {
            e_index = (old_selection->len - 1) - i;

            expr = &selection->list[e_index].expr;
            old_expr = &old_selection->list[e_index].expr;

            for (j = 0, n = old_expr->len; j < old_expr->len; j++)
            {
                elem = old_expr->list[j];
                if (elem->elemtype == SPT_AGGREGATION)
                    n -= elem->u.aggr.significant;
            }

            expr->list = sql_mem_get_free_postfixelem_list(NULL, n);
            if (expr->list == NULL)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return RET_ERROR;
            }
            for (j = 0; j < n; ++j)
            {
                expr->list[j] = sql_mem_get_free_postfixelem(NULL);
                if (expr->list[j] == NULL)
                {
                    for (k = 0; k < j; ++k)
                        sql_mem_set_free_postfixelem(NULL, expr->list[k]);
                    sql_mem_set_free_postfixelem_list(NULL, expr->list);
                    return RET_ERROR;
                }
            }
            expr->len = n;
            expr->max = n;
        }
        else
        {
            expr = &selection->list[i].expr;
            expr->list = sql_mem_get_free_postfixelem_list(NULL, 1);
            if (expr->list == NULL)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return RET_ERROR;
            }

            expr->list[0] = sql_mem_get_free_postfixelem(NULL);
            if (expr->list[0] == NULL)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return RET_ERROR;
            }
            expr->len = 1;
            expr->max = 1;

            attr = &expr->list[0]->u.value.attrdesc;
            attr->table.tablename = PMEM_STRDUP(wTable);
            if (!attr->table.tablename)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return RET_ERROR;
            }
            attr->table.aliasname = NULL;
            attr->attrname = PMEM_STRDUP(col_name);
            if (!attr->attrname)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return RET_ERROR;
            }

            attr->posi.ordinal = fieldinfo->position;
            attr->posi.offset = fieldinfo->offset;
            attr->type = fieldinfo->type;
            attr->len = fieldinfo->length;
            attr->Htable = fieldinfo->tableId;
            attr->Hattr = fieldinfo->fieldId;
            attr->collation = fieldinfo->collation;

            elem = old_selection->list[i].expr.list[0];

            if (old_selection->list[i].expr.len == 1 &&
                    elem->elemtype == SPT_VALUE &&
                    elem->u.value.valueclass == SVC_VARIABLE)
            {
                sc_memcpy(&attr->defvalue,
                        &elem->u.value.attrdesc.defvalue,
                        sizeof(T_DEFAULTVALUE));

                if (IS_BYTES_OR_STRING(elem->u.value.valuetype))
                {
                    if (attr->defvalue.not_null)
                    {
                        attr->defvalue.u.ptr =
                                PMEM_STRDUP(attr->defvalue.u.ptr);
                        if (!attr->defvalue.u.ptr)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_OUTOFMEMORY, 0);
                            return RET_ERROR;
                        }
                    }
                }
                else if (IS_NBS(elem->u.value.valuetype))
                {
                    if (attr->defvalue.not_null)
                    {
                        attr->defvalue.u.Wptr =
                                PMEM_WSTRDUP(attr->defvalue.u.Wptr);
                        if (!attr->defvalue.u.Wptr)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_OUTOFMEMORY, 0);
                            return RET_ERROR;
                        }
                    }
                }
            }
            else
            {
                attr->defvalue.defined = 1;
                attr->defvalue.not_null = 0;
            }

            expr->list[0]->elemtype = SPT_VALUE;
            expr->list[0]->u.value.valueclass = SVC_VARIABLE;
            expr->list[0]->u.value.valuetype =
                    select->queryresult.list[i].value.valuetype;
            expr->list[0]->u.value.call_free = 0;
        }
    }

    selection->len = i;

    if (select->planquerydesc.querydesc.is_distinct &&
            init_distinct_table(&handle, select) != RET_SUCCESS)
        return RET_ERROR;

    return RET_SUCCESS;
}

static int
make_temp_select(int handle, T_SELECT * select, int kindofwTable)
{
    T_PLANNEDQUERYDESC *plan;
    T_QUERYRESULT *qresult;
    SYSFIELD_T *fieldinfo;
    register T_ORDERBYDESC *orderby, *old_orderby;
    register T_ORDERBYITEM *ob_list;

    char wTable[REL_NAME_LENG];
    char wIndex[REL_NAME_LENG];
    int i, j;

    select->tmp_sub = (T_SELECT *) PMEM_ALLOC(sizeof(T_SELECT));
    if (select->tmp_sub == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return RET_ERROR;
    }

    select->tmp_sub->kindofwTable = kindofwTable;

    plan = &select->tmp_sub->planquerydesc;

    plan->querydesc.querytype = SQT_NORMAL;
    plan->querydesc.limit = select->planquerydesc.querydesc.limit;
    select->planquerydesc.querydesc.limit.offset = 0;
    select->planquerydesc.querydesc.limit.rows = -1;
    select->planquerydesc.querydesc.limit.offset_param_idx = -1;
    select->planquerydesc.querydesc.limit.rows_param_idx = -1;
    wTable[0] = '\0';

    if (make_fromlist_for_temp_select(handle, select, wTable) != RET_SUCCESS)
        return RET_ERROR;

    if (make_selection_for_temp_select(handle, select, wTable) != RET_SUCCESS)
        return RET_ERROR;

    if (kindofwTable == iTABLE_DISTINCT
            && select->queryresult.len <= MAX_KEY_FIELD_NO)
    {
        if (dbi_Trans_NextTempName(handle, 0, wIndex, NULL) < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
            return RET_ERROR;
        }

        plan->nest->indexname = PMEM_STRDUP(wIndex);
        if (!plan->nest->indexname)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return RET_ERROR;
        }
    }

    if (select->planquerydesc.querydesc.orderby.len > 0 && wTable[0] != '\0')
    {
        T_NESTING *nest = select->planquerydesc.nest;

        orderby = &plan->querydesc.orderby;
        old_orderby = &select->planquerydesc.querydesc.orderby;
        fieldinfo = plan->querydesc.fromlist.list->fieldinfo;

        if (nest->indexname)
        {
            for (i = 0; i < old_orderby->len &&
                    nest->index_field_num >= old_orderby->len; i++)
            {
                if (old_orderby->list[i].s_ref >= 0)
                {
                    if (nest->index_field_info->order !=
                            old_orderby->list[i].attr.order ||
                            nest->index_field_info->collation !=
                            old_orderby->list[i].attr.collation ||
                            nest->index_field_info->posi.offset !=
                            old_orderby->list[i].attr.posi.offset)
                        break;
                }
            }
        }
        else
        {
            i = 0;
        }

        if (i < old_orderby->len)
        {
            int orderby_pos;

            orderby->len = old_orderby->len;
            orderby->max = old_orderby->max;
            orderby->list =
                    PMEM_ALLOC(sizeof(T_ORDERBYITEM) * old_orderby->len);

            orderby_pos = select->queryresult.len;

            for (i = 0; i < old_orderby->len; i++)
            {
                if (old_orderby->list[i].s_ref < 0)
                {
                    if (select->planquerydesc.querydesc.groupby.len > 0)
                    {
                        j = old_orderby->list[i].i_ref;
                    }
                    else
                    {
                        j = orderby_pos;
                        orderby_pos++;
                    }
                }
                else
                    j = old_orderby->list[i].s_ref;

                ob_list = &orderby->list[i];

                ob_list->ordertype = old_orderby->list[i].ordertype;
                ob_list->i_ref = j;
                ob_list->s_ref = old_orderby->list[i].s_ref;
                ob_list->param_idx = old_orderby->list[i].param_idx;
                ob_list->attr.table.tablename = PMEM_STRDUP(wTable);
                if (!ob_list->attr.table.tablename)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_OUTOFMEMORY, 0);
                    return RET_ERROR;
                }
                ob_list->attr.attrname = PMEM_STRDUP(fieldinfo[j].fieldName);
                if (!ob_list->attr.attrname)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_OUTOFMEMORY, 0);
                    return RET_ERROR;
                }
                ob_list->attr.posi.ordinal = fieldinfo[j].position;
                ob_list->attr.posi.offset = fieldinfo[j].offset;
                ob_list->attr.type = fieldinfo[j].type;
                ob_list->attr.len = fieldinfo[j].length;
                ob_list->attr.Htable = fieldinfo[j].tableId;
                ob_list->attr.Hattr = fieldinfo[j].fieldId;
                if (old_orderby->list[i].attr.collation != MDB_COL_NUMERIC &&
                        old_orderby->list[i].attr.collation != MDB_COL_NONE)
                {
                    ob_list->attr.collation =
                            old_orderby->list[i].attr.collation;
                }
                else
                    ob_list->attr.collation = fieldinfo[j].collation;
            }

            select->kindofwTable |= iTABLE_ORDERBY;

            if (plan->nest[0].indexname == NULL)
            {
                if (dbi_Trans_NextTempName(handle, 0, wIndex, NULL) < 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
                    return RET_ERROR;
                }

                plan->nest[0].indexname = PMEM_STRDUP(wIndex);
                if (!plan->nest[0].indexname)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_OUTOFMEMORY, 0);
                    return RET_ERROR;
                }
            }
        }
    }

    if (wTable[0] != '\0')
    {
        select->tmp_sub->rttable[0].table.tablename
                = plan->querydesc.fromlist.list[0].table.tablename;
        select->tmp_sub->rttable[0].table.aliasname = NULL;

        select->tmp_sub->rttable[0].rec_values =
                init_rec_values(&select->tmp_sub->rttable[0],
                &plan->querydesc.fromlist.list[0].tableinfo,
                plan->querydesc.fromlist.list[0].fieldinfo, 1);

        if (select->tmp_sub->rttable[0].rec_values == NULL)
            return RET_ERROR;
    }

    qresult = &select->tmp_sub->queryresult;

    qresult->len = select->queryresult.len;
    qresult->include_rid = 0;

    qresult->list = (T_RESULTCOLUMN *)
            PMEM_XCALLOC(sizeof(T_RESULTCOLUMN) * (qresult->len + 1));
    if (qresult->list == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return RET_ERROR;
    }

    for (i = 0; i < qresult->len; i++)
    {
        sc_memcpy(&qresult->list[i], &select->queryresult.list[i],
                sizeof(T_RESULTCOLUMN));
        qresult->list[i].value.call_free = 0;
    }

    return RET_SUCCESS;
}

static int
do_simple_orderby_query(int handle, T_SELECT * select)
{

    T_QUERYDESC *qdesc;
    T_NESTING *nest;
    T_TABLEDESC *table = NULL;
    T_INDEXINFO *index;
    char *tablename;
    int i, ret = RET_SUCCESS;

    qdesc = &select->planquerydesc.querydesc;
    nest = &select->planquerydesc.nest[0];

    if (qdesc->fromlist.len > 0)
        table = &qdesc->fromlist.list[0].table;

    if (qdesc->fromlist.len > 1 || (table && (table->correlated_tbl != NULL)))
        return RET_ERROR;

    if (table && table->tablename && table->tablename[0] != '\0')
        tablename = table->tablename;
    else
        tablename = qdesc->orderby.list[0].attr.table.tablename;

    for (i = 0; i < qdesc->orderby.len; i++)
    {
        if (qdesc->orderby.list[i].s_ref >= 0)
        {
            if (qdesc->selection.list[qdesc->orderby.list[i].s_ref].expr.len >
                    1)
                break;
        }
    }
    if (i < qdesc->orderby.len)
        return RET_ERROR;

    index = sql_xcalloc(sizeof(T_INDEXINFO), 0);
    if (index == NULL)
        return RET_ERROR;

    if (!is_firstgroupofindex(handle, &qdesc->orderby, nest->opentype,
                    tablename, nest->indexname, index))
    {
        ret = RET_ERROR;
        goto end_process;
    }
    if (nest->indexname == NULL)
    {
        nest->indexname = PMEM_STRDUP(index->info.indexName);
        if (nest->indexname == NULL)
        {
            ret = RET_ERROR;
            goto end_process;
        }
    }
    else if (mdb_strcmp(nest->indexname, index->info.indexName))
    {
        ret = RET_ERROR;
        goto end_process;
    }

  end_process:

    PMEM_FREENUL(index->fields);
    PMEM_FREE(index);

    return ret;
}

static int
set_temp_select(int connid, T_SELECT * select)
{
    T_QUERYDESC *qdesc;
    int ret;
    int i;

#ifdef MDB_DEBUG
    sc_assert(select != NULL, __FILE__, __LINE__);
#endif

    if (select->first_sub &&
            (ret = set_temp_select(connid, select->first_sub)) != RET_SUCCESS)
        return ret;
    if (select->sibling_query &&
            (ret = set_temp_select(connid, select->sibling_query))
            != RET_SUCCESS)
        return ret;

    qdesc = &select->planquerydesc.querydesc;
    if (qdesc->setlist.len > 0)
    {
        for (i = 0; i < qdesc->setlist.len; i++)
        {
            if (qdesc->setlist.list[i]->elemtype == SPT_SUBQUERY)
            {
                ret = set_temp_select(connid, qdesc->setlist.list[i]->u.subq);
                if (ret != RET_SUCCESS)
                    return ret;
            }
        }
        return RET_SUCCESS;
    }

    if (qdesc->querytype == SQT_AGGREGATION)
    {
        return make_temp_select(connid, select, iTABLE_AGGREGATION);
    }
    else if (qdesc->is_distinct)
    {
        return make_temp_select(connid, select, iTABLE_DISTINCT);
    }
    else if (qdesc->orderby.len > 0)
    {
        if (do_simple_orderby_query(connid, select) == RET_SUCCESS)
            return RET_SUCCESS;
        return make_temp_select(connid, select, iTABLE_ORDERBY);
    }

    return RET_SUCCESS;
}

static int
sql_planning_for_select(int handle, T_SELECT * select)
{
    if (make_nestorder(handle, select) < 0)
        return RET_ERROR;
    if (arrange_rttableorder(handle, select) < 0)
        return RET_ERROR;
    if (determine_eval_expr(handle, select) < 0)
        return RET_ERROR;
    if (align_collation_of_nest_expr(handle, select) < 0)
        return RET_ERROR;
    if (align_collation_of_rttalbe_expr(handle, select) < 0)
        return RET_ERROR;
    if (set_temp_select(handle, select) < 0)
        return RET_ERROR;
    if (set_rec_values_info_select(handle, select) < 0)
        return RET_ERROR;

    if (select->planquerydesc.nest[0].op_in &&
            select->planquerydesc.querydesc.limit.start_at != NULL_OID)
    {

        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_LIMITRID_NOTALLOWED, 0);
        return RET_ERROR;
    }

    return RET_SUCCESS;
}

static int
sql_planning_for_other(int handle, T_PLANNEDQUERYDESC * plan,
        T_RTTABLE * rttable)
{
    T_JOINTABLEDESC *join;

    join = plan->querydesc.fromlist.list;

    initialize_nest(plan->nest, &join->table);

    /* set index range */
    if (put_exprs_for_nest(plan->nest,
                    plan->querydesc.expr_list) != RET_SUCCESS)
        return RET_ERROR;

    if (set_expr_attribute(plan->nest->attr_expr_list,
                    &join->table, NULL) != RET_SUCCESS)
        return RET_ERROR;

    if (make_scan_info(handle, plan->nest, join) != RET_SUCCESS)
        return RET_ERROR;

    compute_iscan_cost(plan->nest);

    if (set_indexrange(handle, plan->nest, join->tableinfo.numFields,
                    join->fieldinfo) != RET_SUCCESS)
        return RET_ERROR;

    if (separate_exprs_by_attr(plan->nest, rttable) != RET_SUCCESS)
        return RET_ERROR;

    if (align_collation_in_nest(handle, plan->nest, 1) < 0)
        return RET_ERROR;
    if (align_collation_in_rttable(handle, plan->nest, rttable, 1) < 0)
        return RET_ERROR;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! sql_plan

/*! \breif SQL 문장의 PLAN을 만듬
 ************************************
 * \param handle(in) :
 * \param stmt(in/out) :
 ************************************
 * \return  RET_SUCCESS/RET_ERROR
 ************************************
 * \note
 *  - rttable의 rec_values를 미리 설정한다
 *  - RRTABLE의 FIELDVALUES_T의 값을 설정
 *****************************************************************************/
int
sql_planning(int handle, T_STATEMENT * stmt)
{
    // 1 phase: plan을 generation한다.
    switch (stmt->sqltype)
    {
    case ST_SELECT:
        if (sql_planning_for_select(handle, &stmt->u._select) < 0)
            return RET_ERROR;
        break;
    case ST_UPDATE:
        if (sql_planning_for_other(handle, &stmt->u._update.planquerydesc,
                        stmt->u._update.rttable) < 0)
            return RET_ERROR;

        if (stmt->u._update.subselect.first_sub)
        {
            if (sql_planning_for_select(handle,
                            stmt->u._update.subselect.first_sub) < 0)
                return RET_ERROR;
        }

        break;
    case ST_DELETE:
        if (sql_planning_for_other(handle, &stmt->u._delete.planquerydesc,
                        stmt->u._delete.rttable) < 0)
            return RET_ERROR;

        if (stmt->u._delete.subselect.first_sub)
        {
            if (sql_planning_for_select(handle,
                            stmt->u._delete.subselect.first_sub) < 0)
                return RET_ERROR;
        }

        break;
    case ST_INSERT:
        break;
    default:
        break;
    }

    return RET_SUCCESS;
}

static void
set_evalfld_of_rec_values(FIELDVALUES_T * rec_values, int fld_pos)
{
    int i;

#if defined(MDB_DEBUG)
    if (rec_values->fld_cnt <= fld_pos)
        sc_assert(0, __FILE__, __LINE__);
#endif

    for (i = 0; i < rec_values->eval_fld_cnt; ++i)
    {
        if (rec_values->eval_fld_pos[i] == fld_pos)
            break;
    }

    if (i == rec_values->eval_fld_cnt)
    {
        rec_values->eval_fld_pos[i] = fld_pos;
        ++rec_values->eval_fld_cnt;
    }
}

static void
set_selfld_of_rec_values(FIELDVALUES_T * rec_values, int fld_pos)
{
    int i;

#if defined(MDB_DEBUG)
    if (rec_values->fld_cnt <= fld_pos)
        sc_assert(0, __FILE__, __LINE__);
#endif

    // eval과 중복되는 것이 있다면 skip한다
    for (i = 0; i < rec_values->eval_fld_cnt; ++i)
    {
        if (rec_values->eval_fld_pos[i] == fld_pos)
            return;
    }

    for (i = 0; i < rec_values->sel_fld_cnt; ++i)
    {
        if (rec_values->sel_fld_pos[i] == fld_pos)
            break;
    }

    if (i == rec_values->sel_fld_cnt)
    {
        rec_values->sel_fld_pos[i] = fld_pos;
        ++rec_values->sel_fld_cnt;
    }
}

/*****************************************************************************/
//! set_rec_values_info_delete

/*! \breif RRTABLE의 FIELDVALUES_T안의 sel_fld와 eval_fld의 값을 설정한다
 ************************************
 * \param handle(in) :
 * \param delete(in/out) :
 ************************************
 * \return  RET_SUCCESS/RET_ERROR
 ************************************
 * \note
 *  1. EVALUATION FIELDS INFO SETTING
 *      step 1 : condition list info
 *      step 2 : groupby info
 *          step 2.1 : groupby info
 *          step 2.2 : having info
 *      step 3. orderby info
 *  2. SELECTION FIELDS INFO SETTING
 *      step 1 : selection list info
 *  - caution
 *      특정 필드가 selection fields와 evaluation fields에 동시에 나오는 경우
 *      evaluation fields의 정보에만 setting 된다
 *  - 함수 추가
 *
 *****************************************************************************/
int
set_rec_values_info_delete(int handle, T_DELETE * delete)
{
    T_POSTFIXELEM *elem;
    T_RTTABLE *rttable = delete->rttable;
    T_EXPRESSIONDESC *condition = &(delete->planquerydesc.querydesc.condition); // postfix 사용
    int i;
    T_SELECT *select;

    // 1. EVALUATION 필드 정보를 SETTING
    for (i = 0; i < condition->len; ++i)
    {
        elem = condition->list[i];

        if (elem->elemtype != SPT_VALUE ||
                elem->u.value.valueclass != SVC_VARIABLE)
            continue;

        if (mdb_strcmp(rttable->table.tablename,
                        elem->u.value.attrdesc.table.tablename))
            continue;

        set_evalfld_of_rec_values(&rttable->rec_values[0],
                elem->u.value.attrdesc.posi.ordinal);
    }

    /* delete의 경우 rrtable설정이 prepare에서 이루어지지 않아 correl_attr의
       evaluate field 설정이 안됨.  해당 영역에서 evaluate field 설정 */
    select = delete->subselect.first_sub;
    while (select)
    {
        if (select && select->kindofwTable & iTABLE_CORRELATED)
        {
            CORRELATED_ATTR *cl;

            for (cl = select->planquerydesc.querydesc.correl_attr; cl;
                    cl = cl->next)
            {
                elem = cl->ptr;

                if (elem->u.value.attrdesc.table.correlated_tbl !=
                        &delete->subselect)
                {
                    continue;
                }

                if (elem->elemtype != SPT_VALUE ||
                        elem->u.value.valueclass != SVC_VARIABLE)
                {
                    continue;
                }

                set_evalfld_of_rec_values(&rttable->rec_values[0],
                        elem->u.value.attrdesc.posi.ordinal);
            }
        }

        select = select->first_sub;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! set_sel_of_rec_values

/*! \breif RRTABLE의 FIELDVALUES_T안의 sel_fld의 정보를 설정한다
 ************************************
 * \param handle(in) :
 * \param select(in/out) :
 ************************************
 * \return  RET_SUCCESS/RET_ERROR
 ************************************
 * \note
 *  - 함수의 call 순서
 *      1. set_eval_of_rec_values_other
 *      2. int set_eval_of_rec_values_where
 *      3. int set_sel_of_rec_values
 *
 *****************************************************************************/
int
set_sel_of_rec_values(int handle, T_SELECT * select)
{
    T_LIST_SELECTION *selection;

    T_LIST_JOINTABLEDESC *fromlist;
    T_POSTFIXELEM *elem;
    T_RTTABLE *rttable;
    int i, j, k;

#ifdef MDB_DEBUG
    sc_assert(select != NULL, __FILE__, __LINE__);
#endif

    selection = &(select->planquerydesc.querydesc.selection);
    for (i = 0; i < selection->len; ++i)
    {
        for (j = 0; j < selection->list[i].expr.len; ++j)
        {
            elem = selection->list[i].expr.list[j];
            if (elem->elemtype != SPT_VALUE ||
                    elem->u.value.valueclass != SVC_VARIABLE)
                continue;

            if (elem->u.value.attrdesc.table.correlated_tbl)    // corrated인 경우
                fromlist =
                        &(elem->u.value.attrdesc.table.correlated_tbl->
                        planquerydesc.querydesc.fromlist);
            else        // correlated가 아닌 경우
                fromlist = &(select->planquerydesc.querydesc.fromlist);

            for (k = 0; k < fromlist->len; ++k)
            {
                if (elem->u.value.attrdesc.Hattr == -1)
                    continue;
                if (elem->u.value.attrdesc.table.correlated_tbl)        // correlated인 경우
                    rttable =
                            &(elem->u.value.attrdesc.table.correlated_tbl->
                            rttable[k]);
                else    // correlated가 아닌 경우
                    rttable = &(select->rttable[k]);

                if (mdb_strcmp(rttable->table.tablename,
                                elem->u.value.attrdesc.table.tablename))
                    continue;

                if (select->planquerydesc.nest[0].scanrid != NULL_OID ||
                        select->planquerydesc.nest[0].is_param_scanrid == 1)
                {
                    set_evalfld_of_rec_values(rttable->rec_values,
                            elem->u.value.attrdesc.posi.ordinal);
                }
                else
                    set_selfld_of_rec_values(rttable->rec_values,
                            elem->u.value.attrdesc.posi.ordinal);
                if ((rttable->table.tablename &&
                                isTempTable_name(rttable->table.tablename)) ||
                        (rttable->table.aliasname &&
                                isTempTable_name(rttable->table.aliasname)))
                    set_selfld_of_rec_values(rttable->rec_values,
                            rttable->rec_values->fld_cnt - 1);


            }
        }
    }

    if (!select->planquerydesc.querydesc.select_star)
        return RET_SUCCESS;

    // (*)가 존재하는 경우
    for (i = 0; i < select->planquerydesc.querydesc.fromlist.len; ++i)
    {
        rttable = &(select->rttable[i]);
        for (j = 0; j < rttable->rec_values->fld_cnt; ++j)
        {
            set_selfld_of_rec_values(rttable->rec_values, j);
        }
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! set_eval_of_rec_values_other

/*! \breif RRTABLE의 FIELDVALUES_T안의 eval_fld의 값을 설정한다
 ************************************
 * \param handle(in) :
 * \param select(in/out) :
 ************************************
 * \return  RET_SUCCESS/RET_ERROR
 ************************************
 * \note
 *  - 설정 순서
 *      1. correlated column 설정
 *      2. OUTER-JOIN CONDITION CHECK
 *      3. WHERE-CLAUSE에  OR 조건이 들어간 경우
 *         아래의 값은 NULL로 나오므로 where-clause에 있는 값들을 모두 뒤져야 한다
 *      4. GROUPBY INFO
 *      5. HAVING INFO
 *      6. ORDERBY INFO
 *  - 함수의 call 순서
 *      1. set_eval_of_rec_values_other
 *      2. int set_eval_of_rec_values_where
 *      3. int set_sel_of_rec_values
 *
 *****************************************************************************/
int
set_eval_of_rec_values_other(int handle, T_SELECT * select)
{
    T_GROUPBYDESC *groupby;
    T_ORDERBYDESC *orderby;

    T_LIST_JOINTABLEDESC *fromlist;
    T_POSTFIXELEM *elem;
    T_RTTABLE *rttable;
    int i, j, k;
    CORRELATED_ATTR *cl;
    T_EXPRESSIONDESC *condition;

#ifdef MDB_DEBUG
    sc_assert(select != NULL, __FILE__, __LINE__);
#endif

    // 1. correlated column 설정
    for (cl = select->planquerydesc.querydesc.correl_attr; cl; cl = cl->next)
    {
        elem = cl->ptr;

        if (elem->elemtype != SPT_VALUE ||
                elem->u.value.valueclass != SVC_VARIABLE)
            continue;


        fromlist =
                &(elem->u.value.attrdesc.table.correlated_tbl->planquerydesc.
                querydesc.fromlist);
        for (j = 0; j < fromlist->len; ++j)
        {
            rttable =
                    &(elem->u.value.attrdesc.table.correlated_tbl->rttable[j]);
            if (rttable->table.tablename == NULL)
                continue;       // correlated가 없는 경우는 skip한다

            if (mdb_strcmp(rttable->table.tablename,
                            elem->u.value.attrdesc.table.tablename))
                continue;

            set_evalfld_of_rec_values(rttable->rec_values,
                    elem->u.value.attrdesc.posi.ordinal);
        }
    }

    // 2. OUTER-JOIN CONDITION CHECK
    fromlist = &(select->planquerydesc.querydesc.fromlist);
    for (i = 0; i < fromlist->len; ++i)
    {
        for (j = 0; j < fromlist->list[i].condition.len; ++j)
        {
            elem = fromlist->list[i].condition.list[j];
            if (elem->elemtype != SPT_VALUE ||
                    elem->u.value.valueclass != SVC_VARIABLE)
                continue;

            for (k = 0; k < fromlist->len; ++k)
            {
                rttable = &(select->rttable[k]);

                if (mdb_strcmp(rttable->table.tablename,
                                elem->u.value.attrdesc.table.tablename))
                    continue;

                set_evalfld_of_rec_values(rttable->rec_values,
                        elem->u.value.attrdesc.posi.ordinal);
            }
        }
    }

    orderby = &(select->planquerydesc.querydesc.orderby);
    groupby = &(select->planquerydesc.querydesc.groupby);
    fromlist = &(select->planquerydesc.querydesc.fromlist);
    for (i = 0; i < fromlist->len; ++i)
    {
        rttable = &(select->rttable[i]);

        // 3. WHERE-CLAUSE에  OR 조건이 들어간 경우
        // 아래의 값은 NULL로 나오므로 where-clause에 있는 값들을 모두 뒤져야 한다
        if (select->planquerydesc.querydesc.expr_list == NULL)
        {
            condition = &(select->planquerydesc.querydesc.condition);
            for (j = 0; j < condition->len; ++j)
            {
                elem = condition->list[j];

                if (elem->elemtype != SPT_VALUE ||
                        elem->u.value.valueclass != SVC_VARIABLE)
                    continue;

                if (mdb_strcmp(rttable->table.tablename,
                                elem->u.value.attrdesc.table.tablename))
                    continue;

                set_evalfld_of_rec_values(rttable->rec_values,
                        elem->u.value.attrdesc.posi.ordinal);
            }
        }

        // 4. GROUPBY INFO
        for (j = 0; j < groupby->len; ++j)
        {
            if (mdb_strcmp(rttable->table.tablename,
                            groupby->list[j].attr.table.tablename))
                continue;

            set_evalfld_of_rec_values(rttable->rec_values,
                    groupby->list[j].attr.posi.ordinal);
        }

        // 5. HAVING INFO
        for (j = 0; j < groupby->having.len; ++j)
        {
            elem = groupby->having.list[j];
            if (elem->elemtype != SPT_VALUE ||
                    elem->u.value.valueclass != SVC_VARIABLE)
                continue;

            if (mdb_strcmp(rttable->table.tablename,
                            elem->u.value.attrdesc.table.tablename))
                continue;

            set_evalfld_of_rec_values(rttable->rec_values,
                    elem->u.value.attrdesc.posi.ordinal);
        }

        // 6. ORDERBY INFO
        // TODO : ORDER BY의 경우.. SELECT로 
        k = 0;
        if (orderby->len > 0 && fromlist->list[i].table.correlated_tbl == NULL
                && fromlist->len == 1)
        {
            T_NESTING *nest = &(select->planquerydesc.nest[i]);

            if (orderby->len <= nest->index_field_num)
            {
                for (j = 0; j < orderby->len; j++)
                {
                    if (orderby->list[j].param_idx > 0)
                        continue;
                    if (select->planquerydesc.querydesc.select_star &&
                            orderby->list[j].s_ref != -1)
                        continue;

                    if (orderby->list[j].attr.Htable !=
                            nest->index_field_info[j].Htable ||
                            orderby->list[j].attr.Hattr !=
                            nest->index_field_info[j].Hattr)
                        break;

                    if (orderby->list[j].attr.order !=
                            nest->index_field_info[j].order)
                        break;
                }

                if (j == orderby->len)
                    k = 1;
            }
        }

        if (k != 1)
            for (j = 0; j < orderby->len; ++j)
            {
                if (orderby->list[j].param_idx > 0)
                    continue;
                if (select->planquerydesc.querydesc.select_star &&
                        orderby->list[j].s_ref != -1)
                    continue;
                if (orderby->list[j].s_ref != -1
                        && orderby->list[j].i_ref == 0)
                    continue;
                if (mdb_strcmp(rttable->table.tablename,
                                orderby->list[j].attr.table.tablename))
                    continue;

                set_evalfld_of_rec_values(rttable->rec_values,
                        orderby->list[j].attr.posi.ordinal);
            }
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! set_eval_of_rec_values_where

/*! \breif RRTABLE의 FIELDVALUES_T안의 eval_fld의 값을 설정한다
 ************************************
 * \param handle(in) :
 * \param select(in/out) :
 ************************************
 * \return  RET_SUCCESS/RET_ERROR
 ************************************
 * \note
 *  - 설정 순서
 *      1. SQL Filter에 사용되는 fields를 설정함
 *      2. index key values(case : join)
 *          2.1. index's min
 *          2.2. index's max
 *          2.3.  2개 이상의 table을 join 하는 경우 outer table의 db key값
 *          2.4. 2개 이상의 table을 join 하는 경우 outer table의 SQL Filter값
 *      3. sort merge
 *  - 함수의 call 순서
 *      1. set_eval_of_rec_values_other
 *      2. int set_eval_of_rec_values_where
 *      3. int set_sel_of_rec_values
 *
 *****************************************************************************/
int
set_eval_of_rec_values_where(int handle, T_SELECT * select)
{
    T_RTTABLE *rttable;
    T_RTTABLE *outer_rttable;
    T_RTTABLE *inner_rttable;

    T_LIST_JOINTABLEDESC *fromlist;
    T_NESTING *nest;
    EXPRDESC_LIST *cur_expr_list;
    T_EXPRESSIONDESC *expr;
    T_POSTFIXELEM *elem;
    int i, j, k;
    T_NESTING *inner_nest;
    int idx;
    int cur_run;
    int n_tables;
    register char *tablename;
    EXPRDESC_LIST *expr_list;

#ifdef MDB_DEBUG
    sc_assert(select != NULL, __FILE__, __LINE__);
#endif

    fromlist = &(select->planquerydesc.querydesc.fromlist);
    n_tables = fromlist->len;

    // 1. SQL Filter에 사용되는 fields를 설정함
    for (i = 0; i < n_tables; ++i)
    {
        for (cur_expr_list = select->rttable[i].sql_expr_list;
                cur_expr_list; cur_expr_list = cur_expr_list->next)
        {
            expr = cur_expr_list->ptr;
            for (j = 0; j < expr->len; ++j)
            {
                elem = expr->list[j];
                if (elem->elemtype != SPT_VALUE ||
                        elem->u.value.valueclass != SVC_VARIABLE)
                    continue;

                for (k = 0; k < n_tables; ++k)
                {
                    rttable = &(select->rttable[k]);
                    if (mdb_strcmp(rttable->table.tablename,
                                    elem->u.value.attrdesc.table.tablename))
                        continue;
                    if (elem->u.value.attrdesc.Hattr == -1)
                    {
                        continue;
                    }

                    set_evalfld_of_rec_values(rttable->rec_values,
                            elem->u.value.attrdesc.posi.ordinal);
                }
            }
        }
        if (select->rttable[i].cnt_merge_item > 0)
        {
            for (cur_expr_list = select->rttable[i].db_expr_list;
                    cur_expr_list; cur_expr_list = cur_expr_list->next)
            {
                expr = cur_expr_list->ptr;
                for (j = 0; j < expr->len; ++j)
                {
                    elem = expr->list[j];
                    if (elem->elemtype != SPT_VALUE ||
                            elem->u.value.valueclass != SVC_VARIABLE)
                    {
                        continue;
                    }

                    for (k = 0; k < n_tables; ++k)
                    {
                        rttable = &(select->rttable[k]);
                        if (mdb_strcmp(rttable->table.tablename,
                                        elem->u.value.attrdesc.table.
                                        tablename))
                        {
                            continue;
                        }

                        if (elem->u.value.attrdesc.Hattr == -1)
                        {
                            continue;
                        }

                        set_evalfld_of_rec_values(rttable->rec_values,
                                elem->u.value.attrdesc.posi.ordinal);
                    }
                }
            }
        }
        if (fromlist->outer_join_exists)
        {
            for (cur_expr_list = select->planquerydesc.querydesc.expr_list;
                    cur_expr_list; cur_expr_list = cur_expr_list->next)
            {
                expr = cur_expr_list->ptr;
                for (j = 0; j < expr->len; ++j)
                {
                    elem = expr->list[j];
                    if (elem->elemtype != SPT_VALUE ||
                            elem->u.value.valueclass != SVC_VARIABLE)
                        continue;

                    for (k = 0; k < n_tables; ++k)
                    {
                        rttable = &(select->rttable[k]);
                        if (mdb_strcmp(rttable->table.tablename,
                                        elem->u.value.attrdesc.table.
                                        tablename))
                            continue;

                        if (elem->u.value.attrdesc.Hattr == -1)
                        {
                            continue;
                        }

                        set_evalfld_of_rec_values(rttable->rec_values,
                                elem->u.value.attrdesc.posi.ordinal);
                    }
                }
            }
        }
    }

    // 2. index key values(case : join)
    for (cur_run = 0; cur_run < n_tables; ++cur_run)
    {
        outer_rttable = &(select->rttable[cur_run]);

        for (i = (cur_run + 1); i < n_tables; i++)
        {
            idx = select->rttable[i].table_ordinal;
            inner_nest = &(select->planquerydesc.nest[idx]);

            // 1. min
            if (inner_nest->min)
            {
                for (j = 0; j < inner_nest->index_field_num; j++)
                {
                    expr = &inner_nest->min[j].expr;
                    if (expr->len == 0)
                    {
                        break;
                    }

                    for (k = 0; k < expr->len; k++)
                    {
                        elem = expr->list[k];

                        if (elem->elemtype == SPT_OPERATOR ||
                                elem->elemtype == SPT_FUNCTION ||
                                elem->elemtype == SPT_AGGREGATION ||
                                elem->u.value.valueclass == SVC_CONSTANT ||
                                elem->u.value.valueclass == SVC_NONE ||
                                elem->u.value.attrdesc.Hattr == -1 ||
                                mdb_strcmp(elem->u.value.attrdesc.table.
                                        tablename,
                                        outer_rttable->table.tablename))
                        {
                            continue;
                        }

                        set_evalfld_of_rec_values(outer_rttable->rec_values,
                                elem->u.value.attrdesc.posi.ordinal);
                    }
                }
            }

            // 2. max
            if (inner_nest->max)
            {
                for (j = 0; j < inner_nest->index_field_num; j++)
                {
                    expr = &inner_nest->max[j].expr;
                    if (expr->len == 0)
                    {
                        break;
                    }

                    for (k = 0; k < expr->len; k++)
                    {
                        elem = expr->list[k];

                        if (elem->elemtype == SPT_OPERATOR ||
                                elem->elemtype == SPT_FUNCTION ||
                                elem->elemtype == SPT_AGGREGATION ||
                                elem->u.value.valueclass == SVC_CONSTANT ||
                                elem->u.value.valueclass == SVC_NONE ||
                                elem->u.value.attrdesc.Hattr == -1 ||
                                mdb_strcmp(elem->u.value.attrdesc.table.
                                        tablename,
                                        outer_rttable->table.tablename))
                        {
                            continue;
                        }

                        set_evalfld_of_rec_values(outer_rttable->rec_values,
                                elem->u.value.attrdesc.posi.ordinal);
                    }
                }
            }

            // 2개 이상의 table을 join 하는 경우 outer table의 db key값이 사용되므로
            inner_rttable = &(select->rttable[i]);
            for (expr_list = inner_rttable->db_expr_list; expr_list;
                    expr_list = expr_list->next)
            {
                expr = expr_list->ptr;

                tablename = outer_rttable->table.aliasname ?
                        outer_rttable->table.aliasname : outer_rttable->table.
                        tablename;

                for (j = 0; j < expr->len; ++j)
                {
                    if (expr->list[j]->elemtype != SPT_VALUE ||
                            expr->list[j]->u.value.valueclass != SVC_VARIABLE)
                        continue;

                    /* subquery의 경우 table이름이 실제 이름과 다를수 있다.
                       왜냐면 (subquery_) as table_alias_name(i) 일 경우
                       i의 테이블 이름은 subquery_의 from절에 오는 테이블
                       이름이 지만, rttable에는 (subquery_)의 결과로 생기는
                       임시 테이블 이름이 들어가기 때문이다.
                       다른 경우에도 테이블 이름을 alias시켰으면 테이블
                       이름 대신 alias이름으로 평가해야 하기 때문에 ...
                       쫑나는 경우는 이미 check_validation()에서 검증했다 ? */
                    if (mdb_strcmp(expr->list[j]->u.value.attrdesc.table.
                                    aliasname ? expr->list[j]->u.value.
                                    attrdesc.table.aliasname : expr->list[j]->
                                    u.value.attrdesc.table.tablename,
                                    tablename))
                        continue;
                    if (expr->list[j]->u.value.attrdesc.Hattr == -1)
                    {
                        continue;
                    }

                    set_evalfld_of_rec_values(outer_rttable->rec_values,
                            expr->list[j]->u.value.attrdesc.posi.ordinal);
                }
            }

            /* 2개 이상의 table을 join 하는 경우 outer table의
               SQL Filter값이 사용되므로 */
            for (expr_list = inner_rttable->sql_expr_list; expr_list;
                    expr_list = expr_list->next)
            {
                expr = expr_list->ptr;

                tablename = outer_rttable->table.aliasname ?
                        outer_rttable->table.aliasname : outer_rttable->table.
                        tablename;

                for (j = 0; j < expr->len; ++j)
                {
                    if (expr->list[j]->elemtype != SPT_VALUE ||
                            expr->list[j]->u.value.valueclass != SVC_VARIABLE)
                        continue;

                    /* subquery의 경우 table이름이 실제 이름과 다를수 있다.
                       왜냐면 (subquery_) as table_alias_name(i) 일 경우 i의
                       테이블 이름은 subquery_의 from절에 오는 테이블 이름이
                       지만, rttable에는 (subquery_)의 결과로 생기는 임시
                       테이블 이름이 들어가기 때문이다.
                       다른 경우에도 테이블 이름을 alias시켰으면 테이블 이름
                       대신 alias이름으로 평가해야 하기 때문에 ...
                       쫑나는 경우는 이미 check_validation()에서 검증했다 ? */

                    if (mdb_strcmp(expr->list[j]->u.value.attrdesc.table.
                                    aliasname ? expr->list[j]->u.value.
                                    attrdesc.table.aliasname : expr->list[j]->
                                    u.value.attrdesc.table.tablename,
                                    tablename))
                        continue;
                    if (expr->list[j]->u.value.attrdesc.Hattr == -1)
                    {
                        continue;
                    }

                    set_evalfld_of_rec_values(outer_rttable->rec_values,
                            expr->list[j]->u.value.attrdesc.posi.ordinal);
                }
            }
        }
    }

    // SORT-MERGE의 경우를 설정해준다
    for (i = 0; i < n_tables; ++i)
    {
        // 1. SQL index key
        rttable = &(select->rttable[i]);
        nest = &(select->planquerydesc.nest[rttable->table_ordinal]);

        for (j = 0; j < rttable->cnt_merge_item; ++j)
        {
#if defined(MDB_DEBUG)
            if (mdb_strcmp(rttable->table.tablename,
                            nest->index_field_info[j].table.tablename))
                sc_assert(0, __FILE__, __LINE__);

#endif
            if (nest->index_field_info[j].Hattr == -1)
            {
                continue;
            }

            set_evalfld_of_rec_values(rttable->rec_values,
                    nest->index_field_info[j].posi.ordinal);
        }
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! set_sel_of_rec_values

/*! \breif RRTABLE의 FIELDVALUES_T안의 sel_fld의 정보를 설정한다
 ************************************
 * \param handle(in) :
 * \param select(in/out) :
 ************************************
 * \return  RET_SUCCESS/RET_ERROR
 ************************************
 * \note
 *  - caution : 함수 호출 순서
 *      1. set_eval_of_rec_values()
 *      3. set_sel_of_rec_values()
 *
 *****************************************************************************/
int
set_rec_values_info_select(int handle, T_SELECT * select)
{
    int ret;
    int i;

#ifdef MDB_DEBUG
    sc_assert(select != NULL, __FILE__, __LINE__);
#endif

    if (select->first_sub)
    {
        ret = set_rec_values_info_select(handle, select->first_sub);
        if (ret != RET_SUCCESS)
            return ret;
    }
    if (select->sibling_query)
    {
        ret = set_rec_values_info_select(handle, select->sibling_query);
        if (ret != RET_SUCCESS)
            return ret;
    }
    if (select->tmp_sub)
    {
        ret = set_rec_values_info_select(handle, select->tmp_sub);
        if (ret != RET_SUCCESS)
            return ret;
    }

    if (select->planquerydesc.querydesc.setlist.len > 0)
    {
        T_EXPRESSIONDESC *setlist;

        setlist = &select->planquerydesc.querydesc.setlist;

        for (i = 0; i < setlist->len; i++)
        {
            if (setlist->list[i]->elemtype == SPT_SUBQUERY)
            {
                ret = set_rec_values_info_select(handle,
                        setlist->list[i]->u.subq);
                if (ret != RET_SUCCESS)
                {
                    return ret;
                }
            }
        }
        return RET_SUCCESS;
    }

    ret = set_eval_of_rec_values_other(handle, select);
    if (ret != RET_SUCCESS)
        return ret;

    ret = set_eval_of_rec_values_where(handle, select);
    if (ret != RET_SUCCESS)
        return ret;

    ret = set_sel_of_rec_values(handle, select);
    if (ret != RET_SUCCESS)
        return ret;


    return RET_SUCCESS;
}

/*****************************************************************************/
//! set_scanrid_in_nest

/*! \breif 
 ************************************
 * \param nest(in/out) :
 * \param expr_list(in) :
 ************************************
 * \return  RET_SUCCESS/RET_ERROR
 ************************************
 * \note
 *
 *****************************************************************************/
int
set_scanrid_in_nest(T_NESTING * nest, T_EXPRESSIONDESC * condition)
{
    int i, ret = 1;
    T_POSTFIXELEM *var_elem, *con_elem;

    if (condition->list[2]->u.op.type == SOT_EQ)
    {
        for (i = 0; i < 2; i++)
        {
            var_elem = condition->list[i];
            if (var_elem->u.value.valueclass == SVC_VARIABLE &&
                    var_elem->u.value.attrdesc.type == DT_OID)
            {
                i = ~i & 1;
                con_elem = condition->list[i];

                if (con_elem->u.value.valueclass != SVC_CONSTANT)
                    break;

                if (con_elem->u.value.u.oid != NULL_OID &&
                        (ret = dbi_check_valid_rid(var_elem->u.value.attrdesc.
                                        table.tablename,
                                        con_elem->u.value.u.oid)) < 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    return ret;
                }
                if (con_elem->u.value.valuetype == DT_NULL_TYPE)
                    nest->is_param_scanrid = 1;

                nest->scanrid = con_elem->u.value.u.oid;
                break;
            }
        }
    }
    else if (condition->list[0]->u.value.valueclass == SVC_VARIABLE &&
            condition->list[0]->u.value.attrdesc.type == DT_OID
            && condition->list[condition->len - 1]->u.op.type == SOT_IN)
    {
        if (condition->list[1]->u.value.u.oid != NULL_OID
                && (ret =
                        dbi_check_valid_rid(condition->list[0]->u.value.
                                attrdesc.table.tablename,
                                condition->list[1]->u.value.u.oid)) < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return ret;
        }

        nest->scanrid = condition->list[1]->u.value.u.oid;
        nest->ridin_cnt = condition->len - 2;
    }

    return ret;
}
