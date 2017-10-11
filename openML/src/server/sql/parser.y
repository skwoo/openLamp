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

%{

#include "mdb_config.h"
#include "dbport.h"
#include "mdb_define.h"

#include "isql.h"
#include "sql_datast.h"
#include "sql_parser.h"
#include "sql_util.h"
#include "ErrorCode.h"

#include "mdb_PMEM.h"
#include "sql_calc_timefunc.h"
#include "sql_func_timeutil.h"
#include "mdb_er.h"
#include "sql_datast.h"
#include "mdb_comm_stub.h"

#include "mdb_Server.h"

    // CC에서 에러나는 부분 때문에..
#define YYMALLOC    PMEM_ALLOC
#define YYFREE      PMEM_FREENUL

char *Yacc_Xstrdup(T_YACC *yacc, char *s);
#define YACC_XSTRDUP(a,b)     Yacc_Xstrdup(a, b)

static int ACTION_default_scalar_exp(T_YACC* yacc, T_EXPRESSIONDESC *expr);
static int ACTION_drop_statement(T_YACC* yacc, T_DROPTYPE droptype,
        char *target);
static int expandListArray(T_YACC *yacc, void **ptr, int size, int orglen);
static int expandListArraybyLimit(T_YACC *yacc,
        void **ptr, int size, int orglen, int limit);

extern int check_defaultvalue(T_VALUEDESC *value, DataType type, int length);
extern int calc_func_srandom(T_VALUEDESC *seed, T_VALUEDESC *res, int is_init);

extern void my_yy_flush_buffer(T_YACC *);
extern void my_yy_delete_buffer(void *);

extern MDB_COL_TYPE db_find_collation(char *identifier);

extern int calc_func_convert(T_VALUEDESC *base, T_VALUEDESC *src,
        T_VALUEDESC *res, MDB_INT32 is_init);

#ifdef stdin
#undef stdin
#endif
#define stdin 0

#ifdef stdout
#undef stdout
#endif
#define stdout 0

#ifdef stderr
#undef stderr
#endif
#define stderr 0

#define fprintf(a,b,c) printf(b,c)

#define vSQL(y)         ((T_STATEMENT*)((y)->stmt))
#define vCREATE(y)      ((y)->_create)
#define vALTER(y)       ((y)->_alter)
#define vDELETE(y)      ((y)->_delete)
#define vTRUNCATE(y)    ((y)->_truncate)
#define vDESCRIBE(y)    ((y)->_describe)
#define vDROP(y)        ((y)->_drop)
#define vRENAME(y)      ((y)->_rename)
#define vINSERT(y)      ((y)->_insert)
#define vSELECT(y)      ((y)->_select)
#define vSET(y)         ((y)->_set)
#define vUPDATE(y)      ((y)->_update)
#define vADMIN(y)       ((y)->_admin)

#define vPLAN(y)        (&(vSELECT(y)->planquerydesc))


static void my_yyerror(T_YACC *yacc, char *s);

static void __INCOMPLETION(T_YACC* yacc, char *t);
static void __isnull(T_YACC *yacc, void* p);
static int __CHECK_ARRAY(T_YACC* yacc, T_STATEMENT *s,
        int b, int *max, int *len, void **list);
static int __CHECK_ARRAY_BY_LIMIT(T_YACC* yacc, T_STATEMENT *s,
        T_LIST_JOINTABLEDESC *a, int b, int l);
static void ____yyerror(T_YACC *yacc, char *e);

#define INCOMPLETION(t)             \
    do {                            \
        __INCOMPLETION(Yacc, t);    \
        yyresult = RET_ERROR;       \
        goto yyreturn;              \
    } while(0)

#define __yyerror(e)                \
    do {                            \
        ____yyerror(Yacc, e);       \
        yyresult = RET_ERROR;       \
        goto yyreturn;              \
    } while(0)

#define isnull(p) __isnull(Yacc, p)

#define CHECK_ARRAY(s,a,b)                                      \
    do {                                                        \
        if (__CHECK_ARRAY(Yacc, s,b,&((a)->max), &((a)->len),   \
                    (void**)&((a)->list)) == RET_ERROR)         \
        return RET_ERROR;                                       \
} while(0)

#define CHECK_ARRAY_BY_LIMIT(s,a,b,l)                           \
    do {                                                        \
        if (__CHECK_ARRAY_BY_LIMIT(Yacc, s,a,b,l) == RET_ERROR) \
        return RET_ERROR;                                       \
} while(0)

#define SET_DEFAULT_VALUE_ERROR(Yacc) \
    ((Yacc))->yerrno = SQL_E_INVALIDDEFAULTVALUE

#include "mdb_ppthread.h"

#if defined(sun)
static pthread_once_t gOnce = {PTHREAD_ONCE_INIT};
#else
static pthread_once_t gOnce = PTHREAD_ONCE_INIT;
#endif

static int gKey;

static T_EXPRESSIONDESC *get_expr_with_item(T_YACC *yacc, T_POSTFIXELEM *item);
static char *get_remain_qstring(T_YACC *yacc);
static T_EXPRESSIONDESC *merge_expression(T_YACC *yacc, T_EXPRESSIONDESC *lexpr,
        T_EXPRESSIONDESC *rexpr, T_POSTFIXELEM *item);
static void append(T_LIST_ATTRDESC *nlist, T_ATTRDESC *attr);
static int init_limit(T_LIMITDESC *limitdesc);

// SYNTAX : LENGTH CHECK
#define MSG_TABLE_LONG      "TABLE name exceeded 63 bytes"
#define MSG_VIEW_LONG       "VIEW name exceeded 63 bytes"
#define MSG_INDEX_LONG      "INDEX name exceeded 63 bytes"
#define MSG_ALIAS_LONG      "ALIAS name exceeded 63 bytes"
#define MSG_FIELD_LONG      "FIELD name exceeded 63 bytes"
#define MSG_SEQUENCE_LONG   "SEQUENCE name exceeded 63 bytes"

#define __CHECK_TABLE_NAME_LENG(_t)                     \
    do {                                                \
        if (sc_strlen(_t) >= REL_NAME_LENG)             \
           __yyerror(MSG_TABLE_LONG);                   \
    } while(0) 
#define __CHECK_VIEW_NAME_LENG(_v)                      \
    do {                                                \
        if (sc_strlen(_v) >= REL_NAME_LENG)             \
            __yyerror(MSG_VIEW_LONG);                   \
    } while(0) 
#define __CHECK_INDEX_NAME_LENG(_i)                     \
    do {                                                \
        if (sc_strlen(_i) >= INDEX_NAME_LENG)           \
            __yyerror(MSG_INDEX_LONG);                  \
    } while(0) 
#define __CHECK_ALIAS_NAME_LENG(_a)                     \
    do {                                                \
        if (sc_strlen(_a) >= REL_NAME_LENG)             \
            __yyerror(MSG_ALIAS_LONG);                  \
    } while(0) 
#define __CHECK_FIELD_NAME_LENG(_f)                     \
    do {                                                \
        if ((_f) && sc_strlen(_f) >= FIELD_NAME_LENG)   \
            __yyerror(MSG_FIELD_LONG);                  \
    } while(0) 
#define __CHECK_SEQUENCE_NAME_LENG(_s)                  \
    do {                                                \
       if (sc_strlen(_s) >= REL_NAME_LENG)              \
           __yyerror(MSG_SEQUENCE_LONG);                \
    } while(0) 

%}

        /* symbolic tokens */

%union {
    ibool               boolean;
    MDB_UINT64          intval;
    double              floatval;
    char                *strval;

    T_OPDESC            subtok;

    T_LIST_FIELDS       lfield;
    T_IDX_LIST_FIELDS   idxfields;
    T_IDX_FIELD         idxfield;
    T_TABLEDESC         table;
    T_SCANHINT          scanhint;

    T_POSTFIXELEM       elem;
    T_EXPRESSIONDESC    *expr;

    int                 num;

    T_VALUEDESC         *item;
    struct _tag_STATEMENT *stmt;
}

%pure_parser            /* For multi-thread safe */

%{

#include "sql_y.tab.h"

#define __yylex    my_yylex
int                my_yylex(YYSTYPE *yylval_param, void *__yacc);


#ifdef YYPARSE_PARAM
#undef YYPARSE_PARAM
#endif
#define YYPARSE_PARAM   __yacc

#define YYLEX_PARAM     __yacc

%}


%token <strval>     IDENTIFIER
%token <strval>     STRINGVAL
%token <strval>     NSTRINGVAL
%token <strval>     BINARYVAL
%token <strval>     HEXADECIMALVAL

%token <intval>     INTNUM
%token <floatval>   FLOATNUM APPROXNUM

        /* operators */

%left       OR_SYM
%left       AND_SYM
%left       NOT_SYM
%left       <subtok>    COMPARISON MERGE /* = <> != < > <= >= */
%left       '+' '-'
%left       '*' '/' '%'
%right       '^'
%nonassoc   NEG
%left       <subtok>    SET_OPERATION
%left       ADD_SYM

        /* literal keyword tokens */

%token  ALL_SYM
%token  ALTER_SYM
%token  ANY_SYM
%token  AS_SYM
%token  ASC_SYM
%token  ATTACH_SYM
%token  AUTOCOMMIT_SYM
%token  AVG_SYM
%token  BETWEEN_SYM
%token  BIGINT_SYM
%token  BY_SYM
%token  BYTES_SYM
%token  NBYTES_SYM
%token  CHARACTER_SYM
%token  NCHARACTER_SYM
%token  CHECK_SYM
%token  CLOSE_SYM
%token  COMMIT_SYM
%token  FLUSH_SYM
%token  COUNT_SYM
%token  DIRTY_COUNT_SYM
%token  CONVERT_SYM
%token  CREATE_SYM
%token  CROSS_SYM
%token  CURRENT_SYM
%token  CURRENT_DATE_SYM
%token  CURRENT_TIME_SYM
%token  CURRENT_TIMESTAMP_SYM
%token  DATE_ADD_SYM
%token  DATE_DIFF_SYM
%token  DATE_FORMAT_SYM
%token  DATE_SUB_SYM
%token  TIME_ADD_SYM
%token  TIME_DIFF_SYM
%token  TIME_FORMAT_SYM
%token  TIME_SUB_SYM
%token  DATE_SYM
%token  DATETIME_SYM
%token  DECIMAL_SYM
%token  DECODE_SYM
%token  DEFAULT_SYM
%token  DELETE_SYM
%token  DESC_SYM
%token  DESCRIBE_SYM
%token  DISTINCT_SYM
%token  DOUBLE_SYM
%token  DROP_SYM
%token  ESCAPE_SYM
%token  EXISTS_SYM
%token  EXPLAIN_SYM
%token  FEEDBACK_SYM
%token  FLOAT_SYM
%token  FOREIGN_SYM
%token  FROM_SYM
%token  FULL_SYM
%token  GROUP_SYM
%token  HAVING_SYM
%token  HEADING_SYM
%token  HEXCOMP_SYM
%token  CHARTOHEX_SYM
%token  IFNULL_SYM
%token  IN_SYM
%token  INDEX_SYM
%token  INNER_SYM
%token  INSERT_SYM
%token  INT_SYM
%token  INTO_SYM
%token  IS_SYM
%token  ISCAN_SYM
%token  JOIN_SYM
%token  KEY_SYM
%token  LEFT_SYM
%token  LIKE_SYM
%token  ILIKE_SYM

%token  LIMIT_SYM
%token  LOWERCASE_SYM
%token  LTRIM_SYM
%token  MAX_SYM
%token  MIN_SYM
%token  LOGGING_SYM
%token  NOLOGGING_SYM
%token  NOW_SYM
%token  NULL_SYM
%token  NUMERIC_SYM
%token  OFF_SYM
%token  ON_SYM
%token  ONLY_SYM
%token  OPTION_SYM
%token  ORDER_SYM
%token  OUTER_SYM
%token  PLAN_SYM
%token  PRECISION
%token  PRIMARY_SYM
%token  REAL_SYM
%token  RECONNECT_SYM
%token  REFERENCES_SYM
%token  RENAME_SYM
%token  RIGHT_SYM
%token  ROLLBACK_SYM
%token  ROUND_SYM
%token  RTRIM_SYM
%token  SELECT_SYM
%token  SET_SYM
%token  SIGN_SYM
%token  SMALLINT_SYM
%token  SOME_SYM
%token  SSCAN_SYM
%token  STRING_SYM
%token  SUBSTRING_SYM
%token  SUM_SYM
%token  SYSDATE_SYM
%token  TABLE_SYM
%token  RIDTABLENAME_SYM
%token  TIME_SYM
%token  TIMESTAMP_SYM
%token  TINYINT_SYM
%token  TRUNC_SYM
%token  TRUNCATE_SYM
%token  UNIQUE_SYM
%token  UPDATE_SYM
%token  UPPERCASE_SYM
%token  VALUES_SYM
%token  VARCHAR_SYM
%token  NVARCHAR_SYM
%token  VIEW_SYM
%token  WHERE_SYM
%token  WORK_SYM
%token  RANDOM_SYM
%token  SRANDOM_SYM

%token  END_SYM
%token  END_OF_INPUT
%token  USING_SYM
%token  NSTRING_SYM

%token  ROWNUM_SYM

%token  SEQUENCE_SYM
%token  START_SYM
%token  AT_SYM
%token  WITH_SYM
%token  INCREMENT_SYM
%token  MAXVALUE_SYM
%token  NOMAXVALUE_SYM
%token  MINVALUE_SYM
%token  NOMINVALUE_SYM
%token  CYCLE_SYM
%token  NOCYCLE_SYM
%token  CURRVAL_SYM
%token  NEXTVAL_SYM

%token  RID_SYM

%token  REPLACE_SYM

%token  BYTE_SYM
%token  VARBYTE_SYM
%token  BINARY_SYM
%token  HEXADECIMAL_SYM
%token  BYTE_SIZE_SYM
%token  COPYFROM_SYM
%token  COPYTO_SYM

%token  OBJECTSIZE_SYM
%token  TABLEDATA_SYM
%token  TABLEINDEX_SYM

%token  COLLATION_SYM

%token  REBUILD_SYM

%token  EXPORT_SYM
%token  IMPORT_SYM

%token  HEXASTRING_SYM
%token  BINARYSTRING_SYM

%token  RUNTIME_SYM

%token  SHOW_SYM

%token  UPSERT_SYM

%token  DATA_SYM
%token  SCHEMA_SYM

%token  OID_SYM

%token  BEFORE_SYM
%token  ENABLE_SYM
%token  DISABLE_SYM

%token  RESIDENT_SYM

%token  AUTOINCREMENT_SYM

%token  BLOB_SYM

%token  INCLUDE_SYM

%token  COMPILE_OPT_SYM

%token  MSYNC_SYM
%token  SYNCED_RECORD_SYM
%token  INSERT_RECORD_SYM
%token  UPDATE_RECORD_SYM
%token  DELETE_RECORD_SYM

%type <boolean>     on_or_off
%type <boolean>     table_type_def
%type <boolean>     logging_clause

%type <intval>      any_all_some

%type <elem>        atom
%type <elem>        literal
%type <elem>        parameter_marker
%type <elem>        special_function_arg

%type <expr>        aggregation_ref
%type <expr>        aggregation_ref_arg
%type <expr>        all_or_any_predicate
%type <expr>        atom_commalist
%type <expr>        between_predicate
%type <expr>        comparison_predicate
%type <expr>        existence_test
%type <expr>        function_ref
%type <expr>        general_function_ref
%type <expr>        in_predicate
%type <expr>        like_predicate
%type <expr>        numeric_function_ref
%type <expr>        opt_having_clause
%type <expr>        predicate
%type <expr>        scalar_exp
%type <expr>        column_ref_or_pvar
%type <expr>        search_and_result
%type <expr>        search_and_result_commalist
%type <expr>        search_condition
%type <expr>        special_function_predicate
%type <expr>        special_function_ref
%type <expr>        string_function_ref
%type <expr>        test_for_null
%type <expr>        time_function_ref
%type <expr>        where_clause
%type <expr>        subquery_exp
%type <expr>        random_function_ref
%type <expr>        rownum_ref
%type <expr>        object_function_ref
%type <expr>        byte_function_ref
%type <expr>        objectsize_function_ref
%type <expr>        objecttype

%type <expr>        rebuild_definition
%type <expr>        rebuild_object_definition

%type <idxfields>   index_element_commalist
%type <idxfield>    index_column

%type <strval>      column
%type <strval>      column_ref

%type <strval>      rid_column_ref

%type <strval>      index
%type <strval>      opt_describe_column
%type <strval>      opt_selection_item_alias
%type <strval>      table_alias_name
%type <strval>      table
%type <strval>      columnalias_item

%type <table>       table_ref
%type <scanhint>    opt_scan_hint
/* %type <lfield>      iscan_hint_column_commalist */
%type <idxfields>      iscan_hint_column_commalist

%type <intval>      fixedlen_intnum
%type <strval>      copyfrom_type
%type <intval>      export_op
%type <strval>      export_all_or_table
%type <intval>      export_data_or_schema
%type <strval>      show_all_or_table

%type <num>         order_info
%type <num>         order_info_str
%type <num>         collation_info
%type <num>         collation_info_str

%type <boolean>     enable_or_disable

%type <boolean>     is_autoincrement

%type <num>         minus_symbol

%type <strval>      view
%type <strval>      sequence

%type <boolean>     set_opt_all
%type <expr>        set_operation_exp
%type <expr>        set_query_body
%type <expr>        set_query_exp

%type <num>         like_op

%type <boolean>     enable_or_disable_or_flush
%type <boolean>     msynced_opt
%type <num>         fetch_record_type

%start query
%%

query:    /* Root */
    END_OF_INPUT
    {}
    | sql END_OF_INPUT
    {
        /* return RET_SUCCESS; */
    }
    | query sql END_OF_INPUT
    {
        /* return RET_SUCCESS; */
    }
    ;

sql:
    definitive_statement
    | manipulative_statement
    | private_statement
    ;

definitive_statement:
    create_statement
    | alter_statement
    | drop_statement
    | rename_statement
    ;

manipulative_statement:
    commit_statement
    | delete_statement
    | insert_statement
    | upsert_statement    
    | rollback_statement
    | select_statement
    | update_statement
    | truncate_statement
    ;

private_statement:
    desc_statement
    | set_statement
    | admin_statement
    ;
;

create_statement:
    CREATE_SYM
    {
        vSQL(Yacc)->sqltype = ST_CREATE;
        vCREATE(Yacc) = &vSQL(Yacc)->u._create;
        Yacc->userdefined = 0;
        vCREATE(Yacc)->type = SCT_DUMMY;
    }
    table_type_def TABLE_SYM table table_elements max_records as_query_spec
    {
        if (vCREATE(Yacc)->type == SCT_TABLE_ELEMENTS)
        {
            T_CREATETABLE  *elements;

            elements = &vCREATE(Yacc)->u.table_elements;

            elements->table_type = (int)($3);

            if (elements->table.max_records < 0)
            {
                __yyerror("LIMIT BY is out range.");
            }

            elements->table.tablename = $5;
            elements->table.aliasname = NULL;

            for (i = 0; i < Yacc->attr_list.len; i++) {
                p = YACC_XSTRDUP(Yacc, $5);
                Yacc->attr_list.list[i].table.tablename = p;
                Yacc->attr_list.list[i].table.aliasname = NULL;
                isnull(Yacc->attr_list.list[i].table.tablename);
            }

            elements->col_list = Yacc->attr_list;
            elements->fields   = Yacc->field_list;
            sc_memset(&(Yacc->field_list), 0, sizeof(T_LIST_FIELDS));
            sc_memset(&(Yacc->attr_list), 0, sizeof(T_LIST_ATTRDESC));
        }
        else
        {
            T_CREATETABLE_QUERY *query;

            query = &vCREATE(Yacc)->u.table_query;

            if ($3 == 7)
            {
                __yyerror("ERROR");
            }
            query->table_type = (int)($3);

            if (query->table.max_records < 0)
            {
                __yyerror("LIMIT BY is out range.");
            }

            query->table.tablename = $5;
            query->table.aliasname = NULL;
        }
    }
    | CREATE_SYM
    {
        vSQL(Yacc)->sqltype = ST_CREATE;
        vCREATE(Yacc) = &vSQL(Yacc)->u._create;
        sc_memset(&yylval.lfield, 0, sizeof(T_LIST_FIELDS));
        Yacc->userdefined = 0;
    }
    index_opt_unique INDEX_SYM index ON_SYM table '(' index_element_commalist ')'
    {
        vCREATE(Yacc)->type = SCT_INDEX;
        vCREATE(Yacc)->u.index.table.tablename = $7;
        vCREATE(Yacc)->u.index.table.aliasname = NULL;
        vCREATE(Yacc)->u.index.indexname       = $5;

        vCREATE(Yacc)->u.index.fields = $9;
        sc_memset(&(Yacc->field_list), 0, sizeof(T_LIST_FIELDS));
    }
    | CREATE_SYM
    {
        vSQL(Yacc)->sqltype = ST_CREATE;
        vCREATE(Yacc) = &vSQL(Yacc)->u._create;
        Yacc->_select = NULL;
        Yacc->userdefined = 0;
    }
    VIEW_SYM view opt_column_commalist
    {
        vCREATE(Yacc)->type = SCT_VIEW;
        vCREATE(Yacc)->u.view.name = $4;
        vCREATE(Yacc)->u.view.columns = Yacc->col_list;
        sc_memset(&(Yacc->col_list), 0, sizeof(T_LIST_ATTRDESC));
    }
    as_query_spec
    | CREATE_SYM
    {
        vSQL(Yacc)->sqltype = ST_CREATE;
        vCREATE(Yacc) = &vSQL(Yacc)->u._create;
        Yacc->_select = NULL;
        Yacc->userdefined = 0;
    }
    SEQUENCE_SYM sequence
    {
        vCREATE(Yacc)->type = SCT_SEQUENCE;
        vCREATE(Yacc)->u.sequence.name = $4;
    }
    SEQUENCE_DESCRIPTION_CREATE
    ;

SEQUENCE_DESCRIPTION_CREATE:
    /* Empty */
    | SEQUENCE_DESC_START
    SEQUENCE_DESC_INCREMENT 
    SEQUENCE_DESC_MAXVALUE 
    SEQUENCE_DESC_MINVALUE 
    SEQUENCE_DESC_CYCLE 
    ;

SEQUENCE_DESCRIPTION_ALTER:
    /* Empty */
    {
        vALTER(Yacc)->u.sequence.bStart = DB_FALSE;
    }
    | SEQUENCE_DESC_INCREMENT 
    SEQUENCE_DESC_MAXVALUE 
    SEQUENCE_DESC_MINVALUE 
    SEQUENCE_DESC_CYCLE 
    {
        vALTER(Yacc)->u.sequence.bStart = DB_FALSE;
    }
    ;

SEQUENCE_DESC_START:
      /* Empty */
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE)
            vCREATE(Yacc)->u.sequence.bStart= DB_FALSE;
        else if (vSQL(Yacc)->sqltype == ST_ALTER)
            vALTER(Yacc)->u.sequence.bStart = DB_FALSE;
    }
    | START_SYM WITH_SYM minus_symbol INTNUM 
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE)   {
            vCREATE(Yacc)->u.sequence.start = (MDB_INT32)($4 * $3);
            vCREATE(Yacc)->u.sequence.bStart= DB_TRUE;
        } else if (vSQL(Yacc)->sqltype == ST_ALTER) {
            vALTER(Yacc)->u.sequence.start  = (MDB_INT32)($4 * $3);
            vALTER(Yacc)->u.sequence.bStart = DB_TRUE;
        }
    }
    ;

SEQUENCE_DESC_INCREMENT:
    /* Empty */
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE)
            vCREATE(Yacc)->u.sequence.bIncrement= DB_FALSE;
        else if (vSQL(Yacc)->sqltype == ST_ALTER)
            vALTER(Yacc)->u.sequence.bIncrement = DB_FALSE;
    }
    | INCREMENT_SYM BY_SYM minus_symbol INTNUM
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE)   {
            vCREATE(Yacc)->u.sequence.increment = (MDB_INT32)($4 * $3);
            vCREATE(Yacc)->u.sequence.bIncrement= DB_TRUE;
        } else if (vSQL(Yacc)->sqltype == ST_ALTER) {
            vALTER(Yacc)->u.sequence.increment  = (MDB_INT32)($4 * $3);
            vALTER(Yacc)->u.sequence.bIncrement = DB_TRUE;
        }
    }
    ;

