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

#include "sys_inc.h"
#include "mdb_compat.h"
#include "isql.h"
#include "sql_datast.h"
#include "sql_util.h"
#include "ErrorCode.h"
#include "mdb_comm_stub.h"
#include "mdb_Mem_Mgr.h"

#include "mdb_charset.h"
#include "sql_mclient.h"

int isql_FETCHROW_real(iSQL_RES * res);

int isql_EXECUTE_real(iSQL_STMT * stmt);
int isql_STMTSTORERESULT_real(iSQL_STMT * stmt, int use_result);
int isql_FETCH_real(iSQL_STMT * isql_stmt);
int isql_CLOSESTATEMENT_real(iSQL_STMT * isql_stmt);

int isql_QUERY_real(iSQL * isql, char *query, int len);

int isql_GETRESULT_real(iSQL * isql, const int retrieval);

int isql_GETLISTFIELDS_real(iSQL * isql, char *table, char *column,
        iSQL_FIELD ** fields);

int isql_FLUSHRESULTSET_real(iSQL * isql);

int isql_BEGINTRANSACTION_real(iSQL * isql);
int isql_CLOSETRANSACTION_real(iSQL * isql, const int mode, int f_flush);
int isql_PREPARE_real(iSQL * isql, char *query, int len, unsigned int *stmt_id,
        iSQL_FIELD ** pfields, unsigned int *param_count, iSQL_FIELD ** fields,
        unsigned int *field_count);
int isql_DESCUPDATERID_real(iSQL * isql, OID rid, int numfields,
        iSQL_FIELD * fielddesc, iSQL_BIND * fieldvalue);

int isql_GETPLAN_real(iSQL * isql, iSQL_STMT * stmt, char **plan_string);

int get_last_error(iSQL * isql, int user_errno);
T_STATEMENT *get_statement(T_SQL * sql, unsigned int id);

extern char *datetime2char(char *str_datetime, char *datetime);
extern char *timestamp2char(char *str_timestamp, t_astime * timestamp);
extern iSQL_TIME *char2timest(iSQL_TIME * timest, char *stime, int sleng);

static int isql_reconnect(iSQL *);

/*****************************************************************************/
//! get_timest 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param ti : output
 * \param str : input
 ************************************
 * \return  return_value
 ************************************
 * \note 초기화를 0으로 해주고, 
 *          delimiter 가 없는 string 을 iSQL_TIME 으로 변환
 *          string 에 포함된 정보까지만 iSQL_TIME 구조체에 입력
 *****************************************************************************/
static void
get_timest(iSQL_TIME * ti, const char *str)
{
    iSQL_TIME local;
    char buf[5];
    int i, len;

    sc_memset(&local, 0, sizeof(iSQL_TIME));
    local.month = local.day = 1;

    len = sc_strlen(str);

    /* fetch year */
    for (i = 0; i < 4 && str[i]; i++)
        buf[0] = str[i];
    buf[i] = '\0';
    local.year = (unsigned short int) sc_atoi(buf);
    if ((len -= 4) <= 0)
        goto RETURN_LABEL;
    str += 4;

    /* fetch month */
    for (i = 0; i < 2 && str[i]; i++)
        buf[0] = str[i];
    buf[i] = '\0';
    local.month = (unsigned short int) sc_atoi(buf);
    if ((len -= 2) <= 0)
        goto RETURN_LABEL;
    str += 2;

    /* fetch day */
    for (i = 0; i < 2 && str[i]; i++)
        buf[0] = str[i];
    buf[i] = '\0';
    local.day = (unsigned short int) sc_atoi(buf);
    if ((len -= 2) <= 0)
        goto RETURN_LABEL;
    str += 2;

    /* fetch hour */
    for (i = 0; i < 2 && str[i]; i++)
        buf[0] = str[i];
    buf[i] = '\0';
    local.hour = (unsigned short int) sc_atoi(buf);
    if ((len -= 2) <= 0)
        goto RETURN_LABEL;
    str += 2;

    /* fetch minute */
    for (i = 0; i < 2 && str[i]; i++)
        buf[0] = str[i];
    buf[i] = '\0';
    local.minute = (unsigned short int) sc_atoi(buf);
    if ((len -= 2) <= 0)
        goto RETURN_LABEL;
    str += 2;

    /* fetch second */
    for (i = 0; i < 2 && str[i]; i++)
        buf[0] = str[i];
    buf[i] = '\0';
    local.second = (unsigned short int) sc_atoi(buf);
    if ((len -= 2) <= 0)
        goto RETURN_LABEL;
    str += 2;

    /* fetch fraction */
    for (i = 0; i < 2 && str[i]; i++)
        buf[0] = str[i];
    buf[i] = '\0';
    local.fraction = (unsigned short int) sc_atoi(buf);
    if ((len -= 2) <= 0)
        goto RETURN_LABEL;
    str += 2;

  RETURN_LABEL:
    sc_memcpy(ti, &local, sizeof(iSQL_TIME));
}

static void
isql_free_result_data(iSQL_RES * res)
{
    SMEM_FREENUL(res->data->rows_buf);
    SMEM_FREENUL(res->data->data_buf);

    if (res->data && res->data->result_page_list)
    {
        iSQL_RESULT_PAGE *cur_pg;

        while (res->data->result_page_list)
        {
            cur_pg = res->data->result_page_list;
            res->data->result_page_list = cur_pg->next;
            SMEM_FREE(cur_pg);
        }

    }
    SMEM_FREENUL(res->data);
}


/*****************************************************************************/
//! close_connection 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static void
close_connection(iSQL * isql)
{
    if (isql->handle >= 0)
    {
        CS_Disconnect(isql->handle);
        isql->handle = -1;
    }
}

/*****************************************************************************/
//! read_one_row 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res :
 * \param row : 
 * \param lengths :
 * \param fields :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
read_one_row(iSQL_RES * res, iSQL_ROW row, iSQL_LENGTH lengths,
        unsigned long int fields)
{
    unsigned int i;
    int ret;

    if (res == NULL)
        return -1;

    if (res->data)
    {       /* if buffered */
        if (!res->data_cursor)
        {
            if (res->is_partial_result &&
                    (long int) res->row_count == res->data_offset &&
                    !res->data->use_eof)
            {
                MDB_DB_LOCK_SQL_API();

                ret = isql_FETCHROW_real(res);

                MDB_DB_UNLOCK_SQL_API();

                if (ret < 0)
                    return -1;
            }
            if (!res->data_cursor)
                return 0;
        }

        for (i = 0; i < fields; i++)
            row[i] = res->data_cursor->data[i];
        row[i] = NULL;
        res->data_cursor = res->data_cursor->next;
    }
    else
        return -1;

    if (res->eof)
        return 0;
    else
        return 1;
}

/*****************************************************************************/
//! get_converted_binary_data 
/*! \breif  RESULTSET을 BINARY TYPE에서 STRING TYPE으로 변환해주는 함수
 ************************************
 * \param bind(in/out)  : RESULTSET BIND AREA
 * \param field(in)     : fields info
 * \param row(in)       : result set
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - BYTE/VARBYTE TYPE을 지원함
 *****************************************************************************/
