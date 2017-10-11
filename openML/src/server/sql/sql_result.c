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

#include "sql_datast.h"
#include "sql_main.h"

#include "sql_func_timeutil.h"

#include "isql.h"
#include "sql_util.h"

#include "sql_mclient.h"

#include "mdb_er.h"
#include "ErrorCode.h"

iSQL_TIME *char2timest(iSQL_TIME * timest, char *stime, int sleng);

/*****************************************************************************/
//! strcpy_datatype

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param buf(in/out)   :
 * \param bufsz(in)     : 
 * \param field(in)     :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - BYTE, VARBYTE 지원
 *****************************************************************************/
void
strcpy_datatype(char *buf, int bufsz, SYSFIELD_T * field)
{
    switch (field->type)
    {
    case DT_TINYINT:
        sc_strcpy(buf, "TINYINT");
        break;
    case DT_SMALLINT:
        sc_strcpy(buf, "SMALLINT");
        break;
    case DT_INTEGER:
        sc_strcpy(buf, "INT");
        break;
    case DT_BIGINT:
        sc_strcpy(buf, "BIGINT");
        break;
    case DT_FLOAT:
        sc_strcpy(buf, "FLOAT");
        break;
    case DT_DOUBLE:
        sc_strcpy(buf, "DOUBLE");
        break;
    case DT_DECIMAL:
        sc_snprintf(buf, bufsz, "DECIMAL(%d,%d)", field->length, field->scale);
        break;
    case DT_CHAR:
        sc_snprintf(buf, bufsz, "CHAR(%d)", field->length);
        break;
    case DT_VARCHAR:
        sc_snprintf(buf, bufsz, "VARCHAR(%d, %d)", field->fixed_part,
                field->length);
        break;
    case DT_NCHAR:
        sc_snprintf(buf, bufsz, "NCHAR(%d)", field->length);
        break;
    case DT_NVARCHAR:
        sc_snprintf(buf, bufsz, "NVARCHAR(%d, %d)", field->fixed_part >> 1,
                field->length);
        break;
    case DT_BYTE:
        sc_snprintf(buf, bufsz, "BYTE(%d)", field->length);
        break;
    case DT_VARBYTE:
        sc_snprintf(buf, bufsz, "VARBYTE(%d, %d)", field->fixed_part,
                field->length);
        break;
    case DT_DATETIME:
        sc_strcpy(buf, "DATETIME");
        break;
    case DT_DATE:
        sc_strcpy(buf, "DATE");
        break;
    case DT_TIME:
        sc_strcpy(buf, "TIME");
        break;
    case DT_TIMESTAMP:
        sc_strcpy(buf, "TIMESTAMP");
        break;
    case DT_OID:
        sc_strcpy(buf, "OID");
        break;
    default:
#if defined(MDB_DEBUG)
        // debug로 컴파일 한 경우에 assert를 발생
        sc_assert(0, __FILE__, __LINE__);
#endif
        break;
    }
}

#define MAX_DESC_RECSIZE        256

