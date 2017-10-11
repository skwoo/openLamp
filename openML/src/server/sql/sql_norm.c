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

#include "sql_norm.h"
#include "sql_util.h"

#include "mdb_er.h"
#include "ErrorCode.h"

/*************************************************
    begin of tree utilities {
 *************************************************/
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
calc_exprlen4tree(T_NODE * node)
{
    int len = 0;

    if (node->left)
        len += calc_exprlen4tree(node->left);

    if (node->right)
        len += calc_exprlen4tree(node->right);

    len += node->end - node->start + 1;

    return len;
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
free_tree(T_NODE * node)
{
    if (node)
    {
        if (node->left)
            free_tree(node->left);

        if (node->right)
            free_tree(node->right);

        PMEM_FREENUL(node);
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
T_NODE *
copy_tree(T_NODE * node)
{
    T_NODE *new = NULL;
    T_NODE *left = NULL, *right = NULL;

    if (node)
    {
        if (node->left)
            left = copy_tree(node->left);

        if (node->right)
            right = copy_tree(node->right);

        new = (T_NODE *) PMEM_ALLOC(sizeof(T_NODE));
        if (new == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            free_tree(left);
            free_tree(right);
            return NULL;
        }
        new->type = node->type;
        new->start = node->start;
        new->end = node->end;
        new->left = left;
        new->right = right;
    }

    return new;
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
#if defined(MDB_DEBUG)
void
print_tree(T_NODE * node)
{
    if (node)
    {
        printf("%d ", node->type);

        if (node->left)
            print_tree(node->left);

        if (node->right)
            print_tree(node->right);
    }
}
#endif

/*****************************************************************************/
//! estimate_cnf_size 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param node(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *      cond_normalize()에서 호출됨..
 *****************************************************************************/
int
estimate_cnf_size(T_NODE * node)
{
    int count;

    switch (node->type)
    {
    case SOT_AND:
    case SOT_OR:
        count = estimate_cnf_size(node->left);
        if (node->type == SOT_AND)
            count += estimate_cnf_size(node->right);
        else
            count *= estimate_cnf_size(node->right);
        break;
    default:
        count = 1;
        break;
    }
    return count;

}           /* estimate_cnf_size */

/*************************************************
    } end of tree utilities
 *************************************************/

/*************************************************
    begin of stack utilities {
 *************************************************/
typedef struct node_stack
{
    T_NODE *value;              /* ptr to tree */
    struct node_stack *next;
} t_node_stack;

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
init_node_stack(t_node_stack ** stack)
{
    *stack = NULL;
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
remove_node_stack(t_node_stack ** stack)
{
    t_node_stack *next;

    while (*stack != NULL)
    {
        next = (*stack)->next;
        free_tree((*stack)->value);
        PMEM_FREENUL(*stack);
        *stack = next;
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
int
push_node_stack(T_NODE * value, t_node_stack ** stack)
{
    t_node_stack *new;

    new = PMEM_ALLOC(sizeof(t_node_stack));
    if (new == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return RET_ERROR;
    }
    new->value = value;
    new->next = *stack;
    *stack = new;

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
pop_node_stack(T_NODE ** value, t_node_stack ** stack)
{
    t_node_stack *old;

    if (stack == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_STACKUNDERFLOW, 0);
        return SQL_E_STACKUNDERFLOW;
    }

    old = *stack;
    *stack = old->next;
    *value = old->value;
    PMEM_FREENUL(old);

    return RET_SUCCESS;
}

/*************************************************
    } end of stack utilities
 *************************************************/

/*************************************************
    begin of expression utilities {
 *************************************************/
/*****************************************************************************/
//! add_expression 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param des(in/out) :
 * \param src(in) : 
 * \param src_start() :
 * \param src_end() :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  이 함수는 현재.. trans_tree2expr_work()에서만 사용된다.
 *
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
T_EXPRESSIONDESC *
add_expression(T_EXPRESSIONDESC * des,
        T_EXPRESSIONDESC * src, int src_start, int src_end)
{
    T_POSTFIXELEM *des_elem, *src_elem;
    int i, len;

    if (src == NULL || des == NULL)
        return NULL;

    if ((des->max - des->len) < (src_end - src_start + 1))
        return NULL;

    for (i = src_start; i <= src_end; i++)
    {
        src_elem = src->list[i];
        des_elem = des->list[des->len++];
        switch (src_elem->elemtype)
        {
        case SPT_VALUE:
            sc_memcpy(des_elem, src_elem, sizeof(T_POSTFIXELEM));

            if (src_elem->u.value.valueclass == SVC_CONSTANT)
            {
                DataType _type = src_elem->u.value.valuetype;

                /* type이 존재하는 null인 경우 skip */
                if (src_elem->u.value.is_null)
                {
                    break;
                }

                switch (_type)
                {
                case DT_VARCHAR:
                case DT_CHAR:
                case DT_NVARCHAR:
                case DT_NCHAR:
                    if (!src_elem->u.value.u.ptr)
                        break;
                    len = strlen_func[_type] (src_elem->u.value.u.ptr) + 1;

                    sql_value_ptrInit(&des_elem->u.value);

                    if (sql_value_ptrAlloc(&des_elem->u.value, len) < 0)
                        return NULL;

                    strncpy_func[_type] (des_elem->u.value.u.ptr,
                            src_elem->u.value.u.ptr, len);

                default:
                    break;

                }
            }
            else
            {   /* SVC_VARIABLE */
                T_ATTRDESC *des_attr, *src_attr;

                des_attr = &des_elem->u.value.attrdesc;
                src_attr = &src_elem->u.value.attrdesc;

                if (src_attr->table.tablename)
                {
                    des_attr->table.tablename =
                            PMEM_STRDUP(src_attr->table.tablename);
                    if (!des_attr->table.tablename)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_OUTOFMEMORY, 0);
                        return NULL;
                    }
                }

                if (src_attr->table.aliasname)
                {
                    des_attr->table.aliasname =
                            PMEM_STRDUP(src_attr->table.aliasname);
                    if (!des_attr->table.aliasname)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_OUTOFMEMORY, 0);
                        return NULL;
                    }
                }
                else
                    src_attr->table.aliasname = NULL;

                des_attr->table.correlated_tbl =
                        src_attr->table.correlated_tbl;

                if (src_attr->attrname)
                {
                    des_attr->attrname = PMEM_STRDUP(src_attr->attrname);
                    if (!des_attr->attrname)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_OUTOFMEMORY, 0);
                        return NULL;
                    }
                }

                if (src_attr->defvalue.defined &&
                        src_attr->defvalue.not_null
                        && src_attr->defvalue.u.ptr)
                {
                    if (IS_BS(src_attr->type))
                    {
                        des_attr->defvalue.u.ptr =
                                PMEM_STRDUP(src_attr->defvalue.u.ptr);
                        if (!des_attr->defvalue.u.ptr)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_OUTOFMEMORY, 0);
                            return NULL;
                        }
                    }
                    else if (IS_NBS(src_attr->type))
                    {
                        des_attr->defvalue.u.Wptr =
                                PMEM_WSTRDUP(src_attr->defvalue.u.Wptr);
                        if (!des_attr->defvalue.u.Wptr)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_OUTOFMEMORY, 0);
                            return NULL;
                        }
                    }
                    else if (IS_BYTE(src_attr->type))
                    {
                        des_attr->defvalue.u.ptr = NULL;
                        if (src_attr->defvalue.value_len)
                        {
                            des_attr->defvalue.u.ptr =
                                    PMEM_ALLOC(src_attr->defvalue.value_len);
                            if (des_attr->defvalue.u.ptr == NULL)
                            {
                                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                        SQL_E_OUTOFMEMORY, 0);
                                return NULL;

                            }
                            sc_memcpy(des_attr->defvalue.u.ptr,
                                    src_attr->defvalue.u.ptr,
                                    src_attr->defvalue.value_len);
                        }
                    }
                }
            }
            break;

        default:       /* SPT_OPERATOR, SPT_FUNCTION, SPT_AGGREGATIN */
            sc_memcpy(des_elem, src_elem, sizeof(T_POSTFIXELEM));
            break;
        }
    }       /* for (i = src_start; i <= src_end; i++) */

    return des;

}           /* add_expression */

