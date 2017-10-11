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

#ifndef __FIELDDESC_H
#define __FIELDDESC_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include "mdb_config.h"
#include "mdb_typedef.h"
#include "mdb_FieldVal.h"
#include "mdb_define.h"
#include "mdb_collation.h"

/* when updating TTree struct, the DB image changed, thus should do createdb */
    typedef struct keyfielddesc
    {
        DB_INT8 type_;
        char order_;            /* 'A': ASC, 'D': DESC */
        DB_UINT16 position_;
        DB_UINT16 offset_;
        DB_UINT16 h_offset_;
        DB_UINT32 length_;
        DB_INT32 fixed_part;
        DB_UINT8 collation;
        DB_UINT8 flag_;
    } KEY_FIELDDESC_T;

#define TEMPVAR_FIXEDPART         40

/* define FIXED_SIZE_FOR_VARIABLE bigger than TNODE_KEYVAL_LEN */
#define MINIMUM_FIXEDPART         (TNODE_KEYVAL_LEN + INT_SIZE + OID_SIZE)
#define FIXED_SIZE_FOR_VARIABLE   20    /* (TNODE_KEYVAL_LEN + , INT_SIZE) */

#define FIXED_VARIABLE                  -1

#define MAX_FIXED_SIZE_FOR_VARIABLE     (MDB_UCHAR_MAX -1)

#define IS_FIXED_DATA(type, fixed_part)     \
                 (!(IS_VARIABLE(type)) || fixed_part == FIXED_VARIABLE)
#define IS_VARIABLE_DATA(type, fixed_part)   \
                 (IS_VARIABLE(type) && fixed_part > FIXED_VARIABLE)

    typedef unsigned char DB_FIX_PART_LEN;
#define MDB_FIX_LEN_SIZE    1   // fixed part's length buffer size

    // CHANGE 2 BYTES
    typedef DB_UINT16 DB_EXT_PART_LEN;
#define MDB_EXT_LEN_SIZE    2   // extened part's length buffer size
#define MDB_EXTENDED_FLAG_BIT   0x8000

#define DB_GET_HFIELD_BLENGTH(blength_, type_, length_, fixed_part_)\
    do {                                                            \
        blength_ = 0;                                               \
        if (IS_VARIABLE_DATA(type_, fixed_part_)) {                 \
            blength_ += fixed_part_;                                \
            blength_ += MDB_FIX_LEN_SIZE;                           \
        } else {                                                    \
            if (IS_NBS(type_))                                      \
                blength_ += length_ * sizeof(DB_WCHAR);             \
            else                                                    \
                blength_ += length_;                                \
        }                                                           \
    } while(0);
#define IS_VARIABLE_DATA_EXTENDED(hvalue_, fixed_part_)             \
            (((DB_FIX_PART_LEN)*((DB_FIX_PART_LEN*)hvalue_ + fixed_part_)) == MDB_UCHAR_MAX)

#define DB_GET_MFIELD_BLENGTH(blength_, type_, length_)             \
    do {                                                            \
        blength_ = 0;                                               \
        if (IS_NBS(type_))                                          \
            blength_ += length_ * sizeof(DB_WCHAR);                 \
        else                                                        \
            blength_ += length_;                                    \
    } while(0);

#define DBI_GET_FIXEDPART(h_value_, fixed_part_, dest_, dest_blen_, oid_)  \
    do {                                                                    \
        dest_blen_ = (DB_FIX_PART_LEN)*((DB_FIX_PART_LEN*)(h_value_ + fixed_part)); \
        if (dest_blen_ != MDB_UCHAR_MAX)                                    \
        {   /* NOT_EXTENDED */                                              \
            dest_       = h_value_;                                         \
            oid_        = NULL_OID;                                         \
        }                                                                   \
        else                                                                \
        {   /* EXTENDED */                                                  \
            dest_blen_  = fixed_part_ - OID_SIZE;                           \
            dest_       = h_value_ + OID_SIZE;                              \
            sc_memcpy((char*)&oid_, h_value_, OID_SIZE);                    \
        }                                                                   \
    } while(0);

#define DBI_GET_EXTENDPART(oid_, dest_, dest_blen_)                        \
    do {                                                                   \
        char *_oid_ptr_ = OID_Point(oid_);                                 \
        dest_blen_ = (DB_EXT_PART_LEN)(*(DB_EXT_PART_LEN*)_oid_ptr_);      \
        dest_ = _oid_ptr_ + MDB_EXT_LEN_SIZE;                              \
    } while(0);

// for 4byte-align
    typedef struct FieldDesc
    {
        char fieldName[FIELD_NAME_LENG];
        DataType type_;
        DB_UINT32 offset_;
        DB_INT32 h_offset_;     // heap record's offset
        DB_UINT32 length_;
        DB_UINT16 position_;
        DB_INT8 scale_;
        DB_UINT8 flag;          /* 0x1: iskey, 0x2: null_allowed, 0x4: DEF_SYSDATE, ...
                                   following define's */
        DB_INT32 fixed_part;    /* if type_ is DT_VARCHAR(DT_NVARCHAR) and then has meaning.
                                   fixed_part == -1 --> CHAR(length),
                                   fixed_part >=  0 --> VARCHAR(length)

                                   if fixed_part is greater than zero and 
                                   the realsize of the field is 
                                   less than and equal tofixed_part, 
                                   this field's data will be stored in record.

                                   if fixed_part is greater than zero and 
                                   the realsize of the field is greater than fixed_part, 
                                   this field's data will be stored in other record. */
        MDB_COL_TYPE collation;
        char defaultValue[DEFAULT_VALUE_LEN];
    } FIELDDESC_T;

#ifdef  __cplusplus
}
#endif

#endif
