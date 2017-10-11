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

#ifndef _SQL_EXECUTE_H
#define _SQL_EXECUTE_H

#include "mdb_config.h"
#include "sql_main.h"

/* Function prototypes */
extern int make_fieldvalue(T_VALUEDESC * val, FIELDVALUE_T * fval,
        MDB_UINT8 * bound);

extern int get_minvalue(T_RESULTCOLUMN * res, T_VALUEDESC * val);
extern int get_maxvalue(T_RESULTCOLUMN * res, T_VALUEDESC * val);
extern int get_sumvalue(T_RESULTCOLUMN * res, T_VALUEDESC * val);
extern int exec_ddl(int handle, T_STATEMENT * stmt);
extern int exec_dml(int handle, T_STATEMENT * stmt, int result_type);
extern int exec_describe(int, T_STATEMENT *, int);
extern int exec_others(int handle, T_STATEMENT * stmt, int result_type);
extern int open_cursor(int handle, T_RTTABLE * rttable, T_NESTING * nest,
        EXPRDESC_LIST * expr_list, MDB_UINT8 mode);
extern int open_cursor_for_insertion(int handle, T_RTTABLE * rttable,
        MDB_UINT8 mode);
extern int reopen_cursor(int handle, T_RTTABLE * rttable, T_NESTING * nest,
        EXPRDESC_LIST * expr_list, MDB_UINT8 mode);
extern int reposition_cursor(int handle, T_RTTABLE * rttable,
        T_NESTING * nest);
extern void close_cursor(int handle, T_RTTABLE * rttable);


/* sql_result.c */
extern T_RESULT_INFO *store_result(int handle, T_STATEMENT * stmt,
        int result_type);
/* sql_select.c */
extern int fetch_cursor(int handle, T_SELECT * select, T_RESULT_INFO * result);

extern int calc_func_convert(T_VALUEDESC * base, T_VALUEDESC * src,
        T_VALUEDESC * res, MDB_INT32 is_init);
#endif // _SQL_EXECUTE_H