/*************************************************
    } end of expression utilities
 *************************************************/

/*************************************************
    begin of tree transformation {
 *************************************************/
/*****************************************************************************/
//! trans_expr2tree 

/*! \breif  TREE를 이용해서 NOMARIZE를 시킨다.(stack을 이용)
 ************************************
 * \param expr()
 * \param tree(): 
 ************************************
 * \return  return_value
 ************************************
 * \note
 *      cond_normalize()에서 호출된다.
 *
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
T_NODE *
trans_expr2tree(T_EXPRESSIONDESC * expr, T_NODE * tree)
{
    T_POSTFIXELEM *elem;
    T_NODE *node = NULL, *arg1, *arg2;
    int start, i;
    t_node_stack *stack;

    init_node_stack(&stack);
    start = -1;
    for (i = 0; i < expr->len; i++)
    {
        elem = expr->list[i];
        if (start == -1)
            start = i;
        if (elem->elemtype == SPT_OPERATOR || elem->elemtype == SPT_SUBQ_OP)
        {
            switch (elem->u.op.type)
            {
                /* boolean operators */
            case SOT_AND:
            case SOT_OR:
                pop_node_stack(&arg2, &stack);
                pop_node_stack(&arg1, &stack);
                node = (T_NODE *) PMEM_ALLOC(sizeof(T_NODE));
                if (node == NULL)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY,
                            0);
                    remove_node_stack(&stack);
                    return NULL;
                }
                node->type = elem->u.op.type;
                node->start = i;
                node->end = i;
                node->left = arg1;
                node->right = arg2;
                push_node_stack(node, &stack);
                start = -1;
                break;

            case SOT_NOT:
                pop_node_stack(&arg1, &stack);
                node = (T_NODE *) PMEM_ALLOC(sizeof(T_NODE));
                if (node == NULL)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY,
                            0);
                    remove_node_stack(&stack);
                    return NULL;
                }
                node->type = elem->u.op.type;
                node->start = i;
                node->end = i;
                node->left = arg1;
                node->right = NULL;
                push_node_stack(node, &stack);
                start = -1;
                break;

                /* predicate operators */
            case SOT_ISNULL:
            case SOT_ISNOTNULL:
            case SOT_LT:
            case SOT_LE:
            case SOT_GT:
            case SOT_GE:
            case SOT_EQ:
            case SOT_NE:
            case SOT_IN:
            case SOT_NIN:
            case SOT_LIKE:
            case SOT_ILIKE:
            case SOT_SOME_LT:
            case SOT_SOME_LE:
            case SOT_SOME_GT:
            case SOT_SOME_GE:
            case SOT_SOME_EQ:
            case SOT_SOME_NE:
            case SOT_ALL_LT:
            case SOT_ALL_LE:
            case SOT_ALL_GT:
            case SOT_ALL_GE:
            case SOT_ALL_EQ:
            case SOT_ALL_NE:
            case SOT_EXISTS:
                node = (T_NODE *) PMEM_ALLOC(sizeof(T_NODE));
                if (node == NULL)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY,
                            0);
                    remove_node_stack(&stack);
                    return NULL;
                }
                node->type = elem->u.op.type;
                node->start = start;
                node->end = i;
                node->left = node->right = NULL;
                push_node_stack(node, &stack);
                start = -1;
                break;

            default:
                break;
            }
        }
        else if (elem->elemtype == SPT_FUNCTION)
        {
            if (elem->u.func.type == SFT_HEXCOMP)
            {
                node = (T_NODE *) PMEM_ALLOC(sizeof(T_NODE));
                if (node == NULL)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY,
                            0);
                    remove_node_stack(&stack);
                    return NULL;
                }
                node->type = SOT_HEXCOMP;
                node->start = start;
                node->end = i;
                node->left = node->right = NULL;
                push_node_stack(node, &stack);
                start = -1;
            }
        }
        /* else SPT_VALUE or SPT_AGGREGATION */
    }

    pop_node_stack(&node, &stack);
    remove_node_stack(&stack);

    return node;

}           /* trans_expr2tree */

