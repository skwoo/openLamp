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
#include "mdb_compat.h"

#include "mdb_ppthread.h"

#include "isql.h"
#include "sql_datast.h"
#include "sql_parser.h"
#include "sql_util.h"
#include "ErrorCode.h"

static int lex_errno;

char *Yacc_Xstrdup(T_YACC *yacc, char *s);
#define YACC_XSTRDUP(a,b)     Yacc_Xstrdup(a, b)

#ifdef ECHO
#undef ECHO
#endif
#define ECHO {}

#ifdef YY_FATAL_ERROR
#undef YY_FATAL_ERROR
#endif
#define YY_FATAL_ERROR(msg) {}

%}

%{

#include "sql_y.tab.h"

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

/*
 * Change default declaration of generated scanner - yylex()
 * For multi-thread safe application
 */
#ifdef YY_DECL
#undef YY_DECL
#endif // YY_DECL
#define YY_DECL static int yylex YY_PROTO((YYSTYPE *yylval_param, yyscan_t yyscanner, void *__yacc))

#ifndef YY_USE_PROTOS
#ifdef YY_PROTO
#undef YY_PROTO
#endif
#define YY_USE_PROTOS
#define YY_PROTO(proto) proto
#endif

#undef YY_INPUT
#define YY_INPUT(buf, result, max)    (result = my_yyinput(yyg->yyextra_r, buf, max))

#define MY_YY_BUF_SIZE    iSQL_QUERY_MAX

#define PUT(A,B,C,D)    put_token((A),(B),(C)); if ((D)) yyless((C))

static int my_yyinput(T_YACC *yacc, char *buf, int max_size);
static void put_token(T_YACC *yacc, char *token, int len);

extern void my_yy_delete_buffer(void *);

static MDB_UINT64 yacc_atoull(char *str);

%}

%option noyywrap
%option noyylineno
%option reentrant

white               [\t ]+
nl                  (\n)|(\r\n)

digit               [0-9]
letter              [a-zA-Z]
korean1             [\xa0-\xc8]
korean2             [\xa1-\xfe]
korean              ({korean1}{korean2})+

integer             {digit}+
float               ({digit}+"."{digit}*)|("."{digit}+)
exponant            [eE][-+]?{integer}
approximate         ({integer}|{float}){exponant}

identifier          ("_"|{letter}|{korean})+("_"|{letter}|{korean}|{digit})*

delimiter           ({white}|{nl})*
function            {delimiter}"("[^)]*")"        


%%

"--".*$             ;    /* comment */

{white}             { PUT(Yacc, yytext, yyleng, 0); }

{nl}                {
                        PUT(Yacc, yytext, yyleng, 0);
                        if (Yacc->end_of_query) {
                            Yacc->end_of_query    = 0;
                            Yacc->my_yylineno        = 1;
                            return yytext[0];
                        } else Yacc->my_yylineno++;
                    }

    /* literal keyword tokens */