SEQUENCE_DESC_MAXVALUE:
    /* Empty */
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE) 
            vCREATE(Yacc)->u.sequence.bMaxValue = DB_FALSE;
        else if (vSQL(Yacc)->sqltype == ST_ALTER)
            vALTER(Yacc)->u.sequence.bMaxValue  = DB_FALSE;
    }
    | MAXVALUE_SYM minus_symbol INTNUM
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE)   {
            vCREATE(Yacc)->u.sequence.maxValue  = (MDB_INT32)($3 * $2);
            vCREATE(Yacc)->u.sequence.bMaxValue = DB_TRUE;
        } else if (vSQL(Yacc)->sqltype == ST_ALTER) {
            vALTER(Yacc)->u.sequence.maxValue   = (MDB_INT32)($3 * $2);
            vALTER(Yacc)->u.sequence.bMaxValue  = DB_TRUE;
        }
    }
    | NOMAXVALUE_SYM
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE) 
            vCREATE(Yacc)->u.sequence.bMaxValue = DB_FALSE;
        else if (vSQL(Yacc)->sqltype == ST_ALTER)
            vALTER(Yacc)->u.sequence.bMaxValue  = DB_FALSE;
    }
    ;

SEQUENCE_DESC_MINVALUE:
    /* Empty */
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE) 
            vCREATE(Yacc)->u.sequence.bMinValue = DB_FALSE;
        else if (vSQL(Yacc)->sqltype == ST_ALTER)
            vALTER(Yacc)->u.sequence.bMinValue  = DB_FALSE;
    }
    | MINVALUE_SYM minus_symbol INTNUM
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE) {
            vCREATE(Yacc)->u.sequence.minValue = (MDB_INT32)($3 * $2);
            vCREATE(Yacc)->u.sequence.bMinValue = DB_TRUE;
        } else if (vSQL(Yacc)->sqltype == ST_ALTER) {
            vALTER(Yacc)->u.sequence.minValue = (MDB_INT32)($3 * $2);
            vALTER(Yacc)->u.sequence.bMinValue  = DB_TRUE;
        }
    }
    | NOMINVALUE_SYM 
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE) 
            vCREATE(Yacc)->u.sequence.bMinValue = DB_FALSE;
        else if (vSQL(Yacc)->sqltype == ST_ALTER)
            vALTER(Yacc)->u.sequence.bMinValue  = DB_FALSE;
    }
    ;

SEQUENCE_DESC_CYCLE :
    /* Empty */
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE) 
            vCREATE(Yacc)->u.sequence.cycled= DB_FALSE;
        else if (vSQL(Yacc)->sqltype == ST_ALTER)
            vALTER(Yacc)->u.sequence.cycled = DB_FALSE;
    }
    | CYCLE_SYM
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE) 
            vCREATE(Yacc)->u.sequence.cycled= DB_TRUE;
        else if (vSQL(Yacc)->sqltype == ST_ALTER)
            vALTER(Yacc)->u.sequence.cycled = DB_TRUE;
    }
    | NOCYCLE_SYM
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE) 
            vCREATE(Yacc)->u.sequence.cycled= DB_FALSE;
        else if (vSQL(Yacc)->sqltype == ST_ALTER)
            vALTER(Yacc)->u.sequence.cycled = DB_FALSE;
    }
    ;

max_records:
    {
        if (vCREATE(Yacc)->type == SCT_TABLE_ELEMENTS)
        {
            vCREATE(Yacc)->u.table_elements.table.max_records = 0;
            vCREATE(Yacc)->u.table_elements.table.column_name = NULL;
        }
        else
        {
            vCREATE(Yacc)->u.table_query.table.max_records = 0;
            vCREATE(Yacc)->u.table_query.table.column_name = NULL;
        }
    }
    | LIMIT_SYM BY_SYM INTNUM USING_SYM column
    {
        if ($3 < 1 || $3 > MDB_INT_MAX)
        {
            if (vCREATE(Yacc)->type == SCT_TABLE_ELEMENTS)
            {
                vCREATE(Yacc)->u.table_elements.table.max_records = -1;
                vCREATE(Yacc)->u.table_elements.table.column_name = NULL;
            }
            else
            {
                vCREATE(Yacc)->u.table_query.table.max_records = -1;
                vCREATE(Yacc)->u.table_query.table.column_name = NULL;
            }
        }
        else
        {
            if (vCREATE(Yacc)->type == SCT_TABLE_ELEMENTS)
            {
                vCREATE(Yacc)->u.table_elements.table.max_records = (int)$3;
                vCREATE(Yacc)->u.table_elements.table.column_name = $5;
            }
            else
            {
                vCREATE(Yacc)->u.table_query.table.max_records = (int)$3;
                vCREATE(Yacc)->u.table_query.table.column_name = $5;
            }
        }
    }
    | LIMIT_SYM BY_SYM INTNUM
    {    
        if ($3 < 1 || $3 > MDB_INT_MAX)
        {
            if (vCREATE(Yacc)->type == SCT_TABLE_ELEMENTS)
            {
                vCREATE(Yacc)->u.table_elements.table.max_records = -1;
                vCREATE(Yacc)->u.table_elements.table.column_name = NULL;
            }
            else
            {
                vCREATE(Yacc)->u.table_query.table.max_records = -1;
                vCREATE(Yacc)->u.table_query.table.column_name = NULL;
            }
        }
        else
        {
            if (vCREATE(Yacc)->type == SCT_TABLE_ELEMENTS)
            {
                vCREATE(Yacc)->u.table_elements.table.max_records = (int)$3;
                vCREATE(Yacc)->u.table_elements.table.column_name =
                    YACC_XSTRDUP(Yacc, "#");
                isnull(vCREATE(Yacc)->u.table_elements.table.column_name);
            }
            else
            {
                vCREATE(Yacc)->u.table_query.table.max_records = (int)$3;
                vCREATE(Yacc)->u.table_query.table.column_name =
                    YACC_XSTRDUP(Yacc, "#");
                isnull(vCREATE(Yacc)->u.table_query.table.column_name);
            }
        }
    }
;

enable_or_disable:
    ENABLE_SYM
    {
        $$ = 1;
    }
    | DISABLE_SYM
    {
        $$ = 0;
    }
    ;
    
table_type_def:
    /* empty */
    {
        $$ = 0;
    }
    | NOLOGGING_SYM
    {
        $$ = 2;
    }
    | RESIDENT_SYM
    {
        $$ = 6;
    }
    | MSYNC_SYM
    {
        $$ = 7;
    }
    ;

logging_clause:
    LOGGING_SYM
    {
        $$ = 0;
    }
    | NOLOGGING_SYM
    {
        $$ = 2;
    }
    ;

table_elements:
    /* empty */
    | '(' base_table_element_commalist ')'
    {
        vCREATE(Yacc)->type = SCT_TABLE_ELEMENTS;
    }
    ;

as_query_spec:
    /* empty */
    {
        if (vCREATE(Yacc)->type == SCT_VIEW)
        {
            __yyerror("syntax error");
        }
    }
    | AS_SYM
    {
        if (vCREATE(Yacc)->type == SCT_VIEW)
        {
            vCREATE(Yacc)->u.view.qstring =
                sql_mem_strdup(vSQL(Yacc)->parsing_memory,
                        get_remain_qstring(Yacc));
            isnull(vCREATE(Yacc)->u.view.qstring);
        }
    }
    select_body
    {
        if (vCREATE(Yacc)->type != SCT_VIEW)
        {
            if (vCREATE(Yacc)->type == SCT_TABLE_ELEMENTS)
                __yyerror("ERROR");
            vCREATE(Yacc)->type = SCT_TABLE_QUERY;
        }
    }
;

base_table_element_commalist:
    base_table_element
    | base_table_element_commalist ',' base_table_element
    ;

base_table_element:
    column_def
    {
        CHECK_ARRAY(Yacc->stmt, &(Yacc->attr_list), sizeof(T_ATTRDESC));
        append(&(Yacc->attr_list), &(Yacc->attr));
        sc_memset(&(Yacc->attr), 0, sizeof(T_ATTRDESC));
    }
    | table_constraint_def
    ;

column_def:
    column data_type
    {
        Yacc->attr.attrname         = $1;
        Yacc->attr.flag             = NULL_BIT;
        Yacc->attr.defvalue.defined = 0;
        Yacc->userdefined           = 0;
    }
    column_def_opt_list
    {
        if ((Yacc->attr.flag & AUTO_BIT) && Yacc->attr.defvalue.defined)
            __yyerror("AUTOINCREMENT not allow DEFAULT value");
        if ((Yacc->attr.flag & AUTO_BIT) &&
                (Yacc->attr.type != DT_INTEGER))
            __yyerror("AUTOINCREMENT allowed only INTEGER PRIMARY KEY");
    }
    ;

