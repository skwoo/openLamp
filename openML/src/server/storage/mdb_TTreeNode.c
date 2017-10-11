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

#include "mdb_Mem_Mgr.h"
#include "mdb_TTreeNode.h"
#include "mdb_TTree.h"
#include "mdb_PMEM.h"

extern int Index_OID_HeavyWrite(OID curr, char *data, DB_INT32 size);
extern int KeyDesc_Compare2(struct TTree *ttree,
        const char *r1, const char *r2, int *isnull1);

int
TTreeNode_Type_Check(struct TTreeNode *node)
{
    struct TTreeNode *parent;

    if (node == NULL)
        return 0;

    if (node->type_ == ROOT_NODE)
    {
        if (node->parent_ == NULL_OID)
            return 0;   /* OK */

#ifdef MDB_DEBUG
        sc_assert(0, __FILE__, __LINE__);
#endif
        return 1;
    }
    else    /* non-root, it should have parent */
    {
        parent = (struct TTreeNode *) Index_OID_Point(node->parent_);
        if (parent == NULL)
        {
#ifdef MDB_DEBUG
            sc_assert(0, __FILE__, __LINE__);
#endif
            return 1;
        }

        if (node->type_ == LEFT_CHILD)
        {
            if (parent->left_ == node->self_ && node->parent_ == parent->self_)
                return 0;       /* OK */

#ifdef MDB_DEBUG
            sc_assert(0, __FILE__, __LINE__);
#endif
            return 1;
        }
        else
        {
            if (node->type_ != RIGHT_CHILD)
            {
#ifdef MDB_DEBUG
                sc_assert(0, __FILE__, __LINE__);
#endif
                return 1;
            }

            if (parent->right_ == node->self_ &&
                    node->parent_ == parent->self_)
                return 0;       /* OK */
        }
    }

#ifdef MDB_DEBUG
    sc_assert(0, __FILE__, __LINE__);
#endif
    return 1;
}

int
TTreeNode_Create(OID self_id, OID parent_id, NodeType node_type,
        OID record_id, struct TTree *ttree)
{
    struct TTreeNode *node;
    int ret;

    node = (struct TTreeNode *) PMEM_ALLOC(sizeof(struct TTreeNode));
    if (node == NULL)
        return DB_E_OUTOFMEMORY;

    node->self_ = self_id;
    node->parent_ = parent_id;
    node->left_ = NULL_OID;
    node->right_ = NULL_OID;
    node->height_ = 0;
    node->type_ = node_type;

    node->item_count_ = 1;
    node->item_[0] = record_id;

    ret = Index_OID_HeavyWrite(self_id, (char *) node,
            sizeof(struct TTreeNode));

    PMEM_FREE(node);

    return ret;
}

int
TTreeNode_InsertKey(struct TTreeNode *node, DB_INT16 index, OID rec_id,
        struct TTree *ttree)
{
    if (index < node->item_count_)
    {
        sc_memmove(node->item_ + index + 1, node->item_ + index,
                OID_SIZE * (node->item_count_ - index));
    }
    node->item_[index] = rec_id;
    node->item_count_ += 1;

    Index_SetDirtyFlag(node->self_);
#ifdef MDB_DEBUG
    TTreeNode_Type_Check(node);
#endif
    return DB_SUCCESS;
}

int
TTreeNode_InsertKey_DeleteFirst(struct TTreeNode *node, DB_INT16 index,
        OID rec_id, struct TTree *ttree)
{
#ifdef MDB_DEBUG
    sc_assert(index >= 0, __FILE__, __LINE__);
#endif

    if (index > 0)
    {
        sc_memmove(node->item_, node->item_ + 1, OID_SIZE * index);
    }
    node->item_[index] = rec_id;

    Index_SetDirtyFlag(node->self_);
#ifdef MDB_DEBUG
    TTreeNode_Type_Check(node);
#endif
    return DB_SUCCESS;
}

