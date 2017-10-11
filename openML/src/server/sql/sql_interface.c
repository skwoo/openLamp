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
#include "mdb_comm_stub.h"
#include "sql_packet_format.h"

#include "sql_main.h"
#include "sql_func_timeutil.h"
#include "sql_mclient.h"

#include "sql_parser.h"

/* for RESULT LIST handling */
#include "sql_util.h"

#include "mdb_syslog.h"

#include "mdb_Server.h"

extern int calc_func_convert(T_VALUEDESC * base, T_VALUEDESC * src,
        T_VALUEDESC * res, MDB_INT32 is_init);
extern int calc_function(T_FUNCDESC * func, T_POSTFIXELEM * res,
        T_POSTFIXELEM ** args, MDB_INT32 is_init);

extern int CSizeOfType[];

void
check_end_of_query(char *query, int *len)
{
    while ((*len)--)
    {
        if (*len < 0)
            return;

        // psos에서 존재하는지 모름 
        if (sc_isspace((int) (query[*len])) || query[*len] == '\0')
            continue;
        else if (query[*len] == ';')
        {
            /* do nothing */
        }
        else
        {
            break;
        }
    }
}

int
get_last_error(iSQL * isql, int user_errno)
{
    char *p;

    SMEM_FREENUL(isql->last_error);

    if (user_errno > 0)
    {
        isql->last_errno = er_errid();
        p = er_message();
        if (p != NULL)
            isql->last_error = mdb_strdup(p);
        else
            isql->last_error = mdb_strdup("");
    }
    else
    {
        isql->last_errno = user_errno;
        isql->last_error = NULL;
    }

    return 0;
}

/*****************************************************************************/
//!  allocate_statement

/*! \breif  gClient에 statement를 할당할때 사용됨
 ************************************
 * \param sql(in/out)      : gClient's T_SQL
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  gClient에는 기본적으로 stmt가 한개 할당되어 있다.
 *  PREPARE - EXECUTE를 사용하는 경우에는 여러개의 statement를 할당한다.
 *  (2가지 경우에서만 사용되는 듯함)
 *
 *****************************************************************************/
static int
allocate_statement(T_SQL * sql)
{
    T_STATEMENT *stmt = sql->stmts;
    T_STATEMENT *tmp;
    T_PARSING_MEMORY *sql_parsing_memory = NULL;

    if (sql->stmt_num >= STMT_MAX)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_EXCEEDMAXSTATEMENT, 0);
        return SQL_E_EXCEEDMAXSTATEMENT;
    }

    if (sql->stmt_num == 0)
    {
        // A. 처음으로 stmt를 할당 받는 경우
        sql_parsing_memory = sql->stmts->parsing_memory;

        sc_memset(sql->stmts, 0, sizeof(T_STATEMENT));
        sql->stmts->id = sql->qs_id++;
        sql->stmts->prev = sql->stmts;
        sql->stmts->next = sql->stmts;

        sql->stmts->is_main_stmt = MDB_TRUE;

        if (!sql_parsing_memory)
            sql->stmts->parsing_memory = sql_mem_create_chunk();
        else
            sql->stmts->parsing_memory = sql_parsing_memory;

        if (!sql->stmts->parsing_memory)
        {
#ifdef MDB_DEBUG
            sc_assert(0, __FILE__, __LINE__);
#endif
            return SQL_E_OUTOFMEMORY;
        }
    }
    else
    {
        // B. prepare-execute의 형태로 새로운 stmt를 할당받음
        tmp = (T_STATEMENT *) PMEM_ALLOC(sizeof(T_STATEMENT));
        if (!tmp)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return SQL_E_OUTOFMEMORY;
        }

        tmp->status = iSQL_STAT_READY;
        tmp->id = sql->qs_id++;

        tmp->is_main_stmt = MDB_TRUE;

        if (sql->qs_id == MDB_UINT_MAX)
            sql->qs_id = 0;

        tmp->parsing_memory = sql_mem_create_chunk();
        if (!tmp->parsing_memory)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return SQL_E_OUTOFMEMORY;
        }

        tmp->next = stmt;
        tmp->prev = stmt->prev;
        stmt->prev->next = tmp;
        stmt->prev = tmp;
    }

    sql->stmt_num++;

    return SQL_E_SUCCESS;
}

/*****************************************************************************/
//!  get_statement

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param sql(in)       :
 * \param id(in)        :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static T_STATEMENT *
get_statement(T_SQL * sql, unsigned int id)
{
    T_STATEMENT *stmt = sql->stmts;
    int i;

    for (i = 0; i < sql->stmt_num; i++)
    {
        if (stmt->id == id)
            return stmt;

        stmt = stmt->next;
    }

    return NULL;
}

/*****************************************************************************/
//!  close_all_cursor
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param hDB(in):
 * \param stmt(in):
 ************************************
 * \return  void
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static void
close_all_cursor(int hDB, T_STATEMENT * stmt)
{
    SQL_close_all_cursors(hDB, stmt);
}

/*****************************************************************************/
//! release_statement

/*! \breif  T_STATEMENT를 해제
 ************************************
 * \param sql(in/out)   :
 * \param id(in)        : 해제하려는 id
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  gClient의 T_SQL에 할당되어 있는 id에 해당하는 stmt를 해제 시킴
 *
 *****************************************************************************/
static void
release_statement(T_SQL * sql, unsigned int id)
{
    T_STATEMENT *stmt = sql->stmts;
    T_STATEMENT *tmp = NULL;
    int i;

    for (i = 0; i < sql->stmt_num; i++)
    {
        if (stmt->id == id)
            break;
        stmt = stmt->next;
    }


    if (i == sql->stmt_num)
        return; // do nothing

    if (i)
    {
        tmp = stmt;
        stmt->prev->next = stmt->next;
        stmt->next->prev = stmt->prev;
        tmp->parsing_memory = sql_mem_destroy_chunk(tmp->parsing_memory);
    }
    else
    {
        // 첫번째로 올때..
        if (sql->stmt_num > 2)
        {
            tmp = stmt->next;
            stmt->next->prev = stmt->prev;
            stmt->prev->next = sql->stmts;
            stmt->next->next->prev = sql->stmts;
            sql->stmts->parsing_memory =
                    sql_mem_destroy_chunk(sql->stmts->parsing_memory);
            sc_memcpy(sql->stmts, stmt->next, sizeof(T_STATEMENT));
        }
        else if (sql->stmt_num == 2)
        {
            tmp = stmt->next;
            stmt->next->prev = stmt;
            stmt->next->next = stmt;
            sql->stmts->parsing_memory =
                    sql_mem_destroy_chunk(sql->stmts->parsing_memory);
            sc_memcpy(sql->stmts, stmt->next, sizeof(T_STATEMENT));
        }
        else
            tmp = NULL;
    }

    PMEM_FREENUL(tmp);
    sql->stmt_num--;    // 항상 최소는 0으로 셋팅
}

/*****************************************************************************/
//! release_all_statements

/*! \breif  존재하는 모든 T_STATEMENT를 해제
 ************************************
 * \param   sql(in/out):
 ************************************
 * \return  void
 ************************************
 * \note
 *  DISCONNECT/LOGOUT 시 사용된다.
 *  - 위 태크 내용과 동일함
 *
 *****************************************************************************/
void
release_all_statements(T_SQL * sql)
{
    T_STATEMENT *stmt = sql->stmts->next;
    T_STATEMENT *tmp = NULL;
    int i;

    for (i = 1; i < sql->stmt_num; i++)
    {
        tmp = stmt;
        stmt = stmt->next;
        if (tmp)
        {
            if (tmp->is_main_stmt)      // stmt가 main인 경우 true.. sub query에서는 false
                tmp->parsing_memory =
                        sql_mem_destroy_chunk(tmp->parsing_memory);
            else
                tmp->parsing_memory = NULL;
            PMEM_FREENUL(tmp);
        }
    }

    sql->stmt_num = 0;
}

/*****************************************************************************/
//! unpack_field_list

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)    :
 * \param sql(in/out)   :
 * \param table(in)     :
 * \param column(in)    :
 * \param fields(in/out):
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - T_VALUEDESC의 멤버 변수 valuetype 제거
 *****************************************************************************/
static int
unpack_field_list(int handle, char *table, char *column, iSQL_FIELD ** fields)
{
    SYSFIELD_T *tmp_fieldinfo, *fieldinfo;

    int i, ret = RET_ERROR;
    int fieldnum, pos;

    fieldnum = dbi_Schema_GetFieldsInfo(handle, table, &fieldinfo);
    if (fieldnum < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, fieldnum, 0);
        return RET_ERROR;
    }

    if (column && column[0] != '\0')
    {
        T_VALUEDESC rc, left, right;

        sc_memset(&left, 0, sizeof(T_VALUEDESC));
        sc_memset(&right, 0, sizeof(T_VALUEDESC));

        tmp_fieldinfo =
                (SYSFIELD_T *) PMEM_ALLOC(sizeof(SYSFIELD_T) * fieldnum);
        if (!tmp_fieldinfo)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            goto RETURN_LABEL;
        }

        right.valuetype = DT_VARCHAR;
        right.attrdesc.collation = DB_Params.default_col_char;
        right.u.ptr = column;

        left.valuetype = DT_VARCHAR;
        left.attrdesc.collation = DB_Params.default_col_char;
        left.u.ptr = (char *) PMEM_ALLOC(sizeof(char) * 33);
        if (!left.u.ptr)
        {
            PMEM_FREENUL(tmp_fieldinfo);
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            goto RETURN_LABEL;
        }

        for (i = 0, pos = 0; i < fieldnum; i++)
        {
            sc_strcpy(left.u.ptr, fieldinfo[i].fieldName);
            if (calc_like(&left, &right, NULL, &rc, LIKE, 0) < 1)
            {
                PMEM_FREENUL(tmp_fieldinfo);
                PMEM_FREENUL(left.u.ptr);
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                goto RETURN_LABEL;
            }
            if (rc.u.bl)
                sc_memcpy(&tmp_fieldinfo[pos++],
                        &fieldinfo[i], sizeof(SYSFIELD_T));
        }
        PMEM_FREENUL(left.u.ptr);
        PMEM_FREENUL(fieldinfo);

        fieldnum = pos;
        fieldinfo = tmp_fieldinfo;
    }

    if (fieldnum < 1)
    {
        ret = fieldnum;
        goto RETURN_LABEL;
    }

    (*fields) = (iSQL_FIELD *) PMEM_ALLOC(sizeof(iSQL_FIELD) * fieldnum);
    if (!(*fields))
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto RETURN_LABEL;
    }

    for (i = 0; i < fieldnum; i++)
    {
        sc_memcpy((*fields)[i].name, fieldinfo[i].fieldName,
                sc_strlen(fieldinfo[i].fieldName));
        (*fields)[i].name[sc_strlen(fieldinfo[i].fieldName) + 1] = '\0';
        sc_memcpy((*fields)[i].table, table, sc_strlen(table));
        (*fields)[i].table[sc_strlen(table) + 1] = '\0';
        sc_memcpy((*fields)[i].def, fieldinfo[i].defaultValue,
                DEFAULT_VALUE_LEN);

        (*fields)[i].def[DEFAULT_VALUE_LEN - 1] = '\0';
        (*fields)[i].type = fieldinfo[i].type;
        (*fields)[i].length = fieldinfo[i].length;
        (*fields)[i].max_length = fieldinfo[i].length;
        (*fields)[i].flags = fieldinfo[i].flag;
        (*fields)[i].decimals = 0;
    }

    ret = fieldnum;

  RETURN_LABEL:
    PMEM_FREENUL(fieldinfo);

    if (ret == RET_ERROR)
    {
        if (*fields)
        {
            PMEM_FREENUL(*fields);
            *fields = NULL;
        }
    }

    return ret;
}

//#define ASSIGN(A)    ((A)?((int)(A)-1)/4*4+4:0)
#define ASSIGN(A)    ((A)?((((int)(A)-1)>>2)<<2)+4:0)

/*****************************************************************************/
//! parameter_value

/*! \breif convert param to val
 ************************************
 * \param param(in)     : parameter value
 * \param val(in/out)   : value
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - called : parameter_value_in_expression(), parameter_value_in_key()
 *             parameter_value_in_merge_items()
 *  - BYTE/VARBYTE TYPE 지원
 *      FIELD VALUE의 길이만큼만 복사가 이루어지도록 수정
 *  - constant인 경우 collation을 default로 설정한다
 *****************************************************************************/
static void
parameter_value(t_parameter * param, T_VALUEDESC * val)
{
    iSQL_TIME *l_time;

    if (val->valueclass == SVC_CONSTANT)
    {
        if (IS_BS(param->buffer_type))
            val->attrdesc.collation = DB_Params.default_col_char;
        else if (IS_NBS(param->buffer_type))
            val->attrdesc.collation = DB_Params.default_col_nchar;
        else if (IS_BYTE(param->buffer_type))
            val->attrdesc.collation = MDB_COL_DEFAULT_BYTE;
        else
            val->attrdesc.collation = MDB_COL_NUMERIC;
    }

    if (param->is_null)
    {
        val->valuetype = DT_NULL_TYPE;
        sql_value_ptrFree(val);
        sc_memset(&val->u, 0, sizeof(val->u));
        val->is_null = 1;
    }
    else
    {
        val->is_null = 0;
        val->valuetype = param->buffer_type;
        switch (param->buffer_type)
        {
        case SQL_DATA_TINYINT:
            val->u.i8 = *(tinyint *) param->buffer;
            break;
        case SQL_DATA_SMALLINT:
            val->u.i16 = *(smallint *) param->buffer;
            break;
        case SQL_DATA_INT:
            val->u.i32 = *(int *) param->buffer;
            break;
        case SQL_DATA_BIGINT:
            val->u.i64 = *(bigint *) param->buffer;
            break;
        case SQL_DATA_OID:
            val->u.oid = *(OID *) param->buffer;
            break;
        case SQL_DATA_FLOAT:
            val->u.f16 = *(float *) param->buffer;
            break;
        case SQL_DATA_DOUBLE:
            val->u.f32 = *(double *) param->buffer;
            break;
        case SQL_DATA_DECIMAL:
            val->u.dec = *(decimal *) param->buffer;
            break;
        case SQL_DATA_CHAR:
        case SQL_DATA_NCHAR:
        case SQL_DATA_VARCHAR:
        case SQL_DATA_NVARCHAR:
        case SQL_DATA_BYTE:
        case SQL_DATA_VARBYTE:
            if (val->attrdesc.len < param->buffer_length || !val->u.ptr)
            {
                sql_value_ptrFree(val);
                if (sql_value_ptrXcalloc(val, param->buffer_length + 1) < 0)
                {
                    return;
                }
            }

            if (IS_NBS(val->valuetype))
                sc_memset(val->u.ptr, 0, param->buffer_length * WCHAR_SIZE);
            else
                sc_memset(val->u.ptr, 0, param->buffer_length);

            // set field's length
            val->attrdesc.len = param->buffer_length;
            // set field's value length
            val->value_len = param->value_length;
            memcpy_func[param->buffer_type] (val->u.ptr, param->buffer,
                    val->value_len);
            break;

        case SQL_DATA_DATETIME:
            val->valuetype = DT_VARCHAR;
            val->attrdesc.collation = DB_Params.default_col_char;
            if (val->call_free == 0 && sql_value_ptrAlloc(val, 32) < 0)
                return;
            l_time = (iSQL_TIME *) param->buffer;
            val->value_len = sc_snprintf(val->u.ptr,
                    32, "%hu-%hu-%hu %hu:%hu:%hu",
                    l_time->year, l_time->month, l_time->day,
                    l_time->hour, l_time->minute, l_time->second);
            break;
        case SQL_DATA_DATE:
            val->valuetype = DT_VARCHAR;
            val->attrdesc.collation = DB_Params.default_col_char;
            if (val->call_free == 0 && sql_value_ptrAlloc(val, 16) < 0)
                return;
            l_time = (iSQL_TIME *) param->buffer;
            val->value_len = sc_snprintf(val->u.ptr,
                    16, "%hu-%hu-%hu", l_time->year, l_time->month,
                    l_time->day);
            break;
        case SQL_DATA_TIME:
            val->valuetype = DT_VARCHAR;
            val->attrdesc.collation = DB_Params.default_col_char;
            if (val->call_free == 0 && sql_value_ptrAlloc(val, 16) < 0)
                return;
            l_time = (iSQL_TIME *) param->buffer;
            val->value_len = sc_snprintf(val->u.ptr,
                    16, "%hu:%hu:%hu",
                    l_time->hour, l_time->minute, l_time->second);
            break;
        case SQL_DATA_TIMESTAMP:
            val->valuetype = DT_VARCHAR;
            val->attrdesc.collation = DB_Params.default_col_char;
            if (val->call_free == 0 && sql_value_ptrAlloc(val, 32) < 0)
                return;

            l_time = (iSQL_TIME *) param->buffer;
            val->value_len = sc_snprintf(val->u.ptr,
                    32, "%hu-%hu-%hu %hu:%hu:%hu.%hu",
                    l_time->year, l_time->month, l_time->day,
                    l_time->hour, l_time->minute, l_time->second,
                    l_time->fraction);
            break;
        default:
            MDB_SYSLOG(("ERROR:Unknown buffer type %d for parameter_value\n",
                            param->buffer_type));
            break;
        }
    }
}

/*****************************************************************************/
//! parameter_value_in_expression

/*! \breif  params의 값을 expr에 할당한다
 ************************************
 * \param params(in)    :
 * \param expr(in/out)  :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - called : set_parameter_values()
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static void
parameter_value_in_expression(T_DBMS_CS_EXECUTE * params,
        T_EXPRESSIONDESC * expr)
{
    T_POSTFIXELEM *elem;
    t_parameter *param;
    int i, j, posi;

    for (i = 0; i < expr->len; i++)
    {
        elem = expr->list[i];

        if (elem->elemtype == SPT_OPERATOR)
            PMEM_FREENUL(elem->u.op.likeStr);

        if (elem->elemtype != SPT_VALUE)
            continue;
        if (elem->u.value.valueclass != SVC_CONSTANT)
            continue;
        if (!elem->u.value.param_idx)
            continue;

        for (j = 0, posi = 0; j < elem->u.value.param_idx - 1; j++)
        {
            param = (t_parameter *) ((char *) params->params + posi);
            posi += sizeof(t_parameter) - sizeof(((t_parameter *) 0)->buffer) +
                    ASSIGN(DB_BYTE_LENG(param->buffer_type,
                            param->buffer_length));
        }
        param = (t_parameter *) ((char *) params->params + posi);
        parameter_value(param, &elem->u.value);
    }
}



/*****************************************************************************/
//! get_buffer_length

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param type(in)          : data's type
 * \param max_length(in)    : date's max length
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - called : desc_parameter4column()
 *  - date(?)
 *  - BYTE/VARBYTE TYPE 지원
 *****************************************************************************/
