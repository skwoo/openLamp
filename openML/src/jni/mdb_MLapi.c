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

#include <stdio.h>
#include <stdlib.h>
#include "mdb_MLapi.h"

static EDBiSQL gISQL[EDB_MAX_CONNECTION];

static int is_init_connection = 0;

int
ML_getconnection()
{
    int i;

    if (!is_init_connection)
    {
        memset(gISQL, 0x00, sizeof(EDBiSQL) * EDB_MAX_CONNECTION);
        is_init_connection = 1;
        gISQL[0].bUsed = -1;
        return (0);
    }

    for (i = 0; i < EDB_MAX_CONNECTION; ++i)
    {
        if (gISQL[i].bUsed == 0)
        {
            break;
        }
    }

    if (i == EDB_MAX_CONNECTION)
    {
        return (EDB_ERROR_OVERFLOW_CONNECT);
    }

    gISQL[i].bUsed = -1;

    return (i);
}

int
ML_connect(int connid, char *dbname)
{
    iSQL *rc;

    if (connid < 0 || connid >= EDB_MAX_CONNECTION
            || gISQL[connid].bUsed != -1)
    {
        return (EDB_ERROR_INVALID_CONNID);
    }

    rc = iSQL_connect(&gISQL[connid].isql, "127.0.0.1", dbname,
            "litedb", "litedb");
    if (rc == NULL)
    {
        gISQL[connid].bUsed = -1;
        return (EDB_ERROR_CONNECT);
    }

    gISQL[connid].bUsed = 1;

    memset(gISQL[connid].pool, 0x00,
            sizeof(EDBQuery) * EDB_MAX_STMT_HANDLE_CNT);

    return (connid);
}

int
ML_disconnect(int connid)
{
    int i;

    if (gISQL[connid].bUsed == 1)
    {
        for (i = 0; i < EDB_MAX_STMT_HANDLE_CNT; i++)
        {
            if (gISQL[connid].pool[i].bUsed == 1 && gISQL[connid].pool[i].stmt)
            {
                ML_destroyquery(connid, i);
            }
        }

        iSQL_disconnect(&gISQL[connid].isql);

        gISQL[connid].bUsed = -1;
    }

    return (connid);
}

void
ML_releaseconnection(int connid)
{
    if (!is_init_connection)
    {
        return;
    }

    if (connid >= 0 && connid < EDB_MAX_CONNECTION)
    {
        ML_disconnect(connid);
        gISQL[connid].bUsed = 0;
    }

    return;
}

int
ML_commit(int connid, int flush)
{
    iSQL *isql;

    if (connid < 0 || connid >= EDB_MAX_CONNECTION || gISQL[connid].bUsed != 1)
    {
        return (EDB_ERROR_INVALID_CONNID);
    }

    isql = &gISQL[connid].isql;

    if (flush == 0)
    {
        iSQL_commit(isql);
    }
    else
    {
        iSQL_commit_flush(isql);
    }

    return (0);
}

int
ML_rollback(int connid, int flush)
{
    iSQL *isql;

    if (connid < 0 || connid >= EDB_MAX_CONNECTION || gISQL[connid].bUsed != 1)
    {
        return (EDB_ERROR_INVALID_CONNID);
    }

    isql = &gISQL[connid].isql;

    if (flush == 0)
    {
        iSQL_rollback(isql);
    }
    else
    {
        iSQL_rollback_flush(isql);
    }

    return (0);
}

int
ML_createquery(int connid, char *query, int querylen, int UTF16)
{
    int result;
    iSQL *isql;
    EDBQuery *stmtpool;
    int i;

    if (connid < 0 || connid >= EDB_MAX_CONNECTION || gISQL[connid].bUsed != 1)
    {
        return (EDB_ERROR_INVALID_CONNID);
    }

    isql = &gISQL[connid].isql;
    stmtpool = gISQL[connid].pool;

    for (i = 0; i < EDB_MAX_STMT_HANDLE_CNT; i++)
    {
        if (stmtpool[i].bUsed == 0)
        {
            stmtpool[i].bUsed = 1;
            break;
        }
    }

    if (i == EDB_MAX_STMT_HANDLE_CNT)
    {
        return (EDB_ERROR_OVERFLOW_QUERY);
    }

    if (UTF16 == 1)
    {
        ((DB_WCHAR *) (query))[querylen] = 0x00;
        stmtpool[i].stmt = iSQL_nprepare(isql, (DB_WCHAR *) query, querylen);
    }
    else
    {
        stmtpool[i].stmt = iSQL_prepare(isql, query, querylen);
    }

    if (!stmtpool[i].stmt)
    {
        result = iSQL_errno(isql);
        goto ERROR_LABEL;
    }

    result = iSQL_describe(stmtpool[i].stmt, &stmtpool[i].bindParam,
            &stmtpool[i].bindRes);
    if (result < 0)
    {
        goto ERROR_LABEL;
    }

    switch (iSQL_stmt_querytype(stmtpool[i].stmt))
    {
    case SQL_STMT_SELECT:
    case SQL_STMT_DESCRIBE:
        stmtpool[i].res = iSQL_prepare_result(stmtpool[i].stmt);
        if (!stmtpool[i].res)
        {
            result = iSQL_errno(isql);
            goto ERROR_LABEL;
        }
        break;
    default:
        break;
    }

    return (i);

  ERROR_LABEL:

    ML_destroyquery(connid, i);

    return (result);
}