/*****************************************************************************/
//! trans_tree2expr_work 

/*! \breif  nomalize된 T_POSTFIXELEM 정보들이 이동하는 곳
 *          cond의 정보들이 expr에 들어온다.
 ************************************
 * \param tree():
 * \param expr(in/out): 
 * \param cond(in):
 ************************************
 * \return  return_value
 ************************************
 * \note
 *      - T_EXPRESSIONDESC 구조체 변경
 *      - 여기서 leak이 발생함..(expr이 제대로 할당되는지 확인할것)
 *
 *****************************************************************************/
int
trans_tree2expr_work(T_NODE * tree, T_EXPRESSIONDESC * expr,
        T_EXPRESSIONDESC * cond)
{
    if (tree->left)
        trans_tree2expr_work(tree->left, expr, cond);

    if (tree->right)
        trans_tree2expr_work(tree->right, expr, cond);

    if (add_expression(expr, cond, tree->start, tree->end) == NULL)
        return RET_ERROR;

    /* during normalization, some operators may be negated.
       so copy the operator */
    expr->list[expr->len - 1]->u.op.type =
            (tree->type == SOT_HEXCOMP ? SFT_HEXCOMP :
            (tree->type == SOT_HEXNOTCOMP ? SFT_HEXNOTCOMP : tree->type));

    return RET_SUCCESS;

}           /* trans_tree2expr_work */