data_type:
    BIGINT_SYM
    {
        Yacc->attr.type        = DT_BIGINT;
        Yacc->attr.len        = sizeof(bigint);
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec        = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | BYTES_SYM '(' INTNUM ')'
    {
        Yacc->attr.type     = DT_CHAR;
        Yacc->attr.len      = (int)$3;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = DB_Params.default_col_char;
    }
    | NBYTES_SYM '(' INTNUM ')'
    {
        Yacc->attr.type     = DT_NCHAR;
        Yacc->attr.len      = (int)$3;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = DB_Params.default_col_nchar;
    }
    | CHARACTER_SYM collation_info
    {
        Yacc->attr.type     = DT_CHAR;
        Yacc->attr.len      = 1;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        if ($2 == MDB_COL_NONE)
        {
            Yacc->attr.collation = DB_Params.default_col_char;
        }
        else
        {
            Yacc->attr.collation= $2;
        }
    }
    | CHARACTER_SYM '(' INTNUM ')' collation_info
    {
        Yacc->attr.type     = DT_CHAR;
        Yacc->attr.len      = (int)$3;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        if ($5 == MDB_COL_NONE)
        {
            Yacc->attr.collation = DB_Params.default_col_char;
        }
        else
        {
            Yacc->attr.collation= $5;
        }
    }
    | NCHARACTER_SYM collation_info
    {
        Yacc->attr.type     = DT_NCHAR;
        Yacc->attr.len      = 1;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        if ($2 == MDB_COL_NONE)
        {
            Yacc->attr.collation = DB_Params.default_col_nchar;
        }
        else
        {
            Yacc->attr.collation= $2;
        }
    }
    | NCHARACTER_SYM '(' INTNUM ')' collation_info
    {
        Yacc->attr.type     = DT_NCHAR;
        Yacc->attr.len      = (int)$3;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        if ($5 == MDB_COL_NONE)
        {
            Yacc->attr.collation = DB_Params.default_col_nchar;
        }
        else
        {
            Yacc->attr.collation= $5;
        }
    }
    | DATETIME_SYM
    {
        Yacc->attr.type     = DT_DATETIME;
        Yacc->attr.len      = MAX_DATETIME_FIELD_LEN;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | DATE_SYM
    {
        Yacc->attr.type     = DT_DATE;
        Yacc->attr.len      = MAX_DATE_FIELD_LEN;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | TIME_SYM
    {
        Yacc->attr.type     = DT_TIME;
        Yacc->attr.len      = MAX_TIME_FIELD_LEN;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | DECIMAL_SYM
    {
        Yacc->attr.type     = DT_DECIMAL;
        Yacc->attr.len      = DEFAULT_DECIMAL_SIZE;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | DECIMAL_SYM '(' INTNUM ')'
    {
        Yacc->attr.type     = DT_DECIMAL;
        if ($3 > MAX_DECIMAL_PRECISION)
            __yyerror("numeric precision specifier is out of range");
        Yacc->attr.len         = (int)$3;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec         = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | DECIMAL_SYM '(' INTNUM ',' INTNUM ')'
    {
        Yacc->attr.type     = DT_DECIMAL;
        if ($3 > MAX_DECIMAL_PRECISION)
            __yyerror("numeric precision specifier is out of range");
        if ($3 < $5) __yyerror("scale larger than specified precision");

        Yacc->attr.len         = (int)$3;
        Yacc->attr.fixedlen    = -1;
        if ($5 < MIN_DECIMAL_SCALE) Yacc->attr.dec = MIN_DECIMAL_SCALE;
        else if ($5 > MAX_DECIMAL_SCALE)
            Yacc->attr.dec = MAX_DECIMAL_SCALE;
        else Yacc->attr.dec = (int)$5;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | DOUBLE_SYM
    {
        Yacc->attr.type     = DT_DOUBLE;
        Yacc->attr.len      = sizeof(double);
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | DOUBLE_SYM PRECISION
    {
        Yacc->attr.type     = DT_DOUBLE;
        Yacc->attr.len      = sizeof(double);
        Yacc->attr.fixedlen    = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | FLOAT_SYM
    {
        Yacc->attr.type     = DT_FLOAT;
        Yacc->attr.len      = sizeof(float);
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | FLOAT_SYM '(' INTNUM ')'
    {
        Yacc->attr.type     = DT_FLOAT;
        Yacc->attr.len      = sizeof(float);
        Yacc->attr.fixedlen    = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | INT_SYM
    {
        Yacc->attr.type     = DT_INTEGER;
        Yacc->attr.len      = sizeof(int);
        Yacc->attr.fixedlen    = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | OID_SYM
    {
        /* OID convert */
        Yacc->attr.type     = DT_OID;
        Yacc->attr.len      = sizeof(OID);
        Yacc->attr.fixedlen    = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | NUMERIC_SYM
    {
        Yacc->attr.type     = DT_DECIMAL;
        Yacc->attr.len      = DEFAULT_DECIMAL_SIZE;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | NUMERIC_SYM '(' INTNUM ')'
    {
        Yacc->attr.type     = DT_DECIMAL;
        if ($3 > MAX_DECIMAL_PRECISION)
            __yyerror("numeric precision specifier is out of range");
        Yacc->attr.len         = (int)$3;
        Yacc->attr.fixedlen    = -1;
        Yacc->attr.dec         = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | NUMERIC_SYM '(' INTNUM ',' INTNUM ')'
    {
        Yacc->attr.type     = DT_DECIMAL;
        if ($3 > MAX_DECIMAL_PRECISION)
            __yyerror("numeric precision specifier is out of range");
        if ($3 < $5) __yyerror("scale larger than specified precision");

        Yacc->attr.len         = (int)$3;
        Yacc->attr.fixedlen    = -1;
        if ($5 < MIN_DECIMAL_SCALE) Yacc->attr.dec = MIN_DECIMAL_SCALE;
        else if ($5 > MAX_DECIMAL_SCALE)
            Yacc->attr.dec = MAX_DECIMAL_SCALE;
        else Yacc->attr.dec = (int)$5;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | REAL_SYM
    {
        Yacc->attr.type     = DT_DOUBLE;
        Yacc->attr.len      = sizeof(double);
        Yacc->attr.fixedlen    = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | SMALLINT_SYM
    {
        Yacc->attr.type     = DT_SMALLINT;
        Yacc->attr.len      = sizeof(smallint);
        Yacc->attr.fixedlen    = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | STRING_SYM '(' INTNUM ')'
    {
        Yacc->attr.type     = DT_VARCHAR;
        Yacc->attr.len      = (int)$3;
        Yacc->attr.fixedlen = 0;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = DB_Params.default_col_char;
    }
    | STRING_SYM '(' fixedlen_intnum ',' INTNUM ')'
    {
        Yacc->attr.type     = DT_VARCHAR;
        Yacc->attr.len      = (int)$5;
        Yacc->attr.fixedlen = (int)$3;
        if (Yacc->attr.len < Yacc->attr.fixedlen)
            __yyerror("fixed length of varchar is out of range");
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = DB_Params.default_col_char;
    }
    | TIMESTAMP_SYM
    {
        Yacc->attr.type     = DT_TIMESTAMP;
        Yacc->attr.len      = MAX_TIMESTAMP_FIELD_LEN;
        Yacc->attr.fixedlen    = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | TINYINT_SYM
    {
        Yacc->attr.type     = DT_TINYINT;
        Yacc->attr.len      = sizeof(tinyint);
        Yacc->attr.fixedlen    = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    | VARCHAR_SYM '(' INTNUM ')' collation_info
    {
        Yacc->attr.type     = DT_VARCHAR;
        Yacc->attr.len      = (int)$3;
        Yacc->attr.fixedlen = 0;
        Yacc->attr.dec      = 0;
        if ($5 == MDB_COL_NONE)
        {
            Yacc->attr.collation = DB_Params.default_col_char;
        }
        else
        {
            Yacc->attr.collation= $5;
        }
    }
    | NVARCHAR_SYM '(' INTNUM ')' collation_info
    {
        Yacc->attr.type     = DT_NVARCHAR;
        Yacc->attr.len      = (int)$3;
        Yacc->attr.fixedlen = 0;
        Yacc->attr.dec      = 0;
        if ($5 == MDB_COL_NONE)
        {
            Yacc->attr.collation = DB_Params.default_col_nchar;
        }
        else
        {
            Yacc->attr.collation= $5;
        }
    }
    | VARCHAR_SYM '(' fixedlen_intnum ',' INTNUM ')' collation_info
    {
        Yacc->attr.type     = DT_VARCHAR;
        Yacc->attr.len      = (int)$5;
        Yacc->attr.fixedlen = (int)$3;
        if (Yacc->attr.len < Yacc->attr.fixedlen)
            __yyerror("fixed length of varchar is out of range");
        Yacc->attr.dec      = 0;
        if ($7 == MDB_COL_NONE)
        {
            Yacc->attr.collation = DB_Params.default_col_char;
        }
        else
        {
            Yacc->attr.collation= $7;
        }
    }
    | NVARCHAR_SYM '(' fixedlen_intnum ',' INTNUM ')' collation_info
    {
        Yacc->attr.type     = DT_NVARCHAR;
        Yacc->attr.len      = (int)$5;
        Yacc->attr.fixedlen = (int)$3;
        if (Yacc->attr.len < Yacc->attr.fixedlen)
            __yyerror("fixed length of varchar is out of range");
        Yacc->attr.dec      = 0;
        if ($7 == MDB_COL_NONE)
        {
            Yacc->attr.collation = DB_Params.default_col_nchar;
        }
        else
        {
            Yacc->attr.collation= $7;
        }
    }
    | BYTE_SYM
    {
        Yacc->attr.type     = DT_BYTE;
        Yacc->attr.len      = 1;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_DEFAULT_BYTE;
    }
    | BYTE_SYM '(' INTNUM ')'
    {
        Yacc->attr.type     = DT_BYTE;
        Yacc->attr.len      = (int)$3;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_DEFAULT_BYTE;
    }
    | VARBYTE_SYM '(' INTNUM ')'
    {
        Yacc->attr.type     = DT_VARBYTE;
        Yacc->attr.len      = (int)$3;
        Yacc->attr.fixedlen = 0;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_DEFAULT_BYTE;
    }
    | VARBYTE_SYM '(' fixedlen_intnum ',' INTNUM ')'
    {
        Yacc->attr.type     = DT_VARBYTE;
        Yacc->attr.len      = (int)$5;
        Yacc->attr.fixedlen = (int)$3;
        if (Yacc->attr.len < Yacc->attr.fixedlen)
            __yyerror("fixed length of varchar is out of range");
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_DEFAULT_BYTE;
    }
    | BLOB_SYM
    {
        Yacc->attr.type     = DT_VARBYTE;
        Yacc->attr.len      = 8000;
        Yacc->attr.fixedlen = 0;
        if (Yacc->attr.len < Yacc->attr.fixedlen)
            __yyerror("fixed length of varchar is out of range");
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_DEFAULT_BYTE;
    }
    ;

fixedlen_intnum:
    INTNUM
    {
        $$ = $1;
    }
    | '-' INTNUM
    {
        if ($2 != 1)
            __yyerror("fixed length of varchar is out of range");
        $$ = -1;
    }
    ;
       

column_def_opt_list:
    /* empty */
    | column_def_opt_list column_def_opt
    {
        ;
    }
    ;

column_def_opt:
    NULL_SYM
    {
        if (Yacc->userdefined & 0x02 || Yacc->attr.flag&PRI_KEY_BIT)
            __yyerror("ERROR");
        else {
            Yacc->userdefined |= 0x01;
            Yacc->attr.flag   |= EXPLICIT_NULL_BIT;
        }
    }
    | NOT_SYM NULL_SYM
    {
        if (Yacc->userdefined & 0x01) __yyerror("ERROR");
        else {
            Yacc->attr.flag   &= ~NULL_BIT;
            Yacc->userdefined |= 0x02;
        }
    }
    | UNIQUE_SYM
    {
        INCOMPLETION("UNIQUE");
    }
    | PRIMARY_SYM KEY_SYM is_autoincrement
    {
        if (Yacc->field_list.list) __yyerror("ERROR");
        if (Yacc->userdefined & 0x01) __yyerror("ERROR");
        if ($3)
            Yacc->attr.flag |= AUTO_BIT;
        Yacc->attr.flag |= PRI_KEY_BIT; // indicate a key field.
        Yacc->attr.flag &= ~NULL_BIT; // remove the NULL-allowed bit.

        p = YACC_XSTRDUP(Yacc, Yacc->attr.attrname);
        isnull(p);

        CHECK_ARRAY(Yacc->stmt, &(Yacc->field_list), sizeof(char*));
        Yacc->field_list.list[Yacc->field_list.len++] = p;
    }
    | DEFAULT_SYM scalar_exp
    {
        ret = ACTION_default_scalar_exp(Yacc, $2);
        if (ret < 0)
            return ret;
    }
    | CHECK_SYM '(' search_condition ')'
    {
        INCOMPLETION("CHECK");
    }
    | REFERENCES_SYM table
    {
        INCOMPLETION("REFERENCES");
    }
    | REFERENCES_SYM table '(' column_commalist ')'
    {
        INCOMPLETION("REFERENCES");
    }
    | NOLOGGING_SYM
    {
        if (IS_VARIABLE(Yacc->attr.type))
        {
            __yyerror("Invalid NOLOGGING column.");
        }

        Yacc->attr.flag   |= NOLOGGING_BIT;
    }
    ;

is_autoincrement:
    /* empty */
    {
        $$ = 0;
    }
    | AUTOINCREMENT_SYM
    {
        $$ = 1;
    }

table_constraint_def:
    UNIQUE_SYM '(' key_column_commalist ')'
    {
        INCOMPLETION("UNIQUE");
    }
    | PRIMARY_SYM KEY_SYM
    {
        if (Yacc->field_list.list) __yyerror("ERROR");
    }
    '(' key_column_commalist ')'
    | FOREIGN_SYM KEY_SYM '(' key_column_commalist ')' REFERENCES_SYM table
    {
        INCOMPLETION("FOREIGN KEY");
    }
    | FOREIGN_SYM KEY_SYM '(' key_column_commalist ')' REFERENCES_SYM table '(' key_column_commalist ')'
    {
        INCOMPLETION("FOREIGN KEY");
    }
    | CHECK_SYM '(' search_condition ')'
    {
        INCOMPLETION("CHECK");
    }
    ;

key_column_commalist:
    column
    {
        CHECK_ARRAY(Yacc->stmt, &(Yacc->field_list), sizeof(char*));
        Yacc->field_list.list[Yacc->field_list.len++] = $1;
    }
    | key_column_commalist ',' column
    {
        CHECK_ARRAY(Yacc->stmt, &(Yacc->field_list), sizeof(char*));
        Yacc->field_list.list[Yacc->field_list.len++] = $3;
    }
    ;

column_commalist:
    column_ref
    {
        T_ATTRDESC *attr;

        CHECK_ARRAY(Yacc->stmt, &(Yacc->col_list), sizeof(T_ATTRDESC));

        attr = &(Yacc->col_list.list[Yacc->col_list.len++]);

        p = sc_strchr($1, '.');
        if (p) {
            sc_strncpy(tablename, $1, p-$1);
            tablename[p++-$1] = '\0';

            attr->table.tablename = YACC_XSTRDUP(Yacc, tablename);
            isnull(attr->table.tablename);
            attr->table.aliasname = NULL;
            attr->attrname = YACC_XSTRDUP(Yacc, p);
            isnull(attr->attrname);
        } else {
            attr->table.tablename = NULL;
            attr->table.aliasname = NULL;
            attr->attrname = $1;
        }
    }
    | column_commalist ',' column_ref
    {
        T_ATTRDESC *attr;

        CHECK_ARRAY(Yacc->stmt, &(Yacc->col_list), sizeof(T_ATTRDESC));

        attr = &(Yacc->col_list.list[Yacc->col_list.len++]);
        p = sc_strchr($3, '.');
        if (p) {
            sc_strncpy(tablename, $3, p-$3);
            tablename[p++-$3] = '\0';

            attr->table.tablename = YACC_XSTRDUP(Yacc, tablename);
            isnull(attr->table.tablename);
            attr->table.aliasname = NULL;
            attr->attrname = YACC_XSTRDUP(Yacc, p);
            isnull(attr->attrname);
        } else {
            attr->table.tablename = NULL;
            attr->table.aliasname = NULL;
            attr->attrname = $3;
        }
    }
    ;

column_commalist_new:
    column
    {
        T_COLDESC *column;

        CHECK_ARRAY(Yacc->stmt, &(Yacc->coldesc_list), sizeof(T_COLDESC));
        column = &(Yacc->coldesc_list.list[Yacc->coldesc_list.len++]);
        column->name = $1;
        column->fieldinfo = NULL;
    }
    | column_commalist_new ',' column
    {
        T_COLDESC *column;

        CHECK_ARRAY(Yacc->stmt, &(Yacc->coldesc_list), sizeof(T_COLDESC));
        column = &(Yacc->coldesc_list.list[Yacc->coldesc_list.len++]);
        column->name = $3;
        column->fieldinfo = NULL;
    }
    ;

index_opt_unique:
    /* empty */
    | UNIQUE_SYM
    {
        vCREATE(Yacc)->u.index.uniqueness = UNIQUE_INDEX_BIT;
    }
    ;

  
index_element_commalist:
    index_column
    {
        sc_memset(&$$, 0, sizeof(T_IDX_LIST_FIELDS));
        CHECK_ARRAY(Yacc->stmt, &$$, sizeof(T_IDX_FIELD));
        $$.list[$$.len++] = $1;
    }
    | index_element_commalist ',' index_column
    {
        CHECK_ARRAY(Yacc->stmt, &$$, sizeof(T_IDX_FIELD));
        $$.list[$$.len++] = $3;
    }
    ;

index_column:
    column order_info
    {
        $$.name = $1;
        $$.ordertype = $2;
        $$.collation = MDB_COL_NONE;
    }
    | column collation_info_str
    {
        $$.name = $1;
        $$.ordertype = 'A';
        $$.collation = $2;
    }
    | column order_info_str collation_info_str 
    {
        $$.name = $1;
        $$.ordertype = $2;
        $$.collation = $3;
    }
    | column collation_info_str order_info_str
    {
        $$.name = $1;
        $$.ordertype = $3;
        $$.collation = $2;
    }
    ;

alter_statement:
    ALTER_SYM TABLE_SYM
    {
        vSQL(Yacc)->sqltype = ST_ALTER;
        vALTER(Yacc) = &vSQL(Yacc)->u._alter;
        Yacc->userdefined = 0;
    }
    table alter_definition
    {
        vALTER(Yacc)->tablename = $4;
    }
    | ALTER_SYM SEQUENCE_SYM 
    {
        vSQL(Yacc)->sqltype = ST_ALTER;
        vALTER(Yacc) = &vSQL(Yacc)->u._alter;
        Yacc->userdefined = 0;
    }
    sequence
    {
        vALTER(Yacc)->type = SAT_SEQUENCE;
        vALTER(Yacc)->u.sequence.name = $4;
    }
    SEQUENCE_DESCRIPTION_ALTER
    | REBUILD_SYM INDEX_SYM rebuild_definition
    {
        vSQL(Yacc)->sqltype = ST_ALTER;
        vALTER(Yacc) = &vSQL(Yacc)->u._alter;
        vALTER(Yacc)->type = SAT_REBUILD;
        Yacc->userdefined = 0;

        expr = (T_EXPRESSIONDESC *)$3;
        vALTER(Yacc)->u.sequence.bStart = DB_FALSE;
        vALTER(Yacc)->u.rebuild_idx.num_object = expr->len;
        vALTER(Yacc)->u.rebuild_idx.objectname =
            (char**)sql_mem_alloc(vSQL(Yacc)->parsing_memory,
                    sizeof(char*)*expr->len);    // with in NULL

        for(i = 0; i < expr->len; ++i)
        {
            vALTER(Yacc)->u.rebuild_idx.objectname[i] = 
                (char*)sql_mem_strdup(vSQL(Yacc)->parsing_memory,
                        expr->list[i]->u.value.u.ptr);
            isnull(vALTER(Yacc)->u.rebuild_idx.objectname[i]);
        }
    }
    | REBUILD_SYM INDEX_SYM ALL_SYM
    {
        vSQL(Yacc)->sqltype = ST_ALTER;
        vALTER(Yacc) = &vSQL(Yacc)->u._alter;
        vALTER(Yacc)->type = SAT_REBUILD;
        Yacc->userdefined = 0;

        vALTER(Yacc)->u.sequence.bStart = DB_FALSE;
        vALTER(Yacc)->u.rebuild_idx.num_object = 0;
        vALTER(Yacc)->u.rebuild_idx.objectname = NULL;
    }
    ;

rebuild_definition:
    rebuild_object_definition
    | rebuild_definition ',' rebuild_object_definition
    {
        $$ = merge_expression(Yacc, $1, $3, NULL);
    }
    ;

rebuild_object_definition:
    IDENTIFIER
    {
        sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
        elem.elemtype = SPT_VALUE;
        elem.u.value.valueclass     = SVC_CONSTANT;
        elem.u.value.valuetype      = DT_VARCHAR;
        elem.u.value.u.ptr          = $1;
        elem.u.value.attrdesc.len   = sc_strlen($1);
        elem.u.value.value_len      = elem.u.value.attrdesc.len;
        $$ = get_expr_with_item(Yacc, &elem);
    }   
    | IDENTIFIER '.' IDENTIFIER
    {
        int     idt_len1, idt_len2;

        idt_len1 = sc_strlen($1);
        idt_len2 = sc_strlen($3);

        sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
        elem.elemtype = SPT_VALUE;
        elem.u.value.valueclass     = SVC_CONSTANT;
        elem.u.value.valuetype      = DT_VARCHAR;
        elem.u.value.u.ptr = sql_mem_alloc(vSQL(Yacc)->parsing_memory, 
                idt_len1+idt_len2+2);    // with in NULL

        sc_memcpy(elem.u.value.u.ptr, $1, idt_len1);
        elem.u.value.u.ptr[idt_len1] = '.';
        sc_memcpy(&(elem.u.value.u.ptr[idt_len1+1]), $3, idt_len2);
        elem.u.value.u.ptr[idt_len1+1+idt_len2] = 0x00;

        elem.u.value.attrdesc.len   = idt_len1 + idt_len2 + 1;
        elem.u.value.value_len      = elem.u.value.attrdesc.len;
        $$ = get_expr_with_item(Yacc, &elem);
    }
    |   IDENTIFIER '.' PRIMARY_SYM
    {
        int     idt_len1, idt_len2;
        char    primary[8] = "primary";

        idt_len1 = sc_strlen($1);
        idt_len2 = sc_strlen(primary);

        sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
        elem.elemtype = SPT_VALUE;
        elem.u.value.valueclass     = SVC_CONSTANT;
        elem.u.value.valuetype      = DT_VARCHAR;
        elem.u.value.u.ptr = sql_mem_alloc(vSQL(Yacc)->parsing_memory, 
                idt_len1+idt_len2+2);    // with in NULL

        sc_memcpy(elem.u.value.u.ptr, $1, idt_len1);
        elem.u.value.u.ptr[idt_len1] = '.';
        sc_memcpy(&(elem.u.value.u.ptr[idt_len1+1]), primary, idt_len2);
        elem.u.value.u.ptr[idt_len1+1+idt_len2] = 0x00;

        elem.u.value.attrdesc.len   = idt_len1 + idt_len2 + 1;
        elem.u.value.attrdesc.len   = idt_len1 + idt_len2 + 1;
        elem.u.value.value_len      = elem.u.value.attrdesc.len;
        $$ = get_expr_with_item(Yacc, &elem);
    }
    ;

alter_definition:
    ADD_SYM column_def_list
    {
        vALTER(Yacc)->type = SAT_ADD;
        vALTER(Yacc)->scope = SAT_COLUMN;
        vALTER(Yacc)->column.attr_list = Yacc->attr_list;
        sc_memset(&(Yacc->attr_list), 0, sizeof(T_LIST_ATTRDESC));
        if (Yacc->field_list.list) {
            vALTER(Yacc)->constraint.type = SAT_PRIMARY_KEY;
            vALTER(Yacc)->constraint.list = Yacc->field_list;
            sc_memset(&(Yacc->field_list), 0, sizeof(T_LIST_FIELDS));
        }
    }
    | ADD_SYM table_constraint_def
            {
                vALTER(Yacc)->type = SAT_ADD;
                vALTER(Yacc)->scope = SAT_CONSTRAINT;
                vALTER(Yacc)->constraint.type = SAT_PRIMARY_KEY;
                vALTER(Yacc)->constraint.list = Yacc->field_list;
                sc_memset(&(Yacc->field_list), 0, sizeof(T_LIST_FIELDS));
            }
    |    ALTER_SYM column_def_list
            {
                vALTER(Yacc)->type = SAT_ALTER;
                vALTER(Yacc)->scope = SAT_COLUMN;
                vALTER(Yacc)->column.attr_list = Yacc->attr_list;
                sc_memset(&(Yacc->attr_list), 0, sizeof(T_LIST_ATTRDESC));
                if (Yacc->field_list.list) {
                    vALTER(Yacc)->constraint.type = SAT_PRIMARY_KEY;
                    vALTER(Yacc)->constraint.list = Yacc->field_list;
                    sc_memset(&(Yacc->field_list), 0, sizeof(T_LIST_FIELDS));
                }
            }
    |    ALTER_SYM column RENAME_SYM column
            {
                vALTER(Yacc)->type = SAT_ALTER;
                vALTER(Yacc)->scope = SAT_COLUMN;

                vALTER(Yacc)->column.name_list.list = sql_mem_alloc(vSQL(Yacc)->parsing_memory, sizeof(char *));
                vALTER(Yacc)->column.name_list.list[0] = $2;
                vALTER(Yacc)->column.name_list.max = 1;
                vALTER(Yacc)->column.name_list.len = 1;

                vALTER(Yacc)->column.attr_list.list = sql_mem_alloc(vSQL(Yacc)->parsing_memory, sizeof(T_ATTRDESC));
                sc_memset(vALTER(Yacc)->column.attr_list.list, 0, sizeof(T_ATTRDESC)*1);
                vALTER(Yacc)->column.attr_list.list[0].defvalue.defined = 0;
                vALTER(Yacc)->column.attr_list.list[0].attrname = $4;
                vALTER(Yacc)->column.attr_list.max = 1;
                vALTER(Yacc)->column.attr_list.len = 1;
            }
    |    ALTER_SYM table_constraint_def
            {
                vALTER(Yacc)->type = SAT_ALTER;
                vALTER(Yacc)->scope = SAT_CONSTRAINT;
                vALTER(Yacc)->constraint.type = SAT_PRIMARY_KEY;
                vALTER(Yacc)->constraint.list = Yacc->field_list;
                sc_memset(&(Yacc->field_list), 0, sizeof(T_LIST_FIELDS));
            }
    |    DROP_SYM column_list
            {
                vALTER(Yacc)->type = SAT_DROP;
                vALTER(Yacc)->scope = SAT_COLUMN;
                vALTER(Yacc)->column.name_list = Yacc->field_list;
                sc_memset(&(Yacc->field_list), 0, sizeof(T_LIST_FIELDS));
            }
    |    DROP_SYM PRIMARY_SYM KEY_SYM
            {
                vALTER(Yacc)->type = SAT_DROP;
                vALTER(Yacc)->scope = SAT_CONSTRAINT;
                vALTER(Yacc)->constraint.type = SAT_PRIMARY_KEY;
            }
    |    logging_clause '(' RUNTIME_SYM ')'
            {
                vALTER(Yacc)->type = SAT_LOGGING;
                vALTER(Yacc)->logging.on = $1;
                vALTER(Yacc)->logging.runtime = 1;
            }
    |    logging_clause
            {
                vALTER(Yacc)->type = SAT_LOGGING;
                vALTER(Yacc)->logging.on = $1;
                vALTER(Yacc)->logging.runtime = 0;
            }
     |    MSYNC_SYM enable_or_disable_or_flush
            {
                vALTER(Yacc)->type = SAT_MSYNC;
                vALTER(Yacc)->msync_alter_type = $2;
            }
    ;

enable_or_disable_or_flush:
    enable_or_disable
    {
        $$ = $1;
    }
    | FLUSH_SYM
    {
        $$ = 2;
    }
    ;

column_def_list:
        column_def
            {
                CHECK_ARRAY(Yacc->stmt, &(Yacc->attr_list), sizeof(T_ATTRDESC));
                append(&(Yacc->attr_list), &(Yacc->attr));
                sc_memset(&(Yacc->attr), 0, sizeof(T_ATTRDESC));
            }
    |    column_def_list ',' column_def
            {
                CHECK_ARRAY(Yacc->stmt, &(Yacc->attr_list), sizeof(T_ATTRDESC));
                append(&(Yacc->attr_list), &(Yacc->attr));
                sc_memset(&(Yacc->attr), 0, sizeof(T_ATTRDESC));
            }
    ;

column_list:
        column
            {
                CHECK_ARRAY(Yacc->stmt, &(Yacc->field_list), sizeof(char*));
                Yacc->field_list.list[Yacc->field_list.len++] = $1;
            }
    |    column_list ',' column
            {
                CHECK_ARRAY(Yacc->stmt, &(Yacc->field_list), sizeof(char*));
                Yacc->field_list.list[Yacc->field_list.len++] = $3;
            }
    ;

drop_statement:
        DROP_SYM TABLE_SYM table
            {
                ACTION_drop_statement(Yacc, SDT_TABLE, $3);
            }
    |   DROP_SYM INDEX_SYM index
            {
                ACTION_drop_statement(Yacc, SDT_INDEX, $3);
            }
    |   DROP_SYM VIEW_SYM view
            {
                ACTION_drop_statement(Yacc, SDT_VIEW, $3);
            }
    |    DROP_SYM SEQUENCE_SYM sequence
            {
                ACTION_drop_statement(Yacc, SDT_SEQUENCE, $3);
            }
    |    DROP_SYM RID_SYM INTNUM
            {
                OID *rid = sql_malloc(OID_SIZE, 0);

                *rid = (OID)$3;

                ACTION_drop_statement(Yacc, SDT_RID, (char*)rid);
            }
    ;

rename_statement:
        RENAME_SYM TABLE_SYM table AS_SYM table
            {
                vSQL(Yacc)->sqltype    = ST_RENAME;
                vRENAME(Yacc)        = &vSQL(Yacc)->u._rename;
                vRENAME(Yacc)->type = SRT_TABLE;
                vRENAME(Yacc)->oldname    = $3;
                vRENAME(Yacc)->newname    = $5;
            }
    |    RENAME_SYM INDEX_SYM index AS_SYM index
            {
                vSQL(Yacc)->sqltype    = ST_RENAME;
                vRENAME(Yacc)        = &vSQL(Yacc)->u._rename;
                vRENAME(Yacc)->type = SRT_INDEX;
                vRENAME(Yacc)->oldname    = $3;
                vRENAME(Yacc)->newname    = $5;
            }
    ;

commit_statement:
        COMMIT_SYM
            {
                vSQL(Yacc)->sqltype = ST_COMMIT;
            }
    |    COMMIT_SYM WORK_SYM
            {
                vSQL(Yacc)->sqltype = ST_COMMIT;
            }
    |    COMMIT_SYM FLUSH_SYM
            {
                vSQL(Yacc)->sqltype = ST_COMMIT_FLUSH;
            }
    ;

delete_statement:
        DELETE_SYM
            {
                vSQL(Yacc)->sqltype = ST_DELETE;
                vDELETE(Yacc) = &vSQL(Yacc)->u._delete;
                vSELECT(Yacc)       = &vSQL(Yacc)->u._delete.subselect;
                vSELECT(Yacc)->born_stmt = vSQL(Yacc);
                init_limit(&vSELECT(Yacc)->planquerydesc.querydesc.limit);

                Yacc->userdefined = 0;
                Yacc->param_count = 0;
            }
        FROM_SYM table_ref opt_scan_hint opt_where_clause 
            {
                init_limit(&vDELETE(Yacc)->planquerydesc.querydesc.limit);
            }
        opt_limit_clause
            {
                T_LIST_JOINTABLEDESC *ltable;

                ltable = &vDELETE(Yacc)->planquerydesc.querydesc.fromlist;
                CHECK_ARRAY_BY_LIMIT(Yacc->stmt, ltable,
                        sizeof(T_JOINTABLEDESC), MAX_JOIN_TABLE_NUM);
                ltable->list[ltable->len].join_type     = SJT_NONE;
                ltable->list[ltable->len].condition.len = 0;
                ltable->list[ltable->len].condition.max = 0;
                ltable->list[ltable->len].condition.list= NULL;
                ltable->list[ltable->len].table         = $4;
                ltable->list[ltable->len].scan_hint     = $5;
                ++ltable->len;
                vDELETE(Yacc)->param_count = Yacc->param_count;
            }
    ;

insert_statement:
        INSERT_SYM
            {
                vSQL(Yacc)->sqltype = ST_INSERT;
                vINSERT(Yacc) = &vSQL(Yacc)->u._insert;

                Yacc->userdefined = 0;
                Yacc->param_count = 0;
            }
        INTO_SYM table_ref opt_column_commalist_new values_or_query_spec
            {
                vINSERT(Yacc)->table = $4;

                if (vINSERT(Yacc)->type == SIT_VALUES) {
                    if (Yacc->coldesc_list.len > 0) {
                        if (Yacc->coldesc_list.len < Yacc->val_list.len)
                            __yyerror("too many values");
                        else if (Yacc->coldesc_list.len > Yacc->val_list.len)
                            __yyerror("not enough values");
                    }
                }
                vINSERT(Yacc)->columns = Yacc->coldesc_list;
                sc_memset(&(Yacc->coldesc_list), 0, sizeof(T_LIST_COLDESC));

                if (vINSERT(Yacc)->type == SIT_VALUES) {
                    vINSERT(Yacc)->u.values = Yacc->val_list;
                    sc_memset(&(Yacc->val_list), 0, sizeof(T_LIST_VALUE));
                }

                vINSERT(Yacc)->param_count = Yacc->param_count;
            }
    ;

opt_column_commalist:
        /* empty */
    |    '(' column_commalist ')'
    ;

opt_column_commalist_new:
        /* empty */
    |    '(' column_commalist_new ')'
    ;

values_or_query_spec:
        VALUES_SYM '(' insert_atom_commalist ')'
            {
                vINSERT(Yacc)->type = SIT_VALUES;
            }
    |   select_body
            {
                vINSERT(Yacc)->type = SIT_QUERY;
            }
    ;

insert_atom_commalist:
        scalar_exp 
            {
                CHECK_ARRAY(Yacc->stmt,
                        &(Yacc->val_list), sizeof(T_EXPRESSIONDESC));
                expr = &(Yacc->val_list.list[Yacc->val_list.len++]);
                sc_memcpy(expr, $1, sizeof(T_EXPRESSIONDESC));
            }
    |    insert_atom_commalist ',' scalar_exp
            {
                CHECK_ARRAY(Yacc->stmt,
                        &(Yacc->val_list), sizeof(T_EXPRESSIONDESC));
                expr = &(Yacc->val_list.list[Yacc->val_list.len++]);
                sc_memcpy(expr, $3, sizeof(T_EXPRESSIONDESC));
            }
    ;

msynced_opt:
    /* empty */
    {
        $$ = 0;
    }
    | SYNCED_RECORD_SYM
    {
        $$ = 1;
    }

upsert_statement:
        UPSERT_SYM
            {
                vSQL(Yacc)->sqltype = ST_UPSERT;
                vINSERT(Yacc) = &vSQL(Yacc)->u._insert;

                Yacc->userdefined = 0;
                Yacc->param_count = 0;
            }
        INTO_SYM table_ref opt_column_commalist_new VALUES_SYM '(' insert_atom_commalist ')' msynced_opt
            {
                vINSERT(Yacc)->table = $4;

                vINSERT(Yacc)->type = SIT_VALUES;

                if (Yacc->coldesc_list.len > 0) {
                    if (Yacc->coldesc_list.len < Yacc->val_list.len)
                        __yyerror("too many values");
                    else if (Yacc->coldesc_list.len > Yacc->val_list.len)
                        __yyerror("not enough values");
                }

                vINSERT(Yacc)->columns = Yacc->coldesc_list;
                sc_memset(&(Yacc->coldesc_list), 0, sizeof(T_LIST_COLDESC));

                vINSERT(Yacc)->u.values = Yacc->val_list;
                sc_memset(&(Yacc->val_list), 0, sizeof(T_LIST_VALUE));

                vINSERT(Yacc)->param_count = Yacc->param_count;
                vINSERT(Yacc)->is_upsert_msync = $10;
            }
    ;                            

rollback_statement:
        ROLLBACK_SYM
            {
                vSQL(Yacc)->sqltype = ST_ROLLBACK;
            }
    |    ROLLBACK_SYM WORK_SYM
            {
                vSQL(Yacc)->sqltype = ST_ROLLBACK;
            }
    |    ROLLBACK_SYM FLUSH_SYM
            {
                vSQL(Yacc)->sqltype = ST_ROLLBACK_FLUSH;
            }
    ;

select_statement:
    select_body
    ;

select_body:
    select_exp
    | set_operation_exp
    {
        T_EXPRESSIONDESC *setlist;
        T_QUERYDESC *qdesc;

        setlist = $1;

        for (i = 0; i < setlist->len; ++i)
        {
            if (setlist->list[i]->elemtype == SPT_SUBQUERY)
            {
                qdesc = &setlist->list[i]->u.subq->planquerydesc.querydesc;

                if (qdesc->orderby.len > 0)
                {
                    __yyerror("Invalid orderby expression.");
                }

                if (qdesc->limit.rows != -1)
                {
                    __yyerror("Invalid limit expression.");
                }

                if (qdesc->limit.start_at != NULL_OID)
                {
                    __yyerror("limit @rid not allowed "
                            "set operation query.");
                }
            }
        }

        sc_memcpy(&vPLAN(Yacc)->querydesc.setlist, setlist,
                sizeof(T_EXPRESSIONDESC));
    }
    ;

select_exp:
    SELECT_SYM
    {
        if (Yacc->stmt->sqltype == ST_NONE)
        {
            vSQL(Yacc)->sqltype = ST_SELECT;
            vSELECT(Yacc) = &vSQL(Yacc)->u._select;

            Yacc->userdefined = 0;
            Yacc->param_count = 0;
        }
        else if (vSELECT(Yacc) &&
                vSELECT(Yacc)->planquerydesc.querydesc.setlist.len > 0)
        {
            T_SELECT *sub_select;

            sub_select = sql_mem_alloc(vSQL(Yacc)->parsing_memory,
                    sizeof(T_SELECT));
            if(sub_select == NULL)
            {
                __yyerror("PMEM_ALLOC fail");
            }

            sc_memset(sub_select, 0, sizeof(T_SELECT));
            sub_select->first_sub = NULL;
            sub_select->sibling_query = NULL;
            sub_select->super_query = Yacc->_select;
            sub_select->kindofwTable = iTABLE_SUBSET;
            sub_select->main_parsing_chunk = vSQL(Yacc)->parsing_memory;

            Yacc->_select = sub_select;
            vSQL(Yacc)->do_recompile = MDB_TRUE;
        }
        else if(Yacc->stmt->sqltype == ST_CREATE ||
                Yacc->stmt->sqltype == ST_INSERT)
        {
            if (vSQL(Yacc)->sqltype == ST_CREATE)
            {
                if (vCREATE(Yacc)->type == SCT_VIEW)
                {
                    vSELECT(Yacc) = &vCREATE(Yacc)->u.view.query;
                }
                else
                {
                    vSELECT(Yacc) = &vCREATE(Yacc)->u.table_query.query;
                }
            }
            else if (vSQL(Yacc)->sqltype == ST_INSERT)
            {
                vSELECT(Yacc) = &vINSERT(Yacc)->u.query;
            }

            vSELECT(Yacc)->tstatus = TSS_PARSED;
            Yacc->param_count   = 0;

            Yacc->prev_sqltype  = vSQL(Yacc)->sqltype;
            vSQL(Yacc)->sqltype = ST_SELECT;
        }
        else
        {
            SELECT_STACK *tmp_sem;
            T_SELECT *sub_select;

            if(Yacc->stmt->sqltype != ST_SELECT &&
                    Yacc->stmt->sqltype != ST_UPDATE &&
                    Yacc->stmt->sqltype != ST_DELETE)
            {

                INCOMPLETION("subquery");
            }

            if (Yacc->selection.len > 0)
            {
                vPLAN(Yacc)->querydesc.selection = Yacc->selection;
                sc_memset(&(Yacc->selection), 0, sizeof(T_LIST_SELECTION));
            }

            tmp_sem = sql_mem_alloc(vSQL(Yacc)->parsing_memory,
                    sizeof(SELECT_STACK));
            sub_select = sql_mem_alloc(vSQL(Yacc)->parsing_memory,
                    sizeof(T_SELECT));
            if(tmp_sem == NULL || sub_select == NULL)
            {
                __yyerror("PMEM_ALLOC fail");
            }

            /* initialize sub_select */
            sc_memset(sub_select, 0, sizeof(T_SELECT));
            sub_select->first_sub = NULL;
            sub_select->sibling_query = NULL;
            sub_select->super_query = Yacc->_select;
            /* where 절 subquery인 경우 iTABLE_SUBWHERE로 설정 */
            /* SUB QUERY에서 MAIN_STMT'S의 PARSING MEMORY가 필요하다
               (FOR SQL_CLEANUP) */
            sub_select->main_parsing_chunk = vSQL(Yacc)->parsing_memory;

            /* push current select informations */
            tmp_sem->ptr = (void *)Yacc->_select;
            tmp_sem->next = Yacc->select_top;
            Yacc->select_top = tmp_sem;

            /* set current select as .. */
            Yacc->_select = sub_select;
            vSQL(Yacc)->do_recompile = MDB_TRUE;
        }
    }
    opt_all_distinct
    selection
    table_exp
    fetch_record_type
    {
        if ($6)
        {

            if (vSELECT(Yacc)->sibling_query != NULL
                    || vPLAN(Yacc)->querydesc.fromlist.len > 1
                    || vPLAN(Yacc)->querydesc.groupby.len > 0
                    || vPLAN(Yacc)->querydesc.orderby.len > 0)
            {
                __yyerror("Invalid mSync operation.");
            }

            vPLAN(Yacc)->querydesc.fromlist.list->scan_hint.scan_type =
                SMT_SSCAN;
        }

        vSELECT(Yacc)->msync_flag = $6;

        if (Yacc->prev_sqltype != ST_NONE)
        {
            vSQL(Yacc)->sqltype = Yacc->prev_sqltype;
            Yacc->prev_sqltype = ST_NONE;
        }
        if (!Yacc->_select->super_query)
        {
            vSELECT(Yacc)->param_count = Yacc->param_count;
        }
    }
    ;

fetch_record_type:
    /* empty */
    {
        $$ = 0;
    }
    | SYNCED_RECORD_SYM
    {
        $$ = SYNCED_SLOT;
    }
    | INSERT_RECORD_SYM
    {
        $$ = USED_SLOT;
    }
    | UPDATE_RECORD_SYM
    {
        $$ = UPDATE_SLOT;
    }
    | DELETE_RECORD_SYM
    {
        $$ = DELETE_SLOT;
    }
    ;

set_operation_exp:
    set_query_body SET_OPERATION set_opt_all set_query_body
    {
        sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
        elem.elemtype = SPT_SUBQ_OP;
        elem.u.op.type = $2.type;
        if ($3)
        {
            if (elem.u.op.type == SOT_UNION)
            {
                elem.u.op.type = SOT_UNION_ALL;
            }
            else
            {
                __yyerror("Invalid set operation.");
            }
        }
        elem.u.op.argcnt = 2;

        $$ = merge_expression(Yacc, $1, $4, &elem);
    }
    | '(' set_operation_exp ')' opt_order_by_clause opt_limit_clause
    {
        $$ = $2;
    }
    ;

set_opt_all:
    {
        $$ = 0;
    }
    | ALL_SYM
    {
        $$ = 1;
    }
    ;

set_query_body:
    set_query_exp
    {
        $$ = $1;
    }
    | '(' set_query_exp ')'
    {
        $$ = $2;
    }
    | set_operation_exp
    {
        $$ = $1;
    }
    ;

set_query_exp:
    select_exp
    {
        if (!vSELECT(Yacc)->super_query ||
                (vSELECT(Yacc)->super_query &&
                 vSELECT(Yacc)->super_query->planquerydesc.querydesc.setlist.len == 0))
        {
            T_SELECT *sub_select;
            T_SELECT *current_tst;

            sub_select = 
                sql_mem_alloc(vSQL(Yacc)->parsing_memory, sizeof(T_SELECT));

            sc_memcpy(sub_select, vSELECT(Yacc), sizeof(T_SELECT));
            sub_select->super_query = vSELECT(Yacc);
            sc_memset(&vSELECT(Yacc)->planquerydesc, 0,
                    sizeof(T_PLANNEDQUERYDESC));
            init_limit(&vPLAN(Yacc)->querydesc.limit);

            current_tst = Yacc->_select;
            if(current_tst->first_sub)
            {
                if (current_tst->first_sub->super_query ==
                        sub_select->super_query)
                {
                    current_tst->first_sub->super_query = sub_select;
                }
                current_tst = current_tst->first_sub;

                while(current_tst->sibling_query)
                {
                    if (current_tst->sibling_query->super_query ==
                            sub_select->super_query)
                    {
                        current_tst->sibling_query->super_query = sub_select;
                    }
                    current_tst = current_tst->sibling_query;
                }
            }

            Yacc->_select->first_sub = NULL; 
            Yacc->_select->sibling_query = NULL; 

            sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
            elem.elemtype = SPT_SUBQUERY;
            elem.u.subq   = sub_select;
            elem.u.subq->kindofwTable = iTABLE_SUBSET;
        }
        else
        {
            sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
            elem.elemtype = SPT_SUBQUERY;
            elem.u.subq   = Yacc->_select;
            Yacc->_select = Yacc->_select->super_query;
        }

        if (vPLAN(Yacc)->querydesc.orderby.len > 0)
        {
            __yyerror("Invalid orderby expression.");
        }

        if (vPLAN(Yacc)->querydesc.limit.rows != -1)
        {
            __yyerror("Invalid limit expression.");
        }

        if (vPLAN(Yacc)->querydesc.limit.start_at != NULL_OID)
        {
            __yyerror("limit @rid not allowed set operation query.");
        }

        ++(vSELECT(Yacc)->planquerydesc.querydesc.setlist.len);

        $$ = get_expr_with_item(Yacc, &elem);
    }
    ;

show_all_or_table:
        ALL_SYM
            {
                $$ = NULL;
            }
        | table
            {
                $$ = $1;
            }
    ;    

opt_all_distinct:
        /* empty */
            {
                vPLAN(Yacc)->querydesc.is_distinct = 0;
            }
    |    ALL_SYM
            {
                vPLAN(Yacc)->querydesc.is_distinct = 0;
            }
    |    DISTINCT_SYM
            {
                vPLAN(Yacc)->querydesc.is_distinct = 1;
            }
    ;

selection:
        selection_item_commalist
            {
                vPLAN(Yacc)->querydesc.selection = Yacc->selection;
                sc_memset(&(Yacc->selection), 0, sizeof(T_LIST_SELECTION));
                vPLAN(Yacc)->querydesc.select_star = 0;
            }
    |    '*'
            {
                sc_memset(&(vPLAN(Yacc)->querydesc.selection), 0,
                            sizeof(T_LIST_SELECTION));

                vPLAN(Yacc)->querydesc.select_star = 1;
            }
    ;

selection_item_commalist:
        scalar_exp opt_selection_item_alias
            {
                CHECK_ARRAY(Yacc->stmt, &(Yacc->selection),
                        sizeof(T_SELECTION));

                if ($2)
                {
                    sc_strncpy(
                            Yacc->selection.list[Yacc->selection.len].res_name,
                            $2, FIELD_NAME_LENG-1);
                }
                else
                    Yacc->selection.list[Yacc->selection.len].res_name[0] =
                        '\0';
                Yacc->selection.list[Yacc->selection.len].res_name[
                    FIELD_NAME_LENG-1] = '\0';
                expr = &(Yacc->selection.list[Yacc->selection.len].expr);
                if ($1) sc_memcpy(expr, $1, sizeof(T_EXPRESSIONDESC));
                else {
                    expr->len  = 0;
                    expr->max  = 0;
                    expr->list = NULL;
                }

                Yacc->selection.len++;
            }
    |    selection_item_commalist ',' scalar_exp opt_selection_item_alias
            {
                if (vPLAN(Yacc)->querydesc.selection.len > 0)
                {
                    Yacc->selection = vPLAN(Yacc)->querydesc.selection;
                    sc_memset(&(vPLAN(Yacc)->querydesc.selection),
                            0, sizeof(T_LIST_SELECTION));
                }

                CHECK_ARRAY(Yacc->stmt, &(Yacc->selection),
                        sizeof(T_SELECTION));
                if ($4)
                {
                    sc_strncpy(
                            Yacc->selection.list[Yacc->selection.len].res_name,
                            $4, FIELD_NAME_LENG-1);
                }
                else
                    Yacc->selection.list[Yacc->selection.len].res_name[0] =
                        '\0';
                Yacc->selection.list[Yacc->selection.len].res_name[
                    FIELD_NAME_LENG-1] = '\0';
                expr = &(Yacc->selection.list[Yacc->selection.len].expr);
                if ($3) sc_memcpy(expr, $3, sizeof(T_EXPRESSIONDESC));
                else {
                    expr->len  = 0;
                    expr->max  = 0;
                    expr->list = NULL;
                }

                Yacc->selection.len++;
            }
    ;

opt_selection_item_alias:
        /* empty */
            {
                $$ = NULL;
            }
    |    opt_ansi_style STRINGVAL
            {
                __CHECK_ALIAS_NAME_LENG($2);
                $$ = $2;
            }
    |    opt_ansi_style columnalias_item
            {
                $$ = $2;
            }
    ;

update_statement:
        UPDATE_SYM
        {
            vSQL(Yacc)->sqltype = ST_UPDATE;
            vUPDATE(Yacc)       = &vSQL(Yacc)->u._update;
            vSELECT(Yacc)            = &vSQL(Yacc)->u._update.subselect;
            vSELECT(Yacc)->born_stmt = vSQL(Yacc);
            init_limit(&vSELECT(Yacc)->planquerydesc.querydesc.limit);
            Yacc->userdefined = 0;
            Yacc->param_count = 0;
        }
        update_table_or_rid
        {
            vUPDATE(Yacc)->param_count = Yacc->param_count;
            vUPDATE(Yacc)->columns     = Yacc->coldesc_list;
            vUPDATE(Yacc)->values      = Yacc->val_list;
            sc_memset(&(Yacc->coldesc_list), 0, sizeof(T_LIST_COLDESC));
            sc_memset(&(Yacc->val_list), 0, sizeof(T_LIST_VALUE));
        }
    ;

update_table_or_rid:
        table_ref opt_scan_hint SET_SYM assignment_commalist opt_where_clause 
        {
            init_limit(&vUPDATE(Yacc)->planquerydesc.querydesc.limit);
        }
        opt_limit_clause
        {
            T_LIST_JOINTABLEDESC *ltable;

            ltable = &vUPDATE(Yacc)->planquerydesc.querydesc.fromlist;
            CHECK_ARRAY_BY_LIMIT(Yacc->stmt, ltable,
                    sizeof(T_JOINTABLEDESC), MAX_JOIN_TABLE_NUM);
            ltable->list[ltable->len].join_type     = SJT_NONE;
            ltable->list[ltable->len].condition.len = 0;
            ltable->list[ltable->len].condition.max = 0;
            ltable->list[ltable->len].condition.list= NULL;
            ltable->list[ltable->len].table         = $1;
            ltable->list[ltable->len].scan_hint     = $2;
            ++ltable->len;
            vUPDATE(Yacc)->rid   = NULL_OID;
        }
        | RID_SYM INTNUM SET_SYM assignment_commalist
        {
            vUPDATE(Yacc)->rid   = (OID)$2;
        }
    ;

assignment_commalist:
        assignment
    |    assignment_commalist ',' assignment
    ;

assignment:
        column COMPARISON scalar_exp
        {
            T_COLDESC *column;

            if ($2.type != SOT_EQ) __yyerror("ERROR");
            else {
                if ($3->list[0]->elemtype == SPT_SUBQUERY)
                {
                    T_QUERYDESC * qdesc;
                    qdesc = &$3->list[0]->u.subq->planquerydesc.querydesc;
                    if (qdesc->selection.len > 1)
                    {
                        __yyerror("too many values");
                    }
                }

                CHECK_ARRAY(Yacc->stmt, &(Yacc->coldesc_list),
                        sizeof(T_COLDESC));
                column = &(Yacc->coldesc_list.list[Yacc->coldesc_list.len++]);
                column->name = $1;
                column->fieldinfo = NULL;

                CHECK_ARRAY(Yacc->stmt,
                        &(Yacc->val_list), sizeof(T_EXPRESSIONDESC));
                expr = &(Yacc->val_list.list[Yacc->val_list.len++]);
                sc_memcpy(expr, $3, sizeof(T_EXPRESSIONDESC));
            }
        }
;

truncate_statement:
        TRUNCATE_SYM table
            {
                vSQL(Yacc)->sqltype = ST_TRUNCATE;
                vTRUNCATE(Yacc) = &vSQL(Yacc)->u._truncate;
                vTRUNCATE(Yacc)->objectname = $2;
            }
        | TRUNCATE_SYM TABLE_SYM table
            {
                vSQL(Yacc)->sqltype = ST_TRUNCATE;
                vTRUNCATE(Yacc) = &vSQL(Yacc)->u._truncate;
                vTRUNCATE(Yacc)->objectname = $3;
            }
    ;

desc_statement:
        DESC_SYM
            {
                vSQL(Yacc)->sqltype = ST_DESCRIBE;
                vDESCRIBE(Yacc) = &vSQL(Yacc)->u._desc;
                vDESCRIBE(Yacc)->type = SBT_DESC_TABLE;
                Yacc->userdefined = 0;
                Yacc->param_count = 0;
            }
    |
        DESCRIBE_SYM
            {
                vSQL(Yacc)->sqltype = ST_DESCRIBE;
                vDESCRIBE(Yacc) = &vSQL(Yacc)->u._desc;
                vDESCRIBE(Yacc)->type = SBT_DESC_TABLE;
                Yacc->userdefined = 0;
            }
    |
        DESC_SYM
            {
                vSQL(Yacc)->sqltype = ST_DESCRIBE;
                vDESCRIBE(Yacc) = &vSQL(Yacc)->u._desc;
                vDESCRIBE(Yacc)->type = SBT_DESC_TABLE;
                Yacc->userdefined = 0;
            }
        table opt_describe_column
            {
                vDESCRIBE(Yacc)->describe_table  = $3;
                vDESCRIBE(Yacc)->describe_column = $4;
            }
    |
        DESCRIBE_SYM
            {
                vSQL(Yacc)->sqltype = ST_DESCRIBE;
                vDESCRIBE(Yacc) = &vSQL(Yacc)->u._desc;
                vDESCRIBE(Yacc)->type = SBT_DESC_TABLE;
                Yacc->userdefined = 0;
            }
        table opt_describe_column
            {
                vDESCRIBE(Yacc)->describe_table  = $3;
                vDESCRIBE(Yacc)->describe_column = $4;
            }
    |
        DESC_SYM COMPILE_OPT_SYM
            {
#if defined(USE_DESC_COMPILE_OPT)
                vSQL(Yacc)->sqltype = ST_DESCRIBE;
                vDESCRIBE(Yacc) = &vSQL(Yacc)->u._desc;
                vDESCRIBE(Yacc)->type = SBT_DESC_COMPILE_OPT;
                Yacc->userdefined = 0;
#else
                __yyerror("DESCRIBE COMPILE Option is not supported.");
#endif
            }
    |
        DESCRIBE_SYM COMPILE_OPT_SYM
            {
#if defined(USE_DESC_COMPILE_OPT)
                vSQL(Yacc)->sqltype = ST_DESCRIBE;
                vDESCRIBE(Yacc) = &vSQL(Yacc)->u._desc;
                vDESCRIBE(Yacc)->type = SBT_DESC_COMPILE_OPT;
                Yacc->userdefined = 0;
#else
                __yyerror("DESCRIBE COMPILE Option is not supported.");
#endif
            }
    |   SHOW_SYM TABLE_SYM show_all_or_table
            {
                vSQL(Yacc)->sqltype = ST_DESCRIBE;
                vDESCRIBE(Yacc) = &vSQL(Yacc)->u._desc;
                vDESCRIBE(Yacc)->type = SBT_SHOW_TABLE;
                Yacc->userdefined = 0;
                vDESCRIBE(Yacc)->describe_table  = $3;
            }
    |   SHOW_SYM TABLE_SYM show_all_or_table INTO_SYM STRINGVAL export_op
            {
                vSQL(Yacc)->sqltype = ST_ADMIN;
                vADMIN(Yacc) = &vSQL(Yacc)->u._admin;
                Yacc->userdefined = 0;
                Yacc->param_count = 0;

                vADMIN(Yacc)->u.export.data_or_schema = 1;
                if ($3 == NULL)
                    vADMIN(Yacc)->type  = SADM_EXPORT_ALL;
                else
                    vADMIN(Yacc)->type  = SADM_EXPORT_ONE;

                vADMIN(Yacc)->u.export.table_name   = $3;
                vADMIN(Yacc)->u.export.export_file  = $5;
                vADMIN(Yacc)->u.export.f_append     = $6;
            }
    ;

on_or_off:
        ON_SYM    { $$ = 1; }
    |    OFF_SYM { $$ = 0; }
    ;

set_variable_list:
        AUTOCOMMIT_SYM on_or_off
            {
                vSET(Yacc)->type     = SST_AUTOCOMMIT;
                vSET(Yacc)->u.on_off = $2;
            }
    |    TIME_SYM on_or_off
            {
                vSET(Yacc)->type     = SST_TIME;
                vSET(Yacc)->u.on_off = $2;
            }
    |    HEADING_SYM on_or_off
            {
                vSET(Yacc)->type     = SST_HEADING;
                vSET(Yacc)->u.on_off = $2;
            }
    |    FEEDBACK_SYM on_or_off
            {
                vSET(Yacc)->type     = SST_FEEDBACK;
                vSET(Yacc)->u.on_off = $2;
            }
    |    RECONNECT_SYM on_or_off
            {
                vSET(Yacc)->type     = SST_RECONNECT;
                vSET(Yacc)->u.on_off = $2;
            }
    |    EXPLAIN_SYM PLAN_SYM on_or_off
            {
                vSET(Yacc)->type     = ($3 ? SST_PLAN_ON: SST_PLAN_OFF);
                vSET(Yacc)->u.on_off = 1;
            }
    |    EXPLAIN_SYM PLAN_SYM ONLY_SYM
            {
                vSET(Yacc)->type     = SST_PLAN_ONLY;
                vSET(Yacc)->u.on_off = 1;
            }
    ;

set_statement:
        SET_SYM
            {
                vSQL(Yacc)->sqltype = ST_SET;
                vSET(Yacc) = &vSQL(Yacc)->u._set;
                Yacc->userdefined = 0;
            }
        set_variable_list

        | SET_SYM
        {
        }
        IDENTIFIER COMPARISON scalar_exp
        {
        }
    ;

opt_describe_column:
        /* empty */
            { $$ = NULL; }
    |    column
            { $$ = $1; }
    |    STRINGVAL
            { 
                __CHECK_FIELD_NAME_LENG($1);
                $$ = $1;
            }
    ;

    /* query expressions */

table_exp:
        from_clause
        opt_where_clause
        opt_group_by_clause
        opt_order_by_clause
        {
            init_limit(&vSELECT(Yacc)->planquerydesc.querydesc.limit);
        }
        opt_limit_clause
        {
            if (vPLAN(Yacc)->querydesc.groupby.len && 
                vPLAN(Yacc)->querydesc.limit.start_at != NULL_OID)
            {
                __yyerror("limit @rid not allowed with group by.");
            }
        }
    ;

from_clause:
        /* empty */
        {
                T_LIST_JOINTABLEDESC *ltable;

                ltable = &vPLAN(Yacc)->querydesc.fromlist;
                CHECK_ARRAY_BY_LIMIT(Yacc->stmt, ltable,
                        sizeof(T_JOINTABLEDESC), MAX_JOIN_TABLE_NUM);
                ltable->list[ltable->len].join_type     = SJT_NONE;
                ltable->list[ltable->len].condition.len = 0;
                ltable->list[ltable->len].condition.max = 0;
                ltable->list[ltable->len].condition.list= NULL;
                ltable->list[ltable->len].table.tablename =
                    YACC_XSTRDUP(Yacc, "sysdummy");
                isnull(ltable->list[ltable->len].table.tablename);
                ltable->list[ltable->len].table.aliasname = NULL;
                ltable->list[ltable->len].table.correlated_tbl = NULL;
                sc_memset(&ltable->list[ltable->len].scan_hint, 0,
                        sizeof(T_SCANHINT));
                ++ltable->len;
        }
        | FROM_SYM join_table_commalist
    ;

join_table_commalist:
        table_ref opt_scan_hint
            {
                T_LIST_JOINTABLEDESC *ltable;

                ltable = &vPLAN(Yacc)->querydesc.fromlist;
                CHECK_ARRAY_BY_LIMIT(Yacc->stmt, ltable,
                        sizeof(T_JOINTABLEDESC), MAX_JOIN_TABLE_NUM);
                ltable->list[ltable->len].join_type     = SJT_NONE;
                ltable->list[ltable->len].condition.len = 0;
                ltable->list[ltable->len].condition.max = 0;
                ltable->list[ltable->len].condition.list= NULL;
                ltable->list[ltable->len].table         = $1;
                ltable->list[ltable->len].scan_hint     = $2;
                ++ltable->len;
            }
    |    join_table_commalist cross_join table_ref opt_scan_hint
            {
                T_LIST_JOINTABLEDESC *ltable;

                ltable = &vPLAN(Yacc)->querydesc.fromlist;
                CHECK_ARRAY_BY_LIMIT(Yacc->stmt, ltable,
                        sizeof(T_JOINTABLEDESC), MAX_JOIN_TABLE_NUM);
                ltable->list[ltable->len].join_type     = SJT_CROSS_JOIN;
                ltable->list[ltable->len].condition.len = 0;
                ltable->list[ltable->len].condition.max = 0;
                ltable->list[ltable->len].condition.list= NULL;
                ltable->list[ltable->len].table         = $3;
                ltable->list[ltable->len].scan_hint     = $4;
                ++ltable->len;
            }
    |    join_table_commalist INNER_SYM JOIN_SYM table_ref opt_scan_hint ON_SYM search_condition
            {
                T_LIST_JOINTABLEDESC *ltable;

                ltable = &vPLAN(Yacc)->querydesc.fromlist;
                CHECK_ARRAY_BY_LIMIT(Yacc->stmt, ltable,
                        sizeof(T_JOINTABLEDESC), MAX_JOIN_TABLE_NUM);
                ltable->list[ltable->len].join_type = SJT_INNER_JOIN;
                sc_memcpy(&(ltable->list[ltable->len].condition),
                        $7, sizeof(T_EXPRESSIONDESC));
                ltable->list[ltable->len].table     = $4;
                ltable->list[ltable->len].scan_hint = $5;
                ++ltable->len;
            }
    |    join_table_commalist LEFT_SYM opt_outer JOIN_SYM table_ref opt_scan_hint ON_SYM search_condition
            {
                T_LIST_JOINTABLEDESC *ltable;

                ltable = &vPLAN(Yacc)->querydesc.fromlist;
                CHECK_ARRAY_BY_LIMIT(Yacc->stmt, ltable,
                        sizeof(T_JOINTABLEDESC), MAX_JOIN_TABLE_NUM);
                ltable->outer_join_exists = 1;
                ltable->list[ltable->len].join_type = SJT_LEFT_OUTER_JOIN;
                sc_memcpy(&(ltable->list[ltable->len].condition),
                            $8, sizeof(T_EXPRESSIONDESC));
                ltable->list[ltable->len].table     = $5;
                ltable->list[ltable->len].scan_hint = $6;
                ++ltable->len;
            }
    |    join_table_commalist RIGHT_SYM opt_outer JOIN_SYM table_ref opt_scan_hint ON_SYM search_condition
            {
                T_LIST_JOINTABLEDESC *ltable;

                INCOMPLETION("RIGHT OUTER JOIN");

                ltable = &vPLAN(Yacc)->querydesc.fromlist;
                CHECK_ARRAY_BY_LIMIT(Yacc->stmt, ltable,
                        sizeof(T_JOINTABLEDESC), MAX_JOIN_TABLE_NUM);
                ltable->outer_join_exists = 1;
                ltable->list[ltable->len].join_type = SJT_RIGHT_OUTER_JOIN;
                sc_memcpy(&(ltable->list[ltable->len].condition),
                        $8, sizeof(T_EXPRESSIONDESC));
                ltable->list[ltable->len].table     = $5;
                ltable->list[ltable->len].scan_hint = $6;
                ++ltable->len;
            }
    |    join_table_commalist FULL_SYM opt_outer JOIN_SYM table_ref opt_scan_hint ON_SYM search_condition
            {
                INCOMPLETION("FULL OUTER JOIN");
            }
    |    '(' join_table_commalist ')'
    ;

cross_join:
        ','
    |    JOIN_SYM
    |    CROSS_SYM JOIN_SYM
    ;

opt_outer:
        /* empty */
    |    OUTER_SYM
    ;

table_ref:
        table
            {
                $$.tablename = $1;
                $$.aliasname = NULL;
                $$.correlated_tbl = NULL;
            }
    |    table opt_ansi_style table_alias_name
            {
                $$.tablename = $1;
                $$.aliasname = $3;
                $$.correlated_tbl = NULL;
            }
    |   '(' select_body ')' opt_ansi_style table_alias_name table_column_alias
            {
                T_SELECT *current_tst;

                $$.tablename = $5;
                $$.aliasname = NULL;
                $$.correlated_tbl = Yacc->_select;
                /* kindofwTable의 용도에 맞는 사용인가 ? 몰러 .. */
                $$.correlated_tbl->kindofwTable = iTABLE_SUBSELECT;

                /* if the number of element in columnalias_commalist is not
                 * equal to Yacc->_select.selection.len, return error.
                 * selection.list[Yacc->selection.len].res_name has already
                 * set
                 * with ...
                 */

                /* pop T_SELECT node */
                /* probably not select query */
                if(Yacc->select_top == NULL)
                    __yyerror("subquery");
                if((T_SELECT *)Yacc->select_top->ptr == NULL)
                    __yyerror("subquery");
                Yacc->_select = (T_SELECT *)Yacc->select_top->ptr;
                Yacc->select_top = Yacc->select_top->next;

                /* construct T_SELECT tree */
                current_tst = Yacc->_select;
                if(current_tst->first_sub == NULL) {
                    current_tst->first_sub = $$.correlated_tbl;
                } else {
                    current_tst = current_tst->first_sub;
                    /* insert into the last position. how about the first ? */
                    while(current_tst->sibling_query)
                        current_tst = current_tst->sibling_query;
                    current_tst->sibling_query = $$.correlated_tbl;
                }
            }
    ;

table_column_alias:
        /* empty */
        | '(' columnalias_commalist ')'
    ;

columnalias_commalist:
        columnalias_item
        {
            int idx;
            idx = vSELECT(Yacc)->queryresult.len;

            if (vPLAN(Yacc)->querydesc.selection.len > 0 && idx == 0)
            {
                vSELECT(Yacc)->queryresult.list =
                    sql_mem_alloc(vSQL(Yacc)->parsing_memory,
                            sizeof(T_RESULTCOLUMN)
                            *(vPLAN(Yacc)->querydesc.selection.len+1));
            }
            else if (vPLAN(Yacc)->querydesc.selection.len == 0 &&
                    (idx == 0 || (idx + 1) > vSELECT(Yacc)->queryresult.max))
            {
                CHECK_ARRAY(Yacc->stmt, &(vSELECT(Yacc)->queryresult),
                        sizeof(T_RESULTCOLUMN));
            }

            if (vPLAN(Yacc)->querydesc.selection.len != 0
                    && idx >= vPLAN(Yacc)->querydesc.selection.len)
            {
                __yyerror("selection length and the number of column aliases"
                        "of derived table doesn't match.");
            }

            __CHECK_FIELD_NAME_LENG($1);
            sc_strncpy(vSELECT(Yacc)->queryresult.list[idx].res_name, $1,
                    FIELD_NAME_LENG-1);
            vSELECT(Yacc)->queryresult.list[idx].res_name[FIELD_NAME_LENG-1] =
                '\0';

            ++vSELECT(Yacc)->queryresult.len;
        }
    |   columnalias_commalist ',' columnalias_item
        {
            int idx;
            idx = vSELECT(Yacc)->queryresult.len;

            if (vPLAN(Yacc)->querydesc.selection.len == 0 &&
                    (idx == 0 || (idx + 1) > vSELECT(Yacc)->queryresult.max))
            {
                CHECK_ARRAY(Yacc->stmt, &(vSELECT(Yacc)->queryresult),
                        sizeof(T_RESULTCOLUMN));
            }

            if (vPLAN(Yacc)->querydesc.selection.len != 0 
                    && idx >= vPLAN(Yacc)->querydesc.selection.len)
            {
                __yyerror("selection length and the number of column aliases"
                        "of derived table doesn't match.");
            }

            __CHECK_FIELD_NAME_LENG($3);
            sc_strncpy(vSELECT(Yacc)->queryresult.list[idx].res_name, $3,
                    FIELD_NAME_LENG-1);
            vSELECT(Yacc)->queryresult.list[idx].res_name[FIELD_NAME_LENG-1] =
                '\0';

            ++vSELECT(Yacc)->queryresult.len;
        }
    ;

columnalias_item:
        IDENTIFIER
            {
                __CHECK_ALIAS_NAME_LENG($1);
                $$ = $1;
            }
    ;

opt_scan_hint:
        /* empty */
            {
                $$.scan_type = SMT_NONE;
                $$.indexname = NULL;
                $$.fields.len = 0;
                $$.fields.max = 0;
                $$.fields.list = NULL;
            }
    |    SSCAN_SYM
            {
                $$.scan_type = SMT_SSCAN;
                $$.indexname = NULL;
                $$.fields.len = 0;
                $$.fields.max = 0;
                $$.fields.list = NULL;
            }
    |    ISCAN_SYM PRIMARY_SYM
            {
                $$.scan_type = SMT_ISCAN;
                $$.indexname = NULL;
                $$.fields.len = 0;
                $$.fields.max = 0;
                $$.fields.list = NULL;
            }
    |    ISCAN_SYM index
            {
                $$.scan_type = SMT_ISCAN;
                $$.indexname = $2;
                $$.fields.len = 0;
                $$.fields.max = 0;
                $$.fields.list = NULL;
            }
    |    ISCAN_SYM '(' iscan_hint_column_commalist ')'
            {
                $$.scan_type = SMT_ISCAN;
                $$.indexname = NULL;
                $$.fields.len = 0;
                $$.fields.max = 0;
                $$.fields.list = NULL;
                $$.fields = $3;
            }
    ;

iscan_hint_column_commalist:
/*        column */
        index_column
            {
                /* sc_memset(&$$, 0, sizeof(T_LIST_FIELDS)); */
                sc_memset(&$$, 0, sizeof(T_IDX_LIST_FIELDS));
                /* CHECK_ARRAY(Yacc->stmt, &$$, sizeof(char*)); */
                CHECK_ARRAY(Yacc->stmt, &$$, sizeof(T_IDX_FIELD));
                $$.list[$$.len++] = $1;
            }
/*    |    iscan_hint_column_commalist ',' column */
    |    iscan_hint_column_commalist ',' index_column 
            {
                /* CHECK_ARRAY(Yacc->stmt, &$$, sizeof(char*)); */
                CHECK_ARRAY(Yacc->stmt, &$$, sizeof(T_IDX_FIELD));
                $$.list[$$.len++] = $3;
            }
    ;

opt_where_clause:
        /* empty */
            {
                switch (vSQL(Yacc)->sqltype) {
                    case ST_DELETE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vDELETE(Yacc)->subselect)
                          sc_memset(&vPLAN(Yacc)->querydesc.condition,
                                0, sizeof(T_EXPRESSIONDESC));
                        else
                        sc_memset(&vDELETE(Yacc)->planquerydesc.querydesc.condition,
                                0, sizeof(T_EXPRESSIONDESC));
                        break;
                    case ST_SELECT:
                    case ST_CREATE: /* for view creation */
                        sc_memset(&vPLAN(Yacc)->querydesc.condition,
                                0, sizeof(T_EXPRESSIONDESC));
                        break;
                    case ST_UPDATE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vUPDATE(Yacc)->subselect)
                          sc_memset(&vPLAN(Yacc)->querydesc.condition,
                                0, sizeof(T_EXPRESSIONDESC));
                        else
                        sc_memset(&vUPDATE(Yacc)->planquerydesc.querydesc.condition,
                                0, sizeof(T_EXPRESSIONDESC));
                        break;
                    default:
                        break;
                }
            }
    |    where_clause
            {
                switch (vSQL(Yacc)->sqltype) {
                    case ST_DELETE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vDELETE(Yacc)->subselect)
                            sc_memcpy(&vPLAN(Yacc)->querydesc.condition,
                                $1, sizeof(T_EXPRESSIONDESC));
                        else
                        sc_memcpy(&vDELETE(Yacc)->planquerydesc.querydesc.condition,
                                $1, sizeof(T_EXPRESSIONDESC));
                        break;
                    case ST_SELECT:
                    case ST_CREATE: /* for view creation */
                        sc_memcpy(&vPLAN(Yacc)->querydesc.condition,
                                $1, sizeof(T_EXPRESSIONDESC));
                        break;
                    case ST_UPDATE:
                        {
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vUPDATE(Yacc)->subselect)
                            sc_memcpy(&vPLAN(Yacc)->querydesc.condition,
                                $1, sizeof(T_EXPRESSIONDESC));
                        else
                        sc_memcpy(&vUPDATE(Yacc)->planquerydesc.querydesc.condition,
                                $1, sizeof(T_EXPRESSIONDESC));
                        break;
                        }
                    default:
                        break;
                }
            }
    ;

where_clause:
        WHERE_SYM search_condition
            {
                $$ = $2;
            }
    ;

opt_group_by_clause:
        /* empty */
    |    GROUP_SYM BY_SYM group_by_item_commalist opt_having_clause
            {
                vPLAN(Yacc)->querydesc.groupby = Yacc->groupby;
                sc_memset(&(Yacc->groupby), 0, sizeof(T_GROUPBYDESC));
                if ($4) {
                    sc_memcpy(&vPLAN(Yacc)->querydesc.groupby.having,
                            $4, sizeof(T_EXPRESSIONDESC));
                }
            }
    ;

group_by_item_commalist:
        group_by_item
    |    group_by_item_commalist ',' group_by_item
    ;

group_by_item:
        column_ref
            {
                char *attr;

                CHECK_ARRAY(Yacc->stmt,&(Yacc->groupby),sizeof(T_GROUPBYITEM));

                attr = sc_strchr($1, '.');
                if (attr) {
                    sc_strncpy(tablename, $1, attr-$1);
                    tablename[attr++-$1] = '\0';
                    Yacc->groupby.list[Yacc->groupby.len].attr.table.tablename
                            = YACC_XSTRDUP(Yacc, tablename);
                    isnull(Yacc->groupby.list[Yacc->groupby.len].attr.table.tablename);
                    Yacc->groupby.list[Yacc->groupby.len].attr.table.aliasname
                            = NULL;
                    Yacc->groupby.list[Yacc->groupby.len].attr.attrname
                            = YACC_XSTRDUP(Yacc, attr);
                    isnull(Yacc->groupby.list[Yacc->groupby.len].attr.attrname);
                } 
                else 
                {
                    Yacc->groupby.list[Yacc->groupby.len].attr.attrname = $1;
                }
                Yacc->groupby.list[Yacc->groupby.len].s_ref = -1;
                Yacc->groupby.len++;
            }
    |    INTNUM
            {

                CHECK_ARRAY(Yacc->stmt,&(Yacc->groupby),sizeof(T_GROUPBYITEM));

                if (vPLAN(Yacc)->querydesc.selection.len < (int)$1) 
                    __yyerror("Invalid selection item index");

                expr = &vPLAN(Yacc)->querydesc.selection.list[$1-1].expr;
                if (expr->len <= 0)
                    INCOMPLETION("not allowed to group an expression");

                if (expr->list[0]->u.value.attrdesc.table.tablename) {
                    Yacc->groupby.list[Yacc->groupby.len].attr.table.tablename
                            = YACC_XSTRDUP(Yacc,
                                expr->list[0]->u.value.attrdesc.table.tablename);
                    isnull(Yacc->groupby.list[Yacc->groupby.len].attr.table.tablename);
                    Yacc->groupby.list[Yacc->groupby.len].attr.table.aliasname
                            = NULL;
                    Yacc->groupby.list[Yacc->groupby.len].attr.attrname
                            = YACC_XSTRDUP(Yacc,
                                expr->list[0]->u.value.attrdesc.attrname);
                } else {
                    if (!expr->list[0]->u.value.attrdesc.attrname)
                        __yyerror("Invalid selection item index");
                    Yacc->groupby.list[Yacc->groupby.len].attr.attrname
                            = YACC_XSTRDUP(Yacc,
                            expr->list[0]->u.value.attrdesc.attrname);
                }

                isnull(Yacc->groupby.list[Yacc->groupby.len].attr.attrname);

                Yacc->groupby.list[Yacc->groupby.len].s_ref = (int)$1-1;
                Yacc->groupby.len++;
            }
    ;

opt_having_clause:
        /* empty */
            {
                $$ = NULL;
            }
    |    HAVING_SYM search_condition
            {
                $$ = $2;
            }
    ;

opt_order_by_clause:
        /* empty */
    |    ORDER_SYM BY_SYM order_by_item_commalist
            {
                vPLAN(Yacc)->querydesc.orderby = Yacc->orderby;
                sc_memset(&(Yacc->orderby), 0, sizeof(T_ORDERBYDESC));
            }
    ;

opt_limit_clause:
        /* empty */
    { }
    |    LIMIT_SYM limit_num4rows
    |    LIMIT_SYM limit_rid4offset
    |    LIMIT_SYM limit_num4offset ',' limit_num4rows
    |    LIMIT_SYM limit_rid4offset ',' limit_num4rows
    ;

limit_rid4offset:
        AT_SYM INTNUM
            {
                switch (vSQL(Yacc)->sqltype) {
                    case ST_DELETE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vDELETE(Yacc)->subselect)
                        {
                            vPLAN(Yacc)->querydesc.limit.start_at = (OID)$2;
                        }
                        else
                        {
                            __yyerror("limit @rid not allowed delete query.");
                        }
                        break;
                    case ST_SELECT:
                    case ST_CREATE: /* for view creation */
                        vPLAN(Yacc)->querydesc.limit.start_at = (OID)$2;
                        break;
                    case ST_UPDATE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vUPDATE(Yacc)->subselect)
                        {
                            vPLAN(Yacc)->querydesc.limit.start_at = (OID)$2;
                        }
                        else
                        {
                            __yyerror("limit @rid not allowed update query.");
                        }
                        break;
                    default:
                        break;
                }
            }
        | AT_SYM '?' parameter_binding_dummy
            {
                switch (vSQL(Yacc)->sqltype) {
                    case ST_DELETE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vDELETE(Yacc)->subselect)
                        {
                            vPLAN(Yacc)->querydesc.limit.startat_param_idx =
                                ++Yacc->param_count;
                        }
                        else
                        {
                            vDELETE(Yacc)->planquerydesc.querydesc.limit.startat_param_idx = 
                                ++Yacc->param_count;
                        }
                        break;
                    case ST_SELECT:
                    case ST_CREATE: /* for view creation */
                        vPLAN(Yacc)->querydesc.limit.startat_param_idx = 
                            ++Yacc->param_count;
                        break;
                    case ST_UPDATE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vUPDATE(Yacc)->subselect)
                        {
                            vPLAN(Yacc)->querydesc.limit.startat_param_idx = 
                                ++Yacc->param_count;
                        }
                        else
                        {
                            vUPDATE(Yacc)->planquerydesc.querydesc.limit.startat_param_idx = 
                                ++Yacc->param_count;
                        }
                        break;
                    default:
                        break;
                }
            }
    ;

limit_num4offset:
        INTNUM
            {
                switch (vSQL(Yacc)->sqltype) {
                    case ST_DELETE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vDELETE(Yacc)->subselect)
                          vPLAN(Yacc)->querydesc.limit.offset = (int)$1;
                        else
                        __yyerror("limit offset not allowed delete query.");
                        break;
                    case ST_SELECT:
                    case ST_CREATE: /* for view creation */
                        vPLAN(Yacc)->querydesc.limit.offset = (int)$1;
                        break;
                    case ST_UPDATE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vUPDATE(Yacc)->subselect)
                          vPLAN(Yacc)->querydesc.limit.offset = (int)$1;
                        else
                        __yyerror("limit offset not allowed update query.");
                        break;
                    default:
                        break;
                }
            }
        | '?' parameter_binding_dummy
            {
                switch (vSQL(Yacc)->sqltype) {
                    case ST_DELETE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vDELETE(Yacc)->subselect)
                          vPLAN(Yacc)->querydesc.limit.offset_param_idx = ++Yacc->param_count;
                        else
                        vDELETE(Yacc)->planquerydesc.querydesc.limit.offset_param_idx = ++Yacc->param_count;
                        break;
                    case ST_SELECT:
                    case ST_CREATE: /* for view creation */
                        vPLAN(Yacc)->querydesc.limit.offset_param_idx = ++Yacc->param_count;
                        break;
                    case ST_UPDATE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vUPDATE(Yacc)->subselect)
                          vPLAN(Yacc)->querydesc.limit.offset_param_idx = ++Yacc->param_count;
                        else
                        vUPDATE(Yacc)->planquerydesc.querydesc.limit.offset_param_idx = ++Yacc->param_count;
                        break;
                    default:
                        break;
                }
            }
        ;

limit_num4rows:
        INTNUM
            {
                switch (vSQL(Yacc)->sqltype) {
                    case ST_DELETE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vDELETE(Yacc)->subselect)
                          vPLAN(Yacc)->querydesc.limit.rows = (int)$1;
                        else
                        vDELETE(Yacc)->planquerydesc.querydesc.limit.rows = (int)$1;
                        break;
                    case ST_SELECT:
                    case ST_CREATE: /* for view creation */
                        vPLAN(Yacc)->querydesc.limit.rows = (int)$1;
                        break;
                    case ST_UPDATE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vUPDATE(Yacc)->subselect)
                          vPLAN(Yacc)->querydesc.limit.rows = (int)$1;
                        else
                        vUPDATE(Yacc)->planquerydesc.querydesc.limit.rows = (int)$1;
                        break;
                    default:
                        break;
                }
            }
        | '?' parameter_binding_dummy
            {
                switch (vSQL(Yacc)->sqltype) {
                    case ST_DELETE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vDELETE(Yacc)->subselect)
                          vPLAN(Yacc)->querydesc.limit.rows_param_idx = ++Yacc->param_count;
                        else
                        vDELETE(Yacc)->planquerydesc.querydesc.limit.rows_param_idx = ++Yacc->param_count;
                        break;
                    case ST_SELECT:
                    case ST_CREATE: /* for view creation */
                        vPLAN(Yacc)->querydesc.limit.rows_param_idx = ++Yacc->param_count;
                        break;
                    case ST_UPDATE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vUPDATE(Yacc)->subselect)
                          vPLAN(Yacc)->querydesc.limit.rows_param_idx = ++Yacc->param_count;
                        else
                        vUPDATE(Yacc)->planquerydesc.querydesc.limit.rows_param_idx = ++Yacc->param_count;
                        break;
                    default:
                        break;
                }
            }
        ;


order_by_item_commalist:
        order_by_item
    |    order_by_item_commalist ',' order_by_item
    ;

order_by_item:
        column_ref collation_info order_info
            {
                char *attr;

                CHECK_ARRAY(Yacc->stmt,&(Yacc->orderby),sizeof(T_ORDERBYITEM));

                attr = sc_strchr($1, '.');
                if (attr) {
                    sc_strncpy(tablename, $1, attr-$1);
                    tablename[attr++-$1] = '\0';
                    Yacc->orderby.list[Yacc->orderby.len].attr.table.tablename =
                            YACC_XSTRDUP(Yacc, tablename);
                    isnull(Yacc->orderby.list[Yacc->orderby.len].attr.table.tablename);
                    Yacc->orderby.list[Yacc->orderby.len].attr.table.aliasname =
                            NULL;
                    Yacc->orderby.list[Yacc->orderby.len].attr.attrname =
                            YACC_XSTRDUP(Yacc, attr);
                    isnull(Yacc->orderby.list[Yacc->orderby.len].attr.attrname);
                } 
                else 
                {
                    Yacc->orderby.list[Yacc->orderby.len].attr.attrname  = $1;
                }

                Yacc->orderby.list[Yacc->orderby.len].attr.collation = $2;
                Yacc->orderby.list[Yacc->orderby.len].ordertype = $3;
                Yacc->orderby.list[Yacc->orderby.len].s_ref      = -1;
                Yacc->orderby.list[Yacc->orderby.len].param_idx  = -1;
                Yacc->orderby.len++;
            }
    |    INTNUM order_info
            {
                if ((int)$1 == 0)
                    __yyerror("Invalid selection item index");

                if (vSELECT(Yacc)->planquerydesc.querydesc.setlist.len == 0 
                        && vSELECT(Yacc)->kindofwTable != iTABLE_SUBSET)
                {
                    if (vPLAN(Yacc)->querydesc.selection.len > 0) 
                    {
                        if (vPLAN(Yacc)->querydesc.selection.len < (int)$1) 
                            __yyerror("Invalid selection item index");

                        expr = &vPLAN(Yacc)->querydesc.selection.list[$1-1].expr;
                        if (expr->len <= 0)
                            INCOMPLETION("not allowed to order an expression");

                        CHECK_ARRAY(Yacc->stmt,&(Yacc->orderby),sizeof(T_ORDERBYITEM));

                        for (i = 0; i < expr->len; i++)
                        {
                            if (expr->list[i]->elemtype == SPT_VALUE &&
                                    expr->list[i]->u.value.valueclass == SVC_VARIABLE)
                                break;
                        }

                        if (i != expr->len)
                        {
                            if (expr->list[i]->u.value.attrdesc.table.tablename) {
                                Yacc->orderby.list[Yacc->orderby.len].attr.table.tablename =
                                    YACC_XSTRDUP(Yacc,
                                            expr->list[i]->u.value.attrdesc.table.tablename);
                                isnull(Yacc->orderby.list[Yacc->orderby.len].attr.table.tablename);
                                Yacc->orderby.list[Yacc->orderby.len].attr.table.aliasname =
                                    NULL;
                                Yacc->orderby.list[Yacc->orderby.len].attr.attrname = 
                                    YACC_XSTRDUP(Yacc,
                                            expr->list[i]->u.value.attrdesc.attrname);
                            } else {
                                if (!expr->list[i]->u.value.attrdesc.attrname)
                                    __yyerror("Invalid selection item index");
                                Yacc->orderby.list[Yacc->orderby.len].attr.attrname = 
                                    YACC_XSTRDUP(Yacc,
                                            expr->list[i]->u.value.attrdesc.attrname);
                            }
                            isnull(Yacc->orderby.list[Yacc->orderby.len].attr.attrname);
                        }
                    }
                    else
                        CHECK_ARRAY(Yacc->stmt,&(Yacc->orderby),sizeof(T_ORDERBYITEM));
                }
                else
                    CHECK_ARRAY(Yacc->stmt,&(Yacc->orderby),sizeof(T_ORDERBYITEM));

                Yacc->orderby.list[Yacc->orderby.len].ordertype = $2;
                Yacc->orderby.list[Yacc->orderby.len].s_ref      = (int)$1-1;
                Yacc->orderby.list[Yacc->orderby.len].param_idx  = -1;
                Yacc->orderby.len++;
            }
    |    '?' parameter_binding_dummy order_info
            {
                CHECK_ARRAY(Yacc->stmt,&(Yacc->orderby),sizeof(T_ORDERBYITEM));
                Yacc->orderby.list[Yacc->orderby.len].param_idx =
                    ++Yacc->param_count;
                Yacc->orderby.list[Yacc->orderby.len].ordertype = $3;
                Yacc->orderby.len++;
            }
    ;
        
collation_info: 
    {
        $$ = MDB_COL_NONE;
    }
    | collation_info_str
    {
        $$ = $1;
    }
    ;

collation_info_str:
    COLLATION_SYM IDENTIFIER
    {
        $$ = db_find_collation($2);
        if ($$ == MDB_COL_NONE)
        {
            __yyerror("invalid collation's type name");
        }
    }
    ;

order_info:
    /* empty */
    {
        $$  = 'A';
    }
    | order_info_str
    {
        $$  = $1;
    }
    ;

order_info_str:
    ASC_SYM 
    {
        $$ = 'A';
    }
    | DESC_SYM
    {
        $$ = 'D';
    } 
    ;

    
    /* search conditions */

search_condition:
        search_condition OR_SYM search_condition
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_OR;
                elem.u.op.argcnt = 2;

                $$ = merge_expression(Yacc, $1, $3, &elem);
            }
    |    search_condition AND_SYM search_condition
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_AND;
                elem.u.op.argcnt = 2;

                $$ = merge_expression(Yacc, $1, $3, &elem);
            }
    |    NOT_SYM search_condition
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_NOT;
                elem.u.op.argcnt = 1;

                $$ = merge_expression(Yacc, NULL, $2, &elem);
            }
    |    '(' search_condition ')'
            {
                $$ = $2;
            }
    |    predicate
            {
                $$ = $1;
            }
    ;

predicate:
        comparison_predicate
            {
                $$ = $1;
            }
    |    between_predicate
            {
                $$ = $1;
            }
    |    like_predicate
            {
                $$ = $1;
            }
    |    test_for_null
            {
                $$ = $1;
            }
    |    in_predicate
            {
                $$ = $1;
            }
    |    all_or_any_predicate
            {
                $$ = $1;
            }
    |    existence_test
            {
                $$ = $1;
            }
    |    special_function_predicate
            {
                $$ = $1;
            }
    ;

comparison_predicate:
        scalar_exp COMPARISON scalar_exp
            {
                if ($3->len == 1
                        && ($3->list[0]->u.value.is_null == 1
                            && !$3->list[0]->u.value.param_idx))
                {
                    if ($2.type == SOT_EQ) 
                    {
                        sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                        elem.elemtype    = SPT_OPERATOR;
                        elem.u.op.type   = SOT_ISNULL;
                        elem.u.op.argcnt = 1;

                        $$ = merge_expression(Yacc, $1, NULL, &elem);
                    }
                    else if ($2.type == SOT_NE)
                    {
                        sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));

                        elem.elemtype    = SPT_OPERATOR;
                        elem.u.op.type   = SOT_ISNOTNULL;
                        elem.u.op.argcnt = 1;

                        $$ = merge_expression(Yacc, $1, NULL, &elem);

                    }
                    else
                    {
                        sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                        elem.elemtype = SPT_OPERATOR;
                        elem.u.op     = $2;

                        $$ = merge_expression(Yacc, $1, $3, &elem);
                    }
                }
                else
                {
                    sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                    elem.elemtype = SPT_OPERATOR;
                    elem.u.op     = $2;

                    $$ = merge_expression(Yacc, $1, $3, &elem);
                }
            }
    |    scalar_exp IS_SYM NULL_SYM
         {
            sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
            elem.elemtype    = SPT_OPERATOR;
            elem.u.op.type   = SOT_ISNULL;
            elem.u.op.argcnt = 1;

            $$ = merge_expression(Yacc, $1, NULL, &elem);
         } 
    |    scalar_exp IS_SYM NOT_SYM NULL_SYM
         {
            sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
            elem.elemtype    = SPT_OPERATOR;
            elem.u.op.type   = SOT_ISNOTNULL;
            elem.u.op.argcnt = 1;

            $$ = merge_expression(Yacc, $1, NULL, &elem);
         } 
        | column_ref_or_pvar
            {
                T_EXPRESSIONDESC *rexpr;

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_BIGINT;
                elem.u.value.u.i64      = 0;
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;

                rexpr = get_expr_with_item(Yacc, &elem);


                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_OPERATOR;
                elem.u.op.type   = SOT_NE;
                elem.u.op.argcnt = 2;

                $$ = merge_expression(Yacc, $1, rexpr, &elem);
            } 
    ;