ADD                     { PUT(Yacc, yytext, yyleng, 0); return ADD_SYM; }
ALL                     { PUT(Yacc, yytext, yyleng, 0); return ALL_SYM; }
ALTER                   { PUT(Yacc, yytext, yyleng, 0); return ALTER_SYM; }
AND                     { PUT(Yacc, yytext, yyleng, 0); return AND_SYM; }
ANY                     { PUT(Yacc, yytext, yyleng, 0); return ANY_SYM; }
AS                      { PUT(Yacc, yytext, yyleng, 0); return AS_SYM; }
ASC                     { PUT(Yacc, yytext, yyleng, 0); return ASC_SYM; }
AUTOCOMMIT              { PUT(Yacc, yytext, yyleng, 0); return AUTOCOMMIT_SYM; }
AVG{function}           { PUT(Yacc, yytext, 3, 1); return AVG_SYM; }
BETWEEN                 { PUT(Yacc, yytext, yyleng, 0); return BETWEEN_SYM; }
BIGINT(EGER)?           { PUT(Yacc, yytext, yyleng, 0); return BIGINT_SYM; }
BY                      { PUT(Yacc, yytext, yyleng, 0); return BY_SYM; }
BYTES                   { PUT(Yacc, yytext, yyleng, 0); return BYTES_SYM; }
NBYTES                  { PUT(Yacc, yytext, yyleng, 0); return NBYTES_SYM; }
CHAR(ACTER)?            { PUT(Yacc, yytext, yyleng, 0); return CHARACTER_SYM; }
NCHAR(ACTER)?           { PUT(Yacc, yytext, yyleng, 0); return NCHARACTER_SYM; }
CHECK                   { PUT(Yacc, yytext, yyleng, 0); return CHECK_SYM; }
CLOSE                   { PUT(Yacc, yytext, yyleng, 0); return CLOSE_SYM; }
COMMIT                  { PUT(Yacc, yytext, yyleng, 0); return COMMIT_SYM; }
FLUSH                   { PUT(Yacc, yytext, yyleng, 0); return FLUSH_SYM; }
COUNT{function}         { PUT(Yacc, yytext, 5, 1); return COUNT_SYM; }
CONVERT{function}       { PUT(Yacc, yytext, 7, 1); return CONVERT_SYM; }
CREATE                  { PUT(Yacc, yytext, yyleng, 0); return CREATE_SYM; }
CROSS                   { PUT(Yacc, yytext, yyleng, 0); return CROSS_SYM; }
CURRENT                 { PUT(Yacc, yytext, yyleng, 0); return CURRENT_SYM; }
CURRENT_DATE            { PUT(Yacc, yytext, yyleng, 0); return CURRENT_DATE_SYM; }
CURRENT_TIME            { PUT(Yacc, yytext, yyleng, 0); return CURRENT_TIME_SYM; }
CURRENT_TIMESTAMP       { PUT(Yacc, yytext, yyleng, 0); return CURRENT_TIMESTAMP_SYM; }
DATE                    { PUT(Yacc, yytext, yyleng, 0); return DATE_SYM; }
DATE_ADD{function}      { PUT(Yacc, yytext, 8, 1); return DATE_ADD_SYM; }
DATE_DIFF{function}     { PUT(Yacc, yytext, 9, 1); return DATE_DIFF_SYM; }
DATE_FORMAT{function}   { PUT(Yacc, yytext, 11, 1); return DATE_FORMAT_SYM; }
DATE_SUB{function}      { PUT(Yacc, yytext, 8, 1); return DATE_SUB_SYM; }
TIME_ADD{function}      { PUT(Yacc, yytext, 8, 1); return TIME_ADD_SYM; }
TIME_DIFF{function}     { PUT(Yacc, yytext, 9, 1); return TIME_DIFF_SYM; }
TIME_FORMAT{function}   { PUT(Yacc, yytext, 11, 1); return TIME_FORMAT_SYM; }
TIME_SUB{function}      { PUT(Yacc, yytext, 8, 1); return TIME_SUB_SYM; }
DATETIME                { PUT(Yacc, yytext, yyleng, 0); return DATETIME_SYM; }
DEC(IMAL)?              { PUT(Yacc, yytext, yyleng, 0); return DECIMAL_SYM; }
DECODE{function}        { PUT(Yacc, yytext, 6, 1); return DECODE_SYM; }
DEFAULT                 { PUT(Yacc, yytext, yyleng, 0); return DEFAULT_SYM; }
DELETE                  { PUT(Yacc, yytext, yyleng, 0); return DELETE_SYM; }
DESC                    { PUT(Yacc, yytext, yyleng, 0); return DESC_SYM; }
DESCRIBE                { PUT(Yacc, yytext, yyleng, 0); return DESCRIBE_SYM; }
DISTINCT                { PUT(Yacc, yytext, yyleng, 0); return DISTINCT_SYM; }
DOUBLE                  { PUT(Yacc, yytext, yyleng, 0); return DOUBLE_SYM; }
DROP                    { PUT(Yacc, yytext, yyleng, 0); return DROP_SYM; }
ESCAPE                  { PUT(Yacc, yytext, yyleng, 0); return ESCAPE_SYM; }
EXISTS                  { PUT(Yacc, yytext, yyleng, 0); return EXISTS_SYM; }
EXPLAIN                 { PUT(Yacc, yytext, yyleng, 0); return EXPLAIN_SYM; }
FEEDBACK                { PUT(Yacc, yytext, yyleng, 0); return FEEDBACK_SYM; }
FLOAT                   { PUT(Yacc, yytext, yyleng, 0); return FLOAT_SYM; }
FOREIGN                 { PUT(Yacc, yytext, yyleng, 0); return FOREIGN_SYM; }
FROM                    { PUT(Yacc, yytext, yyleng, 0); return FROM_SYM; }
FULL                    { PUT(Yacc, yytext, yyleng, 0); return FULL_SYM; }
GROUP                   { PUT(Yacc, yytext, yyleng, 0); return GROUP_SYM; }
HAVING                  { PUT(Yacc, yytext, yyleng, 0); return HAVING_SYM; }
HEADING                 { PUT(Yacc, yytext, yyleng, 0); return HEADING_SYM; }
HEXCOMP{function}       { PUT(Yacc, yytext, 7, 1); return HEXCOMP_SYM; }
CHARTOHEX{function}     { PUT(Yacc, yytext, 9, 1); return CHARTOHEX_SYM; }
IFNULL{function}        { PUT(Yacc, yytext, 6, 1); return IFNULL_SYM; }
IN                      { PUT(Yacc, yytext, yyleng, 0); return IN_SYM; }
INDEX                   { PUT(Yacc, yytext, yyleng, 0); return INDEX_SYM; }
INNER                   { PUT(Yacc, yytext, yyleng, 0); return INNER_SYM; }
INSERT                  { PUT(Yacc, yytext, yyleng, 0); return INSERT_SYM; }
INT(EGER)?              { PUT(Yacc, yytext, yyleng, 0); return INT_SYM; }
INTO                    { PUT(Yacc, yytext, yyleng, 0); return INTO_SYM; }
IS                      { PUT(Yacc, yytext, yyleng, 0); return IS_SYM; }
ISCAN                   { PUT(Yacc, yytext, yyleng, 0); return ISCAN_SYM; }
JOIN                    { PUT(Yacc, yytext, yyleng, 0); return JOIN_SYM; }
KEY                     { PUT(Yacc, yytext, yyleng, 0); return KEY_SYM; }
LEFT                    { PUT(Yacc, yytext, yyleng, 0); return LEFT_SYM; }
LIKE                    { PUT(Yacc, yytext, yyleng, 0); return LIKE_SYM; }
ILIKE                    { PUT(Yacc, yytext, yyleng, 0); return ILIKE_SYM; }
LIMIT                   { PUT(Yacc, yytext, yyleng, 0); return LIMIT_SYM; }
LOWER{function}         { PUT(Yacc, yytext, 5, 1); return LOWERCASE_SYM; }
LOWERCASE{function}     { PUT(Yacc, yytext, 9, 1); return LOWERCASE_SYM; }
LTRIM{function}         { PUT(Yacc, yytext, 5, 1); return LTRIM_SYM; }
MAX{function}           { PUT(Yacc, yytext, 3, 1); return MAX_SYM; }
MIN{function}           { PUT(Yacc, yytext, 3, 1); return MIN_SYM; }
LOGGING                 { PUT(Yacc, yytext, yyleng, 0); return LOGGING_SYM; }
NOLOGGING               { PUT(Yacc, yytext, yyleng, 0); return NOLOGGING_SYM; }
NOT                     { PUT(Yacc, yytext, yyleng, 0); return NOT_SYM; }
NOW{function}           { PUT(Yacc, yytext, 3, 1); return NOW_SYM; }
NULL                    { PUT(Yacc, yytext, yyleng, 0); return NULL_SYM; }
NUM(ERIC)?              { PUT(Yacc, yytext, yyleng, 0); return NUMERIC_SYM; }
OFF                     { PUT(Yacc, yytext, yyleng, 0); return OFF_SYM; }
ON                      { PUT(Yacc, yytext, yyleng, 0); return ON_SYM; }
ONLY                    { PUT(Yacc, yytext, yyleng, 0); return ONLY_SYM; }
OPTION_SYM              { PUT(Yacc, yytext, yyleng, 0); return OPTION_SYM; }
OR                      { PUT(Yacc, yytext, yyleng, 0); return OR_SYM; }
ORDER                   { PUT(Yacc, yytext, yyleng, 0); return ORDER_SYM; }
OUTER                   { PUT(Yacc, yytext, yyleng, 0); return OUTER_SYM; }
PLAN                    { PUT(Yacc, yytext, yyleng, 0); return PLAN_SYM; }
PRIMARY                 { PUT(Yacc, yytext, yyleng, 0); return PRIMARY_SYM; }
REAL                    { PUT(Yacc, yytext, yyleng, 0); return REAL_SYM; }
RECONNECT               { PUT(Yacc, yytext, yyleng, 0); return RECONNECT_SYM; }
REFERENCES              { PUT(Yacc, yytext, yyleng, 0); return REFERENCES_SYM; }
RENAME                  { PUT(Yacc, yytext, yyleng, 0); return RENAME_SYM; }
RIGHT                   { PUT(Yacc, yytext, yyleng, 0); return RIGHT_SYM; }
ROLLBACK                { PUT(Yacc, yytext, yyleng, 0); return ROLLBACK_SYM; }
ROUND{function}         { PUT(Yacc, yytext, 5, 1); return ROUND_SYM; }
RTRIM{function}         { PUT(Yacc, yytext, 5, 1); return RTRIM_SYM; }
SELECT                  { PUT(Yacc, yytext, yyleng, 0); return SELECT_SYM; }
SET                     { PUT(Yacc, yytext, yyleng, 0); return SET_SYM; }
SMALLINT(EGER)?         { PUT(Yacc, yytext, yyleng, 0); return SMALLINT_SYM; }
SIGN{function}          { PUT(Yacc, yytext, 4, 1); return SIGN_SYM; }
SOME                    { PUT(Yacc, yytext, yyleng, 0); return SOME_SYM; }
SSCAN                   { PUT(Yacc, yytext, yyleng, 0); return SSCAN_SYM; }
STRING                  { PUT(Yacc, yytext, yyleng, 0); return STRING_SYM; }
NSTRING                 { PUT(Yacc, yytext, yyleng, 0); return NSTRING_SYM; }
SUBSTR{function}        { PUT(Yacc, yytext, 6, 1); return SUBSTRING_SYM; }
SUBSTRING{function}     { PUT(Yacc, yytext, 9, 1); return SUBSTRING_SYM; }
SUM{function}           { PUT(Yacc, yytext, 3, 1); return SUM_SYM; }
SYSDATE                 { PUT(Yacc, yytext, yyleng, 0); return SYSDATE_SYM; }
TABLE                   { PUT(Yacc, yytext, yyleng, 0); return TABLE_SYM; }
RIDTABLENAME            { PUT(Yacc, yytext, yyleng, 0); return RIDTABLENAME_SYM; }
TIME                    { PUT(Yacc, yytext, yyleng, 0); return TIME_SYM; }
TIMESTAMP               { PUT(Yacc, yytext, yyleng, 0); return TIMESTAMP_SYM; }
TINYINT(EGER)?          { PUT(Yacc, yytext, yyleng, 0); return TINYINT_SYM; }
TRUNC{function}         { PUT(Yacc, yytext, 5, 1); return TRUNC_SYM; }
TRUNCATE                { PUT(Yacc, yytext, yyleng, 0); return TRUNCATE_SYM; }
UNIQUE                  { PUT(Yacc, yytext, yyleng, 0); return UNIQUE_SYM; }
UPDATE                  { PUT(Yacc, yytext, yyleng, 0); return UPDATE_SYM; }
UPPER{function}         { PUT(Yacc, yytext, 5, 1); return UPPERCASE_SYM; }
UPPERCASE{function}     { PUT(Yacc, yytext, 9, 1); return UPPERCASE_SYM; }
VALUES                  { PUT(Yacc, yytext, yyleng, 0); return VALUES_SYM; }
VARCHAR                 { PUT(Yacc, yytext, yyleng, 0); return VARCHAR_SYM; }
NVARCHAR                { PUT(Yacc, yytext, yyleng, 0); return NVARCHAR_SYM; }
VIEW                    { PUT(Yacc, yytext, yyleng, 0); return VIEW_SYM; }
WHERE                   { PUT(Yacc, yytext, yyleng, 0); return WHERE_SYM; }
WORK                    { PUT(Yacc, yytext, yyleng, 0); return WORK_SYM; }
RANDOM                  { PUT(Yacc, yytext, yyleng, 0); return RANDOM_SYM; }
SRANDOM                 { PUT(Yacc, yytext, yyleng, 0); return SRANDOM_SYM; }
END                     { PUT(Yacc, yytext, yyleng, 0); return END_SYM; }
USING                   { PUT(Yacc, yytext, yyleng, 0); return USING_SYM; }
DIRTY_COUNT             { PUT(Yacc, yytext, yyleng, 0); return DIRTY_COUNT_SYM; }
ROWNUM                  { PUT(Yacc, yytext, yyleng, 0); return ROWNUM_SYM; }