static int
get_converted_binary_data(iSQL_BIND * bind, iSQL_FIELD * field, char *row)
{
    union
    {
        tinyint ti;
        smallint si;
        int ii;
        bigint bi;
        OID oi;
        float fl;
        double dl;
        decimal dc;
    } raw;                      /* raw value */
    iSQL_TIME tm;
    char buf[32];
    MDB_UINT32 len = 0;

    if (bind->length)
    {
        if (IS_NUMERIC(bind->buffer_type))
            *(bind->length) = SizeOfType[bind->buffer_type];
    }

    switch (field->type)
    {
    case SQL_DATA_TINYINT:
        sc_memcpy(&raw, row, sizeof(tinyint));
        switch (bind->buffer_type)
        {
        case SQL_DATA_TINYINT:
            break;
        case SQL_DATA_SMALLINT:
            *(smallint *) (bind->buffer) = (smallint) raw.ti;
            break;
        case SQL_DATA_INT:
            *(int *) (bind->buffer) = (int) raw.ti;
            break;
        case SQL_DATA_BIGINT:
            *(bigint *) (bind->buffer) = (bigint) raw.ti;
            break;
        case SQL_DATA_FLOAT:
            *(float *) (bind->buffer) = (float) raw.ti;
            break;
        case SQL_DATA_DOUBLE:
            *(double *) (bind->buffer) = (double) raw.ti;
            break;
        case SQL_DATA_DECIMAL:
            *(decimal *) (bind->buffer) = (decimal) raw.ti;
            break;
        case SQL_DATA_CHAR:
        case SQL_DATA_VARCHAR:
            if (bind->length)
                *(bind->length) =
                        sc_snprintf(bind->buffer, bind->buffer_length,
                        "%d", (int) raw.ti);
            break;
        case SQL_DATA_DATE:
        case SQL_DATA_TIME:
        case SQL_DATA_DATETIME:
        case SQL_DATA_TIMESTAMP:
            sc_snprintf(buf, sizeof(buf), "%d", (int) raw.ti);
            get_timest((iSQL_TIME *) bind->buffer, buf);
            if (bind->buffer_type != SQL_DATA_TIMESTAMP)
                ((iSQL_TIME *) bind->buffer)->fraction = 0;
            if (bind->length)
                *(bind->length) = sizeof(iSQL_TIME);
            break;
        default:
            break;
        }
        len = sizeof(tinyint);
        break;
    case SQL_DATA_SMALLINT:
        sc_memcpy(&raw, row, sizeof(smallint));
        switch (bind->buffer_type)
        {
        case SQL_DATA_TINYINT:
            *(tinyint *) (bind->buffer) = (tinyint) raw.si;
            break;
        case SQL_DATA_SMALLINT:
            break;
        case SQL_DATA_INT:
            *(int *) (bind->buffer) = (int) raw.si;
            break;
        case SQL_DATA_BIGINT:
            *(bigint *) (bind->buffer) = (bigint) raw.si;
            break;
        case SQL_DATA_FLOAT:
            *(float *) (bind->buffer) = (float) raw.si;
            break;
        case SQL_DATA_DOUBLE:
            *(double *) (bind->buffer) = (double) raw.si;
            break;
        case SQL_DATA_DECIMAL:
            *(decimal *) (bind->buffer) = (decimal) raw.si;
            break;
        case SQL_DATA_CHAR:
        case SQL_DATA_VARCHAR:
            if (bind->length)
                *(bind->length) =
                        sc_snprintf(bind->buffer, bind->buffer_length,
                        "%hd", raw.si);
            break;
        case SQL_DATA_DATE:
        case SQL_DATA_TIME:
        case SQL_DATA_DATETIME:
        case SQL_DATA_TIMESTAMP:
            sc_snprintf(buf, sizeof(buf), "%hd", raw.si);
            get_timest((iSQL_TIME *) bind->buffer, buf);
            if (bind->buffer_type != SQL_DATA_TIMESTAMP)
                ((iSQL_TIME *) bind->buffer)->fraction = 0;
            if (bind->length)
                *(bind->length) = sizeof(iSQL_TIME);
            break;
        default:
            break;
        }
        len = sizeof(smallint);
        break;
    case SQL_DATA_INT:
        sc_memcpy(&raw, row, sizeof(int));
        switch (bind->buffer_type)
        {
        case SQL_DATA_TINYINT:
            *(tinyint *) (bind->buffer) = (tinyint) raw.ii;
            break;
        case SQL_DATA_SMALLINT:
            *(smallint *) (bind->buffer) = (smallint) raw.ii;
            break;
        case SQL_DATA_INT:
            break;
        case SQL_DATA_BIGINT:
            *(bigint *) (bind->buffer) = (bigint) raw.ii;
            break;
        case SQL_DATA_FLOAT:
            *(float *) (bind->buffer) = (float) raw.ii;
            break;
        case SQL_DATA_DOUBLE:
            *(double *) (bind->buffer) = (double) raw.ii;
            break;
        case SQL_DATA_DECIMAL:
            *(decimal *) (bind->buffer) = (decimal) raw.ii;
            break;
        case SQL_DATA_CHAR:
        case SQL_DATA_VARCHAR:
            if (bind->length)
                *(bind->length) =
                        sc_snprintf(bind->buffer,
                        bind->buffer_length, "%d", raw.ii);
            break;
        case SQL_DATA_DATE:
        case SQL_DATA_TIME:
        case SQL_DATA_DATETIME:
        case SQL_DATA_TIMESTAMP:
            sc_snprintf(buf, sizeof(buf), "%d", raw.ii);
            get_timest((iSQL_TIME *) bind->buffer, buf);
            if (bind->buffer_type != SQL_DATA_TIMESTAMP)
                ((iSQL_TIME *) bind->buffer)->fraction = 0;
            if (bind->length)
                *(bind->length) = sizeof(iSQL_TIME);
            break;
        default:
            break;
        }
        len = sizeof(int);
        break;
    case SQL_DATA_BIGINT:
        sc_memcpy(&raw, row, sizeof(bigint));
        switch (bind->buffer_type)
        {
        case SQL_DATA_TINYINT:
            *(tinyint *) (bind->buffer) = (tinyint) raw.bi;
            break;
        case SQL_DATA_SMALLINT:
            *(smallint *) (bind->buffer) = (smallint) raw.bi;
            break;
        case SQL_DATA_INT:
            *(int *) (bind->buffer) = (int) raw.bi;
            break;
        case SQL_DATA_BIGINT:
            break;
        case SQL_DATA_FLOAT:
            *(float *) (bind->buffer) = (float) raw.bi;
            break;
        case SQL_DATA_DOUBLE:
            *(double *) (bind->buffer) = (double) raw.bi;
            break;
        case SQL_DATA_DECIMAL:
            *(decimal *) (bind->buffer) = (decimal) raw.bi;
            break;
        case SQL_DATA_CHAR:
        case SQL_DATA_VARCHAR:
            if (bind->length)
                *(bind->length) =
                        sc_snprintf(bind->buffer,
                        bind->buffer_length, I64_FORMAT, raw.bi);
            break;
        case SQL_DATA_DATE:
        case SQL_DATA_TIME:
        case SQL_DATA_DATETIME:
        case SQL_DATA_TIMESTAMP:
            sc_snprintf(buf, sizeof(buf), I64_FORMAT, raw.bi);
            get_timest((iSQL_TIME *) bind->buffer, buf);
            if (bind->buffer_type != SQL_DATA_TIMESTAMP)
                ((iSQL_TIME *) bind->buffer)->fraction = 0;
            if (bind->length)
                *(bind->length) = sizeof(iSQL_TIME);
            break;
        default:
            break;
        }
        len = sizeof(bigint);
        break;
    case SQL_DATA_OID:
        sc_memcpy(&raw, row, sizeof(OID));
        switch (bind->buffer_type)
        {
        case SQL_DATA_TINYINT:
            *(tinyint *) (bind->buffer) = (tinyint) raw.oi;
            break;
        case SQL_DATA_SMALLINT:
            *(smallint *) (bind->buffer) = (smallint) raw.oi;
            break;
        case SQL_DATA_INT:
            *(int *) (bind->buffer) = (int) raw.oi;
            break;
        case SQL_DATA_BIGINT:
            break;
        case SQL_DATA_FLOAT:
#if defined(_64BIT) && defined(WIN32)
            *(float *) (bind->buffer) = (float) (DB_INT64) raw.oi;
#else
            *(float *) (bind->buffer) = (float) raw.oi;
#endif
            break;
        case SQL_DATA_DOUBLE:
#if defined(_64BIT) && defined(WIN32)
            *(double *) (bind->buffer) = (double) (DB_INT64) raw.oi;
#else
            *(double *) (bind->buffer) = (double) raw.oi;
#endif
            break;
        case SQL_DATA_DECIMAL:
#if defined(_64BIT) && defined(WIN32)
            *(decimal *) (bind->buffer) = (decimal) (DB_INT64) raw.oi;
#else
            *(decimal *) (bind->buffer) = (decimal) raw.oi;
#endif
            break;
        case SQL_DATA_CHAR:
        case SQL_DATA_VARCHAR:
            if (bind->length)
                *(bind->length) =
                        sc_snprintf(bind->buffer,
                        bind->buffer_length, "%lu", raw.oi);
            break;
        case SQL_DATA_DATE:
        case SQL_DATA_TIME:
        case SQL_DATA_DATETIME:
        case SQL_DATA_TIMESTAMP:
            sc_snprintf(buf, sizeof(buf), "%lu", raw.oi);
            get_timest((iSQL_TIME *) bind->buffer, buf);
            if (bind->buffer_type != SQL_DATA_TIMESTAMP)
                ((iSQL_TIME *) bind->buffer)->fraction = 0;
            if (bind->length)
                *(bind->length) = sizeof(iSQL_TIME);
            break;
        default:
            break;
        }
        len = sizeof(OID);
        break;
    case SQL_DATA_FLOAT:
        sc_memcpy(&raw, row, sizeof(float));
        switch (bind->buffer_type)
        {
        case SQL_DATA_TINYINT:
            *(tinyint *) (bind->buffer) = (tinyint) raw.fl;
            break;
        case SQL_DATA_SMALLINT:
            *(smallint *) (bind->buffer) = (smallint) raw.fl;
            break;
        case SQL_DATA_INT:
            *(int *) (bind->buffer) = (int) raw.fl;
            break;
        case SQL_DATA_BIGINT:
            *(bigint *) (bind->buffer) = (bigint) raw.fl;
            break;
        case SQL_DATA_FLOAT:
            break;
        case SQL_DATA_DOUBLE:
            *(double *) (bind->buffer) = (double) raw.fl;
            break;
        case SQL_DATA_DECIMAL:
            *(decimal *) (bind->buffer) = (decimal) raw.fl;
            break;
        case SQL_DATA_CHAR:
        case SQL_DATA_VARCHAR:
            len = MDB_FLOAT2STR(bind->buffer, bind->buffer_length, raw.fl);
            if (bind->length)
                *(bind->length) = len;
            break;
        case SQL_DATA_DATE:
        case SQL_DATA_TIME:
        case SQL_DATA_DATETIME:
        case SQL_DATA_TIMESTAMP:
            len = MDB_FLOAT2STR(buf, sizeof(buf), raw.fl);
            get_timest((iSQL_TIME *) bind->buffer, buf);
            if (bind->buffer_type != SQL_DATA_TIMESTAMP)
                ((iSQL_TIME *) bind->buffer)->fraction = 0;
            if (bind->length)
                *(bind->length) = sizeof(iSQL_TIME);
            break;
        default:
            break;
        }
        len = sizeof(float);
        break;
    case SQL_DATA_DOUBLE:
        sc_memcpy(&raw, row, sizeof(double));
        switch (bind->buffer_type)
        {
        case SQL_DATA_TINYINT:
            *(tinyint *) (bind->buffer) = (tinyint) raw.dl;
            break;
        case SQL_DATA_SMALLINT:
            *(smallint *) (bind->buffer) = (smallint) raw.dl;
            break;
        case SQL_DATA_INT:
            *(int *) (bind->buffer) = (int) raw.dl;
            break;
        case SQL_DATA_BIGINT:
            *(bigint *) (bind->buffer) = (bigint) raw.dl;
            break;
        case SQL_DATA_FLOAT:
            *(float *) (bind->buffer) = (float) raw.dl;
            break;
        case SQL_DATA_DOUBLE:
            break;
        case SQL_DATA_DECIMAL:
            *(decimal *) (bind->buffer) = (decimal) raw.dl;
            break;
        case SQL_DATA_CHAR:
        case SQL_DATA_VARCHAR:
            len = MDB_DOUBLE2STR(bind->buffer, bind->buffer_length, raw.dl);
            if (bind->length)
                *(bind->length) = len;
            break;
        case SQL_DATA_DATE:
        case SQL_DATA_TIME:
        case SQL_DATA_DATETIME:
        case SQL_DATA_TIMESTAMP:
            len = MDB_DOUBLE2STR(buf, sizeof(buf), raw.dl);
            get_timest((iSQL_TIME *) bind->buffer, buf);
            if (bind->buffer_type != SQL_DATA_TIMESTAMP)
                ((iSQL_TIME *) bind->buffer)->fraction = 0;
            if (bind->length)
                *(bind->length) = sizeof(iSQL_TIME);
            break;
        default:
            break;
        }
        len = sizeof(double);
        break;
    case SQL_DATA_DECIMAL:
        sc_memcpy(&raw, row, sizeof(decimal));
        switch (bind->buffer_type)
        {
        case SQL_DATA_TINYINT:
            *(tinyint *) (bind->buffer) = (tinyint) raw.dc;
            break;
        case SQL_DATA_SMALLINT:
            *(smallint *) (bind->buffer) = (smallint) raw.dc;
            break;
        case SQL_DATA_INT:
            *(int *) (bind->buffer) = (int) raw.dc;
            break;
        case SQL_DATA_BIGINT:
            *(bigint *) (bind->buffer) = (bigint) raw.dc;
            break;
        case SQL_DATA_FLOAT:
            *(float *) (bind->buffer) = (float) raw.dc;
            break;
        case SQL_DATA_DOUBLE:
            *(double *) (bind->buffer) = (double) raw.dc;
            break;
        case SQL_DATA_DECIMAL:
            break;
        case SQL_DATA_CHAR:
        case SQL_DATA_VARCHAR:
            len = MDB_DOUBLE2STR(bind->buffer, bind->buffer_length, raw.dc);
            if (bind->length)
                *(bind->length) = len;
            break;
        case SQL_DATA_DATE:
        case SQL_DATA_TIME:
        case SQL_DATA_DATETIME:
        case SQL_DATA_TIMESTAMP:
            len = MDB_DOUBLE2STR(buf, sizeof(buf), raw.dc);
            get_timest((iSQL_TIME *) bind->buffer, buf);
            if (bind->buffer_type != SQL_DATA_TIMESTAMP)
                ((iSQL_TIME *) bind->buffer)->fraction = 0;
            if (bind->length)
                *(bind->length) = sizeof(iSQL_TIME);
            break;
        default:
            break;
        }
        len = sizeof(decimal);
        break;
    case SQL_DATA_CHAR:
        switch (bind->buffer_type)
        {
        case SQL_DATA_TINYINT:
            *(tinyint *) (bind->buffer) = (tinyint) sc_atoi(row);
            break;
        case SQL_DATA_SMALLINT:
            *(smallint *) (bind->buffer) = (smallint) sc_atoi(row);
            break;
        case SQL_DATA_INT:
            *(int *) (bind->buffer) = (int) sc_atoi(row);
            break;
        case SQL_DATA_BIGINT:
            *(bigint *) (bind->buffer) = (bigint) sc_atoll(row);
            break;
        case SQL_DATA_FLOAT:
            *(float *) (bind->buffer) = (float) sc_atof(row);
            break;
        case SQL_DATA_DOUBLE:
            *(double *) (bind->buffer) = (double) sc_atof(row);
            break;
        case SQL_DATA_DECIMAL:
            *(decimal *) (bind->buffer) = (decimal) sc_atof(row);
            break;
        case SQL_DATA_CHAR:
            break;
        case SQL_DATA_VARCHAR:
            if (bind->buffer_length == 0)
                break;
            if (bind->buffer_length <= field->buffer_length)
            {
                sc_memcpy(bind->buffer, row, bind->buffer_length);
                if (bind->length)
                    *bind->length = bind->buffer_length - 1;
                bind->buffer[bind->buffer_length - 1] = '\0';
            }
            else
            {
                sc_memcpy(bind->buffer, row, field->buffer_length);
                if (bind->length)
                    *bind->length = field->buffer_length;
                bind->buffer[field->buffer_length] = '\0';
            }
            break;
        case SQL_DATA_DATE:
        case SQL_DATA_TIME:
        case SQL_DATA_DATETIME:
        case SQL_DATA_TIMESTAMP:
            get_timest((iSQL_TIME *) bind->buffer, row);
            if (bind->buffer_type != SQL_DATA_TIMESTAMP)
                ((iSQL_TIME *) bind->buffer)->fraction = 0;
            if (bind->length)
                *(bind->length) = sizeof(iSQL_TIME);
            break;
        default:
            break;
        }
        len = field->buffer_length;
        break;
    case SQL_DATA_NCHAR:
        switch (bind->buffer_type)
        {
        case SQL_DATA_NCHAR:
            break;
        case SQL_DATA_NVARCHAR:
            if (bind->buffer_length == 0)
                break;
            if (bind->buffer_length <= field->buffer_length)
            {
                sc_wmemcpy((DB_WCHAR *) bind->buffer, (DB_WCHAR *) row,
                        bind->buffer_length);
                if (bind->length)
                    *bind->length = bind->buffer_length - 1;
                bind->buffer[bind->buffer_length - 1] = '\0';
            }
            else
            {
                sc_wmemcpy((DB_WCHAR *) bind->buffer, (DB_WCHAR *) row,
                        field->buffer_length);
                if (bind->length)
                    *bind->length = field->buffer_length;
                bind->buffer[field->buffer_length] = '\0';
            }
            break;
        default:
            break;
        }
        len = field->buffer_length * sizeof(DB_WCHAR);
        break;
    case SQL_DATA_VARCHAR:
        switch (bind->buffer_type)
        {
        case SQL_DATA_TINYINT:
            *(tinyint *) (bind->buffer) = (tinyint) sc_atoi(row);
            break;
        case SQL_DATA_SMALLINT:
            *(smallint *) (bind->buffer) = (smallint) sc_atoi(row);
            break;
        case SQL_DATA_INT:
            *(int *) (bind->buffer) = (int) sc_atoi(row);
            break;
        case SQL_DATA_BIGINT:
            *(bigint *) (bind->buffer) = (bigint) sc_atoll(row);
            break;
        case SQL_DATA_FLOAT:
            *(float *) (bind->buffer) = (float) sc_atof(row);
            break;
        case SQL_DATA_DOUBLE:
            *(double *) (bind->buffer) = (double) sc_atof(row);
            break;
        case SQL_DATA_DECIMAL:
            *(decimal *) (bind->buffer) = (decimal) sc_atof(row);
            break;
        case SQL_DATA_CHAR:
            if (bind->buffer_length == 0)
                break;
            if (bind->buffer_length <= field->buffer_length)
            {
                sc_memcpy(bind->buffer, row, bind->buffer_length);
                bind->buffer[bind->buffer_length - 1] = '\0';
            }
            else
            {
                sc_memcpy(bind->buffer, row, field->buffer_length);
                bind->buffer[field->buffer_length] = '\0';
            }
            if (bind->length)
                *bind->length = sc_strlen(bind->buffer);
            break;
        case SQL_DATA_VARCHAR:
            break;
        case SQL_DATA_DATE:
        case SQL_DATA_TIME:
        case SQL_DATA_DATETIME:
        case SQL_DATA_TIMESTAMP:
            get_timest((iSQL_TIME *) bind->buffer, row);
            if (bind->buffer_type != SQL_DATA_TIMESTAMP)
                ((iSQL_TIME *) bind->buffer)->fraction = 0;
            if (bind->length)
                *(bind->length) = sizeof(iSQL_TIME);
            break;
        default:
            break;
        }
        len = field->buffer_length;
        break;
    case SQL_DATA_NVARCHAR:
        switch (bind->buffer_type)
        {
        case SQL_DATA_NCHAR:
            if (bind->buffer_length == 0)
                break;
            if (bind->buffer_length <= field->buffer_length)
            {
                sc_wmemcpy((DB_WCHAR *) bind->buffer, (DB_WCHAR *) row,
                        bind->buffer_length);
                *((DB_WCHAR *) bind->buffer + (bind->buffer_length - 1)) = 0;
            }
            else
            {
                sc_wmemcpy((DB_WCHAR *) bind->buffer, (DB_WCHAR *) row,
                        field->buffer_length);
                *((DB_WCHAR *) bind->buffer + (bind->buffer_length)) = 0;
            }

            if (bind->length)
                *bind->length = sc_wcslen((DB_WCHAR *) bind->buffer);
            break;
        case SQL_DATA_NVARCHAR:
            break;
        default:
            break;
        }
        len = field->buffer_length * sizeof(DB_WCHAR);
        break;
    case SQL_DATA_BYTE:
    case SQL_DATA_VARBYTE:
        switch (bind->buffer_type)
        {
        case SQL_DATA_BYTE:
        case SQL_DATA_VARBYTE:
            sc_memcpy(&len, row, INT_SIZE);

            if (len == 0)
                break;

            if (len > field->buffer_length)
                len = field->buffer_length;
            sc_memcpy(bind->buffer, row + INT_SIZE, len);
            if (bind->length)
                *bind->length = len;

            break;
        default:
            break;
        }
        len = INT_SIZE + field->buffer_length;
        break;
    case SQL_DATA_DATE:
        sc_memcpy(&tm, row, sizeof(iSQL_TIME));
        if (SQL_DATA_TINYINT <= bind->buffer_type &&
                bind->buffer_type <= SQL_DATA_DECIMAL)
            sc_snprintf(buf,
                    sizeof(buf), "%hd%hd%hd", tm.year, tm.month, tm.day);
        switch (bind->buffer_type)
        {
        case SQL_DATA_TINYINT:
            *(tinyint *) (bind->buffer) = (tinyint) sc_atoll(buf);
            break;
        case SQL_DATA_SMALLINT:
            *(smallint *) (bind->buffer) = (smallint) sc_atoll(buf);
            break;
        case SQL_DATA_INT:
            *(int *) (bind->buffer) = (int) sc_atoll(buf);
            break;
        case SQL_DATA_BIGINT:
            *(bigint *) (bind->buffer) = (bigint) sc_atoll(buf);
            break;
        case SQL_DATA_FLOAT:
            *(float *) (bind->buffer) = (float) sc_atof(buf);
            break;
        case SQL_DATA_DOUBLE:
            *(double *) (bind->buffer) = (double) sc_atof(buf);
            break;
        case SQL_DATA_DECIMAL:
            *(decimal *) (bind->buffer) = (decimal) sc_atof(buf);
            break;
        case SQL_DATA_CHAR:
        case SQL_DATA_VARCHAR:
            if (bind->length)
                *(bind->length) =
                        sc_snprintf(bind->buffer, bind->buffer_length,
                        "%.4hd-%02hd-%02hd", tm.year, tm.month, tm.day);
            break;
        case SQL_DATA_DATE:
        case SQL_DATA_DATETIME:
            break;
        case SQL_DATA_TIMESTAMP:
            sc_memcpy(bind->buffer, &tm, sizeof(iSQL_TIME));
            if (bind->length)
                *(bind->length) = sizeof(iSQL_TIME);
            break;
        default:
            break;
        }
        len = sizeof(iSQL_TIME);
        break;
    case SQL_DATA_TIME:
        sc_memcpy(&tm, row, sizeof(iSQL_TIME));
        if (SQL_DATA_TINYINT <= bind->buffer_type &&
                bind->buffer_type <= SQL_DATA_DECIMAL)
            sc_snprintf(buf,
                    sizeof(buf), "%hd%hd%hd", tm.hour, tm.minute, tm.second);
        switch (bind->buffer_type)
        {
        case SQL_DATA_TINYINT:
            *(tinyint *) (bind->buffer) = (tinyint) sc_atoll(buf);
            break;
        case SQL_DATA_SMALLINT:
            *(smallint *) (bind->buffer) = (smallint) sc_atoll(buf);
            break;
        case SQL_DATA_INT:
            *(int *) (bind->buffer) = (int) sc_atoll(buf);
            break;
        case SQL_DATA_BIGINT:
            *(bigint *) (bind->buffer) = (bigint) sc_atoll(buf);
            break;
        case SQL_DATA_FLOAT:
            *(float *) (bind->buffer) = (float) sc_atof(buf);
            break;
        case SQL_DATA_DOUBLE:
            *(double *) (bind->buffer) = (double) sc_atof(buf);
            break;
        case SQL_DATA_DECIMAL:
            *(decimal *) (bind->buffer) = (decimal) sc_atof(buf);
            break;
        case SQL_DATA_CHAR:
        case SQL_DATA_VARCHAR:
            if (bind->length)
                *(bind->length) =
                        sc_snprintf(bind->buffer, bind->buffer_length,
                        "%02d:%02hd:%02hd", tm.hour, tm.minute, tm.second);
            break;
        case SQL_DATA_DATE:
        case SQL_DATA_DATETIME:
            break;
        case SQL_DATA_TIMESTAMP:
            sc_memcpy(bind->buffer, &tm, sizeof(iSQL_TIME));
            if (bind->length)
                *(bind->length) = sizeof(iSQL_TIME);
            break;
        default:
            break;
        }
        len = sizeof(iSQL_TIME);
        break;
    case SQL_DATA_DATETIME:
        sc_memcpy(&tm, row, sizeof(iSQL_TIME));
        if (SQL_DATA_TINYINT <= bind->buffer_type &&
                bind->buffer_type <= SQL_DATA_DECIMAL)
            sc_snprintf(buf,
                    sizeof(buf), "%hd%hd%hd%hd%hd%hd",
                    tm.year, tm.month, tm.day, tm.hour, tm.minute, tm.second);
        switch (bind->buffer_type)
        {
        case SQL_DATA_TINYINT:
            *(tinyint *) (bind->buffer) = (tinyint) sc_atoll(buf);
            break;
        case SQL_DATA_SMALLINT:
            *(smallint *) (bind->buffer) = (smallint) sc_atoll(buf);
            break;
        case SQL_DATA_INT:
            *(int *) (bind->buffer) = (int) sc_atoll(buf);
            break;
        case SQL_DATA_BIGINT:
            *(bigint *) (bind->buffer) = (bigint) sc_atoll(buf);
            break;
        case SQL_DATA_FLOAT:
            *(float *) (bind->buffer) = (float) sc_atof(buf);
            break;
        case SQL_DATA_DOUBLE:
            *(double *) (bind->buffer) = (double) sc_atof(buf);
            break;
        case SQL_DATA_DECIMAL:
            *(decimal *) (bind->buffer) = (decimal) sc_atof(buf);
            break;
        case SQL_DATA_CHAR:
        case SQL_DATA_VARCHAR:
            if (bind->length)
                *(bind->length) =
                        sc_snprintf(bind->buffer,
                        bind->buffer_length,
                        "%.4hd-%02hd-%02hd %02hd:%02hd:%02hd",
                        tm.year, tm.month, tm.day,
                        tm.hour, tm.minute, tm.second);
            break;
        case SQL_DATA_DATE:
        case SQL_DATA_TIME:
        case SQL_DATA_DATETIME:
            break;
        case SQL_DATA_TIMESTAMP:
            sc_memcpy(bind->buffer, &tm, sizeof(iSQL_TIME));
            if (bind->length)
                *(bind->length) = sizeof(iSQL_TIME);
            break;
        default:
            break;
        }
        len = sizeof(iSQL_TIME);
        break;
    case SQL_DATA_TIMESTAMP:
        sc_memcpy(&tm, row, sizeof(iSQL_TIME));
        if (SQL_DATA_TINYINT <= bind->buffer_type &&
                bind->buffer_type <= SQL_DATA_DECIMAL)
            sc_snprintf(buf,
                    sizeof(buf), "%hd%hd%hd%hd%hd%hd%hd",
                    tm.year, tm.month, tm.day,
                    tm.hour, tm.minute, tm.second, tm.fraction);
        switch (bind->buffer_type)
        {
        case SQL_DATA_TINYINT:
            *(tinyint *) (bind->buffer) = (tinyint) sc_atoll(buf);
            break;
        case SQL_DATA_SMALLINT:
            *(smallint *) (bind->buffer) = (smallint) sc_atoll(buf);
            break;
        case SQL_DATA_INT:
            *(int *) (bind->buffer) = (int) sc_atoll(buf);
            break;
        case SQL_DATA_BIGINT:
            *(bigint *) (bind->buffer) = (bigint) sc_atoll(buf);
            break;
        case SQL_DATA_FLOAT:
            *(float *) (bind->buffer) = (float) sc_atof(buf);
            break;
        case SQL_DATA_DOUBLE:
            *(double *) (bind->buffer) = (double) sc_atof(buf);
            break;
        case SQL_DATA_DECIMAL:
            *(decimal *) (bind->buffer) = (decimal) sc_atof(buf);
            break;
        case SQL_DATA_CHAR:
        case SQL_DATA_VARCHAR:
            if (bind->length)
                *(bind->length) =
                        sc_snprintf(bind->buffer,
                        bind->buffer_length,
                        "%.4d-%02hd-%02hd %02hd:%02hd:%02hd.%03hd",
                        tm.year, tm.month, tm.day,
                        tm.hour, tm.minute, tm.second, tm.fraction);
            break;

        case SQL_DATA_DATE:
        case SQL_DATA_TIME:
        case SQL_DATA_DATETIME:
            sc_memcpy(bind->buffer, &tm, sizeof(iSQL_TIME));
            ((iSQL_TIME *) bind->buffer)->fraction = 0;
            if (bind->length)
                *(bind->length) = sizeof(iSQL_TIME);
            break;
        case SQL_DATA_TIMESTAMP:
            break;
        default:
            break;
        }
        len = sizeof(iSQL_TIME);
        break;
    default:
        break;
    }

    return len;
}

static int iSQL_convert_binary_data_tinyint(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, tinyint ti);
static int iSQL_convert_binary_data_smallint(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, smallint si);
static int iSQL_convert_binary_data_int(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, int ii);
static int iSQL_convert_binary_data_bigint(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, bigint bi);
static int iSQL_convert_binary_data_float(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, float fl);
static int iSQL_convert_binary_data_double(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, double dl);
static int iSQL_convert_binary_data_decimal(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, decimal dc);
static int iSQL_convert_binary_data_char(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, char *row,
        iSQL_FIELD * field);
static int iSQL_convert_binary_data_date(isql_data_type bind_buffer_type,
        unsigned int *bind_length, char *bind_buffer, int bind_buffer_length,
        iSQL_TIME * tm);
static int iSQL_convert_binary_data_time(isql_data_type bind_buffer_type,
        unsigned int *bind_length, char *bind_buffer, int bind_buffer_length,
        iSQL_TIME * tm);
static int iSQL_convert_binary_data_datetime(isql_data_type bind_buffer_type,
        unsigned int *bind_length, char *bind_buffer, int bind_buffer_length,
        iSQL_TIME * tm);
static int iSQL_convert_binary_data_timestamp(isql_data_type bind_buffer_type,
        unsigned int *bind_length, char *bind_buffer, int bind_buffer_length,
        iSQL_TIME * tm);
static int iSQL_convert_binary_data_oid(isql_data_type bind_buffer_type, unsigned int *bind_length, char *bind_buffer, int bind_buffer_length, OID oi); /* _64BIT */

__DECL_PREFIX int
iSQL_convert_binary_data(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, iSQL_FIELD * field,
        char *row)
{
    union
    {
        tinyint ti;
        smallint si;
        int ii;
        long li;
        OID oi;                 /* _64BIT */
        bigint bi;
        float fl;
        double dl;
        decimal dc;
    } raw;                      /* raw value */
    iSQL_TIME tm;
    MDB_UINT32 len = 0;

    switch (field->type)
    {
    case SQL_DATA_TINYINT:
        sc_memcpy(&raw, row, sizeof(tinyint));
        iSQL_convert_binary_data_tinyint(bind_buffer_type,
                bind_length, bind_buffer, bind_buffer_length, raw.ti);
        len = sizeof(tinyint);
        break;
    case SQL_DATA_SMALLINT:
        sc_memcpy(&raw, row, sizeof(smallint));
        iSQL_convert_binary_data_smallint(bind_buffer_type,
                bind_length, bind_buffer, bind_buffer_length, raw.si);
        len = sizeof(smallint);
        break;
    case SQL_DATA_INT:
        sc_memcpy(&raw, row, sizeof(int));
        iSQL_convert_binary_data_int(bind_buffer_type,
                bind_length, bind_buffer, bind_buffer_length, raw.ii);
        len = sizeof(int);
        break;
    case SQL_DATA_BIGINT:
        sc_memcpy(&raw, row, sizeof(bigint));
        iSQL_convert_binary_data_bigint(bind_buffer_type,
                bind_length, bind_buffer, bind_buffer_length, raw.bi);
        len = sizeof(bigint);
        break;
    case SQL_DATA_FLOAT:
        sc_memcpy(&raw, row, sizeof(float));
        iSQL_convert_binary_data_float(bind_buffer_type,
                bind_length, bind_buffer, bind_buffer_length, raw.fl);
        len = sizeof(float);
        break;
    case SQL_DATA_DOUBLE:
        sc_memcpy(&raw, row, sizeof(double));
        iSQL_convert_binary_data_double(bind_buffer_type,
                bind_length, bind_buffer, bind_buffer_length, raw.dl);
        len = sizeof(double);
        break;
    case SQL_DATA_DECIMAL:
        sc_memcpy(&raw, row, sizeof(decimal));
        iSQL_convert_binary_data_decimal(bind_buffer_type,
                bind_length, bind_buffer, bind_buffer_length, raw.dc);
        len = sizeof(decimal);
        break;
    case SQL_DATA_CHAR:
    case SQL_DATA_VARCHAR:
        iSQL_convert_binary_data_char(bind_buffer_type,
                bind_length, bind_buffer, bind_buffer_length, row, field);
        len = field->buffer_length;
        break;
    case SQL_DATA_DATE:
        sc_memcpy(&tm, row, sizeof(iSQL_TIME));
        iSQL_convert_binary_data_date(bind_buffer_type,
                bind_length, bind_buffer, bind_buffer_length, &tm);
        len = sizeof(iSQL_TIME);
        break;
    case SQL_DATA_TIME:
        sc_memcpy(&tm, row, sizeof(iSQL_TIME));
        iSQL_convert_binary_data_time(bind_buffer_type,
                bind_length, bind_buffer, bind_buffer_length, &tm);
        len = sizeof(iSQL_TIME);
        break;
    case SQL_DATA_DATETIME:
        sc_memcpy(&tm, row, sizeof(iSQL_TIME));
        iSQL_convert_binary_data_datetime(bind_buffer_type,
                bind_length, bind_buffer, bind_buffer_length, &tm);
        len = sizeof(iSQL_TIME);
        break;
    case SQL_DATA_TIMESTAMP:
        sc_memcpy(&tm, row, sizeof(iSQL_TIME));
        iSQL_convert_binary_data_timestamp(bind_buffer_type,
                bind_length, bind_buffer, bind_buffer_length, &tm);
        len = sizeof(iSQL_TIME);
        break;
    case SQL_DATA_OID:
        sc_memcpy(&raw, row, sizeof(OID));
        iSQL_convert_binary_data_oid(bind_buffer_type,
                bind_length, bind_buffer, bind_buffer_length, raw.oi);
        len = sizeof(OID);
        break;
    default:
        break;
    }

    return len;
}

