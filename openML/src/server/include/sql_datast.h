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

#ifndef _SQL_DATAST_H
#define _SQL_DATAST_H

#include "mdb_config.h"
#include "mdb_compat.h"
#include "mdb_typedef.h"
#include "systable.h"
#include "mdb_Csr.h"
#include "sql_const.h"
#include "isql.h"
#include "mdb_FieldVal.h"

#include "mdb_collation.h"

typedef char ibool;
typedef int ihandle;

typedef signed char tinyint;
typedef signed short int smallint;

typedef double decimal;

typedef bigint t_astime;
typedef struct db_tm t_astm;

/* __linked_list : sql_util.h */
typedef struct __linked_list CORRELATED_ATTR;
typedef struct __linked_list EXPRDESC_LIST;
typedef struct __linked_list ATTRED_EXPRDESC_LIST;

#ifdef MAX_TIMESTAMP_FIELD_LEN
#undef MAX_TIMESTAMP_FIELD_LEN
#endif

#define MAX_TIMESTAMP_FIELD_LEN        sizeof(t_astime)

#define LEFT_TABLE_AHEAD    -1
#define ANY_TABLE_AHEAD     0
#define RIGHT_TABLE_AHEAD   1

typedef enum
{
    ST_NONE = 0,
    ST_SELECT,
    ST_INSERT,
    ST_UPDATE,
    ST_DELETE,
    ST_UPSERT,
    ST_DESCRIBE,
    ST_CREATE,
    ST_RENAME,
    ST_ALTER,
    ST_TRUNCATE,
    ST_ADMIN,
    ST_DROP,
    ST_COMMIT,
    ST_COMMIT_FLUSH,
    ST_ROLLBACK_FLUSH,
    ST_ROLLBACK,
    ST_SET,
    ST_DUMMY = MDB_INT_MAX      /* to guarantee sizeof(enum) == 4 */
} T_SQLTYPE;

#define SQL_IS_DDL(_sqltype)        \
    ((_sqltype) == ST_CREATE   ||   \
     (_sqltype) == ST_RENAME   ||   \
     (_sqltype) == ST_ALTER    ||   \
     (_sqltype) == ST_TRUNCATE ||   \
     (_sqltype) == ST_ADMIN    ||   \
     (_sqltype) == ST_DROP)

typedef enum
{
    SQT_NONE = 0,
    SQT_NORMAL,
    SQT_AGGREGATION,
    SQT_DUMMY = MDB_INT_MAX     /* to guarantee sizeof(enum) == 4 */
} T_QUERYTYPE;

typedef struct
{
    char *tablename;
    char *aliasname;
    struct t_select_ *correlated_tbl;
    int max_records;            /* the number of max records */
    char *column_name;          /* column name for keeping max records */
} T_TABLEDESC;

typedef struct
{
    int ordinal;
    int offset;
} T_POSITION;

typedef union
{
    ibool bl;
    tinyint i8;
    smallint i16;
    int i32;
    bigint i64;
    float f16;
    double f32;
    decimal dec;
    char ch;
    char *ptr;
    DB_WCHAR *Wptr;
    t_astime timestamp;
    char datetime[MAX_DATETIME_FIELD_LEN];
    char date[MAX_DATE_FIELD_LEN];
    char time[MAX_TIME_FIELD_LEN];
    OID oid;
} T_VALUE;

typedef struct
{
    ibool defined;
    ibool not_null;
    int value_len;              // value's length(not byte length)
    T_VALUE u;
} T_DEFAULTVALUE;

typedef struct
{
    T_TABLEDESC table;
    char *attrname;
    T_DEFAULTVALUE defvalue;
    T_POSITION posi;
    MDB_UINT8 flag;
    DataType type;
    char order;                 /* 'A': ASC, 'D': DESC */
    int len;
    int fixedlen;               /* fixed_part size as character not byte */
    int dec;
    MDB_COL_TYPE collation;
    ihandle Htable;
    ihandle Hattr;
} T_ATTRDESC;

typedef struct
{
    int len;
    int max;
    T_ATTRDESC *list;
} T_LIST_ATTRDESC;

typedef struct
{
    char *name;
    SYSFIELD_T *fieldinfo;
} T_COLDESC;

typedef struct
{
    int len;
    int max;
    int *ins_cols;
    T_COLDESC *list;
} T_LIST_COLDESC;

