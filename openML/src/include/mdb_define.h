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

#ifndef __MDB__DEFINE_H
#define __MDB__DEFINE_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include "mdb_config.h"
#include "ErrorCode.h"
#include "dbport.h"

#define _INTERFACE              /* interface is C default scope */
#define _PUBLIC                 /* public is C default scope    */
#define _PRIVATE     static     /* static really means private  */

#define LK_MODE_NO          0
#define LK_MODE_SS          1
#define LK_MODE_DS          2
#define LK_MODE_DX          3
#define LK_MODE_SX          4
#define LK_NUM_LOCKS        5

#define LK_NOLOCK           LK_MODE_NO
#define LK_SCHEMA_SHARED    LK_MODE_SS
#define LK_SHARED           LK_MODE_DS
#define LK_EXCLUSIVE        LK_MODE_DX
#define LK_SCHEMA_EXCLUSIVE LK_MODE_SX

// object length
#define REL_NAME_LENG       64
#define FIELD_NAME_LENG     64
#define INDEX_NAME_LENG     64
#define VOL_NAME_LENG       64
#define USER_NAME_LENG      20
#define USER_PWD_LEN        20
#define DEFAULT_VALUE_LEN   40

#define CONT_NAME_LENG      REL_NAME_LENG

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

// variable for filter
#define MAX_FIELD_VALUE_SIZE    40

// KEY DESC DEFINE
#define MAX_KEY_FIELD_NO        8

    /* A problem may occur in case of bigger than PAGE_BODY's SIZE */
#define MAX_OF_CHAR             REC_SLOT_SIZE(PAGE_BODY_SIZE - 4)

#define MAX_OF_VARCHAR          MAX_OF_CHAR

#define MDB_BUFSIZ              1024

    typedef enum DataType
    {
        DT_NULL_TYPE = 0,
        DT_BOOL,
        DT_TINYINT,
        DT_SMALLINT,
        DT_INTEGER,
        DT_LONG,
        DT_BIGINT,
        DT_FLOAT,
        DT_DOUBLE,
        DT_DECIMAL,     /* the end of numeric types */
        DT_CHAR,
        DT_NCHAR,
        DT_VARCHAR,
        DT_NVARCHAR,
        DT_BYTE,
        DT_VARBYTE,
        DT_TEXT,
        DT_BLOB,
        DT_TIMESTAMP,
        DT_DATETIME,
        DT_DATE,
        DT_TIME,
        DT_HEX, /* only for special version */
        DT_OID,
        DT_DUMMY = MDB_INT_MAX  /* to guarantee sizeof(enum) == 4 */
    } DataType;

#define DT_SHORT        DT_SMALLINT

#define FD_KEY              0x01
#define FD_NULLABLE         0x02
#define FD_DEF_SYSDATE      0x04
#define FD_AUTOINCREMENT    0x08
#define FD_DESC             0x10
#define FD_NOLOGGING        0x40

#ifdef  __cplusplus
}
#endif

#endif