static unsigned int
get_buffer_length(DataType type, int max_length)
{
    switch (type)
    {
    case DT_TINYINT:
    case DT_SMALLINT:
    case DT_INTEGER:
    case DT_BIGINT:
    case DT_OID:
    case DT_FLOAT:
    case DT_DOUBLE:
    case DT_DECIMAL:
        return SizeOfType[type];
    case DT_CHAR:
    case DT_NCHAR:
    case DT_VARCHAR:
    case DT_NVARCHAR:
    case DT_BYTE:
    case DT_VARBYTE:
        return max_length;
    case DT_DATETIME:
    case DT_DATE:
    case DT_TIME:
    case DT_TIMESTAMP:
        return sizeof(iSQL_TIME);
    default:
        return 0;
    }
    return 0;
}

/*****************************************************************************/
//! arg_type_of_func

/*! \breif  DYNIMIC SQL에서 SQL FUNCTION 인자의 정보를 할당할때 사용함
 ************************************
 * \param func(in):
 * \param funcs_arg_idx(in) : 몇번째 파라미터
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 * - called : desc_parameter4expression()
 * - reference(see also) : result_elem_idx_of_func()
 *  - SFT_DATE_SUB 추가
 *  - 지원하지 function은 error로 처리한다.
 *
 *****************************************************************************/
static int
arg_type_of_func(T_FUNCDESC * func, int func_arg_idx, int *arg_length)
{
    switch (func->type)
    {
    case SFT_DATE_ADD:
    case SFT_DATE_SUB:
        if (func_arg_idx == 0)
            return SQL_DATA_VARCHAR;
        else if (func_arg_idx == 1)
            return SQL_DATA_INT;
        else if (func_arg_idx == 2)
            return SQL_DATA_TIMESTAMP;
        else
        {
#ifdef MDB_DEBUG
            sc_assert(0, __FILE__, __LINE__);
#endif
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTYPE, 0);
            return SQL_DATA_NONE;
        }

    case SFT_DATE_DIFF:
        if (func_arg_idx == 0)
            return SQL_DATA_INT;
        else if (func_arg_idx == 1)
            return SQL_DATA_TIMESTAMP;
        else if (func_arg_idx == 2)
            return SQL_DATA_TIMESTAMP;
        else
        {
#ifdef MDB_DEBUG
            sc_assert(0, __FILE__, __LINE__);
#endif
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTYPE, 0);
            return SQL_DATA_NONE;
        }

    case SFT_TIME_ADD:
    case SFT_TIME_SUB:
        if (func_arg_idx == 0)
            return SQL_DATA_VARCHAR;
        else if (func_arg_idx == 1)
            return SQL_DATA_INT;
        else if (func_arg_idx == 2)
            return SQL_DATA_TIMESTAMP;
        else
        {
#ifdef MDB_DEBUG
            sc_assert(0, __FILE__, __LINE__);
#endif
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTYPE, 0);
            return SQL_DATA_NONE;
        }

    case SFT_TIME_DIFF:
        if (func_arg_idx == 0)
            return SQL_DATA_INT;
        else if (func_arg_idx == 1)
            return SQL_DATA_TIME;
        else if (func_arg_idx == 2)
            return SQL_DATA_TIME;
        else
        {
#ifdef MDB_DEBUG
            sc_assert(0, __FILE__, __LINE__);
#endif
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTYPE, 0);
            return SQL_DATA_NONE;
        }
        /* calc_func_time_diff(); */

    case SFT_COPYFROM:
        if (func_arg_idx == 0)
        {
            if (arg_length)
                *arg_length = MDB_FILE_NAME;

            return SQL_DATA_VARCHAR;
        }
        else
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTYPE, 0);
            return SQL_DATA_NONE;
        }
        break;

    case SFT_COPYTO:
        if (func_arg_idx == 1)
        {
            if (arg_length)
                *arg_length = MDB_FILE_NAME;

            return SQL_DATA_VARCHAR;
        }
        else
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTYPE, 0);
            return SQL_DATA_NONE;
        }
        break;
    default:
        {
            return SQL_DATA_NONE;
        }
    }

    return -1;
}


/*****************************************************************************/
//! desc_parameter4expression

/*! \breif  DYNAMIC API에서 parameter의 field 정보를 할당해주는 부분
 ************************************
 * \param params(in/out)    :
 * \param expr(in/out)      :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static void
desc_parameter4expression(t_fieldinfo * params, T_EXPRESSIONDESC * expr)
{
    T_POSTFIXELEM *elem, *res, **args;
    T_ATTRDESC *a;
    t_fieldinfo *p;
    t_stack_template *stack;
    int i, j;
    int func_arg_idx;

    T_POSTFIXELEM result_org;
    T_POSTFIXELEM *result = NULL;
    isql_data_type arg_type = SQL_DATA_NONE;
    int op_check = 0;

    result = &result_org;

    sc_memset(result, 0, sizeof(T_POSTFIXELEM));

    init_stack_template(&stack);
    args = NULL;

    for (i = 0; i < expr->len; i++)
    {
        op_check = 0;
        elem = expr->list[i];
        switch (elem->elemtype)
        {
        case SPT_VALUE:
        case SPT_SUBQUERY:
            push_stack_template(elem, &stack);
            break;

        case SPT_OPERATOR:
            if (elem->u.op.type == SOT_MERGE ||
                    elem->u.op.type == SOT_AND ||
                    elem->u.op.type == SOT_OR ||
                    elem->u.op.type == SOT_NOT ||
                    elem->u.op.type == SOT_PLUS ||
                    elem->u.op.type == SOT_MINUS ||
                    elem->u.op.type == SOT_TIMES ||
                    elem->u.op.type == SOT_DIVIDE ||
                    elem->u.op.type == SOT_EXPONENT ||
                    elem->u.op.type == SOT_MODULA)
                op_check = 1;
        case SPT_SUBQ_OP:
            args = sql_mem_get_free_postfixelem_list(NULL, elem->u.op.argcnt);
            if (args == NULL)
                break;
            for (j = 0, res = NULL; j < elem->u.op.argcnt; j++)
            {
                args[j] = pop_stack_template(&stack);
                if (args[j] && !args[j]->u.value.param_idx && !op_check)
                    res = args[j];
            }

            op_check = 0;
            for (j = 0; j < elem->u.op.argcnt; j++)
            {
                if (args[j] && args[j]->u.value.param_idx)
                {
                    p = &params[args[j]->u.value.param_idx - 1];
                    if (res == NULL)
                    {
                        op_check = 1;
                        push_stack_template(args[j], &stack);
                        continue;
                    }
                    if ((elem->u.op.type == SOT_LIKE
                                    || elem->u.op.type == SOT_ILIKE)
                            && !IS_ALLOCATED_TYPE(res->u.value.valuetype))
                    {
                        a = &res->u.value.attrdesc;
                        p->type = DT_CHAR;
                        p->length = CSizeOfType[a->type] * 2;
                        p->max_length = p->length;
                        p->buffer_length =
                                get_buffer_length(p->type, p->length);
                        p->flags = a->flag;
                    }
                    else if (res->u.value.valueclass == SVC_CONSTANT)
                    {
                        p->type = res->u.value.valuetype;
                        p->buffer_length = get_buffer_length(p->type,
                                res->u.value.attrdesc.len);
                        p->flags = FD_NULLABLE;
                    }
                    else        /* res->u.value.valueclass == SVC_VARIABLE */
                    {
                        a = &res->u.value.attrdesc;

                        p->type = res->u.value.valuetype;
                        p->length = a->len;
                        p->max_length = p->length;

                        p->buffer_length =
                                get_buffer_length(p->type, p->length);

                        p->flags = a->flag;
                        p->decimals = a->dec;
                    }
                }
            }

            if (res == NULL)
            {
                res = args[0];
            }

            if (!op_check)
            {
                push_stack_template(res, &stack);
            }

            PMEM_FREENUL(args);
            break;

        case SPT_FUNCTION:
            if (elem->u.func.argcnt)
            {
                args = sql_mem_get_free_postfixelem_list(NULL,
                        elem->u.func.argcnt);
                if (args == NULL)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_OUTOFMEMORY, 0);
                    break;
                }
            }

            arg_type = SQL_DATA_NONE;

            for (func_arg_idx = elem->u.func.argcnt - 1;
                    func_arg_idx >= 0; func_arg_idx--)
            {
                args[func_arg_idx] = pop_stack_template(&stack);

                if (args[func_arg_idx]->u.value.param_idx)
                {
                    p = &params[args[func_arg_idx]->u.value.param_idx - 1];
                    arg_type = arg_type_of_func(&elem->u.func,
                            func_arg_idx, (int *) (&p->length));
                    if (arg_type == SQL_DATA_NONE)
                        break;

                    p->type = arg_type;
                    p->buffer_length = get_buffer_length(p->type, p->length);
                    p->flags = FD_NULLABLE;

                }
            }

            if (arg_type == SQL_DATA_NONE && func_arg_idx != -1)
            {
                PMEM_FREENUL(args);
                break;
            }

            if (calc_function(&elem->u.func, result, args, 1) < 0)
            {
                PMEM_FREENUL(args);
                break;
            }

            result->u.value.attrdesc.len += 2;

            push_stack_template(result, &stack);

            PMEM_FREENUL(args);
            break;

        default:
            break;
        }
    }

    /* remove the check of stack & pop_stack_template */

    remove_stack_template(&stack);
}

/*****************************************************************************/
//! desc_parameter4column

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param params(in/out)  :
 * \param expr()          :
 * \param column(in)      :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - called : desc_parameters()
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static void
desc_parameter4column(t_fieldinfo * params,
        T_EXPRESSIONDESC * expr, SYSFIELD_T * column)
{
    T_POSTFIXELEM *elem, **args = NULL;
    t_fieldinfo *p;
    t_stack_template *stack;
    DataType arg_type;
    T_POSTFIXELEM result_org;
    T_POSTFIXELEM *result;
    int i, j;

    result = &result_org;

    init_stack_template(&stack);

    for (i = 0; i < expr->len; i++)
    {
        elem = expr->list[i];
        switch (elem->elemtype)
        {
        case SPT_VALUE:
            if (elem->u.value.param_idx)
            {
                p = &params[elem->u.value.param_idx - 1];

                p->type = column->type;
                p->length = column->length;
                p->max_length = p->length;
                p->buffer_length = get_buffer_length(p->type, p->length);
                p->flags = column->flag;
                p->decimals = column->scale;
            }
        case SPT_SUBQUERY:
            push_stack_template(elem, &stack);
            break;
        case SPT_OPERATOR:
        case SPT_SUBQ_OP:
            for (j = 0; j < elem->u.op.argcnt - 1; j++)
                pop_stack_template(&stack);

            push_stack_template(pop_stack_template(&stack), &stack);
            break;
        case SPT_FUNCTION:
            if (elem->u.func.argcnt)
            {
                args = sql_mem_get_free_postfixelem_list(NULL,
                        elem->u.func.argcnt);
                if (args == NULL)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_OUTOFMEMORY, 0);
                    goto RETURN_LABEL;
                }
            }
            else
                args = NULL;

            for (j = elem->u.func.argcnt - 1; j > -1; --j)
            {
                args[j] = pop_stack_template(&stack);
                if (args[j]->u.value.param_idx)
                {
                    p = &params[args[j]->u.value.param_idx - 1];

                    arg_type = arg_type_of_func(&elem->u.func,
                            j, (int *) (&p->length));
                    if (arg_type == DT_NULL_TYPE)
                        continue;

                    p->type = arg_type;
                    p->length = p->max_length = p->buffer_length =
                            get_buffer_length(p->type, p->length);
                    p->flags = FD_NULLABLE;
                    p->decimals = 0;
                }
            }

            if (calc_function(&elem->u.func, result, args, 1) < 0)
            {
                PMEM_FREENUL(args);
                goto RETURN_LABEL;
            }

            result->u.value.attrdesc.len += 2;
            push_stack_template(result, &stack);
            PMEM_FREENUL(args);
            break;
        default:
            break;
        }
    }

  RETURN_LABEL:

    remove_stack_template(&stack);
}

/*****************************************************************************/
//! parameter_value4orderby

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param params()      :
 * \param param_idx()   :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
parameter_value4orderby(T_DBMS_CS_EXECUTE * params, int param_idx)
{
    T_POSTFIXELEM elem_org;
    T_POSTFIXELEM *elem;
    t_parameter *param;
    int j, posi = 0;

    elem = &elem_org;

    for (j = 0, posi = 0; j < param_idx - 1; j++)
    {
        param = (t_parameter *) ((char *) params->params + posi);

        posi += sizeof(t_parameter) - sizeof(((t_parameter *) 0)->buffer) +
                ASSIGN(DB_BYTE_LENG(param->buffer_type, param->buffer_length));

    }

    elem->u.value.valuetype = DT_INTEGER;
    elem->u.value.attrdesc.collation = MDB_COL_NUMERIC;
    param = (t_parameter *) ((char *) params->params + posi);

    if (param->buffer_type == SQL_DATA_INT)
    {
        parameter_value(param, &elem->u.value);
    }
    else
    {
        T_VALUEDESC value;

        sc_memset(&value, 0, sizeof(T_VALUEDESC));
        value.valuetype = (DataType) param->buffer_type;

        parameter_value(param, &value);

        if (calc_func_convert(&elem->u.value, &value, &elem->u.value,
                        MDB_FALSE) < 0)
        {
            sql_value_ptrFree(&value);
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTYPE, 0);
            return -2;
        }
        sql_value_ptrFree(&value);
    }


    if (elem->u.value.u.i32 < 1)
        j = -1;
    else
        j = elem->u.value.u.i32 - 1;

    return j;
}

/*****************************************************************************/
//! parameter_value4limit

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param params()      :
 * \param param_idx()   :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
parameter_value4limit(T_DBMS_CS_EXECUTE * params, int param_idx)
{
    T_POSTFIXELEM elem_org;
    T_POSTFIXELEM *elem;
    t_parameter *param;
    int j, posi = 0;

    elem = &elem_org;

    for (j = 0, posi = 0; j < param_idx - 1; j++)
    {
        param = (t_parameter *) ((char *) params->params + posi);

        posi += sizeof(t_parameter) - sizeof(((t_parameter *) 0)->buffer) +
                ASSIGN(DB_BYTE_LENG(param->buffer_type, param->buffer_length));

    }

    elem->u.value.valuetype = DT_INTEGER;
    elem->u.value.attrdesc.collation = MDB_COL_NUMERIC;
    param = (t_parameter *) ((char *) params->params + posi);

    if (param->buffer_type == SQL_DATA_INT)
    {
        parameter_value(param, &elem->u.value);
    }
    else
    {
        T_VALUEDESC value;

        sc_memset(&value, 0, sizeof(T_VALUEDESC));
        value.valuetype = (DataType) param->buffer_type;

        parameter_value(param, &value);

        if (calc_func_convert(&elem->u.value, &value, &elem->u.value,
                        MDB_FALSE) < 0)
        {
            sql_value_ptrFree(&value);
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTYPE, 0);
            return -2;
        }
        sql_value_ptrFree(&value);
    }


    if (elem->u.value.u.i32 < -1)
        j = -1;
    else
        j = elem->u.value.u.i32;

    return j;
}

/*****************************************************************************/
//! parameter_rid4limit

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param params(in/out)    :
 * \param param_idx(in)     :
 * \param startat_rid(in)   :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *****************************************************************************/
static int
parameter_rid4limit(T_DBMS_CS_EXECUTE * params, int param_idx,
        OID * startat_rid)
{
    T_POSTFIXELEM *elem;
    t_parameter *param;

    int j, posi = 0;

    elem = (T_POSTFIXELEM *) PMEM_ALLOC(sizeof(T_POSTFIXELEM));
    if (elem == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return SQL_E_OUTOFMEMORY;
    }

    for (j = 0, posi = 0; j < param_idx - 1; j++)
    {
        param = (t_parameter *) ((char *) params->params + posi);

        posi += sizeof(t_parameter) - sizeof(((t_parameter *) 0)->buffer) +
                ASSIGN(DB_BYTE_LENG((DataType) param->buffer_type,
                        param->buffer_length));

    }

    elem->u.value.valuetype = DT_OID;
    elem->u.value.attrdesc.collation = MDB_COL_NUMERIC;
    param = (t_parameter *) ((char *) params->params + posi);

    if (param->buffer_type == SQL_DATA_OID)
    {
        parameter_value(param, &elem->u.value);
    }
    else
    {
        T_VALUEDESC value;

        value.valuetype = (DataType) param->buffer_type;
        value.call_free = 0;

        parameter_value(param, &value);

        if (calc_func_convert(&elem->u.value, &value, &elem->u.value,
                        MDB_FALSE) < 0)
        {
            sql_value_ptrFree(&value);
            PMEM_FREE(elem);
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTYPE, 0);
            return SQL_E_INVALIDTYPE;
        }
        sql_value_ptrFree(&value);
    }

    *startat_rid = elem->u.value.u.oid;

    PMEM_FREE(elem);

    return 0;
}

/*****************************************************************************/
//! desc_parameter4orderby

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param params()      :
 * \param param_idx()   :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static void
desc_parameter4orderby(t_fieldinfo * params, int param_idx, DataType type)
{
    t_fieldinfo *p;

    p = &params[param_idx - 1];

    p->type = type;
    p->length = p->max_length = 4;
    p->buffer_length = get_buffer_length((DataType) p->type, p->length);
    p->flags = 0;
    p->decimals = 0;
}

/*****************************************************************************/
//! desc_parameter4limit

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param params()      :
 * \param param_idx()   :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static void
desc_parameter4limit(t_fieldinfo * params, int param_idx, DataType type)
{
    t_fieldinfo *p;

    p = &params[param_idx - 1];

    p->type = type;
    p->length = p->max_length = 4;
    p->buffer_length = get_buffer_length((DataType) p->type, p->length);
    p->flags = 0;
    p->decimals = 0;
}

/*****************************************************************************/
//! parameter_value_in_key

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param params()      :
 * \param nest()        :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
