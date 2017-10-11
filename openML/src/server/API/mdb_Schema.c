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

#include "mdb_config.h"
#include "dbport.h"
#include "mdb_compat.h"
#include "mdb_Server.h"
#include "mdb_PMEM.h"

#define INDEX_LIST_INCREMENT    4

int KeyFieldDesc_Create(KEY_FIELDDESC_T * fielddesc,
        SYSFIELD_T * pSysField, char order, MDB_COL_TYPE * collation);

SCHEMA_CACHE_T *Schema_Cache = NULL;

static int
GET_REGU_TABSCH_HASH_VALUE(const char *tableName)
{
    register unsigned int hash_val = 0;
    register DB_UINT32 count = 0;
    register char c;

    for (count = 0;; count++)
    {
        c = tableName[count];
        if (c == '\0')
            break;
        hash_val += (('a' <= c && c <= 'z') ? c & 0xdf : c);
    }
    return (hash_val % REGU_TABSCH_HASH_SIZE);
}

static int
GET_REGU_IDXSCH_HASH_VALUE(const char *indexName)
{
    register unsigned int hash_val = 0;
    register DB_UINT32 count = 0;
    register char c;

    for (count = 0;; count++)
    {
        c = indexName[count];
        if (c == '\0')
            break;
        hash_val += (('a' <= c && c <= 'z') ? c & 0xdf : c);
    }
    return (hash_val % REGU_IDXSCH_HASH_SIZE);
}

static REGU_TABSCH_NODE_T *
GET_REGU_TABSCH_NODE(char *tableName, REGU_TABSCH_HASH_T * tab_hash,
        OID table_oid, MDB_BOOL isPrev, int *ret)
{
    REGU_TABSCH_NODE_T *tab_node;
    REGU_TABSCH_NODE_T *prev_tab_node = NULL;
    SYSTABLE_T *table_info;

    tab_node = DBMem_V2P(tab_hash->head);
    while (tab_node != NULL)
    {
        if (table_oid)
        {
            if (tab_node->table_oid == table_oid)
            {
                break;
            }
        }
        else
        {
            table_info = (SYSTABLE_T *) OID_Point(tab_node->table_oid);
            if (table_info == NULL)
            {
                *ret = DB_E_OIDPOINTER;
                tab_node = NULL;
                goto RETURN_LABEL;
            }
            if (!mdb_strcmp(table_info->tableName, tableName))
            {
                break;
            }
        }

        prev_tab_node = tab_node;
        tab_node = DBMem_V2P(tab_node->next);
    }

    if (tab_node == NULL)
    {
        *ret = DB_E_NOTCACHED;
    }
    else
    {
        if (isPrev)
        {
            if (prev_tab_node == NULL)
            {
                tab_hash->head = tab_node->next;
            }
            else
            {
                prev_tab_node->next = tab_node->next;
            }
            tab_hash->count -= 1;
        }

        *ret = DB_SUCCESS;
    }

  RETURN_LABEL:

    return tab_node;
}

static REGU_IDXSCH_NODE_T *
GET_REGU_IDXSCH_NODE(char *indexName, REGU_IDXSCH_HASH_T * idx_hash,
        OID index_oid, MDB_BOOL isPrev, int *ret)
{
    REGU_IDXSCH_NODE_T *idx_node;
    REGU_IDXSCH_NODE_T *prev_idx_node = NULL;
    SYSINDEX_T *index_info;

    idx_node = DBMem_V2P(idx_hash->head);
    while (idx_node != NULL)
    {
        if (index_oid)
        {
            if (idx_node->index_oid == index_oid)
            {
                break;
            }
        }
        else
        {
            index_info = (SYSINDEX_T *) OID_Point(idx_node->index_oid);
            if (index_info == NULL)
            {
                *ret = DB_E_OIDPOINTER;
                idx_node = NULL;
                goto RETURN_LABEL;
            }
            if (!mdb_strcmp(index_info->indexName, indexName))
            {
                break;
            }
        }

        prev_idx_node = idx_node;
        idx_node = DBMem_V2P(idx_node->next);
    }

    if (idx_node == NULL)
    {
        *ret = DB_E_NOTCACHED;
    }
    else
    {
        if (isPrev)
        {
            if (prev_idx_node == NULL)
            {
                idx_hash->head = idx_node->next;
            }
            else
            {
                prev_idx_node->next = idx_node->next;
            }
            idx_hash->count -= 1;
        }

        *ret = DB_SUCCESS;
    }

  RETURN_LABEL:

    return idx_node;
}

/*****************************************************************************/
//! _Cache_AddTableSchema
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   table_oid     :
 * \param   num_fields    :
 * \param   field_oid_list:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_AddTableSchema(OID table_oid, int num_fields, OID * field_oid_list)
{
    int hash_val;
    REGU_TABSCH_HASH_T *tab_hash;
    REGU_TABSCH_NODE_T *tab_node, *new_node;
    SYSTABLE_T sysTable;
    SYSTABLE_T *pSysTable;
    int ret;
    OID *new_field_oid_list;

    pSysTable = (SYSTABLE_T *) OID_Point(table_oid);
    if (pSysTable == NULL)
    {
        return DB_E_SYSTABLECONT;
    }

    /* To avoid segment buffer replacement */
    sc_memcpy((char *) &sysTable, (char *) pSysTable, sizeof(SYSTABLE_T));
    pSysTable = &sysTable;

    /* build new table schema node */

    new_node = DBMem_Alloc(sizeof(REGU_TABSCH_NODE_T));
    if (new_node == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFDBMEMORY, 0);
        return DB_E_OUTOFDBMEMORY;
    }
    sc_memset(new_node, 0, sizeof(REGU_TABSCH_NODE_T));

    new_field_oid_list = DBMem_Alloc(OID_SIZE * num_fields);
    if (new_field_oid_list == NULL)
    {
        DBMem_Free(new_node);
        new_node = NULL;
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFDBMEMORY, 0);
        return DB_E_OUTOFDBMEMORY;
    }
    sc_memcpy(new_field_oid_list, field_oid_list, (OID_SIZE * num_fields));

    new_node->table_oid = table_oid;
    new_node->field_oid_list = DBMem_P2V(new_field_oid_list);

    /* insert the new table schema node into schema hash table */

    hash_val = GET_REGU_TABSCH_HASH_VALUE(pSysTable->tableName);
    tab_hash = &Schema_Cache->HashTable_ReguTabSch[hash_val];

    tab_node = NULL;
    tab_node = GET_REGU_TABSCH_NODE(pSysTable->tableName, tab_hash,
            pSysTable->tableId, MDB_FALSE, &ret);
    if (tab_node || ret < 0)
    {
        if (ret != DB_E_NOTCACHED)
        {
            DBMem_Free(new_field_oid_list);
            new_field_oid_list = NULL;
            DBMem_Free(new_node);
            new_node = NULL;

            if (tab_node)
            {
                ret = DB_E_ALREADYCACHED;
            }

            return ret;
        }
    }

    /* connect it into the hash chain */
    new_node->next = tab_hash->head;
    tab_hash->head = DBMem_P2V(new_node);
    tab_hash->count += 1;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! _Cache_AddIndexSchema
