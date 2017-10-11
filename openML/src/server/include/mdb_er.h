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

#ifndef ER_H
#define ER_H

#include "ErrorCode.h"
#include "mdb_inner_define.h"
#include "mdb_syslog.h"

#if defined(MDB_DEBUG) || defined(WIN32)
#define ARG_FILE_LINE           __FILE__, __LINE__
#else
#define ARG_FILE_LINE           (NULL)
#endif

#ifdef WIN32
#define PRINT_ERRNO()    MDB_SYSLOG(("%d, %d, %d\n", errno, GetLastError(), WSAGetLastError()))
#else
#define PRINT_ERRNO()    MDB_SYSLOG(("%d\n", errno))
#endif

typedef enum
{
    ER_WARNING_SEVERITY = 0,
    ER_NOTIFICATION_SEVERITY = 1,
    ER_ERROR_SEVERITY = 2,
    ER_FATAL_ERROR_SEVERITY = 3,        /* Fatal system error */

    ER_MAX_SEVERITY = ER_FATAL_ERROR_SEVERITY,
    ER_DUMMY = MDB_INT_MAX      /* to guarantee sizeof(enum) == 4 */
} ER_SEVERITY;



typedef struct er_msg
{
    int er_id;                  /* Error identifier of the current message */
    ER_SEVERITY severity;       /* Warning, Error, FATAL Error, etc... */
#if defined(MDB_DEBUG) || defined(WIN32)
    char file_name[MDB_FILE_NAME];      /* File where the error is set */
    int lineno;                 /* Line in the file where the error is set */
#endif
    int msg_area_size;          /* Size of the message area */
    char *msg_area;             /* Pointer to message area */
} ER_MSG;

extern const char *er_get_msg(int errorno);

#if defined(MDB_DEBUG) || defined(WIN32)
extern void er_set(int severity, const char *file_name,
        const int lineno, int errid, int num_args, ...);
extern void er_set_parser(int severity, const char *file_name,
        const int lineno, int er_id, char *err_msg);
#else
extern void er_set_real(int severity, int errid, int num_args, ...);
extern void er_set_parser_real(int severity, int er_id, char *err_msg);

#define er_set(a, b, c, ...)       er_set_real(a, c, ## __VA_ARGS__)
#define er_set_parser(a, b, c, d)  er_set_parser_real(a, c, d)
#endif

extern void er_clear(void);
extern __DECL_PREFIX int er_errid(void);
extern __DECL_PREFIX char *er_message(void);

#endif
