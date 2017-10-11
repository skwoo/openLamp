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

#ifndef __PMEM_H
#define __PMEM_H

#include "mdb_config.h"
#include "dbport.h"

/* Shared memory management */
#define SMEM_ALLOC(a)       mdb_malloc_sys(a)
#define SMEM_REALLOC(a,b)   sc_realloc((a),(b))
#define SMEM_FREE(a)        mdb_free_sys(a)

#define SMEM_XCALLOC(a)     xcalloc(a)

#define SMEM_FREENUL(p) \
    do { \
        if ((p) != NULL) { \
            mdb_free((p)); \
            (p) = NULL; \
        } \
    } while(0)

#ifdef MDB_DEBUG
#define PMEM_ALLOC(a)       xcalloc(a)
#else
#define PMEM_ALLOC(a)       xcalloc(a)  //mdb_malloc(a)
#endif
#define PMEM_REALLOC(a,b)   sc_realloc((a),(b))
#define PMEM_FREE(a)        mdb_free(a)
#define PMEM_XCALLOC(a)     xcalloc(a)

#define PMEM_FREENUL(p) \
    do { \
        if ((p) != NULL) { \
            PMEM_FREE((p)); \
            (p) = NULL; \
        } \
    } while(0)

__DECL_PREFIX void *PMEM_XCALLOC(unsigned long int s);

extern __DECL_PREFIX void *xcalloc(unsigned long int s);

extern char *PMEM_STRDUP(const char *s1);

extern void *PMEM_WALLOC(unsigned int a);

extern DB_WCHAR *PMEM_WSTRDUP(const DB_WCHAR * s1);

#endif /* __PMEM_H */