static int
iSQL_convert_binary_data_tinyint(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, tinyint ti)
{
    char buf[32];

    switch (bind_buffer_type)
    {
    case SQL_DATA_TINYINT:
        *(tinyint *) (bind_buffer) = (tinyint) ti;
        if (bind_length)
            *(bind_length) = sizeof(tinyint);
        break;
    case SQL_DATA_SMALLINT:
        *(smallint *) (bind_buffer) = (smallint) ti;
        if (bind_length)
            *(bind_length) = sizeof(smallint);
        break;
    case SQL_DATA_INT:
        *(int *) (bind_buffer) = (int) ti;
        if (bind_length)
            *(bind_length) = sizeof(int);
        break;
    case SQL_DATA_BIGINT:
        *(bigint *) (bind_buffer) = (bigint) ti;
        if (bind_length)
            *(bind_length) = sizeof(bigint);
        break;
    case SQL_DATA_FLOAT:
        *(float *) (bind_buffer) = (float) ti;
        if (bind_length)
            *(bind_length) = sizeof(float);
        break;
    case SQL_DATA_DOUBLE:
        *(double *) (bind_buffer) = (double) ti;
        if (bind_length)
            *(bind_length) = sizeof(double);
        break;
    case SQL_DATA_DECIMAL:
        *(decimal *) (bind_buffer) = (decimal) ti;
        if (bind_length)
            *(bind_length) = sizeof(decimal);
        break;
    case SQL_DATA_CHAR:
        if (bind_length)
            *(bind_length) =
                    sc_snprintf(bind_buffer, bind_buffer_length, "%d",
                    (int) ti);
        break;
    case SQL_DATA_VARCHAR:
        if (bind_length)
            *(bind_length) =
                    sc_snprintf(bind_buffer, bind_buffer_length, "%d",
                    (int) ti);
        break;
    case SQL_DATA_DATE:
    case SQL_DATA_TIME:
    case SQL_DATA_DATETIME:
    case SQL_DATA_TIMESTAMP:
        sc_snprintf(buf, sizeof(buf), "%d", (int) ti);
        get_timest((iSQL_TIME *) bind_buffer, buf);
        if (bind_buffer_type != SQL_DATA_TIMESTAMP)
            ((iSQL_TIME *) bind_buffer)->fraction = 0;
        if (bind_length)
            *(bind_length) = sizeof(iSQL_TIME);
        break;
    default:
        break;
    }

    return RET_SUCCESS;
}

static int
iSQL_convert_binary_data_smallint(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, smallint si)
{
    char buf[32];

    switch (bind_buffer_type)
    {
    case SQL_DATA_TINYINT:
        *(tinyint *) (bind_buffer) = (tinyint) si;
        if (bind_length)
            *(bind_length) = sizeof(tinyint);
        break;
    case SQL_DATA_SMALLINT:
        *(smallint *) (bind_buffer) = (smallint) si;
        if (bind_length)
            *(bind_length) = sizeof(smallint);
        break;
    case SQL_DATA_INT:
        *(int *) (bind_buffer) = (int) si;
        if (bind_length)
            *(bind_length) = sizeof(int);
        break;
    case SQL_DATA_BIGINT:
        *(bigint *) (bind_buffer) = (bigint) si;
        if (bind_length)
            *(bind_length) = sizeof(bigint);
        break;
    case SQL_DATA_FLOAT:
        *(float *) (bind_buffer) = (float) si;
        if (bind_length)
            *(bind_length) = sizeof(float);
        break;
    case SQL_DATA_DOUBLE:
        *(double *) (bind_buffer) = (double) si;
        if (bind_length)
            *(bind_length) = sizeof(double);
        break;
    case SQL_DATA_DECIMAL:
        *(decimal *) (bind_buffer) = (decimal) si;
        if (bind_length)
            *(bind_length) = sizeof(decimal);
        break;
    case SQL_DATA_CHAR:
        if (bind_length)
            *(bind_length) =
                    sc_snprintf(bind_buffer, bind_buffer_length, "%hd", si);
        break;
    case SQL_DATA_VARCHAR:
        if (bind_length)
            *(bind_length) =
                    sc_snprintf(bind_buffer, bind_buffer_length, "%hd", si);
        break;
    case SQL_DATA_DATE:
    case SQL_DATA_TIME:
    case SQL_DATA_DATETIME:
    case SQL_DATA_TIMESTAMP:
        sc_snprintf(buf, sizeof(buf), "%hd", si);
        get_timest((iSQL_TIME *) bind_buffer, buf);
        if (bind_buffer_type != SQL_DATA_TIMESTAMP)
            ((iSQL_TIME *) bind_buffer)->fraction = 0;
        if (bind_length)
            *(bind_length) = sizeof(iSQL_TIME);
        break;
    default:
        break;
    }

    return RET_SUCCESS;
}

static int
iSQL_convert_binary_data_oid(isql_data_type bind_buffer_type, unsigned int *bind_length, char *bind_buffer, int bind_buffer_length, OID oi)     /* _64BIT */
{
    char buf[32];

    switch (bind_buffer_type)
    {
    case SQL_DATA_TINYINT:
        *(tinyint *) (bind_buffer) = (tinyint) oi;
        if (bind_length)
            *(bind_length) = sizeof(tinyint);
        break;
    case SQL_DATA_SMALLINT:
        *(smallint *) (bind_buffer) = (smallint) oi;
        if (bind_length)
            *(bind_length) = sizeof(smallint);
        break;
    case SQL_DATA_INT:
        *(int *) (bind_buffer) = (int) oi;
        if (bind_length)
            *(bind_length) = sizeof(int);
        break;
    case SQL_DATA_BIGINT:
        break;
    case SQL_DATA_FLOAT:
#if defined(_64BIT) && defined(WIN32)
        *(float *) (bind_buffer) = (float) (DB_INT64) oi;
#else
        *(float *) (bind_buffer) = (float) oi;
#endif
        if (bind_length)
            *(bind_length) = sizeof(float);
        break;
    case SQL_DATA_DOUBLE:
#if defined(_64BIT) && defined(WIN32)
        *(double *) (bind_buffer) = (double) (DB_INT64) oi;
#else
        *(double *) (bind_buffer) = (double) oi;
#endif
        if (bind_length)
            *(bind_length) = sizeof(double);
        break;
    case SQL_DATA_DECIMAL:
#if defined(_64BIT) && defined(WIN32)
        *(decimal *) (bind_buffer) = (decimal) (DB_INT64) oi;
#else
        *(decimal *) (bind_buffer) = (decimal) oi;
#endif
        if (bind_length)
            *(bind_length) = sizeof(decimal);
        break;
    case SQL_DATA_CHAR:
        if (bind_length)
            *(bind_length) =
                    sc_snprintf(bind_buffer, bind_buffer_length, "%lu", oi);
        break;
    case SQL_DATA_VARCHAR:
        if (bind_length)
            *(bind_length) =
                    sc_snprintf(bind_buffer, bind_buffer_length, "%lu", oi);
        break;
    case SQL_DATA_DATE:
    case SQL_DATA_TIME:
    case SQL_DATA_DATETIME:
    case SQL_DATA_TIMESTAMP:
        sc_snprintf(buf, sizeof(buf), "%lu", oi);
        get_timest((iSQL_TIME *) bind_buffer, buf);
        if (bind_buffer_type != SQL_DATA_TIMESTAMP)
            ((iSQL_TIME *) bind_buffer)->fraction = 0;
        if (bind_length)
            *(bind_length) = sizeof(iSQL_TIME);
        break;
    default:
        break;
    }

    return RET_SUCCESS;
}

static int
iSQL_convert_binary_data_int(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, int ii)
{
    char buf[32];

    switch (bind_buffer_type)
    {
    case SQL_DATA_TINYINT:
        *(tinyint *) (bind_buffer) = (tinyint) ii;
        if (bind_length)
            *(bind_length) = sizeof(tinyint);
        break;
    case SQL_DATA_SMALLINT:
        *(smallint *) (bind_buffer) = (smallint) ii;
        if (bind_length)
            *(bind_length) = sizeof(smallint);
        break;
    case SQL_DATA_INT:
        *(int *) (bind_buffer) = (int) ii;
        if (bind_length)
            *(bind_length) = sizeof(int);
        break;
    case SQL_DATA_BIGINT:
        *(bigint *) (bind_buffer) = (bigint) ii;
        if (bind_length)
            *(bind_length) = sizeof(bigint);
        break;
    case SQL_DATA_FLOAT:
        *(float *) (bind_buffer) = (float) ii;
        if (bind_length)
            *(bind_length) = sizeof(float);
        break;
    case SQL_DATA_DOUBLE:
        *(double *) (bind_buffer) = (double) ii;
        if (bind_length)
            *(bind_length) = sizeof(double);
        break;
    case SQL_DATA_DECIMAL:
        *(decimal *) (bind_buffer) = (decimal) ii;
        if (bind_length)
            *(bind_length) = sizeof(decimal);
        break;
    case SQL_DATA_CHAR:
        if (bind_length)
            *(bind_length) =
                    sc_snprintf(bind_buffer, bind_buffer_length, "%d", ii);
        break;
    case SQL_DATA_VARCHAR:
        if (bind_length)
            *(bind_length) =
                    sc_snprintf(bind_buffer, bind_buffer_length, "%d", ii);
        break;
    case SQL_DATA_DATE:
    case SQL_DATA_TIME:
    case SQL_DATA_DATETIME:
    case SQL_DATA_TIMESTAMP:
        sc_snprintf(buf, sizeof(buf), "%d", ii);
        get_timest((iSQL_TIME *) bind_buffer, buf);
        if (bind_buffer_type != SQL_DATA_TIMESTAMP)
            ((iSQL_TIME *) bind_buffer)->fraction = 0;
        if (bind_length)
            *(bind_length) = sizeof(iSQL_TIME);
        break;
    default:
        break;
    }

    return RET_SUCCESS;
}

static int
iSQL_convert_binary_data_bigint(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, bigint bi)
{
    char buf[32];

    switch (bind_buffer_type)
    {
    case SQL_DATA_TINYINT:
        *(tinyint *) (bind_buffer) = (tinyint) bi;
        if (bind_length)
            *(bind_length) = sizeof(tinyint);
        break;
    case SQL_DATA_SMALLINT:
        *(smallint *) (bind_buffer) = (smallint) bi;
        if (bind_length)
            *(bind_length) = sizeof(smallint);
        break;
    case SQL_DATA_INT:
        *(int *) (bind_buffer) = (int) bi;
        if (bind_length)
            *(bind_length) = sizeof(int);
        break;
    case SQL_DATA_BIGINT:
        *(bigint *) (bind_buffer) = (bigint) bi;
        if (bind_length)
            *(bind_length) = sizeof(bigint);
        break;
    case SQL_DATA_FLOAT:
        *(float *) (bind_buffer) = (float) bi;
        if (bind_length)
            *(bind_length) = sizeof(float);
        break;
    case SQL_DATA_DOUBLE:
        *(double *) (bind_buffer) = (double) bi;
        if (bind_length)
            *(bind_length) = sizeof(double);
        break;
    case SQL_DATA_DECIMAL:
        *(decimal *) (bind_buffer) = (decimal) bi;
        if (bind_length)
            *(bind_length) = sizeof(decimal);
        break;
    case SQL_DATA_CHAR:
        if (bind_length)
            *(bind_length) =
                    sc_snprintf(bind_buffer, bind_buffer_length, I64_FORMAT,
                    bi);
        break;
    case SQL_DATA_VARCHAR:
        if (bind_length)
            *(bind_length) =
                    sc_snprintf(bind_buffer, bind_buffer_length, I64_FORMAT,
                    bi);
        break;
    case SQL_DATA_DATE:
    case SQL_DATA_TIME:
    case SQL_DATA_DATETIME:
    case SQL_DATA_TIMESTAMP:
        sc_snprintf(buf, sizeof(buf), I64_FORMAT, bi);
        get_timest((iSQL_TIME *) bind_buffer, buf);
        if (bind_buffer_type != SQL_DATA_TIMESTAMP)
            ((iSQL_TIME *) bind_buffer)->fraction = 0;
        if (bind_length)
            *(bind_length) = sizeof(iSQL_TIME);
        break;
    default:
        break;
    }

    return RET_SUCCESS;
}

static int
iSQL_convert_binary_data_float(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, float fl)
{
    char buf[32];
    int len;

    switch (bind_buffer_type)
    {
    case SQL_DATA_TINYINT:
        *(tinyint *) (bind_buffer) = (tinyint) fl;
        if (bind_length)
            *(bind_length) = sizeof(tinyint);
        break;
    case SQL_DATA_SMALLINT:
        *(smallint *) (bind_buffer) = (smallint) fl;
        if (bind_length)
            *(bind_length) = sizeof(smallint);
        break;
    case SQL_DATA_INT:
        *(int *) (bind_buffer) = (int) fl;
        if (bind_length)
            *(bind_length) = sizeof(int);
        break;
    case SQL_DATA_BIGINT:
        *(bigint *) (bind_buffer) = (bigint) fl;
        if (bind_length)
            *(bind_length) = sizeof(bigint);
        break;
    case SQL_DATA_FLOAT:
        *(float *) (bind_buffer) = (float) fl;
        if (bind_length)
            *(bind_length) = sizeof(float);
        break;
    case SQL_DATA_DOUBLE:
        *(double *) (bind_buffer) = (double) fl;
        if (bind_length)
            *(bind_length) = sizeof(double);
        break;
    case SQL_DATA_DECIMAL:
        *(decimal *) (bind_buffer) = (decimal) fl;
        if (bind_length)
            *(bind_length) = sizeof(decimal);
        break;
    case SQL_DATA_CHAR:
        len = sc_snprintf(bind_buffer, bind_buffer_length, "%f", fl);
        if (sc_strchr(bind_buffer, '.'))
        {
            while (len > 0 && bind_buffer[--len] == '0');
            if (bind_buffer[len] != '.')
                ++len;
            bind_buffer[len] = '\0';
        }
        if (bind_length)
            *(bind_length) = len;
        break;
    case SQL_DATA_VARCHAR:
        len = sc_snprintf(bind_buffer, bind_buffer_length, "%f", fl);
        if (sc_strchr(bind_buffer, '.'))
        {
            while (len > 0 && bind_buffer[--len] == '0');
            if (bind_buffer[len] != '.')
                ++len;
            bind_buffer[len] = '\0';
        }
        if (bind_length)
            *(bind_length) = len;
        break;
    case SQL_DATA_DATE:
    case SQL_DATA_TIME:
    case SQL_DATA_DATETIME:
    case SQL_DATA_TIMESTAMP:
        len = sc_snprintf(buf, sizeof(buf), "%f", fl);
        if (sc_strchr(buf, '.'))
        {
            while (len > 0 && buf[--len] == '0');
            if (buf[len] != '.')
                ++len;
            buf[len] = '\0';
        }
        get_timest((iSQL_TIME *) bind_buffer, buf);
        if (bind_buffer_type != SQL_DATA_TIMESTAMP)
            ((iSQL_TIME *) bind_buffer)->fraction = 0;
        if (bind_length)
            *(bind_length) = sizeof(iSQL_TIME);
        break;
    default:
        break;
    }

    return RET_SUCCESS;
}

static int
iSQL_convert_binary_data_double(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, double dl)
{
    char buf[32];
    int len;

    switch (bind_buffer_type)
    {
    case SQL_DATA_TINYINT:
        *(tinyint *) (bind_buffer) = (tinyint) dl;
        if (bind_length)
            *(bind_length) = sizeof(tinyint);
        break;
    case SQL_DATA_SMALLINT:
        *(smallint *) (bind_buffer) = (smallint) dl;
        if (bind_length)
            *(bind_length) = sizeof(smallint);
        break;
    case SQL_DATA_INT:
        *(int *) (bind_buffer) = (int) dl;
        if (bind_length)
            *(bind_length) = sizeof(int);
        break;
    case SQL_DATA_BIGINT:
        *(bigint *) (bind_buffer) = (bigint) dl;
        if (bind_length)
            *(bind_length) = sizeof(bigint);
        break;
    case SQL_DATA_FLOAT:
        *(float *) (bind_buffer) = (float) dl;
        if (bind_length)
            *(bind_length) = sizeof(float);
        break;
    case SQL_DATA_DOUBLE:
        *(double *) (bind_buffer) = (double) dl;
        if (bind_length)
            *(bind_length) = sizeof(double);
        break;
    case SQL_DATA_DECIMAL:
        *(decimal *) (bind_buffer) = (decimal) dl;
        if (bind_length)
            *(bind_length) = sizeof(decimal);
        break;
    case SQL_DATA_CHAR:
        len = sc_snprintf(bind_buffer, bind_buffer_length, "%f", dl);
        if (sc_strchr(bind_buffer, '.'))
        {
            while (len > 0 && bind_buffer[--len] == '0');
            if (bind_buffer[len] != '.')
                ++len;
            bind_buffer[len] = '\0';
        }
        if (bind_length)
            *(bind_length) = len;
        break;
    case SQL_DATA_VARCHAR:
        len = sc_snprintf(bind_buffer, bind_buffer_length, "%f", dl);
        if (sc_strchr(bind_buffer, '.'))
        {
            while (len > 0 && bind_buffer[--len] == '0');
            if (bind_buffer[len] != '.')
                ++len;
            bind_buffer[len] = '\0';
        }
        if (bind_length)
            *(bind_length) = len;
        break;
    case SQL_DATA_DATE:
    case SQL_DATA_TIME:
    case SQL_DATA_DATETIME:
    case SQL_DATA_TIMESTAMP:
        len = sc_snprintf(buf, sizeof(buf), "%f", dl);
        if (sc_strchr(buf, '.'))
        {
            while (len > 0 && buf[--len] == '0');
            if (buf[len] != '.')
                ++len;
            buf[len] = '\0';
        }
        get_timest((iSQL_TIME *) bind_buffer, buf);
        if (bind_buffer_type != SQL_DATA_TIMESTAMP)
            ((iSQL_TIME *) bind_buffer)->fraction = 0;
        if (bind_length)
            *(bind_length) = sizeof(iSQL_TIME);
        break;
    default:
        break;
    }

    return RET_SUCCESS;
}

static int
iSQL_convert_binary_data_decimal(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, decimal dc)
{
    char buf[32];
    int len;

    switch (bind_buffer_type)
    {
    case SQL_DATA_TINYINT:
        *(tinyint *) (bind_buffer) = (tinyint) dc;
        if (bind_length)
            *(bind_length) = sizeof(tinyint);
        break;
    case SQL_DATA_SMALLINT:
        *(smallint *) (bind_buffer) = (smallint) dc;
        if (bind_length)
            *(bind_length) = sizeof(smallint);
        break;
    case SQL_DATA_INT:
        *(int *) (bind_buffer) = (int) dc;
        if (bind_length)
            *(bind_length) = sizeof(int);
        break;
    case SQL_DATA_BIGINT:
        *(bigint *) (bind_buffer) = (bigint) dc;
        if (bind_length)
            *(bind_length) = sizeof(bigint);
        break;
    case SQL_DATA_FLOAT:
        *(float *) (bind_buffer) = (float) dc;
        if (bind_length)
            *(bind_length) = sizeof(float);
        break;
    case SQL_DATA_DOUBLE:
        *(double *) (bind_buffer) = (double) dc;
        if (bind_length)
            *(bind_length) = sizeof(double);
        break;
    case SQL_DATA_DECIMAL:
        *(decimal *) (bind_buffer) = (decimal) dc;
        if (bind_length)
            *(bind_length) = sizeof(decimal);
        break;
    case SQL_DATA_CHAR:
        len = sc_snprintf(bind_buffer, bind_buffer_length, "%f", dc);
        if (sc_strchr(bind_buffer, '.'))
        {
            while (len > 0 && bind_buffer[--len] == '0');
            if (bind_buffer[len] != '.')
                ++len;
            bind_buffer[len] = '\0';
        }
        if (bind_length)
            *(bind_length) = len;
        break;
    case SQL_DATA_VARCHAR:
        len = sc_snprintf(bind_buffer, bind_buffer_length, "%f", dc);
        if (sc_strchr(bind_buffer, '.'))
        {
            while (len > 0 && bind_buffer[--len] == '0');
            if (bind_buffer[len] != '.')
                ++len;
            bind_buffer[len] = '\0';
        }
        if (bind_length)
            *(bind_length) = len;
        break;
    case SQL_DATA_DATE:
    case SQL_DATA_TIME:
    case SQL_DATA_DATETIME:
    case SQL_DATA_TIMESTAMP:
        len = sc_snprintf(buf, sizeof(buf), "%f", dc);
        if (sc_strchr(buf, '.'))
        {
            while (len > 0 && buf[--len] == '0');
            if (buf[len] != '.')
                ++len;
            buf[len] = '\0';
        }
        get_timest((iSQL_TIME *) bind_buffer, buf);
        if (bind_buffer_type != SQL_DATA_TIMESTAMP)
            ((iSQL_TIME *) bind_buffer)->fraction = 0;
        if (bind_length)
            *(bind_length) = sizeof(iSQL_TIME);
        break;
    default:
        break;
    }

    return RET_SUCCESS;
}

