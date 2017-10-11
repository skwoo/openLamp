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
#include "mdb_Csr.h"
#include "mdb_TTreeIter.h"
#include "mdb_Mem_Mgr.h"
#include "mdb_dbi.h"
#include "mdb_PMEM.h"
#include "sql_mclient.h"

int Filter_Compare(struct Filter *, char *, int);
int TTree_Increment(struct TTreePosition *);
struct TTreeNode *TTree_SmallestChild(struct TTree *);
struct TTreeNode *TTree_GreatestChild(struct TTree *);
struct TTreeNode *TTree_NextNode(struct TTreeNode *);
struct TTreeNode *TTree_PrevNode(struct TTreeNode *);
int KeyValue_Compare(struct KeyValue *, char *, int);
int TTree_Decrement(struct TTreePosition *p);
int KeyValue_Create(const char *record, struct KeyDesc *key_desc,
        int recSize, struct KeyValue *keyValue, int f_memory_record);
int KeyValue_Delete2(struct KeyValue *pKeyValue);
int KeyValue_Check(struct KeyValue *min, struct KeyValue *max);

/////////////////////////////////////////////////////////////////
//
// Function Name :
//
/////////////////////////////////////////////////////////////////

/*****************************************************************************/
//! TTreeIter_Create
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param ttreeiter(in/out) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
TTreeIter_Create(struct TTreeIterator *ttreeiter)
{
    DB_BOOL use_min_key;
    DB_BOOL use_max_key;
    DB_BOOL use_filter;
    char *record;
    OID record_id;
    int issys;
    int comp_res;
    int ret = DB_SUCCESS;
    struct TTreePosition first, last;
    struct Container *Cont = (struct Container *)
            OID_Point(ttreeiter->iterator_.cont_id_);
    int issys_cont;

    SetBufSegment(issys_cont, Cont->collect_.cont_id_);

#ifdef INDEX_BUFFERING
    first.self_ = NULL_OID;
    last.self_ = NULL_OID;
#endif
    first.node_ = NULL;
    last.node_ = NULL;

#ifdef INDEX_BUFFERING
    ttreeiter->first_.self_ = NULL_OID;
    ttreeiter->last_.self_ = NULL_OID;
    ttreeiter->curr_.self_ = NULL_OID;
#endif
    ttreeiter->first_.node_ = NULL;
    ttreeiter->last_.node_ = NULL;
    ttreeiter->curr_.node_ = NULL;

    if (ttreeiter->iterator_.min_.field_count_ == 0)
        use_min_key = FALSE;
    else
        use_min_key = TRUE;

    if (ttreeiter->iterator_.max_.field_count_ == 0)
        use_max_key = FALSE;
    else
        use_max_key = TRUE;

    if (ttreeiter->iterator_.filter_.expression_count_ == 0)
        use_filter = FALSE;
    else
        use_filter = TRUE;

    /* In this case, do not apply keyfilter. It will be applied, later. */
    ret = TTree_Set_First(use_min_key, FALSE, &first, ttreeiter);
    if (ret < 0)
    {
        goto end;
    }

    if (ttreeiter->is_unique_ == TRUE &&
            KeyValue_Check(&ttreeiter->iterator_.min_,
                    &ttreeiter->iterator_.max_) == DB_SUCCESS)
    {
#ifdef INDEX_BUFFERING
        first.node_ = (struct TTreeNode *) Index_OID_Point(first.self_);
        if (first.node_ == NULL)
        {
            ret = DB_E_OIDPOINTER;
            goto end;
        }
#endif
        record_id = first.node_->item_[first.idx_];
        record = (char *) OID_Point(record_id);
        if (record == NULL)
        {
            ret = DB_E_OIDPOINTER;
            goto end;
        }

        SetBufSegment(issys, record_id);

        comp_res = KeyValue_Compare(&(ttreeiter->iterator_.min_), record,
                ttreeiter->iterator_.record_size_);

        UnsetBufSegment(issys);

        if (comp_res == 0)
        {
            last = first;
            goto done;
        }
    }

    /* In this case, do not apply keyfilter. It will be applied, later. */
    ret = TTree_Set_Last(use_max_key, FALSE, &last, ttreeiter);
    if (ret < 0)
    {
        goto end;
    }

  done:
    if (use_min_key &&
            ttreeiter->iterator_.min_.field_value_[0].mode_ != MDB_AL)
    {
#ifdef INDEX_BUFFERING
        last.node_ = (struct TTreeNode *) Index_OID_Point(last.self_);
        if (last.node_ == NULL)
        {
            ret = DB_E_OIDPOINTER;
            goto end;
        }
#endif
        record_id = last.node_->item_[last.idx_];
        record = (char *) OID_Point(record_id);
        if (record == NULL)
        {
            ret = DB_E_OIDPOINTER;
            goto end;
        }

        SetBufSegment(issys, record_id);

        comp_res = KeyValue_Compare(&(ttreeiter->iterator_.min_), record,
                ttreeiter->iterator_.record_size_);

        UnsetBufSegment(issys);

        if (comp_res > 0)
        {
            ret = DB_E_NOMORERECORDS;
            goto end;
        }
    }

    if (ttreeiter->iterator_.type_ == TTREE_FORWARD_CURSOR)
    {
        ttreeiter->first_ = first;
        ttreeiter->last_ = last;
        ttreeiter->curr_ = first;
    }
    else
    {
        ttreeiter->first_ = first;
        ttreeiter->last_ = last;
        ttreeiter->curr_ = last;
    }

    /* apply key filter on the current key */
    if (use_filter)
    {
#ifdef INDEX_BUFFERING
        ttreeiter->curr_.node_ = (struct TTreeNode *)
                Index_OID_Point(ttreeiter->curr_.self_);
        if (ttreeiter->curr_.node_ == NULL)
        {
            ret = DB_E_OIDPOINTER;
            goto end;
        }
#endif
        record_id = ttreeiter->curr_.node_->item_[ttreeiter->curr_.idx_];
        record = (char *) OID_Point(record_id);
        if (record == NULL)
        {
            ret = DB_E_OIDPOINTER;
            goto end;
        }

        SetBufSegment(issys, record_id);

        comp_res = Filter_Compare(&(ttreeiter->iterator_.filter_), record,
                ttreeiter->iterator_.record_size_);

        UnsetBufSegment(issys);

        if (comp_res < 0)
        {
            if (Iter_FindNext((struct Iterator *) ttreeiter, NULL_OID) < 0)
            {
#ifdef INDEX_BUFFERING
                ttreeiter->curr_.self_ = NULL_OID;
#endif
                ttreeiter->curr_.node_ = NULL;
                ret = DB_E_NOMORERECORDS;
                goto end;
            }
        }
    }

    ret = DB_SUCCESS;
  end:
    UnsetBufSegment(issys_cont);
    return ret;
}