between_predicate:
        scalar_exp NOT_SYM BETWEEN_SYM scalar_exp AND_SYM scalar_exp
            {
                T_POSTFIXELEM *elist;

                // merge_expression()내부에서 $1을 free()시키므로
                // clon expression을 하나 가지고 있어야 한다.
                expr = sql_mem_get_free_expressiondesc(vSQL(Yacc)->parsing_memory, $1->len, $1->max);
                if (!expr) 
                    return RET_ERROR;

                for (i = 0; i < expr->len; ++i)
                    sc_memcpy(expr->list[i], $1->list[i], sizeof(T_POSTFIXELEM));

                for (i = 0; i < expr->len; i++) {
                    elist = expr->list[i];
                    if (elist->elemtype == SPT_VALUE) 
                    {
                        if (elist->u.value.valueclass == SVC_VARIABLE) {
                            if (elist->u.value.attrdesc.table.tablename) {
                                elist->u.value.attrdesc.table.tablename =
                                YACC_XSTRDUP(Yacc,
                                    elist->u.value.attrdesc.table.tablename);
                              isnull(elist->u.value.attrdesc.table.tablename);
                              elist->u.value.attrdesc.table.aliasname = NULL;
                            }
                            if (elist->u.value.attrdesc.attrname) {
                                elist->u.value.attrdesc.attrname =
                                    YACC_XSTRDUP(Yacc,
                                        elist->u.value.attrdesc.attrname);
                                isnull(elist->u.value.attrdesc.attrname);
                            }
                        }
                        if (elist->u.value.valueclass == SVC_CONSTANT &&
                            IS_BS_OR_NBS(elist->u.value.valuetype))
                        {
                            if (elist->u.value.u.ptr)
                            {
                                sql_value_ptrStrdup(&(elist->u.value), elist->u.value.u.ptr, 1);
                            }
                        }
                    }
                }

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_GE;
                elem.u.op.argcnt = 2;
                $$ = merge_expression(Yacc, $1, $4, &elem);

                $$ = merge_expression(Yacc, $$, expr, NULL);

                elem.u.op.type   = SOT_LE;
                elem.u.op.argcnt = 2;
                $$ = merge_expression(Yacc, $$, $6, &elem);

                elem.u.op.type   = SOT_AND;
                elem.u.op.argcnt = 2;
                $$ = merge_expression(Yacc, $$, NULL, &elem);

                elem.u.op.type   = SOT_NOT;
                elem.u.op.argcnt = 1;
                $$ = merge_expression(Yacc, $$, NULL, &elem);
            }
    |    scalar_exp BETWEEN_SYM scalar_exp AND_SYM scalar_exp
            {
                T_POSTFIXELEM *elist;

                expr = sql_mem_get_free_expressiondesc(vSQL(Yacc)->parsing_memory, $1->len, $1->max);
                if (!expr)
                    return RET_ERROR;

                for(i = 0; i < $1->len; ++i)
                    sc_memcpy(expr->list[i], $1->list[i], sizeof(T_POSTFIXELEM));

                for (i = 0; i < expr->len; i++) {
                    elist = expr->list[i];
                    if (elist->elemtype == SPT_VALUE)
                    {
                        if (elist->u.value.valueclass == SVC_VARIABLE) {
                            if (elist->u.value.attrdesc.table.tablename) {
                                elist->u.value.attrdesc.table.tablename =
                                    YACC_XSTRDUP(Yacc,
                                       elist->u.value.attrdesc.table.tablename);
                                isnull(elist->u.value.attrdesc.table.tablename);
                                elist->u.value.attrdesc.table.aliasname = NULL;
                            }
                            if (elist->u.value.attrdesc.attrname) {
                                elist->u.value.attrdesc.attrname =
                                    YACC_XSTRDUP(Yacc,
                                            elist->u.value.attrdesc.attrname);
                                isnull(elist->u.value.attrdesc.attrname);
                            }
                        }
                        if (elist->u.value.valueclass == SVC_CONSTANT &&
                            IS_BS_OR_NBS(elist->u.value.valuetype))
                        {
                            if (elist->u.value.u.ptr)
                            {
                                sql_value_ptrStrdup(&(elist->u.value), elist->u.value.u.ptr, 1);
                            }
                        }
                    }
                }

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_GE;
                elem.u.op.argcnt = 2;
                $$ = merge_expression(Yacc, $1, $3, &elem);

                $$ = merge_expression(Yacc, $$, expr, NULL);

                elem.u.op.type   = SOT_LE;
                elem.u.op.argcnt = 2;
                $$ = merge_expression(Yacc, $$, $5, &elem);

                elem.u.op.type   = SOT_AND;
                elem.u.op.argcnt = 2;
                $$ = merge_expression(Yacc, $$, NULL, &elem);
            }
    ;