/*! \breif  copy index_schema to the cached-area
 ************************************
 * \param   tableName     :
 * \param   index_oid     :
 * \param   num_fields    :
 * \param   field_oid_list:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_AddIndexSchema(char *tableName, OID index_oid,
        int num_fields, OID * field_oid_list)
{
    int hash_val;
    REGU_TABSCH_HASH_T *tab_hash;
    REGU_TABSCH_NODE_T *tab_node;
    REGU_IDXSCH_HASH_T *idx_hash;
    REGU_IDXSCH_NODE_T *idx_node, *new_node;
    SYSINDEX_T sysIndex;
    SYSINDEX_T *pSysIndex;
    int ret;
    OID *act_index_oid_list;
    OID *new_field_oid_list;

    pSysIndex = (SYSINDEX_T *) OID_Point(index_oid);
    if (pSysIndex == NULL)
    {
        return DB_E_SYSINDEXCONT;
    }

    /* To avoid segment buffer replacement */
    sc_memcpy((char *) &sysIndex, (char *) pSysIndex, sizeof(SYSINDEX_T));
    pSysIndex = &sysIndex;

    /* find the table schema node */

    hash_val = GET_REGU_TABSCH_HASH_VALUE(tableName);
    tab_hash = &Schema_Cache->HashTable_ReguTabSch[hash_val];

    tab_node = GET_REGU_TABSCH_NODE(tableName, tab_hash, NULL_OID, MDB_FALSE,
            &ret);
    if (ret < 0)
    {
        return ret;
    }

    /* expand index_oid_list of table schema node if needed */
    if (tab_node->num_regu_indexes == tab_node->max_regu_indexes)
    {
        OID *old_index_oid_list;
        OID *new_index_oid_list;
        int new_list_size = tab_node->max_regu_indexes + INDEX_LIST_INCREMENT;

        old_index_oid_list = DBMem_V2P(tab_node->index_oid_list);

        new_index_oid_list = DBMem_Alloc(OID_SIZE * new_list_size);
        if (new_index_oid_list == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFDBMEMORY, 0);
            return DB_E_OUTOFDBMEMORY;
        }

        if (old_index_oid_list != NULL)
        {
            sc_memcpy(new_index_oid_list, old_index_oid_list,
                    (OID_SIZE * tab_node->num_regu_indexes));
            DBMem_Free(old_index_oid_list);
            old_index_oid_list = NULL;
        }

        tab_node->index_oid_list = DBMem_P2V(new_index_oid_list);
        tab_node->max_regu_indexes = new_list_size;
    }

    /* build new index schema node */

    new_node = DBMem_Alloc(sizeof(REGU_IDXSCH_NODE_T));
    if (new_node == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFDBMEMORY, 0);
        return DB_E_OUTOFDBMEMORY;
    }
    sc_memset(new_node, 0, sizeof(REGU_IDXSCH_NODE_T));

    new_field_oid_list = DBMem_Alloc(OID_SIZE * num_fields);
    if (new_field_oid_list == NULL)
    {
        DBMem_Free(new_node);
        new_node = NULL;
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFDBMEMORY, 0);
        return DB_E_OUTOFDBMEMORY;
    }
    sc_memcpy(new_field_oid_list, field_oid_list, (OID_SIZE * num_fields));

    new_node->table_oid = tab_node->table_oid;
    new_node->index_oid = index_oid;
    new_node->field_oid_list = DBMem_P2V(new_field_oid_list);

    /* add the new index schema node if not cached. */

    hash_val = GET_REGU_IDXSCH_HASH_VALUE(pSysIndex->indexName);
    idx_hash = &Schema_Cache->HashTable_ReguIdxSch[hash_val];

    idx_node = NULL;
    idx_node = GET_REGU_IDXSCH_NODE(pSysIndex->indexName, idx_hash, NULL_OID,
            MDB_FALSE, &ret);
    if (ret < 0 || idx_node)
    {
        if (ret != DB_E_NOTCACHED)
        {
            DBMem_Free(new_field_oid_list);
            new_field_oid_list = NULL;
            DBMem_Free(new_node);
            new_node = NULL;

            if (idx_node)
            {
                ret = DB_E_ALREADYCACHED;
            }

            return ret;
        }
    }

    new_node->next = idx_hash->head;
    idx_hash->head = DBMem_P2V(new_node);
    idx_hash->count += 1;

    /* register the index schema node into the table schema node */
    act_index_oid_list = DBMem_V2P(tab_node->index_oid_list);
    act_index_oid_list[tab_node->num_regu_indexes] = index_oid;
    tab_node->num_regu_indexes += 1;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! _Cache_DeleteTableSchema
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   tableName:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_DeleteTableSchema(char *tableName, OID table_oid)
{
    int hash_val, i;
    int ret;
    REGU_TABSCH_HASH_T *tab_hash;
    REGU_TABSCH_NODE_T *tab_node;
    REGU_IDXSCH_HASH_T *idx_hash;
    REGU_IDXSCH_NODE_T *idx_node;
    SYSINDEX_T *index_info;
    OID *act_index_oid_list;

    /* find table scheme node and disconnect it from hash chain */

    hash_val = GET_REGU_TABSCH_HASH_VALUE(tableName);
    tab_hash = &Schema_Cache->HashTable_ReguTabSch[hash_val];

    tab_node = GET_REGU_TABSCH_NODE(tableName, tab_hash, table_oid, MDB_TRUE,
            &ret);
    if (ret < 0)
    {
        return ret;
    }

    /* for each index, remove index schema node */
    act_index_oid_list = DBMem_V2P(tab_node->index_oid_list);
    for (i = 0; i < tab_node->num_regu_indexes; i++)
    {
        /* find index scheme node and disconnect it from hash chain */
        index_info = (SYSINDEX_T *) OID_Point(act_index_oid_list[i]);
        if (index_info == NULL)
        {
            return DB_E_OIDPOINTER;
        }
        hash_val = GET_REGU_IDXSCH_HASH_VALUE(index_info->indexName);
        idx_hash = &Schema_Cache->HashTable_ReguIdxSch[hash_val];

        idx_node = GET_REGU_IDXSCH_NODE(index_info->indexName, idx_hash,
                act_index_oid_list[i], MDB_TRUE, &ret);
        if (ret < 0)
        {
            if (ret == DB_E_NOTCACHED)
            {
                MDB_SYSLOG(("Index Schema Node: Not Found in Hash Chain.\n"));
                continue;
            }

            return ret;
        }

        /* free the index schema node */
        DBMem_Free(DBMem_V2P(idx_node->field_oid_list));
        idx_node->field_oid_list = NULL;
        DBMem_Free(idx_node);
        idx_node = NULL;
    }

    /* free the table schema node */
    if (tab_node->index_oid_list)
    {
        DBMem_Free(DBMem_V2P(tab_node->index_oid_list));
        tab_node->index_oid_list = NULL;
    }
    DBMem_Free(DBMem_V2P(tab_node->field_oid_list));
    tab_node->field_oid_list = NULL;
    DBMem_Free(tab_node);
    tab_node = NULL;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! _Cache_DeleteIndexSchema
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   indexName:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_DeleteIndexSchema(char *indexName, OID index_oid)
{
    int hash_val, i, j;
    REGU_TABSCH_HASH_T *tab_hash;
    REGU_TABSCH_NODE_T *tab_node;
    REGU_IDXSCH_HASH_T *idx_hash;
    REGU_IDXSCH_NODE_T *idx_node;
    int ret;
    SYSTABLE_T *table_info;
    OID *act_index_oid_list;

    /* find the index schema node and disconnect it from hash chain */

    hash_val = GET_REGU_IDXSCH_HASH_VALUE(indexName);
    idx_hash = &Schema_Cache->HashTable_ReguIdxSch[hash_val];

    idx_node = GET_REGU_IDXSCH_NODE(indexName, idx_hash, index_oid, MDB_TRUE,
            &ret);
    if (ret < 0)
    {
        return ret;
    }

    /* remove the index info from table schema node */

    table_info = (SYSTABLE_T *) OID_Point(idx_node->table_oid);
    if (table_info == NULL)
    {
        return DB_E_OIDPOINTER;
    }
    hash_val = GET_REGU_TABSCH_HASH_VALUE(table_info->tableName);
    tab_hash = &Schema_Cache->HashTable_ReguTabSch[hash_val];

    tab_node = GET_REGU_TABSCH_NODE(table_info->tableName, tab_hash,
            idx_node->table_oid, MDB_FALSE, &ret);
    if (ret < 0)
    {
        if (ret == DB_E_NOTCACHED)
        {
            MDB_SYSLOG(("Table Schema Node: Not Found in Hash Chain.\n"));
        }
        else
        {
            return ret;
        }
    }
    else
    {
        act_index_oid_list = DBMem_V2P(tab_node->index_oid_list);
        for (i = 0; i < tab_node->num_regu_indexes; i++)
        {
            if (act_index_oid_list[i] == idx_node->index_oid)
                break;
        }
        if (i == tab_node->num_regu_indexes)
        {
            MDB_SYSLOG(
                    ("Index Schema Node: Not Found in Table Schema Node.\n"));
        }
        else
        {
            for (j = i + 1; j < tab_node->num_regu_indexes; j++)
                act_index_oid_list[j - 1] = act_index_oid_list[j];
            tab_node->num_regu_indexes -= 1;
        }
    }

    /* free the index schema node */
    DBMem_Free(DBMem_V2P(idx_node->field_oid_list));
    idx_node->field_oid_list = NULL;
    DBMem_Free(idx_node);
    idx_node = NULL;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! _Cache_RenameTableName
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   oldName:
 * \param   newName:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_RenameTableName(char *oldName, char *newName)
{
    int old_hash_val;
    int new_hash_val;

    /* This function is called after systables record is updated. */
    /* So that, systables record is currently having new table name. */

    old_hash_val = GET_REGU_TABSCH_HASH_VALUE(oldName);
    new_hash_val = GET_REGU_TABSCH_HASH_VALUE(newName);

    if (old_hash_val != new_hash_val)
    {
        REGU_TABSCH_HASH_T *tab_hash;
        REGU_TABSCH_NODE_T *tab_node;
        int ret;

        /* delete table schema node from old hash chain */

        tab_hash = &Schema_Cache->HashTable_ReguTabSch[old_hash_val];

        tab_node = GET_REGU_TABSCH_NODE(newName, tab_hash, NULL_OID, MDB_TRUE,
                &ret);
        if (ret < 0)
        {
            return ret;
        }

        /* insert table schema node into new hash chain */

        tab_hash = &Schema_Cache->HashTable_ReguTabSch[new_hash_val];

        tab_node->next = tab_hash->head;
        tab_hash->head = DBMem_P2V(tab_node);
        tab_hash->count += 1;
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! _Cache_RenameIndexName
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   oldName:
 * \param   newName:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_RenameIndexName(char *oldName, char *newName)
{
    int old_hash_val;
    int new_hash_val;

    /* This function is called after sysindexes record is updated. */
    /* So that, sysindexes record is currently having new index name. */

    old_hash_val = GET_REGU_IDXSCH_HASH_VALUE(oldName);
    new_hash_val = GET_REGU_IDXSCH_HASH_VALUE(newName);

    if (old_hash_val != new_hash_val)
    {
        REGU_IDXSCH_HASH_T *idx_hash;
        REGU_IDXSCH_NODE_T *idx_node;
        int ret;

        /* delete the index schema node from old hash chain */

        idx_hash = &Schema_Cache->HashTable_ReguIdxSch[old_hash_val];

        idx_node = GET_REGU_IDXSCH_NODE(newName, idx_hash, NULL_OID, MDB_TRUE,
                &ret);
        if (ret < 0)
        {
            return ret;
        }

        /* insert the index schema node into new hash chain */

        idx_hash = &Schema_Cache->HashTable_ReguIdxSch[new_hash_val];

        idx_node->next = idx_hash->head;
        idx_hash->head = DBMem_P2V(idx_node);
        idx_hash->count += 1;
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! _Cache_GetTableInfo
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   tableName:
 * \param   table_oid:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_GetTableInfo(char *tableName, OID * table_oid, SYSTABLE_T ** systbl)
{
    int hash_val;
    REGU_TABSCH_HASH_T *tab_hash;
    REGU_TABSCH_NODE_T *tab_node;
    int ret;

    /* find the table schema node */

    hash_val = GET_REGU_TABSCH_HASH_VALUE(tableName);
    tab_hash = &Schema_Cache->HashTable_ReguTabSch[hash_val];

    tab_node = GET_REGU_TABSCH_NODE(tableName, tab_hash, NULL_OID, MDB_FALSE,
            &ret);
    if (ret < 0)
    {
        return ret;
    }

    if (systbl)
    {

        *systbl = (SYSTABLE_T *) OID_Point(tab_node->table_oid);
    }

    if (table_oid != NULL)
    {
        *table_oid = tab_node->table_oid;
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! _Cache_GetFieldInfo
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   tableName     :
 * \param   field_oid_list:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_GetFieldInfo(char *tableName, OID ** field_oid_list)
{
    int hash_val;
    REGU_TABSCH_HASH_T *tab_hash;
    REGU_TABSCH_NODE_T *tab_node;
    SYSTABLE_T *table_info = NULL;
    int ret;

    /* find the table schema node */

    hash_val = GET_REGU_TABSCH_HASH_VALUE(tableName);
    tab_hash = &Schema_Cache->HashTable_ReguTabSch[hash_val];

    tab_node = GET_REGU_TABSCH_NODE(tableName, tab_hash, NULL_OID, MDB_FALSE,
            &ret);
    if (ret < 0)
    {
        return ret;
    }

    table_info = (SYSTABLE_T *) OID_Point(tab_node->table_oid);

    if (field_oid_list != NULL)
    {
        *field_oid_list = DBMem_V2P(tab_node->field_oid_list);
    }

    return table_info->numFields;
}

/*****************************************************************************/
//! _Cache_GetTableIndexesInfo
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   tableName     :
 * \param   index_oid_list:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_GetTableIndexesInfo(char *tableName, OID ** index_oid_list)
{
    int hash_val;
    REGU_TABSCH_HASH_T *tab_hash;
    REGU_TABSCH_NODE_T *tab_node;
    int ret;

    /* find table schema node */

    hash_val = GET_REGU_TABSCH_HASH_VALUE(tableName);
    tab_hash = &Schema_Cache->HashTable_ReguTabSch[hash_val];

    tab_node = GET_REGU_TABSCH_NODE(tableName, tab_hash, NULL_OID, MDB_FALSE,
            &ret);
    if (ret < 0)
    {
        return ret;
    }

    if (index_oid_list != NULL)
    {
        *index_oid_list = DBMem_V2P(tab_node->index_oid_list);
    }

    return tab_node->num_regu_indexes;
}

/*****************************************************************************/
//! _Cache_GetIndexInfo
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   indexName     :
 * \param   index_oid     : 
 * \param   field_oid_list:
 * \param   tableName     :
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_GetIndexInfo(char *indexName, OID * index_oid,
        OID ** field_oid_list, char *tableName,
        OID table_cont_oid, OID * index_cont_oid)
{
    int hash_val;
    int ret;
    REGU_IDXSCH_HASH_T *idx_hash;
    REGU_IDXSCH_NODE_T *idx_node;
    SYSINDEX_T *index_info;
    struct TTree *ttree;
    SYSTABLE_T *table_info = NULL;
    int indexId;

    /* find the index schema node */
    hash_val = GET_REGU_IDXSCH_HASH_VALUE(indexName);
    idx_hash = &Schema_Cache->HashTable_ReguIdxSch[hash_val];

    idx_node = GET_REGU_IDXSCH_NODE(indexName, idx_hash, NULL_OID, MDB_FALSE,
            &ret);
    if (ret < 0)
    {
        return ret;
    }

    index_info = (SYSINDEX_T *) OID_Point(idx_node->index_oid);

    if (index_oid != NULL)
    {
        *index_oid = idx_node->index_oid;
    }
    if (field_oid_list != NULL)
    {
        *field_oid_list = DBMem_V2P(idx_node->field_oid_list);
    }

    indexId = index_info->indexId;

    if (tableName != NULL)
    {
        /* dbi_Schema_GetIndexInfo() sets '$' in case of that the value of its
         * parameter tablename is NULL, where table_cont_oid is stored. */
        if (tableName[0] == '$' && idx_node->table_cont_oid)
        {
            table_cont_oid = idx_node->table_cont_oid;
            goto skip_get_tablename;
        }

        table_info = (SYSTABLE_T *) OID_Point(idx_node->table_oid);
        if (table_info == NULL)
        {
            return DB_E_OIDPOINTER;
        }

        sc_strcpy(tableName, table_info->tableName);

        if (idx_node->table_cont_oid == NULL_OID)
        {
            struct Container *Cont = Cont_Search(tableName);

            if (Cont == NULL)
                return DB_E_UNKNOWNTABLE;

            idx_node->table_cont_oid =
                    table_cont_oid = Cont->collect_.cont_id_;
        }
    }

  skip_get_tablename:

    if (table_cont_oid)
    {
        if (idx_node->index_cont_oid == NULL_OID)
        {
            ttree = TTree_Search(table_cont_oid, indexId);
            if (ttree == NULL)
                return DB_E_UNKNOWNINDEX;
            idx_node->index_cont_oid = ttree->collect_.cont_id_;
        }
        if (index_cont_oid)
            *index_cont_oid = idx_node->index_cont_oid;
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! _Cache_AllocReguTempIndexId
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   tableName:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_AllocReguTempIndexId(char *tableName)
{
    int indexId;
    int hash_val;
    REGU_TABSCH_HASH_T *tab_hash;
    REGU_TABSCH_NODE_T *tab_node;
    int ret;

    hash_val = GET_REGU_TABSCH_HASH_VALUE(tableName);
    tab_hash = &Schema_Cache->HashTable_ReguTabSch[hash_val];

    tab_node = GET_REGU_TABSCH_NODE(tableName, tab_hash, NULL_OID, MDB_FALSE,
            &ret);
    if (ret < 0)
    {
        return ret;
    }

    for (indexId = 0; indexId < MAX_INDEX_NO; indexId++)
    {
        if ((tab_node->temp_indexID_map[indexId >> 3] & (1 << (7 -
                                        (indexId & 0x07)))) == 0)
            break;
    }
    if (indexId >= MAX_INDEX_NO)
    {
        return DB_E_UNKNOWNINDEX;
    }

    tab_node->temp_indexID_map[indexId >> 3] |= (1 << (7 - (indexId & 0x07)));
    tab_node->num_temp_indexes++;

    return (MAX_INDEX_NO + indexId);
}

/*****************************************************************************/
//! _Cache_FreeReguTempIndexId
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   tableName:
 * \param   indexId  :
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_FreeReguTempIndexId(char *tableName, int indexId)
{
    int hash_val;
    int BitIdx = indexId - MAX_INDEX_NO;
    REGU_TABSCH_HASH_T *tab_hash;
    REGU_TABSCH_NODE_T *tab_node;
    int ret;

    hash_val = GET_REGU_TABSCH_HASH_VALUE(tableName);
    tab_hash = &Schema_Cache->HashTable_ReguTabSch[hash_val];

    tab_node = GET_REGU_TABSCH_NODE(tableName, tab_hash, NULL_OID, MDB_FALSE,
            &ret);
    if (ret < 0)
    {
        return ret;
    }

    if ((tab_node->temp_indexID_map[BitIdx >> 3] & (1 << (7 -
                                    (BitIdx & 0x07)))) == 0)
    {
        return DB_E_UNKNOWNINDEX;
    }

    /* clear the indexId bit */
    tab_node->temp_indexID_map[BitIdx >> 3] &= ~(1 << (7 - (BitIdx & 0x07)));
    tab_node->num_temp_indexes--;

    return DB_SUCCESS;
}

/*************************************************
    Temp Table & Index Schema Cache Functions
*************************************************/

/*****************************************************************************/
//! _Cache_AddTempTableSchema
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   table_info     :
 * \param   num_fields     :
 * \param   field_info_list:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_AddTempTableSchema(SYSTABLE_T * table_info,
        int num_fields, SYSFIELD_T * field_info_list)
{
    int transID = *(int *) CS_getspecific(my_trans_id);
    int hash_val;
    TEMP_TABSCH_HASH_T *tab_hash;
    TEMP_TABSCH_NODE_T *tab_node;
    SYSFIELD_T *new_field_info_list;

    if (transID == -1)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_NOTTRANSBEGIN, 0);
        return DB_E_NOTTRANSBEGIN;
    }

    hash_val = transID % TEMP_TABSCH_HASH_SIZE;
    tab_hash = &Schema_Cache->HashTable_TempTabSch[hash_val];

    /* check if already cached */
    tab_node = DBMem_V2P(tab_hash->head);
    while (tab_node != NULL)
    {
        if (tab_node->table_info.tableId == table_info->tableId)
            break;
        tab_node = DBMem_V2P(tab_node->next);
    }
    if (tab_node != NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_ALREADYCACHED, 0);
        return DB_E_ALREADYCACHED;
    }

    /* allocate and initialize temp table schema node */
    tab_node = DBMem_Alloc(sizeof(TEMP_TABSCH_NODE_T));
    if (tab_node == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFDBMEMORY, 0);
        return DB_E_OUTOFDBMEMORY;
    }
    sc_memset(tab_node, 0, sizeof(TEMP_TABSCH_NODE_T));

    /* allocate and initialize temp table schema node */
    new_field_info_list =
            DBMem_Alloc(sizeof(SYSFIELD_T) * table_info->numFields);
    if (new_field_info_list == NULL)
    {
        DBMem_Free(tab_node);
        tab_node = NULL;
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFDBMEMORY, 0);
        return DB_E_OUTOFDBMEMORY;
    }
    sc_memcpy(new_field_info_list, field_info_list,
            (sizeof(SYSFIELD_T) * table_info->numFields));

    sc_memcpy(&(tab_node->table_info), table_info, sizeof(SYSTABLE_T));
    tab_node->field_info_list = DBMem_P2V(new_field_info_list);

    /* connect it into the hash chain */
    tab_node->next = tab_hash->head;
    tab_hash->head = DBMem_P2V(tab_node);
    tab_hash->count += 1;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! _Cache_AddTempIndexSchema
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   tableName      :
 * \param   index_info     :
 * \param   num_fields     :
 * \param   field_info_list:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_AddTempIndexSchema(char *tableName, SYSINDEX_T * index_info,
        int num_fields, SYSINDEXFIELD_T * field_info_list)
{
    int transID = *(int *) CS_getspecific(my_trans_id);
    int hash_val;
    TEMP_IDXSCH_HASH_T *idx_hash;
    TEMP_IDXSCH_NODE_T *idx_node;
    SYSINDEXFIELD_T *new_field_info_list;

    if (transID == -1)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_NOTTRANSBEGIN, 0);
        return DB_E_NOTTRANSBEGIN;
    }

    /* find hash entry */
    hash_val = transID % TEMP_IDXSCH_HASH_SIZE;
    idx_hash = &Schema_Cache->HashTable_TempIdxSch[hash_val];

    /* check if already cached */
    idx_node = DBMem_V2P(idx_hash->head);
    while (idx_node != NULL)
    {
        if (!mdb_strcmp(idx_node->index_info.indexName, index_info->indexName))
            break;
        idx_node = DBMem_V2P(idx_node->next);
    }
    if (idx_node != NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_ALREADYCACHED, 0);
        return DB_E_ALREADYCACHED;
    }

    /* allocate and initialize temp index schema node */
    idx_node = DBMem_Alloc(sizeof(TEMP_IDXSCH_NODE_T));
    if (idx_node == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFDBMEMORY, 0);
        return DB_E_OUTOFDBMEMORY;
    }
    sc_memset(idx_node, 0, sizeof(TEMP_IDXSCH_NODE_T));

    new_field_info_list = DBMem_Alloc(sizeof(SYSINDEXFIELD_T)
            * index_info->numFields);
    if (new_field_info_list == NULL)
    {
        DBMem_Free(idx_node);
        idx_node = NULL;
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFDBMEMORY, 0);
        return DB_E_OUTOFDBMEMORY;
    }
    sc_memcpy(new_field_info_list, field_info_list,
            (sizeof(SYSINDEXFIELD_T) * index_info->numFields));

    sc_strcpy(idx_node->table_name, tableName);
    sc_memcpy(&(idx_node->index_info), index_info, sizeof(SYSINDEX_T));
    idx_node->field_info_list = DBMem_P2V(new_field_info_list);

    /* connect it into the hash chain */
    idx_node->next = idx_hash->head;
    idx_hash->head = DBMem_P2V(idx_node);
    idx_hash->count += 1;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! _Cache_DeleteTempTableSchema
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   tableName:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
int
_Cache_DeleteTempTableSchema(char *tableName)
{
    int transID = *(int *) CS_getspecific(my_trans_id);
    int hash_val;
    TEMP_TABSCH_HASH_T *tab_hash;
    TEMP_TABSCH_NODE_T *tab_node, *prev_tab_node;
    TEMP_IDXSCH_HASH_T *idx_hash;
    TEMP_IDXSCH_NODE_T *idx_node, *prev_idx_node;

    if (transID == -1)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_NOTTRANSBEGIN, 0);
        return DB_E_NOTTRANSBEGIN;
    }

    hash_val = transID % TEMP_TABSCH_HASH_SIZE;
    tab_hash = &Schema_Cache->HashTable_TempTabSch[hash_val];

    /* find table schema node */
    prev_tab_node = NULL;
    tab_node = DBMem_V2P(tab_hash->head);
    while (tab_node != NULL)
    {
        if (!mdb_strcmp(tab_node->table_info.tableName, tableName))
            break;
        prev_tab_node = tab_node;
        tab_node = DBMem_V2P(tab_node->next);
    }
    if (tab_node == NULL)
    {
        return DB_E_NOTCACHED;
    }

    /* disconnect it from hash chain */
    if (prev_tab_node == NULL)
        tab_hash->head = tab_node->next;
    else
        prev_tab_node->next = tab_node->next;
    tab_hash->count -= 1;

    hash_val = transID % TEMP_IDXSCH_HASH_SIZE;
    idx_hash = &Schema_Cache->HashTable_TempIdxSch[hash_val];

    /* find index schema node */
    prev_idx_node = NULL;
    idx_node = DBMem_V2P(idx_hash->head);
    while (idx_node != NULL)
    {
        if (idx_node->index_info.tableId == tab_node->table_info.tableId)
        {
            if (prev_idx_node == NULL)
                idx_hash->head = idx_node->next;
            else
                prev_idx_node->next = idx_node->next;
            idx_hash->count -= 1;

            /* free index schema node */
            if (idx_node->field_info_list)
            {
                DBMem_Free(DBMem_V2P(idx_node->field_info_list));
                idx_node->field_info_list = NULL;
            }
            DBMem_Free(idx_node);
            idx_node = NULL;

            if (prev_idx_node == NULL)
                idx_node = DBMem_V2P(idx_hash->head);
            else
                idx_node = DBMem_V2P(prev_idx_node->next);
        }
        else    /* not found */
        {
            prev_idx_node = idx_node;
            idx_node = DBMem_V2P(idx_node->next);
        }
    }

    /* free table schema node */
    if (tab_node->field_info_list)
    {
        DBMem_Free(DBMem_V2P(tab_node->field_info_list));
        tab_node->field_info_list = NULL;
    }
    DBMem_Free(tab_node);
    tab_node = NULL;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! _Cache_DeleteTempIndexSchema
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   indexName(in)   : temp index's name
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
int
_Cache_DeleteTempIndexSchema(char *indexName)
{
    int transID = *(int *) CS_getspecific(my_trans_id);
    int hash_val;
    TEMP_IDXSCH_HASH_T *idx_hash;
    TEMP_IDXSCH_NODE_T *idx_node, *prev_idx_node;

    if (transID == -1)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_NOTTRANSBEGIN, 0);
        return DB_E_NOTTRANSBEGIN;
    }

    hash_val = transID % TEMP_IDXSCH_HASH_SIZE;
    idx_hash = &Schema_Cache->HashTable_TempIdxSch[hash_val];

    /* find index schema node */
    prev_idx_node = NULL;
    idx_node = DBMem_V2P(idx_hash->head);
    while (idx_node != NULL)
    {
        if (!mdb_strcmp(idx_node->index_info.indexName, indexName))
            break;
        prev_idx_node = idx_node;
        idx_node = DBMem_V2P(idx_node->next);
    }
    if (idx_node == NULL)
    {
        return DB_E_NOTCACHED;
    }

    /* disconnect it from hash chain */
    if (prev_idx_node == NULL)
        idx_hash->head = idx_node->next;
    else
        prev_idx_node->next = idx_node->next;
    idx_hash->count -= 1;

    if (idx_node->index_info.indexId >= MAX_INDEX_NO)
    {
        /* free indexID of ReguTemp index */
        _Cache_FreeReguTempIndexId(idx_node->table_name,
                idx_node->index_info.indexId);
    }

    /* free the index schema node */
    if (idx_node->field_info_list)
    {
        DBMem_Free(DBMem_V2P(idx_node->field_info_list));
        idx_node->field_info_list = NULL;
    }
    DBMem_Free(idx_node);
    idx_node = NULL;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! _Cache_GetTempTableInfo
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   tableName :
 * \param   ppSysTable:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_GetTempTableInfo(char *tableName, SYSTABLE_T ** ppSysTable)
{
    int transID = *(int *) CS_getspecific(my_trans_id);
    int hash_val;
    TEMP_TABSCH_HASH_T *tab_hash;
    TEMP_TABSCH_NODE_T *tab_node;

    if (transID == -1)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_NOTTRANSBEGIN, 0);
        return DB_E_NOTTRANSBEGIN;
    }

    hash_val = transID % TEMP_TABSCH_HASH_SIZE;
    tab_hash = &Schema_Cache->HashTable_TempTabSch[hash_val];
    tab_node = DBMem_V2P(tab_hash->head);
    while (tab_node != NULL)
    {
        if (!mdb_strcmp(tab_node->table_info.tableName, tableName))
            break;
        tab_node = DBMem_V2P(tab_node->next);
    }

    if (tab_node == NULL)
    {       /* not found */
        return DB_E_NOTCACHED;
    }

    if (ppSysTable != NULL)
    {
        *ppSysTable = &(tab_node->table_info);
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! _Cache_GetTempFieldInfo
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   tableName :
 * \param   ppSysField:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_GetTempFieldInfo(char *tableName, SYSFIELD_T ** ppSysField)
{
    int transID = *(int *) CS_getspecific(my_trans_id);
    int hash_val;
    TEMP_TABSCH_HASH_T *tab_hash;
    TEMP_TABSCH_NODE_T *tab_node;

    if (transID == -1)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_NOTTRANSBEGIN, 0);
        return DB_E_NOTTRANSBEGIN;
    }

    hash_val = transID % TEMP_TABSCH_HASH_SIZE;
    tab_hash = &Schema_Cache->HashTable_TempTabSch[hash_val];
    tab_node = DBMem_V2P(tab_hash->head);
    while (tab_node != NULL)
    {
        if (!mdb_strcmp(tab_node->table_info.tableName, tableName))
            break;
        tab_node = DBMem_V2P(tab_node->next);
    }

    if (tab_node == NULL)
    {       /* not found */
        return DB_E_NOTCACHED;
    }

    if (ppSysField != NULL)
    {
        *ppSysField = DBMem_V2P(tab_node->field_info_list);
    }

    return tab_node->table_info.numFields;
}

