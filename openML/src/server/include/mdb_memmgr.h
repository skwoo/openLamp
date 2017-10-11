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

#ifndef _DB_MEMMGR_H
#define _DB_MEMMGR_H

#include "mdb_config.h"
#include "mdb_inner_define.h"

int DBMem_Init(int shmkey, char **ptr);
void __DECL_PREFIX *DBMem_Alloc(int size);
void __DECL_PREFIX DBMem_Free(void *ptr);
void DBMem_AllFree(void);
void __DECL_PREFIX *DBMem_V2P(void *v);
void __DECL_PREFIX *DBMem_P2V(void *p);
void *DBDataMem_Alloc(int size, unsigned long *memid);
void *DBDataMem_V2P(unsigned long v);
void DBDataMem_Free(long v);
void DBDataMem_All_Free(void);

#endif /* _DB_MEMMGR_H */
