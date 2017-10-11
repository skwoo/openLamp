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

#ifndef _SQL_PLAN_H
#define _SQL_PLAN_H

#include "mdb_config.h"
#include "sql_datast.h"
#include "ErrorCode.h"

typedef enum
{
    LOWER_BOUND,
    UPPER_BOUND,
    BOUND_DUMMY = MDB_INT_MAX   /* to guarantee sizeof(enum) == 4 */
} T_BOUND;

#define IS_FROM_JOIN_TABLE(t_)          \
    ((t_) == SJT_INNER_JOIN ||          \
     (t_) == SJT_LEFT_OUTER_JOIN ||     \
     (t_) == SJT_RIGHT_OUTER_JOIN ||    \
     (t_) == SJT_FULL_OUTER_JOIN)

int sql_planning(int handle, T_STATEMENT * stmt);

#endif // _SQL_PLAN_H