static int
iSQL_convert_binary_data_char(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, char *row,
        iSQL_FIELD * field)
{
    unsigned int value_length;

    if (field->type == SQL_DATA_VARCHAR)
    {
        sc_memcpy(&value_length, row, INT_SIZE);
        row += (unsigned short) INT_SIZE;
    }

    switch (bind_buffer_type)
    {
    case SQL_DATA_TINYINT:
        *(tinyint *) (bind_buffer) = (tinyint) sc_atoi(row);
        if (bind_length)
            *(bind_length) = sizeof(tinyint);
        break;
    case SQL_DATA_SMALLINT:
        *(smallint *) (bind_buffer) = (smallint) sc_atoi(row);
        if (bind_length)
            *(bind_length) = sizeof(smallint);
        break;
    case SQL_DATA_INT:
        *(int *) (bind_buffer) = (int) sc_atoi(row);
        if (bind_length)
            *(bind_length) = sizeof(int);
        break;
    case SQL_DATA_BIGINT:
        *(bigint *) (bind_buffer) = (bigint) sc_atoll(row);
        if (bind_length)
            *(bind_length) = sizeof(bigint);
        break;
    case SQL_DATA_FLOAT:
        *(float *) (bind_buffer) = (float) sc_atof(row);
        if (bind_length)
            *(bind_length) = sizeof(float);
        break;
    case SQL_DATA_DOUBLE:
        *(double *) (bind_buffer) = (double) sc_atof(row);
        if (bind_length)
            *(bind_length) = sizeof(double);
        break;
    case SQL_DATA_DECIMAL:
        *(decimal *) (bind_buffer) = (decimal) sc_atof(row);
        if (bind_length)
            *(bind_length) = sizeof(decimal);
        break;
    case SQL_DATA_CHAR:
        if (bind_buffer_length == 0)
            break;
        if (field->type == SQL_DATA_VARCHAR)
        {
            sc_memcpy(bind_buffer, row, value_length);
            bind_buffer[value_length] = '\0';
            if (bind_length)
                *bind_length = value_length;
            row += (unsigned short) (value_length);
        }
        else
        {
            if (bind_buffer_length <= (int) field->buffer_length)
            {
                sc_memcpy(bind_buffer, row, bind_buffer_length);
                bind_buffer[bind_buffer_length - 1] = '\0';
            }
            else
            {
                sc_memcpy(bind_buffer, row, field->buffer_length);
                bind_buffer[field->buffer_length] = '\0';
            }
            if (bind_length)
                *bind_length = sc_strlen(bind_buffer);
        }
        break;
    case SQL_DATA_VARCHAR:
        if (bind_buffer_length == 0)
            break;
        if (field->type == SQL_DATA_VARCHAR)
        {
            sc_memcpy(bind_buffer, row, value_length);
            bind_buffer[value_length] = '\0';
            if (bind_length)
                *bind_length = value_length;
            row += (unsigned short) (value_length);
        }
        else
        {
            if (bind_buffer_length <= (int) field->buffer_length)
            {
                sc_memcpy(bind_buffer, row, bind_buffer_length);
                if (bind_length)
                    *bind_length = bind_buffer_length - 1;
                bind_buffer[bind_buffer_length - 1] = '\0';
            }
            else
            {
                sc_memcpy(bind_buffer, row, field->buffer_length);
                if (bind_length)
                    *bind_length = field->buffer_length;
                bind_buffer[field->buffer_length] = '\0';
            }
        }
        break;
    case SQL_DATA_DATE:
    case SQL_DATA_TIME:
    case SQL_DATA_DATETIME:
    case SQL_DATA_TIMESTAMP:
        get_timest((iSQL_TIME *) bind_buffer, row);
        if (bind_buffer_type != SQL_DATA_TIMESTAMP)
            ((iSQL_TIME *) bind_buffer)->fraction = 0;
        if (bind_length)
            *(bind_length) = sizeof(iSQL_TIME);
        break;
    default:
        break;
    }

    return RET_SUCCESS;
}

static int
iSQL_convert_binary_data_date(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, iSQL_TIME * tm)
{
    char buf[32];
    int len;

    len = sc_snprintf(buf, sizeof(buf), "%hd%hd%hd",
            tm->year, tm->month, tm->day);

    switch (bind_buffer_type)
    {
    case SQL_DATA_TINYINT:
        *(tinyint *) (bind_buffer) = (tinyint) sc_atoll(buf);
        if (bind_length)
            *(bind_length) = sizeof(tinyint);
        break;
    case SQL_DATA_SMALLINT:
        *(smallint *) (bind_buffer) = (smallint) sc_atoll(buf);
        if (bind_length)
            *(bind_length) = sizeof(smallint);
        break;
    case SQL_DATA_INT:
        *(int *) (bind_buffer) = (int) sc_atoll(buf);
        if (bind_length)
            *(bind_length) = sizeof(int);
        break;
    case SQL_DATA_BIGINT:
        *(bigint *) (bind_buffer) = (bigint) sc_atoll(buf);
        if (bind_length)
            *(bind_length) = sizeof(bigint);
        break;
    case SQL_DATA_FLOAT:
        *(float *) (bind_buffer) = (float) sc_atof(buf);
        if (bind_length)
            *(bind_length) = sizeof(float);
        break;
    case SQL_DATA_DOUBLE:
        *(double *) (bind_buffer) = (double) sc_atof(buf);
        if (bind_length)
            *(bind_length) = sizeof(double);
        break;
    case SQL_DATA_DECIMAL:
        *(decimal *) (bind_buffer) = (decimal) sc_atof(buf);
        if (bind_length)
            *(bind_length) = sizeof(decimal);
        break;
    case SQL_DATA_CHAR:
        len = sc_snprintf(bind_buffer, bind_buffer_length,
                "%04hd-%02hd-%02hd", tm->year, tm->month, tm->day);
        if (bind_length)
            *(bind_length) = len;
        break;
    case SQL_DATA_VARCHAR:
        len = sc_snprintf(bind_buffer, bind_buffer_length,
                "%04hd-%02hd-%02hd", tm->year, tm->month, tm->day);
        if (bind_length)
            *(bind_length) = len;
        break;
    case SQL_DATA_DATE:
    case SQL_DATA_DATETIME:
        sc_memcpy(bind_buffer, tm, sizeof(iSQL_TIME));
        if (bind_length)
            *(bind_length) = sizeof(iSQL_TIME);
        break;
    case SQL_DATA_TIMESTAMP:
        sc_memcpy(bind_buffer, tm, sizeof(iSQL_TIME));
        if (bind_length)
            *(bind_length) = sizeof(iSQL_TIME);
        break;
    default:
        break;
    }

    return RET_SUCCESS;
}

static int
iSQL_convert_binary_data_time(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, iSQL_TIME * tm)
{
    char buf[32];
    int len;

    len = sc_snprintf(buf, sizeof(buf), "%hd%hd%hd",
            tm->hour, tm->minute, tm->second);

    switch (bind_buffer_type)
    {
    case SQL_DATA_TINYINT:
        *(tinyint *) (bind_buffer) = (tinyint) sc_atoll(buf);
        if (bind_length)
            *(bind_length) = sizeof(tinyint);
        break;
    case SQL_DATA_SMALLINT:
        *(smallint *) (bind_buffer) = (smallint) sc_atoll(buf);
        if (bind_length)
            *(bind_length) = sizeof(smallint);
        break;
    case SQL_DATA_INT:
        *(int *) (bind_buffer) = (int) sc_atoll(buf);
        if (bind_length)
            *(bind_length) = sizeof(int);
        break;
    case SQL_DATA_BIGINT:
        *(bigint *) (bind_buffer) = (bigint) sc_atoll(buf);
        if (bind_length)
            *(bind_length) = sizeof(bigint);
        break;
    case SQL_DATA_FLOAT:
        *(float *) (bind_buffer) = (float) sc_atof(buf);
        if (bind_length)
            *(bind_length) = sizeof(float);
        break;
    case SQL_DATA_DOUBLE:
        *(double *) (bind_buffer) = (double) sc_atof(buf);
        if (bind_length)
            *(bind_length) = sizeof(double);
        break;
    case SQL_DATA_DECIMAL:
        *(decimal *) (bind_buffer) = (decimal) sc_atof(buf);
        if (bind_length)
            *(bind_length) = sizeof(decimal);
        break;
    case SQL_DATA_CHAR:
        len = sc_snprintf(bind_buffer, bind_buffer_length,
                "%02hd:%02hd:%02hd", tm->hour, tm->minute, tm->second);
        if (bind_length)
            *(bind_length) = len;
        break;
    case SQL_DATA_VARCHAR:
        len = sc_snprintf(bind_buffer, bind_buffer_length,
                "%02hd:%02hd:%02hd", tm->hour, tm->minute, tm->second);
        if (bind_length)
            *(bind_length) = len;
        break;
    case SQL_DATA_DATE:
    case SQL_DATA_TIME:
    case SQL_DATA_DATETIME:
        sc_memcpy(bind_buffer, tm, sizeof(iSQL_TIME));
        if (bind_length)
            *(bind_length) = sizeof(iSQL_TIME);
        break;
    case SQL_DATA_TIMESTAMP:
        sc_memcpy(bind_buffer, tm, sizeof(iSQL_TIME));
        if (bind_length)
            *(bind_length) = sizeof(iSQL_TIME);
        break;
    default:
        break;
    }

    return RET_SUCCESS;
}

static int
iSQL_convert_binary_data_datetime(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, iSQL_TIME * tm)
{
    char buf[32];
    int len;

    len = sc_snprintf(buf, sizeof(buf), "%hd%hd%hd%hd%hd%hd",
            tm->year, tm->month, tm->day, tm->hour, tm->minute, tm->second);

    switch (bind_buffer_type)
    {
    case SQL_DATA_TINYINT:
        *(tinyint *) (bind_buffer) = (tinyint) sc_atoll(buf);
        if (bind_length)
            *(bind_length) = sizeof(tinyint);
        break;
    case SQL_DATA_SMALLINT:
        *(smallint *) (bind_buffer) = (smallint) sc_atoll(buf);
        if (bind_length)
            *(bind_length) = sizeof(smallint);
        break;
    case SQL_DATA_INT:
        *(int *) (bind_buffer) = (int) sc_atoll(buf);
        if (bind_length)
            *(bind_length) = sizeof(int);
        break;
    case SQL_DATA_BIGINT:
        *(bigint *) (bind_buffer) = (bigint) sc_atoll(buf);
        if (bind_length)
            *(bind_length) = sizeof(bigint);
        break;
    case SQL_DATA_FLOAT:
        *(float *) (bind_buffer) = (float) sc_atof(buf);
        if (bind_length)
            *(bind_length) = sizeof(float);
        break;
    case SQL_DATA_DOUBLE:
        *(double *) (bind_buffer) = (double) sc_atof(buf);
        if (bind_length)
            *(bind_length) = sizeof(double);
        break;
    case SQL_DATA_DECIMAL:
        *(decimal *) (bind_buffer) = (decimal) sc_atof(buf);
        if (bind_length)
            *(bind_length) = sizeof(decimal);
        break;
    case SQL_DATA_CHAR:
        len = sc_snprintf(bind_buffer, bind_buffer_length,
                "%04hd-%02hd-%02hd %02hd:%02hd:%02hd",
                tm->year, tm->month, tm->day,
                tm->hour, tm->minute, tm->second);
        if (bind_length)
            *(bind_length) = len;
        break;
    case SQL_DATA_VARCHAR:
        len = sc_snprintf(bind_buffer, bind_buffer_length,
                "%04hd-%02hd-%02hd %02hd:%02hd:%02hd",
                tm->year, tm->month, tm->day,
                tm->hour, tm->minute, tm->second);
        if (bind_length)
            *(bind_length) = len;
        break;
    case SQL_DATA_DATE:
    case SQL_DATA_TIME:
    case SQL_DATA_DATETIME:
        sc_memcpy(bind_buffer, tm, sizeof(iSQL_TIME));
        if (bind_length)
            *(bind_length) = sizeof(iSQL_TIME);
        break;
    case SQL_DATA_TIMESTAMP:
        sc_memcpy(bind_buffer, tm, sizeof(iSQL_TIME));
        if (bind_length)
            *(bind_length) = sizeof(iSQL_TIME);
        break;
    default:
        break;
    }

    return RET_SUCCESS;
}

static int
iSQL_convert_binary_data_timestamp(isql_data_type bind_buffer_type,
        unsigned int *bind_length,
        char *bind_buffer, int bind_buffer_length, iSQL_TIME * tm)
{
    char buf[32];
    int len;

    len = sc_snprintf(buf,
            sizeof(buf), "%hd%hd%hd%hd%hd%hd%hd",
            tm->year, tm->month, tm->day,
            tm->hour, tm->minute, tm->second, tm->fraction);

    switch (bind_buffer_type)
    {
    case SQL_DATA_TINYINT:
        *(tinyint *) (bind_buffer) = (tinyint) sc_atoll(buf);
        if (bind_length)
            *(bind_length) = sizeof(tinyint);
        break;
    case SQL_DATA_SMALLINT:
        *(smallint *) (bind_buffer) = (smallint) sc_atoll(buf);
        if (bind_length)
            *(bind_length) = sizeof(smallint);
        break;
    case SQL_DATA_INT:
        *(int *) (bind_buffer) = (int) sc_atoll(buf);
        if (bind_length)
            *(bind_length) = sizeof(int);
        break;
    case SQL_DATA_BIGINT:
        *(bigint *) (bind_buffer) = (bigint) sc_atoll(buf);
        if (bind_length)
            *(bind_length) = sizeof(bigint);
        break;
    case SQL_DATA_FLOAT:
        *(float *) (bind_buffer) = (float) sc_atof(buf);
        if (bind_length)
            *(bind_length) = sizeof(float);
        break;
    case SQL_DATA_DOUBLE:
        *(double *) (bind_buffer) = (double) sc_atof(buf);
        if (bind_length)
            *(bind_length) = sizeof(double);
        break;
    case SQL_DATA_DECIMAL:
        *(decimal *) (bind_buffer) = (decimal) sc_atof(buf);
        if (bind_length)
            *(bind_length) = sizeof(decimal);
        break;
    case SQL_DATA_CHAR:
        len = sc_snprintf(bind_buffer, bind_buffer_length,
                "%04hd-%02hd-%02hd %02hd:%02hd:%02hd.%03d",
                tm->year, tm->month, tm->day,
                tm->hour, tm->minute, tm->second, tm->fraction);
        if (bind_length)
            *(bind_length) = len;
        break;
    case SQL_DATA_VARCHAR:
        len = sc_snprintf(bind_buffer, bind_buffer_length,
                "%04hd-%02hd-%02hd %02hd:%02hd:%02hd.%03d",
                tm->year, tm->month, tm->day,
                tm->hour, tm->minute, tm->second, tm->fraction);
        if (bind_length)
            *(bind_length) = len;
        break;
    case SQL_DATA_DATE:
    case SQL_DATA_TIME:
    case SQL_DATA_DATETIME:
    case SQL_DATA_TIMESTAMP:
        sc_memcpy(bind_buffer, tm, sizeof(iSQL_TIME));
        if (bind_buffer_type != SQL_DATA_TIMESTAMP)
            ((iSQL_TIME *) bind_buffer)->fraction = 0;
        if (bind_length)
            *(bind_length) = sizeof(iSQL_TIME);
        break;
    default:
        break;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! stmt_fetch_row 
/*! \breif result set 중에서 다음 result set을 뽑아낸다
 ************************************
 * \param stmt(in)       :
 * \param row(in/out)    : 
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - reference : copy_one_binary_column()
 *  - called : iSQL_fetch
 *  - BYTE/VARBYTE의 지원..
 *      레코드 포맷이.. MEMORY RECORD FORMAT과 동일하다..
 *      쩝.. 특정 함수를 참조하도록 나중에 주석을 달것
 *****************************************************************************/
static int
stmt_fetch_row(iSQL_STMT * stmt, char *row)
{
    iSQL_BIND *bind = stmt->rbind;
    iSQL_FIELD *fields = stmt->fields;
    char *null_ptr, null_bit;
    MDB_UINT32 i;

    int value_length;

    null_ptr = row;
    null_bit = 1;
    row += ((stmt->field_count - 1) >> 3) + 1;  /* skip null bit area */

    for (i = 0; i < stmt->field_count; i++)
    {
        if (*null_ptr & null_bit)
        {
            if (bind[i].is_null)
                *bind[i].is_null = 1;
        }
        else
        {
            if (bind[i].is_null)
                *bind[i].is_null = 0;
            if (bind[i].buffer_type == (isql_data_type) fields[i].type)
            {
                switch (bind[i].buffer_type)
                {
                case SQL_DATA_TINYINT:
                    bind[i].buffer[0] = row[0];
                    if (bind[i].length)
                        *bind[i].length = fields[i].buffer_length;
                    row += fields[i].buffer_length;
                    break;
                case SQL_DATA_INT:
                    if (((unsigned long) row & 0x3) == 0 &&
                            ((unsigned long) bind[i].buffer & 0x3) == 0)
                    {
                        *(int *) (bind[i].buffer) = *(int *) row;
                        if (bind[i].length)
                            *bind[i].length = fields[i].buffer_length;
                        row += fields[i].buffer_length;
                        break;
                    }
                case SQL_DATA_SMALLINT:
                case SQL_DATA_BIGINT:
                case SQL_DATA_FLOAT:
                case SQL_DATA_DOUBLE:
                case SQL_DATA_DECIMAL:
                case SQL_DATA_OID:
                    sc_memcpy(bind[i].buffer, row, fields[i].buffer_length);
                    if (bind[i].length)
                        *bind[i].length = fields[i].buffer_length;
                    row += fields[i].buffer_length;
                    break;
                case SQL_DATA_CHAR:
                    if (bind[i].buffer_length == 0)
                        break;
                    if (bind[i].buffer_length <= fields[i].buffer_length)
                    {
                        sc_memcpy(bind[i].buffer, row, bind[i].buffer_length);
                        if (bind[i].length)
                            *bind[i].length = bind[i].buffer_length - 1;
                        bind[i].buffer[bind[i].buffer_length - 1] = '\0';
                    }
                    else
                    {
                        sc_memcpy(bind[i].buffer, row,
                                fields[i].buffer_length);
                        if (bind[i].length)
                            *bind[i].length = fields[i].buffer_length;
                        bind[i].buffer[fields[i].buffer_length] = '\0';
                    }
                    row += fields[i].buffer_length;
                    break;
                case SQL_DATA_VARCHAR:
                    if (bind[i].buffer_length == 0)
                        break;
                    sc_memcpy(&value_length, row, INT_SIZE);

                    sc_memcpy(bind[i].buffer, row + INT_SIZE, value_length);
                    bind[i].buffer[value_length] = '\0';
                    if (bind[i].length)
                        *bind[i].length = value_length;
                    row += (unsigned short) (value_length + INT_SIZE);
                    break;

                case SQL_DATA_NCHAR:
                    if (bind[i].buffer_length == 0)
                        break;

                    if (bind[i].buffer_length <= fields[i].buffer_length)
                    {
                        DB_WCHAR *wptr = (DB_WCHAR *) bind[i].buffer;

                        sc_wmemcpy(wptr, (DB_WCHAR *) row,
                                bind[i].buffer_length);

                        if (bind[i].length)
                            *bind[i].length = bind[i].buffer_length - 1;

                        wptr[bind[i].buffer_length - 1] = L'\0';
                    }
                    else
                    {
                        DB_WCHAR *wptr = (DB_WCHAR *) bind[i].buffer;

                        sc_wmemcpy(wptr, (DB_WCHAR *) row,
                                fields[i].buffer_length);
                        if (bind[i].length)
                            *bind[i].length = fields[i].buffer_length;

                        wptr[fields[i].buffer_length] = L'\0';
                    }
                    row += fields[i].buffer_length * sizeof(DB_WCHAR);
                    break;

                case SQL_DATA_NVARCHAR:
                    if (bind[i].buffer_length == 0)
                        break;

                    sc_memcpy(&value_length, row, INT_SIZE);
                    {
                        DB_WCHAR *wptr = (DB_WCHAR *) bind[i].buffer;

                        sc_wmemcpy(wptr, (DB_WCHAR *) (row + INT_SIZE),
                                value_length);
                        wptr[value_length] = L'\0';
                    }
                    if (bind[i].length)
                        *bind[i].length = value_length;

                    row += (unsigned short)
                            (value_length * sizeof(DB_WCHAR) + INT_SIZE);
                    break;
                case SQL_DATA_BYTE:
                case SQL_DATA_VARBYTE:
                    if (bind[i].buffer_length == 0)
                        break;

                    sc_memcpy(&value_length, row, INT_SIZE);

                    sc_memcpy(bind[i].buffer, row + INT_SIZE, value_length);
                    if (bind[i].length)
                        *bind[i].length = value_length;
                    row += (unsigned short) (value_length + INT_SIZE);
                    break;

                case SQL_DATA_DATE:
                case SQL_DATA_TIME:
                case SQL_DATA_TIMESTAMP:
                case SQL_DATA_DATETIME:
                    sc_memcpy(bind[i].buffer, row, fields[i].buffer_length);
                    if (bind[i].length)
                        *bind[i].length = fields[i].buffer_length;
                    row += fields[i].buffer_length;
                    break;
                default:
                    break;
                }
            }
            else
            {
                get_converted_binary_data(&bind[i], &fields[i], row);
                if (IS_NBS(fields[i].type))
                    row += (fields[i].buffer_length * sizeof(DB_WCHAR));
                else if (IS_BYTE(fields[i].type))
                    row += fields[i].buffer_length + INT_SIZE;
                else
                    row += fields[i].buffer_length;
            }
        }

        if (null_bit & 0x7F)
        {
            null_bit <<= 1;
        }
        else
        {
            null_bit = 1;
            null_ptr++;
        }
    }

    return 0;
}

/*****************************************************************************/
//! isql_reconnect 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql :
 ************************************
 * \return  return_value
 ************************************
 * \note    
 *        mobile용 task lock 추가
 *****************************************************************************/
static int
isql_reconnect(iSQL * isql)
{
    return SQL_E_NOTCONNECTED;
}

/*****************************************************************************/
//! iSQL_init 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX iSQL *
iSQL_init(iSQL * isql)
{
    if (!isql)
    {
        isql = SMEM_ALLOC(sizeof(iSQL));
        if (!isql)
            return NULL;
        isql->free_me = 1;
    }
    else
        isql->free_me = 0;

    isql->status = iSQL_STAT_NOT_READY;
    isql->dbname[0] = '\0';
    isql->port = 0;
    isql->handle = -1;
    isql->flags = OPT_DEFAULT;
    isql->fields = NULL;
    isql->field_count = 0;
    isql->affected_rows = 0;
    isql->affected_rid = 0;
    isql->last_errno = 0;
    isql->last_error = NULL;
    isql->last_querytype = SQL_STMT_NONE;
    isql->data = NULL;
    isql->clientType = CS_CLIENT_NORMAL;

    return isql;
}

/*****************************************************************************/
//! iSQL_options 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql : 
 * \param opt :
 * \param optval :
 * \param optlen :
 ************************************
 * \return  ret < 0 is Error.
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX int
iSQL_options(iSQL * isql, const isql_option opt, const void *optval,
        int optlen)
{
    int ret = 0;
    int _g_connid_bk = -2;
    int index;
    char tmp[32];

    if (isql->status != iSQL_STAT_READY)
    {
        ret = -1;
        goto end;
    }

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    /* THREAD_HANDLE could be refered only after call to PUSH_G_CONNID() */
    index = THREAD_HANDLE;

    if (gClients[index].status == iSQL_STAT_CONNECTED)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTLOGON, 0);
        POP_G_CONNID_SQL_API();
        MDB_DB_UNLOCK_SQL_API();
        goto end;
    }

    switch (opt)
    {
    case OPT_AUTOCOMMIT:
    case OPT_TIME:
    case OPT_HEADING:
    case OPT_FEEDBACK:
    case OPT_RECONNECT:
        ret = SQL_E_SUCCESS;
        if (*(int *) optval)
            gClients[index].flags |= opt;
        else
            gClients[index].flags &= ~opt;
        break;
    case OPT_PLAN_ON:
        ret = SQL_E_SUCCESS;
        gClients[index].flags &= ~(OPT_PLAN_OFF + OPT_PLAN_ONLY);
        gClients[index].flags |= OPT_PLAN_ON;
        break;
    case OPT_PLAN_OFF:
        ret = SQL_E_SUCCESS;
        gClients[index].flags &= ~(OPT_PLAN_ON + OPT_PLAN_ONLY);
        gClients[index].flags |= OPT_PLAN_OFF;
        break;
    case OPT_PLAN_ONLY:
        ret = SQL_E_SUCCESS;
        gClients[index].flags &= ~(OPT_PLAN_ON + OPT_PLAN_OFF);
        gClients[index].flags |= OPT_PLAN_ONLY;
        break;
    default:
        isql->last_errno = SQL_E_ERROR;
        sc_snprintf(tmp, 32, "Unknown option(%x).", opt);
        er_set_parser(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, tmp);
        break;
    }

    isql->flags = gClients[index].flags;

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (ret < 1)
    {
        ret = -1;
        goto end;
    }

  end:

    return ret;
}