int
ML_destroyquery(int connid, int hstmt)
{
    EDBQuery *stmtptr;

    if (connid < 0 || connid >= EDB_MAX_CONNECTION || gISQL[connid].bUsed != 1)
    {
        return (EDB_ERROR_INVALID_CONNID);
    }

    stmtptr = &(gISQL[connid].pool[hstmt]);

    if (hstmt < 0 || hstmt >= EDB_MAX_STMT_HANDLE_CNT || !stmtptr->bUsed)
    {
        return (EDB_ERROR_MISS_HANDLE);
    }

    if (stmtptr->res)
    {
        iSQL_free_result(stmtptr->res);
        stmtptr->res = NULL;
    }
    if (stmtptr->stmt)
    {
        iSQL_stmt_close(stmtptr->stmt);
        stmtptr->stmt = NULL;
    }

    if (stmtptr->bindParam)
    {
        stmtptr->bindParam = NULL;
    }
    if (stmtptr->bindRes)
    {
        stmtptr->bindRes = NULL;
    }

    stmtptr->bUsed = 0;

    return (0);
}

extern iSQL_TIME *char2timest(iSQL_TIME * timest, char *stime, int sleng);
int
ML_bindparam(int connid, int hstmt, int param_idx, int is_null, int type,
        int param_len, char *data)
{
    EDBQuery *stmtptr;

    DB_INT8 ivalue8;
    DB_INT16 ivalue16;
    DB_INT32 ivalue32;
    DB_INT64 ivalue64;
    float fvalue;
    double dvalue;
    iSQL_TIME tm;
    iSQL_BIND *bindParam = NULL;

    if (connid < 0 || connid >= EDB_MAX_CONNECTION || gISQL[connid].bUsed != 1)
    {
        return (EDB_ERROR_INVALID_CONNID);
    }

    stmtptr = &(gISQL[connid].pool[hstmt]);

    if (hstmt < 0 || hstmt >= EDB_MAX_STMT_HANDLE_CNT || !stmtptr->bUsed)
    {
        return (EDB_ERROR_MISS_HANDLE);
    }

    if (stmtptr->bindParam)
    {
        bindParam = &(stmtptr->bindParam[param_idx]);
    }

    if (!bindParam)
    {
        return (0);
    }

    *(bindParam->is_null) = is_null;

    if (is_null)
    {
        return (0);
    }

    switch (type)
    {
    case SQL_DATA_TINYINT:
        ivalue8 = sc_atoi(data);
        sc_memcpy(bindParam->buffer, &ivalue8, sizeof(DB_INT8));
        break;
    case SQL_DATA_SMALLINT:
        ivalue16 = sc_atoi(data);
        sc_memcpy(bindParam->buffer, &ivalue16, sizeof(DB_INT16));
        break;
    case SQL_DATA_INT:
        ivalue32 = sc_atoi(data);
        sc_memcpy(bindParam->buffer, &ivalue32, sizeof(DB_INT32));
        break;
    case SQL_DATA_BIGINT:
        ivalue64 = sc_atoll(data);
        sc_memcpy(bindParam->buffer, &ivalue64, sizeof(DB_INT64));
        break;
    case SQL_DATA_FLOAT:
        fvalue = (float) sc_atof(data);
        sc_memcpy(bindParam->buffer, &fvalue, sizeof(float));
        break;
    case SQL_DATA_DOUBLE:
    case SQL_DATA_DECIMAL:
        dvalue = sc_atof(data);
        sc_memcpy(bindParam->buffer, &dvalue, sizeof(double));
        break;
    case SQL_DATA_CHAR:
    case SQL_DATA_VARCHAR:
        sc_memcpy(bindParam->buffer, data, param_len);
        bindParam->buffer[param_len] = '\0';
        *bindParam->length = param_len;
        break;
    case SQL_DATA_NCHAR:
    case SQL_DATA_NVARCHAR:
        sc_memcpy(bindParam->buffer, data, param_len * sizeof(DB_WCHAR));
        bindParam->buffer[param_len] = '\0';
        *bindParam->length = param_len;
        break;
    case SQL_DATA_BYTE:
    case SQL_DATA_VARBYTE:
        sc_memcpy(bindParam->buffer, data, param_len);
        *bindParam->length = param_len;
        break;
    case SQL_DATA_TIME:
    case SQL_DATA_DATE:
    case SQL_DATA_TIMESTAMP:
    case SQL_DATA_DATETIME:
        char2timest(&tm, data, param_len);
        sc_memcpy(bindParam->buffer, &tm, sizeof(iSQL_TIME));
        break;
    default:
        break;
    }

    return (0);
}

