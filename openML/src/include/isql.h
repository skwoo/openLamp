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

#ifndef _ISQL_H
#define _ISQL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "mdb_config.h"
#include "mdb_define.h"
#include "db_api.h"

#define iSQL_QUERY_MAX              (8*1024)

    /* handle status */
#define iSQL_STAT_NOT_READY         0
#define iSQL_STAT_CONNECTED         1
#define iSQL_STAT_READY             2
#define iSQL_STAT_IN_USE            3
#define iSQL_STAT_USE_RESULT        4
#define iSQL_STAT_PREPARED          5
#define iSQL_STAT_EXECUTED          6

    /* field flag; bit-vector */
#define iSQL_FIELD_FLAG_PRI         FD_KEY
#define iSQL_FIELD_FLAG_NULL        FD_NULLABLE
#define iSQL_FIELD_FLAG_AUTO        FD_AUTOINCREMENT

    /* scroll option */
#define iSQL_DATA_SEEK_START        0
#define iSQL_DATA_SEEK_CURRENT      1
#define iSQL_DATA_SEEK_END          2

    /* status information */
#define iSQL_NO_DATA                100


    /* SQL statement types */
    typedef enum
    {
        SQL_STMT_NONE = 0,
        SQL_STMT_SELECT,
        SQL_STMT_INSERT,
        SQL_STMT_UPDATE,
        SQL_STMT_DELETE,
        SQL_STMT_UPSERT,
        SQL_STMT_DESCRIBE,
        SQL_STMT_CREATE,
        SQL_STMT_RENAME,
        SQL_STMT_ALTER,
        SQL_STMT_TRUNCATE,
        SQL_STMT_ADMIN,
        SQL_STMT_DROP,
        SQL_STMT_COMMIT,
        SQL_STMT_ROLLBACK,
        SQL_STMT_SET,
        SQL_STMT_DUMMY = MDB_INT_MAX    /* to guarantee sizeof(enum) == 4 */
    } isql_stmt_type;

    typedef enum
    {
        SQL_RES_BINARY = 0,
        SQL_RES_STRING,
        SQL_RES_DUMMY = MDB_INT_MAX     /* to guarantee sizeof(enum) == 4 */
    } isql_res_type;

    /* SQL data types */
    typedef enum
    {
        SQL_DATA_NONE = 0,
        SQL_DATA_TINYINT = DT_TINYINT,
        SQL_DATA_SMALLINT = DT_SMALLINT,
        SQL_DATA_INT = DT_INTEGER,
        SQL_DATA_BIGINT = DT_BIGINT,
        SQL_DATA_FLOAT = DT_FLOAT,
        SQL_DATA_DOUBLE = DT_DOUBLE,
        SQL_DATA_DECIMAL = DT_DECIMAL,
        SQL_DATA_CHAR = DT_CHAR,
        SQL_DATA_VARCHAR = DT_VARCHAR,
        SQL_DATA_NCHAR = DT_NCHAR,
        SQL_DATA_NVARCHAR = DT_NVARCHAR,
        SQL_DATA_BYTE = DT_BYTE,
        SQL_DATA_VARBYTE = DT_VARBYTE,
        SQL_DATA_TIMESTAMP = DT_TIMESTAMP,
        SQL_DATA_DATETIME = DT_DATETIME,
        SQL_DATA_DATE = DT_DATE,
        SQL_DATA_TIME = DT_TIME,
        SQL_DATA_OID = DT_OID,
        SQL_DATA_DUMMY = MDB_INT_MAX    /* to guarantee sizeof(enum) == 4 */
    } isql_data_type;


    /* bit-vector flag for client */
    typedef enum
    {
        OPT_AUTOCOMMIT = 0x0001,
        OPT_TIME = 0x0002,
        OPT_HEADING = 0x0004,
        OPT_FEEDBACK = 0x0008,
        OPT_RECONNECT = 0x0010,
        OPT_PLAN_ON = 0x0020,
        OPT_PLAN_OFF = 0x0040,
        OPT_PLAN_ONLY = 0x0080,
        OPT_PLAN_DUMMY = MDB_INT_MAX    /* to guarantee sizeof(enum) == 4 */
    } isql_option;

