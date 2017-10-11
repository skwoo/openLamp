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

#ifndef _DB_API_H
#define _DB_API_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include "mdb_config.h"
#include "systable.h"
#include "mdb_typedef.h"
#include "ErrorCode.h"
#include "mdb_inner_define.h"
#include "mdb_schema_cache.h"
#include "mdb_ppthread.h"
#include "mdb_FieldDesc.h"
#include "mdb_compat.h"
#include "mdb_define.h"
#include "mdb_typedef.h"
#include "mdb_KeyValue.h"
#include "mdb_KeyDesc.h"
#include "mdb_Filter.h"
#include "mdb_Csr.h"

#include "db_api.h"

    extern int dbi_Connect_Server(int handle, char *dbserver, char *dbname,
            char *username, char *password);
    extern int dbi_Disconnect_Server(int connectionid);

    extern int dbi_Trans_ID(int connid);

    extern int dbi_Trans_Begin(int connid);
    extern int dbi_Trans_Commit(int connid);
    extern int dbi_Trans_Abort(int connid);

    extern int dbi_Log_Buffer_Flush(int connid);
    extern int dbi_Trans_Implicit_Savepoint(int connid);
    extern int dbi_Trans_Implicit_Savepoint_Release(int connid);
    extern int dbi_Trans_Implicit_Partial_Rollback(int connid);
    extern int dbi_Trans_NextTempName(int connid, int table_flag,
            char *temp_name, char *suf);

    extern int dbi_Cursor_Open(int connid, char *relation_name,
            char *indexName, struct KeyValue *min, struct KeyValue *max,
            struct Filter *filter, LOCKMODE lock, DB_UINT8 msync_flag);

    extern int dbi_Cursor_Open_Desc(int connid, char *relation_name,
            char *indexName, struct KeyValue *min, struct KeyValue *max,
            struct Filter *filter, LOCKMODE lock);

    extern int dbi_Cursor_Close(int connid, int cursorID);
    extern int dbi_Cursor_Insert(int connid, int cursorId,
            char *record, int record_leng, int isPreprocessed,
            OID * inserted_rid);
    extern int dbi_Cursor_Distinct_Insert(int connid, int cursorId,
            char *record, int record_leng);
    extern int dbi_Cursor_Read(int connid, int cursorId, char *record,
            int *record_leng, FIELDVALUES_T * rec_values, int direct,
            OID page);

    extern int dbi_Cursor_GetRid(int connid, int cursorId, OID * rid);

    extern int dbi_Cursor_Remove(int connid, int cursorId);
    extern int dbi_Cursor_Update(int connid, int cursorId,
            char *record, int record_leng, int *col_list);
    extern int dbi_Cursor_Update_Field(int connid, int cursorId,
            struct UpdateValue *newValues);
    extern int dbi_Direct_Read(int connid, char *tableName,
            char *indexName, struct KeyValue *findKey, OID rid_to_read,
            LOCKMODE lock, char *record, DB_UINT32 * size,
            FIELDVALUES_T * rec_values);

    extern int dbi_Direct_Read_ByRid(int connid, char *tableName,
            OID rid_to_read, SYSTABLE_T * tableinfo, LOCKMODE lock,
            char *record, DB_UINT32 * size, FIELDVALUES_T * rec_values);

    extern int dbi_Direct_Remove(int connid, char *tableName, char *indexName,
            struct KeyValue *findKey, OID rid_to_remove, LOCKMODE lock);

    int dbi_Direct_Remove_ByRid(int connid, char *tableName, OID rid_to_remove,
            SYSTABLE_T * tableinfo, LOCKMODE lock);

    extern int dbi_Direct_Update(int connid, char *tableName,
            char *indexName, struct KeyValue *findKey,
            OID rid_to_update, char *record, int record_leng, LOCKMODE lock,
            struct UpdateValue *newValues);
    extern int dbi_Direct_Update_Field(int connid, char *tableName,
            char *indexName, struct KeyValue *findKey,
            struct UpdateValue *newValues, LOCKMODE lock);
    extern int dbi_Cursor_Count(int connid, char *relation_name,
            char *indexName, struct KeyValue *min, struct KeyValue *max,
            struct Filter *filter);
    extern int dbi_Cursor_Count2(int connid, char *relation_name,
            char *indexName, struct KeyValue *min, struct KeyValue *max,
            struct Filter *filter);
    extern int dbi_Cursor_DirtyCount(int connid, char *relation_name);
    extern int dbi_Cursor_Seek(int connid, int cursorId, int offset);
    extern int dbi_Cursor_Reopen(int connid, int cursorId,
            struct KeyValue *min, struct KeyValue *max, struct Filter *filter);
    extern int dbi_Cursor_GetPosition(int connid, int cursorId,
            struct Cursor_Position *cursorPos);
    extern int dbi_Cursor_SetPosition(int connid, int cursorId,
            struct Cursor_Position *cursorPos);

    extern int dbi_Cursor_SetPosition_UnderRid(int connid, int cursorId,
            OID set_here);

    extern int Exist_Record(DB_INT32 cursor_id, char *item, int item_size);
    extern int dbi_Cursor_Exist_by_Record(int connid,
            int cursorId, char *record, int record_leng);

    extern int dbi_Relation_Create(int connid,
            char *relation_name, int numFields, FIELDDESC_T * pFieldDesc,
            ContainerType type, int userid, int max_records,
            char *column_name);

    extern int dbi_Relation_Drop(int connid, char *relation_name);

    extern int dbi_Relation_Rename(int connid, char *old_name, char *new_name);
    extern int dbi_Index_Rename(int connid, char *old_name, char *new_name);
    extern int dbi_Relation_Truncate(int connid, char *relation_name);

    extern int dbi_Index_Create(int connid, char *relationName,
            char *indexName,
            int numFields, FIELDDESC_T * pFieldDesc, DB_UINT8 flag);
    extern int dbi_Index_Create_With_KeyInsCond(int connid,
            char *relationName, char *indexName_p,
            int numFields, FIELDDESC_T * pFieldDesc, DB_UINT8 flag,
            int keyins_flag, SCANDESC_T * pScanDesc);

    extern int dbi_Index_Drop(int connid, char *indexName);
    extern int dbi_Index_Rebuild(int connid, int num, char **obj_name);

    extern int dbi_Schema_GetTableInfo(int connid,
            char *relation_name, SYSTABLE_T * pSysTable);
    extern int dbi_Schema_GetUserTablesInfo(int connid,
            SYSTABLE_T ** pSysTable);

    extern int dbi_Schema_GetFieldsInfo(int connid,
            char *relation_name, SYSFIELD_T ** ppSysField);

    extern int dbi_Schema_GetSingleFieldsInfo(int connid,
            char *relation_name, char *field_name, SYSFIELD_T * pSysField);

    extern int dbi_Schema_GetTableFieldsInfo(int connid, char *relation_name,
            SYSTABLE_T * pSysTable, SYSFIELD_T ** ppSysField);
    extern int dbi_Schema_GetIndexInfo(int connid, char *indexName,
            SYSINDEX_T * pSysIndex, SYSINDEXFIELD_T ** ppSysIndexField,
            char *relationName);
    extern int dbi_Schema_GetTableIndexesInfo(int connid, char *relation_name,
            int schIndexType, SYSINDEX_T ** ppSysIndex);
    extern int dbi_ViewDefinition_Create(int connid, char *view_name,
            char *definition);

    extern int dbi_ViewDefinition_Drop(int connid, char *view_name);
    extern int dbi_Schema_GetViewDefinition(int connid, char *view_name,
            SYSVIEW_T ** ppSysView);

    extern int dbi_Trans_Set_Query_Timeout(int connid,
            MDB_UINT32 query_timeout);

    extern int dbi_Checkpoint(int connid);
    extern int dbi_AddVolume(int connid, int numsegment,
            int idxseg_num, int tmpseg_num);
    extern int dbi_AddSegment(int connid, int numsegment, int numIdxSegment);

    extern void dbi_DataMem_Protect_RO(void);
    extern void dbi_DataMem_Protect_RW(void);

    int dbi_Cursor_Set_updidx(int connid, int cursorId, int *col_pos);

    extern int Filter_Delete(struct Filter *pFilter);


    int GET_HIDDENFIELDSNUM(void);
    int GET_HIDDENFIELDLENGTH(int field_index,
            int real_field_num, int real_field_len);