/*****************************************************************************/
//! iSQL_connect 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql :
 * \param dbhost : 
 * \param dbname :
 * \param user :
 * \param passwd :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int _init_db = 0;

__DECL_PREFIX int
iSQL_initdb(void)
{
    int ret = sc_db_lock_init();

    if (ret == 0)
        _init_db = 1;

    return ret;
}

__DECL_PREFIX iSQL *
iSQL_connect(iSQL * isql, char *dbhost, char *dbname, char *user, char *passwd)
{
    isql = iSQL_init(isql);
    if (!isql)
        return NULL;

    isql->clientType = CS_CLIENT_NORMAL;

    if (_init_db == 0)
    {
        iSQL_initdb();
        if (_init_db == 0)
            return NULL;
    }

    MDB_DB_LOCK_SQL_API();

    if (!iSQL_real_connect(isql, dbhost, dbname, user, passwd, 0, isql->flags))
    {
        if (isql->free_me)
            SMEM_FREE(isql);
        isql = NULL;
    }

    MDB_DB_UNLOCK_SQL_API();

    return isql;
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
__DECL_PREFIX iSQL *
iSQL_real_connect(iSQL * isql, char *dbhost, char *dbname, char *user,
        char *passwd, unsigned short port, unsigned int flags)
{
    char localaddr[] = "127.0.0.1";
    int ret = 0;
    int index;
    extern T_CLIENT *gClients;
    int InsertClient();
    int RemoveClient();
    int _g_connid_bk = -2;

    if (!dbhost)
        dbhost = localaddr;

    if (!dbname)
    {
        isql->last_errno = SQL_E_INVALIDARGUMENT;
        SMEM_FREENUL(isql->last_error);
        return NULL;
    }

    isql->handle = CS_Connect(dbhost, dbname, user, passwd, isql->clientType);
    if (isql->handle < 0)
    {
        isql->last_errno = isql->handle;
        SMEM_FREENUL(isql->last_error);
        return NULL;
    }

    MDB_DB_LOCK_SQL_API();

    er_clear();

    /* It's a new connection. Should we FREENUL last error? - XXX */
    SMEM_FREENUL(isql->last_error);

    PUSH_G_CONNID_SQL_API(-1);  /* connid will be set in InsertClient() */

    index = InsertClient(dbhost, dbname, user, passwd, flags);
    if (index < 0)
    {
        ret = er_errid();
        goto DONE;
    }

    ret = dbi_Connect_Server(isql->handle, "127.0.0.1", dbname, user, passwd);
    if (ret < 0)
    {
        RemoveClient();
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        goto DONE;
    }

    gClients[index].DBHandle = isql->handle;
    isql->flags |= 0;
    isql->server_handle = isql->handle;
    ret = SQL_E_SUCCESS;

  DONE:
    POP_G_CONNID_SQL_API();


    MDB_DB_UNLOCK_SQL_API();

    if (ret < 0)
    {
        close_connection(isql);
        return NULL;
    }

    isql->port = port;

    sc_strncpy(isql->dbname, dbname, 255);
    isql->dbname[255] = '\0';

    if (user)
        isql->status = iSQL_STAT_READY;
    else
        isql->status = iSQL_STAT_CONNECTED;
    CS_Set_iSQL(isql->handle, (void *) isql);

    return isql;
}

/*****************************************************************************/
//! iSQL_disconnect 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX int
iSQL_disconnect(iSQL * isql)
{
    int ret = SQL_E_NOERROR;
    int _g_connid_bk = -2;
    T_STATEMENT *stmt;
    int i;
    int index;
    void release_all_statements(T_SQL * sql);

    if (isql->last_querytype == SQL_STMT_SELECT
            || isql->last_querytype == SQL_STMT_DESCRIBE)
    {
        SMEM_FREENUL(isql->fields);
    }

    isql->dbname[0] = '\0';
    SMEM_FREENUL(isql->last_error);

    if (isql->handle >= 0)
    {
        MDB_DB_LOCK_SQL_API();
        PUSH_G_CONNID_SQL_API(isql->handle);

        er_clear();

        index = THREAD_HANDLE;
        if (gClients[index].status == iSQL_STAT_NOT_READY)
        {
            POP_G_CONNID_SQL_API();
            MDB_DB_UNLOCK_SQL_API();
            ret = -1;
            goto end;
        }

        /* AS_DBMS_CS_DISCONNECT has no parameters */

        stmt = gClients[index].sql->stmts;
        for (i = 0; i < gClients[index].sql->stmt_num; i++)
        {
            if (stmt->status == iSQL_STAT_IN_USE ||
                    stmt->status == iSQL_STAT_USE_RESULT ||
                    stmt->status == iSQL_STAT_PREPARED ||
                    stmt->status == iSQL_STAT_EXECUTED)
                SQL_cleanup(&gClients[index].DBHandle, stmt, MODE_AUTO, 1);

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

            stmt = stmt->next;
        }
        release_all_statements(gClients[index].sql);

        if (gClients[index].DBHandle >= 0)
            dbi_Disconnect_Server(gClients[index].DBHandle);

        RemoveClient();

        {
            void free_gbuffer(void);
            void *current_thr(void);

            if (current_thr() != NULL)
            {
                free_gbuffer();
            }
        }

        POP_G_CONNID_SQL_API();
        MDB_DB_UNLOCK_SQL_API();
        close_connection(isql);
    }

  end:
    if (ret < 0)
        ret = isql->last_errno;

    if (isql->free_me)
    {
        SMEM_FREE(isql);
    }
    else
        isql->status = iSQL_STAT_NOT_READY;

    return ret;
}

/*****************************************************************************/
//! iSQL_prepare 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql(in)    : iSQL's handle
 * \param query(in) : query
 * \param length(in)    : query's length
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *        mobile용 task lock 추가
 *****************************************************************************/
static iSQL_STMT *
_isql_prepare(iSQL * isql, char *query, unsigned long int length,
        isql_res_type resulttype)
{
    iSQL_STMT *stmt;
    iSQL_FIELD *fields = NULL, *pfields = NULL;
    unsigned int stmt_id, param_count, field_count;
    int ret;
    int _g_connid_bk = -2;

    if (isql->handle < 0)
    {
        if (isql_reconnect(isql))
            return NULL;
    }

    /* error clean */
    isql->last_errno = SQL_E_NOERROR;
    SMEM_FREENUL(isql->last_error);

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    ret = isql_PREPARE_real(isql, query, (int) length, &stmt_id,
            &pfields, &param_count, &fields, &field_count);

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (ret < 0)
    {
        if (pfields)
            SMEM_FREE(pfields);

        if (fields)
            SMEM_FREE(fields);

        close_connection(isql);

        return NULL;
    }
    else if (ret == 0)
    {
        if (pfields)
            SMEM_FREE(pfields);

        if (fields)
            SMEM_FREE(fields);

        return NULL;
    }

    stmt = SMEM_ALLOC(sizeof(iSQL_STMT));

    if (!stmt)
    {
        if (pfields)
            SMEM_FREE(pfields);

        if (fields)
            SMEM_FREE(fields);

        isql->last_errno = SQL_E_OUTOFMEMORY;
        SMEM_FREENUL(isql->last_error);

        return NULL;
    }

    stmt->stmt_id = stmt_id;
    stmt->querytype = isql->last_querytype;
    stmt->isql = isql;
    stmt->field_count = field_count;
    stmt->param_count = param_count;
    stmt->res = NULL;
    stmt->pbind = NULL;
    stmt->rbind = NULL;
    stmt->fields = fields;
    stmt->pfields = pfields;
    stmt->query_timeout = 0;

    stmt->bind_space = NULL;
    stmt->indicator_space = NULL;
    stmt->length_space = NULL;
    stmt->resulttype = resulttype;
    stmt->is_set_bind_param = 0;

    return stmt;
}

__DECL_PREFIX iSQL_STMT *
iSQL_prepare(iSQL * isql, char *query, unsigned long int length)
{
    iSQL_STMT *stmt;

    stmt = _isql_prepare(isql, query, length, SQL_RES_BINARY);

    return stmt;
}

__DECL_PREFIX iSQL_STMT *
iSQL_prepare2(iSQL * isql, char *query, unsigned long int length,
        isql_res_type resulttype)
{
    iSQL_STMT *stmt;

    stmt = _isql_prepare(isql, query, length, resulttype);

    return stmt;
}

/*****************************************************************************/
//! iSQL_bind_param 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param stmt :
 * \param bind : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX int
iSQL_bind_param(iSQL_STMT * stmt, iSQL_BIND * bind)
{
    if (!stmt)
        return -1;

    if (!stmt->param_count)
    {
        stmt->isql->last_errno = SQL_E_NOPARAMETERS;
        SMEM_FREENUL(stmt->isql->last_error);
        return -1;
    }

    if (stmt->pbind == NULL)
    {
        stmt->pbind = SMEM_ALLOC(sizeof(iSQL_BIND) * stmt->param_count);
        if (stmt->pbind == NULL)
        {
            stmt->isql->last_errno = SQL_E_OUTOFMEMORY;
            SMEM_FREENUL(stmt->isql->last_error);
            return -1;
        }
    }

    sc_memcpy(stmt->pbind, bind, sizeof(iSQL_BIND) * stmt->param_count);

    stmt->is_set_bind_param = 1;

    return 0;
}

/*****************************************************************************/
//! iSQL_param_count 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param stmt :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX unsigned int
iSQL_param_count(iSQL_STMT * stmt)
{
    if (!stmt)
        return 0;

    return stmt->param_count;
}

/*****************************************************************************/
//! iSQL_describe_param 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param stmt :
 * \param bind : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX int
iSQL_describe_param(iSQL_STMT * stmt, iSQL_FIELD ** fields)
{
    if (!stmt)
        return SQL_E_NOTPREPARED;

    *fields = stmt->pfields;
    return 0;
}

/*****************************************************************************/
//! iSQL_prepare_result 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param stmt :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX iSQL_RES *
iSQL_prepare_result(iSQL_STMT * stmt)
{
    iSQL_RES *res;

    if (!stmt)
        return NULL;

    if (!stmt->field_count || !stmt->fields)
        return NULL;

    if (stmt->res)
        return stmt->res;

    res = SMEM_ALLOC(sizeof(iSQL_RES));
    if (!res)
    {
        stmt->isql->last_errno = SQL_E_OUTOFMEMORY;
        SMEM_FREENUL(stmt->isql->last_error);
        return NULL;
    }

    stmt->res = res;

    sc_memset(res, 0, sizeof(iSQL_RES));

    res->dont_touch = 0xBA | 1;
    res->isql = stmt->isql;
    res->field_count = stmt->field_count;
    res->fields = stmt->fields;

    return res;
}

/*****************************************************************************/
//! _isql_describe
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param stmt(in/out) : iSQL_STMT structure
 * \param stmt_pfields(in) : input bind field info
 * \param stmt_fields(in) : result bind field info
 * \param bind_param(in/out) : input bind area
 * \param bind_res(in/out) : result bind area
 ************************************
 * \return  ret < 0 is FAIL, else SUCCESS
 ************************************
 * \note
 *  - byte/varbyte type 지원
 *****************************************************************************/
static int
_isql_describe(iSQL_STMT * stmt,
        iSQL_FIELD * stmt_pfields, iSQL_FIELD * stmt_fields,
        iSQL_BIND ** bind_param, iSQL_BIND ** bind_res)
{
    iSQL *isql = stmt->isql;
    iSQL_FIELD *fields_param, *fields_res;
    char *buf_space;
    char *p;
    unsigned long int *lengthp;
    int *indp;
    int buf_size = 0, indicator_size;   //, length_size;
    int i, j;

    if (!bind_param || !bind_res)
        return SQL_E_NOTPREPARED;


    if (isql->handle < 0)
    {
        isql->last_errno = SQL_E_NOTCONNECTED;
        SMEM_FREENUL(isql->last_error);
        return -1;
    }

    /* field info for parameter binding */
    fields_param = stmt_pfields;
    fields_res = stmt_fields;

    /* alloc buffer */
    /* iSQL_bind_param() */
    if (!stmt->pbind)
    {
        if (stmt->param_count > 0)
        {
            stmt->pbind = SMEM_ALLOC(sizeof(iSQL_BIND) * stmt->param_count);
            if (stmt->pbind == NULL)
            {
                stmt->isql->last_errno = SQL_E_OUTOFMEMORY;
                SMEM_FREENUL(stmt->isql->last_error);
                return -1;
            }
        }
        else if (stmt->param_count == 0)
            stmt->pbind = NULL;
        else
            return -1;
    }
    *bind_param = stmt->pbind;

    /* iSQL_bind_result() */
    if (!stmt->rbind)
    {
        if (stmt->field_count > 0)
        {
            stmt->rbind = SMEM_ALLOC(sizeof(iSQL_BIND) * stmt->field_count);
            if (stmt->rbind == NULL)
            {
                isql->last_errno = SQL_E_OUTOFMEMORY;
                SMEM_FREENUL(isql->last_error);
                return -1;
            }
        }
        else if (stmt->field_count == 0)
            stmt->rbind = NULL;
        else
            return -1;
    }
    *bind_res = stmt->rbind;

    /* calculate the buffer space */
    /* 1. paramter */
    for (i = 0; i < (int) stmt->param_count; ++i)
    {
        buf_size +=
                DB_BYTE_LENG(fields_param[i].type,
                fields_param[i].buffer_length + 1);

        if (IS_BS_OR_NBS(fields_param[i].type))
            buf_size += DB_BYTE_LENG(fields_param[i].type,
                    (fields_param[i].buffer_length + 2));
        buf_size = _alignedlen(buf_size, sizeof(double));
    }

    /* 2. result  */
    for (i = 0; i < (int) stmt->field_count; i++)
    {
        buf_size +=
                DB_BYTE_LENG(fields_res[i].type,
                fields_res[i].buffer_length + 1);
        buf_size = _alignedlen(buf_size, sizeof(double));
    }

    indicator_size =    /* length_size = */
            (stmt->param_count + stmt->field_count) * sizeof(int);

    if (indicator_size == 0)
        return 0;

    buf_space = SMEM_XCALLOC(buf_size);
    if (buf_space == NULL)
    {
        isql->last_errno = SQL_E_OUTOFMEMORY;
        SMEM_FREENUL(isql->last_error);
        return SQL_E_OUTOFMEMORY;
    }
    stmt->bind_space = buf_space;

    indp = SMEM_XCALLOC(indicator_size * sizeof(int));
    if (indp == NULL)
    {
        isql->last_errno = SQL_E_OUTOFMEMORY;
        SMEM_FREENUL(isql->last_error);
        SMEM_FREENUL(stmt->bind_space);
        return SQL_E_OUTOFMEMORY;
    }
    stmt->indicator_space = indp;

    lengthp = SMEM_XCALLOC(indicator_size * sizeof(unsigned long int));
    if (lengthp == NULL)
    {
        isql->last_errno = SQL_E_OUTOFMEMORY;
        SMEM_FREENUL(isql->last_error);
        SMEM_FREENUL(stmt->bind_space);
        SMEM_FREENUL(stmt->indicator_space);
        return SQL_E_OUTOFMEMORY;
    }
    stmt->length_space = lengthp;

    p = buf_space;

    /* assign the buffer space */
    /* 1. paramter */
    for (i = 0, j = 0; i < (int) stmt->param_count; ++i, ++j)
    {
        (*bind_param)[i].length = &lengthp[j];
        (*bind_param)[i].is_null = &indp[j];
        (*bind_param)[i].buffer_type = fields_param[i].type;
        (*bind_param)[i].buffer_length = fields_param[i].buffer_length;
        (*bind_param)[i].buffer = p;
        if (IS_BS_OR_NBS(fields_param[i].type))
            (*bind_param)[i].buffer_length +=
                    (fields_param[i].buffer_length + 1 + 2);

        p += _alignedlen(DB_BYTE_LENG
                (fields_param[i].type,
                        (*bind_param)[i].buffer_length), sizeof(double));
    }

    /* 2. result */
    for (i = 0; i < (int) stmt->field_count; i++, ++j)
    {
        (*bind_res)[i].length = &lengthp[j];
        (*bind_res)[i].is_null = &indp[j];
        (*bind_res)[i].buffer_type = fields_res[i].type;
        (*bind_res)[i].buffer_length = fields_res[i].buffer_length;
        if (IS_BS_OR_NBS(fields_res[i].type))
            (*bind_res)[i].buffer_length += 1;
        (*bind_res)[i].buffer = p;
        p += _alignedlen(DB_BYTE_LENG
                (fields_res[i].type, fields_res[i].buffer_length + 1),
                sizeof(double));
    }

    if (stmt->param_count > 0)
    {
        stmt->is_set_bind_param = 1;
    }

    return 0;
}

#define FIXED_DATA_STR_LEN 32

static int
_isql_convert_to_varchar_fields_info(iSQL_FIELD * fields_info, int field_count)
{
    int i;

    for (i = 0; i < field_count; i++)
    {
        switch (fields_info[i].type)
        {
        case SQL_DATA_NONE:
            break;
        case SQL_DATA_TINYINT:
        case SQL_DATA_SMALLINT:
        case SQL_DATA_INT:
        case SQL_DATA_BIGINT:
        case SQL_DATA_OID:
        case SQL_DATA_FLOAT:
        case SQL_DATA_DOUBLE:
        case SQL_DATA_DECIMAL:
        case SQL_DATA_TIMESTAMP:
        case SQL_DATA_DATE:
        case SQL_DATA_TIME:
        case SQL_DATA_DATETIME:
            fields_info[i].type = SQL_DATA_VARCHAR;
            fields_info[i].length = FIXED_DATA_STR_LEN;
            fields_info[i].max_length = FIXED_DATA_STR_LEN;
            fields_info[i].buffer_length = FIXED_DATA_STR_LEN;
            break;
        case SQL_DATA_CHAR:
        case SQL_DATA_VARCHAR:
        case SQL_DATA_NCHAR:
        case SQL_DATA_NVARCHAR:
        case SQL_DATA_BYTE:
        case SQL_DATA_VARBYTE:
            break;
        default:
            return -1;
        }
    }
    return 0;
}

__DECL_PREFIX int
iSQL_describe2(iSQL_STMT * stmt, iSQL_BIND ** bind_param,
        iSQL_BIND ** bind_res)
{
    iSQL_FIELD *fields_res = NULL;
    int res = 0;

    if (!stmt)
        return SQL_E_NOTPREPARED;

    if (stmt->field_count > 0)
    {
        fields_res = SMEM_ALLOC(sizeof(iSQL_FIELD) * stmt->field_count);

        if (fields_res == NULL)
        {
            stmt->isql->last_errno = SQL_E_OUTOFMEMORY;
            SMEM_FREENUL(stmt->isql->last_error);
            return -1;
        }
        sc_memcpy(fields_res, stmt->fields,
                sizeof(iSQL_FIELD) * stmt->field_count);
        res = _isql_convert_to_varchar_fields_info(fields_res,
                stmt->field_count);
    }

    if (res == 0)
    {
        res = _isql_describe(stmt, stmt->pfields, fields_res, bind_param,
                bind_res);
    }

    SMEM_FREE(fields_res);

    return res;
}

/*****************************************************************************/
//! iSQL_describe
/*! \breif  get the stmt's paramter field count
 ************************************
 * \param stmt(in)          : iSQL_STMT structure
 * \param bind_param(in/out): input binding area
 * \param bind_res(in/out)  : result binding area
 ************************************
 * \return  stmt's paramter count
 ************************************
 * \note
 *****************************************************************************/
__DECL_PREFIX int
iSQL_describe(iSQL_STMT * stmt, iSQL_BIND ** bind_param, iSQL_BIND ** bind_res)
{
    if (!stmt)
        return SQL_E_NOTPREPARED;

    if (stmt->resulttype == SQL_RES_STRING)
        return iSQL_describe2(stmt, bind_param, bind_res);
    else
        return _isql_describe(stmt, stmt->pfields, stmt->fields,
                bind_param, bind_res);
}

/*****************************************************************************/
//! iSQL_num_parameter_fields 
/*! \breif  get the stmt's paramter field count
 ************************************
 * \param stmt(in) : iSQL_STMT structure
 ************************************
 * \return  stmt's paramter count
 ************************************
 * \note
 *****************************************************************************/
__DECL_PREFIX int
iSQL_num_parameter_fields(iSQL_STMT * stmt)
{
    if (!stmt)
        return SQL_E_NOTPREPARED;

    return stmt->param_count;
}

/*****************************************************************************/
//! iSQL_num_result_fields 
/*! \breif  get the stmt's result field count
 ************************************
 * \param stmt(in) : iSQL_STMT structure
 ************************************
 * \return  stmt's result field count
 ************************************
 * \note
 *****************************************************************************/
__DECL_PREFIX int
iSQL_num_result_fields(iSQL_STMT * stmt)
{
    if (!stmt)
        return SQL_E_NOTPREPARED;

    return stmt->field_count;
}

/*****************************************************************************/
//! iSQL_execute 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param stmt(in)    : iSQL_STMT structure
 ************************************
 * \return  ret < 0 is FAIL, else is SUCCESS
 ************************************
 * \note 
 *  - mobile용 task lock 추가
 *  - DDL인 경우 DB LOCK 처리
 *  - DDL인 경우 DB UNLOCK 처리(TRUNCATE 추가) 
 *****************************************************************************/
__DECL_PREFIX int
iSQL_execute(iSQL_STMT * stmt)
{
    iSQL *isql = NULL;
    iSQL_RES *res;
    MDB_INT32 ret;
    int _g_connid_bk = -2;

    if (!stmt)
        return SQL_E_NOTPREPARED;

    isql = stmt->isql;

    if (isql->handle < 0)
    {       // check the handle
        isql->last_errno = SQL_E_NOTCONNECTED;
        SMEM_FREENUL(isql->last_error);
        return -1;
    }

    if (stmt->param_count > 0 && !stmt->is_set_bind_param)
    {
        isql->last_errno = SQL_E_CANNOTBINDING;
        SMEM_FREENUL(isql->last_error);
        return -1;
    }

    if (iSQL_stmt_querytype(stmt) == SQL_STMT_SELECT
            || iSQL_stmt_querytype(stmt) == SQL_STMT_DESCRIBE)
    {

        if (!stmt->res)
        {
            stmt->res = SMEM_ALLOC(sizeof(iSQL_RES));
            if (!stmt->res)
            {
                isql->last_errno = SQL_E_OUTOFMEMORY;
                SMEM_FREENUL(isql->last_error);
                return -1;
            }
            sc_memset(stmt->res, 0, sizeof(iSQL_RES));
            stmt->res->dont_touch = 0xBA | 1;
        }
        else
        {
            if (stmt->res->data)
            {
                isql_free_result_data(stmt->res);
            }
        }

        res = stmt->res;

        MDB_DB_LOCK_SQL_API();
        PUSH_G_CONNID_SQL_API(isql->handle);

        ret = isql_EXECUTE_real(stmt);

        POP_G_CONNID_SQL_API();
        MDB_DB_UNLOCK_SQL_API();

        if (ret < 0)
        {
            close_connection(isql);
            ret = -1;
        }
        else if (ret == 0)
            ret = -1;
        else
        {
            ret = 0;

            res->isql = isql;
            res->field_count = stmt->field_count;
            res->fields = stmt->fields;
        }
    }
    else
    {
        MDB_DB_LOCK_SQL_API();
        PUSH_G_CONNID_SQL_API(isql->handle);

        ret = isql_EXECUTE_real(stmt);

        POP_G_CONNID_SQL_API();
        MDB_DB_UNLOCK_SQL_API();

        if (ret < 0)
        {
            close_connection(isql);
            ret = -1;
        }
        else if (ret == 0)
            ret = -1;
        else
            ret = 0;
    }

    return ret;
}

__DECL_PREFIX int
iSQL_stmt_set_timeout(iSQL_STMT * stmt, int value)
{
    if (!stmt)
        return SQL_E_NOTPREPARED;

    if (value >= 0)
    {
        stmt->query_timeout = value;
    }

    return 0;
}

/*****************************************************************************/
//! iSQL_stmt_store_result 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param stmt(in)    : iSQL_SMTM structure
 ************************************
 * \return  ret < 0 is FAIL, else is SUCCESS
 ************************************
 * \note
 *    - mobile용 task lock 추가
 *****************************************************************************/
__DECL_PREFIX int
iSQL_stmt_store_result(iSQL_STMT * stmt)
{
    iSQL *isql = stmt->isql;
    iSQL_RES *res = stmt->res;
    int ret;
    int _g_connid_bk = -2;

    if (!stmt)
        return SQL_E_NOTPREPARED;

    if (isql->handle < 0)
    {       // check the handle
        isql->last_errno = SQL_E_NOTCONNECTED;
        SMEM_FREENUL(isql->last_error);
        return -1;
    }

    if (stmt->querytype != SQL_STMT_SELECT
            && stmt->querytype != SQL_STMT_DESCRIBE)
    {
        return 0;
    }

    if (!res)
    {
        SMEM_FREENUL(isql->last_error);
        isql->last_error = mdb_strdup("Not executed");
        isql->last_errno = SQL_E_ERROR;
        return -1;
    }

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    ret = isql_STMTSTORERESULT_real(stmt, 0);

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (ret <= 0)
        return -1;

    return 0;
}

__DECL_PREFIX int
iSQL_stmt_use_result(iSQL_STMT * stmt)
{
    iSQL *isql = stmt->isql;
    iSQL_RES *res = stmt->res;
    int ret;

    int _g_connid_bk = -2;

    if (!stmt)
        return SQL_E_NOTPREPARED;

    if (isql->handle < 0)
    {       // check the handle
        isql->last_errno = SQL_E_NOTCONNECTED;
        SMEM_FREENUL(isql->last_error);
        return -1;
    }

    if (stmt->querytype != SQL_STMT_SELECT
            && stmt->querytype != SQL_STMT_DESCRIBE)
    {
        return 0;
    }

    if (!res)
    {
        SMEM_FREENUL(isql->last_error);
        isql->last_error = mdb_strdup("Not executed");
        isql->last_errno = SQL_E_ERROR;
        return -1;
    }

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    ret = isql_STMTSTORERESULT_real(stmt, 1);

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (ret <= 0)
        return -1;

    return 0;
}

/*****************************************************************************/
//! iSQL_bind_result 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param stmt(in)    : iSQL_STMT structure
 * \param bind(out) : stmt's result bind area
 ************************************
 * \return  ret < 0 is FAIL, else is SUCCESS
 ************************************
 * \note
 *    - mobile용 task lock 추가
 *****************************************************************************/
__DECL_PREFIX int
iSQL_bind_result(iSQL_STMT * stmt, iSQL_BIND * bind)
{
    iSQL *isql = stmt->isql;

    if (!stmt)
        return SQL_E_NOTPREPARED;

    if (!stmt->field_count)
        return 0;

    if (stmt->rbind == NULL)
    {
        stmt->rbind = SMEM_ALLOC(sizeof(iSQL_BIND) * stmt->field_count);
        if (stmt->rbind == NULL)
        {
            isql->last_errno = SQL_E_OUTOFMEMORY;
            SMEM_FREENUL(isql->last_error);
            return -1;
        }
    }

    sc_memcpy(stmt->rbind, bind, sizeof(iSQL_BIND) * stmt->field_count);

    return 0;
}

/*****************************************************************************/
//! iSQL_fetch 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param stmt(in)    : iSQL_SMTM structure
 ************************************
 * \return  ret < 0 is FAIL, else is SUCCESS
 ************************************
 * \note
 *    - mobile용 task lock 추가
 *****************************************************************************/
__DECL_PREFIX int
iSQL_fetch(iSQL_STMT * stmt)
{
    iSQL_RES *res = stmt->res;
    char *row;
    int ret;
    int _g_connid_bk = -2;

    if (!stmt)
    {
        return -1;
    }
    if (!stmt->rbind)
    {
        stmt->isql->last_errno = SQL_E_NOTBINDRESULT;
        SMEM_FREENUL(stmt->isql->last_error);
        return -1;
    }
    if (!res)
        return -1;

    if (res->data)
    {
        if (!res->data_cursor)
        {
            if (res->is_partial_result && res->row_count == res->data_offset
                    && !res->data->use_eof)
            {
                MDB_DB_LOCK_SQL_API();
                PUSH_G_CONNID_SQL_API(stmt->isql->handle);

                ret = isql_FETCH_real(stmt);

                POP_G_CONNID_SQL_API();
                MDB_DB_UNLOCK_SQL_API();

                if (ret < 0)
                    return -1;
            }
            if (!res->data_cursor)
            {
                res->eof = 1;
                return iSQL_NO_DATA;
            }
        }
        row = (char *) res->data_cursor->data;
        res->current_rid = res->data_cursor->rid;
        res->data_cursor = res->data_cursor->next;
        res->data_offset++;
        res->current_row = (iSQL_ROW) row;
        ret = stmt_fetch_row(stmt, row);
    }
    else
    {
        stmt->isql->last_errno = SQL_E_RESULTAPINOTCALLED;
        SMEM_FREENUL(stmt->isql->last_error);
        return -1;
    }

    return ret;

}

/*****************************************************************************/
//! iSQL_stmt_querytype 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param stmt :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX isql_stmt_type
iSQL_stmt_querytype(iSQL_STMT * stmt)
{
    return stmt->querytype;
}

/*****************************************************************************/
//! iSQL_stmt_affected_rows 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param stmt :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX unsigned int
iSQL_stmt_affected_rows(iSQL_STMT * stmt)
{
    if (!stmt)
        return 0;
    if (stmt->res && stmt->res->is_partial_result == 1)
        return -1;
    return stmt->affected_rows;
}

__DECL_PREFIX OID               /* _64BIT */
iSQL_stmt_affected_rid(iSQL_STMT * stmt)
{
    if (!stmt)
        return 0;

    return stmt->affected_rid;
}

/*****************************************************************************/
//! iSQL_stmt_close 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param stmt(in)    : iSQL_STMT structure
 ************************************
 * \return  ret < 0 is FAIL, else is SUCCESS
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX int
iSQL_stmt_close(iSQL_STMT * stmt)
{
    int ret;
    int _g_connid_bk = -2;

    if (!stmt)
        return -1;

    if (iSQL_stmt_querytype(stmt) == SQL_STMT_SELECT
            || iSQL_stmt_querytype(stmt) == SQL_STMT_DESCRIBE)
    {
        if (stmt->res)
        {
            if (stmt->res->data)
            {
                isql_free_result_data(stmt->res);
            }

            SMEM_FREE(stmt->res);
        }

        SMEM_FREE(stmt->fields);
    }

    if (stmt->pfields)
        SMEM_FREE(stmt->pfields);
    if (stmt->pbind)
        SMEM_FREE(stmt->pbind);
    if (stmt->rbind)
        SMEM_FREE(stmt->rbind);

    if (stmt->bind_space)
        SMEM_FREENUL(stmt->bind_space);
    if (stmt->indicator_space)
        SMEM_FREENUL(stmt->indicator_space);
    if (stmt->length_space)
        SMEM_FREENUL(stmt->length_space);

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(stmt->isql->handle);

    ret = isql_CLOSESTATEMENT_real(stmt);

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (ret < 0)
    {
        close_connection(stmt->isql);
    }

    SMEM_FREE(stmt);

    return 0;
}

/*****************************************************************************/
//! iSQL_stmt_errno 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param stmt :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX int
iSQL_stmt_errno(iSQL_STMT * stmt)
{
    return iSQL_errno(stmt->isql);
}

/*****************************************************************************/
//! iSQL_stmt_error 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param stmt :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX char *
iSQL_stmt_error(iSQL_STMT * stmt)
{
    return iSQL_error(stmt->isql);
}

/*****************************************************************************/
//! iSQL_query 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql(in) : iSQL structure
 * \param query(in)    : query string
 ************************************
 * \return  ret < 0 is Error, else is SUCCESS
 ************************************
 * \note
 *  - mobile용 task lock 추가
 *  - DDL인 경우 DB LOCK 처리
 *  - DDL인 경우 DB UNLOCK 처리(TRUNCATE 추가) 
 *****************************************************************************/
__DECL_PREFIX int
iSQL_query(iSQL * isql, char *query)
{
    int ret;

    int _g_connid_bk = -2;

    if (isql->handle < 0)
    {       // check the handle
        if (isql_reconnect(isql))
            return -1;
    }

    /* error clean */
    isql->last_errno = SQL_E_NOERROR;
    SMEM_FREENUL(isql->last_error);

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    ret = isql_QUERY_real(isql, query, sc_strlen(query));

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (ret < 0)
    {
        ret = -1;
    }
    else if (ret == 0)
        ret = -1;
    else
        ret = 0;

    if (isql->last_querytype >= SQL_STMT_CREATE &&
            isql->last_querytype <= SQL_STMT_ROLLBACK)
    {
        MDB_DB_LOCK_SQL_API();
        PUSH_G_CONNID_SQL_API(isql->handle);

        if (isql->last_errno == SQL_E_NOERROR)
            iSQL_commit(isql);
        else
            iSQL_rollback(isql);

        POP_G_CONNID_SQL_API();
        MDB_DB_UNLOCK_SQL_API();
    }

    return ret;
}

__DECL_PREFIX int
iSQL_query3(iSQL * isql, char *query, int query_len)
{
    int ret;

    int _g_connid_bk = -2;

    if (isql->handle < 0)
    {       // check the handle
        if (isql_reconnect(isql))
            return -1;
    }

    /* error clean */
    isql->last_errno = SQL_E_NOERROR;
    SMEM_FREENUL(isql->last_error);

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    ret = isql_QUERY_real(isql, query, sc_strlen(query));

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (ret < 0)
    {
        close_connection(isql);
        return -1;
    }
    else if (ret == 0)
        return -1;

    return 0;
}

__DECL_PREFIX int
iSQL_query2(iSQL * isql, char *query)
{
    int ret;

    int _g_connid_bk = -2;

    /* error clean */
    isql->last_errno = SQL_E_NOERROR;
    SMEM_FREENUL(isql->last_error);

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    ret = isql_QUERY_real(isql, query, sc_strlen(query));

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    return ret;
}

/*****************************************************************************/
//! iSQL_use_result 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX iSQL_RES *
iSQL_use_result(iSQL * isql)
{
    iSQL_RES *res;

    int _g_connid_bk = -2;
    int ret;

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    ret = isql_GETRESULT_real(isql, 0);
    if (ret < 0)
    {
        isql_FLUSHRESULTSET_real(isql);

        POP_G_CONNID_SQL_API();
        MDB_DB_UNLOCK_SQL_API();

        if (isql->fields)
        {
            SMEM_FREE(isql->fields);
            isql->fields = NULL;
        }

        close_connection(isql);
        return NULL;
    }
    else if (ret == 0)
    {
        if (isql->fields)
        {
            SMEM_FREE(isql->fields);
            isql->fields = NULL;
        }

        goto end;
    }

    if (!isql->fields)
    {
        isql->last_errno = SQL_E_OUTOFMEMORY;
        SMEM_FREENUL(isql->last_error);
    }

    res = SMEM_ALLOC(sizeof(iSQL_RES));
    if (!res)
    {
        isql_FLUSHRESULTSET_real(isql);
        if (isql->fields)
        {
            SMEM_FREE(isql->fields);
            isql->fields = NULL;
        }
        isql->last_errno = SQL_E_OUTOFMEMORY;
        SMEM_FREENUL(isql->last_error);

        goto end;
    }

    sc_memset(res, 0, sizeof(iSQL_RES));

    res->lengths = SMEM_ALLOC(sizeof(iSQL_LENGTH) * isql->field_count);
    if (!res->lengths)
    {
        isql_FLUSHRESULTSET_real(isql);
        SMEM_FREE(res);
        if (isql->fields)
        {
            SMEM_FREE(isql->fields);
            isql->fields = NULL;
        }
        isql->last_errno = SQL_E_OUTOFMEMORY;
        SMEM_FREENUL(isql->last_error);

        goto end;
    }

    res->row = SMEM_ALLOC(sizeof(res->row[0]) * (isql->field_count + 1));
    if (!res->row)
    {
        isql_FLUSHRESULTSET_real(isql);
        SMEM_FREE(res->lengths);
        SMEM_FREE(res);
        if (isql->fields)
        {
            SMEM_FREE(isql->fields);
            isql->fields = NULL;
        }
        isql->last_errno = SQL_E_OUTOFMEMORY;
        SMEM_FREENUL(isql->last_error);

        goto end;
    }

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    res->dont_touch = 0xBA;
    res->isql = isql;
    res->row_count = isql->data->row_num;
    res->field_count = isql->field_count;
    res->current_field = 0;
    res->fields = isql->fields;
    res->data = isql->data;
    res->data_cursor = res->data->data;
    res->is_partial_result = 1;
    res->data_offset = 0;
    res->current_row = NULL;
    res->eof = 0;

    isql->affected_rows = -1;
    isql->affected_rid = 0;
    isql->fields = NULL;

    return res;

  end:
    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    return NULL;
}

/*****************************************************************************/
//! iSQL_store_result 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql(in) : iSQL's structure
 ************************************
 * \return  ret < 0 is ERROR, else is SUCCESS
 ************************************
 * \note
 *    - mobile용 task lock 추가
 *****************************************************************************/
__DECL_PREFIX iSQL_RES *
iSQL_store_result(iSQL * isql)
{
    iSQL_RES *res;
    int ret;

    int _g_connid_bk = -2;

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    ret = isql_GETRESULT_real(isql, 1);
    if (ret < 0)
    {
        isql_FLUSHRESULTSET_real(isql);
        if (isql->fields)
        {
            SMEM_FREE(isql->fields);
            isql->fields = NULL;
        }
        POP_G_CONNID_SQL_API();
        MDB_DB_UNLOCK_SQL_API();

        close_connection(isql);
        return NULL;
    }
    else if (ret == 0)
    {
        if (isql->fields)
        {
            SMEM_FREE(isql->fields);
            isql->fields = NULL;
        }

        goto end;
    }

    if (!isql->fields)
    {
        isql->last_errno = SQL_E_OUTOFMEMORY;
        SMEM_FREENUL(isql->last_error);
    }

    res = SMEM_ALLOC(sizeof(iSQL_RES));

    if (!res)
    {
        isql_FLUSHRESULTSET_real(isql);
        if (isql->fields)
        {
            SMEM_FREE(isql->fields);
            isql->fields = NULL;
        }
        isql->last_errno = SQL_E_OUTOFMEMORY;
        SMEM_FREENUL(isql->last_error);

        goto end;
    }

    sc_memset(res, 0, sizeof(iSQL_RES));

    res->lengths = SMEM_ALLOC(sizeof(iSQL_LENGTH) * isql->field_count);
    if (!res->lengths)
    {
        isql_FLUSHRESULTSET_real(isql);
        SMEM_FREE(res);
        if (isql->fields)
        {
            SMEM_FREE(isql->fields);
            isql->fields = NULL;
        }
        isql->last_errno = SQL_E_OUTOFMEMORY;
        SMEM_FREENUL(isql->last_error);

        goto end;
    }

    res->row = SMEM_ALLOC(sizeof(res->row[0]) * (isql->field_count + 1));
    if (!res->row)
    {
        isql_FLUSHRESULTSET_real(isql);
        SMEM_FREE(res->lengths);
        SMEM_FREE(res);
        if (isql->fields)
        {
            SMEM_FREE(isql->fields);
            isql->fields = NULL;
        }
        isql->last_errno = SQL_E_OUTOFMEMORY;
        SMEM_FREENUL(isql->last_error);

        goto end;
    }

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    sc_memset(res->row, 0, sizeof(res->row[0]) * (isql->field_count + 1));

    res->dont_touch = 0xBA;
    res->isql = isql;
    res->row_count = isql->affected_rows;
    res->field_count = isql->field_count;
    res->current_field = 0;
    res->fields = isql->fields;
    res->data = isql->data;
    res->data_cursor = res->data->data;
    res->data_offset = 0;
    res->current_row = NULL;
    res->eof = 0;

    isql->field_count = 0;
    isql->fields = NULL;
    isql->data = NULL;

    return res;

  end:
    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    return NULL;
}

/*****************************************************************************/
//! iSQL_list_fields 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql(in)        :
 * \param table(in)        : 
 * \param column(in)    :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *    - mobile용 task lock 추가
 *****************************************************************************/
__DECL_PREFIX iSQL_RES *
iSQL_list_fields(iSQL * isql, char *table, char *column)
{
    iSQL_RES *res;
    iSQL_FIELD *fields = NULL;
    int ret;

    int _g_connid_bk = -2;

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    ret = isql_GETLISTFIELDS_real(isql, table, column, &fields);

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (ret < 0)
    {
        if (fields)
            SMEM_FREE(fields);
        close_connection(isql);
        return NULL;
    }
    else if (ret == 0)
    {
        if (fields)
            SMEM_FREE(fields);
        return NULL;
    }

    res = SMEM_ALLOC(sizeof(iSQL_RES));
    if (!res)
    {
        if (fields)
            SMEM_FREE(fields);
        isql->last_errno = SQL_E_OUTOFMEMORY;
        SMEM_FREENUL(isql->last_error);
        return NULL;
    }

    res->dont_touch = 0xBA;
    res->row_count = 0;
    res->row = NULL;
    res->lengths = NULL;
    res->field_count = ret;
    res->current_field = 0;
    res->fields = fields;
    res->data = NULL;
    res->data_cursor = NULL;
    res->data_offset = 0;
    res->current_row = NULL;
    res->isql = isql;
    res->eof = 0;

    isql->field_count = ret;

    return res;
}

/*****************************************************************************/
//! iSQL_fetch_row 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *    - mobile용 task lock 추가
 *****************************************************************************/
__DECL_PREFIX iSQL_ROW
iSQL_fetch_row(iSQL_RES * res)
{
    int ret;

    if (!res)
        return NULL;    /* position moved */

    if (res->dont_touch & 0x01)
    {
        SMEM_FREENUL(res->isql->last_error);
        res->isql->last_errno = SQL_E_INVALIDFETCHCALL;
        return NULL;
    }
    if (res->lengths == NULL || res->eof)
        return NULL;

    ret = read_one_row(res, res->row, res->lengths, res->field_count);
    if (ret == 0)
        res->eof = 1;
    else if (ret == 1)
    {
        if (!res->data)
            res->row_count++;
        else
            res->data_offset++;
        res->current_row = res->row;
        return res->current_row;
    }

    return NULL;
}

/*****************************************************************************/
//! iSQL_fetch_lengths 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX iSQL_LENGTH
iSQL_fetch_lengths(iSQL_RES * res)
{
    MDB_UINT32 i;

    if (res == NULL)
        return NULL;

    if (!res->lengths || !res->current_row)
        return NULL;

    if (res->data)
    {       // if buffered
        for (i = 0; i < res->field_count; i++)
        {
            if (IS_NBS(res->fields[i].type))
                res->lengths[i] = sc_wcslen((DB_WCHAR *) res->current_row[i]);
            else
                res->lengths[i] = sc_strlen(res->current_row[i]);
        }
    }

    return res->lengths;
}

/*****************************************************************************/
//! iSQL_fetch_field 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX iSQL_FIELD *
iSQL_fetch_field(iSQL_RES * res)
{
    if (res == NULL)
        return NULL;

    if (res->current_field >= res->field_count)
        return NULL;
    else
        return &res->fields[res->current_field++];
}

/*****************************************************************************/
//! iSQL_fetch_field_direct 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res :
 * \param fieldnr : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX iSQL_FIELD *
iSQL_fetch_field_direct(iSQL_RES * res, unsigned int fieldnr)
{
    if (res == NULL)
        return NULL;

    if (fieldnr >= res->field_count)
        return NULL;
    else
        return &res->fields[fieldnr];
}

/*****************************************************************************/
//! iSQL_fetch_fields 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX iSQL_FIELD *
iSQL_fetch_fields(iSQL_RES * res)
{
    if (res == NULL)
        return NULL;

    return res->fields;
}

/*****************************************************************************/
//! iSQL_row_tell 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX iSQL_ROW_OFFSET
iSQL_row_tell(iSQL_RES * res)
{
    if (res->is_partial_result)
        return NULL;

    if (res == NULL)
        return NULL;

    res->data_cursor->offset = res->data_offset;

    return res->data_cursor;
}

/*****************************************************************************/
//! iSQL_field_tell 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX iSQL_FIELD_OFFSET
iSQL_field_tell(iSQL_RES * res)
{
    if (res == NULL)
        return 0;

    return res->current_field;
}

/*****************************************************************************/
//! iSQL_get_fieldtype 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res :
 * \param fieldid : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX isql_data_type
iSQL_get_fieldtype(iSQL_RES * res, const unsigned int fieldid)
{
    if (res == NULL)
        return 0;

    if (res->field_count < fieldid)
        return 0;
    else
        return res->fields[fieldid].type;
}

/*****************************************************************************/
//! iSQL_num_rows 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX unsigned int
iSQL_num_rows(iSQL_RES * res)
{
    if (res == NULL)
        return 0;

    return res->row_count;
}

/*****************************************************************************/
//! iSQL_num_fields 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX unsigned int
iSQL_num_fields(iSQL_RES * res)
{
    if (res == NULL)
        return 0;

    return res->field_count;
}

/*****************************************************************************/
//! iSQL_data_seek 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res(in) :
 * \param offset(in) : 
 * \param whence(in) : iSQL_DATA_SEEK_START/iSQL_DATA_SEEK_CURRENT/iSQL_DATA_SEEK_END
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX long int
iSQL_data_seek(iSQL_RES * res, long int offset, int whence)
{
    long int old_offset;
    iSQL_ROWS *tmp = NULL;

    if (res == NULL || res->row_count == 0)
        return -1;

    switch (whence)
    {
    case iSQL_DATA_SEEK_START:
        // offset을 그냥 사용하자.
        break;
    case iSQL_DATA_SEEK_CURRENT:
        offset += res->data_offset;
        break;
    case iSQL_DATA_SEEK_END:
        offset += (res->row_count - 1);
        break;
    default:
        SMEM_FREENUL(res->isql->last_error);
        res->isql->last_errno = SQL_E_ERROR;
        res->isql->last_error = mdb_strdup("Invalid argument");
        return -1;
    }

    if (offset < 0 || (unsigned long) offset >= res->data->row_num)
        return SQL_E_INVALIDSEEK;

    if (offset == res->data_offset)
        return res->data_offset;
    else if (offset < res->data_offset)
    {
        old_offset = res->data_offset;
        res->data_offset = offset;
        for (tmp = res->data->data; offset-- && tmp; tmp = tmp->next);
    }
    else if (offset > res->data_offset)
    {
        old_offset = res->data_offset;
        res->data_offset = offset;
        for (tmp = (res->data_cursor ? res->data_cursor : res->data->data);
                (offset--) - old_offset && tmp; tmp = tmp->next);
    }

    res->current_row = NULL;
    res->data_cursor = tmp;
    if (res->data_cursor)
        res->eof = 0;

    return 0;
}

/*****************************************************************************/
//! iSQL_data_tell
/*! \breif  resultset cursor의 현재 위치를 알려줌
    ************************************
* \param res(in) : result res
************************************
* \return  return_value
************************************
* \note
*  - End of Resultset의 경우 -1을 리턴함
*****************************************************************************/
__DECL_PREFIX long int
iSQL_data_tell(iSQL_RES * res)
{
    if (res->data_offset == res->row_count)
        return -1;
    else
        return res->data_offset;
}

/*****************************************************************************/
//! iSQL_row_seek 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res :
 * \param row : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX iSQL_ROW_OFFSET
iSQL_row_seek(iSQL_RES * res, iSQL_ROW_OFFSET row)
{
    iSQL_ROW_OFFSET retval = res->data_cursor;

    if (res->is_partial_result)
        return NULL;

    if (row && res->eof == 1)
        res->eof = 0;

    res->current_row = NULL;
    res->data_cursor = row;
    res->data_offset = row->offset;

    return retval;
}

/*****************************************************************************/
//! iSQL_field_seek 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res :
 * \param field_offset : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX iSQL_FIELD_OFFSET
iSQL_field_seek(iSQL_RES * res, iSQL_FIELD_OFFSET field_offset)
{
    iSQL_FIELD_OFFSET ret_offset = res->current_field;

    res->current_field = field_offset;

    return ret_offset;
}

/*****************************************************************************/
//! iSQL_eof
/*! \breif  Resultset Cursor가 EOR의 위치인지를 검사
************************************
* \param res :
************************************
* \return  return_value
************************************
* \note 내부 알고리즘\n
*      및 기타 설명
*****************************************************************************/
__DECL_PREFIX int
iSQL_eof(iSQL_RES * res)
{
    if (res->eof)
        return 1;

    if (res->is_partial_result == 1)
        return 0;

    if (res->data_offset == res->row_count)
        return 1;
    else
        return 0;
}

/*****************************************************************************/
//! iSQL_free_result 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res(in) :
 ************************************
 * \return  ret < 0 is Error, else is SUCCESS
 ************************************
 * \note 
 *    - mobile용 task lock 추가
 *****************************************************************************/
__DECL_PREFIX void
iSQL_free_result(iSQL_RES * res)
{
    unsigned int i;
    int ret;

    int _g_connid_bk = -2;

    if (res)
    {
        if (res->dont_touch & 0x01)
            return;
        if ((res->dont_touch & 0xFF) != 0xBA)
            return;
        if (res->data)
        {   // if buffered

            isql_free_result_data(res);

            // already free it..
            res->row[0] = NULL;
        }
        else
        {   // if unbuffered
            if (res->isql)
            {
                MDB_DB_LOCK_SQL_API();
                PUSH_G_CONNID_SQL_API(res->isql->handle);

                ret = isql_FLUSHRESULTSET_real(res->isql);

                POP_G_CONNID_SQL_API();
                MDB_DB_UNLOCK_SQL_API();

                if (ret < 0)
                    close_connection(res->isql);
            }
        }

        if (res->fields)
            SMEM_FREENUL(res->fields);
        if (res->lengths)
            SMEM_FREENUL(res->lengths);
        if (res->row)
        {
            for (i = 0; res->row[i] != NULL; i++)
                SMEM_FREENUL(res->row[i]);
            SMEM_FREENUL(res->row);
        }

        res->dont_touch = 0x0;
        SMEM_FREE(res);
    }
}

/*****************************************************************************/
//! iSQL_last_querytype 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX isql_stmt_type
iSQL_last_querytype(iSQL * isql)
{
    return isql->last_querytype;
}

/*****************************************************************************/
//! iSQL_field_count 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX unsigned int
iSQL_field_count(iSQL * isql)
{
    return isql->field_count;
}

/*****************************************************************************/
//! iSQL_affected_rows 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX unsigned int
iSQL_affected_rows(iSQL * isql)
{
    return isql->affected_rows;
}

__DECL_PREFIX OID               /* _64BIT */
iSQL_affected_rid(iSQL * isql)
{
    return isql->affected_rid;
}

__DECL_PREFIX int
iSQL_begin_transaction(iSQL * isql)
{
    MDB_INT32 ret = 1;
    MDB_INT32 funcRet = 1;

    int _g_connid_bk = -2;

    if (isql->handle < 0)
    {       // check the handle
        if (isql_reconnect(isql))
            return -1;
    }

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    funcRet = isql_BEGINTRANSACTION_real(isql);

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (funcRet < 0)
    {
        close_connection(isql);
        ret = -1;
    }
    else if (funcRet == 0)
        ret = -1;


    return ret;
}

/*****************************************************************************/
//! iSQL_commit 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql(in)    : iSQL's handle
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *    - mobile용 task lock 추가
 *****************************************************************************/
static int __iSQL_commit_internal(iSQL * isql, int flush_mode);
__DECL_PREFIX int
iSQL_commit(iSQL * isql)
{
    return __iSQL_commit_internal(isql, 0);
}

__DECL_PREFIX int
iSQL_commit_flush(iSQL * isql)
{
    return __iSQL_commit_internal(isql, 1);
}

static int
__iSQL_commit_internal(iSQL * isql, int flush_mode)
{
    MDB_INT32 ret = 1;
    MDB_INT32 funcRet = 1;

    int _g_connid_bk = -2;

    if (isql->handle < 0)
    {       // check the handle
        /* do not reconnect at commit */
        isql->last_errno = SQL_E_NOTCONNECTED;
        return -1;
    }

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    if (dbi_Trans_ID(isql->handle) < 0)
    {
        POP_G_CONNID_SQL_API();
        MDB_DB_UNLOCK_SQL_API();

        return 1;
    }

    funcRet = isql_CLOSETRANSACTION_real(isql, 0, flush_mode);

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (funcRet < 0)
    {
        close_connection(isql);
        ret = -1;
    }
    else if (funcRet == 0)
        ret = -1;


    return ret;
}

/*****************************************************************************/
//! iSQL_rollback 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql(in) : iSQL handle
 ************************************
 * \return  return_value
 ************************************
 * \note
 *    - mobile용 task lock 추가
 *****************************************************************************/
static int __iSQL_rollback_internal(iSQL * isql, int flush_mode);

__DECL_PREFIX int
iSQL_rollback(iSQL * isql)
{
    return __iSQL_rollback_internal(isql, 0);
}

__DECL_PREFIX int
iSQL_rollback_flush(iSQL * isql)
{
    return __iSQL_rollback_internal(isql, 1);
}

static int
__iSQL_rollback_internal(iSQL * isql, int flush_mode)
{
    MDB_INT32 ret = 1;
    MDB_INT32 funcRet;

    int _g_connid_bk = -2;

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    if (dbi_Trans_ID(isql->handle) < 0)
    {
        POP_G_CONNID_SQL_API();
        MDB_DB_UNLOCK_SQL_API();
        return 1;
    }

    funcRet = isql_CLOSETRANSACTION_real(isql, 1, flush_mode);

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (funcRet < 0)
    {
        close_connection(isql);
        ret = -1;
    }
    else if (funcRet == 0)
        ret = -1;

    return ret;
}

/*****************************************************************************/
//! iSQL_errno 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX int
iSQL_errno(iSQL * isql)
{
    return isql->last_errno;
}

/*****************************************************************************/
//! iSQL_error 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX char *
iSQL_error(iSQL * isql)
{
    if (isql->last_error == NULL)
    {
        isql->last_error = mdb_strdup(DB_strerror(isql->last_errno));
    }

    return isql->last_error;

}

/*****************************************************************************/
//! iSQL_get_plan_string 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql(in)    : iSQL's handle
 ************************************
 * \return  return_value
 ************************************
 * \note
 *    - mobile용 task lock 추가
 *****************************************************************************/
__DECL_PREFIX char *
iSQL_get_plan_string(iSQL * isql)
{
    char *plan_string = NULL;
    int ret;

    int _g_connid_bk = -2;

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    ret = isql_GETPLAN_real(isql, NULL, &plan_string);

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (ret < 0)
    {
        if (plan_string)
            SMEM_FREE(plan_string);
        close_connection(isql);
        return NULL;
    }
    else if (ret == 0)
    {
        if (plan_string)
            SMEM_FREE(plan_string);
        return NULL;
    }

    return plan_string;
}

__DECL_PREFIX char *
iSQL_stmt_plan_string(iSQL_STMT * stmt)
{
    char *plan_string = NULL;
    int ret;
    int _g_connid_bk = -2;

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(stmt->isql->handle);

    ret = isql_GETPLAN_real(NULL, stmt, &plan_string);

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (ret < 0)
    {
        if (plan_string)
            SMEM_FREE(plan_string);
        return NULL;
    }
    else if (ret == 0)
    {
        if (plan_string)
            SMEM_FREE(plan_string);
        return NULL;
    }

    return plan_string;
}

/*****************************************************************************/
//! iSQL_query_end 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param isql :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX int
iSQL_query_end(iSQL * isql)
{
    int ret = 0;

    int _g_connid_bk = -2;
    int index;
    T_STATEMENT *statement;

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    // THREAD_HANDLE must be accessed after call to PUSH_G_CONNID()
    index = THREAD_HANDLE;
    if (gClients[index].status == iSQL_STAT_CONNECTED)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTLOGON, 0);
        ret = SQL_E_NOTLOGON;
        goto RETURN_LABEL;
    }

    /* if client is an isql,
     * server keeps only one sql statement for correspoding
     * session. RIGHT ? */
    statement = &gClients[index].sql->stmts[0];
    if (statement == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_STATEMENTNOTFOUND, 0);
        ret = SQL_E_STATEMENTNOTFOUND;
        goto RETURN_LABEL;
    }

    SQL_cleanup(&gClients[index].DBHandle, statement, MODE_MANUAL, 1);
    ret = SQL_E_SUCCESS;

  RETURN_LABEL:

    if (get_last_error(isql, 1) < 0)
        ret = -1;

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (ret < 0)
    {
        close_connection(isql);
        return -1;
    }
    else if (ret == 0)
        return -1;

    return 1;
}