#define OPT_DEFAULT   (OPT_HEADING|OPT_FEEDBACK|OPT_RECONNECT)


    typedef char **iSQL_ROW;
    typedef unsigned long int *iSQL_LENGTH;

    typedef struct tag_isql_rows
    {
        OID rid;
        int offset;
        iSQL_ROW data;
        struct tag_isql_rows *next;
    } iSQL_ROWS;

    typedef struct
    {
        char name[FIELD_NAME_LENG];     /* name of column */
        char base_name[FIELD_NAME_LENG];        /* base nameof column */
        char table[REL_NAME_LENG];      /* name of table */
        char def[MAX_FIELD_VALUE_SIZE]; /* default value of field */
        int type;               /* type of field */
        unsigned int length;    /* width of column */
        unsigned int max_length;        /* max width of column */
        unsigned int flags;     /* bit-flags for field */
        unsigned int decimals;  /* number of decimals in field */
        unsigned int buffer_length;     /* data length as bytes.
                                           in case of string types it means
                                           the number of characters */
    } iSQL_FIELD;

    typedef struct tag_isql_result_page
    {
        int body_size;
        int rows_in_page;
        struct tag_isql_result_page *next;
        char body[1];
    } iSQL_RESULT_PAGE;

    typedef struct
    {
        unsigned long int row_num;
        unsigned int field_count;
        iSQL_ROWS *data;
        iSQL_RESULT_PAGE *result_page_list;
        char *data_space;
        iSQL_ROWS *rows_buf;
        iSQL_ROW data_buf;
        int buf_len;
        int use_eof;
    } iSQL_DATA;

    struct tag_isql;
    typedef struct
    {
        unsigned char dont_touch;       /* check if result set for iSQL_STMT */
        struct tag_isql *isql;
        unsigned long int row_count;    /* means # of rows fetched */
        unsigned int field_count;       /* # of iSQL_FIELD */
        unsigned int current_field;     /* for iSQL_fetch_field() */
        iSQL_FIELD *fields;     /* information of column */
        iSQL_DATA *data;
        iSQL_ROWS *data_cursor; /* next data position */
        long int data_offset;   /* current data offset,
                                   for cursor scrolling */
        iSQL_LENGTH lengths;    /* array as length of column */
        iSQL_ROW row;           /* if unbeffered read */
        iSQL_ROW current_row;   /* current row fetched */
        OID current_rid;
        int rid_included;
        int eof;
        int is_partial_result;
        DB_BOOL bFirst;
    } iSQL_RES;

    typedef struct
    {
        unsigned long int *length;      /* output length pointer:value_len */
        int *is_null;           /* null indicator */
        isql_data_type buffer_type;     /* buffer type */
        unsigned long int buffer_length;        /* the fields's definition length */
        char *buffer;           /* target buffer */
    } iSQL_BIND;

    typedef struct
    {
        unsigned int stmt_id;   /* statement id */
        isql_stmt_type querytype;       /* query type */
        isql_res_type resulttype;
        struct tag_isql *isql;  /* connection */
        unsigned int affected_rows;
        OID affected_rid;       /* _64BIT */
        unsigned int field_count;       /* # of iSQL_FIELDs */
        unsigned int param_count;       /* # of parameter markers */
        iSQL_RES *res;          /* result set */
        iSQL_BIND *pbind;       /* parameter binding */
        iSQL_BIND *rbind;       /* record binding */
        iSQL_FIELD *fields;     /* field information */
        iSQL_FIELD *pfields;    /* field info for parameter binding */
        DB_BOOL is_set_bind_param;

        int query_timeout;      /* 0 means timeoutless */

        char *bind_space;
        int *indicator_space;
        unsigned long int *length_space;
    } iSQL_STMT;

    typedef struct tag_isql
    {
        int status;
        char dbname[256];
        unsigned short int port;
        unsigned short int free_me;
        int handle;
        int server_handle;
        int flags;
        iSQL_FIELD *fields;
        unsigned int field_count;
        unsigned int affected_rows;
        OID affected_rid;       /* _64BIT */
        int last_errno;
        char *last_error;
        isql_stmt_type last_querytype;
        iSQL_DATA *data;        /* for fecthed data */
        int clientType;
    } iSQL;

    typedef struct
    {
        unsigned short int year;
        unsigned short int month;
        unsigned short int day;
        unsigned short int hour;
        unsigned short int minute;
        unsigned short int second;
        unsigned short int fraction;    /* milli-second */
    } iSQL_TIME;

    typedef iSQL_ROWS *iSQL_ROW_OFFSET;
    typedef unsigned long int iSQL_FIELD_OFFSET;


    __DECL_PREFIX iSQL *iSQL_init(iSQL * isql);
    __DECL_PREFIX int iSQL_initdb(void);
    __DECL_PREFIX iSQL *iSQL_connect(iSQL * isql,
            char *dbhost, char *dbname, char *user, char *passwd);
    __DECL_PREFIX iSQL *iSQL_real_connect(iSQL * isql,
            char *dbhost, char *dbname, char *user, char *passwd,
            unsigned short port, unsigned int client_flag);
    __DECL_PREFIX int iSQL_disconnect(iSQL * isql);

    __DECL_PREFIX iSQL_STMT *iSQL_prepare(iSQL * isql,
            char *query, unsigned long int length);
    __DECL_PREFIX iSQL_STMT *iSQL_prepare2(iSQL * isql, char *query,
            unsigned long int length, isql_res_type resulttype);
    __DECL_PREFIX unsigned int iSQL_param_count(iSQL_STMT * stmt);
    __DECL_PREFIX int iSQL_describe_param(iSQL_STMT * stmt,
            iSQL_FIELD ** fields);
    __DECL_PREFIX iSQL_RES *iSQL_prepare_result(iSQL_STMT * stmt);
    __DECL_PREFIX int iSQL_execute(iSQL_STMT * stmt);
    __DECL_PREFIX int iSQL_stmt_set_timeout(iSQL_STMT * stmt, int value);

    __DECL_PREFIX int iSQL_stmt_store_result(iSQL_STMT * stmt);
    __DECL_PREFIX int iSQL_stmt_use_result(iSQL_STMT * stmt);
    __DECL_PREFIX int iSQL_bind_param(iSQL_STMT * stmt, iSQL_BIND * bind);
    __DECL_PREFIX int iSQL_bind_result(iSQL_STMT * stmt, iSQL_BIND * bind);
    __DECL_PREFIX int iSQL_fetch(iSQL_STMT * stmt);
    __DECL_PREFIX isql_stmt_type iSQL_stmt_querytype(iSQL_STMT * stmt);
    __DECL_PREFIX unsigned int iSQL_stmt_affected_rows(iSQL_STMT * stmt);
    __DECL_PREFIX OID iSQL_stmt_affected_rid(iSQL_STMT * stmt); /* _64BIT */
    __DECL_PREFIX int iSQL_stmt_close(iSQL_STMT * stmt);
    __DECL_PREFIX int iSQL_stmt_errno(iSQL_STMT * stmt);
    __DECL_PREFIX char *iSQL_stmt_error(iSQL_STMT * stmt);
    __DECL_PREFIX int iSQL_query(iSQL * isql, char *query);
    __DECL_PREFIX iSQL_RES *iSQL_use_result(iSQL * isql);
    __DECL_PREFIX iSQL_RES *iSQL_store_result(iSQL * isql);
    __DECL_PREFIX iSQL_ROW iSQL_fetch_row(iSQL_RES * res);
    __DECL_PREFIX iSQL_LENGTH iSQL_fetch_lengths(iSQL_RES * res);
    __DECL_PREFIX iSQL_FIELD *iSQL_fetch_field(iSQL_RES * res);
    __DECL_PREFIX iSQL_FIELD *iSQL_fetch_field_direct(iSQL_RES * res,
            unsigned int fieldnr);
    __DECL_PREFIX iSQL_FIELD *iSQL_fetch_fields(iSQL_RES * res);
    __DECL_PREFIX iSQL_ROW_OFFSET iSQL_row_tell(iSQL_RES * res);
    __DECL_PREFIX iSQL_FIELD_OFFSET iSQL_field_tell(iSQL_RES * res);
    __DECL_PREFIX iSQL_RES *iSQL_list_fields(iSQL * isql,
            char *table, char *column);
    __DECL_PREFIX isql_data_type iSQL_get_fieldtype(iSQL_RES * res,
            const unsigned int fieldid);
    __DECL_PREFIX unsigned int iSQL_num_rows(iSQL_RES * res);
    __DECL_PREFIX unsigned int iSQL_num_fields(iSQL_RES * res);
    __DECL_PREFIX long int iSQL_data_seek(iSQL_RES * res,
            long int offset, int whence);

    __DECL_PREFIX long int iSQL_data_tell(iSQL_RES * res);
    __DECL_PREFIX iSQL_ROW_OFFSET iSQL_row_seek(iSQL_RES * res,
            iSQL_ROW_OFFSET row);
    __DECL_PREFIX iSQL_FIELD_OFFSET iSQL_field_seek(iSQL_RES * res,
            iSQL_FIELD_OFFSET field_offset);
    __DECL_PREFIX int iSQL_eof(iSQL_RES * res);
    __DECL_PREFIX void iSQL_free_result(iSQL_RES * res);
    __DECL_PREFIX isql_stmt_type iSQL_last_querytype(iSQL * isql);
    __DECL_PREFIX int iSQL_options(iSQL * isql, const isql_option opt,
            const void *optval, int optlen);
    __DECL_PREFIX unsigned int iSQL_field_count(iSQL * isql);
    __DECL_PREFIX unsigned int iSQL_affected_rows(iSQL * isql);
    __DECL_PREFIX OID iSQL_affected_rid(iSQL * isql);   /* _64BIT */
    __DECL_PREFIX int iSQL_begin_transaction(iSQL * isql);
    __DECL_PREFIX int iSQL_commit(iSQL * isql);
    __DECL_PREFIX int iSQL_commit_flush(iSQL * isql);
    __DECL_PREFIX int iSQL_rollback(iSQL * isql);
    __DECL_PREFIX int iSQL_rollback_flush(iSQL * isql);
    __DECL_PREFIX int iSQL_errno(iSQL * isql);
    __DECL_PREFIX char *iSQL_error(iSQL * isql);
    __DECL_PREFIX char *iSQL_get_plan_string(iSQL * isql);
    __DECL_PREFIX char *iSQL_stmt_plan_string(iSQL_STMT * stmt);
    __DECL_PREFIX int iSQL_query_end(iSQL * isql);

    __DECL_PREFIX int iSQL_describe(iSQL_STMT * stmt, iSQL_BIND ** bind_param,
            iSQL_BIND ** bind_res);
    __DECL_PREFIX int iSQL_num_parameter_fields(iSQL_STMT * stmt);
    __DECL_PREFIX int iSQL_num_result_fields(iSQL_STMT * stmt);

    __DECL_PREFIX int iSQL_nquery(iSQL * isql, DB_WCHAR * wQuery);

    __DECL_PREFIX int iSQL_get_rid(iSQL_STMT * stmt, OID * rid);
    __DECL_PREFIX int iSQL_desc_update_rid(iSQL * isql, OID rid, int numfields,
            iSQL_FIELD * fielddesc, iSQL_BIND * fieldvalue);
    __DECL_PREFIX int iSQL_update_rid(iSQL * isql, OID rid, char *fieldname,
            iSQL_BIND * fieldvalue);
    __DECL_PREFIX int iSQL_drop_rid(iSQL * isql, OID rid);

    __DECL_PREFIX iSQL_STMT *iSQL_nprepare(iSQL * isql, DB_WCHAR * wQuery,
            unsigned long int length);
    __DECL_PREFIX iSQL_STMT *iSQL_nprepare2(iSQL * isql, DB_WCHAR * wQuery,
            unsigned long int length, isql_res_type resulttype);

    __DECL_PREFIX int iSQL_Rid_Read(iSQL * isql, OID record_oid, int dColCnt,
            char **ppColumns, char *pDbTblName, iSQL_BIND ** pResult);
    __DECL_PREFIX int iSQL_Rid_ReadFree(iSQL_BIND ** pResult, int dColCnt);


    //----------------------------------------------------------

    __DECL_PREFIX int iSQL_checkpoint(iSQL * isql);

    __DECL_PREFIX int iSQL_flush_buffer(void);

    __DECL_PREFIX int iSQL_get_numLoadSegments(iSQL * isql, int *buf);

#ifdef __cplusplus
}
#endif

#endif                          /* _ISQL_H */