static void
parameter_value_in_key(T_DBMS_CS_EXECUTE * params, T_NESTING * nest)
{
    T_POSTFIXELEM *elem;
    t_parameter *param;

    int i, j, k, posi;

    if (nest->min)
        for (k = 0; k < nest->index_field_num; k++)
        {
            for (i = 0; i < nest->min[k].expr.len; i++)
            {
                elem = nest->min[k].expr.list[i];
                if (elem->elemtype != SPT_VALUE)
                    continue;
                if (elem->u.value.valueclass != SVC_CONSTANT)
                    continue;
                if (!elem->u.value.param_idx)
                    continue;

                for (j = 0, posi = 0; j < elem->u.value.param_idx - 1; j++)
                {
                    param = (t_parameter *) ((char *) params->params + posi);
                    posi += sizeof(t_parameter) -
                            sizeof(((t_parameter *) 0)->buffer) +
                            ASSIGN(DB_BYTE_LENG(param->buffer_type,
                                    param->buffer_length));
                }
                param = (t_parameter *) ((char *) params->params + posi);
                parameter_value(param, &elem->u.value);
            }
        }

    if (nest->max)
        for (k = 0; k < nest->index_field_num; k++)
        {
            for (i = 0; i < nest->max[k].expr.len; i++)
            {
                elem = nest->max[k].expr.list[i];
                if (elem->elemtype != SPT_VALUE)
                    continue;
                if (elem->u.value.valueclass != SVC_CONSTANT)
                    continue;
                if (!elem->u.value.param_idx)
                    continue;

                for (j = 0, posi = 0; j < elem->u.value.param_idx - 1; j++)
                {
                    param = (t_parameter *) ((char *) params->params + posi);
                    posi += sizeof(t_parameter) -
                            sizeof(((t_parameter *) 0)->buffer) +
                            ASSIGN(DB_BYTE_LENG(param->buffer_type,
                                    param->buffer_length));
                }
                param = (t_parameter *) ((char *) params->params + posi);
                parameter_value(param, &elem->u.value);
            }
        }

    /* ENHANCEMENT
     * filter는 어떻게 하지?? 아직은 filter를 쓰고 있지 않으니깐
     * filter를 사용할때 생각하자..
     */
}

/*****************************************************************************/
//! parameter_value_in_merge_items

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param params():
 * \param rttable:
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static void
parameter_value_in_merge_items(T_DBMS_CS_EXECUTE * params, T_RTTABLE * rttable)
{
    T_VALUEDESC *val;
    t_parameter *param;
    int i, j, posi;

    for (i = 0; i < rttable->cnt_merge_item; i++)
    {
        val = &rttable->merge_key_values[i];
        if (val->valueclass != SVC_CONSTANT)
            continue;
        if (!val->param_idx)
            continue;

        for (j = 0, posi = 0; j < val->param_idx - 1; j++)
        {
            param = (t_parameter *) ((char *) params->params + posi);
            posi += sizeof(t_parameter) - sizeof(((t_parameter *) 0)->buffer) +
                    ASSIGN(DB_BYTE_LENG(param->buffer_type,
                            param->buffer_length));
        }
        param = (t_parameter *) ((char *) params->params + posi);
        parameter_value(param, val);
    }
}

/*****************************************************************************/
//! set_param_values4select

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param select(in/out)    :
 * \param params(in)        :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - called : set_parameter_values()
 *****************************************************************************/
static MDB_INT32
set_param_values4select(T_SELECT * select, T_DBMS_CS_EXECUTE * params)
{
    EXPRDESC_LIST *cur;
    int i;

    T_NESTING *nest;
    T_RTTABLE *rttable;
    T_QUERYDESC *qdesc;
    T_LIST_JOINTABLEDESC *fromlist;

    T_LIMITDESC *limitdesc = NULL;

    if (select->first_sub)
    {
        if (set_param_values4select(select->first_sub, params) == RET_ERROR)
            return RET_ERROR;
    }

    if (select->sibling_query)
    {
        if (set_param_values4select(select->sibling_query,
                        params) == RET_ERROR)
            return RET_ERROR;
    }

    if (select->tmp_sub)
    {
        if (set_param_values4select(select->tmp_sub, params) == RET_ERROR)
            return RET_ERROR;
    }

    nest = select->planquerydesc.nest;
    qdesc = &select->planquerydesc.querydesc;

    for (i = 0; i < qdesc->setlist.len; i++)
    {
        if (qdesc->setlist.list[i]->elemtype == SPT_SUBQUERY)
        {
            if (set_param_values4select(qdesc->setlist.list[i]->u.subq, params)
                    == RET_ERROR)
            {
                return RET_ERROR;
            }
        }
    }

    fromlist = &qdesc->fromlist;
    rttable = select->rttable;

    for (i = 0; i < qdesc->selection.len; i++)
    {
        parameter_value_in_expression(params, &qdesc->selection.list[i].expr);
    }

    for (i = 0; i < fromlist->len; i++)
    {
        for (cur = fromlist->list[i].expr_list; cur; cur = cur->next)
            parameter_value_in_expression(params, cur->ptr);
    }

    if (qdesc->condition.list && qdesc->expr_list == NULL)
    {
        parameter_value_in_expression(params, &qdesc->condition);
    }
    else
    {
        if (fromlist->outer_join_exists && qdesc->condition.list)
            parameter_value_in_expression(params, &qdesc->condition);
        for (cur = qdesc->expr_list; cur; cur = cur->next)
        {
            parameter_value_in_expression(params, cur->ptr);

            if (nest[0].is_param_scanrid == 1)
            {
                T_EXPRESSIONDESC *expr;
                T_POSTFIXELEM *elem;

                expr = (T_EXPRESSIONDESC *) cur->ptr;
                for (i = 0; i < expr->len; i++)
                {
                    elem = expr->list[i];
                    if (elem->u.value.valueclass == SVC_CONSTANT
                            && elem->u.value.valuetype == DT_OID)
                        nest[0].scanrid = elem->u.value.u.oid;
                }
                parameter_value_in_expression(params, &qdesc->condition);
            }
        }
    }

    for (i = 0; i < fromlist->len; i++)
    {
        parameter_value_in_key(params, &nest[i]);
        parameter_value_in_merge_items(params, &rttable[i]);
    }

    parameter_value_in_expression(params, &qdesc->groupby.having);

    for (i = 0; i < qdesc->orderby.len; i++)
    {
        if (qdesc->orderby.list[i].param_idx > -1)
        {
            qdesc->orderby.list[i].s_ref =
                    parameter_value4orderby(params,
                    qdesc->orderby.list[i].param_idx);
            if (qdesc->orderby.list[i].s_ref < 0
                    || qdesc->selection.len <= qdesc->orderby.list[i].s_ref)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDITEMINDEX, 0);
                return RET_ERROR;
            }
        }
    }

    limitdesc = &qdesc->limit;

    if (limitdesc->offset_param_idx > -1)
    {
        limitdesc->offset =
                parameter_value4limit(params, limitdesc->offset_param_idx);

        if (limitdesc->offset == -2)
            return RET_ERROR;
    }

    if (limitdesc->rows_param_idx > -1)
    {
        limitdesc->rows =
                parameter_value4limit(params, limitdesc->rows_param_idx);

        if (limitdesc->rows == -2)
            return RET_ERROR;
    }

    if (limitdesc->startat_param_idx > -1)
    {
        if (parameter_rid4limit(params, limitdesc->startat_param_idx,
                        &limitdesc->start_at) < 0)
            return RET_ERROR;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! desc_parameter4select

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param select()  :
 * \param params()  :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static MDB_INT32
desc_parameter4select(T_SELECT * select, t_fieldinfo * params)
{
    EXPRDESC_LIST *cur;
    int i;

    T_QUERYDESC *qdesc;
    T_LIST_JOINTABLEDESC *fromlist;

    T_EXPRESSIONDESC *having = NULL;

    T_LIMITDESC *limitdesc = NULL;

    if (select->first_sub)
    {
        if (desc_parameter4select(select->first_sub, params) == RET_ERROR)
            return RET_ERROR;
    }

    if (select->sibling_query)
    {
        if (desc_parameter4select(select->sibling_query, params) == RET_ERROR)
            return RET_ERROR;
    }

    if (select->tmp_sub)
    {
        if (desc_parameter4select(select->tmp_sub, params) == RET_ERROR)
            return RET_ERROR;
    }

    qdesc = &select->planquerydesc.querydesc;
    fromlist = &qdesc->fromlist;

    for (i = 0; i < qdesc->setlist.len; i++)
    {
        if (qdesc->setlist.list[i]->elemtype == SPT_SUBQUERY)
        {
            if (desc_parameter4select(qdesc->setlist.list[i]->u.subq,
                            params) == RET_ERROR)
            {
                return RET_ERROR;
            }
        }
    }

    having = &qdesc->groupby.having;

    for (i = 0; i < qdesc->selection.len; i++)
    {
        desc_parameter4expression(params, &qdesc->selection.list[i].expr);
    }

    for (i = 0; i < fromlist->len; i++)
    {
        for (cur = fromlist->list[i].expr_list; cur; cur = cur->next)
            desc_parameter4expression(params, cur->ptr);
    }

    if (qdesc->condition.list && qdesc->expr_list == NULL)
    {
        desc_parameter4expression(params, &qdesc->condition);
    }
    else
    {
        for (cur = qdesc->expr_list; cur; cur = cur->next)
            desc_parameter4expression(params, cur->ptr);
    }

    desc_parameter4expression(params, having);

    for (i = 0; i < qdesc->orderby.len; i++)
    {
        if (qdesc->orderby.list[i].param_idx > -1)
        {
            desc_parameter4orderby(params, qdesc->orderby.list[i].param_idx,
                    DT_INTEGER);
        }
    }

    limitdesc = &qdesc->limit;

    if (limitdesc->offset_param_idx > -1)
    {
        desc_parameter4limit(params, limitdesc->offset_param_idx, DT_INTEGER);
    }

    if (limitdesc->rows_param_idx > -1)
    {
        desc_parameter4limit(params, limitdesc->rows_param_idx, DT_INTEGER);
    }

    if (limitdesc->startat_param_idx > -1)
    {
        desc_parameter4limit(params, limitdesc->startat_param_idx, DT_OID);
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//!  set_parameter_values
//
/*! \breif  EXECUTE시 client로부터 넘겨져온 parameter값을 stmt에 적절히 할당
 ************************************
 * \param stmt(in/out)  :
 * \param params(in)    :
 ************************************
 * \return  ret < 0 is ERROR
 ************************************
 * \note 내부 알고리즘\n
 *  SELECT된 RESULT SET을 이용하여 특정 TABLE에 INSERT시키는 경우
 *  사용되는 함수이다
 *
 *  - main stmt의 할당된 CHUNK 공간을 임시 stmt에서도 사용할 수 있도록
 *      공유하여 준다
 *
 *****************************************************************************/
static MDB_INT32
set_parameter_values(T_STATEMENT * stmt, T_DBMS_CS_EXECUTE * params)
{
    EXPRDESC_LIST *cur;
    int i;

    T_LIMITDESC *limitdesc = NULL;

    if (stmt->sqltype == ST_SELECT)
    {
        if (set_param_values4select(&stmt->u._select, params) == RET_ERROR)
            return RET_ERROR;
    }
    else if (stmt->sqltype == ST_UPSERT || stmt->sqltype == ST_INSERT)
    {
        if (stmt->u._insert.type == SIT_VALUES)
        {
            for (i = 0; i < stmt->u._insert.u.values.len; i++)
            {
                parameter_value_in_expression(params,
                        &stmt->u._insert.u.values.list[i]);
            }
        }
        else
        {
// SELECT 해서 INSERT 하는 경우 사용된다.
            T_STATEMENT *tmp_stmt;

            tmp_stmt = (T_STATEMENT *) PMEM_ALLOC(sizeof(T_STATEMENT));
            if (tmp_stmt == NULL)
                return RET_ERROR;

            tmp_stmt->sqltype = ST_SELECT;
            sc_memcpy(&tmp_stmt->u._select, &stmt->u._insert.u.query,
                    sizeof(T_SELECT));
            tmp_stmt->parsing_memory = stmt->parsing_memory;

            if (set_parameter_values(tmp_stmt, params) == RET_ERROR)
            {
                PMEM_FREE(tmp_stmt);
                return RET_ERROR;
            }

            sc_memcpy(&stmt->u._insert.u.query, &tmp_stmt->u._select,
                    sizeof(T_SELECT));
            PMEM_FREE(tmp_stmt);
        }

    }
    else if (stmt->sqltype == ST_UPDATE)
    {
        for (i = 0; i < stmt->u._update.values.len; i++)
        {
            parameter_value_in_expression(params,
                    &stmt->u._update.values.list[i]);
        }
        for (cur = stmt->u._update.planquerydesc.querydesc.expr_list; cur;
                cur = cur->next)
        {
            parameter_value_in_expression(params, cur->ptr);
        }
        parameter_value_in_key(params, stmt->u._update.planquerydesc.nest);

        limitdesc = &stmt->u._update.planquerydesc.querydesc.limit;

        if (stmt->u._update.subselect.first_sub &&
                set_param_values4select(&stmt->u._update.subselect,
                        params) == RET_ERROR)
            return RET_ERROR;
    }
    else if (stmt->sqltype == ST_DELETE)
    {
        for (cur = stmt->u._delete.planquerydesc.querydesc.expr_list; cur;
                cur = cur->next)
        {
            parameter_value_in_expression(params, cur->ptr);
        }
        parameter_value_in_key(params, stmt->u._delete.planquerydesc.nest);

        limitdesc = &stmt->u._delete.planquerydesc.querydesc.limit;

        if (stmt->u._delete.subselect.first_sub &&
                set_param_values4select(&stmt->u._delete.subselect,
                        params) == RET_ERROR)
            return RET_ERROR;
    }
    else if (stmt->sqltype == ST_CREATE
            && stmt->u._create.type == SCT_TABLE_QUERY)
    {
        T_STATEMENT *tmp_stmt;

        tmp_stmt = (T_STATEMENT *) PMEM_ALLOC(sizeof(T_STATEMENT));
        if (tmp_stmt == NULL)
            return RET_ERROR;

        tmp_stmt->sqltype = ST_SELECT;
        sc_memcpy(&tmp_stmt->u._select,
                &stmt->u._create.u.table_query.query, sizeof(T_SELECT));

        tmp_stmt->parsing_memory = stmt->parsing_memory;

        if (set_parameter_values(tmp_stmt, params) == RET_ERROR)
        {
            PMEM_FREE(tmp_stmt);
            return RET_ERROR;
        }

        sc_memcpy(&stmt->u._create.u.table_query.query,
                &tmp_stmt->u._select, sizeof(T_SELECT));

        limitdesc =
                &stmt->u._create.u.table_query.query.planquerydesc.querydesc.
                limit;

        PMEM_FREE(tmp_stmt);
    }

    if (limitdesc && limitdesc->offset_param_idx > -1)
    {
        limitdesc->offset =
                parameter_value4limit(params, limitdesc->offset_param_idx);
    }

    if (limitdesc && limitdesc->rows_param_idx > -1)
    {
        limitdesc->rows =
                parameter_value4limit(params, limitdesc->rows_param_idx);
    }

    params->param_count = 0;

    return RET_SUCCESS;
}

extern int check_numeric_bound(T_VALUEDESC * value, DataType type, int length);
extern int update_fieldvalue(struct UpdateValue *upd_value_p, int fldidx,
        SYSFIELD_T * field, T_VALUEDESC * fval);

/*****************************************************************************/
//! set_update_values

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

typedef struct
{
    OID rid;
    int numfields;
    t_parameter params[1];
} T_DESCUPDATERID;

static MDB_INT32
set_update_values(struct UpdateValue *upd_values, T_DESCUPDATERID * update_rid)
{
    SYSFIELD_T *field = NULL;
    SYSFIELD_T *fields_info = NULL;
    SYSTABLE_T table_info;

    T_VALUEDESC val, fval, fdesc;
    int i, fld_idx;
    int ret = RET_SUCCESS;
    t_parameter *params;
    char tablename[MAX_TABLE_NAME_LEN];

    /* local value initialization */
    val.call_free = 0;
    fval.call_free = 0;

    *upd_values = dbi_UpdateValue_Create(update_rid->numfields);

    params = update_rid->params;

    /* make fields_info */

    if (dbi_Rid_Tablename(-1, update_rid->rid, tablename) < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_NOTPERMITTED, 0);
        return RET_ERROR;
    }

    if (!mdb_strcmp(tablename, "systables"))
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_NOTPERMITTED, 0);
        return RET_ERROR;
    }

    ret = dbi_Schema_GetTableFieldsInfo(-1, tablename,
            &table_info, &fields_info);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        ret = RET_ERROR;
        goto RETURN_LABEL;
    }

    sc_memset(&val, 0, sizeof(T_VALUEDESC));

    for (i = 0; i < update_rid->numfields; i++)
    {
        for (fld_idx = 0; fld_idx < table_info.numFields; fld_idx++)
        {
            if (!mdb_strcmp(params[i].name, fields_info[fld_idx].fieldName))
            {
                field = &fields_info[fld_idx];
                break;
            }
        }

        if (fld_idx == table_info.numFields)
        {
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }

        parameter_value(&params[i], &val);

        if (check_numeric_bound(&val, field->type, field->length) == RET_FALSE)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_TOOLARGEVALUE, 0);
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }

        fdesc.valuetype = field->type;
        fdesc.attrdesc.len = field->length;
        fdesc.attrdesc.collation = field->collation;
        sc_memset(&fval, 0, sizeof(T_VALUEDESC));

        ret = calc_func_convert(&fdesc, &val, &fval, MDB_FALSE);
        if (ret < 0)
        {
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }

        ret = update_fieldvalue(upd_values, i, field, &fval);
        if (ret < 0)
        {
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }

        sql_value_ptrFree(&val);
        sql_value_ptrFree(&fval);
    }

  RETURN_LABEL:
    if (fields_info)
        PMEM_FREE(fields_info);

    sql_value_ptrFree(&val);
    sql_value_ptrFree(&fval);

    return ret;
}

/*****************************************************************************/
//!  desc_parameters

/*! \breif  DYNAMIC API에서 parameter의 field 정보를 할당해주는 부분
 ************************************
 * \param stmt(in/out)  :
 * \param params(in/out):
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - called : 
 *  - cleanup시 chunk 범위를 알기위해서,
 *      TEMP stmt에 main stmt의 chunk 공간을 알려준다
 *
 *****************************************************************************/