typedef enum
{
    DB_NORMAL_START = 0,
    DB_NORMAL_QUOTE,
    DB_NQUERY_START,
    DB_NQUERY_QUOTE,
    DB_NQUERY_DUMMY = MDB_INT_MAX       /* to guarantee sizeof(enum) == 4 */
} DB_NQUERY_STEP;

static DB_NQUERY_STEP
__nquery_step(DB_WCHAR wch, DB_NQUERY_STEP step)
{
    switch (step)
    {
    case DB_NORMAL_START:
        if (wch == L'\'')
            return DB_NORMAL_QUOTE;
        else if (wch == L'n' || wch == L'N')
            return DB_NQUERY_START;
        else
            return DB_NORMAL_START;
    case DB_NORMAL_QUOTE:
        if (wch == L'\'')
            return DB_NORMAL_START;
        else
            return DB_NORMAL_QUOTE;
    case DB_NQUERY_START:
        if (wch == L'\'')
            return DB_NQUERY_QUOTE;
        else
            return DB_NORMAL_START;
    case DB_NQUERY_QUOTE:
        if (wch == L'\'')
            return DB_NORMAL_START;
        else
            return DB_NQUERY_QUOTE;
    default:
        return DB_NQUERY_DUMMY;
    }

    return (DB_NQUERY_STEP) - 1;
}