int
ML_executequery(int connid, int hstmt)
{
    EDBQuery *stmtptr;
    int result;

    if (connid < 0 || connid >= EDB_MAX_CONNECTION || gISQL[connid].bUsed != 1)
    {
        return (EDB_ERROR_INVALID_CONNID);
    }

    stmtptr = &(gISQL[connid].pool[hstmt]);

    if (hstmt < 0 || hstmt >= EDB_MAX_STMT_HANDLE_CNT
            || !stmtptr->bUsed || stmtptr->stmt == NULL)
    {
        return (EDB_ERROR_MISS_HANDLE);
    }

    if (iSQL_execute(stmtptr->stmt) < 0)
    {
        return (iSQL_stmt_errno(stmtptr->stmt));
    }

    switch (iSQL_stmt_querytype(stmtptr->stmt))
    {
    case SQL_STMT_SELECT:
    case SQL_STMT_DESCRIBE:
        result = iSQL_stmt_store_result(stmtptr->stmt);
        if (result < 0)
        {
            return (iSQL_stmt_errno(stmtptr->stmt));
        }
    case SQL_STMT_INSERT:
    case SQL_STMT_UPSERT:
    case SQL_STMT_UPDATE:
    case SQL_STMT_DELETE:
        result = iSQL_stmt_affected_rows(stmtptr->stmt);
        break;
    default:
        result = 0;
        break;
    }

    return (result);
}

int
ML_clearrow(int connid, int hstmt)
{
    EDBQuery *stmtptr;

    if (connid < 0 || connid >= EDB_MAX_CONNECTION || gISQL[connid].bUsed != 1)
    {
        return (EDB_ERROR_INVALID_CONNID);
    }

    stmtptr = &(gISQL[connid].pool[hstmt]);

    if (hstmt < 0 || hstmt >= EDB_MAX_STMT_HANDLE_CNT
            || !stmtptr->bUsed || stmtptr->stmt == NULL)
    {
        return (EDB_ERROR_MISS_HANDLE);
    }

    if (stmtptr->res)
    {
        iSQL_free_result(stmtptr->res);
        stmtptr->res = NULL;
    }

    return (0);
}

int
ML_getnextrow(int connid, int hstmt)
{
    EDBQuery *stmtptr;

    if (connid < 0 || connid >= EDB_MAX_CONNECTION || gISQL[connid].bUsed != 1)
    {
        return (EDB_ERROR_INVALID_CONNID);
    }

    stmtptr = &(gISQL[connid].pool[hstmt]);

    if (hstmt < 0 || hstmt >= EDB_MAX_STMT_HANDLE_CNT
            || !stmtptr->bUsed || stmtptr->stmt == NULL)
    {
        return (EDB_ERROR_MISS_HANDLE);
    }

    if (iSQL_fetch(stmtptr->stmt) < 0)
    {
        return (iSQL_stmt_errno(stmtptr->stmt));
    }

    return (0);
}

int
ML_getfieldcount(int connid, int hstmt)
{
    EDBQuery *stmtptr;

    if (connid < 0 || connid >= EDB_MAX_CONNECTION || gISQL[connid].bUsed != 1)
    {
        return (EDB_ERROR_INVALID_CONNID);
    }

    stmtptr = &(gISQL[connid].pool[hstmt]);

    if (hstmt < 0 || hstmt >= EDB_MAX_STMT_HANDLE_CNT
            || !stmtptr->bUsed || stmtptr->stmt == NULL)
    {
        return (EDB_ERROR_MISS_HANDLE);
    }

    return (iSQL_num_fields(stmtptr->res));
}

