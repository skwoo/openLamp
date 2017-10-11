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


#ifndef __MSYNCDBCTL_H__ 
#define __MSYNCDBCTL_H__

#if defined(MSYNCDBCTL_EXPORTS)            
#   define __DECL_PREFIX_MSYNC    __declspec(dllexport)
#elif defined(MSYNCDBCTL_IMPORTS)
#   define __DECL_PREFIX_MSYNC    __declspec(dllimport)
#elif defined(linux) 
#   define __DECL_PREFIX_MSYNC    __attribute__((visibility("default")))
#else
#   define __DECL_PREFIX_MSYNC
#endif


#ifdef  __cplusplus
extern "C" {
#endif

#define MAX_ODBC_DSN_NAME_LEN (50) 
#define MAX_ODBC_USERID_LEN   (50)
#define MAX_ODBC_PASSWD_LEN   (50)
#define MAX_ODBC_DB_TYPE_LEN  (50)

#define DBSRV_TYPE_ORACLE    (1)
#define DBSRV_TYPE_MSSQL     (2)
#define DBSRV_TYPE_MYSQL     (3)
#define DBSRV_TYPE_ACCESS    (4)
#define DBSRV_TYPE_SYBASE    (5)
#define DBSRV_TYPE_CUBRID    (6)
#define DBSRV_TYPE_DB2       (7)

typedef struct {
    /* for DB Server */
    char szDsn[MAX_ODBC_DSN_NAME_LEN+1]; 
    char szUid[MAX_ODBC_USERID_LEN+1];
    char szPasswd[MAX_ODBC_PASSWD_LEN+1];
    int  nDbType;
    
    /* for mSync Server */
    int  nPortNo;        
    int  nMaxUser;
    int  ntimeout;
} S_mSyncSrv_Info;
    
__DECL_PREFIX_MSYNC int SyncMain(S_mSyncSrv_Info *pSrvInfo, HWND hWnd);
__DECL_PREFIX_MSYNC void SetPath(char *path);
__DECL_PREFIX_MSYNC void MYDeleteCriticalSection();
__DECL_PREFIX_MSYNC void ErrLog(char *format,...);

#ifdef  __cplusplus
}
#endif

#endif /* __MSYNCDBCTL_H__ */