SEQUENCE                { PUT(Yacc, yytext, yyleng, 0); return SEQUENCE_SYM; }
START                    { PUT(Yacc, yytext, yyleng, 0); return START_SYM; }
WITH                    { PUT(Yacc, yytext, yyleng, 0); return WITH_SYM; }
INCREMENT                { PUT(Yacc, yytext, yyleng, 0); return INCREMENT_SYM; }
MAXVALUE                { PUT(Yacc, yytext, yyleng, 0); return MAXVALUE_SYM; }
NOMAXVALUE                { PUT(Yacc, yytext, yyleng, 0); return NOMAXVALUE_SYM; }
MINVALUE                { PUT(Yacc, yytext, yyleng, 0); return MINVALUE_SYM; }
NOMINVALUE                { PUT(Yacc, yytext, yyleng, 0); return NOMINVALUE_SYM; }
CYCLE                    { PUT(Yacc, yytext, yyleng, 0); return CYCLE_SYM; }
NOCYCLE                    { PUT(Yacc, yytext, yyleng, 0); return NOCYCLE_SYM; }
CURRVAL                    { PUT(Yacc, yytext, yyleng, 0); return CURRVAL_SYM; }
NEXTVAL                 { PUT(Yacc, yytext, yyleng, 0); return NEXTVAL_SYM; }

