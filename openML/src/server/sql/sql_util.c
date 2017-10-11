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
#include "sql_util.h"

#include "sql_decimal.h"
#include "sql_func_timeutil.h"

#include "mdb_er.h"
#include "ErrorCode.h"

extern char *__systables[];

extern int _g_connid;

extern int MDB_FLOAT2STR(char *buf, int buf_size, float f);
extern int MDB_DOUBLE2STR(char *buf, int buf_size, double f);
extern int calc_func_convert(T_VALUEDESC * base, T_VALUEDESC * src,
        T_VALUEDESC * res, MDB_INT32 is_init);

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
init_stack_template(t_stack_template ** stack)
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
remove_stack_template(t_stack_template ** stack)
{
    t_stack_template *node;

    while (*stack != NULL)
    {
        node = (*stack)->next;
        PMEM_FREENUL(*stack);
        *stack = node;
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
push_stack_template(void *vptr, t_stack_template ** stack)
{
    t_stack_template *node;

    node = PMEM_ALLOC(sizeof(t_stack_template));
    if (node == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return SQL_E_OUTOFMEMORY;
    }
    node->vptr = vptr;
    node->next = *stack;
    *stack = node;

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
void *
pop_stack_template(t_stack_template ** stack)
{
    t_stack_template *node;
    void *vptr;

    if (*stack == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_STACKUNDERFLOW, 0);
        return NULL;
    }
    node = *stack;
    *stack = node->next;
    vptr = node->vptr;
    PMEM_FREENUL(node);

    return vptr;
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
is_emptystack_template(t_stack_template ** stack)
{
    return (*stack == NULL);
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
append_nullstring(_T_STRING * sbuf, const char *s)
{
    unsigned char *p;
    int n;

    n = sc_strlen(s);
    if (sbuf->max < (sbuf->len + n))
    {
        p = sbuf->bytes;
        sbuf->max += STRING_BLOCK_SIZE;
        sbuf->bytes = PMEM_XCALLOC(sbuf->max + 1);
        if (sbuf->bytes == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return SQL_E_OUTOFMEMORY;
        }

        if (sbuf->len > 0)
            sc_memcpy(sbuf->bytes, p, sbuf->len);

        PMEM_FREENUL(p);
    }
    sc_memcpy((sbuf->bytes + sbuf->len), s, n);
    sbuf->len += n;
    sbuf->bytes[sbuf->len] = '\0';

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
traverse_tree_node(T_TREE_NODE * node, TRAVERSE_TREE_ARG * arg)
{
    if (node)
    {
        if (arg->pre_fun)
            (*(arg->pre_fun)) (arg->pre_arg, node->vptr);

        traverse_tree_node(node->left, arg);

        if (arg->in_fun)
            (*(arg->in_fun)) (arg->in_arg, node->vptr);

        traverse_tree_node(node->right, arg);

        if (arg->post_fun)
            (*(arg->post_fun)) (arg->post_arg, node->vptr);
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
destroy_tree_node(T_TREE_NODE * node)
{
    if (node)
    {
        destroy_tree_node(node->left);
        destroy_tree_node(node->right);
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
int
is_filterop(T_OPTYPE optype)
{
    if (optype == SOT_EQ)
        return 1;
    else if (optype == SOT_LE)
        return 1;
    else if (optype == SOT_LT)
        return 1;
    else if (optype == SOT_GE)
        return 1;
    else if (optype == SOT_GT)
        return 1;
    else if (optype == SOT_NE)
        return 1;
    else if (optype == SOT_ISNULL)
        return 1;
    else if (optype == SOT_ISNOTNULL)
        return 1;
    else if (optype == SOT_LIKE)
        return 1;
    else if (optype == SOT_ILIKE)
        return 1;
    else
        return 0;
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
is_indexop(T_OPTYPE optype)
{
    if (optype == SOT_EQ)
        return 1;
    else if (optype == SOT_LE)
        return 1;
    else if (optype == SOT_LT)
        return 1;
    else if (optype == SOT_GE)
        return 1;
    else if (optype == SOT_GT)
        return 1;
    else if (optype == SOT_LIKE)
        return 1;
    else if (optype == SOT_ISNULL)
        return 1;
    else if (optype == SOT_ISNOTNULL)
        return 1;
    else if (optype == SOT_IN)
        return 1;
    else
        return 0;
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
is_logicalop(T_OPTYPE optype)
{
    switch (optype)
    {
    case SOT_AND:
    case SOT_OR:
    case SOT_NOT:
        return 1;
    default:
        return 0;
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
T_OPTYPE
get_commute_op(T_OPTYPE optype)
{
    switch (optype)
    {
    case SOT_GT:
        return SOT_LT;
    case SOT_GE:
        return SOT_LE;
    case SOT_LT:
        return SOT_GT;
    case SOT_LE:
        return SOT_GE;
    default:
        return optype;
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
is_firstmemberofindex(int connid, T_ATTRDESC * attr, char *hint_indexname,
        T_INDEXINFO * index)
{
    SYSINDEX_T *indexinfo = NULL;
    int i = 0, ret = 0;
    int indexnum;

    /* already found it!!! */
    if (index->fields)
        goto RETURN_LABEL;

    if (hint_indexname)
    {
        ret = dbi_Schema_GetIndexInfo(connid, hint_indexname,
                &index->info, &index->fields, attr->table.tablename);
        if (ret < 0)
        {
            /* do nothing */
        }
        else
        {
            if (attr->Hattr == index->fields[0].fieldId)
                return 1;
        }
    }

    indexnum = dbi_Schema_GetTableIndexesInfo(connid,
            attr->table.tablename, SCHEMA_ALL_INDEX, &indexinfo);
    if (indexnum < 0)
    {
        ret = 0;
        goto RETURN_LABEL;
    }

    for (i = 0; i < indexnum; i++)
    {
        ret = dbi_Schema_GetIndexInfo(connid, indexinfo[i].indexName,
                NULL, &index->fields, attr->table.tablename);
        if (ret < 0)
        {
            ret = 0;
            goto RETURN_LABEL;
        }
        if (attr->Hattr == index->fields[0].fieldId)
        {
            ret = 1;
            goto RETURN_LABEL;
        }
        PMEM_FREENUL(index->fields);
    }

  RETURN_LABEL:
    if (ret == 1)       // index 구성 field중에서 첫번째 field인 경우.
        sc_memcpy(index, &(indexinfo[i]), sizeof(SYSINDEX_T));
    else
        sc_memset(index, 0, sizeof(T_INDEXINFO));
    PMEM_FREENUL(indexinfo);

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
int
is_firstgroupofindex(int connid, T_ORDERBYDESC * orderby, OPENTYPE opentype,
        char *tablename, char *indexname, T_INDEXINFO * index)
{
    SYSINDEX_T *indexinfo = NULL;
    int indexnum;
    int i, j;

    indexnum = dbi_Schema_GetTableIndexesInfo(connid, tablename,
            SCHEMA_ALL_INDEX, &indexinfo);
    if (indexnum < 0)
        return RET_END;

    if (indexnum && indexname)
    {
        SYSINDEX_T tmpIndex;

        if (dbi_Schema_GetIndexInfo(connid, indexname,
                        &tmpIndex, &index->fields, tablename) < 0)
        {
            PMEM_FREENUL(indexinfo);
            return RET_END;
        }
        for (j = 0; j < orderby->len && j < tmpIndex.numFields; j++)
        {
            if (orderby->list[j].attr.Hattr != index->fields[j].fieldId)
                break;

            if (orderby->list[j].attr.collation !=
                    (MDB_COL_TYPE) index->fields[j].collation)
            {
                break;
            }

            if (orderby->list[j].ordertype == index->fields[j].order)
            {
                if (opentype != ASCENDING)
                    break;
            }
            else
            {
                if (opentype != DESCENDING)
                    break;
            }
        }
        if (j >= orderby->len)
        {
            sc_memcpy(index, &tmpIndex, sizeof(SYSINDEX_T));
            PMEM_FREENUL(indexinfo);
            return RET_SUCCESS;
        }

        /* memory leak !!! */
        PMEM_FREENUL(index->fields);
    }

    for (i = 0; i < indexnum; i++)
    {
        if (dbi_Schema_GetIndexInfo(connid, indexinfo[i].indexName,
                        NULL, &index->fields, tablename) < 0)
        {
            PMEM_FREENUL(indexinfo);
            return RET_END;
        }

        for (j = 0; j < orderby->len && j < indexinfo[i].numFields; j++)
        {
            if (orderby->list[j].attr.Hattr != index->fields[j].fieldId)
                break;

            if (orderby->list[j].attr.collation !=
                    (MDB_COL_TYPE) index->fields[j].collation)
            {
                break;
            }
            if (orderby->list[j].ordertype == index->fields[j].order)
            {
                if (opentype != ASCENDING)
                    break;
            }
            else
            {
                if (opentype != DESCENDING)
                    break;
            }
        }

        if (j >= orderby->len)
        {
            sc_memcpy(index, &(indexinfo[i]), sizeof(SYSINDEX_T));
            PMEM_FREENUL(indexinfo);
            return RET_SUCCESS;
        }

        PMEM_FREENUL(index->fields);
    }

    PMEM_FREENUL(indexinfo);
    return RET_END;
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
__DECL_PREFIX int
check_systable(char *table)
{
    int i;

    for (i = 0; __systables[i]; ++i)
    {
        if (__systables[i][0] == '#')
        {
            continue;
        }

        if (!mdb_strcmp(__systables[i], table))
        {
            return i + 1;
        }
    }

    return -1;
}


/*****************************************************************************/
//! set_attrinfobyname 

/*! \breif  attr에 실제 Fields 정보를 설정
 ************************************
 * \param connid(in) :
 * \param attr(in/out) : 
 * \param field(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *  - A. 함수를 통해서 값을 할당하도록 수정함
 *****************************************************************************/
int
set_attrinfobyname(int connid, T_ATTRDESC * attr, SYSFIELD_T * field)
{
    SYSFIELD_T *fieldinfo = NULL;
    int i, fieldnum;
    int ret;

    if (field == NULL)
    {
        SYSFIELD_T *fieldinfo_i;

        fieldnum =
                dbi_Schema_GetFieldsInfo(connid, attr->table.tablename,
                &fieldinfo);
        if (fieldnum < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, fieldnum, 0);

            return RET_ERROR;
        }

        for (i = 0; i < fieldnum; i++)
        {
            fieldinfo_i = fieldinfo + i;
            if (!mdb_strcmp(attr->attrname, fieldinfo_i->fieldName))
            {
                attr->posi.ordinal = fieldinfo_i->position;
                attr->posi.offset = fieldinfo_i->offset;
                attr->flag = fieldinfo_i->flag;
                attr->type = fieldinfo_i->type;
                attr->len = fieldinfo_i->length;
                attr->fixedlen = fieldinfo_i->fixed_part;
                if (attr->type == DT_NVARCHAR)
                    attr->fixedlen /= WCHAR_SIZE;
                attr->dec = fieldinfo_i->scale;
                attr->Htable = fieldinfo_i->tableId;
                attr->Hattr = fieldinfo_i->fieldId;
                if (!IS_BS_OR_NBS(fieldinfo_i->type)
                        || attr->collation == MDB_COL_NONE)
                {
                    attr->collation = fieldinfo_i->collation;
                }
                ret = get_defaultvalue(attr, fieldinfo_i);
                if (ret < 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    return RET_ERROR;
                }

                break;
            }
        }
        PMEM_FREENUL(fieldinfo);

        /* no error 상황을 없애기 위해 error 설정 */
        if (i == fieldnum)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDCOLUMN, 0);
            return RET_ERROR;
        }
    }
    else
    {
        attr->posi.ordinal = field->position;
        attr->posi.offset = field->offset;
        attr->flag = field->flag;
        attr->type = field->type;
        attr->len = field->length;
        attr->fixedlen = field->fixed_part;
        if (attr->type == DT_NVARCHAR)
            attr->fixedlen /= WCHAR_SIZE;
        attr->dec = field->scale;
        attr->Htable = field->tableId;
        attr->Hattr = field->fieldId;
        if (!IS_BS_OR_NBS(field->type) || attr->collation == MDB_COL_NONE)
        {
            attr->collation = field->collation;
        }
        ret = get_defaultvalue(attr, field);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return RET_ERROR;
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
check_validtable(int connid, char *table)
{
    SYSTABLE_T tableinfo;
    int ret;

    ret = dbi_Schema_GetTableInfo(connid, table, &tableinfo);

    if (ret < 0)
        return -1;
    else
        return 1;
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
check_valid_basetable(int connid, char *table)
{
    SYSTABLE_T tableinfo;
    int ret;

    ret = dbi_Schema_GetTableInfo(connid, table, &tableinfo);

    if (ret < 0)
        return -1;
    else if ((tableinfo.flag & _CONT_VIEW) != 0)
        return 0;
    else
        return 1;
}

/*****************************************************************************/
//! set_minvalue 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param type :
 * \param len : 
 * \param res :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void
set_minvalue(DataType type, int len, T_VALUEDESC * res)
{
    res->valueclass = SVC_CONSTANT;
    res->valuetype = type;
    switch (type)
    {
    case DT_VARCHAR:   // string
    case DT_CHAR:      // bytes
        sc_memset(res->u.ptr, 0x00, len);
        break;
    case DT_NVARCHAR:  // string
    case DT_NCHAR:     // bytes
        sc_wmemset(res->u.Wptr, 0x00, len);
        break;
    case DT_SMALLINT:  // short
        res->u.i16 = MDB_SHRT_MIN;
        break;
    case DT_INTEGER:   // integer
        res->u.i32 = MDB_INT_MIN;
        break;
    case DT_FLOAT:     // float
        res->u.f16 = -MDB_FLT_MAX;
        break;
    case DT_DOUBLE:    // double
        res->u.f32 = -MDB_DBL_MAX;
        break;
    case DT_TINYINT:   // tinyint
        res->u.i8 = MDB_CHAR_MIN;
        break;
    case DT_BIGINT:    // bigint
        res->u.i64 = MDB_LLONG_MIN;
        break;
    case DT_DATETIME:  // datetime
        sc_memset(res->u.datetime, 0x00, MAX_DATETIME_FIELD_LEN);
        break;
    case DT_DATE:      // date
        sc_memset(res->u.date, 0x00, MAX_DATE_FIELD_LEN);
        break;
    case DT_TIME:      // date
        sc_memset(res->u.time, 0x00, MAX_TIME_FIELD_LEN);
        break;
    case DT_TIMESTAMP: // timestamp
        res->u.timestamp = MDB_LLONG_MIN;
        break;
    case DT_DECIMAL:   // decimal
        res->u.dec = -MDB_DBL_MAX;
        break;
    default:   // vchar, blob, time, date
        break;
    }
}

/*****************************************************************************/
//! set_maxvalue 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param type :
 * \param len : 
 * \param res :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void
set_maxvalue(DataType type, int len, T_VALUEDESC * res)
{
    res->valueclass = SVC_CONSTANT;
    res->valuetype = type;

    switch (type)
    {
    case DT_VARCHAR:   // string, corresponding to varchar(n)/string(n)
        sc_memset(res->u.ptr, MDB_UCHAR_MAX, len);
        res->u.ptr[len] = '\0';
        break;
    case DT_NVARCHAR:
        if (sizeof(DB_WCHAR) == INT_SIZE)
            sc_wmemset(res->u.Wptr, (DB_WCHAR) DB_WCHAR_INTMAX, len);
        else
            sc_wmemset(res->u.Wptr, DB_WCHAR_SHRTMAX, len);
        res->u.Wptr[len] = L'\0';
        break;
    case DT_CHAR:      // bytes, corresponding to char(n)/bytes(n)
        sc_memset(res->u.ptr, MDB_UCHAR_MAX, len);
        break;
    case DT_NCHAR:
        if (sizeof(DB_WCHAR) == INT_SIZE)
            sc_wmemset(res->u.Wptr, (DB_WCHAR) DB_WCHAR_INTMAX, len);
        else
            sc_wmemset(res->u.Wptr, DB_WCHAR_SHRTMAX, len);
        break;
    case DT_SMALLINT:  // short
        res->u.i16 = MDB_SHRT_MAX;
        break;
    case DT_INTEGER:   // integer
        res->u.i32 = MDB_INT_MAX;
        break;
    case DT_FLOAT:     // float
        res->u.f16 = MDB_FLT_MAX;
        break;
    case DT_DOUBLE:    // double
        res->u.f32 = MDB_DBL_MAX;
        break;
    case DT_TINYINT:   // tinyint
        res->u.ch = MDB_CHAR_MAX;
        break;
    case DT_BIGINT:    // bigint
        res->u.i64 = MDB_LLONG_MAX;
        break;
    case DT_DATETIME:  // datetime
        sc_memset(res->u.datetime, MDB_UCHAR_MAX, MAX_DATETIME_FIELD_LEN);
        break;
    case DT_DATE:      // date
        sc_memset(res->u.date, MDB_UCHAR_MAX, MAX_DATE_FIELD_LEN);
        break;
    case DT_TIME:      // time
        sc_memset(res->u.time, MDB_UCHAR_MAX, MAX_TIME_FIELD_LEN);
        break;
    case DT_TIMESTAMP: // timestamp
        res->u.timestamp = MDB_LLONG_MAX;
        break;
    case DT_DECIMAL:   // decimal
        res->u.dec = MDB_DBL_MAX;
        break;
    case DT_OID:       // oid
        res->u.oid = -1;
        break;
    default:   // vchar, blob, time, date
        break;
    }
}

/*****************************************************************************/
//! get_recordlen

/*! \breif  get table's memory-record length
 ************************************
 * \param connid(in)            : connection id
 * \param tablename(in)         : table's name
 * \param pTableInfo(in/out)    : table's info
 ************************************
 * \return  table's memory-record length
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
get_recordlen(int connid, char *tablename, SYSTABLE_T * pTableInfo)
{
    int length;

    if (pTableInfo != NULL)
    {
        length = pTableInfo->recordLen
                + GET_HIDDENFIELDLENGTH(-1, pTableInfo->numFields,
                pTableInfo->recordLen);
    }
    else
    {
        SYSTABLE_T tableinfo;
        int ret;

        ret = dbi_Schema_GetTableInfo(connid, tablename, &tableinfo);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return ret;
        }

        length = tableinfo.recordLen
                + GET_HIDDENFIELDLENGTH(-1, tableinfo.numFields,
                tableinfo.recordLen);
    }

    if (length < -1)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, E_INVALID_RECLEN, 1, length);
    }

    return length;
}

int
h_get_recordlen(int connid, char *tablename, SYSTABLE_T * pTableInfo)
{
    int length;

    if (pTableInfo != NULL)
    {
        length = pTableInfo->h_recordLen
                + GET_HIDDENFIELDLENGTH(-1, pTableInfo->numFields,
                pTableInfo->h_recordLen);
    }
    else
    {
        SYSTABLE_T tableinfo;
        int ret;

        ret = dbi_Schema_GetTableInfo(connid, tablename, &tableinfo);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return ret;
        }

        length = tableinfo.h_recordLen
                + GET_HIDDENFIELDLENGTH(-1, tableinfo.numFields,
                tableinfo.h_recordLen);
    }

    if (length < -1)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, E_INVALID_RECLEN, 1, length);
    }

    return length;
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
get_nullflag_offset(int connid, char *tablename, SYSTABLE_T * pTableInfo)
{
    if (pTableInfo != NULL)
    {
        return pTableInfo->recordLen;
    }
    else
    {
        SYSTABLE_T tableinfo;
        int ret;

        ret = dbi_Schema_GetTableInfo(connid, tablename, &tableinfo);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return ret;
        }

        return tableinfo.recordLen;
    }
}

int
h_get_nullflag_offset(int connid, char *tablename, SYSTABLE_T * pTableInfo)
{
    if (pTableInfo != NULL)
    {
        return pTableInfo->h_recordLen;
    }
    else
    {
        SYSTABLE_T tableinfo;
        int ret;

        ret = dbi_Schema_GetTableInfo(connid, tablename, &tableinfo);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return ret;
        }

        return tableinfo.h_recordLen;
    }
}

/*****************************************************************************/
//! get_defaultvalue 

/*! \breif  field의 default값을 attr의 디폴트값으로 할당한다.
 ************************************
 * \param attr(in/out) :
 * \param field(in) : 
 ************************************
 * \return  void
 ************************************
 * \note 
 *  - SYSDATE/CURRENT_TIMESTAMP 를 지원한다
 *  - BYTE/VARBYTE의 경우.. 값을 측정한다
 *****************************************************************************/
int
get_defaultvalue(T_ATTRDESC * attr, SYSFIELD_T * field)
{
    attr->defvalue.defined = 1;
    if (!DB_DEFAULT_ISNULL(field->defaultValue))
    {
        attr->defvalue.not_null = 1;
        if (IS_DATE_OR_TIME_TYPE(field->type) &&
                (field->flag & DEF_SYSDATE_BIT))
        {
            struct db_tm ltm;
            db_time_t l_clock;
            int msec;
            char timestamp[MAX_TIMESTAMP_STRING_LEN];
            int pos;

            struct db_timeval tv;

            sc_gettimeofday(&tv, NULL);
            l_clock = tv.tv_sec;
            msec = (int) (tv.tv_usec / 1000);

            sc_localtime_r(&l_clock, &ltm);

            if (!mdb_strcmp(field->defaultValue, "current_date"))
            {
                ltm.tm_hour = 0;
                ltm.tm_min = 0;
                ltm.tm_sec = 0;
                msec = 0;
            }

            pos = sc_sprintf(timestamp, "%.4d", ltm.tm_year + 1900);
            pos += sc_sprintf(timestamp + pos, "-%02d", ltm.tm_mon + 1);
            pos += sc_sprintf(timestamp + pos, "-%02d", ltm.tm_mday);
            pos += sc_sprintf(timestamp + pos, " %02d", ltm.tm_hour);
            pos += sc_sprintf(timestamp + pos, ":%02d", ltm.tm_min);
            pos += sc_sprintf(timestamp + pos, ":%02d", ltm.tm_sec);

            switch (field->type)
            {
            case DT_DATETIME:
                char2datetime(attr->defvalue.u.datetime, timestamp,
                        sc_strlen(timestamp));
                break;
            case DT_DATE:
                char2date(attr->defvalue.u.date, timestamp,
                        sc_strlen(timestamp));
                break;
            case DT_TIME:
                char2time(attr->defvalue.u.time, timestamp,
                        sc_strlen(timestamp));
                break;
            case DT_TIMESTAMP:
                pos += sc_sprintf(timestamp + pos, ".%03d", msec);
                char2timestamp(&attr->defvalue.u.timestamp, timestamp,
                        sc_strlen(timestamp));
                break;
            }
        }
        else
        {
            switch (field->type)
            {
            case DT_TINYINT:
                {
                    int tmp;

                    tmp = sc_atoi(field->defaultValue);
                    attr->defvalue.u.i8 = (tinyint) tmp;
                    break;
                }
            case DT_SMALLINT:
                attr->defvalue.u.i16 = (smallint) sc_atoi(field->defaultValue);
                break;
            case DT_INTEGER:
                attr->defvalue.u.i32 = sc_atoi(field->defaultValue);
                break;
            case DT_BIGINT:
                attr->defvalue.u.i64 = sc_atoll(field->defaultValue);
                break;
            case DT_FLOAT:
                attr->defvalue.u.f16 = (float) sc_atof(field->defaultValue);
                break;
            case DT_DOUBLE:
                attr->defvalue.u.f32 = sc_atof(field->defaultValue);
                break;
            case DT_DECIMAL:
                char2decimal(&attr->defvalue.u.dec,
                        field->defaultValue, sc_strlen(field->defaultValue));
                break;
            case DT_CHAR:
                attr->defvalue.u.ptr = PMEM_STRDUP(field->defaultValue);
                if (!attr->defvalue.u.ptr)
                {
                    return SQL_E_OUTOFMEMORY;
                }
                attr->defvalue.value_len =
                        strlen_func[DT_CHAR] (attr->defvalue.u.ptr);
                break;
            case DT_NCHAR:
                attr->defvalue.u.Wptr =
                        PMEM_WSTRDUP((DB_WCHAR *) field->defaultValue);
                if (!attr->defvalue.u.Wptr)
                {
                    return SQL_E_OUTOFMEMORY;
                }
                attr->defvalue.value_len =
                        strlen_func[DT_NCHAR] (attr->defvalue.u.Wptr);
                break;
            case DT_VARCHAR:
                attr->defvalue.u.ptr = PMEM_STRDUP(field->defaultValue);
                if (!attr->defvalue.u.ptr)
                {
                    return SQL_E_OUTOFMEMORY;
                }
                attr->defvalue.value_len =
                        strlen_func[DT_VARCHAR] (field->defaultValue);
                break;
            case DT_NVARCHAR:
                attr->defvalue.u.Wptr =
                        PMEM_WSTRDUP((DB_WCHAR *) field->defaultValue);
                if (!attr->defvalue.u.Wptr)
                {
                    return SQL_E_OUTOFMEMORY;
                }
                attr->defvalue.value_len =
                        strlen_func[DT_NVARCHAR] (attr->defvalue.u.Wptr);
                break;
            case DT_BYTE:
            case DT_VARBYTE:
                DB_GET_BYTE_LENGTH(field->defaultValue,
                        attr->defvalue.value_len);
                if (attr->defvalue.value_len)
                {
                    attr->defvalue.u.ptr =
                            PMEM_ALLOC(attr->defvalue.value_len);
                    if (!attr->defvalue.u.ptr)
                    {
                        return SQL_E_OUTOFMEMORY;
                    }
                    sc_memcpy(attr->defvalue.u.ptr,
                            DB_GET_BYTE_VALUE(field->defaultValue),
                            attr->defvalue.value_len);
                }
                else
                    attr->defvalue.not_null = 0;
                break;
            case DT_DATETIME:
                {
                    char datetime[MAX_DATETIME_STRING_LEN];

                    char2datetime(datetime,
                            field->defaultValue,
                            sc_strlen(field->defaultValue));
                    sc_memcpy(attr->defvalue.u.datetime, datetime,
                            MAX_DATETIME_FIELD_LEN);
                }
                break;
            case DT_DATE:
                {
                    char date[MAX_DATE_STRING_LEN];

                    char2date(date,
                            field->defaultValue,
                            sc_strlen(field->defaultValue));
                    sc_memcpy(attr->defvalue.u.date, date, MAX_DATE_FIELD_LEN);
                }
                break;
            case DT_TIME:
                {
                    char time[MAX_TIME_STRING_LEN];

                    char2time(time,
                            field->defaultValue,
                            sc_strlen(field->defaultValue));
                    sc_memcpy(attr->defvalue.u.time, time, MAX_TIME_FIELD_LEN);
                }
                break;
            case DT_TIMESTAMP:
                {
                    t_astime timestamp;

                    char2timestamp(&timestamp,
                            field->defaultValue,
                            sc_strlen(field->defaultValue));
                    sc_memcpy(&attr->defvalue.u.timestamp, &timestamp,
                            MAX_TIMESTAMP_FIELD_LEN);
                }
                break;
            default:
                attr->defvalue.not_null = 0;
                break;
            }   /* switch */
        }   /* else */
    }
    else
        attr->defvalue.not_null = 0;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! print_value_as_string

/*! \breif  resultset을 string으로 변환
 ************************************
 * \param value(in)     :
 * \param buf(in/out)   : 
 * \param bufsize(in)   :
 ************************************
 * \return  string size including the terminating null byte.
 ************************************
 * \note
 *  - called : append_element_string(), calc_like()
 *  confusing bufsize's meaning.
 *  anyway at this time, its meaning is the number of characters.
 *  and you must consider DT_VARCHAR, DT_NVARCHAR.
 *      
 *****************************************************************************/
int
print_value_as_string(T_VALUEDESC * value, char *buf, int bufsize)
{
    int len;
    char t_buf[32] = "\0";

    /* max of (MAX_DATETIME_STRING_LEN, MAX_DATE_STRING_LEN,
       MAX_TIME_STRING_LEN, MAX_TIMESTAMP_STRING_LEN) */

    switch (value->valuetype)
    {
    case DT_TINYINT:
        mdb_itoa((int) value->u.i8, t_buf, 10);
        return mdb_strlcpy(buf, t_buf, bufsize) + 1;
    case DT_SMALLINT:
        mdb_itoa((int) value->u.i16, t_buf, 10);
        return mdb_strlcpy(buf, t_buf, bufsize) + 1;
    case DT_INTEGER:
        mdb_itoa((int) value->u.i32, t_buf, 10);
        return mdb_strlcpy(buf, t_buf, bufsize) + 1;
    case DT_BIGINT:
        MDB_bitoa(value->u.i64, t_buf, 10);
        return mdb_strlcpy(buf, t_buf, bufsize) + 1;
    case DT_FLOAT:
        MDB_FLOAT2STR(t_buf, bufsize, value->u.f16);
        return mdb_strlcpy(buf, t_buf, bufsize) + 1;
    case DT_DOUBLE:
        MDB_DOUBLE2STR(t_buf, bufsize, value->u.f32);
        return mdb_strlcpy(buf, t_buf, bufsize) + 1;
    case DT_DECIMAL:
        /* not to print -0 */
        if (bufsize > 1 && !value->attrdesc.dec && value->u.dec == 0.0)
        {
            buf[0] = '0';
            buf[1] = '\0';
            return 2;
        }

        MDB_DOUBLE2STR(t_buf, bufsize, value->u.dec);
        return mdb_strlcpy(buf, t_buf, bufsize) + 1;
    case DT_CHAR:
    case DT_VARCHAR:
        if (value->u.ptr)
        {
            return mdb_strlcpy(buf, value->u.ptr, bufsize) + 1;
        }
        else
        {
            return mdb_strlcpy(buf, "NULL", bufsize) + 1;
        }
    case DT_NCHAR:
    case DT_NVARCHAR:
        {
            char *ptr = buf;

            ptr = (char *) _alignedlen((unsigned long) ptr, sizeof(DB_WCHAR));

            if (value->u.Wptr)
                sc_wcsncpy((DB_WCHAR *) ptr, value->u.Wptr, bufsize);
            else
                sc_strncpy(ptr, "NULL", bufsize);

            ptr += (sc_wcslen((DB_WCHAR *) ptr) + 1) * sizeof(DB_WCHAR);

            return (ptr - buf);
        }
    case DT_BYTE:
    case DT_VARBYTE:
        if (value->u.ptr)
        {
            if (bufsize < value->value_len)
                len = bufsize;
            else
                len = value->value_len;

            sc_memcpy(buf, value->u.ptr, len);
            return len + 1;
        }
        else
        {
            sc_strncpy(buf, "NULL", bufsize);
            return sc_strlen(buf) + 1;
        }

    case DT_DATETIME:
        datetime2char(t_buf, value->u.datetime);
        return mdb_strlcpy(buf, t_buf, bufsize) + 1;
    case DT_DATE:
        date2char(t_buf, value->u.date);
        return mdb_strlcpy(buf, t_buf, bufsize) + 1;
    case DT_TIME:
        time2char(t_buf, value->u.date);
        return mdb_strlcpy(buf, t_buf, bufsize) + 1;
    case DT_TIMESTAMP:
        timestamp2char(t_buf, &value->u.timestamp);
        return mdb_strlcpy(buf, t_buf, bufsize) + 1;
    case DT_OID:
        MDB_bitoa(value->u.oid, t_buf, 10);
        return mdb_strlcpy(buf, t_buf, bufsize) + 1;
    default:
        return -1;
    }

    return -1;
}           /* print_value_as_string */

/*****************************************************************************/
//! make_value_from_string

/*! \breif  resultset을 string으로 변환
 ************************************
 * \param value(in)     :
 * \param buf(in/out)   : 
 * \param bufsize(in)   :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - called : calc_func_substring()
 *      
 *****************************************************************************/
int
make_value_from_string(T_VALUEDESC * value, char *row, DataType type)
{
    switch (type)
    {
    case DT_TINYINT:
    case DT_INTEGER:
    case DT_BIGINT:
        value->valuetype = DT_BIGINT;
        value->u.i64 = sc_atoll(row);
        break;

    case DT_FLOAT:
    case DT_DOUBLE:
    case DT_DECIMAL:
        value->valuetype = DT_DOUBLE;
        value->u.f32 = sc_atof(row);
        break;
    case DT_CHAR:
    case DT_NCHAR:
    case DT_VARCHAR:
    case DT_NVARCHAR:
        value->valuetype = type;
        if (sql_value_ptrStrdup(value, row, 1) < 0)
            return DB_FAIL;
        break;
    case DT_DATETIME:
        value->valuetype = DT_DATETIME;
        /*datetime2char(datetime, value->u.datetime); */
        break;
    case DT_DATE:
        value->valuetype = DT_DATE;
        /*date2char(date, value->u.date); */
        break;
    case DT_TIME:
        value->valuetype = DT_TIME;
        /*date2char(date, value->u.date); */
        break;
    case DT_TIMESTAMP:
        value->valuetype = DT_TIMESTAMP;
        /*timestamp2char(datetime, value->u.datetime); */
        break;
    default:
        return DB_FAIL;
        break;
    }

    return DB_SUCCESS;
}

int
copy_variable_into_record(int cursorId, char *target, char *val,
        SYSFIELD_T * field, int var_bytelen)
{
    int ret = DB_SUCCESS;
    OID var_rec_id = NULL_OID;
    DB_FIX_PART_LEN blength_value;
    DB_EXT_PART_LEN exted_bytelen;

    var_bytelen = var_bytelen * SizeOfType[field->type];

    sc_memset(target, 0, field->fixed_part);

    if (var_bytelen <= field->fixed_part)
    {       /* CASE A. can store entire value in heap. */
        blength_value = (DB_FIX_PART_LEN) var_bytelen;

#if defined(MDB_DEBUG)
        if (var_bytelen >= MAX_FIXED_SIZE_FOR_VARIABLE)
            sc_assert(0, __FILE__, __LINE__);
#endif
        // real value in the fixed-part
        sc_memcpy(target, val, var_bytelen);

        // length info in length flag
        sc_memcpy(target + field->fixed_part, &blength_value,
                MDB_FIX_LEN_SIZE);
    }
    else
    {       /* CASE B. store partial value in heap. */
        blength_value = (DB_FIX_PART_LEN) MDB_UCHAR_MAX;

        // real value in the fixed-part
        sc_memcpy(target + OID_SIZE, val, (field->fixed_part - OID_SIZE));

        // length info in length flag
        sc_memcpy(target + field->fixed_part, &blength_value,
                MDB_FIX_LEN_SIZE);

        val += (field->fixed_part - OID_SIZE);

        exted_bytelen = var_bytelen - (field->fixed_part - OID_SIZE);
        /* the last extended var record must have a null character */
        if (IS_N_STRING(field->type))
            exted_bytelen += SizeOfType[field->type];   /* null character */

        ret = dbi_Cursor_Insert_Variable(_g_connid, cursorId, val,
                exted_bytelen, &var_rec_id);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return ret;
        }

        // set OID in fixed-part
        sc_memcpy(target, (char *) &var_rec_id, OID_SIZE);
    }


    return ret;

}

/*****************************************************************************/
//! copy_value_into_record

/*! \breif convert the value to the memory record
 ************************************
 * \param target(in/out): memory record
 * \param val(in)       : value
 * \param field(in)     : schema infomation
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - reference the memory record format : _Schema_CheckFieldDesc()
 *  - called : set_insertrecord()
 *  - BYTE/VARBYTE 지원(value_len 만큼 복사한다)
 *      
 *****************************************************************************/
int
copy_value_into_record(int cursorId, char *target, T_VALUEDESC * val,
        SYSFIELD_T * field, int heap_check)
{
    T_VALUEDESC base, tmp;
    T_VALUEDESC *val_to_cpy;
    int ret = RET_SUCCESS;

    /* valgrind check. uninitialized */
    char str_decimal[MAX_DECIMAL_STRING_LEN] = "\0";

    tmp.call_free = 0;

    if (val->valuetype == (DataType) field->type ||
            (IS_BS(val->valuetype) && IS_BS(field->type)) ||
            (IS_NBS(val->valuetype) && IS_NBS(field->type)) ||
            (IS_BYTE(val->valuetype) && IS_BYTE(field->type)) ||
            (val->valuetype == DT_HEX))
    {
        val_to_cpy = val;
    }
    else
    {

        base.valuetype = field->type;
        base.attrdesc.len = field->length;
        base.valueclass = SVC_NONE;

        if (field->type < DT_CHAR &&
                check_numeric_bound(val, field->type, field->length)
                == RET_FALSE)

        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_TOOLARGEVALUE, 0);
            sql_value_ptrFree(&tmp);
            return RET_ERROR;
        }

        ret = calc_func_convert(&base, val, &tmp, MDB_FALSE);
        if (ret < 0)
        {
            sql_value_ptrFree(&tmp);
            return RET_ERROR;
        }

        val_to_cpy = &tmp;
    }

    switch (field->type)
    {
    case DT_TINYINT:
        *(tinyint *) target = val_to_cpy->u.i8;
        break;
    case DT_SMALLINT:
        *(smallint *) target = val_to_cpy->u.i16;
        break;
    case DT_INTEGER:
        *(int *) target = val_to_cpy->u.i32;
        break;
    case DT_BIGINT:
        *(bigint *) target = val_to_cpy->u.i64;
        break;
    case DT_FLOAT:
        *(float *) target = val_to_cpy->u.f16;
        break;
    case DT_DOUBLE:
        *(double *) target = val_to_cpy->u.f32;
        break;
    case DT_OID:
        sc_memcpy(target, (char *) &val_to_cpy->u.oid, sizeof(OID));
        break;
    case DT_DECIMAL:
        sc_memset(str_decimal, 0, sizeof(str_decimal));
        decimal2char(str_decimal, val_to_cpy->u.dec, field->length,
                field->scale);
        sc_memcpy(target, str_decimal, field->length + 3);
        break;
    case DT_CHAR:
        if (val->valuetype == DT_HEX)
        {
            /* only for special version */
            if (field->length < val->attrdesc.len)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_TOOLARGEVALUE, 0);
                return RET_ERROR;
            }
            sc_memcpy(target, val->u.ptr, val->attrdesc.len);
            break;
        }
        /* fallthrough */
    case DT_NCHAR:
    case DT_VARCHAR:
    case DT_NVARCHAR:
        if (heap_check && IS_VARIABLE_DATA(field->type, field->fixed_part))
        {
            sc_memset(target, 0, field->fixed_part);
        }
        else
            sc_memset(target, 0, field->length * SizeOfType[field->type]);

        val_to_cpy->value_len = strlen_func[field->type] (val_to_cpy->u.ptr);
        if (val_to_cpy->value_len > field->length)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_TOOLARGEVALUE, 0);
            ret = RET_ERROR;
        }
        else
        {
            if (heap_check && IS_VARIABLE_DATA(field->type, field->fixed_part))
                ret = copy_variable_into_record(cursorId, target,
                        val_to_cpy->u.ptr, field, val_to_cpy->value_len);
            else
                memcpy_func[field->type] (target, val_to_cpy->u.ptr,
                        val_to_cpy->value_len);
        }
        break;
    case DT_BYTE:
    case DT_VARBYTE:
        if (val_to_cpy->value_len > field->length)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_TOOLARGEVALUE, 0);
            ret = RET_ERROR;
        }
        else if (heap_check &&
                IS_VARIABLE_DATA(field->type, field->fixed_part))
            ret = copy_variable_into_record(cursorId, target,
                    val_to_cpy->u.ptr, field, val_to_cpy->value_len);
        else
            DB_SET_BYTE(target, val_to_cpy->u.ptr, val->value_len);

        break;

    case DT_DATETIME:
        sc_memcpy(target, val_to_cpy->u.datetime, MAX_DATETIME_FIELD_LEN);
        break;
    case DT_DATE:
        sc_memcpy(target, val_to_cpy->u.date, MAX_DATE_FIELD_LEN);
        break;
    case DT_TIME:
        sc_memcpy(target, val_to_cpy->u.time, MAX_TIME_FIELD_LEN);
        break;
    case DT_TIMESTAMP:
        sc_memcpy(target, &val_to_cpy->u.timestamp, MAX_TIMESTAMP_FIELD_LEN);
        break;

    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
        ret = RET_ERROR;
    }

    sql_value_ptrFree(&tmp);

    return ret;
}

T_RESULT_LIST *
RESULT_LIST_Create(int max_rec_size)
{
    T_RESULT_LIST *list;
    int min_page_size;

    min_page_size = max_rec_size + RESULT_PGHDR_SIZE;
    min_page_size = (min_page_size > RESULT_PAGE_SIZE) ?
            min_page_size : RESULT_PAGE_SIZE;

    list = (T_RESULT_LIST *) PMEM_ALLOC(sizeof(T_RESULT_LIST));
    if (list == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return NULL;
    }

    list->page_sz = min_page_size;
    list->n_pages = 0;
    list->cursor = NULL;
    list->cur_pg = NULL;
    list->head = NULL;
    list->tail = NULL;
    return list;
}

int
RESULT_LIST_Add(T_RESULT_LIST * list, char *row, int size)
{
    T_RESULT_PAGE *pg;

    pg = list->tail;
    if ((pg == NULL) ||
            (size > (int) (list->page_sz - RESULT_PGHDR_SIZE - pg->body_size)))
    {
        /* allocate result page and initialize it */
        pg = (T_RESULT_PAGE *) SMEM_ALLOC(list->page_sz);
        if (pg == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return -1;
        }
        pg->body_size = 0;
        pg->rows_in_page = 0;
        pg->next = NULL;

        /* connect the result page into the last of result page list */
        if (list->tail == NULL)
        {
            list->head = pg;
            list->tail = pg;
        }
        else
        {
            list->tail->next = pg;
            list->tail = pg;
        }
        list->n_pages += 1;
    }

    /* insert the given row into the result page */
    sc_memcpy(&pg->body[pg->body_size], row, size);
    pg->body_size += size;
    pg->rows_in_page += 1;

    return 0;
}

#define IN_PAGE(ptr_, page_) ((ptr_ - &((page_)->body[0])) < (page_)->body_size)

__DECL_PREFIX void
RESULT_LIST_Destroy(T_RESULT_LIST * list)
{
    T_RESULT_PAGE *pg;

    if (list)
    {
        while (list->head)
        {
            pg = list->head;
            list->head = pg->next;
            SMEM_FREE(pg);
        }
        PMEM_FREE(list);
    }
}

MDB_INLINE void *
sql_malloc(unsigned int size, DataType type)
{
    void *ptr;

    if (IS_NBS(type))
        ptr = PMEM_WALLOC(size);
    else
        ptr = PMEM_ALLOC(size);

    if (ptr == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return NULL;
    }

    return ptr;
}

MDB_INLINE void *
sql_xcalloc(unsigned int size, DataType type)
{
    void *ptr;

    if (IS_NBS(type))
        ptr = PMEM_XCALLOC(size * WCHAR_SIZE);
    else
        ptr = PMEM_XCALLOC(size);

    if (ptr == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return NULL;
    }

    return ptr;
}

MDB_INLINE char *
sql_strdup(char *str, DataType type)
{
    if (IS_NBS(type))
        return (char *) PMEM_WSTRDUP((DB_WCHAR *) str);
    else
        return PMEM_STRDUP(str);
}

MDB_INLINE void
sql_mfree(void *ptr)
{
    PMEM_FREENUL(ptr);
}

MDB_INLINE int
sql_value_ptrAlloc(T_VALUEDESC * value, int size)
{
#ifdef MDB_DEBUG
    if (value->call_free)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, -1, 1, "Invalid call_free\n");
        sc_assert(0, __FILE__, __LINE__);
    }

    if (value->valuetype > DT_HEX)
        sc_assert(0, __FILE__, __LINE__);
#else
    if (value->call_free)
        return RET_SUCCESS;
#endif


    if ((value->u.ptr = sql_malloc(size, value->valuetype)) == NULL)
        return RET_ERROR;

    value->call_free = 1;

    return RET_SUCCESS;
}

int
sql_value_ptrStrdup(T_VALUEDESC * value, char *src, int default_size)
{
    int size = strlen_func[value->valuetype] (src) + 1;

#ifdef MDB_DEBUG
    if (value->valuetype > DT_HEX)
        sc_assert(0, __FILE__, __LINE__);

    if (default_size > MAX_OF_VARCHAR)
        sc_assert(0, __FILE__, __LINE__);

    if (default_size < 0)
        sc_assert(0, __FILE__, __LINE__);
#endif

    if (value->call_free)
    {
        PMEM_FREENUL(value->u.ptr);
    }

    value->value_len = size - 1;

    if (size < default_size)
        size = default_size;

    if ((value->u.ptr = sql_malloc(size, value->valuetype)) == NULL)
        return RET_ERROR;

    strcpy_func[value->valuetype] (value->u.ptr, src);

    value->call_free = 1;


    return RET_SUCCESS;
}

MDB_INLINE int
sql_value_ptrXcalloc(T_VALUEDESC * value, int size)
{
#ifdef MDB_DEBUG
    if (value->call_free)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, -1, 1, "Invalid call_free\n");
        sc_assert(0, __FILE__, __LINE__);
    }

    if (value->valuetype > DT_HEX)
        sc_assert(0, __FILE__, __LINE__);
#else
    if (value->call_free)
        return RET_SUCCESS;
#endif

    if ((value->u.ptr = sql_xcalloc(size, value->valuetype)) == NULL)
        return RET_ERROR;

    value->call_free = 1;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! sql_value_ptrFree 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param value(in/out) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
MDB_INLINE void
sql_value_ptrFree(T_VALUEDESC * value)
{
    if (value->call_free == 0)
        return;

    sql_mfree(value->u.ptr);
    value->u.ptr = NULL;

    value->call_free = 0;
    return;
}

/*****************************************************************************/
//! sql_value_ptrInit

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param value(in/out) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
MDB_INLINE void
sql_value_ptrInit(T_VALUEDESC * value)
{
    value->call_free = 0;

    return;
}

#define _ESTIMATE_MICRO

T_PARSING_MEMORY *
sql_mem_create_chunk(void)
{
    T_PARSING_MEMORY *chunk = NULL;

    // A. Allocate the T_PARSING_MEMORY
    chunk = PMEM_XCALLOC(_alignedlen(sizeof(T_PARSING_MEMORY),
                    sizeof(DB_INT64)));
    if (chunk == NULL)
        goto error;

    // B. Allocate the T_PARSING_MEMORY's first chunk
    chunk->chunk_size[0] = _alignedlen(SQL_PARSING_MEMORY_CHUNK_SIZE,
            sizeof(DB_INT64));
    chunk->alloc_chunk_num = 0;

    chunk->buf[0] = PMEM_XCALLOC(chunk->chunk_size[0]);
    if (chunk->buf[0] == NULL)
        goto error;

    chunk->buf_end[0] = chunk->buf[0] + chunk->chunk_size[0];
    chunk->pos[0] = 0;

    return chunk;

  error:
    sc_assert(0, __FILE__, __LINE__);
    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
            SQL_MEM_E_OUTOFMEMORY, 1, "SQL Memory Init Fail...\n");
    return NULL;
}

T_PARSING_MEMORY *
sql_mem_destroy_chunk(T_PARSING_MEMORY * chunk)
{
    int i = 0;

    if (chunk == NULL)
        return NULL;

    for (i = chunk->alloc_chunk_num; i >= 0; --i)
        PMEM_FREENUL(chunk->buf[i]);
    chunk->alloc_chunk_num = i; // debug info
    PMEM_FREENUL(chunk);

    return chunk;
}

T_PARSING_MEMORY *
sql_mem_cleanup_chunk(T_PARSING_MEMORY * chunk)
{
    int i = 0;

    if (chunk)
    {
        for (i = chunk->alloc_chunk_num; i > 0; --i)
        {
            PMEM_FREENUL(chunk->buf[i]);
        }

        chunk->alloc_chunk_num = i;

        // optimize시 지울수 있음
        sc_memset(chunk->buf[0], 0x00, chunk->pos[0]);
        chunk->pos[0] = 0;
    }

    return chunk;
}

// 새로운 memory allocate
void *
sql_mem_alloc(T_PARSING_MEMORY * chunk, int size)
{
    void *pos = NULL;

#ifdef MDB_DEBUG
    if (size <= 0)      // optimize를 위해서는 제거한다
        sc_assert(0, __FILE__, __LINE__);
#endif

    // A. size를 8byte로 align 시켰을때의 size를 구한다
    size = _alignedlen(size, sizeof(DB_INT64));

    // B. chunk의 size가 모자라면 새로운 chunk를 할당한다.
    if (size >
            (chunk->chunk_size[chunk->alloc_chunk_num] -
                    chunk->pos[chunk->alloc_chunk_num]))
    {
        int chunk_size = 0;

        if (size >= SQL_PARSING_MEMORY_CHUNK_SIZE)
            chunk_size =
                    _alignedlen(size + SQL_PARSING_MEMORY_CHUNK_SIZE,
                    sizeof(DB_INT64));
        else
            chunk_size = SQL_PARSING_MEMORY_CHUNK_SIZE;

        ++chunk->alloc_chunk_num;
        if (chunk->alloc_chunk_num == SQL_PARSING_MEMORY_CHUNK_NUM)
        {
            sc_assert(0, __FILE__, __LINE__);
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_MEM_E_FULL_CHUNK_ARRAY, 1,
                    "SQL Chunk Array is FULL!\n");
            return NULL;
        }

        chunk->buf[chunk->alloc_chunk_num] = PMEM_XCALLOC(chunk_size);
        if (chunk->buf[chunk->alloc_chunk_num] == NULL)
        {
            --chunk->alloc_chunk_num;
            sc_assert(0, __FILE__, __LINE__);
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_MEM_E_OUTOFMEMORY, 1,
                    "can't allocate memory!\n");
            return NULL;
        }
        chunk->buf_end[chunk->alloc_chunk_num] =
                chunk->buf[chunk->alloc_chunk_num] + chunk_size;
        chunk->pos[chunk->alloc_chunk_num] = 0;
        chunk->chunk_size[chunk->alloc_chunk_num] = chunk_size;
    }

    pos = (void *) (chunk->buf[chunk->alloc_chunk_num] +
            chunk->pos[chunk->alloc_chunk_num]);
    chunk->pos[chunk->alloc_chunk_num] =
            chunk->pos[chunk->alloc_chunk_num] + size;

#ifdef MDB_DEBUG
    sc_assert((unsigned long) pos >=
            (unsigned long) chunk->buf[chunk->alloc_chunk_num],
            __FILE__, __LINE__);
    sc_assert((unsigned long) pos <
            (unsigned long) chunk->buf_end[chunk->alloc_chunk_num],
            __FILE__, __LINE__);
#endif
    return pos;
}

void *
sql_mem_strdup(T_PARSING_MEMORY * chunk, char *src)
{
    int len = 0;
    void *pos = NULL;

#ifdef MDB_DEBUG
    if (!src)
    {
        sc_assert(0, __FILE__, __LINE__);
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_MEM_E_INVALIDARGUMENT,
                1, "SQL memory.. source string is NULL ...\n");
        return NULL;
    }
#endif
    len = sc_strlen(src);
    pos = sql_mem_alloc(chunk, len + 1);
    if (!pos)
        return NULL;

    sc_memcpy(pos, src, len + 1);
    return pos;
}

void *
sql_mem_calloc(T_PARSING_MEMORY * chunk, int size)
{
    void *ptr = NULL;

    ptr = sql_mem_alloc(chunk, size);
#ifdef MDB_DEBUG
    if (!ptr)
        sc_assert(0, __FILE__, __LINE__);
#endif
    sc_memset(ptr, 0x00, size);
    return ptr;
}


void
sql_mem_free(T_PARSING_MEMORY * chunk, char *p_)
{
    int i = 0;

    if ((chunk))
    {
        for (i = 0; i <= (chunk)->alloc_chunk_num; ++i)
        {
            if (((chunk)->buf[i] <= (char *) (p_)) &&
                    ((char *) p_ <= (chunk)->buf_end[i]))
            {
                (p_) = NULL;
                break;
            }
        }

        if ((p_) != NULL)
            PMEM_FREE(p_);
    }
    else
    {
        if ((p_) != NULL)
            PMEM_FREE(p_);
    }

    return;
}

// T_POSTFIXELEM을 관리하기 위한 함수들..
// 임시로 이렇게 할당한다
T_POSTFIXELEM *
sql_mem_get_free_postfixelem(T_PARSING_MEMORY * chunk)
{
    if (chunk == NULL)
        return PMEM_ALLOC(sizeof(T_POSTFIXELEM));
    else
        return sql_mem_alloc(chunk, sizeof(T_POSTFIXELEM));
}

void
sql_mem_set_free_postfixelem(T_PARSING_MEMORY * chunk, T_POSTFIXELEM * elem)
{
    if (chunk == NULL)
        PMEM_FREENUL(elem);
    else
        sql_mem_freenul(chunk, elem);
}


T_POSTFIXELEM **
sql_mem_get_free_postfixelem_list(T_PARSING_MEMORY * chunk, int max_expr)
{
    T_POSTFIXELEM **elem;

    if (chunk == NULL)
        elem = (T_POSTFIXELEM **) PMEM_XCALLOC(sizeof(T_POSTFIXELEM *) *
                max_expr);
    else
        elem = (T_POSTFIXELEM **) sql_mem_calloc(chunk,
                (sizeof(T_POSTFIXELEM *) * max_expr));
    if (elem == NULL)
        return NULL;

    return elem;
}

// SQL_NEW_MEMORY
// macro로 수정되어야 할 부분임..
void
sql_mem_set_free_postfixelem_list(T_PARSING_MEMORY * chunk,
        T_POSTFIXELEM ** list)
{
    if (chunk == NULL)
        PMEM_FREENUL(list);
    else
        sql_mem_freenul(chunk, list);
}

// T_EXPRESSIONDESC에 해당하는 공간을 할당한다.
T_EXPRESSIONDESC *
sql_mem_get_free_expressiondesc(T_PARSING_MEMORY * chunk, int use_expr,
        int max_expr)
{
    int i = 0, j = 0;
    T_EXPRESSIONDESC *expr = NULL;

    if (chunk == NULL)
        expr = PMEM_ALLOC(sizeof(T_EXPRESSIONDESC));
    else
        expr = sql_mem_alloc(chunk, sizeof(T_EXPRESSIONDESC));
    if (!expr)
        return NULL;

    if (max_expr == 0)
        return expr;

    expr->list = sql_mem_get_free_postfixelem_list(chunk, max_expr);
    if (!expr->list)
        goto ERROR_LABEL;

    expr->len = use_expr;
    expr->max = max_expr;

    for (i = 0; i < use_expr; ++i)
    {
        expr->list[i] = sql_mem_get_free_postfixelem(chunk);
        if (expr->list[i] == NULL)
            goto ERROR_LABEL;
    }
    return expr;

  ERROR_LABEL:
    for (j = 0; j < expr->max; ++j)
        sql_mem_freenul(chunk, expr->list[j]);
    sql_mem_freenul(chunk, expr->list);

    return NULL;
}


// T_POSTFIXELEM**을 관리하기 위한 함수들


/*****************************************************************************/
//! sc_bstring2binary
/*! \breif 2진수 string을 binary로 변환
 ************************************
 * \param src(in)       : 2진수 string
 * \param dst(in/out)   : binary string값
 ************************************
 * \return if ret > 0 then binary string's len, else if ret < 0 is error
 ************************************
 * \note
 *  - 함수 추가  
 *****************************************************************************/
int
sc_bstring2binary(char *src, char *dst)
{
    int conveted = 0;
    int len = sc_strlen(src);
    char value;

    for (conveted = 0; conveted < (len >> 3); ++conveted)
    {
        value = 0;
        value += (src[conveted * 8] - '0') * 128;
        value += (src[conveted * 8 + 1] - '0') * 64;
        value += (src[conveted * 8 + 2] - '0') * 32;
        value += (src[conveted * 8 + 3] - '0') * 16;
        value += (src[conveted * 8 + 4] - '0') * 8;
        value += (src[conveted * 8 + 5] - '0') * 4;
        value += (src[conveted * 8 + 6] - '0') * 2;
        value += (src[conveted * 8 + 7] - '0') * 1;
        dst[conveted] = value;
    }
    return conveted;
}

/*****************************************************************************/
//! sc_hstring2binary
/*! \breif 16진수 string을 binary로 변환
 ************************************
 * \param src(in)       : 16진수 string
 * \param dst(in/out)   : binary string값
 ************************************
 * \return if ret > 0 then binary string's len, else if ret < 0 is error
 ************************************
 * \note
 *  - 함수 추가  
 *****************************************************************************/
int
sc_hstring2binary(char *src, char *dst)
{
    int conveted = 0;
    int len = sc_strlen(src);
    char value, ch;

    for (conveted = 0; conveted < (len >> 1); ++conveted)
    {
        // 1. first characeter
        ch = src[conveted * 2];
        if (ch >= '0' && ch <= '9')
            ch = ch - '0';
        else if (ch >= 'a' && ch <= 'f')
            ch = 10 + ch - 'a';
        else if (ch >= 'A' && ch <= 'F')
            ch = 10 + ch - 'A';
        value = ch * 16;

        // 2. second characeter
        ch = src[conveted * 2 + 1];
        if (ch >= '0' && ch <= '9')
            ch = ch - '0';
        else if (ch >= 'a' && ch <= 'f')
            ch = 10 + ch - 'a';
        else if (ch >= 'A' && ch <= 'F')
            ch = 10 + ch - 'A';
        value = value + ch;
        dst[conveted] = value;
    }
    return conveted;
}

int
sql_expr_has_ridfunc(T_EXPRESSIONDESC * expr)
{
    int i;

    for (i = 0; i < expr->len; i++)
    {
        if ((expr->list[i]->elemtype == SPT_FUNCTION &&
                        expr->list[i]->u.func.type == SFT_RID)
                || (expr->list[i]->u.value.attrdesc.attrname != NULL
                        && !mdb_strcmp("rid",
                                expr->list[i]->u.value.attrdesc.attrname)))
        {
            return DB_TRUE;
        }
    }

    return DB_FALSE;
}


int
sql_collation_compatible_check(DataType datatype, MDB_COL_TYPE coltype)
{
    int compatible = DB_TRUE;

    switch (coltype)
    {
    case MDB_COL_NONE:
        break;

    case MDB_COL_NUMERIC:
        break;

    case MDB_COL_CHAR_NOCASE:
    case MDB_COL_CHAR_CASE:
    case MDB_COL_CHAR_DICORDER:
        if (!IS_BS(datatype))
            compatible = DB_FALSE;

        break;

    case MDB_COL_CHAR_REVERSE:
        if (datatype != DT_CHAR)
            compatible = DB_FALSE;
        break;

    case MDB_COL_NCHAR_NOCASE:
    case MDB_COL_NCHAR_CASE:
        if (!IS_NBS(datatype))
            compatible = DB_FALSE;

        break;

    case MDB_COL_BYTE:
    case MDB_COL_USER_TYPE1:
    case MDB_COL_USER_TYPE2:
    case MDB_COL_USER_TYPE3:
        /* ... */
        break;
    default:
        break;
    }

    return compatible;
}

static int
get_value_to_int(T_VALUEDESC * value, bigint * result_value)
{
    bigint int_value = 0;
    double float_value = 0;

    if (IS_INTEGER(value->valuetype))
    {
        GET_INT_VALUE(int_value, bigint, value);
        *result_value = int_value;
    }
    else if (IS_REAL(value->valuetype))
    {
        GET_FLOAT_VALUE(float_value, double, value);

        *result_value = (bigint) float_value;
    }
    else
        return RET_ERROR;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! get_value_to_float

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param value(in)         :
 * \param result_value(out) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *****************************************************************************/
static int
get_value_to_float(T_VALUEDESC * value, double *result_value)
{
    bigint int_value = 0;
    double float_value = 0;

    if (IS_INTEGER(value->valuetype))
    {
        GET_INT_VALUE(int_value, bigint, value);
        *result_value = (double) int_value;
    }
    else if (IS_REAL(value->valuetype))
    {
        GET_FLOAT_VALUE(float_value, double, value);

        *result_value = float_value;
    }
    else
        return RET_ERROR;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! __compare_values 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param left(in)          :
 * \param right(in)         :
 * \param result(in/out)    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - T_VALUEDESC의 멤버 변수 valuetype 제거
 *      collation 함수 추가함
 *  - 비교시 collation이 다를 경우.. 일단은 error
 *****************************************************************************/
int
sql_compare_values(T_VALUEDESC * left, T_VALUEDESC * right, bigint * result)
{
    char time_buf[32];
    char *datetime = time_buf;
    char *date = time_buf;
    char *time = time_buf;
    t_astime timestamp;
    int right_len;
    bigint int4left = 0, int4right = 0;
    double float4left = 0, float4right = 0;

    switch (left->valuetype)
    {
    case DT_NULL_TYPE:
        return RET_ERROR;

    case DT_TINYINT:
    case SQL_DATA_SMALLINT:
    case DT_INTEGER:
    case SQL_DATA_BIGINT:
    case DT_OID:
        if (left->valuetype == right->valuetype)
        {
            *result = left->u.i64 - right->u.i64;
        }
        else
        {
            GET_INT_VALUE(int4left, bigint, left);
            if (get_value_to_int(right, &int4right) == RET_ERROR)
                return RET_ERROR;

            *result = int4left - int4right;
        }

        return RET_SUCCESS;

    case DT_FLOAT:
    case DT_DOUBLE:
    case DT_DECIMAL:

        GET_FLOAT_VALUE(float4left, double, left);

        if (get_value_to_float(right, &float4right) == RET_ERROR)
            return RET_ERROR;

        if (left->valuetype == DT_FLOAT || right->valuetype == DT_FLOAT)
        {
            if ((float) float4left == (float) float4right)
                *result = 0;
            else if ((float) float4left < (float) float4right)
                *result = -1;
            else
                *result = 1;
        }
        else
        {
            if (float4left == float4right)
                *result = 0;
            else if (float4left < float4right)
                *result = -1;
            else
                *result = 1;
        }

        return RET_SUCCESS;

    case DT_CHAR:
        {
            if (left->valueclass == SVC_CONSTANT &&
                    right->valueclass == SVC_VARIABLE)
                left->attrdesc.collation = right->attrdesc.collation;
            else if (left->valueclass == SVC_VARIABLE &&
                    right->valueclass == SVC_CONSTANT)
                right->attrdesc.collation = left->attrdesc.collation;

            if (left->attrdesc.collation != right->attrdesc.collation)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_MISMATCH_COLLATION, 0);
                return RET_ERROR;
            }

            if (right->valuetype == DT_CHAR)
            {
                if (left->attrdesc.len < right->attrdesc.len)
                {
                    *result =
                            sc_ncollate_func[left->attrdesc.collation] (left->
                            u.ptr, right->u.ptr, left->attrdesc.len);

                    if (*result == 0 &&
                            right->u.ptr[left->attrdesc.len] != '\0')
                        *result = -1;

                }
                else if (left->attrdesc.len > right->attrdesc.len)
                {
                    *result =
                            sc_ncollate_func[left->attrdesc.collation] (left->
                            u.ptr, right->u.ptr, right->attrdesc.len);

                    if (*result == 0 &&
                            left->u.ptr[right->attrdesc.len] != '\0')
                        *result = 1;
                }
                else
                {
                    *result =
                            sc_ncollate_func[left->attrdesc.collation] (left->
                            u.ptr, right->u.ptr, right->attrdesc.len);
                }
            }
            else if (right->valuetype == DT_VARCHAR)
            {
                right_len = sc_strlen(right->u.ptr);
                if (left->attrdesc.len < right_len)
                {
                    *result =
                            sc_ncollate_func[left->attrdesc.collation] (left->
                            u.ptr, right->u.ptr, left->attrdesc.len);
                    if (*result == 0)
                        *result = 1;
                }
                else if (left->attrdesc.len > right_len)
                {
                    *result =
                            sc_ncollate_func[left->attrdesc.collation] (left->
                            u.ptr, right->u.ptr, left->attrdesc.len);

                    if (*result == 0 && left->u.ptr[right_len] != '\0')
                        *result = -1;

                }
                else
                {
                    *result =
                            sc_ncollate_func[left->attrdesc.collation] (left->
                            u.ptr, right->u.ptr, right_len);
                }
            }
            else if (right->valuetype == DT_DATETIME)
            {
                char2datetime(datetime, left->u.ptr, sc_strlen(left->u.ptr));

                *result = sc_memcmp(datetime, right->u.datetime,
                        MAX_DATETIME_FIELD_LEN);
            }
            else if (right->valuetype == DT_DATE)
            {
                char2date(date, left->u.ptr, sc_strlen(left->u.ptr));

                *result = sc_memcmp(date, right->u.date, MAX_DATE_FIELD_LEN);

            }
            else if (right->valuetype == DT_TIME)
            {
                char2time(time, left->u.ptr, sc_strlen(left->u.ptr));

                *result = sc_memcmp(time, right->u.time, MAX_TIME_FIELD_LEN);

            }
            else if (right->valuetype == DT_TIMESTAMP)
            {
                char2timestamp(&timestamp, left->u.ptr,
                        sc_strlen(left->u.ptr));

                *result = timestamp - right->u.timestamp;
            }
            else
                return RET_ERROR;

            break;
        }

    case DT_NCHAR:
        {
            if (left->valueclass == SVC_CONSTANT &&
                    right->valueclass == SVC_VARIABLE)
                left->attrdesc.collation = right->attrdesc.collation;
            else if (left->valueclass == SVC_VARIABLE &&
                    right->valueclass == SVC_CONSTANT)
                right->attrdesc.collation = left->attrdesc.collation;

            if (left->attrdesc.collation != right->attrdesc.collation)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_MISMATCH_COLLATION, 0);
                return RET_ERROR;
            }

            if (right->valuetype == DT_NCHAR)
            {
                if (left->attrdesc.len < right->attrdesc.len)
                {
                    *result =
                            sc_ncollate_func[left->attrdesc.collation] (left->
                            u.ptr, right->u.ptr, left->attrdesc.len);

                    if (*result == 0 &&
                            right->u.Wptr[left->attrdesc.len] != L'\0')
                        *result = -1;
                }
                if (left->attrdesc.len > right->attrdesc.len)
                {
                    *result =
                            sc_ncollate_func[left->attrdesc.collation] (left->
                            u.ptr, right->u.ptr, right->attrdesc.len);

                    if (*result == 0 &&
                            left->u.Wptr[left->attrdesc.len] != L'\0')
                        *result = 1;
                }
                else
                {
                    *result =
                            sc_ncollate_func[left->attrdesc.collation] (left->
                            u.ptr, right->u.ptr, right->attrdesc.len);
                }
            }
            else if (right->valuetype == DT_NVARCHAR)
            {
                right_len = sc_wcslen(right->u.Wptr);

                if (left->attrdesc.len < right_len)
                {
                    *result =
                            sc_ncollate_func[left->attrdesc.collation] (left->
                            u.ptr, right->u.ptr, left->attrdesc.len);

                    if (*result == 0)
                        *result = -1;
                }
                if (left->attrdesc.len > right_len)
                {
                    *result =
                            sc_ncollate_func[left->attrdesc.collation] (left->
                            u.ptr, right->u.ptr, right_len);

                    if (*result == 0 && left->u.Wptr[right_len] != L'\0')
                        *result = 1;
                }
                else
                {
                    *result =
                            sc_ncollate_func[left->attrdesc.collation] (left->
                            u.ptr, right->u.ptr, right_len);
                }
            }
            else
                return RET_ERROR;

            break;
        }

    case DT_VARCHAR:
        {
            if (left->valueclass == SVC_CONSTANT &&
                    right->valueclass == SVC_VARIABLE)
                left->attrdesc.collation = right->attrdesc.collation;
            else if (left->valueclass == SVC_VARIABLE &&
                    right->valueclass == SVC_CONSTANT)
                right->attrdesc.collation = left->attrdesc.collation;

            if (left->attrdesc.collation != right->attrdesc.collation)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_MISMATCH_COLLATION, 0);
                return RET_ERROR;
            }

            if (right->valuetype == DT_CHAR)
            {
                if (left->attrdesc.len < right->attrdesc.len)
                {
                    *result =
                            sc_ncollate_func[left->attrdesc.collation] (left->
                            u.ptr, right->u.ptr, left->attrdesc.len);

                    if (*result == 0 &&
                            right->u.ptr[left->attrdesc.len] != '\0')
                        *result = -1;
                }
                else if (left->attrdesc.len > right->attrdesc.len)
                {
                    *result =
                            sc_ncollate_func[left->attrdesc.collation] (left->
                            u.ptr, right->u.ptr, right->attrdesc.len);

                    if (*result == 0 &&
                            left->u.ptr[left->attrdesc.len] != '\0')
                        *result = 1;
                }
                else
                {
                    *result =
                            sc_ncollate_func[left->attrdesc.collation] (left->
                            u.ptr, right->u.ptr, right->attrdesc.len);
                }
            }
            else if (right->valuetype == DT_VARCHAR)
            {
                *result =
                        sc_collate_func[left->attrdesc.collation] (left->u.ptr,
                        right->u.ptr);
            }
            else if (right->valuetype == DT_DATETIME)
            {
                char2datetime(datetime, left->u.ptr, sc_strlen(left->u.ptr));

                *result = sc_memcmp(datetime, right->u.datetime,
                        MAX_DATETIME_FIELD_LEN);
            }
            else if (right->valuetype == DT_DATE)
            {
                char2date(date, left->u.ptr, sc_strlen(left->u.ptr));

                *result = sc_memcmp(date, right->u.date, MAX_DATE_FIELD_LEN);
            }
            else if (right->valuetype == DT_TIME)
            {
                char2time(time, left->u.ptr, sc_strlen(left->u.ptr));

                *result = sc_memcmp(time, right->u.time, MAX_TIME_FIELD_LEN);
            }
            else if (right->valuetype == DT_TIMESTAMP)
            {
                char2timestamp(&timestamp, left->u.ptr,
                        sc_strlen(left->u.ptr));
                *result = timestamp - right->u.timestamp;
            }
            else
                return RET_ERROR;

            break;
        }

    case DT_NVARCHAR:
        {
            if (left->valueclass == SVC_CONSTANT &&
                    right->valueclass == SVC_VARIABLE)
                left->attrdesc.collation = right->attrdesc.collation;
            else if (left->valueclass == SVC_VARIABLE &&
                    right->valueclass == SVC_CONSTANT)
                right->attrdesc.collation = left->attrdesc.collation;

            if (left->attrdesc.collation != right->attrdesc.collation)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_MISMATCH_COLLATION, 0);
                return RET_ERROR;
            }

            if (right->valuetype == DT_NCHAR)
            {
                if (left->attrdesc.len < right->attrdesc.len)
                {
                    *result =
                            sc_ncollate_func[left->attrdesc.collation] (left->
                            u.ptr, right->u.ptr, left->attrdesc.len);

                    if (*result == 0 &&
                            right->u.Wptr[left->attrdesc.len] != '\0')
                        *result = -1;
                }
                else if (left->attrdesc.len > right->attrdesc.len)
                {
                    *result =
                            sc_ncollate_func[left->attrdesc.collation] (left->
                            u.ptr, right->u.ptr, right->attrdesc.len);

                    if (*result == 0 &&
                            left->u.Wptr[left->attrdesc.len] != '\0')
                        *result = 1;
                }
                else
                {
                    *result =
                            sc_ncollate_func[left->attrdesc.collation] (left->
                            u.ptr, right->u.ptr, right->attrdesc.len);
                }
            }
            else if (right->valuetype == DT_NVARCHAR)
            {
                *result =
                        sc_collate_func[left->attrdesc.collation] (left->u.ptr,
                        right->u.ptr);
            }

            break;
        }

    case DT_TIMESTAMP:
        {
            if (IS_BS(right->valuetype))
            {
                char2timestamp(&timestamp, right->u.ptr,
                        sc_strlen(right->u.ptr));
                *result = left->u.timestamp - timestamp;
            }
            else if (right->attrdesc.type == DT_DATETIME)
            {
                datetime2char(datetime, right->u.datetime);
                char2timestamp(&timestamp, datetime, sc_strlen(datetime));
                *result = left->u.timestamp - timestamp;
            }
            else if (right->attrdesc.type == DT_DATE)
            {
                date2char(date, right->u.date);
                char2timestamp(&timestamp, date, sc_strlen(date));
                *result = left->u.timestamp - timestamp;
            }
            else if (right->attrdesc.type == DT_TIME)
            {
            }
            else if (right->valuetype == DT_TIMESTAMP)
            {
                *result = left->u.timestamp - right->u.timestamp;
            }

            break;
        }

    case DT_DATETIME:
        {
            if (right->valuetype == DT_CHAR)
            {
                char2datetime(datetime, right->u.ptr, sc_strlen(right->u.ptr));
                *result = sc_memcmp(left->u.datetime, datetime,
                        MAX_DATETIME_FIELD_LEN);
            }
            else if (right->valuetype == DT_VARCHAR)
            {
                char2datetime(datetime, right->u.ptr, sc_strlen(right->u.ptr));
                *result = sc_memcmp(left->u.datetime, datetime,
                        MAX_DATETIME_FIELD_LEN);
            }
            else if (right->valuetype == DT_DATETIME)
            {
                *result = sc_memcmp(left->u.datetime, right->u.datetime,
                        MAX_DATETIME_FIELD_LEN);
            }
            else if (right->valuetype == DT_DATE)
            {
                date2char(date, right->u.date);
                char2datetime(datetime, date, sc_strlen(date));
                *result = sc_memcmp(left->u.datetime, datetime,
                        MAX_DATETIME_FIELD_LEN);
            }
            else if (right->valuetype == DT_TIME)
            {
            }
            else if (right->valuetype == DT_TIMESTAMP)
            {
                datetime2char(datetime, left->u.datetime);
                char2timestamp(&timestamp, datetime, sc_strlen(datetime));
                *result = timestamp - right->u.timestamp;
            }

            break;
        }
    case DT_DATE:
        {
            if (right->valuetype == DT_CHAR)
            {
                char2date(date, right->u.ptr, sc_strlen(right->u.ptr));
                *result = sc_memcmp(left->u.date, date, MAX_DATE_FIELD_LEN);
            }
            else if (right->valuetype == DT_VARCHAR)
            {
                char2date(date, right->u.ptr, sc_strlen(right->u.ptr));
                *result = sc_memcmp(left->u.date, date, MAX_DATE_FIELD_LEN);
            }
            else if (right->valuetype == DT_DATETIME)
            {
                datetime2char(datetime, right->u.datetime);
                char2date(date, datetime, sc_strlen(datetime));
                *result = sc_memcmp(left->u.date, date, MAX_DATE_FIELD_LEN);
            }
            else if (right->valuetype == DT_DATE)
            {
                *result = sc_memcmp(left->u.date, right->u.date,
                        MAX_DATE_FIELD_LEN);
            }
            else if (right->valuetype == DT_TIMESTAMP)
            {
                date2char(date, left->u.date);
                char2timestamp(&timestamp, date, sc_strlen(date));
                *result = timestamp - right->u.timestamp;
            }

            break;
        }
    case DT_TIME:
        {
            if (right->valuetype == DT_CHAR)
            {
                char2time(time, right->u.ptr, sc_strlen(right->u.ptr));
                *result = sc_memcmp(left->u.time, time, MAX_TIME_FIELD_LEN);
            }
            else if (right->valuetype == DT_VARCHAR)
            {
                char2time(time, right->u.ptr, sc_strlen(right->u.ptr));
                *result = sc_memcmp(left->u.time, time, MAX_TIME_FIELD_LEN);
            }
            break;
        }

    default:
        return RET_ERROR;
    }

    return RET_SUCCESS;
}

