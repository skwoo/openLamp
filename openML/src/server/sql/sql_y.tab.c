/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.5"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         __yyparse
#define yylex           __yylex
#define yyerror         __yyerror
#define yylval          __yylval
#define yychar          __yychar
#define yydebug         __yydebug
#define yynerrs         __yynerrs


/* Copy the first part of user declarations.  */

/* Line 268 of yacc.c  */
#line 21 "parser.y"


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



/* Line 268 of yacc.c  */
#line 260 "sql_y.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     IDENTIFIER = 258,
     STRINGVAL = 259,
     NSTRINGVAL = 260,
     BINARYVAL = 261,
     HEXADECIMALVAL = 262,
     INTNUM = 263,
     FLOATNUM = 264,
     APPROXNUM = 265,
     OR_SYM = 266,
     AND_SYM = 267,
     NOT_SYM = 268,
     MERGE = 269,
     COMPARISON = 270,
     NEG = 271,
     SET_OPERATION = 272,
     ADD_SYM = 273,
     ALL_SYM = 274,
     ALTER_SYM = 275,
     ANY_SYM = 276,
     AS_SYM = 277,
     ASC_SYM = 278,
     ATTACH_SYM = 279,
     AUTOCOMMIT_SYM = 280,
     AVG_SYM = 281,
     BETWEEN_SYM = 282,
     BIGINT_SYM = 283,
     BY_SYM = 284,
     BYTES_SYM = 285,
     NBYTES_SYM = 286,
     CHARACTER_SYM = 287,
     NCHARACTER_SYM = 288,
     CHECK_SYM = 289,
     CLOSE_SYM = 290,
     COMMIT_SYM = 291,
     FLUSH_SYM = 292,
     COUNT_SYM = 293,
     DIRTY_COUNT_SYM = 294,
     CONVERT_SYM = 295,
     CREATE_SYM = 296,
     CROSS_SYM = 297,
     CURRENT_SYM = 298,
     CURRENT_DATE_SYM = 299,
     CURRENT_TIME_SYM = 300,
     CURRENT_TIMESTAMP_SYM = 301,
     DATE_ADD_SYM = 302,
     DATE_DIFF_SYM = 303,
     DATE_FORMAT_SYM = 304,
     DATE_SUB_SYM = 305,
     TIME_ADD_SYM = 306,
     TIME_DIFF_SYM = 307,
     TIME_FORMAT_SYM = 308,
     TIME_SUB_SYM = 309,
     DATE_SYM = 310,
     DATETIME_SYM = 311,
     DECIMAL_SYM = 312,
     DECODE_SYM = 313,
     DEFAULT_SYM = 314,
     DELETE_SYM = 315,
     DESC_SYM = 316,
     DESCRIBE_SYM = 317,
     DISTINCT_SYM = 318,
     DOUBLE_SYM = 319,
     DROP_SYM = 320,
     ESCAPE_SYM = 321,
     EXISTS_SYM = 322,
     EXPLAIN_SYM = 323,
     FEEDBACK_SYM = 324,
     FLOAT_SYM = 325,
     FOREIGN_SYM = 326,
     FROM_SYM = 327,
     FULL_SYM = 328,
     GROUP_SYM = 329,
     HAVING_SYM = 330,
     HEADING_SYM = 331,
     HEXCOMP_SYM = 332,
     CHARTOHEX_SYM = 333,
     IFNULL_SYM = 334,
     IN_SYM = 335,
     INDEX_SYM = 336,
     INNER_SYM = 337,
     INSERT_SYM = 338,
     INT_SYM = 339,
     INTO_SYM = 340,
     IS_SYM = 341,
     ISCAN_SYM = 342,
     JOIN_SYM = 343,
     KEY_SYM = 344,
     LEFT_SYM = 345,
     LIKE_SYM = 346,
     ILIKE_SYM = 347,
     LIMIT_SYM = 348,
     LOWERCASE_SYM = 349,
     LTRIM_SYM = 350,
     MAX_SYM = 351,
     MIN_SYM = 352,
     LOGGING_SYM = 353,
     NOLOGGING_SYM = 354,
     NOW_SYM = 355,
     NULL_SYM = 356,
     NUMERIC_SYM = 357,
     OFF_SYM = 358,
     ON_SYM = 359,
     ONLY_SYM = 360,
     OPTION_SYM = 361,
     ORDER_SYM = 362,
     OUTER_SYM = 363,
     PLAN_SYM = 364,
     PRECISION = 365,
     PRIMARY_SYM = 366,
     REAL_SYM = 367,
     RECONNECT_SYM = 368,
     REFERENCES_SYM = 369,
     RENAME_SYM = 370,
     RIGHT_SYM = 371,
     ROLLBACK_SYM = 372,
     ROUND_SYM = 373,
     RTRIM_SYM = 374,
     SELECT_SYM = 375,
     SET_SYM = 376,
     SIGN_SYM = 377,
     SMALLINT_SYM = 378,
     SOME_SYM = 379,
     SSCAN_SYM = 380,
     STRING_SYM = 381,
     SUBSTRING_SYM = 382,
     SUM_SYM = 383,
     SYSDATE_SYM = 384,
     TABLE_SYM = 385,
     RIDTABLENAME_SYM = 386,
     TIME_SYM = 387,
     TIMESTAMP_SYM = 388,
     TINYINT_SYM = 389,
     TRUNC_SYM = 390,
     TRUNCATE_SYM = 391,
     UNIQUE_SYM = 392,
     UPDATE_SYM = 393,
     UPPERCASE_SYM = 394,
     VALUES_SYM = 395,
     VARCHAR_SYM = 396,
     NVARCHAR_SYM = 397,
     VIEW_SYM = 398,
     WHERE_SYM = 399,
     WORK_SYM = 400,
     RANDOM_SYM = 401,
     SRANDOM_SYM = 402,
     END_SYM = 403,
     END_OF_INPUT = 404,
     USING_SYM = 405,
     NSTRING_SYM = 406,
     ROWNUM_SYM = 407,
     SEQUENCE_SYM = 408,
     START_SYM = 409,
     AT_SYM = 410,
     WITH_SYM = 411,
     INCREMENT_SYM = 412,
     MAXVALUE_SYM = 413,
     NOMAXVALUE_SYM = 414,
     MINVALUE_SYM = 415,
     NOMINVALUE_SYM = 416,
     CYCLE_SYM = 417,
     NOCYCLE_SYM = 418,
     CURRVAL_SYM = 419,
     NEXTVAL_SYM = 420,
     RID_SYM = 421,
     REPLACE_SYM = 422,
     BYTE_SYM = 423,
     VARBYTE_SYM = 424,
     BINARY_SYM = 425,
     HEXADECIMAL_SYM = 426,
     BYTE_SIZE_SYM = 427,
     COPYFROM_SYM = 428,
     COPYTO_SYM = 429,
     OBJECTSIZE_SYM = 430,
     TABLEDATA_SYM = 431,
     TABLEINDEX_SYM = 432,
     COLLATION_SYM = 433,
     REBUILD_SYM = 434,
     EXPORT_SYM = 435,
     IMPORT_SYM = 436,
     HEXASTRING_SYM = 437,
     BINARYSTRING_SYM = 438,
     RUNTIME_SYM = 439,
     SHOW_SYM = 440,
     UPSERT_SYM = 441,
     DATA_SYM = 442,
     SCHEMA_SYM = 443,
     OID_SYM = 444,
     BEFORE_SYM = 445,
     ENABLE_SYM = 446,
     DISABLE_SYM = 447,
     RESIDENT_SYM = 448,
     AUTOINCREMENT_SYM = 449,
     BLOB_SYM = 450,
     INCLUDE_SYM = 451,
     COMPILE_OPT_SYM = 452,
     MSYNC_SYM = 453,
     SYNCED_RECORD_SYM = 454,
     INSERT_RECORD_SYM = 455,
     UPDATE_RECORD_SYM = 456,
     DELETE_RECORD_SYM = 457
   };
#endif
/* Tokens.  */
#define IDENTIFIER 258
#define STRINGVAL 259
#define NSTRINGVAL 260
#define BINARYVAL 261
#define HEXADECIMALVAL 262
#define INTNUM 263
#define FLOATNUM 264
#define APPROXNUM 265
#define OR_SYM 266
#define AND_SYM 267
#define NOT_SYM 268
#define MERGE 269
#define COMPARISON 270
#define NEG 271
#define SET_OPERATION 272
#define ADD_SYM 273
#define ALL_SYM 274
#define ALTER_SYM 275
#define ANY_SYM 276
#define AS_SYM 277
#define ASC_SYM 278
#define ATTACH_SYM 279
#define AUTOCOMMIT_SYM 280
#define AVG_SYM 281
#define BETWEEN_SYM 282
#define BIGINT_SYM 283
#define BY_SYM 284
#define BYTES_SYM 285
#define NBYTES_SYM 286
#define CHARACTER_SYM 287
#define NCHARACTER_SYM 288
#define CHECK_SYM 289
#define CLOSE_SYM 290
#define COMMIT_SYM 291
#define FLUSH_SYM 292
#define COUNT_SYM 293
#define DIRTY_COUNT_SYM 294
#define CONVERT_SYM 295
#define CREATE_SYM 296
#define CROSS_SYM 297
#define CURRENT_SYM 298
#define CURRENT_DATE_SYM 299
#define CURRENT_TIME_SYM 300
#define CURRENT_TIMESTAMP_SYM 301
#define DATE_ADD_SYM 302
#define DATE_DIFF_SYM 303
#define DATE_FORMAT_SYM 304
#define DATE_SUB_SYM 305
#define TIME_ADD_SYM 306
#define TIME_DIFF_SYM 307
#define TIME_FORMAT_SYM 308
#define TIME_SUB_SYM 309
#define DATE_SYM 310
#define DATETIME_SYM 311
#define DECIMAL_SYM 312
#define DECODE_SYM 313
#define DEFAULT_SYM 314
#define DELETE_SYM 315
#define DESC_SYM 316
#define DESCRIBE_SYM 317
#define DISTINCT_SYM 318
#define DOUBLE_SYM 319
#define DROP_SYM 320
#define ESCAPE_SYM 321
#define EXISTS_SYM 322
#define EXPLAIN_SYM 323
#define FEEDBACK_SYM 324
#define FLOAT_SYM 325
#define FOREIGN_SYM 326
#define FROM_SYM 327
#define FULL_SYM 328
#define GROUP_SYM 329
#define HAVING_SYM 330
#define HEADING_SYM 331
#define HEXCOMP_SYM 332
#define CHARTOHEX_SYM 333
#define IFNULL_SYM 334
#define IN_SYM 335
#define INDEX_SYM 336
#define INNER_SYM 337
#define INSERT_SYM 338
#define INT_SYM 339
#define INTO_SYM 340
#define IS_SYM 341
#define ISCAN_SYM 342
#define JOIN_SYM 343
#define KEY_SYM 344
#define LEFT_SYM 345
#define LIKE_SYM 346
#define ILIKE_SYM 347
#define LIMIT_SYM 348
#define LOWERCASE_SYM 349
#define LTRIM_SYM 350
#define MAX_SYM 351
#define MIN_SYM 352
#define LOGGING_SYM 353
#define NOLOGGING_SYM 354
#define NOW_SYM 355
#define NULL_SYM 356
#define NUMERIC_SYM 357
#define OFF_SYM 358
#define ON_SYM 359
#define ONLY_SYM 360
#define OPTION_SYM 361
#define ORDER_SYM 362
#define OUTER_SYM 363
#define PLAN_SYM 364
#define PRECISION 365
#define PRIMARY_SYM 366
#define REAL_SYM 367
#define RECONNECT_SYM 368
#define REFERENCES_SYM 369
#define RENAME_SYM 370
#define RIGHT_SYM 371
#define ROLLBACK_SYM 372
#define ROUND_SYM 373
#define RTRIM_SYM 374
#define SELECT_SYM 375
#define SET_SYM 376
#define SIGN_SYM 377
#define SMALLINT_SYM 378
#define SOME_SYM 379
#define SSCAN_SYM 380
#define STRING_SYM 381
#define SUBSTRING_SYM 382
#define SUM_SYM 383
#define SYSDATE_SYM 384
#define TABLE_SYM 385
#define RIDTABLENAME_SYM 386
#define TIME_SYM 387
#define TIMESTAMP_SYM 388
#define TINYINT_SYM 389
#define TRUNC_SYM 390
#define TRUNCATE_SYM 391
#define UNIQUE_SYM 392
#define UPDATE_SYM 393
#define UPPERCASE_SYM 394
#define VALUES_SYM 395
#define VARCHAR_SYM 396
#define NVARCHAR_SYM 397
#define VIEW_SYM 398
#define WHERE_SYM 399
#define WORK_SYM 400
#define RANDOM_SYM 401
#define SRANDOM_SYM 402
#define END_SYM 403
#define END_OF_INPUT 404
#define USING_SYM 405
#define NSTRING_SYM 406
#define ROWNUM_SYM 407
#define SEQUENCE_SYM 408
#define START_SYM 409
#define AT_SYM 410
#define WITH_SYM 411
#define INCREMENT_SYM 412
#define MAXVALUE_SYM 413
#define NOMAXVALUE_SYM 414
#define MINVALUE_SYM 415
#define NOMINVALUE_SYM 416
#define CYCLE_SYM 417
#define NOCYCLE_SYM 418
#define CURRVAL_SYM 419
#define NEXTVAL_SYM 420
#define RID_SYM 421
#define REPLACE_SYM 422
#define BYTE_SYM 423
#define VARBYTE_SYM 424
#define BINARY_SYM 425
#define HEXADECIMAL_SYM 426
#define BYTE_SIZE_SYM 427
#define COPYFROM_SYM 428
#define COPYTO_SYM 429
#define OBJECTSIZE_SYM 430
#define TABLEDATA_SYM 431
#define TABLEINDEX_SYM 432
#define COLLATION_SYM 433
#define REBUILD_SYM 434
#define EXPORT_SYM 435
#define IMPORT_SYM 436
#define HEXASTRING_SYM 437
#define BINARYSTRING_SYM 438
#define RUNTIME_SYM 439
#define SHOW_SYM 440
#define UPSERT_SYM 441
#define DATA_SYM 442
#define SCHEMA_SYM 443
#define OID_SYM 444
#define BEFORE_SYM 445
#define ENABLE_SYM 446
#define DISABLE_SYM 447
#define RESIDENT_SYM 448
#define AUTOINCREMENT_SYM 449
#define BLOB_SYM 450
#define INCLUDE_SYM 451
#define COMPILE_OPT_SYM 452
#define MSYNC_SYM 453
#define SYNCED_RECORD_SYM 454
#define INSERT_RECORD_SYM 455
#define UPDATE_RECORD_SYM 456
#define DELETE_RECORD_SYM 457




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 293 of yacc.c  */
#line 203 "parser.y"

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



/* Line 293 of yacc.c  */
#line 725 "sql_y.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */

/* Line 343 of yacc.c  */
#line 228 "parser.y"


#include "sql_y.tab.h"

#define __yylex    my_yylex
int                my_yylex(YYSTYPE *yylval_param, void *__yacc);


#ifdef YYPARSE_PARAM
#undef YYPARSE_PARAM
#endif
#define YYPARSE_PARAM   __yacc

#define YYLEX_PARAM     __yacc