RID                      { PUT(Yacc, yytext, yyleng, 0); return RID_SYM; }

REPLACE                 { PUT(Yacc, yytext, yyleng, 0); return REPLACE_SYM; }
"@"                     { PUT(Yacc, yytext, yyleng, 0); return AT_SYM; }

BYTE                    { PUT(Yacc, yytext, yyleng, 0); return BYTE_SYM; }
VARBYTE                 { PUT(Yacc, yytext, yyleng, 0); return VARBYTE_SYM; }
BINARY                  { PUT(Yacc, yytext, yyleng, 0); return BINARY_SYM; }
HEXADECIMAL             { PUT(Yacc, yytext, yyleng, 0); return HEXADECIMAL_SYM; }
BYTE_SIZE               { PUT(Yacc, yytext, yyleng, 0); return BYTE_SIZE_SYM; }
COPYFROM                { PUT(Yacc, yytext, yyleng, 0); return COPYFROM_SYM; }
COPYTO                  { PUT(Yacc, yytext, yyleng, 0); return COPYTO_SYM; }


OBJECTSIZE              { PUT(Yacc, yytext, yyleng, 0); return OBJECTSIZE_SYM; }
TABLEDATA               { PUT(Yacc, yytext, yyleng, 0); return TABLEDATA_SYM; }
TABLEINDEX              { PUT(Yacc, yytext, yyleng, 0); return TABLEINDEX_SYM; }