/*****************************************************************************/
//! _Cache_GetTempTableIndexesInfo
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   tableName :
 * \param   ppSysIndex:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_GetTempTableIndexesInfo(char *tableName, SYSINDEX_T ** ppSysIndex)
{
    int transID = *(int *) CS_getspecific(my_trans_id);
    int hash_val;
    int num_indexes = 0;
    TEMP_IDXSCH_HASH_T *idx_hash;
    TEMP_IDXSCH_NODE_T *idx_node;

    if (transID == -1)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_NOTTRANSBEGIN, 0);
        return DB_E_NOTTRANSBEGIN;
    }

    hash_val = transID % TEMP_IDXSCH_HASH_SIZE;
    idx_hash = &Schema_Cache->HashTable_TempIdxSch[hash_val];
    idx_node = DBMem_V2P(idx_hash->head);
    while (idx_node != NULL)
    {
        if (!mdb_strcmp(idx_node->table_name, tableName))
        {
            if (ppSysIndex != NULL)
            {
                ppSysIndex[num_indexes] = &(idx_node->index_info);
            }
            num_indexes++;
        }
        idx_node = DBMem_V2P(idx_node->next);
    }

    return num_indexes;
}

/*****************************************************************************/
//! _Cache_GetTempIndexInfo
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   indexName      :
 * \param   ppSysIndex     :
 * \param   ppSysIndexField:
 * \param   tableName      :
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_GetTempIndexInfo(char *indexName, SYSINDEX_T ** ppSysIndex,
        SYSINDEXFIELD_T ** ppSysIndexField, char *tableName)
{
    int transID = *(int *) CS_getspecific(my_trans_id);
    int hash_val;
    TEMP_IDXSCH_HASH_T *idx_hash;
    TEMP_IDXSCH_NODE_T *idx_node;

    if (transID == -1)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_NOTTRANSBEGIN, 0);
        return DB_E_NOTTRANSBEGIN;
    }

    hash_val = transID % TEMP_IDXSCH_HASH_SIZE;
    idx_hash = &Schema_Cache->HashTable_TempIdxSch[hash_val];
    idx_node = DBMem_V2P(idx_hash->head);
    while (idx_node != NULL)
    {
        if (!mdb_strcmp(idx_node->index_info.indexName, indexName))
            break;
        idx_node = DBMem_V2P(idx_node->next);
    }

    if (idx_node == NULL)
    {       /* not found */
        return DB_E_NOTCACHED;
    }

    if (ppSysIndex != NULL)
    {
        *ppSysIndex = &(idx_node->index_info);
    }
    if (ppSysIndexField != NULL)
    {
        *ppSysIndexField = DBMem_V2P(idx_node->field_info_list);
    }
    if (tableName != NULL)
    {
        sc_strcpy(tableName, idx_node->table_name);
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! _Cache_FindFreeTempIndexId
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   tableId:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_FindFreeTempIndexId(int tableId)
{
    int transID = *(int *) CS_getspecific(my_trans_id);
    int hash_val;
    int indexId;
    DB_UINT8 indexMap[((MAX_INDEX_NO - 1) >> 3) + 1];
    TEMP_IDXSCH_HASH_T *idx_hash;
    TEMP_IDXSCH_NODE_T *idx_node;

    if (transID == -1)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_NOTTRANSBEGIN, 0);
        return DB_E_NOTTRANSBEGIN;
    }

    sc_memset(indexMap, 0, sizeof(indexMap));

    hash_val = transID % TEMP_IDXSCH_HASH_SIZE;
    idx_hash = &Schema_Cache->HashTable_TempIdxSch[hash_val];
    idx_node = DBMem_V2P(idx_hash->head);
    while (idx_node != NULL)
    {
        if (idx_node->index_info.tableId == tableId)
        {
            indexMap[idx_node->index_info.indexId >> 3] |=
                    (1 << (7 - (idx_node->index_info.indexId & 0x07)));
        }
        idx_node = DBMem_V2P(idx_node->next);
    }

    /* find free indexID from indexID map */
    for (indexId = 0; indexId < MAX_INDEX_NO; indexId++)
    {
        if ((indexMap[indexId >> 3] & (1 << (7 - (indexId & 0x07)))) == 0)
            break;
    }

    return indexId;
}

/*****************************************************************************/
//! _Cache_Trans_TempObjectExist
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   transId:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
#ifdef MDB_DEBUG
static int
_Cache_Trans_TempObjectExist(int transId)
{
    int hash_val;
    TEMP_TABSCH_HASH_T *tab_hash;
    TEMP_IDXSCH_HASH_T *idx_hash;

    hash_val = transId % TEMP_TABSCH_HASH_SIZE;
    tab_hash = &Schema_Cache->HashTable_TempTabSch[hash_val];
    hash_val = transId % TEMP_IDXSCH_HASH_SIZE;
    idx_hash = &Schema_Cache->HashTable_TempIdxSch[hash_val];

    if (tab_hash->head != NULL || idx_hash->head != NULL)
        return TRUE;
    else
        return FALSE;
}
#endif

/*****************************************************************************/
//! _Cache_Trans_FirstTempTableName
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   transId  :
 * \param   tableName:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_Trans_FirstTempTableName(int transId, char *tableName)
{
    int hash_val;
    TEMP_TABSCH_HASH_T *tab_hash;
    TEMP_TABSCH_NODE_T *tab_node;

    hash_val = transId % TEMP_TABSCH_HASH_SIZE;
    tab_hash = &Schema_Cache->HashTable_TempTabSch[hash_val];
    if (tab_hash->head == NULL)
        return DB_FAIL;

    tab_node = DBMem_V2P(tab_hash->head);
    sc_strcpy(tableName, tab_node->table_info.tableName);
    return DB_SUCCESS;
}

/*****************************************************************************/
//! _Cache_Trans_FirstTempIndexName
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   transId  :
 * \param   indexName:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Cache_Trans_FirstTempIndexName(int transId, char *indexName)
{
    int hash_val;
    TEMP_IDXSCH_HASH_T *idx_hash;
    TEMP_IDXSCH_NODE_T *idx_node;

    hash_val = transId % TEMP_IDXSCH_HASH_SIZE;
    idx_hash = &Schema_Cache->HashTable_TempIdxSch[hash_val];
    if (idx_hash->head == NULL)
        return DB_FAIL;

    idx_node = DBMem_V2P(idx_hash->head);
    sc_strcpy(indexName, idx_node->index_info.indexName);
    return DB_SUCCESS;
}

/* table 변경(insert, delete, update) 시 modified_times의 count가 증가 */
static int
_Cache_SetTable_modifiedtimes(char *tableName)
{
    int transID = *(int *) CS_getspecific(my_trans_id);
    int hash_val;
    REGU_TABSCH_HASH_T *tab_hash;
    REGU_TABSCH_NODE_T *tab_node;
    int ret;

    if (transID == -1)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_NOTTRANSBEGIN, 0);
        return DB_E_NOTTRANSBEGIN;
    }

    /* find the table schema node */
    hash_val = GET_REGU_TABSCH_HASH_VALUE(tableName);
    tab_hash = &Schema_Cache->HashTable_ReguTabSch[hash_val];

    tab_node = GET_REGU_TABSCH_NODE(tableName, tab_hash, NULL_OID, MDB_FALSE,
            &ret);
    if (ret < 0)
    {
        if (ret == DB_E_NOTCACHED)
        {
            ret = DB_SUCCESS;
        }

        return ret;
    }

    tab_node->modified_times += 1;

    return DB_SUCCESS;
}

/* 해당 table의  modified_times return */
static int
_Cache_GetTable_modifiedtimes(char *tableName)
{
    int transID = *(int *) CS_getspecific(my_trans_id);
    int hash_val;
    REGU_TABSCH_HASH_T *tab_hash;
    REGU_TABSCH_NODE_T *tab_node;
    int ret;
    int modified_times = 0;

    if (transID == -1)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_NOTTRANSBEGIN, 0);
        return DB_E_NOTTRANSBEGIN;
    }

    /* find the table schema node */
    hash_val = GET_REGU_TABSCH_HASH_VALUE(tableName);
    tab_hash = &Schema_Cache->HashTable_ReguTabSch[hash_val];

    tab_node = GET_REGU_TABSCH_NODE(tableName, tab_hash, NULL_OID, MDB_FALSE,
            &ret);
    if (ret < 0)
    {
        return ret;
    }

    modified_times = tab_node->modified_times;

    return modified_times;
}