typedef enum
{
    SVC_NONE = 0,
    SVC_VARIABLE,
    SVC_CONSTANT,
    SVC_DUMMY = MDB_INT_MAX     /* to guarantee sizeof(enum) == 4 */
} T_VALUECLASS;

typedef struct
{
    T_VALUECLASS valueclass;    // whether variable or constant
    DataType valuetype;         // int, float and etc
    T_ATTRDESC attrdesc;        // only use in case of variable
    ibool call_free;            // u.ptr free여부 표시
    ibool is_null;
    int param_idx;              // 1부터 시작;
    MDB_INT32 value_len;        // value's length(not bytes length)
    T_VALUE u;
} T_VALUEDESC;

typedef enum
{
    SOT_NONE = 0,

    SOT_AND,
    SOT_OR,
    SOT_NOT,
    SOT_LIKE,
    SOT_ILIKE,

    SOT_ISNULL,
    SOT_ISNOTNULL,

    SOT_PLUS,
    SOT_MINUS,
    SOT_TIMES,
    SOT_DIVIDE,
    SOT_EXPONENT,
    SOT_MODULA,

    SOT_MERGE,

    SOT_LT,
    SOT_LE,
    SOT_GT,
    SOT_GE,
    SOT_EQ,
    SOT_NE,

    SOT_IN,
    SOT_NIN,
    SOT_NLIKE,
    SOT_NILIKE,
    SOT_SOME,   /* temp status only for parser.y */
    SOT_SOME_LT,        /* < */
    SOT_SOME_LE,        /* <= */
    SOT_SOME_GT,        /* > */
    SOT_SOME_GE,        /* >= */
    SOT_SOME_EQ,        /* = */
    SOT_SOME_NE,        /* <> */
    SOT_ALL,    /* temporary status only for parser.y */
    SOT_ALL_LT,
    SOT_ALL_LE,
    SOT_ALL_GT,
    SOT_ALL_GE,
    SOT_ALL_EQ,
    SOT_ALL_NE,

    SOT_EXISTS,
    SOT_NEXISTS,
    SOT_HEXCOMP,
    SOT_HEXNOTCOMP,
    SOT_UNION_ALL,
    SOT_UNION,
    SOT_INTERSECT,
    SOT_EXCEPT,
    SOT_DUMMY = MDB_INT_MAX     /* to guarantee sizeof(enum) == 4 */
} T_OPTYPE;

typedef struct
{
    T_OPTYPE type;
    int argcnt;
    void *likeStr;
} T_OPDESC;

typedef enum
{
    SFT_NONE = 0,

    SFT_CONVERT,
    SFT_DECODE,
    SFT_IFNULL,

    SFT_ROUND,
    SFT_SIGN,
    SFT_TRUNC,

    SFT_LOWERCASE,
    SFT_LTRIM,
    SFT_RTRIM,
    SFT_SUBSTRING,
    SFT_UPPERCASE,

    SFT_CURRENT_DATE,   /* no support */
    SFT_CURRENT_TIME,   /* no support */
    SFT_CURRENT_TIMESTAMP,
    SFT_DATE_ADD,
    SFT_DATE_DIFF,
    SFT_DATE_SUB,
    SFT_DATE_FORMAT,
    SFT_TIME_ADD,
    SFT_TIME_DIFF,
    SFT_TIME_SUB,
    SFT_TIME_FORMAT,
    SFT_NOW,
    SFT_SYSDATE,
    SFT_CHARTOHEX,      /* only for special version */
    SFT_HEXCOMP,        /* only for special version */
    SFT_HEXNOTCOMP,     /* only for special version */
    SFT_RANDOM,
    SFT_SRANDOM,
    SFT_ROWNUM,
    SFT_RID,
    SFT_SEQUENCE_CURRVAL,
    SFT_SEQUENCE_NEXTVAL,
    SFT_REPLACE,
    SFT_BYTESIZE,
    SFT_COPYFROM,
    SFT_COPYTO,
    SFT_OBJECTSIZE,
    SFT_TABLENAME,
    SFT_HEXASTRING,     /* convert byte to hexa string */
    SFT_BINARYSTRING,   /* convert byte to binary string */
    SFT_DUMMY = MDB_INT_MAX     /* to guarantee sizeof(enum) == 4 */
} T_FUNCTYPE;

typedef enum
{
    OBJ_TABLE = 0,
    OBJ_TABLEDATA,
    OBJ_TABLEINDEX
} T_OBJECTSIZE;

typedef struct
{
    T_FUNCTYPE type;
    int argcnt;
    int *rownump;               /* stmt->u._select.rowsp */
} T_FUNCDESC;