COLLATION               { PUT(Yacc, yytext, yyleng, 0); return COLLATION_SYM; }
COLLATE                 { PUT(Yacc, yytext, yyleng, 0); return COLLATION_SYM; }

REBUILD                 { PUT(Yacc, yytext, yyleng, 0); return REBUILD_SYM; }

EXPORT                  { PUT(Yacc, yytext, yyleng, 0); return EXPORT_SYM; }
IMPORT                  { PUT(Yacc, yytext, yyleng, 0); return IMPORT_SYM; }

HEXASTRING              { PUT(Yacc, yytext, 10, 1); return HEXASTRING_SYM; }
BINARYSTRING            { PUT(Yacc, yytext, 12, 1); return BINARYSTRING_SYM; }

RUNTIME                 { PUT(Yacc, yytext, yyleng, 0); return RUNTIME_SYM; }
SHOW                    { PUT(Yacc, yytext, 4, 1); return SHOW_SYM; }

UPSERT                  { PUT(Yacc, yytext, yyleng, 0); return UPSERT_SYM; }

DATA                    { PUT(Yacc, yytext, yyleng, 0); return DATA_SYM; } 
SCHEMA                  { PUT(Yacc, yytext, yyleng, 0); return SCHEMA_SYM; }

OID                     { PUT(Yacc, yytext, yyleng, 0); return OID_SYM; }

BEFORE                  { PUT(Yacc, yytext, yyleng, 0); return BEFORE_SYM; }

RESIDENT                { PUT(Yacc, yytext, yyleng, 0); return RESIDENT_SYM; }

ENABLE                  { PUT(Yacc, yytext, yyleng, 0); return ENABLE_SYM; }
DISABLE                 { PUT(Yacc, yytext, yyleng, 0); return DISABLE_SYM; }

AUTOINCREMENT           { PUT(Yacc, yytext, yyleng, 0); return AUTOINCREMENT_SYM; }

BLOB                    { PUT(Yacc, yytext, yyleng, 0); return BLOB_SYM; }
INCLUDE                 { PUT(Yacc, yytext, yyleng, 0); return INCLUDE_SYM; }

COMPILE_OPT                { PUT(Yacc, yytext, yyleng, 0); return COMPILE_OPT_SYM; }
MSYNC                   { PUT(Yacc, yytext, yyleng, 0); return MSYNC_SYM; }
SYNCED_RECORD           { PUT(Yacc, yytext, yyleng, 0); return SYNCED_RECORD_SYM; }
INSERT_RECORD           { PUT(Yacc, yytext, yyleng, 0); return INSERT_RECORD_SYM; }
UPDATE_RECORD           { PUT(Yacc, yytext, yyleng, 0); return UPDATE_RECORD_SYM; }
DELETE_RECORD           { PUT(Yacc, yytext, yyleng, 0); return DELETE_RECORD_SYM; }

    /* punctuation */

UNION               {
                        PUT(Yacc, yytext, yyleng, 0);
                        yylval_param->subtok.type   = SOT_UNION;
                        yylval_param->subtok.argcnt = 2;
                        return SET_OPERATION;
                    }

INTERSECT           {
                        PUT(Yacc, yytext, yyleng, 0);
                        yylval_param->subtok.type   = SOT_INTERSECT;
                        yylval_param->subtok.argcnt = 2;
                        return SET_OPERATION;
                    }

EXCEPT              {
                        PUT(Yacc, yytext, yyleng, 0);
                        yylval_param->subtok.type   = SOT_EXCEPT;
                        yylval_param->subtok.argcnt = 2;
                        return SET_OPERATION;
                    }