int
ML_getfielddata(int connid, int hstmt, int field_idx, int *uniflag,
        int *is_null, int *type, int *data_len, char *data)
{
    EDBQuery *stmtptr;
    DB_INT64 lvalue;
    double dvalue;
    iSQL_TIME *tm;
    iSQL_BIND *bindRes = NULL;

    if (connid < 0 || connid >= EDB_MAX_CONNECTION || gISQL[connid].bUsed != 1)
    {
        return (EDB_ERROR_INVALID_CONNID);
    }

    stmtptr = &(gISQL[connid].pool[hstmt]);

    if (stmtptr->bindRes)
    {
        bindRes = &(stmtptr->bindRes[field_idx]);
    }

    if (!bindRes || *(bindRes->is_null))
    {
        *is_null = 1;
        return (0);
    }

    *uniflag = 0;
    *is_null = 0;
    *data_len = 0;
    *type = bindRes->buffer_type;

    switch (bindRes->buffer_type)
    {
    case SQL_DATA_TINYINT:
        sc_sprintf(data, "%d", bindRes->buffer[0]);
        *data_len = sc_strlen(data);
        break;
    case SQL_DATA_SMALLINT:
        sc_sprintf(data, "%d", *(short int *) bindRes->buffer);
        *data_len = sc_strlen(data);
        break;
    case SQL_DATA_INT:
        sc_sprintf(data, "%d", *(int *) bindRes->buffer);
        *data_len = sc_strlen(data);
        break;
    case SQL_DATA_BIGINT:
        sc_memcpy(&lvalue, bindRes->buffer, sizeof(DB_INT64));
        sc_sprintf(data, I64_FORMAT, lvalue);
        *data_len = sc_strlen(data);
        break;
    case SQL_DATA_FLOAT:
        sc_sprintf(data, "%f", *(float *) bindRes->buffer);
        {
            char *p = sc_strchr(data, '.');
            int len = sc_strlen(data);

            if (p)
            {
                while (len > 0 && data[--len] == '0');
                if (data[len] != '.')
                {
                    len++;
                }
                data[len] = '\0';
            }
        }
        *data_len = sc_strlen(data);
        break;
    case SQL_DATA_DOUBLE:
    case SQL_DATA_DECIMAL:
        sc_memcpy(&dvalue, bindRes->buffer, sizeof(double));
        sc_sprintf(data, "%f", dvalue);
        {
            char *p = sc_strchr(data, '.');

            if (p)
            {
                *p = '\0';
            }
        }
        *data_len = sc_strlen(data);
        break;
    case SQL_DATA_CHAR:
    case SQL_DATA_VARCHAR:
        {
            char *bytes = data;

            *uniflag = 0;

            memcpy(data, bindRes->buffer, bindRes->buffer_length);

            while (*bytes != '\0' && !(*is_null))
            {
                char utf8 = (*bytes >> 4);

                if (utf8 >= 0x00 && utf8 <= 0x07)
                {
                    bytes++;
                    continue;
                }
                *uniflag = 1;
                break;
            }
        }
        *data_len = sc_strlen(data);
        break;
    case SQL_DATA_NCHAR:
    case SQL_DATA_NVARCHAR:
        *data_len = sc_wcslen((DB_WCHAR *) bindRes->buffer) * sizeof(DB_WCHAR);
        sc_memcpy(data, bindRes->buffer, *data_len);
        break;
    case SQL_DATA_BYTE:
    case SQL_DATA_VARBYTE:
        *data_len = (*bindRes->length);
        sc_memcpy(data, bindRes->buffer, (*bindRes->length));
        break;
    case SQL_DATA_TIMESTAMP:
        tm = (iSQL_TIME *) bindRes->buffer;
        sc_sprintf(data, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                tm->year, tm->month, tm->day, tm->hour, tm->minute,
                tm->second, tm->fraction);
        *data_len = sc_strlen(data);
        break;
    case SQL_DATA_DATE:
        tm = (iSQL_TIME *) bindRes->buffer;
        sc_sprintf(data, "%04d-%02d-%02d", tm->year, tm->month, tm->day);
        *data_len = sc_strlen(data);
        break;
    case SQL_DATA_TIME:
        tm = (iSQL_TIME *) bindRes->buffer;
        sc_sprintf(data, "%02d:%02d:%02d", tm->hour, tm->minute, tm->second);
        *data_len = sc_strlen(data);
        break;
    case SQL_DATA_DATETIME:
        tm = (iSQL_TIME *) bindRes->buffer;
        sc_sprintf(data, "%04d-%02d-%02d %02d:%02d:%02d", tm->year,
                tm->month, tm->day, tm->hour, tm->minute, tm->second);
        *data_len = sc_strlen(data);
        break;
    default:
        break;
    }

    return (0);
}

