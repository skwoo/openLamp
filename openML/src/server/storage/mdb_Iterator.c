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
#include "mdb_Server.h"
#include "sql_mclient.h"

int TTree_Find_Next_Without_Filter(struct TTreePosition *curr,
        struct TTreePosition *last);
int TTree_Find_Prev_Without_Filter(struct TTreePosition *curr,
        struct TTreePosition *first);
int Filter_Compare(struct Filter *filter, char *, int);

extern struct CursorMgr *Cursor_Mgr;

///////////////////////////////////////////////////////////////////
//
// Function Name : Iter_Current
// Call By : __Read(), __Update(), __Remove()
//
///////////////////////////////////////////////////////////////////
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
OID
Iter_Current(struct Iterator *Iter)
{
    struct TTreeIterator *ttreeIter;

    switch (Iter->type_)
    {
    case SEQ_CURSOR:   // SEQ_CURSOR
        return ((struct ContainerIterator *) Iter)->curr_;

    case TTREE_FORWARD_CURSOR: // TTREE CURSOR

        ttreeIter = (struct TTreeIterator *) Iter;

#ifdef INDEX_BUFFERING
        if (ttreeIter->curr_.self_ == NULL_OID)
#else
        if (ttreeIter->curr_.node_ == NULL)
#endif
            return NULL_OID;

#ifdef INDEX_BUFFERING
        ttreeIter->curr_.node_ =
                (struct TTreeNode *) Index_OID_Point(ttreeIter->curr_.self_);
#endif
        return ttreeIter->curr_.node_->item_[ttreeIter->curr_.idx_];
    case TTREE_BACKWARD_CURSOR:        // TTREE CURSOR

        ttreeIter = (struct TTreeIterator *) Iter;

#ifdef INDEX_BUFFERING
        if (ttreeIter->curr_.self_ == NULL_OID)
#else
        if (ttreeIter->curr_.node_ == NULL)
#endif
            return NULL_OID;

#ifdef INDEX_BUFFERING
        ttreeIter->curr_.node_ =
                (struct TTreeNode *) Index_OID_Point(ttreeIter->curr_.self_);
#endif
        return ttreeIter->curr_.node_->item_[ttreeIter->curr_.idx_];
    }

    return NULL_OID;
}


///////////////////////////////////////////////////////////////////
//
// Function Name : Iter_Attach
// Call By : __Insert(), __Read(),  __Update(), __Remove()
//
///////////////////////////////////////////////////////////////////
/*****************************************************************************/
//! Iter_Attach
/*! \breif  Cursor 관련 구조체(Iterator)를 얻어온다
 ************************************
 * \param cursor_id(in) :
 * \param optype(in)    : 
 * \param page(in)      :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
struct Iterator *
Iter_Attach(DB_INT32 cursor_id, int optype, OID page)
{
    struct Cursor *pCursor;
    struct Iterator *pIter;

#ifdef MDB_DEBUG
    if (cursor_id == 0 || cursor_id > MAX_ALLOC_CURSORID_NUM)
        return NULL;
#endif

    pCursor = &(Cursor_Mgr->cursor[cursor_id - 1]);
    if (pCursor->fAllocated_ == 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_INVALIDCURSORID, 0);
        return NULL;
    }

    pIter = (struct Iterator *) (pCursor->chunk_);

    /* insert는 optype이 0
       insert이거나 초기화 되었다면 pIter return */
    if (!optype || pIter->fInit)
        return pIter;

    if (pIter->type_ == SEQ_CURSOR)
    {
        ContIter_Create((struct ContainerIterator *) pIter, page);
    }
    else
    {
        TTreeIter_Create((struct TTreeIterator *) pIter);
    }

    pIter->fInit = 1;
    pIter->real_next = NULL_OID;

    return pIter;
}