/*****************************************************************************/
//! _Schema_Cache_Load
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   void
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
static int
_Schema_Cache_Load(void)
{
#define SCHEMA_CACHE_HASH_SIZE  11
    REGU_TABSCH_HASH_T *Table_Hash;
    REGU_TABSCH_HASH_T *table_hash;
    REGU_IDXSCH_HASH_T *Index_Hash;
    REGU_IDXSCH_HASH_T *index_hash;

    int hash_val, i, j;
    REGU_TABSCH_HASH_T *tab_hash;
    REGU_TABSCH_NODE_T *tab_node;
    REGU_IDXSCH_HASH_T *idx_hash;
    REGU_IDXSCH_NODE_T *idx_node;

    SYSTABLE_T *pSysTable;
    SYSFIELD_T *pSysField;
    SYSINDEX_T *pSysIndex;
    SYSINDEXFIELD_T *pSysIndexField;
    SYSTABLE_T sysTable;
    SYSFIELD_T sysField;
    SYSINDEX_T sysIndex;
    SYSINDEXFIELD_T sysIndexField;
    struct Container *cont;
    OID record_id;
    OID *field_oid_list;
    SYSTABLE_T *table_info;
    SYSINDEX_T *index_info;

    int ret;

    DB_INT32 slot_size;
    OID link_tail;

    Table_Hash = (REGU_TABSCH_HASH_T *)
            PMEM_ALLOC(sizeof(REGU_TABSCH_HASH_T) * SCHEMA_CACHE_HASH_SIZE);
    if (Table_Hash == NULL)
        return DB_E_OUTOFMEMORY;

    Index_Hash = (REGU_IDXSCH_HASH_T *)
            PMEM_ALLOC(sizeof(REGU_IDXSCH_HASH_T) * SCHEMA_CACHE_HASH_SIZE);
    if (Index_Hash == NULL)
    {
        PMEM_FREE(Table_Hash);
        return DB_E_OUTOFMEMORY;
    }

    /* initialize temporary hash table */
    for (i = 0; i < SCHEMA_CACHE_HASH_SIZE; i++)
    {
        Table_Hash[i].count = 0;
        Table_Hash[i].head = NULL;
        Index_Hash[i].count = 0;
        Index_Hash[i].head = NULL;
    }

    /* build table schema cache while the first systables scanning */
    cont = Cont_Search("systables");
    if (cont == NULL)
    {
        ret = DB_E_OIDPOINTER;
        goto error;
    }

    slot_size = cont->collect_.slot_size_;
    link_tail = cont->collect_.page_link_.tail_;
    record_id = Col_GetFirstDataID(cont, 0);
    while (record_id != NULL_OID)
    {
        if (record_id == INVALID_OID)
        {
            ret = DB_E_OIDPOINTER;
            goto error;
        }

        pSysTable = (SYSTABLE_T *) OID_Point(record_id);
        if (pSysTable == NULL)
        {
            MDB_SYSLOG(("Schema Load: systables is inconsistent.\n"));
            ret = DB_E_SYSTABLECONT;

            goto error;
        }

        /* build table schema node */
        tab_node = DBMem_Alloc(sizeof(REGU_TABSCH_NODE_T));
        if (tab_node == NULL)
        {
            ret = DB_E_OUTOFMEMORY;

            goto error;
        }
        sc_memset(tab_node, 0, sizeof(REGU_TABSCH_NODE_T));

        field_oid_list = DBMem_Alloc(OID_SIZE * pSysTable->numFields);
        if (field_oid_list == NULL)
        {
            DBMem_Free(tab_node);
            tab_node = NULL;
            ret = DB_E_OUTOFMEMORY;

            goto error;
        }
        sc_memset(field_oid_list, 0, (OID_SIZE * pSysTable->numFields));

        tab_node->table_oid = record_id;
        tab_node->field_oid_list = field_oid_list;      /* private address */

        /* insert it into tableid hash chain */
        table_hash = &Table_Hash[pSysTable->tableId % SCHEMA_CACHE_HASH_SIZE];
        tab_node->next = table_hash->head;
        table_hash->head = tab_node;
        table_hash->count += 1;

        Col_GetNextDataID(record_id, slot_size, link_tail, 0);
    }

    /* build table_info while the second systables scanning */
    cont = Cont_Search("systables");
    if (cont == NULL)
    {
        ret = DB_E_OIDPOINTER;
        goto error;
    }

    slot_size = cont->collect_.slot_size_;
    link_tail = cont->collect_.page_link_.tail_;
    record_id = Col_GetFirstDataID(cont, 0);
    while (record_id != NULL_OID)
    {
        if (record_id == INVALID_OID)
        {
            ret = DB_E_OIDPOINTER;
            goto error;
        }

        pSysTable = (SYSTABLE_T *) OID_Point(record_id);
        if (pSysTable == NULL)
        {
            MDB_SYSLOG(("Schema Load: systables is inconsistent.\n"));
            ret = DB_E_SYSTABLECONT;
            goto error;
        }

        /* To avoid segment buffer replacement */
        sc_memcpy((char *) &sysTable, (char *) pSysTable, sizeof(SYSTABLE_T));
        pSysTable = &sysTable;

        Col_GetNextDataID(record_id, slot_size, link_tail, 0);
    }

    cont = Cont_Search("sysfields");
    if (cont == NULL)
    {
        ret = DB_E_OIDPOINTER;
        goto error;
    }

    slot_size = cont->collect_.slot_size_;
    link_tail = cont->collect_.page_link_.tail_;
    record_id = Col_GetFirstDataID(cont, 0);
    while (record_id != NULL_OID)
    {
        if (record_id == INVALID_OID)
        {
            ret = DB_E_OIDPOINTER;
            goto error;
        }

        pSysField = (SYSFIELD_T *) OID_Point(record_id);
        if (pSysField == NULL)
        {
            MDB_SYSLOG(("Schema Load: sysfields is inconsistent.\n"));
            ret = DB_E_SYSFIELDCONT;
            goto error;
        }

        /* To avoid segment buffer replacement */
        sc_memcpy((char *) &sysField, (char *) pSysField, sizeof(SYSFIELD_T));
        pSysField = &sysField;

        /* find table schema node */
        table_hash = &Table_Hash[pSysField->tableId % SCHEMA_CACHE_HASH_SIZE];
        tab_node = table_hash->head;
        while (tab_node != NULL)
        {
            table_info = (SYSTABLE_T *) OID_Point(tab_node->table_oid);
            if (table_info == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto error;
            }

            if (table_info->tableId == pSysField->tableId)
                break;
            tab_node = tab_node->next;
        }
        if (tab_node == NULL)
        {   /* not found */
            MDB_SYSLOG(("Schema Load: Table Schema Node Not Found "
                            "%s, %d, %d\n", pSysField->fieldName,
                            pSysField->tableId, __LINE__));
            goto next;
        }

        tab_node->field_oid_list[pSysField->position] = record_id;

      next:

        Col_GetNextDataID(record_id, slot_size, link_tail, 0);
    }

    /* build index schema cache while the first sysindexes scanning */
    cont = Cont_Search("sysindexes");
    if (cont == NULL)
    {
        ret = DB_E_OIDPOINTER;
        goto error;
    }

    slot_size = cont->collect_.slot_size_;
    link_tail = cont->collect_.page_link_.tail_;
    record_id = Col_GetFirstDataID(cont, 0);
    while (record_id != NULL_OID)
    {
        if (record_id == INVALID_OID)
        {
            ret = DB_E_OIDPOINTER;
            goto error;
        }

        pSysIndex = (SYSINDEX_T *) OID_Point(record_id);
        if (pSysIndex == NULL)
        {
            MDB_SYSLOG(("Schema Load: sysindexes is inconsistent.\n"));
            ret = DB_E_SYSINDEXCONT;
            goto error;
        }

        /* To avoid segment buffer replacement */
        sc_memcpy((char *) &sysIndex, (char *) pSysIndex, sizeof(SYSINDEX_T));
        pSysIndex = &sysIndex;

        /* find table schema node */
        table_hash = &Table_Hash[pSysIndex->tableId % SCHEMA_CACHE_HASH_SIZE];
        tab_node = table_hash->head;
        while (tab_node != NULL)
        {
            table_info = (SYSTABLE_T *) OID_Point(tab_node->table_oid);
            if (table_info == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto error;
            }

            if (table_info->tableId == pSysIndex->tableId)
                break;
            tab_node = tab_node->next;
        }
        if (tab_node == NULL)
        {   /* not found */
            MDB_SYSLOG(("Schema Load: Table Schema Node Not Found "
                            "%s %d, %d\n",
                            pSysIndex->indexName, pSysIndex->tableId,
                            __LINE__));
            ret = DB_E_TABLESCHEMA;
            goto error;
        }

        /* expand index_oid_list if it is short of space */
        if (tab_node->num_regu_indexes == tab_node->max_regu_indexes)
        {
            OID *old_index_oid_list;
            OID *new_index_oid_list;
            int new_list_size =
                    tab_node->max_regu_indexes + INDEX_LIST_INCREMENT;

            old_index_oid_list = tab_node->index_oid_list;

            new_index_oid_list = DBMem_Alloc(OID_SIZE * new_list_size);
            if (new_index_oid_list == NULL)
            {
                ret = DB_E_OUTOFMEMORY;
                goto error;
            }
            if (old_index_oid_list != NULL)
            {
                sc_memcpy(new_index_oid_list, old_index_oid_list,
                        (OID_SIZE * tab_node->num_regu_indexes));
                DBMem_Free(old_index_oid_list);
                old_index_oid_list = NULL;
            }

            tab_node->index_oid_list = new_index_oid_list;
            tab_node->max_regu_indexes = new_list_size;
        }

        /* register index information into table schema node */
        tab_node->index_oid_list[tab_node->num_regu_indexes] = record_id;
        tab_node->num_regu_indexes += 1;

        /* build new index schema node */
        idx_node = DBMem_Alloc(sizeof(REGU_IDXSCH_NODE_T));
        if (idx_node == NULL)
        {
            ret = DB_E_OUTOFMEMORY;
            goto error;
        }
        sc_memset(idx_node, 0, sizeof(REGU_IDXSCH_NODE_T));

        field_oid_list = DBMem_Alloc(OID_SIZE * pSysIndex->numFields);
        if (field_oid_list == NULL)
        {
            DBMem_Free(idx_node);
            idx_node = NULL;
            ret = DB_E_OUTOFMEMORY;
            goto error;
        }

        sc_memset(field_oid_list, 0, (OID_SIZE * pSysIndex->numFields));

        idx_node->table_oid = tab_node->table_oid;
        idx_node->index_oid = record_id;
        idx_node->field_oid_list = field_oid_list;      /* private address */

        /* insert it into tableid hash chain */
        index_hash = &Index_Hash[pSysIndex->tableId % SCHEMA_CACHE_HASH_SIZE];
        idx_node->next = index_hash->head;
        index_hash->head = idx_node;
        index_hash->count += 1;

        Col_GetNextDataID(record_id, slot_size, link_tail, 0);
    }

    /* build index_info while the second sysindexes scanning */
    cont = Cont_Search("sysindexes");
    if (cont == NULL)
    {
        ret = DB_E_OIDPOINTER;
        goto error;
    }

    slot_size = cont->collect_.slot_size_;
    link_tail = cont->collect_.page_link_.tail_;
    record_id = Col_GetFirstDataID(cont, 0);
    while (record_id != NULL_OID)
    {
        if (record_id == INVALID_OID)
        {
            ret = DB_E_OIDPOINTER;
            goto error;
        }

        pSysIndex = (SYSINDEX_T *) OID_Point(record_id);
        if (pSysIndex == NULL)
        {
            MDB_SYSLOG(("Schema Load: sysindexes is inconsistent.\n"));
            ret = DB_E_SYSINDEXCONT;
            goto error;
        }

        /* To avoid segment buffer replacement */
        sc_memcpy((char *) &sysIndex, (char *) pSysIndex, sizeof(SYSINDEX_T));
        pSysIndex = &sysIndex;

        Col_GetNextDataID(record_id, slot_size, link_tail, 0);
    }

    cont = Cont_Search("sysindexfields");
    if (cont == NULL)
    {
        ret = DB_E_OIDPOINTER;
        goto error;
    }

    slot_size = cont->collect_.slot_size_;
    link_tail = cont->collect_.page_link_.tail_;
    record_id = Col_GetFirstDataID(cont, 0);
    while (record_id != NULL_OID)
    {
        if (record_id == INVALID_OID)
        {
            ret = DB_E_OIDPOINTER;
            goto error;
        }

        pSysIndexField = (SYSINDEXFIELD_T *) OID_Point(record_id);
        if (pSysIndexField == NULL)
        {
            MDB_SYSLOG(("Schema Load: sysindexfields is inconsistent.\n"));
            ret = DB_E_SYSINDEXFIELDCONT;
            goto error;
        }

        /* To avoid segment buffer replacement */
        sc_memcpy((char *) &sysIndexField, (char *) pSysIndexField,
                sizeof(SYSINDEXFIELD_T));
        pSysIndexField = &sysIndexField;

        /* find index schema node */
        index_hash =
                &Index_Hash[pSysIndexField->tableId % SCHEMA_CACHE_HASH_SIZE];
        idx_node = index_hash->head;
        while (idx_node != NULL)
        {
            index_info = (SYSINDEX_T *) OID_Point(idx_node->index_oid);
            if (index_info == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto error;
            }

            if (index_info->tableId == pSysIndexField->tableId &&
                    index_info->indexId == pSysIndexField->indexId)
                break;
            idx_node = idx_node->next;
        }
        if (idx_node == NULL)
        {   /* not found */
            MDB_SYSLOG(("Schema Load: Index Schema Node Not Found "
                            "tableid %d, indexid %d\n",
                            pSysIndexField->tableId, pSysIndexField->indexId,
                            __LINE__));
            ret = DB_E_INDEXSCHEMA;
            goto error;
        }

        idx_node->field_oid_list[pSysIndexField->keyPosition] = record_id;

        Col_GetNextDataID(record_id, slot_size, link_tail, 0);
    }

    for (i = 0; i < SCHEMA_CACHE_HASH_SIZE; i++)
    {
        table_hash = &Table_Hash[i];
        while (table_hash->head != NULL)
        {
            /* disconnect table schema node from temp hash table */
            tab_node = table_hash->head;
            table_hash->head = tab_node->next;
            table_hash->count -= 1;

            /* get table info */
            pSysTable = (SYSTABLE_T *) OID_Point(tab_node->table_oid);
            if (pSysTable == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto error;
            }

            /* transform actual addresses into virtual addresses */
            for (j = 0; j < pSysTable->numFields; j++)
            {
                if (tab_node->field_oid_list[j] == NULL_OID)
                {
                    MDB_SYSLOG(("Schema Load: Num Fields MisMatch %s, "
                                    "numfields %d missing %d, %d\n",
                                    pSysTable->tableName, pSysTable->numFields,
                                    j, __LINE__));

                    if (tab_node->index_oid_list)
                    {
                        DBMem_Free(tab_node->index_oid_list);
                        tab_node->index_oid_list = NULL;
                    }
                    DBMem_Free(tab_node->field_oid_list);
                    tab_node->field_oid_list = NULL;
                    DBMem_Free(tab_node);
                    tab_node = NULL;
                    ret = DB_E_FIELDNUM;
                    goto error;
                }
            }
            for (j = 0; j < tab_node->num_regu_indexes; j++)
            {
                if (tab_node->index_oid_list[j] == NULL_OID)
                {
                    MDB_SYSLOG(("Schema Load: Num Regu Indexes MisMatch\n"));
                    if (tab_node->index_oid_list)
                    {
                        DBMem_Free(tab_node->index_oid_list);
                        tab_node->index_oid_list = NULL;
                    }
                    DBMem_Free(tab_node->field_oid_list);
                    tab_node->field_oid_list = NULL;
                    DBMem_Free(tab_node);
                    tab_node = NULL;
                    ret = DB_E_INDEXNUM;
                    goto error;
                }
            }
            tab_node->field_oid_list = DBMem_P2V(tab_node->field_oid_list);
            tab_node->index_oid_list = DBMem_P2V(tab_node->index_oid_list);

            /* insert table schema node into schema hash table */
            hash_val = GET_REGU_TABSCH_HASH_VALUE(pSysTable->tableName);
            tab_hash = &Schema_Cache->HashTable_ReguTabSch[hash_val];
            tab_node->next = tab_hash->head;
            tab_hash->head = DBMem_P2V(tab_node);
            tab_hash->count += 1;

        }
    }

    for (i = 0; i < SCHEMA_CACHE_HASH_SIZE; i++)
    {
        index_hash = &Index_Hash[i];
        while (index_hash->head != NULL)
        {
            /* disconnect index schema node from temp hash table */
            idx_node = index_hash->head;
            index_hash->head = idx_node->next;
            index_hash->count -= 1;

            /* get index info */
            pSysIndex = (SYSINDEX_T *) OID_Point(idx_node->index_oid);
            if (pSysIndex == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto error;
            }

            /* transform actual addresses into virtual addresses */
            for (j = 0; j < pSysIndex->numFields; j++)
            {
                if (idx_node->field_oid_list[j] == NULL_OID)
                {
                    DBMem_Free(idx_node->field_oid_list);
                    idx_node->field_oid_list = NULL;
                    DBMem_Free(idx_node);
                    idx_node = NULL;
                    MDB_SYSLOG(("Schema Load: Num Index Fields MisMatch\n"));
                    ret = DB_E_FIELDNUM;
                    goto error;
                }
            }
            idx_node->field_oid_list = DBMem_P2V(idx_node->field_oid_list);

            /* insert table schema node into schema hash table */
            hash_val = GET_REGU_IDXSCH_HASH_VALUE(pSysIndex->indexName);
            idx_hash = &Schema_Cache->HashTable_ReguIdxSch[hash_val];
            idx_node->next = idx_hash->head;
            idx_hash->head = DBMem_P2V(idx_node);
            idx_hash->count += 1;
        }
    }

    PMEM_FREE(Table_Hash);
    PMEM_FREE(Index_Hash);

    return DB_SUCCESS;

  error:
    /* free temporay table hash table */
    for (i = 0; i < SCHEMA_CACHE_HASH_SIZE; i++)
    {
        table_hash = &Table_Hash[i];
        while (table_hash->head != NULL)
        {
            tab_node = table_hash->head;
            table_hash->head = tab_node->next;
            if (tab_node->index_oid_list)
            {
                DBMem_Free(tab_node->index_oid_list);
                tab_node->index_oid_list = NULL;
            }
            DBMem_Free(tab_node->field_oid_list);
            tab_node->field_oid_list = NULL;
            DBMem_Free(tab_node);
            tab_node = NULL;
        }
    }
    /* free temporay index hash table */
    for (i = 0; i < SCHEMA_CACHE_HASH_SIZE; i++)
    {
        index_hash = &Index_Hash[i];
        while (index_hash->head != NULL)
        {
            idx_node = index_hash->head;
            index_hash->head = idx_node->next;
            DBMem_Free(idx_node->field_oid_list);
            idx_node->field_oid_list = NULL;
            DBMem_Free(idx_node);
            idx_node = NULL;
        }
    }

    PMEM_FREE(Table_Hash);
    PMEM_FREE(Index_Hash);

    return ret;
}

