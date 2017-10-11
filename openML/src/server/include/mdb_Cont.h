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

#ifndef __CONT_H
#define __CONT_H

#include "mdb_config.h"
#include "mdb_BaseCont.h"
#include "mdb_Collect.h"
#include "mdb_TTree.h"
#include "mdb_LockMgrPI.h"
#include "systable.h"
#include "mdb_Trans.h"

#include "mdb_KeyValue.h"
#include "mdb_Filter.h"

#define CONTAINER_HASH_SIZE         11

/* variable collection count */
#define VAR_COLLECT_CNT             25
#define MIN_VAR_COLLECT_CNT         2

struct Container
{
    struct Collection collect_;
    struct BaseContainer base_cont_;
    struct Collection var_collects_[VAR_COLLECT_CNT];
};

typedef struct
{
    char *idxname;
    struct KeyValue minkey;
    struct KeyValue maxkey;
    struct Filter filter;
} SCANDESC_T;

typedef struct rec_header_struct rec_header_t;
struct rec_header_struct
{
    DB_INT8 used;
    DB_INT8 lock_mode;
    DB_INT8 update_mode;
    DB_INT8 dummy1;
    DB_INT32 transid;
};

/* FREE_SLOT과 USED_SLOT은 slot이 레코드 저장용으로 사용되는지를
   나타내는 bit로 계속 유지가 되어야 함 */
#define FREE_SLOT               0x00    /* permanent - 초기값 */
#define USED_SLOT               0x01    /* permanent */
#define SYNCED_SLOT             0x03
#define UPDATE_SLOT             0x05
#define DELETE_SLOT             0x08

struct Container_Name_Node
{
    char name[CONT_NAME_LENG];
    OID cont_oid;
    struct Container_Name_Node *next;
};

struct Container_Name_Hash
{
    struct Container_Name_Node *hashptr[CONTAINER_HASH_SIZE];
};
extern __DECL_PREFIX struct Container_Name_Hash *hash_cont_name;

struct Container_Tableid_Node
{
    int tableid;
    OID cont_oid;
    struct Container_Tableid_Node *next;
};

struct Container_Tableid_Hash
{
    struct Container_Tableid_Node *hashptr[CONTAINER_HASH_SIZE];
};
extern __DECL_PREFIX struct Container_Tableid_Hash *hash_cont_tableid;

#define CONT_ITEM_SIZE  sizeof(struct Container)

#define REC_ITEM_SIZE(recsize, numfields)                           \
    (recsize + GET_HIDDENFIELDLENGTH(-1, numfields, recsize))

/* qsid(2byte)와 slot flag(1byte) 용으로 4바이트 추가하고 
   sizeof(long)바이트 align 맞춤 */
#define REC_SLOT_SIZE(itemsize)                                     \
    _alignedlen(itemsize + 4, sizeof(long))

#define REC_SIZE(l, n) (l + _alignedlen(((n-1)>>3)+1, sizeof(long))+4)

struct Container *Cont_Search(const char *cont_name);
struct Container *Cont_Search_Tableid(int tableid);
struct Container *Cont_Search_OID(OID oid);

OID Col_GetFirstDataID(struct Container *Cont, DB_UINT8 msync_flag);
OID Col_GetFirstDataID_Page(struct Container *Cont, OID page,
        DB_UINT8 msync_flag);
DB_INT32 Col_get_collect_index(OID recordid);
int Col_Insert(struct Container *Cont, int collect_index, char *item,
        DB_UINT32 opentime, DB_UINT16 qsid, OID * record_id, int val_len,
        DB_UINT8 msync_flag);

int Col_check_valid_rid(char *tablename, OID rid);

int Col_Insert_Variable(struct Container *Cont, char *value, DB_INT32 val_len,
        OID * record_id);
int Col_Read(DB_INT32 item_size, OID record_oid, char *item);

struct Update_Idx_Id_Array
{
    int cnt;
    OID idx_oid[MAX_INDEX_NO];
};

int Col_Update_Record(struct Container *Cont, OID record_oid, char *item,
        DB_UINT32 open_time, DB_UINT16 qsid,
        struct Update_Idx_Id_Array *upd_idx_id_array, DB_UINT8 msync_flag);
int Col_Update_Field(struct Container *Cont, SYSFIELD_T * fields_info,
        struct UpdateValue *newValues, OID record_id, char *item,
        DB_UINT32 open_time, DB_UINT16 qsid,
        struct Update_Idx_Id_Array *upd_idx_id_array);

int Col_Remove(struct Container *Cont, int collect_index, OID record_oid,
        int remove_msync);
int Col_Check_Exist(struct Container *Cont,
        SYSTABLE_T * table_info, SYSFIELD_T * fields_info, char *newrecord);

__DECL_PREFIX int
Create_Table(char *Cont_name,
        DB_UINT32 memory_record_size, DB_UINT32 heap_record_size,
        int numFields, ContainerType type, int tableid, int creator,
        int max_records, FIELDDESC_T * pFieldDesc);

int Drop_Table(char *Cont_name);
int Truncate_Table(char *Cont_name);
int Create_Index(char *indexName, int indexNo, int indexType, char *cont_name,
        struct KeyDesc *keydesc, int keyins_flag, SCANDESC_T * scandesc);
int Drop_Index(char *Cont_name, DB_INT16 index_no);
int Rename_Index(char *Cont_name, DB_INT16 index_no, char *new_name);

int Cont_CreateHashing(void);
int Cont_FreeHashing(void);
int Cont_InputHashing(const char *cont_name, const int tableId,
        const OID cont_oid);
int Cont_RemoveHashing(const char *cont_name, const int tableid);
int Cont_NewTableID(int userid);
int Cont_NewTempTableID(int userid);
int Cont_NewFieldID(void);
int Cont_CreateCont(char *cont_name, DB_UINT32 memory_record_size,
        DB_UINT32 heap_record_size, ContainerID container_id, int numFields,
        ContainerType type, int tableId, int creator, int max_records,
        FIELDDESC_T * pFieldDesc);
int Cont_Rename(struct Container *Cont, char *new_cont_name);
extern int Cont_ChangeProperty(struct Container *cont, ContainerType cont_type,
        int duringRuntime);

int Collect_AllocateSlot(struct Container *Cont, struct Collection *collection,
        OID * new_slot_id, int find_only_flag);

int Collect_reconstruct_freepagelist(struct Collection *collect);

int Cont_RemoveTempTables(int connid);
int Cont_RemoveTransTempObjects(int connid, int transid);

struct Container *Cont_get(OID recordid);

int Cont_ResidentTable_Init(struct Container *cont);

#endif
