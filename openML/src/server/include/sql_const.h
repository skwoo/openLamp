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

#ifndef _SQL_CONST_H
#define _SQL_CONST_H

#include "mdb_config.h"
#include "mdb_FieldDesc.h"

#define MAX_SYSTEM_TABLE_NUM        100

#define MAX_HOST_NAME_LEN           20
#define MAX_DATABASE_NAME_LEN       20
#define MAX_USER_NAME_LEN           20
#define MAX_PASSWORD_LEN            20

#define STMT_MAX                    0x7fff

#define MAX_TABLE_NAME_LEN          REL_NAME_LENG
#define MAX_TABLE_ALIAS_NAME_LEN    MAX_TABLE_NAME_LEN

#define MAX_TOKEN_NAME_LEN          FIELD_NAME_LENG
#define MAX_RESULT_NAME_LEN         FIELD_NAME_LENG

#define MAX_INDEX_FIELD_NUM         MAX_KEY_FIELD_NO

#define MAX_DEFAULT_VALUE_LEN       MAX_FIELD_VALUE_SIZE


#define MAX_INIT_LIST_NUM           16
// merge_expression() 에서 아래 변수가 사용되며 
// 복잡한 질의문인 경우, 아래 변수값을 증가시켜 주면 
// parsing시 메모리 사용량을 줄이는 것이 가능
#define MAX_NEXT_LIST_NUM           16

#define MAX_LIST_NUM                128
#define MAX_EXPR_ELEM_NUM           16

#define MAX_OP_LEN                  8

#define MAX_COLUMN_LIST_NUM         16

#define MAX_JOIN_TABLE_NUM          8

#define MAX_DATE_FIELD_LEN          4
#define MAX_DATE_STRING_LEN         11
#define MAX_TIME_STRING_LEN         9
#define MAX_TIME_FIELD_LEN          4
#define MAX_DATETIME_FIELD_LEN      8
#define MAX_DATETIME_STRING_LEN     20
#define MAX_TIMESTAMP_FIELD_LEN     8
#define MAX_TIMESTAMP_STRING_LEN    24

#define DEFAULT_DECIMAL_SIZE        10
#define MAX_DECIMAL_PRECISION       15
#define MIN_DECIMAL_SCALE           0
#define MAX_DECIMAL_SCALE           14

#define MAX_DECIMAL_STRING_LEN      (MAX_DECIMAL_PRECISION+3)

#define MAX_BIGINT_STRING_LEN       20

#define MAX_DISTINCT_TABLE_NUM      8

/* char array size */
#define CSIZE_CHAR                  1
#define CSIZE_TINYINT               8
#define CSIZE_SMALLINT              8
#define CSIZE_INT                   12
#define CSIZE_BIGINT                20
#define CSIZE_FLOAT                 15
#define CSIZE_DOUBLE                15
#define CSIZE_DECIMAL               (MAX_DECIMAL_PRECISION+2)
#define CSIZE_DEFAULT               16


/* struct size */
#define SSIZE_DATE                  6
#define SSIZE_TIME                  6

#define PRIMARY_KEY_PREFIX          "$pk."
#define MAXREC_KEY_PREFIX           "$max."

#define INTERNAL_TABLE_PREFIX       "#tt"
#define INTERNAL_FIELD_PREFIX       "#tf"
#define INTERNAL_INDEX_PREFIX       "#ti"

/* bit-vector: internal table mode */
#define iTABLE_AGGREGATION          1
#define iTABLE_ORDERBY              2
#define iTABLE_DISTINCT             4
/* from절 subquery에 의한 T_SELECT구조의 kindofwTable */
#define iTABLE_SUBSELECT            8
/* where절 subquery에 의한 T_SELECT구조의 kindofwTable */
#define iTABLE_SUBWHERE             16
#define iTABLE_CORRELATED           32
#define iTABLE_SUBSET               64

/* bit-vector: column information */
#define PRI_KEY_BIT                 FD_KEY
#define NULL_BIT                    FD_NULLABLE
#define AUTO_BIT                    FD_AUTOINCREMENT
#define DEF_SYSDATE_BIT             FD_DEF_SYSDATE
#define UNIQUE_INDEX_BIT            0x80
#define NOLOGGING_BIT               FD_NOLOGGING
#define EXPLICIT_NULL_BIT           0x20

/* result set type */
#define RES_TYPE_NONE               0
#define RES_TYPE_BINARY             1
#define RES_TYPE_STRING             2

/* return value */
#define RET_ERROR                   -1
#define RET_END                     0   /* end of cursor */
#define RET_SUCCESS                 1
#define RET_START                   2
#define RET_NEXT                    3
#define RET_REPOSITION              4
#define RET_TRUE                    5
#define RET_FALSE                   6
#define RET_UNKNOWN                 7
#define RET_END_USED_DIRECT_API     8

#endif // _SQL_CONST_H