"="                 {
                        PUT(Yacc, yytext, yyleng, 0);
                        yylval_param->subtok.type   = SOT_EQ;
                        yylval_param->subtok.argcnt = 2;
                        return COMPARISON;
                    }
"<>"                {
                        PUT(Yacc, yytext, yyleng, 0);
                        yylval_param->subtok.type   = SOT_NE;
                        yylval_param->subtok.argcnt = 2;
                        return COMPARISON;
                    }
"!="                {
                        PUT(Yacc, yytext, yyleng, 0);
                        yylval_param->subtok.type   = SOT_NE;
                        yylval_param->subtok.argcnt = 2;
                        return COMPARISON;
                    }
"<"                 {
                        PUT(Yacc, yytext, yyleng, 0);
                        yylval_param->subtok.type   = SOT_LT;
                        yylval_param->subtok.argcnt = 2;
                        return COMPARISON;
                    }
">"                 {
                        PUT(Yacc, yytext, yyleng, 0);
                        yylval_param->subtok.type   = SOT_GT;
                        yylval_param->subtok.argcnt = 2;
                        return COMPARISON;
                    }
"<="                {
                        PUT(Yacc, yytext, yyleng, 0);
                        yylval_param->subtok.type   = SOT_LE;
                        yylval_param->subtok.argcnt = 2;
                        return COMPARISON;
                    }
">="                {
                        PUT(Yacc, yytext, yyleng, 0);
                        yylval_param->subtok.type   = SOT_GE;
                        yylval_param->subtok.argcnt = 2;
                        return COMPARISON;
                    }
"||"                {
                        PUT(Yacc, yytext, yyleng, 0);
                        return MERGE;
                        /* initialize void * */
                    }

[-+*/%^:(),.?]      {
                        PUT(Yacc, yytext, yyleng, 0);
                        if (yytext[0] == '-')
                            Yacc->is_minus = 1;
                        else
                            Yacc->is_minus = 0;
                        return yytext[0];
                    }

;                   {
                        PUT(Yacc, yytext, yyleng, 0);
                        /* Yacc->end_of_query = 1; */
                        return yytext[0];
                    }
";\t"                {
                        PUT(Yacc, yytext, yyleng, 0);
                         /* Yacc->end_of_query = 1;   */
                        return END_OF_INPUT;
                    }


    /* identifier */

{identifier}        {
                        PUT(Yacc, yytext, yyleng, 0);
                        yylval_param->strval = YACC_XSTRDUP(Yacc, yytext);
                        if (yylval_param->strval == NULL) {
                            Yacc->yerror = "out of memory";
                            Yacc->yerrno = SQL_E_OUTOFMEMORY;
                            return RET_ERROR;
                        }

                        mdb_strlwr(yylval_param->strval);

                        return IDENTIFIER;
                    }

    /* numbers */

{integer}           {
                        PUT(Yacc, yytext, yyleng, 0);
                        yylval_param->intval  = yacc_atoull(yytext);

                        if (Yacc->is_minus)
                        {
                            Yacc->is_minus = 0;
                            if (yylval_param->intval ==
                                    ((MDB_UINT64)MDB_LLONG_MAX + (MDB_UINT64)2))
                            {
                                Yacc->yerror = "Specified value too large for column";
                                Yacc->yerrno = SQL_E_OUTOFMEMORY;
                                return RET_ERROR;
                            }

                            return INTNUM;
                        }
                        else
                        {
                            if (yylval_param->intval >
                                    (MDB_UINT64)(MDB_LLONG_MAX))
                            {
                                Yacc->yerror = "Specified value too large for column";
                                Yacc->yerrno = SQL_E_OUTOFMEMORY;
                                return RET_ERROR;
                            }
                        }
                        return INTNUM;
                    }
{float}                {
                        PUT(Yacc, yytext, yyleng, 0);
                        yylval_param->floatval = sc_atof(yytext);
                        return FLOATNUM;
                    }
{approximate}       {
                        PUT(Yacc, yytext, yyleng, 0);
                        yylval_param->floatval = sc_atof(yytext);
                        return APPROXNUM;
                    }

    /* strings */