/* it can not compare diffent types */
int
sql_compare_elems(void const *_elem1, void const *_elem2)
{
    bigint res;
    T_POSTFIXELEM *elem1 = (T_POSTFIXELEM *) _elem1;
    T_POSTFIXELEM *elem2 = (T_POSTFIXELEM *) _elem2;

    /* remove conditional jump */
    if (sql_compare_values(&elem1->u.value, &elem2->u.value, &res) < 0)
    {
        /* caller how to check it? */
        return RET_ERROR;
    }

    if (res > 0)
        res = 1;
    else if (res < 0)
        res = -1;

    return (int) res;
}

bigint
__GET_INT_VALUE(T_VALUEDESC * v_, DB_BOOL is_float)
{
    char buf[32];

    switch (v_->valuetype)
    {
    case DT_TINYINT:
        return (bigint) v_->u.i8;
    case DT_SMALLINT:
        return (bigint) v_->u.i16;
    case DT_INTEGER:
        return (bigint) v_->u.i32;
    case DT_BIGINT:
        return v_->u.i64;
    case DT_OID:
        return (bigint) v_->u.oid;
    case DT_FLOAT:
        return (bigint) v_->u.f16;
    case DT_DOUBLE:
        return (bigint) v_->u.f32;
    case DT_DECIMAL:
        return (bigint) v_->u.dec;
    case DT_CHAR:
    case DT_VARCHAR:
        if (is_float)
            return (bigint) sc_atof(v_->u.ptr);
        else
            return sc_atoll(v_->u.ptr);
    case DT_DATETIME:
        datetime2char(buf, v_->u.datetime);
        return mdb_char2bigint(buf);
    case DT_DATE:
        date2char(buf, v_->u.date);
        return mdb_char2bigint(buf);
    case DT_TIME:
        time2char(buf, v_->u.time);
        return mdb_char2bigint(buf);
    case DT_TIMESTAMP:
        timestamp2char(buf, &v_->u.timestamp);
        return mdb_char2bigint(buf);
    default:
        return 0;
    }

    return 0;
}