static T_RESULT_INFO *
store_table_desc_result(int handle, T_STATEMENT * stmt, int res_type)
{
    SYSFIELD_T *fieldinfo = NULL;
    SYSTABLE_T *sysTable = NULL;
    T_RESULT_INFO *result = NULL;
    T_RESULT_LIST *list = NULL;

    char *p, recbuf[MAX_DESC_RECSIZE];
    char *target, *null_ptr;
    int fieldnum;
    int *rec_len, real_len;
    int i;

    sc_memset(recbuf, 0, sizeof(recbuf));

    if (stmt->u._desc.describe_table)
    {       // GET the table's info
        /* column name이 존재하는 경우 dbi_Schema_GetSingleFieldsInfo()를
           이용하여 처리 */
        if (stmt->u._desc.describe_column)
        {
            fieldinfo = (SYSFIELD_T *) PMEM_ALLOC(sizeof(SYSFIELD_T));
            if (fieldinfo == NULL)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                goto ERROR_LABEL;
            }

            fieldnum = dbi_Schema_GetSingleFieldsInfo(handle,
                    stmt->u._desc.describe_table,
                    stmt->u._desc.describe_column, fieldinfo);
            if (fieldnum < 1)
            {
                PMEM_FREENUL(fieldinfo);
                if (fieldnum == DB_E_INVALIDPARAM)
                {
                    fieldnum = SQL_E_INVALIDCOLUMN;
                }
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, fieldnum, 0);
                goto ERROR_LABEL;
            }
        }
        else
        {
            fieldnum = dbi_Schema_GetFieldsInfo(handle,
                    stmt->u._desc.describe_table, &fieldinfo);
            if (fieldnum < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, fieldnum, 0);
                goto ERROR_LABEL;
            }
        }


        result = PMEM_XCALLOC(sizeof(T_RESULT_INFO));
        if (result == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            goto ERROR_LABEL;
        }

        if (res_type != RES_TYPE_NONE)
        {
            list = RESULT_LIST_Create(MAX_DESC_RECSIZE);
            if (list == NULL)
            {
                goto ERROR_LABEL;
            }
        }

        if (res_type == RES_TYPE_STRING)
        {

            for (i = 0; i < fieldnum; i++)
            {

                p = recbuf;

                real_len = sc_strlen(fieldinfo[i].fieldName) + 1;

                sc_memcpy(p, fieldinfo[i].fieldName, real_len);
                p += real_len;

                strcpy_datatype(p, MAX_DESC_RECSIZE - (p - recbuf),
                        &fieldinfo[i]);
                p += sc_strlen(p) + 1;

                if (fieldinfo[i].flag & NULL_BIT)
                    sc_strcpy(p, "NULL");
                else
                    sc_strcpy(p, "NOT NULL");
                p += sc_strlen(p) + 1;

                if (fieldinfo[i].flag & PRI_KEY_BIT)
                    sc_strcpy(p, "PRI");
                else
                    p[0] = '\0';

                p += sc_strlen(p) + 1;

                real_len = sc_strlen(fieldinfo[i].defaultValue) + 1;

                sc_memcpy(p, fieldinfo[i].defaultValue, real_len);
                p += real_len;

                if (fieldinfo[i].flag & AUTO_BIT)
                {
                    sc_strcpy(p, "AUTOINCREMENT");
                    p += 13;
                }
                else
                {
                    p[0] = '\0';
                }

                ++p;

                if (fieldinfo[i].flag & NOLOGGING_BIT)
                {
                    sc_strcpy(p, "NOLOGGING");
                }
                else
                {
                    p[0] = '\0';
                }

                p += sc_strlen(p) + 1;

                real_len = p - recbuf;

                real_len = _alignedlen(real_len, sizeof(MDB_INT32));

                if (RESULT_LIST_Add(list, recbuf, real_len) < 0)
                {
                    goto ERROR_LABEL;
                }
            }

        }
        else if (res_type == RES_TYPE_BINARY)
        {
            for (i = 0; i < fieldnum; i++)
            {
                p = recbuf;

                sc_memset(p, 0, OID_SIZE + sizeof(int) + 1);
                rec_len = (int *) (p + OID_SIZE);
                null_ptr = (char *) p + OID_SIZE + sizeof(int);
                target = (char *) p + OID_SIZE + sizeof(int) + 1;

                (*rec_len) = OID_SIZE + sizeof(int) + 1;


                sc_strcpy(target, fieldinfo[i].fieldName);
                target += 32;
                (*rec_len) += 32;

                strcpy_datatype(target, MAX_DESC_RECSIZE - (*rec_len),
                        &fieldinfo[i]);
                target += 32;
                (*rec_len) += 32;

                if (fieldinfo[i].flag & NULL_BIT)
                    sc_strcpy(target, "NULL");
                else
                    sc_strcpy(target, "NOT NULL");

                target += 8;
                (*rec_len) += 8;

                if (fieldinfo[i].flag & PRI_KEY_BIT)
                {
                    sc_strcpy(target, "PRI");
                    target += 3;
                    (*rec_len) += 3;
                }
                else
                    *null_ptr |= 0x08;

                if (fieldinfo[i].defaultValue[0])
                {
                    sc_snprintf(target,
                            MAX_DEFAULT_VALUE_LEN + 1, "%s",
                            fieldinfo[i].defaultValue);

                    target += MAX_DEFAULT_VALUE_LEN;
                    (*rec_len) += MAX_DEFAULT_VALUE_LEN;
                }
                else
                    *null_ptr |= 0x10;

                if (fieldinfo[i].flag & AUTO_BIT)
                {
                    sc_strcpy(target, "AUTOINCREMENT");
                    target += 13;
                    (*rec_len) += 13;
                }
                else
                    *null_ptr |= 0x20;

                if (fieldinfo[i].flag & NOLOGGING_BIT)
                {
                    sc_strcpy(target, "NOLOGGING");
                    target += 9;
                    (*rec_len) += 9;
                }
                else
                {
                    *null_ptr |= 0x40;
                }

                if (RESULT_LIST_Add(list, recbuf, *rec_len) < 0)
                {
                    goto ERROR_LABEL;
                }
            }
        }

        result->row_num = fieldnum;
        result->field_count = 7;
        result->list = list;

    }
    else
    {       // Get User Table's Name
        fieldnum = dbi_Schema_GetUserTablesInfo(handle, &sysTable);
        if (fieldnum < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, fieldnum, 0);
            goto ERROR_LABEL;
        }

        result = PMEM_XCALLOC(sizeof(T_RESULT_INFO));
        if (result == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            goto ERROR_LABEL;
        }

        if (res_type != RES_TYPE_NONE)
        {
            list = RESULT_LIST_Create(MAX_DESC_RECSIZE);
            if (list == NULL)
            {
                goto ERROR_LABEL;
            }
        }

        if (res_type == RES_TYPE_STRING)
        {
            for (i = 0; i < fieldnum; i++)
            {
                p = recbuf;
                real_len = sc_strlen(sysTable[i].tableName) + 1;
                sc_memcpy(p, sysTable[i].tableName, real_len);
                p += real_len;

                real_len = _alignedlen(real_len, sizeof(MDB_INT32));

                if (RESULT_LIST_Add(list, recbuf, real_len) < 0)
                {
                    goto ERROR_LABEL;
                }
            }
        }
        else if (res_type == RES_TYPE_BINARY)
        {

            for (i = 0; i < fieldnum; i++)
            {
                p = recbuf;
                sc_memset(p, 0, OID_SIZE + sizeof(int) + 1);
                rec_len = (int *) (p + OID_SIZE);
                null_ptr = (char *) p + OID_SIZE + sizeof(int);
                target = (char *) p + OID_SIZE + sizeof(int) + 1;

                (*rec_len) = OID_SIZE + sizeof(int) + 1;

                sc_strcpy(target, sysTable[i].tableName);
                target += REL_NAME_LENG;
                (*rec_len) += REL_NAME_LENG;

                if (RESULT_LIST_Add(list, recbuf, *rec_len) < 0)
                {
                    goto ERROR_LABEL;
                }
            }
        }
        result->row_num = fieldnum;
        result->field_count = 1;
        result->list = list;
    }
    result->fetch_cnt = result->row_num + 1;

    PMEM_FREENUL(fieldinfo);
    PMEM_FREENUL(sysTable);
    return result;

  ERROR_LABEL:

    PMEM_FREENUL(fieldinfo);
    if (result)
        PMEM_FREENUL(result->field_datatypes);
    PMEM_FREENUL(result);
    PMEM_FREENUL(sysTable);
    RESULT_LIST_Destroy(list);

    return NULL;
}

