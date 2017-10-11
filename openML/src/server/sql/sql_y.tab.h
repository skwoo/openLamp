/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
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

/* Line 2068 of yacc.c  */
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



/* Line 2068 of yacc.c  */
#line 479 "sql_y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif




