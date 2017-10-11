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
#include "mdb_KeyValue.h"
#include "mdb_Mem_Mgr.h"
#include "ErrorCode.h"
#include "mdb_syslog.h"

#include "sql_datast.h"

#define __MAX(a,b) (((a) > (b)) ? (a) : (b))

struct TTreeNode *TTree_GlbChild(struct TTreeNode *);
struct TTreeNode *TTree_LubChild(struct TTreeNode *);
struct TTreeNode *TTree_NextNode(struct TTreeNode *);
int Index_Col_Remove(struct Collection *, OID);
struct TTreeNode *TTree_PrevNode(struct TTreeNode *node);
int KeyValue_Delete2(struct KeyValue *pKeyValue);
int KeyValue_Create(const char *record, struct KeyDesc *key_desc,
        int recSize, struct KeyValue *keyValue, int f_memory_record);

int TTreeNode_Type_Check(struct TTreeNode *node);

//////////////////////////////////////////////////////////////////
//
// Function Name :
//
//////////////////////////////////////////////////////////////////

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
TTree_Destory(IndexID index)
{
    struct TTree *ttree;
    int issys_ttree;
    int ret;

    ttree = (struct TTree *) OID_Point(index);
    if (ttree == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    SetBufSegment(issys_ttree, index);

    ret = Index_MemMgr_FreeAllNodes(ttree);

    UnsetBufSegment(issys_ttree);

    if (ret < 0)
        return DB_FAIL;

    return DB_SUCCESS;
}

//////////////////////////////////////////////////////////////////
//
// Function Name :
//
//////////////////////////////////////////////////////////////////

/*****************************************************************************/
//! TTree_Get_First_Ge
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param ttree(in/out) :
 * \param key(in/out)   : 
 * \param first(in/out) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
TTree_Get_First_Ge(struct TTree *ttree, struct KeyValue *key,
        struct TTreePosition *first)
{
    int bucketnum;
    struct TTreeNode *gld;
    register struct TTreeNode *node;
    register int comp_res;
    register DB_INT16 f, t, m;
    int ret;

#ifdef INDEX_BUFFERING
    int issys_node = 0;
    OID oid_node = NULL_OID;
    int issys_gld = 0;
    OID oid_gld = NULL_OID;
#endif
    bucketnum = 0;

    if (ttree->root_[bucketnum] == NULL_OID)
    {
#ifdef INDEX_BUFFERING
        first->self_ = NULL_OID;
#endif
        first->node_ = NULL;
        return DB_E_NOMORERECORDS;
    }

#ifdef INDEX_BUFFERING
    oid_node = ttree->root_[bucketnum];
#endif
    node = (struct TTreeNode *) Index_OID_Point(ttree->root_[bucketnum]);

    while (node != NULL)
    {
#ifdef INDEX_BUFFERING
        SetBufIndexSegment(issys_node, oid_node);
#endif
        comp_res = TTreeNode_CompKeyValWithMinimum(key, node, ttree);
        if (comp_res <= 0)
        {
            /* key가 첫번째 item보다 작거나 같으면 left child에서 search */

            gld = (struct TTreeNode *) TTree_GlbChild(node);

            if (gld == NULL)
            {   /* GLB가 없으면 현재 node가 MDB_GE node */
#ifdef INDEX_BUFFERING
                first->self_ = node->self_;
#endif
                first->node_ = (struct TTreeNode *) node;
                first->idx_ = 0;

                if (comp_res == 0)
                    ret = DB_SUCCESS;
                else
                    ret = DB_W_ITEMMORETHANKEY;

                goto end;
            }
            else
            {
                /* GLB에서 마지막 item 값과 비교.
                   key 값이 record 값보다 작거나 같으면 left tree에서 search,
                   크면 현재 node/item이 MDB_GE */
#ifdef INDEX_BUFFERING
                oid_gld = gld->self_;
                SetBufIndexSegment(issys_gld, oid_gld);
                ret = TTreeNode_CompKeyValWithMaximum(key, gld, ttree);
                UnsetBufIndexSegment(issys_gld);
                if (ret <= 0)
#else
                if (TTreeNode_CompKeyValWithMaximum(key, gld, ttree) <= 0)
#endif
                {
#ifdef INDEX_BUFFERING
                    UnsetBufIndexSegment(issys_node);
                    oid_node = node->left_;
#endif
                    node = (struct TTreeNode *) Index_OID_Point(node->left_);
                }
                else
                {
#ifdef INDEX_BUFFERING
                    first->self_ = node->self_;
#endif
                    first->node_ = (struct TTreeNode *) node;
                    first->idx_ = 0;

                    if (comp_res == 0)
                        ret = DB_SUCCESS;
                    else
                        ret = DB_W_ITEMMORETHANKEY;

                    goto end;
                }
            }
        }
        else
        {
            /* key가 첫번째 item보다 큰 경우 마지막 item과 비교 */
            if (TTreeNode_CompKeyValWithMaximum(key, node, ttree) > 0)
            {
#ifdef INDEX_BUFFERING
                UnsetBufIndexSegment(issys_node);
                oid_node = node->right_;
#endif
                /* key가 마지막 item보다 큰 경우 right tree 검색 */
                node = (struct TTreeNode *) Index_OID_Point(node->right_);
            }
            else
            {
                /* [0] < key <= [last]...
                   key가 마지막 item보다 작거나 같은 경우,
                   현재 node에서 검색 */

                if (node->item_count_ == 1)
                {
#ifdef INDEX_BUFFERING
                    first->self_ = NULL_OID;
#endif
                    first->node_ = NULL;
                    ret = DB_E_NOMORERECORDS;   //DB_FAIL;
                    goto end;
                }

                comp_res = TTreeNode_CompKeyValWithItem(key, node, 1, ttree);
                if (comp_res <= 0)
                {       /* 찾았음 */
#ifdef INDEX_BUFFERING
                    first->self_ = node->self_;
#endif
                    first->node_ = (struct TTreeNode *) node;
                    first->idx_ = 1;

                    if (comp_res == 0)
                        ret = DB_SUCCESS;
                    else
                        ret = DB_W_ITEMMORETHANKEY;
                    goto end;
                }

                f = 2;
                t = node->item_count_ - 1;

                /* binary search */
                while (1)
                {
                    if (f > t)
                        break;

                    m = (f + t) >> 1;

                    comp_res =
                            TTreeNode_CompKeyValWithItem(key, node, m, ttree);
                    if (comp_res == 0)
                    {   /* 같으면 첫번째 레코드를 찾는다 */
                        m--;
                        while (f <= m)
                        {
                            if (TTreeNode_CompKeyValWithItem(key, node, m,
                                            ttree) != 0)
                                break;
                            m--;
                        }

#ifdef INDEX_BUFFERING
                        first->self_ = node->self_;
#endif
                        first->node_ = (struct TTreeNode *) node;
                        first->idx_ = m + 1;

                        ret = DB_SUCCESS;
                        goto end;

                    }
                    else if (comp_res > 0)
                    {
                        /*
                           [bug fix] m에서 m+1로 수정.
                           f == t이면 f == m 인 상태.
                           key와 m을 비교해서 key가 m보다 큰 상태이미로 찾는
                           key 값은 m+1 임.
                           현재 [0] < key <= [last] 상태인 node 이므로,
                           key와 last를
                           비교하면 comp_res <= 0 인 결과를 받게 되므로,
                           m은 last가 아님. 따라서, m+1 가능. */
                        if (f == t)
                        {
#ifdef INDEX_BUFFERING
                            first->self_ = node->self_;
#endif
                            first->node_ = (struct TTreeNode *) node;
                            first->idx_ = m + 1;

                            ret = DB_SUCCESS;
                            goto end;
                        }

                        f = m + 1;
                    }
                    else
                    {
                        if (f == t || f == m)
                        {
#ifdef INDEX_BUFFERING
                            first->self_ = node->self_;
#endif
                            first->node_ = (struct TTreeNode *) node;
                            first->idx_ = m;

                            ret = DB_W_ITEMMORETHANKEY;
                            goto end;
                        }

                        t = m - 1;
                    }
                }

#ifdef INDEX_BUFFERING
                first->self_ = NULL_OID;
#endif
                first->node_ = NULL;
                ret = DB_E_NOMORERECORDS;       //DB_FAIL;
                goto end;
            }
        }
    }

#ifdef INDEX_BUFFERING
    first->self_ = NULL_OID;
#endif
    first->node_ = NULL;
    return DB_E_NOMORERECORDS;  //DB_FAIL;

  end:
#ifdef INDEX_BUFFERING
    UnsetBufIndexSegment(issys_node);
#endif
    return ret;
}

//////////////////////////////////////////////////////////////////
//
// Function Name :
//
//////////////////////////////////////////////////////////////////
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
TTree_Get_Last_Le(struct TTree *ttree, struct KeyValue *key,
        struct TTreePosition *last)
{
    struct TTreeNode *node;
    int bucketnum;
    int comp_res;
    int ret;

#ifdef INDEX_BUFFERING
    int issys_node = 0;
    OID oid_node;
#endif
    NodeID node_id;

    bucketnum = 0;

#ifdef INDEX_BUFFERING
    oid_node = ttree->root_[bucketnum];
#endif

    node_id = ttree->root_[bucketnum];
    node = (struct TTreeNode *) Index_OID_Point(node_id);

    while (node != NULL)
    {
#ifdef INDEX_BUFFERING
        SetBufIndexSegment(issys_node, oid_node);
#endif
        comp_res = TTreeNode_CompKeyValWithMaximum(key, node, ttree);
        if (comp_res >= 0)
        {
            struct TTreeNode *lub;

#ifdef INDEX_BUFFERING
            int issys_lub = 0;
            OID oid_lub;
#endif

            lub = (struct TTreeNode *) TTree_LubChild(node);

            if (lub == NULL)
            {
#ifdef INDEX_BUFFERING
                last->self_ = node->self_;
#endif
                last->node_ = (struct TTreeNode *) node;
                last->idx_ = node->item_count_ - 1;
                ret = DB_SUCCESS;
                goto end;
            }
            else
            {
#ifdef INDEX_BUFFERING
                oid_lub = lub->self_;
                SetBufIndexSegment(issys_lub, oid_lub);
                ret = TTreeNode_CompKeyValWithMinimum(key, lub, ttree);
                UnsetBufIndexSegment(issys_lub);
                if (ret >= 0)
#else
                if (TTreeNode_CompKeyValWithMinimum(key, lub, ttree) >= 0)
#endif
                {
#ifdef INDEX_BUFFERING
                    UnsetBufIndexSegment(issys_node);
                    oid_node = node->right_;
#endif
                    node = (struct TTreeNode *) Index_OID_Point(node->right_);
                }
                else
                {
#ifdef INDEX_BUFFERING
                    last->self_ = node->self_;
#endif
                    last->node_ = (struct TTreeNode *) node;
                    last->idx_ = node->item_count_ - 1;
                    ret = DB_SUCCESS;
                    goto end;
                }
            }
        }
        else
        {
            if (TTreeNode_CompKeyValWithMinimum(key, node, ttree) < 0)
            {
#ifdef INDEX_BUFFERING
                UnsetBufIndexSegment(issys_node);
                oid_node = node->left_;
#endif
                node = (struct TTreeNode *) Index_OID_Point(node->left_);
            }
            else
            {
                /* [0] <= key < [last] */
                DB_INT16 f, t, m;

                if (TTreeNode_CompKeyValWithItem(key, node,
                                (node->item_count_ - 2), ttree) >= 0)
                {       /* 찾았음 */
#ifdef INDEX_BUFFERING
                    last->self_ = node->self_;
#endif
                    last->node_ = (struct TTreeNode *) node;
                    last->idx_ = node->item_count_ - 2;
                    ret = DB_SUCCESS;
                    goto end;
                }

                f = 0;
                t = node->item_count_ - 3;

                /* binary search */
                while (1)
                {
                    if (f > t)
                        break;

                    m = (f + t) >> 1;
                    //m = (f + t) / 2;

                    comp_res =
                            TTreeNode_CompKeyValWithItem(key, node, m, ttree);
                    if (comp_res == 0)
                    {   /* 같으면 첫번째 레코드를 찾는다 */
                        m++;
                        while (m <= t)
                        {
                            if (TTreeNode_CompKeyValWithItem(key, node, m,
                                            ttree) != 0)
                                break;
                            m++;
                        }

#ifdef INDEX_BUFFERING
                        last->self_ = node->self_;
#endif
                        last->node_ = (struct TTreeNode *) node;
                        last->idx_ = m - 1;
                        ret = DB_SUCCESS;
                        goto end;

                    }
                    else if (comp_res > 0)
                    {
                        if (f == t)
                        {
#ifdef INDEX_BUFFERING
                            last->self_ = node->self_;
#endif
                            last->node_ = (struct TTreeNode *) node;
                            last->idx_ = m;
                            ret = DB_SUCCESS;
                            goto end;
                        }

                        f = m + 1;
                    }
                    else
                    {
                        if (f == t || f == m)
                        {
#ifdef INDEX_BUFFERING
                            last->self_ = node->self_;
#endif
                            last->node_ = (struct TTreeNode *) node;
                            last->idx_ = m - 1;
                            ret = DB_SUCCESS;
                            goto end;
                        }

                        t = m - 1;
                    }
                }

#ifdef INDEX_BUFFERING
                last->self_ = NULL_OID;
#endif
                last->node_ = NULL;
                ret = DB_E_NOMORERECORDS;
                goto end;
            }
        }
    }

#ifdef INDEX_BUFFERING
    last->self_ = NULL_OID;
#endif
    last->node_ = NULL;

    return DB_E_NOMORERECORDS;

  end:
#ifdef INDEX_BUFFERING
    UnsetBufIndexSegment(issys_node);
#endif
    return ret;
}


//////////////////////////////////////////////////////////////////
//
// Function Name :
//
//////////////////////////////////////////////////////////////////
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
TTree_Get_First_Eq(struct TTree *ttree, struct KeyValue *key,
        struct TTreePosition *p)
{
    register struct TTreeNode *node;
    register int comp_res;
    register DB_INT16 f, t, m;
    struct TTreeNode *gld;
    int bucketnum;
    int ret;
    NodeID node_id;

#ifdef INDEX_BUFFERING
    int issys_node = 0;
    OID oid_node;
    int issys_gld = 0;
    OID oid_gld;
#endif

    bucketnum = 0;

    node_id = ttree->root_[bucketnum];
    node = (struct TTreeNode *) Index_OID_Point(node_id);

    if (node == NULL)
    {
#ifdef INDEX_BUFFERING
        p->self_ = NULL_OID;
#endif
        p->node_ = NULL;
        return DB_E_NOMORERECORDS;
    }

    if (node->item_[0] == NULL_OID)
    {
        MDB_SYSLOG(("Index error: remake index! %d first eq\n", bucketnum));
#ifdef INDEX_BUFFERING
        p->self_ = NULL_OID;
#endif
        p->node_ = NULL;
        return DB_FAIL;
    }
#ifdef INDEX_BUFFERING
    oid_node = ttree->root_[bucketnum];
#endif

    while (node)
    {
#ifdef INDEX_BUFFERING
        SetBufIndexSegment(issys_node, oid_node);
#endif
        comp_res = TTreeNode_CompKeyValWithMinimum(key, node, ttree);
        if (comp_res < 0)
        {
#ifdef INDEX_BUFFERING
            UnsetBufIndexSegment(issys_node);
            oid_node = node->left_;
#endif
            node = (struct TTreeNode *) Index_OID_Point(node->left_);
        }
        else
        {
            if (comp_res == 0)
            {
                gld = (struct TTreeNode *) TTree_GlbChild(node);

                if (gld == NULL)
                {
#ifdef INDEX_BUFFERING
                    p->self_ = node->self_;
#endif
                    p->node_ = (struct TTreeNode *) node;
                    p->idx_ = 0;
                    ret = DB_SUCCESS;
                    goto end;
                }
                else
                {
#ifdef INDEX_BUFFERING
                    oid_gld = gld->self_;
                    SetBufIndexSegment(issys_gld, oid_gld);
                    ret = TTreeNode_CompKeyValWithMaximum(key, gld, ttree);
                    UnsetBufIndexSegment(issys_gld);
                    if (ret == 0)
#else
                    if (TTreeNode_CompKeyValWithMaximum(key, gld, ttree) == 0)
#endif
                    {
#ifdef INDEX_BUFFERING
                        UnsetBufIndexSegment(issys_node);
                        oid_node = node->left_;
#endif
                        node = (struct TTreeNode *) Index_OID_Point(node->
                                left_);
                    }
                    else
                    {
#ifdef INDEX_BUFFERING
                        p->self_ = node->self_;
#endif
                        p->node_ = (struct TTreeNode *) node;
                        p->idx_ = 0;
                        ret = DB_SUCCESS;
                        goto end;
                    }
                }
            }
            else
            {
                if (TTreeNode_CompKeyValWithMaximum(key, node, ttree) > 0)
                {
#ifdef INDEX_BUFFERING
                    UnsetBufIndexSegment(issys_node);
                    oid_node = node->right_;
#endif
                    node = (struct TTreeNode *) Index_OID_Point(node->right_);
                }
                else
                {
                    /* [0] < key <= [last] */
                    f = 1;
                    t = node->item_count_ - 1;

                    /* binary search */
                    while (1)
                    {
                        if (f > t)
                            break;

                        m = (f + t) >> 1;

                        comp_res = TTreeNode_CompKeyValWithItem(key, node, m,
                                ttree);
                        if (comp_res == 0)
                        {       /* 같으면 첫번째 레코드를 찾는다 */
                            m--;
                            while (f <= m)
                            {
                                if (TTreeNode_CompKeyValWithItem(key, node, m,
                                                ttree) != 0)
                                    break;
                                m--;
                            }

#ifdef INDEX_BUFFERING
                            p->self_ = node->self_;
#endif
                            p->node_ = (struct TTreeNode *) node;
                            p->idx_ = m + 1;
                            ret = DB_SUCCESS;
                            goto end;

                        }
                        else if (comp_res > 0)
                        {
                            f = m + 1;
                        }
                        else
                        {
                            t = m - 1;
                        }
                    }

#ifdef INDEX_BUFFERING
                    p->self_ = NULL_OID;
#endif
                    p->node_ = NULL;
                    ret = DB_E_NOMORERECORDS;
                    goto end;
                }
            }
        }
    }

#ifdef INDEX_BUFFERING
    p->self_ = NULL_OID;
#endif
    p->node_ = NULL;
    return DB_E_NOMORERECORDS;

  end:
#ifdef INDEX_BUFFERING
    UnsetBufIndexSegment(issys_node);
#endif
    return ret;
}

//////////////////////////////////////////////////////////////////
//
// Function Name :
//
//////////////////////////////////////////////////////////////////
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
TTree_Get_Last_Eq(struct TTree *ttree, struct KeyValue *key,
        struct TTreePosition *p)
{
    struct TTreeNode *node;
    int comp_res;
    int bucketnum;
    int ret;
    NodeID node_id;

#ifdef INDEX_BUFFERING
    int issys_node = 0;
    OID oid_node;
#endif

    bucketnum = 0;

    node_id = ttree->root_[bucketnum];
    node = (struct TTreeNode *) Index_OID_Point(node_id);

    if (node == NULL)
    {
#ifdef INDEX_BUFFERING
        p->self_ = NULL_OID;
#endif
        p->node_ = NULL;
        return DB_E_NOMORERECORDS;
    }

    if (node->item_[0] == NULL_OID)
    {
        MDB_SYSLOG(("Index error: remake index! %d, last eq\n", bucketnum));
#ifdef INDEX_BUFFERING
        p->self_ = NULL_OID;
#endif
        p->node_ = NULL;
        return DB_FAIL;
    }

#ifdef INDEX_BUFFERING
    oid_node = ttree->root_[bucketnum];
#endif

    while (node != NULL)
    {
#ifdef INDEX_BUFFERING
        SetBufIndexSegment(issys_node, oid_node);
#endif
        if (TTreeNode_CompKeyValWithMinimum(key, node, ttree) < 0)
        {
#ifdef INDEX_BUFFERING
            UnsetBufIndexSegment(issys_node);
            oid_node = node->left_;
#endif
            node = (struct TTreeNode *) Index_OID_Point(node->left_);
        }
        else
        {
            if (TTreeNode_CompKeyValWithMaximum(key, node, ttree) > 0)
            {
#ifdef INDEX_BUFFERING
                UnsetBufIndexSegment(issys_node);
                oid_node = node->right_;
#endif
                node = (struct TTreeNode *) Index_OID_Point(node->right_);
            }
            else
            {
                if (TTreeNode_CompKeyValWithMaximum(key, node, ttree) == 0)
                {
                    struct TTreeNode *lub;

#ifdef INDEX_BUFFERING
                    int issys_lub = 0;
                    OID oid_lub;
#endif

                    lub = (struct TTreeNode *) TTree_LubChild(node);
                    if (lub == NULL)
                    {
#ifdef INDEX_BUFFERING
                        p->self_ = node->self_;
#endif
                        p->node_ = (struct TTreeNode *) node;
                        p->idx_ = node->item_count_ - 1;
                        ret = DB_SUCCESS;
                        goto end;
                    }
                    else
                    {
#ifdef INDEX_BUFFERING
                        oid_lub = lub->self_;
                        SetBufIndexSegment(issys_lub, oid_lub);
                        ret = TTreeNode_CompKeyValWithMinimum(key, lub, ttree);
                        UnsetBufIndexSegment(issys_lub);
                        if (ret == 0)
#else
                        if (TTreeNode_CompKeyValWithMinimum(key, lub,
                                        ttree) == 0)
#endif
                        {
#ifdef INDEX_BUFFERING
                            UnsetBufIndexSegment(issys_node);
                            oid_node = node->right_;
#endif
                            node = (struct TTreeNode *) Index_OID_Point(node->
                                    right_);
                        }
                        else
                        {
#ifdef INDEX_BUFFERING
                            p->self_ = node->self_;
#endif
                            p->node_ = (struct TTreeNode *) node;
                            p->idx_ = node->item_count_ - 1;
                            ret = DB_SUCCESS;
                            goto end;
                        }
                    }
                }
                else
                {
                    /* [0] <= key < [last] */

                    DB_INT16 f, t, m;

                    f = 0;
                    t = node->item_count_ - 2;

                    /* binary search */
                    while (1)
                    {
                        if (f > t)
                            break;

                        m = (f + t) >> 1;

                        comp_res = TTreeNode_CompKeyValWithItem(key, node, m,
                                ttree);
                        if (comp_res == 0)
                        {       /* 같으면 첫번째 레코드를 찾는다 */
                            m++;
                            while (m <= t)
                            {
                                if (TTreeNode_CompKeyValWithItem(key, node, m,
                                                ttree) != 0)
                                    break;
                                m++;
                            }

#ifdef INDEX_BUFFERING
                            p->self_ = node->self_;
#endif
                            p->node_ = (struct TTreeNode *) node;
                            p->idx_ = m - 1;
                            ret = DB_SUCCESS;
                            goto end;

                        }
                        else if (comp_res > 0)
                        {
                            f = m + 1;
                        }
                        else
                        {
                            t = m - 1;
                        }
                    }

#ifdef INDEX_BUFFERING
                    p->self_ = NULL_OID;
#endif
                    p->node_ = NULL;
                    ret = DB_E_NOMORERECORDS;
                    goto end;
                }
            }
        }
    }

#ifdef INDEX_BUFFERING
    p->self_ = NULL_OID;
#endif
    p->node_ = NULL;
    return DB_E_NOMORERECORDS;

  end:
#ifdef INDEX_BUFFERING
    UnsetBufIndexSegment(issys_node);
#endif
    return ret;
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
int
TTree_Get_First_Eq_Unique(struct TTree *ttree, struct KeyValue *key,
        struct TTreePosition *p)
{
    struct TTreeNode *node;
    int comp_res;
    int bucketnum;
    int ret;

#ifdef INDEX_BUFFERING
    int issys_node = 0;
    OID oid_node;
#endif
    NodeID node_id;

    bucketnum = 0;

#ifdef INDEX_BUFFERING
    oid_node = ttree->root_[bucketnum];
#endif

    node_id = ttree->root_[bucketnum];
    node = (struct TTreeNode *) Index_OID_Point(node_id);

    while (node != NULL)
    {
#ifdef INDEX_BUFFERING
        SetBufIndexSegment(issys_node, oid_node);
#endif

        comp_res = TTreeNode_CompKeyValWithMinimum(key, node, ttree);
        if (comp_res == 0)
        {
#ifdef INDEX_BUFFERING
            p->self_ = node->self_;
#endif
            p->node_ = (struct TTreeNode *) node;
            p->idx_ = 0;
            ret = DB_SUCCESS;
            goto end;
        }
        else if (comp_res < 0)
        {
#ifdef INDEX_BUFFERING
            UnsetBufIndexSegment(issys_node);
            oid_node = node->left_;
#endif
            node = (struct TTreeNode *) Index_OID_Point(node->left_);
        }
        else    /* (comp_res > 0) */
        {
            comp_res = TTreeNode_CompKeyValWithMaximum(key, node, ttree);
            if (comp_res == 0)
            {
#ifdef INDEX_BUFFERING
                p->self_ = node->self_;
#endif
                p->node_ = (struct TTreeNode *) node;
                p->idx_ = node->item_count_ - 1;
                ret = DB_SUCCESS;
                goto end;
            }
            else if (comp_res > 0)
            {
#ifdef INDEX_BUFFERING
                UnsetBufIndexSegment(issys_node);
                oid_node = node->right_;
#endif
                node = (struct TTreeNode *) Index_OID_Point(node->right_);
            }
            else
            {   /* node 내 binary search */
                DB_INT16 f, t, m;

                f = 1;
                t = node->item_count_ - 2;

                /* binary search */
                while (1)
                {
                    if (f > t)
                        break;

                    m = (f + t) >> 1;

                    comp_res =
                            TTreeNode_CompKeyValWithItem(key, node, m, ttree);
                    if (comp_res == 0)
                    {
#ifdef INDEX_BUFFERING
                        p->self_ = node->self_;
#endif
                        p->node_ = (struct TTreeNode *) node;
                        p->idx_ = m;
                        ret = DB_SUCCESS;
                        goto end;
                    }
                    else if (comp_res > 0)
                        f = m + 1;
                    else
                        t = m - 1;
                }

#ifdef INDEX_BUFFERING
                p->self_ = NULL_OID;
#endif
                p->node_ = NULL;
                ret = DB_E_NOMORERECORDS;
                goto end;
            }
        }
    }
#ifdef INDEX_BUFFERING
    p->self_ = NULL_OID;
#endif
    p->node_ = NULL;
    return DB_E_NOMORERECORDS;

  end:
#ifdef INDEX_BUFFERING
    UnsetBufIndexSegment(issys_node);
#endif
    return ret;
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
int
TTree_Get_Offset(struct TTree *ttree, struct TTreePosition *p)
{
    int item_count = 0;
    struct TTreeNode *node;

    node = TTree_SmallestChild(ttree);

    while (node)
    {
#ifdef INDEX_BUFFERING
        /* 아래 TTree_NextNode() 수행시 버퍼 교체로 인하여 p.node_의 내용이
         * 변경될 수 있으므로 p->self_를 비교하는 것으로 변경함.
         * index buffering에서는 node의 oid(self_)를 비교해야 함.*/
        if (node->self_ == p->self_)
#else
        if (node == p->node_)
#endif
        {
            item_count += p->idx_;
            break;
        }

        item_count += node->item_count_;

        node = TTree_NextNode(node);
    }

    if (node == NULL)
    {
        MDB_SYSLOG(("p = %x, p->node_ = %x, p->idx_ = %d\n",
                        (unsigned long) p, p->node_, p->idx_));

        return DB_E_CURSORNODE;
    }

    return item_count;
}

int
TTree_Get_Position(struct TTree *ttree, OID rid, char type, int rec_size,
        struct TTreePosition *curr)
{
    struct KeyValue keyValue;
    int result;
    char *record_to_find = NULL;

    record_to_find = OID_Point(rid);
    if (record_to_find == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    result = KeyValue_Create(record_to_find, &ttree->key_desc_,
            rec_size, &keyValue, 0);
    if (result < 0)
    {
        MDB_SYSLOG(("KeyValue Create FAIL: %d\n", result));
        return result;
    }

    if (type == TTREE_FORWARD_CURSOR)
        result = TTree_Get_First_Ge(ttree, &keyValue, curr);
    else    /* TTREE_BACKWARD_CURSOR */
        result = TTree_Get_Last_Le(ttree, &keyValue, curr);

    KeyValue_Delete2(&keyValue);

    return result;
}

/* return
 *  -1: rebuild the indexes of the table
 *  -2: rebuild the indexes of all the tables */
int
TTree_Get_ItemCount(struct TTree *ttree)
{
    int item_count = 0;
    struct TTreeNode *node;
    int i;
    int has_next;
    int root_check;
    int ret;
    int max_node;

    /* index segment 총 갯수에 들어갈 수 있는 최대 node 수 계산
     * item_count가 max_node보다 크다면 오류 발생 */
    max_node = (mem_anchor_->iallocated_segment_no_ * SEGMENT_SIZE)
            / sizeof(struct TTreeNode) * TNODE_MAX_ITEM;

    node = TTree_SmallestChild(ttree);

    /* In the case of the index file crash, TTree_SmallestChild() returns
     * NULL. We determine the case as that ttree has the root node, but
     * NULL returned from TTree_SmallestChild. */
    if (node == NULL)
    {
        if (ttree->root_[0])
            return -2;
        else
            return 0;
    }
    root_check = 0;
    ret = 0;

    while (node)
    {
        if (node->self_ == 0)
            return -2;
        if (TTreeNode_Type_Check(node))
            return -2;
        /* Check the number of the root node. */
        if (node->type_ == ROOT_NODE)
        {
            root_check++;
            if (root_check > 1)
                return -2;
        }
        if (node->item_count_ < 0 || node->item_count_ > TNODE_MAX_ITEM)
            return -2;
        /* check the last rid is null */
        if (node->item_count_
                && node->item_[node->item_count_ - 1] == NULL_OID)
        {
            ret = -1;
            break;
        }

        for (i = 0; i < node->item_count_; i++)
        {
            if (isInvalidRid(node->item_[i]))
            {
                ret = -1;
                break;
            }
        }

        item_count += node->item_count_;

        if (item_count > max_node)
            return -2;
        if (node->type_ == LEFT_CHILD)
            has_next = 1;
        else
            has_next = 0;

        node = TTree_NextNode(node);
        if (node == NULL && has_next)
            return -2;
    }

    if (root_check != 1)
        return -2;

    if (ret == 0)
        return item_count;

    return -1;
}

int
TTree_Get_ItemCount_Interval(struct TTree *ttree,
        struct TTreePosition *first, struct TTreePosition *last, int type)
{
    int item_count = 0;
    struct TTreePosition p;

    if (type == TTREE_FORWARD_CURSOR)
    {
        p = *first;

        while (p.node_)
        {
            if (p.node_ == last->node_)
            {
                item_count += (last->idx_ - p.idx_ + 1);
                break;
            }

            item_count += (p.node_->item_count_ - p.idx_);

            p.node_ = TTree_NextNode(p.node_);
            p.idx_ = 0;
        }
    }
    else if (type == TTREE_FORWARD_CURSOR)
    {
        p = *first;

        while (p.node_)
        {
            if (p.node_ == last->node_)
            {
                item_count += (p.idx_ - last->idx_ + 1);
                break;
            }

            item_count += (p.idx_ + 1);

            p.node_ = TTree_PrevNode(p.node_);
            p.idx_ = p.node_->item_count_;
        }
    }
    else
    {
        return DB_E_NOTSUPPORTED;
    }

    return item_count;
}

int
TTree_Seek_Offset(struct TTree *ttree, struct TTreePosition *p, int offset)
{
    int item_count = 0;
    struct TTreeNode *node;

    if (offset < 0)
        return DB_E_INVALIDPARAM;

    node = TTree_SmallestChild(ttree);

    while (node)
    {
        if (item_count + node->item_count_ > offset)
        {
#ifdef INDEX_BUFFERING
            p->self_ = node->self_;
#endif
            p->node_ = node;
            p->idx_ = offset - item_count;
            break;
        }

        item_count += node->item_count_;
        node = TTree_NextNode(node);
    }

    if (node == NULL)
    {
        return DB_E_CURSOROFFSET;
    }

    return DB_SUCCESS;
}

////////////////////////////////////////////////////////////////
//
// Function Name :
//
////////////////////////////////////////////////////////////////
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
TTree_Increment(struct TTreePosition *p)
{
    if (p->node_ == NULL)
        return DB_E_INVALIDPARAM;

    if (p->idx_ < p->node_->item_count_ - 1)
    {
        p->idx_++;
        return DB_SUCCESS;
    }
    else
    {
        p->node_ = (struct TTreeNode *) TTree_NextNode(p->node_);
        if (p->node_ == NULL)
        {
#ifdef INDEX_BUFFERING
            p->self_ = NULL_OID;
#endif
            return DB_E_TTREEPOSITION;
        }
        else
        {
#ifdef INDEX_BUFFERING
            p->self_ = p->node_->self_;
#endif
            p->idx_ = 0;
            return DB_SUCCESS;
        }
    }
}

////////////////////////////////////////////////////////////////
//
// Function Name :
//
////////////////////////////////////////////////////////////////
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
TTree_Decrement(struct TTreePosition *p)
{
    if (p->node_ == NULL)
        return DB_FAIL;

    if (p->idx_ > 0)
    {
        p->idx_--;
        return DB_SUCCESS;
    }
    else
    {
        p->node_ = (struct TTreeNode *) TTree_PrevNode(p->node_);
        if (p->node_ == NULL)
        {
#ifdef INDEX_BUFFERING
            p->self_ = NULL_OID;
#endif
            return DB_FAIL;
        }
        else
        {
#ifdef INDEX_BUFFERING
            p->self_ = p->node_->self_;
#endif
            p->idx_ = p->node_->item_count_ - 1;
            return DB_SUCCESS;
        }
    }
}


//////////////////////////////////////////////////////////////////
//
// Function Name :
//
//////////////////////////////////////////////////////////////////
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
TTree_Merge_With_Left_Child(struct TTree *ttree, struct TTreeNode *curr_node,
        int bucketnum)
{
    struct TTreeNode *left_node;
    DB_INT16 left_size;

#ifdef INDEX_BUFFERING
    int issys_node = 0;
    int oid_node;
#endif

    left_node = (struct TTreeNode *) Index_OID_Point(curr_node->left_);
    if (left_node == NULL)
    {
        return DB_E_OIDPOINTER;
    }
#ifdef INDEX_BUFFERING
    oid_node = curr_node->left_;
    SetBufIndexSegment(issys_node, oid_node);
#endif
    left_size = left_node->item_count_;

    if ((curr_node->item_count_ + left_size) <= TNODE_MAX_ITEM)
    {
        TTreeNode_MoveLeftToRight(left_node, curr_node, left_size, ttree);

        curr_node->left_ = NULL_OID;
        curr_node->height_ = 0;

        if (Index_Col_Remove(&ttree->collect_, left_node->self_) < 0)
        {
            MDB_SYSLOG(("TTree remove() FAIL.\n"));
#ifdef INDEX_BUFFERING
            UnsetBufIndexSegment(issys_node);
#endif
            return DB_FAIL;
        }

        Index_SetDirtyFlag(curr_node->self_);

#ifdef INDEX_BUFFERING
        UnsetBufIndexSegment(issys_node);
#endif

        return TTree_Rebalance_For_Delete(ttree, curr_node, bucketnum);
    }
    else
    {
        DB_INT16 delta;

        delta = TNODE_MAX_ITEM - curr_node->item_count_;

        TTreeNode_MoveLeftToRight(left_node, curr_node, delta, ttree);

        Index_SetDirtyFlag(curr_node->self_);
        Index_SetDirtyFlag(left_node->self_);

#ifdef INDEX_BUFFERING
        UnsetBufIndexSegment(issys_node);
#endif

        return DB_SUCCESS;
    }
}

//////////////////////////////////////////////////////////////////
//
// Function Name :
//
//////////////////////////////////////////////////////////////////
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
TTree_Merge_With_Right_Child(struct TTree *ttree, struct TTreeNode *curr_node,
        int bucketnum)
{
#ifdef INDEX_BUFFERING
    int issys_node = 0;
    int oid_node;
#endif
    struct TTreeNode *right_node;

    right_node = (struct TTreeNode *) Index_OID_Point(curr_node->right_);
    if (right_node == NULL)
    {
        return DB_E_OIDPOINTER;
    }

#ifdef INDEX_BUFFERING
    oid_node = curr_node->right_;
    SetBufIndexSegment(issys_node, oid_node);
#endif

    if ((curr_node->item_count_ + right_node->item_count_) <= TNODE_MAX_ITEM)
    {
        TTreeNode_MoveRightToLeft(curr_node, right_node,
                right_node->item_count_, ttree);
        curr_node->right_ = NULL_OID;
        curr_node->height_ = 0;

        Index_Col_Remove(&ttree->collect_, right_node->self_);

        Index_SetDirtyFlag(curr_node->self_);

#ifdef INDEX_BUFFERING
        UnsetBufIndexSegment(issys_node);
#endif

        return TTree_Rebalance_For_Delete(ttree, curr_node, bucketnum);
    }
    else
    {
        DB_INT16 delta;

        delta = TNODE_MAX_ITEM - curr_node->item_count_;

        TTreeNode_MoveRightToLeft(curr_node, right_node, delta, ttree);

        Index_SetDirtyFlag(curr_node->self_);
        Index_SetDirtyFlag(right_node->self_);

#ifdef INDEX_BUFFERING
        UnsetBufIndexSegment(issys_node);
#endif

        return DB_SUCCESS;
    }
}

//////////////////////////////////////////////////////////////////
//
// Function Name :
//
//////////////////////////////////////////////////////////////////
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
TTree_Rebalance_For_Delete(struct TTree *ttree, struct TTreeNode *node,
        int bucketnum)
{
    struct TTreeNode *L;
    struct TTreeNode *R;
    Height left_height, right_height, height;
    DB_INT16 delta;

#ifdef INDEX_BUFFERING
    int issys_node = 0;
    OID oid_node = NULL_OID;
    OID L_left, L_right, R_left, R_right;
#endif
    int ret;

    if (node == NULL)
        return DB_FAIL;

    for (;;)
    {
        if (node->left_ != NULL_OID)
        {
            L = (struct TTreeNode *) Index_OID_Point(node->left_);
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
                Height height_LL, height_LR;

                node_L = (struct TTreeNode *) Index_OID_Point(L_right);
                if (node_L == NULL)
                {
                    TTree_LL_Rotate(ttree, node, bucketnum);
                }
                else
                {
                    height_LR = node_L->height_;

                    node_L = (struct TTreeNode *) Index_OID_Point(L_left);
                    if (node_L == NULL)
                    {
                        TTree_LR_Rotate(ttree, node, bucketnum);
                    }
                    else
                    {
                        height_LL = node_L->height_;

                        if (height_LL >= height_LR)
                            TTree_LL_Rotate(ttree, node, bucketnum);
                        else
                            TTree_LR_Rotate(ttree, node, bucketnum);
                    }
                }
#else
                struct TTreeNode *LL;
                struct TTreeNode *LR;

                LL = (struct TTreeNode *) Index_OID_Point(L->left_);
                LR = (struct TTreeNode *) Index_OID_Point(L->right_);
                if (LR == NULL)
                    TTree_LL_Rotate(ttree, node, bucketnum);
                else if (LL == NULL)
                    TTree_LR_Rotate(ttree, node, bucketnum);
                else if (LL->height_ >= LR->height_)
                    TTree_LL_Rotate(ttree, node, bucketnum);
                else
                    TTree_LR_Rotate(ttree, node, bucketnum);
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
                    TTree_RR_Rotate(ttree, node, bucketnum);
                }
                else
                {
                    height_RL = node_R->height_;

                    node_R = (struct TTreeNode *) Index_OID_Point(R_right);
                    if (node_R == NULL)
                    {
                        TTree_RL_Rotate(ttree, node, bucketnum);
                    }
                    else
                    {
                        height_RR = node_R->height_;

                        if (height_RR >= height_RL)
                            TTree_RR_Rotate(ttree, node, bucketnum);
                        else
                            TTree_RL_Rotate(ttree, node, bucketnum);
                    }
                }
#else
                struct TTreeNode *RL;
                struct TTreeNode *RR;

                RL = (struct TTreeNode *) Index_OID_Point(R->left_);
                RR = (struct TTreeNode *) Index_OID_Point(R->right_);

                if (RL == NULL_OID)
                    TTree_RR_Rotate(ttree, node, bucketnum);
                else if (RR == NULL)
                    TTree_RL_Rotate(ttree, node, bucketnum);
                else if (RR->height_ >= RL->height_)
                    TTree_RR_Rotate(ttree, node, bucketnum);
                else
                    TTree_RL_Rotate(ttree, node, bucketnum);
#endif
            }
        }

        if (node->parent_ == NULL_OID)
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
#endif
            node = (struct TTreeNode *) Index_OID_Point(node->parent_);
        }
    }       /* for */

  end:
#ifdef INDEX_BUFFERING
    UnsetBufIndexSegment(issys_node);
#endif

    return ret;
}

//////////////////////////////////////////////////////////////////
//
// Function Name :
//
// unique index에서 같은 값의 레코드가 이미 들어 있는지를 점검하는 목적
//
// 비교 대상이 되는 record_to_find 레코드는 key 값에 NULL을
// 포함하지 않는 상태이어야 함. 즉, 미리 점검해서 들어와야 함.
// Check_NullFields().
//////////////////////////////////////////////////////////////////
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
TTree_IsExist(struct TTree *ttree, char *record_to_find,
        OID record_id, int record_size, int f_memory_record)
{
    struct KeyValue keyValue;
    struct TTreePosition p1;    //, p2;
    int result;

    /* memory record size for Check_NullFields */
    if (f_memory_record == 0 && record_size != ttree->collect_.record_size_)
    {
        MDB_SYSLOG(("Warn: record size mismatch! %d %d\n",
                        record_size, ttree->collect_.record_size_));

        ttree->collect_.record_size_ = (DB_INT16) record_size;
    }

    result = KeyValue_Create(record_to_find, &ttree->key_desc_,
            record_size, &keyValue, f_memory_record);
    if (result < 0)
    {
        MDB_SYSLOG(("KeyValue Create FAIL: %d\n", result));
        return FALSE;
    }

    if (TTree_Get_First_Eq(ttree, &keyValue, &p1) < 0)
    {       /* 같은 값이 없음 */
        result = FALSE;
    }
    else
    {
#ifdef INDEX_BUFFERING
        p1.node_ = (struct TTreeNode *) Index_OID_Point(p1.self_);
#endif
        /* 같은 값 존재 */
        if (record_id != NULL_OID)      /* update인 경우 */
        {
            /* update하려는 레코드가 자신이라면 OK */
            if (p1.node_->item_[p1.idx_] == record_id)
                result = FALSE;
            else
            {
                result = p1.node_->item_[p1.idx_];
            }
        }
        else    /* insert인 경우 */
        {
            result = p1.node_->item_[p1.idx_];
        }
    }


    KeyValue_Delete2(&keyValue);

    return result;
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
int
TTree_Get_NodeCount(struct TTree *ttree)
{
    int node_count = 0;
    struct TTreeNode *node;

    node = TTree_SmallestChild(ttree);

    while (node)
    {
        node_count++;

        node = TTree_NextNode(node);
    }

    return node_count;
}