static T_RESULT_INFO *
store_table_show_result(int handle, T_STATEMENT * stmt, int res_type)
{
    T_RESULT_INFO *result = NULL;
    T_RESULT_LIST *list = NULL;
    T_RESULT_SHOW res_show;
    char *recbuf = NULL, *p, *target;
    int *rec_len;
    int real_len, row_num = 0;
    int ret;
    char *str;

    sc_memset(&res_show, 0, sizeof(T_RESULT_SHOW));
    res_show.fd = -1;

    if (res_type == RES_TYPE_STRING)
    {
        real_len = _alignedlen(MAX_SHOW_RECSIZE + 1, sizeof(MDB_INT32));
    }
    else if (res_type == RES_TYPE_BINARY)
    {
        real_len = MAX_SHOW_RECSIZE + INT_SIZE + OID_SIZE + INT_SIZE + 1;
    }
    else
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto ERROR_LABEL;
    }

    recbuf = PMEM_ALLOC(real_len);
    if (recbuf == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto ERROR_LABEL;
    }

    sc_memset(recbuf, 0, real_len);

    result = PMEM_XCALLOC(sizeof(T_RESULT_INFO));
    if (result == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto ERROR_LABEL;
    }

    list = RESULT_LIST_Create(real_len);
    if (list == NULL)
    {
        goto ERROR_LABEL;
    }

    if (stmt->u._desc.describe_table)
    {
        ret = dbi_Relation_Show(handle, stmt->u._desc.describe_table,
                &res_show, 0);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            goto ERROR_LABEL;
        }
    }
    else
    {
        ret = dbi_All_Relation_Show(handle, &res_show);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            goto ERROR_LABEL;
        }
    }

    str = res_show.rec_buf;
    while (str)
    {
        real_len = MAX_SHOW_RECSIZE;
        if (res_type == RES_TYPE_STRING)
        {
            sc_memcpy(recbuf, str, real_len);
            real_len = sc_strlen(recbuf) + 1;
            real_len = _alignedlen(real_len, sizeof(MDB_INT32));
        }
        else
        {
            p = recbuf;
            sc_memset(p, 0, OID_SIZE + sizeof(int) + 1);
            rec_len = (int *) (p + OID_SIZE);
            target = (char *) p + OID_SIZE + sizeof(int) + 1;

            (*rec_len) = OID_SIZE + sizeof(int) + 1;

            sc_memcpy(target, &real_len, INT_SIZE);
            target += INT_SIZE;
            sc_memcpy(target, str, real_len);

            (*rec_len) += INT_SIZE + real_len;
            real_len = *rec_len;
        }

        if (RESULT_LIST_Add(list, recbuf, real_len) < 0)
        {
            goto ERROR_LABEL;
        }

        ++row_num;

        res_show.max_rec_len -= MAX_SHOW_RECSIZE;
        if (res_show.max_rec_len <= 0)
            break;
        str += MAX_SHOW_RECSIZE;
    }

    PMEM_FREENUL(res_show.rec_buf);
    PMEM_FREENUL(recbuf);
    result->row_num = row_num;
    result->field_count = 1;
    result->list = list;
    result->fetch_cnt = result->row_num + 1;

    return result;

  ERROR_LABEL:

    PMEM_FREENUL(res_show.rec_buf);
    PMEM_FREENUL(recbuf);
    PMEM_FREENUL(result);
    RESULT_LIST_Destroy(list);

    return NULL;
}

#if defined(USE_DESC_COMPILE_OPT)
static T_RESULT_INFO *
store_desc_compile_opt_result(int res_type)
{
    T_RESULT_INFO *result = NULL;
    T_RESULT_LIST *list = NULL;
    char *recbuf = NULL, *p, *target;
    int *rec_len;
    int real_len, row_num = 0;
    char *str;

    if (res_type == RES_TYPE_STRING)
    {
        real_len = _alignedlen(MAX_SHOW_RECSIZE + 1, sizeof(MDB_INT32));
    }
    else if (res_type == RES_TYPE_BINARY)
    {
        real_len = MAX_SHOW_RECSIZE + INT_SIZE + OID_SIZE + INT_SIZE + 1;
    }
    else
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto ERROR_LABEL;
    }

    recbuf = PMEM_ALLOC(real_len);
    if (recbuf == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto ERROR_LABEL;
    }

    sc_memset(recbuf, 0, real_len);

    result = PMEM_XCALLOC(sizeof(T_RESULT_INFO));
    if (result == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto ERROR_LABEL;
    }

    list = RESULT_LIST_Create(real_len);
    if (list == NULL)
    {
        goto ERROR_LABEL;
    }

    while (compileOpt[row_num])
    {
        str = compileOpt[row_num];
        real_len = MAX_SHOW_RECSIZE;
        if (res_type == RES_TYPE_STRING)
        {
            sc_memcpy(recbuf, str, real_len);
            real_len = sc_strlen(recbuf) + 1;
            real_len = _alignedlen(real_len, sizeof(MDB_INT32));
        }
        else
        {
            p = recbuf;
            sc_memset(p, 0, OID_SIZE + sizeof(int) + 1);
            rec_len = (int *) (p + OID_SIZE);
            target = (char *) p + OID_SIZE + sizeof(int) + 1;

            (*rec_len) = OID_SIZE + sizeof(int) + 1;

            sc_memcpy(target, &real_len, INT_SIZE);
            target += INT_SIZE;
            sc_memcpy(target, str, real_len);

            (*rec_len) += INT_SIZE + real_len;
            real_len = *rec_len;
        }

        if (RESULT_LIST_Add(list, recbuf, real_len) < 0)
        {
            goto ERROR_LABEL;
        }
        ++row_num;
    }

    PMEM_FREENUL(recbuf);
    result->row_num = row_num;
    result->field_count = 1;
    result->list = list;
    result->fetch_cnt = result->row_num + 1;

    return result;

  ERROR_LABEL:

    PMEM_FREENUL(recbuf);
    PMEM_FREENUL(result);
    RESULT_LIST_Destroy(list);

    return NULL;
}
#endif
/*****************************************************************************/
//! store_desc_result 

