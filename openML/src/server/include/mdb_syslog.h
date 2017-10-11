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

#ifndef __SYSLOG_H_
#define __SYSLOG_H_

#ifdef  __cplusplus
extern "C"
{
#endif                          // _cplusplus

#include "mdb_config.h"
#include "dbport.h"

#if defined(psos)
#define __SYSLOG        printf
#define __SYSLOG_TIME   printf
#define __SYSLOG2       printf
#define __SYSLOG2_TIME  printf
#else
    __DECL_PREFIX void __SYSLOG(char *format, ...);
    __DECL_PREFIX void __SYSLOG_TIME(char *format, ...);
    /* SYSLOG_BUFFER */
    void __SYSLOG_syncbuf(int f_discard);
    void __SYSLOG_close(void);

#if !defined(WIN32_MOBILE) && !defined(linux) && !defined(sun)
#define __SYSLOG2       __SYSLOG
#define __SYSLOG2_TIME  __SYSLOG_TIME
#else
    __DECL_PREFIX void __SYSLOG2(char *format, ...);
    void __SYSLOG2_TIME(char *format, ...);
#endif
#endif                          // !psos

#if defined(DO_NOT_SUPPORT_MDB_SYSLOG)
#define MDB_SYSLOG(a)
#define MDB_SYSLOG2(a)
#else
#define MDB_SYSLOG(a)   __SYSLOG a
#define MDB_SYSLOG2(a)  __SYSLOG2 a
#endif

#ifdef  __cplusplus
}
#endif                          // _cplusplus

#endif