/*******************************************************************************/
/********************** functions for inner action *****************************/
/*******************************************************************************/
    extern __DECL_PREFIX int SpeedMain(char *argv_dbname, int f_state,
            int connid);
#define STATE_SERVER    0
#define STATE_LIB        1
#define STATE_CREATEDB    2
    extern __DECL_PREFIX void Server_Down(int exitcode);

    extern int __Insert(DB_INT32 cursor_id, char *item,
            int item_size, OID * record_id, int isPreprocessed);
    extern int Insert_Variable(struct Container *Cont, SYSFIELD_T * field_info,
            char *h_value, char *value);
    extern int Insert_Distinct(DB_INT32 cursor_id, char *item, int item_size);
    extern int __Read(DB_INT32 cursor_id, char *record,
            FIELDVALUES_T * rec_values, int direction, DB_UINT32 * size,
            OID page);
    extern int Read_Itemsize(DB_INT32 cursor_id, DB_UINT32 * size);
    extern int __Remove(DB_INT32 cursor_id);
    extern int __Update(DB_INT32 cursor_id, char *data, int data_leng,
            int *col_list);
    extern int Find_Seek(DB_INT32 cursor_id, int offset);

    extern int Find_GetRid(DB_INT32 cursor_id, OID * rid);

    extern char *make_memory_record(DB_UINT16 cont_numfields,
            DB_INT32 cont_item_size, DB_UINT16 cont_record_size,
            DB_UINT32 cont_memory_record_size, SYSFIELD_T * nfields_info,
            char *m_record, char *h_record, int m_item_size,
            int *copy_not_cols);

    extern DB_INT32 dbi_strcpy_variable(char *dest, char *h_value,
            MDB_INT32 fixed_part, DataType type, int buflen);
    extern DB_INT32 dbi_strncmp_variable(char *str1, char *str2,
            KEY_FIELDDESC_T * fielddesc);