/*! \breif      desc 명령어 수행 결과를 담아간다
 ************************************
 * \param handle(in)    :
 * \param stmt(in/out)  : 
 * \param res_type(in)  :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *  - table 명이 안들어온 경우 user table을 return 하도록 수정한다
 *****************************************************************************/
static T_RESULT_INFO *
store_desc_result(int handle, T_STATEMENT * stmt, int res_type)
{
    switch (stmt->u._desc.type)
    {
    case SBT_DESC_TABLE:
        return store_table_desc_result(handle, stmt, res_type);

#if defined(USE_DESC_COMPILE_OPT)
    case SBT_DESC_COMPILE_OPT:
        return store_desc_compile_opt_result(res_type);
#endif

    case SBT_SHOW_TABLE:
        return store_table_show_result(handle, stmt, res_type);

    default:
        return NULL;
    }
}

/*****************************************************************************/
//! bin_result_record_len

/*! \breif  binary resultset의 record's length를 구하는 부분임
 ************************************
 * \param qres(in)      :
 * \param is_subq(in)   : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *  - BYTE/VARBYTE 지원
 *
 *****************************************************************************/
int
bin_result_record_len(T_QUERYRESULT * qres, int is_subq)
{
    // bi_rec_len에 저장되는 내용은
    // 처음 sizeof(int) : 전체 버퍼의 크기
    // 다음 n bytes : null value인지 나타내는 bit(1byte에 8개의 칼럼이 배당)
    // 마지막 : 실제 데이터가 공백없이 차례로 저장
    // row length + null area
    int bi_rec_len = sizeof(int) + (((qres->len - 1) >> 3) + 1);
    int i;

    bi_rec_len += sizeof(OID);

    for (i = 0; i < qres->len; i++)
    {
        switch (qres->list[i].value.valuetype)
        {
        case DT_TINYINT:
        case DT_SMALLINT:
        case DT_INTEGER:
        case DT_BIGINT:
        case DT_OID:
        case DT_FLOAT:
        case DT_DOUBLE:
        case DT_DECIMAL:
            bi_rec_len += SizeOfType[qres->list[i].value.valuetype];
            break;
        case DT_CHAR:
            bi_rec_len += qres->list[i].len;
            break;
        case DT_NCHAR:
            bi_rec_len += qres->list[i].len * sizeof(DB_WCHAR);
            break;
        case DT_VARCHAR:
            bi_rec_len += qres->list[i].len + INT_SIZE;
            break;
        case DT_NVARCHAR:
            bi_rec_len += qres->list[i].len * sizeof(DB_WCHAR) + INT_SIZE;
            break;
        case DT_BYTE:
        case DT_VARBYTE:
            bi_rec_len += INT_SIZE + qres->list[i].len;
            break;
        case DT_DATETIME:
            if (is_subq)
                bi_rec_len += sizeof(char) * MAX_DATETIME_FIELD_LEN;
            else
                bi_rec_len += sizeof(iSQL_TIME);
            break;
        case DT_DATE:
            if (is_subq)
                bi_rec_len += sizeof(char) * MAX_DATE_FIELD_LEN;
            else
                bi_rec_len += sizeof(iSQL_TIME);
            break;
        case DT_TIME:
            if (is_subq)
                bi_rec_len += sizeof(char) * MAX_TIME_FIELD_LEN;
            else
                bi_rec_len += sizeof(iSQL_TIME);
            break;
        case DT_TIMESTAMP:
            if (is_subq)
                bi_rec_len += sizeof(t_astime);
            else
                bi_rec_len += sizeof(iSQL_TIME);
            break;
        default:
            break;
        }
    }

    return bi_rec_len;
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
get_result_column_offset(T_QUERYRESULT * qres, int pos, int is_subq)
{
    // bi_rec_len에 저장되는 내용은
    // 처음 sizeof(int) : 전체 버퍼의 크기
    // 다음 n bytes : null value인지 나타내는 bit(1byte에 8개의 칼럼이 배당)
    // 마지막 : 실제 데이터가 공백없이 차례로 저장
    // row length + null area
    int offset = sizeof(int) + (((qres->len - 1) >> 3) + 1);
    int i;

    if (qres->len < pos)
        return -1;

    for (i = 0; i < pos - 1; i++)
    {
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
        case DT_OID:
            break;
        case DT_FLOAT:
            break;
        case DT_DOUBLE:
            break;
            offset += SizeOfType[qres->list[i].value.valuetype];
            break;
        case DT_CHAR:
        case DT_VARCHAR:
            offset += qres->list[i].len;
            break;
        case DT_NCHAR:
        case DT_NVARCHAR:
            offset += qres->list[i].len * sizeof(DB_WCHAR);
            break;
        case DT_DATETIME:
            if (is_subq)
                offset += sizeof(char) * MAX_DATETIME_FIELD_LEN;
            else
                offset += sizeof(iSQL_TIME);
            break;
        case DT_DATE:
            if (is_subq)
                offset += sizeof(char) * MAX_DATE_FIELD_LEN;
            else
                offset += SSIZE_DATE;
            break;
        case DT_TIME:
            if (is_subq)
                offset += sizeof(char) * MAX_TIME_FIELD_LEN;
            else
                offset += SSIZE_TIME;
            break;
        case DT_TIMESTAMP:
            if (is_subq)
                offset += sizeof(t_astime);
            else
                offset += sizeof(iSQL_TIME);
            break;
        default:
            break;
        }
    }

    return offset;
}