like_predicate:
        scalar_exp NOT_SYM like_op scalar_exp opt_escape
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = $3;
                elem.u.op.argcnt = 2;

                $$ = merge_expression(Yacc, $1, $4, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_NOT;
                elem.u.op.argcnt = 1;

                $$ = merge_expression(Yacc, $$, NULL, &elem);
            }
    |    scalar_exp like_op scalar_exp opt_escape
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = $2;
                elem.u.op.argcnt = 2;

                $$ = merge_expression(Yacc, $1, $3, &elem);
            }
    ;

like_op:
    LIKE_SYM
    {
        $$ = SOT_LIKE;
    }
    | ILIKE_SYM
    {
        $$ = SOT_ILIKE;
    }
    ;

opt_escape:
        /* empty */
    |    ESCAPE_SYM atom
            {
                INCOMPLETION("ESCAPE");
            }
    ;

test_for_null:
        column_ref IS_SYM NOT_SYM NULL_SYM
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_VARIABLE;
                elem.u.value.valuetype  = DT_NULL_TYPE;

                p = sc_strchr($1, '.');
                if (p == NULL) {
                    elem.u.value.attrdesc.table.tablename = NULL;
                    elem.u.value.attrdesc.table.aliasname = NULL;
                    elem.u.value.attrdesc.attrname  = $1;
                }
                else {
                    *p++ = '\0';
                    elem.u.value.attrdesc.table.tablename =
                        YACC_XSTRDUP(Yacc, $1);
                    elem.u.value.attrdesc.table.aliasname = NULL;
                    elem.u.value.attrdesc.attrname  = YACC_XSTRDUP(Yacc, p);

                    isnull(elem.u.value.attrdesc.table.tablename);
                    isnull(elem.u.value.attrdesc.attrname);

                }

                $$ = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_ISNOTNULL;
                elem.u.op.argcnt = 1;

                $$ = merge_expression(Yacc, $$, NULL, &elem);
            }
    |    column_ref IS_SYM NULL_SYM
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_VARIABLE;
                elem.u.value.valuetype  = DT_NULL_TYPE;

                p = sc_strchr($1, '.');
                if (p == NULL) {
                    elem.u.value.attrdesc.table.tablename = NULL;
                    elem.u.value.attrdesc.table.aliasname = NULL;
                    elem.u.value.attrdesc.attrname  = $1;
                }
                else {
                    *p++ = '\0';
                    elem.u.value.attrdesc.table.tablename =
                        YACC_XSTRDUP(Yacc, $1);
                    elem.u.value.attrdesc.table.aliasname = NULL;
                    elem.u.value.attrdesc.attrname  = YACC_XSTRDUP(Yacc, p);
                    isnull(elem.u.value.attrdesc.table.tablename);
                    isnull(elem.u.value.attrdesc.attrname);
                }

                $$ = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_ISNULL;
                elem.u.op.argcnt = 1;

                $$ = merge_expression(Yacc, $$, NULL, &elem);
            }
    ;

