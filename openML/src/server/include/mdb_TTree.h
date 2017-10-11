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

#ifndef __TTREE_H
#define __TTREE_H

#include "mdb_config.h"
#include "mdb_TTreeNode.h"
#include "mdb_Collect.h"
#include "mdb_BaseCont.h"
#include "mdb_KeyValue.h"
#include "mdb_KeyDesc.h"

#define NUM_TTREE_BUCKET    11

struct TTreePosition
{
#ifdef INDEX_BUFFERING
    OID self_;
#endif
    struct TTreeNode *node_;
    DB_INT16 idx_;
};

struct TTree
{
    struct Collection collect_;
    struct BaseContainer base_cont_;
    struct KeyDesc key_desc_;
    DB_UINT32 modified_time_;
    NodeID root_[NUM_TTREE_BUCKET];
};

#define INDEX_ITEM_SIZE sizeof(struct TTree)

extern int TTreeNode_Create(OID self_id, OID parent_id, NodeType node_type,
        OID record_id, struct TTree *ttree);
extern int TTreeNode_InsertKey(struct TTreeNode *node, DB_INT16 index,
        OID rec_id, struct TTree *ttree);
extern int TTreeNode_InsertKey_DeleteFirst(struct TTreeNode *node,
        DB_INT16 index, OID rec_id, struct TTree *ttree);
extern int TTreeNode_DeleteKey(struct TTreeNode *node, DB_INT16 index,
        struct TTree *ttree);
extern void TTreeNode_MoveLeftToRight(struct TTreeNode *L, struct TTreeNode *R,
        int move_count, struct TTree *ttree);
extern void TTreeNode_MoveRightToLeft(struct TTreeNode *L, struct TTreeNode *R,
        int move_count, struct TTree *ttree);
extern int TTreeNode_CompKeyValWithMinimum(struct KeyValue *keyval,
        struct TTreeNode *node, struct TTree *ttree);
extern int TTreeNode_CompKeyValWithMaximum(struct KeyValue *keyval,
        struct TTreeNode *node, struct TTree *ttree);
extern int TTreeNode_CompRecordWithMinimum(char *record, int *isnull,
        struct TTreeNode *node, struct TTree *ttree);
extern int TTreeNode_CompRecordWithMaximum(char *record, int *isnull,
        struct TTreeNode *node, struct TTree *ttree);
extern int TTreeNode_CompKeyValWithItem(struct KeyValue *keyval,
        struct TTreeNode *node, int item_index, struct TTree *ttree);
extern int TTreeNode_CompRecordWithItem(char *record, int *isnull,
        struct TTreeNode *node, int item_index, struct TTree *ttree);

int TTree_Record_Hash(struct TTree *ttree, char *record);

int TTree_InsertRecord(struct TTree *ttree, char *new_record, OID insert_id,
        int unique_check_flag);
int TTree_RemoveRecord(struct TTree *ttree, const char *record,
        RecordID record_id);
int TTree_FindRecord(struct TTree *ttree, char *record, RecordID record_id,
        int *found);

int TTree_Rebalance_For_Insert(struct TTree *ttree, struct TTreeNode *node,
        int bucketnum);
int TTree_Rebalance_For_Delete(struct TTree *ttree, struct TTreeNode *node,
        int bucketnum);

int TTree_Merge_With_Left_Child(struct TTree *ttree,
        struct TTreeNode *curr_node, int bucketnum);
int TTree_Merge_With_Right_Child(struct TTree *ttree,
        struct TTreeNode *curr_node, int bucketnum);

int TTree_MakeLeftChild(struct TTree *ttree, struct TTreeNode *node,
        OID insert_oid, int bucketnum);
int TTree_MakeRightChild(struct TTree *ttree, struct TTreeNode *node,
        OID insert_oid, int bucketnum);

int TTree_LL_Rotate(struct TTree *ttree, struct TTreeNode *N1, int bucketnum);
int TTree_LR_Rotate(struct TTree *ttree, struct TTreeNode *N1, int bucketnum);
int TTree_RR_Rotate(struct TTree *ttree, struct TTreeNode *N1, int bucketnum);
int TTree_RL_Rotate(struct TTree *ttree, struct TTreeNode *N1, int bucketnum);

int TTree_Remove(struct TTree *ttree, struct TTreeNode *node,
        DB_INT16 index_in_node, int bucketnum);

int TTree_FindPositionWithValueAndID(struct TTree *ttree, const char *record,
        RecordID record_id, struct TTreePosition *p);

int TTree_Get_First_Eq(struct TTree *ttree, struct KeyValue *key,
        struct TTreePosition *p);
int TTree_Get_Last_Eq(struct TTree *ttree, struct KeyValue *key,
        struct TTreePosition *p);
int TTree_Get_First_Ge(struct TTree *ttree, struct KeyValue *key,
        struct TTreePosition *first);
int TTree_Get_Last_Le(struct TTree *ttree, struct KeyValue *key,
        struct TTreePosition *last);
int TTree_Get_First_Eq_Unique(struct TTree *ttree, struct KeyValue *key,
        struct TTreePosition *p);

int TTree_Increment(struct TTreePosition *p);
int TTree_Decrement(struct TTreePosition *p);

int TTree_IsExist(struct TTree *ttree, char *record_to_find,
        OID record_id, int record_size, int f_memory_record);
int Index_MemMgr_FreeAllNodes(struct TTree *ttree);
int TTree_Get_Offset(struct TTree *ttree, struct TTreePosition *p);
int TTree_Seek_Offset(struct TTree *ttree, struct TTreePosition *p,
        int offset);

struct TTreeNode *TTree_SmallestChild(struct TTree *ttree);
struct TTreeNode *TTree_GreatestChild(struct TTree *ttree);
int TTree_Find_Next_Without_Filter(struct TTreePosition *curr,
        struct TTreePosition *last);
int TTree_Find_Prev_Without_Filter(struct TTreePosition *curr,
        struct TTreePosition *first);
int TTree_Get_Position(struct TTree *ttree, OID rid, char type, int rec_size,
        struct TTreePosition *curr);
#endif

int TTree_Get_ItemCount_Interval(struct TTree *ttree,
        struct TTreePosition *first, struct TTreePosition *last, int type);