typedef enum
{
    SAT_NONE = 0,
    SAT_COUNT,
    SAT_DCOUNT, /* dirty_count() */
    SAT_MIN,
    SAT_MAX,
    SAT_SUM,
    SAT_AVG,
    SAT_DUMMY = MDB_INT_MAX     /* to guarantee sizeof(enum) == 4 */
} T_AGGRTYPE;

typedef struct
{
    T_AGGRTYPE type;
    DataType valtype;
    int length;
    int decimals;
    int cnt_idx;
    int significant;            /* length of operand of agg */
    int argcnt;
    // 필요하기는 한데... 이걸 제거할 수 있는 방법을 생각할것(MIN/MAX 때문에..)
    MDB_COL_TYPE collation;
} T_AGGRDESC;

typedef enum
{
    SPT_NONE = 0,
    SPT_OPERATOR,
    SPT_VALUE,
    SPT_FUNCTION,
    SPT_AGGREGATION,
    SPT_SUBQUERY,       /* pointer to subquery structure, that is t_select_ */
    SPT_SUBQ_OP,        /* SOT_IN, SOT_ALL_GT, SOT_EXISTS ... */
    SPT_DUMMY = MDB_INT_MAX     /* to guarantee sizeof(enum) == 4 */
} E_POSTFIXELEMTYPE;

typedef enum
{
    ORT_NONE = 0x00,
    ORT_SCANNABLE = 0x01,
    ORT_FILTERABLE = 0x02,
    ORT_INDEXABLE = 0x04,
    ORT_JOINABLE = 0x08,
    ORT_REVERSED = 0x10,
    ORT_INDEXRANGED = 0x20,
    ORT_DUMMY = MDB_INT_MAX     /* to guarantee sizeof(enum) == 4 */
} T_ORTYPE;

typedef enum
{
    PRED_NONE = 0,
    PRED_SCAN,
    PRED_JOIN,
    PRED_MULTIJOIN,
    PRED_DUMMY = MDB_INT_MAX    /* to guarantee sizeof(enum) == 4 */
} T_PREDTYPE;

typedef struct
{
    E_POSTFIXELEMTYPE elemtype;
    ibool is_distinct;
    MDB_UINT32 m_offset;
    char *m_name;
    union
    {
        T_VALUEDESC value;
        T_OPDESC op;
        T_FUNCDESC func;
        T_AGGRDESC aggr;
        struct t_select_ *subq; /* T_SELECT ?? */
    } u;
} T_POSTFIXELEM;

typedef struct
{
    int len;
    int max;
    T_POSTFIXELEM **list;
} T_EXPRESSIONDESC;

typedef struct
{
    T_OPTYPE type;
    T_EXPRESSIONDESC expr;
} T_KEY_EXPRDESC;

typedef struct
{
    T_ORTYPE type;
    T_EXPRESSIONDESC *expr;
} T_OR_EXPRDESC;

typedef struct
{
    int len;
    int max;
    char **list;
} T_LIST_FIELDS;

typedef enum
{
    ASCENDING,
    DESCENDING,
    ORDERTYPE_DUMMY = MDB_INT_MAX       /* to guarantee sizeof(enum) == 4 */
} ORDER_TYPE;

typedef struct
{
    MDB_COL_TYPE collation;
    char ordertype;             /* 'A': ASC, 'D': DESC */
    char *name;
} T_IDX_FIELD;

typedef struct
{
    int len;
    int max;
    T_IDX_FIELD *list;
} T_IDX_LIST_FIELDS;

typedef enum
{
    SMT_NONE = 0,               /* scan method type: none */
    SMT_SSCAN,  /* scan method type: sequential scan */
    SMT_ISCAN,  /* scan method type: index scan */
    SMT_DUMMY = MDB_INT_MAX     /* to guarantee sizeof(enum) == 4 */
} T_SCANTYPE;

typedef struct
{
    T_SCANTYPE scan_type;
    char *indexname;
    T_IDX_LIST_FIELDS fields;
} T_SCANHINT;

typedef enum
{
    SJT_NONE = 0,
    SJT_CROSS_JOIN,
    SJT_INNER_JOIN,
    SJT_LEFT_OUTER_JOIN,
    SJT_RIGHT_OUTER_JOIN,
    SJT_FULL_OUTER_JOIN,
    SJT_DUMMY = MDB_INT_MAX     /* to guarantee sizeof(enum) == 4 */
} T_JOINTYPE;

