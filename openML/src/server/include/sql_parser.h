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

#ifndef _SQL_PARSER_H
#define _SQL_PARSER_H

#include "mdb_config.h"
#include "sql_datast.h"
#include "ErrorCode.h"
#include "sql_util.h"

#define MAX_LINE_SIZE    1024

typedef struct __linked_list t_alloclist;
typedef struct __linked_list SELECT_STACK;

typedef struct type_yacc
{
    T_STATEMENT *stmt;          /* &gClient[i].sql */
    T_STATEMENT *root_stmt;

    T_CREATE *_create;
    T_DELETE *_delete;
    T_TRUNCATE *_truncate;
    T_DESCRIBE *_describe;
    T_DROP *_drop;
    T_RENAME *_rename;
    T_ALTER *_alter;
    T_INSERT *_insert;
    T_SELECT *_select;          /* 현재 파싱중인 select 문 */
    T_SET *_set;
    T_UPDATE *_update;
    T_ADMIN *_admin;

    T_LIST_SELECTION selection;
    T_LIST_ATTRDESC col_list;
    T_LIST_COLDESC coldesc_list;
    T_LIST_VALUE val_list;
    T_LIST_FIELDS field_list;
    T_GROUPBYDESC groupby;
    T_ORDERBYDESC orderby;
    T_LIST_ATTRDESC attr_list;
    T_ATTRDESC attr;
    ibool is_distinct;

    int userdefined;            // sqltype을 저장하기도 하고, paramcount등등.. 여러가지로 사용함
    int param_count;

    t_alloclist *pFirst, *pEnd;
    char *yyinput;
    char *yyinput_posi;
    unsigned int yyinput_len;
    unsigned int my_yylineno;
    unsigned int my_yyinput_cnt;

    char *err_pos;
    int err_len;

    int end_of_query;

    char *yerror;
    int yerrno;

    t_astime timestamp;

    /* subquery가 나타나면 현재 파싱 중이던 T_SELECT구조를 스택에 넣어두고
     * 새로운 T_SELECT구조를 할당해서 처리해야 한다.
     */
    SELECT_STACK *select_top;

    /*
     * thread-safeness를 위해 thread specific data area에
     * lexer의 buffer pointer를 가져가자.
     */
    ibool init;
    ibool is_nchar;
    ibool is_minus;
    void *scanner;
    int rownum_buf;             /* for processing rownum */

    T_SQLTYPE prev_sqltype;
} T_YACC;

/* Function prototypes */
extern int sql_parsing(int handle, T_STATEMENT * stmt);
extern int sql_validating(int handle, T_STATEMENT * stmt);
int sql_yyparse(void *);
extern void *redirect_yyinput(T_STATEMENT * stmt, char *ptr, int len);
extern __DECL_PREFIX void *current_thr(void);
extern int check_defaultvalue(T_VALUEDESC * value, DataType type, int length);
extern int check_numeric_bound(T_VALUEDESC * value, DataType type, int length);

#define Yacc            ((T_YACC*)__yacc)

#endif // _SQL_PARSER_H