/*****************************************************************************/
//! _nstring2string

/*! \breif  
 ************************************
 * \param dest(out) :
 * \param src(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - UTF8도 사용 가능하도록 수정
 *
 *****************************************************************************/
static int
_nstring2string(char *dest, DB_WCHAR * src)
{
    int wstringLen = -1;
    int srcLen = 0;
    int destLen = 0;
    DB_NQUERY_STEP beforeStep = 0;
    DB_NQUERY_STEP currStep = 0;
    int i = 0;
    int size_of_wchar = sizeof(DB_WCHAR);

    // 1 bytes를 converting하기 위해서.. NULL 문자도 필요함
    char ch[DB_LOCALCODE_SIZE_MAX * 2];
    DB_WCHAR src_tmp[DB_LOCALCODE_SIZE_MAX];

    wstringLen = sc_wcslen(src);

    sc_memset(src_tmp, 0x00, sizeof(src_tmp));

    while (srcLen < wstringLen)
    {
        // 아래 변환 부분은.. 밑에 1 byte copy로 넣는다..
        currStep = __nquery_step(src[srcLen], beforeStep);

        // 1. '\''가 n'  ' 사이에 들어가는 경우.. 중첩해서 처리해야함
        if (src[srcLen] == L'\'' && beforeStep == DB_NQUERY_QUOTE &&
                currStep == DB_NORMAL_START)
        {   // 요때에는 다음에 나오는 녀석이 '\n'인지 확인하고 끝낸다.

            // 1. error check 길이 문제 해결
            if ((srcLen + 1) >= wstringLen)
                goto normal_continue;

            // caution.. mocha는 2bytes나.. linux에서는 4바이트이다...
            // 고로.. platform 별로 다 처리를 해줘야 한다 
            if (src[srcLen + 1] == L'\'')
            {   // 이 경우에만 2*sizeof(wchar_t) byte 카피

                dest[destLen++] = '\'';
                dest[destLen++] = '\'';
                if (WCHAR_SIZE == 4)
                {
                    dest[destLen++] = '\'';
                    dest[destLen++] = '\'';
                }
                srcLen += 2;
                beforeStep = DB_NQUERY_QUOTE;
                continue;
            }
        }

      normal_continue:
        if ((beforeStep == DB_NQUERY_QUOTE) && (currStep == DB_NQUERY_QUOTE))
        {   // 이 경우에만 sizeof(wchar_t) byte 카피
            char *tmp = (char *) &(src[srcLen]);

            for (i = 0; i < size_of_wchar; ++i)
            {
                dest[destLen] = *(tmp + i);
                if (dest[destLen] == '\'')
                {
                    dest[++destLen] = '\'';
                    dest[++destLen] = '\0';
                    dest[++destLen] = '\0';
                }
                destLen++;
            }
        }
        else
        {   // 이 경우에는 1byte 카피
            src_tmp[0] = src[srcLen];
            src_tmp[1] = src_tmp[2] = '\0';
            sc_wcstombs(ch, src_tmp, DB_LOCALCODE_SIZE_MAX);
            dest[destLen++] = ch[0];
        }
        ++srcLen;
        beforeStep = currStep;
    }

    return destLen;
}