int
TTreeNode_DeleteKey(struct TTreeNode *node, DB_INT16 index,
        struct TTree *ttree)
{
    if (index >= node->item_count_)
        return DB_FAIL;

    sc_memmove(node->item_ + index, node->item_ + index + 1,
            OID_SIZE * (node->item_count_ - 1 - index));
    node->item_[node->item_count_ - 1] = NULL_OID;

    node->item_count_ -= 1;

    Index_SetDirtyFlag(node->self_);
#ifdef MDB_DEBUG
    TTreeNode_Type_Check(node);
#endif

    return DB_SUCCESS;
}

void
TTreeNode_MoveLeftToRight(struct TTreeNode *L, struct TTreeNode *R,
        int move_count, struct TTree *ttree)
{
    /* right node */
    sc_memmove(R->item_ + move_count, R->item_, (OID_SIZE * R->item_count_));
    sc_memmove(R->item_, L->item_ + (L->item_count_ - move_count),
            (OID_SIZE * move_count));

    R->item_count_ += move_count;

    /* left node */
    L->item_count_ -= move_count;

    if (L->item_count_ > 0)
    {
        sc_memset(L->item_ + L->item_count_, 0,
                (OID_SIZE * (TNODE_MAX_ITEM - L->item_count_)));
    }

    Index_SetDirtyFlag(R->self_);
    Index_SetDirtyFlag(L->self_);
}

void
TTreeNode_MoveRightToLeft(struct TTreeNode *L, struct TTreeNode *R,
        int move_count, struct TTree *ttree)
{
    /* left node */
    sc_memmove(L->item_ + L->item_count_, R->item_, OID_SIZE * move_count);

    L->item_count_ += move_count;

    /* right node */
    R->item_count_ -= move_count;

    if (R->item_count_ > 0)
    {
        sc_memmove(R->item_, R->item_ + move_count, OID_SIZE * R->item_count_);
        sc_memset(R->item_ + R->item_count_, 0,
                OID_SIZE * (TNODE_MAX_ITEM - R->item_count_));
    }

    Index_SetDirtyFlag(L->self_);
    Index_SetDirtyFlag(R->self_);
}

int
TTreeNode_CompKeyValWithMinimum(struct KeyValue *keyval,
        struct TTreeNode *node, struct TTree *ttree)
{
    int ret;
    int issys;
    char *pbuf = NULL;

    SetBufSegment(issys, node->item_[0]);

    pbuf = (char *) OID_Point(node->item_[0]);
    if (pbuf == NULL)
    {
        UnsetBufSegment(issys);
        return DB_E_OIDPOINTER;
    }

    ret = KeyValue_Compare(keyval, pbuf, ttree->collect_.record_size_);

    UnsetBufSegment(issys);

    return ret;
}

int
TTreeNode_CompKeyValWithMaximum(struct KeyValue *keyval,
        struct TTreeNode *node, struct TTree *ttree)
{
    int ret;
    int issys;
    char *pbuf = NULL;

    SetBufSegment(issys, node->item_[node->item_count_ - 1]);
    pbuf = (char *) OID_Point(node->item_[node->item_count_ - 1]);
    if (pbuf == NULL)
    {
        UnsetBufSegment(issys);
        return DB_E_OIDPOINTER;
    }

    ret = KeyValue_Compare(keyval, pbuf, ttree->collect_.record_size_);
    UnsetBufSegment(issys);

    return ret;
}

int
TTreeNode_CompKeyValWithItem(struct KeyValue *keyval,
        struct TTreeNode *node, int item_index, struct TTree *ttree)
{
    int ret;
    int issys;
    char *pbuf = NULL;

    pbuf = (char *) OID_Point(node->item_[item_index]);
    if (pbuf == NULL)
        return DB_E_OIDPOINTER;

    SetBufSegment(issys, node->item_[item_index]);

    ret = KeyValue_Compare(keyval, pbuf, ttree->collect_.record_size_);

    UnsetBufSegment(issys);

    return ret;
}