/////////////////////////////////////////////////////////////////
//
// Function Name :
//
/////////////////////////////////////////////////////////////////

/*****************************************************************************/
//! TTree_Set_First
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param use_min_key(in)   :
 * \param use_filter(in)    : 
 * \param first(in/out)     :
 * \param ttreeiter(in/out) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
TTree_Set_First(DB_BOOL use_min_key, DB_BOOL use_filter,
        struct TTreePosition *first, struct TTreeIterator *ttreeiter)
{
    int issys;
    OID record_id;
    char *record;
    struct KeyValue *min;
    struct Filter *f;
    struct TTree *ttree;
    DB_UINT32 utime;
    DB_UINT16 qsid;
    register int utime_offset, qsid_offset;
    int ret = DB_SUCCESS;
    int issys_ttree;

    min = &(ttreeiter->iterator_.min_);
    f = &(ttreeiter->iterator_.filter_);
    ttree = (struct TTree *) OID_Point(ttreeiter->tree_oid_);

    if (use_min_key == TRUE && min->field_value_[0].mode_ != MDB_AL)
    {
        SetBufSegment(issys_ttree, ttreeiter->tree_oid_);
        ret = TTree_Get_First_Ge(ttree, min, first);
        UnsetBufSegment(issys_ttree);
        if (ret < 0)
            goto end;
    }
    else
    {
        first->node_ = (struct TTreeNode *) TTree_SmallestChild(ttree);
        first->idx_ = 0;
        if (first->node_ == NULL)
        {
#ifdef INDEX_BUFFERING
            first->self_ = NULL_OID;
#endif
            ret = DB_FAIL;
            goto end;
        }
#ifdef INDEX_BUFFERING
        first->self_ = first->node_->self_;
#endif
    }

    utime_offset = ttreeiter->iterator_.slot_size_ - 8;
    qsid_offset = ttreeiter->iterator_.slot_size_ - 4;

