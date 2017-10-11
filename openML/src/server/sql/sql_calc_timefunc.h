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

#ifndef _CALC_TIMEFUNC_H
#define _CALC_TIMEFUNC_H

#include "mdb_config.h"
#include "dbport.h"
#include "sql_datast.h"

extern int calc_func_current_date(T_VALUEDESC * res, MDB_INT32 is_init);
extern int calc_func_current_time(T_VALUEDESC * res, MDB_INT32 is_init);
extern int calc_func_current_timestamp(T_VALUEDESC * res, MDB_INT32 is_init);
extern int calc_func_sysdate(T_VALUEDESC * res, MDB_INT32 is_init);
extern int calc_func_date_add(T_VALUEDESC * arg1,
        T_VALUEDESC * arg2, T_VALUEDESC * arg3,
        T_VALUEDESC * res, MDB_INT32 is_init);
extern int calc_func_date_sub(T_VALUEDESC * arg1,
        T_VALUEDESC * arg2, T_VALUEDESC * arg3,
        T_VALUEDESC * res, MDB_INT32 is_init);
extern int calc_func_date_diff(T_VALUEDESC * arg1,
        T_VALUEDESC * arg2, T_VALUEDESC * arg3,
        T_VALUEDESC * res, MDB_INT32 is_init);

extern int calc_func_date_format(T_VALUEDESC * arg1,
        T_VALUEDESC * arg2, T_VALUEDESC * res, MDB_INT32 is_init);
extern int calc_func_time_add(T_VALUEDESC * arg1,
        T_VALUEDESC * arg2, T_VALUEDESC * arg3,
        T_VALUEDESC * res, MDB_INT32 is_init);
extern int calc_func_time_sub(T_VALUEDESC * arg1,
        T_VALUEDESC * arg2, T_VALUEDESC * arg3,
        T_VALUEDESC * res, MDB_INT32 is_init);
extern int calc_func_time_diff(T_VALUEDESC * arg1,
        T_VALUEDESC * arg2, T_VALUEDESC * arg3,
        T_VALUEDESC * res, MDB_INT32 is_init);

extern int calc_func_time_format(T_VALUEDESC * arg1,
        T_VALUEDESC * arg2, T_VALUEDESC * res, MDB_INT32 is_init);
extern int value2astime(T_VALUEDESC * arg /*in */ , t_astime * sec /*out */ ,
        t_astime * msec /*out */ );

#endif // _CALC_TIMEFUNC_H