///////////////////////////////////////////////////////////////////
//
// Function Name : Iter_FindNext
// Call By : __Read(),  __Update(), __Remove()
//
///////////////////////////////////////////////////////////////////
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
Iter_FindPrev(struct Iterator *Iter, OID page)
{
    char *record;
    DB_UINT32 utime;
    DB_UINT16 qsid;
    int utime_offset, qsid_offset;
    struct TTreeIterator *ttreeIter = (struct TTreeIterator *) Iter;
    struct Container *Cont = (struct Container *) OID_Point(Iter->cont_id_);
    OID record_id = NULL_OID;
    int issys;
    int issys_cont;
    int ret;

    SetBufSegment(issys_cont, Cont->collect_.cont_id_);

    utime_offset = Cont->collect_.slot_size_ - 8;
    qsid_offset = Cont->collect_.slot_size_ - 4;

    /* 순차 커서일 경우 Filter를 사용하지 않았을때 */
    switch (Iter->type_)
    {
    case SEQ_CURSOR:
        /* backward cursor not suppored yet for SEQ CURSOR */
        break;

    case TTREE_BACKWARD_CURSOR:
        issys = 0;
        while (1)
        {
            UnsetBufSegment(issys);

            if (TTree_Find_Next_Without_Filter(&(ttreeIter->curr_),
                            &(ttreeIter->last_)) < 0)
            {
                ret = DB_FAIL;
                goto end;
            }

#ifdef INDEX_BUFFERING
            ttreeIter->curr_.node_ =
                    (struct TTreeNode *) Index_OID_Point(ttreeIter->curr_.
                    self_);
            if (ttreeIter->curr_.node_ == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }
#endif
            record_id = ttreeIter->curr_.node_->item_[ttreeIter->curr_.idx_];
            record = (char *) OID_Point(record_id);
            if (record == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }

            SetBufSegment(issys, record_id);

            utime = *(DB_UINT32 *) (record + utime_offset);
            qsid = *(DB_UINT16 *) (record + qsid_offset);

            /* update cursor인 경우 visited된 record는 skip */
            /* insert into..as select.. 가 발생하는 경우, 새로
               insert된(같은 cursor 내에서) 레코드는 skip하기 위해서
               Iter->updateVisitedInit_ == 1 비교를 없앰 */
            if (Iter->open_time_ == utime && Iter->qsid_ == qsid)
            {
            }
            else
            {
                if (Iter->filter_.expression_count_ == 0)
                {
                    UnsetBufSegment(issys);
                    Iter->real_next = record_id;
                    ret = DB_SUCCESS;
                    goto end;
                }

                if (Filter_Compare(&(Iter->filter_), record,
                                Cont->collect_.record_size_) == DB_SUCCESS)
                {
                    UnsetBufSegment(issys);
                    Iter->real_next = record_id;
                    ret = DB_SUCCESS;
                    goto end;
                }
            }
        }
        break;

    case TTREE_FORWARD_CURSOR:
        issys = 0;
        while (1)
        {
            UnsetBufSegment(issys);

            if (TTree_Find_Prev_Without_Filter(&(ttreeIter->curr_),
                            &(ttreeIter->first_)) < 0)
            {
                ret = DB_FAIL;
                goto end;
            }

#ifdef INDEX_BUFFERING
            ttreeIter->curr_.node_ =
                    (struct TTreeNode *) Index_OID_Point(ttreeIter->curr_.
                    self_);
            if (ttreeIter->curr_.node_ == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }
#endif
            record_id = ttreeIter->curr_.node_->item_[ttreeIter->curr_.idx_];
            record = (char *) OID_Point(record_id);
            if (record == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }

            SetBufSegment(issys, record_id);

            utime = *(DB_UINT32 *) (record + utime_offset);
            qsid = *(DB_UINT16 *) (record + qsid_offset);

            /* update cursor인 경우 visited된 record는 skip */
            /* insert into..as select.. 가 발생하는 경우, 새로
               insert된(같은 cursor 내에서) 레코드는 skip하기 위해서
               Iter->updateVisitedInit_ == 1 비교를 없앰 */
            if (Iter->open_time_ == utime && Iter->qsid_ == qsid)
            {
            }
            else
            {
                if (Iter->filter_.expression_count_ == 0)
                {
                    UnsetBufSegment(issys);
                    Iter->real_next = record_id;
                    ret = DB_SUCCESS;
                    goto end;
                }

                if (Filter_Compare(&(Iter->filter_), record,
                                Cont->collect_.record_size_) == DB_SUCCESS)
                {
                    UnsetBufSegment(issys);
                    Iter->real_next = record_id;
                    ret = DB_SUCCESS;
                    goto end;
                }
            }
        }
        break;
    }

    ret = DB_FAIL;
  end:
    UnsetBufSegment(issys_cont);

    return ret;
}