static MDB_INT32
desc_parameters(T_STATEMENT * stmt, t_fieldinfo * params)
{
    EXPRDESC_LIST *cur;
    SYSFIELD_T *field;
    int i;

    T_LIMITDESC *limitdesc = NULL;

    if (stmt->sqltype == ST_SELECT)
    {
        if (desc_parameter4select(&stmt->u._select, params) == RET_ERROR)
            return RET_ERROR;
    }
    else if (stmt->sqltype == ST_INSERT || stmt->sqltype == ST_UPSERT)
    {
        if (stmt->u._insert.type == SIT_VALUES)
        {
            for (i = 0; i < stmt->u._insert.u.values.len; i++)
            {
                if (stmt->u._insert.columns.list)
                    field = stmt->u._insert.columns.list[i].fieldinfo;
                else
                    field = &stmt->u._insert.fieldinfo[i];

                desc_parameter4column(params,
                        &stmt->u._insert.u.values.list[i], field);
            }
        }
        else
        {
            T_STATEMENT *tmp_stmt;

            tmp_stmt = (T_STATEMENT *) PMEM_ALLOC(sizeof(T_STATEMENT));
            if (tmp_stmt == NULL)
                return RET_ERROR;

            tmp_stmt->sqltype = ST_SELECT;
            sc_memcpy(&tmp_stmt->u._select, &stmt->u._insert.u.query,
                    sizeof(T_SELECT));

            tmp_stmt->parsing_memory = stmt->parsing_memory;

            if (desc_parameters(tmp_stmt, params) == RET_ERROR)
            {
                PMEM_FREE(tmp_stmt);
                return RET_ERROR;
            }
            PMEM_FREE(tmp_stmt);
        }

    }
    else if (stmt->sqltype == ST_UPDATE)
    {
        for (i = 0; i < stmt->u._update.values.len; i++)
        {
            if (stmt->u._update.columns.list)
                field = stmt->u._update.columns.list[i].fieldinfo;
            else
                field = &stmt->u._update.planquerydesc.querydesc.fromlist.
                        list[0].fieldinfo[i];

            desc_parameter4column(params,
                    &stmt->u._update.values.list[i], field);
        }
        for (cur = stmt->u._update.planquerydesc.querydesc.expr_list; cur;
                cur = cur->next)
        {
            desc_parameter4expression(params, cur->ptr);
        }

        limitdesc = &stmt->u._update.planquerydesc.querydesc.limit;

        if (stmt->u._update.subselect.first_sub &&
                desc_parameter4select(&stmt->u._update.subselect,
                        params) == RET_ERROR)
            return RET_ERROR;

    }
    else if (stmt->sqltype == ST_DELETE)
    {
        for (cur = stmt->u._delete.planquerydesc.querydesc.expr_list; cur;
                cur = cur->next)
        {
            desc_parameter4expression(params, cur->ptr);
        }

        limitdesc = &stmt->u._delete.planquerydesc.querydesc.limit;

        if (stmt->u._delete.subselect.first_sub &&
                desc_parameter4select(&stmt->u._delete.subselect,
                        params) == RET_ERROR)
            return RET_ERROR;
    }
    else if (stmt->sqltype == ST_CREATE
            && stmt->u._create.type == SCT_TABLE_QUERY)
    {
        T_STATEMENT *tmp_stmt;

        tmp_stmt = (T_STATEMENT *) PMEM_ALLOC(sizeof(T_STATEMENT));
        if (tmp_stmt == NULL)
            return RET_ERROR;

        tmp_stmt->sqltype = ST_SELECT;
        sc_memcpy(&tmp_stmt->u._select,
                &stmt->u._create.u.table_query.query, sizeof(T_SELECT));

        tmp_stmt->parsing_memory = stmt->parsing_memory;

        if (desc_parameters(tmp_stmt, params) == RET_ERROR)
        {
            PMEM_FREE(tmp_stmt);
            return RET_ERROR;
        }

        limitdesc =
                &stmt->u._create.u.table_query.query.planquerydesc.querydesc.
                limit;
        PMEM_FREE(tmp_stmt);
    }

    if (limitdesc && limitdesc->offset_param_idx > -1)
    {
        desc_parameter4limit(params, limitdesc->offset_param_idx, DT_INTEGER);
    }

    if (limitdesc && limitdesc->rows_param_idx > -1)
    {
        desc_parameter4limit(params, limitdesc->rows_param_idx, DT_INTEGER);
    }

    return RET_SUCCESS;
}

int
__INTSQL_closetransaction(int handle, int mode)
{
    int index = THREAD_HANDLE;
    T_STATEMENT *stmt;
    int i;

    if (index == -1)
        return DB_E_NOTTRANSBEGIN;

    if (gClients[index].status == iSQL_STAT_CONNECTED)
    {
        return SQL_E_NOTLOGON;
    }

    stmt = gClients[index].sql->stmts;
    for (i = 0; i < gClients[index].sql->stmt_num; i++)
    {
        close_all_cursor(gClients[index].DBHandle, stmt);
        stmt = stmt->next;
    }

    MDB_DB_LOCK_API();
    if (mode == 0)
        SQL_trans_commit(gClients[index].DBHandle, gClients[index].sql->stmts);
    else
        SQL_trans_rollback(gClients[index].DBHandle,
                gClients[index].sql->stmts);
    MDB_DB_UNLOCK_API();

    return DB_SUCCESS;
}

__DECL_PREFIX int
INTSQL_closetransaction(int handle, int mode)
{
    int ret;
    int _g_connid_bk = -2;

    MDB_DB_LOCK();
    PUSH_G_CONNID(handle);

    ret = __INTSQL_closetransaction(handle, mode);

    POP_G_CONNID();
    MDB_DB_UNLOCK();
    return ret;
}

int
isql_PREPARE_real(iSQL * isql, char *query, int len, unsigned int *stmt_id,
        iSQL_FIELD ** pfields, unsigned int *param_count, iSQL_FIELD ** fields,
        unsigned int *field_count)
{
    int index;
    t_fieldinfo *field = NULL;
    T_LIST_SELECTION *selection;
    T_EXPRESSIONDESC *expr;
    T_QUERYRESULT *qres = NULL;
    T_ATTRDESC *attr;
    t_fieldinfo *rcvfields;
    T_SELECT *select = NULL;
    T_STATEMENT *stmt;
    char *qptr;
    int i, ret;
    int get_error;
    int set_flag = 0, on_or_off = 0, sqltype = 0;
    int fieldinfo_length;


    *param_count = *field_count = fieldinfo_length = 0;
    er_clear();

    if (len > iSQL_QUERY_MAX)
    {
        SMEM_FREENUL(isql->last_error);
        isql->last_errno = SQL_E_ERROR;
        isql->last_error = mdb_strdup("Query size is too big");
        return -1;
    }

    check_end_of_query(query, &len);
    len++;

    index = THREAD_HANDLE;
    if (gClients[index].status == iSQL_STAT_NOT_READY)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTCONNECTED, 0);
        ret = SQL_E_NOTCONNECTED;
        goto RETURN_LABEL;
    }

    if (gClients[index].status == iSQL_STAT_CONNECTED)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTLOGON, 0);
        ret = SQL_E_NOTLOGON;
        goto RETURN_LABEL;
    }

    ret = allocate_statement(gClients[index].sql);
    if (ret < 0)
        goto RETURN_LABEL;

    stmt = gClients[index].sql->stmts->prev;
    stmt->trans_info = &gClients[index].trans_info;

    stmt->qlen = len + 2;

    qptr = (char *) PMEM_XCALLOC(stmt->qlen + 1);
    if (qptr == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        ret = SQL_E_OUTOFMEMORY;
        release_statement(gClients[index].sql, stmt->id);
        goto RETURN_LABEL;
    }

    sc_memcpy(qptr, query, stmt->qlen - 2);

    qptr[stmt->qlen - 2] = ';';
    qptr[stmt->qlen - 1] = '\t';
    qptr[stmt->qlen] = '\0';

    stmt->query = qptr;
    stmt->sqltype = ST_NONE;

    ret = SQL_prepare(&gClients[index].DBHandle, stmt, gClients[index].flags);
    if (ret < 0)
    {
        ret = er_errid();
        PMEM_FREENUL(stmt->query);
        release_statement(gClients[index].sql, stmt->id);
        goto RETURN_LABEL;
    }

    stmt->status = iSQL_STAT_PREPARED;

    ret = SQL_E_SUCCESS;

    *stmt_id = stmt->id;

    switch (stmt->sqltype)
    {
    case ST_SELECT:
        sqltype = SQL_STMT_SELECT;
        *param_count = stmt->u._select.param_count;
        break;
    case ST_UPSERT:
        sqltype = SQL_STMT_UPSERT;
        *param_count = stmt->u._insert.param_count;
        break;
    case ST_INSERT:
        sqltype = SQL_STMT_INSERT;
        *param_count = stmt->u._insert.param_count;
        break;
    case ST_UPDATE:
        sqltype = SQL_STMT_UPDATE;
        *param_count = stmt->u._update.param_count;
        break;
    case ST_DELETE:
        sqltype = SQL_STMT_DELETE;
        *param_count = stmt->u._delete.param_count;
        break;
    case ST_CREATE:
        sqltype = SQL_STMT_CREATE;
        break;
    case ST_DROP:
        sqltype = SQL_STMT_DROP;
        break;
    case ST_RENAME:
        sqltype = SQL_STMT_RENAME;
        break;
    case ST_ALTER:
        sqltype = SQL_STMT_ALTER;
        break;
    case ST_TRUNCATE:
        sqltype = SQL_STMT_TRUNCATE;
        break;
    case ST_ADMIN:
        sqltype = SQL_STMT_ADMIN;
        break;
    case ST_COMMIT:
    case ST_COMMIT_FLUSH:
        sqltype = SQL_STMT_COMMIT;
        break;
    case ST_ROLLBACK:
    case ST_ROLLBACK_FLUSH:
        sqltype = SQL_STMT_ROLLBACK;
        break;
    case ST_DESCRIBE:
        sqltype = SQL_STMT_DESCRIBE;
        break;
    case ST_SET:
        sqltype = SQL_STMT_SET;
        break;
    default:
        sqltype = 0;
        *stmt_id = MDB_UINT_MAX;
        break;
    }

    if (stmt->sqltype == ST_SELECT)
    {       // field list를 보내자.

        if (stmt->u._select.planquerydesc.querydesc.setlist.len > 0)
        {
            select = stmt->u._select.planquerydesc.querydesc.setlist.list[0]->
                    u.subq;
            len = select->planquerydesc.querydesc.selection.len;
        }
        else
            len = stmt->u._select.planquerydesc.querydesc.selection.len;
        *field_count = len;

        if (*field_count > 0)
            fieldinfo_length += *field_count * sizeof(t_fieldinfo);

        if (*param_count > 0)
            fieldinfo_length += *param_count * sizeof(t_fieldinfo);

        if (fieldinfo_length)
        {
            field = PMEM_ALLOC(fieldinfo_length);
            if (!field)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                ret = SQL_E_OUTOFMEMORY;
                PMEM_FREENUL(stmt->query);
                gClients[index].store_or_use_result = MDB_UINT_MAX;
                release_statement(gClients[index].sql, stmt->id);

                goto RETURN_LABEL;
            }
            sc_memset(field, 0, fieldinfo_length);
        }

        if (select)
        {
            selection = &select->planquerydesc.querydesc.selection;
            qres = &select->queryresult;
        }
        else
        {
            selection = &stmt->u._select.planquerydesc.querydesc.selection;
            qres = &stmt->u._select.queryresult;
        }

        for (i = 0; i < selection->len; i++)
        {
            sc_memcpy(field[i].fieldname, qres->list[i].res_name,
                    FIELD_NAME_LENG - 1);
            field[i].fieldname[FIELD_NAME_LENG - 1] = '\0';
            expr = &selection->list[i].expr;

            if (expr->len == 1 &&
                    expr->list[0]->elemtype == SPT_VALUE &&
                    expr->list[0]->u.value.valueclass == SVC_VARIABLE &&
                    expr->list[0]->u.value.attrdesc.table.tablename)
            {
                attr = &expr->list[0]->u.value.attrdesc;

                sc_strcpy(field[i].tablename, attr->table.tablename);
                sc_strcpy(field[i].base_fieldname, attr->attrname);

                field[i].flags = attr->flag;
                if (attr->defvalue.not_null)
                {
                    switch (attr->type)
                    {
                    case DT_TINYINT:
                        sc_snprintf(field[i].default_value,
                                MAX_DEFAULT_VALUE_LEN, "%d",
                                attr->defvalue.u.i8);
                        break;
                    case DT_SMALLINT:
                        sc_snprintf(field[i].default_value,
                                MAX_DEFAULT_VALUE_LEN,
                                "%hd", attr->defvalue.u.i16);
                        break;
                    case DT_INTEGER:
                        sc_snprintf(field[i].default_value,
                                MAX_DEFAULT_VALUE_LEN, "%d",
                                attr->defvalue.u.i32);
                        break;
                    case DT_BIGINT:
                        sc_snprintf(field[i].default_value,
                                MAX_DEFAULT_VALUE_LEN,
                                I64_FORMAT, attr->defvalue.u.i64);
                        break;
                    case DT_FLOAT:
                        sc_snprintf(field[i].default_value,
                                MAX_DEFAULT_VALUE_LEN, "%g",
                                attr->defvalue.u.f16);
                        break;
                    case DT_DOUBLE:
                        sc_snprintf(field[i].default_value,
                                MAX_DEFAULT_VALUE_LEN, "%g",
                                attr->defvalue.u.f32);
                        break;
                    case DT_DECIMAL:
                        sc_snprintf(field[i].default_value,
                                MAX_DEFAULT_VALUE_LEN, "%g",
                                attr->defvalue.u.dec);
                        break;
                    case DT_CHAR:
                    case DT_VARCHAR:
                        sc_strncpy(field[i].default_value,
                                attr->defvalue.u.ptr,
                                attr->defvalue.value_len);

                        break;
                    case DT_DATETIME:
                        datetime2char(field[i].default_value,
                                attr->defvalue.u.datetime);
                        break;
                    case DT_DATE:
                        date2char(field[i].default_value,
                                attr->defvalue.u.date);
                        break;
                    case DT_TIME:
                        time2char(field[i].default_value,
                                attr->defvalue.u.time);
                        break;
                    case DT_BYTE:
                    case DT_VARBYTE:
                        sc_memcpy(field[i].default_value,
                                attr->defvalue.u.ptr,
                                attr->defvalue.value_len);
                        break;
                    case DT_TIMESTAMP:
                        timestamp2char(field[i].default_value,
                                &attr->defvalue.u.timestamp);
                        break;
                    default:
                        field[i].default_value[0] = '\0';
                        break;
                    }
                }
                else
                    field[i].default_value[0] = '\0';
            }
            else
            {
                field[i].tablename[0] = '\0';
                field[i].base_fieldname[0] = '\0';
                field[i].flags = 0;
                field[i].default_value[0] = '\0';
            }

            field[i].type = qres->list[i].value.valuetype;
            field[i].length = qres->list[i].len;
            field[i].max_length = qres->list[i].len;
            field[i].decimals = 0;

            switch (qres->list[i].value.valuetype)
            {
            case DT_TINYINT:
                field[i].buffer_length = sizeof(tinyint);
                break;
            case DT_SMALLINT:
                field[i].buffer_length = sizeof(smallint);
                break;
            case DT_INTEGER:
                field[i].buffer_length = sizeof(int);
                break;
            case DT_BIGINT:
                field[i].buffer_length = sizeof(bigint);
                break;
            case DT_OID:
                field[i].buffer_length = sizeof(OID);
                break;
            case DT_FLOAT:
                field[i].buffer_length = sizeof(float);
                break;
            case DT_DOUBLE:
                field[i].buffer_length = sizeof(double);
                break;
            case DT_DECIMAL:
                field[i].length = qres->list[i].len + 2;
                field[i].max_length = qres->list[i].len + 2;
                field[i].decimals = qres->list[i].value.attrdesc.dec;
                field[i].buffer_length = sizeof(decimal);
                break;
            case DT_CHAR:
            case DT_NCHAR:
            case DT_VARCHAR:
            case DT_NVARCHAR:
            case DT_BYTE:
            case DT_VARBYTE:
                field[i].length = qres->list[i].value.value_len;
                field[i].max_length = qres->list[i].value.attrdesc.len;
                field[i].decimals = 0;
                field[i].buffer_length = qres->list[i].len;
                break;
            case DT_DATETIME:
                field[i].max_length = MAX_DATETIME_STRING_LEN - 1;
                field[i].decimals = 0;
                field[i].buffer_length = sizeof(iSQL_TIME);
                break;
            case DT_DATE:
                field[i].max_length = sizeof(iSQL_TIME);
                field[i].decimals = 0;
                field[i].buffer_length = sizeof(iSQL_TIME);
                break;
            case DT_TIME:
                field[i].max_length = sizeof(iSQL_TIME);
                field[i].decimals = 0;
                field[i].buffer_length = sizeof(iSQL_TIME);
                break;
            case DT_TIMESTAMP:
                field[i].max_length = MAX_TIMESTAMP_STRING_LEN - 1;
                field[i].decimals = 0;
                field[i].buffer_length = sizeof(iSQL_TIME);
                break;
            case DT_NULL_TYPE:
                field[i].max_length = sc_strlen(qres->list[i].res_name);
                field[i].decimals = 0;
                field[i].buffer_length = 0;
                break;
            default:
                field[i].type = 0;
                break;
            }
        }
    }
    else if (stmt->sqltype == ST_DESCRIBE)
    {
        if (stmt->u._desc.type == SBT_DESC_TABLE)
        {
            if (stmt->u._desc.describe_table)
            {
                *field_count = 7;
            }
            else
            {
                *field_count = 1;
            }
        }
        else if (stmt->u._desc.type == SBT_SHOW_TABLE)
        {
            *field_count = 1;
        }
#if defined(USE_DESC_COMPILE_OPT)
        else if (stmt->u._desc.type == SBT_DESC_COMPILE_OPT)
        {
            *field_count = 1;
        }
#endif

        fieldinfo_length += *field_count * sizeof(t_fieldinfo);

        field = PMEM_ALLOC(fieldinfo_length);
        if (!field)
        {
            fieldinfo_length = 0;
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            ret = SQL_E_OUTOFMEMORY;
            PMEM_FREENUL(stmt->query);
            gClients[index].store_or_use_result = MDB_UINT_MAX;
            release_statement(gClients[index].sql, stmt->id);

            goto RETURN_LABEL;
        }
        sc_memset(field, 0, fieldinfo_length);

        for (i = 0; i < *field_count; i++)
        {
            sc_memset(&field[i], 0, sizeof(t_fieldinfo));
            if (stmt->u._desc.type == SBT_DESC_TABLE)
            {
                switch (i)
                {
                case 0:
                    sc_strcpy(field[i].fieldname, "NAME");
                    field[i].length = 32;
                    field[i].max_length = 32;
                    field[i].flags &= ~NULL_BIT;
                    break;
                case 1:
                    sc_strcpy(field[i].fieldname, "TYPE");
                    field[i].length = 32;
                    field[i].max_length = 32;
                    field[i].flags &= ~NULL_BIT;
                    break;
                case 2:
                    sc_strcpy(field[i].fieldname, "NULL");
                    field[i].length = 8;
                    field[i].max_length = 8;
                    field[i].flags &= ~NULL_BIT;
                    break;
                case 3:
                    sc_strcpy(field[i].fieldname, "KEY");
                    field[i].length = 3;
                    field[i].max_length = 3;
                    field[i].flags |= NULL_BIT;
                    break;
                case 4:
                    sc_strcpy(field[i].fieldname, "DEFAULT");
                    field[i].length = MAX_DEFAULT_VALUE_LEN;
                    field[i].max_length = MAX_DEFAULT_VALUE_LEN;
                    field[i].flags |= NULL_BIT;
                    break;
                case 5:
                    sc_strcpy(field[i].fieldname, "AUTOINCREMENT");
                    field[i].length = 13;
                    field[i].max_length = 13;
                    field[i].flags |= NULL_BIT;
                    break;
                case 6:
                    sc_strcpy(field[i].fieldname, "NOLOGGING");
                    field[i].length = 9;
                    field[i].max_length = 9;
                    field[i].flags |= NULL_BIT;
                    break;
                default:
                    break;
                }

                field[i].type = SQL_DATA_CHAR;
                field[i].buffer_length = field[i].length;
            }
            else if (stmt->u._desc.type == SBT_SHOW_TABLE)
            {
                sc_strcpy(field[i].fieldname, "#SHOW");
                field[i].length = 8000;
                field[i].max_length = 8000;
                field[i].flags &= ~NULL_BIT;
                field[i].type = SQL_DATA_VARCHAR;
                field[i].buffer_length = field[i].length;
            }
#if defined(USE_DESC_COMPILE_OPT)
            else if (stmt->u._desc.type == SBT_DESC_COMPILE_OPT)
            {
                sc_strcpy(field[i].fieldname, "COMPILE_OPT");
                field[i].length = 8000;
                field[i].max_length = 8000;
                field[i].flags &= ~NULL_BIT;
                field[i].type = SQL_DATA_VARCHAR;
                field[i].buffer_length = field[i].length;
            }
#endif
        }
    }
    else if (stmt->sqltype == ST_SET)
    {
        switch (stmt->u._set.type)
        {
        case SST_AUTOCOMMIT:
            SQL_trans_commit(gClients[index].DBHandle, stmt);
            set_flag = OPT_AUTOCOMMIT;
            on_or_off = stmt->u._set.u.on_off;
            if (stmt->u._set.u.on_off)
                gClients[index].flags |= OPT_AUTOCOMMIT;
            else
                gClients[index].flags &= ~OPT_AUTOCOMMIT;
            break;
        case SST_TIME:
            set_flag = OPT_TIME;
            on_or_off = stmt->u._set.u.on_off;
            if (stmt->u._set.u.on_off)
                gClients[index].flags |= OPT_TIME;
            else
                gClients[index].flags &= ~OPT_TIME;
            break;
        case SST_HEADING:
            set_flag = OPT_HEADING;
            on_or_off = stmt->u._set.u.on_off;
            if (stmt->u._set.u.on_off)
                gClients[index].flags |= OPT_HEADING;
            else
                gClients[index].flags &= ~OPT_HEADING;
            break;
        case SST_FEEDBACK:
            set_flag = OPT_FEEDBACK;
            on_or_off = stmt->u._set.u.on_off;
            if (stmt->u._set.u.on_off)
                gClients[index].flags |= OPT_FEEDBACK;
            else
                gClients[index].flags &= ~OPT_FEEDBACK;
            break;
        case SST_RECONNECT:
            set_flag = OPT_RECONNECT;
            on_or_off = stmt->u._set.u.on_off;
            if (stmt->u._set.u.on_off)
                gClients[index].flags |= OPT_RECONNECT;
            else
                gClients[index].flags &= ~OPT_RECONNECT;
            break;
        case SST_PLAN_ON:
            set_flag = OPT_PLAN_ON;
            on_or_off = 1;
            gClients[index].flags &= ~(OPT_PLAN_OFF + OPT_PLAN_ONLY);
            gClients[index].flags |= OPT_PLAN_ON;
            break;
        case SST_PLAN_OFF:
            set_flag = OPT_PLAN_OFF;
            on_or_off = 1;
            gClients[index].flags &= ~(OPT_PLAN_ON + OPT_PLAN_ONLY);
            gClients[index].flags |= OPT_PLAN_OFF;
            break;
        case SST_PLAN_ONLY:
            set_flag = OPT_PLAN_ONLY;
            on_or_off = 1;
            gClients[index].flags &= ~(OPT_PLAN_ON + OPT_PLAN_OFF);
            gClients[index].flags |= OPT_PLAN_ONLY;
            break;
        default:
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
            ret = SQL_E_ERROR;
            PMEM_FREENUL(stmt->query);
            release_statement(gClients[index].sql, stmt->id);
            goto RETURN_LABEL;
        }
    }
    else if (stmt->sqltype == ST_INSERT || stmt->sqltype == ST_UPDATE ||
            stmt->sqltype == ST_DELETE || stmt->sqltype == ST_UPSERT)
    {
        if (*param_count > 0)
            fieldinfo_length += *param_count * sizeof(t_fieldinfo);

        if (fieldinfo_length)
        {
            field = PMEM_ALLOC(fieldinfo_length);
            if (!field)
            {
                fieldinfo_length = 0;
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                ret = SQL_E_OUTOFMEMORY;
                PMEM_FREENUL(stmt->query);
                gClients[index].store_or_use_result = MDB_UINT_MAX;
                release_statement(gClients[index].sql, stmt->id);

                goto RETURN_LABEL;
            }
            sc_memset(field, 0, fieldinfo_length);
        }
    }


    if (*param_count > 0)
    {
        desc_parameters(stmt, &field[*field_count]);
        /*fieldinfo_length += n; */
        for (i = *field_count; i < (*field_count + *param_count); ++i)
        {
            if (field[i].type == SQL_DATA_NONE)
            {
                fieldinfo_length = 0;
                PMEM_FREENUL(stmt->fields);
                PMEM_FREENUL(stmt->query);
                SQL_cleanup_memory(isql->handle, stmt);
                release_statement(gClients[index].sql, stmt->id);

                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_CANNOTBINDING,
                        0);
                ret = SQL_E_CANNOTBINDING;
                goto RETURN_LABEL;
            }
        }
    }

    stmt->size_fields = fieldinfo_length;
    stmt->field_count = *field_count;
    stmt->fields = field;


  RETURN_LABEL:

    get_error = 1;

    if (ret < 0)
    {
        ret = 0;
        goto ERROR_AND_EXIT;
    }

    *fields = NULL;

    if (sqltype == SQL_STMT_SELECT || sqltype == SQL_STMT_DESCRIBE)
    {
        if (*field_count > 0)
        {
            *fields = SMEM_ALLOC(sizeof(iSQL_FIELD) * (*field_count));
            if (*fields == NULL)
            {
                get_error = SQL_E_OUTOFMEMORY;
                ret = 0;
                goto ERROR_AND_EXIT;
            }

            sc_memcpy((char *) (*fields), (char *) field,
                    (sizeof(iSQL_FIELD) * (*field_count)));

        }
    }
    else if (sqltype == SQL_STMT_SET)
    {       // if SET
        switch (set_flag)
        {
        case OPT_PLAN_ON:
        case OPT_PLAN_OFF:
        case OPT_PLAN_ONLY:
            isql->flags &= ~(OPT_PLAN_ON + OPT_PLAN_OFF + OPT_PLAN_ONLY);
            isql->flags |= set_flag;
            break;
        default:
            if (on_or_off)
                isql->flags |= set_flag;
            else
                isql->flags &= ~set_flag;
            break;
        }
    }

    *pfields = NULL;
    if (*param_count > 0)
    {
        rcvfields = &field[*field_count];
        *pfields = SMEM_ALLOC(sizeof(iSQL_FIELD) * (*param_count));
        if (*pfields == NULL)
        {
            get_error = SQL_E_OUTOFMEMORY;
            ret = 0;
            goto ERROR_AND_EXIT;
        }
        sc_memcpy((char *) (*pfields), (char *) rcvfields,
                (sizeof(iSQL_FIELD) * (*param_count)));
    }

    isql->last_querytype = sqltype;


    return 1;

  ERROR_AND_EXIT:

    get_last_error(isql, get_error);

    return ret;

    // to here
}