    /* 레코드의 utime이 cursor open 시간 이전 것을 찾는다 */
    while (1)
    {
#ifdef INDEX_BUFFERING
        first->node_ = (struct TTreeNode *) Index_OID_Point(first->self_);
        if (first->node_ == NULL)
        {
            ret = DB_E_OIDPOINTER;
            goto end;
        }
#endif
        record_id = first->node_->item_[first->idx_];
        record = (char *) OID_Point(record_id);
        if (record == NULL)
        {
            ret = DB_E_OIDPOINTER;
            goto end;
        }

        SetBufSegment(issys, record_id);

        utime = *(DB_UINT32 *) (record + utime_offset);
        qsid = *(DB_UINT16 *) (record + qsid_offset);

        /* table lock이므로 s-x, x-x lock이 걸릴 수 없어
           != 만 점검을 해도 될 듯 */

        /* insert into..as select.. 가 발생하는 경우, 새로
           insert된(같은 cursor 내에서) 레코드는 skip하기 위해서 */
        if (utime == ttreeiter->iterator_.open_time_ &&
                ttreeiter->iterator_.qsid_ == qsid)
        {
        }
        else
        {
            if (use_filter == FALSE)
            {
                UnsetBufSegment(issys);
                ret = DB_SUCCESS;
                goto end;
            }

            if (Filter_Compare(f, record,
                            ttreeiter->iterator_.record_size_) == DB_SUCCESS)
            {
                UnsetBufSegment(issys);
                ret = DB_SUCCESS;
                goto end;
            }
        }
        UnsetBufSegment(issys);
        if (TTree_Increment(first) < 0)
        {
            ret = DB_FAIL;
            goto end;
        }
    }

  end:
    return ret;
}

/*****************************************************************************/
//! TTree_Set_Last 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param use_max_key(in)   :
 * \param use_filter(in)    : 
 * \param last(in/out)      :
 * \param ttreeiter():
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
TTree_Set_Last(DB_BOOL use_max_key, DB_BOOL use_filter,
        struct TTreePosition *last, struct TTreeIterator *ttreeiter)
{
    int issys;
    OID record_id;
    char *record;
    struct KeyValue *max;
    struct Filter *f;
    int ret = DB_SUCCESS;
    struct TTree *ttree;
    int issys_ttree;

    DB_UINT32 utime;
    DB_UINT16 qsid;
    int utime_offset, qsid_offset;

    max = &(ttreeiter->iterator_.max_);
    f = &(ttreeiter->iterator_.filter_);
    ttree = (struct TTree *) OID_Point(ttreeiter->tree_oid_);

    if (use_max_key == TRUE && max->field_value_[0].mode_ != MDB_AL)
    {
        SetBufSegment(issys_ttree, ttreeiter->tree_oid_);
        ret = TTree_Get_Last_Le(ttree, max, last);
        UnsetBufSegment(issys_ttree);

        if (ret < 0)
            goto end;
    }
    else
    {
        last->node_ = (struct TTreeNode *) TTree_GreatestChild(ttree);
        if (last->node_ == NULL)
        {
#ifdef INDEX_BUFFERING
            last->self_ = NULL_OID;
#endif
            ret = DB_FAIL;
            goto end;
        }
#ifdef INDEX_BUFFERING
        last->self_ = last->node_->self_;
#endif
        last->idx_ = last->node_->item_count_ - 1;
    }

    utime_offset = ttreeiter->iterator_.slot_size_ - 8;
    qsid_offset = ttreeiter->iterator_.slot_size_ - 4;

    /* 레코드의 utime이 cursor open 시간 이전 것을 찾는다 */
    while (1)
    {
#ifdef INDEX_BUFFERING
        last->node_ = (struct TTreeNode *) Index_OID_Point(last->self_);
        if (last->node_ == NULL)
        {
            ret = DB_E_OIDPOINTER;
            goto end;
        }
#endif
        record_id = last->node_->item_[last->idx_];
        record = (char *) OID_Point(record_id);
        if (record == NULL)
        {
            ret = DB_E_OIDPOINTER;
            goto end;
        }

        SetBufSegment(issys, record_id);

        utime = *(DB_UINT32 *) (record + utime_offset);
        qsid = *(DB_UINT16 *) (record + qsid_offset);

        /* insert into..as select.. 가 발생하는 경우, 새로
           insert된(같은 cursor 내에서) 레코드는 skip하기 위해서 */

        if (utime == ttreeiter->iterator_.open_time_ &&
                ttreeiter->iterator_.qsid_ == qsid)
        {
        }
        else
        {
            if (use_filter == FALSE)
            {
                UnsetBufSegment(issys);
                ret = DB_SUCCESS;
                goto end;
            }

            if (Filter_Compare(f, record,
                            ttreeiter->iterator_.record_size_) == DB_SUCCESS)
            {
                UnsetBufSegment(issys);
                ret = DB_SUCCESS;
                goto end;
            }
        }
        UnsetBufSegment(issys);
        if (TTree_Decrement(last) < 0)
        {
            ret = DB_FAIL;
            goto end;
        }
    }