#if defined(__DEF_TYPES_FUNC__)
    int SizeOfType[] = {
        0,  /* DT_NULL_TYPE     */
        sizeof(DB_BOOL),        /* DT_BOOL */
        sizeof(DB_INT8),        /* DT_TINYINT */
        sizeof(DB_INT16),       /* DT_SMALLINT */
        sizeof(DB_INT32),       /* DT_INTEGER */
        sizeof(long),   /* DT_LONG */
        sizeof(DB_INT64),       /* DT_BIGINT */
        sizeof(float),  /* DT_FLOAT */
        sizeof(double), /* DT_DOUBLE */
        sizeof(double), /* DT_DECIMAL */
        sizeof(char),   /* DT_CHAR */
        sizeof(DB_WCHAR),       /* DT_NCHAR */
        sizeof(char),   /* DT_VARCHAR */
        sizeof(DB_WCHAR),       /* DT_NVARCHAR */
        sizeof(char),   /* DT_BYTE */
        sizeof(char),   /*DT_VARBYTE */
        sizeof(char),   /* DT_BLOB */
        sizeof(char),   /* DT_TEXT */
        sizeof(char),   /* DT_TIMESTAMP */
        sizeof(char),   /* DT_DATETIME */
        sizeof(char),   /* DT_DATE */
        sizeof(char),   /* DT_TIME */
        sizeof(char),   /* DT_HEX */
        sizeof(OID)     /* DT_OID */
    };

    int sc_strlen2(char *str);

    int sc_strlen_byte(char *str);

    __DECL_PREFIX int (*strlen_func[]) () =
    {
        sc_strlen2,     /* DT_NULL_TYPE */
                sc_strlen2,     /* DT_BOOL */
                sc_strlen2,     /* DT_TINYINT */
                sc_strlen2,     /* DT_SMALLINT */
                sc_strlen2,     /* DT_INTEGER */
                sc_strlen2,     /* DT_LONG */
                sc_strlen2,     /* DT_BIGINT */
                sc_strlen2,     /* DT_FLOAT */
                sc_strlen2,     /* DT_DOUBLE */
                sc_strlen2,     /* DT_DECIMAL */
                sc_strlen2,     /* DT_CHAR */
                sc_wcslen,      /* DT_NCHAR */
                sc_strlen2,     /* DT_VARCHAR */
                sc_wcslen,      /* DT_NVARCHAR */
                // 酒府价窃.. -_-;
                sc_strlen_byte, /* DT_BYTE */
                sc_strlen_byte, /*DT_VARBYTE */
                sc_strlen2,     /* DT_BLOB */
                sc_strlen2,     /* DT_TEXT */
                sc_strlen2,     /* DT_TIMESTAMP */
                sc_strlen2,     /* DT_DATETIME */
                sc_strlen2,     /* DT_DATE */
                sc_strlen2,     /* DT_TIME */
    /*_HEX */
    };

    int sc_strcmp2(const char *s1, const char *s2);

    int (*strcmp_func[]) () =
    {
        sc_strcmp2,     /* DT_NULL_TYPE, */
                sc_strcmp2,     /* DT_BOOL */
                sc_strcmp2,     /* DT_TINYINT */
                sc_strcmp2,     /* DT_SMALLINT */
                sc_strcmp2,     /* DT_INTEGER */
                sc_strcmp2,     /* DT_LONG */
                sc_strcmp2,     /* DT_BIGINT */
                sc_strcmp2,     /* DT_FLOAT */
                sc_strcmp2,     /* DT_DOUBLE */
                sc_strcmp2,     /* DT_DECIMAL */
                sc_strcmp2,     /* DT_CHAR */
                sc_wcscmp,      /* DT_NCHAR */
                sc_strcmp2,     /* DT_VARCHAR */
                sc_wcscmp,      /* DT_NVARCHAR */
                // 酒府价窃.. -_-;
                sc_strcmp2,     /* DT_BYTE */
                sc_strcmp2,     /*DT_VARBYTE */
                sc_strcmp2,     /* DT_BLOB */
                sc_strcmp2,     /* DT_TEXT */
                sc_strcmp2,     /* DT_TIMESTAMP */
                sc_strcmp2,     /* DT_DATETIME */
                sc_strcmp2,     /* DT_DATE */
                sc_strcmp2,     /* DT_TIME */
                sc_strcmp2,     /* DT_HEX */
                sc_strcmp2      /* DT_OID */
    };

    int sc_strncmp2(const char *s1, const char *s2, int n);

    int (*strncmp_func[]) () =
    {
        sc_strncmp2,    /* DT_NULL_TYPE */
                sc_strncmp2,    /* DT_BOOL */
                sc_strncmp2,    /* DT_TINYINT */
                sc_strncmp2,    /* DT_SMALLINT */
                sc_strncmp2,    /* DT_INTEGER */
                sc_strncmp2,    /* DT_LONG */
                sc_strncmp2,    /* DT_BIGINT */
                sc_strncmp2,    /* DT_FLOAT */
                sc_strncmp2,    /* DT_DOUBLE */
                sc_strncmp2,    /* DT_DECIMAL */
                sc_strncmp2,    /* DT_CHAR */
                sc_wcsncmp,     /* DT_NCHAR */
                sc_strncmp2,    /* DT_VARCHAR */
                sc_wcsncmp,     /* DT_NVARCHAR */
                // 酒府价窃.. -_-;
                sc_strncmp2,    /* DT_BYTE */
                sc_strncmp2,    /*DT_VARBYTE */
                sc_strncmp2,    /* DT_BLOB */
                sc_strncmp2,    /* DT_TEXT */
                sc_strncmp2,    /* DT_TIMESTAMP */
                sc_strncmp2,    /* DT_DATETIME */
                sc_strncmp2,    /* DT_DATE */
                sc_strncmp2,    /* DT_TIME */
                sc_strncmp2,    /* DT_HEX */
                sc_strncmp2     /* DT_OID */
    };

    int (*strncasecmp_func[]) () =
    {
        sc_strncasecmp, /* DT_NULL_TYPE, */
                sc_strncasecmp, /* DT_BOOL */
                sc_strncasecmp, /* DT_TINYINT */
                sc_strncasecmp, /* DT_SMALLINT */
                sc_strncasecmp, /* DT_INTEGER */
                sc_strncasecmp, /* DT_LONG */
                sc_strncasecmp, /* DT_BIGINT */
                sc_strncasecmp, /* DT_FLOAT */
                sc_strncasecmp, /* DT_DOUBLE */
                sc_strncasecmp, /* DT_DECIMAL */
                sc_strncasecmp, /* DT_CHAR */
                sc_wcsncmp,     /* DT_NCHAR */
                sc_strncasecmp, /* DT_VARCHAR */
                sc_wcsncmp,     /* DT_NVARCHAR */
                // 酒府价窃.. -_-;
                sc_strncasecmp, /* DT_BYTE */
                sc_strncasecmp, /*DT_VARBYTE */
                sc_strncasecmp, /* DT_BLOB */
                sc_strncasecmp, /* DT_TEXT */
                sc_strncasecmp, /* DT_TIMESTAMP */
                sc_strncasecmp, /* DT_DATETIME */
                sc_strncasecmp, /* DT_DATE */
                sc_strncasecmp, /* DT_TIME */
                sc_strncasecmp, /* DT_HEX */
                sc_strncasecmp  /* DT_OID */
    };

    char *sc_wstrcpy(char *s1, char *s2);

    char *(*strcpy_func[]) () =
    {
        sc_strcpy,      /* DT_NULL_TYPE, */
                sc_strcpy,      /* DT_BOOL */
                sc_strcpy,      /* DT_TINYINT */
                sc_strcpy,      /* DT_SMALLINT */
                sc_strcpy,      /* DT_INTEGER */
                sc_strcpy,      /* DT_LONG */
                sc_strcpy,      /* DT_BIGINT */
                sc_strcpy,      /* DT_FLOAT */
                sc_strcpy,      /* DT_DOUBLE */
                sc_strcpy,      /* DT_DECIMAL */
                sc_strcpy,      /* DT_CHAR */
                sc_wstrcpy,     /* DT_NCHAR */
                sc_strcpy,      /* DT_VARCHAR */
                sc_wstrcpy,     /* DT_NVARCHAR */
                // 酒府价窃.. -_-;
                sc_strcpy,      /* DT_BYTE */
                sc_strcpy,      /*DT_VARBYTE */
                sc_strcpy,      /* DT_BLOB */
                sc_strcpy,      /* DT_TEXT */
                sc_strcpy,      /* DT_TIMESTAMP */
                sc_strcpy,      /* DT_DATETIME */
                sc_strcpy,      /* DT_DATE */
                sc_strcpy,      /* DT_TIME */
                sc_strcpy,      /* DT_HEX */
                sc_strcpy       /* DT_OID */
    };

    char *sc_strncpy2(char *s1, const char *s2, int n);
    char *sc_wstrncpy(char *s1, char *s2, int n);

    char *sc_strncpy_byte(char *s1, const char *s2, int n);

    char *(*strncpy_func[]) () =
    {
        sc_strncpy2,    /* DT_NULL_TYPE */
                sc_strncpy2,    /* DT_BOOL */
                sc_strncpy2,    /* DT_TINYINT */
                sc_strncpy2,    /* DT_SMALLINT */
                sc_strncpy2,    /* DT_INTEGER */
                sc_strncpy2,    /* DT_LONG */
                sc_strncpy2,    /* DT_BIGINT */
                sc_strncpy2,    /* DT_FLOAT */
                sc_strncpy2,    /* DT_DOUBLE */
                sc_strncpy2,    /* DT_DECIMAL */
                sc_strncpy2,    /* DT_CHAR */
                sc_wstrncpy,    /* DT_NCHAR */
                sc_strncpy2,    /* DT_VARCHAR */
                sc_wstrncpy,    /* DT_NVARCHAR */
                // 酒府价窃.. -_-;
                sc_strncpy_byte,        /* DT_BYTE */
                sc_strncpy_byte,        /*DT_VARBYTE */
                sc_strncpy2,    /* DT_BLOB */
                sc_strncpy2,    /* DT_TEXT */
                sc_strncpy2,    /* DT_TIMESTAMP */
                sc_strncpy2,    /* DT_DATETIME */
                sc_strncpy2,    /* DT_DATE */
                sc_strncpy2,    /* DT_TIME */
                sc_strncpy2,    /* DT_HEX */
                sc_strncpy2     /* DT_OID */
    };

    void *sc_memcpy2(void *mem1, void *mem2, size_t len);

    __DECL_PREFIX void *(*memcpy_func[]) () =
    {
        sc_memcpy2,     /* DT_NULL_TYPE, */
                sc_memcpy2,     /* DT_BOOL */
                sc_memcpy2,     /* DT_TINYINT */
                sc_memcpy2,     /* DT_SMALLINT */
                sc_memcpy2,     /* DT_INTEGER */
                sc_memcpy2,     /* DT_LONG */
                sc_memcpy2,     /* DT_BIGINT */
                sc_memcpy2,     /* DT_FLOAT */
                sc_memcpy2,     /* DT_DOUBLE */
                sc_memcpy2,     /* DT_DECIMAL */
                sc_memcpy2,     /* DT_CHAR */
                sc_wmemcpy,     /* DT_NCHAR */
                sc_memcpy2,     /* DT_VARCHAR */
                sc_wmemcpy,     /* DT_NVARCHAR */
                // 酒府价窃.. -_-;
                sc_memcpy2,     /* DT_BYTE */
                sc_memcpy2,     /*DT_VARBYTE */
                sc_memcpy2,     /* DT_BLOB */
                sc_memcpy2,     /* DT_TEXT */
                sc_memcpy2,     /* DT_TIMESTAMP */
                sc_memcpy2,     /* DT_DATETIME */
                sc_memcpy2,     /* DT_DATE */
                sc_memcpy2,     /* DT_TIME */
                sc_memcpy2,     /* DT_HEX */
                sc_memcpy2      /* DT_OID */
    };

    char *sc_strstr2(const char *haystack, const char *needle);
    char *sc_wcsstr2(const char *haystack, const char *needle);

    char *(*strstr_func[]) () =
    {
        sc_strstr2,     /* DT_NULL_TYPE, */
                sc_strstr2,     /* DT_BOOL */
                sc_strstr2,     /* DT_TINYINT */
                sc_strstr2,     /* DT_SMALLINT */
                sc_strstr2,     /* DT_INTEGER */
                sc_strstr2,     /* DT_LONG */
                sc_strstr2,     /* DT_BIGINT */
                sc_strstr2,     /* DT_FLOAT */
                sc_strstr2,     /* DT_DOUBLE */
                sc_strstr2,     /* DT_DECIMAL */
                sc_strstr2,     /* DT_CHAR */
                sc_wcsstr2,     /* DT_NCHAR */
                sc_strstr2,     /* DT_VARCHAR */
                sc_wcsstr2,     /* DT_NVARCHAR */
                // 酒府价窃.. -_-;
                sc_strstr2,     /* DT_BYTE */
                sc_strstr2,     /*DT_VARBYTE */
                sc_strstr2,     /* DT_BLOB */
                sc_strstr2,     /* DT_TEXT */
                sc_strstr2,     /* DT_TIMESTAMP */
                sc_strstr2,     /* DT_DATETIME */
                sc_strstr2,     /* DT_DATE */
                sc_strstr2,     /* DT_TIME */
                sc_strstr2,     /* DT_HEX */
                sc_strstr2      /* DT_OID */
    };