/*****************************************************************************/
//! trans_tree2expr 

/*! \breif  NORMALIZE된 정보를 옮기는 곳..
 ************************************
 * \param tree():
 * \param expr(in/out): expr에 cond와 tree의 정보들이 종합되어 들어감 
 * \param cond(in): condition 정보
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - T_EXPRESSIONDESC 구조체 변경
 *  - T_POSTFIXELEM을 할당한다
 *
 *****************************************************************************/
T_EXPRESSIONDESC *
trans_tree2expr(T_NODE * tree, T_EXPRESSIONDESC * expr,
        T_EXPRESSIONDESC * cond)
{
    int i = 0;

    if (expr == NULL)
    {
        expr = (T_EXPRESSIONDESC *) PMEM_ALLOC(sizeof(T_EXPRESSIONDESC));
        if (expr)
        {
            expr->len = 0;
            expr->max = calc_exprlen4tree(tree);

            expr->list = sql_mem_get_free_postfixelem_list(NULL, expr->max);
            if (expr->list == NULL)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return NULL;
            }

            for (i = 0; i < expr->max; ++i)
            {
                expr->list[i] = sql_mem_get_free_postfixelem(NULL);
                if (expr->list[i] == NULL)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY,
                            0);
                    return NULL;
                }
                sc_memset(expr->list[i], 0x00, sizeof(T_POSTFIXELEM));
            }
        }
    }

    if (trans_tree2expr_work(tree, expr, cond) != RET_SUCCESS)
        return NULL;

    return expr;
}           /* trans_tree2expr */