/*****************************************************************************/
//! TTreeNode_CompRecordWithMinimum
/*! \breif  
 ************************************
 * \param record():
 * \param isnull(): 
 * \param node(): 
 * \param ttree():
 ************************************
 * \return  return_value
 ************************************
 *  - variable field format
 *      reference heap format : make_heap_record()
 *
 * \note 
 *  - varchar와 nvarchar의 경우 val_len의 값을 heap record에서 직접 가져옴
 *
 *****************************************************************************/
int
TTreeNode_CompRecordWithMinimum(char *record, int *isnull,
        struct TTreeNode *node, struct TTree *ttree)
{
    int ret;
    int issys;
    char *pbuf = NULL;

    SetBufSegment(issys, node->item_[0]);
    pbuf = (char *) OID_Point(node->item_[0]);
    if (pbuf == NULL)
    {
        UnsetBufSegment(issys);
        return DB_E_OIDPOINTER;
    }

    ret = KeyDesc_Compare2(ttree, record, pbuf, isnull);

    UnsetBufSegment(issys);

    return ret;
}

/*****************************************************************************/
//! TTreeNode_CompRecordWithMaximum
/*! \breif  memory record와 heap record의 size를 계산한다
 ************************************
 * \param record():
 * \param isnull(): 
 * \param node(): 
 * \param ttree():
 ************************************
 * \return  return_value
 ************************************
 *  - variable field format
 *      reference heap record format : _Schema_CheckFieldDesc()
 *
 * \note 
 *  - heap record에서 val_len을 직접 가져온다
 *****************************************************************************/
int
TTreeNode_CompRecordWithMaximum(char *record, int *isnull,
        struct TTreeNode *node, struct TTree *ttree)
{
    int ret;
    int issys;
    char *pbuf = NULL;

    SetBufSegment(issys, node->item_[node->item_count_ - 1]);

    pbuf = (char *) OID_Point(node->item_[node->item_count_ - 1]);
    if (pbuf == NULL)
    {
        UnsetBufSegment(issys);
        return DB_E_OIDPOINTER;
    }

    ret = KeyDesc_Compare2(ttree, record, pbuf, isnull);

    UnsetBufSegment(issys);

    return ret;
}

int
TTreeNode_CompRecordWithItem(char *record, int *isnull,
        struct TTreeNode *node, int item_index, struct TTree *ttree)
{
    int ret;
    int issys;
    char *item_record;
    extern struct Container *Log_Cont;
    extern int fRecovered;

    item_record = OID_Point(node->item_[item_index]);
    if (item_record == NULL)
        return DB_E_OIDPOINTER;

    if (server->status == SERVER_STATUS_RECOVERY && Log_Cont)
    {
        if ((*(record + Log_Cont->collect_.slot_size_ - 1) & USED_SLOT) == 0)
        {
            fRecovered |= ALL_REBUILD;
            return DB_E_REMOVEDRECORD;
        }
    }

    SetBufSegment(issys, node->item_[item_index]);

    ret = KeyDesc_Compare2(ttree, record, item_record, isnull);

    UnsetBufSegment(issys);

    return ret;
}

/////////////////////////////////////////////////////////////////
//
// Function Name :
//
/////////////////////////////////////////////////////////////////
/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
struct TTreeNode *
TTree_LubChild(struct TTreeNode *node)
{
    register struct TTreeNode *tree_node;

    if (node->right_ == NULL_OID)
        return NULL;

    tree_node = (struct TTreeNode *) Index_OID_Point(node->right_);

    if (tree_node == NULL)
        return NULL;

    while (tree_node->left_ != NULL_OID)
    {
        tree_node = (struct TTreeNode *) Index_OID_Point(tree_node->left_);
        if (tree_node == NULL)
            return NULL;
    }

    return tree_node;
}