in_predicate:
         scalar_exp NOT_SYM IN_SYM subquery_exp
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_SUBQ_OP;
                elem.u.op.type   = SOT_NIN;
                elem.u.op.argcnt = 2;

                $$ = merge_expression(Yacc, $1, $4, &elem);
            }
    |   scalar_exp IN_SYM subquery_exp
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_SUBQ_OP;
                elem.u.op.type   = SOT_IN;
                elem.u.op.argcnt = 2;

                $$ = merge_expression(Yacc, $1, $3, &elem);
            }
    |    scalar_exp NOT_SYM IN_SYM '(' atom_commalist ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_NIN;
                elem.u.op.argcnt = $5->len+1;

                $$ = merge_expression(Yacc, $1, $5, &elem);
            }
    |    scalar_exp IN_SYM '(' atom_commalist ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_IN;
                elem.u.op.argcnt = $4->len+1;

                $$ = merge_expression(Yacc, $1, $4, &elem);
            }
    ;

atom_commalist:
        atom
            {
                $$ = get_expr_with_item(Yacc, &$1);
            }
    |    atom_commalist ',' atom
            {
                $$ = merge_expression(Yacc, $1, NULL, &$3);
            }
    ;

all_or_any_predicate:
        scalar_exp COMPARISON any_all_some subquery_exp
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_SUBQ_OP;
                if($3 == SOT_SOME) {
                    switch($2.type) {
                    case SOT_LT:
                    case SOT_LE:
                    case SOT_GT:
                    case SOT_GE:
                    case SOT_EQ:
                    case SOT_NE:
                        elem.u.op.type = SOT_SOME_LT + ($2.type - SOT_LT);
                        break;
                    default:
                        __yyerror("invalid op type");
                        break;
                    }
                } else if($3 == SOT_ALL) {
                    switch($2.type) {
                    case SOT_LT:
                    case SOT_LE:
                    case SOT_GT:
                    case SOT_GE:
                    case SOT_EQ:
                    case SOT_NE:
                        elem.u.op.type = SOT_ALL_LT + ($2.type - SOT_LT);
                        break;
                    default:
                        __yyerror("invalid op type");
                        break;
                    }
                } else {
                    __yyerror("any_all_some rule returned wrong value");
                }
                elem.u.op.argcnt = 2;   /* 순서대로 scalar_exp, comp, subq */

                $$ = merge_expression(Yacc, $1, $4, &elem);
            }
    ;

any_all_some:
        ANY_SYM
            {
                $$ = SOT_SOME;
            }
    |   ALL_SYM
            {
                $$ = SOT_ALL;
            }
    |   SOME_SYM
            {
                $$ = SOT_SOME;
            }
    ;

existence_test:
        EXISTS_SYM subquery_exp
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_SUBQ_OP;
                elem.u.op.type   = SOT_EXISTS;
                elem.u.op.argcnt = 1;

                $$ = merge_expression(Yacc, $2, NULL, &elem);
            }
    ;

subquery_exp:
        '(' select_body ')'
            {
                T_SELECT *current_tst;

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_SUBQUERY;
                elem.u.subq   = Yacc->_select;
                /* kindofwTable의 용도에 맞는 사용인가 ? 몰러 .. */
                elem.u.subq->kindofwTable = iTABLE_SUBWHERE;

                /* if the number of element in columnalias_commalist is not
                 * equal to Yacc->_select.selection.len, return error.
                 * selection.list[Yacc->selection.len].res_name has already
                 * set
                 * with ...
                 */

                /* pop T_SELECT node */
                /* not select query ? */
                if(Yacc->select_top == NULL)
                    __yyerror("subquery");
                if((T_SELECT *)Yacc->select_top->ptr == NULL)
                    __yyerror("subquery");
                Yacc->_select = (T_SELECT *)Yacc->select_top->ptr;
                Yacc->select_top = Yacc->select_top->next;

                /* construct T_SELECT tree */
                current_tst = Yacc->_select;
                if(current_tst->first_sub == NULL) {
                    current_tst->first_sub = elem.u.subq;
                } else {
                    current_tst = current_tst->first_sub;
                    /* insert into the last position. how about the first ? */
                    while(current_tst->sibling_query)
                        current_tst = current_tst->sibling_query;
                    current_tst->sibling_query = elem.u.subq;
                }
                $$ = get_expr_with_item(Yacc, &elem);
            }
    ;

    /* scalar expressions */

scalar_exp:
        scalar_exp '+' scalar_exp
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_PLUS;
                elem.u.op.argcnt = 2;

                $$ = merge_expression(Yacc, $1, $3, &elem);
            }
    |    scalar_exp '-' scalar_exp
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_MINUS;
                elem.u.op.argcnt = 2;

                $$ = merge_expression(Yacc, $1, $3, &elem);
            }
    |    scalar_exp '*' scalar_exp
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_TIMES;
                elem.u.op.argcnt = 2;

                $$ = merge_expression(Yacc, $1, $3, &elem);
            }
    |    scalar_exp '/' scalar_exp
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_DIVIDE;
                elem.u.op.argcnt = 2;

                $$ = merge_expression(Yacc, $1, $3, &elem);
            }
    |    scalar_exp '%' scalar_exp
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_MODULA;
                elem.u.op.argcnt = 2;

                $$ = merge_expression(Yacc, $1, $3, &elem);
            }
    |    scalar_exp '^' scalar_exp
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_EXPONENT;
                elem.u.op.argcnt = 2;

                $$ = merge_expression(Yacc, $1, $3, &elem);
            }
    |    scalar_exp MERGE scalar_exp
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_MERGE;
                elem.u.op.argcnt = 2;

                $$ = merge_expression(Yacc, $1, $3, &elem);
            }
    |    '+' scalar_exp %prec NEG
            {
                $$ = $2;
            }
    |    '-' scalar_exp %prec NEG
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_BIGINT;
                elem.u.value.u.i64      = -1;
                elem.u.value.attrdesc.collation   = MDB_COL_NUMERIC;

                $$ = merge_expression(Yacc, $2, NULL, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_TIMES;
                elem.u.op.argcnt = 2;

                $$ = merge_expression(Yacc, $$, NULL, &elem);
            }
    |    atom
            {
                $$ = get_expr_with_item(Yacc, &$1);
            }
    |    column_ref_or_pvar
            {
                $$ = $1;
            }
    |    aggregation_ref
            {
                $$ = $1;
            }
    |    function_ref
            {
                $$ = $1;
            }
    |    special_function_ref
            {
                $$ = $1;
            }
    |   subquery_exp
            {
                $$ = $1;
            }
    |    '(' scalar_exp ')'
            {
                $$ = $2;
            }
    ;

atom:
        literal
            {
                $$ = $1;
            }
    |    parameter_marker
            {
                $$ = $1;
            }
    ;

parameter_marker:
        '?' parameter_binding_dummy
            {
                sc_memset(&$$, 0, sizeof(T_POSTFIXELEM));
                $$.elemtype = SPT_VALUE;
                $$.u.value.valueclass = SVC_CONSTANT;
                $$.u.value.valuetype  = DT_NULL_TYPE;
                $$.u.value.is_null    = 1;

                /* param_idx는 1부터 시작하자.
                 * 만약, 0부터 시작하게 되면 parameter가 아닌 모든 item에 대해
                 * 항상 -1로 설정해야 하는 부담이 있다.
                 */
                $$.u.value.param_idx  = ++Yacc->param_count;
            }
    ;

parameter_binding_dummy:
    /* empty */
    {    }
    | INTNUM
    {    }
    ;
    
aggregation_ref_arg:
        DISTINCT_SYM scalar_exp
            {
                $$ = $2;
                Yacc->is_distinct = 1;
                if (sql_expr_has_ridfunc($2))
                    __yyerror("RID as aggregation's arg is not supported.");
            }
    |    ALL_SYM scalar_exp
            {
                $$ = $2;
                Yacc->is_distinct = 0;
                if (sql_expr_has_ridfunc($2))
                    __yyerror("RID as aggregation's arg is not supported.");
            }
    |    scalar_exp
            {
                $$ = $1;
                Yacc->is_distinct = 0;
                if (sql_expr_has_ridfunc($1))
                    __yyerror("RID as aggregation's arg is not supported.");
            }
    ;

aggregation_ref:
        AVG_SYM '(' aggregation_ref_arg ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_AGGREGATION;
                elem.u.aggr.type   = SAT_AVG;
                elem.u.aggr.argcnt = 1;
                elem.u.aggr.significant = $3->len;
                if (Yacc->is_distinct) {
                    Yacc->is_distinct = 0;
                    elem.is_distinct = 1;
                }

                $$ = merge_expression(Yacc, NULL, $3, &elem);
            }
    |    COUNT_SYM '(' '*' ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_AGGREGATION;
                elem.u.aggr.type   = SAT_COUNT;
                elem.u.aggr.argcnt = 0;
                elem.u.aggr.significant = 0;

                $$ = get_expr_with_item(Yacc, &elem);
            }
    |    COUNT_SYM '(' aggregation_ref_arg ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_AGGREGATION;
                elem.u.aggr.type   = SAT_COUNT;
                elem.u.aggr.argcnt = 1;
                elem.u.aggr.significant = $3->len;
                if (Yacc->is_distinct) {
                    Yacc->is_distinct = 0;
                    elem.is_distinct = 1;
                }

                $$ = merge_expression(Yacc, NULL, $3, &elem);
            }
    |    DIRTY_COUNT_SYM
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_AGGREGATION;
                elem.u.aggr.type   = SAT_DCOUNT;
                elem.u.aggr.argcnt = 0;
                elem.u.aggr.significant = 0;

                $$ = get_expr_with_item(Yacc, &elem);
            }
    |    DIRTY_COUNT_SYM '(' '*' ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_AGGREGATION;
                elem.u.aggr.type   = SAT_DCOUNT;
                elem.u.aggr.argcnt = 0;
                elem.u.aggr.significant = 0;

                $$ = get_expr_with_item(Yacc, &elem);
            }
    |    MAX_SYM '(' aggregation_ref_arg ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_AGGREGATION;
                elem.u.aggr.type   = SAT_MAX;
                elem.u.aggr.argcnt = 1;
                elem.u.aggr.significant = $3->len;
                if (Yacc->is_distinct) {
                    Yacc->is_distinct = 0;
                    elem.is_distinct = 1;
                }

                $$ = merge_expression(Yacc, NULL, $3, &elem);
            }
    |    MIN_SYM '(' aggregation_ref_arg ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_AGGREGATION;
                elem.u.aggr.type   = SAT_MIN;
                elem.u.aggr.argcnt = 1;
                elem.u.aggr.significant = $3->len;
                if (Yacc->is_distinct) {
                    Yacc->is_distinct = 0;
                    elem.is_distinct = 1;
                }

                $$ = merge_expression(Yacc, NULL, $3, &elem);
            }
    |    SUM_SYM '(' aggregation_ref_arg ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_AGGREGATION;
                if (Yacc->is_distinct) elem.is_distinct = 1;
                elem.u.aggr.type   = SAT_SUM;
                elem.u.aggr.argcnt = 1;
                elem.u.aggr.significant = $3->len;
                if (Yacc->is_distinct) {
                    Yacc->is_distinct = 0;
                    elem.is_distinct = 1;
                }

                $$ = merge_expression(Yacc, NULL, $3, &elem);
            }
    ;

function_ref:
        general_function_ref
            {
                $$ = $1;
            }
    |    string_function_ref
            {
                $$ = $1;
            }
    |    numeric_function_ref
            {
                $$ = $1;
            }
    |    time_function_ref
            {
                $$ = $1;
            }
    |    random_function_ref
            {
                $$ = $1;
            }
    |    rownum_ref
            {
                $$ = $1;
            }
    |    object_function_ref
            {
                $$ = $1;
            }
    |    byte_function_ref
            {
                $$ = $1;
            }
    |    objectsize_function_ref
            {
                $$ = $1;
            }
    ;

special_function_arg:
        atom
            {
                if (($1.elemtype != SPT_VALUE ||
                        $1.u.value.valueclass != SVC_CONSTANT ||
                        $1.u.value.valuetype != DT_VARCHAR) &&
                        !($1.elemtype == SPT_VALUE &&
                        $1.u.value.valueclass == SVC_CONSTANT &&
                        $1.u.value.valuetype == DT_NULL_TYPE &&
                        $1.u.value.param_idx))
                    __yyerror("CHARTOHEX() or HEXCOMP()'s arguments must be a string if constant value");
                $$ = $1;
            }
    |    column_ref
            {

                sc_memset(&$$, 0, sizeof(T_POSTFIXELEM));
                $$.elemtype = SPT_VALUE;
                $$.u.value.valueclass = SVC_VARIABLE;
                $$.u.value.valuetype  = DT_NULL_TYPE;

                p = sc_strchr($1, '.');
                if (p == NULL) {
                    $$.u.value.attrdesc.table.tablename = NULL;
                    $$.u.value.attrdesc.table.aliasname = NULL;
                    $$.u.value.attrdesc.attrname  = $1;
                }
                else {
                    *p++ = '\0';
                    $$.u.value.attrdesc.table.tablename = YACC_XSTRDUP(Yacc, $1);
                    $$.u.value.attrdesc.table.aliasname = NULL;
                    $$.u.value.attrdesc.attrname  = YACC_XSTRDUP(Yacc, p);
                    isnull($$.u.value.attrdesc.table.tablename);
                    isnull($$.u.value.attrdesc.attrname);
                }
            }
    ;