int
Iter_FindNext(struct Iterator *Iter, OID page)
{
    char *record;
    DB_UINT32 utime;
    DB_UINT16 qsid;
    int utime_offset, qsid_offset;
    struct TTreeIterator *ttreeIter;
    struct ContainerIterator *contIter;
    struct Container *Cont = (struct Container *) OID_Point(Iter->cont_id_);
    OID tail;
    OID record_id = NULL_OID;
    int issys;
    int issys_cont;
    int ret;

    SetBufSegment(issys_cont, Cont->collect_.cont_id_);

    if (page == NULL_OID)
        tail = Cont->collect_.page_link_.tail_;
    else
        tail = page;

    utime_offset = Cont->collect_.slot_size_ - 8;
    qsid_offset = Cont->collect_.slot_size_ - 4;

    /* 순차 커서일 경우 Filter를 사용하지 않았을때 */
    switch (Iter->type_)
    {
    case SEQ_CURSOR:
        contIter = (struct ContainerIterator *) Iter;
        issys = 0;
        while (1)
        {
            UnsetBufSegment(issys);

            Col_GetNextDataID(contIter->curr_,
                    Cont->collect_.slot_size_, tail,
                    contIter->iterator_.msync_flag);

            if (contIter->curr_ == NULL_OID)
            {
                ret = DB_FAIL;
                goto end;
            }
            if (contIter->curr_ == INVALID_OID)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }

            record_id = contIter->curr_;
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
            if (utime == Iter->open_time_ && Iter->qsid_ == qsid)
            {
                continue;
            }

            /* filter가 없으면 return */
            if (Iter->filter_.expression_count_ == 0)
            {
                UnsetBufSegment(issys);
                ret = DB_SUCCESS;
                goto end;
            }
            /* filter를 만족하면 return */
            if (Filter_Compare(&(Iter->filter_), record,
                            Cont->collect_.record_size_) == DB_SUCCESS)
            {
                UnsetBufSegment(issys);
                ret = DB_SUCCESS;
                goto end;
            }
        }
        break;

    case TTREE_FORWARD_CURSOR:
        ttreeIter = (struct TTreeIterator *) Iter;
        issys = 0;
        while (1)
        {
            UnsetBufSegment(issys);

            if (TTree_Find_Next_Without_Filter(&(ttreeIter->curr_),
                            &(ttreeIter->last_)) < 0)
            {
                ret = DB_FAIL;
                goto end;
            }

#ifdef INDEX_BUFFERING
            ttreeIter->curr_.node_ =
                    (struct TTreeNode *) Index_OID_Point(ttreeIter->curr_.
                    self_);
            if (ttreeIter->curr_.node_ == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }
#endif
            record_id = ttreeIter->curr_.node_->item_[ttreeIter->curr_.idx_];
            /* filter가 없고, insert..select 형태의 query가 수행되지
             * 않았다면 return. insert..select 형태의 query 수행은
             * 트랜잭션의 fReadOnly flag로 판단. */
            if (Iter->filter_.expression_count_ == 0)
            {
                if (Iter->trans_ptr->fReadOnly)
                {
                    Iter->real_next = record_id;
                    ret = DB_SUCCESS;
                    goto end;
                }
            }

            record = (char *) OID_Point(record_id);
            if (record == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }

            SetBufSegment(issys, record_id);

            utime = *(DB_UINT32 *) (record + utime_offset);
            qsid = *(DB_UINT16 *) (record + qsid_offset);

            /* update cursor인 경우 visited된 record는 skip */
            /* insert into..as select.. 가 발생하는 경우, 새로
               insert된(같은 cursor 내에서) 레코드는 skip하기 위해서
               Iter->updateVisitedInit_ == 1 비교를 없앰 */
            if (Iter->open_time_ == utime && Iter->qsid_ == qsid)
            {
            }
            else
            {
                if (Iter->filter_.expression_count_ == 0)
                {
                    UnsetBufSegment(issys);
                    Iter->real_next = record_id;
                    ret = DB_SUCCESS;
                    goto end;
                }

                if (Filter_Compare(&(Iter->filter_), record,
                                Cont->collect_.record_size_) == DB_SUCCESS)
                {
                    UnsetBufSegment(issys);
                    Iter->real_next = record_id;
                    ret = DB_SUCCESS;
                    goto end;
                }
            }
        }
        break;

    case TTREE_BACKWARD_CURSOR:
        ttreeIter = (struct TTreeIterator *) Iter;
        issys = 0;
        while (1)
        {
            UnsetBufSegment(issys);

            if (TTree_Find_Prev_Without_Filter(&(ttreeIter->curr_),
                            &(ttreeIter->first_)) < 0)
            {
                ret = DB_FAIL;
                goto end;
            }
#ifdef INDEX_BUFFERING
            ttreeIter->curr_.node_ =
                    (struct TTreeNode *) Index_OID_Point(ttreeIter->curr_.
                    self_);
            if (ttreeIter->curr_.node_ == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }
#endif
            record_id = ttreeIter->curr_.node_->item_[ttreeIter->curr_.idx_];
            if (Iter->filter_.expression_count_ == 0)
            {
                if (Iter->trans_ptr->fReadOnly)
                {
                    Iter->real_next = record_id;
                    ret = DB_SUCCESS;
                    goto end;
                }
            }

            record = (char *) OID_Point(record_id);
            if (record == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }


            SetBufSegment(issys, record_id);

            utime = *(DB_UINT32 *) (record + utime_offset);
            qsid = *(DB_UINT16 *) (record + qsid_offset);

            /* update cursor인 경우 visited된 record는 skip */
            /* insert into..as select.. 가 발생하는 경우, 새로
               insert된(같은 cursor 내에서) 레코드는 skip하기 위해서
               Iter->updateVisitedInit_ == 1 비교를 없앰 */
            if (Iter->open_time_ == utime && Iter->qsid_ == qsid)
            {
            }
            else
            {
                if (Iter->filter_.expression_count_ == 0)
                {
                    UnsetBufSegment(issys);
                    Iter->real_next = record_id;
                    ret = DB_SUCCESS;
                    goto end;
                }

                if (Filter_Compare(&(Iter->filter_), record,
                                Cont->collect_.record_size_) == DB_SUCCESS)
                {
                    UnsetBufSegment(issys);
                    Iter->real_next = record_id;
                    ret = DB_SUCCESS;
                    goto end;
                }
            }
        }
        break;
    }

    ret = DB_FAIL;
  end:
    UnsetBufSegment(issys_cont);

    return ret;
}