'[^']*'            {
                        char *p;
                        int c = input(yyscanner);
                        
                        PUT(Yacc, yytext, yyleng, 0);
                        unput(c);    /* just peeking., rewind it */
                        /* sc_wcschr(yytext, '\'');  */

                        if (c != '\'') 
                        {

                           yylval_param->strval = (char*)sql_mem_alloc(Yacc->stmt->parsing_memory, yyleng+ WCHAR_SIZE);
                           if (yylval_param->strval == NULL) {
                                Yacc->yerror = "out of memory";
                                Yacc->yerrno = SQL_E_OUTOFMEMORY;
                                return RET_ERROR;
                           }

                          if (Yacc->is_nchar == 1)
                          {
                            int i, local_len = yyleng -3;
                            DB_WCHAR quaotation = '\'';

                            sc_memcpy(yylval_param->strval, yytext+2, yyleng -3); 
                            p = yylval_param->strval;

                            for (i = 0; i < local_len; i++) 
                            {
                                if (p[i] == '\'')
                                {
                                    if (i+3 < local_len && p[i+1] == '\'' && 
                                            p[i+2] =='\0' && p[i+3] =='\0')
                                    {
                                        sc_memmove(p+i+1, p+i+4, 
                                                (yyleng)-(int)(p+i+4-yylval_param->strval));
                                        local_len -= 3;
                                    }
                                    else
                                    {
                                        *(DB_WCHAR*)(p+i) = quaotation;

                                        if (WCHAR_SIZE == 2)
                                            i += 1;
                                        else
                                            i += 3;
                                    }
                                }
                            }

                            Yacc->is_nchar = 0;

                            return NSTRINGVAL;
                          }
                          else
                          {
                            sc_strncpy(yylval_param->strval, yytext+1, yyleng);
                            yylval_param->strval[yyleng-2] = '\0'; /* cut last '\'' */
                            p = yylval_param->strval;

                            while (1) 
                            {
                                p = sc_strchr(p, '\''); 
                                if (p == NULL)
                                    break;
                                sc_memmove(p, p+1,
                                        (yyleng-2)-(int)(p-yylval_param->strval));
                                p++;
                            }

                            Yacc->is_nchar = 0;

                            return STRINGVAL;
                          }
                        } else yymore();
                    }

N'[^']*'            {
                        int c = input(yyscanner);
                            
                        /* sc_wcschr(yytext, '\''); */
                        Yacc->is_nchar = 1;
                        
                        PUT(Yacc, yytext, yyleng, 0);
                        unput(c);    /* just peeking., rewind it */

                        if (c != '\'') 
                        {
                            yylval_param->strval = (char *)sql_mem_alloc(Yacc->stmt->parsing_memory, yyleng+ WCHAR_SIZE);
                            if (yylval_param->strval == NULL) {
                                Yacc->yerror = "out of memory";
                                Yacc->yerrno = SQL_E_OUTOFMEMORY;
                                return RET_ERROR;
                            }
                            sc_memcpy(yylval_param->strval, yytext+2, yyleng -3); 

                            return NSTRINGVAL;
                        } else yymore();
                    }
'[^']*$             {
                        PUT(Yacc, yytext, yyleng, 0);
                        Yacc->yerror = "unterminated string";
                        return RET_ERROR;
                    }
        
\"[^"]*\"           {
                        char *p;
                        int c = input(yyscanner);

                        PUT(Yacc, yytext, yyleng, 0);
                        unput(c);    /* just peeking., rewind it */

                        if (c != '\"') {
                            yylval_param->strval = YACC_XSTRDUP(Yacc, yytext+1);
                            if (yylval_param->strval == NULL) {
                                Yacc->yerror = "out of memory";
                                Yacc->yerrno = SQL_E_OUTOFMEMORY;
                                return RET_ERROR;
                            }
                            yylval_param->strval[yyleng-2] = '\0'; /* cut last '\"' */

                            p = yylval_param->strval;
                            while (1)
                            {
                                p = sc_strchr(p, '\"');
                                if (p == NULL)
                                    break;
                                sc_memmove(p, p+1,
                                        (yyleng-2)-(int)(p-yylval_param->strval));
                                p++;
                            }

                            mdb_strlwr(yylval_param->strval);

                            return IDENTIFIER;
                        } else yymore();
                    }
        
\"[^"]*$            {
                        PUT(Yacc, yytext, yyleng, 0);
                        Yacc->yerror = "unterminated string";
                        return RET_ERROR;
                    }
        
.                   {
                        PUT(Yacc, yytext, yyleng, 0);
                        Yacc->yerror = "unknown character";
                        return RET_ERROR;
                    }

B'[0-1']*'          {   /* THE BINARY EXPRESSOIN */ 
                       int c = input(yyscanner);
                            
                        /* sc_wcschr(yytext, '\''); */
                        Yacc->is_nchar = 1;
                        
                        PUT(Yacc, yytext, yyleng, 0);
                        unput(c);    /* just peeking., rewind it */
                        
                        if (c != '\'') 
                        {
                            yylval_param->strval = (char *)sql_mem_alloc(Yacc->stmt->parsing_memory, yyleng+ WCHAR_SIZE);
                            if (yylval_param->strval == NULL) {
                                Yacc->yerror = "out of memory";
                                Yacc->yerrno = SQL_E_OUTOFMEMORY;
                                return RET_ERROR;
                            }
                            sc_memcpy(yylval_param->strval, yytext+2, yyleng -3); 
                            return BINARYVAL; 
                        } else yymore();
                    }

