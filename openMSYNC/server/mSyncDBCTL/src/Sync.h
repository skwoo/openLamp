/* 
    This file is part of openMSync Server module, DB synchronization software.

    Copyright (C) 2012 Inervit Co., Ltd.
        support@inervit.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SYNC_H
#define SYNC_H

#include <stdlib.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <odbcss.h>

#include "../include/mSyncDBCTL.h"
#include "../../protocol.h"


# define FIELD_LEN	(100)         /* 100 byte */

#define LOG_FILE_SIZE (300*1024)  /* 256K */
#define hENV		(1)
#define	hDBC	    (2)
#define hCONNECT	(4)

#define USERDB_ONLY (1)
#define ALL_HANDLES	(2)

#define SYSTEM      (1)
#define PROCESSING	(2)

#define DISCONNECT_ERROR_STATE (2)
#define CONNECT_STATE          (1)
#define DISCONNECT_STATE       (0)


/* db connection 상태 및 user ID 체크*/
#define NO_USER      (-1)   
#define INVALID_USER (-2)
#define EXCESS_USER  (-3)

#define USER_ID_LEN	        (20)
#define VERSION_LEN	        (20)
#define DATE_LEN	        (50)
#define PASSWD_LEN	        (20)
#define APPLICATION_ID_LEN	(30)
#define QUERY_MAX_LEN       (1024)

#define DSN_LEN	       (30)
#define DSN_UID_LEN    (30)
#define DSN_PASSWD_LEN (30)

#define TABLE_NAME_LEN (30)
#define SCRIPT_LEN	   (4000)

#define TEXT_TYPE_LEN  (3072)
#define TIMESTAMP_TYPE_LEN (23)


#include "ODBC.h"

/* process.cpp */
void DisplayError(SQLHSTMT hstmt, char *uid, int order, int returncode);

int	ParseData(char *ptr, 
              char *uid, char *passwd, char *applicationID, int *version);

/* SQLProcessing.cpp */
int  UpdateConnectFlag(ODBC_PARAMETER *pOdbcParam, int flag);
void SetMaxUserCount(int max);
int  CheckCurrentUserCount(ODBC_PARAMETER *pOdbcParam);
int  CheckUserInfo(ODBC_PARAMETER *pOdbcParam, SQLCHAR *passwd);
int  GetStatement(ODBC_PARAMETER *pOdbcParam, SQLCHAR *Statement);
int  ProcessStmt(char *dataBuffer, int dataLength, 
                 ODBC_PARAMETER *pOdbcParam, SQLCHAR *Statement);
int  FetchResult(ODBC_PARAMETER *pOdbcParam, char *sendBuffer, char *recordBuffer);
int  PrepareDownload(char *dataBuffer, ODBC_PARAMETER *pOdbcParam, 
                     int dataLength, SQLCHAR *Statement);
void FreeArrays(ODBC_PARAMETER *pOdbcParam);
int  GetDBConnectTime(ODBC_PARAMETER *pOdbcParam, SQLHSTMT *hstmt, 
                      int flag, char **connectTime);
int  CheckVersion(ODBC_PARAMETER *pOdbcParam, SQLCHAR *application, int version);
int  GetDSN(ODBC_PARAMETER *pOdbcParam, char *srvDSN, SQLCHAR *dsn, 
            SQLCHAR *dsn_uid, SQLCHAR *dsn_passwd);


#endif