///////////////////////////////////////////////////////////////////
//
// Function Name : Iter_FindFindFirst
// Call By : __Read(),
//
///////////////////////////////////////////////////////////////////
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
Iter_FindFirst(struct Iterator *Iter)
{
    int ret;

    if (Iter->type_ == SEQ_CURSOR)
        ((struct ContainerIterator *) Iter)->curr_ =
                ((struct ContainerIterator *) Iter)->first_;
    else
    {
        if (Iter->type_ == TTREE_FORWARD_CURSOR)
            ((struct TTreeIterator *) Iter)->curr_ =
                    ((struct TTreeIterator *) Iter)->first_;
        else    /* TTREE_BACKWARD_CURSOR */
            ((struct TTreeIterator *) Iter)->curr_ =
                    ((struct TTreeIterator *) Iter)->last_;
        Iter->real_next = Iter_Current(Iter);
    }

    ret = DB_SUCCESS;

    return ret;
}

int
Iter_FindLast(struct Iterator *Iter)
{
    int ret;

    if (Iter->type_ == SEQ_CURSOR)
        ((struct ContainerIterator *) Iter)->curr_ =
                ((struct ContainerIterator *) Iter)->last_;
    else
    {
        if (Iter->type_ == TTREE_FORWARD_CURSOR)
            ((struct TTreeIterator *) Iter)->curr_ =
                    ((struct TTreeIterator *) Iter)->last_;
        else    /* TTREE_BACKWARD_CURSOR */
            ((struct TTreeIterator *) Iter)->curr_ =
                    ((struct TTreeIterator *) Iter)->first_;
        Iter->real_next = Iter_Current(Iter);
    }

    ret = DB_SUCCESS;

    return ret;
}