X'[0-9a-fA-F']*'    {   /* THE BINARY EXPRESSOIN */ 
                       int c = input(yyscanner);
                            
                        /* sc_wcschr(yytext, '\''); */
                        Yacc->is_nchar = 1;
                        
                        PUT(Yacc, yytext, yyleng, 0);
                        unput(c);    /* just peeking., rewind it */

                        if (c != '\'') 
                        {
                            yylval_param->strval = (char *)sql_mem_alloc(Yacc->stmt->parsing_memory, yyleng+ WCHAR_SIZE);
                            if (yylval_param->strval == NULL) {
                                Yacc->yerror = "out of memory";
                                Yacc->yerrno = SQL_E_OUTOFMEMORY;
                                return RET_ERROR;
                            }
                            sc_memcpy(yylval_param->strval, yytext+2, yyleng -3); 
                            return HEXADECIMALVAL;
                        } else yymore();
                    }


%%

void *redirect_yyinput(T_STATEMENT *stmt, char *ptr, int len)
{
    /*
    extern void my_yy_flush_buffer(T_YACC *);
    */
    T_YACC *yacc = current_thr();

    yacc->_select       = NULL;
    yacc->yyinput        = ptr;
    yacc->yyinput_posi    = ptr;
    yacc->yyinput_len    = len;
    yacc->my_yylineno      = 1;
    yacc->my_yyinput_cnt = 0;
    yacc->stmt          = stmt;
    yacc->root_stmt     = stmt;

    yacc->select_top    = NULL;
#ifdef WIN32
    yacc->timestamp        = 0;
#else
    yacc->timestamp        = 0LL;
#endif

    yacc->err_pos = ptr;
    yacc->err_len = 0;
    yacc->is_minus = 0;

    return (void*)yacc;
}

static int my_yyinput(T_YACC *yacc, char *buf, int max_size)
{
    int n;
    
    n = min(max_size,
            yacc->yyinput_len - (int)(yacc->yyinput_posi-yacc->yyinput));
    if (n > 0) {
        sc_memcpy(buf, yacc->yyinput_posi, n);
        yacc->yyinput_posi += n;
    }
    return n;
}

int my_yylex(YYSTYPE *yylval_param, void *__yacc)
{
    if (!Yacc->scanner) yylex_init(&Yacc->scanner);
    if (Yacc->scanner)
    {
        struct yyguts_t *yyg = (struct yyguts_t *)Yacc->scanner;
        yyg->yyextra_r = Yacc;
    }
    Yacc->is_nchar = 0;
    return yylex(yylval_param, Yacc->scanner, __yacc);
}

void my_yy_flush_buffer(T_YACC *yacc)
{
    /* WARNING
     * yyguts_t 은 flex 버전에 dependent한 structure이다.
     * flex 버전을 바꿔사용할때마다, 반드시 확인하자.
     */
    struct yyguts_t    *yyg = (struct yyguts_t *)yacc->scanner;
    yy_flush_buffer(YY_CURRENT_BUFFER, yacc->scanner);
}

void my_yy_delete_buffer(void *__yacc)
{
    /* WARNING
     * yyguts_t 은 flex 버전에 dependent한 structure이다.
     * flex 버전을 바꿔사용할때마다, 반드시 확인하자.
     */
    struct yyguts_t    *yyg = (struct yyguts_t *)Yacc->scanner;
    if (yyg == NULL)
        return;
    yy_delete_buffer(YY_CURRENT_BUFFER, Yacc->scanner);
    if (Yacc->scanner)
    {
        yylex_destroy(Yacc->scanner);
        Yacc->scanner = NULL;
    }
}

void put_token(T_YACC *yacc, char *token, int len)
{
    if (((yacc->err_pos + yacc->err_len) - yacc->yyinput) < yacc->yyinput_len)
    {
        yacc->err_pos += yacc->err_len;
        yacc->err_len = len; 
    }
    yacc->my_yyinput_cnt += len;
}

MDB_UINT64 yacc_atoull(char *str)
{
    char *p;
    MDB_UINT64 bi = 0;
    int i = 0;

    p = str;

    for (i = 0; *p; i++, p++)
    {
        if (i == CSIZE_BIGINT-2)
        {
            if ((bi > MDB_LLONG_MIN_18) 
                || (bi == MDB_LLONG_MIN_18 && *p > '8'))
            {
                return ((MDB_UINT64)MDB_LLONG_MAX + (MDB_UINT64)2);
            }
        }
        else if (i > CSIZE_BIGINT-2)
            return ((MDB_UINT64)MDB_LLONG_MAX + (MDB_UINT64)2);

        if (sc_isdigit(*p))
            bi = bi * 10 + (*p - '0');
        else
            return 0;
    }

    return bi;
}