/*****************************************************************************/
//! copy_one_binary_column 

/*! \breif  result set을 binary type으로 전달하는 경우 사용
 ************************************
 * \param target() :
 * \param rescol() : 
 * \param is_subq() :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
*   - reference : stmt_fetch_row()
 *  - called : store_select_binary_result()
 *  - BYTE/VARBYTE TYPE : 길이 정의(4 bytes)를 붙여서 전달함
 *****************************************************************************/
int
copy_one_binary_column(char *target, T_RESULTCOLUMN * rescol, int is_subq)
{
    int col_len;
    char buf[64];
    iSQL_TIME timest;

    switch (rescol->value.valuetype)
    {
    case DT_TINYINT:
        target[0] = rescol->value.u.i8;
        col_len = sizeof(tinyint);
        break;
    case DT_SMALLINT:
        sc_memcpy(target, &rescol->value.u.i16, sizeof(smallint));
        col_len = sizeof(smallint);
        break;
    case DT_INTEGER:
        sc_memcpy(target, &rescol->value.u.i32, sizeof(int));
        col_len = sizeof(int);
        break;
    case DT_BIGINT:
        sc_memcpy(target, &rescol->value.u.i64, sizeof(bigint));
        col_len = sizeof(bigint);
        break;
    case DT_OID:
        sc_memcpy(target, &rescol->value.u.oid, sizeof(OID));
        col_len = sizeof(OID);
        break;
    case DT_FLOAT:
        sc_memcpy(target, &rescol->value.u.f16, sizeof(float));
        col_len = sizeof(float);
        break;
    case DT_DOUBLE:
        sc_memcpy(target, &rescol->value.u.f32, sizeof(double));
        col_len = sizeof(double);
        break;
    case DT_DECIMAL:
        sc_memcpy(target, &rescol->value.u.dec, sizeof(decimal));
        col_len = sizeof(decimal);
        break;
    case DT_CHAR:
        sc_memcpy(target, rescol->value.u.ptr, rescol->len);
        col_len = rescol->len;
        break;
    case DT_NCHAR:
        sc_wmemcpy((DB_WCHAR *) target, rescol->value.u.Wptr, rescol->len);
        /* pay attention!!!  col_len is a real memory size */
        col_len = rescol->len * sizeof(DB_WCHAR);
        break;
    case DT_VARCHAR:
        if (!is_subq)
        {
            if (rescol->value.value_len == 0 && rescol->value.u.ptr[0])
                rescol->value.value_len =
                        strlen_func[rescol->value.valuetype] (rescol->value.u.
                        ptr);

            sc_memcpy(target, &(rescol->value.value_len), INT_SIZE);
            sc_memcpy(target + INT_SIZE, rescol->value.u.ptr,
                    rescol->value.value_len);
            col_len = rescol->value.value_len + INT_SIZE;
        }
        else
        {
            sc_memcpy(target, rescol->value.u.ptr, rescol->len);
            col_len = rescol->len;
        }
        break;
    case DT_NVARCHAR:
        if (!is_subq)
        {
            if (rescol->value.value_len == 0 && rescol->value.u.ptr[0])
                rescol->value.value_len =
                        strlen_func[rescol->value.valuetype] (rescol->value.u.
                        ptr);

            sc_memcpy(target, &(rescol->value.value_len), INT_SIZE);
            sc_wmemcpy((DB_WCHAR *) (target + INT_SIZE), rescol->value.u.Wptr,
                    rescol->value.value_len);
            col_len = rescol->value.value_len * sizeof(DB_WCHAR) + INT_SIZE;
        }
        else
        {
            sc_wmemcpy((DB_WCHAR *) target, rescol->value.u.Wptr, rescol->len);
            /* pay attention!!!
               col_len is a real memory size */
            col_len = rescol->len * sizeof(DB_WCHAR);
        }
        break;
    case DT_BYTE:
    case DT_VARBYTE:
        sc_memcpy(target, &(rescol->value.value_len), INT_SIZE);
        sc_memcpy(target + INT_SIZE, rescol->value.u.ptr,
                rescol->value.value_len);
        col_len = rescol->value.value_len + INT_SIZE;
        break;
    case DT_DATETIME:
        if (is_subq)
        {
            col_len = sizeof(char) * MAX_DATETIME_FIELD_LEN;
            sc_memcpy(target, rescol->value.u.datetime, col_len);
        }
        else
        {
            datetime2char(buf, rescol->value.u.datetime);
            char2timest(&timest, buf, sc_strlen(buf));
            sc_memcpy(target, &timest, sizeof(iSQL_TIME));
            col_len = sizeof(iSQL_TIME);
        }
        break;
    case DT_DATE:
        if (is_subq)
        {
            col_len = sizeof(char) * MAX_DATE_FIELD_LEN;
            sc_memcpy(target, rescol->value.u.date, col_len);
        }
        else
        {
            date2char(buf, rescol->value.u.date);
            char2timest(&timest, buf, sc_strlen(buf));
            sc_memcpy(target, &timest, sizeof(iSQL_TIME));
            col_len = sizeof(iSQL_TIME);
        }
        break;
    case DT_TIME:
        if (is_subq)
        {
            col_len = sizeof(char) * MAX_TIME_FIELD_LEN;
            sc_memcpy(target, rescol->value.u.time, col_len);
        }
        else
        {
            time2char(buf, rescol->value.u.time);
            char2timest(&timest, buf, sc_strlen(buf));
            sc_memcpy(target, &timest, sizeof(iSQL_TIME));
            col_len = sizeof(iSQL_TIME);
        }
        break;
    case DT_TIMESTAMP:
        if (is_subq)
        {
            col_len = sizeof(t_astime);
            sc_memcpy(target, &rescol->value.u.timestamp, col_len);
        }
        else
        {
            timestamp2timest(&timest, &rescol->value.u.timestamp);
            sc_memcpy(target, &timest, sizeof(iSQL_TIME));
            col_len = sizeof(iSQL_TIME);
        }
        break;
    default:
        col_len = 0;
        break;
    }       // END: switch()

    return col_len;
}