/*****************************************************************************/
//! trans_tree2exprlist 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param tree(in) :
 * \param expr_list(in) : 
 * \param cond(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *      cond_normalize()에서 호출됨
 *
 *****************************************************************************/
EXPRDESC_LIST *
trans_tree2exprlist(T_NODE * tree, EXPRDESC_LIST * expr_list,
        T_EXPRESSIONDESC * cond)
{
    T_EXPRESSIONDESC *expr;
    int ret;

    if (tree->type == SOT_AND)
    {
        expr_list = trans_tree2exprlist(tree->left, expr_list, cond);
        if (expr_list == NULL)
            return NULL;
        expr_list = trans_tree2exprlist(tree->right, expr_list, cond);
        if (expr_list == NULL)
            return NULL;
    }
    else
    {
        expr = trans_tree2expr(tree, NULL, cond);
        if (expr == NULL)
            return NULL;
        LINK_EXPRDESC_LIST(expr_list, expr, ret);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return NULL;
        }
    }
    return expr_list;
}

/************************************************
    } end of tree transformation
 ************************************************/

/************************************************
    begin of NOT elimination {
 ************************************************/
/*****************************************************************************/
//! norm_negate_op 

/*! \breif  NOT의 들어간 EXPRESSOIN에서 NOT을 제거한다
 ************************************
 * \param node(in/out)
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *  - AND/OR인 경우 2번 바뀌서, 값을 원래대로 원복시키는 버그가 존재했다
 *****************************************************************************/
void
norm_negate_op(T_NODE * node)
{
    T_NODE *tmp;

    T_OPTYPE neg_op_tbl[] = {
        0, SOT_OR, SOT_AND, 0, SOT_NLIKE,
        SOT_NILIKE,
        SOT_ISNOTNULL, SOT_ISNULL, 0, 0, 0, 0, 0, 0, 0,
        SOT_GE, SOT_GT, SOT_LE, SOT_LT, SOT_NE, SOT_EQ,
        SOT_NIN, SOT_IN, SOT_LIKE,
        SOT_ILIKE,
        0, SOT_ALL_GE, SOT_ALL_GT, SOT_ALL_LE, SOT_ALL_LT,
        SOT_ALL_NE, SOT_ALL_EQ, 0,
        SOT_SOME_GE, SOT_SOME_GT, SOT_SOME_LE, SOT_SOME_LT,
        SOT_SOME_NE, SOT_SOME_EQ, SOT_NEXISTS, SOT_EXISTS,
        SOT_HEXCOMP, SOT_HEXNOTCOMP
    };

    switch (node->type)
    {
    case SOT_AND:
        norm_negate_op(node->left);
        norm_negate_op(node->right);
        break;

    case SOT_OR:
        norm_negate_op(node->left);
        norm_negate_op(node->right);
        break;

    case SOT_NOT:
        tmp = node->left;
        sc_memcpy(node, node->left, sizeof(T_NODE));
        PMEM_FREENUL(tmp);
        break;

    default:
        break;
    }

    node->type = neg_op_tbl[node->type];

}           /* norm_negate_op */

/*****************************************************************************/
//! norm_eliminate_NOT 

/*! \breif  NOT 조건을 negate시킴
 *         
 ************************************
 * \param node(in/out) :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *      cond_normalize()에서 호출
 *  
 *****************************************************************************/
void
norm_eliminate_NOT(T_NODE * node)
{
    T_NODE *tmp;

    if (node->type != SOT_NOT)
    {
        if (node->left)
            norm_eliminate_NOT(node->left);

        if (node->right)
            norm_eliminate_NOT(node->right);

    }
    else
    {       /* node->type == SOT_NOT */
        tmp = node->left;
        sc_memcpy(node, node->left, sizeof(T_NODE));
        PMEM_FREENUL(tmp);

        norm_negate_op(node);
    }

}           /* norm_eliminate_NOT */

/************************************************
    } end of NOT elimination
 ************************************************/

/************************************************
    begin of CNF transformation {
 ************************************************/

