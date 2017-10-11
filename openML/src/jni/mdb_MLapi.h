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

#ifndef    _EDB_FUNC_
#define    _EDB_FUNC_

#include "isql.h"

/* define */
#define EDB_MAX_CONNECTION          20
#define EDB_MAX_STMT_HANDLE_CNT     100

#ifndef BOOL
#define BOOL    int
#endif

#ifndef TRUE
#define    TRUE    1
#endif

#ifndef FALSE
#define FALSE    0
#endif

#define EDB_SUCCESS                 0
#define EDB_NOERROR                 0
#define EDB_ERROR                   -1

#define EDB_ERROR_CONNECT           -100000
#define EDB_ERROR_OVERFLOW_CONNECT  -100001
#define EDB_ERROR_OVERFLOW_QUERY    -100002
#define EDB_ERROR_INVALID_CONNID    -100003
#define EDB_ERROR_MISS_HANDLE       -100004

#define HEDBSTMT int

typedef struct
{
    iSQL_STMT *stmt;
    iSQL_BIND *bindParam;
    iSQL_BIND *bindRes;
    iSQL_RES *res;
    BOOL bUsed;
} EDBQuery;

typedef struct
{
    BOOL bUsed;
    iSQL isql;
    EDBQuery pool[EDB_MAX_STMT_HANDLE_CNT];
} EDBiSQL;

int ML_getconnection();
int ML_connect(int connid, char *dbname);
int ML_disconnect(int connid);
void ML_releaseconnection(int connid);
int ML_commit(int connid, int flush);
int ML_rollback(int connid, int flush);
int ML_createquery(int connid, char *query, int querylen, int UTF16);
int ML_destroyquery(int connid, int hstmt);
int ML_bindparam(int connid, int hstmt, int param_idx, int is_null, int type,
        int param_len, char *data);
int ML_executequery(int connid, int hstmt);
int ML_clearrow(int connid, int hstmt);
int ML_getnextrow(int connid, int hstmt);
int ML_getfieldcount(int connid, int hstmt);
int ML_getfielddata(int connid, int hstmt, int field_idx, int *uniflag,
        int *is_null, int *type, int *data_len, char *data);
int ML_isfieldnull(int connid, int hstmt, int field_idx);
int ML_getfieldname(int connid, int hstmt, int field_idx, char *field_name);
int ML_getfieldtype(int connid, int hstmt, int field_idx);
int ML_getfieldlength(int connid, int hstmt, int field_idx);
int ML_getquerytype(int connid, int hstmt);

#endif