#else
    extern int SizeOfType[];
    extern __DECL_PREFIX int (*strlen_func[]) ();
    extern int (*strcmp_func[]) ();
    extern int (*strncmp_func[]) ();
    extern int (*strncasecmp_func[]) ();
    extern char *(*strcpy_func[]) ();
    extern char *(*strncpy_func[]) ();
    extern __DECL_PREFIX void *(*memcpy_func[]) ();
    extern char *(*strstr_func[]) ();
#endif

#define SCHEMA_LOAD_REGU_TBL    0x01
#define SCHEMA_LOAD_REGU_IDX    0x02
#define SCHEMA_LOAD_TEMP_TBL    0x04
#define SCHEMA_LOAD_TEMP_IDX    0x08
#define SCHEMA_LOAD_REGU    (SCHEMA_LOAD_REGU_TBL | SCHEMA_LOAD_REGU_IDX)
#define SCHEMA_LOAD_TEMP    (SCHEMA_LOAD_TEMP_TBL | SCHEMA_LOAD_TEMP_IDX)
#define SCHEMA_LOAD_ALL     (SCHEMA_LOAD_REGU | SCHEMA_LOAD_TEMP)
    extern int _Schema_Cache_Init(int schema_load_flag);
    extern int _Schema_Cache_Free(int schema_load_flag);
    extern int _Schema_Cache_Dump(void);
    extern int _Schema_Cache_InsertTable_Undo(OID table_oid, char *data);
    extern int _Schema_Cache_InsertIndex_Undo(OID index_oid, char *data);
    extern int _Schema_Cache_DeleteTable_Undo(OID table_oid);
    extern int _Schema_Cache_DeleteIndex_Undo(OID index_oid);
    extern int _Schema_Cache_UpdateTable_Undo(OID table_oid,
            char *data, int data_leng);
    extern int _Schema_Cache_UpdateIndex_Undo(OID index_oid,
            char *data, int data_leng);
    extern int _Schema_InputTable(int transId, char *tableName,
            int tableId, ContainerType type, DB_BOOL has_variabletype,
            int recordLen, int heap_recordLen, int numFields,
            FIELDDESC_T * pFieldDesc, int max_records, char *column_name);
    extern int _Schema_InputIndex(int transId, char *tableName,
            char *indexName, int numFields, FIELDDESC_T * pFieldDesc,
            DB_UINT8 flag);
    extern int _Schema_RemoveTable(int transId, char *tableName);
    extern int _Schema_RemoveIndex(int transId, char *indexName);
    extern int _Schema_RenameTable(int transId, char *old_name,
            char *new_name);

    extern int _Schema_RenameIndex(int transId, char *old_name,
            char *new_name);
    extern int _Schema_GetTableInfo(char *tableName, SYSTABLE_T * pSysTable);
    extern int _Schema_GetFieldsInfo(char *tableName, SYSFIELD_T * pFieldInfo);

    extern int _Schema_GetSingleFieldsInfo(char *tableName, char *field_name,
            SYSFIELD_T * pFieldInfo);

    extern int _Schema_GetTableIndexesInfo(char *tableName,
            int schIndexType, SYSINDEX_T * pSysIndex);
    extern int _Schema_GetIndexInfo(char *indexName, SYSINDEX_T * pSysIndex,
            SYSINDEXFIELD_T * pSysIndexField, char *tableName);
    extern int _Schema_GetIndexInfo2(char *indexName, SYSINDEX_T * pSysIndex,
            SYSINDEXFIELD_T * pSysIndexField, char *tableName,
            OID table_cont_oid, OID * index_cont_oid);
    extern int _Schema_Trans_TempObjectExist(int transId);
    extern int _Schema_Trans_FirstTempTableName(int transId, char *tableName);
    extern int _Schema_Trans_FirstTempIndexName(int transId, char *index_name);
    extern int _Schema_CheckFieldDesc(int numFields,
            FIELDDESC_T * pFieldDesc, DB_BOOL is_heaprecord);
    extern int _Schema_MakeKeyDesc(int transId, char *tableName, int numFields,
            FIELDDESC_T * pFieldDesc, struct KeyDesc *key);

    extern int _Schema_ChangeTableProperty(int transId, char *table_name,
            ContainerType type);

    extern int _Schema_SetTable_modifiedtimes(char *tableName);
    extern int _Schema_GetTable_modifiedtimes(char *tableName);

    int KeyValue_Compare(struct KeyValue *keyvalue, char *record, int recSize);
    extern int KeyValue_Delete(struct KeyValue *pKeyValue);
    int Filter_Compare(struct Filter *, char *, int);

    extern int KeyDesc_Compare(struct KeyDesc *keydesc,
            const char *r1, const char *r2, int recSize1, int recSize2);
    int FieldDesc_Compare(KEY_FIELDDESC_T *, const char *, const char *);

    extern int dbi_FieldDesc(char *fieldname, DataType type, DB_UINT32 length,
            DB_INT32 scale, DB_UINT8 flag, void *defaultValue,
            DB_INT32 fixed_part, FIELDDESC_T * fieldDesc,
            MDB_COL_TYPE collation);

    typedef struct stDB_SEQUENCE
    {
        char sequenceName[FIELD_NAME_LENG];
        DB_INT32 startNum;
        DB_INT32 increaseNum;
        DB_INT32 maxNum;
        DB_INT32 minNum;
        DB_INT8 cycled;         /* TRUE/FALSE */
    } DB_SEQUENCE;

    extern int dbi_Sequence_Create(int connid, DB_SEQUENCE * pDbSequence);
    extern int dbi_Sequence_Alter(int connid, DB_SEQUENCE * pDbSequence);
    int dbi_Sequence_Drop(int connid, char *sequenceName);
    int dbi_Sequence_Read(int connid, char *sequenceName,
            DB_SEQUENCE * pSequence);
    int dbi_Sequence_Currval(int connid, char *sequenceName, int *pCurVal);
    extern int dbi_Sequence_Nextval(int connid,
            char *sequenceName, int *pNextVal);

    int dbi_prepare_like(char *likestr, DataType type, int mode,
            int nfixedbytelen, void **ppLikeInfo);

    extern int dbi_like_compare(char *value, DataType type, int fixed_part,
            void *likevalue, int mode, MDB_COL_TYPE collation);

    int dbi_Sort_rid(OID * pszOids, int nMaxCnt, DB_BOOL IsAsc);

    extern int dbi_Rid_Drop(int connid, OID rid);
    extern int dbi_Rid_Update(int connid, OID rid,
            struct UpdateValue *newValues);
    extern int dbi_Rid_Tablename(int connid, OID rid, char *tablename);

    extern int dbi_Table_Size(int connid, char *tablename);
    extern int dbi_Index_Size(int connid, char *tablename);

    extern int dbi_Index_Rebuild(int connid, int num, char **obj_name);

    extern int dbi_Get_numLoadSegments(int connid, void *buf);

    extern int dbi_Relation_Drop_byUser(int connid,
            char *relation_name, int userid);
    extern int dbi_Cursor_Open_byUser(int connid,
            char *relation_name, char *indexName, struct KeyValue *min,
            struct KeyValue *max, struct Filter *filter, LOCKMODE lock,
            int userid, DB_UINT8 msync_flag);
    extern int dbi_Cursor_Open_Desc_byUser(int connid, char *relation_name,
            char *indexName, struct KeyValue *min, struct KeyValue *max,
            struct Filter *filter, LOCKMODE lock, int userid);
    extern int dbi_Direct_Remove_byUser(int connid,
            char *tableName, char *indexName, struct KeyValue *findKey,
            OID rid_to_remove, LOCKMODE lock, int userid);
    extern int dbi_Direct_Remove2_byUser(int connid, int inputCsrId,
            char *relation_name, char *indexName, struct KeyValue *min,
            struct KeyValue *max, struct Filter *filter, LOCKMODE lock,
            int userid);
    extern int dbi_Direct_Update_byUser(int connid,
            char *tableName, char *indexName, struct KeyValue *findKey,
            OID rid_to_update, char *record, int record_leng, LOCKMODE lock,
            int userid, struct UpdateValue *newValues);
    extern int dbi_Direct_Update_Field_byUser(int connid,
            char *tableName, char *indexName, struct KeyValue *findKey,
            struct UpdateValue *newValues, LOCKMODE lock, int userid);
    extern int dbi_Relation_Rename_byUser(int connid, char *old_name,
            char *new_name, int userid);
    extern int dbi_Index_Rename_byUser(int connid, char *old_name,
            char *new_name, int userid);
    extern int dbi_Relation_Truncate_byUser(int connid,
            char *relation_name, int userid);
    extern int dbi_Index_Drop_byUser(int connid, char *indexName, int userid);
    extern int dbi_Schema_GetUserTablesInfo_byUser(int connid,
            SYSTABLE_T ** pSysTable, int userid);