/*****************************************************************************/
//! norm_transform_CNF 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param node()    :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
int
norm_transform_CNF(T_NODE * node)
{
    T_NODE *t1, *t2, *t3;

    if (node->type != SOT_AND && node->type != SOT_OR)
        return RET_SUCCESS;

    if (node->left)
    {
        if (norm_transform_CNF(node->left) == RET_ERROR)
            return RET_ERROR;

        if (node->type == SOT_OR && node->left->type == SOT_AND)
        {

            /* (A AND B) OR C => (A OR C) AND (B OR C) */

            t1 = node->right;
            t2 = node->left->right;
            t3 = copy_tree(t1);
            if (t3 == NULL)
                return RET_ERROR;

            node->type = SOT_AND;
            node->left->type = SOT_OR;
            node->left->right = t1;

            node->right = (T_NODE *) PMEM_ALLOC(sizeof(T_NODE));
            if (node->right == NULL)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return RET_ERROR;
            }
            sc_memcpy(node->right, node->left, sizeof(T_NODE));
            node->right->left = t2;
            node->right->right = t3;

            if (norm_transform_CNF(node) == RET_ERROR)
                return RET_ERROR;
        }
    }

    if (node->right)
    {
        if (norm_transform_CNF(node->right) == RET_ERROR)
            return RET_ERROR;

        if (node->type == SOT_OR)
        {
            if (node->right->type == SOT_AND)
            {

                /* A OR (B AND C) => (A OR B) AND (A OR C) */

                t1 = node->left;
                t2 = node->right->left;
                t3 = copy_tree(t1);
                if (t3 == NULL)
                    return RET_ERROR;

                node->type = SOT_AND;
                node->right->type = SOT_OR;
                node->right->left = t1;

                node->left = (T_NODE *) PMEM_ALLOC(sizeof(T_NODE));
                if (node->left == NULL)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY,
                            0);
                    return RET_ERROR;
                }
                sc_memcpy(node->left, node->right, sizeof(T_NODE));
                node->left->left = t3;
                node->left->right = t2;

                if (norm_transform_CNF(node) == RET_ERROR)
                    return RET_ERROR;

            }
            else if (node->right->type == SOT_OR)
            {

                /* A OR (B OR C) => (A OR B) OR C */

                t1 = node->left;
                t2 = node->right->left;
                t3 = node->right->right;

                node->right->left = t1;
                node->right->right = t2;
                node->left = node->right;
                node->right = t3;

                if (norm_transform_CNF(node) == RET_ERROR)
                    return RET_ERROR;
            }

        }
        else if (node->type == SOT_AND)
        {
            if (node->right->type == SOT_AND)
            {

                /* A AND (B AND C) => (A AND B) AND C */

                t1 = node->left;
                t2 = node->right->left;
                t3 = node->right->right;

                node->right->left = t1;
                node->right->right = t2;
                node->left = node->right;
                node->right = t3;

                if (norm_transform_CNF(node) == RET_ERROR)
                    return RET_ERROR;
            }
        }
    }

    return RET_SUCCESS;
}           /* norm_transform_CNF */

/************************************************
    } end of CNF tranformation
 ************************************************/

/*****************************************************************************/
//! cond_normalize 

/*! \breif SQL 문장을 실질적으로 NOMARLIZE 시키는 경우 사용됨

 ************************************
 * \param cond(in) : PARSING시 들어온 정보
 * \param list_ptr(in/out) :  NORMALIZE가 종료된 정보 
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - SELECT 문장인 경우 select_normalize()을 거쳐서 이리 들어옴 
 *  - UPDATE/DELETE 문장을 NOMARLIZE 시킴(바로 호출)
 *
 *****************************************************************************/