/*****************************************************************************/
//! one_binary_column_length

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param rescol(in/out)    :
 * \param is_subq(in)       : 
 ************************************
 * \return column 길이
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
/* length is a byte length. */
int
one_binary_column_length(T_RESULTCOLUMN * rescol, int is_subq)
{
    switch (rescol->value.valuetype)
    {
    case DT_TINYINT:
    case DT_SMALLINT:
    case DT_INTEGER:
    case DT_BIGINT:
    case DT_OID:
    case DT_FLOAT:
    case DT_DOUBLE:
    case DT_DECIMAL:
        return SizeOfType[rescol->value.valuetype];
    case DT_CHAR:
    case DT_VARCHAR:
    case DT_BYTE:
    case DT_VARBYTE:
        return rescol->len;
    case DT_NCHAR:
    case DT_NVARCHAR:
        return rescol->len * sizeof(DB_WCHAR);
    case DT_DATETIME:
        if (is_subq)
            return sizeof(char) * MAX_DATETIME_FIELD_LEN;
        else
            return sizeof(iSQL_TIME);
    case DT_DATE:
        if (is_subq)
            return sizeof(char) * MAX_DATE_FIELD_LEN;
        else
            return SSIZE_DATE;
    case DT_TIME:
        if (is_subq)
            return sizeof(char) * MAX_TIME_FIELD_LEN;
        else
            return SSIZE_TIME;
    case DT_TIMESTAMP:
        if (is_subq)
            return sizeof(t_astime);
        else
            return sizeof(iSQL_TIME);
    default:
        return 0;
    }       // END: switch()
}

/*****************************************************************************/
//! store_select_string_result 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in) :
 * \param stmt(in/out) : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static T_RESULT_INFO *
