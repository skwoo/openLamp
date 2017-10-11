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

#ifndef _SQL_H
#define _SQL_H

#include "mdb_config.h"

#include "mdb_dbi.h"
#include "systable.h"

#include "isql.h"

#include "sql_const.h"
#include "sql_datast.h"
#include "sql_util.h"

#include "ErrorCode.h"
#include "mdb_er.h"
#include "mdb_charset.h"

extern int SQL_prepare(int *handle, T_STATEMENT * stmt, int flag);
extern int SQL_execute(int *handle, T_STATEMENT * stmt, int flag,
        int result_type);
extern int SQL_store_result(int handle, T_STATEMENT * stmt, int flag,
        int result_type);

extern int SQL_fetch(int *handle, T_SELECT * select, T_RESULT_INFO * result);
extern int SQL_cleanup(int *handle, T_STATEMENT * stmt, int commit_mode,
        int free_memory);
extern void SQL_cleanup_memory(int handle, T_STATEMENT * stmt);

extern void SQL_cleanup_subselect(int handle, T_SELECT * select,
        T_PARSING_MEMORY * chunk_memory);

extern char *SQL_get_plan_string(int *handle, T_STATEMENT * stmt);

extern int SQL_trans_start(int handle, T_STATEMENT * stmt);
extern int SQL_trans_commit(int handle, T_STATEMENT * stmt);
extern int SQL_trans_rollback(int handle, T_STATEMENT * stmt);
extern int SQL_trans_implicit_savepoint(int handle, T_STATEMENT * stmt);
extern int SQL_trans_implicit_savepoint_release(int handle,
        T_STATEMENT * stmt);
extern int SQL_trans_implicit_partial_rollback(int handle, T_STATEMENT * stmt);

extern int SQL_errno(T_STATEMENT *);
extern char *SQL_error(T_STATEMENT *);

extern void SQL_close_all_cursors(int handle, T_STATEMENT * stmt);

extern int calc_expression(T_EXPRESSIONDESC * expr, T_VALUEDESC * val,
        MDB_INT32 is_init);
extern int calc_like(T_VALUEDESC * left, T_VALUEDESC * right,
        void **preProcessedlikeStr, T_VALUEDESC * res, int mode,
        MDB_INT32 is_init);

extern void POSTFIXELEM_CLEANUP(T_PARSING_MEMORY * memory,
        T_POSTFIXELEM * elem_);
extern void POSTFIXELEM_CLEANUP2(T_PARSING_MEMORY * memory,
        T_POSTFIXELEM * elem_);

extern int SQL_destroy_ResultSet(iSQL_ROWS * result_rows);

int check_all_constant(int argcnt, T_POSTFIXELEM ** args);
int expression_convert(int argcnt, T_POSTFIXELEM * result,
        T_EXPRESSIONDESC * expr, int *position);
int preconvert_expression(int argcnt, T_EXPRESSIONDESC * expr, int *position);
int type_convert(T_VALUEDESC * val1, T_VALUEDESC * val2);

void EXPRDESC_CLEANUP(T_PARSING_MEMORY * chunk, T_EXPRESSIONDESC * expr_);
void EXPRDESC_LIST_CLEANUP(T_PARSING_MEMORY * chunk, EXPRDESC_LIST * list_);
void EXPRDESC_LIST_FREENUL(T_PARSING_MEMORY * chunk, EXPRDESC_LIST * list_);
void ATTRED_EXPRDESC_LIST_CLEANUP(T_PARSING_MEMORY * chunk,
        ATTRED_EXPRDESC_LIST * list_);

/* internal SQL interface for other Layer interface(dbi, irs, etc. ) */
typedef struct _t_dbms_cs_execute T_DBMS_CS_EXECUTE;
extern __DECL_PREFIX int INTSQL_closetransaction(int handle, int mode);

#define MDB_ALIGN(A)    ((A)?((((int)(A)-1)>>2)<<2)+4:0)

#endif // _SQL_H