int
isql_EXECUTE_real(iSQL_STMT * isql_stmt)
{
    int index = THREAD_HANDLE;
    int ret, posi, tot_size, affected_rows;
    T_STATEMENT *stmt;
    T_DBMS_CS_EXECUTE *packed_params = NULL;
    MDB_UINT32 i;
    t_parameter *param;
    iSQL *isql = isql_stmt->isql;
    iSQL_BIND *bind = isql_stmt->pbind;
    const int fixed = sizeof(T_DBMS_CS_EXECUTE) - sizeof(t_parameter);
    const int fixed_for_parameter = sizeof(t_parameter) - FOUR_BYTE_ALIGN;

    if (gClients[index].status == iSQL_STAT_CONNECTED)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTLOGON, 0);
        ret = SQL_E_NOTLOGON;
        goto RETURN_LABEL;
    }

    stmt = get_statement(gClients[index].sql, isql_stmt->stmt_id);
    if (stmt == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_STATEMENTNOTFOUND, 0);
        ret = SQL_E_STATEMENTNOTFOUND;
        goto RETURN_LABEL;
    }

    stmt->query_timeout = isql_stmt->query_timeout;

    if (stmt->status != iSQL_STAT_PREPARED &&
            stmt->status != iSQL_STAT_EXECUTED)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTPREPARED, 0);
        ret = SQL_E_NOTPREPARED;
        goto RETURN_LABEL;
    }

    if (stmt->status == iSQL_STAT_EXECUTED && stmt->do_recompile)
    {
        if (stmt->query && stmt->qlen > 0)
        {
            char *_query_tmp = stmt->query;
            int _qlen_tmp = stmt->qlen;

            stmt->query = NULL;
            stmt->qlen = 0;

            SQL_cleanup_memory(isql_stmt->isql->handle, stmt);

            stmt->query = _query_tmp;
            stmt->qlen = _qlen_tmp;

            if (SQL_prepare(&(isql_stmt->isql->handle), stmt, 0) == RET_ERROR)
            {
                return RET_ERROR;
            }
        }
        else
        {
            /* must not happen this!!! */
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDCOMMAND, 0);

            return RET_ERROR;
        }
    }

    if (isql_stmt->param_count > 0 && !isql_stmt->pbind)
    {
        ret = SQL_E_CANNOTBINDING;
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_CANNOTBINDING, 0);
        goto RETURN_LABEL;
    }

    // pack parameters according to T_DBMS_CS_EXECUTE
    tot_size = 0;
    if (isql_stmt->pbind)
    {
        for (i = 0; i < isql_stmt->param_count; i++)
        {
            if (bind[i].buffer_type == SQL_DATA_TIME &&
                    bind[i].buffer_length == SSIZE_TIME)
            {
                tot_size += fixed_for_parameter +
                        MDB_ALIGN(DB_BYTE_LENG(bind[i].buffer_type,
                                sizeof(iSQL_TIME)));
            }
            else
            {
                tot_size += fixed_for_parameter +
                        MDB_ALIGN(DB_BYTE_LENG(bind[i].buffer_type,
                                bind[i].buffer_length));
            }
        }
    }

    packed_params = SMEM_ALLOC(fixed + tot_size);
    if (packed_params == NULL)
    {
        isql->last_errno = SQL_E_OUTOFMEMORY;
        return -1;
    }

    packed_params->stmt_id = isql_stmt->stmt_id;
    packed_params->query_timeout = isql_stmt->query_timeout;

    if (isql_stmt->pbind)
    {
        packed_params->param_count = isql_stmt->param_count;
        for (i = 0, posi = 0; i < isql_stmt->param_count; i++)
        {
            param = (t_parameter *) ((char *) packed_params->params + posi);
            if (bind[i].is_null)
                param->is_null = *bind[i].is_null;
            else
                param->is_null = 0;

            param->buffer_type = bind[i].buffer_type;
            param->buffer_length = bind[i].buffer_length;
            if (IS_BS_OR_NBS(bind[i].buffer_type))
            {
                // 일단.. 이곳에서 설정하지만..
                // 외부에서 값을 설정하도록 유도하자..
                param->value_length =
                        strlen_func[bind[i].buffer_type] (bind[i].buffer);

                /* value_length cannot exceed buffer_length */
                if (param->value_length > param->buffer_length)
                    param->value_length = param->buffer_length;
            }
            else if (IS_BYTE(bind[i].buffer_type))
            {
                /* byte type value must provide length info. of the value */
                if (bind[i].length == NULL
                        || *(bind[i].length) > (unsigned) param->buffer_length)
                {
                    SMEM_FREENUL(isql->last_error);
                    isql->last_errno = DB_E_INVALIDPARAM;
                    return -1;
                }

                param->value_length = *(bind[i].length);
            }
            else
                param->value_length = bind[i].buffer_length;

            sc_memset(param->buffer, 0, param->buffer_length);

            if (bind[i].buffer_type == SQL_DATA_TIME &&
                    bind[i].buffer_length == SSIZE_TIME)
            {
                memcpy_func[param->buffer_type] (param->buffer + SSIZE_DATE,
                        bind[i].buffer, param->value_length);
            }
            else
            {
                memcpy_func[param->buffer_type] (param->buffer,
                        bind[i].buffer, param->value_length);
            }
            posi += fixed_for_parameter +
                    MDB_ALIGN(DB_BYTE_LENG(bind[i].buffer_type,
                            bind[i].buffer_length));
        }
    }
    else
        packed_params->param_count = 0;

    // parameter packing end

    if (isql_stmt->param_count)
    {
        if (set_parameter_values(stmt, packed_params) == RET_ERROR)
        {
            ret = er_errid();
            goto RETURN_LABEL;
        }
    }

    if (stmt->sqltype != ST_INSERT)
    {
        int stmt_idx;
        T_STATEMENT *tmp = gClients[index].sql->stmts;

        for (stmt_idx = 0; stmt_idx < gClients[index].sql->stmt_num;
                stmt_idx++)
        {
            if (tmp->sqltype == ST_INSERT)
                SQL_close_all_cursors(gClients[index].DBHandle, tmp);
            tmp = tmp->next;
        }
    }

    affected_rows = SQL_execute(&gClients[index].DBHandle,
            stmt, gClients[index].flags, RES_TYPE_BINARY);

    /* one CursorOpen, multi CursorInsret */
    if (stmt->sqltype != ST_INSERT)
        if (stmt->sqltype != ST_SELECT)
            close_all_cursor(gClients[index].DBHandle, stmt);

    if (affected_rows < 0)
    {
        isql_stmt->affected_rid = 0;
        isql_stmt->affected_rows = 0;
        ret = er_errid();
        goto RETURN_LABEL;
    }
    else if (stmt->sqltype == ST_INSERT || stmt->sqltype == ST_UPSERT)
    {
        isql_stmt->affected_rid = stmt->affected_rid;
    }
    else
    {
        isql_stmt->affected_rid = 0;
    }

    ret = SQL_E_SUCCESS;

    if (stmt->sqltype == ST_SELECT || stmt->sqltype == ST_DESCRIBE)
    {
        isql_stmt->affected_rows = 0;
        stmt->status = iSQL_STAT_EXECUTED;
    }

    if (affected_rows < 0)
        isql_stmt->affected_rows = 0;
    else
        isql_stmt->affected_rows = affected_rows;

  RETURN_LABEL:

    get_last_error(isql, 1);
    SMEM_FREENUL(packed_params);

    if (ret < 0)
        return 0;
    else
        return 1;
}

int
isql_STMTSTORERESULT_real(iSQL_STMT * isql_stmt, int use_result)
{
    int index = THREAD_HANDLE;
    int ret, row_count, use_eof, rid_included;
    int i, row_len, get_error = 1;
    char *cursor;
    void *rst_page_list;
    iSQL *isql = isql_stmt->isql;
    iSQL_DATA *data = NULL;
    iSQL_ROWS *data_cursor = NULL;
    iSQL_RESULT_PAGE *rst_pg_head = NULL;
    iSQL_RESULT_PAGE *cur_pg = NULL;
    T_STATEMENT *stmt;

    if (gClients[index].status == iSQL_STAT_CONNECTED)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTLOGON, 0);
        ret = SQL_E_NOTLOGON;
        goto RETURN_LABEL;
    }

    stmt = get_statement(gClients[index].sql, isql_stmt->stmt_id);
    if (stmt == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_STATEMENTNOTFOUND, 0);
        ret = SQL_E_STATEMENTNOTFOUND;
        goto RETURN_LABEL;
    }

    if (stmt->status != iSQL_STAT_EXECUTED ||
            (stmt->sqltype != ST_SELECT && stmt->sqltype != ST_DESCRIBE))
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTAVAILABLEHANDLE, 0);
        ret = SQL_E_NOTAVAILABLEHANDLE;
        goto RETURN_LABEL;
    }

    if (use_result == 1)
    {
        // call to tell(to set fUseResult to 1) this is second, discrete from
        // first, bulk read and hence be carefull if there are any change to
        // index currently using.
        dbi_Trans_Set_RU_fUseResult(isql_stmt->isql->handle);
    }

    stmt->is_partial_result = use_result;

    if (SQL_store_result(gClients[index].DBHandle, stmt, gClients[index].flags,
                    RES_TYPE_BINARY) < 0)
    {
        ret = er_errid();
        row_count = 0;
        goto RETURN_LABEL;
    }

    if (use_result == 0 || stmt->result->row_num < stmt->result->fetch_cnt)
    {
        close_all_cursor(gClients[index].DBHandle, stmt);
    }

    if (stmt->result->tmp_rec_len == 0)
        use_eof = 1;
    else
        use_eof = 0;

    ret = SQL_E_SUCCESS;
    row_count = stmt->result->row_num;
    rid_included = stmt->result->rid_included;

    /* send the pointer to the result page list */
    rst_page_list = stmt->result->list->head;

    rst_pg_head = (T_RESULT_PAGE *) rst_page_list;

    data = SMEM_XCALLOC(sizeof(iSQL_DATA));
    if (data == NULL)
    {
        get_error = SQL_E_OUTOFMEMORY;
        ret = 0;
        goto FREE_AND_EXIT;
    }

    data->data_buf = NULL;

    cur_pg = rst_pg_head;
    cursor = &cur_pg->body[0];

    for (i = 0; i < row_count; i++)
    {
        if (i == 0)
        {
            data->rows_buf = SMEM_XCALLOC(sizeof(iSQL_ROWS) * row_count);
            if (data->rows_buf == NULL)
            {
                get_error = SQL_E_OUTOFMEMORY;
                ret = 0;
                goto FREE_AND_EXIT;

            }
            data->buf_len = row_count;
            data->data = &data->rows_buf[0];
            data_cursor = data->data;
        }
        else
        {
            data_cursor->next = &data->rows_buf[i];
            data_cursor = data_cursor->next;
        }

        if ((cursor - &cur_pg->body[0]) >= cur_pg->body_size)
        {
            cur_pg = cur_pg->next;
            if (cur_pg == NULL)
            {
                get_error = SQL_E_ERROR;
                ret = 0;
                goto FREE_AND_EXIT;
            }
            cursor = &cur_pg->body[0];
        }

        sc_memcpy((char *) &data_cursor->rid, cursor, OID_SIZE);
        sc_memcpy(&row_len, cursor + OID_SIZE, INT_SIZE);
        data_cursor->data = (char **) (cursor + OID_SIZE + INT_SIZE);
        cursor += row_len;
    }

    data->result_page_list = rst_pg_head;
    data->row_num = row_count;
    data->use_eof = use_eof;

    isql_stmt->res->data = data;
    isql_stmt->res->data_cursor = isql_stmt->res->data->data;
    isql_stmt->res->current_row = NULL;
    isql_stmt->res->data_offset = 0;

    if (use_result == 0)
    {
        isql_stmt->res->is_partial_result = 0;
    }
    else
    {
        isql_stmt->res->is_partial_result = 1;
    }

    isql->affected_rows = data->row_num;
    isql_stmt->res->row_count = data->row_num;
    isql_stmt->res->rid_included = rid_included;
    isql_stmt->affected_rows = data->row_num;

    if (use_result == 0 || (stmt->result && stmt->result->tmp_rec_len == 0))
    {
        PMEM_FREENUL(stmt->result->list);
        PMEM_FREENUL(stmt->result->field_datatypes);
        if (use_result == 1)
        {
            PMEM_FREENUL(stmt->result->tmp_recbuf);
        }
        PMEM_FREENUL(stmt->result);
    }

    stmt->status = iSQL_STAT_PREPARED;

    return ret;

  FREE_AND_EXIT:
    while (rst_pg_head)
    {
        cur_pg = rst_pg_head;
        rst_pg_head = cur_pg->next;
        SMEM_FREE(cur_pg);
    }
    if (data)
        SMEM_FREENUL(data->rows_buf);
    SMEM_FREENUL(data);

  RETURN_LABEL:

    get_last_error(isql, get_error);

    //
    // to here
    //

    return ret;

}