store_select_string_result(int handle, T_STATEMENT * stmt)
{
    T_QUERYRESULT *qres;
    T_RESULTCOLUMN *rescol;
    T_RESULT_INFO *result = NULL;
    T_RESULT_LIST *list = NULL;
    T_SELECT *select;

    char *p, *recbuf = NULL;
    int max_rec_len = 0;
    int row_num = 0;
    int i, ret;
    int total_rec_len = 0;
    int max_rec_cnt = 0;

    DataType *datatypes = NULL;

    if (stmt->u._select.tmp_sub)
        select = stmt->u._select.tmp_sub;
    else
        select = &stmt->u._select;

    qres = &select->queryresult;

    if (stmt->result && stmt->is_partial_result)
    {
        result = stmt->result;
        max_rec_len = stmt->result->max_rec_len;
        list = result->list;
        if (list->tail != NULL)
        {
            list->head = list->tail;
            list->tail->body_size = 0;
            list->tail->rows_in_page = 0;
        }
    }
    else
    {
        datatypes = PMEM_ALLOC(sizeof(DataType) * qres->len);
        if (datatypes == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            goto ERROR_LABEL;
        }

        for (i = 0, max_rec_len = 0; i < qres->len; i++)
        {
            if (IS_NBS(qres->list[i].value.valuetype))
            {
                max_rec_len = _alignedlen(max_rec_len, sizeof(DB_WCHAR));
                max_rec_len += (qres->list[i].len + 1) * sizeof(DB_WCHAR);
            }
            else
                max_rec_len += qres->list[i].len + 1;
            datatypes[i] = qres->list[i].value.valuetype;
        }

        max_rec_len = _alignedlen(max_rec_len, sizeof(MDB_INT32));

        result = PMEM_XCALLOC(sizeof(T_RESULT_INFO));
        if (result == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            goto ERROR_LABEL;
        }

        sc_memset(result, 0, sizeof(T_RESULT_INFO));
        result->field_datatypes = datatypes;

        list = RESULT_LIST_Create(max_rec_len);
        if (list == NULL)
        {
            goto ERROR_LABEL;
        }
    }

    recbuf = PMEM_ALLOC(max_rec_len);
    if (recbuf == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto ERROR_LABEL;
    }

    if (stmt->is_partial_result)
    {
        if (result->tmp_rec_len > 0)
        {
            if (RESULT_LIST_Add(list, result->tmp_recbuf,
                            result->tmp_rec_len) < 0)
                goto ERROR_LABEL;

            row_num++;
            if (stmt->is_partial_result)
                max_rec_cnt++;

            total_rec_len = result->tmp_rec_len;
            result->tmp_rec_len = 0;
        }
    }

    while (1)
    {
        ret = SQL_fetch(&handle, select, result);
        if (ret == RET_END)
        {
            break;
        }
        else if (ret == RET_ERROR)
        {
            goto ERROR_LABEL;
        }

        sc_memset(recbuf, 0, max_rec_len);

        for (i = 0, p = recbuf; i < qres->len; i++)
        {
            rescol = &qres->list[i];

            if (rescol->value.is_null)
            {
                if (IS_NBS(rescol->value.valuetype))
                {
                    p = (char *) _alignedlen((unsigned long) p,
                            sizeof(DB_WCHAR));
                    sc_memset(p, 0, sizeof(DB_WCHAR));
                    p += sizeof(DB_WCHAR);
                }
                else
                {
                    (*p) = '\0';
                    p++;
                }
            }
            else
            {
                p += print_value_as_string(&rescol->value, p, rescol->len);
            }
        }

        p = (char *) _alignedlen((unsigned long) p, sizeof(MDB_INT32));

        if (stmt->is_partial_result)
        {
            total_rec_len += (p - recbuf);
            max_rec_cnt++;

            if ((max_rec_cnt > 1 &&
                            total_rec_len >
                            (RESULT_PAGE_SIZE - RESULT_PGHDR_SIZE)))
            {
                max_rec_cnt--;

                result->tmp_rec_len = (p - recbuf);
                if (result->tmp_recbuf == NULL)
                {
                    result->tmp_recbuf = PMEM_ALLOC(max_rec_len);
                    if (result->tmp_recbuf == NULL)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_OUTOFMEMORY, 0);
                        goto ERROR_LABEL;
                    }
                }
                sc_memcpy(result->tmp_recbuf, recbuf, (p - recbuf));
                break;
            }
        }

        if (RESULT_LIST_Add(list, recbuf, (p - recbuf)) < 0)
        {
            goto ERROR_LABEL;
        }

        ++row_num;

        if (ret == RET_END_USED_DIRECT_API)
            break;
    }

    result->row_num = row_num;
    result->field_count = qres->len;
    result->list = list;
    if (stmt->is_partial_result)
    {
        result->fetch_cnt = max_rec_cnt;
        result->max_rec_len = max_rec_len;
    }
    else
    {
        result->fetch_cnt = 0;
        result->max_rec_len = 0;
    }

    PMEM_FREENUL(recbuf);
    return result;

  ERROR_LABEL:

    PMEM_FREENUL(recbuf);
    if (result)
    {
        PMEM_FREENUL(result->field_datatypes);
        if (stmt->is_partial_result)
        {
            PMEM_FREENUL(result->tmp_recbuf);
        }
        PMEM_FREENUL(result);
    }
    RESULT_LIST_Destroy(list);

    return NULL;
}

/*****************************************************************************/
//! store_select_binary_result 

/*! \breif  BINARY TYPE으로 RESULT SET을 만드는 함수이다.
 ************************************
 * \param handle(in)    :
 * \param stmt(in/out)  : 
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - T_EXPRESSIONDESC 구조체 변경
 *  - byte/varbyte type은 앞에 4 byte 길이 length가 붙는다
 *
 *****************************************************************************/
