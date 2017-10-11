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

#ifndef _MCLIENT_H_
#define _MCLIENT_H_

#include "sql_main.h"

#include "mdb_ppthread.h"

typedef struct
{
    int DBHandle;
    int status;


    int flags;

    unsigned int store_or_use_result;

    T_SQL *sql;

    T_TRANSDESC trans_info;
    T_SQLTYPE last_query;
} T_CLIENT;

extern __DECL_PREFIX T_CLIENT *gClients;

#define THREAD_HANDLE   (*((int*)get_thread_global_area()))
#define CSLOT           gClients[THREAD_HANDLE]

extern __DECL_PREFIX void *get_thread_global_area(void);
extern __DECL_PREFIX int InitializeClient(void);
extern __DECL_PREFIX int InsertClient(char *dbhost, char *dbname,
        char *dbuser, char *dbpassword, int flags);
extern int RemoveClient(void);

#endif // _MCLIENT_H_