/* Line 343 of yacc.c  */
#line 755 "sql_y.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  87
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2519

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  214
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  182
/* YYNRULES -- Number of rules.  */
#define YYNRULES  508
/* YYNRULES -- Number of states.  */
#define YYNSTATES  1023

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   457

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,    20,     2,     2,
     209,   210,    18,    16,   211,    17,   212,    19,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   213,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    21,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,   106,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,   158,   159,   160,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   189,   190,
     191,   192,   193,   194,   195,   196,   197,   198,   199,   200,
     201,   202,   203,   204,   205,   206,   207,   208
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     8,    12,    14,    16,    18,    20,
      22,    24,    26,    28,    30,    32,    34,    36,    38,    40,
      42,    44,    46,    48,    49,    58,    59,    70,    71,    72,
      80,    81,    82,    89,    90,    96,    97,   102,   103,   108,
     109,   114,   115,   119,   121,   122,   126,   128,   129,   131,
     133,   134,   140,   144,   146,   148,   149,   151,   153,   155,
     157,   159,   160,   164,   165,   166,   170,   172,   176,   178,
     180,   181,   186,   188,   193,   198,   201,   207,   210,   216,
     218,   220,   222,   224,   229,   236,   238,   241,   243,   248,
     250,   252,   254,   259,   266,   268,   270,   275,   282,   284,
     286,   292,   298,   306,   314,   316,   321,   326,   333,   335,
     337,   340,   341,   344,   346,   349,   351,   355,   358,   363,
     366,   372,   374,   375,   377,   382,   383,   390,   398,   409,
     414,   416,   420,   422,   426,   428,   432,   433,   435,   437,
     441,   444,   447,   451,   455,   456,   462,   463,   464,   471,
     475,   479,   481,   485,   487,   491,   495,   498,   501,   504,
     509,   512,   515,   519,   524,   526,   529,   531,   533,   535,
     539,   541,   545,   549,   553,   557,   561,   565,   571,   577,
     579,   582,   585,   586,   587,   596,   597,   604,   605,   609,
     610,   614,   619,   621,   623,   627,   628,   630,   631,   642,
     644,   647,   650,   652,   654,   656,   657,   664,   665,   667,
     669,   671,   673,   678,   684,   685,   687,   689,   693,   695,
     697,   699,   701,   702,   704,   706,   708,   710,   713,   718,
     719,   722,   725,   726,   730,   731,   739,   744,   746,   750,
     754,   757,   761,   763,   765,   766,   771,   772,   777,   780,
     783,   787,   794,   796,   798,   801,   804,   807,   810,   813,
     817,   821,   822,   826,   827,   833,   834,   836,   838,   839,
     846,   847,   850,   853,   858,   866,   875,   884,   893,   897,
     899,   901,   904,   905,   907,   909,   913,   920,   921,   925,
     927,   931,   933,   934,   936,   939,   942,   947,   949,   953,
     954,   956,   959,   960,   965,   967,   971,   973,   975,   976,
     979,   980,   984,   985,   988,   991,   996,  1001,  1004,  1008,
    1010,  1013,  1015,  1018,  1020,  1024,  1028,  1031,  1035,  1036,
    1038,  1041,  1042,  1044,  1046,  1048,  1052,  1056,  1059,  1063,
    1065,  1067,  1069,  1071,  1073,  1075,  1077,  1079,  1081,  1085,
    1089,  1094,  1096,  1103,  1109,  1115,  1120,  1122,  1124,  1125,
    1128,  1133,  1137,  1142,  1146,  1153,  1159,  1161,  1165,  1170,
    1172,  1174,  1176,  1179,  1183,  1187,  1191,  1195,  1199,  1203,
    1207,  1211,  1214,  1217,  1219,  1221,  1223,  1225,  1227,  1229,
    1233,  1235,  1237,  1240,  1241,  1243,  1246,  1249,  1251,  1256,
    1261,  1266,  1268,  1273,  1278,  1283,  1288,  1290,  1292,  1294,
    1296,  1298,  1300,  1302,  1304,  1306,  1308,  1310,  1317,  1322,
    1326,  1328,  1332,  1339,  1346,  1355,  1362,  1367,  1372,  1379,
    1384,  1391,  1398,  1407,  1412,  1417,  1422,  1429,  1434,  1441,
    1450,  1455,  1460,  1465,  1467,  1469,  1471,  1473,  1477,  1486,
    1495,  1504,  1511,  1520,  1529,  1538,  1545,  1547,  1551,  1555,
    1560,  1565,  1572,  1579,  1581,  1583,  1585,  1587,  1589,  1593,
    1598,  1605,  1607,  1609,  1611,  1614,  1617,  1620,  1622,  1624,
    1626,  1628,  1630,  1631,  1633,  1635,  1639,  1641,  1643,  1645,
    1647,  1649,  1653,  1659,  1663,  1665,  1667,  1669,  1671,  1672,
    1674,  1681,  1685,  1687,  1689,  1691,  1694,  1695,  1697
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     215,     0,    -1,   155,    -1,   216,   155,    -1,   215,   216,
     155,    -1,   217,    -1,   218,    -1,   219,    -1,   220,    -1,
     258,    -1,   268,    -1,   269,    -1,   270,    -1,   271,    -1,
     274,    -1,   281,    -1,   283,    -1,   284,    -1,   298,    -1,
     304,    -1,   305,    -1,   310,    -1,   392,    -1,    -1,    47,
     221,   236,   136,   382,   238,   234,   239,    -1,    -1,    47,
     222,   255,    87,   389,   110,   382,   209,   256,   210,    -1,
      -1,    -1,    47,   223,   149,   383,   276,   224,   239,    -1,
      -1,    -1,    47,   225,   159,   390,   226,   227,    -1,    -1,
     229,   230,   231,   232,   233,    -1,    -1,   230,   231,   232,
     233,    -1,    -1,   160,   162,   381,     8,    -1,    -1,   163,
      35,   381,     8,    -1,    -1,   164,   381,     8,    -1,   165,
      -1,    -1,   166,   381,     8,    -1,   167,    -1,    -1,   168,
      -1,   169,    -1,    -1,    99,    35,     8,   156,   388,    -1,
      99,    35,     8,    -1,   197,    -1,   198,    -1,    -1,   105,
      -1,   199,    -1,   204,    -1,   104,    -1,   105,    -1,    -1,
     209,   241,   210,    -1,    -1,    -1,    28,   240,   285,    -1,
     242,    -1,   241,   211,   242,    -1,   243,    -1,   250,    -1,
      -1,   388,   245,   244,   247,    -1,    34,    -1,    36,   209,
       8,   210,    -1,    37,   209,     8,   210,    -1,    38,   339,
      -1,    38,   209,     8,   210,   339,    -1,    39,   339,    -1,
      39,   209,     8,   210,   339,    -1,    62,    -1,    61,    -1,
     138,    -1,    63,    -1,    63,   209,     8,   210,    -1,    63,
     209,     8,   211,     8,   210,    -1,    70,    -1,    70,   116,
      -1,    76,    -1,    76,   209,     8,   210,    -1,    90,    -1,
     195,    -1,   108,    -1,   108,   209,     8,   210,    -1,   108,
     209,     8,   211,     8,   210,    -1,   118,    -1,   129,    -1,
     132,   209,     8,   210,    -1,   132,   209,   246,   211,     8,
     210,    -1,   139,    -1,   140,    -1,   147,   209,     8,   210,
     339,    -1,   148,   209,     8,   210,   339,    -1,   147,   209,
     246,   211,     8,   210,   339,    -1,   148,   209,   246,   211,
       8,   210,   339,    -1,   174,    -1,   174,   209,     8,   210,
      -1,   175,   209,     8,   210,    -1,   175,   209,   246,   211,
       8,   210,    -1,   201,    -1,     8,    -1,    17,     8,    -1,
      -1,   247,   248,    -1,   107,    -1,    13,   107,    -1,   143,
      -1,   117,    95,   249,    -1,    65,   357,    -1,    40,   209,
     343,   210,    -1,   120,   382,    -1,   120,   382,   209,   253,
     210,    -1,   105,    -1,    -1,   200,    -1,   143,   209,   252,
     210,    -1,    -1,   117,    95,   251,   209,   252,   210,    -1,
      77,    95,   209,   252,   210,   120,   382,    -1,    77,    95,
     209,   252,   210,   120,   382,   209,   252,   210,    -1,    40,
     209,   343,   210,    -1,   388,    -1,   252,   211,   388,    -1,
     386,    -1,   253,   211,   386,    -1,   388,    -1,   254,   211,
     388,    -1,    -1,   143,    -1,   257,    -1,   256,   211,   257,
      -1,   388,   341,    -1,   388,   340,    -1,   388,   342,   340,
      -1,   388,   340,   342,    -1,    -1,    26,   136,   259,   382,
     264,    -1,    -1,    -1,    26,   159,   260,   390,   261,   228,
      -1,   185,    87,   262,    -1,   185,    87,    25,    -1,   263,
      -1,   262,   211,   263,    -1,     3,    -1,     3,   212,     3,
      -1,     3,   212,   117,    -1,    24,   266,    -1,    24,   250,
      -1,    26,   266,    -1,    26,   388,   121,   388,    -1,    26,
     250,    -1,    71,   267,    -1,    71,   117,    95,    -1,   237,
     209,   190,   210,    -1,   237,    -1,   204,   265,    -1,   235,
      -1,    43,    -1,   243,    -1,   266,   211,   243,    -1,   388,
      -1,   267,   211,   388,    -1,    71,   136,   382,    -1,    71,
      87,   389,    -1,    71,   149,   383,    -1,    71,   159,   390,
      -1,    71,   172,     8,    -1,   121,   136,   382,    28,   382,
      -1,   121,    87,   389,    28,   389,    -1,    42,    -1,    42,
     151,    -1,    42,    43,    -1,    -1,    -1,    66,   272,    78,
     320,   324,   326,   273,   333,    -1,    -1,    89,   275,    91,
     320,   277,   278,    -1,    -1,   209,   253,   210,    -1,    -1,
     209,   254,   210,    -1,   146,   209,   279,   210,    -1,   285,
      -1,   357,    -1,   279,   211,   357,    -1,    -1,   205,    -1,
      -1,   192,   282,    91,   320,   277,   146,   209,   279,   210,
     280,    -1,   123,    -1,   123,   151,    -1,   123,    43,    -1,
     285,    -1,   286,    -1,   289,    -1,    -1,   126,   287,   294,
     295,   314,   288,    -1,    -1,   205,    -1,   206,    -1,   207,
      -1,   208,    -1,   291,    23,   290,   291,    -1,   209,   289,
     210,   332,   333,    -1,    -1,    25,    -1,   292,    -1,   209,
     292,   210,    -1,   289,    -1,   286,    -1,    25,    -1,   382,
      -1,    -1,    25,    -1,    69,    -1,   296,    -1,    18,    -1,
     357,   297,    -1,   296,   211,   357,   297,    -1,    -1,   391,
       4,    -1,   391,   323,    -1,    -1,   144,   299,   300,    -1,
      -1,   320,   324,   127,   302,   326,   301,   333,    -1,   172,
       8,   127,   302,    -1,   303,    -1,   302,   211,   303,    -1,
     388,    15,   357,    -1,   142,   382,    -1,   142,   136,   382,
      -1,    67,    -1,    68,    -1,    -1,    67,   306,   382,   313,
      -1,    -1,    68,   307,   382,   313,    -1,    67,   203,    -1,
      68,   203,    -1,   191,   136,   293,    -1,   191,   136,   293,
      91,     4,   393,    -1,   110,    -1,   109,    -1,    31,   308,
      -1,   138,   308,    -1,    82,   308,    -1,    75,   308,    -1,
     119,   308,    -1,    74,   115,   308,    -1,    74,   115,   111,
      -1,    -1,   127,   311,   309,    -1,    -1,   127,   312,     3,
      15,   357,    -1,    -1,   388,    -1,     4,    -1,    -1,   316,
     326,   328,   332,   315,   333,    -1,    -1,    78,   317,    -1,
     320,   324,    -1,   317,   318,   320,   324,    -1,   317,    88,
      94,   320,   324,   110,   343,    -1,   317,    96,   319,    94,
     320,   324,   110,   343,    -1,   317,   122,   319,    94,   320,
     324,   110,   343,    -1,   317,    79,   319,    94,   320,   324,
     110,   343,    -1,   209,   317,   210,    -1,   211,    -1,    94,
      -1,    48,    94,    -1,    -1,   114,    -1,   382,    -1,   382,
     391,   384,    -1,   209,   285,   210,   391,   384,   321,    -1,
      -1,   209,   322,   210,    -1,   323,    -1,   322,   211,   323,
      -1,     3,    -1,    -1,   131,    -1,    93,   117,    -1,    93,
     389,    -1,    93,   209,   325,   210,    -1,   257,    -1,   325,
     211,   257,    -1,    -1,   327,    -1,   150,   343,    -1,    -1,
      80,    35,   329,   331,    -1,   330,    -1,   329,   211,   330,
      -1,   386,    -1,     8,    -1,    -1,    81,   343,    -1,    -1,
     113,    35,   337,    -1,    -1,    99,   336,    -1,    99,   334,
      -1,    99,   335,   211,   336,    -1,    99,   334,   211,   336,
      -1,   161,     8,    -1,   161,   213,   360,    -1,     8,    -1,
     213,   360,    -1,     8,    -1,   213,   360,    -1,   338,    -1,
     337,   211,   338,    -1,   386,   339,   341,    -1,     8,   341,
      -1,   213,   360,   341,    -1,    -1,   340,    -1,   184,     3,
      -1,    -1,   342,    -1,    29,    -1,    67,    -1,   343,    11,
     343,    -1,   343,    12,   343,    -1,    13,   343,    -1,   209,
     343,   210,    -1,   344,    -1,   345,    -1,   346,    -1,   347,
      -1,   350,    -1,   351,    -1,   353,    -1,   355,    -1,   365,
      -1,   357,    15,   357,    -1,   357,    92,   107,    -1,   357,
      92,    13,   107,    -1,   385,    -1,   357,    13,    33,   357,
      12,   357,    -1,   357,    33,   357,    12,   357,    -1,   357,
      13,   348,   357,   349,    -1,   357,   348,   357,   349,    -1,
      97,    -1,    98,    -1,    -1,    72,   358,    -1,   386,    92,
      13,   107,    -1,   386,    92,   107,    -1,   357,    13,    86,
     356,    -1,   357,    86,   356,    -1,   357,    13,    86,   209,
     352,   210,    -1,   357,    86,   209,   352,   210,    -1,   358,
      -1,   352,   211,   358,    -1,   357,    15,   354,   356,    -1,
      27,    -1,    25,    -1,   130,    -1,    73,   356,    -1,   209,
     285,   210,    -1,   357,    16,   357,    -1,   357,    17,   357,
      -1,   357,    18,   357,    -1,   357,    19,   357,    -1,   357,
      20,   357,    -1,   357,    21,   357,    -1,   357,    14,   357,
      -1,    16,   357,    -1,    17,   357,    -1,   358,    -1,   385,
      -1,   362,    -1,   363,    -1,   366,    -1,   356,    -1,   209,
     357,   210,    -1,   380,    -1,   359,    -1,   213,   360,    -1,
      -1,     8,    -1,    69,   357,    -1,    25,   357,    -1,   357,
      -1,    32,   209,   361,   210,    -1,    44,   209,    18,   210,
      -1,    44,   209,   361,   210,    -1,    45,    -1,    45,   209,
      18,   210,    -1,   102,   209,   361,   210,    -1,   103,   209,
     361,   210,    -1,   134,   209,   361,   210,    -1,   369,    -1,
     371,    -1,   370,    -1,   372,    -1,   377,    -1,   373,    -1,
     374,    -1,   375,    -1,   378,    -1,   358,    -1,   386,    -1,
      83,   209,   364,   211,   364,   210,    -1,    84,   209,   364,
     210,    -1,   357,   211,   357,    -1,   367,    -1,   368,   211,
     367,    -1,    46,   209,   245,   211,   357,   210,    -1,    64,
     209,   357,   211,   368,   210,    -1,    64,   209,   357,   211,
     368,   211,   357,   210,    -1,    85,   209,   357,   211,   357,
     210,    -1,   128,   209,   357,   210,    -1,   124,   209,   357,
     210,    -1,   124,   209,   357,   211,   357,   210,    -1,   141,
     209,   357,   210,    -1,   141,   209,   357,   211,   357,   210,
      -1,   133,   209,   357,   211,     8,   210,    -1,   133,   209,
     357,   211,     8,   211,     8,   210,    -1,   100,   209,   357,
     210,    -1,   145,   209,   357,   210,    -1,   101,   209,   357,
     210,    -1,   101,   209,   357,   211,   357,   210,    -1,   125,
     209,   357,   210,    -1,   125,   209,   357,   211,   357,   210,
      -1,   173,   209,   357,   211,   357,   211,   357,   210,    -1,
     137,   209,   357,   210,    -1,   188,   209,   357,   210,    -1,
     189,   209,   357,   210,    -1,    50,    -1,    51,    -1,    52,
      -1,   135,    -1,   106,   209,   210,    -1,    53,   209,     3,
     211,   357,   211,   357,   210,    -1,    56,   209,     3,   211,
     357,   211,   357,   210,    -1,    54,   209,     3,   211,   357,
     211,   357,   210,    -1,    55,   209,   357,   211,     4,   210,
      -1,    57,   209,     3,   211,   357,   211,   357,   210,    -1,
      60,   209,     3,   211,   357,   211,   357,   210,    -1,    58,
     209,     3,   211,   357,   211,   357,   210,    -1,    59,   209,
     357,   211,     4,   210,    -1,   158,    -1,     3,   212,   170,
      -1,     3,   212,   171,    -1,   178,   209,   357,   210,    -1,
     179,   209,   357,   210,    -1,   179,   209,   357,   211,   376,
     210,    -1,   180,   209,   357,   211,   357,   210,    -1,    38,
      -1,   147,    -1,   174,    -1,   175,    -1,   152,    -1,   152,
     209,   210,    -1,   153,   209,     8,   210,    -1,   181,   209,
     379,   211,   357,   210,    -1,   136,    -1,   182,    -1,   183,
      -1,   381,    10,    -1,   381,     9,    -1,   381,     8,    -1,
       4,    -1,     5,    -1,     6,    -1,     7,    -1,   107,    -1,
      -1,    17,    -1,     3,    -1,     3,   212,     3,    -1,     3,
      -1,     3,    -1,   386,    -1,   387,    -1,   388,    -1,     3,
     212,   388,    -1,     3,   212,     3,   212,   388,    -1,     3,
     212,   172,    -1,   172,    -1,     3,    -1,     3,    -1,     3,
      -1,    -1,    28,    -1,   186,   395,   394,    91,     4,   393,
      -1,   187,    78,     4,    -1,    47,    -1,    30,    -1,    25,
      -1,   136,   382,    -1,    -1,   193,    -1,   194,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   582,   582,   584,   588,   595,   596,   597,   601,   602,
     603,   604,   608,   609,   610,   611,   612,   613,   614,   615,
     619,   620,   621,   627,   626,   685,   684,   702,   709,   701,
     717,   724,   716,   731,   733,   742,   745,   756,   762,   776,
     782,   796,   802,   812,   823,   829,   839,   850,   856,   863,
     873,   885,   914,   950,   954,   962,   965,   969,   973,   980,
     984,   990,   992,  1000,  1007,  1006,  1028,  1029,  1033,  1039,
    1044,  1043,  1061,  1069,  1077,  1085,  1100,  1115,  1130,  1145,
    1153,  1161,  1169,  1177,  1187,  1202,  1210,  1218,  1226,  1234,
    1242,  1251,  1259,  1269,  1284,  1292,  1300,  1308,  1318,  1326,
    1334,  1349,  1364,  1381,  1398,  1406,  1414,  1422,  1432,  1445,
    1449,  1458,  1460,  1467,  1476,  1484,  1488,  1503,  1509,  1513,
    1517,  1521,  1534,  1537,  1543,  1548,  1547,  1552,  1556,  1560,
    1567,  1572,  1580,  1604,  1630,  1639,  1650,  1652,  1660,  1666,
    1674,  1680,  1686,  1692,  1702,  1701,  1712,  1718,  1711,  1723,
    1745,  1759,  1760,  1767,  1778,  1801,  1829,  1841,  1849,  1861,
    1878,  1886,  1893,  1899,  1905,  1911,  1919,  1923,  1930,  1936,
    1945,  1950,  1958,  1962,  1966,  1970,  1974,  1985,  1993,  2004,
    2008,  2012,  2020,  2031,  2019,  2054,  2053,  2085,  2087,  2090,
    2092,  2096,  2100,  2107,  2114,  2125,  2128,  2135,  2134,  2167,
    2171,  2175,  2182,  2186,  2187,  2225,  2224,  2365,  2368,  2372,
    2376,  2380,  2387,  2407,  2414,  2417,  2424,  2428,  2432,  2439,
    2516,  2520,  2528,  2531,  2535,  2542,  2548,  2558,  2584,  2620,
    2623,  2628,  2636,  2635,  2657,  2656,  2676,  2683,  2684,  2688,
    2719,  2725,  2734,  2743,  2752,  2751,  2765,  2764,  2777,  2789,
    2800,  2808,  2828,  2829,  2833,  2838,  2843,  2848,  2853,  2858,
    2863,  2872,  2871,  2880,  2879,  2889,  2890,  2892,  2906,  2902,
    2921,  2940,  2944,  2959,  2974,  2988,  3003,  3020,  3024,  3028,
    3029,  3030,  3033,  3035,  3039,  3045,  3051,  3091,  3093,  3097,
    3131,  3161,  3170,  3177,  3185,  3193,  3201,  3214,  3223,  3233,
    3262,  3297,  3303,  3305,  3317,  3318,  3322,  3348,  3387,  3390,
    3396,  3398,  3407,  3408,  3409,  3410,  3411,  3415,  3448,  3489,
    3514,  3542,  3567,  3596,  3597,  3601,  3631,  3690,  3701,  3704,
    3711,  3723,  3726,  3733,  3737,  3747,  3756,  3765,  3774,  3778,
    3785,  3789,  3793,  3797,  3801,  3805,  3809,  3813,  3820,  3864,
    3873,  3882,  3906,  3969,  4029,  4045,  4057,  4061,  4067,  4069,
    4076,  4110,  4145,  4154,  4163,  4172,  4184,  4188,  4195,  4237,
    4241,  4245,  4252,  4264,  4308,  4317,  4326,  4335,  4344,  4353,
    4362,  4371,  4375,  4393,  4397,  4401,  4405,  4409,  4413,  4417,
    4424,  4428,  4435,  4453,  4454,  4459,  4466,  4473,  4483,  4497,
    4507,  4521,  4531,  4541,  4555,  4569,  4587,  4591,  4595,  4599,
    4603,  4607,  4611,  4615,  4619,  4626,  4638,  4664,  4678,  4691,
    4698,  4702,  4709,  4739,  4766,  4794,  4806,  4815,  4833,  4842,
    4860,  4872,  4899,  4926,  4935,  4944,  4953,  4962,  4971,  4980,
    4990,  5003,  5012,  5024,  5032,  5040,  5048,  5056,  5065,  5087,
    5109,  5131,  5151,  5173,  5195,  5217,  5240,  5258,  5276,  5297,
    5305,  5325,  5344,  5355,  5359,  5363,  5367,  5374,  5383,  5392,
    5409,  5421,  5431,  5441,  5455,  5464,  5473,  5482,  5494,  5506,
    5528,  5550,  5562,  5565,  5572,  5577,  5585,  5593,  5601,  5630,
    5663,  5667,  5674,  5681,  5688,  5699,  5707,  5715,  5722,  5724,
    5728,  5744,  5757,  5761,  5768,  5772,  5780,  5783,  5787
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "IDENTIFIER", "STRINGVAL", "NSTRINGVAL",
  "BINARYVAL", "HEXADECIMALVAL", "INTNUM", "FLOATNUM", "APPROXNUM",
  "OR_SYM", "AND_SYM", "NOT_SYM", "MERGE", "COMPARISON", "'+'", "'-'",
  "'*'", "'/'", "'%'", "'^'", "NEG", "SET_OPERATION", "ADD_SYM", "ALL_SYM",
  "ALTER_SYM", "ANY_SYM", "AS_SYM", "ASC_SYM", "ATTACH_SYM",
  "AUTOCOMMIT_SYM", "AVG_SYM", "BETWEEN_SYM", "BIGINT_SYM", "BY_SYM",
  "BYTES_SYM", "NBYTES_SYM", "CHARACTER_SYM", "NCHARACTER_SYM",
  "CHECK_SYM", "CLOSE_SYM", "COMMIT_SYM", "FLUSH_SYM", "COUNT_SYM",
  "DIRTY_COUNT_SYM", "CONVERT_SYM", "CREATE_SYM", "CROSS_SYM",
  "CURRENT_SYM", "CURRENT_DATE_SYM", "CURRENT_TIME_SYM",
  "CURRENT_TIMESTAMP_SYM", "DATE_ADD_SYM", "DATE_DIFF_SYM",
  "DATE_FORMAT_SYM", "DATE_SUB_SYM", "TIME_ADD_SYM", "TIME_DIFF_SYM",
  "TIME_FORMAT_SYM", "TIME_SUB_SYM", "DATE_SYM", "DATETIME_SYM",
  "DECIMAL_SYM", "DECODE_SYM", "DEFAULT_SYM", "DELETE_SYM", "DESC_SYM",
  "DESCRIBE_SYM", "DISTINCT_SYM", "DOUBLE_SYM", "DROP_SYM", "ESCAPE_SYM",
  "EXISTS_SYM", "EXPLAIN_SYM", "FEEDBACK_SYM", "FLOAT_SYM", "FOREIGN_SYM",
  "FROM_SYM", "FULL_SYM", "GROUP_SYM", "HAVING_SYM", "HEADING_SYM",
  "HEXCOMP_SYM", "CHARTOHEX_SYM", "IFNULL_SYM", "IN_SYM", "INDEX_SYM",
  "INNER_SYM", "INSERT_SYM", "INT_SYM", "INTO_SYM", "IS_SYM", "ISCAN_SYM",
  "JOIN_SYM", "KEY_SYM", "LEFT_SYM", "LIKE_SYM", "ILIKE_SYM", "LIMIT_SYM",
  "LOWERCASE_SYM", "LTRIM_SYM", "MAX_SYM", "MIN_SYM", "LOGGING_SYM",
  "NOLOGGING_SYM", "NOW_SYM", "NULL_SYM", "NUMERIC_SYM", "OFF_SYM",
  "ON_SYM", "ONLY_SYM", "OPTION_SYM", "ORDER_SYM", "OUTER_SYM", "PLAN_SYM",
  "PRECISION", "PRIMARY_SYM", "REAL_SYM", "RECONNECT_SYM",
  "REFERENCES_SYM", "RENAME_SYM", "RIGHT_SYM", "ROLLBACK_SYM", "ROUND_SYM",
  "RTRIM_SYM", "SELECT_SYM", "SET_SYM", "SIGN_SYM", "SMALLINT_SYM",
  "SOME_SYM", "SSCAN_SYM", "STRING_SYM", "SUBSTRING_SYM", "SUM_SYM",
  "SYSDATE_SYM", "TABLE_SYM", "RIDTABLENAME_SYM", "TIME_SYM",
  "TIMESTAMP_SYM", "TINYINT_SYM", "TRUNC_SYM", "TRUNCATE_SYM",
  "UNIQUE_SYM", "UPDATE_SYM", "UPPERCASE_SYM", "VALUES_SYM", "VARCHAR_SYM",
  "NVARCHAR_SYM", "VIEW_SYM", "WHERE_SYM", "WORK_SYM", "RANDOM_SYM",
  "SRANDOM_SYM", "END_SYM", "END_OF_INPUT", "USING_SYM", "NSTRING_SYM",
  "ROWNUM_SYM", "SEQUENCE_SYM", "START_SYM", "AT_SYM", "WITH_SYM",
  "INCREMENT_SYM", "MAXVALUE_SYM", "NOMAXVALUE_SYM", "MINVALUE_SYM",
  "NOMINVALUE_SYM", "CYCLE_SYM", "NOCYCLE_SYM", "CURRVAL_SYM",
  "NEXTVAL_SYM", "RID_SYM", "REPLACE_SYM", "BYTE_SYM", "VARBYTE_SYM",
  "BINARY_SYM", "HEXADECIMAL_SYM", "BYTE_SIZE_SYM", "COPYFROM_SYM",
  "COPYTO_SYM", "OBJECTSIZE_SYM", "TABLEDATA_SYM", "TABLEINDEX_SYM",
  "COLLATION_SYM", "REBUILD_SYM", "EXPORT_SYM", "IMPORT_SYM",
  "HEXASTRING_SYM", "BINARYSTRING_SYM", "RUNTIME_SYM", "SHOW_SYM",
  "UPSERT_SYM", "DATA_SYM", "SCHEMA_SYM", "OID_SYM", "BEFORE_SYM",
  "ENABLE_SYM", "DISABLE_SYM", "RESIDENT_SYM", "AUTOINCREMENT_SYM",
  "BLOB_SYM", "INCLUDE_SYM", "COMPILE_OPT_SYM", "MSYNC_SYM",
  "SYNCED_RECORD_SYM", "INSERT_RECORD_SYM", "UPDATE_RECORD_SYM",
  "DELETE_RECORD_SYM", "'('", "')'", "','", "'.'", "'?'", "$accept",
  "query", "sql", "definitive_statement", "manipulative_statement",
  "private_statement", "create_statement", "$@1", "$@2", "$@3", "$@4",
  "$@5", "$@6", "SEQUENCE_DESCRIPTION_CREATE",
  "SEQUENCE_DESCRIPTION_ALTER", "SEQUENCE_DESC_START",
  "SEQUENCE_DESC_INCREMENT", "SEQUENCE_DESC_MAXVALUE",
  "SEQUENCE_DESC_MINVALUE", "SEQUENCE_DESC_CYCLE", "max_records",
  "enable_or_disable", "table_type_def", "logging_clause",
  "table_elements", "as_query_spec", "$@7", "base_table_element_commalist",
  "base_table_element", "column_def", "$@8", "data_type",
  "fixedlen_intnum", "column_def_opt_list", "column_def_opt",
  "is_autoincrement", "table_constraint_def", "$@9",
  "key_column_commalist", "column_commalist", "column_commalist_new",
  "index_opt_unique", "index_element_commalist", "index_column",
  "alter_statement", "$@10", "$@11", "$@12", "rebuild_definition",
  "rebuild_object_definition", "alter_definition",
  "enable_or_disable_or_flush", "column_def_list", "column_list",
  "drop_statement", "rename_statement", "commit_statement",
  "delete_statement", "$@13", "$@14", "insert_statement", "$@15",
  "opt_column_commalist", "opt_column_commalist_new",
  "values_or_query_spec", "insert_atom_commalist", "msynced_opt",
  "upsert_statement", "$@16", "rollback_statement", "select_statement",
  "select_body", "select_exp", "$@17", "fetch_record_type",
  "set_operation_exp", "set_opt_all", "set_query_body", "set_query_exp",
  "show_all_or_table", "opt_all_distinct", "selection",
  "selection_item_commalist", "opt_selection_item_alias",
  "update_statement", "$@18", "update_table_or_rid", "$@19",
  "assignment_commalist", "assignment", "truncate_statement",
  "desc_statement", "$@20", "$@21", "on_or_off", "set_variable_list",
  "set_statement", "$@22", "$@23", "opt_describe_column", "table_exp",
  "$@24", "from_clause", "join_table_commalist", "cross_join", "opt_outer",
  "table_ref", "table_column_alias", "columnalias_commalist",
  "columnalias_item", "opt_scan_hint", "iscan_hint_column_commalist",
  "opt_where_clause", "where_clause", "opt_group_by_clause",
  "group_by_item_commalist", "group_by_item", "opt_having_clause",
  "opt_order_by_clause", "opt_limit_clause", "limit_rid4offset",
  "limit_num4offset", "limit_num4rows", "order_by_item_commalist",
  "order_by_item", "collation_info", "collation_info_str", "order_info",
  "order_info_str", "search_condition", "predicate",
  "comparison_predicate", "between_predicate", "like_predicate", "like_op",
  "opt_escape", "test_for_null", "in_predicate", "atom_commalist",
  "all_or_any_predicate", "any_all_some", "existence_test", "subquery_exp",
  "scalar_exp", "atom", "parameter_marker", "parameter_binding_dummy",
  "aggregation_ref_arg", "aggregation_ref", "function_ref",
  "special_function_arg", "special_function_predicate",
  "special_function_ref", "search_and_result",
  "search_and_result_commalist", "general_function_ref",
  "numeric_function_ref", "string_function_ref", "time_function_ref",
  "rownum_ref", "object_function_ref", "byte_function_ref",
  "copyfrom_type", "random_function_ref", "objectsize_function_ref",
  "objecttype", "literal", "minus_symbol", "table", "view",
  "table_alias_name", "column_ref_or_pvar", "column_ref", "rid_column_ref",
  "column", "index", "sequence", "opt_ansi_style", "admin_statement",
  "export_op", "export_all_or_table", "export_data_or_schema", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,    43,    45,    42,    47,
      37,    94,   271,   272,   273,   274,   275,   276,   277,   278,
     279,   280,   281,   282,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,   314,   315,   316,   317,   318,
     319,   320,   321,   322,   323,   324,   325,   326,   327,   328,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   354,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   373,   374,   375,   376,   377,   378,
     379,   380,   381,   382,   383,   384,   385,   386,   387,   388,
     389,   390,   391,   392,   393,   394,   395,   396,   397,   398,
     399,   400,   401,   402,   403,   404,   405,   406,   407,   408,
     409,   410,   411,   412,   413,   414,   415,   416,   417,   418,
     419,   420,   421,   422,   423,   424,   425,   426,   427,   428,
     429,   430,   431,   432,   433,   434,   435,   436,   437,   438,
     439,   440,   441,   442,   443,   444,   445,   446,   447,   448,
     449,   450,   451,   452,   453,   454,   455,   456,   457,    40,
      41,    44,    46,    63
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   214,   215,   215,   215,   216,   216,   216,   217,   217,
     217,   217,   218,   218,   218,   218,   218,   218,   218,   218,
     219,   219,   219,   221,   220,   222,   220,   223,   224,   220,
     225,   226,   220,   227,   227,   228,   228,   229,   229,   230,
     230,   231,   231,   231,   232,   232,   232,   233,   233,   233,
     234,   234,   234,   235,   235,   236,   236,   236,   236,   237,
     237,   238,   238,   239,   240,   239,   241,   241,   242,   242,
     244,   243,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   245,
     245,   245,   245,   245,   245,   245,   245,   245,   245,   246,
     246,   247,   247,   248,   248,   248,   248,   248,   248,   248,
     248,   248,   249,   249,   250,   251,   250,   250,   250,   250,
     252,   252,   253,   253,   254,   254,   255,   255,   256,   256,
     257,   257,   257,   257,   259,   258,   260,   261,   258,   258,
     258,   262,   262,   263,   263,   263,   264,   264,   264,   264,
     264,   264,   264,   264,   264,   264,   265,   265,   266,   266,
     267,   267,   268,   268,   268,   268,   268,   269,   269,   270,
     270,   270,   272,   273,   271,   275,   274,   276,   276,   277,
     277,   278,   278,   279,   279,   280,   280,   282,   281,   283,
     283,   283,   284,   285,   285,   287,   286,   288,   288,   288,
     288,   288,   289,   289,   290,   290,   291,   291,   291,   292,
     293,   293,   294,   294,   294,   295,   295,   296,   296,   297,
     297,   297,   299,   298,   301,   300,   300,   302,   302,   303,
     304,   304,   305,   305,   306,   305,   307,   305,   305,   305,
     305,   305,   308,   308,   309,   309,   309,   309,   309,   309,
     309,   311,   310,   312,   310,   313,   313,   313,   315,   314,
     316,   316,   317,   317,   317,   317,   317,   317,   317,   318,
     318,   318,   319,   319,   320,   320,   320,   321,   321,   322,
     322,   323,   324,   324,   324,   324,   324,   325,   325,   326,
     326,   327,   328,   328,   329,   329,   330,   330,   331,   331,
     332,   332,   333,   333,   333,   333,   333,   334,   334,   335,
     335,   336,   336,   337,   337,   338,   338,   338,   339,   339,
     340,   341,   341,   342,   342,   343,   343,   343,   343,   343,
     344,   344,   344,   344,   344,   344,   344,   344,   345,   345,
     345,   345,   346,   346,   347,   347,   348,   348,   349,   349,
     350,   350,   351,   351,   351,   351,   352,   352,   353,   354,
     354,   354,   355,   356,   357,   357,   357,   357,   357,   357,
     357,   357,   357,   357,   357,   357,   357,   357,   357,   357,
     358,   358,   359,   360,   360,   361,   361,   361,   362,   362,
     362,   362,   362,   362,   362,   362,   363,   363,   363,   363,
     363,   363,   363,   363,   363,   364,   364,   365,   366,   367,
     368,   368,   369,   369,   369,   369,   370,   370,   370,   370,
     370,   371,   371,   371,   371,   371,   371,   371,   371,   371,
     371,   371,   371,   372,   372,   372,   372,   372,   372,   372,
     372,   372,   372,   372,   372,   372,   373,   374,   374,   375,
     375,   375,   375,   376,   376,   376,   376,   377,   377,   377,
     378,   379,   379,   379,   380,   380,   380,   380,   380,   380,
     380,   380,   381,   381,   382,   382,   383,   384,   385,   385,
     386,   386,   386,   387,   387,   388,   389,   390,   391,   391,
     392,   392,   393,   393,   394,   394,   395,   395,   395
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     8,     0,    10,     0,     0,     7,
       0,     0,     6,     0,     5,     0,     4,     0,     4,     0,
       4,     0,     3,     1,     0,     3,     1,     0,     1,     1,
       0,     5,     3,     1,     1,     0,     1,     1,     1,     1,
       1,     0,     3,     0,     0,     3,     1,     3,     1,     1,
       0,     4,     1,     4,     4,     2,     5,     2,     5,     1,
       1,     1,     1,     4,     6,     1,     2,     1,     4,     1,
       1,     1,     4,     6,     1,     1,     4,     6,     1,     1,
       5,     5,     7,     7,     1,     4,     4,     6,     1,     1,
       2,     0,     2,     1,     2,     1,     3,     2,     4,     2,
       5,     1,     0,     1,     4,     0,     6,     7,    10,     4,
       1,     3,     1,     3,     1,     3,     0,     1,     1,     3,
       2,     2,     3,     3,     0,     5,     0,     0,     6,     3,
       3,     1,     3,     1,     3,     3,     2,     2,     2,     4,
       2,     2,     3,     4,     1,     2,     1,     1,     1,     3,
       1,     3,     3,     3,     3,     3,     3,     5,     5,     1,
       2,     2,     0,     0,     8,     0,     6,     0,     3,     0,
       3,     4,     1,     1,     3,     0,     1,     0,    10,     1,
       2,     2,     1,     1,     1,     0,     6,     0,     1,     1,
       1,     1,     4,     5,     0,     1,     1,     3,     1,     1,
       1,     1,     0,     1,     1,     1,     1,     2,     4,     0,
       2,     2,     0,     3,     0,     7,     4,     1,     3,     3,
       2,     3,     1,     1,     0,     4,     0,     4,     2,     2,
       3,     6,     1,     1,     2,     2,     2,     2,     2,     3,
       3,     0,     3,     0,     5,     0,     1,     1,     0,     6,
       0,     2,     2,     4,     7,     8,     8,     8,     3,     1,
       1,     2,     0,     1,     1,     3,     6,     0,     3,     1,
       3,     1,     0,     1,     2,     2,     4,     1,     3,     0,
       1,     2,     0,     4,     1,     3,     1,     1,     0,     2,
       0,     3,     0,     2,     2,     4,     4,     2,     3,     1,
       2,     1,     2,     1,     3,     3,     2,     3,     0,     1,
       2,     0,     1,     1,     1,     3,     3,     2,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     3,
       4,     1,     6,     5,     5,     4,     1,     1,     0,     2,
       4,     3,     4,     3,     6,     5,     1,     3,     4,     1,
       1,     1,     2,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     2,     1,     1,     1,     1,     1,     1,     3,
       1,     1,     2,     0,     1,     2,     2,     1,     4,     4,
       4,     1,     4,     4,     4,     4,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     6,     4,     3,
       1,     3,     6,     6,     8,     6,     4,     4,     6,     4,
       6,     6,     8,     4,     4,     4,     6,     4,     6,     8,
       4,     4,     4,     1,     1,     1,     1,     3,     8,     8,
       8,     6,     8,     8,     8,     6,     1,     3,     3,     4,
       4,     6,     6,     1,     1,     1,     1,     1,     3,     4,
       6,     1,     1,     1,     2,     2,     2,     1,     1,     1,
       1,     1,     0,     1,     1,     3,     1,     1,     1,     1,
       1,     3,     5,     3,     1,     1,     1,     1,     0,     1,
       6,     3,     1,     1,     1,     2,     0,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     0,   179,    23,   182,   242,   243,     0,   185,     0,
     199,   205,   261,     0,   232,     2,     0,   506,     0,     0,
     197,     0,     0,     0,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,   202,   203,   204,
       0,   216,    18,    19,    20,    21,    22,   144,   146,   181,
     180,    55,   136,     0,     0,     0,   248,     0,   249,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   201,   200,
     222,     0,     0,   484,     0,   240,     0,     0,   507,   508,
       0,     0,     0,     0,   219,   218,   216,     1,     0,     3,
     214,     0,     0,    56,    57,    58,     0,   137,     0,     0,
       0,     0,   265,   265,   496,   173,   172,   486,   174,   497,
     175,   176,     0,     0,     0,   223,   224,   482,     0,     0,
       0,     0,     0,     0,   262,     0,     0,   241,     0,     0,
     233,   292,   284,   153,   150,   149,   151,   504,     0,     0,
     501,   220,   250,   221,     0,   310,   217,     4,   215,     0,
       0,   147,     0,     0,   187,    31,   292,   495,   267,   245,
     266,   247,   189,     0,     0,   495,   477,   478,   479,   480,
     482,   482,   226,     0,     0,   401,     0,   443,   444,   445,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   481,     0,     0,     0,
       0,     0,   446,     0,     0,     0,   467,     0,   456,   494,
       0,     0,     0,     0,     0,     0,     0,   482,   393,   270,
     225,   388,   229,   383,   391,   385,   386,   387,   406,   408,
     407,   409,   411,   412,   413,   410,   414,   390,     0,   384,
     488,   489,   490,   253,   252,   254,     0,   257,   256,   258,
     255,   482,   485,     0,     0,     0,   293,     0,   499,     0,
       0,     0,   505,     0,     0,   189,     0,   312,   218,   212,
       0,     0,     0,    59,    60,     0,   164,   145,    39,    61,
       0,     0,    28,    37,   299,     0,     0,   178,   177,     0,
     381,   382,   482,   482,     0,     0,     0,     0,   482,     0,
       0,     0,   482,     0,   482,   482,   482,   482,   482,   482,
     482,     0,   482,   482,   482,   482,   482,   482,   482,   482,
       0,     0,   482,   482,   482,   482,     0,   482,   482,   482,
       0,     0,   394,   392,     0,   207,   299,   482,   482,   482,
     482,   482,   482,   482,   482,   227,     0,   476,   475,   474,
     260,   259,   264,     0,   498,   294,     0,   295,     0,   487,
     285,   154,   155,   152,     0,     0,     0,     0,     0,   213,
       0,     0,     0,     0,   168,   157,   156,     0,   160,   158,
       0,     0,   161,   170,   167,    53,    54,   166,   165,     0,
       0,   148,    41,     0,    50,     0,   495,     0,   132,    63,
       0,    32,    39,   482,   183,   300,     0,   134,     0,   186,
     192,   495,   457,   458,   493,   491,   482,   482,   397,     0,
       0,     0,     0,    72,     0,     0,   328,   328,    80,    79,
      82,    85,    87,    89,    91,    94,    95,     0,    81,    98,
      99,     0,     0,   104,     0,    90,   108,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   483,   415,     0,
     416,     0,     0,     0,     0,     0,   447,     0,     0,     0,
       0,     0,     0,     0,     0,   468,     0,     0,     0,     0,
       0,   471,   472,   473,     0,     0,     0,   218,   373,   389,
       0,   271,   292,   208,   209,   210,   211,   206,   302,   229,
     380,   374,   375,   376,   377,   378,   379,   291,   230,   231,
     236,   237,     0,     0,   297,     0,   331,   299,   503,   502,
     500,   251,     0,   331,   393,   311,   323,   328,   321,     0,
     393,   314,     0,   313,   482,     0,   125,     0,     0,    70,
       0,   162,     0,     0,   482,   482,    43,    44,     0,    66,
      68,    69,     0,    63,     0,     0,   188,     0,    64,    29,
     482,    41,   482,     0,     0,   482,   301,   339,   340,   341,
     342,   343,   344,   345,   346,     0,   347,   351,   488,   312,
     190,     0,   482,     0,   396,   395,   398,   399,   400,   402,
       0,     0,     0,     0,    75,   329,     0,    77,     0,    86,
       0,     0,     0,     0,     0,     0,     0,   482,   482,   482,
       0,   482,   482,   482,     0,   482,   482,   418,   482,   433,
     435,   482,   403,   404,   427,   482,   437,   482,   426,     0,
     405,   440,   429,   482,   434,   469,   482,   459,   460,     0,
     482,   482,   441,   442,     0,     0,     0,   282,     0,   280,
     282,   282,   279,     0,   272,     0,   310,   228,     0,   482,
     287,   296,     0,   333,   334,   141,   140,   332,   234,   482,
     326,   332,   331,     0,   331,   317,   393,   322,     0,     0,
       0,     0,     0,     0,   130,   169,   111,   159,   171,   163,
       0,     0,   482,    46,    47,    62,     0,     0,    24,     0,
     133,     0,     0,    44,   337,     0,   372,   482,   482,     0,
       0,   482,   482,     0,   482,   482,     0,     0,   356,   357,
     482,     0,   184,   135,     0,   193,   492,     0,     0,   330,
       0,     0,     0,     0,     0,   109,     0,     0,   109,     0,
     109,     0,     0,   109,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   420,     0,     0,     0,     0,
       0,     0,     0,     0,   463,   464,   465,   466,     0,     0,
       0,   278,   281,   283,     0,     0,     0,     0,   292,     0,
     268,   238,   239,     0,   286,   298,   143,   142,   312,     0,
     327,   324,   325,   318,   321,   393,   316,   315,   129,     0,
       0,   124,     0,    71,    40,    42,     0,    48,    49,    36,
      67,    52,     0,   138,    65,    38,    47,     0,   338,   335,
     336,   482,     0,   482,   370,   369,   371,     0,   348,     0,
     482,   363,     0,   349,   358,     0,   361,   191,   482,    73,
      74,   328,   328,    83,     0,    88,    92,     0,    96,   110,
       0,   328,     0,   328,     0,   105,   106,     0,   422,   482,
     482,   451,   482,   482,   482,   455,   482,   482,   423,   482,
     425,   436,   428,   438,   431,     0,   430,   482,   461,   462,
     470,     0,   292,     0,     0,   273,   307,   308,   304,   306,
     312,     0,   289,   235,   195,   322,     0,     0,   131,     0,
       0,   482,   121,   113,     0,     0,   115,   112,    45,     0,
      26,     0,    34,   482,     0,   482,   362,   358,   368,   482,
       0,   366,   350,   482,   355,   360,   194,    76,    78,     0,
       0,     0,   100,     0,   101,     0,     0,     0,     0,     0,
       0,     0,     0,   419,     0,   421,     0,     0,   292,     0,
     292,   292,   482,     0,   303,   269,   288,     0,   196,   198,
       0,   126,   114,   482,   117,   122,   119,    51,   139,     0,
     482,     0,   354,   353,   365,   482,   359,    84,    93,    97,
     328,   328,   107,   448,   450,   449,   452,   454,   453,   424,
     432,   439,     0,   482,     0,     0,   309,   305,   290,   127,
       0,   123,   116,     0,   417,   352,   364,   367,   102,   103,
     482,   274,   482,   482,     0,   118,     0,   277,   275,   276,
       0,   120,   128
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    22,    23,    24,    25,    26,    27,    51,    52,    53,
     399,    54,   283,   401,   391,   402,   392,   547,   694,   809,
     553,   387,    96,   276,   394,   559,   701,   548,   549,   374,
     686,   539,   737,   803,   907,  1002,   551,   682,   683,   397,
     406,    98,   812,   514,    28,    91,    92,   278,   135,   136,
     277,   388,   376,   382,    29,    30,    31,    32,    55,   579,
      33,    65,   282,   286,   409,   724,   959,    34,    83,    35,
      36,   330,    38,    70,   497,    39,   149,    40,    41,   142,
     117,   219,   220,   345,    42,    76,   130,   788,   510,   511,
      43,    44,    57,    59,   245,   124,    45,    71,    72,   159,
     335,   890,   336,   645,   653,   774,   492,   784,   891,   509,
     257,   515,   404,   405,   656,   887,   888,   954,   267,   369,
     531,   532,   533,   525,   526,   594,   595,   666,   671,   709,
     567,   568,   569,   570,   720,   924,   571,   572,   920,   573,
     827,   574,   221,   575,   223,   224,   333,   419,   225,   226,
     459,   576,   227,   755,   756,   228,   229,   230,   231,   232,
     233,   234,   768,   235,   236,   484,   237,   238,   132,   108,
     360,   239,   240,   241,   242,   105,   110,   346,    46,   520,
     139,    80
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -762
static const yytype_int16 yypact[] =
{
    2141,   232,   229,   477,  -762,    22,    48,   407,  -762,   -10,
     238,  -762,    31,    27,  -762,  -762,    -6,    95,     9,   -38,
    -762,   181,   280,   -44,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,   115,   149,
     170,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,   211,    99,   172,   169,   246,  -762,   335,  -762,   335,
     363,   335,   374,   392,   362,   314,   363,   335,  -762,  -762,
      40,   394,   424,   219,   335,  -762,    17,   371,  -762,  -762,
     135,   446,   401,   399,  -762,   282,   297,  -762,   364,  -762,
     496,   335,   392,  -762,  -762,  -762,   381,  -762,   444,   374,
     392,    32,   331,   331,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,    32,   523,   525,  -762,  -762,  1711,   302,   448,
     302,   302,   302,   302,  -762,   566,   562,  -762,   581,   181,
    -762,   212,   113,   382,  -762,   380,  -762,  -762,   335,   506,
    -762,  -762,   518,  -762,    32,   493,  -762,  -762,  -762,   181,
      62,  -762,   335,   363,   403,  -762,   212,  -762,  -762,  -762,
    -762,  -762,   408,   363,   335,   410,  -762,  -762,  -762,  -762,
    2069,  2069,  -762,   432,   447,   457,   459,  -762,  -762,  -762,
     470,   481,   489,   491,   504,   533,   540,   542,   549,   556,
     568,   570,   579,   582,   584,   587,  -762,   590,   615,   616,
     617,   618,  -762,   619,   622,   625,   627,   628,  -762,  -762,
     629,   650,   656,   669,   678,   679,   681,  1890,   644,   546,
     452,  -762,  1014,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,   550,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,   535,  -762,  -762,  -762,
    -762,  2069,  -762,   577,   520,    18,  -762,   607,  -762,   692,
      44,   751,  -762,   798,   898,   408,   869,   812,  -762,  -762,
     453,   453,    57,  -762,  -762,    72,   705,  -762,   127,   706,
     810,   918,  -762,   275,   775,   923,    10,  -762,  -762,   105,
    -762,  -762,  1532,   995,   909,  2318,   925,   934,  2069,   936,
     940,   942,  2069,   943,  2069,    33,  2069,  2069,  2069,  1532,
    1532,   738,  2069,  2069,  2069,  2069,  1532,  2069,  2069,  2069,
     742,   946,  2069,  2069,  2069,  2069,   -77,  2069,  2069,  1890,
     745,    83,  -762,  -762,    38,   379,   775,  2069,  2069,  2069,
    2069,  2069,  2069,  2069,  2069,  -762,   430,  -762,  -762,  -762,
    -762,  -762,   837,   923,   928,  -762,   923,  -762,   923,  -762,
    -762,  -762,  -762,  -762,   278,   278,   813,    15,    37,  -762,
     749,   867,   868,   756,  -762,  -762,   753,  2318,  -762,   753,
    2253,   872,   759,  -762,  -762,  -762,  -762,  -762,  -762,   770,
     937,  -762,   309,   453,   874,   335,   765,   267,  -762,   956,
     809,  -762,   822,  1174,  -762,  -762,   286,  -762,   777,  -762,
    -762,   778,  -762,  -762,  -762,  -762,  2069,  2069,   837,   781,
     782,   783,   793,  -762,   797,   799,   -57,   120,  -762,  -762,
     800,   871,   801,  -762,   805,  -762,  -762,   806,  -762,  -762,
    -762,   807,   814,   815,   817,  -762,  -762,   796,   808,   811,
     129,   826,   827,   832,   367,   833,   428,  -762,  -762,   846,
    -762,   521,   103,    75,   847,   848,  -762,   336,   344,   162,
     555,   850,   877,   400,   889,  -762,   851,   641,   915,   443,
     653,  -762,  -762,  -762,   852,   962,  1051,   282,  -762,  -762,
      29,   713,   212,  -762,  -762,  -762,  -762,  -762,   941,  1014,
     691,   613,   613,  1041,  1041,  1041,  1041,  -762,  -762,  -762,
     855,  -762,  1058,   692,  -762,   313,    61,   -98,  -762,  -762,
    -762,  -762,   866,   101,   644,   863,  -762,   893,   870,    16,
     644,   873,   883,  -762,  1174,   876,  -762,   923,   923,  -762,
     923,  -762,   923,   890,  1061,  1061,  -762,   361,   389,  -762,
    -762,  -762,  1047,   956,   894,  1080,  -762,   918,  -762,  -762,
    1061,   309,  1174,   896,   903,   816,   592,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  1589,  -762,  1610,  1021,   812,
    -762,   923,  2069,   923,   837,   837,  -762,  -762,  -762,  -762,
    1106,  1107,  1113,  1109,  -762,  -762,  1110,  -762,  1114,  -762,
    1116,  1118,   315,   415,   462,  1119,   495,  2069,  2069,  2069,
    1117,  2069,  2069,  2069,  1127,  2069,  2069,  -762,  2069,  -762,
    -762,  2069,  -762,  -762,  -762,  2069,  -762,  2069,  -762,  1126,
    -762,  -762,  -762,  2069,  -762,  -762,  2069,  -762,  -762,   156,
    2069,  2069,  -762,  -762,    29,   165,  1039,  1023,  1044,  -762,
    1023,  1023,  -762,    32,  -762,  1100,   493,  -762,   923,  2069,
     951,  -762,   923,  -762,  -762,   101,  -762,   893,  -762,  2069,
    -762,  -762,   101,    15,   101,  -762,   644,   938,    41,    41,
      64,   923,   952,   404,  -762,  -762,  -762,  -762,  -762,  -762,
    1142,  1143,  1061,  -762,   479,  -762,   453,  1154,  -762,   923,
    -762,   181,  1155,   361,  -762,   181,  -762,    33,   816,    68,
     281,  1174,  1174,   311,  1353,  2069,   955,   116,  -762,  -762,
    2069,   141,  -762,  -762,   440,   837,  -762,   959,   960,  -762,
     961,   972,   454,   975,   465,   976,  1157,   977,   979,   982,
     990,   991,   993,  1005,  1006,  1072,   666,   701,  1011,   719,
     727,   755,  1012,   764,   825,  -762,   478,  1090,  1125,  1138,
    1178,   482,  1223,   865,  -762,  -762,  -762,  -762,  1013,  1232,
    1248,  -762,  -762,  -762,  1122,    32,  1141,  1151,   212,   390,
    -762,  -762,   837,  1163,  -762,  -762,  -762,  -762,   812,   486,
    -762,  -762,  -762,  -762,  -762,   644,  -762,  -762,  -762,   492,
     923,  -762,   923,   485,  -762,  -762,  1228,  -762,  -762,  -762,
    -762,  1045,   513,  -762,  -762,  -762,   479,  1043,  -762,  1243,
    -762,  2069,  1054,  2069,  -762,  -762,  -762,   896,   837,  1193,
      51,  -762,  1149,  -762,  1483,  1153,  -762,  -762,  2069,  -762,
    -762,   893,   893,  -762,  1262,  -762,  -762,  1263,  -762,  -762,
    1264,   893,  1265,   893,  1270,  -762,  -762,  1271,  -762,  2069,
    2069,  -762,  2069,  2069,  2069,  -762,  2069,  2069,  -762,  2069,
    -762,  -762,  -762,  -762,  -762,  1276,  -762,  2069,  -762,  -762,
    -762,    32,   212,    32,    32,  -762,  -762,    56,  -762,  -762,
     812,   515,  -762,  -762,  1086,  -762,  1172,   517,  -762,  1186,
    1085,  2069,  -762,  -762,  1200,   335,  -762,  -762,  -762,   923,
    -762,   923,  -762,    33,  1501,    51,  -762,  1483,  -762,  2069,
     553,  -762,  -762,    66,  -762,  -762,   837,  -762,  -762,  1087,
    1091,  1093,  -762,  1094,  -762,  1095,  1096,  1269,  1304,  1320,
    1402,  1410,  1427,   837,   467,  -762,  1102,  1448,   212,  1203,
     212,   212,  1174,   390,  -762,  -762,  -762,  1163,  -762,  -762,
     335,  -762,  -762,  1174,   837,  1128,  1101,  -762,  -762,  1104,
    2069,   557,  -762,   837,  -762,    66,  -762,  -762,  -762,  -762,
     893,   893,  -762,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  1206,  1174,  1207,  1219,   592,  -762,  -762,  1121,
      73,  -762,  -762,   918,  -762,   837,  -762,  -762,  -762,  -762,
    1174,   592,  1174,  1174,   923,  -762,   576,   592,   592,   592,
     593,  -762,  -762
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -762,  -762,  1274,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,   929,   772,   639,   527,
    -762,  -762,  -762,  -762,  -762,   791,  -762,  -762,   649,  -385,
    -762,  1055,  -151,  -762,  -762,  -762,   545,  -762,  -669,   346,
    -762,  -762,  -762,  -646,  -762,  -762,  -762,  -762,  -762,  1103,
    -762,  -762,  1097,  -762,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  1108,  -762,   682,  -762,  -762,  -762,  -762,
    -762,     5,     8,  -762,  -762,   -17,  -762,  1212,   -14,  -762,
    -762,  -762,  -762,   875,  -762,  -762,  -762,  -762,  1007,   708,
    -762,  -762,  -762,  -762,   197,  -762,  -762,  -762,  -762,  1268,
    -762,  -762,  -762,  1033,  -762,   155,   -70,  -762,  -762,  -761,
    -153,  -762,  -274,  -762,  -762,  -762,   419,  -762,   720,  -564,
    -762,  -762,   139,  -762,   702,  -413,  -404,  -416,  -490,  -370,
    -762,  -762,  -762,  -762,   664,   464,  -762,  -762,   469,  -762,
    -762,  -762,  -554,  -107,  -294,  -762,  -511,   206,  -762,  -762,
    -664,  -762,  -762,   510,  -762,  -762,  -762,  -762,  -762,  -762,
    -762,  -762,  -762,  -762,  -762,  -762,  -762,  -478,   -13,  1283,
     878,  -375,  -163,  -762,  -101,   196,   300,  -115,  -762,  1024,
    -762,  -762
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -499
static const yytype_int16 yytable[] =
{
      75,   160,   160,   284,    85,    37,   131,    86,   550,   706,
     222,   458,   799,   672,   597,   722,   785,   259,   396,   677,
      73,   104,   892,   523,   675,  -244,   667,    37,   577,    84,
      73,   156,    73,   566,  -263,    73,   396,   166,   167,   168,
     169,    73,   162,   817,   102,   528,   103,   361,   106,   794,
     457,  -246,   403,   813,   114,   166,   167,   168,   169,   481,
     157,   127,   498,   290,   291,   115,   690,   691,   457,   143,
     166,   167,   168,   169,   265,   711,   712,    66,   150,   711,
     712,    77,   702,   457,   711,   712,   270,    81,   271,   338,
     663,   339,   340,   341,   342,   343,   344,   338,    82,   339,
     340,   341,   342,   343,   344,   482,   483,   670,   411,   116,
     331,    89,   665,   658,   674,   384,  -498,   338,   398,   339,
     340,   341,   342,   343,   344,   262,    67,   592,   664,   832,
     663,   897,   268,   272,   254,   355,    11,   952,  -219,   279,
     196,   258,   460,   338,   352,   339,   340,   341,   342,   343,
     344,   288,   593,   685,   835,    11,   408,    84,   196,   577,
     137,   362,   831,    74,   680,   793,   273,   274,   664,   377,
     380,   383,  -218,   196,   381,   786,   338,    11,   339,   340,
     341,   342,   343,   344,   407,   418,   418,   577,   415,   128,
     577,   450,   704,    90,   764,   454,   998,   456,   529,   461,
     462,   463,   418,   418,   527,   467,   468,   469,   470,   418,
     472,   473,   474,   646,   806,   477,   478,   479,   480,    21,
     485,   486,   331,   833,   893,    56,   129,   356,   524,   676,
     499,   500,   501,   502,   503,   504,   505,   506,   644,   513,
     578,   129,    97,   668,   647,   592,   218,   490,   836,   969,
     530,    58,   512,   648,   795,   516,   790,   512,   792,   649,
      21,   650,   113,   787,   218,   968,   275,   953,   916,   385,
     386,   138,    49,   918,   798,   412,   413,   414,   818,   218,
      87,    68,   -35,  1015,   895,   620,   621,   651,    78,    79,
     390,   410,   377,   489,   713,   338,   714,   339,   340,   341,
     342,   343,   344,   765,   592,   255,     1,    11,   518,   584,
     585,   550,   487,   619,   715,    86,    93,   247,   248,   249,
     250,    99,     2,   735,   101,   519,   955,     3,   100,   596,
     766,   767,   736,   577,   157,   158,   577,   577,    73,   654,
     610,   819,   820,   256,   821,  1020,     4,     5,     6,   280,
     338,     7,   339,   340,   341,   342,   343,   344,   338,   287,
     339,   340,   341,   342,   343,   344,   104,   716,    47,     8,
     111,   578,   628,   717,   133,   771,   652,   107,   718,   719,
      50,   338,   554,   339,   340,   341,   342,   343,   344,    69,
      21,    48,   151,   396,   700,   109,   134,   822,   886,   578,
     155,     9,   578,    10,    73,   112,    11,    12,   718,   719,
      94,   243,   244,   458,   338,    95,   339,   340,   341,   342,
     343,   344,    13,   738,    14,   118,   141,   125,   927,   928,
     -33,   126,   736,   507,   508,   400,   684,   377,   932,   687,
     934,   688,   338,   351,   339,   340,   341,   342,   343,   344,
     140,   357,   739,   741,   415,   744,   157,   338,   710,   339,
     340,   341,   342,   343,   344,    16,    17,    18,   119,   120,
     740,    19,    20,   545,   546,   725,   121,   556,   557,   736,
     723,   338,   726,   339,   340,   341,   342,   343,   344,    21,
     144,   489,   145,   370,    60,   254,   580,   581,   899,   421,
     745,   746,   747,   743,   749,   750,   751,   146,   753,   754,
     527,   757,   736,   122,   758,   464,   465,   152,   759,   147,
     760,   148,   471,   661,   662,   900,   762,   692,   693,   763,
     371,   153,   123,   769,   770,   338,   921,   339,   340,   341,
     342,   343,   344,    61,   460,   578,   624,   625,   578,   578,
     901,   163,   782,   164,   626,   627,    62,   512,   347,   348,
     349,   516,   725,   246,   -25,   252,    63,  1008,  1009,   338,
     372,   339,   340,   341,   342,   343,   344,   577,   614,    64,
     684,   251,   996,   778,   493,   494,   495,   496,   577,   253,
     902,   261,   903,  1000,   260,   377,   373,   263,   516,   695,
     696,   710,   904,   711,   712,   905,   266,   828,   829,   264,
     632,   633,   281,   834,   801,   802,   889,   285,   577,   458,
     -25,   921,   289,  1011,   334,   885,   -27,   487,   906,   976,
      86,   341,   342,   343,   344,   577,   -30,   577,   577,   616,
    1017,   292,  1018,  1019,   243,   244,   350,   807,   808,   254,
     837,   838,   332,   638,   639,   338,   293,   339,   340,   341,
     342,   343,   344,   337,   843,   844,   294,   338,   295,   339,
     340,   341,   342,   343,   344,   846,   847,   989,   867,   296,
     338,  1007,   339,   340,   341,   342,   343,   344,   868,   869,
     297,   487,   874,   875,    86,   359,   894,   838,   298,   684,
     299,   898,   896,   802,   353,   882,   814,   339,   340,   341,
     342,   343,   344,   300,   914,   338,   917,   339,   340,   341,
     342,   343,   344,   910,   911,   956,   957,   961,   802,   949,
     354,   926,   618,   338,   358,   339,   340,   341,   342,   343,
     344,   338,   301,   339,   340,   341,   342,   343,   344,   302,
     460,   303,   937,   938,   133,   939,   940,   941,   304,   942,
     943,   646,   944,   974,   975,   305,   629,  1006,   975,   338,
     947,   339,   340,   341,   342,   343,   344,   306,   338,   307,
     339,   340,   341,   342,   343,   344,  1021,   557,   308,   578,
     889,   309,   647,   310,   964,   992,   311,   994,   995,   312,
     578,   648,   364,  1022,   802,   776,   777,   649,   967,   650,
     516,   948,   973,   950,   951,   375,   378,   796,   797,   165,
     166,   167,   168,   169,   313,   314,   315,   316,   317,   562,
     578,   318,   170,   171,   319,   651,   320,   321,   322,   338,
     398,   339,   340,   341,   342,   343,   344,   578,   173,   578,
     578,   338,   636,   339,   340,   341,   342,   343,   344,   323,
     174,   175,   176,  1005,   640,   324,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   859,   325,   338,
     188,   339,   340,   341,   342,   343,   344,   326,   327,   563,
     328,   338,   966,   339,   340,   341,   342,   343,   344,   564,
     189,   190,   365,   338,   367,   339,   340,   341,   342,   343,
     344,   368,   860,   684,   389,   393,   191,   192,   193,   194,
     395,   396,   195,   196,   652,   403,   157,   422,   448,   338,
     862,   339,   340,   341,   342,   343,   344,   449,   863,   451,
     197,   198,    11,   452,   199,   453,   455,   999,   466,   200,
     201,   202,   475,   203,   476,   488,   258,   204,   534,   522,
     543,   205,   535,   536,   538,   537,   864,   541,   206,   207,
     542,   560,   544,   552,   208,   866,   338,   555,   339,   340,
     341,   342,   343,   344,   558,   390,   582,   599,   209,   210,
     583,   586,   587,   588,   211,   212,   213,   214,   165,   166,
     167,   168,   169,   589,   215,   216,   590,   607,   591,   598,
     600,   170,   171,   420,   601,   602,   603,  -498,  -498,   608,
     416,   655,   609,   604,   605,   708,   606,   173,   338,   218,
     339,   340,   341,   342,   343,   344,   867,   611,   612,   174,
     175,   176,   258,   613,   615,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   187,   617,   622,   623,   188,
     630,   635,   344,   641,   417,   338,   658,   339,   340,   341,
     342,   343,   344,   659,   673,   669,   877,   592,   457,   189,
     190,  -319,   697,   411,   678,   681,   338,   631,   339,   340,
     341,   342,   343,   344,   679,   191,   192,   193,   194,   634,
     689,   195,   196,   699,   338,   705,   339,   340,   341,   342,
     343,   344,   707,   721,   727,   728,   729,   730,   731,   197,
     198,   748,   732,   199,   733,   637,   734,   742,   200,   201,
     202,   752,   203,   772,   761,   779,   204,   773,   775,   338,
     205,   339,   340,   341,   342,   343,   344,   206,   207,  -320,
     804,   805,   338,   208,   339,   340,   341,   342,   343,   344,
     783,   800,   811,   815,   830,   849,   507,   209,   210,   839,
     840,   841,   642,   211,   212,   213,   214,   165,   166,   167,
     168,   169,   842,   215,   216,   845,   848,   562,   850,   851,
     170,   171,   338,   852,   339,   340,   341,   342,   343,   344,
     853,   909,   854,   855,   217,   919,   173,   338,   218,   339,
     340,   341,   342,   343,   344,   856,   881,   857,   174,   175,
     176,   861,   865,   878,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   187,   883,   908,   338,   188,   339,
     340,   341,   342,   343,   344,   884,   338,   563,   339,   340,
     341,   342,   343,   344,   913,   712,   922,   564,   189,   190,
     925,   643,   338,   915,   339,   340,   341,   342,   343,   344,
     929,   930,   931,   933,   191,   192,   193,   194,   935,   936,
     195,   196,   858,   338,   946,   339,   340,   341,   342,   343,
     344,   958,   960,   962,   963,   965,    88,   977,   197,   198,
     870,   978,   199,   979,   980,   981,   982,   200,   201,   202,
    1003,   203,   990,   993,  1004,   204,  1010,  1012,   338,   205,
     339,   340,   341,   342,   343,   344,   206,   207,  1001,  1013,
    1014,   561,   208,   703,   338,   871,   339,   340,   341,   342,
     343,   344,   816,   912,   698,   810,   209,   210,   872,  1016,
     447,   789,   211,   212,   213,   214,   165,   166,   167,   168,
     169,   269,   215,   216,   363,   517,   781,   491,   379,   170,
     171,   161,   997,   366,   657,   791,   780,   823,   824,   945,
     825,   972,   154,   565,   971,   173,     0,   218,   873,   521,
       0,   660,     0,     0,     0,     0,     0,   174,   175,   176,
       0,     0,     0,   177,   178,   179,   180,   181,   182,   183,
     184,   185,   186,   187,     0,     0,   338,   188,   339,   340,
     341,   342,   343,   344,   338,     0,   339,   340,   341,   342,
     343,   344,     0,   876,     0,     0,     0,   189,   190,     0,
       0,   338,   879,   339,   340,   341,   342,   343,   344,     0,
       0,     0,     0,   191,   192,   193,   194,     0,   880,   195,
     196,     0,   338,     0,   339,   340,   341,   342,   343,   344,
       0,     0,     0,     0,     0,     0,     0,   197,   198,   983,
       0,   199,     0,   826,     0,     0,   200,   201,   202,     0,
     203,     0,     0,     0,   204,     0,     0,   338,   205,   339,
     340,   341,   342,   343,   344,   206,   207,     0,     0,     0,
       0,   208,     0,   970,   984,   338,     0,   339,   340,   341,
     342,   343,   344,     0,     0,   209,   210,     0,     0,     0,
     985,   211,   212,   213,   214,   165,   166,   167,   168,   169,
       0,   215,   216,     0,     0,     0,     0,     0,   170,   171,
       0,     0,     0,     0,     0,   923,     0,   416,     0,     0,
       0,     0,   217,     0,   173,     0,   218,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   174,   175,   176,     0,
       0,     0,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,     0,     0,     0,   188,     0,     0,     0,
       0,   417,   713,   338,   714,   339,   340,   341,   342,   343,
     344,     0,   986,     0,     0,     0,   189,   190,     0,     0,
     987,     0,   715,  -384,  -384,  -384,  -384,  -384,  -384,  -384,
    -384,  -384,   191,   192,   193,   194,     0,   988,   195,   196,
       0,     0,     0,  -384,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   197,   198,   991,     0,
     199,     0,     0,     0,     0,   200,   201,   202,     0,   203,
       0,     0,     0,   204,     0,   716,     0,   205,     0,     0,
       0,   717,     0,     0,   206,   207,   718,   719,     0,     0,
     208,     0,     0,     0,     0,     0,  -384,     0,     0,     0,
       0,     0,  -384,     0,   209,   210,     0,  -384,  -384,     0,
     211,   212,   213,   214,   165,   166,   167,   168,   169,     0,
     215,   216,     0,     0,     0,     0,     0,   170,   171,   172,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   217,     0,   173,     0,   218,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   174,   175,   176,     0,     0,
       0,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,     0,     0,     0,   188,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   189,   190,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   191,   192,   193,   194,     0,     0,   195,   196,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   197,   198,     0,     0,   199,
       0,     0,     0,     0,   200,   201,   202,     0,   203,     0,
       0,     0,   204,     0,     0,     0,   205,     0,     0,     0,
       0,     0,     0,   206,   207,     0,     0,     0,     0,   208,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   209,   210,     0,     0,     0,     0,   211,
     212,   213,   214,   165,   166,   167,   168,   169,     0,   215,
     216,     0,     0,     0,     0,     0,   170,   171,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     217,     0,   173,     0,   218,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   174,   175,   176,     0,     0,     0,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
     187,     0,     0,     0,   188,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   189,   190,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     191,   192,   193,   194,     0,     0,   195,   196,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   197,   198,    11,     0,   199,     0,
       0,     0,     0,   200,   201,   202,     0,   203,     0,     0,
       0,   204,     0,     0,     0,   205,     0,     0,     0,     0,
       0,     0,   206,   207,     0,     0,     0,     0,   208,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   209,   210,     0,     0,     0,     0,   211,   212,
     213,   214,   165,   166,   167,   168,   169,     0,   215,   216,
       0,     0,     0,     0,     0,   170,   171,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   329,
       0,   173,     0,   218,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   174,   175,   176,     0,     0,     0,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
       0,     0,     0,   188,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   189,   190,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     1,     0,   191,
     192,   193,   194,     0,     0,   195,   196,     0,     0,     0,
       0,     0,     0,     2,     0,     0,     0,     0,     3,     0,
       0,     0,     0,   197,   198,     0,     0,   199,     0,     0,
       0,     0,   200,   201,   202,     0,   203,     4,     5,     6,
     204,     0,     7,     0,   205,     0,     0,     0,     0,     0,
       0,   206,   207,     0,     0,     0,     0,   208,     0,     0,
       8,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   209,   210,     0,     0,     0,     0,   211,   212,   213,
     214,     0,     0,     0,     0,     0,     0,   215,   216,     0,
       0,     0,     9,     0,    10,     0,     0,    11,    12,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   217,     0,
       0,     0,   218,    13,     0,    14,     0,   423,     0,   424,
     425,   426,   427,     0,     0,     0,    15,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   428,   429,   430,     0,     0,     0,
       0,     0,     0,   431,     0,     0,    16,    17,    18,   432,
       0,     0,    19,    20,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   433,     0,     0,     0,     0,     0,     0,
      21,     0,   423,     0,   424,   425,   426,   427,     0,     0,
       0,   434,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   435,     0,     0,   540,     0,     0,     0,     0,   428,
     429,   430,   436,     0,     0,   437,     0,     0,   431,     0,
       0,   438,   439,   440,   432,     0,     0,     0,     0,     0,
     441,   442,     0,     0,     0,     0,     0,     0,   433,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   434,   443,   444,     0,
       0,     0,     0,     0,     0,     0,   435,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   436,   445,     0,
     437,     0,     0,     0,   446,     0,   438,   439,   440,     0,
       0,     0,     0,     0,     0,   441,   442,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   443,   444,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   445,     0,     0,     0,     0,     0,   446
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-762))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
      13,   102,   103,   156,    21,     0,    76,    21,   393,   563,
     117,   305,   681,   524,   427,   579,   662,   132,     3,   530,
       3,     3,   783,     8,     8,     3,   516,    22,   403,    21,
       3,   101,     3,   403,     3,     3,     3,     4,     5,     6,
       7,     3,   112,   707,    57,     8,    59,     3,    61,     8,
      17,     3,   150,   699,    67,     4,     5,     6,     7,   136,
       3,    74,   336,   170,   171,    25,   544,   545,    17,    82,
       4,     5,     6,     7,   144,    11,    12,    87,    91,    11,
      12,    87,   560,    17,    11,    12,    24,    78,    26,    14,
      29,    16,    17,    18,    19,    20,    21,    14,   136,    16,
      17,    18,    19,    20,    21,   182,   183,   523,     3,    69,
     217,   155,   516,   211,   527,    43,     3,    14,   281,    16,
      17,    18,    19,    20,    21,   138,   136,   184,    67,    13,
      29,   800,   149,    71,   129,   117,   126,    81,    23,   152,
     107,    28,   305,    14,   251,    16,    17,    18,    19,    20,
      21,   164,   209,   538,    13,   126,   146,   149,   107,   534,
      25,   117,   716,   136,   534,   676,   104,   105,    67,   270,
     271,   272,    23,   107,   117,   665,    14,   126,    16,    17,
      18,    19,    20,    21,   285,   292,   293,   562,   289,   172,
     565,   298,   562,    23,    38,   302,   957,   304,   161,   306,
     307,   308,   309,   310,   367,   312,   313,   314,   315,   316,
     317,   318,   319,    48,   692,   322,   323,   324,   325,   209,
     327,   328,   329,   107,   788,   203,   209,   209,   213,   213,
     337,   338,   339,   340,   341,   342,   343,   344,   209,   354,
     403,   209,   143,   517,    79,   184,   213,   209,   107,   913,
     213,   203,   353,    88,   213,   356,   672,   358,   674,    94,
     209,    96,    66,   667,   213,   911,   204,   211,   822,   197,
     198,   136,    43,   827,   210,   170,   171,   172,   210,   213,
       0,    43,   155,   210,   795,   210,   211,   122,   193,   194,
     163,   286,   393,   210,    13,    14,    15,    16,    17,    18,
      19,    20,    21,   147,   184,    93,    26,   126,    30,   416,
     417,   696,   329,   210,    33,   329,   105,   120,   121,   122,
     123,   149,    42,     8,    78,    47,   890,    47,   159,   209,
     174,   175,    17,   708,     3,     4,   711,   712,     3,   492,
     211,   711,   712,   131,    33,  1014,    66,    67,    68,   153,
      14,    71,    16,    17,    18,    19,    20,    21,    14,   163,
      16,    17,    18,    19,    20,    21,     3,    86,   136,    89,
       8,   534,   210,    92,     3,   210,   211,     3,    97,    98,
     151,    14,   395,    16,    17,    18,    19,    20,    21,   151,
     209,   159,    92,     3,   557,     3,    25,    86,     8,   562,
     100,   121,   565,   123,     3,    91,   126,   127,    97,    98,
     199,   109,   110,   707,    14,   204,    16,    17,    18,    19,
      20,    21,   142,     8,   144,    31,    25,     3,   841,   842,
     155,   212,    17,     3,     4,   160,   537,   538,   851,   540,
     853,   542,    14,   246,    16,    17,    18,    19,    20,    21,
       4,   255,   603,   604,   555,   606,     3,    14,   565,    16,
      17,    18,    19,    20,    21,   185,   186,   187,    74,    75,
       8,   191,   192,   164,   165,   582,    82,   210,   211,    17,
     581,    14,   583,    16,    17,    18,    19,    20,    21,   209,
      91,   210,   210,    40,    87,   490,   210,   211,    13,   293,
     607,   608,   609,     8,   611,   612,   613,   210,   615,   616,
     673,   618,    17,   119,   621,   309,   310,   136,   625,   155,
     627,    25,   316,   210,   211,    40,   633,   166,   167,   636,
      77,    87,   138,   640,   641,    14,   830,    16,    17,    18,
      19,    20,    21,   136,   707,   708,   210,   211,   711,   712,
      65,    28,   659,    28,   210,   211,   149,   658,     8,     9,
      10,   662,   669,   115,    87,     3,   159,   980,   981,    14,
     117,    16,    17,    18,    19,    20,    21,   952,   211,   172,
     681,    15,   952,   653,   205,   206,   207,   208,   963,     8,
     105,   211,   107,   963,   212,   696,   143,    91,   699,   210,
     211,   708,   117,    11,    12,   120,   113,   714,   715,    91,
     210,   211,   209,   720,   210,   211,   779,   209,   993,   913,
     143,   915,   212,   993,    78,   778,   149,   644,   143,   923,
     644,    18,    19,    20,    21,  1010,   159,  1012,  1013,   211,
    1010,   209,  1012,  1013,   109,   110,   111,   168,   169,   644,
     210,   211,     8,   210,   211,    14,   209,    16,    17,    18,
      19,    20,    21,   211,   210,   211,   209,    14,   209,    16,
      17,    18,    19,    20,    21,   210,   211,   210,   211,   209,
      14,   975,    16,    17,    18,    19,    20,    21,   210,   211,
     209,   708,   210,   211,   708,     3,   210,   211,   209,   800,
     209,   802,   210,   211,   127,   775,   701,    16,    17,    18,
      19,    20,    21,   209,   821,    14,   823,    16,    17,    18,
      19,    20,    21,   210,   211,   210,   211,   210,   211,   882,
     210,   838,   211,    14,   127,    16,    17,    18,    19,    20,
      21,    14,   209,    16,    17,    18,    19,    20,    21,   209,
     913,   209,   859,   860,     3,   862,   863,   864,   209,   866,
     867,    48,   869,   210,   211,   209,   211,   210,   211,    14,
     877,    16,    17,    18,    19,    20,    21,   209,    14,   209,
      16,    17,    18,    19,    20,    21,   210,   211,   209,   952,
     953,   209,    79,   209,   901,   948,   209,   950,   951,   209,
     963,    88,     4,   210,   211,   650,   651,    94,   909,    96,
     911,   881,   919,   883,   884,   270,   271,   678,   679,     3,
       4,     5,     6,     7,   209,   209,   209,   209,   209,    13,
     993,   209,    16,    17,   209,   122,   209,   209,   209,    14,
    1003,    16,    17,    18,    19,    20,    21,  1010,    32,  1012,
    1013,    14,   211,    16,    17,    18,    19,    20,    21,   209,
      44,    45,    46,   970,   211,   209,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,   211,   209,    14,
      64,    16,    17,    18,    19,    20,    21,   209,   209,    73,
     209,    14,   905,    16,    17,    18,    19,    20,    21,    83,
      84,    85,     4,    14,    35,    16,    17,    18,    19,    20,
      21,    99,   211,  1014,   209,   209,   100,   101,   102,   103,
     110,     3,   106,   107,   211,   150,     3,    18,     3,    14,
     211,    16,    17,    18,    19,    20,    21,     3,   211,     3,
     124,   125,   126,     3,   128,     3,     3,   960,   210,   133,
     134,   135,   210,   137,     8,   210,    28,   141,   209,   146,
     190,   145,    95,    95,   211,   209,   211,    95,   152,   153,
     211,   162,    35,    99,   158,   211,    14,   212,    16,    17,
      18,    19,    20,    21,    28,   163,   209,   116,   172,   173,
     212,   210,   210,   210,   178,   179,   180,   181,     3,     4,
       5,     6,     7,   210,   188,   189,   209,   211,   209,   209,
     209,    16,    17,    18,   209,   209,   209,     3,     4,   211,
      25,    80,   211,   209,   209,   209,   209,    32,    14,   213,
      16,    17,    18,    19,    20,    21,   211,   211,   211,    44,
      45,    46,    28,   211,   211,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,   210,   210,   210,    64,
     210,   210,    21,   211,    69,    14,   211,    16,    17,    18,
      19,    20,    21,    15,   211,   209,   211,   184,    17,    84,
      85,   211,    35,     3,   211,   209,    14,   210,    16,    17,
      18,    19,    20,    21,   211,   100,   101,   102,   103,   210,
     210,   106,   107,   209,    14,   209,    16,    17,    18,    19,
      20,    21,   209,    92,     8,     8,     3,     8,     8,   124,
     125,     4,     8,   128,     8,   210,     8,     8,   133,   134,
     135,     4,   137,    94,     8,    35,   141,   114,    94,    14,
     145,    16,    17,    18,    19,    20,    21,   152,   153,   211,
       8,     8,    14,   158,    16,    17,    18,    19,    20,    21,
     209,   209,     8,     8,   209,     8,     3,   172,   173,   210,
     210,   210,   210,   178,   179,   180,   181,     3,     4,     5,
       6,     7,   210,   188,   189,   210,   210,    13,   211,   210,
      16,    17,    14,   211,    16,    17,    18,    19,    20,    21,
     210,   156,   211,   210,   209,    12,    32,    14,   213,    16,
      17,    18,    19,    20,    21,   210,    94,   211,    44,    45,
      46,   210,   210,   210,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    94,     8,    14,    64,    16,
      17,    18,    19,    20,    21,    94,    14,    73,    16,    17,
      18,    19,    20,    21,   211,    12,   107,    83,    84,    85,
     107,   210,    14,   209,    16,    17,    18,    19,    20,    21,
       8,     8,     8,     8,   100,   101,   102,   103,     8,     8,
     106,   107,   210,    14,     8,    16,    17,    18,    19,    20,
      21,   205,   120,   107,   209,    95,    22,   210,   124,   125,
     210,   210,   128,   210,   210,   210,   210,   133,   134,   135,
     209,   137,   210,   110,   210,   141,   110,   110,    14,   145,
      16,    17,    18,    19,    20,    21,   152,   153,   200,   110,
     209,   402,   158,   561,    14,   210,    16,    17,    18,    19,
      20,    21,   703,   816,   553,   696,   172,   173,   210,  1003,
     295,   669,   178,   179,   180,   181,     3,     4,     5,     6,
       7,   149,   188,   189,   261,   358,   658,   334,   271,    16,
      17,   103,   953,   265,   499,   673,   656,   713,    25,   869,
      27,   917,    99,   209,   915,    32,    -1,   213,   210,   365,
      -1,   513,    -1,    -1,    -1,    -1,    -1,    44,    45,    46,
      -1,    -1,    -1,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    -1,    -1,    14,    64,    16,    17,
      18,    19,    20,    21,    14,    -1,    16,    17,    18,    19,
      20,    21,    -1,   210,    -1,    -1,    -1,    84,    85,    -1,
      -1,    14,   210,    16,    17,    18,    19,    20,    21,    -1,
      -1,    -1,    -1,   100,   101,   102,   103,    -1,   210,   106,
     107,    -1,    14,    -1,    16,    17,    18,    19,    20,    21,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   124,   125,   210,
      -1,   128,    -1,   130,    -1,    -1,   133,   134,   135,    -1,
     137,    -1,    -1,    -1,   141,    -1,    -1,    14,   145,    16,
      17,    18,    19,    20,    21,   152,   153,    -1,    -1,    -1,
      -1,   158,    -1,    12,   210,    14,    -1,    16,    17,    18,
      19,    20,    21,    -1,    -1,   172,   173,    -1,    -1,    -1,
     210,   178,   179,   180,   181,     3,     4,     5,     6,     7,
      -1,   188,   189,    -1,    -1,    -1,    -1,    -1,    16,    17,
      -1,    -1,    -1,    -1,    -1,    72,    -1,    25,    -1,    -1,
      -1,    -1,   209,    -1,    32,    -1,   213,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    44,    45,    46,    -1,
      -1,    -1,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    -1,    -1,    -1,    64,    -1,    -1,    -1,
      -1,    69,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    -1,   210,    -1,    -1,    -1,    84,    85,    -1,    -1,
     210,    -1,    33,    13,    14,    15,    16,    17,    18,    19,
      20,    21,   100,   101,   102,   103,    -1,   210,   106,   107,
      -1,    -1,    -1,    33,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   124,   125,   210,    -1,
     128,    -1,    -1,    -1,    -1,   133,   134,   135,    -1,   137,
      -1,    -1,    -1,   141,    -1,    86,    -1,   145,    -1,    -1,
      -1,    92,    -1,    -1,   152,   153,    97,    98,    -1,    -1,
     158,    -1,    -1,    -1,    -1,    -1,    86,    -1,    -1,    -1,
      -1,    -1,    92,    -1,   172,   173,    -1,    97,    98,    -1,
     178,   179,   180,   181,     3,     4,     5,     6,     7,    -1,
     188,   189,    -1,    -1,    -1,    -1,    -1,    16,    17,    18,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   209,    -1,    32,    -1,   213,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    44,    45,    46,    -1,    -1,
      -1,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    84,    85,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   100,   101,   102,   103,    -1,    -1,   106,   107,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   124,   125,    -1,    -1,   128,
      -1,    -1,    -1,    -1,   133,   134,   135,    -1,   137,    -1,
      -1,    -1,   141,    -1,    -1,    -1,   145,    -1,    -1,    -1,
      -1,    -1,    -1,   152,   153,    -1,    -1,    -1,    -1,   158,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   172,   173,    -1,    -1,    -1,    -1,   178,
     179,   180,   181,     3,     4,     5,     6,     7,    -1,   188,
     189,    -1,    -1,    -1,    -1,    -1,    16,    17,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     209,    -1,    32,    -1,   213,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    44,    45,    46,    -1,    -1,    -1,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    84,    85,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     100,   101,   102,   103,    -1,    -1,   106,   107,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   124,   125,   126,    -1,   128,    -1,
      -1,    -1,    -1,   133,   134,   135,    -1,   137,    -1,    -1,
      -1,   141,    -1,    -1,    -1,   145,    -1,    -1,    -1,    -1,
      -1,    -1,   152,   153,    -1,    -1,    -1,    -1,   158,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   172,   173,    -1,    -1,    -1,    -1,   178,   179,
     180,   181,     3,     4,     5,     6,     7,    -1,   188,   189,
      -1,    -1,    -1,    -1,    -1,    16,    17,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   209,
      -1,    32,    -1,   213,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    44,    45,    46,    -1,    -1,    -1,    50,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      -1,    -1,    -1,    64,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    84,    85,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    26,    -1,   100,
     101,   102,   103,    -1,    -1,   106,   107,    -1,    -1,    -1,
      -1,    -1,    -1,    42,    -1,    -1,    -1,    -1,    47,    -1,
      -1,    -1,    -1,   124,   125,    -1,    -1,   128,    -1,    -1,
      -1,    -1,   133,   134,   135,    -1,   137,    66,    67,    68,
     141,    -1,    71,    -1,   145,    -1,    -1,    -1,    -1,    -1,
      -1,   152,   153,    -1,    -1,    -1,    -1,   158,    -1,    -1,
      89,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   172,   173,    -1,    -1,    -1,    -1,   178,   179,   180,
     181,    -1,    -1,    -1,    -1,    -1,    -1,   188,   189,    -1,
      -1,    -1,   121,    -1,   123,    -1,    -1,   126,   127,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   209,    -1,
      -1,    -1,   213,   142,    -1,   144,    -1,    34,    -1,    36,
      37,    38,    39,    -1,    -1,    -1,   155,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    61,    62,    63,    -1,    -1,    -1,
      -1,    -1,    -1,    70,    -1,    -1,   185,   186,   187,    76,
      -1,    -1,   191,   192,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    90,    -1,    -1,    -1,    -1,    -1,    -1,
     209,    -1,    34,    -1,    36,    37,    38,    39,    -1,    -1,
      -1,   108,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   118,    -1,    -1,   121,    -1,    -1,    -1,    -1,    61,
      62,    63,   129,    -1,    -1,   132,    -1,    -1,    70,    -1,
      -1,   138,   139,   140,    76,    -1,    -1,    -1,    -1,    -1,
     147,   148,    -1,    -1,    -1,    -1,    -1,    -1,    90,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   108,   174,   175,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   118,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   129,   195,    -1,
     132,    -1,    -1,    -1,   201,    -1,   138,   139,   140,    -1,
      -1,    -1,    -1,    -1,    -1,   147,   148,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   174,   175,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   195,    -1,    -1,    -1,    -1,    -1,   201
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,    26,    42,    47,    66,    67,    68,    71,    89,   121,
     123,   126,   127,   142,   144,   155,   185,   186,   187,   191,
     192,   209,   215,   216,   217,   218,   219,   220,   258,   268,
     269,   270,   271,   274,   281,   283,   284,   285,   286,   289,
     291,   292,   298,   304,   305,   310,   392,   136,   159,    43,
     151,   221,   222,   223,   225,   272,   203,   306,   203,   307,
      87,   136,   149,   159,   172,   275,    87,   136,    43,   151,
     287,   311,   312,     3,   136,   382,   299,    87,   193,   194,
     395,    78,   136,   282,   286,   289,   292,     0,   216,   155,
      23,   259,   260,   105,   199,   204,   236,   143,   255,   149,
     159,    78,   382,   382,     3,   389,   382,     3,   383,     3,
     390,     8,    91,   389,   382,    25,    69,   294,    31,    74,
      75,    82,   119,   138,   309,     3,   212,   382,   172,   209,
     300,   320,   382,     3,    25,   262,   263,    25,   136,   394,
       4,    25,   293,   382,    91,   210,   210,   155,    25,   290,
     382,   390,   136,    87,   383,   390,   320,     3,     4,   313,
     388,   313,   320,    28,    28,     3,     4,     5,     6,     7,
      16,    17,    18,    32,    44,    45,    46,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    64,    84,
      85,   100,   101,   102,   103,   106,   107,   124,   125,   128,
     133,   134,   135,   137,   141,   145,   152,   153,   158,   172,
     173,   178,   179,   180,   181,   188,   189,   209,   213,   295,
     296,   356,   357,   358,   359,   362,   363,   366,   369,   370,
     371,   372,   373,   374,   375,   377,   378,   380,   381,   385,
     386,   387,   388,   109,   110,   308,   115,   308,   308,   308,
     308,    15,     3,     8,   285,    93,   131,   324,    28,   391,
     212,   211,   382,    91,    91,   320,   113,   332,   289,   291,
      24,    26,    71,   104,   105,   204,   237,   264,   261,   382,
     389,   209,   276,   226,   324,   209,   277,   389,   382,   212,
     357,   357,   209,   209,   209,   209,   209,   209,   209,   209,
     209,   209,   209,   209,   209,   209,   209,   209,   209,   209,
     209,   209,   209,   209,   209,   209,   209,   209,   209,   209,
     209,   209,   209,   209,   209,   209,   209,   209,   209,   209,
     285,   357,     8,   360,    78,   314,   316,   211,    14,    16,
      17,    18,    19,    20,    21,   297,   391,     8,     9,    10,
     111,   308,   357,   127,   210,   117,   209,   389,   127,     3,
     384,     3,   117,   263,     4,     4,   277,    35,    99,   333,
      40,    77,   117,   143,   243,   250,   266,   388,   250,   266,
     388,   117,   267,   388,    43,   197,   198,   235,   265,   209,
     163,   228,   230,   209,   238,   110,     3,   253,   386,   224,
     160,   227,   229,   150,   326,   327,   254,   388,   146,   278,
     285,     3,   170,   171,   172,   388,    25,    69,   357,   361,
      18,   361,    18,    34,    36,    37,    38,    39,    61,    62,
      63,    70,    76,    90,   108,   118,   129,   132,   138,   139,
     140,   147,   148,   174,   175,   195,   201,   245,     3,     3,
     357,     3,     3,     3,   357,     3,   357,    17,   358,   364,
     386,   357,   357,   357,   361,   361,   210,   357,   357,   357,
     357,   361,   357,   357,   357,   210,     8,   357,   357,   357,
     357,   136,   182,   183,   379,   357,   357,   289,   210,   210,
     209,   317,   320,   205,   206,   207,   208,   288,   326,   357,
     357,   357,   357,   357,   357,   357,   357,     3,     4,   323,
     302,   303,   388,   391,   257,   325,   388,   302,    30,    47,
     393,   393,   146,     8,   213,   337,   338,   386,     8,   161,
     213,   334,   335,   336,   209,    95,    95,   209,   211,   245,
     121,    95,   211,   190,    35,   164,   165,   231,   241,   242,
     243,   250,    99,   234,   382,   212,   210,   211,    28,   239,
     162,   230,    13,    73,    83,   209,   343,   344,   345,   346,
     347,   350,   351,   353,   355,   357,   365,   385,   386,   273,
     210,   211,   209,   212,   357,   357,   210,   210,   210,   210,
     209,   209,   184,   209,   339,   340,   209,   339,   209,   116,
     209,   209,   209,   209,   209,   209,   209,   211,   211,   211,
     211,   211,   211,   211,   211,   211,   211,   210,   211,   210,
     210,   211,   210,   210,   210,   211,   210,   211,   210,   211,
     210,   210,   210,   211,   210,   210,   211,   210,   210,   211,
     211,   211,   210,   210,   209,   317,    48,    79,    88,    94,
      96,   122,   211,   318,   324,    80,   328,   297,   211,    15,
     384,   210,   211,    29,    67,   340,   341,   342,   326,   209,
     341,   342,   360,   211,   339,     8,   213,   360,   211,   211,
     343,   209,   251,   252,   388,   243,   244,   388,   388,   210,
     381,   381,   166,   167,   232,   210,   211,    35,   239,   209,
     386,   240,   381,   231,   343,   209,   356,   209,   209,   343,
     357,    11,    12,    13,    15,    33,    86,    92,    97,    98,
     348,    92,   333,   388,   279,   357,   388,     8,     8,     3,
       8,     8,     8,     8,     8,     8,    17,   246,     8,   246,
       8,   246,     8,     8,   246,   357,   357,   357,     4,   357,
     357,   357,     4,   357,   357,   367,   368,   357,   357,   357,
     357,     8,   357,   357,    38,   147,   174,   175,   376,   357,
     357,   210,    94,   114,   319,    94,   319,   319,   320,    35,
     332,   303,   357,   209,   321,   257,   342,   340,   301,   279,
     341,   338,   341,   360,     8,   213,   336,   336,   210,   252,
     209,   210,   211,   247,     8,     8,   381,   168,   169,   233,
     242,     8,   256,   257,   285,     8,   232,   364,   210,   343,
     343,    33,    86,   348,    25,    27,   130,   354,   357,   357,
     209,   356,    13,   107,   357,    13,   107,   210,   211,   210,
     210,   210,   210,   210,   211,   210,   210,   211,   210,     8,
     211,   210,   211,   210,   211,   210,   210,   211,   210,   211,
     211,   210,   211,   211,   211,   210,   211,   211,   210,   211,
     210,   210,   210,   210,   210,   211,   210,   211,   210,   210,
     210,    94,   320,    94,    94,   324,     8,   329,   330,   386,
     315,   322,   323,   333,   210,   360,   210,   252,   388,    13,
      40,    65,   105,   107,   117,   120,   143,   248,     8,   156,
     210,   211,   233,   211,   357,   209,   356,   357,   356,    12,
     352,   358,   107,    72,   349,   107,   357,   339,   339,     8,
       8,     8,   339,     8,   339,     8,     8,   357,   357,   357,
     357,   357,   357,   357,   357,   367,     8,   357,   320,   324,
     320,   320,    81,   211,   331,   333,   210,   211,   205,   280,
     120,   210,   107,   209,   357,    95,   382,   388,   257,   364,
      12,   352,   349,   357,   210,   211,   358,   210,   210,   210,
     210,   210,   210,   210,   210,   210,   210,   210,   210,   210,
     210,   210,   324,   110,   324,   324,   343,   330,   323,   382,
     343,   200,   249,   209,   210,   357,   210,   358,   339,   339,
     110,   343,   110,   110,   209,   210,   253,   343,   343,   343,
     252,   210,   210
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* This macro is provided for backward compatibility. */