static T_RESULT_INFO *
store_select_binary_result(int handle, T_STATEMENT * stmt)
{
    T_QUERYRESULT *qres;
    T_RESULTCOLUMN *rescol;
    T_RESULT_INFO *result = NULL;
    T_RESULT_LIST *list = NULL;
    char *target = NULL, *null_ptr = NULL, bit;
    T_SELECT *select;

    char *recbuf = NULL;
    int *rec_len = NULL, max_rec_len, col_len;
    int row_num = 0;
    int i, ret;

    char *rec_ptr;
    OID *rid;

    int total_rec_len = 0;
    int max_rec_cnt = 0;

    if (stmt->u._select.tmp_sub)
        select = stmt->u._select.tmp_sub;
    else
        select = &stmt->u._select;

    qres = &select->queryresult;

    max_rec_len = bin_result_record_len(qres, 0);
    recbuf = PMEM_ALLOC(max_rec_len);
    if (recbuf == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto ERROR_LABEL;
    }

    if (!stmt->result)
    {
        result = PMEM_XCALLOC(sizeof(T_RESULT_INFO));
        if (result == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            goto ERROR_LABEL;
        }
        sc_memset(result, 0, sizeof(T_RESULT_INFO));
    }
    else
        result = stmt->result;

    result->rid_included = qres->include_rid;

    if (result->list)
    {
        list = result->list;
        if (stmt->result->row_num == 0 && list->tail != NULL)
        {
            list->head = list->tail;
            list->tail->body_size = 0;
            list->tail->rows_in_page = 0;
        }

        row_num = stmt->result->row_num;
    }
    else
        list = RESULT_LIST_Create(max_rec_len);
    if (list == NULL)
    {
        goto ERROR_LABEL;
    }

    if (stmt->is_partial_result)
    {
        if (result->tmp_rec_len > 0)
        {
            if (RESULT_LIST_Add(list, result->tmp_recbuf,
                            result->tmp_rec_len) < 0)
                goto ERROR_LABEL;

            ++row_num;
            if (stmt->is_partial_result)
                max_rec_cnt++;

            total_rec_len = result->tmp_rec_len;

            result->tmp_rec_len = 0;
        }
    }

    while (1)
    {
        ret = SQL_fetch(&handle, select, result);
        if (ret == RET_END)
        {
            break;
        }
        else if (ret == RET_ERROR)
            goto ERROR_LABEL;

        {
            MDB_BOOL has_values = FALSE;
            T_LIST_VALUE *val_list;

            val_list = &select->values;

            if (val_list->len > 0)
            {
                has_values = TRUE;
            }
            else
            {
                rid = (OID *) recbuf;
                rec_ptr = recbuf + OID_SIZE;

                rec_len = (int *) rec_ptr;
                rec_ptr += INT_SIZE;

                null_ptr = (char *) rec_ptr;
                rec_ptr += (((qres->len - 1) >> 3) + 1);

                target = (char *) rec_ptr;

                sc_memset(recbuf, 0, target - recbuf);

                *rec_len = target - recbuf;

                /* if it's from temptable, oid will be NULL_OID */
                if (qres->include_rid)
                {
#ifdef MDB_DEBUG
                    if (qres->list[qres->len].value.valuetype != DT_OID)
                        sc_assert(0, __FILE__, __LINE__);
#endif
                    *rid = qres->list[qres->len].value.u.oid;
                }
            }

            for (i = 0, bit = 1; i < qres->len; i++)
            {
                rescol = &qres->list[i];

                if (has_values)
                {
                }
                else
                {
                    if (rescol->value.is_null)
                    {
                        col_len = 0;
                        *null_ptr |= bit;
                    }
                    else
                        col_len = copy_one_binary_column(target, rescol, 0);

                    target += col_len;
                    (*rec_len) += col_len;

                    if (!((bit <<= 1) & 0xFF))
                    {
                        bit = 1;
                        null_ptr++;
                    }
                }
            }

            if (!has_values)
            {
                if (stmt->is_partial_result)
                {
                    total_rec_len += *rec_len;
                    max_rec_cnt++;

                    if ((max_rec_cnt > 1 &&
                                    total_rec_len >
                                    (RESULT_PAGE_SIZE - RESULT_PGHDR_SIZE)))
                    {
                        max_rec_cnt--;

                        result->tmp_rec_len = *rec_len;
                        if (result->tmp_recbuf == NULL)
                        {
                            result->tmp_recbuf = PMEM_ALLOC(max_rec_len);
                            if (result->tmp_recbuf == NULL)
                            {
                                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                        SQL_E_OUTOFMEMORY, 0);
                                goto ERROR_LABEL;
                            }
                        }
                        sc_memcpy(result->tmp_recbuf, recbuf, *rec_len);
                        break;
                    }
                }

                if (RESULT_LIST_Add(list, recbuf, *rec_len) < 0)
                {
                    goto ERROR_LABEL;
                }
            }
        }
        ++row_num;
        if (ret == RET_END_USED_DIRECT_API)
            break;
    }

    result->row_num = row_num;
    result->field_count = qres->len;
    result->list = list;
    if (stmt->is_partial_result)
    {
        result->fetch_cnt = max_rec_cnt;
        result->max_rec_len = max_rec_len;
    }

    PMEM_FREENUL(recbuf);
    return result;

  ERROR_LABEL:

    PMEM_FREENUL(recbuf);
    if (result)
    {
        PMEM_FREENUL(result->field_datatypes);
        if (stmt->is_partial_result)
        {
            PMEM_FREENUL(result->tmp_recbuf);
        }
        PMEM_FREENUL(result);
    }
    RESULT_LIST_Destroy(list);

    return NULL;
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
static T_RESULT_INFO *
store_select_no_result(int handle, T_STATEMENT * stmt)
{
    T_QUERYRESULT *qres;
    T_RESULT_INFO *result = NULL;
    int row_num = 0;
    int ret;

    qres = &stmt->u._select.queryresult;

    result = PMEM_XCALLOC(sizeof(T_RESULT_INFO));
    if (result == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return NULL;
    }

    while (1)
    {
        ret = SQL_fetch(&handle, &stmt->u._select, result);
        if (ret == RET_END)
            break;

        else if (ret == RET_ERROR)
        {
            if (result)
                PMEM_FREENUL(result->field_datatypes);
            PMEM_FREENUL(result);
            return NULL;
        }

        ++row_num;

        if (ret == RET_END_USED_DIRECT_API)
            break;
    }

    result->row_num = row_num;
    result->field_count = qres->len;
    result->list = NULL;

    return result;
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
static T_RESULT_INFO *
store_select_result(int handle, T_STATEMENT * stmt, int res_type)
{
    switch (res_type)
    {
    case RES_TYPE_BINARY:
        return store_select_binary_result(handle, stmt);

    case RES_TYPE_STRING:
        return store_select_string_result(handle, stmt);

    case RES_TYPE_NONE:
        return store_select_no_result(handle, stmt);

    default:
        return NULL;
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
T_RESULT_INFO *
store_result(int handle, T_STATEMENT * stmt, int result_type)
{
    T_RESULT_INFO *result;

    if (!stmt->trans_info->opened)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
        return NULL;
    }

    if (stmt->sqltype == ST_DESCRIBE)
        result = store_desc_result(handle, stmt, result_type);
    else if (stmt->sqltype == ST_SELECT)
        result = store_select_result(handle, stmt, result_type);
    else
        result = NULL;

    stmt->result = result;

    return result;
}
