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

#ifndef __FIELDVAL_H
#define __FIELDVAL_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include "mdb_config.h"
#include "mdb_typedef.h"
#include "mdb_define.h"
#include "mdb_collation.h"

/* field mode (most filter mode)*/
#define MDB_EQ    1
#define MDB_GT    2
#define MDB_GE    3
#define MDB_LT    4
#define MDB_LE    5
#define MDB_NE    6
#define MDB_NU    8             // is null            both filter & key value
#define MDB_NN    9             // is not null        both filter & key value
#define MDB_AL   10             // null + not null    for key value

#define EQ    MDB_EQ
#define GT    MDB_GT
#define GE    MDB_GE
#define LT    MDB_LT
#define LE    MDB_LE
#define NE    MDB_NE
#define NU    MDB_NU
#define NN    MDB_NN
#define AL    MDB_AL

#define MDB_LIKE 11
#define LIKE MDB_LIKE
#define MDB_ILIKE 12

#define DEF_BS              ((1 << DT_CHAR) | (1 << DT_VARCHAR))
#define DEF_NBS             ((1 << DT_NCHAR) | (1 << DT_NVARCHAR))
#define DEF_BYTE            ((1 << DT_BYTE) | (1 << DT_VARBYTE))
#define DEF_VARIABLE        ((1 << DT_VARCHAR) | (1 << DT_NVARCHAR) |   \
                            (1 << DT_VARBYTE))
#define DEF_N_STRING        ((1 << DT_VARCHAR) | (1 << DT_NVARCHAR))
#define DEF_N_BYTES         ((1 << DT_CHAR) | (1 << DT_NCHAR))
#define DEF_DATE            ((1 << DT_TIMESTAMP) | (1 << DT_DATETIME) | \
                            (1 << DT_DATE))
#define DEF_DATE_OR_TIME    (DEF_DATE | (1 << DT_TIME))
#define DEF_TIMES           DEF_DATE_OR_TIME

#define IS_BS(_type)                ((1 << _type) & DEF_BS)
#define IS_NBS(_type)               ((1 << _type) & DEF_NBS)
#define IS_BS_OR_NBS(_type)         ((1 << _type) & (DEF_BS | DEF_NBS))
#define IS_BYTE(_type)              ((1 << _type) & DEF_BYTE)
#define IS_VARIABLE(_type)          ((1 << _type) & DEF_VARIABLE)
#define IS_ALLOCATED_TYPE(_type)    ((1 << _type) & (DEF_BS|DEF_NBS|DEF_BYTE))
#define IS_N_STRING(_type)          ((1 << _type) & DEF_N_STRING)
#define IS_N_BYTES(_type)           ((1 << _type) & DEF_N_BYTES)
#define IS_DATE_TYPE(_type)         ((1 << _type) & DEF_DATE_TYPE)
#define IS_TIME_TYPE(_type)         (_type == DT_TIME)
#define IS_DATE_OR_TIME_TYPE(_type) ((1 << _type) & DEF_DATE_OR_TIME)
#define IS_TIMES(_type)             ((1 << _type) & DEF_TIMES)

    union Value
    {
        char char_;
        short short_;
        int int_;
        long long_;
        float float_;
        double double_;
        DB_INT64 bigint_;
        char datetime_[8];
        char date_[4];
        char time_[4];
        DB_INT64 timestamp_;
        char *decimal_;
        char *pointer_;
        DB_WCHAR *Wpointer_;
        OID oid_;
    };

    typedef struct FieldValue
    {
        DB_INT8 is_null;        /* Server will set. */
        DB_UINT8 mode_;
        DB_UINT16 position_;
        DataType type_;
        DB_UINT32 offset_;
        DB_UINT32 h_offset_;
        DB_UINT32 length_;      /* field schema length */
        DB_UINT32 value_length_;        /* value_ length *//* not bytes length */
        DB_INT32 fixed_part;
        MDB_COL_TYPE collation;
        DB_INT8 scale_;
        char order_;
        union Value value_;
    } FIELDVALUE_T;

    typedef struct FieldValues
    {
        DB_UINT16 fld_cnt;
        OID rec_oid;
        struct FieldValue *fld_value;
        DB_UINT16 sel_fld_cnt;
        DB_UINT16 *sel_fld_pos;
        DB_UINT16 eval_fld_cnt;
        DB_UINT16 *eval_fld_pos;
    } FIELDVALUES_T;

#define UPDATE_FLDVAL_FIXEDSIZE    12

    typedef struct UpdateFieldValue
    {
        DB_UINT16 pos;
        DB_UINT8 isnull;
        DB_UINT8 dummy;         /* not used */
        DB_UINT32 Blength;      /* byte length of this value.
                                   DT_CHAR, DT_NCHAR, DT_VARCHAR, DT_NVARCHAR,
                                   DT_DECIMAL, DT_BYTE, DT_VARBYTE. */
        char *data;
        char space[UPDATE_FLDVAL_FIXEDSIZE];
    } UPDATEFIELDVALUE_T;

    struct UpdateValue
    {
        DB_UINT32 update_field_count;
        struct UpdateFieldValue *update_field_value;
    };

    int dbi_FieldValue(int mode, int type, int position, int offset, int length, int scale, int collation, void *value, DataType value_type, int value_length,  /* input */
            FIELDVALUE_T * fieldval /* output */ );

#ifdef  __cplusplus
}
#endif

#endif