#define FREE_MEM_TO_VALUE(type_, value_)                \
    do {                                                \
        switch(type_)                                   \
        {                                               \
        case DT_CHAR: case DT_VARCHAR:                  \
        case DT_BYTE: case DT_VARBYTE:                  \
        case DT_NCHAR: case DT_NVARCHAR:                \
            PMEM_FREENUL(value_->Wpointer_);            \
            break;                                      \
        default:                                        \
            break;                                      \
        }                                               \
    }while(0)

#define ALLOC_MEM_TO_VALUE(defined_len_, fld_val_)                        \
    do {                                                                    \
        if (fld_val_->type_ == DT_DECIMAL)                                  \
            defined_len_ = fld_val_->length_ + 3;                           \
        else                                                                \
            defined_len_ = fld_val_->length_ * SizeOfType[fld_val_->type_]; \
        if (fld_val_->value_.pointer_ == NULL)                              \
        {                                                                   \
            fld_val_->value_.pointer_ =                                     \
            PMEM_XCALLOC(defined_len_ + WCHAR_SIZE);                        \
            if (fld_val_->value_.pointer_ == NULL)                          \
            {                                                               \
                er_set(ER_ERROR_SEVERITY,                                   \
                        ARG_FILE_LINE, DB_E_OUTOFTHRMEMORY, 0);             \
                defined_len_ = DB_E_OUTOFTHRMEMORY;                         \
            }                                                               \
        }                                                                   \
    } while(0)

    int dbi_Table_Export(int conn_id, char *export_file, char *table_name,
            int f_append);
    int dbi_Table_Export_Schema(int conn_id, char *export_file,
            char *table_name, int f_append, int isAll);
    int dbi_Table_Import(int conn_id, char *import_file);

    extern void dbi_GetServerLogPath(char *pSrvPath);

    int dbi_Relation_Logging(int connid, char *relationName, int logging,
            int duringRuntime, int userid);