typedef struct
{
    T_JOINTYPE join_type;
    T_EXPRESSIONDESC condition;
    T_TABLEDESC table;
    T_SCANHINT scan_hint;
    SYSTABLE_T tableinfo;
    /* fromlist에 저장된 table의 modified_times */
    unsigned int modified_times;
    SYSFIELD_T *fieldinfo;
    EXPRDESC_LIST *expr_list;
} T_JOINTABLEDESC;

typedef struct
{
    int len;
    int max;
    T_JOINTABLEDESC *list;
    ibool outer_join_exists;
} T_LIST_JOINTABLEDESC;

typedef struct
{
    char res_name[FIELD_NAME_LENG];
    ibool is_distinct;
    T_EXPRESSIONDESC expr;
} T_SELECTION;

typedef struct
{
    int len;
    int max;
    int aliased_len;            // (subquery) as T(a, b)인 경우
    // 2 <= (a, b)
    T_SELECTION *list;
} T_LIST_SELECTION;

typedef struct
{
    int s_ref;                  // selection list reference.
    int i_ref;                  // field info reference.
    T_ATTRDESC attr;
} T_GROUPBYITEM;

typedef struct
{
    int len;
    int max;
    T_GROUPBYITEM *list;
    T_EXPRESSIONDESC having;
    char distinct_table[REL_NAME_LENG];
} T_GROUPBYDESC;

typedef struct
{
    char ordertype;             /* 'A': ASC, 'D': DESC */
    int s_ref;                  // selection list reference.
    int i_ref;                  // field info. reference.
    int param_idx;
    T_ATTRDESC attr;
} T_ORDERBYITEM;

typedef struct
{
    int len;
    int max;
    T_ORDERBYITEM *list;
} T_ORDERBYDESC;

typedef struct
{
    int offset;
    int rows;

    OID start_at;
    int startat_param_idx;
    int offset_param_idx;
    int rows_param_idx;
} T_LIMITDESC;

typedef struct
{
    ihandle Htable;
    char *table;
    char *index;
    SYSFIELD_T *fieldinfo;
    int fieldnum;
    char *rec_buf;
    int rec_buf_len;
    int nullflag_offset;
    int column_index;
    int field_index;
} T_DISTINCTINFO;

typedef struct
{
    int len;
    T_DISTINCTINFO *distinct_table;
} T_DISTINCTDESC;

typedef struct
{
    int posi_in_having;
    int posi_in_fields;
    int idx_of_count;
} T_HAVINGINFO;

typedef struct
{
    int len;
    T_HAVINGINFO *hvinfo;
} T_HAVINGDESC;

typedef struct
{
    char res_name[FIELD_NAME_LENG];
    int len;                    // for mdb_malloc()
    T_VALUEDESC value;
} T_RESULTCOLUMN;

typedef struct
{
    int len;
    int n_sel_avg;
    int n_not_sref_gby;
    int n_hv_avg;
    int n_expr;
    int use_dosimple;
    T_RESULTCOLUMN *aggr;
} T_AGGRINFO;

// T_QUERYDESC: select 문 하나에 대응하는 구조체
typedef struct
{
    ibool is_distinct;
    T_QUERYTYPE querytype;      // whether normal or aggregative type
    T_LIST_SELECTION selection;
    T_LIST_JOINTABLEDESC fromlist;
    T_EXPRESSIONDESC condition; // postfix 사용
    T_GROUPBYDESC groupby;
    T_ORDERBYDESC orderby;
    T_LIMITDESC limit;
    T_EXPRESSIONDESC setlist;
    T_DISTINCTDESC distinct;
    T_HAVINGDESC having;
    T_AGGRINFO aggr_info;
    /* result of cnf normalization */
    EXPRDESC_LIST *expr_list;
    // correlated subquery일 경우 correlated column을 모두 링크시킴
    // subquery 수행시 이 리스트를 따라 가면서 correlated column 값을 치환함
    CORRELATED_ATTR *correl_attr;
    int select_star;
} T_QUERYDESC;

typedef struct Cursor_Position t_cursorposi;

// rttable의 커서 상태를 표시
typedef enum
{
    SCS_INITIAL_VACANCY = -2,   // from clause subquery에서 subq결과 테이블에 
    // 데에터를 만들어 내기 전의 상태.
    SCS_REOPEN,
    SCS_CLOSE,
    SCS_OPEN,
    SCS_FORWARD,
    SCS_REPOSITION,
    SCS_SM_LAST,
    SCS_DUMMY = MDB_INT_MAX     /* to guarantee sizeof(enum) == 4 */
} E_CURSORSTATUS;