double
__GET_FLOAT_VALUE(T_VALUEDESC * v_, DB_BOOL is_float)
{
    char buf[32];

    switch (v_->valuetype)
    {
    case DT_TINYINT:
        return (double) v_->u.i8;
    case DT_SMALLINT:
        return (double) v_->u.i16;
    case DT_INTEGER:
        return (double) v_->u.i32;
    case DT_BIGINT:
        return (double) v_->u.i64;
    case DT_OID:
#if defined(_64BIT) && defined(WIN32)
        return (double) (DB_INT64) v_->u.oid;
#else
        return (double) v_->u.oid;
#endif
    case DT_FLOAT:
        return (double) v_->u.f16;
    case DT_DOUBLE:
        return v_->u.f32;
    case DT_DECIMAL:
        return (double) v_->u.dec;
    case DT_CHAR:
    case DT_VARCHAR:
        if (is_float)
            return sc_atof(v_->u.ptr);
        else
            return (double) sc_atoll(v_->u.ptr);
    case DT_DATETIME:
        datetime2char(buf, v_->u.datetime);
        return (double) mdb_char2bigint(buf);
    case DT_DATE:
        date2char(buf, v_->u.date);
        return (double) mdb_char2bigint(buf);
    case DT_TIME:
        time2char(buf, v_->u.time);
        return (double) mdb_char2bigint(buf);
    case DT_TIMESTAMP:
        timestamp2char(buf, &v_->u.timestamp);
        return (double) mdb_char2bigint(buf);

    default:
        return 0;
    }

    return 0;
}

