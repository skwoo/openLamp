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

#ifndef SYSTABLE_H
#define SYSTABLE_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include "mdb_config.h"
#include "mdb_define.h"
#include "mdb_typedef.h"

#ifndef _DUMMY_FIELD_LEN_
#define _DUMMY_FIELD_LEN_
#define DUMMY_FIELD_LEN(n) ( sizeof(long) == 4 ? \
        ((((n-1) / 8 + 1) * 2 ) / sizeof(long) + 6) : \
        ((((n-1) / 8 + 1) * 2 ) / sizeof(long) + 5) )
#endif

    typedef struct SYSTABLE
    {
        DB_INT32 tableId;
        char tableName[REL_NAME_LENG];
        DB_INT32 numFields;
        DB_INT32 recordLen;
        DB_INT32 h_recordLen;   /* heap record length */
        DB_INT32 numRecords;
        DB_INT32 nextId;
        DB_INT32 maxRecords;    /* max of the number of records.
                                   If 0, infinite. */
        char columnName[FIELD_NAME_LENG];
        DB_UINT16 flag;

        DB_BOOL has_variabletype;       /* has variable length data type */
    } SYSTABLE_T;

    typedef struct SYSTABLE_REC
    {
        DB_INT32 tableId;
        char tableName[REL_NAME_LENG];
        DB_INT32 numFields;
        DB_INT32 recordLen;
        DB_INT32 h_recordLen;   /* heap record length */
        DB_INT32 numRecords;
        DB_INT32 nextId;
        DB_INT32 maxRecords;    /* max of the number of records.
                                   If 0, infinite. */
        char columnName[FIELD_NAME_LENG];
        DB_UINT16 flag;

        DB_BOOL has_variabletype;       /* has variable length data type */
        long _hidden[DUMMY_FIELD_LEN(11)];      /* hidden fields */
    } SYSTABLE_REC_T;

// do sync with FIELDDESC_T
    typedef struct SYSFIELD
    {
        DB_INT32 fieldId;
        DB_INT32 tableId;
        DB_INT32 position;
        char fieldName[FIELD_NAME_LENG];
        DB_INT32 offset;
        DB_INT32 h_offset;      /* heap_offset */
        DB_INT32 length;
        DB_INT8 scale;
        DB_INT8 type;
        DB_UINT8 flag;          /* 0x1: iskey, 0x2: null_allowed */
        DB_UINT8 collation;
        char defaultValue[DEFAULT_VALUE_LEN];
        DB_INT32 fixed_part;    /* if this field's type is DT_VARCHAR(DT_NVARCHAR)
                                   and the size of the field is less than fixed_size, 
                                   this field will be stored in record */
        /* this is bytes length */
    } SYSFIELD_T;

    typedef struct SYSFIELD_REC
    {
        DB_INT32 fieldId;
        DB_INT32 tableId;
        DB_INT32 position;
        char fieldName[FIELD_NAME_LENG];
        DB_INT32 offset;
        DB_INT32 h_offset;      /* heap_offset */
        DB_INT32 length;
        DB_INT8 scale;
        DB_INT8 type;
        DB_UINT8 flag;          /* 0x1: iskey, 0x2: null_allowed */
        DB_UINT8 collation;
        char defaultValue[DEFAULT_VALUE_LEN];
        DB_INT32 fixed_part;    /* if this field's type is DT_VARCHAR(DT_NVARCHAR)
                                   and the size of the field is less than fixed_size, 
                                   this field will be stored in record */
        long _hidden[DUMMY_FIELD_LEN(13)];      /* hidden fields */
    } SYSFIELD_REC_T;

    typedef struct SYSINDEX
    {
        DB_INT32 tableId;
        DB_INT8 indexId;        /* Index id per table, 0 ~ 7 */
        DB_INT8 type;
        DB_INT8 numFields;
        char indexName[INDEX_NAME_LENG];
    } SYSINDEX_T;

    typedef struct SYSINDEX_REC
    {
        DB_INT32 tableId;
        DB_INT8 indexId;        /* Index id per table, 0 ~ 7 */
        DB_INT8 type;
        DB_INT8 numFields;
        char indexName[INDEX_NAME_LENG];
        long _hidden[DUMMY_FIELD_LEN(5)];       /* hidden fields */
    } SYSINDEX_REC_T;

    typedef struct SYSINDEXFIELD
    {
        DB_INT32 tableId;
        DB_INT8 indexId;        /* index id per table 0 ~ 7 */
        DB_INT8 keyPosition;    /* key position */
        char order;             /* 'A': ASC, 'D': DESC */
        DB_INT8 collation;
        DB_INT32 fieldId;       /* field id per table */
    } SYSINDEXFIELD_T;

    typedef struct SYSINDEXFIELD_REC
    {
        DB_INT32 tableId;
        DB_INT8 indexId;        /* index id per table 0 ~ 7 */
        DB_INT8 keyPosition;    /* key position */
        char order;             /* 'A': ASC, 'D': DESC */
        DB_INT8 collation;
        DB_INT32 fieldId;       /* field id per table */
        long _hidden[DUMMY_FIELD_LEN(6)];       /* hidden fields */
    } SYSINDEXFIELD_REC_T;

#define MAX_SYSVIEW_DEF     (S_PAGE_SIZE / 2)   //4096

    typedef struct SYSVIEW
    {
        DB_INT32 tableId;
        char definition[1];
    } SYSVIEW_T;

    typedef struct sysstatus
    {
        char version[32];
        int procid;
        int bits;
        char db_name[16];
        char db_path[64];
        int db_dsize;
        int db_isize;
        int num_connects;
        int num_trans;
    } SYSSTATUS_T;

    typedef struct SYSSEQUENCE
    {
        DB_INT32 sequenceId;
        char sequenceName[FIELD_NAME_LENG];
        DB_INT32 minNum;
        DB_INT32 maxNum;
        DB_INT32 increaseNum;
        DB_INT32 lastNum;
        DB_INT8 cycled;
        DB_INT8 _inited;
        long _hidden[DUMMY_FIELD_LEN(8)];
    } SYSSEQUENCE_REC_T;

#define DEFAULT_SYSTABLES_ID        1
#define DEFUALT_SYSFIELDS_ID        2
#define DEFAULT_SYSINDEXES_ID       3
#define DEFAULT_SYSINDEXFIELDS_ID   4
#define DEFAULT_SYSSTATUS_ID        5
#define DEFAULT_SYSDUMMY_ID         6
#define DEFAULT_SYSVIEWS_ID         7
#define DEFAULT_SYSSEQUENCES_ID     8

#ifdef  __cplusplus
}
#endif

#endif