int
isql_FETCH_real(iSQL_STMT * isql_stmt)
{
    int index = THREAD_HANDLE;
    int ret, use_eof, row_count, rid_included;
    int get_error, i, row_len;
    char *cursor;
    void *rst_page_list;
    iSQL *isql = isql_stmt->isql;
    iSQL_DATA *data = NULL;
    iSQL_ROWS *data_cursor = NULL;
    iSQL_RESULT_PAGE *cur_pg = NULL;
    iSQL_RESULT_PAGE *rst_pg_head = NULL;
    T_STATEMENT *stmt;

    if (gClients[index].status == iSQL_STAT_CONNECTED)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTLOGON, 0);
        ret = SQL_E_NOTLOGON;
        goto RETURN_LABEL;
    }

    stmt = get_statement(gClients[index].sql, isql_stmt->stmt_id);
    if (stmt == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_STATEMENTNOTFOUND, 0);
        ret = SQL_E_STATEMENTNOTFOUND;
        goto RETURN_LABEL;
    }

    if (stmt->status != iSQL_STAT_PREPARED ||
            (stmt->sqltype != ST_SELECT && stmt->sqltype != ST_DESCRIBE))
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTAVAILABLEHANDLE, 0);
        ret = SQL_E_NOTAVAILABLEHANDLE;
        goto RETURN_LABEL;
    }

    if (stmt->result == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ILLEGALCALLSEQUENCE, 0);
        ret = SQL_E_ILLEGALCALLSEQUENCE;
        goto RETURN_LABEL;
    }

    if (stmt->result->tmp_rec_len != 0)
    {
        // it's second read

    }

    use_eof = 0;
    stmt->result->row_num = 0;
    if (SQL_store_result(gClients[index].DBHandle, stmt,
                    gClients[index].flags, RES_TYPE_BINARY) < 0)
    {
        ret = er_errid();
        row_count = 0;
        goto RETURN_LABEL;

    }
    ret = SQL_E_SUCCESS;
    row_count = stmt->result->row_num;
    if (stmt->result->tmp_rec_len == 0)
        use_eof = 1;

    rid_included = stmt->result->rid_included;

    /* send the pointer to the result page list */
    rst_page_list = stmt->result->list->head;

    rst_pg_head = (T_RESULT_PAGE *) rst_page_list;
    data = isql_stmt->res->data;
    if (data == NULL)
    {
        data = SMEM_XCALLOC(sizeof(iSQL_DATA));
        if (data == NULL)
        {
            get_error = SQL_E_OUTOFMEMORY;
            ret = 0;
            goto ERROR_AND_EXIT;
        }
    }

    data->data_buf = NULL;

    cur_pg = rst_pg_head;
    cursor = &cur_pg->body[0];

    for (i = 0; i < row_count; i++)
    {
        if (i == 0)
        {
            if (data->buf_len < row_count)
            {
                SMEM_FREENUL(data->rows_buf);
                data->rows_buf = SMEM_XCALLOC(sizeof(iSQL_ROWS) * row_count);
                if (data->rows_buf == NULL)
                {
                    get_error = SQL_E_OUTOFMEMORY;
                    ret = 0;
                    goto ERROR_AND_EXIT;

                }
                data->buf_len = row_count;
            }
            data->data = &data->rows_buf[0];
            data_cursor = data->data;
        }
        else
        {
            data_cursor->next = &data->rows_buf[i];
            data_cursor = data_cursor->next;
        }

        if ((cursor - &cur_pg->body[0]) >= cur_pg->body_size)
        {
            cur_pg = cur_pg->next;
            if (cur_pg == NULL)
            {
                get_error = SQL_E_ERROR;
                ret = 0;
                goto ERROR_AND_EXIT;
            }
            cursor = &cur_pg->body[0];
        }

        sc_memcpy((char *) &data_cursor->rid, cursor, OID_SIZE);
        sc_memcpy(&row_len, cursor + OID_SIZE, INT_SIZE);
        data_cursor->data = (char **) (cursor + OID_SIZE + INT_SIZE);
        cursor += row_len;
    }

    if (data_cursor)
        data_cursor->next = NULL;

    if (row_count == 0)
        isql_stmt->res->data->data = NULL;

    data->result_page_list = rst_pg_head;
    data->row_num = row_count;
    data->use_eof = use_eof;

    isql_stmt->res->data = data;
    isql_stmt->res->data_cursor = isql_stmt->res->data->data;
    isql_stmt->res->current_row = NULL;

    isql->affected_rows = data->row_num;
    isql_stmt->res->row_count += data->row_num;
    isql_stmt->res->rid_included = rid_included;
    isql_stmt->affected_rows = data->row_num;

    if (stmt->result->tmp_rec_len == 0)
    {
        if (stmt->result->list->tail)
            stmt->result->list->head = stmt->result->list->tail;

        PMEM_FREENUL(stmt->result->list);
        PMEM_FREENUL(stmt->result->field_datatypes);
        PMEM_FREENUL(stmt->result->tmp_recbuf);
        PMEM_FREENUL(stmt->result);
    }

    stmt->status = iSQL_STAT_PREPARED;

    return 1;

  ERROR_AND_EXIT:
    while (rst_pg_head)
    {
        cur_pg = rst_pg_head;
        rst_pg_head = cur_pg->next;
        SMEM_FREE(cur_pg);
    }

    //
    // to here
    //

  RETURN_LABEL:

    get_last_error(isql, get_error);

    return ret;
}

int
isql_CLOSESTATEMENT_real(iSQL_STMT * isql_stmt)
{
    int index = THREAD_HANDLE;
    int ret;
    T_STATEMENT *stmt;

    if (gClients[index].status == iSQL_STAT_CONNECTED)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTLOGON, 0);
        ret = SQL_E_NOTLOGON;
        goto RETURN_LABEL;
    }

    stmt = get_statement(gClients[index].sql, isql_stmt->stmt_id);
    if (stmt == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_STATEMENTNOTFOUND, 0);
        ret = SQL_E_STATEMENTNOTFOUND;
        goto RETURN_LABEL;
    }

    if (stmt->status == iSQL_STAT_IN_USE
            || stmt->status == iSQL_STAT_PREPARED
            || stmt->status == iSQL_STAT_EXECUTED)
    {
        index = THREAD_HANDLE;
        SQL_cleanup(&gClients[index].DBHandle, stmt, MODE_AUTO, 1);
    }

    PMEM_FREENUL(stmt->query);
    PMEM_FREENUL(stmt->fields);

    if (stmt->result)
    {
        if (stmt->is_partial_result)
            PMEM_FREE(stmt->result->list);
        else
            RESULT_LIST_Destroy(stmt->result->list);
        PMEM_FREENUL(stmt->result->field_datatypes);
        PMEM_FREENUL(stmt->result->tmp_recbuf);
        PMEM_FREENUL(stmt->result);
    }

    release_statement(gClients[index].sql, isql_stmt->stmt_id);

    ret = SQL_E_SUCCESS;

  RETURN_LABEL:

    if (ret < 0)
    {
        get_last_error(isql_stmt->isql, 1);
        ret = 0;
    }

    return ret;
}

int
isql_GETRESULT_real(iSQL * isql, const int retrieval)
{
    int index = THREAD_HANDLE;
    int ret, row_count, field_count, use_eof;
    int i, j, get_error;
    char *cursor;
    void *rst_page_list = NULL;
    iSQL_DATA *data = NULL;
    iSQL_RESULT_PAGE *rst_pg_head = NULL;
    iSQL_RESULT_PAGE *cur_pg = NULL;
    iSQL_ROWS *data_cursor = NULL;

    T_STATEMENT *stmt;

    if (gClients[index].status == iSQL_STAT_CONNECTED)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTLOGON, 0);
        ret = SQL_E_NOTLOGON;
        goto RETURN_LABEL;
    }

    if (gClients[index].store_or_use_result == MDB_UINT_MAX)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ILLEGALCALLSEQUENCE, 0);
        ret = SQL_E_ILLEGALCALLSEQUENCE;
        goto RETURN_LABEL;
    }

    stmt = get_statement(gClients[index].sql,
            gClients[index].store_or_use_result);
    if (stmt == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_STATEMENTNOTFOUND, 0);
        ret = SQL_E_STATEMENTNOTFOUND;
        goto RETURN_LABEL;
    }

    if (stmt->status != iSQL_STAT_IN_USE &&
            !((stmt->sqltype == ST_SELECT || stmt->sqltype == ST_DESCRIBE) &&
                    retrieval == 1))
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTAVAILABLEHANDLE, 0);
        ret = SQL_E_NOTAVAILABLEHANDLE;
        goto RETURN_LABEL;
    }

    if (retrieval == 0)
    {
        stmt->is_partial_result = 1;
    }
    else
    {
        stmt->is_partial_result = 0;
    }
    row_count = SQL_store_result(gClients[index].DBHandle, stmt,
            gClients[index].flags, RES_TYPE_STRING);

    if (retrieval == 1 || (retrieval == 0 &&
                    stmt->result && stmt->result->tmp_rec_len == 0))
    {
        SQL_cleanup(&gClients[index].DBHandle, stmt, MODE_MANUAL, 1);
    }

    if (row_count < 0)
    {
        ret = er_errid();
        goto RETURN_LABEL;
    }

    ret = SQL_E_SUCCESS;
    field_count = stmt->result->field_count;

    if ((retrieval == 0 && stmt->result && stmt->result->tmp_rec_len == 0))
        use_eof = 1;
    else
        use_eof = 0;


    if (retrieval == 0)
    {
        stmt->status = iSQL_STAT_USE_RESULT;
    }
    else
    {
        /* retrieval != 0: retrieving a complete result set */

        /* send the pointer of the result page list */
        rst_page_list = stmt->result->list->head;
    }

    rst_pg_head = (T_RESULT_PAGE *) rst_page_list;

    data = SMEM_XCALLOC(sizeof(iSQL_DATA));
    if (data == NULL)
    {
        get_error = SQL_E_OUTOFMEMORY;
        ret = 0;
        goto RETURN_LABEL;
    }

    cur_pg = rst_pg_head;
    cursor = &cur_pg->body[0];

    for (i = 0; i < row_count; i++)
    {
        if (i == 0)
        {
            data->rows_buf = SMEM_XCALLOC(sizeof(iSQL_ROWS) * row_count);

            if (data->rows_buf != NULL)
            {
                data->data_buf =
                        SMEM_XCALLOC(sizeof(char *) * row_count * field_count);
            }

            if (data->rows_buf == NULL || data->data_buf == NULL)
            {
                get_error = SQL_E_OUTOFMEMORY;
                ret = 0;
                goto RETURN_LABEL;
            }
            data->buf_len = row_count;
            data->data = &data->rows_buf[0];
            data_cursor = data->data;
        }
        else
        {
            data_cursor->next = &data->rows_buf[i];
            data_cursor = data_cursor->next;
        }

        data_cursor->data = &data->data_buf[i * field_count];

        if ((cursor - &cur_pg->body[0]) >= cur_pg->body_size)
        {
            cur_pg = cur_pg->next;
            if (cur_pg == NULL)
            {
                get_error = SQL_E_ERROR;
                ret = 0;
                goto RETURN_LABEL;
            }
            cursor = &cur_pg->body[0];
        }

        for (j = 0; j < field_count; j++)
        {
            if (IS_NBS(isql->fields[j].type))
            {
                cursor = (char *) _alignedlen((unsigned long) cursor,
                        sizeof(DB_WCHAR));

                data_cursor->data[j] = cursor;
                cursor +=
                        (sc_wcslen((DB_WCHAR *) cursor) +
                        1) * sizeof(DB_WCHAR);
            }
            else
            {
                data_cursor->data[j] = cursor;
                cursor += (sc_strlen(cursor) + 1);
            }
        }

        /* length of record align to sizeof(DB_INT32) */
        cursor = (char *) _alignedlen((unsigned long) cursor,
                sizeof(MDB_INT32));
    }


    data->result_page_list = rst_pg_head;

    data->row_num = row_count;
    data->field_count = field_count;
    data->use_eof = use_eof;

    isql->data = data;
    isql->affected_rows = row_count;

    //
    // to here
    //

    if (retrieval == 1 || (stmt->result && retrieval == 0 &&
                    stmt->result->tmp_rec_len == 0))
    {
        PMEM_FREENUL(stmt->result->list);
        PMEM_FREENUL(stmt->result->field_datatypes);
        PMEM_FREENUL(stmt->result->tmp_recbuf);
        PMEM_FREENUL(stmt->result);

        release_statement(gClients[index].sql, stmt->id);
        gClients[index].store_or_use_result = MDB_UINT_MAX;
    }

    return 1;

  RETURN_LABEL:

    while (rst_pg_head)
    {
        cur_pg = rst_pg_head;
        rst_pg_head = cur_pg->next;
        SMEM_FREE(cur_pg);
    }
    if (data)
    {
        SMEM_FREENUL(data->data_buf);
        SMEM_FREENUL(data->rows_buf);
    }
    SMEM_FREENUL(data);

    if (ret < 0)
    {
        get_last_error(isql, get_error);
        ret = 0;
    }

    return ret;
}

int
isql_FETCHROW_real(iSQL_RES * res)
{
    int index = THREAD_HANDLE;
    int ret;
    int get_error = 1;
    int field_count;
    int row_count;
    void *rst_page_list;
    int use_eof = 0;
    iSQL_DATA *data = NULL;
    iSQL_ROWS *data_cursor = NULL;
    iSQL_RESULT_PAGE *rst_pg_head = NULL;
    iSQL_RESULT_PAGE *cur_pg = NULL;
    char *cursor;
    int i, j;
    iSQL *isql = res->isql;
    T_STATEMENT *stmt;

    if (gClients[index].status == iSQL_STAT_CONNECTED)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTLOGON, 0);
        ret = SQL_E_NOTLOGON;
        goto RETURN_LABEL;
    }

    if (gClients[index].store_or_use_result == MDB_UINT_MAX)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ILLEGALCALLSEQUENCE, 0);
        ret = SQL_E_ILLEGALCALLSEQUENCE;
        goto RETURN_LABEL;
    }

    stmt = get_statement(gClients[index].sql,
            gClients[index].store_or_use_result);
    if (stmt == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_STATEMENTNOTFOUND, 0);
        ret = SQL_E_STATEMENTNOTFOUND;
        goto RETURN_LABEL;
    }

    if (stmt->status != iSQL_STAT_USE_RESULT)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTAVAILABLEHANDLE, 0);
        ret = SQL_E_NOTAVAILABLEHANDLE;
        goto RETURN_LABEL;
    }

    if (stmt->result)
    {
        stmt->is_partial_result = 1;

        row_count = SQL_store_result(gClients[index].DBHandle, stmt,
                gClients[index].flags, RES_TYPE_STRING);
        if (row_count > 0)
        {
            ret = SQL_E_SUCCESS;
            field_count = stmt->result->field_count;
            if (stmt->result->tmp_rec_len == 0)
                use_eof = 1;
            else
                use_eof = 0;
        }
        else
        {
            ret = SQL_E_NOERROR;
            field_count = 0;
        }
    }
    else
    {
        ret = 0;
        field_count = 0;

        release_statement(gClients[index].sql, stmt->id);
        gClients[index].store_or_use_result = MDB_UINT_MAX;
        SQL_cleanup(&gClients[index].DBHandle, stmt, MODE_MANUAL, 1);

        goto RETURN_LABEL;
    }

    /* send the pointer of the result page list */
    rst_page_list = stmt->result->list->head;

    if (ret < 0)
    {
        ret = 0;
        goto ERROR_AND_EXIT;
    }

    rst_pg_head = (T_RESULT_PAGE *) rst_page_list;

    data = res->data;
    if (data == NULL)
    {
        data = SMEM_XCALLOC(sizeof(iSQL_DATA));
        if (data == NULL)
        {
            get_error = SQL_E_OUTOFMEMORY;
            ret = 0;
            goto ERROR_AND_EXIT;
        }
    }

    cur_pg = rst_pg_head;
    cursor = &cur_pg->body[0];

    for (i = 0; i < row_count; i++)
    {
        if (i == 0)
        {
            if (data->buf_len < row_count)
            {
                SMEM_FREENUL(data->rows_buf);
                SMEM_FREENUL(data->data_buf);
                data->rows_buf = SMEM_XCALLOC(sizeof(iSQL_ROWS) * row_count);
                if (data->rows_buf != NULL)
                    data->data_buf =
                            SMEM_XCALLOC(sizeof(char *) *
                            row_count * field_count);
                if (data->rows_buf == NULL || data->data_buf == NULL)
                {
                    get_error = SQL_E_OUTOFMEMORY;
                    ret = 0;
                    goto ERROR_AND_EXIT;
                    // if rows_buf alloc'ed and data_buf failed,
                    // leak possible. But second thoughts, it looks O.K.
                }
                data->buf_len = row_count;
            }

            if (data->rows_buf == NULL || data->data_buf == NULL)
            {
                get_error = SQL_E_OUTOFMEMORY;
                ret = 0;
                goto ERROR_AND_EXIT;
            }

            data->data = &data->rows_buf[0];
            data_cursor = data->data;
        }
        else
        {
            data_cursor->next = &data->rows_buf[i];
            data_cursor = data_cursor->next;
        }

        data_cursor->data = &data->data_buf[i * field_count];

        if ((cursor - &cur_pg->body[0]) >= cur_pg->body_size)
        {
            cur_pg = cur_pg->next;
            if (cur_pg == NULL)
            {
                get_error = SQL_E_ERROR;
                ret = 0;
                goto ERROR_AND_EXIT;
            }
            cursor = &cur_pg->body[0];
        }

        for (j = 0; j < field_count; j++)
        {
            if (IS_NBS(res->fields[j].type))
            {
                cursor = (char *) _alignedlen((unsigned long) cursor,
                        sizeof(DB_WCHAR));

                data_cursor->data[j] = cursor;
                cursor +=
                        (sc_wcslen((DB_WCHAR *) cursor) +
                        1) * sizeof(DB_WCHAR);
            }
            else
            {
                data_cursor->data[j] = cursor;
                cursor += (sc_strlen(cursor) + 1);
            }
        }

        /* length of record align to sizeof(DB_INT32) */
        cursor = (char *) _alignedlen((unsigned long) cursor,
                sizeof(MDB_INT32));
    }

    if (data_cursor)
        data_cursor->next = NULL;

    data->result_page_list = rst_pg_head;

    data->row_num = row_count;
    data->field_count = field_count;
    data->use_eof = use_eof;

    isql->data = data;
    isql->affected_rows = -1;

    if (data->row_num != 0)
    {
        res->current_field = 0;
        res->data = res->isql->data;
        res->data_cursor = res->data->data;
        res->row_count += data->row_num;
        res->current_row = NULL;
    }

    //
    // to here
    //

    if (row_count == 0 || stmt->result->tmp_rec_len == 0)
    {
        if (stmt->result->list->tail)
            stmt->result->list->head = stmt->result->list->tail;

        PMEM_FREENUL(stmt->result->list);
        PMEM_FREENUL(stmt->result->field_datatypes);
        PMEM_FREENUL(stmt->result->tmp_recbuf);
        PMEM_FREENUL(stmt->result);
        release_statement(gClients[index].sql, stmt->id);
        gClients[index].store_or_use_result = MDB_UINT_MAX;
        SQL_cleanup(&gClients[index].DBHandle, stmt, MODE_MANUAL, 1);
    }

    return 1;

  RETURN_LABEL:

    get_last_error(isql, get_error);

    return 0;

  ERROR_AND_EXIT:
    while (rst_pg_head)
    {
        cur_pg = rst_pg_head;
        rst_pg_head = cur_pg->next;
        SMEM_FREE(cur_pg);
    }

    get_last_error(isql, get_error);

    return ret;
}

