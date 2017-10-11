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
#include "mdb_define.h"
#include "mdb_syslog.h"
#include "mdb_ppthread.h"
#include "mdb_Trans.h"
#include "mdb_PMEM.h"
#include "mdb_Server.h"

/*****************************************************************************/
//! xcalloc
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param s(in) :    memory size
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX void *
xcalloc(unsigned long int s)
{
    void *p = mdb_malloc(s);

    if (p)
        sc_memset(p, 0, s);
    return p;
}

/*****************************************************************************/
//! PMEM_STRDUP
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param s1 :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
char *
PMEM_STRDUP(const char *s1)
{
    char *s2;

    if (s1 == NULL)
        return NULL;

    s2 = PMEM_ALLOC(sc_strlen(s1) + 1);
    if (s2 != NULL)
        sc_strcpy(s2, s1);

    return s2;
}

void *
PMEM_WALLOC(unsigned int size)
{
    void *p;

    size *= sizeof(DB_WCHAR);

    p = PMEM_ALLOC(size);

    return p;
}

DB_WCHAR *
PMEM_WSTRDUP(const DB_WCHAR * s1)
{
    DB_WCHAR *s2;
    int wstrlen = 0;

    if (s1 == NULL)
        return NULL;

    wstrlen = sc_wcslen(s1);

    s2 = (DB_WCHAR *) PMEM_WALLOC(wstrlen + 1);
    if (s2 != NULL)
        sc_wcsncpy(s2, s1, wstrlen);

    return s2;
}