char *
__PUT_DEFAULT_VALUE(T_ATTRDESC * attr_, char *str_dec_)
{
    char *defval_;

    if (attr_->flag & DEF_SYSDATE_BIT)
    {
        return attr_->defvalue.u.ptr;
    }
    switch (attr_->type)
    {
    case DT_VARCHAR:
    case DT_CHAR:
        defval_ = attr_->defvalue.u.ptr;
        if (defval_ && (int) sc_strlen(defval_) > attr_->len)
            defval_[attr_->len] = '0';
        break;
    case DT_NVARCHAR:
    case DT_NCHAR:
        defval_ = (char *) attr_->defvalue.u.Wptr;
        if (defval_ && sc_wcslen(attr_->defvalue.u.Wptr) > attr_->len)
            attr_->defvalue.u.Wptr[attr_->len] = '0';
        break;
    case DT_BYTE:
    case DT_VARBYTE:
        DB_SET_BYTE(str_dec_, attr_->defvalue.u.ptr,
                attr_->defvalue.value_len);
        defval_ = str_dec_;
        break;
    case DT_SMALLINT:
        defval_ = (char *) &attr_->defvalue.u.i16;
        break;
    case DT_INTEGER:
        defval_ = (char *) &attr_->defvalue.u.i32;
        break;
    case DT_FLOAT:
        defval_ = (char *) &attr_->defvalue.u.f16;
        break;
    case DT_DOUBLE:
        defval_ = (char *) &attr_->defvalue.u.f32;
        break;
    case DT_TINYINT:
        defval_ = (char *) &attr_->defvalue.u.i8;
        break;
    case DT_BIGINT:
        defval_ = (char *) &attr_->defvalue.u.i64;
        break;
    case DT_DATETIME:
        defval_ = attr_->defvalue.u.datetime;
        break;
    case DT_TIME:
        defval_ = attr_->defvalue.u.time;
        break;
    case DT_DATE:
        defval_ = attr_->defvalue.u.date;
        break;
    case DT_TIMESTAMP:
        defval_ = (char *) &attr_->defvalue.u.timestamp;
        break;
    case DT_DECIMAL:
        decimal2char(str_dec_, attr_->defvalue.u.dec, attr_->len, attr_->dec);
        defval_ = str_dec_;
        break;
    default:
        defval_ = NULL;
        break;
    }

    return defval_;
}