/////////////////////////////////////////////////////////////////
//
// Function Name :
//
/////////////////////////////////////////////////////////////////
/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
struct TTreeNode *
TTree_GlbChild(struct TTreeNode *node)
{
    register struct TTreeNode *tree_node;

    if (node->left_ == NULL_OID)
        return NULL;

    tree_node = (struct TTreeNode *) Index_OID_Point(node->left_);
    if (tree_node == NULL)
    {
        return NULL;
    }

    while (tree_node->right_ != NULL_OID)
        tree_node = (struct TTreeNode *) Index_OID_Point(tree_node->right_);

    return tree_node;
}

/////////////////////////////////////////////////////////////////
//
// Function Name :
//
/////////////////////////////////////////////////////////////////
/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
struct TTreeNode *
TTree_SmallestChild(struct TTree *ttree)
{
    int node_count = ttree->collect_.item_count_;
    int count;
    struct TTreeNode *node;

    node = (struct TTreeNode *) Index_OID_Point(ttree->root_[0]);

    if (node == NULL)
    {
        return NULL;
    }

    if (node->left_ == NULL_OID)
        return node;

    count = 0;
    while (node->left_ != NULL_OID)
    {
        node = (struct TTreeNode *) Index_OID_Point(node->left_);
        if (node == NULL)
        {
            return NULL;
        }

        if (++count > node_count + 1)
        {
            return NULL;
        }
    }

    return node;
}

/////////////////////////////////////////////////////////////////
//
// Function Name :
//
/////////////////////////////////////////////////////////////////
/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
struct TTreeNode *
TTree_GreatestChild(struct TTree *ttree)
{
    struct TTreeNode *node;

    node = (struct TTreeNode *) Index_OID_Point(ttree->root_[0]);

    if (node == NULL)
    {
        return NULL;
    }

    if (node->right_ == NULL_OID)
        return node;

    while (node->right_ != NULL_OID)
    {
        node = (struct TTreeNode *) Index_OID_Point(node->right_);
        if (node == NULL)
        {
            return NULL;
        }
    }

    return node;
}

/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
struct TTreeNode *
TTree_NextNode(struct TTreeNode *node)
{
    if (node->right_ != NULL_OID)
        return (struct TTreeNode *) TTree_LubChild(node);

    switch (node->type_)
    {
    case LEFT_CHILD:
        return (struct TTreeNode *) Index_OID_Point(node->parent_);

    case RIGHT_CHILD:
        {
            struct TTreeNode *tree_node;

            if (node->type_ == ROOT_NODE || node->type_ == LEFT_CHILD)
                return NULL;

            tree_node = (struct TTreeNode *) Index_OID_Point(node->parent_);

            if (tree_node == NULL)
                return NULL;

            while (tree_node->type_ == RIGHT_CHILD)
            {
                tree_node = (struct TTreeNode *)
                        Index_OID_Point(tree_node->parent_);
                if (tree_node == NULL)
                    return NULL;
            }

            if (tree_node->type_ == LEFT_CHILD)
            {
                return (struct TTreeNode *)
                        Index_OID_Point(tree_node->parent_);
            }
            else
                return NULL;
        }
    default:
        return NULL;
    }
}

/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
struct TTreeNode *
TTree_PrevNode(struct TTreeNode *node)
{
    if (node->left_ != NULL_OID)
        return TTree_GlbChild(node);

    switch (node->type_)
    {
    case RIGHT_CHILD:
        return (struct TTreeNode *) Index_OID_Point(node->parent_);
    case LEFT_CHILD:
        {
            struct TTreeNode *tree_node;

            if (node->type_ == ROOT_NODE || node->type_ == RIGHT_CHILD)
                return NULL;

            tree_node = (struct TTreeNode *) Index_OID_Point(node->parent_);
            if (tree_node == NULL)
            {
                return NULL;
            }

            while (tree_node->type_ == LEFT_CHILD)
                tree_node =
                        (struct TTreeNode *) Index_OID_Point(tree_node->
                        parent_);

            if (tree_node->type_ == RIGHT_CHILD)
                return (struct TTreeNode *) Index_OID_Point(tree_node->
                        parent_);
            else
                return NULL;
        }
    default:
        return NULL;
    }
}