#define PREV_REC_MAX_CNT  5

typedef struct
{
    int cnt_merge_item;
    T_VALUEDESC *merge_key_values;
    t_cursorposi *before_cursor_posi;
    T_TABLEDESC table;
    ihandle Hcursor;
    E_CURSORSTATUS status;
    FIELDVALUES_T *rec_values;  /* select */
    /* use nullflag_offset, rec_len and rec_buf 
       for insert, update or some temp table related works. */
    unsigned int nullflag_offset;
    int rec_len;
    char *rec_buf;
    int table_ordinal;
    int jump2offset;
    OID start_at;
    int in_expr_idx;

    FIELDVALUES_T *save_rec_values;
    FIELDVALUES_T *prev_rec_values;
    short prev_rec_max_cnt;
    short prev_rec_cnt;
    short prev_rec_idx;
    char has_prev_cursor_posi;  /* flag : 0 or 1 */
    char has_next_record_read;  /* flag : 0 or 1 */
    EXPRDESC_LIST *db_expr_list;
    EXPRDESC_LIST *sql_expr_list;
    DB_UINT8 msync_flag;
} T_RTTABLE;

typedef struct
{
    SYSINDEX_T info;
    SYSINDEXFIELD_T *fields;
} T_INDEXINFO;

typedef struct KeyValue T_KEYDESC;
typedef struct Filter T_FILTER;

typedef struct t_iscanitem
{
    SYSINDEX_T indexinfo;
    SYSINDEXFIELD_T *indexfields;
    T_OR_EXPRDESC **orexprs;
    short match_ifield_count;
    short index_orexpr_count;
    short match_orexpr_count;
    short unmatch_scan_count;
    short filter_orexpr_count;
    char index_equal_op_flag;
    char recompute_cost_flag;
    char chosen_by_hint_flag;
    struct t_iscanitem *next;
} T_ISCANITEM;

typedef ORDER_TYPE OPENTYPE;

typedef struct
{
    T_TABLEDESC table;
    int op_like;                // LIKE로 이뤄진 넘들을
    // 나타내기 위한 bit-vector
    // int type임을 주의하자.
    // 즉, 최대 index field는
    // 32개까지만 가능하다.
    char *indexname;
    int is_temp_index;
    int is_subq_orderby_index;
    int keyins_flag;
    SCANDESC_T *scandesc;
    int index_field_num;
    OPENTYPE opentype;

    T_ATTRDESC *index_field_info;
    T_KEY_EXPRDESC *min;
    T_KEY_EXPRDESC *max;
    T_FILTER *filter;
    int cnt_merge_item;         // for SELECT statement
    int cnt_single_item;

    OID scanrid;
    int ridin_cnt;
    int ridin_row;
    int is_param_scanrid;

    /* one table related expression list */
    ATTRED_EXPRDESC_LIST *attr_expr_list;
    short total_orexpr_count;
    short joinable_orexpr_count;
    short indexable_orexpr_count;
    T_OR_EXPRDESC **indexable_orexprs;
    T_ISCANITEM *iscan_list;

    int in_exprs;
    int op_in;
} T_NESTING;

typedef struct
{
    T_QUERYDESC querydesc;
    T_NESTING nest[MAX_JOIN_TABLE_NUM];
} T_PLANNEDQUERYDESC;

typedef struct
{
    int len;
    int max;
    int include_rid;
    T_RESULTCOLUMN *list;
} T_LIST_RESULTCOLUMN;

typedef T_LIST_RESULTCOLUMN T_QUERYRESULT;

typedef struct
{
    int len;
    int max;
    T_EXPRESSIONDESC *list;
} T_LIST_VALUE;

#define SQL_PARSING_MEMORY_CHUNK_SIZE   (1024 * 10)

#define SQL_PARSING_MEMORY_CHUNK_NUM    100
typedef struct
{
    char *buf[SQL_PARSING_MEMORY_CHUNK_NUM];
    char *buf_end[SQL_PARSING_MEMORY_CHUNK_NUM];
    int pos[SQL_PARSING_MEMORY_CHUNK_NUM];
    int chunk_size[SQL_PARSING_MEMORY_CHUNK_NUM];       // aligned chunk size
    int alloc_chunk_num;        // allocate chunk num : start 0
} T_PARSING_MEMORY;


