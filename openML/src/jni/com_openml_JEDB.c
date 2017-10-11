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

#include <jni.h>
#include "ConvertString.h"
#include "mdb_MLapi.h"
#include "com_openml_JEDB.h"

JNIEXPORT jint JNICALL
Java_com_openml_JEDB_CreateDB1(JNIEnv * env, jclass obj, jstring dbname)
{
    char *dbtemp = (char *) ((*env)->GetStringUTFChars(env, dbname, 0));
    int ret;

    ret = createdb(dbtemp);

    (*env)->ReleaseStringUTFChars(env, dbname, dbtemp);

    return (ret);
}

JNIEXPORT jint JNICALL
Java_com_openml_JEDB_CreateDB2(JNIEnv * env, jclass obj, jstring dbname,
        jint dbseg_num)
{
    char *dbtemp = (char *) ((*env)->GetStringUTFChars(env, dbname, 0));
    int ret;

    ret = createdb2(dbtemp, dbseg_num);

    (*env)->ReleaseStringUTFChars(env, dbname, dbtemp);

    return (ret);
}

JNIEXPORT jint JNICALL
Java_com_openml_JEDB_CreateDB3(JNIEnv * env, jclass obj, jstring dbname,
        jint dbseg_num, jint idxseg_num, jint tmpseg_num)
{
    char *dbtemp = (char *) ((*env)->GetStringUTFChars(env, dbname, 0));
    int ret;

    ret = createdb3(dbtemp, dbseg_num, idxseg_num, tmpseg_num);

    (*env)->ReleaseStringUTFChars(env, dbname, dbtemp);

    return (ret);
}

JNIEXPORT jint JNICALL
Java_com_openml_JEDB_DestoryDB(JNIEnv * env, jclass obj, jstring dbname)
{
    char *dbtemp = (char *) ((*env)->GetStringUTFChars(env, dbname, 0));
    int ret;

    ret = Drop_DB(dbtemp);

    (*env)->ReleaseStringUTFChars(env, dbname, dbtemp);

    return (ret);
}

JNIEXPORT jint JNICALL
Java_com_openml_JEDB_GetConnection(JNIEnv * env, jclass obj)
{
    return (ML_getconnection());
}

JNIEXPORT jint JNICALL
Java_com_openml_JEDB_Connect(JNIEnv * env, jclass obj, jint connid,
        jstring dbname)
{
    char *dbtemp = (char *) ((*env)->GetStringUTFChars(env, dbname, 0));
    int ret = 0;

    ret = ML_connect(connid, dbtemp);

    (*env)->ReleaseStringUTFChars(env, dbname, dbtemp);

    return (ret);
}

JNIEXPORT void JNICALL
Java_com_openml_JEDB_Disconnect(JNIEnv * env, jclass obj, jint connid)
{
    ML_disconnect(connid);
}

JNIEXPORT void JNICALL
Java_com_openml_JEDB_ReleaseConnection(JNIEnv * env, jclass obj, jint connid)
{
    ML_releaseconnection(connid);
}

JNIEXPORT void JNICALL
Java_com_openml_JEDB_Commit(JNIEnv * env, jclass obj, jint connid)
{
    if (connid >= 0)
        ML_commit(connid, 0);
}

JNIEXPORT void JNICALL
Java_com_openml_JEDB_CommitFlush(JNIEnv * env, jclass obj, jint connid)
{
    if (connid >= 0)
        ML_commit(connid, 1);
}

JNIEXPORT void JNICALL
Java_com_openml_JEDB_Rollback(JNIEnv * env, jclass obj, jint connid)
{
    if (connid >= 0)
        ML_rollback(connid, 0);
}

JNIEXPORT void JNICALL
Java_com_openml_JEDB_RollbackFlush(JNIEnv * env, jclass obj, jint connid)
{
    if (connid >= 0)
        ML_rollback(connid, 1);
}