special_function_predicate:
        HEXCOMP_SYM '(' special_function_arg ',' special_function_arg ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_HEXCOMP;
                elem.u.func.argcnt = 2;

                $$ = get_expr_with_item(Yacc, &$3);
                $$ = merge_expression(Yacc, $$, NULL, &$5);
                $$ = merge_expression(Yacc, $$, NULL, &elem);
            }
    ;

special_function_ref:
        CHARTOHEX_SYM '(' special_function_arg ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_CHARTOHEX;
                elem.u.func.argcnt = 1;

                $$ = merge_expression(Yacc,
                            get_expr_with_item(Yacc, &$3), NULL, &elem);
            }
    ;

search_and_result:
        scalar_exp ',' scalar_exp
            {
                $$ = merge_expression(Yacc, $1, $3, NULL);
            }
    ;

search_and_result_commalist:
        search_and_result
            {
                $$ = $1;
            }
    |    search_and_result_commalist ',' search_and_result
            {
                $$ = merge_expression(Yacc, $1, $3, NULL);
            }
    ;

general_function_ref:
        CONVERT_SYM '(' data_type ',' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
 /* OID convert */
                if (Yacc->attr.type == DT_OID)
                {
                    Yacc->attr.type = DT_VARCHAR;
                    Yacc->attr.len = 30;
                    Yacc->attr.fixedlen = -2;
                    Yacc->attr.attrname = sql_mem_strdup(vSQL(Yacc)->parsing_memory, "rid(sn, pn, offset)");
                    isnull(Yacc->attr.attrname);
                }
                sc_memcpy(&elem.u.value.attrdesc, &(Yacc->attr), sizeof(T_ATTRDESC));

                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = Yacc->attr.type;
                elem.u.value.attrdesc.collation = Yacc->attr.collation;

                $$ = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_CONVERT;
                elem.u.func.argcnt = 2;

                $$ = merge_expression(Yacc, $$, $5, &elem);

                sc_memset(&(Yacc->attr), 0, sizeof(T_ATTRDESC));
            }
    |    DECODE_SYM '(' scalar_exp ',' search_and_result_commalist ')'
            {
                int arg_cnt;
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_DECODE;
                arg_cnt = $5->len + 1;
                for (i = 0; i <$5->len; ++i)
                {
                    if ($5->list[i]->elemtype == SPT_OPERATOR)
                    {
                        arg_cnt -= $5->list[i]->u.op.argcnt;
                    }
                    else if ($5->list[i]->elemtype == SPT_FUNCTION)
                    {
                        arg_cnt -= $5->list[i]->u.func.argcnt;
                    }
                    else if ($5->list[i]->elemtype == SPT_AGGREGATION)
                    {
                        arg_cnt -= $5->list[i]->u.aggr.argcnt;
                    }
                }

                elem.u.func.argcnt = arg_cnt;

                $$ = merge_expression(Yacc, $3, $5, &elem);
            }
    |    DECODE_SYM '(' scalar_exp ',' search_and_result_commalist ',' scalar_exp ')'
            {
                int arg_cnt;
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_DECODE;
                arg_cnt = $5->len + 2;
                for (i = 0; i <$5->len; ++i)
                {
                    if ($5->list[i]->elemtype == SPT_OPERATOR)
                    {
                        arg_cnt -= $5->list[i]->u.op.argcnt;
                    }
                    else if ($5->list[i]->elemtype == SPT_FUNCTION)
                    {
                        arg_cnt -= $5->list[i]->u.func.argcnt;
                    }
                    else if ($5->list[i]->elemtype == SPT_AGGREGATION)
                    {
                        arg_cnt -= $5->list[i]->u.aggr.argcnt;
                    }
                }

                elem.u.func.argcnt = arg_cnt;

                $$ = merge_expression(Yacc, $3, $5, NULL);
                $$ = merge_expression(Yacc, $$, $7, &elem);
            }
    |    IFNULL_SYM '(' scalar_exp ',' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_IFNULL;
                elem.u.func.argcnt = 2;

                $$ = merge_expression(Yacc, $3, $5, &elem);
            }
    ;

numeric_function_ref:
        SIGN_SYM '(' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_SIGN;
                elem.u.func.argcnt = 1;

                $$ = merge_expression(Yacc, $3, NULL, &elem);
            }
    |    ROUND_SYM '(' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype           = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_INTEGER;
                elem.u.value.u.i32      = 0;
                elem.u.value.attrdesc.collation   = MDB_COL_NUMERIC;

                $$ = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_ROUND;
                elem.u.func.argcnt = 2;

                $$ = merge_expression(Yacc, $3, $$, &elem);
            }
    |    ROUND_SYM '(' scalar_exp ',' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_ROUND;
                elem.u.func.argcnt = 2;

                $$ = merge_expression(Yacc, $3, $5, &elem);
            }
    |    TRUNC_SYM '(' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype           = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_INTEGER;
                elem.u.value.u.i32      = 0;
                elem.u.value.attrdesc.collation   = MDB_COL_NUMERIC;

                $$ = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_TRUNC;
                elem.u.func.argcnt = 2;

                $$ = merge_expression(Yacc, $3, $$, &elem);
            }
    |    TRUNC_SYM '(' scalar_exp ',' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_TRUNC;
                elem.u.func.argcnt = 2;

                $$ = merge_expression(Yacc, $3, $5, &elem);
            }
    ;

string_function_ref:
        SUBSTRING_SYM '(' scalar_exp ',' INTNUM ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_BIGINT;
                elem.u.value.u.i64      = $5;
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;

                $$ = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_BIGINT;
                elem.u.value.u.i64      = -1;
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;

                $$ = merge_expression(Yacc, $3, $$, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_SUBSTRING;
                elem.u.func.argcnt = 3;

                $$ = merge_expression(Yacc, $$, NULL, &elem);
            }
    |    SUBSTRING_SYM '(' scalar_exp ',' INTNUM ',' INTNUM ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_BIGINT;
                elem.u.value.u.i64      = $5;
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;

                $$ = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_BIGINT;
                elem.u.value.u.i64      = $7;
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;

                $$ = merge_expression(Yacc, $3, $$, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_SUBSTRING;
                elem.u.func.argcnt = 3;

                $$ = merge_expression(Yacc, $$, NULL, &elem);
            }
    |    LOWERCASE_SYM '(' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_LOWERCASE;
                elem.u.func.argcnt = 1;

                $$ = merge_expression(Yacc, $3, NULL, &elem);
            }
    |    UPPERCASE_SYM '(' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_UPPERCASE;
                elem.u.func.argcnt = 1;

                $$ = merge_expression(Yacc, $3, NULL, &elem);
            }
    |    LTRIM_SYM '(' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_LTRIM;
                elem.u.func.argcnt = 1;

                $$ = merge_expression(Yacc, $3, NULL, &elem);
            }
    |    LTRIM_SYM '(' scalar_exp ',' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_LTRIM;
                elem.u.func.argcnt = 2;

                $$ = merge_expression(Yacc, $3, $5, &elem);
            }
    |    RTRIM_SYM '(' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_RTRIM;
                elem.u.func.argcnt = 1;

                $$ = merge_expression(Yacc, $3, NULL, &elem);
            }
    |    RTRIM_SYM '(' scalar_exp ',' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_RTRIM;
                elem.u.func.argcnt = 2;

                $$ = merge_expression(Yacc, $3, $5, &elem);
            }
    |   REPLACE_SYM '(' scalar_exp ',' scalar_exp ',' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_REPLACE;
                elem.u.func.argcnt = 3;

                $$ = merge_expression(Yacc, $3, $5, NULL);
                $$ = merge_expression(Yacc, $$, $7, &elem);        // 1.2
            }
    |   RIDTABLENAME_SYM '(' scalar_exp ')'
            {
#if defined(MDB_DEBUG)
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_TABLENAME;
                elem.u.func.argcnt = 1;

                $$ = merge_expression(Yacc, $3, NULL, &elem);
#else
                __yyerror("RID in query is not supported.");
#endif
            }
    |    HEXASTRING_SYM '(' scalar_exp ')'
        {
            sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
            elem.elemtype      = SPT_FUNCTION;
            elem.u.func.type   = SFT_HEXASTRING;
            elem.u.func.argcnt = 1;

            $$ = merge_expression(Yacc, $3, NULL, &elem);
        }
    |    BINARYSTRING_SYM '(' scalar_exp ')'
        {
            sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
            elem.elemtype      = SPT_FUNCTION;
            elem.u.func.type   = SFT_BINARYSTRING;
            elem.u.func.argcnt = 1;

            $$ = merge_expression(Yacc, $3, NULL, &elem);
        }
    ;

time_function_ref:
        CURRENT_DATE_SYM
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_CURRENT_DATE;
                elem.u.func.argcnt = 0;
                $$ = get_expr_with_item(Yacc, &elem);
            }
    |    CURRENT_TIME_SYM
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_CURRENT_TIME;
                elem.u.func.argcnt = 0;
                $$ = get_expr_with_item(Yacc, &elem);
            }
    |    CURRENT_TIMESTAMP_SYM
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_CURRENT_TIMESTAMP;
                elem.u.func.argcnt = 0;
                $$ = get_expr_with_item(Yacc, &elem);
            }
    |    SYSDATE_SYM
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_SYSDATE;
                elem.u.func.argcnt = 0;
                $$ = get_expr_with_item(Yacc, &elem);
            }
    |    NOW_SYM '(' ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_NOW;
                elem.u.func.argcnt = 0;

                $$ = get_expr_with_item(Yacc, &elem);
            }
    |    DATE_ADD_SYM '(' IDENTIFIER ',' scalar_exp ',' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = $3;
                elem.u.value.attrdesc.len = sc_strlen($3);
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                Yacc->attr.collation = DB_Params.default_col_char;

                $$ = get_expr_with_item(Yacc, &elem);

                $$ = merge_expression(Yacc, $$, $5, NULL);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_DATE_ADD;
                elem.u.func.argcnt = 3;

                $$ = merge_expression(Yacc, $$, $7, &elem);
            }
    |    DATE_SUB_SYM '(' IDENTIFIER ',' scalar_exp ',' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = $3;
                elem.u.value.attrdesc.len = sc_strlen($3);
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;

                $$ = get_expr_with_item(Yacc, &elem);

                $$ = merge_expression(Yacc, $$, $5, NULL);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_DATE_SUB;
                elem.u.func.argcnt = 3;

                $$ = merge_expression(Yacc, $$, $7, &elem);
            }
    |    DATE_DIFF_SYM '(' IDENTIFIER ',' scalar_exp ',' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = $3;
                elem.u.value.attrdesc.len = sc_strlen($3);
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;

                $$ = get_expr_with_item(Yacc, &elem);

                $$ = merge_expression(Yacc, $$, $5, NULL);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_DATE_DIFF;
                elem.u.func.argcnt = 3;

                $$ = merge_expression(Yacc, $$, $7, &elem);
            }
    |    DATE_FORMAT_SYM '(' scalar_exp ',' STRINGVAL ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = $5;
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;
                elem.u.value.attrdesc.len = sc_strlen($5);

                $$ = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_DATE_FORMAT;
                elem.u.func.argcnt = 2;

                $$ = merge_expression(Yacc, $3, $$, &elem);
            }
    |    TIME_ADD_SYM '(' IDENTIFIER ',' scalar_exp ',' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = $3;
                elem.u.value.attrdesc.len = sc_strlen($3);
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                Yacc->attr.collation = DB_Params.default_col_char;

                $$ = get_expr_with_item(Yacc, &elem);

                $$ = merge_expression(Yacc, $$, $5, NULL);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_TIME_ADD;
                elem.u.func.argcnt = 3;

                $$ = merge_expression(Yacc, $$, $7, &elem);
            }
    |    TIME_SUB_SYM '(' IDENTIFIER ',' scalar_exp ',' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = $3;
                elem.u.value.attrdesc.len = sc_strlen($3);
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;

                $$ = get_expr_with_item(Yacc, &elem);

                $$ = merge_expression(Yacc, $$, $5, NULL);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_TIME_SUB;
                elem.u.func.argcnt = 3;

                $$ = merge_expression(Yacc, $$, $7, &elem);
            }
    |    TIME_DIFF_SYM '(' IDENTIFIER ',' scalar_exp ',' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = $3;
                elem.u.value.attrdesc.len = sc_strlen($3);
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;

                $$ = get_expr_with_item(Yacc, &elem);

                $$ = merge_expression(Yacc, $$, $5, NULL);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_TIME_DIFF;
                elem.u.func.argcnt = 3;

                $$ = merge_expression(Yacc, $$, $7, &elem);
            }
    |    TIME_FORMAT_SYM '(' scalar_exp ',' STRINGVAL ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = $5;
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;
                elem.u.value.attrdesc.len = sc_strlen($5);

                $$ = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_TIME_FORMAT;
                elem.u.func.argcnt = 2;

                $$ = merge_expression(Yacc, $3, $$, &elem);
            }
    ;

rownum_ref:
        ROWNUM_SYM
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype       = SPT_FUNCTION;
                elem.u.func.type    = SFT_ROWNUM;
                elem.u.func.argcnt  = 0;
                if (vSQL(Yacc)->sqltype != ST_SELECT)
                {
                    __yyerror("rownum allowed only select query.");
                }

                elem.u.func.rownump = vSELECT(Yacc)->rowsp = &(Yacc->rownum_buf);

                $$ = get_expr_with_item(Yacc, &elem);
            }
        ;

object_function_ref:
        IDENTIFIER '.' CURRVAL_SYM 
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype           = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_VARCHAR;
                elem.u.value.u.ptr      = $1;
                elem.u.value.attrdesc.len = sc_strlen($1);
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;
                $$ = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype       = SPT_FUNCTION;
                elem.u.func.type    = SFT_SEQUENCE_CURRVAL;
                elem.u.func.argcnt  = 1;
                $$ = merge_expression(Yacc, $$, NULL, &elem);
            }
        |   IDENTIFIER '.' NEXTVAL_SYM
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype           = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_VARCHAR;
                elem.u.value.u.ptr      = $1;
                elem.u.value.attrdesc.len = sc_strlen($1);
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;
                $$ = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype       = SPT_FUNCTION;
                elem.u.func.type    = SFT_SEQUENCE_NEXTVAL;
                elem.u.func.argcnt  = 1;
                $$ = merge_expression(Yacc, $$, NULL, &elem);
            } 
        ;

byte_function_ref:
        BYTE_SIZE_SYM '(' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype       = SPT_FUNCTION;
                elem.u.func.type    = SFT_BYTESIZE;
                elem.u.func.argcnt  = 1;
                $$ = merge_expression(Yacc, $3, NULL, &elem);
            }
        | COPYFROM_SYM '(' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = sql_mem_strdup(vSQL(Yacc)->parsing_memory, "byte");
                isnull(elem.u.value.u.ptr);
                elem.u.value.value_len    = sc_strlen(elem.u.value.u.ptr);
                elem.u.value.attrdesc.len = elem.u.value.value_len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;
                $$ = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype       = SPT_FUNCTION;
                elem.u.func.type    = SFT_COPYFROM;
                elem.u.func.argcnt = 2;

                $$ = merge_expression(Yacc, $3, $$, &elem);
            }
        | COPYFROM_SYM '(' scalar_exp ',' copyfrom_type ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = $5;
                elem.u.value.value_len    = sc_strlen(elem.u.value.u.ptr);
                elem.u.value.attrdesc.len = elem.u.value.value_len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;
                $$ = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype       = SPT_FUNCTION;
                elem.u.func.type    = SFT_COPYFROM;
                elem.u.func.argcnt = 2;

                $$ = merge_expression(Yacc, $3, $$, &elem);
            }
        | COPYTO_SYM '(' scalar_exp ',' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype       = SPT_FUNCTION;
                elem.u.func.type    = SFT_COPYTO;
                elem.u.func.argcnt  = 2;
                $$ = merge_expression(Yacc, $3, $5, &elem);
            }
        ;

copyfrom_type:
        CHARACTER_SYM
            {
                $$ = "char";
            }
    |    VARCHAR_SYM
            {
                $$ = "varchar";
            }
    |    BYTE_SYM 
            {
                $$ = "byte";
            }
    |    VARBYTE_SYM
            {
                $$ = "varbyte";
            }
    ;

random_function_ref:
        RANDOM_SYM
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_RANDOM;
                elem.u.func.argcnt = 0;

                $$ = get_expr_with_item(Yacc, &elem);
            }
        | RANDOM_SYM '(' ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_RANDOM;
                elem.u.func.argcnt = 0;

                $$ = get_expr_with_item(Yacc, &elem);
            }
        | SRANDOM_SYM '(' INTNUM ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));

                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_BIGINT;
                elem.u.value.u.i64 = $3;
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;

                calc_func_srandom(&elem.u.value, NULL, 0);

                $$ = get_expr_with_item(Yacc, &elem);
            }
        ;

objectsize_function_ref:
        OBJECTSIZE_SYM '(' objecttype ',' scalar_exp ')'
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_OBJECTSIZE;
                elem.u.func.argcnt = 2;

                $$ = merge_expression(Yacc, $3, $5, &elem);
            }
        ;

objecttype:
        TABLE_SYM
            { 
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_INTEGER;
                elem.u.value.u.i32      = OBJ_TABLE,
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;
                $$ = get_expr_with_item(Yacc, &elem);
            }
        | TABLEDATA_SYM
            { 
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_INTEGER;
                elem.u.value.u.i32      = OBJ_TABLEDATA,
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;
                $$ = get_expr_with_item(Yacc, &elem);
            }
        | TABLEINDEX_SYM
            { 
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_INTEGER;
                elem.u.value.u.i32      = OBJ_TABLEINDEX,
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;
                $$ = get_expr_with_item(Yacc, &elem);
            }
        ;


literal:
       minus_symbol APPROXNUM
            {
                sc_memset(&$$, 0, sizeof(T_POSTFIXELEM));
                $$.elemtype = SPT_VALUE;
                $$.u.value.valueclass = SVC_CONSTANT;
                $$.u.value.valuetype  = DT_DOUBLE;
                $$.u.value.u.f32      = $1 * $2;
                $$.u.value.attrdesc.collation = MDB_COL_NUMERIC;
            }
    |    minus_symbol FLOATNUM
            {
                sc_memset(&$$, 0, sizeof(T_POSTFIXELEM));
                $$.elemtype = SPT_VALUE;
                $$.u.value.valueclass = SVC_CONSTANT;
                $$.u.value.valuetype  = DT_DOUBLE;
                $$.u.value.u.f32      = $1 * $2;
                $$.u.value.attrdesc.collation = MDB_COL_NUMERIC;
            }
    |    minus_symbol INTNUM
            {
                sc_memset(&$$, 0, sizeof(T_POSTFIXELEM));
                $$.elemtype = SPT_VALUE;
                $$.u.value.valueclass = SVC_CONSTANT;
                $$.u.value.valuetype  = DT_BIGINT;
                $$.u.value.u.i64      = $1 * $2;
                $$.u.value.attrdesc.collation = MDB_COL_NUMERIC;
            }
    |    STRINGVAL
            {
                sc_memset(&$$, 0, sizeof(T_POSTFIXELEM));
                $$.elemtype = SPT_VALUE;
                $$.u.value.valueclass   = SVC_CONSTANT;
                $$.u.value.valuetype    = DT_VARCHAR;
                $$.u.value.u.ptr        = $1;
                $$.u.value.attrdesc.len = sc_strlen($1);
                $$.u.value.value_len = sc_strlen($1);
                $$.u.value.attrdesc.collation = DB_Params.default_col_char;

            }
    |  NSTRINGVAL
            {
                sc_memset(&$$, 0, sizeof(T_POSTFIXELEM));
                $$.elemtype = SPT_VALUE;
                $$.u.value.valueclass   = SVC_CONSTANT;
                $$.u.value.valuetype    =  DT_NVARCHAR;
                $$.u.value.u.Wptr       = (DB_WCHAR*)$1;
                $$.u.value.attrdesc.len = sc_wcslen((DB_WCHAR*)$1);
                $$.u.value.value_len = sc_wcslen((DB_WCHAR*)$1);
                $$.u.value.attrdesc.collation = DB_Params.default_col_nchar;

            }
    |   BINARYVAL
            {   
                int     len = sc_strlen($1);
                char    *ptr= 0;

                if ((len % 8) != 0)
                    __yyerror("Invalid Binary Value");

                len = len / 8;
                ptr = sql_mem_calloc(vSQL(Yacc)->parsing_memory, len + 1);
                len = sc_bstring2binary($1, ptr);
                sql_mem_freenul(vSQL(Yacc)->parsing_memory, $1);

                sc_memset(&$$, 0, sizeof(T_POSTFIXELEM));
                $$.elemtype = SPT_VALUE;
                $$.u.value.valueclass   = SVC_CONSTANT;
                $$.u.value.valuetype    = DT_VARBYTE;
                $$.u.value.u.ptr        = ptr;
                $$.u.value.attrdesc.len = len;  // binary string'len
                $$.u.value.value_len = len;
                $$.u.value.attrdesc.collation = MDB_COL_DEFAULT_BYTE;
            }
    |   HEXADECIMALVAL
            {
                int     len = sc_strlen($1);
                char    *ptr= 0;

                if ((len % 2) != 0)
                    __yyerror("Invalid Hexadecimal Value");

                len = len / 2;
                ptr = sql_mem_calloc(vSQL(Yacc)->parsing_memory, len + 1);
                len = sc_hstring2binary($1, ptr);
                sql_mem_freenul(vSQL(Yacc)->parsing_memory, $1);

                sc_memset(&$$, 0, sizeof(T_POSTFIXELEM));
                $$.elemtype = SPT_VALUE;
                $$.u.value.valueclass   = SVC_CONSTANT;
                $$.u.value.valuetype    = DT_VARBYTE;
                $$.u.value.u.ptr        = ptr;
                $$.u.value.attrdesc.len = len;  // binary string'len
                $$.u.value.value_len = len;
                $$.u.value.attrdesc.collation = MDB_COL_DEFAULT_BYTE;
            }
    |   NULL_SYM
            {
                sc_memset(&$$, 0, sizeof(T_POSTFIXELEM));
                $$.elemtype            = SPT_VALUE;
                $$.u.value.valueclass   = SVC_CONSTANT;
                $$.u.value.valuetype   = DT_NULL_TYPE;
                $$.u.value.is_null     = 1;
            }
    ;

minus_symbol:
    /* empty */
    {
        $$ = 1;
    }
    | '-'
    {
        $$ = -1;
    }
    ;

table:
        IDENTIFIER
            {
                __CHECK_TABLE_NAME_LENG($1);
                $$ = $1;
            }
    |    IDENTIFIER '.' IDENTIFIER
            {
                __CHECK_TABLE_NAME_LENG($3);
                $$ = $3; 
            }
    ;

view:
        IDENTIFIER
            {
                __CHECK_VIEW_NAME_LENG($1);
                $$ = $1;
            }
    ;

table_alias_name:
        IDENTIFIER
            {
                __CHECK_ALIAS_NAME_LENG($1);
                $$ = $1;
            }
    ;