typedef enum
{
    TSS_RAW = 0,
    TSS_PARSED,
    TSS_EXECUTED,       // uncorrelated subquery의 경우 이미 수행했음을 나타냄
    TSS_END,
    TSS_DUMMY = MDB_INT_MAX     /* to guarantee sizeof(enum) == 4 */
} E_TSELECT_STATUS;

typedef struct t_select_
{
    int rows;
    int *rowsp;
    int param_count;
    T_PLANNEDQUERYDESC planquerydesc;
    T_RTTABLE rttable[MAX_JOIN_TABLE_NUM];
    T_QUERYRESULT queryresult;

    T_LIST_VALUE values;
    char *wTable;
    // 테이블의 종류 : 임시테이블일 그 종류를 표시
    int kindofwTable;
    /* orderby, group by, aggregation에 의해 생성된 temp table의 
       재생성 check */
    DB_BOOL is_retemp;
    DB_UINT8 msync_flag;

    // INSERT .. SELECT 와 CREATE .. AS SELECT 의 경우 상태가 TSS_PARSED. 
    // SUBQUERY를 수행해서 결과를 만들어 냈으면 TSS_EXECUTED로 설정
    E_TSELECT_STATUS tstatus;   //

    int handle;                 // connection id ?
    struct t_select_ *first_sub;
    struct t_select_ *sibling_query;
    struct t_select_ *super_query;
    struct t_select_ *tmp_sub;
    int rttable_idx;            // from절 subquery인 경우 super_query에서의 위치
    struct _tag_STATEMENT *born_stmt;

    // subquery의 결과를 저장
    // correlated subquery의 경우 한번 할당한 것은 재 사용. 
    // query결과의 크기는 t_select_.rows 에 저장함
    iSQL_ROWS *subq_result_rows;

// FIXU : sub query인 경우.. main stmt's 정보가 이곳에 들어간다
//        최종적으로는 없어져야 하는넘.. -_-
    T_PARSING_MEMORY *main_parsing_chunk;

} T_SELECT;

typedef enum
{
    SIT_NONE = 0,
    SIT_VALUES,
    SIT_QUERY,
    SIT_DUMMY = MDB_INT_MAX     /* to guarantee sizeof(enum) == 4 */
} T_INSERTTYPE;

typedef struct
{
    int rows;
    int param_count;
    T_INSERTTYPE type;          // values or query spec.
    T_TABLEDESC table;
    SYSTABLE_T tableinfo;
    SYSFIELD_T *fieldinfo;
    T_LIST_COLDESC columns;
    union
    {
        T_LIST_VALUE values;
        T_SELECT query;
    } u;
    T_RTTABLE rttable[1];
    ibool is_upsert_msync;
} T_INSERT;

typedef struct
{
    int rows;
    int param_count;
    T_PLANNEDQUERYDESC planquerydesc;
    T_LIST_COLDESC columns;
    T_LIST_VALUE values;
    ibool has_variable;
    T_RTTABLE rttable[1];
    OID rid;
    struct t_select_ subselect;
} T_UPDATE;

typedef struct
{
    int rows;
    int param_count;
    T_PLANNEDQUERYDESC planquerydesc;
    T_RTTABLE rttable[1];
    T_SELECT subselect;

} T_DELETE;

typedef struct
{
    char *objectname;
} T_TRUNCATE;

typedef struct
{
    int table_type;
    T_TABLEDESC table;
    T_LIST_FIELDS fields;
    T_LIST_ATTRDESC col_list;
} T_CREATETABLE;

typedef struct
{
    int table_type;

    T_TABLEDESC table;
    T_SELECT query;
} T_CREATETABLE_QUERY;

typedef struct
{
    T_TABLEDESC table;
    char *indexname;
    MDB_UINT8 uniqueness;
    T_IDX_LIST_FIELDS fields;
} T_CREATEINDEX;

typedef struct
{
    char *name;
    char *qstring;
    T_LIST_ATTRDESC columns;
    T_SELECT query;
} T_CREATEVIEW;

typedef struct
{
    char *name;
    MDB_INT32 start;
    MDB_INT32 minValue;
    MDB_INT32 maxValue;
    MDB_INT32 increment;
    MDB_INT8 bStart;
    MDB_INT8 bMinValue;
    MDB_INT8 bMaxValue;
    MDB_INT8 bIncrement;
    MDB_INT8 cycled;
} T_SEQUENCE;

typedef struct
{
    char **objectname;
    int num_object;
} T_REBUILD_IDX;

