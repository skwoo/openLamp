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

#ifndef _COMMSTUB_H_
#define _COMMSTUB_H_

#ifdef  __cplusplus
extern "C"
{
#endif

#include "mdb_config.h"

#include "mdb_typedef.h"
#include "mdb_ppthread.h"
#include "ErrorCode.h"

#include "mdb_inner_define.h"

#define NUM_SPECIFIC    4

    typedef struct
    {
        char dbname[MDB_FILE_NAME];
        char dbserver[32];
        char userid[32];
        char passwd[32];
        short int dbport;
        DB_UINT8 connType;
        DB_UINT8 clientType;
        int pid;
        pthread_t thrid;
        void *isql;
        int sockfd;
        int bufsize;
        int read_pos;
        int write_pos;
        char *buf;
        short int status;       /* 1: server 측 수행 */
        short int interrupt;
        /* connection 별 정보. thread specific data 대신해서 사용
         * 0: thread_entry_key, 1: my_trans_id
         * 2: gKey, 3: gClientidx_key */
        void *specific[NUM_SPECIFIC];
    } T_CS_OBJECT;

/* connection type */
#define CS_CONNECT_NONE     0xF
#define CS_CONNECT_DEFAULT  0
#define CS_CONNECT_LIB      10

/* client Type */
#define CS_CLIENT_NORMAL    0x00

    __DECL_PREFIX extern int _g_connid;

    __DECL_PREFIX int CS_Init(int numConn);

    __DECL_PREFIX int CS_GetCSObject(char *dbserver, char *dbname,
            char *userid, char *passwd, int connType, int clientType,
            int sockfd);

    __DECL_PREFIX void CS_FreeCSObject(int connid);

    int CS_Connect(char *dbserver, char *dbname, char *userid, char *passwd,
            int clientType);

    int CS_Disconnect(int connid);

    __DECL_PREFIX int CS_Get_ClientType(int connid);

    int CS_Check_Connid(int connid);

    void *CS_getspecific(int key);
    int CS_setspecific(int key, const void *value);
    int CS_setspecific_int(int key, void **value);

    int CS_Set_iSQL(int handle, void *isql);

#ifdef  __cplusplus
}
#endif

#endif                          /* _COMMSTUB_H_ */