#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  YYSIZE_T yysize1;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = 0;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                yysize1 = yysize + yytnamerr (0, yytname[yyx]);
                if (! (yysize <= yysize1
                       && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                  return 2;
                yysize = yysize1;
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  yysize1 = yysize + yystrlen (yyformat);
  if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
    return 2;
  yysize = yysize1;

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
/* The lookahead symbol.  */
int yychar; T_POSTFIXELEM elem; char buf[MDB_BUFSIZ]; T_EXPRESSIONDESC *expr; char *p; int i; char tablename[REL_NAME_LENG]; int ret;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1806 of yacc.c  */
#line 583 "parser.y"
    {}
    break;

  case 3:

/* Line 1806 of yacc.c  */
#line 585 "parser.y"
    {
        /* return RET_SUCCESS; */
    }
    break;

  case 4:

/* Line 1806 of yacc.c  */
#line 589 "parser.y"
    {
        /* return RET_SUCCESS; */
    }
    break;

  case 23:

/* Line 1806 of yacc.c  */
#line 627 "parser.y"
    {
        vSQL(Yacc)->sqltype = ST_CREATE;
        vCREATE(Yacc) = &vSQL(Yacc)->u._create;
        Yacc->userdefined = 0;
        vCREATE(Yacc)->type = SCT_DUMMY;
    }
    break;

  case 24:

/* Line 1806 of yacc.c  */
#line 634 "parser.y"
    {
        if (vCREATE(Yacc)->type == SCT_TABLE_ELEMENTS)
        {
            T_CREATETABLE  *elements;

            elements = &vCREATE(Yacc)->u.table_elements;

            elements->table_type = (int)((yyvsp[(3) - (8)].boolean));

            if (elements->table.max_records < 0)
            {
                __yyerror("LIMIT BY is out range.");
            }

            elements->table.tablename = (yyvsp[(5) - (8)].strval);
            elements->table.aliasname = NULL;

            for (i = 0; i < Yacc->attr_list.len; i++) {
                p = YACC_XSTRDUP(Yacc, (yyvsp[(5) - (8)].strval));
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

            if ((yyvsp[(3) - (8)].boolean) == 7)
            {
                __yyerror("ERROR");
            }
            query->table_type = (int)((yyvsp[(3) - (8)].boolean));

            if (query->table.max_records < 0)
            {
                __yyerror("LIMIT BY is out range.");
            }

            query->table.tablename = (yyvsp[(5) - (8)].strval);
            query->table.aliasname = NULL;
        }
    }
    break;

  case 25:

/* Line 1806 of yacc.c  */
#line 685 "parser.y"
    {
        vSQL(Yacc)->sqltype = ST_CREATE;
        vCREATE(Yacc) = &vSQL(Yacc)->u._create;
        sc_memset(&yylval.lfield, 0, sizeof(T_LIST_FIELDS));
        Yacc->userdefined = 0;
    }
    break;

  case 26:

/* Line 1806 of yacc.c  */
#line 692 "parser.y"
    {
        vCREATE(Yacc)->type = SCT_INDEX;
        vCREATE(Yacc)->u.index.table.tablename = (yyvsp[(7) - (10)].strval);
        vCREATE(Yacc)->u.index.table.aliasname = NULL;
        vCREATE(Yacc)->u.index.indexname       = (yyvsp[(5) - (10)].strval);

        vCREATE(Yacc)->u.index.fields = (yyvsp[(9) - (10)].idxfields);
        sc_memset(&(Yacc->field_list), 0, sizeof(T_LIST_FIELDS));
    }
    break;

  case 27:

/* Line 1806 of yacc.c  */
#line 702 "parser.y"
    {
        vSQL(Yacc)->sqltype = ST_CREATE;
        vCREATE(Yacc) = &vSQL(Yacc)->u._create;
        Yacc->_select = NULL;
        Yacc->userdefined = 0;
    }
    break;

  case 28:

/* Line 1806 of yacc.c  */
#line 709 "parser.y"
    {
        vCREATE(Yacc)->type = SCT_VIEW;
        vCREATE(Yacc)->u.view.name = (yyvsp[(4) - (5)].strval);
        vCREATE(Yacc)->u.view.columns = Yacc->col_list;
        sc_memset(&(Yacc->col_list), 0, sizeof(T_LIST_ATTRDESC));
    }
    break;

  case 30:

/* Line 1806 of yacc.c  */
#line 717 "parser.y"
    {
        vSQL(Yacc)->sqltype = ST_CREATE;
        vCREATE(Yacc) = &vSQL(Yacc)->u._create;
        Yacc->_select = NULL;
        Yacc->userdefined = 0;
    }
    break;

  case 31:

/* Line 1806 of yacc.c  */
#line 724 "parser.y"
    {
        vCREATE(Yacc)->type = SCT_SEQUENCE;
        vCREATE(Yacc)->u.sequence.name = (yyvsp[(4) - (4)].strval);
    }
    break;

  case 35:

/* Line 1806 of yacc.c  */
#line 742 "parser.y"
    {
        vALTER(Yacc)->u.sequence.bStart = DB_FALSE;
    }
    break;

  case 36:

/* Line 1806 of yacc.c  */
#line 749 "parser.y"
    {
        vALTER(Yacc)->u.sequence.bStart = DB_FALSE;
    }
    break;

  case 37:

/* Line 1806 of yacc.c  */
#line 756 "parser.y"
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE)
            vCREATE(Yacc)->u.sequence.bStart= DB_FALSE;
        else if (vSQL(Yacc)->sqltype == ST_ALTER)
            vALTER(Yacc)->u.sequence.bStart = DB_FALSE;
    }
    break;

  case 38:

/* Line 1806 of yacc.c  */
#line 763 "parser.y"
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE)   {
            vCREATE(Yacc)->u.sequence.start = (MDB_INT32)((yyvsp[(4) - (4)].intval) * (yyvsp[(3) - (4)].num));
            vCREATE(Yacc)->u.sequence.bStart= DB_TRUE;
        } else if (vSQL(Yacc)->sqltype == ST_ALTER) {
            vALTER(Yacc)->u.sequence.start  = (MDB_INT32)((yyvsp[(4) - (4)].intval) * (yyvsp[(3) - (4)].num));
            vALTER(Yacc)->u.sequence.bStart = DB_TRUE;
        }
    }
    break;

  case 39:

/* Line 1806 of yacc.c  */
#line 776 "parser.y"
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE)
            vCREATE(Yacc)->u.sequence.bIncrement= DB_FALSE;
        else if (vSQL(Yacc)->sqltype == ST_ALTER)
            vALTER(Yacc)->u.sequence.bIncrement = DB_FALSE;
    }
    break;

  case 40:

/* Line 1806 of yacc.c  */
#line 783 "parser.y"
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE)   {
            vCREATE(Yacc)->u.sequence.increment = (MDB_INT32)((yyvsp[(4) - (4)].intval) * (yyvsp[(3) - (4)].num));
            vCREATE(Yacc)->u.sequence.bIncrement= DB_TRUE;
        } else if (vSQL(Yacc)->sqltype == ST_ALTER) {
            vALTER(Yacc)->u.sequence.increment  = (MDB_INT32)((yyvsp[(4) - (4)].intval) * (yyvsp[(3) - (4)].num));
            vALTER(Yacc)->u.sequence.bIncrement = DB_TRUE;
        }
    }
    break;

  case 41:

/* Line 1806 of yacc.c  */
#line 796 "parser.y"
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE) 
            vCREATE(Yacc)->u.sequence.bMaxValue = DB_FALSE;
        else if (vSQL(Yacc)->sqltype == ST_ALTER)
            vALTER(Yacc)->u.sequence.bMaxValue  = DB_FALSE;
    }
    break;

  case 42:

/* Line 1806 of yacc.c  */
#line 803 "parser.y"
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE)   {
            vCREATE(Yacc)->u.sequence.maxValue  = (MDB_INT32)((yyvsp[(3) - (3)].intval) * (yyvsp[(2) - (3)].num));
            vCREATE(Yacc)->u.sequence.bMaxValue = DB_TRUE;
        } else if (vSQL(Yacc)->sqltype == ST_ALTER) {
            vALTER(Yacc)->u.sequence.maxValue   = (MDB_INT32)((yyvsp[(3) - (3)].intval) * (yyvsp[(2) - (3)].num));
            vALTER(Yacc)->u.sequence.bMaxValue  = DB_TRUE;
        }
    }
    break;

  case 43:

/* Line 1806 of yacc.c  */
#line 813 "parser.y"
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE) 
            vCREATE(Yacc)->u.sequence.bMaxValue = DB_FALSE;
        else if (vSQL(Yacc)->sqltype == ST_ALTER)
            vALTER(Yacc)->u.sequence.bMaxValue  = DB_FALSE;
    }
    break;

  case 44:

/* Line 1806 of yacc.c  */
#line 823 "parser.y"
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE) 
            vCREATE(Yacc)->u.sequence.bMinValue = DB_FALSE;
        else if (vSQL(Yacc)->sqltype == ST_ALTER)
            vALTER(Yacc)->u.sequence.bMinValue  = DB_FALSE;
    }
    break;

  case 45:

/* Line 1806 of yacc.c  */
#line 830 "parser.y"
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE) {
            vCREATE(Yacc)->u.sequence.minValue = (MDB_INT32)((yyvsp[(3) - (3)].intval) * (yyvsp[(2) - (3)].num));
            vCREATE(Yacc)->u.sequence.bMinValue = DB_TRUE;
        } else if (vSQL(Yacc)->sqltype == ST_ALTER) {
            vALTER(Yacc)->u.sequence.minValue = (MDB_INT32)((yyvsp[(3) - (3)].intval) * (yyvsp[(2) - (3)].num));
            vALTER(Yacc)->u.sequence.bMinValue  = DB_TRUE;
        }
    }
    break;

  case 46:

/* Line 1806 of yacc.c  */
#line 840 "parser.y"
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE) 
            vCREATE(Yacc)->u.sequence.bMinValue = DB_FALSE;
        else if (vSQL(Yacc)->sqltype == ST_ALTER)
            vALTER(Yacc)->u.sequence.bMinValue  = DB_FALSE;
    }
    break;

  case 47:

/* Line 1806 of yacc.c  */
#line 850 "parser.y"
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE) 
            vCREATE(Yacc)->u.sequence.cycled= DB_FALSE;
        else if (vSQL(Yacc)->sqltype == ST_ALTER)
            vALTER(Yacc)->u.sequence.cycled = DB_FALSE;
    }
    break;

  case 48:

/* Line 1806 of yacc.c  */
#line 857 "parser.y"
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE) 
            vCREATE(Yacc)->u.sequence.cycled= DB_TRUE;
        else if (vSQL(Yacc)->sqltype == ST_ALTER)
            vALTER(Yacc)->u.sequence.cycled = DB_TRUE;
    }
    break;

  case 49:

/* Line 1806 of yacc.c  */
#line 864 "parser.y"
    {
        if (vSQL(Yacc)->sqltype == ST_CREATE) 
            vCREATE(Yacc)->u.sequence.cycled= DB_FALSE;
        else if (vSQL(Yacc)->sqltype == ST_ALTER)
            vALTER(Yacc)->u.sequence.cycled = DB_FALSE;
    }
    break;

  case 50:

/* Line 1806 of yacc.c  */
#line 873 "parser.y"
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
    break;

  case 51:

/* Line 1806 of yacc.c  */
#line 886 "parser.y"
    {
        if ((yyvsp[(3) - (5)].intval) < 1 || (yyvsp[(3) - (5)].intval) > MDB_INT_MAX)
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
                vCREATE(Yacc)->u.table_elements.table.max_records = (int)(yyvsp[(3) - (5)].intval);
                vCREATE(Yacc)->u.table_elements.table.column_name = (yyvsp[(5) - (5)].strval);
            }
            else
            {
                vCREATE(Yacc)->u.table_query.table.max_records = (int)(yyvsp[(3) - (5)].intval);
                vCREATE(Yacc)->u.table_query.table.column_name = (yyvsp[(5) - (5)].strval);
            }
        }
    }
    break;

  case 52:

/* Line 1806 of yacc.c  */
#line 915 "parser.y"
    {    
        if ((yyvsp[(3) - (3)].intval) < 1 || (yyvsp[(3) - (3)].intval) > MDB_INT_MAX)
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
                vCREATE(Yacc)->u.table_elements.table.max_records = (int)(yyvsp[(3) - (3)].intval);
                vCREATE(Yacc)->u.table_elements.table.column_name =
                    YACC_XSTRDUP(Yacc, "#");
                isnull(vCREATE(Yacc)->u.table_elements.table.column_name);
            }
            else
            {
                vCREATE(Yacc)->u.table_query.table.max_records = (int)(yyvsp[(3) - (3)].intval);
                vCREATE(Yacc)->u.table_query.table.column_name =
                    YACC_XSTRDUP(Yacc, "#");
                isnull(vCREATE(Yacc)->u.table_query.table.column_name);
            }
        }
    }
    break;

  case 53:

/* Line 1806 of yacc.c  */
#line 951 "parser.y"
    {
        (yyval.boolean) = 1;
    }
    break;

  case 54:

/* Line 1806 of yacc.c  */
#line 955 "parser.y"
    {
        (yyval.boolean) = 0;
    }
    break;

  case 55:

/* Line 1806 of yacc.c  */
#line 962 "parser.y"
    {
        (yyval.boolean) = 0;
    }
    break;

  case 56:

/* Line 1806 of yacc.c  */
#line 966 "parser.y"
    {
        (yyval.boolean) = 2;
    }
    break;

  case 57:

/* Line 1806 of yacc.c  */
#line 970 "parser.y"
    {
        (yyval.boolean) = 6;
    }
    break;

  case 58:

/* Line 1806 of yacc.c  */
#line 974 "parser.y"
    {
        (yyval.boolean) = 7;
    }
    break;

  case 59:

/* Line 1806 of yacc.c  */
#line 981 "parser.y"
    {
        (yyval.boolean) = 0;
    }
    break;

  case 60:

/* Line 1806 of yacc.c  */
#line 985 "parser.y"
    {
        (yyval.boolean) = 2;
    }
    break;

  case 62:

/* Line 1806 of yacc.c  */
#line 993 "parser.y"
    {
        vCREATE(Yacc)->type = SCT_TABLE_ELEMENTS;
    }
    break;

  case 63:

/* Line 1806 of yacc.c  */
#line 1000 "parser.y"
    {
        if (vCREATE(Yacc)->type == SCT_VIEW)
        {
            __yyerror("syntax error");
        }
    }
    break;

  case 64:

/* Line 1806 of yacc.c  */
#line 1007 "parser.y"
    {
        if (vCREATE(Yacc)->type == SCT_VIEW)
        {
            vCREATE(Yacc)->u.view.qstring =
                sql_mem_strdup(vSQL(Yacc)->parsing_memory,
                        get_remain_qstring(Yacc));
            isnull(vCREATE(Yacc)->u.view.qstring);
        }
    }
    break;

  case 65:

/* Line 1806 of yacc.c  */
#line 1017 "parser.y"
    {
        if (vCREATE(Yacc)->type != SCT_VIEW)
        {
            if (vCREATE(Yacc)->type == SCT_TABLE_ELEMENTS)
                __yyerror("ERROR");
            vCREATE(Yacc)->type = SCT_TABLE_QUERY;
        }
    }
    break;

  case 68:

/* Line 1806 of yacc.c  */
#line 1034 "parser.y"
    {
        CHECK_ARRAY(Yacc->stmt, &(Yacc->attr_list), sizeof(T_ATTRDESC));
        append(&(Yacc->attr_list), &(Yacc->attr));
        sc_memset(&(Yacc->attr), 0, sizeof(T_ATTRDESC));
    }
    break;

  case 70:

/* Line 1806 of yacc.c  */
#line 1044 "parser.y"
    {
        Yacc->attr.attrname         = (yyvsp[(1) - (2)].strval);
        Yacc->attr.flag             = NULL_BIT;
        Yacc->attr.defvalue.defined = 0;
        Yacc->userdefined           = 0;
    }
    break;

  case 71:

/* Line 1806 of yacc.c  */
#line 1051 "parser.y"
    {
        if ((Yacc->attr.flag & AUTO_BIT) && Yacc->attr.defvalue.defined)
            __yyerror("AUTOINCREMENT not allow DEFAULT value");
        if ((Yacc->attr.flag & AUTO_BIT) &&
                (Yacc->attr.type != DT_INTEGER))
            __yyerror("AUTOINCREMENT allowed only INTEGER PRIMARY KEY");
    }
    break;

  case 72:

/* Line 1806 of yacc.c  */
#line 1062 "parser.y"
    {
        Yacc->attr.type        = DT_BIGINT;
        Yacc->attr.len        = sizeof(bigint);
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec        = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 73:

/* Line 1806 of yacc.c  */
#line 1070 "parser.y"
    {
        Yacc->attr.type     = DT_CHAR;
        Yacc->attr.len      = (int)(yyvsp[(3) - (4)].intval);
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = DB_Params.default_col_char;
    }
    break;

  case 74:

/* Line 1806 of yacc.c  */
#line 1078 "parser.y"
    {
        Yacc->attr.type     = DT_NCHAR;
        Yacc->attr.len      = (int)(yyvsp[(3) - (4)].intval);
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = DB_Params.default_col_nchar;
    }
    break;

  case 75:

/* Line 1806 of yacc.c  */
#line 1086 "parser.y"
    {
        Yacc->attr.type     = DT_CHAR;
        Yacc->attr.len      = 1;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        if ((yyvsp[(2) - (2)].num) == MDB_COL_NONE)
        {
            Yacc->attr.collation = DB_Params.default_col_char;
        }
        else
        {
            Yacc->attr.collation= (yyvsp[(2) - (2)].num);
        }
    }
    break;

  case 76:

/* Line 1806 of yacc.c  */
#line 1101 "parser.y"
    {
        Yacc->attr.type     = DT_CHAR;
        Yacc->attr.len      = (int)(yyvsp[(3) - (5)].intval);
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        if ((yyvsp[(5) - (5)].num) == MDB_COL_NONE)
        {
            Yacc->attr.collation = DB_Params.default_col_char;
        }
        else
        {
            Yacc->attr.collation= (yyvsp[(5) - (5)].num);
        }
    }
    break;

  case 77:

/* Line 1806 of yacc.c  */
#line 1116 "parser.y"
    {
        Yacc->attr.type     = DT_NCHAR;
        Yacc->attr.len      = 1;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        if ((yyvsp[(2) - (2)].num) == MDB_COL_NONE)
        {
            Yacc->attr.collation = DB_Params.default_col_nchar;
        }
        else
        {
            Yacc->attr.collation= (yyvsp[(2) - (2)].num);
        }
    }
    break;

  case 78:

/* Line 1806 of yacc.c  */
#line 1131 "parser.y"
    {
        Yacc->attr.type     = DT_NCHAR;
        Yacc->attr.len      = (int)(yyvsp[(3) - (5)].intval);
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        if ((yyvsp[(5) - (5)].num) == MDB_COL_NONE)
        {
            Yacc->attr.collation = DB_Params.default_col_nchar;
        }
        else
        {
            Yacc->attr.collation= (yyvsp[(5) - (5)].num);
        }
    }
    break;

  case 79:

/* Line 1806 of yacc.c  */
#line 1146 "parser.y"
    {
        Yacc->attr.type     = DT_DATETIME;
        Yacc->attr.len      = MAX_DATETIME_FIELD_LEN;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 80:

/* Line 1806 of yacc.c  */
#line 1154 "parser.y"
    {
        Yacc->attr.type     = DT_DATE;
        Yacc->attr.len      = MAX_DATE_FIELD_LEN;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 81:

/* Line 1806 of yacc.c  */
#line 1162 "parser.y"
    {
        Yacc->attr.type     = DT_TIME;
        Yacc->attr.len      = MAX_TIME_FIELD_LEN;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 82:

/* Line 1806 of yacc.c  */
#line 1170 "parser.y"
    {
        Yacc->attr.type     = DT_DECIMAL;
        Yacc->attr.len      = DEFAULT_DECIMAL_SIZE;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 83:

/* Line 1806 of yacc.c  */
#line 1178 "parser.y"
    {
        Yacc->attr.type     = DT_DECIMAL;
        if ((yyvsp[(3) - (4)].intval) > MAX_DECIMAL_PRECISION)
            __yyerror("numeric precision specifier is out of range");
        Yacc->attr.len         = (int)(yyvsp[(3) - (4)].intval);
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec         = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 84:

/* Line 1806 of yacc.c  */
#line 1188 "parser.y"
    {
        Yacc->attr.type     = DT_DECIMAL;
        if ((yyvsp[(3) - (6)].intval) > MAX_DECIMAL_PRECISION)
            __yyerror("numeric precision specifier is out of range");
        if ((yyvsp[(3) - (6)].intval) < (yyvsp[(5) - (6)].intval)) __yyerror("scale larger than specified precision");

        Yacc->attr.len         = (int)(yyvsp[(3) - (6)].intval);
        Yacc->attr.fixedlen    = -1;
        if ((yyvsp[(5) - (6)].intval) < MIN_DECIMAL_SCALE) Yacc->attr.dec = MIN_DECIMAL_SCALE;
        else if ((yyvsp[(5) - (6)].intval) > MAX_DECIMAL_SCALE)
            Yacc->attr.dec = MAX_DECIMAL_SCALE;
        else Yacc->attr.dec = (int)(yyvsp[(5) - (6)].intval);
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 85:

/* Line 1806 of yacc.c  */
#line 1203 "parser.y"
    {
        Yacc->attr.type     = DT_DOUBLE;
        Yacc->attr.len      = sizeof(double);
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 86:

/* Line 1806 of yacc.c  */
#line 1211 "parser.y"
    {
        Yacc->attr.type     = DT_DOUBLE;
        Yacc->attr.len      = sizeof(double);
        Yacc->attr.fixedlen    = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 87:

/* Line 1806 of yacc.c  */
#line 1219 "parser.y"
    {
        Yacc->attr.type     = DT_FLOAT;
        Yacc->attr.len      = sizeof(float);
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 88:

/* Line 1806 of yacc.c  */
#line 1227 "parser.y"
    {
        Yacc->attr.type     = DT_FLOAT;
        Yacc->attr.len      = sizeof(float);
        Yacc->attr.fixedlen    = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 89:

/* Line 1806 of yacc.c  */
#line 1235 "parser.y"
    {
        Yacc->attr.type     = DT_INTEGER;
        Yacc->attr.len      = sizeof(int);
        Yacc->attr.fixedlen    = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 90:

/* Line 1806 of yacc.c  */
#line 1243 "parser.y"
    {
        /* OID convert */
        Yacc->attr.type     = DT_OID;
        Yacc->attr.len      = sizeof(OID);
        Yacc->attr.fixedlen    = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 91:

/* Line 1806 of yacc.c  */
#line 1252 "parser.y"
    {
        Yacc->attr.type     = DT_DECIMAL;
        Yacc->attr.len      = DEFAULT_DECIMAL_SIZE;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 92:

/* Line 1806 of yacc.c  */
#line 1260 "parser.y"
    {
        Yacc->attr.type     = DT_DECIMAL;
        if ((yyvsp[(3) - (4)].intval) > MAX_DECIMAL_PRECISION)
            __yyerror("numeric precision specifier is out of range");
        Yacc->attr.len         = (int)(yyvsp[(3) - (4)].intval);
        Yacc->attr.fixedlen    = -1;
        Yacc->attr.dec         = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 93:

/* Line 1806 of yacc.c  */
#line 1270 "parser.y"
    {
        Yacc->attr.type     = DT_DECIMAL;
        if ((yyvsp[(3) - (6)].intval) > MAX_DECIMAL_PRECISION)
            __yyerror("numeric precision specifier is out of range");
        if ((yyvsp[(3) - (6)].intval) < (yyvsp[(5) - (6)].intval)) __yyerror("scale larger than specified precision");

        Yacc->attr.len         = (int)(yyvsp[(3) - (6)].intval);
        Yacc->attr.fixedlen    = -1;
        if ((yyvsp[(5) - (6)].intval) < MIN_DECIMAL_SCALE) Yacc->attr.dec = MIN_DECIMAL_SCALE;
        else if ((yyvsp[(5) - (6)].intval) > MAX_DECIMAL_SCALE)
            Yacc->attr.dec = MAX_DECIMAL_SCALE;
        else Yacc->attr.dec = (int)(yyvsp[(5) - (6)].intval);
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 94:

/* Line 1806 of yacc.c  */
#line 1285 "parser.y"
    {
        Yacc->attr.type     = DT_DOUBLE;
        Yacc->attr.len      = sizeof(double);
        Yacc->attr.fixedlen    = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 95:

/* Line 1806 of yacc.c  */
#line 1293 "parser.y"
    {
        Yacc->attr.type     = DT_SMALLINT;
        Yacc->attr.len      = sizeof(smallint);
        Yacc->attr.fixedlen    = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 96:

/* Line 1806 of yacc.c  */
#line 1301 "parser.y"
    {
        Yacc->attr.type     = DT_VARCHAR;
        Yacc->attr.len      = (int)(yyvsp[(3) - (4)].intval);
        Yacc->attr.fixedlen = 0;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = DB_Params.default_col_char;
    }
    break;

  case 97:

/* Line 1806 of yacc.c  */
#line 1309 "parser.y"
    {
        Yacc->attr.type     = DT_VARCHAR;
        Yacc->attr.len      = (int)(yyvsp[(5) - (6)].intval);
        Yacc->attr.fixedlen = (int)(yyvsp[(3) - (6)].intval);
        if (Yacc->attr.len < Yacc->attr.fixedlen)
            __yyerror("fixed length of varchar is out of range");
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = DB_Params.default_col_char;
    }
    break;

  case 98:

/* Line 1806 of yacc.c  */
#line 1319 "parser.y"
    {
        Yacc->attr.type     = DT_TIMESTAMP;
        Yacc->attr.len      = MAX_TIMESTAMP_FIELD_LEN;
        Yacc->attr.fixedlen    = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 99:

/* Line 1806 of yacc.c  */
#line 1327 "parser.y"
    {
        Yacc->attr.type     = DT_TINYINT;
        Yacc->attr.len      = sizeof(tinyint);
        Yacc->attr.fixedlen    = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_NUMERIC;
    }
    break;

  case 100:

/* Line 1806 of yacc.c  */
#line 1335 "parser.y"
    {
        Yacc->attr.type     = DT_VARCHAR;
        Yacc->attr.len      = (int)(yyvsp[(3) - (5)].intval);
        Yacc->attr.fixedlen = 0;
        Yacc->attr.dec      = 0;
        if ((yyvsp[(5) - (5)].num) == MDB_COL_NONE)
        {
            Yacc->attr.collation = DB_Params.default_col_char;
        }
        else
        {
            Yacc->attr.collation= (yyvsp[(5) - (5)].num);
        }
    }
    break;

  case 101:

/* Line 1806 of yacc.c  */
#line 1350 "parser.y"
    {
        Yacc->attr.type     = DT_NVARCHAR;
        Yacc->attr.len      = (int)(yyvsp[(3) - (5)].intval);
        Yacc->attr.fixedlen = 0;
        Yacc->attr.dec      = 0;
        if ((yyvsp[(5) - (5)].num) == MDB_COL_NONE)
        {
            Yacc->attr.collation = DB_Params.default_col_nchar;
        }
        else
        {
            Yacc->attr.collation= (yyvsp[(5) - (5)].num);
        }
    }
    break;

  case 102:

/* Line 1806 of yacc.c  */
#line 1365 "parser.y"
    {
        Yacc->attr.type     = DT_VARCHAR;
        Yacc->attr.len      = (int)(yyvsp[(5) - (7)].intval);
        Yacc->attr.fixedlen = (int)(yyvsp[(3) - (7)].intval);
        if (Yacc->attr.len < Yacc->attr.fixedlen)
            __yyerror("fixed length of varchar is out of range");
        Yacc->attr.dec      = 0;
        if ((yyvsp[(7) - (7)].num) == MDB_COL_NONE)
        {
            Yacc->attr.collation = DB_Params.default_col_char;
        }
        else
        {
            Yacc->attr.collation= (yyvsp[(7) - (7)].num);
        }
    }
    break;

  case 103:

/* Line 1806 of yacc.c  */
#line 1382 "parser.y"
    {
        Yacc->attr.type     = DT_NVARCHAR;
        Yacc->attr.len      = (int)(yyvsp[(5) - (7)].intval);
        Yacc->attr.fixedlen = (int)(yyvsp[(3) - (7)].intval);
        if (Yacc->attr.len < Yacc->attr.fixedlen)
            __yyerror("fixed length of varchar is out of range");
        Yacc->attr.dec      = 0;
        if ((yyvsp[(7) - (7)].num) == MDB_COL_NONE)
        {
            Yacc->attr.collation = DB_Params.default_col_nchar;
        }
        else
        {
            Yacc->attr.collation= (yyvsp[(7) - (7)].num);
        }
    }
    break;

  case 104:

/* Line 1806 of yacc.c  */
#line 1399 "parser.y"
    {
        Yacc->attr.type     = DT_BYTE;
        Yacc->attr.len      = 1;
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_DEFAULT_BYTE;
    }
    break;

  case 105:

/* Line 1806 of yacc.c  */
#line 1407 "parser.y"
    {
        Yacc->attr.type     = DT_BYTE;
        Yacc->attr.len      = (int)(yyvsp[(3) - (4)].intval);
        Yacc->attr.fixedlen = -1;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_DEFAULT_BYTE;
    }
    break;

  case 106:

/* Line 1806 of yacc.c  */
#line 1415 "parser.y"
    {
        Yacc->attr.type     = DT_VARBYTE;
        Yacc->attr.len      = (int)(yyvsp[(3) - (4)].intval);
        Yacc->attr.fixedlen = 0;
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_DEFAULT_BYTE;
    }
    break;

  case 107:

/* Line 1806 of yacc.c  */
#line 1423 "parser.y"
    {
        Yacc->attr.type     = DT_VARBYTE;
        Yacc->attr.len      = (int)(yyvsp[(5) - (6)].intval);
        Yacc->attr.fixedlen = (int)(yyvsp[(3) - (6)].intval);
        if (Yacc->attr.len < Yacc->attr.fixedlen)
            __yyerror("fixed length of varchar is out of range");
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_DEFAULT_BYTE;
    }
    break;

  case 108:

/* Line 1806 of yacc.c  */
#line 1433 "parser.y"
    {
        Yacc->attr.type     = DT_VARBYTE;
        Yacc->attr.len      = 8000;
        Yacc->attr.fixedlen = 0;
        if (Yacc->attr.len < Yacc->attr.fixedlen)
            __yyerror("fixed length of varchar is out of range");
        Yacc->attr.dec      = 0;
        Yacc->attr.collation = MDB_COL_DEFAULT_BYTE;
    }
    break;

  case 109:

/* Line 1806 of yacc.c  */
#line 1446 "parser.y"
    {
        (yyval.intval) = (yyvsp[(1) - (1)].intval);
    }
    break;

  case 110:

/* Line 1806 of yacc.c  */
#line 1450 "parser.y"
    {
        if ((yyvsp[(2) - (2)].intval) != 1)
            __yyerror("fixed length of varchar is out of range");
        (yyval.intval) = -1;
    }
    break;

  case 112:

/* Line 1806 of yacc.c  */
#line 1461 "parser.y"
    {
        ;
    }
    break;

  case 113:

/* Line 1806 of yacc.c  */
#line 1468 "parser.y"
    {
        if (Yacc->userdefined & 0x02 || Yacc->attr.flag&PRI_KEY_BIT)
            __yyerror("ERROR");
        else {
            Yacc->userdefined |= 0x01;
            Yacc->attr.flag   |= EXPLICIT_NULL_BIT;
        }
    }
    break;

  case 114:

/* Line 1806 of yacc.c  */
#line 1477 "parser.y"
    {
        if (Yacc->userdefined & 0x01) __yyerror("ERROR");
        else {
            Yacc->attr.flag   &= ~NULL_BIT;
            Yacc->userdefined |= 0x02;
        }
    }
    break;

  case 115:

/* Line 1806 of yacc.c  */
#line 1485 "parser.y"
    {
        INCOMPLETION("UNIQUE");
    }
    break;

  case 116:

/* Line 1806 of yacc.c  */
#line 1489 "parser.y"
    {
        if (Yacc->field_list.list) __yyerror("ERROR");
        if (Yacc->userdefined & 0x01) __yyerror("ERROR");
        if ((yyvsp[(3) - (3)].boolean))
            Yacc->attr.flag |= AUTO_BIT;
        Yacc->attr.flag |= PRI_KEY_BIT; // indicate a key field.
        Yacc->attr.flag &= ~NULL_BIT; // remove the NULL-allowed bit.

        p = YACC_XSTRDUP(Yacc, Yacc->attr.attrname);
        isnull(p);

        CHECK_ARRAY(Yacc->stmt, &(Yacc->field_list), sizeof(char*));
        Yacc->field_list.list[Yacc->field_list.len++] = p;
    }
    break;

  case 117:

/* Line 1806 of yacc.c  */
#line 1504 "parser.y"
    {
        ret = ACTION_default_scalar_exp(Yacc, (yyvsp[(2) - (2)].expr));
        if (ret < 0)
            return ret;
    }
    break;

  case 118:

/* Line 1806 of yacc.c  */
#line 1510 "parser.y"
    {
        INCOMPLETION("CHECK");
    }
    break;

  case 119:

/* Line 1806 of yacc.c  */
#line 1514 "parser.y"
    {
        INCOMPLETION("REFERENCES");
    }
    break;

  case 120:

/* Line 1806 of yacc.c  */
#line 1518 "parser.y"
    {
        INCOMPLETION("REFERENCES");
    }
    break;

  case 121:

/* Line 1806 of yacc.c  */
#line 1522 "parser.y"
    {
        if (IS_VARIABLE(Yacc->attr.type))
        {
            __yyerror("Invalid NOLOGGING column.");
        }

        Yacc->attr.flag   |= NOLOGGING_BIT;
    }
    break;

  case 122:

/* Line 1806 of yacc.c  */
#line 1534 "parser.y"
    {
        (yyval.boolean) = 0;
    }
    break;

  case 123:

/* Line 1806 of yacc.c  */
#line 1538 "parser.y"
    {
        (yyval.boolean) = 1;
    }
    break;

  case 124:

/* Line 1806 of yacc.c  */
#line 1544 "parser.y"
    {
        INCOMPLETION("UNIQUE");
    }
    break;

  case 125:

/* Line 1806 of yacc.c  */
#line 1548 "parser.y"
    {
        if (Yacc->field_list.list) __yyerror("ERROR");
    }
    break;

  case 127:

/* Line 1806 of yacc.c  */
#line 1553 "parser.y"
    {
        INCOMPLETION("FOREIGN KEY");
    }
    break;

  case 128:

/* Line 1806 of yacc.c  */
#line 1557 "parser.y"
    {
        INCOMPLETION("FOREIGN KEY");
    }
    break;

  case 129:

/* Line 1806 of yacc.c  */
#line 1561 "parser.y"
    {
        INCOMPLETION("CHECK");
    }
    break;

  case 130:

/* Line 1806 of yacc.c  */
#line 1568 "parser.y"
    {
        CHECK_ARRAY(Yacc->stmt, &(Yacc->field_list), sizeof(char*));
        Yacc->field_list.list[Yacc->field_list.len++] = (yyvsp[(1) - (1)].strval);
    }
    break;

  case 131:

/* Line 1806 of yacc.c  */
#line 1573 "parser.y"
    {
        CHECK_ARRAY(Yacc->stmt, &(Yacc->field_list), sizeof(char*));
        Yacc->field_list.list[Yacc->field_list.len++] = (yyvsp[(3) - (3)].strval);
    }
    break;

  case 132:

/* Line 1806 of yacc.c  */
#line 1581 "parser.y"
    {
        T_ATTRDESC *attr;

        CHECK_ARRAY(Yacc->stmt, &(Yacc->col_list), sizeof(T_ATTRDESC));

        attr = &(Yacc->col_list.list[Yacc->col_list.len++]);

        p = sc_strchr((yyvsp[(1) - (1)].strval), '.');
        if (p) {
            sc_strncpy(tablename, (yyvsp[(1) - (1)].strval), p-(yyvsp[(1) - (1)].strval));
            tablename[p++-(yyvsp[(1) - (1)].strval)] = '\0';

            attr->table.tablename = YACC_XSTRDUP(Yacc, tablename);
            isnull(attr->table.tablename);
            attr->table.aliasname = NULL;
            attr->attrname = YACC_XSTRDUP(Yacc, p);
            isnull(attr->attrname);
        } else {
            attr->table.tablename = NULL;
            attr->table.aliasname = NULL;
            attr->attrname = (yyvsp[(1) - (1)].strval);
        }
    }
    break;

  case 133:

/* Line 1806 of yacc.c  */
#line 1605 "parser.y"
    {
        T_ATTRDESC *attr;

        CHECK_ARRAY(Yacc->stmt, &(Yacc->col_list), sizeof(T_ATTRDESC));

        attr = &(Yacc->col_list.list[Yacc->col_list.len++]);
        p = sc_strchr((yyvsp[(3) - (3)].strval), '.');
        if (p) {
            sc_strncpy(tablename, (yyvsp[(3) - (3)].strval), p-(yyvsp[(3) - (3)].strval));
            tablename[p++-(yyvsp[(3) - (3)].strval)] = '\0';

            attr->table.tablename = YACC_XSTRDUP(Yacc, tablename);
            isnull(attr->table.tablename);
            attr->table.aliasname = NULL;
            attr->attrname = YACC_XSTRDUP(Yacc, p);
            isnull(attr->attrname);
        } else {
            attr->table.tablename = NULL;
            attr->table.aliasname = NULL;
            attr->attrname = (yyvsp[(3) - (3)].strval);
        }
    }
    break;

  case 134:

/* Line 1806 of yacc.c  */
#line 1631 "parser.y"
    {
        T_COLDESC *column;

        CHECK_ARRAY(Yacc->stmt, &(Yacc->coldesc_list), sizeof(T_COLDESC));
        column = &(Yacc->coldesc_list.list[Yacc->coldesc_list.len++]);
        column->name = (yyvsp[(1) - (1)].strval);
        column->fieldinfo = NULL;
    }
    break;

  case 135:

/* Line 1806 of yacc.c  */
#line 1640 "parser.y"
    {
        T_COLDESC *column;

        CHECK_ARRAY(Yacc->stmt, &(Yacc->coldesc_list), sizeof(T_COLDESC));
        column = &(Yacc->coldesc_list.list[Yacc->coldesc_list.len++]);
        column->name = (yyvsp[(3) - (3)].strval);
        column->fieldinfo = NULL;
    }
    break;

  case 137:

/* Line 1806 of yacc.c  */
#line 1653 "parser.y"
    {
        vCREATE(Yacc)->u.index.uniqueness = UNIQUE_INDEX_BIT;
    }
    break;

  case 138:

/* Line 1806 of yacc.c  */
#line 1661 "parser.y"
    {
        sc_memset(&(yyval.idxfields), 0, sizeof(T_IDX_LIST_FIELDS));
        CHECK_ARRAY(Yacc->stmt, &(yyval.idxfields), sizeof(T_IDX_FIELD));
        (yyval.idxfields).list[(yyval.idxfields).len++] = (yyvsp[(1) - (1)].idxfield);
    }
    break;

  case 139:

/* Line 1806 of yacc.c  */
#line 1667 "parser.y"
    {
        CHECK_ARRAY(Yacc->stmt, &(yyval.idxfields), sizeof(T_IDX_FIELD));
        (yyval.idxfields).list[(yyval.idxfields).len++] = (yyvsp[(3) - (3)].idxfield);
    }
    break;

  case 140:

/* Line 1806 of yacc.c  */
#line 1675 "parser.y"
    {
        (yyval.idxfield).name = (yyvsp[(1) - (2)].strval);
        (yyval.idxfield).ordertype = (yyvsp[(2) - (2)].num);
        (yyval.idxfield).collation = MDB_COL_NONE;
    }
    break;

  case 141:

/* Line 1806 of yacc.c  */
#line 1681 "parser.y"
    {
        (yyval.idxfield).name = (yyvsp[(1) - (2)].strval);
        (yyval.idxfield).ordertype = 'A';
        (yyval.idxfield).collation = (yyvsp[(2) - (2)].num);
    }
    break;

  case 142:

/* Line 1806 of yacc.c  */
#line 1687 "parser.y"
    {
        (yyval.idxfield).name = (yyvsp[(1) - (3)].strval);
        (yyval.idxfield).ordertype = (yyvsp[(2) - (3)].num);
        (yyval.idxfield).collation = (yyvsp[(3) - (3)].num);
    }
    break;

  case 143:

/* Line 1806 of yacc.c  */
#line 1693 "parser.y"
    {
        (yyval.idxfield).name = (yyvsp[(1) - (3)].strval);
        (yyval.idxfield).ordertype = (yyvsp[(3) - (3)].num);
        (yyval.idxfield).collation = (yyvsp[(2) - (3)].num);
    }
    break;

  case 144:

/* Line 1806 of yacc.c  */
#line 1702 "parser.y"
    {
        vSQL(Yacc)->sqltype = ST_ALTER;
        vALTER(Yacc) = &vSQL(Yacc)->u._alter;
        Yacc->userdefined = 0;
    }
    break;

  case 145:

/* Line 1806 of yacc.c  */
#line 1708 "parser.y"
    {
        vALTER(Yacc)->tablename = (yyvsp[(4) - (5)].strval);
    }
    break;

  case 146:

/* Line 1806 of yacc.c  */
#line 1712 "parser.y"
    {
        vSQL(Yacc)->sqltype = ST_ALTER;
        vALTER(Yacc) = &vSQL(Yacc)->u._alter;
        Yacc->userdefined = 0;
    }
    break;

  case 147:

/* Line 1806 of yacc.c  */
#line 1718 "parser.y"
    {
        vALTER(Yacc)->type = SAT_SEQUENCE;
        vALTER(Yacc)->u.sequence.name = (yyvsp[(4) - (4)].strval);
    }
    break;

  case 149:

/* Line 1806 of yacc.c  */
#line 1724 "parser.y"
    {
        vSQL(Yacc)->sqltype = ST_ALTER;
        vALTER(Yacc) = &vSQL(Yacc)->u._alter;
        vALTER(Yacc)->type = SAT_REBUILD;
        Yacc->userdefined = 0;

        expr = (T_EXPRESSIONDESC *)(yyvsp[(3) - (3)].expr);
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
    break;

  case 150:

/* Line 1806 of yacc.c  */
#line 1746 "parser.y"
    {
        vSQL(Yacc)->sqltype = ST_ALTER;
        vALTER(Yacc) = &vSQL(Yacc)->u._alter;
        vALTER(Yacc)->type = SAT_REBUILD;
        Yacc->userdefined = 0;

        vALTER(Yacc)->u.sequence.bStart = DB_FALSE;
        vALTER(Yacc)->u.rebuild_idx.num_object = 0;
        vALTER(Yacc)->u.rebuild_idx.objectname = NULL;
    }
    break;

  case 152:

/* Line 1806 of yacc.c  */
#line 1761 "parser.y"
    {
        (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), NULL);
    }
    break;

  case 153:

/* Line 1806 of yacc.c  */
#line 1768 "parser.y"
    {
        sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
        elem.elemtype = SPT_VALUE;
        elem.u.value.valueclass     = SVC_CONSTANT;
        elem.u.value.valuetype      = DT_VARCHAR;
        elem.u.value.u.ptr          = (yyvsp[(1) - (1)].strval);
        elem.u.value.attrdesc.len   = sc_strlen((yyvsp[(1) - (1)].strval));
        elem.u.value.value_len      = elem.u.value.attrdesc.len;
        (yyval.expr) = get_expr_with_item(Yacc, &elem);
    }
    break;

  case 154:

/* Line 1806 of yacc.c  */
#line 1779 "parser.y"
    {
        int     idt_len1, idt_len2;

        idt_len1 = sc_strlen((yyvsp[(1) - (3)].strval));
        idt_len2 = sc_strlen((yyvsp[(3) - (3)].strval));

        sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
        elem.elemtype = SPT_VALUE;
        elem.u.value.valueclass     = SVC_CONSTANT;
        elem.u.value.valuetype      = DT_VARCHAR;
        elem.u.value.u.ptr = sql_mem_alloc(vSQL(Yacc)->parsing_memory, 
                idt_len1+idt_len2+2);    // with in NULL

        sc_memcpy(elem.u.value.u.ptr, (yyvsp[(1) - (3)].strval), idt_len1);
        elem.u.value.u.ptr[idt_len1] = '.';
        sc_memcpy(&(elem.u.value.u.ptr[idt_len1+1]), (yyvsp[(3) - (3)].strval), idt_len2);
        elem.u.value.u.ptr[idt_len1+1+idt_len2] = 0x00;

        elem.u.value.attrdesc.len   = idt_len1 + idt_len2 + 1;
        elem.u.value.value_len      = elem.u.value.attrdesc.len;
        (yyval.expr) = get_expr_with_item(Yacc, &elem);
    }
    break;

  case 155:

/* Line 1806 of yacc.c  */
#line 1802 "parser.y"
    {
        int     idt_len1, idt_len2;
        char    primary[8] = "primary";

        idt_len1 = sc_strlen((yyvsp[(1) - (3)].strval));
        idt_len2 = sc_strlen(primary);

        sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
        elem.elemtype = SPT_VALUE;
        elem.u.value.valueclass     = SVC_CONSTANT;
        elem.u.value.valuetype      = DT_VARCHAR;
        elem.u.value.u.ptr = sql_mem_alloc(vSQL(Yacc)->parsing_memory, 
                idt_len1+idt_len2+2);    // with in NULL

        sc_memcpy(elem.u.value.u.ptr, (yyvsp[(1) - (3)].strval), idt_len1);
        elem.u.value.u.ptr[idt_len1] = '.';
        sc_memcpy(&(elem.u.value.u.ptr[idt_len1+1]), primary, idt_len2);
        elem.u.value.u.ptr[idt_len1+1+idt_len2] = 0x00;

        elem.u.value.attrdesc.len   = idt_len1 + idt_len2 + 1;
        elem.u.value.attrdesc.len   = idt_len1 + idt_len2 + 1;
        elem.u.value.value_len      = elem.u.value.attrdesc.len;
        (yyval.expr) = get_expr_with_item(Yacc, &elem);
    }
    break;

  case 156:

/* Line 1806 of yacc.c  */
#line 1830 "parser.y"
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
    break;

  case 157:

/* Line 1806 of yacc.c  */
#line 1842 "parser.y"
    {
                vALTER(Yacc)->type = SAT_ADD;
                vALTER(Yacc)->scope = SAT_CONSTRAINT;
                vALTER(Yacc)->constraint.type = SAT_PRIMARY_KEY;
                vALTER(Yacc)->constraint.list = Yacc->field_list;
                sc_memset(&(Yacc->field_list), 0, sizeof(T_LIST_FIELDS));
            }
    break;

  case 158:

/* Line 1806 of yacc.c  */
#line 1850 "parser.y"
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
    break;

  case 159:

/* Line 1806 of yacc.c  */
#line 1862 "parser.y"
    {
                vALTER(Yacc)->type = SAT_ALTER;
                vALTER(Yacc)->scope = SAT_COLUMN;

                vALTER(Yacc)->column.name_list.list = sql_mem_alloc(vSQL(Yacc)->parsing_memory, sizeof(char *));
                vALTER(Yacc)->column.name_list.list[0] = (yyvsp[(2) - (4)].strval);
                vALTER(Yacc)->column.name_list.max = 1;
                vALTER(Yacc)->column.name_list.len = 1;

                vALTER(Yacc)->column.attr_list.list = sql_mem_alloc(vSQL(Yacc)->parsing_memory, sizeof(T_ATTRDESC));
                sc_memset(vALTER(Yacc)->column.attr_list.list, 0, sizeof(T_ATTRDESC)*1);
                vALTER(Yacc)->column.attr_list.list[0].defvalue.defined = 0;
                vALTER(Yacc)->column.attr_list.list[0].attrname = (yyvsp[(4) - (4)].strval);
                vALTER(Yacc)->column.attr_list.max = 1;
                vALTER(Yacc)->column.attr_list.len = 1;
            }
    break;

  case 160:

/* Line 1806 of yacc.c  */
#line 1879 "parser.y"
    {
                vALTER(Yacc)->type = SAT_ALTER;
                vALTER(Yacc)->scope = SAT_CONSTRAINT;
                vALTER(Yacc)->constraint.type = SAT_PRIMARY_KEY;
                vALTER(Yacc)->constraint.list = Yacc->field_list;
                sc_memset(&(Yacc->field_list), 0, sizeof(T_LIST_FIELDS));
            }
    break;

  case 161:

/* Line 1806 of yacc.c  */
#line 1887 "parser.y"
    {
                vALTER(Yacc)->type = SAT_DROP;
                vALTER(Yacc)->scope = SAT_COLUMN;
                vALTER(Yacc)->column.name_list = Yacc->field_list;
                sc_memset(&(Yacc->field_list), 0, sizeof(T_LIST_FIELDS));
            }
    break;

  case 162:

/* Line 1806 of yacc.c  */
#line 1894 "parser.y"
    {
                vALTER(Yacc)->type = SAT_DROP;
                vALTER(Yacc)->scope = SAT_CONSTRAINT;
                vALTER(Yacc)->constraint.type = SAT_PRIMARY_KEY;
            }
    break;

  case 163:

/* Line 1806 of yacc.c  */
#line 1900 "parser.y"
    {
                vALTER(Yacc)->type = SAT_LOGGING;
                vALTER(Yacc)->logging.on = (yyvsp[(1) - (4)].boolean);
                vALTER(Yacc)->logging.runtime = 1;
            }
    break;

  case 164:

/* Line 1806 of yacc.c  */
#line 1906 "parser.y"
    {
                vALTER(Yacc)->type = SAT_LOGGING;
                vALTER(Yacc)->logging.on = (yyvsp[(1) - (1)].boolean);
                vALTER(Yacc)->logging.runtime = 0;
            }
    break;

  case 165:

/* Line 1806 of yacc.c  */
#line 1912 "parser.y"
    {
                vALTER(Yacc)->type = SAT_MSYNC;
                vALTER(Yacc)->msync_alter_type = (yyvsp[(2) - (2)].boolean);
            }
    break;

  case 166:

/* Line 1806 of yacc.c  */
#line 1920 "parser.y"
    {
        (yyval.boolean) = (yyvsp[(1) - (1)].boolean);
    }
    break;

  case 167:

/* Line 1806 of yacc.c  */
#line 1924 "parser.y"
    {
        (yyval.boolean) = 2;
    }
    break;

  case 168:

/* Line 1806 of yacc.c  */
#line 1931 "parser.y"
    {
                CHECK_ARRAY(Yacc->stmt, &(Yacc->attr_list), sizeof(T_ATTRDESC));
                append(&(Yacc->attr_list), &(Yacc->attr));
                sc_memset(&(Yacc->attr), 0, sizeof(T_ATTRDESC));
            }
    break;

  case 169:

/* Line 1806 of yacc.c  */
#line 1937 "parser.y"
    {
                CHECK_ARRAY(Yacc->stmt, &(Yacc->attr_list), sizeof(T_ATTRDESC));
                append(&(Yacc->attr_list), &(Yacc->attr));
                sc_memset(&(Yacc->attr), 0, sizeof(T_ATTRDESC));
            }
    break;

  case 170:

/* Line 1806 of yacc.c  */
#line 1946 "parser.y"
    {
                CHECK_ARRAY(Yacc->stmt, &(Yacc->field_list), sizeof(char*));
                Yacc->field_list.list[Yacc->field_list.len++] = (yyvsp[(1) - (1)].strval);
            }
    break;

  case 171:

/* Line 1806 of yacc.c  */
#line 1951 "parser.y"
    {
                CHECK_ARRAY(Yacc->stmt, &(Yacc->field_list), sizeof(char*));
                Yacc->field_list.list[Yacc->field_list.len++] = (yyvsp[(3) - (3)].strval);
            }
    break;

  case 172:

/* Line 1806 of yacc.c  */
#line 1959 "parser.y"
    {
                ACTION_drop_statement(Yacc, SDT_TABLE, (yyvsp[(3) - (3)].strval));
            }
    break;

  case 173:

/* Line 1806 of yacc.c  */
#line 1963 "parser.y"
    {
                ACTION_drop_statement(Yacc, SDT_INDEX, (yyvsp[(3) - (3)].strval));
            }
    break;

  case 174:

/* Line 1806 of yacc.c  */
#line 1967 "parser.y"
    {
                ACTION_drop_statement(Yacc, SDT_VIEW, (yyvsp[(3) - (3)].strval));
            }
    break;

  case 175:

/* Line 1806 of yacc.c  */
#line 1971 "parser.y"
    {
                ACTION_drop_statement(Yacc, SDT_SEQUENCE, (yyvsp[(3) - (3)].strval));
            }
    break;

  case 176:

/* Line 1806 of yacc.c  */
#line 1975 "parser.y"
    {
                OID *rid = sql_malloc(OID_SIZE, 0);

                *rid = (OID)(yyvsp[(3) - (3)].intval);

                ACTION_drop_statement(Yacc, SDT_RID, (char*)rid);
            }
    break;

  case 177:

/* Line 1806 of yacc.c  */
#line 1986 "parser.y"
    {
                vSQL(Yacc)->sqltype    = ST_RENAME;
                vRENAME(Yacc)        = &vSQL(Yacc)->u._rename;
                vRENAME(Yacc)->type = SRT_TABLE;
                vRENAME(Yacc)->oldname    = (yyvsp[(3) - (5)].strval);
                vRENAME(Yacc)->newname    = (yyvsp[(5) - (5)].strval);
            }
    break;

  case 178:

/* Line 1806 of yacc.c  */
#line 1994 "parser.y"
    {
                vSQL(Yacc)->sqltype    = ST_RENAME;
                vRENAME(Yacc)        = &vSQL(Yacc)->u._rename;
                vRENAME(Yacc)->type = SRT_INDEX;
                vRENAME(Yacc)->oldname    = (yyvsp[(3) - (5)].strval);
                vRENAME(Yacc)->newname    = (yyvsp[(5) - (5)].strval);
            }
    break;

  case 179:

/* Line 1806 of yacc.c  */
#line 2005 "parser.y"
    {
                vSQL(Yacc)->sqltype = ST_COMMIT;
            }
    break;

  case 180:

/* Line 1806 of yacc.c  */
#line 2009 "parser.y"
    {
                vSQL(Yacc)->sqltype = ST_COMMIT;
            }
    break;

  case 181:

/* Line 1806 of yacc.c  */
#line 2013 "parser.y"
    {
                vSQL(Yacc)->sqltype = ST_COMMIT_FLUSH;
            }
    break;

  case 182:

/* Line 1806 of yacc.c  */
#line 2020 "parser.y"
    {
                vSQL(Yacc)->sqltype = ST_DELETE;
                vDELETE(Yacc) = &vSQL(Yacc)->u._delete;
                vSELECT(Yacc)       = &vSQL(Yacc)->u._delete.subselect;
                vSELECT(Yacc)->born_stmt = vSQL(Yacc);
                init_limit(&vSELECT(Yacc)->planquerydesc.querydesc.limit);

                Yacc->userdefined = 0;
                Yacc->param_count = 0;
            }
    break;

  case 183:

/* Line 1806 of yacc.c  */
#line 2031 "parser.y"
    {
                init_limit(&vDELETE(Yacc)->planquerydesc.querydesc.limit);
            }
    break;

  case 184:

/* Line 1806 of yacc.c  */
#line 2035 "parser.y"
    {
                T_LIST_JOINTABLEDESC *ltable;

                ltable = &vDELETE(Yacc)->planquerydesc.querydesc.fromlist;
                CHECK_ARRAY_BY_LIMIT(Yacc->stmt, ltable,
                        sizeof(T_JOINTABLEDESC), MAX_JOIN_TABLE_NUM);
                ltable->list[ltable->len].join_type     = SJT_NONE;
                ltable->list[ltable->len].condition.len = 0;
                ltable->list[ltable->len].condition.max = 0;
                ltable->list[ltable->len].condition.list= NULL;
                ltable->list[ltable->len].table         = (yyvsp[(4) - (8)].table);
                ltable->list[ltable->len].scan_hint     = (yyvsp[(5) - (8)].scanhint);
                ++ltable->len;
                vDELETE(Yacc)->param_count = Yacc->param_count;
            }
    break;

  case 185:

/* Line 1806 of yacc.c  */
#line 2054 "parser.y"
    {
                vSQL(Yacc)->sqltype = ST_INSERT;
                vINSERT(Yacc) = &vSQL(Yacc)->u._insert;

                Yacc->userdefined = 0;
                Yacc->param_count = 0;
            }
    break;

  case 186:

/* Line 1806 of yacc.c  */
#line 2062 "parser.y"
    {
                vINSERT(Yacc)->table = (yyvsp[(4) - (6)].table);

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
    break;

  case 191:

/* Line 1806 of yacc.c  */
#line 2097 "parser.y"
    {
                vINSERT(Yacc)->type = SIT_VALUES;
            }
    break;

  case 192:

/* Line 1806 of yacc.c  */
#line 2101 "parser.y"
    {
                vINSERT(Yacc)->type = SIT_QUERY;
            }
    break;

  case 193:

/* Line 1806 of yacc.c  */
#line 2108 "parser.y"
    {
                CHECK_ARRAY(Yacc->stmt,
                        &(Yacc->val_list), sizeof(T_EXPRESSIONDESC));
                expr = &(Yacc->val_list.list[Yacc->val_list.len++]);
                sc_memcpy(expr, (yyvsp[(1) - (1)].expr), sizeof(T_EXPRESSIONDESC));
            }
    break;

  case 194:

/* Line 1806 of yacc.c  */
#line 2115 "parser.y"
    {
                CHECK_ARRAY(Yacc->stmt,
                        &(Yacc->val_list), sizeof(T_EXPRESSIONDESC));
                expr = &(Yacc->val_list.list[Yacc->val_list.len++]);
                sc_memcpy(expr, (yyvsp[(3) - (3)].expr), sizeof(T_EXPRESSIONDESC));
            }
    break;

  case 195:

/* Line 1806 of yacc.c  */
#line 2125 "parser.y"
    {
        (yyval.boolean) = 0;
    }
    break;

  case 196:

/* Line 1806 of yacc.c  */
#line 2129 "parser.y"
    {
        (yyval.boolean) = 1;
    }
    break;

  case 197:

/* Line 1806 of yacc.c  */
#line 2135 "parser.y"
    {
                vSQL(Yacc)->sqltype = ST_UPSERT;
                vINSERT(Yacc) = &vSQL(Yacc)->u._insert;

                Yacc->userdefined = 0;
                Yacc->param_count = 0;
            }
    break;

  case 198:

/* Line 1806 of yacc.c  */
#line 2143 "parser.y"
    {
                vINSERT(Yacc)->table = (yyvsp[(4) - (10)].table);

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
                vINSERT(Yacc)->is_upsert_msync = (yyvsp[(10) - (10)].boolean);
            }
    break;

  case 199:

/* Line 1806 of yacc.c  */
#line 2168 "parser.y"
    {
                vSQL(Yacc)->sqltype = ST_ROLLBACK;
            }
    break;

  case 200:

/* Line 1806 of yacc.c  */
#line 2172 "parser.y"
    {
                vSQL(Yacc)->sqltype = ST_ROLLBACK;
            }
    break;

  case 201:

/* Line 1806 of yacc.c  */
#line 2176 "parser.y"
    {
                vSQL(Yacc)->sqltype = ST_ROLLBACK_FLUSH;
            }
    break;

  case 204:

/* Line 1806 of yacc.c  */
#line 2188 "parser.y"
    {
        T_EXPRESSIONDESC *setlist;
        T_QUERYDESC *qdesc;

        setlist = (yyvsp[(1) - (1)].expr);

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
    break;

  case 205:

/* Line 1806 of yacc.c  */
#line 2225 "parser.y"
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
    break;

  case 206:

/* Line 1806 of yacc.c  */
#line 2333 "parser.y"
    {
        if ((yyvsp[(6) - (6)].num))
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

        vSELECT(Yacc)->msync_flag = (yyvsp[(6) - (6)].num);

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
    break;

  case 207:

/* Line 1806 of yacc.c  */
#line 2365 "parser.y"
    {
        (yyval.num) = 0;
    }
    break;

  case 208:

/* Line 1806 of yacc.c  */
#line 2369 "parser.y"
    {
        (yyval.num) = SYNCED_SLOT;
    }
    break;

  case 209:

/* Line 1806 of yacc.c  */
#line 2373 "parser.y"
    {
        (yyval.num) = USED_SLOT;
    }
    break;

  case 210:

/* Line 1806 of yacc.c  */
#line 2377 "parser.y"
    {
        (yyval.num) = UPDATE_SLOT;
    }
    break;

  case 211:

/* Line 1806 of yacc.c  */
#line 2381 "parser.y"
    {
        (yyval.num) = DELETE_SLOT;
    }
    break;

  case 212:

/* Line 1806 of yacc.c  */
#line 2388 "parser.y"
    {
        sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
        elem.elemtype = SPT_SUBQ_OP;
        elem.u.op.type = (yyvsp[(2) - (4)].subtok).type;
        if ((yyvsp[(3) - (4)].boolean))
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

        (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (4)].expr), (yyvsp[(4) - (4)].expr), &elem);
    }
    break;

  case 213:

/* Line 1806 of yacc.c  */
#line 2408 "parser.y"
    {
        (yyval.expr) = (yyvsp[(2) - (5)].expr);
    }
    break;

  case 214:

/* Line 1806 of yacc.c  */
#line 2414 "parser.y"
    {
        (yyval.boolean) = 0;
    }
    break;

  case 215:

/* Line 1806 of yacc.c  */
#line 2418 "parser.y"
    {
        (yyval.boolean) = 1;
    }
    break;

  case 216:

/* Line 1806 of yacc.c  */
#line 2425 "parser.y"
    {
        (yyval.expr) = (yyvsp[(1) - (1)].expr);
    }
    break;

  case 217:

/* Line 1806 of yacc.c  */
#line 2429 "parser.y"
    {
        (yyval.expr) = (yyvsp[(2) - (3)].expr);
    }
    break;

  case 218:

/* Line 1806 of yacc.c  */
#line 2433 "parser.y"
    {
        (yyval.expr) = (yyvsp[(1) - (1)].expr);
    }
    break;

  case 219:

/* Line 1806 of yacc.c  */
#line 2440 "parser.y"
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

        (yyval.expr) = get_expr_with_item(Yacc, &elem);
    }
    break;

  case 220:

/* Line 1806 of yacc.c  */
#line 2517 "parser.y"
    {
                (yyval.strval) = NULL;
            }
    break;

  case 221:

/* Line 1806 of yacc.c  */
#line 2521 "parser.y"
    {
                (yyval.strval) = (yyvsp[(1) - (1)].strval);
            }
    break;

  case 222:

/* Line 1806 of yacc.c  */
#line 2528 "parser.y"
    {
                vPLAN(Yacc)->querydesc.is_distinct = 0;
            }
    break;

  case 223:

/* Line 1806 of yacc.c  */
#line 2532 "parser.y"
    {
                vPLAN(Yacc)->querydesc.is_distinct = 0;
            }
    break;

  case 224:

/* Line 1806 of yacc.c  */
#line 2536 "parser.y"
    {
                vPLAN(Yacc)->querydesc.is_distinct = 1;
            }
    break;

  case 225:

/* Line 1806 of yacc.c  */
#line 2543 "parser.y"
    {
                vPLAN(Yacc)->querydesc.selection = Yacc->selection;
                sc_memset(&(Yacc->selection), 0, sizeof(T_LIST_SELECTION));
                vPLAN(Yacc)->querydesc.select_star = 0;
            }
    break;

  case 226:

/* Line 1806 of yacc.c  */
#line 2549 "parser.y"
    {
                sc_memset(&(vPLAN(Yacc)->querydesc.selection), 0,
                            sizeof(T_LIST_SELECTION));

                vPLAN(Yacc)->querydesc.select_star = 1;
            }
    break;

  case 227:

/* Line 1806 of yacc.c  */
#line 2559 "parser.y"
    {
                CHECK_ARRAY(Yacc->stmt, &(Yacc->selection),
                        sizeof(T_SELECTION));

                if ((yyvsp[(2) - (2)].strval))
                {
                    sc_strncpy(
                            Yacc->selection.list[Yacc->selection.len].res_name,
                            (yyvsp[(2) - (2)].strval), FIELD_NAME_LENG-1);
                }
                else
                    Yacc->selection.list[Yacc->selection.len].res_name[0] =
                        '\0';
                Yacc->selection.list[Yacc->selection.len].res_name[
                    FIELD_NAME_LENG-1] = '\0';
                expr = &(Yacc->selection.list[Yacc->selection.len].expr);
                if ((yyvsp[(1) - (2)].expr)) sc_memcpy(expr, (yyvsp[(1) - (2)].expr), sizeof(T_EXPRESSIONDESC));
                else {
                    expr->len  = 0;
                    expr->max  = 0;
                    expr->list = NULL;
                }

                Yacc->selection.len++;
            }
    break;

  case 228:

/* Line 1806 of yacc.c  */
#line 2585 "parser.y"
    {
                if (vPLAN(Yacc)->querydesc.selection.len > 0)
                {
                    Yacc->selection = vPLAN(Yacc)->querydesc.selection;
                    sc_memset(&(vPLAN(Yacc)->querydesc.selection),
                            0, sizeof(T_LIST_SELECTION));
                }

                CHECK_ARRAY(Yacc->stmt, &(Yacc->selection),
                        sizeof(T_SELECTION));
                if ((yyvsp[(4) - (4)].strval))
                {
                    sc_strncpy(
                            Yacc->selection.list[Yacc->selection.len].res_name,
                            (yyvsp[(4) - (4)].strval), FIELD_NAME_LENG-1);
                }
                else
                    Yacc->selection.list[Yacc->selection.len].res_name[0] =
                        '\0';
                Yacc->selection.list[Yacc->selection.len].res_name[
                    FIELD_NAME_LENG-1] = '\0';
                expr = &(Yacc->selection.list[Yacc->selection.len].expr);
                if ((yyvsp[(3) - (4)].expr)) sc_memcpy(expr, (yyvsp[(3) - (4)].expr), sizeof(T_EXPRESSIONDESC));
                else {
                    expr->len  = 0;
                    expr->max  = 0;
                    expr->list = NULL;
                }

                Yacc->selection.len++;
            }
    break;

  case 229:

/* Line 1806 of yacc.c  */
#line 2620 "parser.y"
    {
                (yyval.strval) = NULL;
            }
    break;

  case 230:

/* Line 1806 of yacc.c  */
#line 2624 "parser.y"
    {
                __CHECK_ALIAS_NAME_LENG((yyvsp[(2) - (2)].strval));
                (yyval.strval) = (yyvsp[(2) - (2)].strval);
            }
    break;

  case 231:

/* Line 1806 of yacc.c  */
#line 2629 "parser.y"
    {
                (yyval.strval) = (yyvsp[(2) - (2)].strval);
            }
    break;

  case 232:

/* Line 1806 of yacc.c  */
#line 2636 "parser.y"
    {
            vSQL(Yacc)->sqltype = ST_UPDATE;
            vUPDATE(Yacc)       = &vSQL(Yacc)->u._update;
            vSELECT(Yacc)            = &vSQL(Yacc)->u._update.subselect;
            vSELECT(Yacc)->born_stmt = vSQL(Yacc);
            init_limit(&vSELECT(Yacc)->planquerydesc.querydesc.limit);
            Yacc->userdefined = 0;
            Yacc->param_count = 0;
        }
    break;

  case 233:

/* Line 1806 of yacc.c  */
#line 2646 "parser.y"
    {
            vUPDATE(Yacc)->param_count = Yacc->param_count;
            vUPDATE(Yacc)->columns     = Yacc->coldesc_list;
            vUPDATE(Yacc)->values      = Yacc->val_list;
            sc_memset(&(Yacc->coldesc_list), 0, sizeof(T_LIST_COLDESC));
            sc_memset(&(Yacc->val_list), 0, sizeof(T_LIST_VALUE));
        }
    break;

  case 234:

/* Line 1806 of yacc.c  */
#line 2657 "parser.y"
    {
            init_limit(&vUPDATE(Yacc)->planquerydesc.querydesc.limit);
        }
    break;

  case 235:

/* Line 1806 of yacc.c  */
#line 2661 "parser.y"
    {
            T_LIST_JOINTABLEDESC *ltable;

            ltable = &vUPDATE(Yacc)->planquerydesc.querydesc.fromlist;
            CHECK_ARRAY_BY_LIMIT(Yacc->stmt, ltable,
                    sizeof(T_JOINTABLEDESC), MAX_JOIN_TABLE_NUM);
            ltable->list[ltable->len].join_type     = SJT_NONE;
            ltable->list[ltable->len].condition.len = 0;
            ltable->list[ltable->len].condition.max = 0;
            ltable->list[ltable->len].condition.list= NULL;
            ltable->list[ltable->len].table         = (yyvsp[(1) - (7)].table);
            ltable->list[ltable->len].scan_hint     = (yyvsp[(2) - (7)].scanhint);
            ++ltable->len;
            vUPDATE(Yacc)->rid   = NULL_OID;
        }
    break;

  case 236:

/* Line 1806 of yacc.c  */
#line 2677 "parser.y"
    {
            vUPDATE(Yacc)->rid   = (OID)(yyvsp[(2) - (4)].intval);
        }
    break;

  case 239:

/* Line 1806 of yacc.c  */
#line 2689 "parser.y"
    {
            T_COLDESC *column;

            if ((yyvsp[(2) - (3)].subtok).type != SOT_EQ) __yyerror("ERROR");
            else {
                if ((yyvsp[(3) - (3)].expr)->list[0]->elemtype == SPT_SUBQUERY)
                {
                    T_QUERYDESC * qdesc;
                    qdesc = &(yyvsp[(3) - (3)].expr)->list[0]->u.subq->planquerydesc.querydesc;
                    if (qdesc->selection.len > 1)
                    {
                        __yyerror("too many values");
                    }
                }

                CHECK_ARRAY(Yacc->stmt, &(Yacc->coldesc_list),
                        sizeof(T_COLDESC));
                column = &(Yacc->coldesc_list.list[Yacc->coldesc_list.len++]);
                column->name = (yyvsp[(1) - (3)].strval);
                column->fieldinfo = NULL;

                CHECK_ARRAY(Yacc->stmt,
                        &(Yacc->val_list), sizeof(T_EXPRESSIONDESC));
                expr = &(Yacc->val_list.list[Yacc->val_list.len++]);
                sc_memcpy(expr, (yyvsp[(3) - (3)].expr), sizeof(T_EXPRESSIONDESC));
            }
        }
    break;

  case 240:

/* Line 1806 of yacc.c  */
#line 2720 "parser.y"
    {
                vSQL(Yacc)->sqltype = ST_TRUNCATE;
                vTRUNCATE(Yacc) = &vSQL(Yacc)->u._truncate;
                vTRUNCATE(Yacc)->objectname = (yyvsp[(2) - (2)].strval);
            }
    break;

  case 241:

/* Line 1806 of yacc.c  */
#line 2726 "parser.y"
    {
                vSQL(Yacc)->sqltype = ST_TRUNCATE;
                vTRUNCATE(Yacc) = &vSQL(Yacc)->u._truncate;
                vTRUNCATE(Yacc)->objectname = (yyvsp[(3) - (3)].strval);
            }
    break;

  case 242:

/* Line 1806 of yacc.c  */
#line 2735 "parser.y"
    {
                vSQL(Yacc)->sqltype = ST_DESCRIBE;
                vDESCRIBE(Yacc) = &vSQL(Yacc)->u._desc;
                vDESCRIBE(Yacc)->type = SBT_DESC_TABLE;
                Yacc->userdefined = 0;
                Yacc->param_count = 0;
            }
    break;

  case 243:

/* Line 1806 of yacc.c  */
#line 2744 "parser.y"
    {
                vSQL(Yacc)->sqltype = ST_DESCRIBE;
                vDESCRIBE(Yacc) = &vSQL(Yacc)->u._desc;
                vDESCRIBE(Yacc)->type = SBT_DESC_TABLE;
                Yacc->userdefined = 0;
            }
    break;

  case 244:

/* Line 1806 of yacc.c  */
#line 2752 "parser.y"
    {
                vSQL(Yacc)->sqltype = ST_DESCRIBE;
                vDESCRIBE(Yacc) = &vSQL(Yacc)->u._desc;
                vDESCRIBE(Yacc)->type = SBT_DESC_TABLE;
                Yacc->userdefined = 0;
            }
    break;

  case 245:

/* Line 1806 of yacc.c  */
#line 2759 "parser.y"
    {
                vDESCRIBE(Yacc)->describe_table  = (yyvsp[(3) - (4)].strval);
                vDESCRIBE(Yacc)->describe_column = (yyvsp[(4) - (4)].strval);
            }
    break;

  case 246:

/* Line 1806 of yacc.c  */
#line 2765 "parser.y"
    {
                vSQL(Yacc)->sqltype = ST_DESCRIBE;
                vDESCRIBE(Yacc) = &vSQL(Yacc)->u._desc;
                vDESCRIBE(Yacc)->type = SBT_DESC_TABLE;
                Yacc->userdefined = 0;
            }
    break;

  case 247:

/* Line 1806 of yacc.c  */
#line 2772 "parser.y"
    {
                vDESCRIBE(Yacc)->describe_table  = (yyvsp[(3) - (4)].strval);
                vDESCRIBE(Yacc)->describe_column = (yyvsp[(4) - (4)].strval);
            }
    break;

  case 248:

/* Line 1806 of yacc.c  */
#line 2778 "parser.y"
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
    break;

  case 249:

/* Line 1806 of yacc.c  */
#line 2790 "parser.y"
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
    break;

  case 250:

/* Line 1806 of yacc.c  */
#line 2801 "parser.y"
    {
                vSQL(Yacc)->sqltype = ST_DESCRIBE;
                vDESCRIBE(Yacc) = &vSQL(Yacc)->u._desc;
                vDESCRIBE(Yacc)->type = SBT_SHOW_TABLE;
                Yacc->userdefined = 0;
                vDESCRIBE(Yacc)->describe_table  = (yyvsp[(3) - (3)].strval);
            }
    break;

  case 251:

/* Line 1806 of yacc.c  */
#line 2809 "parser.y"
    {
                vSQL(Yacc)->sqltype = ST_ADMIN;
                vADMIN(Yacc) = &vSQL(Yacc)->u._admin;
                Yacc->userdefined = 0;
                Yacc->param_count = 0;

                vADMIN(Yacc)->u.export.data_or_schema = 1;
                if ((yyvsp[(3) - (6)].strval) == NULL)
                    vADMIN(Yacc)->type  = SADM_EXPORT_ALL;
                else
                    vADMIN(Yacc)->type  = SADM_EXPORT_ONE;

                vADMIN(Yacc)->u.export.table_name   = (yyvsp[(3) - (6)].strval);
                vADMIN(Yacc)->u.export.export_file  = (yyvsp[(5) - (6)].strval);
                vADMIN(Yacc)->u.export.f_append     = (yyvsp[(6) - (6)].intval);
            }
    break;

  case 252:

/* Line 1806 of yacc.c  */
#line 2828 "parser.y"
    { (yyval.boolean) = 1; }
    break;

  case 253:

/* Line 1806 of yacc.c  */
#line 2829 "parser.y"
    { (yyval.boolean) = 0; }
    break;

  case 254:

/* Line 1806 of yacc.c  */
#line 2834 "parser.y"
    {
                vSET(Yacc)->type     = SST_AUTOCOMMIT;
                vSET(Yacc)->u.on_off = (yyvsp[(2) - (2)].boolean);
            }
    break;

  case 255:

/* Line 1806 of yacc.c  */
#line 2839 "parser.y"
    {
                vSET(Yacc)->type     = SST_TIME;
                vSET(Yacc)->u.on_off = (yyvsp[(2) - (2)].boolean);
            }
    break;

  case 256:

/* Line 1806 of yacc.c  */
#line 2844 "parser.y"
    {
                vSET(Yacc)->type     = SST_HEADING;
                vSET(Yacc)->u.on_off = (yyvsp[(2) - (2)].boolean);
            }
    break;

  case 257:

/* Line 1806 of yacc.c  */
#line 2849 "parser.y"
    {
                vSET(Yacc)->type     = SST_FEEDBACK;
                vSET(Yacc)->u.on_off = (yyvsp[(2) - (2)].boolean);
            }
    break;

  case 258:

/* Line 1806 of yacc.c  */
#line 2854 "parser.y"
    {
                vSET(Yacc)->type     = SST_RECONNECT;
                vSET(Yacc)->u.on_off = (yyvsp[(2) - (2)].boolean);
            }
    break;

  case 259:

/* Line 1806 of yacc.c  */
#line 2859 "parser.y"
    {
                vSET(Yacc)->type     = ((yyvsp[(3) - (3)].boolean) ? SST_PLAN_ON: SST_PLAN_OFF);
                vSET(Yacc)->u.on_off = 1;
            }
    break;

  case 260:

/* Line 1806 of yacc.c  */
#line 2864 "parser.y"
    {
                vSET(Yacc)->type     = SST_PLAN_ONLY;
                vSET(Yacc)->u.on_off = 1;
            }
    break;

  case 261:

/* Line 1806 of yacc.c  */
#line 2872 "parser.y"
    {
                vSQL(Yacc)->sqltype = ST_SET;
                vSET(Yacc) = &vSQL(Yacc)->u._set;
                Yacc->userdefined = 0;
            }
    break;

  case 263:

/* Line 1806 of yacc.c  */
#line 2880 "parser.y"
    {
        }
    break;

  case 264:

/* Line 1806 of yacc.c  */
#line 2883 "parser.y"
    {
        }
    break;

  case 265:

/* Line 1806 of yacc.c  */
#line 2889 "parser.y"
    { (yyval.strval) = NULL; }
    break;

  case 266:

/* Line 1806 of yacc.c  */
#line 2891 "parser.y"
    { (yyval.strval) = (yyvsp[(1) - (1)].strval); }
    break;

  case 267:

/* Line 1806 of yacc.c  */
#line 2893 "parser.y"
    { 
                __CHECK_FIELD_NAME_LENG((yyvsp[(1) - (1)].strval));
                (yyval.strval) = (yyvsp[(1) - (1)].strval);
            }
    break;

  case 268:

/* Line 1806 of yacc.c  */
#line 2906 "parser.y"
    {
            init_limit(&vSELECT(Yacc)->planquerydesc.querydesc.limit);
        }
    break;

  case 269:

/* Line 1806 of yacc.c  */
#line 2910 "parser.y"
    {
            if (vPLAN(Yacc)->querydesc.groupby.len && 
                vPLAN(Yacc)->querydesc.limit.start_at != NULL_OID)
            {
                __yyerror("limit @rid not allowed with group by.");
            }
        }
    break;

  case 270:

/* Line 1806 of yacc.c  */
#line 2921 "parser.y"
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
    break;

  case 272:

/* Line 1806 of yacc.c  */
#line 2945 "parser.y"
    {
                T_LIST_JOINTABLEDESC *ltable;

                ltable = &vPLAN(Yacc)->querydesc.fromlist;
                CHECK_ARRAY_BY_LIMIT(Yacc->stmt, ltable,
                        sizeof(T_JOINTABLEDESC), MAX_JOIN_TABLE_NUM);
                ltable->list[ltable->len].join_type     = SJT_NONE;
                ltable->list[ltable->len].condition.len = 0;
                ltable->list[ltable->len].condition.max = 0;
                ltable->list[ltable->len].condition.list= NULL;
                ltable->list[ltable->len].table         = (yyvsp[(1) - (2)].table);
                ltable->list[ltable->len].scan_hint     = (yyvsp[(2) - (2)].scanhint);
                ++ltable->len;
            }
    break;

  case 273:

/* Line 1806 of yacc.c  */
#line 2960 "parser.y"
    {
                T_LIST_JOINTABLEDESC *ltable;

                ltable = &vPLAN(Yacc)->querydesc.fromlist;
                CHECK_ARRAY_BY_LIMIT(Yacc->stmt, ltable,
                        sizeof(T_JOINTABLEDESC), MAX_JOIN_TABLE_NUM);
                ltable->list[ltable->len].join_type     = SJT_CROSS_JOIN;
                ltable->list[ltable->len].condition.len = 0;
                ltable->list[ltable->len].condition.max = 0;
                ltable->list[ltable->len].condition.list= NULL;
                ltable->list[ltable->len].table         = (yyvsp[(3) - (4)].table);
                ltable->list[ltable->len].scan_hint     = (yyvsp[(4) - (4)].scanhint);
                ++ltable->len;
            }
    break;

  case 274:

/* Line 1806 of yacc.c  */
#line 2975 "parser.y"
    {
                T_LIST_JOINTABLEDESC *ltable;

                ltable = &vPLAN(Yacc)->querydesc.fromlist;
                CHECK_ARRAY_BY_LIMIT(Yacc->stmt, ltable,
                        sizeof(T_JOINTABLEDESC), MAX_JOIN_TABLE_NUM);
                ltable->list[ltable->len].join_type = SJT_INNER_JOIN;
                sc_memcpy(&(ltable->list[ltable->len].condition),
                        (yyvsp[(7) - (7)].expr), sizeof(T_EXPRESSIONDESC));
                ltable->list[ltable->len].table     = (yyvsp[(4) - (7)].table);
                ltable->list[ltable->len].scan_hint = (yyvsp[(5) - (7)].scanhint);
                ++ltable->len;
            }
    break;

  case 275:

/* Line 1806 of yacc.c  */
#line 2989 "parser.y"
    {
                T_LIST_JOINTABLEDESC *ltable;

                ltable = &vPLAN(Yacc)->querydesc.fromlist;
                CHECK_ARRAY_BY_LIMIT(Yacc->stmt, ltable,
                        sizeof(T_JOINTABLEDESC), MAX_JOIN_TABLE_NUM);
                ltable->outer_join_exists = 1;
                ltable->list[ltable->len].join_type = SJT_LEFT_OUTER_JOIN;
                sc_memcpy(&(ltable->list[ltable->len].condition),
                            (yyvsp[(8) - (8)].expr), sizeof(T_EXPRESSIONDESC));
                ltable->list[ltable->len].table     = (yyvsp[(5) - (8)].table);
                ltable->list[ltable->len].scan_hint = (yyvsp[(6) - (8)].scanhint);
                ++ltable->len;
            }
    break;

  case 276:

/* Line 1806 of yacc.c  */
#line 3004 "parser.y"
    {
                T_LIST_JOINTABLEDESC *ltable;

                INCOMPLETION("RIGHT OUTER JOIN");

                ltable = &vPLAN(Yacc)->querydesc.fromlist;
                CHECK_ARRAY_BY_LIMIT(Yacc->stmt, ltable,
                        sizeof(T_JOINTABLEDESC), MAX_JOIN_TABLE_NUM);
                ltable->outer_join_exists = 1;
                ltable->list[ltable->len].join_type = SJT_RIGHT_OUTER_JOIN;
                sc_memcpy(&(ltable->list[ltable->len].condition),
                        (yyvsp[(8) - (8)].expr), sizeof(T_EXPRESSIONDESC));
                ltable->list[ltable->len].table     = (yyvsp[(5) - (8)].table);
                ltable->list[ltable->len].scan_hint = (yyvsp[(6) - (8)].scanhint);
                ++ltable->len;
            }
    break;

  case 277:

/* Line 1806 of yacc.c  */
#line 3021 "parser.y"
    {
                INCOMPLETION("FULL OUTER JOIN");
            }
    break;

  case 284:

/* Line 1806 of yacc.c  */
#line 3040 "parser.y"
    {
                (yyval.table).tablename = (yyvsp[(1) - (1)].strval);
                (yyval.table).aliasname = NULL;
                (yyval.table).correlated_tbl = NULL;
            }
    break;

  case 285:

/* Line 1806 of yacc.c  */
#line 3046 "parser.y"
    {
                (yyval.table).tablename = (yyvsp[(1) - (3)].strval);
                (yyval.table).aliasname = (yyvsp[(3) - (3)].strval);
                (yyval.table).correlated_tbl = NULL;
            }
    break;

  case 286:

/* Line 1806 of yacc.c  */
#line 3052 "parser.y"
    {
                T_SELECT *current_tst;

                (yyval.table).tablename = (yyvsp[(5) - (6)].strval);
                (yyval.table).aliasname = NULL;
                (yyval.table).correlated_tbl = Yacc->_select;
                /* kindofwTable의 용도에 맞는 사용인가 ? 몰러 .. */
                (yyval.table).correlated_tbl->kindofwTable = iTABLE_SUBSELECT;

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
                    current_tst->first_sub = (yyval.table).correlated_tbl;
                } else {
                    current_tst = current_tst->first_sub;
                    /* insert into the last position. how about the first ? */
                    while(current_tst->sibling_query)
                        current_tst = current_tst->sibling_query;
                    current_tst->sibling_query = (yyval.table).correlated_tbl;
                }
            }
    break;

  case 289:

/* Line 1806 of yacc.c  */
#line 3098 "parser.y"
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

            __CHECK_FIELD_NAME_LENG((yyvsp[(1) - (1)].strval));
            sc_strncpy(vSELECT(Yacc)->queryresult.list[idx].res_name, (yyvsp[(1) - (1)].strval),
                    FIELD_NAME_LENG-1);
            vSELECT(Yacc)->queryresult.list[idx].res_name[FIELD_NAME_LENG-1] =
                '\0';

            ++vSELECT(Yacc)->queryresult.len;
        }
    break;

  case 290:

/* Line 1806 of yacc.c  */
#line 3132 "parser.y"
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

            __CHECK_FIELD_NAME_LENG((yyvsp[(3) - (3)].strval));
            sc_strncpy(vSELECT(Yacc)->queryresult.list[idx].res_name, (yyvsp[(3) - (3)].strval),
                    FIELD_NAME_LENG-1);
            vSELECT(Yacc)->queryresult.list[idx].res_name[FIELD_NAME_LENG-1] =
                '\0';

            ++vSELECT(Yacc)->queryresult.len;
        }
    break;

  case 291:

/* Line 1806 of yacc.c  */
#line 3162 "parser.y"
    {
                __CHECK_ALIAS_NAME_LENG((yyvsp[(1) - (1)].strval));
                (yyval.strval) = (yyvsp[(1) - (1)].strval);
            }
    break;

  case 292:

/* Line 1806 of yacc.c  */
#line 3170 "parser.y"
    {
                (yyval.scanhint).scan_type = SMT_NONE;
                (yyval.scanhint).indexname = NULL;
                (yyval.scanhint).fields.len = 0;
                (yyval.scanhint).fields.max = 0;
                (yyval.scanhint).fields.list = NULL;
            }
    break;

  case 293:

/* Line 1806 of yacc.c  */
#line 3178 "parser.y"
    {
                (yyval.scanhint).scan_type = SMT_SSCAN;
                (yyval.scanhint).indexname = NULL;
                (yyval.scanhint).fields.len = 0;
                (yyval.scanhint).fields.max = 0;
                (yyval.scanhint).fields.list = NULL;
            }
    break;

  case 294:

/* Line 1806 of yacc.c  */
#line 3186 "parser.y"
    {
                (yyval.scanhint).scan_type = SMT_ISCAN;
                (yyval.scanhint).indexname = NULL;
                (yyval.scanhint).fields.len = 0;
                (yyval.scanhint).fields.max = 0;
                (yyval.scanhint).fields.list = NULL;
            }
    break;

  case 295:

/* Line 1806 of yacc.c  */
#line 3194 "parser.y"
    {
                (yyval.scanhint).scan_type = SMT_ISCAN;
                (yyval.scanhint).indexname = (yyvsp[(2) - (2)].strval);
                (yyval.scanhint).fields.len = 0;
                (yyval.scanhint).fields.max = 0;
                (yyval.scanhint).fields.list = NULL;
            }
    break;

  case 296:

/* Line 1806 of yacc.c  */
#line 3202 "parser.y"
    {
                (yyval.scanhint).scan_type = SMT_ISCAN;
                (yyval.scanhint).indexname = NULL;
                (yyval.scanhint).fields.len = 0;
                (yyval.scanhint).fields.max = 0;
                (yyval.scanhint).fields.list = NULL;
                (yyval.scanhint).fields = (yyvsp[(3) - (4)].idxfields);
            }
    break;

  case 297:

/* Line 1806 of yacc.c  */
#line 3215 "parser.y"
    {
                /* sc_memset(&$$, 0, sizeof(T_LIST_FIELDS)); */
                sc_memset(&(yyval.idxfields), 0, sizeof(T_IDX_LIST_FIELDS));
                /* CHECK_ARRAY(Yacc->stmt, &$$, sizeof(char*)); */
                CHECK_ARRAY(Yacc->stmt, &(yyval.idxfields), sizeof(T_IDX_FIELD));
                (yyval.idxfields).list[(yyval.idxfields).len++] = (yyvsp[(1) - (1)].idxfield);
            }
    break;

  case 298:

/* Line 1806 of yacc.c  */
#line 3224 "parser.y"
    {
                /* CHECK_ARRAY(Yacc->stmt, &$$, sizeof(char*)); */
                CHECK_ARRAY(Yacc->stmt, &(yyval.idxfields), sizeof(T_IDX_FIELD));
                (yyval.idxfields).list[(yyval.idxfields).len++] = (yyvsp[(3) - (3)].idxfield);
            }
    break;

  case 299:

/* Line 1806 of yacc.c  */
#line 3233 "parser.y"
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
    break;

  case 300:

/* Line 1806 of yacc.c  */
#line 3263 "parser.y"
    {
                switch (vSQL(Yacc)->sqltype) {
                    case ST_DELETE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vDELETE(Yacc)->subselect)
                            sc_memcpy(&vPLAN(Yacc)->querydesc.condition,
                                (yyvsp[(1) - (1)].expr), sizeof(T_EXPRESSIONDESC));
                        else
                        sc_memcpy(&vDELETE(Yacc)->planquerydesc.querydesc.condition,
                                (yyvsp[(1) - (1)].expr), sizeof(T_EXPRESSIONDESC));
                        break;
                    case ST_SELECT:
                    case ST_CREATE: /* for view creation */
                        sc_memcpy(&vPLAN(Yacc)->querydesc.condition,
                                (yyvsp[(1) - (1)].expr), sizeof(T_EXPRESSIONDESC));
                        break;
                    case ST_UPDATE:
                        {
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vUPDATE(Yacc)->subselect)
                            sc_memcpy(&vPLAN(Yacc)->querydesc.condition,
                                (yyvsp[(1) - (1)].expr), sizeof(T_EXPRESSIONDESC));
                        else
                        sc_memcpy(&vUPDATE(Yacc)->planquerydesc.querydesc.condition,
                                (yyvsp[(1) - (1)].expr), sizeof(T_EXPRESSIONDESC));
                        break;
                        }
                    default:
                        break;
                }
            }
    break;

  case 301:

/* Line 1806 of yacc.c  */
#line 3298 "parser.y"
    {
                (yyval.expr) = (yyvsp[(2) - (2)].expr);
            }
    break;

  case 303:

/* Line 1806 of yacc.c  */
#line 3306 "parser.y"
    {
                vPLAN(Yacc)->querydesc.groupby = Yacc->groupby;
                sc_memset(&(Yacc->groupby), 0, sizeof(T_GROUPBYDESC));
                if ((yyvsp[(4) - (4)].expr)) {
                    sc_memcpy(&vPLAN(Yacc)->querydesc.groupby.having,
                            (yyvsp[(4) - (4)].expr), sizeof(T_EXPRESSIONDESC));
                }
            }
    break;

  case 306:

/* Line 1806 of yacc.c  */
#line 3323 "parser.y"
    {
                char *attr;

                CHECK_ARRAY(Yacc->stmt,&(Yacc->groupby),sizeof(T_GROUPBYITEM));

                attr = sc_strchr((yyvsp[(1) - (1)].strval), '.');
                if (attr) {
                    sc_strncpy(tablename, (yyvsp[(1) - (1)].strval), attr-(yyvsp[(1) - (1)].strval));
                    tablename[attr++-(yyvsp[(1) - (1)].strval)] = '\0';
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
                    Yacc->groupby.list[Yacc->groupby.len].attr.attrname = (yyvsp[(1) - (1)].strval);
                }
                Yacc->groupby.list[Yacc->groupby.len].s_ref = -1;
                Yacc->groupby.len++;
            }
    break;

  case 307:

/* Line 1806 of yacc.c  */
#line 3349 "parser.y"
    {

                CHECK_ARRAY(Yacc->stmt,&(Yacc->groupby),sizeof(T_GROUPBYITEM));

                if (vPLAN(Yacc)->querydesc.selection.len < (int)(yyvsp[(1) - (1)].intval)) 
                    __yyerror("Invalid selection item index");

                expr = &vPLAN(Yacc)->querydesc.selection.list[(yyvsp[(1) - (1)].intval)-1].expr;
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

                Yacc->groupby.list[Yacc->groupby.len].s_ref = (int)(yyvsp[(1) - (1)].intval)-1;
                Yacc->groupby.len++;
            }
    break;

  case 308:

/* Line 1806 of yacc.c  */
#line 3387 "parser.y"
    {
                (yyval.expr) = NULL;
            }
    break;

  case 309:

/* Line 1806 of yacc.c  */
#line 3391 "parser.y"
    {
                (yyval.expr) = (yyvsp[(2) - (2)].expr);
            }
    break;

  case 311:

/* Line 1806 of yacc.c  */
#line 3399 "parser.y"
    {
                vPLAN(Yacc)->querydesc.orderby = Yacc->orderby;
                sc_memset(&(Yacc->orderby), 0, sizeof(T_ORDERBYDESC));
            }
    break;

  case 312:

/* Line 1806 of yacc.c  */
#line 3407 "parser.y"
    { }
    break;

  case 317:

/* Line 1806 of yacc.c  */
#line 3416 "parser.y"
    {
                switch (vSQL(Yacc)->sqltype) {
                    case ST_DELETE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vDELETE(Yacc)->subselect)
                        {
                            vPLAN(Yacc)->querydesc.limit.start_at = (OID)(yyvsp[(2) - (2)].intval);
                        }
                        else
                        {
                            __yyerror("limit @rid not allowed delete query.");
                        }
                        break;
                    case ST_SELECT:
                    case ST_CREATE: /* for view creation */
                        vPLAN(Yacc)->querydesc.limit.start_at = (OID)(yyvsp[(2) - (2)].intval);
                        break;
                    case ST_UPDATE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vUPDATE(Yacc)->subselect)
                        {
                            vPLAN(Yacc)->querydesc.limit.start_at = (OID)(yyvsp[(2) - (2)].intval);
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
    break;

  case 318:

/* Line 1806 of yacc.c  */
#line 3449 "parser.y"
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
    break;

  case 319:

/* Line 1806 of yacc.c  */
#line 3490 "parser.y"
    {
                switch (vSQL(Yacc)->sqltype) {
                    case ST_DELETE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vDELETE(Yacc)->subselect)
                          vPLAN(Yacc)->querydesc.limit.offset = (int)(yyvsp[(1) - (1)].intval);
                        else
                        __yyerror("limit offset not allowed delete query.");
                        break;
                    case ST_SELECT:
                    case ST_CREATE: /* for view creation */
                        vPLAN(Yacc)->querydesc.limit.offset = (int)(yyvsp[(1) - (1)].intval);
                        break;
                    case ST_UPDATE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vUPDATE(Yacc)->subselect)
                          vPLAN(Yacc)->querydesc.limit.offset = (int)(yyvsp[(1) - (1)].intval);
                        else
                        __yyerror("limit offset not allowed update query.");
                        break;
                    default:
                        break;
                }
            }
    break;

  case 320:

/* Line 1806 of yacc.c  */
#line 3515 "parser.y"
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
    break;

  case 321:

/* Line 1806 of yacc.c  */
#line 3543 "parser.y"
    {
                switch (vSQL(Yacc)->sqltype) {
                    case ST_DELETE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vDELETE(Yacc)->subselect)
                          vPLAN(Yacc)->querydesc.limit.rows = (int)(yyvsp[(1) - (1)].intval);
                        else
                        vDELETE(Yacc)->planquerydesc.querydesc.limit.rows = (int)(yyvsp[(1) - (1)].intval);
                        break;
                    case ST_SELECT:
                    case ST_CREATE: /* for view creation */
                        vPLAN(Yacc)->querydesc.limit.rows = (int)(yyvsp[(1) - (1)].intval);
                        break;
                    case ST_UPDATE:
                        /* where am i? in subquery? */
                        if (vSELECT(Yacc) != &vUPDATE(Yacc)->subselect)
                          vPLAN(Yacc)->querydesc.limit.rows = (int)(yyvsp[(1) - (1)].intval);
                        else
                        vUPDATE(Yacc)->planquerydesc.querydesc.limit.rows = (int)(yyvsp[(1) - (1)].intval);
                        break;
                    default:
                        break;
                }
            }
    break;

  case 322:

/* Line 1806 of yacc.c  */
#line 3568 "parser.y"
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
    break;

  case 325:

/* Line 1806 of yacc.c  */
#line 3602 "parser.y"
    {
                char *attr;

                CHECK_ARRAY(Yacc->stmt,&(Yacc->orderby),sizeof(T_ORDERBYITEM));

                attr = sc_strchr((yyvsp[(1) - (3)].strval), '.');
                if (attr) {
                    sc_strncpy(tablename, (yyvsp[(1) - (3)].strval), attr-(yyvsp[(1) - (3)].strval));
                    tablename[attr++-(yyvsp[(1) - (3)].strval)] = '\0';
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
                    Yacc->orderby.list[Yacc->orderby.len].attr.attrname  = (yyvsp[(1) - (3)].strval);
                }

                Yacc->orderby.list[Yacc->orderby.len].attr.collation = (yyvsp[(2) - (3)].num);
                Yacc->orderby.list[Yacc->orderby.len].ordertype = (yyvsp[(3) - (3)].num);
                Yacc->orderby.list[Yacc->orderby.len].s_ref      = -1;
                Yacc->orderby.list[Yacc->orderby.len].param_idx  = -1;
                Yacc->orderby.len++;
            }
    break;

  case 326:

/* Line 1806 of yacc.c  */
#line 3632 "parser.y"
    {
                if ((int)(yyvsp[(1) - (2)].intval) == 0)
                    __yyerror("Invalid selection item index");

                if (vSELECT(Yacc)->planquerydesc.querydesc.setlist.len == 0 
                        && vSELECT(Yacc)->kindofwTable != iTABLE_SUBSET)
                {
                    if (vPLAN(Yacc)->querydesc.selection.len > 0) 
                    {
                        if (vPLAN(Yacc)->querydesc.selection.len < (int)(yyvsp[(1) - (2)].intval)) 
                            __yyerror("Invalid selection item index");

                        expr = &vPLAN(Yacc)->querydesc.selection.list[(yyvsp[(1) - (2)].intval)-1].expr;
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

                Yacc->orderby.list[Yacc->orderby.len].ordertype = (yyvsp[(2) - (2)].num);
                Yacc->orderby.list[Yacc->orderby.len].s_ref      = (int)(yyvsp[(1) - (2)].intval)-1;
                Yacc->orderby.list[Yacc->orderby.len].param_idx  = -1;
                Yacc->orderby.len++;
            }
    break;

  case 327:

/* Line 1806 of yacc.c  */
#line 3691 "parser.y"
    {
                CHECK_ARRAY(Yacc->stmt,&(Yacc->orderby),sizeof(T_ORDERBYITEM));
                Yacc->orderby.list[Yacc->orderby.len].param_idx =
                    ++Yacc->param_count;
                Yacc->orderby.list[Yacc->orderby.len].ordertype = (yyvsp[(3) - (3)].num);
                Yacc->orderby.len++;
            }
    break;

  case 328:

/* Line 1806 of yacc.c  */
#line 3701 "parser.y"
    {
        (yyval.num) = MDB_COL_NONE;
    }
    break;

  case 329:

/* Line 1806 of yacc.c  */
#line 3705 "parser.y"
    {
        (yyval.num) = (yyvsp[(1) - (1)].num);
    }
    break;

  case 330:

/* Line 1806 of yacc.c  */
#line 3712 "parser.y"
    {
        (yyval.num) = db_find_collation((yyvsp[(2) - (2)].strval));
        if ((yyval.num) == MDB_COL_NONE)
        {
            __yyerror("invalid collation's type name");
        }
    }
    break;

  case 331:

/* Line 1806 of yacc.c  */
#line 3723 "parser.y"
    {
        (yyval.num)  = 'A';
    }
    break;

  case 332:

/* Line 1806 of yacc.c  */
#line 3727 "parser.y"
    {
        (yyval.num)  = (yyvsp[(1) - (1)].num);
    }
    break;

  case 333:

/* Line 1806 of yacc.c  */
#line 3734 "parser.y"
    {
        (yyval.num) = 'A';
    }
    break;

  case 334:

/* Line 1806 of yacc.c  */
#line 3738 "parser.y"
    {
        (yyval.num) = 'D';
    }
    break;

  case 335:

/* Line 1806 of yacc.c  */
#line 3748 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_OR;
                elem.u.op.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), &elem);
            }
    break;

  case 336:

/* Line 1806 of yacc.c  */
#line 3757 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_AND;
                elem.u.op.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), &elem);
            }
    break;

  case 337:

/* Line 1806 of yacc.c  */
#line 3766 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_NOT;
                elem.u.op.argcnt = 1;

                (yyval.expr) = merge_expression(Yacc, NULL, (yyvsp[(2) - (2)].expr), &elem);
            }
    break;

  case 338:

/* Line 1806 of yacc.c  */
#line 3775 "parser.y"
    {
                (yyval.expr) = (yyvsp[(2) - (3)].expr);
            }
    break;

  case 339:

/* Line 1806 of yacc.c  */
#line 3779 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 340:

/* Line 1806 of yacc.c  */
#line 3786 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 341:

/* Line 1806 of yacc.c  */
#line 3790 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 342:

/* Line 1806 of yacc.c  */
#line 3794 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 343:

/* Line 1806 of yacc.c  */
#line 3798 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 344:

/* Line 1806 of yacc.c  */
#line 3802 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 345:

/* Line 1806 of yacc.c  */
#line 3806 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 346:

/* Line 1806 of yacc.c  */
#line 3810 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 347:

/* Line 1806 of yacc.c  */
#line 3814 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 348:

/* Line 1806 of yacc.c  */
#line 3821 "parser.y"
    {
                if ((yyvsp[(3) - (3)].expr)->len == 1
                        && ((yyvsp[(3) - (3)].expr)->list[0]->u.value.is_null == 1
                            && !(yyvsp[(3) - (3)].expr)->list[0]->u.value.param_idx))
                {
                    if ((yyvsp[(2) - (3)].subtok).type == SOT_EQ) 
                    {
                        sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                        elem.elemtype    = SPT_OPERATOR;
                        elem.u.op.type   = SOT_ISNULL;
                        elem.u.op.argcnt = 1;

                        (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), NULL, &elem);
                    }
                    else if ((yyvsp[(2) - (3)].subtok).type == SOT_NE)
                    {
                        sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));

                        elem.elemtype    = SPT_OPERATOR;
                        elem.u.op.type   = SOT_ISNOTNULL;
                        elem.u.op.argcnt = 1;

                        (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), NULL, &elem);

                    }
                    else
                    {
                        sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                        elem.elemtype = SPT_OPERATOR;
                        elem.u.op     = (yyvsp[(2) - (3)].subtok);

                        (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), &elem);
                    }
                }
                else
                {
                    sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                    elem.elemtype = SPT_OPERATOR;
                    elem.u.op     = (yyvsp[(2) - (3)].subtok);

                    (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), &elem);
                }
            }
    break;

  case 349:

/* Line 1806 of yacc.c  */
#line 3865 "parser.y"
    {
            sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
            elem.elemtype    = SPT_OPERATOR;
            elem.u.op.type   = SOT_ISNULL;
            elem.u.op.argcnt = 1;

            (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), NULL, &elem);
         }
    break;

  case 350:

/* Line 1806 of yacc.c  */
#line 3874 "parser.y"
    {
            sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
            elem.elemtype    = SPT_OPERATOR;
            elem.u.op.type   = SOT_ISNOTNULL;
            elem.u.op.argcnt = 1;

            (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (4)].expr), NULL, &elem);
         }
    break;

  case 351:

/* Line 1806 of yacc.c  */
#line 3883 "parser.y"
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

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (1)].expr), rexpr, &elem);
            }
    break;

  case 352:

/* Line 1806 of yacc.c  */
#line 3907 "parser.y"
    {
                T_POSTFIXELEM *elist;

                // merge_expression()내부에서 $1을 free()시키므로
                // clon expression을 하나 가지고 있어야 한다.
                expr = sql_mem_get_free_expressiondesc(vSQL(Yacc)->parsing_memory, (yyvsp[(1) - (6)].expr)->len, (yyvsp[(1) - (6)].expr)->max);
                if (!expr) 
                    return RET_ERROR;

                for (i = 0; i < expr->len; ++i)
                    sc_memcpy(expr->list[i], (yyvsp[(1) - (6)].expr)->list[i], sizeof(T_POSTFIXELEM));

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
                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (6)].expr), (yyvsp[(4) - (6)].expr), &elem);

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), expr, NULL);

                elem.u.op.type   = SOT_LE;
                elem.u.op.argcnt = 2;
                (yyval.expr) = merge_expression(Yacc, (yyval.expr), (yyvsp[(6) - (6)].expr), &elem);

                elem.u.op.type   = SOT_AND;
                elem.u.op.argcnt = 2;
                (yyval.expr) = merge_expression(Yacc, (yyval.expr), NULL, &elem);

                elem.u.op.type   = SOT_NOT;
                elem.u.op.argcnt = 1;
                (yyval.expr) = merge_expression(Yacc, (yyval.expr), NULL, &elem);
            }
    break;

  case 353:

/* Line 1806 of yacc.c  */
#line 3970 "parser.y"
    {
                T_POSTFIXELEM *elist;

                expr = sql_mem_get_free_expressiondesc(vSQL(Yacc)->parsing_memory, (yyvsp[(1) - (5)].expr)->len, (yyvsp[(1) - (5)].expr)->max);
                if (!expr)
                    return RET_ERROR;

                for(i = 0; i < (yyvsp[(1) - (5)].expr)->len; ++i)
                    sc_memcpy(expr->list[i], (yyvsp[(1) - (5)].expr)->list[i], sizeof(T_POSTFIXELEM));

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
                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (5)].expr), (yyvsp[(3) - (5)].expr), &elem);

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), expr, NULL);

                elem.u.op.type   = SOT_LE;
                elem.u.op.argcnt = 2;
                (yyval.expr) = merge_expression(Yacc, (yyval.expr), (yyvsp[(5) - (5)].expr), &elem);

                elem.u.op.type   = SOT_AND;
                elem.u.op.argcnt = 2;
                (yyval.expr) = merge_expression(Yacc, (yyval.expr), NULL, &elem);
            }
    break;

  case 354:

/* Line 1806 of yacc.c  */
#line 4030 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = (yyvsp[(3) - (5)].num);
                elem.u.op.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (5)].expr), (yyvsp[(4) - (5)].expr), &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_NOT;
                elem.u.op.argcnt = 1;

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), NULL, &elem);
            }
    break;

  case 355:

/* Line 1806 of yacc.c  */
#line 4046 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = (yyvsp[(2) - (4)].num);
                elem.u.op.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (4)].expr), (yyvsp[(3) - (4)].expr), &elem);
            }
    break;

  case 356:

/* Line 1806 of yacc.c  */
#line 4058 "parser.y"
    {
        (yyval.num) = SOT_LIKE;
    }
    break;

  case 357:

/* Line 1806 of yacc.c  */
#line 4062 "parser.y"
    {
        (yyval.num) = SOT_ILIKE;
    }
    break;

  case 359:

/* Line 1806 of yacc.c  */
#line 4070 "parser.y"
    {
                INCOMPLETION("ESCAPE");
            }
    break;

  case 360:

/* Line 1806 of yacc.c  */
#line 4077 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_VARIABLE;
                elem.u.value.valuetype  = DT_NULL_TYPE;

                p = sc_strchr((yyvsp[(1) - (4)].strval), '.');
                if (p == NULL) {
                    elem.u.value.attrdesc.table.tablename = NULL;
                    elem.u.value.attrdesc.table.aliasname = NULL;
                    elem.u.value.attrdesc.attrname  = (yyvsp[(1) - (4)].strval);
                }
                else {
                    *p++ = '\0';
                    elem.u.value.attrdesc.table.tablename =
                        YACC_XSTRDUP(Yacc, (yyvsp[(1) - (4)].strval));
                    elem.u.value.attrdesc.table.aliasname = NULL;
                    elem.u.value.attrdesc.attrname  = YACC_XSTRDUP(Yacc, p);

                    isnull(elem.u.value.attrdesc.table.tablename);
                    isnull(elem.u.value.attrdesc.attrname);

                }

                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_ISNOTNULL;
                elem.u.op.argcnt = 1;

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), NULL, &elem);
            }
    break;

  case 361:

/* Line 1806 of yacc.c  */
#line 4111 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_VARIABLE;
                elem.u.value.valuetype  = DT_NULL_TYPE;

                p = sc_strchr((yyvsp[(1) - (3)].strval), '.');
                if (p == NULL) {
                    elem.u.value.attrdesc.table.tablename = NULL;
                    elem.u.value.attrdesc.table.aliasname = NULL;
                    elem.u.value.attrdesc.attrname  = (yyvsp[(1) - (3)].strval);
                }
                else {
                    *p++ = '\0';
                    elem.u.value.attrdesc.table.tablename =
                        YACC_XSTRDUP(Yacc, (yyvsp[(1) - (3)].strval));
                    elem.u.value.attrdesc.table.aliasname = NULL;
                    elem.u.value.attrdesc.attrname  = YACC_XSTRDUP(Yacc, p);
                    isnull(elem.u.value.attrdesc.table.tablename);
                    isnull(elem.u.value.attrdesc.attrname);
                }

                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_ISNULL;
                elem.u.op.argcnt = 1;

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), NULL, &elem);
            }
    break;

  case 362:

/* Line 1806 of yacc.c  */
#line 4146 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_SUBQ_OP;
                elem.u.op.type   = SOT_NIN;
                elem.u.op.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (4)].expr), (yyvsp[(4) - (4)].expr), &elem);
            }
    break;

  case 363:

/* Line 1806 of yacc.c  */
#line 4155 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_SUBQ_OP;
                elem.u.op.type   = SOT_IN;
                elem.u.op.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), &elem);
            }
    break;

  case 364:

/* Line 1806 of yacc.c  */
#line 4164 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_NIN;
                elem.u.op.argcnt = (yyvsp[(5) - (6)].expr)->len+1;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (6)].expr), (yyvsp[(5) - (6)].expr), &elem);
            }
    break;

  case 365:

/* Line 1806 of yacc.c  */
#line 4173 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_IN;
                elem.u.op.argcnt = (yyvsp[(4) - (5)].expr)->len+1;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (5)].expr), (yyvsp[(4) - (5)].expr), &elem);
            }
    break;

  case 366:

/* Line 1806 of yacc.c  */
#line 4185 "parser.y"
    {
                (yyval.expr) = get_expr_with_item(Yacc, &(yyvsp[(1) - (1)].elem));
            }
    break;

  case 367:

/* Line 1806 of yacc.c  */
#line 4189 "parser.y"
    {
                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), NULL, &(yyvsp[(3) - (3)].elem));
            }
    break;

  case 368:

/* Line 1806 of yacc.c  */
#line 4196 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_SUBQ_OP;
                if((yyvsp[(3) - (4)].intval) == SOT_SOME) {
                    switch((yyvsp[(2) - (4)].subtok).type) {
                    case SOT_LT:
                    case SOT_LE:
                    case SOT_GT:
                    case SOT_GE:
                    case SOT_EQ:
                    case SOT_NE:
                        elem.u.op.type = SOT_SOME_LT + ((yyvsp[(2) - (4)].subtok).type - SOT_LT);
                        break;
                    default:
                        __yyerror("invalid op type");
                        break;
                    }
                } else if((yyvsp[(3) - (4)].intval) == SOT_ALL) {
                    switch((yyvsp[(2) - (4)].subtok).type) {
                    case SOT_LT:
                    case SOT_LE:
                    case SOT_GT:
                    case SOT_GE:
                    case SOT_EQ:
                    case SOT_NE:
                        elem.u.op.type = SOT_ALL_LT + ((yyvsp[(2) - (4)].subtok).type - SOT_LT);
                        break;
                    default:
                        __yyerror("invalid op type");
                        break;
                    }
                } else {
                    __yyerror("any_all_some rule returned wrong value");
                }
                elem.u.op.argcnt = 2;   /* 순서대로 scalar_exp, comp, subq */

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (4)].expr), (yyvsp[(4) - (4)].expr), &elem);
            }
    break;

  case 369:

/* Line 1806 of yacc.c  */
#line 4238 "parser.y"
    {
                (yyval.intval) = SOT_SOME;
            }
    break;

  case 370:

/* Line 1806 of yacc.c  */
#line 4242 "parser.y"
    {
                (yyval.intval) = SOT_ALL;
            }
    break;

  case 371:

/* Line 1806 of yacc.c  */
#line 4246 "parser.y"
    {
                (yyval.intval) = SOT_SOME;
            }
    break;

  case 372:

/* Line 1806 of yacc.c  */
#line 4253 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_SUBQ_OP;
                elem.u.op.type   = SOT_EXISTS;
                elem.u.op.argcnt = 1;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(2) - (2)].expr), NULL, &elem);
            }
    break;

  case 373:

/* Line 1806 of yacc.c  */
#line 4265 "parser.y"
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
                (yyval.expr) = get_expr_with_item(Yacc, &elem);
            }
    break;

  case 374:

/* Line 1806 of yacc.c  */
#line 4309 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_PLUS;
                elem.u.op.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), &elem);
            }
    break;

  case 375:

/* Line 1806 of yacc.c  */
#line 4318 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_MINUS;
                elem.u.op.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), &elem);
            }
    break;

  case 376:

/* Line 1806 of yacc.c  */
#line 4327 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_TIMES;
                elem.u.op.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), &elem);
            }
    break;

  case 377:

/* Line 1806 of yacc.c  */
#line 4336 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_DIVIDE;
                elem.u.op.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), &elem);
            }
    break;

  case 378:

/* Line 1806 of yacc.c  */
#line 4345 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_MODULA;
                elem.u.op.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), &elem);
            }
    break;

  case 379:

/* Line 1806 of yacc.c  */
#line 4354 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_EXPONENT;
                elem.u.op.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), &elem);
            }
    break;

  case 380:

/* Line 1806 of yacc.c  */
#line 4363 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_MERGE;
                elem.u.op.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), &elem);
            }
    break;

  case 381:

/* Line 1806 of yacc.c  */
#line 4372 "parser.y"
    {
                (yyval.expr) = (yyvsp[(2) - (2)].expr);
            }
    break;

  case 382:

/* Line 1806 of yacc.c  */
#line 4376 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_BIGINT;
                elem.u.value.u.i64      = -1;
                elem.u.value.attrdesc.collation   = MDB_COL_NUMERIC;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(2) - (2)].expr), NULL, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype    = SPT_OPERATOR;
                elem.u.op.type   = SOT_TIMES;
                elem.u.op.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), NULL, &elem);
            }
    break;

  case 383:

/* Line 1806 of yacc.c  */
#line 4394 "parser.y"
    {
                (yyval.expr) = get_expr_with_item(Yacc, &(yyvsp[(1) - (1)].elem));
            }
    break;

  case 384:

/* Line 1806 of yacc.c  */
#line 4398 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 385:

/* Line 1806 of yacc.c  */
#line 4402 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 386:

/* Line 1806 of yacc.c  */
#line 4406 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 387:

/* Line 1806 of yacc.c  */
#line 4410 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 388:

/* Line 1806 of yacc.c  */
#line 4414 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 389:

/* Line 1806 of yacc.c  */
#line 4418 "parser.y"
    {
                (yyval.expr) = (yyvsp[(2) - (3)].expr);
            }
    break;

  case 390:

/* Line 1806 of yacc.c  */
#line 4425 "parser.y"
    {
                (yyval.elem) = (yyvsp[(1) - (1)].elem);
            }
    break;

  case 391:

/* Line 1806 of yacc.c  */
#line 4429 "parser.y"
    {
                (yyval.elem) = (yyvsp[(1) - (1)].elem);
            }
    break;

  case 392:

/* Line 1806 of yacc.c  */
#line 4436 "parser.y"
    {
                sc_memset(&(yyval.elem), 0, sizeof(T_POSTFIXELEM));
                (yyval.elem).elemtype = SPT_VALUE;
                (yyval.elem).u.value.valueclass = SVC_CONSTANT;
                (yyval.elem).u.value.valuetype  = DT_NULL_TYPE;
                (yyval.elem).u.value.is_null    = 1;

                /* param_idx는 1부터 시작하자.
                 * 만약, 0부터 시작하게 되면 parameter가 아닌 모든 item에 대해
                 * 항상 -1로 설정해야 하는 부담이 있다.
                 */
                (yyval.elem).u.value.param_idx  = ++Yacc->param_count;
            }
    break;

  case 393:

/* Line 1806 of yacc.c  */
#line 4453 "parser.y"
    {    }
    break;

  case 394:

/* Line 1806 of yacc.c  */
#line 4455 "parser.y"
    {    }
    break;

  case 395:

/* Line 1806 of yacc.c  */
#line 4460 "parser.y"
    {
                (yyval.expr) = (yyvsp[(2) - (2)].expr);
                Yacc->is_distinct = 1;
                if (sql_expr_has_ridfunc((yyvsp[(2) - (2)].expr)))
                    __yyerror("RID as aggregation's arg is not supported.");
            }
    break;

  case 396:

/* Line 1806 of yacc.c  */
#line 4467 "parser.y"
    {
                (yyval.expr) = (yyvsp[(2) - (2)].expr);
                Yacc->is_distinct = 0;
                if (sql_expr_has_ridfunc((yyvsp[(2) - (2)].expr)))
                    __yyerror("RID as aggregation's arg is not supported.");
            }
    break;

  case 397:

/* Line 1806 of yacc.c  */
#line 4474 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
                Yacc->is_distinct = 0;
                if (sql_expr_has_ridfunc((yyvsp[(1) - (1)].expr)))
                    __yyerror("RID as aggregation's arg is not supported.");
            }
    break;

  case 398:

/* Line 1806 of yacc.c  */
#line 4484 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_AGGREGATION;
                elem.u.aggr.type   = SAT_AVG;
                elem.u.aggr.argcnt = 1;
                elem.u.aggr.significant = (yyvsp[(3) - (4)].expr)->len;
                if (Yacc->is_distinct) {
                    Yacc->is_distinct = 0;
                    elem.is_distinct = 1;
                }

                (yyval.expr) = merge_expression(Yacc, NULL, (yyvsp[(3) - (4)].expr), &elem);
            }
    break;

  case 399:

/* Line 1806 of yacc.c  */
#line 4498 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_AGGREGATION;
                elem.u.aggr.type   = SAT_COUNT;
                elem.u.aggr.argcnt = 0;
                elem.u.aggr.significant = 0;

                (yyval.expr) = get_expr_with_item(Yacc, &elem);
            }
    break;

  case 400:

/* Line 1806 of yacc.c  */
#line 4508 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_AGGREGATION;
                elem.u.aggr.type   = SAT_COUNT;
                elem.u.aggr.argcnt = 1;
                elem.u.aggr.significant = (yyvsp[(3) - (4)].expr)->len;
                if (Yacc->is_distinct) {
                    Yacc->is_distinct = 0;
                    elem.is_distinct = 1;
                }

                (yyval.expr) = merge_expression(Yacc, NULL, (yyvsp[(3) - (4)].expr), &elem);
            }
    break;

  case 401:

/* Line 1806 of yacc.c  */
#line 4522 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_AGGREGATION;
                elem.u.aggr.type   = SAT_DCOUNT;
                elem.u.aggr.argcnt = 0;
                elem.u.aggr.significant = 0;

                (yyval.expr) = get_expr_with_item(Yacc, &elem);
            }
    break;

  case 402:

/* Line 1806 of yacc.c  */
#line 4532 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_AGGREGATION;
                elem.u.aggr.type   = SAT_DCOUNT;
                elem.u.aggr.argcnt = 0;
                elem.u.aggr.significant = 0;

                (yyval.expr) = get_expr_with_item(Yacc, &elem);
            }
    break;

  case 403:

/* Line 1806 of yacc.c  */
#line 4542 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_AGGREGATION;
                elem.u.aggr.type   = SAT_MAX;
                elem.u.aggr.argcnt = 1;
                elem.u.aggr.significant = (yyvsp[(3) - (4)].expr)->len;
                if (Yacc->is_distinct) {
                    Yacc->is_distinct = 0;
                    elem.is_distinct = 1;
                }

                (yyval.expr) = merge_expression(Yacc, NULL, (yyvsp[(3) - (4)].expr), &elem);
            }
    break;

  case 404:

/* Line 1806 of yacc.c  */
#line 4556 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_AGGREGATION;
                elem.u.aggr.type   = SAT_MIN;
                elem.u.aggr.argcnt = 1;
                elem.u.aggr.significant = (yyvsp[(3) - (4)].expr)->len;
                if (Yacc->is_distinct) {
                    Yacc->is_distinct = 0;
                    elem.is_distinct = 1;
                }

                (yyval.expr) = merge_expression(Yacc, NULL, (yyvsp[(3) - (4)].expr), &elem);
            }
    break;

  case 405:

/* Line 1806 of yacc.c  */
#line 4570 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_AGGREGATION;
                if (Yacc->is_distinct) elem.is_distinct = 1;
                elem.u.aggr.type   = SAT_SUM;
                elem.u.aggr.argcnt = 1;
                elem.u.aggr.significant = (yyvsp[(3) - (4)].expr)->len;
                if (Yacc->is_distinct) {
                    Yacc->is_distinct = 0;
                    elem.is_distinct = 1;
                }

                (yyval.expr) = merge_expression(Yacc, NULL, (yyvsp[(3) - (4)].expr), &elem);
            }
    break;

  case 406:

/* Line 1806 of yacc.c  */
#line 4588 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 407:

/* Line 1806 of yacc.c  */
#line 4592 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 408:

/* Line 1806 of yacc.c  */
#line 4596 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 409:

/* Line 1806 of yacc.c  */
#line 4600 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 410:

/* Line 1806 of yacc.c  */
#line 4604 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 411:

/* Line 1806 of yacc.c  */
#line 4608 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 412:

/* Line 1806 of yacc.c  */
#line 4612 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 413:

/* Line 1806 of yacc.c  */
#line 4616 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 414:

/* Line 1806 of yacc.c  */
#line 4620 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 415:

/* Line 1806 of yacc.c  */
#line 4627 "parser.y"
    {
                if (((yyvsp[(1) - (1)].elem).elemtype != SPT_VALUE ||
                        (yyvsp[(1) - (1)].elem).u.value.valueclass != SVC_CONSTANT ||
                        (yyvsp[(1) - (1)].elem).u.value.valuetype != DT_VARCHAR) &&
                        !((yyvsp[(1) - (1)].elem).elemtype == SPT_VALUE &&
                        (yyvsp[(1) - (1)].elem).u.value.valueclass == SVC_CONSTANT &&
                        (yyvsp[(1) - (1)].elem).u.value.valuetype == DT_NULL_TYPE &&
                        (yyvsp[(1) - (1)].elem).u.value.param_idx))
                    __yyerror("CHARTOHEX() or HEXCOMP()'s arguments must be a string if constant value");
                (yyval.elem) = (yyvsp[(1) - (1)].elem);
            }
    break;

  case 416:

/* Line 1806 of yacc.c  */
#line 4639 "parser.y"
    {

                sc_memset(&(yyval.elem), 0, sizeof(T_POSTFIXELEM));
                (yyval.elem).elemtype = SPT_VALUE;
                (yyval.elem).u.value.valueclass = SVC_VARIABLE;
                (yyval.elem).u.value.valuetype  = DT_NULL_TYPE;

                p = sc_strchr((yyvsp[(1) - (1)].strval), '.');
                if (p == NULL) {
                    (yyval.elem).u.value.attrdesc.table.tablename = NULL;
                    (yyval.elem).u.value.attrdesc.table.aliasname = NULL;
                    (yyval.elem).u.value.attrdesc.attrname  = (yyvsp[(1) - (1)].strval);
                }
                else {
                    *p++ = '\0';
                    (yyval.elem).u.value.attrdesc.table.tablename = YACC_XSTRDUP(Yacc, (yyvsp[(1) - (1)].strval));
                    (yyval.elem).u.value.attrdesc.table.aliasname = NULL;
                    (yyval.elem).u.value.attrdesc.attrname  = YACC_XSTRDUP(Yacc, p);
                    isnull((yyval.elem).u.value.attrdesc.table.tablename);
                    isnull((yyval.elem).u.value.attrdesc.attrname);
                }
            }
    break;

  case 417:

/* Line 1806 of yacc.c  */
#line 4665 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_HEXCOMP;
                elem.u.func.argcnt = 2;

                (yyval.expr) = get_expr_with_item(Yacc, &(yyvsp[(3) - (6)].elem));
                (yyval.expr) = merge_expression(Yacc, (yyval.expr), NULL, &(yyvsp[(5) - (6)].elem));
                (yyval.expr) = merge_expression(Yacc, (yyval.expr), NULL, &elem);
            }
    break;

  case 418:

/* Line 1806 of yacc.c  */
#line 4679 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_CHARTOHEX;
                elem.u.func.argcnt = 1;

                (yyval.expr) = merge_expression(Yacc,
                            get_expr_with_item(Yacc, &(yyvsp[(3) - (4)].elem)), NULL, &elem);
            }
    break;

  case 419:

/* Line 1806 of yacc.c  */
#line 4692 "parser.y"
    {
                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), NULL);
            }
    break;

  case 420:

/* Line 1806 of yacc.c  */
#line 4699 "parser.y"
    {
                (yyval.expr) = (yyvsp[(1) - (1)].expr);
            }
    break;

  case 421:

/* Line 1806 of yacc.c  */
#line 4703 "parser.y"
    {
                (yyval.expr) = merge_expression(Yacc, (yyvsp[(1) - (3)].expr), (yyvsp[(3) - (3)].expr), NULL);
            }
    break;

  case 422:

/* Line 1806 of yacc.c  */
#line 4710 "parser.y"
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

                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_CONVERT;
                elem.u.func.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), (yyvsp[(5) - (6)].expr), &elem);

                sc_memset(&(Yacc->attr), 0, sizeof(T_ATTRDESC));
            }
    break;

  case 423:

/* Line 1806 of yacc.c  */
#line 4740 "parser.y"
    {
                int arg_cnt;
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_DECODE;
                arg_cnt = (yyvsp[(5) - (6)].expr)->len + 1;
                for (i = 0; i <(yyvsp[(5) - (6)].expr)->len; ++i)
                {
                    if ((yyvsp[(5) - (6)].expr)->list[i]->elemtype == SPT_OPERATOR)
                    {
                        arg_cnt -= (yyvsp[(5) - (6)].expr)->list[i]->u.op.argcnt;
                    }
                    else if ((yyvsp[(5) - (6)].expr)->list[i]->elemtype == SPT_FUNCTION)
                    {
                        arg_cnt -= (yyvsp[(5) - (6)].expr)->list[i]->u.func.argcnt;
                    }
                    else if ((yyvsp[(5) - (6)].expr)->list[i]->elemtype == SPT_AGGREGATION)
                    {
                        arg_cnt -= (yyvsp[(5) - (6)].expr)->list[i]->u.aggr.argcnt;
                    }
                }

                elem.u.func.argcnt = arg_cnt;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr), &elem);
            }
    break;

  case 424:

/* Line 1806 of yacc.c  */
#line 4767 "parser.y"
    {
                int arg_cnt;
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_DECODE;
                arg_cnt = (yyvsp[(5) - (8)].expr)->len + 2;
                for (i = 0; i <(yyvsp[(5) - (8)].expr)->len; ++i)
                {
                    if ((yyvsp[(5) - (8)].expr)->list[i]->elemtype == SPT_OPERATOR)
                    {
                        arg_cnt -= (yyvsp[(5) - (8)].expr)->list[i]->u.op.argcnt;
                    }
                    else if ((yyvsp[(5) - (8)].expr)->list[i]->elemtype == SPT_FUNCTION)
                    {
                        arg_cnt -= (yyvsp[(5) - (8)].expr)->list[i]->u.func.argcnt;
                    }
                    else if ((yyvsp[(5) - (8)].expr)->list[i]->elemtype == SPT_AGGREGATION)
                    {
                        arg_cnt -= (yyvsp[(5) - (8)].expr)->list[i]->u.aggr.argcnt;
                    }
                }

                elem.u.func.argcnt = arg_cnt;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (8)].expr), (yyvsp[(5) - (8)].expr), NULL);
                (yyval.expr) = merge_expression(Yacc, (yyval.expr), (yyvsp[(7) - (8)].expr), &elem);
            }
    break;

  case 425:

/* Line 1806 of yacc.c  */
#line 4795 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_IFNULL;
                elem.u.func.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr), &elem);
            }
    break;

  case 426:

/* Line 1806 of yacc.c  */
#line 4807 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_SIGN;
                elem.u.func.argcnt = 1;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (4)].expr), NULL, &elem);
            }
    break;

  case 427:

/* Line 1806 of yacc.c  */
#line 4816 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype           = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_INTEGER;
                elem.u.value.u.i32      = 0;
                elem.u.value.attrdesc.collation   = MDB_COL_NUMERIC;

                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_ROUND;
                elem.u.func.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (4)].expr), (yyval.expr), &elem);
            }
    break;

  case 428:

/* Line 1806 of yacc.c  */
#line 4834 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_ROUND;
                elem.u.func.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr), &elem);
            }
    break;

  case 429:

/* Line 1806 of yacc.c  */
#line 4843 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype           = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_INTEGER;
                elem.u.value.u.i32      = 0;
                elem.u.value.attrdesc.collation   = MDB_COL_NUMERIC;

                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_TRUNC;
                elem.u.func.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (4)].expr), (yyval.expr), &elem);
            }
    break;

  case 430:

/* Line 1806 of yacc.c  */
#line 4861 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_TRUNC;
                elem.u.func.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr), &elem);
            }
    break;

  case 431:

/* Line 1806 of yacc.c  */
#line 4873 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_BIGINT;
                elem.u.value.u.i64      = (yyvsp[(5) - (6)].intval);
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;

                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_BIGINT;
                elem.u.value.u.i64      = -1;
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (6)].expr), (yyval.expr), &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_SUBSTRING;
                elem.u.func.argcnt = 3;

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), NULL, &elem);
            }
    break;

  case 432:

/* Line 1806 of yacc.c  */
#line 4900 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_BIGINT;
                elem.u.value.u.i64      = (yyvsp[(5) - (8)].intval);
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;

                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_BIGINT;
                elem.u.value.u.i64      = (yyvsp[(7) - (8)].intval);
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (8)].expr), (yyval.expr), &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_SUBSTRING;
                elem.u.func.argcnt = 3;

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), NULL, &elem);
            }
    break;

  case 433:

/* Line 1806 of yacc.c  */
#line 4927 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_LOWERCASE;
                elem.u.func.argcnt = 1;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (4)].expr), NULL, &elem);
            }
    break;

  case 434:

/* Line 1806 of yacc.c  */
#line 4936 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_UPPERCASE;
                elem.u.func.argcnt = 1;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (4)].expr), NULL, &elem);
            }
    break;

  case 435:

/* Line 1806 of yacc.c  */
#line 4945 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_LTRIM;
                elem.u.func.argcnt = 1;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (4)].expr), NULL, &elem);
            }
    break;

  case 436:

/* Line 1806 of yacc.c  */
#line 4954 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_LTRIM;
                elem.u.func.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr), &elem);
            }
    break;

  case 437:

/* Line 1806 of yacc.c  */
#line 4963 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_RTRIM;
                elem.u.func.argcnt = 1;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (4)].expr), NULL, &elem);
            }
    break;

  case 438:

/* Line 1806 of yacc.c  */
#line 4972 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_RTRIM;
                elem.u.func.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr), &elem);
            }
    break;

  case 439:

/* Line 1806 of yacc.c  */
#line 4981 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_REPLACE;
                elem.u.func.argcnt = 3;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (8)].expr), (yyvsp[(5) - (8)].expr), NULL);
                (yyval.expr) = merge_expression(Yacc, (yyval.expr), (yyvsp[(7) - (8)].expr), &elem);        // 1.2
            }
    break;

  case 440:

/* Line 1806 of yacc.c  */
#line 4991 "parser.y"
    {
#if defined(MDB_DEBUG)
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_TABLENAME;
                elem.u.func.argcnt = 1;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (4)].expr), NULL, &elem);
#else
                __yyerror("RID in query is not supported.");
#endif
            }
    break;

  case 441:

/* Line 1806 of yacc.c  */
#line 5004 "parser.y"
    {
            sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
            elem.elemtype      = SPT_FUNCTION;
            elem.u.func.type   = SFT_HEXASTRING;
            elem.u.func.argcnt = 1;

            (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (4)].expr), NULL, &elem);
        }
    break;

  case 442:

/* Line 1806 of yacc.c  */
#line 5013 "parser.y"
    {
            sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
            elem.elemtype      = SPT_FUNCTION;
            elem.u.func.type   = SFT_BINARYSTRING;
            elem.u.func.argcnt = 1;

            (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (4)].expr), NULL, &elem);
        }
    break;

  case 443:

/* Line 1806 of yacc.c  */
#line 5025 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_CURRENT_DATE;
                elem.u.func.argcnt = 0;
                (yyval.expr) = get_expr_with_item(Yacc, &elem);
            }
    break;

  case 444:

/* Line 1806 of yacc.c  */
#line 5033 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_CURRENT_TIME;
                elem.u.func.argcnt = 0;
                (yyval.expr) = get_expr_with_item(Yacc, &elem);
            }
    break;

  case 445:

/* Line 1806 of yacc.c  */
#line 5041 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_CURRENT_TIMESTAMP;
                elem.u.func.argcnt = 0;
                (yyval.expr) = get_expr_with_item(Yacc, &elem);
            }
    break;

  case 446:

/* Line 1806 of yacc.c  */
#line 5049 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_SYSDATE;
                elem.u.func.argcnt = 0;
                (yyval.expr) = get_expr_with_item(Yacc, &elem);
            }
    break;

  case 447:

/* Line 1806 of yacc.c  */
#line 5057 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_NOW;
                elem.u.func.argcnt = 0;

                (yyval.expr) = get_expr_with_item(Yacc, &elem);
            }
    break;

  case 448:

/* Line 1806 of yacc.c  */
#line 5066 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = (yyvsp[(3) - (8)].strval);
                elem.u.value.attrdesc.len = sc_strlen((yyvsp[(3) - (8)].strval));
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                Yacc->attr.collation = DB_Params.default_col_char;

                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), (yyvsp[(5) - (8)].expr), NULL);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_DATE_ADD;
                elem.u.func.argcnt = 3;

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), (yyvsp[(7) - (8)].expr), &elem);
            }
    break;

  case 449:

/* Line 1806 of yacc.c  */
#line 5088 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = (yyvsp[(3) - (8)].strval);
                elem.u.value.attrdesc.len = sc_strlen((yyvsp[(3) - (8)].strval));
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;

                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), (yyvsp[(5) - (8)].expr), NULL);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_DATE_SUB;
                elem.u.func.argcnt = 3;

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), (yyvsp[(7) - (8)].expr), &elem);
            }
    break;

  case 450:

/* Line 1806 of yacc.c  */
#line 5110 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = (yyvsp[(3) - (8)].strval);
                elem.u.value.attrdesc.len = sc_strlen((yyvsp[(3) - (8)].strval));
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;

                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), (yyvsp[(5) - (8)].expr), NULL);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_DATE_DIFF;
                elem.u.func.argcnt = 3;

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), (yyvsp[(7) - (8)].expr), &elem);
            }
    break;

  case 451:

/* Line 1806 of yacc.c  */
#line 5132 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = (yyvsp[(5) - (6)].strval);
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;
                elem.u.value.attrdesc.len = sc_strlen((yyvsp[(5) - (6)].strval));

                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_DATE_FORMAT;
                elem.u.func.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (6)].expr), (yyval.expr), &elem);
            }
    break;

  case 452:

/* Line 1806 of yacc.c  */
#line 5152 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = (yyvsp[(3) - (8)].strval);
                elem.u.value.attrdesc.len = sc_strlen((yyvsp[(3) - (8)].strval));
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                Yacc->attr.collation = DB_Params.default_col_char;

                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), (yyvsp[(5) - (8)].expr), NULL);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_TIME_ADD;
                elem.u.func.argcnt = 3;

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), (yyvsp[(7) - (8)].expr), &elem);
            }
    break;

  case 453:

/* Line 1806 of yacc.c  */
#line 5174 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = (yyvsp[(3) - (8)].strval);
                elem.u.value.attrdesc.len = sc_strlen((yyvsp[(3) - (8)].strval));
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;

                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), (yyvsp[(5) - (8)].expr), NULL);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_TIME_SUB;
                elem.u.func.argcnt = 3;

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), (yyvsp[(7) - (8)].expr), &elem);
            }
    break;

  case 454:

/* Line 1806 of yacc.c  */
#line 5196 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = (yyvsp[(3) - (8)].strval);
                elem.u.value.attrdesc.len = sc_strlen((yyvsp[(3) - (8)].strval));
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;

                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), (yyvsp[(5) - (8)].expr), NULL);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_TIME_DIFF;
                elem.u.func.argcnt = 3;

                (yyval.expr) = merge_expression(Yacc, (yyval.expr), (yyvsp[(7) - (8)].expr), &elem);
            }
    break;

  case 455:

/* Line 1806 of yacc.c  */
#line 5218 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = (yyvsp[(5) - (6)].strval);
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;
                elem.u.value.attrdesc.len = sc_strlen((yyvsp[(5) - (6)].strval));

                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_TIME_FORMAT;
                elem.u.func.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (6)].expr), (yyval.expr), &elem);
            }
    break;

  case 456:

/* Line 1806 of yacc.c  */
#line 5241 "parser.y"
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

                (yyval.expr) = get_expr_with_item(Yacc, &elem);
            }
    break;

  case 457:

/* Line 1806 of yacc.c  */
#line 5259 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype           = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_VARCHAR;
                elem.u.value.u.ptr      = (yyvsp[(1) - (3)].strval);
                elem.u.value.attrdesc.len = sc_strlen((yyvsp[(1) - (3)].strval));
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;
                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype       = SPT_FUNCTION;
                elem.u.func.type    = SFT_SEQUENCE_CURRVAL;
                elem.u.func.argcnt  = 1;
                (yyval.expr) = merge_expression(Yacc, (yyval.expr), NULL, &elem);
            }
    break;

  case 458:

/* Line 1806 of yacc.c  */
#line 5277 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype           = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_VARCHAR;
                elem.u.value.u.ptr      = (yyvsp[(1) - (3)].strval);
                elem.u.value.attrdesc.len = sc_strlen((yyvsp[(1) - (3)].strval));
                elem.u.value.value_len    = elem.u.value.attrdesc.len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;
                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype       = SPT_FUNCTION;
                elem.u.func.type    = SFT_SEQUENCE_NEXTVAL;
                elem.u.func.argcnt  = 1;
                (yyval.expr) = merge_expression(Yacc, (yyval.expr), NULL, &elem);
            }
    break;

  case 459:

/* Line 1806 of yacc.c  */
#line 5298 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype       = SPT_FUNCTION;
                elem.u.func.type    = SFT_BYTESIZE;
                elem.u.func.argcnt  = 1;
                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (4)].expr), NULL, &elem);
            }
    break;

  case 460:

/* Line 1806 of yacc.c  */
#line 5306 "parser.y"
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
                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype       = SPT_FUNCTION;
                elem.u.func.type    = SFT_COPYFROM;
                elem.u.func.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (4)].expr), (yyval.expr), &elem);
            }
    break;

  case 461:

/* Line 1806 of yacc.c  */
#line 5326 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass   = SVC_CONSTANT;
                elem.u.value.valuetype    = DT_VARCHAR;
                elem.u.value.u.ptr        = (yyvsp[(5) - (6)].strval);
                elem.u.value.value_len    = sc_strlen(elem.u.value.u.ptr);
                elem.u.value.attrdesc.len = elem.u.value.value_len;
                elem.u.value.attrdesc.collation = DB_Params.default_col_char;
                (yyval.expr) = get_expr_with_item(Yacc, &elem);

                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype       = SPT_FUNCTION;
                elem.u.func.type    = SFT_COPYFROM;
                elem.u.func.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (6)].expr), (yyval.expr), &elem);
            }
    break;

  case 462:

/* Line 1806 of yacc.c  */
#line 5345 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype       = SPT_FUNCTION;
                elem.u.func.type    = SFT_COPYTO;
                elem.u.func.argcnt  = 2;
                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr), &elem);
            }
    break;

  case 463:

/* Line 1806 of yacc.c  */
#line 5356 "parser.y"
    {
                (yyval.strval) = "char";
            }
    break;

  case 464:

/* Line 1806 of yacc.c  */
#line 5360 "parser.y"
    {
                (yyval.strval) = "varchar";
            }
    break;

  case 465:

/* Line 1806 of yacc.c  */
#line 5364 "parser.y"
    {
                (yyval.strval) = "byte";
            }
    break;

  case 466:

/* Line 1806 of yacc.c  */
#line 5368 "parser.y"
    {
                (yyval.strval) = "varbyte";
            }
    break;

  case 467:

/* Line 1806 of yacc.c  */
#line 5375 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_RANDOM;
                elem.u.func.argcnt = 0;

                (yyval.expr) = get_expr_with_item(Yacc, &elem);
            }
    break;

  case 468:

/* Line 1806 of yacc.c  */
#line 5384 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_RANDOM;
                elem.u.func.argcnt = 0;

                (yyval.expr) = get_expr_with_item(Yacc, &elem);
            }
    break;

  case 469:

/* Line 1806 of yacc.c  */
#line 5393 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));

                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_BIGINT;
                elem.u.value.u.i64 = (yyvsp[(3) - (4)].intval);
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;

                calc_func_srandom(&elem.u.value, NULL, 0);

                (yyval.expr) = get_expr_with_item(Yacc, &elem);
            }
    break;

  case 470:

/* Line 1806 of yacc.c  */
#line 5410 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype      = SPT_FUNCTION;
                elem.u.func.type   = SFT_OBJECTSIZE;
                elem.u.func.argcnt = 2;

                (yyval.expr) = merge_expression(Yacc, (yyvsp[(3) - (6)].expr), (yyvsp[(5) - (6)].expr), &elem);
            }
    break;

  case 471:

/* Line 1806 of yacc.c  */
#line 5422 "parser.y"
    { 
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_INTEGER;
                elem.u.value.u.i32      = OBJ_TABLE,
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;
                (yyval.expr) = get_expr_with_item(Yacc, &elem);
            }
    break;

  case 472:

/* Line 1806 of yacc.c  */
#line 5432 "parser.y"
    { 
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_INTEGER;
                elem.u.value.u.i32      = OBJ_TABLEDATA,
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;
                (yyval.expr) = get_expr_with_item(Yacc, &elem);
            }
    break;

  case 473:

/* Line 1806 of yacc.c  */
#line 5442 "parser.y"
    { 
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_CONSTANT;
                elem.u.value.valuetype  = DT_INTEGER;
                elem.u.value.u.i32      = OBJ_TABLEINDEX,
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;
                (yyval.expr) = get_expr_with_item(Yacc, &elem);
            }
    break;

  case 474:

/* Line 1806 of yacc.c  */
#line 5456 "parser.y"
    {
                sc_memset(&(yyval.elem), 0, sizeof(T_POSTFIXELEM));
                (yyval.elem).elemtype = SPT_VALUE;
                (yyval.elem).u.value.valueclass = SVC_CONSTANT;
                (yyval.elem).u.value.valuetype  = DT_DOUBLE;
                (yyval.elem).u.value.u.f32      = (yyvsp[(1) - (2)].num) * (yyvsp[(2) - (2)].floatval);
                (yyval.elem).u.value.attrdesc.collation = MDB_COL_NUMERIC;
            }
    break;

  case 475:

/* Line 1806 of yacc.c  */
#line 5465 "parser.y"
    {
                sc_memset(&(yyval.elem), 0, sizeof(T_POSTFIXELEM));
                (yyval.elem).elemtype = SPT_VALUE;
                (yyval.elem).u.value.valueclass = SVC_CONSTANT;
                (yyval.elem).u.value.valuetype  = DT_DOUBLE;
                (yyval.elem).u.value.u.f32      = (yyvsp[(1) - (2)].num) * (yyvsp[(2) - (2)].floatval);
                (yyval.elem).u.value.attrdesc.collation = MDB_COL_NUMERIC;
            }
    break;

  case 476:

/* Line 1806 of yacc.c  */
#line 5474 "parser.y"
    {
                sc_memset(&(yyval.elem), 0, sizeof(T_POSTFIXELEM));
                (yyval.elem).elemtype = SPT_VALUE;
                (yyval.elem).u.value.valueclass = SVC_CONSTANT;
                (yyval.elem).u.value.valuetype  = DT_BIGINT;
                (yyval.elem).u.value.u.i64      = (yyvsp[(1) - (2)].num) * (yyvsp[(2) - (2)].intval);
                (yyval.elem).u.value.attrdesc.collation = MDB_COL_NUMERIC;
            }
    break;

  case 477:

/* Line 1806 of yacc.c  */
#line 5483 "parser.y"
    {
                sc_memset(&(yyval.elem), 0, sizeof(T_POSTFIXELEM));
                (yyval.elem).elemtype = SPT_VALUE;
                (yyval.elem).u.value.valueclass   = SVC_CONSTANT;
                (yyval.elem).u.value.valuetype    = DT_VARCHAR;
                (yyval.elem).u.value.u.ptr        = (yyvsp[(1) - (1)].strval);
                (yyval.elem).u.value.attrdesc.len = sc_strlen((yyvsp[(1) - (1)].strval));
                (yyval.elem).u.value.value_len = sc_strlen((yyvsp[(1) - (1)].strval));
                (yyval.elem).u.value.attrdesc.collation = DB_Params.default_col_char;

            }
    break;

  case 478:

/* Line 1806 of yacc.c  */
#line 5495 "parser.y"
    {
                sc_memset(&(yyval.elem), 0, sizeof(T_POSTFIXELEM));
                (yyval.elem).elemtype = SPT_VALUE;
                (yyval.elem).u.value.valueclass   = SVC_CONSTANT;
                (yyval.elem).u.value.valuetype    =  DT_NVARCHAR;
                (yyval.elem).u.value.u.Wptr       = (DB_WCHAR*)(yyvsp[(1) - (1)].strval);
                (yyval.elem).u.value.attrdesc.len = sc_wcslen((DB_WCHAR*)(yyvsp[(1) - (1)].strval));
                (yyval.elem).u.value.value_len = sc_wcslen((DB_WCHAR*)(yyvsp[(1) - (1)].strval));
                (yyval.elem).u.value.attrdesc.collation = DB_Params.default_col_nchar;

            }
    break;

  case 479:

/* Line 1806 of yacc.c  */
#line 5507 "parser.y"
    {   
                int     len = sc_strlen((yyvsp[(1) - (1)].strval));
                char    *ptr= 0;

                if ((len % 8) != 0)
                    __yyerror("Invalid Binary Value");

                len = len / 8;
                ptr = sql_mem_calloc(vSQL(Yacc)->parsing_memory, len + 1);
                len = sc_bstring2binary((yyvsp[(1) - (1)].strval), ptr);
                sql_mem_freenul(vSQL(Yacc)->parsing_memory, (yyvsp[(1) - (1)].strval));

                sc_memset(&(yyval.elem), 0, sizeof(T_POSTFIXELEM));
                (yyval.elem).elemtype = SPT_VALUE;
                (yyval.elem).u.value.valueclass   = SVC_CONSTANT;
                (yyval.elem).u.value.valuetype    = DT_VARBYTE;
                (yyval.elem).u.value.u.ptr        = ptr;
                (yyval.elem).u.value.attrdesc.len = len;  // binary string'len
                (yyval.elem).u.value.value_len = len;
                (yyval.elem).u.value.attrdesc.collation = MDB_COL_DEFAULT_BYTE;
            }
    break;

  case 480:

/* Line 1806 of yacc.c  */
#line 5529 "parser.y"
    {
                int     len = sc_strlen((yyvsp[(1) - (1)].strval));
                char    *ptr= 0;

                if ((len % 2) != 0)
                    __yyerror("Invalid Hexadecimal Value");

                len = len / 2;
                ptr = sql_mem_calloc(vSQL(Yacc)->parsing_memory, len + 1);
                len = sc_hstring2binary((yyvsp[(1) - (1)].strval), ptr);
                sql_mem_freenul(vSQL(Yacc)->parsing_memory, (yyvsp[(1) - (1)].strval));

                sc_memset(&(yyval.elem), 0, sizeof(T_POSTFIXELEM));
                (yyval.elem).elemtype = SPT_VALUE;
                (yyval.elem).u.value.valueclass   = SVC_CONSTANT;
                (yyval.elem).u.value.valuetype    = DT_VARBYTE;
                (yyval.elem).u.value.u.ptr        = ptr;
                (yyval.elem).u.value.attrdesc.len = len;  // binary string'len
                (yyval.elem).u.value.value_len = len;
                (yyval.elem).u.value.attrdesc.collation = MDB_COL_DEFAULT_BYTE;
            }
    break;

  case 481:

/* Line 1806 of yacc.c  */
#line 5551 "parser.y"
    {
                sc_memset(&(yyval.elem), 0, sizeof(T_POSTFIXELEM));
                (yyval.elem).elemtype            = SPT_VALUE;
                (yyval.elem).u.value.valueclass   = SVC_CONSTANT;
                (yyval.elem).u.value.valuetype   = DT_NULL_TYPE;
                (yyval.elem).u.value.is_null     = 1;
            }
    break;

  case 482:

/* Line 1806 of yacc.c  */
#line 5562 "parser.y"
    {
        (yyval.num) = 1;
    }
    break;

  case 483:

/* Line 1806 of yacc.c  */
#line 5566 "parser.y"
    {
        (yyval.num) = -1;
    }
    break;

  case 484:

/* Line 1806 of yacc.c  */
#line 5573 "parser.y"
    {
                __CHECK_TABLE_NAME_LENG((yyvsp[(1) - (1)].strval));
                (yyval.strval) = (yyvsp[(1) - (1)].strval);
            }
    break;

  case 485:

/* Line 1806 of yacc.c  */
#line 5578 "parser.y"
    {
                __CHECK_TABLE_NAME_LENG((yyvsp[(3) - (3)].strval));
                (yyval.strval) = (yyvsp[(3) - (3)].strval); 
            }
    break;

  case 486:

/* Line 1806 of yacc.c  */
#line 5586 "parser.y"
    {
                __CHECK_VIEW_NAME_LENG((yyvsp[(1) - (1)].strval));
                (yyval.strval) = (yyvsp[(1) - (1)].strval);
            }
    break;

  case 487:

/* Line 1806 of yacc.c  */
#line 5594 "parser.y"
    {
                __CHECK_ALIAS_NAME_LENG((yyvsp[(1) - (1)].strval));
                (yyval.strval) = (yyvsp[(1) - (1)].strval);
            }
    break;

  case 488:

/* Line 1806 of yacc.c  */
#line 5602 "parser.y"
    {
                {
                    sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));
                    elem.elemtype = SPT_VALUE;
                    elem.u.value.valueclass = SVC_VARIABLE;

                    elem.u.value.valuetype  = DT_NULL_TYPE;

                    p = sc_strchr((yyvsp[(1) - (1)].strval), '.');
                    if (p == NULL) {
                        elem.u.value.attrdesc.table.tablename = NULL;
                        elem.u.value.attrdesc.table.aliasname = NULL;
                        elem.u.value.attrdesc.attrname  = (yyvsp[(1) - (1)].strval);
                    }

                    else {
                        *p++ = '\0';
                        elem.u.value.attrdesc.table.tablename = 
                                                    YACC_XSTRDUP(Yacc, (yyvsp[(1) - (1)].strval));
                        elem.u.value.attrdesc.table.aliasname = NULL;
                        elem.u.value.attrdesc.attrname  = YACC_XSTRDUP(Yacc, p);
                        isnull(elem.u.value.attrdesc.table.tablename);
                        isnull(elem.u.value.attrdesc.attrname);
                    }
                }

                (yyval.expr) = get_expr_with_item(Yacc, &elem);
            }
    break;

  case 489:

/* Line 1806 of yacc.c  */
#line 5631 "parser.y"
    {
                sc_memset(&elem, 0, sizeof(T_POSTFIXELEM));

                elem.elemtype = SPT_VALUE;
                elem.u.value.valueclass = SVC_VARIABLE;

                elem.u.value.valuetype  = DT_NULL_TYPE;

                p = sc_strchr((yyvsp[(1) - (1)].strval), '.');
                if (p == NULL) {
                    elem.u.value.attrdesc.table.tablename = NULL;
                    elem.u.value.attrdesc.table.aliasname = NULL;
                    elem.u.value.attrdesc.attrname  = (yyvsp[(1) - (1)].strval);
                }
                else {
                    *p++ = '\0';
                    elem.u.value.attrdesc.table.tablename = 
                                                YACC_XSTRDUP(Yacc, (yyvsp[(1) - (1)].strval));
                    elem.u.value.attrdesc.table.aliasname = NULL;
                    elem.u.value.attrdesc.attrname  = YACC_XSTRDUP(Yacc, p);
                    isnull(elem.u.value.attrdesc.table.tablename);
                    isnull(elem.u.value.attrdesc.attrname);
                }
                elem.u.value.attrdesc.type = DT_OID;
                elem.u.value.attrdesc.Hattr = -1;
                elem.u.value.attrdesc.collation = MDB_COL_NUMERIC;

                (yyval.expr) = get_expr_with_item(Yacc, &elem);
            }
    break;

  case 490:

/* Line 1806 of yacc.c  */
#line 5664 "parser.y"
    {
                (yyval.strval) = (yyvsp[(1) - (1)].strval);
            }
    break;

  case 491:

/* Line 1806 of yacc.c  */
#line 5668 "parser.y"
    {
                __CHECK_TABLE_NAME_LENG((yyvsp[(1) - (3)].strval));
                sc_snprintf(buf, MDB_BUFSIZ, "%s.%s", (yyvsp[(1) - (3)].strval), (yyvsp[(3) - (3)].strval));
                (yyval.strval) = YACC_XSTRDUP(Yacc, buf);
                isnull((yyval.strval));
            }
    break;

  case 492:

/* Line 1806 of yacc.c  */
#line 5675 "parser.y"
    {
                INCOMPLETION("OWNERSHIP");
            }
    break;

  case 493:

/* Line 1806 of yacc.c  */
#line 5682 "parser.y"
    {
                __CHECK_TABLE_NAME_LENG((yyvsp[(1) - (3)].strval));
                sc_snprintf(buf, MDB_BUFSIZ, "%s.%s", (yyvsp[(1) - (3)].strval), "rid");
                (yyval.strval) = YACC_XSTRDUP(Yacc, buf);
                isnull((yyval.strval));
            }
    break;

  case 494:

/* Line 1806 of yacc.c  */
#line 5689 "parser.y"
    {
                sc_snprintf(buf, MDB_BUFSIZ, "%s", "rid");
                (yyval.strval) = YACC_XSTRDUP(Yacc, buf);
                isnull((yyval.strval));
            }
    break;

  case 495:

/* Line 1806 of yacc.c  */
#line 5700 "parser.y"
    {
                __CHECK_FIELD_NAME_LENG((yyvsp[(1) - (1)].strval));
                (yyval.strval) = (yyvsp[(1) - (1)].strval);
            }
    break;

  case 496:

/* Line 1806 of yacc.c  */
#line 5708 "parser.y"
    {
                __CHECK_INDEX_NAME_LENG((yyvsp[(1) - (1)].strval));
                (yyval.strval) = (yyvsp[(1) - (1)].strval);
            }
    break;

  case 497:

/* Line 1806 of yacc.c  */
#line 5716 "parser.y"
    {
                __CHECK_SEQUENCE_NAME_LENG((yyvsp[(1) - (1)].strval));
                (yyval.strval) = (yyvsp[(1) - (1)].strval);
            }
    break;

  case 500:

/* Line 1806 of yacc.c  */
#line 5729 "parser.y"
    {
            vSQL(Yacc)->sqltype = ST_ADMIN;
            vADMIN(Yacc) = &vSQL(Yacc)->u._admin;
            Yacc->userdefined = 0;
            Yacc->param_count = 0;
 
            vADMIN(Yacc)->u.export.data_or_schema = (yyvsp[(2) - (6)].intval);
            if ((yyvsp[(3) - (6)].strval) == NULL)
                vADMIN(Yacc)->type  = SADM_EXPORT_ALL;
            else
                vADMIN(Yacc)->type  = SADM_EXPORT_ONE;
            vADMIN(Yacc)->u.export.table_name   = (yyvsp[(3) - (6)].strval);
            vADMIN(Yacc)->u.export.export_file  = (yyvsp[(5) - (6)].strval);
            vADMIN(Yacc)->u.export.f_append     = (yyvsp[(6) - (6)].intval);
        }
    break;

  case 501:

/* Line 1806 of yacc.c  */
#line 5745 "parser.y"
    {
            vSQL(Yacc)->sqltype = ST_ADMIN;
            vADMIN(Yacc) = &vSQL(Yacc)->u._admin;
            Yacc->userdefined = 0;
            Yacc->param_count = 0;

            vADMIN(Yacc)->type  = SADM_IMPORT;
            vADMIN(Yacc)->u.import.import_file = (yyvsp[(3) - (3)].strval);
        }
    break;

  case 502:

/* Line 1806 of yacc.c  */
#line 5758 "parser.y"
    {
                (yyval.intval) = 0;
            }
    break;

  case 503:

/* Line 1806 of yacc.c  */
#line 5762 "parser.y"
    {
                (yyval.intval) = 1;
            }
    break;

  case 504:

/* Line 1806 of yacc.c  */
#line 5769 "parser.y"
    {
                (yyval.strval) = NULL;
            }
    break;

  case 505:

/* Line 1806 of yacc.c  */
#line 5773 "parser.y"
    {
                (yyval.strval) = (yyvsp[(2) - (2)].strval);
            }
    break;

  case 506:

/* Line 1806 of yacc.c  */
#line 5780 "parser.y"
    {
                (yyval.intval) = 0;
            }
    break;

  case 507:

/* Line 1806 of yacc.c  */
#line 5784 "parser.y"
    {
                (yyval.intval) = 0;
            }
    break;

  case 508:

/* Line 1806 of yacc.c  */
#line 5788 "parser.y"
    {
                (yyval.intval) = 1;
            }
    break;



/* Line 1806 of yacc.c  */
#line 10189 "sql_y.tab.c"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 2067 of yacc.c  */
#line 5793 "parser.y"


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