typedef enum
{
    SCT_TABLE_ELEMENTS,
    SCT_USER,
    SCT_TABLE_QUERY,
    SCT_INDEX,
    SCT_VIEW,
    SCT_SEQUENCE,
    SCT_DUMMY = MDB_INT_MAX     /* to guarantee sizeof(enum) == 4 */
} T_CREATETYPE;

typedef struct
{
    T_CREATETYPE type;
    union
    {
        T_CREATETABLE table_elements;
        T_CREATETABLE_QUERY table_query;
        T_CREATEINDEX index;
        T_CREATEVIEW view;
        T_SEQUENCE sequence;
    } u;
} T_CREATE;

typedef enum
{
    SAT_ADD = 1,
    SAT_ALTER,
    SAT_DROP,
    SAT_USER,
    SAT_SEQUENCE,
    SAT_REBUILD,
    SAT_LOGGING,
    SAT_MSYNC,
    SAT_TYPE_DUMMY = MDB_INT_MAX        /* to guarantee sizeof(enum) == 4 */
} T_ALTERTYPE;

typedef enum
{
    SAT_COLUMN,
    SAT_CONSTRAINT,
    SAT_SCOPE_DUMMY = MDB_INT_MAX       /* to guarantee sizeof(enum) == 4 */
} T_ALTERSCOPE;

typedef enum
{
    SAT_KEY = 0x01,
    SAT_NULLABLE = 0x02,
    SAT_TYPE = 0x04,
    SAT_DEFAULT = 0x08,
    SAT_FLAG_DUMMY = MDB_INT_MAX        /* to guarantee sizeof(enum) == 4 */
} T_ALTERFLAG;

typedef enum
{
    SAT_PRIMARY_KEY = 1
} T_CONSTRAINTTYPE;

typedef struct
{
    T_LIST_FIELDS name_list;
    T_LIST_ATTRDESC attr_list;
} T_ALTER_COLUMN;

typedef struct
{
    T_CONSTRAINTTYPE type;
    T_LIST_FIELDS list;
} T_ALTER_CONSTRAINT;

typedef struct
{
    int on;
    int runtime;
} T_ALTER_LOGGING;

typedef struct
{
    T_ALTERTYPE type;
    T_ALTERSCOPE scope;
    char *tablename;
    T_ALTER_COLUMN column;
    T_ALTER_CONSTRAINT constraint;
    SYSFIELD_T *old_fields;
    int old_fieldnum;
    FIELDDESC_T *fielddesc;
    int fieldnum;
    char *real_tablename;
    ibool having_data;
    T_ALTER_LOGGING logging;
    MDB_INT8 msync_alter_type;
    union
    {
        T_SEQUENCE sequence;
        T_REBUILD_IDX rebuild_idx;
    } u;
} T_ALTER;

typedef enum
{
    SDT_TABLE,
    SDT_INDEX,
    SDT_FUNCTION,
    SDT_VIEW,
    SDT_SEQUENCE,
    SDT_RID,
    SDT_DUMMY = MDB_INT_MAX     /* to guarantee sizeof(enum) == 4 */
} T_DROPTYPE;

typedef struct
{
    T_DROPTYPE type;
    char *target;
} T_DROP;

typedef enum
{
    SRT_TABLE,
    SRT_INDEX,
    SRT_DUMMY = MDB_INT_MAX     /* to guarantee sizeof(enum) == 4 */
} T_RENAMETYPE;

typedef struct
{
    T_RENAMETYPE type;
    char *oldname;
    char *newname;
} T_RENAME;

typedef enum
{
    SBT_DESC_TABLE,
    SBT_SHOW_TABLE,
#if defined(USE_DESC_COMPILE_OPT)
    SBT_DESC_COMPILE_OPT,
#endif
    SBT_DUMMY = MDB_INT_MAX     /* to guarantee sizeof(enum) == 4 */
} T_DESCTYPE;

typedef struct
{
    T_DESCTYPE type;
    char *describe_table;
    char *describe_column;
} T_DESCRIBE;

typedef enum
{
    SST_NONE = 0x0000,
    SST_AUTOCOMMIT = 0x0001,
    SST_TIME = 0x0002,
    SST_HEADING = 0x0004,
    SST_FEEDBACK = 0x0008,
    SST_RECONNECT = 0x0010,
    SST_PLAN_ON = 0x0020,
    SST_PLAN_OFF = 0x0040,
    SST_PLAN_ONLY = 0x0080,
    SST_DUMMY = MDB_INT_MAX     /* to guarantee sizeof(enum) == 4 */
} T_SETTYPE;