int
ML_isfieldnull(int connid, int hstmt, int field_idx)
{
    EDBQuery *stmtptr;
    iSQL_BIND *bindRes;

    if (connid < 0 || connid >= EDB_MAX_CONNECTION || gISQL[connid].bUsed != 1)
    {
        return (EDB_ERROR_INVALID_CONNID);
    }

    stmtptr = &(gISQL[connid].pool[hstmt]);

    if (hstmt < 0 || hstmt >= EDB_MAX_STMT_HANDLE_CNT
            || !stmtptr->bUsed || stmtptr->stmt == NULL
            || stmtptr->res == NULL)
    {
        return (EDB_ERROR_MISS_HANDLE);
    }

    bindRes = &(stmtptr->bindRes[field_idx]);

    if (*(bindRes->is_null))
    {
        return (1);
    }
    else
    {
        return (0);
    }

    return (0);
}

int
ML_getfieldname(int connid, int hstmt, int field_idx, char *field_name)
{
    EDBQuery *stmtptr;

    if (connid < 0 || connid >= EDB_MAX_CONNECTION || gISQL[connid].bUsed != 1)
    {
        return (EDB_ERROR_INVALID_CONNID);
    }

    stmtptr = &(gISQL[connid].pool[hstmt]);

    if (hstmt < 0 || hstmt >= EDB_MAX_STMT_HANDLE_CNT
            || !stmtptr->bUsed || stmtptr->stmt == NULL
            || stmtptr->res == NULL)
    {
        return (EDB_ERROR_MISS_HANDLE);
    }

    sc_strncpy(field_name, stmtptr->res->fields[field_idx].name,
            FIELD_NAME_LENG - 1);
    field_name[FIELD_NAME_LENG - 1] = '\0';

    return (0);
}

int
ML_getfieldtype(int connid, int hstmt, int field_idx)
{
    EDBQuery *stmtptr;

    if (connid < 0 || connid >= EDB_MAX_CONNECTION || gISQL[connid].bUsed != 1)
    {
        return (EDB_ERROR_INVALID_CONNID);
    }

    stmtptr = &(gISQL[connid].pool[hstmt]);

    if (hstmt < 0 || hstmt >= EDB_MAX_STMT_HANDLE_CNT
            || !stmtptr->bUsed || stmtptr->stmt == NULL
            || stmtptr->res == NULL)
    {
        return (EDB_ERROR_MISS_HANDLE);
    }

    return (stmtptr->res->fields[field_idx].type);
}

int
ML_getfieldlength(int connid, int hstmt, int field_idx)
{
    EDBQuery *stmtptr;

    if (connid < 0 || connid >= EDB_MAX_CONNECTION || gISQL[connid].bUsed != 1)
    {
        return (EDB_ERROR_INVALID_CONNID);
    }

    stmtptr = &(gISQL[connid].pool[hstmt]);

    if (hstmt < 0 || hstmt >= EDB_MAX_STMT_HANDLE_CNT
            || !stmtptr->bUsed || stmtptr->stmt == NULL
            || stmtptr->res == NULL)
    {
        return (EDB_ERROR_MISS_HANDLE);
    }

    return (stmtptr->res->fields[field_idx].max_length);
}

int
ML_getquerytype(int connid, int hstmt)
{
    EDBQuery *stmtptr;

    if (connid < 0 || connid >= EDB_MAX_CONNECTION || gISQL[connid].bUsed != 1)
    {
        return (EDB_ERROR_INVALID_CONNID);
    }

    stmtptr = &(gISQL[connid].pool[hstmt]);

    if (hstmt < 0 || hstmt >= EDB_MAX_STMT_HANDLE_CNT
            || !stmtptr->bUsed || stmtptr->stmt == NULL)
    {
        return (EDB_ERROR_MISS_HANDLE);
    }

    return (stmtptr->stmt->querytype);
}
