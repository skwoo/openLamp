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

#ifndef _PACKET_FORMAT_H_
#define _PACKET_FORMAT_H_

#include "mdb_config.h"
#include "sql_main.h"

#ifdef USE_PACKET_SIZE_256
#define DEFAULT_PACKET_SIZE                             1024
#else
#define DEFAULT_PACKET_SIZE                             4096
#endif
#define FOUR_BYTE_ALIGN                                 4

/* Body list */

typedef struct
{
    char name[FIELD_NAME_LENG]; /* name of column */
    int is_null;
    isql_data_type buffer_type;
    int buffer_length;          /* the field's schema length */
    int value_length;           /* the field's value's length */
    int _long_allign;
    char buffer[FOUR_BYTE_ALIGN];
} t_parameter;

struct _t_dbms_cs_execute
{
    unsigned int stmt_id;
    unsigned int query_timeout;
    int param_count;
    t_parameter params[1];
};

// typedef struct _t_dbms_cs_execute T_DBMS_CS_EXECUTE; see sql_main.h 

#endif // _PACKET_FORMAT_H_