void
__PUT_INT_VALUE(T_VALUEDESC * v_, bigint int_)
{
    switch (v_->valuetype)
    {
    case DT_TINYINT:
        v_->u.i8 = (tinyint) int_;
        break;
    case DT_SMALLINT:
        v_->u.i16 = (smallint) int_;
        break;
    case DT_INTEGER:
        v_->u.i32 = (int) int_;
        break;
    case DT_BIGINT:
        v_->u.i64 = int_;
        break;
    case DT_OID:
        v_->u.oid = (OID) int_;
        break;
    case DT_FLOAT:
        v_->u.f16 = (float) int_;
        break;
    case DT_DOUBLE:
        v_->u.f32 = (double) int_;
        break;
    case DT_DECIMAL:
        v_->u.dec = (decimal) int_;
        break;
    default:
        v_->u.i64 = 0;
        break;
    }
}

void
__PUT_FLOAT_VALUE(T_VALUEDESC * v_, double flv_)
{
    switch (v_->valuetype)
    {
    case DT_TINYINT:
        v_->u.i8 = (tinyint) flv_;
        break;
    case DT_SMALLINT:
        v_->u.i16 = (smallint) flv_;
        break;
    case DT_INTEGER:
        v_->u.i32 = (int) flv_;
        break;
    case DT_BIGINT:
        v_->u.i64 = (bigint) flv_;
        break;
    case DT_OID:
        v_->u.oid = (OID) flv_;
        break;
    case DT_FLOAT:
        v_->u.f16 = (float) flv_;
        break;
    case DT_DOUBLE:
        v_->u.f32 = flv_;
        break;
    case DT_DECIMAL:
        v_->u.dec = (decimal) flv_;
        break;
    default:
        v_->u.i64 = 0;
        break;
    }
}

char *
mdb_lwrcpy(int n, char *dest, const char *src)
{
    register int i;

    for (i = 0; i < n - 1 && (dest[i] = mdb_tolwr(src[i])) != '\0'; i++);

    dest[i] = '\0';

    return dest;
}
