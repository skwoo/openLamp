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
#include "mdb_TTree.h"
#include "mdb_TTreeNode.h"
#include "mdb_Mem_Mgr.h"
#include "mdb_PMEM.h"

#include "mdb_syslog.h"

#define __MAX(a,b) (((a) > (b)) ? (a) : (b))

#define CHECK_RANGE     20

struct TTreeNode *TTree_GlbChild(const struct TTreeNode *);

int Index_Collect_AllocateSlot(struct Collection *, OID *);

#ifdef MDB_DEBUG
int TTreeNode_Type_Check(struct TTreeNode *node);
#endif

int
TTree_InsertRecord(struct TTree *ttree, char *new_record, OID insert_id,
        int unique_check_flag)
{
    struct TTreeNode *p_node;
    OID new_insert_value;
    int bucketnum = 0;          //TTree_Record_Hash(ttree, new_record);
    DB_INT16 n;
    int i;
    DB_INT32 isnull1[MAX_KEY_FIELD_NO];
    int null_count = 0;
    int need_uniqueness_checking = 0;
    int comp_result;
    int issys;
    int ret;

#ifdef INDEX_BUFFERING
    int issys_inode = 0;
    OID oid_node = NULL_OID;

#ifdef MDB_DEBUG
    OID oid_node1 = NULL_OID;
#endif
#endif

    SetBufSegment(issys, insert_id);

    for (i = 0; i < (ttree->key_desc_.field_count_); i++)
    {
        isnull1[i] = DB_VALUE_ISNULL((char *) new_record,
                ttree->key_desc_.field_desc_[i].position_,
                ttree->collect_.record_size_);
        if (isnull1[i])
        {
            null_count++;
        }
    }

    /* primary key의 경우 null이 존재하는 경우 error 처리 */
    if (ttree->key_desc_.is_unique_ == TRUE && unique_check_flag)
    {
        if (ttree->key_desc_.field_desc_[0].flag_ & FD_KEY)
        {
            if (null_count)
            {
                ret = DB_E_NOTNULLFIELD;
                goto end;
            }

            need_uniqueness_checking = 1;
        }
        else if (null_count == 0)
        {
            need_uniqueness_checking = 1;
        }
    }

    if (ttree->root_[bucketnum] == NULL_OID)
    {
        ret = Index_Collect_AllocateSlot(&ttree->collect_,
                &ttree->root_[bucketnum]);
        if (ret < 0)
        {
            goto end;
        }

        ttree->collect_.item_count_ += 1;

        SetDirtyFlag(ttree->collect_.cont_id_);

        ret = TTreeNode_Create(ttree->root_[bucketnum], NULL_OID, ROOT_NODE,
                insert_id, ttree);
        goto end;
    }


#ifdef INDEX_BUFFERING
    oid_node = ttree->root_[bucketnum];
    SetBufIndexSegment(issys_inode, oid_node);
#ifdef MDB_DEBUG
    if (issys_inode)
        oid_node1 = oid_node;
#endif
#endif
    p_node = (struct TTreeNode *) Index_OID_Point(ttree->root_[bucketnum]);
    if (p_node == NULL)
    {
        ret = DB_E_OIDPOINTER;
        goto end;
    }

    for (;;)
    {
#ifdef INDEX_BUFFERING
#ifdef MDB_DEBUG
        sc_assert(p_node->self_ == oid_node, __FILE__, __LINE__);
#endif
#endif
        comp_result = TTreeNode_CompRecordWithMinimum(new_record, isnull1,
                p_node, ttree);
#ifdef INDEX_BUFFERING
#ifdef MDB_DEBUG
        sc_assert(p_node->self_ == oid_node, __FILE__, __LINE__);
#endif
#endif
        if (comp_result < 0)
        {
            if (p_node->left_ != NULL_OID)
            {
#ifdef INDEX_BUFFERING
                UnsetBufIndexSegment(issys_inode);
                oid_node = p_node->left_;
                SetBufIndexSegment(issys_inode, oid_node);
#ifdef MDB_DEBUG
                if (issys_inode)
                    oid_node1 = oid_node;
#endif
#endif
                p_node = (struct TTreeNode *) Index_OID_Point(p_node->left_);
                if (p_node == NULL)
                {
                    ret = DB_E_OIDPOINTER;
                    goto end;
                }

#ifdef INDEX_BUFFERING
#ifdef MDB_DEBUG
                sc_assert(p_node->self_ == oid_node, __FILE__, __LINE__);
#endif
#endif

                continue;
            }

            if (p_node->item_count_ < TNODE_MAX_ITEM)
            {
                ret = TTreeNode_InsertKey(p_node, 0, insert_id, ttree);
            }
            else
            {
                ret = TTree_MakeLeftChild(ttree, p_node, insert_id, bucketnum);
            }

            goto end;
        }
#ifdef INDEX_BUFFERING
#ifdef MDB_DEBUG
        sc_assert(p_node->self_ == oid_node, __FILE__, __LINE__);
#endif
#endif

        comp_result = TTreeNode_CompRecordWithMaximum(new_record, isnull1,
                p_node, ttree);
#ifdef INDEX_BUFFERING
#ifdef MDB_DEBUG
        sc_assert(p_node->self_ == oid_node, __FILE__, __LINE__);
#endif
#endif
        if (comp_result > 0)
        {
            if (p_node->right_ != NULL_OID)
            {
#ifdef INDEX_BUFFERING
                UnsetBufIndexSegment(issys_inode);
                oid_node = p_node->right_;
                SetBufIndexSegment(issys_inode, oid_node);
#ifdef MDB_DEBUG
                if (issys_inode)
                    oid_node1 = oid_node;
#endif
#endif
                p_node = (struct TTreeNode *) Index_OID_Point(p_node->right_);
                if (p_node == NULL)
                {
                    ret = DB_E_OIDPOINTER;
                    goto end;
                }

#ifdef INDEX_BUFFERING
#ifdef MDB_DEBUG
                sc_assert(p_node->self_ == oid_node, __FILE__, __LINE__);
#endif
#endif
                continue;
            }

            if (p_node->item_count_ < TNODE_MAX_ITEM)
            {
                ret = TTreeNode_InsertKey(p_node, p_node->item_count_,
                        insert_id, ttree);
            }
            else
            {
                ret = TTree_MakeRightChild(ttree, p_node, insert_id,
                        bucketnum);
            }

            goto end;
        }
#ifdef INDEX_BUFFERING
#ifdef MDB_DEBUG
        sc_assert(p_node->self_ == oid_node, __FILE__, __LINE__);
#endif
#endif

        n = p_node->item_count_ - 1;
#ifdef INDEX_BUFFERING
#ifdef MDB_DEBUG
        sc_assert(p_node->self_ == oid_node, __FILE__, __LINE__);
#endif
#endif

        while (1)
        {
            if (n - (CHECK_RANGE - 1) < 0)
                break;

            if (TTreeNode_CompRecordWithItem(new_record, isnull1,
                            p_node, (n - (CHECK_RANGE - 1)), ttree) >= 0)
                break;
#ifdef INDEX_BUFFERING
#ifdef MDB_DEBUG
            sc_assert(p_node->self_ == oid_node, __FILE__, __LINE__);
#endif
#endif

            n -= CHECK_RANGE;
        }

        for (; n >= 0; n--)
        {
            comp_result = TTreeNode_CompRecordWithItem(new_record, isnull1,
                    p_node, n, ttree);
#ifdef INDEX_BUFFERING
#ifdef MDB_DEBUG
            sc_assert(p_node->self_ == oid_node, __FILE__, __LINE__);
#endif
#endif
            if (comp_result == 0)
            {
                if (need_uniqueness_checking)
                {
                    ret = DB_E_DUPUNIQUEKEY;
                    goto end;
                }
                break;
            }
            else
            {
                if (comp_result > 0)
                    break;
            }
        }

        ttree->modified_time_++;

        if (p_node->item_count_ < TNODE_MAX_ITEM)
        {
            ret = TTreeNode_InsertKey(p_node, (DB_INT16) (n + 1), insert_id,
                    ttree);
            goto end;
        }

        /* p_node->item_count_ == TNODE_MAX_ITEM */
        new_insert_value = p_node->item_[0];
        if (n < 0)
            n = 0;
        (void) TTreeNode_InsertKey_DeleteFirst(p_node, n, insert_id, ttree);

        if (p_node->left_ == NULL_OID)
        {
            ret = TTree_MakeLeftChild(ttree, p_node, new_insert_value,
                    bucketnum);
            goto end;
        }
        else
        {
            p_node = (struct TTreeNode *) TTree_GlbChild(p_node);
#ifdef INDEX_BUFFERING
            {
                UnsetBufIndexSegment(issys_inode);
                oid_node = p_node->self_;
                SetBufIndexSegment(issys_inode, oid_node);
#ifdef MDB_DEBUG
                if (issys_inode)
                    oid_node1 = oid_node;
#endif
            }
#ifdef MDB_DEBUG
            sc_assert(p_node->self_ == oid_node, __FILE__, __LINE__);
#endif
#endif
            if (p_node->item_count_ < TNODE_MAX_ITEM)
            {
                ret = TTreeNode_InsertKey(p_node, p_node->item_count_,
                        new_insert_value, ttree);
                goto end;
            }
            else
            {
                ret = TTree_MakeRightChild(ttree, p_node, new_insert_value,
                        bucketnum);
                goto end;
            }
        }
    }

  end:
    UnsetBufSegment(issys);
#ifdef INDEX_BUFFERING
    UnsetBufIndexSegment(issys_inode);
#ifdef MDB_DEBUG
    if (oid_node1)
    {
        if (isBufIndexSegment(Index_getSegmentNo(oid_node1)))
        {
            sc_assert(0, __FILE__, __LINE__);
        }
    }
#endif
#endif

#ifdef CHECK_INDEX_VALIDATION
    {
        void TTree_Check_Root(struct TTree *ttree);

        TTree_Check_Root(ttree);
    }
#endif

    return ret;
}