/*****************************************************************************/
//! _Schema_Cache_Init
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   void
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
int
_Schema_Cache_Init(int schema_load_flag)
{
    int i;
    REGU_TABSCH_HASH_T *regu_tab_hash;
    REGU_IDXSCH_HASH_T *regu_idx_hash;
    TEMP_TABSCH_HASH_T *temp_tab_hash;
    TEMP_IDXSCH_HASH_T *temp_idx_hash;
    int ret;

    if (schema_load_flag & SCHEMA_LOAD_REGU_TBL)
        for (i = 0; i < REGU_TABSCH_HASH_SIZE; i++)
        {
            regu_tab_hash = &Schema_Cache->HashTable_ReguTabSch[i];
            regu_tab_hash->head = NULL;
            regu_tab_hash->count = 0;
        }

    if (schema_load_flag & SCHEMA_LOAD_REGU_IDX)
        for (i = 0; i < REGU_IDXSCH_HASH_SIZE; i++)
        {
            regu_idx_hash = &Schema_Cache->HashTable_ReguIdxSch[i];
            regu_idx_hash->head = NULL;
            regu_idx_hash->count = 0;
        }

    if (schema_load_flag & SCHEMA_LOAD_TEMP_TBL)
        for (i = 0; i < TEMP_TABSCH_HASH_SIZE; i++)
        {
            temp_tab_hash = &Schema_Cache->HashTable_TempTabSch[i];
            temp_tab_hash->head = NULL;
            temp_tab_hash->count = 0;
        }

    if (schema_load_flag & SCHEMA_LOAD_TEMP_IDX)
        for (i = 0; i < TEMP_IDXSCH_HASH_SIZE; i++)
        {
            temp_idx_hash = &Schema_Cache->HashTable_TempIdxSch[i];
            temp_idx_hash->head = NULL;
            temp_idx_hash->count = 0;
        }

    if (schema_load_flag)
    {
        ret = _Schema_Cache_Load();
        if (ret < 0)
        {
            _Schema_Cache_Free(schema_load_flag);
            return ret;
        }
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! _Schema_Cache_Free
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   void
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
int
_Schema_Cache_Free(int schema_load_flag)
{
    int i;

    if (Schema_Cache == NULL)
        return DB_SUCCESS;

    /* free regu table schema hash table */
    if (schema_load_flag & SCHEMA_LOAD_REGU_TBL)
    {
        REGU_TABSCH_HASH_T *tab_hash;
        REGU_TABSCH_NODE_T *tab_node;

        for (i = 0; i < REGU_TABSCH_HASH_SIZE; i++)
        {
            tab_hash = &Schema_Cache->HashTable_ReguTabSch[i];
            while (tab_hash->head != NULL)
            {
                tab_node = DBMem_V2P(tab_hash->head);
                tab_hash->head = tab_node->next;
                tab_hash->count -= 1;

                if (tab_node->index_oid_list)
                {
                    DBMem_Free(DBMem_V2P(tab_node->index_oid_list));
                    tab_node->index_oid_list = NULL;
                }
                DBMem_Free(DBMem_V2P(tab_node->field_oid_list));
                tab_node->field_oid_list = NULL;
                DBMem_Free(tab_node);
                tab_node = NULL;
            }
        }
    }

    /* free regu index schema hash table */
    if (schema_load_flag & SCHEMA_LOAD_REGU_IDX)
    {
        REGU_IDXSCH_HASH_T *idx_hash;
        REGU_IDXSCH_NODE_T *idx_node;

        for (i = 0; i < REGU_IDXSCH_HASH_SIZE; i++)
        {
            idx_hash = &Schema_Cache->HashTable_ReguIdxSch[i];
            while (idx_hash->head != NULL)
            {
                idx_node = DBMem_V2P(idx_hash->head);
                idx_hash->head = idx_node->next;
                idx_hash->count -= 1;

                DBMem_Free(DBMem_V2P(idx_node->field_oid_list));
                idx_node->field_oid_list = NULL;
                DBMem_Free(idx_node);
                idx_node = NULL;
            }
        }
    }

    /* free temp table schema hash table */
    if (schema_load_flag & SCHEMA_LOAD_TEMP_TBL)
    {
        TEMP_TABSCH_HASH_T *tab_hash;
        TEMP_TABSCH_NODE_T *tab_node;

        for (i = 0; i < TEMP_TABSCH_HASH_SIZE; i++)
        {
            tab_hash = &Schema_Cache->HashTable_TempTabSch[i];
            while (tab_hash->head != NULL)
            {
                tab_node = DBMem_V2P(tab_hash->head);
                tab_hash->head = tab_node->next;
                tab_hash->count -= 1;

                DBMem_Free(DBMem_V2P(tab_node->field_info_list));
                tab_node->field_info_list = NULL;
                DBMem_Free(tab_node);
                tab_node = NULL;
            }
        }
    }

    /* free temp index schema hash table */
    if (schema_load_flag & SCHEMA_LOAD_TEMP_IDX)
    {
        TEMP_IDXSCH_HASH_T *idx_hash;
        TEMP_IDXSCH_NODE_T *idx_node;

        for (i = 0; i < TEMP_IDXSCH_HASH_SIZE; i++)
        {
            idx_hash = &Schema_Cache->HashTable_TempIdxSch[i];
            while (idx_hash->head != NULL)
            {
                idx_node = DBMem_V2P(idx_hash->head);
                idx_hash->head = idx_node->next;
                idx_hash->count -= 1;

                DBMem_Free(DBMem_V2P(idx_node->field_info_list));
                idx_node->field_info_list = NULL;
                DBMem_Free(idx_node);
                idx_node = NULL;
            }
        }
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! _Schema_Cache_Dump
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   void
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
#ifdef MDB_DEBUG
int
_Schema_Cache_Dump(void)
{
    int i;

    sc_printf("-------------------------------------------\n");
    sc_printf(" Regular Table Schema Cache Dump \n");
    sc_printf("-------------------------------------------\n");
    if (1)
    {
        REGU_TABSCH_HASH_T *tab_hash;
        REGU_TABSCH_NODE_T *tab_node;
        SYSTABLE_T *table_info;

        for (i = 0; i < REGU_TABSCH_HASH_SIZE; i++)
        {
            tab_hash = &Schema_Cache->HashTable_ReguTabSch[i];
            if (tab_hash->count > 0)
            {
                sc_printf("[%3d](cnt=%3d): ", i, tab_hash->count);
                tab_node = DBMem_V2P(tab_hash->head);
                while (tab_node != NULL)
                {
                    table_info = (SYSTABLE_T *) OID_Point(tab_node->table_oid);
                    if (table_info == NULL)
                    {
                        return DB_E_OIDPOINTER;
                    }

                    sc_printf("%s ", table_info->tableName);
                    tab_node = DBMem_V2P(tab_node->next);
                }
                sc_printf("\n");
            }
        }
    }
    sc_printf("-------------------------------------------\n\n");

    sc_printf("-------------------------------------------\n");
    sc_printf(" Regular Index Schema Cache Dump \n");
    sc_printf("-------------------------------------------\n");
    if (1)
    {
        REGU_IDXSCH_HASH_T *idx_hash;
        REGU_IDXSCH_NODE_T *idx_node;
        SYSINDEX_T *index_info;

        for (i = 0; i < REGU_IDXSCH_HASH_SIZE; i++)
        {
            idx_hash = &Schema_Cache->HashTable_ReguIdxSch[i];
            if (idx_hash->count > 0)
            {
                sc_printf("[%3d](cnt=%3d): ", i, idx_hash->count);
                idx_node = DBMem_V2P(idx_hash->head);
                while (idx_node != NULL)
                {
                    index_info = (SYSINDEX_T *) OID_Point(idx_node->index_oid);
                    if (index_info == NULL)
                    {
                        return DB_E_OIDPOINTER;
                    }

                    sc_printf("%s ", index_info->indexName);
                    idx_node = DBMem_V2P(idx_node->next);
                }
                sc_printf("\n");
            }
        }
    }
    sc_printf("-------------------------------------------\n\n");

    sc_printf("-------------------------------------------\n");
    sc_printf(" Temp Table Schema Cache Dump \n");
    sc_printf("-------------------------------------------\n");
    if (1)
    {
        TEMP_TABSCH_HASH_T *tab_hash;
        TEMP_TABSCH_NODE_T *tab_node;

        for (i = 0; i < TEMP_TABSCH_HASH_SIZE; i++)
        {
            tab_hash = &Schema_Cache->HashTable_TempTabSch[i];
            if (tab_hash->count > 0)
            {
                sc_printf("[%3d](cnt=%3d): ", i, tab_hash->count);
                tab_node = DBMem_V2P(tab_hash->head);
                while (tab_node != NULL)
                {
                    sc_printf("%s ", tab_node->table_info.tableName);
                    tab_node = DBMem_V2P(tab_node->next);
                }
                sc_printf("\n");
            }
        }
    }
    sc_printf("-------------------------------------------\n\n");

    sc_printf("-------------------------------------------\n");
    sc_printf(" Temp Index Schema Cache Dump \n");
    sc_printf("-------------------------------------------\n");
    if (1)
    {
        TEMP_IDXSCH_HASH_T *idx_hash;
        TEMP_IDXSCH_NODE_T *idx_node;

        for (i = 0; i < TEMP_IDXSCH_HASH_SIZE; i++)
        {
            idx_hash = &Schema_Cache->HashTable_TempIdxSch[i];
            if (idx_hash->count > 0)
            {
                sc_printf("[%3d](cnt=%3d): ", i, idx_hash->count);
                idx_node = DBMem_V2P(idx_hash->head);
                while (idx_node != NULL)
                {
                    sc_printf("%s ", idx_node->index_info.indexName);
                    idx_node = DBMem_V2P(idx_node->next);
                }
                sc_printf("\n");
            }
        }
    }
    sc_printf("-------------------------------------------\n\n");

    return DB_SUCCESS;
}
#endif

int
_Schema_Cache_InsertTable_Undo(OID table_oid, char *data)
{
    SYSTABLE_T *pSysTable = (SYSTABLE_T *) data;

    return _Cache_DeleteTableSchema(pSysTable->tableName, table_oid);
}

int
_Schema_Cache_InsertIndex_Undo(OID index_oid, char *data)
{
    SYSINDEX_T *pSysIndex = (SYSINDEX_T *) data;

    return _Cache_DeleteIndexSchema(pSysIndex->indexName, index_oid);
}

int
_Schema_Cache_DeleteTable_Undo(OID table_oid)
{
    SYSTABLE_T sysTable;
    SYSTABLE_T *pSysTable;
    SYSFIELD_T *pSysField;
    struct Container *cont;
    OID field_oid;
    OID *field_oid_list;
    int i, ret_val;
    char *pbuf = NULL;
    DB_INT32 slot_size;
    OID link_tail;

    /* To avoid segment buffer replacement */
    pbuf = (char *) OID_Point(table_oid);
    if (pbuf == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    sc_memcpy((char *) &sysTable, pbuf, sizeof(SYSTABLE_T));

    pSysTable = &sysTable;

    field_oid_list = PMEM_ALLOC(OID_SIZE * pSysTable->numFields);
    if (field_oid_list == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFMEMORY, 0);
        return DB_E_OUTOFMEMORY;
    }

    sc_memset(field_oid_list, 0, OID_SIZE * pSysTable->numFields);
    cont = Cont_Search("sysfields");
    slot_size = cont->collect_.slot_size_;
    link_tail = cont->collect_.page_link_.tail_;
    field_oid = Col_GetFirstDataID(cont, 0);
    while (field_oid != NULL_OID)
    {
        if (field_oid == INVALID_OID)
        {
            return DB_E_OIDPOINTER;
        }

        pSysField = (SYSFIELD_T *) OID_Point(field_oid);
        if (pSysField == NULL)
        {
            PMEM_FREENUL(field_oid_list);
            MDB_SYSLOG(("Schema Cache: sysfields is inconsistent.\n"));
            return DB_FAIL;
        }

        if (pSysField->tableId == pSysTable->tableId)
        {
            field_oid_list[pSysField->position] = field_oid;
        }

        Col_GetNextDataID(field_oid, slot_size, link_tail, 0);
    }

    /* check field_oid_list */
    for (i = 0; i < pSysTable->numFields; i++)
    {
        if (field_oid_list[i] == NULL_OID)
        {
            PMEM_FREENUL(field_oid_list);
            MDB_SYSLOG(("Schema Cache: fields mismatch.\n"));
            return DB_E_FIELDINFO;
        }
    }

    ret_val = _Cache_AddTableSchema(table_oid, pSysTable->numFields,
            field_oid_list);

    PMEM_FREENUL(field_oid_list);

    return ret_val;
}

int
_Schema_Cache_DeleteIndex_Undo(OID index_oid)
{
    SYSINDEX_T sysIndex;
    SYSTABLE_T sysTable;
    SYSINDEX_T *pSysIndex;
    SYSTABLE_T *pSysTable;
    SYSINDEXFIELD_T *pSysIndexField;
    struct Container *cont;
    OID table_oid;
    OID field_oid;
    OID *field_oid_list;
    int i, ret_val;
    char *pbuf = NULL;
    DB_INT32 slot_size;
    OID link_tail;

    /* To avoid segment buffer replacement */
    pbuf = (char *) OID_Point(index_oid);
    if (pbuf == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    sc_memcpy((char *) &sysIndex, pbuf, sizeof(SYSINDEX_T));
    pSysIndex = &sysIndex;

    cont = Cont_Search("systables");
    if (cont == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    slot_size = cont->collect_.slot_size_;
    link_tail = cont->collect_.page_link_.tail_;
    table_oid = Col_GetFirstDataID(cont, 0);
    while (table_oid != NULL_OID)
    {
        if (table_oid == INVALID_OID)
        {
            return DB_E_OIDPOINTER;
        }

        pSysTable = (SYSTABLE_T *) OID_Point(table_oid);
        if (pSysTable == NULL)
        {
            MDB_SYSLOG(("Schema Cache: systables is inconsistent.\n"));
            return DB_FAIL;
        }

        if (pSysTable->tableId == pSysIndex->tableId)
            break;

        Col_GetNextDataID(table_oid, slot_size, link_tail, 0);
    }

    if (table_oid == NULL_OID)
    {       /* not found */
        return DB_FAIL;
    }

    /* To avoid segment buffer replacement */
    pbuf = (char *) OID_Point(table_oid);
    if (pbuf == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    sc_memcpy((char *) &sysTable, pbuf, sizeof(SYSTABLE_T));
    pSysTable = &sysTable;

    field_oid_list = PMEM_ALLOC(OID_SIZE * pSysTable->numFields);
    if (field_oid_list == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFMEMORY, 0);
        return DB_E_OUTOFMEMORY;
    }

    sc_memset(field_oid_list, 0, (OID_SIZE * pSysTable->numFields));
    cont = Cont_Search("sysindexfields");
    if (cont == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    slot_size = cont->collect_.slot_size_;
    link_tail = cont->collect_.page_link_.tail_;
    field_oid = Col_GetFirstDataID(cont, 0);
    while (field_oid != NULL_OID)
    {
        if (field_oid == INVALID_OID)
        {
            return DB_E_OIDPOINTER;
        }

        pSysIndexField = (SYSINDEXFIELD_T *) OID_Point(field_oid);
        if (pSysIndexField == NULL)
        {
            PMEM_FREENUL(field_oid_list);
            MDB_SYSLOG(("Schema Cache: sysindexfields is inconsistent.\n"));
            return DB_FAIL;
        }

        if (pSysIndexField->tableId == pSysIndex->tableId &&
                pSysIndexField->indexId == pSysIndex->indexId)
        {
            field_oid_list[pSysIndexField->keyPosition] = field_oid;
        }

        Col_GetNextDataID(field_oid, slot_size, link_tail, 0);
    }

    /* check new_field_oid_list */
    for (i = 0; i < pSysIndex->numFields; i++)
    {
        if (field_oid_list[i] == NULL_OID)
        {
            PMEM_FREENUL(field_oid_list);
            MDB_SYSLOG(("Schema Cache: index fields mismatch. %s %s %d %d\n",
                            pSysTable->tableName, pSysIndex->indexName,
                            pSysIndex->numFields, i));
            return DB_E_FIELDINFO;
        }
    }

    ret_val = _Cache_AddIndexSchema(pSysTable->tableName, index_oid,
            pSysIndex->numFields, field_oid_list);

    PMEM_FREENUL(field_oid_list);

    return ret_val;
}

int
_Schema_Cache_UpdateTable_Undo(OID table_oid, char *data, int data_leng)
{
    SYSTABLE_T *oldSysTable;
    SYSTABLE_T *newSysTable;

    oldSysTable = (SYSTABLE_T *) (data);
    newSysTable = (SYSTABLE_T *) (data + (data_leng / 2));

    if (mdb_strcmp(oldSysTable->tableName, newSysTable->tableName))
    {
        return _Cache_RenameTableName(newSysTable->tableName,
                oldSysTable->tableName);
    }

    return DB_SUCCESS;
}

int
_Schema_Cache_UpdateIndex_Undo(OID index_oid, char *data, int data_leng)
{
    SYSINDEX_T *oldSysIndex;
    SYSINDEX_T *newSysIndex;

    oldSysIndex = (SYSINDEX_T *) (data);
    newSysIndex = (SYSINDEX_T *) (data + (data_leng / 2));

    if (mdb_strcmp(oldSysIndex->indexName, newSysIndex->indexName))
    {
        return _Cache_RenameIndexName(newSysIndex->indexName,
                oldSysIndex->indexName);
    }

    return DB_SUCCESS;
}

/****************************/
/* Schema Related Functions */
/****************************/

/*****************************************************************************/
//! _Schema_InputTable
/*! \breif  SYSTEM TABLE인 Systables과 Sysfields에 Table 정보를 insert한다
 ************************************
 * \param   transId(in)             :
 * \param   tableName(in)           :
 * \param   tableId(in)             :
 * \param   type(in)                :
 * \param   has_variabletype(in)    :
 * \param   recordLen(in)           :
 * \param   heap_recordLen(in)      :
 * \param   numFields(in)           :
 * \param   pFieldDesc(in)          :
 * \param   max_records(in)         :
 * \param   column_name(in)         :
 ************************************
 * \return  return value
 ************************************
 * \note    
 *  - BYTE TYPE이 경우, SCHEMA를 입력시에 길이를 표시하는 공간(4 BYTES)를 
 *      제거 하고 입력
 *  - SYSFIELD TABLE에 COLLATION 정보도 같이 입력
 *****************************************************************************/
int
_Schema_InputTable(int transId, char *tableName, int tableId,
        ContainerType type, DB_BOOL has_variabletype,
        int recordLen, int heap_recordLen, int numFields,
        FIELDDESC_T * pFieldDesc, int max_records, char *column_name)
{
    SYSTABLE_REC_T newTableRec;
    int ret_val, i;
    FIELDDESC_T *pField;

    /* build systable record */
    sc_memset(&newTableRec, 0, sizeof(SYSTABLE_REC_T));
    newTableRec.tableId = tableId;
    sc_strcpy(newTableRec.tableName, tableName);
    newTableRec.numFields = numFields;
    newTableRec.recordLen = recordLen;
    newTableRec.h_recordLen = heap_recordLen;
    newTableRec.numRecords = -1;
    newTableRec.nextId = -1;
    newTableRec.flag = (DB_UINT16) type;
    newTableRec.has_variabletype = has_variabletype;
    newTableRec.maxRecords = max_records;
    if (max_records > 0)
    {
        sc_strncpy(newTableRec.columnName, column_name, FIELD_NAME_LENG - 1);
        newTableRec.columnName[FIELD_NAME_LENG - 1] = '\0';
    }
    else
        newTableRec.columnName[0] = '\0';

    if (isTempTable_name(tableName))
    {
        SYSFIELD_T *field_info_list, *pList;

        /* prepare fields information structure */
        field_info_list = PMEM_ALLOC(sizeof(SYSFIELD_T) * numFields);
        if (field_info_list == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFMEMORY, 0);
            return DB_E_OUTOFMEMORY;
        }

        /* build fields information */
        for (i = 0; i < numFields; i++)
        {
            pList = field_info_list + i;
            pField = pFieldDesc + i;

            pList->fieldId = i; //Cont_NewFieldID();
            pList->tableId = tableId;
            pList->position = i;
            sc_strcpy(pList->fieldName, pField->fieldName);
            pList->offset = pField->offset_;
            pList->h_offset = pField->h_offset_;
            if (pField->type_ == DT_DECIMAL)
                pList->length = pField->length_ - 3;
            else if (IS_BYTE(pField->type_))
                pList->length = pField->length_ - 4;
            else if (IS_N_STRING(pField->type_))
                pList->length = pField->length_ - 1;
            else
                pList->length = pField->length_;
            pList->scale = pField->scale_;
            pList->type = pField->type_;
            pList->fixed_part = pField->fixed_part;
            pList->flag = pField->flag;
            sc_memcpy(pList->defaultValue, pField->defaultValue, 40);
#if defined(MDB_DEBUG)
            if (pField->collation == MDB_COL_NONE)
                sc_assert(0, __FILE__, __LINE__);
#endif
            pList->collation = pField->collation;
        }

        /* register table and fields information into schema cache */
        ret_val = _Cache_AddTempTableSchema((SYSTABLE_T *) & newTableRec,
                numFields, field_info_list);
        if (ret_val < 0)
        {
            PMEM_FREENUL(field_info_list);
            return ret_val;
        }

        PMEM_FREENUL(field_info_list);
    }
    else    /* regular table */
    {
        int cursor;
        OID table_oid;
        OID *field_oid_list;
        SYSFIELD_REC_T newFieldRec;


        /* prepare fields information pointer array */
        field_oid_list = PMEM_ALLOC(OID_SIZE * numFields);
        if (field_oid_list == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFMEMORY, 0);
            return DB_E_OUTOFMEMORY;
        }

        /* insert field information into sysfields */
        ret_val =
                LockMgr_Lock("sysfields", transId, LOWPRIO, LK_EXCLUSIVE,
                WAIT);
        if (ret_val < 0)
        {
            PMEM_FREENUL(field_oid_list);
            return ret_val;
        }

        cursor = Open_Cursor(transId, "sysfields", LK_EXCLUSIVE,
                -1, NULL, NULL, NULL);
        if (cursor < 0)
        {
            PMEM_FREENUL(field_oid_list);
            return cursor;
        }

        for (i = 0; i < numFields; i++)
        {
            sc_memset(&newFieldRec, 0, sizeof(SYSFIELD_REC_T));
            newFieldRec.fieldId = i;    //Cont_NewFieldID();
            newFieldRec.tableId = tableId;
            newFieldRec.position = i;
            pField = pFieldDesc + i;

            sc_strcpy(newFieldRec.fieldName, pField->fieldName);
            newFieldRec.offset = pField->offset_;
            newFieldRec.h_offset = pField->h_offset_;
            if (pField->type_ == DT_DECIMAL)
                newFieldRec.length = pField->length_ - 3;
            else if (IS_BYTE(pField->type_))
                newFieldRec.length = pField->length_ - 4;
            else if (IS_N_STRING(pField->type_))
                newFieldRec.length = pField->length_ - 1;
            else
                newFieldRec.length = pField->length_;
            newFieldRec.scale = pField->scale_;
            newFieldRec.type = pField->type_;
            newFieldRec.fixed_part = pField->fixed_part;
            newFieldRec.flag = pField->flag;
            sc_memcpy(newFieldRec.defaultValue, pField->defaultValue, 40);
#if defined(MDB_DEBUG)
            if (pField->collation == MDB_COL_NONE)
                sc_assert(0, __FILE__, __LINE__);
#endif
            newFieldRec.collation = pField->collation;

            ret_val = __Insert(cursor, (char *) &newFieldRec,
                    sizeof(SYSFIELD_REC_T), &field_oid_list[i], 0);
            if (ret_val < 0)
            {
                Close_Cursor(cursor);
                PMEM_FREENUL(field_oid_list);
                return ret_val;
            }
        }

        Close_Cursor(cursor);

        /* insert table information into systables */
        ret_val =
                LockMgr_Lock("systables", transId, LOWPRIO, LK_EXCLUSIVE,
                WAIT);
        if (ret_val < 0)
        {
            return ret_val;
        }

        cursor = Open_Cursor(transId, "systables", LK_EXCLUSIVE,
                -1, NULL, NULL, NULL);
        if (cursor < 0)
        {
            return cursor;
        }

        ret_val =
                __Insert(cursor, (char *) &newTableRec, sizeof(SYSTABLE_REC_T),
                &table_oid, 0);
        if (ret_val < 0)
        {
            Close_Cursor(cursor);
            return ret_val;
        }

        Close_Cursor(cursor);

        ret_val = _Cache_AddTableSchema(table_oid, numFields, field_oid_list);
        if (ret_val < 0)
        {
            PMEM_FREENUL(field_oid_list);
            return ret_val;
        }

        PMEM_FREENUL(field_oid_list);
    }

    return tableId;
}

/*****************************************************************************/
//! _Schema_InputIndex 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param transId(in)       :
 * \param tableName(in)     :
 * \param indexName(in)     : 
 * \param numFields(in)     :
 * \param pFieldDesc(in)    :
 * \param flag(in)          :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - SYSINDEXFIELD_T 구조체 변경
 *  - collation의 값을 cache에 가지고 있는다
 *****************************************************************************/
int
_Schema_InputIndex(int transId, char *tableName, char *indexName,
        int numFields, FIELDDESC_T * pFieldDesc, DB_UINT8 flag)
{
    int ret_val, i;
    SYSTABLE_T *pTable;
    int tableId;
    int indexId;
    SYSINDEX_REC_T newIndexRec;

    /***************************/
    /* get tableId and indexId */
    /***************************/

    if (isTempTable_name(tableName))
    {
        ret_val = _Cache_GetTempTableInfo(tableName, &pTable);
        if (ret_val < 0)
        {
            if (ret_val == DB_E_NOTCACHED)
                ret_val = DB_E_UNKNOWNTABLE;
            return ret_val;
        }
        tableId = pTable->tableId;
        ret_val = _Cache_GetTempIndexInfo(indexName, NULL, NULL, NULL);
        if (ret_val == DB_SUCCESS)
            return DB_E_ALREADYCACHED;

        indexId = _Cache_FindFreeTempIndexId(tableId);
        if (indexId < 0 || indexId >= MAX_INDEX_NO)
        {
            return DB_E_EXCEEDNUMINDEXES;
        }
    }
    else    /* regular table */
    {
        OID table_oid;

        ret_val = _Cache_GetTableInfo(tableName, &table_oid, &pTable);
        if (ret_val < 0)
        {
            if (ret_val == DB_E_NOTCACHED)
                ret_val = DB_E_UNKNOWNTABLE;
            return ret_val;
        }

        tableId = pTable->tableId;

        ret_val = _Cache_GetIndexInfo(indexName, NULL, NULL, NULL,
                NULL_OID, NULL);
        if (ret_val == DB_SUCCESS)
        {
            return DB_E_ALREADYCACHED;
        }



        if (isTempIndex_name(indexName))
        {
            indexId = _Cache_AllocReguTempIndexId(tableName);
            if (indexId < MAX_INDEX_NO || indexId >= (2 * MAX_INDEX_NO))
            {
                return DB_E_EXCEEDNUMINDEXES;
            }
        }
        else    /* regular index */
        {
            DB_UINT8 indexId_map[(MAX_INDEX_NO - 1) / 8 + 1];
            int num_indexes;
            OID *index_oid_list;
            SYSINDEX_T **sysIndex_list;

            num_indexes =
                    _Cache_GetTableIndexesInfo(tableName, &index_oid_list);
            if (num_indexes < 0)
            {
                return num_indexes;
            }

            sysIndex_list = (SYSINDEX_T **)
                    PMEM_ALLOC(sizeof(SYSINDEX_T *) * MAX_INDEX_NO);
            if (sysIndex_list == NULL)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFMEMORY, 0);
                return DB_E_OUTOFMEMORY;
            }

            for (i = 0; i < num_indexes; i++)
            {
                sysIndex_list[i] = (SYSINDEX_T *) OID_Point(index_oid_list[i]);
                if (sysIndex_list[i] == NULL)
                {
                    PMEM_FREE(sysIndex_list);
                    return DB_E_OIDPOINTER;
                }
            }

            /* build indexID map */
            sc_memset(indexId_map, 0, sizeof(indexId_map));
            for (i = 0; i < num_indexes; i++)
            {
                indexId_map[sysIndex_list[i]->indexId / 8]
                        |= (1 << (7 - (sysIndex_list[i]->indexId % 8)));
            }

            for (indexId = 0; indexId < MAX_INDEX_NO; indexId++)
            {
                if ((indexId_map[indexId / 8] & (1 << (7 - (indexId % 8)))) ==
                        0)
                    break;
            }

            if (indexId < 0 || indexId >= MAX_INDEX_NO)
            {
                PMEM_FREE(sysIndex_list);
                return DB_E_EXCEEDNUMINDEXES;
            }
            PMEM_FREE(sysIndex_list);
        }
    }

    /* make index information record */
    sc_memset((char *) &newIndexRec, 0, sizeof(SYSINDEX_REC_T));
    newIndexRec.indexId = indexId;
    newIndexRec.tableId = tableId;
    sc_strcpy(newIndexRec.indexName, indexName);
    newIndexRec.type = flag;
    newIndexRec.numFields = numFields;

    if (isTempTable_name(tableName) || isTempIndex_name(indexName))
    {
        SYSINDEXFIELD_T *field_info_list;

        field_info_list = PMEM_ALLOC(sizeof(SYSINDEXFIELD_T) * numFields);
        if (field_info_list == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFMEMORY, 0);
            return DB_E_OUTOFMEMORY;
        }

        for (i = 0; i < numFields; i++)
        {
            field_info_list[i].tableId = tableId;
            field_info_list[i].indexId = indexId;
            field_info_list[i].keyPosition = i;
            field_info_list[i].order =
                    (pFieldDesc[i].flag & FD_DESC) ? 'D' : 'A';
#if defined(MDB_DEBUG)
            if (pFieldDesc[i].collation == MDB_COL_NONE)
                sc_assert(0, __FILE__, __LINE__);
#endif // MDB_DEBUG
            field_info_list[i].collation = pFieldDesc[i].collation;
            sc_memcpy(&(field_info_list[i].fieldId),
                    pFieldDesc[i].defaultValue,
                    sizeof(field_info_list[i].fieldId));
        }

        ret_val = _Cache_AddTempIndexSchema(tableName,
                (SYSINDEX_T *) & newIndexRec, numFields, field_info_list);
        if (ret_val < 0)
        {
            PMEM_FREENUL(field_info_list);
            return ret_val;
        }

        PMEM_FREENUL(field_info_list);
    }
    else    /* regular index on the regular table */
    {
        int cursor;
        OID index_oid;
        OID *field_oid_list;
        SYSINDEXFIELD_REC_T newIndexFieldRec;

        ret_val =
                LockMgr_Lock("sysindexes", transId, LOWPRIO, LK_EXCLUSIVE,
                WAIT);
        if (ret_val < 0)
        {
            return ret_val;
        }

        cursor = Open_Cursor(transId, "sysindexes", LK_EXCLUSIVE,
                -1, NULL, NULL, NULL);
        if (cursor < 0)
        {
            return cursor;
        }

        ret_val =
                __Insert(cursor, (char *) &newIndexRec, sizeof(SYSINDEX_REC_T),
                &index_oid, 0);
        if (ret_val < 0)
        {
            Close_Cursor(cursor);
            if (ret_val == DB_E_DUPUNIQUEKEY)
                ret_val = DB_E_DUPINDEXNAME;
            return ret_val;
        }

        Close_Cursor(cursor);

        field_oid_list = PMEM_ALLOC(OID_SIZE * numFields);
        if (field_oid_list == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFMEMORY, 0);
            return DB_E_OUTOFMEMORY;
        }

        ret_val = LockMgr_Lock("sysindexfields", transId, LOWPRIO,
                LK_EXCLUSIVE, WAIT);
        if (ret_val < 0)
        {
            PMEM_FREENUL(field_oid_list);
            return ret_val;
        }

        cursor = Open_Cursor(transId, "sysindexfields", LK_EXCLUSIVE,
                -1, NULL, NULL, NULL);
        if (cursor < 0)
        {
            PMEM_FREENUL(field_oid_list);
            return cursor;
        }

        for (i = 0; i < numFields; i++)
        {
            sc_memset(&newIndexFieldRec, 0, sizeof(SYSINDEXFIELD_REC_T));
            newIndexFieldRec.tableId = tableId;
            newIndexFieldRec.indexId = indexId;
            newIndexFieldRec.keyPosition = i;
            newIndexFieldRec.order =
                    (pFieldDesc[i].flag & FD_DESC) ? 'D' : 'A';
#if defined(MDB_DEBUG)
            if (pFieldDesc[i].collation == MDB_COL_NONE)
                sc_assert(0, __FILE__, __LINE__);
#endif // MDB_DEBUG

            newIndexFieldRec.collation = pFieldDesc[i].collation;

            sc_memcpy(&(newIndexFieldRec.fieldId), pFieldDesc[i].defaultValue,
                    sizeof(newIndexFieldRec.fieldId));

            ret_val = __Insert(cursor, (char *) &newIndexFieldRec,
                    sizeof(SYSINDEXFIELD_REC_T), &field_oid_list[i], 0);
            if (ret_val < 0)
            {
                Close_Cursor(cursor);
                PMEM_FREENUL(field_oid_list);
                return ret_val;
            }
        }

        Close_Cursor(cursor);

        ret_val = _Cache_AddIndexSchema(tableName, index_oid,
                numFields, field_oid_list);
        if (ret_val < 0)
        {
            PMEM_FREENUL(field_oid_list);
            return ret_val;
        }

        PMEM_FREENUL(field_oid_list);
    }

    return indexId;
}

/*****************************************************************************/
//! _Schema_RemoveTable 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param transId : 
 * \param tableName :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
_Schema_RemoveTable(int transId, char *tableName)
{
    int ret_val;
    int cursor;
    OID Table_OID;
    OID index_oid;
    SYSTABLE_T *pSysTable;
    SYSINDEX_T *pSysIndex;
    int tableId;
    int use_indexId;
    int i, num_indexes;
    OID *index_oid_list;
    OID Index_OIDs[MAX_INDEX_NO];
    char indexName[INDEX_NAME_LENG];
    struct KeyValue minkey;
    struct KeyValue maxkey;
    FIELDVALUE_T fieldvalue;

    if (isTempTable_name(tableName))
    {
        return _Cache_DeleteTempTableSchema(tableName);
    }

    /* get table information */
    ret_val = _Cache_GetTableInfo(tableName, &Table_OID, &pSysTable);
    if (ret_val < 0)
    {
        if (ret_val == DB_E_NOTCACHED)
            ret_val = DB_E_UNKNOWNTABLE;
        return ret_val;
    }

    tableId = pSysTable->tableId;

    num_indexes = _Cache_GetTableIndexesInfo(tableName, &index_oid_list);
    if (num_indexes < 0)
    {
        return num_indexes;
    }

    if (num_indexes > 0)
    {
        sc_memcpy((char *) &Index_OIDs[0], (char *) index_oid_list,
                (OID_SIZE * num_indexes));
    }

    /* Following locking order must be preserved.
       systables => sysfields => sysindexes => sysindexfields 
     */

    ret_val = LockMgr_Lock("systables", transId, LOWPRIO, LK_EXCLUSIVE, WAIT);
    if (ret_val < 0)
    {
        return ret_val;
    }

    ret_val = LockMgr_Lock("sysfields", transId, LOWPRIO, LK_EXCLUSIVE, WAIT);
    if (ret_val < 0)
    {
        return ret_val;
    }

    // sysindexes, sysindexfields will be locked in _Schema_RemoveIndex().
    // I wonder why these two system tables are locked at this function.

    /* Following removing order must be preserver: for rollback problem
       sysindexes => sysindexfields => systables => sysfields 
     */

    /* Remove indexes */
    for (i = 0; i < num_indexes; i++)
    {
        pSysIndex = (SYSINDEX_T *) OID_Point(Index_OIDs[i]);
        if (pSysIndex == NULL)
        {
            MDB_SYSLOG(("Index OID List is inconsistent.\n"));
            return DB_FAIL;
        }
        sc_strcpy(indexName, pSysIndex->indexName);

        ret_val = _Schema_RemoveIndex(transId, indexName);
        if (ret_val < 0)
        {
            return ret_val;
        }
    }

    /* (3) remove table info from systables table */
    ret_val = _Cache_GetIndexInfo("$idx_systables_tblid",
            &index_oid, NULL, NULL, NULL_OID, NULL);
    if (ret_val < 0)
    {
        return ret_val;
    }
    pSysIndex = (SYSINDEX_T *) OID_Point(index_oid);
    if (pSysIndex == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    use_indexId = pSysIndex->indexId;

    ret_val = dbi_FieldValue(MDB_NN, DT_INTEGER, 0,
            mdb_offsetof(SYSTABLE_T, tableId), sizeof(int), 0,
            MDB_COL_NUMERIC, &tableId, DT_INTEGER, sizeof(int), &fieldvalue);
    if (ret_val != DB_SUCCESS)
        return ret_val;
    dbi_KeyValue(1, &fieldvalue, &minkey);
    dbi_KeyValue(1, &fieldvalue, &maxkey);

    cursor = Open_Cursor(transId, "systables", LK_EXCLUSIVE,
            (DB_INT16) use_indexId, &minkey, &maxkey, NULL);
    if (cursor < 0)
    {
        return cursor;
    }

    ret_val = __Remove(cursor);
    if (ret_val < 0)
    {
        Close_Cursor(cursor);
        return ret_val;
    }

    Close_Cursor(cursor);

    /* (4) remove fields info from sysfields table */
    ret_val = _Cache_GetIndexInfo("$idx_sysfields_tblid",
            &index_oid, NULL, NULL, NULL_OID, NULL);
    if (ret_val < 0)
    {
        return ret_val;
    }
    pSysIndex = (SYSINDEX_T *) OID_Point(index_oid);
    if (pSysIndex == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    use_indexId = pSysIndex->indexId;

    ret_val = dbi_FieldValue(MDB_NN, DT_INTEGER, 1,
            mdb_offsetof(SYSFIELD_T, tableId), sizeof(int), 0,
            MDB_COL_NUMERIC, &tableId, DT_INTEGER, sizeof(int), &fieldvalue);
    if (ret_val != DB_SUCCESS)
        return ret_val;
    dbi_KeyValue(1, &fieldvalue, &minkey);
    dbi_KeyValue(1, &fieldvalue, &maxkey);

    cursor = Open_Cursor(transId, "sysfields", LK_EXCLUSIVE,
            (DB_INT16) use_indexId, &minkey, &maxkey, NULL);
    if (cursor < 0)
    {
        return cursor;
    }

    while (1)
    {
        ret_val = __Remove(cursor);
        if (ret_val < 0)
        {
            if (ret_val == DB_E_NOMORERECORDS)
                break;
            Close_Cursor(cursor);
            return ret_val;
        }
    }

    Close_Cursor(cursor);

    /* remove schema cache information */
    ret_val = _Cache_DeleteTableSchema(tableName, Table_OID);

    return ret_val;
}

/*****************************************************************************/
//! _Schema_RemoveIndex 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param transId(in)   : transaction id
 * \param indexName(in) : index's name
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
_Schema_RemoveIndex(int transId, char *indexName)
{
    int ret_val;
    int cursor;
    OID Index_OID;
    OID index_oid;
    SYSINDEX_T *pSysIndex;
    int tableId;
    int indexId;
    int use_indexId;
    struct KeyValue minkey;
    struct KeyValue maxkey;
    FIELDVALUE_T fieldvalue[2];

    if (isTempIndex_name(indexName))
    {
        return _Cache_DeleteTempIndexSchema(indexName);
    }

    /* get index schema information */
    ret_val = _Cache_GetIndexInfo(indexName, &Index_OID, NULL, NULL,
            NULL_OID, NULL);
    if (ret_val < 0)
    {
        if (ret_val == DB_E_NOTCACHED)
            ret_val = DB_E_UNKNOWNINDEX;
        return ret_val;
    }
    pSysIndex = (SYSINDEX_T *) OID_Point(Index_OID);
    if (pSysIndex == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    tableId = pSysIndex->tableId;
    indexId = pSysIndex->indexId;

    /* hold locks */
    ret_val = LockMgr_Lock("sysindexes", transId, LOWPRIO, LK_EXCLUSIVE, WAIT);
    if (ret_val < 0)
    {
        return ret_val;
    }

    ret_val =
            LockMgr_Lock("sysindexfields", transId, LOWPRIO, LK_EXCLUSIVE,
            WAIT);
    if (ret_val < 0)
    {
        return ret_val;
    }

    /* remove index info from sysindexes table */
    ret_val = _Cache_GetIndexInfo("$idx_sysindexes_tblid_idxid",
            &index_oid, NULL, NULL, NULL_OID, NULL);
    if (ret_val < 0)
    {
        return ret_val;
    }
    pSysIndex = (SYSINDEX_T *) OID_Point(index_oid);
    if (pSysIndex == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    use_indexId = pSysIndex->indexId;

    ret_val = dbi_FieldValue(MDB_NN, DT_INTEGER, 0,
            mdb_offsetof(SYSINDEX_T, tableId), sizeof(int), 0,
            MDB_COL_NUMERIC, &tableId, DT_INTEGER, sizeof(int),
            &fieldvalue[0]);
    if (ret_val != DB_SUCCESS)
        return ret_val;

    /* FIELDVALUE bug (value's type: DT_TINYINT --> DT_INTEGER)` */
    ret_val = dbi_FieldValue(MDB_NN, DT_TINYINT, 1,
            mdb_offsetof(SYSINDEX_T, indexId), sizeof(char), 0,
            MDB_COL_NUMERIC, &indexId, DT_INTEGER, sizeof(char),
            &fieldvalue[1]);
    if (ret_val != DB_SUCCESS)
        return ret_val;

    dbi_KeyValue(2, fieldvalue, &minkey);
    dbi_KeyValue(2, fieldvalue, &maxkey);

    cursor = Open_Cursor(transId, "sysindexes", LK_EXCLUSIVE,
            (DB_INT16) use_indexId, &minkey, &maxkey, NULL);
    if (cursor < 0)
    {
        return cursor;
    }

    ret_val = __Remove(cursor);
    if (ret_val < 0)
    {
        Close_Cursor(cursor);
        return ret_val;
    }

    Close_Cursor(cursor);

    /* remove index fields info from sysindexfields table */
    ret_val = _Cache_GetIndexInfo("$idx_sysindexfields_tblid_idxid",
            &index_oid, NULL, NULL, NULL_OID, NULL);
    if (ret_val < 0)
    {
        return ret_val;
    }
    pSysIndex = (SYSINDEX_T *) OID_Point(index_oid);
    if (pSysIndex == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    use_indexId = pSysIndex->indexId;

    ret_val = dbi_FieldValue(MDB_NN, DT_INTEGER, 0,
            mdb_offsetof(SYSINDEXFIELD_T, tableId), sizeof(int), 0,
            MDB_COL_NUMERIC, &tableId, DT_INTEGER, sizeof(int),
            &fieldvalue[0]);
    if (ret_val != DB_SUCCESS)
        return ret_val;

    ret_val = dbi_FieldValue(MDB_NN, DT_TINYINT, 1,
            mdb_offsetof(SYSINDEXFIELD_T, indexId), sizeof(char), 0,
            MDB_COL_NUMERIC, &indexId, DT_INTEGER, sizeof(char),
            &fieldvalue[1]);
    if (ret_val != DB_SUCCESS)
        return ret_val;

    dbi_KeyValue(2, fieldvalue, &minkey);
    dbi_KeyValue(2, fieldvalue, &maxkey);

    cursor = Open_Cursor(transId, "sysindexfields", LK_EXCLUSIVE,
            (DB_INT16) use_indexId, &minkey, &maxkey, NULL);
    if (cursor < 0)
    {
        return cursor;
    }

    while (1)
    {
        ret_val = __Remove(cursor);
        if (ret_val < 0)
        {
            if (ret_val == DB_E_NOMORERECORDS)
                break;
            Close_Cursor(cursor);
            return ret_val;
        }
    }

    Close_Cursor(cursor);

    ret_val = _Cache_DeleteIndexSchema(indexName, Index_OID);

    return ret_val;
}

/*****************************************************************************/
//! _Schema_RenameTable
/*! \breif rename index in schema catalog.
 ************************************
 * \param transId  :
 * \param old_name :
 * \param new_name : 
 ************************************
 * \return  return_value
 ************************************
 * \note  None.
 *****************************************************************************/
int
_Schema_RenameTable(int transId, char *old_name, char *new_name)
{
    int ret_val;
    int cursor;
    int tableId;
    int use_indexId;
    DB_UINT32 recsize;
    OID table_oid;
    OID index_oid;
    SYSTABLE_T *pSysTable;
    SYSINDEX_T *pSysIndex;
    struct KeyValue minkey;
    struct KeyValue maxkey;
    FIELDVALUE_T fieldvalue;
    SYSTABLE_REC_T updTableRec;

    if (isTempTable_name(old_name) || isTempTable_name(new_name))
    {
        return DB_E_INVALIDPARAM;
    }

    /* Regular Table */
    ret_val = _Cache_GetTableInfo(old_name, &table_oid, &pSysTable);
    if (ret_val < 0)
    {
        if (ret_val == DB_E_NOTCACHED)
            ret_val = DB_E_UNKNOWNTABLE;
        return ret_val;
    }
    tableId = pSysTable->tableId;

    ret_val = _Cache_GetIndexInfo("$idx_systables_tblid",
            &index_oid, NULL, NULL, NULL_OID, NULL);
    if (ret_val < 0)
    {
        return ret_val;
    }
    pSysIndex = (SYSINDEX_T *) OID_Point(index_oid);
    if (pSysIndex == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    use_indexId = pSysIndex->indexId;

    ret_val = LockMgr_Lock("systables", transId, LOWPRIO, LK_EXCLUSIVE, WAIT);
    if (ret_val < 0)
    {
        return ret_val;
    }

    ret_val = dbi_FieldValue(MDB_NN, DT_INTEGER, 0,
            mdb_offsetof(SYSTABLE_T, tableId), sizeof(int), 0,
            MDB_COL_NUMERIC, &tableId, DT_INTEGER, sizeof(int), &fieldvalue);
    if (ret_val != DB_SUCCESS)
        return ret_val;
    dbi_KeyValue(1, &fieldvalue, &minkey);
    dbi_KeyValue(1, &fieldvalue, &maxkey);

    cursor = Open_Cursor(transId, "systables", LK_EXCLUSIVE,
            (DB_INT16) use_indexId, &minkey, &maxkey, NULL);
    if (cursor < 0)
    {
        return cursor;
    }

    ret_val = __Read(cursor, (char *) &updTableRec, NULL, 0, &recsize, 0);

    if (ret_val < 0)
    {
        Close_Cursor(cursor);
        if (ret_val == DB_E_NOMORERECORDS)
            ret_val = DB_E_UNKNOWNTABLE;
        return ret_val;
    }

    sc_strncpy(updTableRec.tableName, new_name, REL_NAME_LENG - 1);
    updTableRec.tableName[REL_NAME_LENG - 1] = '\0';

    ret_val = __Update(cursor, (char *) &updTableRec, sizeof(SYSTABLE_REC_T),
            NULL);
    if (ret_val < 0)
    {
        Close_Cursor(cursor);
        return ret_val;
    }

    Close_Cursor(cursor);

    ret_val = _Cache_RenameTableName(old_name, new_name);

    return ret_val;
}

/*****************************************************************************/
//! _Schema_RenameIndex
/*! \breif rename index in schema catalog.
 ************************************
 * \param transId  :
 * \param old_name :
 * \param new_name : 
 ************************************
 * \return  return_value
 ************************************
 * \note  None.
 *****************************************************************************/
int
_Schema_RenameIndex(int transId, char *old_name, char *new_name)
{
    int ret_val;
    int cursor;
    DB_UINT32 recsize;
    OID index_oid;
    SYSINDEX_T *pSysIndex;
    int tableId;
    int indexId;
    int use_indexId;
    struct KeyValue minkey;
    struct KeyValue maxkey;
    FIELDVALUE_T fieldvalue[2];
    SYSINDEX_REC_T updIndexRec;

    if (isTempIndex_name(old_name) || isTempIndex_name(new_name))
        return DB_E_INVALIDPARAM;

    /* Regular Index */

    /* get index schema information */
    ret_val = _Cache_GetIndexInfo(old_name, &index_oid, NULL, NULL,
            NULL_OID, NULL);
    if (ret_val < 0)
    {
        if (ret_val == DB_E_NOTCACHED)
            ret_val = DB_E_UNKNOWNINDEX;
        return ret_val;
    }
    pSysIndex = (SYSINDEX_T *) OID_Point(index_oid);
    if (pSysIndex == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    tableId = pSysIndex->tableId;
    indexId = pSysIndex->indexId;

    /* remove index info from sysindexes table */
    ret_val = _Cache_GetIndexInfo("$idx_sysindexes_tblid_idxid",
            &index_oid, NULL, NULL, NULL_OID, NULL);
    if (ret_val < 0)
    {
        return ret_val;
    }
    pSysIndex = (SYSINDEX_T *) OID_Point(index_oid);
    if (pSysIndex == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    use_indexId = pSysIndex->indexId;

    ret_val = LockMgr_Lock("sysindexes", transId, LOWPRIO, LK_EXCLUSIVE, WAIT);
    if (ret_val < 0)
    {
        return ret_val;
    }

    ret_val = dbi_FieldValue(MDB_NN, DT_INTEGER, 0,
            mdb_offsetof(SYSINDEX_T, tableId), sizeof(int), 0,
            MDB_COL_NUMERIC, &tableId, DT_INTEGER, sizeof(int),
            &fieldvalue[0]);
    if (ret_val != DB_SUCCESS)
        return ret_val;

    ret_val = dbi_FieldValue(MDB_NN, DT_TINYINT, 1,
            mdb_offsetof(SYSINDEX_T, indexId), sizeof(char), 0,
            MDB_COL_NUMERIC, &indexId, DT_INTEGER, sizeof(char),
            &fieldvalue[1]);
    if (ret_val != DB_SUCCESS)
        return ret_val;

    dbi_KeyValue(2, fieldvalue, &minkey);
    dbi_KeyValue(2, fieldvalue, &maxkey);

    cursor = Open_Cursor(transId, "sysindexes", LK_EXCLUSIVE,
            (DB_INT16) use_indexId, &minkey, &maxkey, NULL);
    if (cursor < 0)
    {
        return cursor;
    }

    ret_val = __Read(cursor, (char *) &updIndexRec, NULL, 0, &recsize, 0);

    if (ret_val < 0)
    {
        Close_Cursor(cursor);
        if (ret_val == DB_E_NOMORERECORDS)
            ret_val = DB_E_UNKNOWNINDEX;
        return ret_val;
    }

    sc_strncpy(updIndexRec.indexName, new_name, INDEX_NAME_LENG - 1);
    updIndexRec.indexName[INDEX_NAME_LENG - 1] = '\0';

    ret_val = __Update(cursor, (char *) &updIndexRec, sizeof(SYSINDEX_REC_T),
            NULL);
    if (ret_val < 0)
    {
        Close_Cursor(cursor);
        return ret_val;
    }

    Close_Cursor(cursor);

    ret_val = _Cache_RenameIndexName(old_name, new_name);

    return ret_val;
}

/*****************************************************************************/
//! _Schema_GetTableInfo 
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   tableName:
 * \param   pSysTable: 
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
int
_Schema_GetTableInfo(char *tableName, SYSTABLE_T * pSysTable)
{
    int ret_val;

    if (isTempTable_name(tableName))
    {
        SYSTABLE_T *pTable;

        ret_val = _Cache_GetTempTableInfo(tableName, &pTable);
        if (ret_val < 0)
        {
            if (ret_val == DB_E_NOTCACHED)
                ret_val = DB_E_UNKNOWNTABLE;
            return ret_val;
        }

        sc_memcpy(pSysTable, pTable, sizeof(SYSTABLE_T));
    }
    else    /* regular table */
    {
        OID table_oid;
        SYSTABLE_T *systbl;

        ret_val = _Cache_GetTableInfo(tableName, &table_oid, &systbl);
        if (ret_val < 0)
        {
            if (ret_val == DB_E_NOTCACHED)
                ret_val = DB_E_UNKNOWNTABLE;
            return ret_val;
        }

        sc_memcpy(pSysTable, systbl, sizeof(SYSTABLE_T));
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! _Schema_GetTableInfoEx
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   mode(in)            : 1(User Table), 2(Temp Table)
 * \param   pSysTable(in/out)   : 
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *  - 구현되어 있는 모드는 1번(USER TABLE)만 구현되어 있다
 *****************************************************************************/
int
_Schema_GetTablesInfoEx(int mode, SYSTABLE_T * pSysTable, int userid)
{
    int ret_val;
    int table_cnt = 0;

    struct Container *Cont = NULL;

    if (mode == 1)
    {
        int transId = *(int *) CS_getspecific(my_trans_id);
        SYSINDEX_T *pSysIndex;
        struct KeyValue minkey;
        struct KeyValue maxkey;
        int cursor;
        int use_indexId;
        DB_UINT32 recsize;
        OID index_oid;
        SYSTABLE_REC_T sysTableRec;

        int value;
        FIELDVALUE_T fieldvalue;

        ret_val = _Cache_GetIndexInfo("$idx_systables_tblid",
                &index_oid, NULL, NULL, NULL_OID, NULL);
        if (ret_val < 0)
        {
            return ret_val;
        }
        pSysIndex = (SYSINDEX_T *) OID_Point(index_oid);
        if (pSysIndex == NULL)
        {
            return DB_E_OIDPOINTER;
        }

        use_indexId = pSysIndex->indexId;

        ret_val = LockMgr_Lock("systables", transId, LOWPRIO, LK_SHARED, WAIT);
        if (ret_val < 0)
        {
            return ret_val;
        }

        value = _USER_TABLEID_BASE;
        ret_val = dbi_FieldValue(MDB_NN, DT_INTEGER, 0,
                mdb_offsetof(SYSTABLE_T, tableId), sizeof(int), 0,
                MDB_COL_NUMERIC, &value, DT_INTEGER, sizeof(int), &fieldvalue);
        if (ret_val != DB_SUCCESS)
            return ret_val;
        dbi_KeyValue(1, &fieldvalue, &minkey);

        value = _TEMP_TABLEID_BASE;
        ret_val = dbi_FieldValue(MDB_NN, DT_INTEGER, 0,
                mdb_offsetof(SYSTABLE_T, tableId), sizeof(int), 0,
                MDB_COL_NUMERIC, &value, DT_INTEGER, sizeof(int), &fieldvalue);
        if (ret_val != DB_SUCCESS)
            return ret_val;
        dbi_KeyValue(1, &fieldvalue, &maxkey);

        cursor = Open_Cursor(transId, "systables", LK_SHARED,
                (DB_INT16) use_indexId, &minkey, &maxkey, NULL);
        if (cursor < 0)
        {
            return cursor;
        }

        do
        {
            ret_val =
                    __Read(cursor, (char *) &sysTableRec, NULL, 1, &recsize,
                    0);

            if (ret_val < 0)
            {
                Close_Cursor(cursor);

                if (ret_val != DB_E_NOMORERECORDS)
                    return ret_val;

                break;
            }

            Cont = Cont_Search_Tableid(sysTableRec.tableId);
            if (Cont == NULL)
                return DB_E_UNKNOWNTABLE;
            if (!doesUserCreateTable(Cont, userid))
                continue;
            ++table_cnt;
            sc_memcpy(pSysTable++, (char *) &sysTableRec, sizeof(SYSTABLE_T));
        } while (ret_val != DB_E_NOMORERECORDS);

        Close_Cursor(cursor);
    }
    else
    {
        return DB_E_NOTSUPPORTED;
    }

    return table_cnt;
}

/*****************************************************************************/
//! _Schema_GetFieldsInfo
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   tableName :
 * \param   pFieldInfo:
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
int
_Schema_GetFieldsInfo(char *tableName, SYSFIELD_T * pFieldInfo)
{
    int num_fields;

    if (isTempTable_name(tableName))
    {
        SYSFIELD_T *pFields;

        num_fields = _Cache_GetTempFieldInfo(tableName, &pFields);
        if (num_fields < 0)
        {
            if (num_fields == DB_E_NOTCACHED)
                num_fields = DB_E_UNKNOWNTABLE;
            return num_fields;
        }

        sc_memcpy(pFieldInfo, pFields, (sizeof(SYSFIELD_T) * num_fields));
    }
    else    /* regular table */
    {
        OID *field_oid_list;
        int i;
        char *pbuf = NULL;

        num_fields = _Cache_GetFieldInfo(tableName, &field_oid_list);
        if (num_fields < 0)
        {
            if (num_fields == DB_E_NOTCACHED)
                num_fields = DB_E_UNKNOWNTABLE;
            return num_fields;
        }

        for (i = 0; i < num_fields; i++)
        {
            pbuf = (char *) OID_Point(field_oid_list[i]);
            if (pbuf == NULL)
            {
                return DB_E_OIDPOINTER;
            }

            sc_memcpy(&pFieldInfo[i], pbuf, sizeof(SYSFIELD_T));
        }
    }

    return num_fields;
}

int
_Schema_GetSingleFieldsInfo(char *tableName, char *field_name,
        SYSFIELD_T * pFieldInfo)
{
    int num_fields, i;

    if (isTempTable_name(tableName))
    {
        SYSFIELD_T *pFields;

        num_fields = _Cache_GetTempFieldInfo(tableName, &pFields);
        if (num_fields < 0)
        {
            if (num_fields == DB_E_NOTCACHED)
                num_fields = DB_E_UNKNOWNTABLE;
            return num_fields;
        }

        for (i = 0; i < num_fields; i++)
        {
            /* compare fieldname case-insenstively */
            if (!mdb_strcmp(pFields[i].fieldName, field_name))
            {
                sc_memcpy(pFieldInfo, pFields + i, sizeof(SYSFIELD_T));
                return 1;
            }
        }
    }
    else    /* regular table */
    {
        OID *field_oid_list;
        char *pbuf = NULL;

        num_fields = _Cache_GetFieldInfo(tableName, &field_oid_list);
        if (num_fields < 0)
        {
            if (num_fields == DB_E_NOTCACHED)
                num_fields = DB_E_UNKNOWNTABLE;
            return num_fields;
        }

        for (i = 0; i < num_fields; i++)
        {
            pbuf = (char *) OID_Point(field_oid_list[i]);
            if (pbuf == NULL)
            {
                return DB_E_OIDPOINTER;
            }

            sc_memcpy(pFieldInfo, pbuf, sizeof(SYSFIELD_T));

            /* compare fieldname case-insenstively */
            if (!mdb_strcmp(pFieldInfo->fieldName, field_name))
            {
                return 1;
            }
        }
    }

    return DB_E_INVALIDPARAM;
}


/*****************************************************************************/
//! _Schema_GetTableIndexesInfo
/*! \breif  function description 1 \n
 *          function description 2 
 ************************************
 * \param   tableName   :
 * \param   schIndexType:
 * \param   pSysIndex   :
 ************************************
 * \return  return value
 ************************************
 * \note    internal algorithm \n
 *          and other notes
 *****************************************************************************/
#if defined(MDB_DEBUG)
static int
compare_indexid(const void *idx1, const void *idx2)
{
    int id1 = ((SYSINDEX_T *) idx1)->indexId;
    int id2 = ((SYSINDEX_T *) idx2)->indexId;

    if (id1 < id2)
        return -1;
    if (id1 > id2)
        return 1;
    return 0;
}
#endif

int
_Schema_GetTableIndexesInfo(char *tableName,
        int schIndexType, SYSINDEX_T * pSysIndex)
{
    int num_temp, num_regu;
    int num_indexes, i;
    SYSINDEX_T *pIndexes[MAX_INDEX_NO];
    OID *index_oid_list;
    char *pbuf = NULL;

    /************************************************
    Complete Index vs. Partial Index 
     - Complete Index: have keys of all records of a table 
     - Partial Index: have keys of some records of a table 
    A temporary index can be complete index or partial index.
    A regular index is always complete index.
    In this processing, partial indexes are excluded.
    **************************************************/

    if (isTempTable_name(tableName) || schIndexType == SCHEMA_TEMP_INDEX)
    {
        num_temp = _Cache_GetTempTableIndexesInfo(tableName, &pIndexes[0]);
        if (num_temp < 0)
        {
            return num_temp;
        }

        num_indexes = 0;
        for (i = 0; i < num_temp; i++)
        {
            if (isPartialIndex_name(pIndexes[i]->indexName))
                continue;
            if (pSysIndex != NULL)
            {
                sc_memcpy(&pSysIndex[num_indexes],
                        pIndexes[i], sizeof(SYSINDEX_T));
            }
            num_indexes += 1;
        }
    }
    else if (schIndexType == SCHEMA_REGU_INDEX)
    {
        num_regu = _Cache_GetTableIndexesInfo(tableName, &index_oid_list);
        if (num_regu < 0)
        {
            return num_regu;
        }

        num_indexes = 0;
        for (i = 0; i < num_regu; i++)
        {
            if (pSysIndex != NULL)
            {
                pbuf = (char *) OID_Point(index_oid_list[i]);
                if (pbuf == NULL)
                {
                    return DB_E_OIDPOINTER;
                }

                sc_memcpy(&pSysIndex[num_indexes], pbuf, sizeof(SYSINDEX_T));
            }
            num_indexes += 1;
        }
#if defined(MDB_DEBUG)
        /* recovery test 시 plan 정보에서 index 선택이 변경 되는 것을
         * 예방하기 위해서 indexid에 대해서 sorting을 함.
         * 꼭 필요한 작업은 아님 */
        if (num_indexes && pSysIndex)
        {
            qsort(pSysIndex, num_indexes, sizeof(SYSINDEX_T), compare_indexid);
        }
#endif
    }
    else    /* schIndexType == SCHEMA_ALL_INDEX : All indexes */
    {
        num_regu = _Cache_GetTableIndexesInfo(tableName, &index_oid_list);
        if (num_regu < 0)
        {
            return num_regu;
        }

        num_temp = _Cache_GetTempTableIndexesInfo(tableName, &pIndexes[0]);
        if (num_temp < 0)
        {
            return num_temp;
        }

        num_indexes = 0;
        for (i = 0; i < num_regu; i++)
        {
            if (pSysIndex != NULL)
            {
                pbuf = (char *) OID_Point(index_oid_list[i]);
                if (pbuf == NULL)
                {
                    return DB_E_OIDPOINTER;
                }

                sc_memcpy(&pSysIndex[num_indexes], pbuf, sizeof(SYSINDEX_T));
            }
            num_indexes += 1;
        }
        for (i = 0; i < num_temp; i++)
        {
            if (isPartialIndex_name(pIndexes[i]->indexName))
                continue;
            if (pSysIndex != NULL)
            {
                sc_memcpy(&pSysIndex[num_indexes],
                        pIndexes[i], sizeof(SYSINDEX_T));
            }
            num_indexes += 1;
        }
#if defined(MDB_DEBUG)
        /* recovery test 시 plan 정보에서 index 선택이 변경 되는 것을
         * 예방하기 위해서 indexid에 대해서 sorting을 함.
         * 꼭 필요한 작업은 아님 */
        if (num_indexes && pSysIndex)
        {
            qsort(pSysIndex, num_indexes, sizeof(SYSINDEX_T), compare_indexid);
        }
#endif
    }

    return num_indexes;
}

/*****************************************************************************/
//! _Schema_GetIndexInfo 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param indexName :
 * \param pSysIndex : 
 * \param pSysIndexField :
 * \param tableName :
  ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
__Schema_GetIndexInfo(char *indexName, SYSINDEX_T * pSysIndex,
        SYSINDEXFIELD_T * pSysIndexField, char *tableName,
        OID table_cont_oid, OID * index_cont_oid)
{
    int ret_val;

#ifdef MDB_DEBUG
    if (pSysIndex == NULL)
        return DB_E_INVALIDPARAM;
#endif

    if (isTempIndex_name(indexName))
    {
        SYSINDEX_T *pIndex;
        SYSINDEXFIELD_T *pIndexFields;

        ret_val = _Cache_GetTempIndexInfo(indexName, &pIndex,
                &pIndexFields, tableName);
        if (ret_val < 0)
        {
            if (ret_val == DB_E_NOTCACHED)
                ret_val = DB_E_UNKNOWNINDEX;
            return ret_val;
        }

        sc_memcpy(pSysIndex, pIndex, sizeof(SYSINDEX_T));
        if (pSysIndexField != NULL)
        {
            sc_memcpy(pSysIndexField, pIndexFields,
                    (sizeof(SYSINDEXFIELD_T) * pIndex->numFields));
        }
    }
    else    /* regular index */
    {
        OID index_oid;
        OID *field_oid_list;
        int i;
        char *pbuf = NULL;

        ret_val = _Cache_GetIndexInfo(indexName, &index_oid,
                &field_oid_list, tableName, table_cont_oid, index_cont_oid);
        if (ret_val < 0)
        {
            if (ret_val == DB_E_NOTCACHED)
                ret_val = DB_E_UNKNOWNINDEX;
            return ret_val;
        }

        pbuf = (char *) OID_Point(index_oid);
        if (pbuf == NULL)
        {
            return DB_E_OIDPOINTER;
        }

        sc_memcpy(pSysIndex, pbuf, sizeof(SYSINDEX_T));

        if (pSysIndexField != NULL)
        {
            for (i = 0; i < pSysIndex->numFields; i++)
            {
                pbuf = (char *) OID_Point(field_oid_list[i]);
                if (pbuf == NULL)
                {
                    return DB_E_OIDPOINTER;
                }

                sc_memcpy(&pSysIndexField[i], pbuf, sizeof(SYSINDEXFIELD_T));
            }
        }
    }

    return DB_SUCCESS;
}

int
_Schema_GetIndexInfo(char *indexName, SYSINDEX_T * pSysIndex,
        SYSINDEXFIELD_T * pSysIndexField, char *tableName)
{
    OID index_cont_oid;

    return __Schema_GetIndexInfo(indexName, pSysIndex,
            pSysIndexField, tableName, NULL_OID, &index_cont_oid);
}

int
_Schema_GetIndexInfo2(char *indexName, SYSINDEX_T * pSysIndex,
        SYSINDEXFIELD_T * pSysIndexField, char *tableName,
        OID table_cont_oid, OID * index_cont_oid)
{
    return __Schema_GetIndexInfo(indexName, pSysIndex,
            pSysIndexField, tableName, table_cont_oid, index_cont_oid);
}

/*****************************************************************************/
//! _Schema_Trans_TempObjectExist 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param transId :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
#ifdef MDB_DEBUG
int
_Schema_Trans_TempObjectExist(int transId)
{
    return _Cache_Trans_TempObjectExist(transId);
}
#endif

/*****************************************************************************/
//! _Schema_Trans_FirstTempTableName 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param transId :
 * \param tableName : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
_Schema_Trans_FirstTempTableName(int transId, char *tableName)
{
    return _Cache_Trans_FirstTempTableName(transId, tableName);
}

/*****************************************************************************/
//! _Schema_Trans_FirstTempIndexName 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param transId :
 * \param tableName : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
_Schema_Trans_FirstTempIndexName(int transId, char *index_name)
{
    return _Cache_Trans_FirstTempIndexName(transId, index_name);
}

static int _Check_align(void);
static int align_longlong = -1;
static int align_double = -1;
static int
_Check_align(void)
{
    struct
    {
        char a;
        double b;
    } st1;
    struct
    {
        char a;
        MDB_INT64 b;
    } st2;

    align_double = (unsigned long) &st1.b - (unsigned long) &st1;
    align_longlong = (unsigned long) &st2.b - (unsigned long) &st2;

    return 0;
}

/*****************************************************************************/
//! _Schema_CheckFieldDesc 
/*! \breif  memory record와 heap record의 size를 계산한다
 ************************************
 * \param numFields :
 * \param pFieldDesc : 
 * \param is_heaprecord : TRUE : HEAP_RECORD, FALSE : MEMORY RECORD
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - heap record : DB Engine에서 저장하는 record format(DB 저장단위)
 *  - memory record : DB API에서 사용하는 record format
 *  
 *  A. heap record format
 *    1. VARIABLE TYPE(VARCHAR/NVARCHAR/VARBYTE)
 *    1.1. not extended
 *      - in heap record
 *      -----------------------------------
 *      |   mvcc  | fixed buffer | length |
 *      -----------------------------------
 *      | 4 bytes |   n bytes    | 1 byte |
 *      -----------------------------------
 *          length는 bytes length이다
 *
 *    1.2. extened
 *      - in heap record
 *      --------------------------------------------------
 *      |   mvcc  | fixed buffer                | length |
 *      --------------------------------------------------
 *      | 4 bytes | OID(4 bytes) |   n bytes    | 1 byte |
 *      --------------------------------------------------
 *          length의 값이 UCHAR_MAX인 경우에 extend 되었다고 판단함
 *          fixed buffer 내부의 앞 4 bytes를 OID로 사용한다
 *          length는 bytes length이다
 *
 *      - in extened part

 *      -----------------------------------------------
 *      |   mvcc  | length  |  real buffer            |
 *      -----------------------------------------------
 *      | 4 bytes |4 bytes  |  n - fixed bytes        |
 *      -----------------------------------------------
 *          length는 bytes length이다
 *
 *
 *    2. BYTE TYPE
 *      ---------------------------------
 *      | length  |  real buffer        |
 *      ---------------------------------
 *      | 4 bytes |     n bytes         |
 *      ---------------------------------
 *
 *  B. memory record format
 *    1. CHAR/NCHAR/VARCHAR/NVARCHAR
 *      -----------------------------
 *      | real buffer   | NULL byte |
 *      -----------------------------
 *      |   n bytes     |  1 bytes  |
 *      -----------------------------
 *
 *    2. BYTE/VARBYTE
 *      ---------------------------------
 *      | length  |  real buffer        |
 *      ---------------------------------
 *      | 4 bytes |     n bytes         |
 *      ---------------------------------
 *
 *  - VARIABLE TYPE 이어도 FIXED_PART가 -1인 경우는..
 *    VARIABLE TYPE이 아닌것 처럼 동작함
 * \note 
 *  - align 시킨후 slot size를 계산하도록 수정
 *  - memory record format 및 heap record format이 수정됨
 *****************************************************************************/
int
_Schema_CheckFieldDesc(int numFields, FIELDDESC_T * pFieldDesc,
        DB_BOOL is_heaprecord)
{
    FIELDDESC_T *pField;
    int i, j;
    extern FIELDDESC_T _hiddenField[];
    extern int _numHiddenFields;
    int slotsize;

    int field_blength;

    if (align_double == -1 || align_longlong == -1)
        _Check_align();

    /* hidden field 이름이 사용되었는지 점검 */
    for (i = 0; i < numFields; i++)
    {
        for (j = 0; j < _numHiddenFields; j++)
        {
            if (!mdb_strcmp(pFieldDesc[i].fieldName,
                            _hiddenField[j].fieldName))
            {   /* 같은 이름의 필드가 선언되었음 */
                return DB_E_KEYFIELDNAME;
            }
        }
    }

    /* 중복된 이름의 field가 있는지 점검 */
    for (i = 0; i < numFields - 1; i++)
    {
        for (j = i + 1; j < numFields; j++)
        {
            if (!mdb_strcmp(pFieldDesc[i].fieldName, pFieldDesc[j].fieldName))
            {   /* 같은 이름의 필드가 선언되었음 */
                return DB_E_DUPFIELDNAME;
            }
        }
    }

    /* field offset 설정 */
    for (i = 0, j = 0; i < numFields; i++)
    {

        pField = pFieldDesc + i;

        /* type 따른 align으로 offset(j) 설정 */
        switch (pField->type_)
        {
        case DT_CHAR:
            /* pFieldDesc->length = 선언된 대로... */
            break;
        case DT_NCHAR:
            j = _alignedlen(j, sizeof(DB_WCHAR));
            break;

        case DT_VARCHAR:
            if (!is_heaprecord) /* not good code */
                pField->length_ += 1;
            break;
        case DT_NVARCHAR:
            if (!is_heaprecord)
                pField->length_ += 1;
            j = _alignedlen(j, sizeof(DB_WCHAR));
            break;
        case DT_BYTE:
        case DT_VARBYTE:
            if (!is_heaprecord) /* not good code */
                pField->length_ += 4;   /* sign, point, null 포함 */
            break;
        case DT_DECIMAL:       /* length는 p */
            if (!is_heaprecord) /* not good code */
                pField->length_ += 3;   /* sign, point, null 포함 */
            /* 내부에서 +3으로 사용하고, 외부에는 선언된 precision 길이만큼
               보여줌. */
            break;
        case DT_TINYINT:       /* align 필요 없음 */
            pField->length_ = 1;
            break;
        case DT_SHORT: /* 2 byte align */
            pField->length_ = 2;
            j += (j % 2);
            break;
        case DT_INTEGER:
            /*case DT_LONG: do not use */
        case DT_FLOAT:
            /* 4 byte align */
            pField->length_ = 4;
            if (j % 4)
                j += (4 - j % 4);
            break;
        case DT_DOUBLE:
            pField->length_ = 8;
            if (j % align_double)
                j += (align_double - j % align_double);
            break;
        case DT_BIGINT:
        case DT_TIMESTAMP:
            pField->length_ = 8;
            if (j % align_longlong)
                j += (align_longlong - j % align_longlong);
            break;
        case DT_DATETIME:
            pField->length_ = 8;
            break;
        case DT_DATE:
        case DT_TIME:
            pField->length_ = 4;
            break;
        case DT_OID:
            pField->length_ = OID_SIZE;
            break;

        default:
            MDB_SYSLOG(("Unknown type: %d\n", pField->type_));
            return DB_E_UNKNOWNTYPE;
        }

        if (is_heaprecord)
        {   // heap record's filed
            pField->h_offset_ = j;
            DB_GET_HFIELD_BLENGTH(field_blength, pField->type_,
                    pField->length_, pField->fixed_part);
            j += field_blength;
        }
        else
        {
            pField->offset_ = j;
            DB_GET_MFIELD_BLENGTH(field_blength, pField->type_,
                    pField->length_);
            j += field_blength;
        }
    }

    /* align_fix */
    j = _alignedlen(j, INT_SIZE);       /* record length */
    if (is_heaprecord)
    {
        slotsize = REC_SLOT_SIZE(REC_ITEM_SIZE(j, numFields));
        if (slotsize > PAGE_BODY_SIZE)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_TOOBIGRECORD, 0);
            return DB_E_TOOBIGRECORD;
        }
    }

    return j;
}

/*****************************************************************************/
//! _Schema_MakeKeyDesc 
/*! \breif  index를 만들때 사용하는 keydesc를 설정
 ************************************
 * \param transId(in)   : transaction id
 * \param tableName(in) : table name
 * \param numFields(in) : field'snumber
 * \param pFieldDesc(in): field description
 * \param key(in/out)   : key 정보
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - called function : dbi_Index_Create()
 *  - BYTE/VARBYTE의 경우 INDEX를 만들수 없음
 *  - COLLATION INTERFACE 추가됨
 *
 *****************************************************************************/
int
_Schema_MakeKeyDesc(int transId, char *tableName,
        int numFields, FIELDDESC_T * pFieldDesc, struct KeyDesc *key)
{
    SYSTABLE_T sysTable;
    SYSFIELD_T *pSysField = NULL;
    int i, j;
    int result = DB_FAIL;
    int fieldcount;

    int ret_val;

    ret_val = _Schema_GetTableInfo(tableName, &sysTable);
    if (ret_val < 0)
    {
        return ret_val;
    }

    pSysField = (SYSFIELD_T *) PMEM_ALLOC(sysTable.numFields
            * sizeof(SYSFIELD_T));
    if (pSysField == (SYSFIELD_T *) NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFMEMORY, 0);
        return DB_E_OUTOFMEMORY;
    }

    fieldcount = _Schema_GetFieldsInfo(tableName, pSysField);
    if (fieldcount != sysTable.numFields)
    {
        MDB_SYSLOG(("Error: Getting fields of table %s information (%d)\n",
                        tableName, fieldcount));

        goto END_PROCESSING;
    }

    for (i = 0; i < numFields; i++)
    {
        /* table에 해당 field가 있는지 점검. 없으면 FAIL 처리 */
        for (j = 0; j < sysTable.numFields; j++)
        {
            if (!mdb_strcmp(pFieldDesc[i].fieldName, pSysField[j].fieldName))
                break;
        }

        if (j >= sysTable.numFields)
        {
            goto END_PROCESSING;
        }

        if (pSysField[j].flag & FD_NOLOGGING)
        {
            result = SQL_E_NOLOGGING_COLUMN_INDEX;
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, result, 0);
            goto END_PROCESSING;
        }

        if (pSysField[j].length > MAX_OF_VARCHAR)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, E_MAXLEN_EXEEDED, 0);
            result = E_MAXLEN_EXEEDED;
            goto END_PROCESSING;
        }
        result = KeyFieldDesc_Create(&(key->field_desc_[i]),
                &pSysField[j],
                ((pFieldDesc[i].flag & FD_DESC) ? (char) 'D' : (char) 'A'),
                &(pFieldDesc[i].collation));
        if (result != DB_SUCCESS)
            goto END_PROCESSING;

        /* 나중에 fieldid를 사용하기 위해서 저장... */
        sc_memcpy(pFieldDesc[i].defaultValue, &pSysField[j].fieldId,
                sizeof(pSysField[j].fieldId));
    }       /* for */
    key->field_count_ = numFields;

    result = DB_SUCCESS;

  END_PROCESSING:
    if (pSysField != NULL)
        PMEM_FREENUL(pSysField);

    return result;
}

int
_Schema_ChangeTableProperty(int transId, char *table_name, ContainerType type)
{
    int ret_val;
    int cursor;
    int real_tableId;
    int use_indexId;
    DB_UINT32 recsize;
    OID real_table_oid;
    OID index_oid;
    SYSTABLE_T *pSysTable;
    SYSINDEX_T *pSysIndex;
    struct KeyValue minkey, maxkey;
    FIELDVALUE_T fieldvalue;
    SYSTABLE_REC_T updTableRec;

    ret_val = _Cache_GetTableInfo(table_name, &real_table_oid, &pSysTable);
    if (ret_val < 0)
    {
        if (ret_val == DB_E_NOTCACHED)
            ret_val = DB_E_UNKNOWNTABLE;
        return ret_val;
    }
    real_tableId = pSysTable->tableId;

    ret_val = _Cache_GetIndexInfo("$idx_systables_tblid",
            &index_oid, NULL, NULL, NULL_OID, NULL);
    if (ret_val < 0)
    {
        return ret_val;
    }
    pSysIndex = (SYSINDEX_T *) OID_Point(index_oid);
    if (pSysIndex == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    use_indexId = pSysIndex->indexId;

    ret_val = LockMgr_Lock("systables", transId, LOWPRIO, LK_EXCLUSIVE, WAIT);
    if (ret_val < 0)
    {
        return ret_val;
    }
    ret_val = dbi_FieldValue(MDB_NN, DT_INTEGER, 0,
            mdb_offsetof(SYSTABLE_T, tableId), sizeof(int), 0,
            MDB_COL_NUMERIC, &real_tableId, DT_INTEGER, sizeof(int),
            &fieldvalue);
    if (ret_val != DB_SUCCESS)
        return ret_val;
    dbi_KeyValue(1, &fieldvalue, &minkey);
    dbi_KeyValue(1, &fieldvalue, &maxkey);

    cursor = Open_Cursor(transId, "systables", LK_EXCLUSIVE,
            (DB_INT16) use_indexId, &minkey, &maxkey, NULL);
    if (cursor < 0)
    {
        return cursor;
    }

    ret_val = __Read(cursor, (char *) &updTableRec, NULL, 0, &recsize, 0);

    if (ret_val < 0)
    {
        Close_Cursor(cursor);
        if (ret_val == DB_E_NOMORERECORDS)
            ret_val = DB_E_UNKNOWNTABLE;
        return ret_val;
    }

    /* update systable record */
    updTableRec.flag = (DB_UINT16) type;
    ret_val = __Update(cursor, (char *) &updTableRec, sizeof(SYSTABLE_REC_T),
            NULL);
    if (ret_val < 0)
    {
        Close_Cursor(cursor);
        return ret_val;
    }

    Close_Cursor(cursor);

    return ret_val;
}

/* table 변경(insert, delete, update) 시 modified_times의 count가 증가 */
int
_Schema_SetTable_modifiedtimes(char *tableName)
{
    return _Cache_SetTable_modifiedtimes(tableName);
}

/* 해당 table 변경의 modified_times return */
int
_Schema_GetTable_modifiedtimes(char *tableName)
{
    return _Cache_GetTable_modifiedtimes(tableName);
}