int
isql_FLUSHRESULTSET_real(iSQL * isql)
{
    int index = THREAD_HANDLE;
    int ret;
    T_STATEMENT *stmt;

    if (gClients[index].status == iSQL_STAT_CONNECTED)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTLOGON, 0);
        ret = SQL_E_NOTLOGON;
        goto RETURN_LABEL;
    }

    if (gClients[index].store_or_use_result == MDB_UINT_MAX)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ILLEGALCALLSEQUENCE, 0);
        ret = SQL_E_ILLEGALCALLSEQUENCE;
        goto RETURN_LABEL;
    }

    stmt = get_statement(gClients[index].sql,
            gClients[index].store_or_use_result);
    if (stmt == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_STATEMENTNOTFOUND, 0);
        ret = SQL_E_STATEMENTNOTFOUND;
        goto RETURN_LABEL;
    }

    ret = SQL_E_SUCCESS;

    PMEM_FREENUL(stmt->query);
    PMEM_FREENUL(stmt->fields);

    if (stmt->result)
    {
        RESULT_LIST_Destroy(stmt->result->list);
        PMEM_FREENUL(stmt->result->field_datatypes);
        PMEM_FREENUL(stmt->result->tmp_recbuf);
        PMEM_FREENUL(stmt->result);
    }

    release_statement(gClients[index].sql,
            gClients[index].store_or_use_result);
    gClients[index].store_or_use_result = MDB_UINT_MAX;

  RETURN_LABEL:

    if (ret < 0)
    {
        get_last_error(isql, 1);
        return 0;
    }

    return ret;
}

int
isql_GETLISTFIELDS_real(iSQL * isql, char *table, char *column,
        iSQL_FIELD ** param_fields)
{
    int index = THREAD_HANDLE;
    int ret, field_count, get_error;
    iSQL_FIELD *local_fields = NULL;
    T_STATEMENT *stmt;
    char tablename[MAX_TABLE_NAME_LEN];
    char fieldname[FIELD_NAME_LENG];

    sc_strncpy(tablename, table, MAX_TABLE_NAME_LEN);

    if (column == NULL || column[0] == '\0')
        fieldname[0] = '\0';
    else
        sc_strncpy(fieldname, column, FIELD_NAME_LENG);

    if (gClients[index].status == iSQL_STAT_CONNECTED)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTLOGON, 0);
        ret = SQL_E_NOTLOGON;
        goto RETURN_LABEL;
    }

    if (gClients[index].store_or_use_result != MDB_UINT_MAX)
    {
        stmt = get_statement(gClients[index].sql,
                gClients[index].store_or_use_result);
        if (stmt == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_STATEMENTNOTFOUND,
                    0);
            ret = SQL_E_STATEMENTNOTFOUND;
            goto RETURN_LABEL;
        }
        if (stmt->status == iSQL_STAT_IN_USE)
        {
            SQL_cleanup(&gClients[index].DBHandle, stmt, MODE_AUTO, 1);
            PMEM_FREENUL(stmt->fields);
            stmt->status = iSQL_STAT_READY;
        }
        else
        {
            er_set(ER_ERROR_SEVERITY,
                    ARG_FILE_LINE, SQL_E_NOTAVAILABLEHANDLE, 0);
            ret = SQL_E_NOTAVAILABLEHANDLE;
            goto RETURN_LABEL;
        }
    }
    else
    {
        ret = allocate_statement(gClients[index].sql);
        if (ret < 0)
            goto RETURN_LABEL;

        stmt = gClients[index].sql->stmts->prev;
        stmt->trans_info = &gClients[index].trans_info;
    }

    if (SQL_trans_start(gClients[index].DBHandle, stmt) == RET_ERROR)
    {
        ret = SQL_E_ERROR;
        field_count = 0;
        release_statement(gClients[index].sql, stmt->id);
        gClients[index].store_or_use_result = MDB_UINT_MAX;
        goto RETURN_LABEL;
    }

    field_count = unpack_field_list(gClients[index].DBHandle,
            tablename, fieldname, &local_fields);

    if (gClients[index].flags & OPT_AUTOCOMMIT)
    {
        /* In this case, any one of commit and rollback can be used. */
        SQL_trans_commit(gClients[index].DBHandle, stmt);
    }

    gClients[index].store_or_use_result = stmt->id;

    if (field_count < 0)
    {
        ret = er_errid();
        field_count = 0;
        release_statement(gClients[index].sql, stmt->id);
        gClients[index].store_or_use_result = MDB_UINT_MAX;
        goto RETURN_LABEL;
    }


    ret = 1;

  RETURN_LABEL:

    get_error = 1;

    if (ret < 0)
    {
        ret = 0;
        goto ERROR_AND_EXIT;
    }

    *param_fields = NULL;
    if (field_count > 0)
    {
        *param_fields = SMEM_ALLOC(sizeof(iSQL_FIELD) * field_count);
        if (*param_fields == NULL)
        {
            get_error = SQL_E_OUTOFMEMORY;
            ret = 0;
            goto ERROR_AND_EXIT;
        }

        sc_memcpy((char *) (*param_fields), (char *) (local_fields),
                (sizeof(iSQL_FIELD) * field_count));
    }

    ret = field_count;

    return ret;

  ERROR_AND_EXIT:
    get_last_error(isql, get_error);

    // to here

    if (local_fields)
    {
        PMEM_FREENUL(local_fields);
    }

    return ret;
}

int
isql_BEGINTRANSACTION_real(iSQL * isql)
{
    int index = THREAD_HANDLE;
    int ret = SQL_E_SUCCESS;

    if (index == -1)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_NOTTRANSBEGIN, 0);
        ret = DB_E_NOTTRANSBEGIN;
    }
    else
    {
        if (gClients[index].status == iSQL_STAT_CONNECTED)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTLOGON, 0);
            ret = SQL_E_NOTLOGON;
        }
        else
        {
            ret = SQL_trans_start(gClients[index].DBHandle,
                    gClients[index].sql->stmts);
        }
    }

    get_last_error(isql, 1);

    if (ret < 0)
        return 0;

    return ret;
}

int
isql_CLOSETRANSACTION_real(iSQL * isql, const int mode, int f_flush)
{
    int index = THREAD_HANDLE;
    int i, ret;
    T_STATEMENT *stmt;

    if (gClients[index].status == iSQL_STAT_CONNECTED)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTLOGON, 0);
        ret = SQL_E_NOTLOGON;
        goto RETURN_LABEL;
    }

    stmt = gClients[index].sql->stmts;
    for (i = 0; i < gClients[index].sql->stmt_num; i++)
    {
        close_all_cursor(gClients[index].DBHandle, stmt);
        stmt = stmt->next;
    }

    if (f_flush)
        dbi_FlushBuffer();

    if (mode == 0)
        SQL_trans_commit(gClients[index].DBHandle, gClients[index].sql->stmts);
    else
        SQL_trans_rollback(gClients[index].DBHandle,
                gClients[index].sql->stmts);

    ret = SQL_E_SUCCESS;

  RETURN_LABEL:

    if (ret < 0)
    {
        get_last_error(isql, 1);
        return 0;
    }

    return ret;
}


int
isql_GETPLAN_real(iSQL * isql, iSQL_STMT * stmt, char **plan_string)
{
    int index = THREAD_HANDLE;
    int ret = 0;
    T_STATEMENT *statement;
    char *p = NULL;

    if (gClients[index].status == iSQL_STAT_CONNECTED)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTLOGON, 0);
        ret = SQL_E_NOTLOGON;
        goto RETURN_LABEL;
    }

    if (stmt != NULL)
    {
        isql = stmt->isql;
        statement = get_statement(gClients[index].sql, stmt->stmt_id);
        if (statement == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_STATEMENTNOTFOUND,
                    0);
            ret = SQL_E_STATEMENTNOTFOUND;
            goto RETURN_LABEL;
        }
    }
    else
    {
        statement = &gClients[index].sql->stmts[0];
        if (statement == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_STATEMENTNOTFOUND, 0);
            ret = SQL_E_STATEMENTNOTFOUND;
            goto RETURN_LABEL;
        }
    }

    p = SQL_get_plan_string(&gClients[index].DBHandle, statement);
    if (p == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NULLPLANSTRING, 0);
        ret = SQL_E_NULLPLANSTRING;
        goto RETURN_LABEL;
    }

    *plan_string = sc_strdup(p);

  RETURN_LABEL:

    PMEM_FREENUL(p);

    if (ret < 0)
    {
        get_last_error(isql, 1);
        return 0;
    }

    return SQL_E_SUCCESS;
}