int
cond_normalize(T_EXPRESSIONDESC * cond, EXPRDESC_LIST ** list_ptr)
{
    T_NODE *tree = NULL;
    int size;

    *list_ptr = NULL;

    if (cond->len > 0 && cond->list != NULL)
    {
        tree = trans_expr2tree(cond, tree);
        if (tree == NULL)
            return er_errid();

        size = estimate_cnf_size(tree);
        if (size > MAX_CNF_SIZE || size < 0)
        {
            free_tree(tree);
            return RET_SUCCESS;
        }
        norm_eliminate_NOT(tree);
        if (norm_transform_CNF(tree) != RET_SUCCESS)
        {
            free_tree(tree);
            return er_errid();
        }
        *list_ptr = trans_tree2exprlist(tree, NULL, cond);
        if (*list_ptr == NULL)
        {
            free_tree(tree);
            return er_errid();
        }
        free_tree(tree);
    }
    return RET_SUCCESS;

}           /* cond_normalize */

T_NODE *
trans_setlist2tree(T_EXPRESSIONDESC * expr)
{
    T_POSTFIXELEM *elem;
    T_NODE *node = NULL, *arg1 = NULL, *arg2 = NULL;
    int start, i;
    t_node_stack *stack;

    init_node_stack(&stack);
    start = -1;
    for (i = 0; i < expr->len; i++)
    {
        elem = expr->list[i];
        if (start == -1)
            start = i;
        if (elem->elemtype == SPT_SUBQ_OP)
        {
            switch (elem->u.op.type)
            {
            case SOT_UNION_ALL:
            case SOT_UNION:
            case SOT_INTERSECT:
            case SOT_EXCEPT:
                pop_node_stack(&arg2, &stack);
                pop_node_stack(&arg1, &stack);
                node = (T_NODE *) PMEM_ALLOC(sizeof(T_NODE));
                if (node == NULL)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_OUTOFMEMORY, 0);
                    remove_node_stack(&stack);
                    return NULL;
                }
                node->type = elem->u.op.type;
                node->start = i;
                node->end = i;
                /* intersect 또는 except인 경우 expression의 위치를 변경
                   A OP B --> B OP A
                   except와 inetersect는 오른쪽 select 구문이 비교 대상이
                   되므로 먼저 처리되어야 함 */
                if (elem->u.op.type == SOT_INTERSECT
                        || elem->u.op.type == SOT_EXCEPT)
                {
                    node->left = arg2;
                    node->right = arg1;
                }
                else
                {
                    node->left = arg1;
                    node->right = arg2;
                }
                push_node_stack(node, &stack);
                start = -1;
                break;
            default:
                break;
            }
        }
        else
        {
            node = (T_NODE *) PMEM_ALLOC(sizeof(T_NODE));
            if (node == NULL)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                remove_node_stack(&stack);
                return NULL;
            }
            node->type = SOT_NONE;
            node->start = start;
            node->end = i;
            node->left = node->right = NULL;
            push_node_stack(node, &stack);
            start = -1;
        }
    }

    pop_node_stack(&node, &stack);
    remove_node_stack(&stack);

    return node;

}           /* trans_expr2tree */

void
setlist_tree_normailze(T_NODE * node, T_EXPRESSIONDESC * set_list)
{
    T_POSTFIXELEM *elem;

    if (node->left)
    {
        if (node->type != SOT_UNION_ALL)
        {
            if (node->left->type == SOT_UNION_ALL)
            {
                node->left->type = SOT_UNION;
                elem = set_list->list[node->left->start];
                if (elem->elemtype == SPT_SUBQ_OP)
                {
                    elem->u.op.type = SOT_UNION_ALL;
                }
            }
        }
        setlist_tree_normailze(node->left, set_list);
    }
    if (node->right)
    {
        if (node->type != SOT_UNION_ALL)
        {
            if (node->right->type == SOT_UNION_ALL)
            {
                node->right->type = SOT_UNION;
                elem = set_list->list[node->right->start];
                if (elem->elemtype == SPT_SUBQ_OP)
                {
                    elem->u.op.type = SOT_UNION;
                }
            }
        }
        setlist_tree_normailze(node->right, set_list);
    }
}

/*****************************************************************************/
//! set_operation_normalize 