/*****************************************************************************/
//! iSQL_nquery 
/*! \breif  unicode Query문을 character로 변환하여 실행
 ************************************
 * \param isql(in) :     iSQL structure
 * \param query(in)    :     query string(UNICODE)
 ************************************
 * \return  ret < 0 is Error, else is SUCCESS
 ************************************
 * \note
 *  - mobile용 task lock 추가
 *  - DDL인 경우 DB LOCK 처리
 *  - DDL인 경우 DB UNLOCK 처리(TRUNCATE 추가) 
 *****************************************************************************/
__DECL_PREFIX int
iSQL_nquery(iSQL * isql, DB_WCHAR * wQuery)
{
    char *query = NULL;
    MDB_INT32 ret = -1;
    MDB_INT32 queryLen = -1;

    int _g_connid_bk = -2;

    // check arguments
    if (wQuery == NULL || (sc_wcslen(wQuery) <= 0))
        return -1;

    // check the handle
    if (isql->handle < 0)
    {
        if (isql_reconnect(isql))
            return -1;
    }

    /* error clean */
    isql->last_errno = SQL_E_NOERROR;
    SMEM_FREENUL(isql->last_error);

    query = SMEM_XCALLOC(sizeof(DB_WCHAR) * sc_wcslen(wQuery));
    if (query == NULL)
        goto error;

    // must be convert unicode-string 2 char-string
    queryLen = _nstring2string(query, wQuery);

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    ret = isql_QUERY_real(isql, query, queryLen);

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (ret < 0)
    {
        close_connection(isql);
        ret = -1;
    }
    else if (ret == 0)
        ret = -1;
    else
        ret = 0;

    if (isql->last_querytype >= SQL_STMT_CREATE &&
            isql->last_querytype <= SQL_STMT_ROLLBACK)
    {
        if (isql->last_errno == SQL_E_NOERROR)
            iSQL_commit(isql);
        else
            iSQL_rollback(isql);
    }

  error:
    if (query)
        SMEM_FREENUL(query);

    return ret;
}

__DECL_PREFIX int
iSQL_rid_included(iSQL_RES * res)
{
    return 0;
}

__DECL_PREFIX int
iSQL_drop_rid(iSQL * isql, OID rid)
{
    int ret;
    int connid = isql->handle;

    int _g_connid_bk = -2;
    int index;
    int implicit_savepoint_flag = 0;
    int flag;
    T_STATEMENT *statement = NULL;

    if (isql->status != iSQL_STAT_READY)
        return -1;

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(connid);

    index = THREAD_HANDLE;
    flag = gClients[index].flags;
    statement = &gClients[index].sql->stmts[0];
    if (statement == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_STATEMENTNOTFOUND, 0);
        ret = SQL_E_STATEMENTNOTFOUND;
        goto RETURN_LABEL;
    }

    /* Is the following code really needed ? */
    if (SQL_trans_start(connid, statement) == RET_ERROR)
    {
        ret = RET_ERROR;
        goto RETURN_LABEL;
    }

    if (!(flag & OPT_AUTOCOMMIT))
    {       /* for statement atomicity */
        dbi_Trans_Implicit_Savepoint(connid);
        implicit_savepoint_flag = 1;
    }

    ret = dbi_Rid_Drop(connid, rid);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        goto RETURN_LABEL;
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
                SQL_trans_rollback(connid, statement);
            else
            {
                if (implicit_savepoint_flag)
                    SQL_trans_implicit_partial_rollback(connid, statement);
            }
        }
    }
    else
    {
        if (flag & OPT_AUTOCOMMIT)
            SQL_trans_commit(connid, statement);
        else
        {
            if (implicit_savepoint_flag)
                SQL_trans_implicit_savepoint_release(connid, statement);
        }
    }


    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (ret < 0)
    {
        close_connection(isql);
        return -1;
    }
    else if (ret == 0)
        return -1;
    else
        return 0;
}

__DECL_PREFIX int
iSQL_update_rid(iSQL * isql, OID rid, char *fieldname, iSQL_BIND * fieldvalue)
{
    iSQL_FIELD fielddesc;
    int _g_connid_bk = -2;
    int ret;

    if (isql->status != iSQL_STAT_READY)
        return -1;

    sc_strncpy(fielddesc.name, fieldname, FIELD_NAME_LENG);

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    ret = isql_DESCUPDATERID_real(isql, rid, 1, &fielddesc, fieldvalue);

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (ret < 0)
    {
        close_connection(isql);
        return -1;
    }
    else if (ret == 0)
        return -1;
    else
        return 0;
}

__DECL_PREFIX int
iSQL_desc_update_rid(iSQL * isql, OID rid, int numfields,
        iSQL_FIELD * fielddesc, iSQL_BIND * fieldvalue)
{
    int ret;
    int connid = isql->handle;
    int _g_connid_bk = -2;

    if (isql->status != iSQL_STAT_READY)
        return -1;

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(connid);

    ret = isql_DESCUPDATERID_real(isql, rid, numfields, fielddesc, fieldvalue);

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (ret < 0)
    {
        close_connection(isql);
        return -1;
    }
    else if (ret == 0)
        return -1;
    else
        return 0;
}

__DECL_PREFIX int
iSQL_get_rid(iSQL_STMT * stmt, OID * rid)
{
    iSQL_RES *res = stmt->res;

    if (!stmt)
    {
        stmt->isql->last_errno = SQL_E_NOTPREPARED;
        SMEM_FREENUL(stmt->isql->last_error);
        return -1;
    }

    if (!res)
        return -1;

    if (!rid)
        return -1;

    *rid = res->current_rid;

    return 0;
}

/*****************************************************************************/
//! iSQL_nprepare 
/*! \breif  UNICODE용 iSQL_prepare() function
 ************************************
 * \param isql(in)    : iSQL's handle
 * \param query(in) : query
 * \param length(in)    : query's length
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - create this funtion
 *  unicode를 char로 변환한 후.. iSQL_prepare()를 호출한다.
 *****************************************************************************/
__DECL_PREFIX iSQL_STMT *
iSQL_nprepare(iSQL * isql, DB_WCHAR * wQuery, unsigned long int length)
{
    iSQL_STMT *stmt;
    char *query = NULL;
    MDB_INT32 queryLen = -1;

    int _g_connid_bk = -2;

    if (!wQuery || !wQuery[0] || length == 0)
        return NULL;

#if !defined(ANDROID_OS)
    if ((int) length != sc_wcslen(wQuery))
        return NULL;
#endif

    query = SMEM_XCALLOC((int) (sizeof(DB_WCHAR) * length * 2.5));
    if (query == NULL)
        return NULL;

    // must be convert unicode-string 2 char-string
    queryLen = _nstring2string(query, wQuery);

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    stmt = iSQL_prepare(isql, query, (unsigned long int) queryLen);

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    SMEM_FREENUL(query);

    return stmt;
}

__DECL_PREFIX iSQL_STMT *
iSQL_nprepare2(iSQL * isql, DB_WCHAR * wQuery, unsigned long int length,
        isql_res_type resulttype)
{
    iSQL_STMT *stmt;
    char *query = NULL;
    MDB_INT32 queryLen = -1;

    int _g_connid_bk = -2;

    if (!wQuery || !wQuery[0] || length == 0)
        return NULL;

#if !defined(ANDROID_OS)
    if ((int) length != sc_wcslen(wQuery))
        return NULL;
#endif

    query = SMEM_XCALLOC((int) (sizeof(DB_WCHAR) * length * 2.5));
    if (query == NULL)
        return NULL;

    // must be convert unicode-string 2 char-string
    queryLen = _nstring2string(query, wQuery);

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);

    stmt = iSQL_prepare2(isql, query, (unsigned long int) queryLen,
            resulttype);

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    SMEM_FREENUL(query);

    return stmt;
}

/*****************************************************************************/
//! iSQL_Rid_Read
/*! \breif  RID를 이용해서 DATA를 가져온다
 ************************************
 * \param isql(in)            : iSQL's handle
 * \param record_oid(in)    : record's OID
 * \param dColCnt(in)       : 
 * \param ppColumns(in/out)    : Column Name
 * \param pDbTblName(in)    : Table Name
 * \param pResult(in/out)    : Result set
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - REVIEW : BYTE/VARBYTE의 경우 추가
 *****************************************************************************/
__DECL_PREFIX int
iSQL_Rid_Read(iSQL * isql, OID record_oid, int dColCnt, char **ppColumns,
        char *pDbTblName, iSQL_BIND ** pResult)
{
    SYSFIELD_T *nfields_info = NULL;
    struct Container *Cont;     // OID를 이용해서 Table이름을 알기 위한 구조체
    iSQL_BIND *pCurResult;
    iSQL_TIME timest;
    char time_buf[64];
    char *sRecordVal = NULL;
    int dRecSize, dDataSize, dListIdx = 0, dInCnt, dIsNull;
    int ret = DB_SUCCESS;

    char tablename[REL_NAME_LENG];
    int numfields, memory_record_size;
    int item_size;
    DB_INT8 has_variabletype;

    int _g_connid_bk = -2;

    // OID를 더 이상 알아낼 방법이 없으므로 오류 Return
    if (record_oid == NULL_OID)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_INVALIDPARAM, 0);
        return DB_E_INVALIDPARAM;
    }

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(isql->handle);


    // OID를 이용해서 Table의 기본적인 정보를 가져온다.
    Cont = (struct Container *) Cont_get(record_oid);
    if (Cont == NULL)
    {
        ret = DB_FAIL;
        goto end;
    }

    if (pResult == 0x00)
    {
        // copy table name
        if (pDbTblName)
            sc_strcpy(pDbTblName, Cont->base_cont_.name_);

        ret = DB_SUCCESS;
        goto end;
    }

    sc_strcpy(tablename, Cont->base_cont_.name_);
    has_variabletype = Cont->base_cont_.has_variabletype_;
    numfields = Cont->collect_.numfields_;
    memory_record_size = Cont->base_cont_.memory_record_size_;
    item_size = Cont->collect_.item_size_;

    // Table Name을 이용해서 해당 Field의 정보를 읽어 온다.
    nfields_info = (SYSFIELD_T *) mdb_malloc(sizeof(SYSFIELD_T) * numfields);
    if (nfields_info == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFMEMORY, 0);
        ret = DB_E_OUTOFMEMORY;
        goto end;
    }

    ret = _Schema_GetFieldsInfo(tablename, nfields_info);
    if (ret < DB_SUCCESS)
    {
        mdb_free(nfields_info);
        goto end;
    }

    if (has_variabletype)
        dRecSize = REC_ITEM_SIZE(memory_record_size, numfields);
    else
        dRecSize = item_size;

    // Length를 가져와서 해당 Length 만큼 Buffer를 할당(memory record)
    sRecordVal = (char *) mdb_malloc(dRecSize);
    if (sRecordVal == NULL)
    {
        SMEM_FREENUL(nfields_info);
        ret = DB_E_OUTOFMEMORY;
        goto end;
    }

    sc_memset(sRecordVal, 0x00, dRecSize);

    ret = dbi_Direct_Read(isql->handle, tablename, NULL, NULL,
            record_oid, LK_SHARED, sRecordVal, (RecordSize *) & dRecSize,
            NULL);

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    if (ret)
    {
        mdb_free(sRecordVal);
        SMEM_FREENUL(nfields_info);

        return ret;
    }

    // copy table name
    if (pDbTblName)
        sc_strcpy(pDbTblName, tablename);

    pCurResult = (iSQL_BIND *) mdb_malloc(sizeof(iSQL_BIND) * dColCnt);
    if (pCurResult == NULL)
    {
        mdb_free(sRecordVal);
        SMEM_FREENUL(nfields_info);

        return DB_E_OUTOFMEMORY;
    }
    sc_memset(pCurResult, 0x00, sizeof(iSQL_BIND) * dColCnt);

    *(pResult) = pCurResult;

    for (dListIdx = 0; dListIdx < dColCnt; dListIdx++)
    {
        if (ppColumns[dListIdx][0] == 0x00)
            continue;

        for (dInCnt = 0; dInCnt < numfields; dInCnt++)
        {
            // 동일한 이름을 가지는 Column을 찾는다.
            ret = sc_strcasecmp(ppColumns[dListIdx],
                    nfields_info[dInCnt].fieldName);
            if (ret == 0)
            {
                // 해당 Data가 NULL인 경우 다른 정보들은 NULL로 설정한다.
                pCurResult->is_null = (int *) mdb_malloc(sizeof(int));
                if (pCurResult->is_null == NULL)
                {
                    mdb_free(sRecordVal);
                    SMEM_FREENUL(nfields_info);
                    iSQL_Rid_ReadFree(&(*pResult), dColCnt);
                    return DB_E_OUTOFMEMORY;
                }
                sc_memset(pCurResult->is_null, 0x00, sizeof(int));

                // 가져 올 Field가 NULL인지 확인
                dIsNull =
                        DB_VALUE_ISNULL(sRecordVal,
                        nfields_info[dInCnt].position, memory_record_size);
                *(pCurResult->is_null) = (dIsNull) ? 1 : 0;

                // NULL 인 경우 다음 Field 정보를 가져와서 처리
                if (dIsNull)
                {
                    pCurResult++;
                    break;
                }

                // NULL이 아니면 if(!dIsNull)

                // Field의 껍데기 크기를 가져온다.
                dDataSize = nfields_info[dInCnt].length;

                // UniCode 처리를 위해서 Byte Size로 변경
                if (IS_NBS(nfields_info[dInCnt].type))
                    dDataSize = dDataSize * sizeof(DB_WCHAR);

                pCurResult->length = (unsigned long int *)
                        mdb_malloc(sizeof(unsigned long int));
                if (pCurResult->length == NULL)
                {
                    mdb_free(sRecordVal);
                    SMEM_FREENUL(nfields_info);
                    iSQL_Rid_ReadFree(&(*pResult), dColCnt);
                    return DB_E_OUTOFMEMORY;
                }
                if (IS_DATE_OR_TIME_TYPE(nfields_info[dInCnt].type))
                    pCurResult->buffer =
                            (char *) mdb_malloc(sizeof(iSQL_TIME));
                else
                {
                    pCurResult->buffer =
                            (char *) mdb_malloc(dDataSize + sizeof(DB_WCHAR));
                }

                if (pCurResult->buffer == NULL)
                {
                    mdb_free(sRecordVal);
                    SMEM_FREENUL(nfields_info);
                    iSQL_Rid_ReadFree(&(*pResult), dColCnt);
                    return DB_E_OUTOFMEMORY;
                }

                // schema에 정의된 필드의 길이
                pCurResult->buffer_length = nfields_info[dInCnt].length;
                // Data Type
                pCurResult->buffer_type =
                        (isql_data_type) nfields_info[dInCnt].type;

                if (IS_BYTE(pCurResult->buffer_type))
                {
                    // fields 값의 길이
                    sc_memcpy((char *) pCurResult->length,
                            (char *) (sRecordVal +
                                    nfields_info[dInCnt].offset), 4);
                    sc_memcpy(pCurResult->buffer,
                            sRecordVal + nfields_info[dInCnt].offset +
                            INT_SIZE, dDataSize);
                }
                else if (nfields_info[dInCnt].type == DT_TIMESTAMP)
                {
                    *(pCurResult->length) = sizeof(iSQL_TIME);
                    pCurResult->buffer_length = sizeof(iSQL_TIME);      // why?
                    timestamp2char(time_buf,
                            (t_astime *) (sRecordVal +
                                    nfields_info[dInCnt].offset));
                    char2timest(&timest, time_buf, sc_strlen(time_buf));
                    sc_memcpy(pCurResult->buffer, &timest, sizeof(iSQL_TIME));
                }
                else if (nfields_info[dInCnt].type == DT_DATETIME
                        || nfields_info[dInCnt].type == DT_DATE
                        || nfields_info[dInCnt].type == DT_TIME)
                {
                    *(pCurResult->length) = sizeof(iSQL_TIME);
                    pCurResult->buffer_length = sizeof(iSQL_TIME);      // why?
                    datetime2char(time_buf,
                            sRecordVal + nfields_info[dInCnt].offset);
                    char2timest(&timest, time_buf, sc_strlen(time_buf));
                    sc_memcpy(pCurResult->buffer, &timest, sizeof(iSQL_TIME));
                }
                else
                {
                    // fields 값의 길이
                    *(pCurResult->length) = nfields_info[dInCnt].length;
                    sc_memcpy(pCurResult->buffer,
                            sRecordVal + nfields_info[dInCnt].offset,
                            dDataSize);
                }

                pCurResult++;
                break;
            }
        }

        // Check Not Found Column name
        if (dInCnt >= numfields)
        {
            mdb_free(sRecordVal);
            SMEM_FREENUL(nfields_info);
            iSQL_Rid_ReadFree(&(*pResult), dColCnt);
            return SQL_E_INVALIDCOLUMN;
        }
    }

    mdb_free(sRecordVal);
    SMEM_FREENUL(nfields_info);

    return 0;

  end:
    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    return ret;
}

/*****************************************************************************/
//! iSQL_Rid_ReadFree
/*! \breif  RESULTSET 공간을 해제한다
 ************************************
 * \param pResult(in/out)    : Result set
 * \param dColCnt(in)       : number of columns
 ************************************
 * \return  return_value
 ************************************
 * \note
 *****************************************************************************/
__DECL_PREFIX int
iSQL_Rid_ReadFree(iSQL_BIND ** pResult, int dColCnt)
{
    iSQL_BIND *pCurResult;
    int dCnt;

    if (!pResult || !*pResult)
        return 0;

    pCurResult = *(pResult);

    for (dCnt = 0; dCnt < dColCnt; dCnt++)
    {
        if (!pCurResult)
            continue;
        if (pCurResult->length)
            mdb_free(pCurResult->length);
        if (pCurResult->is_null)
            mdb_free(pCurResult->is_null);
        if (pCurResult->buffer)
            mdb_free(pCurResult->buffer);

        pCurResult++;
    }

    mdb_free(*(pResult));
    *pResult = 0x00;
    return 0;
}

// **********************************************************************

extern int MAX_CONNECTION;
__DECL_PREFIX int
iSQL_checkpoint(iSQL * isql)
{
    int _g_connid_bk = -2;
    int ret;
    int connid;

    extern int Checkpoint_GetDirty(void);
    extern __DECL_PREFIX MDB_INT32 sc_db_islocked(void);

    connid = isql->handle;

    if (connid < 0 || connid >= MAX_CONNECTION)
        return DB_E_INVALIDPARAM;

    if (sc_db_islocked())
        return 0;

    if (Checkpoint_GetDirty() == 0)
        return 0;

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(connid);

    ret = dbi_Checkpoint(connid);

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    return ret;
}

__DECL_PREFIX int
iSQL_flush_buffer(void)
{
    int ret;

    extern int Trans_Mgr_Trans_Count(void);

    if (server == NULL || server->fTerminated)
        return DB_E_TERMINATED;

    MDB_DB_LOCK_API();

    /* transaction이 수행 중이면 처리 안되게 함 */
    if (Trans_Mgr_Trans_Count() == 0)
        ret = dbi_FlushBuffer();
    else
        ret = DB_E_NOTPERMITTED;

    MDB_DB_UNLOCK_API();

    return ret;
}

__DECL_PREFIX int
iSQL_get_numLoadSegments(iSQL * isql, int *buf)
{
    int _g_connid_bk = -2;
    int ret;
    int connid;

    connid = isql->handle;

    if (connid < 0 || connid >= MAX_CONNECTION)
        return DB_E_INVALIDPARAM;

    if (buf == NULL)
        return DB_E_INVALIDPARAM;

    MDB_DB_LOCK_SQL_API();
    PUSH_G_CONNID_SQL_API(connid);

    ret = dbi_Get_numLoadSegments(connid, buf);

    POP_G_CONNID_SQL_API();
    MDB_DB_UNLOCK_SQL_API();

    return ret;
}