JNIEXPORT jint JNICALL
Java_com_openml_JEDB_QueryCreate(JNIEnv * env, jclass obj, jint connid,
        jstring query, jint utf16)
{
    DB_WCHAR *wquerytemp;
    char *querytemp;
    int querylen;

    HEDBSTMT hStmt;

    if (connid < 0)
    {
        return (-1);
    }

    if (utf16)
    {
        wquerytemp = (DB_WCHAR *) ((*env)->GetStringChars(env, query, 0));
        querylen = (*env)->GetStringLength(env, query);
        hStmt = ML_createquery(connid, (char *) wquerytemp, querylen, utf16);
        (*env)->ReleaseStringChars(env, query, wquerytemp);
    }
    else
    {
        querytemp = (char *) ((*env)->GetStringUTFChars(env, query, 0));
        querylen = sc_strlen(querytemp);
        hStmt = ML_createquery(connid, querytemp, querylen, utf16);
        (*env)->ReleaseStringUTFChars(env, query, querytemp);
    }

    return (hStmt);

}

JNIEXPORT jint JNICALL
Java_com_openml_JEDB_QueryDestroy(JNIEnv * env, jclass obj, jint connid,
        jint stmt)
{
    if (connid < 0 || stmt < 0)
    {
        return (-1);
    }

    return (ML_destroyquery(connid, stmt));
}

JNIEXPORT jint JNICALL
Java_com_openml_JEDB_QueryBindParam(JNIEnv * env, jclass obj, jint connid,
        jint stmt, jint param_idx, jint is_null, jint type, jstring buffer,
        jint length)
{
    char *param = NULL;
    DB_WCHAR *wparam = NULL;
    int param_len;
    int ret = 0;

    if (connid < 0 || stmt < 0)
    {
        return (-1);
    }

    switch (type)
    {
    case SQL_DATA_TINYINT:
    case SQL_DATA_SMALLINT:
    case SQL_DATA_INT:
    case SQL_DATA_BIGINT:
    case SQL_DATA_FLOAT:
    case SQL_DATA_DOUBLE:
    case SQL_DATA_DECIMAL:
    case SQL_DATA_TIME:
    case SQL_DATA_DATE:
    case SQL_DATA_TIMESTAMP:
    case SQL_DATA_DATETIME:
    case SQL_DATA_CHAR:
    case SQL_DATA_VARCHAR:
    case SQL_DATA_BYTE:
    case SQL_DATA_VARBYTE:
        param = (char *) ((*env)->GetStringUTFChars(env, buffer, 0));
        param_len = length;
        ML_bindparam(connid, stmt, param_idx, is_null, type, param_len, param);
        (*env)->ReleaseStringUTFChars(env, buffer, param);
        break;
    case SQL_DATA_NCHAR:
    case SQL_DATA_NVARCHAR:
        wparam = (DB_WCHAR *) ((*env)->GetStringChars(env, buffer, 0));
        param_len = length;
        ML_bindparam(connid, stmt, param_idx, is_null, type, param_len,
                (char *) wparam);
        (*env)->ReleaseStringChars(env, buffer, wparam);
        break;
        param = (char *) ((*env)->GetStringUTFChars(env, buffer, 0));
    default:
        break;
    }

    return (ret);
}

JNIEXPORT jint JNICALL
Java_com_openml_JEDB_QueryExecute(JNIEnv * env, jclass obj, jint connid,
        jint stmt)
{
    if (connid < 0 || stmt < 0)
    {
        return (-1);
    }

    return (ML_executequery(connid, stmt));
}

JNIEXPORT jint JNICALL
Java_com_openml_JEDB_QueryClearRow(JNIEnv * env, jclass obj, jint connid,
        jint stmt)
{
    if (connid < 0 || stmt < 0)
    {
        return (-1);
    }

    return (ML_clearrow(connid, stmt));
}

JNIEXPORT jint JNICALL
Java_com_openml_JEDB_QueryGetNextRow(JNIEnv * env, jclass obj, jint connid,
        jint stmt)
{
    if (connid < 0 || stmt < 0)
    {
        return (-1);
    }

    return (ML_getnextrow(connid, stmt));
}