/*! \breif SQL 문장을 실질적으로 NOMARLIZE 시키는 경우 사용됨

 ************************************
 * \param cond(in) : PARSING시 들어온 정보
 * \param list_ptr(in/out) :  NORMALIZE가 종료된 정보 
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - SELECT 문장인 경우 select_normalize()을 거쳐서 이리 들어옴 
 *  - UPDATE/DELETE 문장을 NOMARLIZE 시킴(바로 호출)
 *
 *****************************************************************************/
int
set_operation_normalize(T_EXPRESSIONDESC * set_list)
{
    T_NODE *tree = NULL;

    if (set_list->len > 0 && set_list->list != NULL)
    {
        tree = trans_setlist2tree(set_list);
        if (tree == NULL)
            return er_errid();

        setlist_tree_normailze(tree, set_list);

        free_tree(tree);
    }

    return RET_SUCCESS;

}           /* set_opeartion_normalize */

/*****************************************************************************/
//! select_normalize 

/*! \breif  SELECT 문장을 NOMARLIZE 시킴
 ************************************
 * \param handle(in) : DB Handle
 * \param select(in/out) : T_SELECT
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
select_normalize(int *handle, T_SELECT * select)
{
    T_QUERYDESC *qdesc;
    T_JOINTABLEDESC *jointable;
    int i, ret;

#ifdef MDB_DEBUG
    sc_assert(select != NULL, __FILE__, __LINE__);
#endif
    if (select->first_sub &&
            (ret = select_normalize(handle, select->first_sub)) != RET_SUCCESS)
        return ret;
    if (select->sibling_query &&
            (ret = select_normalize(handle, select->sibling_query))
            != RET_SUCCESS)
        return ret;

    qdesc = &select->planquerydesc.querydesc;

    if (qdesc->setlist.len > 0)
    {
        ret = set_operation_normalize(&qdesc->setlist);
        if (ret != RET_SUCCESS)
        {
            return ret;
        }

        for (i = 0; i < qdesc->setlist.len; i++)
        {
            if (qdesc->setlist.list[i]->elemtype == SPT_SUBQUERY)
            {
                ret = select_normalize(handle, qdesc->setlist.list[i]->u.subq);
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
        jointable = &qdesc->fromlist.list[i];
        ret = cond_normalize(&jointable->condition, &jointable->expr_list);
        if (ret != RET_SUCCESS)
            return ret;
    }
    ret = cond_normalize(&qdesc->condition, &qdesc->expr_list);
    if (ret != RET_SUCCESS)
        return ret;

    return RET_SUCCESS;

}           /* select_normalize */

/*****************************************************************************/
//! sql_normalize 

/*! \breif  SQL 문장을 normalize 할 때 사용하는 함수
 ************************************
 * \param handle(in) : DB Handle
 * \param stmt(in/out) : T_STATEMENT
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
sql_normalizing(int *handle, T_STATEMENT * stmt)
{
    int result = RET_SUCCESS;

    switch (stmt->sqltype)
    {
    case ST_SELECT:
        return (select_normalize(handle, &stmt->u._select));

    case ST_UPDATE:
        if (stmt->u._update.subselect.first_sub)
            result = select_normalize(handle, &stmt->u._update.subselect);

        if (result != RET_SUCCESS)
            return result;
        return (cond_normalize(&stmt->u._update.planquerydesc.querydesc.
                        condition,
                        &stmt->u._update.planquerydesc.querydesc.expr_list));

    case ST_DELETE:
        if (stmt->u._delete.subselect.first_sub)
            result = select_normalize(handle, &stmt->u._delete.subselect);

        if (result != RET_SUCCESS)
            return result;
        return (cond_normalize(&stmt->u._delete.planquerydesc.querydesc.
                        condition,
                        &stmt->u._delete.planquerydesc.querydesc.expr_list));
        break;
    default:
        break;
    }

    return RET_SUCCESS;

}           /* sql_normalize */