  end:
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
int
TTree_Restart(struct TTreeIterator *ttreeiter)
{
    int issys;
    int comp_res;
    OID record_id;
    char *record;
    DB_BOOL use_filter;
    DB_BOOL use_min_key;
    DB_BOOL use_max_key;

    struct TTreePosition first, last;

#ifdef INDEX_BUFFERING
    ttreeiter->first_.self_ = NULL_OID;
    ttreeiter->last_.self_ = NULL_OID;
    ttreeiter->curr_.self_ = NULL_OID;
#endif
    ttreeiter->first_.node_ = NULL;
    ttreeiter->last_.node_ = NULL;
    ttreeiter->curr_.node_ = NULL;

    use_min_key = (ttreeiter->iterator_.min_.field_count_ == 0) ? FALSE : TRUE;
    use_max_key = (ttreeiter->iterator_.max_.field_count_ == 0) ? FALSE : TRUE;
    use_filter =
            (ttreeiter->iterator_.filter_.expression_count_ ==
            0) ? FALSE : TRUE;

    if (TTree_Set_First(use_min_key, use_filter, &first, ttreeiter) < 0)
    {
        return DB_FAIL;
    }

    if (ttreeiter->is_unique_ == TRUE &&
            KeyValue_Check(&ttreeiter->iterator_.min_,
                    &ttreeiter->iterator_.max_) == DB_SUCCESS)
    {
#ifdef INDEX_BUFFERING
        first.node_ = (struct TTreeNode *) Index_OID_Point(first.self_);
        if (first.node_ == NULL)
        {
            return DB_E_OIDPOINTER;
        }
#endif
        record_id = first.node_->item_[first.idx_];
        record = (char *) OID_Point(record_id);
        if (record == NULL)
        {
            return DB_E_OIDPOINTER;
        }

        SetBufSegment(issys, record_id);

        comp_res = KeyValue_Compare(&(ttreeiter->iterator_.min_), record,
                ttreeiter->iterator_.record_size_);

        UnsetBufSegment(issys);

        if (comp_res == 0)
        {
            last = first;
            goto done;
        }
    }

    if (TTree_Set_Last(use_max_key, use_filter, &last, ttreeiter) < 0)
    {
        return DB_FAIL;
    }

  done:
    if (use_min_key &&
            ttreeiter->iterator_.min_.field_value_[0].mode_ != MDB_AL)
    {
#ifdef INDEX_BUFFERING
        last.node_ = (struct TTreeNode *) Index_OID_Point(last.self_);
        if (last.node_ == NULL)
        {
            return DB_E_OIDPOINTER;
        }
#endif
        record_id = last.node_->item_[last.idx_];
        record = (char *) OID_Point(record_id);
        if (record == NULL)
        {
            return DB_E_OIDPOINTER;
        }

        SetBufSegment(issys, record_id);

        comp_res = KeyValue_Compare(&(ttreeiter->iterator_.min_), record,
                ttreeiter->iterator_.record_size_);

        UnsetBufSegment(issys);

        if (comp_res > 0)
        {
            return DB_FAIL;
        }
    }

    if (ttreeiter->iterator_.type_ == TTREE_FORWARD_CURSOR)
    {
        ttreeiter->first_ = first;
        ttreeiter->last_ = last;
        ttreeiter->curr_ = first;
    }
    else
    {
        ttreeiter->first_ = first;
        ttreeiter->last_ = last;
        ttreeiter->curr_ = last;
    }

    return DB_SUCCESS;
}

/* caller must free oidset->array */
/* oidset->array is not reused. */
int
TTree_GetOidSet(struct TTreeIterator *ttreeiter, MDB_OIDARRAY * oidset)
{
    int firstOffset, lastOffset, i, ret;
    int issys_ttree;
    struct TTree *ttree = (struct TTree *) OID_Point(ttreeiter->tree_oid_);
    struct TTreePosition curr = ttreeiter->curr_;

    SetBufSegment(issys_ttree, ttreeiter->tree_oid_);

    firstOffset = TTree_Get_Offset(ttree, &ttreeiter->curr_);
    if (firstOffset < 0)
    {
        UnsetBufSegment(issys_ttree);
        return DB_FAIL;
    }

    lastOffset = TTree_Get_Offset(ttree, &ttreeiter->last_);
    if (lastOffset < 0)
    {
        UnsetBufSegment(issys_ttree);
        return DB_FAIL;
    }

    UnsetBufSegment(issys_ttree);

    oidset->count = lastOffset - firstOffset + 1;

    if (oidset->count < 0)
    {
#ifdef MDB_DEBUG
        sc_assert(0, __FILE__, __LINE__);
#endif
        return DB_E_INVALIDPARAM;       /* do normal delete routine */
    }

    oidset->array = (OID *) PMEM_ALLOC(OID_SIZE * (oidset->count + 1));
    if (oidset->array == NULL)
        return DB_E_OUTOFMEMORY;

#ifdef INDEX_BUFFERING
    curr.node_ = (struct TTreeNode *) Index_OID_Point(curr.self_);
    if (curr.node_ == NULL)
        return DB_E_OIDPOINTER;
#endif

    for (i = 0; i < oidset->count; i++)
    {
        oidset->array[i] = curr.node_->item_[curr.idx_];

        if (i < oidset->count - 1 && (ret = TTree_Increment(&curr)) < 0)
            return ret;
    }

    oidset->array[i] = NULL_OID;        /* set NULL_OID next to last_oid */

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
TTree_Find_Next_Without_Filter(struct TTreePosition *curr,
        struct TTreePosition *last)
{
#ifdef MDB_DEBUG
#ifdef INDEX_BUFFERING
    if (curr->self_ == NULL_OID)
#else
    if (curr->node_ == NULL)
#endif
        return DB_FAIL;
#endif

#ifdef INDEX_BUFFERING
    curr->node_ = (struct TTreeNode *) Index_OID_Point(curr->self_);
    if (curr->node_ == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    if (curr->self_ == last->self_)
#else
    if (curr->node_ == last->node_)
#endif
    {
        if (curr->idx_ == last->idx_ ||
                curr->idx_ == curr->node_->item_count_ - 1)
        {
            curr->node_ = NULL;
#ifdef INDEX_BUFFERING
            curr->self_ = NULL_OID;
#endif
            return DB_FAIL;
        }
        else
        {
            curr->idx_++;
            return DB_SUCCESS;
        }
    }
    else if (curr->idx_ < curr->node_->item_count_ - 1)
    {
        curr->idx_++;
        return DB_SUCCESS;
    }
    else
    {
#ifdef INDEX_BUFFERING
#ifdef MDB_DEBUG
        OID curr_self = curr->self_;
#endif
#endif

        curr->node_ = (struct TTreeNode *) TTree_NextNode(curr->node_);
#ifdef INDEX_BUFFERING
        if (curr->node_ == NULL)
        {
#ifdef MDB_DEBUG
            MDB_SYSLOG(("TTree_Find_Next_Without_Filter: curr self 0x%x, "
                            "last self 0x%x\n", curr_self, last->self_));
#endif
            curr->self_ = NULL_OID;
            sc_assert(0, __FILE__, __LINE__);
        }
        else
        {
            curr->self_ = curr->node_->self_;
        }
#endif
        curr->idx_ = 0;
        return DB_SUCCESS;
    }

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
TTree_Find_Prev_Without_Filter(struct TTreePosition *curr,
        struct TTreePosition *first)
{
#ifdef INDEX_BUFFERING
    if (curr->self_ == NULL_OID)
#else
    if (curr->node_ == NULL)
#endif
        return DB_FAIL;

#ifdef INDEX_BUFFERING
    curr->node_ = (struct TTreeNode *) Index_OID_Point(curr->self_);
    if (curr->node_ == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    if (curr->self_ == first->self_)
#else
    if (curr->node_ == first->node_)
#endif
    {
        if (curr->idx_ == first->idx_ || curr->idx_ == 0)
        {
#ifdef INDEX_BUFFERING
            curr->self_ = NULL_OID;
#endif
            curr->node_ = NULL;
            return DB_FAIL;
        }
        else
        {
            curr->idx_--;
            return DB_SUCCESS;
        }
    }
    else if (curr->idx_ > 0)
    {
        curr->idx_--;
        return DB_SUCCESS;
    }
    else
    {
        curr->node_ = (struct TTreeNode *) TTree_PrevNode(curr->node_);
#ifdef INDEX_BUFFERING
        curr->self_ = curr->node_->self_;
#endif
        curr->idx_ = curr->node_->item_count_ - 1;
        return DB_SUCCESS;
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
 * \note 
 *****************************************************************************/
int
TTree_RealNext(struct TTreeIterator *ttreeiter, RecordID real_next)
{
    DB_BOOL use_filter;
    char *record;
    OID record_id;
    int issys;

    DB_UINT32 utime;
    DB_UINT16 qsid;
    int utime_offset, qsid_offset;
    struct KeyValue keyValue;
    struct TTreePosition ttreePos;
    int issys_ttree;
    struct TTree *ttree = (struct TTree *) OID_Point(ttreeiter->tree_oid_);
    int ret_val;

    if (real_next == NULL_OID)
    {
#ifdef INDEX_BUFFERING
        ttreeiter->curr_.self_ = NULL_OID;
#endif
        ttreeiter->curr_.node_ = NULL;
        return DB_SUCCESS;
    }

    /* real_next에서 key 값을 뽑아서 ttree 내 위치를 찾는다 */
    record = (char *) OID_Point(real_next);
    if (record == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    SetBufSegment(issys_ttree, ttreeiter->tree_oid_);
    SetBufSegment(issys, real_next);

    ret_val = KeyValue_Create(record, &ttree->key_desc_,
            ttree->collect_.record_size_, &keyValue, 0);
    if (ret_val < 0)
    {
        UnsetBufSegment(issys);
        UnsetBufSegment(issys_ttree);
        MDB_SYSLOG(("KeyValue Create FAIL: %d\n", ret_val));
        return ret_val;
    }

    UnsetBufSegment(issys);

    if (TTree_Get_First_Eq(ttree, &keyValue, &ttreePos) == DB_SUCCESS)
        ttreeiter->curr_ = ttreePos;

    UnsetBufSegment(issys_ttree);

    KeyValue_Delete2(&keyValue);

    use_filter =
            (ttreeiter->iterator_.filter_.expression_count_) ? TRUE : FALSE;

    if (use_filter == FALSE)
    {
        while (1)
        {
#ifdef INDEX_BUFFERING
            ttreeiter->curr_.node_ = (struct TTreeNode *)
                    Index_OID_Point(ttreeiter->curr_.self_);
            if (ttreeiter->curr_.node_ == NULL)
            {
                return DB_E_OIDPOINTER;
            }
#endif
            if (real_next ==
                    ttreeiter->curr_.node_->item_[ttreeiter->curr_.idx_])
                return DB_SUCCESS;

            if (TTree_Find_Next_Without_Filter(&ttreeiter->curr_,
                            &ttreeiter->last_) < 0)
                return DB_FAIL;
        }
    }
    else    // Filter가 있을 경우
    {
        utime_offset = ttreeiter->iterator_.slot_size_ - 8;
        qsid_offset = ttreeiter->iterator_.slot_size_ - 4;

        /* 레코드의 utime이 cursor open 시간 이전 것을 찾는다 */
        while (1)
        {
#ifdef INDEX_BUFFERING
            ttreeiter->curr_.node_ = (struct TTreeNode *)
                    Index_OID_Point(ttreeiter->curr_.self_);
            if (ttreeiter->curr_.node_ == NULL)
            {
                return DB_E_OIDPOINTER;
            }
#endif
            record_id = ttreeiter->curr_.node_->item_[ttreeiter->curr_.idx_];
            record = (char *) OID_Point(record_id);
            if (record == NULL)
            {
                return DB_E_OIDPOINTER;
            }

            SetBufSegment(issys, record_id);

            utime = *(DB_UINT32 *) (record + utime_offset);
            qsid = *(DB_UINT16 *) (record + qsid_offset);

            /* insert into..as select.. 가 발생하는 경우, 새로
               insert된(같은 cursor 내에서) 레코드는 skip하기 위해서 */
            if (utime == ttreeiter->iterator_.open_time_ &&
                    ttreeiter->iterator_.qsid_ == qsid)
            {
            }
            else
            {
                if (Filter_Compare(&(ttreeiter->iterator_.filter_), record,
                                ttreeiter->iterator_.record_size_) ==
                        DB_SUCCESS)
                {
                    UnsetBufSegment(issys);
                    return DB_SUCCESS;
                }
            }
            UnsetBufSegment(issys);
            if (TTree_Increment(&ttreeiter->curr_) < 0)
                return DB_FAIL;
        }
    }

    return DB_SUCCESS;
}