JNIEXPORT jint JNICALL
Java_com_openml_JEDB_QueryGetFieldCount(JNIEnv * env, jclass obj,
        jint connid, jint stmt)
{
    if (connid < 0 || stmt < 0)
    {
        return (-1);
    }

    return (ML_getfieldcount(connid, stmt));
}

JNIEXPORT jstring JNICALL
Java_com_openml_JEDB_QueryGetFieldData(JNIEnv * env, jclass obj,
        jint connid, jint stmt, jint field_idx)
{
    jstring result = NULL;
    char ret_str[8192];
    int uniflag, is_null, type, len;

    if (connid < 0 || stmt < 0)
    {
        return NULL;
    }

    memset(ret_str, 0x00, sizeof(ret_str));
    ML_getfielddata(connid, stmt, field_idx, &uniflag, &is_null, &type,
            &len, ret_str);
    switch (type)
    {
    case SQL_DATA_TINYINT:
    case SQL_DATA_SMALLINT:
    case SQL_DATA_INT:
    case SQL_DATA_BIGINT:
    case SQL_DATA_FLOAT:
    case SQL_DATA_DOUBLE:
    case SQL_DATA_DECIMAL:
    case SQL_DATA_CHAR:
    case SQL_DATA_VARCHAR:
    case SQL_DATA_TIMESTAMP:
    case SQL_DATA_DATE:
    case SQL_DATA_TIME:
    case SQL_DATA_DATETIME:
    case SQL_DATA_BYTE:
    case SQL_DATA_VARBYTE:
        ret_str[len] = '\0';
        if (uniflag)
        {
            result = (jstring) javaNewString(env,
                    cstr2jbyteArray(env, ret_str));
        }
        else
        {
            result = (*env)->NewStringUTF(env, ret_str);
        }
        break;
    case SQL_DATA_NCHAR:
    case SQL_DATA_NVARCHAR:
        result = (*env)->NewString(env, (jchar *) ret_str,
                len / sizeof(DB_WCHAR));
        break;
    default:
        break;
    }

    return (result);
}

JNIEXPORT jint JNICALL
Java_com_openml_JEDB_QueryIsFieldNull(JNIEnv * env, jclass obj, jint connid,
        jint stmt, jint field_idx)
{
    if (connid < 0 || stmt < 0)
    {
        return (-1);
    }

    return (ML_isfieldnull(connid, stmt, field_idx));
}

JNIEXPORT jstring JNICALL
Java_com_openml_JEDB_QueryGetFieldName(JNIEnv * env, jclass obj,
        jint connid, jint stmt, jint field_idx)
{
    jstring field_name;
    char ptr[FIELD_NAME_LENG] = "\0";

    if (connid < 0 || stmt < 0)
    {
        return NULL;
    }

    memset(ptr, 0x00, sizeof(ptr));
    ML_getfieldname(connid, stmt, field_idx, ptr);
    field_name = (*env)->NewStringUTF(env, ptr);

    return (field_name);
}

JNIEXPORT jint JNICALL
Java_com_openml_JEDB_QueryGetFieldType(JNIEnv * env, jclass obj,
        jint connid, jint stmt, jint field_idx)
{
    if (connid < 0 || stmt < 0)
    {
        return (-1);
    }

    return (ML_getfieldtype(connid, stmt, field_idx));
}

JNIEXPORT jint JNICALL
Java_com_openml_JEDB_QueryGetFieldLength(JNIEnv * env, jclass obj,
        jint connid, jint stmt, jint field_idx)
{
    if (connid < 0 || stmt < 0)
    {
        return (-1);
    }

    return (ML_getfieldlength(connid, stmt, field_idx));
}

JNIEXPORT jint JNICALL
Java_com_openml_JEDB_QueryGetQueryType(JNIEnv * env, jclass obj,
        jint connid, jint stmt)
{
    if (connid < 0 || stmt < 0)
    {
        return (-1);
    }

    return (ML_getquerytype(connid, stmt));
}