typedef struct
{
    T_SETTYPE type;
    union
    {
        ibool on_off;
    } u;
} T_SET;

typedef struct
{
    short int opened;
    short int when_to_open;
    MDB_UINT32 query_timeout;
} T_TRANSDESC;

typedef iSQL_RESULT_PAGE T_RESULT_PAGE;

struct T_RESULT_PAGE_header
{
    int body_size;
    int rows_in_page;
    T_RESULT_PAGE *next;
};

#define RESULT_PGHDR_SIZE    (sizeof(struct T_RESULT_PAGE_header))

typedef struct T_RESULT_LIST_tag
{
    int page_sz;
    int n_pages;
    char *cursor;               /* for record fetch */
    T_RESULT_PAGE *cur_pg;      /* for record fetch */
    T_RESULT_PAGE *head;
    T_RESULT_PAGE *tail;
} T_RESULT_LIST;

typedef struct T_RESULT_INFO_tag
{
    unsigned long int row_num;
    unsigned long int fetch_cnt;
    unsigned long int max_rec_len;
    unsigned long int tmp_rec_len;
    char *tmp_recbuf;
    unsigned int field_count;
    DataType *field_datatypes;
    int rid_included;
    T_RESULT_LIST *list;
} T_RESULT_INFO;

typedef enum
{
    SADM_NONE = 0x0000,
    SADM_EXPORT_ALL = 0x0001,
    SADM_EXPORT_ONE = 0x0002,
    SADM_IMPORT = 0x0003,
    SADM_DUMMY = MDB_INT_MAX    /* to guarantee sizeof(enum) == 4 */
} T_ADMINTYPE;

typedef struct
{
    char *table_name;
    char *export_file;
    int f_append;
    int data_or_schema;
} T_ADMIN_EXPORT;

typedef struct
{
    char *table_name;
    char *export_file;
    int f_append;
} T_ADMIN_EXPORT_ONE;

typedef struct
{
    char *import_file;
} T_ADMIN_IMPORT;

typedef struct
{
    T_ADMINTYPE type;
    union
    {
        T_ADMIN_EXPORT export;
        T_ADMIN_IMPORT import;
    } u;
} T_ADMIN;

typedef struct _tag_STATEMENT
{
    unsigned int id;            /* statement id */
    int status;
    T_SQLTYPE sqltype;
    union
    {
        T_CREATE _create;
        T_ALTER _alter;
        T_DELETE _delete;
        T_TRUNCATE _truncate;
        T_DESCRIBE _desc;
        T_DROP _drop;
        T_RENAME _rename;
        T_INSERT _insert;
        T_SELECT _select;
        T_SET _set;
        T_UPDATE _update;
        T_ADMIN _admin;
    } u;

    char *query;
    unsigned int qlen;

    T_TRANSDESC *trans_info;

    T_RESULT_INFO *result;

    int field_count;
    int size_fields;
    void *fields;

    int is_partial_result;
    int affected_rid;

    struct _tag_STATEMENT *prev;
    struct _tag_STATEMENT *next;

    MDB_UINT32 query_timeout;

    MDB_BOOL is_main_stmt;      // stmt가 main인 경우 true.. sub query에서는 false
    MDB_BOOL do_recompile;      /* recompile before execution */

    T_PARSING_MEMORY *parsing_memory;

} T_STATEMENT;

typedef struct
{
    unsigned int qs_id;         /* query sequence id */
    short int stmt_num;
    T_STATEMENT stmts[1];
} T_SQL;

typedef struct
{
    int vars;
    int conds;
    int hndlrs;
    int curs;
} t_spblock;

typedef struct
{
    char _temp[4];
    char tablename[REL_NAME_LENG];
    int rec_len;
    int rec_cnt;
} T_EXPORT_FILE_HEADER;

typedef struct
{
    char fieldname[FIELD_NAME_LENG];
    char base_fieldname[FIELD_NAME_LENG];
    char tablename[MAX_TABLE_NAME_LEN];
    char default_value[MAX_DEFAULT_VALUE_LEN];
    int type;
    unsigned int length;
    unsigned int max_length;
    unsigned int flags;
    unsigned int decimals;
    unsigned int buffer_length; // 원래 의도는 value_lenth로 사용할려고 했음
} t_fieldinfo;

#endif // _SQL_DATAST_H