#define MAX_SHOW_RECSIZE 8000

    typedef struct
    {
        int fd;
        int max_rec_len;
        int rec_pos;
        char *rec_buf;
    } T_RESULT_SHOW;

    int dbi_Relation_Show(int connid, char *tablename,
            T_RESULT_SHOW * res_show, int isAll);
    int dbi_All_Relation_Show(int connid, T_RESULT_SHOW * res_show);

    extern int dbi_Cursor_Insert_Variable(int connid, int cursorID,
            char *value, int extend_size, OID * var_rid);

    typedef enum _upsert_cmpmode
    {
        UPSERT_CMP_DEFAULT = 0,
        UPSERT_CMP_ENTIRERECORD,
        UPSERT_CMP_SKIP,
        UPSERT_CMP_MAX = MDB_INT_MAX
    } UPSERT_CMPMODE;

    typedef enum _upsert_result
    {
        UPSERT_EXIST = 0,
        UPSERT_INSERT = 1,
        UPSERT_UPDATE = 2,
        UPSERT_RESULT_MAX = MDB_INT_MAX
    } UPSERT_RESULT;

    int dbi_Direct_Upsert(int connid, char *tableName,
            char *record, int record_leng, LOCKMODE lock);

    int __Upsert(int connid, int cursorId,
            struct Container *cont, char *record, int record_leng,
            int *upsert_cols, OID * insertd_rid, UPSERT_CMPMODE compareMode);
    int dbi_Cursor_Upsert(int connid, int cursorId,
            char *record, int record_leng, int *upsert_cols, OID * insertd_rid,
            UPSERT_CMPMODE compareMode);

    int dbi_check_valid_rid(char *tablename, OID rid);

    int dbi_FlushBuffer(void);
    int dbi_does_table_has_rid(char *tablename, OID rid);

    int dbi_Trans_Set_RU_fUseResult(int connid);

    int dbi_Relation_mSync_Alter(int connid, char *tablename, int enable);
    int dbi_Relation_mSync_Flush(int connid, char *tablename, int flag);

    struct UpdateValue dbi_UpdateValue_Create(DB_UINT32 n);
    int dbi_UpdateValue_Destroy(struct UpdateValue *updValues);

#ifdef  __cplusplus
}
#endif

#endif                          /* _DB_API_H */
