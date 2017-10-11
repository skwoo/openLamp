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

#ifndef _SCHEMA_CACHE_H
#define _SCHEMA_CACHE_H

#include "mdb_config.h"
#include "mdb_Trans.h"

/*************************/
/* schema node structure */
/*************************/
/* regular table and index schema node: save pointer to schema info */
/* temp    table and index schema node: save schema info */

typedef struct _regu_idxsch_node
{
    OID table_oid;
    OID index_oid;
    OID table_cont_oid;
    OID index_cont_oid;
    OID *field_oid_list;
    struct _regu_idxsch_node *next;
} REGU_IDXSCH_NODE_T;

typedef struct _regu_tabsch_node
{
    OID table_oid;
    OID table_cont_oid;
    OID *field_oid_list;
    OID *index_oid_list;
    short num_regu_indexes;
    short max_regu_indexes;
    short num_temp_indexes;
    short reserverd;
    /* table 변경(insert, delete, update) count 정보 */
    unsigned int modified_times;
    unsigned char temp_indexID_map[4];  /* 32 temp indexes (32 - 63) */
    struct _regu_tabsch_node *next;
} REGU_TABSCH_NODE_T;

typedef struct _temp_idxsch_node
{
    char table_name[REL_NAME_LENG];
    SYSINDEX_T index_info;
    SYSINDEXFIELD_T *field_info_list;
    struct _temp_idxsch_node *next;
} TEMP_IDXSCH_NODE_T;

typedef struct _temp_tabsch_node
{
    SYSTABLE_T table_info;
    SYSFIELD_T *field_info_list;
    struct _temp_tabsch_node *next;
} TEMP_TABSCH_NODE_T;

/************************/
/* hash entry structure */
/************************/

typedef struct _regu_idxsch_hash
{
    int count;
    REGU_IDXSCH_NODE_T *head;
} REGU_IDXSCH_HASH_T;

typedef struct _regu_tabsch_hash
{
    int count;
    REGU_TABSCH_NODE_T *head;
} REGU_TABSCH_HASH_T;

typedef struct _temp_idxsch_hash
{
    int count;
    TEMP_IDXSCH_NODE_T *head;
} TEMP_IDXSCH_HASH_T;

typedef struct _temp_tabsch_hash
{
    int count;
    TEMP_TABSCH_NODE_T *head;
} TEMP_TABSCH_HASH_T;

/****************/
/* Schema Cache */
/****************/
/* schmea hash table size */
#define REGU_TABSCH_HASH_SIZE   21
#define REGU_IDXSCH_HASH_SIZE   41
#define TEMP_TABSCH_HASH_SIZE   MAX_TRANS
#define TEMP_IDXSCH_HASH_SIZE   MAX_TRANS

typedef struct _schema_cache
{
    REGU_TABSCH_HASH_T HashTable_ReguTabSch[REGU_TABSCH_HASH_SIZE];
    REGU_IDXSCH_HASH_T HashTable_ReguIdxSch[REGU_IDXSCH_HASH_SIZE];
    TEMP_TABSCH_HASH_T HashTable_TempTabSch[TEMP_TABSCH_HASH_SIZE];
    TEMP_IDXSCH_HASH_T HashTable_TempIdxSch[TEMP_IDXSCH_HASH_SIZE];
} SCHEMA_CACHE_T;

extern __DECL_PREFIX SCHEMA_CACHE_T *Schema_Cache;

#endif /* _SCHEMA_CACHE_H */