int
isql_QUERY_real(iSQL * isql, char *query, int len)
{
    int index = THREAD_HANDLE;
    char *qptr;
    int i, selection_len, query_len, field_count = 0, affected_rows = 0, ret;
    int set_flag = 0, on_or_off = 0;
    OID affected_rid = NULL_OID;
    isql_stmt_type sqltype;
    t_fieldinfo *field = NULL;
    T_STATEMENT *stmt = NULL, *tmp;
    T_LIST_SELECTION *selection;
    T_EXPRESSIONDESC *expr;
    T_QUERYRESULT *qres;
    T_ATTRDESC *attr;
    T_SELECT *select = NULL;

    check_end_of_query(query, &len);
    query_len = len + 1;
    er_clear();

    if (gClients[index].status == iSQL_STAT_NOT_READY)
    {
#ifdef MDB_DEBUG
        __SYSLOG("iSQL_STAT_NOT_READY %d\n", index);
#endif
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTCONNECTED, 0);
        ret = SQL_E_NOTCONNECTED;
        goto RETURN_LABEL;
    }

    if (gClients[index].status == iSQL_STAT_CONNECTED)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTLOGON, 0);
        ret = SQL_E_NOTLOGON;
        goto RETURN_LABEL;
    }

    if (gClients[index].store_or_use_result != MDB_UINT_MAX)
    {
        stmt = get_statement(gClients[index].sql,
                gClients[index].store_or_use_result);
        if (stmt == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_STATEMENTNOTFOUND,
                    0);
            ret = SQL_E_STATEMENTNOTFOUND;
            goto RETURN_LABEL;
        }

        if (stmt->status == iSQL_STAT_IN_USE)
        {
            SQL_cleanup(&gClients[index].DBHandle, stmt, MODE_AUTO, 1);
            PMEM_FREENUL(stmt->fields);
            stmt->status = iSQL_STAT_READY;
        }
        else
        {
            er_set(ER_ERROR_SEVERITY,
                    ARG_FILE_LINE, SQL_E_NOTAVAILABLEHANDLE, 0);
            ret = SQL_E_NOTAVAILABLEHANDLE;
            goto RETURN_LABEL;
        }
    }
    else
    {
        ret = allocate_statement(gClients[index].sql);
        if (ret < 0)
            goto RETURN_LABEL;
        stmt = gClients[index].sql->stmts->prev;
        stmt->trans_info = &gClients[index].trans_info;
    }

    stmt->qlen = query_len + 2;

    qptr = PMEM_XCALLOC(stmt->qlen + 1);
    if (qptr == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        ret = SQL_E_OUTOFMEMORY;
        release_statement(gClients[index].sql, stmt->id);
        goto RETURN_LABEL;
    }
    sc_memcpy(qptr, query, stmt->qlen - 2);

    sc_strcpy(qptr + stmt->qlen - 2, ";\t");

    stmt->query = qptr;
    stmt->sqltype = ST_NONE;
    gClients[index].store_or_use_result = stmt->id;

    ret = SQL_prepare(&gClients[index].DBHandle, stmt, gClients[index].flags);

    stmt->do_recompile = MDB_FALSE;

    if (ret == RET_ERROR)
    {
        ret = er_errid();
        goto ERROR_LABEL;
    }

    stmt->status = iSQL_STAT_IN_USE;

    switch (stmt->sqltype)
    {
    case ST_SELECT:
        if (stmt->u._select.param_count != 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_CANNOTBINDING, 0);
            ret = SQL_E_CANNOTBINDING;
            goto RETURN_LABEL;
        }
        break;
    case ST_INSERT:
        if (stmt->u._insert.param_count != 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_CANNOTBINDING, 0);
            ret = SQL_E_CANNOTBINDING;
            goto RETURN_LABEL;
        }
        break;
    case ST_UPDATE:
        if (stmt->u._update.param_count != 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_CANNOTBINDING, 0);
            ret = SQL_E_CANNOTBINDING;
            goto RETURN_LABEL;
        }
        break;
    case ST_DELETE:
        if (stmt->u._delete.param_count != 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_CANNOTBINDING, 0);
            ret = SQL_E_CANNOTBINDING;
            goto RETURN_LABEL;
        }
        break;
    default:
        break;
    }

    switch (stmt->sqltype)
    {
    case ST_SELECT:
        if (stmt->u._select.planquerydesc.querydesc.setlist.len > 0)
        {
            select = stmt->u._select.planquerydesc.querydesc.setlist.list[0]->
                    u.subq;
            selection_len = select->planquerydesc.querydesc.selection.len;
        }
        else
            selection_len =
                    stmt->u._select.planquerydesc.querydesc.selection.len;
        field_count = selection_len;
        affected_rows = SQL_execute(&gClients[index].DBHandle,
                stmt, gClients[index].flags, RES_TYPE_NONE);

        if (affected_rows == RET_ERROR)
        {
            affected_rid = 0;
            ret = er_errid();
            goto RETURN_LABEL;
        }
        break;
    case ST_DESCRIBE:
        if (stmt->u._desc.type == SBT_DESC_TABLE)
        {
            if (stmt->u._desc.describe_table)
            {   // Get User Table's Name
                field_count = 7;
            }
            else
            {
                field_count = 1;
            }
        }
        else if (stmt->u._desc.type == SBT_SHOW_TABLE)
        {
            field_count = 1;
        }
#if defined(USE_DESC_COMPILE_OPT)
        else if (stmt->u._desc.type == SBT_DESC_COMPILE_OPT)
        {
            field_count = 1;
        }
#endif
        break;
    case ST_COMMIT:
    case ST_COMMIT_FLUSH:
    case ST_ROLLBACK_FLUSH:
    case ST_ROLLBACK:
        tmp = gClients[index].sql->stmts;
        for (i = 0; i < gClients[index].sql->stmt_num; i++)
        {
            close_all_cursor(gClients[index].DBHandle, tmp);
            tmp = tmp->next;
        }
        /* fallthrough */
    case ST_INSERT:
    case ST_UPSERT:
    case ST_UPDATE:
    case ST_DELETE:
    case ST_CREATE:
    case ST_DROP:
    case ST_RENAME:
    case ST_ALTER:
    case ST_TRUNCATE:
    case ST_ADMIN:
        affected_rows = SQL_execute(&gClients[index].DBHandle,
                stmt, gClients[index].flags, RES_TYPE_NONE);

        if (affected_rows == RET_ERROR)
        {
            affected_rid = 0;

            ret = er_errid();
            goto ERROR_LABEL;
        }
        break;
    case ST_SET:
        switch (stmt->u._set.type)
        {
        case SST_AUTOCOMMIT:
            tmp = gClients[index].sql->stmts;
            for (i = 0; i < gClients[index].sql->stmt_num; i++)
            {
                close_all_cursor(gClients[index].DBHandle, tmp);
                tmp = tmp->next;
            }
            SQL_trans_commit(gClients[index].DBHandle, stmt);
            set_flag = OPT_AUTOCOMMIT;
            on_or_off = stmt->u._set.u.on_off;
            if (stmt->u._set.u.on_off)
                gClients[index].flags |= OPT_AUTOCOMMIT;
            else
                gClients[index].flags &= ~OPT_AUTOCOMMIT;
            break;
        case SST_TIME:
            set_flag = OPT_TIME;
            on_or_off = stmt->u._set.u.on_off;
            if (stmt->u._set.u.on_off)
                gClients[index].flags |= OPT_TIME;
            else
                gClients[index].flags &= ~OPT_TIME;
            break;
        case SST_HEADING:
            set_flag = OPT_HEADING;
            on_or_off = stmt->u._set.u.on_off;
            if (stmt->u._set.u.on_off)
                gClients[index].flags |= OPT_HEADING;
            else
                gClients[index].flags &= ~OPT_HEADING;
            break;
        case SST_FEEDBACK:
            set_flag = OPT_FEEDBACK;
            on_or_off = stmt->u._set.u.on_off;
            if (stmt->u._set.u.on_off)
                gClients[index].flags |= OPT_FEEDBACK;
            else
                gClients[index].flags &= ~OPT_FEEDBACK;
            break;
        case SST_RECONNECT:
            set_flag = OPT_RECONNECT;
            on_or_off = stmt->u._set.u.on_off;
            if (stmt->u._set.u.on_off)
                gClients[index].flags |= OPT_RECONNECT;
            else
                gClients[index].flags &= ~OPT_RECONNECT;
            break;
        case SST_PLAN_ON:
            set_flag = OPT_PLAN_ON;
            on_or_off = 1;
            gClients[index].flags &= ~(OPT_PLAN_OFF + OPT_PLAN_ONLY);
            gClients[index].flags |= OPT_PLAN_ON;
            break;
        case SST_PLAN_OFF:
            set_flag = OPT_PLAN_OFF;
            on_or_off = 1;
            gClients[index].flags &= ~(OPT_PLAN_ON + OPT_PLAN_ONLY);
            gClients[index].flags |= OPT_PLAN_OFF;
            break;
        case SST_PLAN_ONLY:
            set_flag = OPT_PLAN_ONLY;
            on_or_off = 1;
            gClients[index].flags &= ~(OPT_PLAN_ON + OPT_PLAN_OFF);
            gClients[index].flags |= OPT_PLAN_ONLY;
            break;
        default:
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
            ret = SQL_E_ERROR;
            goto ERROR_LABEL;
        }
        break;
    case ST_NONE:
        isql->last_querytype = ST_NONE;
        isql->affected_rows = 0;
        isql->affected_rid = NULL_OID;
        goto ERROR_LABEL;
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
        ret = SQL_E_ERROR;
        goto ERROR_LABEL;
    }

    if (stmt->sqltype == ST_INSERT || stmt->sqltype == ST_UPSERT)
        affected_rid = stmt->affected_rid;
    else
        affected_rid = 0;

    ret = SQL_E_SUCCESS;
    switch (stmt->sqltype)
    {
    case ST_SELECT:
        sqltype = SQL_STMT_SELECT;
        break;
    case ST_INSERT:
        sqltype = SQL_STMT_INSERT;
        break;
    case ST_UPSERT:
        sqltype = SQL_STMT_UPSERT;
        break;
    case ST_UPDATE:
        sqltype = SQL_STMT_UPDATE;
        break;
    case ST_DELETE:
        sqltype = SQL_STMT_DELETE;
        break;
    case ST_CREATE:
        sqltype = SQL_STMT_CREATE;
        break;
    case ST_DROP:
        sqltype = SQL_STMT_DROP;
        break;
    case ST_RENAME:
        sqltype = SQL_STMT_RENAME;
        break;
    case ST_ALTER:
        sqltype = SQL_STMT_ALTER;
        break;
    case ST_TRUNCATE:
        sqltype = SQL_STMT_TRUNCATE;
        break;
    case ST_ADMIN:
        sqltype = SQL_STMT_ADMIN;
        break;
    case ST_COMMIT:
    case ST_COMMIT_FLUSH:
        sqltype = SQL_STMT_COMMIT;
        break;
    case ST_ROLLBACK:
    case ST_ROLLBACK_FLUSH:
        sqltype = SQL_STMT_ROLLBACK;
        break;
    case ST_DESCRIBE:
        sqltype = SQL_STMT_DESCRIBE;
        break;
    case ST_SET:
        sqltype = SQL_STMT_SET;
        break;
    default:
        sqltype = 0;
        break;
    }

    if (stmt->sqltype == ST_SELECT || stmt->sqltype == ST_DESCRIBE)
    {
#ifdef MDB_DEBUG
        sc_assert(field_count, __FILE__, __LINE__);
#endif
        field = PMEM_ALLOC(sizeof(t_fieldinfo) * field_count);
    }
    else
    {
        field = PMEM_ALLOC(sizeof(t_fieldinfo));
    }

    if (!field)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        ret = SQL_E_OUTOFMEMORY;
        goto ERROR_LABEL;
    }

    if (stmt->sqltype == ST_SELECT)
    {       // field list를 보내자.
        if (select)
        {
            selection = &select->planquerydesc.querydesc.selection;
            qres = &select->queryresult;
        }
        else
        {
            selection = &stmt->u._select.planquerydesc.querydesc.selection;
            qres = &stmt->u._select.queryresult;
        }

        for (i = 0; i < selection->len; i++)
        {
            sc_memcpy(field[i].fieldname, qres->list[i].res_name,
                    FIELD_NAME_LENG - 1);
            field[i].fieldname[FIELD_NAME_LENG - 1] = '\0';

            expr = &selection->list[i].expr;
            if (expr->len == 1 &&
                    expr->list[0]->elemtype == SPT_VALUE &&
                    expr->list[0]->u.value.valueclass == SVC_VARIABLE &&
                    expr->list[0]->u.value.attrdesc.table.tablename)
            {
                attr = &expr->list[0]->u.value.attrdesc;
                sc_strncpy(field[i].tablename, attr->table.tablename,
                        MAX_TABLE_NAME_LEN - 1);
                field[i].tablename[MAX_TABLE_NAME_LEN - 1] = '\0';
                sc_strncpy(field[i].base_fieldname, attr->attrname,
                        FIELD_NAME_LENG - 1);
                field[i].base_fieldname[FIELD_NAME_LENG - 1] = '\0';
                field[i].flags = attr->flag;
                if (attr->defvalue.not_null)
                {
                    switch (attr->type)
                    {
                    case DT_TINYINT:
                        sc_sprintf(field[i].default_value,
                                "%d", attr->defvalue.u.i8);
                        break;
                    case DT_SMALLINT:
                        sc_sprintf(field[i].default_value,
                                "%hd", attr->defvalue.u.i16);
                        break;
                    case DT_INTEGER:
                        sc_sprintf(field[i].default_value,
                                "%d", attr->defvalue.u.i32);
                        break;
                    case DT_BIGINT:
                        sc_sprintf(field[i].default_value,
                                I64_FORMAT, attr->defvalue.u.i64);
                        break;
                    case DT_FLOAT:
                        sc_snprintf(field[i].default_value,
                                MAX_DEFAULT_VALUE_LEN, "%g",
                                attr->defvalue.u.f16);
                        break;
                    case DT_DOUBLE:
                        sc_snprintf(field[i].default_value,
                                MAX_DEFAULT_VALUE_LEN, "%g",
                                attr->defvalue.u.f32);
                        break;
                    case DT_DECIMAL:
                        sc_sprintf(field[i].default_value,
                                "%g", attr->defvalue.u.dec);
                        break;
                    case DT_CHAR:
                    case DT_VARCHAR:
                        sc_strncpy(field[i].default_value,
                                attr->defvalue.u.ptr,
                                MAX_DEFAULT_VALUE_LEN - 1);
                        field[i].default_value[MAX_DEFAULT_VALUE_LEN - 1] =
                                '\0';
                        break;
                    case DT_BYTE:
                    case DT_VARBYTE:
                        sc_memcpy(field[i].default_value,
                                attr->defvalue.u.ptr,
                                attr->defvalue.value_len);
                        field[i].default_value[attr->defvalue.value_len] =
                                '\0';
                        // 요넘이.. 0으로 되어서 넘어가는데.. -_-;
                        break;
                    case DT_DATETIME:
                        datetime2char(field[i].default_value,
                                attr->defvalue.u.datetime);
                        break;
                    case DT_DATE:
                        date2char(field[i].default_value,
                                attr->defvalue.u.date);
                        break;
                    case DT_TIME:
                        time2char(field[i].default_value,
                                attr->defvalue.u.time);
                        break;
                    case DT_TIMESTAMP:
                        timestamp2char(field[i].default_value,
                                &attr->defvalue.u.timestamp);
                        break;
                    default:
                        field[i].default_value[0] = '\0';
                        break;
                    }
                }
                else
                    field[i].default_value[0] = '\0';
            }
            else
            {
                field[i].tablename[0] = '\0';
                field[i].base_fieldname[0] = '\0';
                field[i].flags = 0;
                field[i].default_value[0] = '\0';
            }

            field[i].type = qres->list[i].value.valuetype;
            field[i].length = qres->list[i].len;
            field[i].max_length = qres->list[i].len;
            field[i].decimals = 0;
            field[i].buffer_length = SizeOfType[field[i].type];

            switch (qres->list[i].value.valuetype)
            {
            case DT_TINYINT:
                break;
            case DT_SMALLINT:
                break;
            case DT_INTEGER:
                break;
            case DT_BIGINT:
                break;
            case DT_FLOAT:
                break;
            case DT_DOUBLE:
                break;
            case DT_DECIMAL:
                field[i].length = qres->list[i].len + 2;
                field[i].max_length = qres->list[i].len + 2;
                field[i].decimals = qres->list[i].value.attrdesc.dec;
                break;
            case DT_BYTE:
            case DT_VARBYTE:
            case DT_CHAR:
            case DT_NCHAR:
            case DT_VARCHAR:
            case DT_NVARCHAR:
                field[i].max_length = qres->list[i].value.attrdesc.len;
                field[i].buffer_length = qres->list[i].value.value_len;
                break;
            case DT_DATETIME:
                field[i].max_length = MAX_DATETIME_STRING_LEN - 1;
                field[i].buffer_length = qres->list[i].len;
                break;
            case DT_DATE:
                field[i].max_length = MAX_DATE_STRING_LEN - 1;
                field[i].buffer_length = qres->list[i].len;
                break;
            case DT_TIME:
                field[i].max_length = MAX_TIME_STRING_LEN - 1;
                field[i].buffer_length = qres->list[i].len;
                break;
            case DT_TIMESTAMP:
                field[i].max_length = MAX_TIMESTAMP_STRING_LEN - 1;
                field[i].buffer_length = qres->list[i].len;
                break;
            case DT_NULL_TYPE:
                field[i].max_length = sc_strlen(qres->list[i].res_name);
                field[i].buffer_length = 0;
                break;
            default:
                field[i].type = 0;
                break;
            }
        }
    }
    else if (stmt->sqltype == ST_DESCRIBE)
    {
        for (i = 0; i < field_count; i++)
        {
            sc_memset(&field[i], 0, sizeof(t_fieldinfo));
            if (stmt->u._desc.type == SBT_DESC_TABLE)
            {
                switch (i)
                {
                case 0:
                    sc_strcpy(field[i].fieldname, "NAME");
                    if (field_count == 1)
                    {   // GET ONLY USER TABLE's NAME
                        field[i].length = MAX_TABLE_NAME_LEN;
                        field[i].max_length = MAX_TABLE_NAME_LEN;
                    }
                    else
                    {
                        field[i].length = 32;
                        field[i].max_length = 32;
                    }
                    break;
                case 1:
                    sc_strcpy(field[i].fieldname, "TYPE");
                    field[i].length = 32;
                    field[i].max_length = 32;
                    break;
                case 2:
                    sc_strcpy(field[i].fieldname, "NULL");
                    field[i].length = 8;
                    field[i].max_length = 8;
                    break;
                case 3:
                    sc_strcpy(field[i].fieldname, "KEY");
                    field[i].length = 3;
                    field[i].max_length = 3;
                    break;
                case 4:
                    sc_strcpy(field[i].fieldname, "DEFAULT");
                    field[i].length = MAX_DEFAULT_VALUE_LEN;
                    field[i].max_length = MAX_DEFAULT_VALUE_LEN;
                    break;
                case 5:
                    sc_strcpy(field[i].fieldname, "AUTOINCREMENT");
                    field[i].length = 13;
                    field[i].max_length = 13;
                    break;
                case 6:
                    sc_strcpy(field[i].fieldname, "NOLOGGING");
                    field[i].length = 9;
                    field[i].max_length = 9;
                    break;
                default:
                    break;
                }
                field[i].type = DT_CHAR;
                field[i].flags &= ~NULL_BIT;
            }
            else if (stmt->u._desc.type == SBT_SHOW_TABLE)
            {
                sc_strcpy(field[i].fieldname, "#SHOW");
                field[i].length = 8000;
                field[i].max_length = 8000;
                field[i].type = DT_VARCHAR;
                field[i].flags &= ~NULL_BIT;
            }
#if defined(USE_DESC_COMPILE_OPT)
            else if (stmt->u._desc.type == SBT_DESC_COMPILE_OPT)
            {
                sc_strcpy(field[i].fieldname, "COMPILE_OPT");
                field[i].length = 8000;
                field[i].max_length = 8000;
                field[i].type = DT_VARCHAR;
                field[i].flags &= ~NULL_BIT;
            }
#endif
        }
    }

    if (sqltype == SQL_STMT_SELECT || sqltype == SQL_STMT_DESCRIBE)
    {
        if (isql->fields)
        {
            SMEM_FREE(isql->fields);
        }

        isql->field_count = field_count;
        isql->fields = SMEM_ALLOC(sizeof(iSQL_FIELD) * isql->field_count);
        if (!isql->fields)
        {
            isql->field_count = 0;
            ret = 0;
            goto RETURN_LABEL;
        }

        sc_memcpy((char *) (isql->fields), (char *) (field),
                (sizeof(iSQL_FIELD) * isql->field_count));
    }
    else if (sqltype == SQL_STMT_SET)
    {       // if SET
        switch (set_flag)
        {
        case OPT_PLAN_ON:
        case OPT_PLAN_OFF:
        case OPT_PLAN_ONLY:
            isql->flags &= ~(OPT_PLAN_ON + OPT_PLAN_OFF + OPT_PLAN_ONLY);
            isql->flags |= set_flag;
            break;
        default:
            if (on_or_off)
                isql->flags |= set_flag;
            else
                isql->flags &= ~set_flag;
            break;
        }
    }

    isql->last_querytype = sqltype;
    isql->affected_rows = affected_rows;
    isql->affected_rid = affected_rid;

    ret = SQL_E_SUCCESS;

    PMEM_FREENUL(field);

  ERROR_LABEL:
    if (ret < 0 || (stmt->sqltype != ST_SELECT
                    && stmt->sqltype != ST_DESCRIBE
                    && stmt->sqltype != ST_UPDATE
                    && stmt->sqltype != ST_DELETE))
    {
        PMEM_FREENUL(stmt->query);
        PMEM_FREENUL(stmt->fields);
        SQL_cleanup(&gClients[index].DBHandle, stmt, MODE_AUTO, 1);
        release_statement(gClients[index].sql,
                gClients[index].store_or_use_result);
        gClients[index].store_or_use_result = MDB_UINT_MAX;
    }

    sc_assert(field == NULL, __FILE__, __LINE__);

  RETURN_LABEL:



    //
    // to here
    //

    if (ret < 0)
    {
        get_last_error(isql, 1);

        return 0;
    }

    return ret;
}

int
isql_DESCUPDATERID_real(iSQL * isql, OID rid, int numfields,
        iSQL_FIELD * fielddesc, iSQL_BIND * fieldvalue)
{
    int index = THREAD_HANDLE;
    int flag = 0;
    int ret = 0;
    int posi;
    int i;
    int tot_size;
    int implicit_savepoint_flag = 0;
    struct UpdateValue upd_values;
    T_STATEMENT *statement = NULL;
    char *p = NULL;
    T_DESCUPDATERID *packed;
    int affected_rows = 0;
    iSQL_BIND *bind = fieldvalue;
    t_parameter *param;

    const int fixed = sizeof(T_DESCUPDATERID) - sizeof(t_parameter);
    const int fixed_for_parameter = sizeof(t_parameter) - FOUR_BYTE_ALIGN;

    tot_size = 0;
    for (i = 0; i < numfields; i++)
    {
        tot_size += fixed_for_parameter;
        tot_size +=
                MDB_ALIGN(DB_BYTE_LENG(bind[i].buffer_type,
                        bind[i].buffer_length));
    }

    p = SMEM_ALLOC(fixed + tot_size);
    if (p == NULL)
    {
        isql->last_errno = SQL_E_OUTOFMEMORY;
        return -1;
    }

    packed = (T_DESCUPDATERID *) p;

    packed->numfields = numfields;
    packed->rid = rid;

    for (i = 0, posi = 0; i < numfields; i++)
    {
        param = (t_parameter *) ((char *) packed->params + posi);

        if (bind[i].is_null)
            param->is_null = *bind[i].is_null;
        else
            param->is_null = 0;

        param->buffer_type = bind[i].buffer_type;
        param->buffer_length = bind[i].buffer_length;
        param->value_length = *bind[i].length;
        sc_memcpy(param->name, fielddesc[i].name, FIELD_NAME_LENG);

        memcpy_func[param->buffer_type] (param->buffer, bind[i].buffer,
                bind[i].buffer_length);
        posi += fixed_for_parameter;
        posi += MDB_ALIGN(DB_BYTE_LENG(bind[i].buffer_type,
                        bind[i].buffer_length));
    }

    if (gClients[index].status == iSQL_STAT_CONNECTED)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTLOGON, 0);
        ret = SQL_E_NOTLOGON;
        goto RETURN_LABEL;
    }

    statement = &gClients[index].sql->stmts[0];
    if (statement == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_STATEMENTNOTFOUND, 0);
        ret = SQL_E_STATEMENTNOTFOUND;
        goto RETURN_LABEL;
    }

    /* Is the following code really needed ? */
    if (SQL_trans_start(isql->handle, statement) == RET_ERROR)
    {
        ret = RET_ERROR;
        goto RETURN_LABEL;
    }

    flag = gClients[index].flags;

    if (!(flag & OPT_AUTOCOMMIT))
    {       /* for statement atomicity */
        dbi_Trans_Implicit_Savepoint(isql->handle);
        implicit_savepoint_flag = 1;
    }

    if (packed->numfields)
    {
        if (set_update_values(&upd_values, packed) == RET_ERROR)
        {
            ret = er_errid();
            goto RETURN_LABEL;
        }
        ret = dbi_Rid_Update(gClients[index].DBHandle, rid, &upd_values);

        dbi_UpdateValue_Destroy(&upd_values);

        if (ret >= 0)
            affected_rows = 1;
        else if (ret == DB_E_NOMORERECORDS)
            affected_rows = 0;
        else if (ret != DB_E_INVALIDPARAM)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            affected_rows = ret;

            goto RETURN_LABEL;
        }
    }

    ret = SQL_E_SUCCESS;

  RETURN_LABEL:

    if (ret < 0)
    {
        /* partial rollback may be more accurate. */
        if (er_errid() != DB_E_DEADLOCKABORT)
        {
            /* In case of deadlock abort,
               the current transaction has already been aborted.
             */
            if (flag & OPT_AUTOCOMMIT)
                SQL_trans_rollback(isql->handle, statement);
            else
            {
                if (implicit_savepoint_flag)
                    SQL_trans_implicit_partial_rollback(isql->handle,
                            statement);
            }
        }
    }
    else
    {
        if (flag & OPT_AUTOCOMMIT)
            SQL_trans_commit(isql->handle, statement);
        else
        {
            if (implicit_savepoint_flag)
                SQL_trans_implicit_savepoint_release(isql->handle, statement);
        }
    }

    SMEM_FREENUL(p);

    if (ret < 0)
    {
        get_last_error(isql, 1);
        return 0;
    }

    isql->affected_rows = affected_rows;

    return 1;

}