#ifdef CHECK_INDEX_VALIDATION
void
TTree_Check_Root(struct TTree *ttree)
{
    struct TTreeNode *p_node;

    if (ttree == NULL)
        return;

    if (isTempIndex_name(ttree->base_cont_.name_))
        return;

    p_node = (struct TTreeNode *) Index_OID_Point(ttree->root_[0]);
    if (p_node == NULL)
        return;

    if (p_node->type_ != ROOT_NODE || p_node->parent_ != NULL_OID)
        sc_assert(0, __FILE__, __LINE__);
}
#endif

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
int
TTree_MakeLeftChild(struct TTree *ttree, struct TTreeNode *node,
        OID insert_oid, int bucketnum)
{
    NodeID new_child_id;
    int ret;
    struct TTreeNode *new_child_node;

#ifdef INDEX_BUFFERING
    int issys_inode = 0;
#endif

    ret = Index_Collect_AllocateSlot(&ttree->collect_, &new_child_id);
    if (ret < 0)
        return ret;

    ttree->collect_.item_count_ += 1;

    TTreeNode_Create(new_child_id, node->self_, LEFT_CHILD, insert_oid, ttree);
    SetDirtyFlag(ttree->collect_.cont_id_);

    // 기존의 방법을 바꿈
    node->left_ = new_child_id;
    node->height_ = 1;
    Index_SetDirtyFlag(node->self_);

#ifdef INDEX_BUFFERING
    SetBufIndexSegment(issys_inode, new_child_id);
#endif
    new_child_node = (struct TTreeNode *) Index_OID_Point(new_child_id);
    if (new_child_node == NULL)
    {
#ifdef INDEX_BUFFERING
        UnsetBufIndexSegment(issys_inode);
#endif
        return DB_E_OIDPOINTER;
    }
#ifdef INDEX_BUFFERING
    ret = TTree_Rebalance_For_Insert(ttree, new_child_node, bucketnum);
    UnsetBufIndexSegment(issys_inode);
    if (ret < 0)
#else
    if (TTree_Rebalance_For_Insert(ttree, new_child_node, bucketnum) < 0)
#endif
    {
        MDB_SYSLOG(("TTree Make Left Child\n"));
        return DB_E_REBALANCEINSERT;
    }

    return DB_SUCCESS;
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
int
TTree_MakeRightChild(struct TTree *ttree, struct TTreeNode *node,
        OID insert_oid, int bucketnum)
{
    NodeID new_child_id;
    int ret;
    struct TTreeNode *new_child_node;

#ifdef INDEX_BUFFERING
    int issys_inode = 0;
#endif

    ret = Index_Collect_AllocateSlot(&ttree->collect_, &new_child_id);
    if (ret < 0)
        return ret;

    ttree->collect_.item_count_ += 1;
    TTreeNode_Create(new_child_id, node->self_, RIGHT_CHILD, insert_oid,
            ttree);
    SetDirtyFlag(ttree->collect_.cont_id_);

    // 기존의 방법을 바꿈
    node->right_ = new_child_id;
    node->height_ = 1;
    Index_SetDirtyFlag(node->self_);

#ifdef INDEX_BUFFERING
    SetBufIndexSegment(issys_inode, new_child_id);
#endif
    new_child_node = (struct TTreeNode *) Index_OID_Point(new_child_id);
    if (new_child_node == NULL)
    {
#ifdef INDEX_BUFFERING
        UnsetBufIndexSegment(issys_inode);
#endif
        return DB_E_OIDPOINTER;
    }

#ifdef INDEX_BUFFERING
    ret = TTree_Rebalance_For_Insert(ttree, new_child_node, bucketnum);
    UnsetBufIndexSegment(issys_inode);

    if (ret < 0)
#else
    if (TTree_Rebalance_For_Insert(ttree, new_child_node, bucketnum) < 0)
#endif
    {
        MDB_SYSLOG(("TTree Make Left Child fail\n"));
        return DB_E_REBALANCEINSERT;
    }

    return DB_SUCCESS;
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
int
TTree_Rebalance_For_Insert(struct TTree *ttree, struct TTreeNode *node,
        int bucketnum)
{
    DB_INT16 delta;
    Height left_height, right_height, height;
    const struct TTreeNode *L;
    const struct TTreeNode *R;

#ifdef INDEX_BUFFERING
    int issys_node = 0;
    OID oid_node = NULL_OID;

#ifdef MDB_DEBUG
    OID oid_node1 = NULL_OID;
#endif
    OID L_left, L_right, R_left, R_right;
#endif
    int ret;

    if (node == NULL)
    {
        return DB_FAIL;
    }

    for (;;)
    {
        if (node->left_ != NULL_OID)
        {
            L = (struct TTreeNode *) Index_OID_Point(node->left_);
            if (L == NULL)
            {
                return DB_E_OIDPOINTER;
            }
            left_height = L->height_ + 1;
#ifdef INDEX_BUFFERING
            L_left = L->left_;
            L_right = L->right_;
#endif
        }
        else
        {
            L = NULL;
            left_height = 0;
#ifdef INDEX_BUFFERING
            L_left = NULL_OID;
            L_right = NULL_OID;
#endif
        }

        if (node->right_ != NULL_OID)
        {
            R = (struct TTreeNode *) Index_OID_Point(node->right_);
            if (R == NULL)
            {
                return DB_E_OIDPOINTER;
            }
            right_height = R->height_ + 1;
#ifdef INDEX_BUFFERING
            R_left = R->left_;
            R_right = R->right_;
#endif
        }
        else
        {
            R = NULL;
            right_height = 0;
#ifdef INDEX_BUFFERING
            R_left = NULL_OID;
            R_right = NULL_OID;
#endif
        }

        delta = sc_abs(left_height - right_height);
        height = __MAX(left_height, right_height);

        if (node->height_ != height)
        {
            node->height_ = height;
            Index_SetDirtyFlag(node->self_);
        }

        if (delta > 1)
        {
            if (left_height > right_height)
            {
#ifdef INDEX_BUFFERING
                struct TTreeNode *node_L;
                Height height_LR, height_LL;

                node_L = (struct TTreeNode *) Index_OID_Point(L_right);
                if (node_L == NULL_OID)
                {
                    ret = TTree_LL_Rotate(ttree, node, bucketnum);
                    goto end;
                }

                height_LR = node_L->height_;

                node_L = (struct TTreeNode *) Index_OID_Point(L_left);
                if (node_L == NULL_OID)
                {
                    ret = TTree_LR_Rotate(ttree, node, bucketnum);
                    goto end;
                }

                height_LL = node_L->height_;

                if (height_LL >= height_LR)
                    ret = TTree_LL_Rotate(ttree, node, bucketnum);
                else
                    ret = TTree_LR_Rotate(ttree, node, bucketnum);

                goto end;
#else
                const struct TTreeNode *LL;
                const struct TTreeNode *LR;

                LR = (const struct TTreeNode *) Index_OID_Point(L->right_);
                if (LR == NULL_OID)
                    return TTree_LL_Rotate(ttree, node, bucketnum);

                LL = (const struct TTreeNode *) Index_OID_Point(L->left_);
                if (LL == NULL_OID)
                    return TTree_LR_Rotate(ttree, node, bucketnum);

                if (LL->height_ >= LR->height_)
                    return TTree_LL_Rotate(ttree, node, bucketnum);
                else
                    return TTree_LR_Rotate(ttree, node, bucketnum);
#endif
            }
            else
            {
#ifdef INDEX_BUFFERING
                struct TTreeNode *node_R;
                Height height_RR, height_RL;

                node_R = (struct TTreeNode *) Index_OID_Point(R_left);
                if (node_R == NULL_OID)
                {
                    ret = TTree_RR_Rotate(ttree, node, bucketnum);
                    goto end;
                }

                height_RL = node_R->height_;

                node_R = (struct TTreeNode *) Index_OID_Point(R_right);
                if (node_R == NULL_OID)
                {
                    ret = TTree_RL_Rotate(ttree, node, bucketnum);
                    goto end;
                }

                height_RR = node_R->height_;

                if (height_RR >= height_RL)
                    ret = TTree_RR_Rotate(ttree, node, bucketnum);
                else
                    ret = TTree_RL_Rotate(ttree, node, bucketnum);

                goto end;
#else
                const struct TTreeNode *RL;
                const struct TTreeNode *RR;

                RL = (const struct TTreeNode *) Index_OID_Point(R->left_);
                if (RL == NULL_OID)
                    return TTree_RR_Rotate(ttree, node, bucketnum);

                RR = (const struct TTreeNode *) Index_OID_Point(R->right_);
                if (RR == NULL_OID)
                    return TTree_RL_Rotate(ttree, node, bucketnum);

                if (RR->height_ >= RL->height_)
                    return TTree_RR_Rotate(ttree, node, bucketnum);
                else
                    return TTree_RL_Rotate(ttree, node, bucketnum);
#endif
            }
        }
        else if (node->parent_ == NULL_OID)
        {
            ret = DB_SUCCESS;
            goto end;
        }
        else
        {
#ifdef INDEX_BUFFERING
            UnsetBufIndexSegment(issys_node);
            oid_node = node->parent_;
            SetBufIndexSegment(issys_node, oid_node);
#ifdef MDB_DEBUG
            if (issys_node)
                oid_node1 = oid_node;
#endif
#endif
            node = (struct TTreeNode *) Index_OID_Point(node->parent_);
        }
    }       /* for */

  end:
#ifdef INDEX_BUFFERING
    UnsetBufIndexSegment(issys_node);
#ifdef MDB_DEBUG
    if (oid_node1)
    {
        if (isBufIndexSegment(Index_getSegmentNo(oid_node1)))
            sc_assert(0, __FILE__, __LINE__);
    }
#endif
#endif

    return ret;
}

//////////////////////////////////////////////////////////////////////
//  errCode
//  TTree::
//  LL_Rotate(RID* N1);
//
//           1 +++                  2 +-+
//         +---+-+--+              +--+-+--+
//         |        |              |       |
//      2 +++      +++ 3        4 +++     +++ 1
//     +--+-+--+   +-+    ====>   +-+   +-+-+-+
//     |       |   +-+            +*+   |     |
//  4 +++   5 +++                      +++5  +++3
//    +-+     +-+                      +-+   +-+
//    +-+     +-+                      +-+   +-+
//    +*+
//
//    RETURN VALUE:
//        TRUE:  when the root of TTree is changed
//        FALSE: when the root of TTree is remained
//////////////////////////////////////////////////////////////////////
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

int
TTree_LL_Rotate(struct TTree *ttree, struct TTreeNode *N1, int bucketnum)
{
    struct TTreeNode *NP;
    struct TTreeNode *N2;
    struct TTreeNode *N3;
    struct TTreeNode *N4;
    struct TTreeNode *N5;
    NodeID root_save;
    DB_UINT8 type_save;
    DB_INT16 lh;
    DB_INT16 rh;

#ifdef INDEX_BUFFERING
    int issys_NP = 0, issys_N2 = 0, issys_N3 = 0, issys_N4 = 0, issys_N5 = 0;
    OID oid_NP, oid_N2, oid_N3, oid_N4, oid_N5;
#endif

#ifdef INDEX_BUFFERING
    oid_NP = N1->parent_;
    SetBufIndexSegment(issys_NP, oid_NP);
#endif
    NP = (struct TTreeNode *) Index_OID_Point(N1->parent_);

#ifdef INDEX_BUFFERING
    oid_N2 = N1->left_;
    SetBufIndexSegment(issys_N2, oid_N2);
#endif
    N2 = (struct TTreeNode *) Index_OID_Point(N1->left_);

#ifdef INDEX_BUFFERING
    oid_N3 = N1->right_;
    SetBufIndexSegment(issys_N3, oid_N3);
#endif
    N3 = (struct TTreeNode *) Index_OID_Point(N1->right_);

#ifdef INDEX_BUFFERING
    oid_N4 = N2->left_;
    SetBufIndexSegment(issys_N4, oid_N4);
#endif
    N4 = (struct TTreeNode *) Index_OID_Point(N2->left_);

#ifdef INDEX_BUFFERING
    oid_N5 = N2->right_;
    SetBufIndexSegment(issys_N5, oid_N5);
#endif
    N5 = (struct TTreeNode *) Index_OID_Point(N2->right_);

    root_save = N1->parent_;
    type_save = N1->type_;

    /* N1 Update */
    N1->parent_ = N2->self_;
    N1->left_ = (N5 == NULL) ? NULL_OID : N5->self_;
    N1->type_ = RIGHT_CHILD;
    lh = (N5 == NULL) ? 0 : N5->height_ + 1;
    rh = (N3 == NULL) ? 0 : N3->height_ + 1;
    N1->height_ = __MAX(lh, rh);
    Index_SetDirtyFlag(N1->self_);

    /* N2 Update */
    N2->parent_ = root_save;
    N2->right_ = N1->self_;
    N2->type_ = type_save;
    lh = (N4 == NULL) ? 0 : N4->height_ + 1;
    rh = N1->height_ + 1;
    N2->height_ = __MAX(lh, rh);
    Index_SetDirtyFlag(N2->self_);

    if (N5 != NULL)
    {
        N5->parent_ = N1->self_;
        N5->type_ = LEFT_CHILD;
        Index_SetDirtyFlag(N5->self_);
    }

    if (NP == NULL)
    {
        ttree->root_[bucketnum] = N2->self_;
        SetDirtyFlag(ttree->collect_.cont_id_);
    }
    else
    {
        struct TTreeNode *other;

        if (NP->left_ == N1->self_)
        {
            NP->left_ = N2->self_;
            other = (struct TTreeNode *) Index_OID_Point(NP->right_);
            lh = N2->height_ + 1;
            rh = (other == NULL) ? 0 : other->height_ + 1;
        }
        else
        {
            NP->right_ = N2->self_;
            other = (struct TTreeNode *) Index_OID_Point(NP->left_);
            rh = N2->height_ + 1;
            lh = (other == NULL) ? 0 : other->height_ + 1;
        }
        NP->height_ = __MAX(lh, rh);

        Index_SetDirtyFlag(NP->self_);
    }

#ifdef MDB_DEBUG
    TTreeNode_Type_Check(NP);
    TTreeNode_Type_Check(N1);
    TTreeNode_Type_Check(N2);
    TTreeNode_Type_Check(N3);
    TTreeNode_Type_Check(N4);
    TTreeNode_Type_Check(N5);
#endif
#ifdef INDEX_BUFFERING
    UnsetBufIndexSegment(issys_NP);
    UnsetBufIndexSegment(issys_N2);
    UnsetBufIndexSegment(issys_N3);
    UnsetBufIndexSegment(issys_N4);
    UnsetBufIndexSegment(issys_N5);
#endif

    return DB_SUCCESS;
}

//////////////////////////////////////////////////////////////////////
//  DB_BOOL
//  TTree::
//      LR_Rotate(const TTreeNode* N1);
//
//           1 +++                     +-+ 5
//         +---+-+--+              +---+-+--+
//      2 +++      +++ 3         2+++      +++1
//     +--+-+--+   +-+    ====> +-+-+-+  +-+-+-+
//  4 +++   5 +++  +-+        4+++  6++++++7  +++3
//    +-+   +-+-+-+            +-+   +-++-+   +-+
//    +-+  +++6  +++7          +-+   +*+      +-+
//         +-+   +-+
//         +*+
//  1,2,5,6,7
//
//////////////////////////////////////////////////////////////////////

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
int
TTree_LR_Rotate(struct TTree *ttree, struct TTreeNode *N1, int bucketnum)
{
    struct TTreeNode *NP;
    struct TTreeNode *N2;
    struct TTreeNode *N3;
    struct TTreeNode *N4;
    struct TTreeNode *N5;
    struct TTreeNode *N6;
    struct TTreeNode *N7;
    struct TTreeNode *other;

    NodeID root_save;
    DB_UINT8 save_type;

    DB_INT16 lh;
    DB_INT16 rh;

#ifdef INDEX_BUFFERING
    int issys_NP = 0, issys_N2 = 0, issys_N3 = 0, issys_N4 = 0,
            issys_N5 = 0, issys_N6 = 0, issys_N7 = 0;
    OID oid_NP, oid_N2, oid_N3, oid_N4, oid_N5, oid_N6, oid_N7;
#endif

#ifdef INDEX_BUFFERING
    oid_NP = N1->parent_;
    SetBufIndexSegment(issys_NP, oid_NP);
#endif
    NP = (struct TTreeNode *) Index_OID_Point(N1->parent_);

#ifdef INDEX_BUFFERING
    oid_N2 = N1->left_;
    SetBufIndexSegment(issys_N2, oid_N2);
#endif
    N2 = (struct TTreeNode *) Index_OID_Point(N1->left_);

#ifdef INDEX_BUFFERING
    oid_N3 = N1->right_;
    SetBufIndexSegment(issys_N3, oid_N3);
#endif
    N3 = (struct TTreeNode *) Index_OID_Point(N1->right_);

#ifdef INDEX_BUFFERING
    oid_N4 = N2->left_;
    SetBufIndexSegment(issys_N4, oid_N4);
#endif
    N4 = (struct TTreeNode *) Index_OID_Point(N2->left_);

#ifdef INDEX_BUFFERING
    oid_N5 = N2->right_;
    SetBufIndexSegment(issys_N5, oid_N5);
#endif
    N5 = (struct TTreeNode *) Index_OID_Point(N2->right_);

#ifdef INDEX_BUFFERING
    oid_N6 = N5->left_;
    SetBufIndexSegment(issys_N6, oid_N6);
#endif
    N6 = (struct TTreeNode *) Index_OID_Point(N5->left_);

#ifdef INDEX_BUFFERING
    oid_N7 = N5->right_;
    SetBufIndexSegment(issys_N7, oid_N7);
#endif
    N7 = (struct TTreeNode *) Index_OID_Point(N5->right_);

    root_save = N1->parent_;
    save_type = N1->type_;

    // Update N1

    N1->parent_ = N5->self_;
    N1->left_ = (N7 == NULL) ? NULL_OID : N7->self_;
    lh = (N7 == NULL) ? 0 : N7->height_ + 1;
    rh = (N3 == NULL) ? 0 : N3->height_ + 1;
    N1->height_ = __MAX(lh, rh);
    N1->type_ = RIGHT_CHILD;

    Index_SetDirtyFlag(N1->self_);

    // Update N6
    if (N6 != NULL)
    {
        N6->parent_ = N2->self_;
        N6->type_ = RIGHT_CHILD;
        Index_SetDirtyFlag(N6->self_);
    }

    // Update N7
    if (N7 != NULL)
    {
        N7->parent_ = N1->self_;
        N7->type_ = LEFT_CHILD;
        Index_SetDirtyFlag(N7->self_);
    }

    // Update N2

    N2->parent_ = N5->self_;
    N2->right_ = (N6 == NULL) ? NULL_OID : N6->self_;
    lh = (N4 == NULL) ? 0 : N4->height_ + 1;
    rh = (N6 == NULL) ? 0 : N6->height_ + 1;
    N2->height_ = __MAX(lh, rh);
    N2->type_ = LEFT_CHILD;


    // Update N5
    N5->parent_ = root_save;
    N5->right_ = N1->self_;
    N5->left_ = N2->self_;
    lh = (N2 == NULL) ? 0 : N2->height_ + 1;
    rh = (N1 == NULL) ? 0 : N1->height_ + 1;
    N5->height_ = __MAX(lh, rh);
    N5->type_ = save_type;


    if (N5->item_count_ < TNODE_MIN_ITEM && N2->right_ == NULL_OID)
    {
        int delta;

        delta = (int) (TNODE_MIN_ITEM) - (int) (N5->item_count_);
        if (delta > (int) N2->item_count_ - 1)
            delta = (int) N2->item_count_ - 1;
        TTreeNode_MoveLeftToRight(N2, N5, delta, ttree);
    }

    Index_SetDirtyFlag(N2->self_);
    Index_SetDirtyFlag(N5->self_);

    if (NP == NULL)
    {
        ttree->root_[bucketnum] = N5->self_;
        SetDirtyFlag(ttree->collect_.cont_id_);
    }
    else
    {
        if (NP->left_ == N1->self_)
        {
            NP->left_ = N5->self_;
            other = (struct TTreeNode *) Index_OID_Point(NP->right_);
            lh = N5->height_ + 1;
            rh = (other == NULL) ? 0 : other->height_ + 1;
        }
        else
        {
            NP->right_ = N5->self_;
            other = (struct TTreeNode *) Index_OID_Point(NP->left_);
            rh = N5->height_ + 1;
            lh = (other == NULL) ? 0 : other->height_ + 1;
        }

        NP->height_ = __MAX(lh, rh);

        Index_SetDirtyFlag(NP->self_);
    }

#ifdef MDB_DEBUG
    TTreeNode_Type_Check(NP);
    TTreeNode_Type_Check(N1);
    TTreeNode_Type_Check(N2);
    TTreeNode_Type_Check(N3);
    TTreeNode_Type_Check(N4);
    TTreeNode_Type_Check(N5);
    TTreeNode_Type_Check(N6);
    TTreeNode_Type_Check(N7);
#endif
#ifdef INDEX_BUFFERING
    UnsetBufIndexSegment(issys_NP);
    UnsetBufIndexSegment(issys_N2);
    UnsetBufIndexSegment(issys_N3);
    UnsetBufIndexSegment(issys_N4);
    UnsetBufIndexSegment(issys_N5);
    UnsetBufIndexSegment(issys_N6);
    UnsetBufIndexSegment(issys_N7);
#endif

    return DB_SUCCESS;
}

//////////////////////////////////////////////////////////////////////
//  DB_BOOL
//  TTree::
//      RR_Rotate(const TTreeNode* N1);
//             **
//           1+++                      3+++
//         +--+-+--+                +---+-+--+          2
//       2+++     +++3             +++ 1
//        +-+  +--+-+-+  ====>   +-+-+-+   5+++
//        +-+ +++4  5+++        +++2 4+++   +-+
//            +-+    +-+        +-+   +-+   +-+
//            +-+    +-+        +-+   +-+   +*+
//                   +*+
//
//////////////////////////////////////////////////////////////////////

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
int
TTree_RR_Rotate(struct TTree *ttree, struct TTreeNode *N1, int bucketnum)
{
    struct TTreeNode *NP;
    struct TTreeNode *N2;
    struct TTreeNode *N3;
    struct TTreeNode *N4;
    struct TTreeNode *N5;
    NodeID root_save;
    DB_UINT8 type_save;
    DB_INT16 lh;
    DB_INT16 rh;

#ifdef INDEX_BUFFERING
    int issys_NP = 0, issys_N2 = 0, issys_N3 = 0, issys_N4 = 0, issys_N5 = 0;
    OID oid_NP, oid_N2, oid_N3, oid_N4, oid_N5;
#endif

#ifdef INDEX_BUFFERING
    oid_NP = N1->parent_;
    SetBufIndexSegment(issys_NP, oid_NP);
#endif
    NP = (struct TTreeNode *) Index_OID_Point(N1->parent_);

#ifdef INDEX_BUFFERING
    oid_N2 = N1->left_;
    SetBufIndexSegment(issys_N2, oid_N2);
#endif
    N2 = (struct TTreeNode *) Index_OID_Point(N1->left_);

#ifdef INDEX_BUFFERING
    oid_N3 = N1->right_;
    SetBufIndexSegment(issys_N3, oid_N3);
#endif
    N3 = (struct TTreeNode *) Index_OID_Point(N1->right_);

#ifdef INDEX_BUFFERING
    oid_N4 = N3->left_;
    SetBufIndexSegment(issys_N4, oid_N4);
#endif
    N4 = (struct TTreeNode *) Index_OID_Point(N3->left_);

#ifdef INDEX_BUFFERING
    oid_N5 = N3->right_;
    SetBufIndexSegment(issys_N5, oid_N5);
#endif
    N5 = (struct TTreeNode *) Index_OID_Point(N3->right_);

    root_save = N1->parent_;
    type_save = N1->type_;

    // N1 Update
    N1->parent_ = N3->self_;
    N1->right_ = (N4 == NULL) ? NULL_OID : N4->self_;
    N1->type_ = LEFT_CHILD;
    lh = (N2 == NULL) ? 0 : N2->height_ + 1;
    rh = (N4 == NULL) ? 0 : N4->height_ + 1;
    N1->height_ = __MAX(lh, rh);
    Index_SetDirtyFlag(N1->self_);

    // N3 Update
    N3->parent_ = root_save;
    N3->left_ = N1->self_;
    N3->type_ = type_save;
    lh = N1->height_ + 1;
    rh = (N5 == NULL) ? 0 : N5->height_ + 1;
    N3->height_ = __MAX(lh, rh);
    Index_SetDirtyFlag(N3->self_);

    if (N4 != NULL)
    {
        N4->parent_ = N1->self_;
        N4->type_ = RIGHT_CHILD;
        Index_SetDirtyFlag(N4->self_);
    }

    if (NP == NULL)
    {
        ttree->root_[bucketnum] = N3->self_;
        SetDirtyFlag(ttree->collect_.cont_id_);
    }
    else
    {
        struct TTreeNode *other;

        if (NP->left_ == N1->self_)
        {
            NP->left_ = N3->self_;
            other = (struct TTreeNode *) Index_OID_Point(NP->right_);
            lh = N3->height_ + 1;
            rh = (other == NULL) ? 0 : other->height_ + 1;
        }
        else
        {
            NP->right_ = N3->self_;
            other = (struct TTreeNode *) Index_OID_Point(NP->left_);
            rh = N3->height_ + 1;
            lh = (other == NULL) ? 0 : other->height_ + 1;
        }
        NP->height_ = __MAX(lh, rh);

        Index_SetDirtyFlag(NP->self_);
    }

#ifdef MDB_DEBUG
    TTreeNode_Type_Check(NP);
    TTreeNode_Type_Check(N1);
    TTreeNode_Type_Check(N2);
    TTreeNode_Type_Check(N3);
    TTreeNode_Type_Check(N4);
    TTreeNode_Type_Check(N5);
#endif
#ifdef INDEX_BUFFERING
    UnsetBufIndexSegment(issys_NP);
    UnsetBufIndexSegment(issys_N2);
    UnsetBufIndexSegment(issys_N3);
    UnsetBufIndexSegment(issys_N4);
    UnsetBufIndexSegment(issys_N5);
#endif
    return DB_SUCCESS;
}

//////////////////////////////////////////////////////////////////////
//  errCode
//  TTree::
//      RL_Rotate(const TTreeNode* N1);
//
//          1+++                         4+-+
//     +-----+-+--+                   +---+-+--+
//    +++2      3+++                1+++      +++3
//    +-+     +--+-+--+   ====>    +-+-+-+  +-+-+-+
//    +-+    +++4    +++5        2+++  6++++++7  +++5
//         +-+-+-+   +-+          +-+   +-++-+   +-+
//        +++6  +++7 +-+          +-+      +*+   +-+
//        +-+   +-+
//              +*+
//  1,3,4,6,7
//
//////////////////////////////////////////////////////////////////////

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
int
TTree_RL_Rotate(struct TTree *ttree, struct TTreeNode *N1, int bucketnum)
{
    struct TTreeNode *NP;
    struct TTreeNode *N2;
    struct TTreeNode *N3;
    struct TTreeNode *N4;
    struct TTreeNode *N5;
    struct TTreeNode *N6;
    struct TTreeNode *N7;
    NodeID root_save;
    NodeType save_type;
    DB_INT16 lh;
    DB_INT16 rh;

#ifdef INDEX_BUFFERING
    int issys_NP = 0, issys_N2 = 0, issys_N3 = 0, issys_N4 = 0,
            issys_N5 = 0, issys_N6 = 0, issys_N7 = 0;
    OID oid_NP, oid_N2, oid_N3, oid_N4, oid_N5, oid_N6, oid_N7;
#endif

#ifdef INDEX_BUFFERING
    oid_NP = N1->parent_;
    SetBufIndexSegment(issys_NP, oid_NP);
#endif
    NP = (struct TTreeNode *) Index_OID_Point(N1->parent_);

#ifdef INDEX_BUFFERING
    oid_N2 = N1->left_;
    SetBufIndexSegment(issys_N2, oid_N2);
#endif
    N2 = (struct TTreeNode *) Index_OID_Point(N1->left_);

#ifdef INDEX_BUFFERING
    oid_N3 = N1->right_;
    SetBufIndexSegment(issys_N3, oid_N3);
#endif
    N3 = (struct TTreeNode *) Index_OID_Point(N1->right_);

#ifdef INDEX_BUFFERING
    oid_N4 = N3->left_;
    SetBufIndexSegment(issys_N4, oid_N4);
#endif
    N4 = (struct TTreeNode *) Index_OID_Point(N3->left_);

#ifdef INDEX_BUFFERING
    oid_N5 = N3->right_;
    SetBufIndexSegment(issys_N5, oid_N5);
#endif
    N5 = (struct TTreeNode *) Index_OID_Point(N3->right_);

#ifdef INDEX_BUFFERING
    oid_N6 = N4->left_;
    SetBufIndexSegment(issys_N6, oid_N6);
#endif
    N6 = (struct TTreeNode *) Index_OID_Point(N4->left_);

#ifdef INDEX_BUFFERING
    oid_N7 = N4->right_;
    SetBufIndexSegment(issys_N7, oid_N7);
#endif
    N7 = (struct TTreeNode *) Index_OID_Point(N4->right_);

    root_save = N1->parent_;
    save_type = N1->type_;

    //N1 Update
    N1->parent_ = N4->self_;
    N1->right_ = (N6 == NULL) ? NULL_OID : N6->self_;
    lh = (N2 == NULL) ? 0 : N2->height_ + 1;
    rh = (N6 == NULL) ? 0 : N6->height_ + 1;
    N1->height_ = __MAX(lh, rh);
    N1->type_ = LEFT_CHILD;
    Index_SetDirtyFlag(N1->self_);

    if (N6 != NULL)
    {
        N6->parent_ = N1->self_;
        N6->type_ = RIGHT_CHILD;
        Index_SetDirtyFlag(N6->self_);
    }

    if (N7 != NULL)
    {
        N7->parent_ = N3->self_;
        N7->type_ = LEFT_CHILD;
        Index_SetDirtyFlag(N7->self_);
    }

    // N3 UPdate
    N3->parent_ = N4->self_;
    N3->left_ = (N7 == NULL) ? NULL_OID : N7->self_;
    lh = (N7 == NULL) ? 0 : N7->height_ + 1;
    rh = (N5 == NULL) ? 0 : N5->height_ + 1;
    N3->height_ = __MAX(lh, rh);
    N3->type_ = RIGHT_CHILD;

    // N4 Update
    N4->parent_ = root_save;
    N4->left_ = N1->self_;
    N4->right_ = N3->self_;
    lh = N1->height_ + 1;
    rh = N3->height_ + 1;
    N4->height_ = __MAX(lh, rh);
    N4->type_ = save_type;

    if (N4->item_count_ < TNODE_MIN_ITEM && N3->left_ == NULL_OID)
    {
        int delta;

        delta = (int) TNODE_MIN_ITEM - (int) N4->item_count_;

        if (delta > (int) N3->item_count_ - 1)
            delta = (int) N3->item_count_ - 1;
        TTreeNode_MoveRightToLeft(N4, N3, delta, ttree);
    }

    Index_SetDirtyFlag(N3->self_);
    Index_SetDirtyFlag(N4->self_);

    if (NP == NULL)
    {
        ttree->root_[bucketnum] = N4->self_;
        SetDirtyFlag(ttree->collect_.cont_id_);
    }
    else
    {
        struct TTreeNode *other;

        if (NP->left_ == N1->self_)
        {
            NP->left_ = N4->self_;
            other = (struct TTreeNode *) Index_OID_Point(NP->right_);
            lh = N4->height_ + 1;
            rh = (other == NULL) ? 0 : other->height_ + 1;
        }
        else
        {
            NP->right_ = N4->self_;
            other = (struct TTreeNode *) Index_OID_Point(NP->left_);
            rh = N4->height_ + 1;
            lh = (other == NULL) ? 0 : other->height_ + 1;
        }
        NP->height_ = __MAX(lh, rh);

        Index_SetDirtyFlag(NP->self_);
    }

#ifdef MDB_DEBUG
    TTreeNode_Type_Check(NP);
    TTreeNode_Type_Check(N1);
    TTreeNode_Type_Check(N2);
    TTreeNode_Type_Check(N3);
    TTreeNode_Type_Check(N4);
    TTreeNode_Type_Check(N5);
    TTreeNode_Type_Check(N6);
    TTreeNode_Type_Check(N7);
#endif
#ifdef INDEX_BUFFERING
    UnsetBufIndexSegment(issys_NP);
    UnsetBufIndexSegment(issys_N2);
    UnsetBufIndexSegment(issys_N3);
    UnsetBufIndexSegment(issys_N4);
    UnsetBufIndexSegment(issys_N5);
    UnsetBufIndexSegment(issys_N6);
    UnsetBufIndexSegment(issys_N7);
#endif

    return DB_SUCCESS;
}