column_ref_or_pvar:
            column_ref
            {
                {
                    sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                    elem.elemtype = SPT_VALUE;
                    elem.u.value.valueclass = SVC_VARIABLE;

                    elem.u.value.valuetype  = DT_NULL_TYPE;

                    p = sc_strchr($1, '.');
                    if (p == NULL) {
                        elem.u.value.attrdesc.table.tablename = NULL;
                        elem.u.value.attrdesc.table.aliasname = NULL;
                        elem.u.value.attrdesc.attrname  = $1;
                    }

                    else {
                        *p++ = '\0';
                        elem.u.value.attrdesc.table.tablename = 
                                                    YACC_XSTRDUP(Yacc, $1);
                        elem.u.value.attrdesc.table.aliasname = NULL;
                        elem.u.value.attrdesc.attrname  = YACC_XSTRDUP(Yacc, p);
                        isnull(elem.u.value.attrdesc.table.tablename);
                        isnull(elem.u.value.attrdesc.attrname);
                    }
                }

                $$ = get_expr_with_item(Yacc, &elem);
            }
    |    rid_column_ref
            {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));

                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_VARIABLE;

                elem.u.value.valuetype  = DT_NULL_TYPE;

                p = sc_strchr($1, '.');
                if (p == NULL) {
                    elem.u.value.attrdesc.table.tablename = NULL;
                    elem.u.value.attrdesc.table.aliasname = NULL;
                    elem.u.value.attrdesc.attrname  = $1;
                }
                else {
                    *p++ = '\0';
                    elem.u.value.attrdesc.table.tablename = 
                                                YACC_XSTRDUP(Yacc, $1);
                    elem.u.value.attrdesc.table.aliasname = NULL;
                    elem.u.value.attrdesc.attrname  = YACC_XSTRDUP(Yacc, p);
                    isnull(elem.u.value.attrdesc.table.tablename);
                    isnull(elem.u.value.attrdesc.attrname);
                }
                elem.u.value.attrdesc.type = DT_OID;
                elem.u.value.attrdesc.Hattr = -1;
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;

                $$ = get_expr_with_item(Yacc, &elem);
            }
    ;

column_ref:
        column
            {
                $$ = $1;
            }
    |    IDENTIFIER '.' column    /* needs semantics */
            {
                __CHECK_TABLE_NAME_LENG($1);
                sc_snprintf(buf, MDB_BUFSIZ, "%s.%s", $1, $3);
                $$ = YACC_XSTRDUP(Yacc, buf);
                isnull($$);
            }
    |    IDENTIFIER '.' IDENTIFIER '.' column
            {
                INCOMPLETION("OWNERSHIP");
            }
    ;

rid_column_ref:
        IDENTIFIER '.' RID_SYM/* needs semantics */
            {
                __CHECK_TABLE_NAME_LENG($1);
                sc_snprintf(buf, MDB_BUFSIZ, "%s.%s", $1, "rid");
                $$ = YACC_XSTRDUP(Yacc, buf);
                isnull($$);
            }
    |    RID_SYM
            {
                sc_snprintf(buf, MDB_BUFSIZ, "%s", "rid");
                $$ = YACC_XSTRDUP(Yacc, buf);
                isnull($$);
            }
    ;

        /* the various things you can name */

column:
        IDENTIFIER
            {
                __CHECK_FIELD_NAME_LENG($1);
                $$ = $1;
            }
    ;

index:
        IDENTIFIER
            {
                __CHECK_INDEX_NAME_LENG($1);
                $$ = $1;
            }
    ;

sequence:
        IDENTIFIER
            {
                __CHECK_SEQUENCE_NAME_LENG($1);
                $$ = $1;
            }
    ;

opt_ansi_style:
        /* empty */
    |    AS_SYM
    ;

admin_statement:
        EXPORT_SYM export_data_or_schema export_all_or_table INTO_SYM STRINGVAL export_op
        {
            vSQL(Yacc)->sqltype = ST_ADMIN;
            vADMIN(Yacc) = &vSQL(Yacc)->u._admin;
            Yacc->userdefined = 0;
            Yacc->param_count = 0;
 
            vADMIN(Yacc)->u.export.data_or_schema = $2;
            if ($3 == NULL)
                vADMIN(Yacc)->type  = SADM_EXPORT_ALL;
            else
                vADMIN(Yacc)->type  = SADM_EXPORT_ONE;
            vADMIN(Yacc)->u.export.table_name   = $3;
            vADMIN(Yacc)->u.export.export_file  = $5;
            vADMIN(Yacc)->u.export.f_append     = $6;
        }
    |   IMPORT_SYM FROM_SYM STRINGVAL
        {
            vSQL(Yacc)->sqltype = ST_ADMIN;
            vADMIN(Yacc) = &vSQL(Yacc)->u._admin;
            Yacc->userdefined = 0;
            Yacc->param_count = 0;

            vADMIN(Yacc)->type  = SADM_IMPORT;
            vADMIN(Yacc)->u.import.import_file = $3;
        }
    ;

export_op:    
        CREATE_SYM
            {
                $$ = 0;
            }
        | ATTACH_SYM
            {
                $$ = 1;
            }
    ;

export_all_or_table:
        ALL_SYM
            {
                $$ = NULL;
            }
        | TABLE_SYM table
            {
                $$ = $2;
            }
    ;

export_data_or_schema:    
/* empty */
            {
                $$ = 0;
            }
        | DATA_SYM
            {
                $$ = 0;
            }
        | SCHEMA_SYM
            {
                $$ = 1;
            }
    ;
            
%%

// 내부에서 ERROR MSG 찍는데.. 8K나 잡아버린다.
// 나중에 OPTIMIZE 할것.
static void my_yyerror(T_YACC *yacc, char *s)
{
    char token[64], *ptr;
    int posi, len;

    len = MAX_LINE_SIZE*5;
    ptr = PMEM_ALLOC(len);
    if (!ptr) {
        yacc->my_yylineno = 1;
        yacc->err_len = 0;
        yacc->err_pos = 0x00; 
        my_yy_flush_buffer(yacc);
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);

        return;
    }

    posi    = 0;
    if( yacc->err_len >= sizeof(token) )
        yacc->err_len = sizeof(token) - 1;

    sc_strncpy(token, yacc->err_pos, yacc->err_len);
    token[yacc->err_len] = '\0';

    yacc->err_len = 0;
    yacc->err_pos = 0x00; 

    if (s) {
        posi += sc_snprintf(ptr+posi, len-posi,
                "%s: near token \'%s\' at line #%d", s, token, yacc->my_yylineno);
    } else {
        posi += sc_snprintf(ptr+posi, len-posi,
                "Syntax error near unexpected token \'%s\' at line #%d",
                token, yacc->my_yylineno);
    }
    if (yacc->yerrno) {
      er_set_parser(ER_ERROR_SEVERITY,ARG_FILE_LINE, yacc->yerrno, ptr);
      yacc->yerrno = 0;
    }
    else
      er_set_parser(ER_ERROR_SEVERITY,ARG_FILE_LINE, SQL_E_INVALIDSYNTAX, ptr);

    yacc->my_yylineno = 1;
    yacc->err_len = 0;
    yacc->err_pos = 0x00; 
    my_yy_flush_buffer(yacc);

    PMEM_FREENUL(ptr);
    return;
}

static void append(T_LIST_ATTRDESC *nlist, T_ATTRDESC *attr)
{
    nlist->list[nlist->len++] = *attr;
}

static void destroy_gbufferkey(void)
{
    return;
}

static void destroy_gbuffer(void *p)
{
    my_yy_delete_buffer(p);
    SMEM_FREENUL(p);
}

static void alloc_gbufferkey(void)
{
    gKey = 2;
}

static void *alloc_gbuffer(unsigned int s)
{
    void *tData;

    tData = SMEM_XCALLOC(s);
    if (CS_setspecific(gKey, tData)) return NULL;
    return tData;
}

__DECL_PREFIX void *current_thr(void)
{
    void *p;

    if (ppthread_once(&gOnce, alloc_gbufferkey)) return NULL;
    p = CS_getspecific(gKey);
    if (p == NULL)
        p = alloc_gbuffer(sizeof(T_YACC));
    return p;
}

/* called from Process_Action before freeing thread */
__DECL_PREFIX void free_gbuffer()
{
    void *p;
    p = CS_getspecific(gKey);
    if (p == NULL)
        return;
    my_yy_delete_buffer(p);

 /* FREE_YACC_BUFFER */
    if (p)
    {
        destroy_gbuffer(p);
        CS_setspecific(gKey, NULL);
        p = NULL;
    }

    if (sc_memcmp(&gOnce, &__once_init__, sizeof(pthread_once_t)))
    {
        if (p)
        {
            destroy_gbuffer(p);
            CS_setspecific(gKey, NULL);
        }
        destroy_gbufferkey();

        gOnce = __once_init__;
    }
}

static int expandListArray(T_YACC *yacc, void **ptr, int size, int orglen)
{
    void *tmp;

    tmp = sql_mem_alloc(yacc->stmt->parsing_memory, size*(orglen+MAX_NEXT_LIST_NUM));
    if (tmp == NULL) return RET_ERROR;

    sc_memcpy(tmp, *ptr, size*orglen);
    *ptr = tmp;

    return (orglen+MAX_NEXT_LIST_NUM);

}

static int expandListArraybyLimit(T_YACC *yacc, void **ptr, int size, int orglen, int limit)
{
    int num;
    void *tmp;

    num = min(limit, orglen+MAX_NEXT_LIST_NUM);
    tmp = sql_mem_alloc(yacc->stmt->parsing_memory, size*num);
    if (tmp == NULL) return RET_ERROR;

    sc_memcpy(tmp, *ptr, size*orglen);
    *ptr = tmp;

    return num;
}

char *Yacc_Xstrdup(T_YACC *yacc, char *s)
{
    char *p;

    p = sql_mem_strdup(yacc->stmt->parsing_memory, s);
    if (p == NULL)
        return NULL;
    return p;
}

static T_EXPRESSIONDESC *
merge_expression(T_YACC *yacc, T_EXPRESSIONDESC *lexpr,
                 T_EXPRESSIONDESC *rexpr, T_POSTFIXELEM *item)
{
    T_EXPRESSIONDESC *expr;
    int     posi = 0;
    int     merge_len, merge_max;
    int     i;
    int     which_expr_list = 0;    // new -> 0, left -> 1, right -> 2

    // merge 시킬 갯수를 계산
    merge_len = (lexpr?lexpr->len:0) + (rexpr?rexpr->len:0) + (item?1:0);
    merge_max = MAX_INIT_LIST_NUM;

    while (1)
    {
        if (merge_max < merge_len)
            merge_max += (MAX_NEXT_LIST_NUM);
        else
            break;
    }

    if ((lexpr?lexpr->max:0) >= merge_len)
        which_expr_list = 1;
    else if ((rexpr?rexpr->max:0) >= merge_len)
        which_expr_list = 2;

    if (which_expr_list == 1)
    {   
        expr = lexpr;

        posi = lexpr->len;
        if (rexpr)
        {
            for(i = 0; i < rexpr->len; ++i)
            {
                expr->list[posi] = rexpr->list[i];
                rexpr->list[i] = NULL;
                ++posi;
            }
            sql_mem_set_free_postfixelem_list(yacc->stmt->parsing_memory, rexpr->list);
            rexpr->len = 0;
            rexpr->max = 0;
        }
    }
    else if (which_expr_list == 2)
    {
        expr = rexpr;
        if (lexpr)
        {
            for(i = (rexpr->len - 1); i >= 0; --i)
            {
                expr->list[i + lexpr->len] = rexpr->list[i];
                rexpr->list[i] = NULL;
            }

            for(i = 0; i < lexpr->len; ++i)
            {
                expr->list[posi] = lexpr->list[i];
                lexpr->list[i] = NULL;
                ++posi;
            }

            sql_mem_set_free_postfixelem_list(yacc->stmt->parsing_memory, lexpr->list);
            posi = lexpr->len + rexpr->len;

            lexpr->len = 0;
            lexpr->max = 0;
        }
        else
            posi = rexpr->len;

    }
    else
    {
        T_POSTFIXELEM **list = NULL;

        list = sql_mem_get_free_postfixelem_list(yacc->stmt->parsing_memory, merge_max);
        if (list == NULL)
            return NULL;

        if (lexpr)
        {
            for(i = 0; i < lexpr->len; ++i)
            {
                list[posi] = lexpr->list[i];
                lexpr->list[i] = NULL;
                ++posi;
            }

            sql_mem_set_free_postfixelem_list(yacc->stmt->parsing_memory, lexpr->list);
            lexpr->list = NULL;
            lexpr->len = 0;
            lexpr->max = 0;
        }

        if (rexpr)
        {
            for(i = 0; i < rexpr->len; ++i)
            {
                list[posi] = rexpr->list[i];
                rexpr->list[i] = NULL;
                ++posi;
            }
            sql_mem_set_free_postfixelem_list(yacc->stmt->parsing_memory, rexpr->list);
            rexpr->list = NULL;
            rexpr->len = 0;
            rexpr->max = 0;
        }

        if (lexpr)
            expr = lexpr;
        else if (rexpr)
            expr = rexpr;
        else 
            expr = sql_mem_alloc(yacc->stmt->parsing_memory, sizeof(T_EXPRESSIONDESC));

        if (!expr)
            return NULL;
        expr->list = list;
    }

    if (item)
    {
        expr->list[posi] = sql_mem_get_free_postfixelem(yacc->stmt->parsing_memory);
        sc_memcpy(expr->list[posi], item, sizeof(T_POSTFIXELEM));
    }

    expr->len = merge_len;
    expr->max = merge_max;

    return expr;
}

static T_EXPRESSIONDESC *
get_expr_with_item(T_YACC *yacc, T_POSTFIXELEM *item)
{
    T_EXPRESSIONDESC *expr;

    if (item)
        expr = sql_mem_get_free_expressiondesc(yacc->stmt->parsing_memory, 1, 1);
    else
        expr = sql_mem_get_free_expressiondesc(yacc->stmt->parsing_memory, 0, 0);

    if (!expr) return NULL;

    if (item)
    {
        sc_memcpy(expr->list[0], item, sizeof(T_POSTFIXELEM));
    }

    return expr;
}

static char *get_remain_qstring(T_YACC *yacc)
{
    char    *s;
    s = yacc->yyinput+yacc->my_yyinput_cnt+1;
    return s;
}

static int init_limit(T_LIMITDESC *limitdesc)
{
    limitdesc->offset   = 0;
    limitdesc->rows     = -1;
    limitdesc->start_at = NULL_OID;

    limitdesc->offset_param_idx  = -1;
    limitdesc->rows_param_idx    = -1;
    limitdesc->startat_param_idx = -1;


    return RET_SUCCESS;
}

static void __INCOMPLETION(T_YACC* yacc, char *t)
{
    void *p = yacc->scanner;
    my_yy_flush_buffer(yacc);

    sc_memset(&(yacc->stmt->u), 0, sizeof(yacc->stmt->u));
    yacc->stmt->sqltype    = ST_NONE;
    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_NOTIMPLEMENTED,  1, t);
    sc_memset(yacc, 0, sizeof(T_YACC));
    yacc->scanner = p;
}

static void ____yyerror(T_YACC *yacc, char *e)
{
    void *p = yacc->scanner;
    if (yacc->yerror)
    {
        my_yyerror(yacc, yacc->yerror);
        yacc->yerror = NULL;
    } else
        my_yyerror(yacc, e);

    if (vSQL(yacc))
    {
        sc_memset(&(yacc->stmt->u), 0, sizeof(yacc->stmt->u));
        yacc->stmt->sqltype = ST_NONE;
    }
    sc_memset(yacc, 0, sizeof(T_YACC));
    yacc->scanner = p;
}

static void __isnull(T_YACC *yacc, void* p)
{
    if ((p) == NULL)
    {
        ____yyerror(yacc, "out of memory");
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
    }
}

static int __CHECK_ARRAY(T_YACC* yacc, T_STATEMENT *s,
                          int b, int *max, int *len, void **list)
{
    if (*list)
    {
        if (*max <= *len)
            *max = expandListArray(yacc, (void**)list,
                                       b, *max);

        if (*max == RET_ERROR)
        {
            void *p = yacc->scanner;
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_OUTOFMEMORY, 0);
            sc_memset(yacc, 0, sizeof(T_YACC));
            yacc->scanner = p;
            return RET_ERROR;
        }
    }
    else
    {
        *list = sql_mem_alloc(yacc->stmt->parsing_memory, (b)*MAX_INIT_LIST_NUM);
        __isnull(yacc, *list);
        *len = 0; *max = MAX_INIT_LIST_NUM;
    }

    return RET_SUCCESS;
}

static int __CHECK_ARRAY_BY_LIMIT(T_YACC* yacc, T_STATEMENT *s,
                                  T_LIST_JOINTABLEDESC *a, int b, int l)
{
    if ((a)->list)
    {
        if ((a)->max <= (a)->len)
        {
            if ((l) <= (a)->max)
            {
                ____yyerror(yacc, "too many join table");
                return RET_ERROR;
            }

            (a)->max =
               expandListArraybyLimit(yacc, (void**)&(a)->list, b,
                       (a)->max, l);

            if ((a)->max == RET_ERROR)
            {
                void *p = yacc->scanner;
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                       SQL_E_OUTOFMEMORY, 0);
                sc_memset(yacc, 0, sizeof(T_YACC));
                yacc->scanner = p;
                return RET_ERROR;
            }
        }
    }
    else
    {
        int size;
        size = min(l, MAX_INIT_LIST_NUM);

        (a)->list = sql_mem_alloc(yacc->stmt->parsing_memory, (b)*size);
        __isnull(yacc, (a)->list);
        (a)->len = 0; (a)->max = size;
    }

    return RET_SUCCESS;
}

/*************************************************************
  Actions.
  ************************************************************/

static int 
ACTION_default_scalar_exp(T_YACC* yacc, T_EXPRESSIONDESC *expr)
{
    if (yacc->userdefined & 0x10) 
    {
        SET_DEFAULT_VALUE_ERROR(yacc);
        ____yyerror(yacc, "ERROR");
        return RET_ERROR;
    }
    else if (expr->len == 1 && expr->list[0]->elemtype == SPT_FUNCTION &&
            (expr->list[0]->u.func.type == SFT_SYSDATE ||
             expr->list[0]->u.func.type == SFT_CURRENT_TIMESTAMP ||
             expr->list[0]->u.func.type == SFT_CURRENT_DATE ||
             expr->list[0]->u.func.type == SFT_CURRENT_TIME))
    {
        if (!IS_DATE_OR_TIME_TYPE(yacc->attr.type))
        {
            SET_DEFAULT_VALUE_ERROR(yacc);
            ____yyerror(yacc, "invalid DEFAULT value checking");
            return RET_ERROR;
        }

        if (expr->list[0]->u.func.type == SFT_SYSDATE)
        {
            yacc->attr.defvalue.u.ptr    = 
                sql_mem_strdup(yacc->stmt->parsing_memory, "sysdate");
            if (!yacc->attr.defvalue.u.ptr)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return RET_ERROR;
            }
        }
        else if (expr->list[0]->u.func.type == SFT_CURRENT_TIMESTAMP)
        {
            yacc->attr.defvalue.u.ptr    = 
                sql_mem_strdup(yacc->stmt->parsing_memory, "current_timestamp");
            if (!yacc->attr.defvalue.u.ptr)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return RET_ERROR;
            }
        }
        else if (expr->list[0]->u.func.type == SFT_CURRENT_DATE)
        {
            yacc->attr.defvalue.u.ptr    = 
                sql_mem_strdup(yacc->stmt->parsing_memory, "current_date");
            if (!yacc->attr.defvalue.u.ptr)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return RET_ERROR;
            }
        }
        else if (expr->list[0]->u.func.type == SFT_CURRENT_TIME)
        {
            if (IS_TIME_TYPE(yacc->attr.type))
            {
                yacc->attr.defvalue.u.ptr    = 
                    sql_mem_strdup(yacc->stmt->parsing_memory, "current_time");
                if (!yacc->attr.defvalue.u.ptr)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                    return RET_ERROR;
                }
            }
            else
            {
                SET_DEFAULT_VALUE_ERROR(yacc);
                ____yyerror(yacc, "invalid DEFAULT value checking");
                return RET_ERROR;
            }
        }
        /* Moved below three lines from #if ... #endif above
         * to set default values */
        yacc->attr.flag |= DEF_SYSDATE_BIT;
        yacc->attr.defvalue.defined  = 1;
        yacc->attr.defvalue.not_null = 1;
    }
    else
    {
        T_VALUEDESC val;
        T_DEFAULTVALUE *defval;
        T_VALUEDESC res;
        int str_len;

        if (calc_expression(expr, &val, MDB_FALSE) < 0) 
        {
            SET_DEFAULT_VALUE_ERROR(yacc);
            ____yyerror(yacc, "ERROR");
            return RET_ERROR;
        }

        if (val.is_null)
        {
            if (yacc->attr.defvalue.defined)
                ____yyerror(yacc, "ERROR");
            else
            {
                yacc->attr.defvalue.defined  = 1;
                yacc->attr.defvalue.not_null = 0;
            }
            return RET_SUCCESS;
        }

        yacc->attr.defvalue.defined  = 1;
        yacc->attr.defvalue.not_null = 1;
        defval = &(yacc->attr.defvalue);

        if (yacc->attr.type != DT_NULL_TYPE &&
                check_defaultvalue(&val, yacc->attr.type,
                    yacc->attr.len) < 0)
        {
            sql_value_ptrFree(&val);
            SET_DEFAULT_VALUE_ERROR(yacc);
            ____yyerror(yacc, "invalid DEFAULT value checking");
            return RET_ERROR;
        }

        /* set the default value on byte/varbyte */
        if (yacc->attr.type ==  DT_BYTE 
                || yacc->attr.type == DT_VARBYTE)
        {
            if (val.valuetype == DT_BYTE
                    || val.valuetype == DT_VARBYTE)
            {
                defval->u.ptr =
                    sql_mem_alloc(yacc->stmt->parsing_memory,
                            yacc->attr.len);
                sc_memcpy(defval->u.ptr, val.u.ptr, val.value_len);
                defval->value_len = val.value_len;
            }
            else
            {
                sql_value_ptrFree(&val);
                SET_DEFAULT_VALUE_ERROR(yacc);
                ____yyerror(yacc, "invalid DEFAULT value string");
                return RET_ERROR;
            }
        }
        else
        {
            sc_memset(&res, 0, sizeof(T_VALUEDESC));
            if (yacc->attr.type == DT_NULL_TYPE)
                yacc->attr.type = val.valuetype;

            res.valuetype = yacc->attr.type;
            res.attrdesc = yacc->attr;

            if (calc_func_convert(&res, &val, &res, 0) < 0)
            {
                sql_value_ptrFree(&val);
                SET_DEFAULT_VALUE_ERROR(yacc);
                ____yyerror(yacc, "invalid DEFAULT value checking");
                return RET_ERROR;
            }

            if (defval->not_null && res.is_null)
            {
                sql_value_ptrFree(&val);
                SET_DEFAULT_VALUE_ERROR(yacc);
                ____yyerror(yacc, "invalid DEFAULT value checking");
                return RET_ERROR;
            }

            if (IS_BS_OR_NBS(res.valuetype))
            {
                defval->value_len = 
                    strlen_func[res.valuetype](res.u.ptr);

                if (yacc->attr.len == 0)
                    str_len = defval->value_len;
                else
                    str_len = yacc->attr.len;

                if (IS_BS(res.valuetype))
                {
                    defval->u.ptr =
                        sql_mem_alloc(yacc->stmt->parsing_memory,
                                str_len+1);
                }
                else
                {
                    defval->u.Wptr =
                        sql_mem_alloc(yacc->stmt->parsing_memory,
                                (str_len+1) * WCHAR_SIZE);
                }

                strcpy_func[res.valuetype](defval->u.ptr, res.u.ptr,
                        str_len);

                PMEM_FREENUL(res.u.ptr);

            }
            else
                sc_memcpy(&defval->u, &res.u, sizeof(T_VALUE));
        }

        sql_value_ptrFree(&val);

        if (IS_BS_OR_NBS(yacc->attr.type))
        {
            if (IS_BS(yacc->attr.type))
            {
                if (defval->value_len >= DEFAULT_VALUE_LEN)
                {
                    ____yyerror(yacc, "Exceed the default values's length");
                    return RET_ERROR;
                }
            }
            else
            {
                if ((defval->value_len*WCHAR_SIZE) >= DEFAULT_VALUE_LEN)
                {
                    ____yyerror(yacc, "Exceed the default values's length");
                    return RET_ERROR;
                }

            }
        }
        if (IS_BYTE(yacc->attr.type))
        {
            if (defval->value_len >= DEFAULT_VALUE_LEN - 4)
            {
                ____yyerror(yacc, "Exceed the default values's length");
                return RET_ERROR;
            }
        }

        yacc->userdefined |= 0x10;

    }
    return RET_SUCCESS;
}


static int 
ACTION_drop_statement(T_YACC* yacc, T_DROPTYPE droptype, char *target)
{
    vSQL(yacc)->sqltype = ST_DROP;
    vDROP(yacc)         = &vSQL(yacc)->u._drop;
    vDROP(yacc)->type   = droptype;
    vDROP(yacc)->target = target;

    return RET_SUCCESS;
}

int sql_yyparse(void *__yacc)
{
    return __yyparse(__yacc);
}
