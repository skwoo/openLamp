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
#include "mdb_Cont.h"
#include "mdb_IndexCat.h"
#include "mdb_syslog.h"

int Index_OID_HeavyWrite(OID, char *, DB_INT32);
int Index_MemMgr_AllocatePage(struct Collection *, PageID *);
int Index_Collect_InitializePage(struct Collection *col, PageID pid);
int MemMgr_Index_DelPage(struct Collection *collect,
        struct PageHeader *pageheader);
int Collect_RecoverIPage(const struct Collection *col, PageID pid);


///////////////////////////////////////////////////////////////////
//
// Function Name
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
Index_Collect_InitializePage(struct Collection *col, PageID pid)
{
    struct Page *ipage;
    DB_UINT32 sn;
    DB_UINT32 pn;
    OID next_oid;
    char *page_body;
    DB_INT32 slot_no;

    ipage = (struct Page *) Index_PageID_Pointer(pid);

    if (ipage == NULL)
    {
        MDB_SYSLOG((" index init FAIL\n"));
        return DB_FAIL;
    }

    sn = Index_getSegmentNo(pid);
    pn = Index_getPageNo(pid);

    page_body = (char *) (ipage->body_);
    sc_memset(page_body, 0, PAGE_BODY_SIZE);

    for (slot_no = 0; slot_no < SLOT_PER_IPAGE; slot_no++)
    {
        if (slot_no < (SLOT_PER_IPAGE - 1))
            next_oid = Index_OID_Cal(sn, pn,
                    PAGE_HEADER_SIZE + (slot_no + 1) * TTREE_SLOT_SIZE);
        else
            next_oid = NULL_OID;

        sc_memcpy(&(ipage->body_[slot_no * TTREE_SLOT_SIZE]),
                &next_oid, sizeof(OID));
        ipage->body_[(slot_no * TTREE_SLOT_SIZE) + TTREE_SLOT_SIZE - 1] =
                FREE_SLOT;
    }

    ipage->header_.free_node_id_in_page_ =
            Index_OID_Cal(sn, pn, PAGE_HEADER_SIZE);
    ipage->header_.node_count_in_page_ = 0;
    ipage->header_.free_page_next_ = NULL_PID;

    Index_SetDirtyFlag(ipage->header_.self_);

    return DB_SUCCESS;
}

///////////////////////////////////////////////////////////////////
//
// Function Name
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
struct Page *
Index_Free_Page_Search(struct Collection *collect)
{
    struct Page *page;

    while (collect->free_page_list_ != NULL_PID)
    {
        page = (struct Page *) Index_PageID_Pointer(collect->free_page_list_);
        if (page == NULL)
        {
            MDB_SYSLOG((" Free Page Search FAIL \n"));
            return NULL;
        }

        if (page->header_.free_slot_id_in_page_ != NULL_OID)
            return page;

        /* Actually, the page has no free slots */
        collect->free_page_list_ = page->header_.free_page_next_;
        page->header_.free_page_next_ = FULL_PID;

        Index_SetDirtyFlag(page->header_.self_);
        SetDirtyFlag(collect->cont_id_);
    }

    /* no page that has free slots */
    return NULL;
}


///////////////////////////////////////////////////////////////////
//
// Function Name
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
Index_Collect_AllocateSlot(struct Collection *collect, OID * new_slot_id)
{
    OID slot_state;
    char used_slot_byte = USED_SLOT;    /* 'U' */
    struct Page *free_page = NULL;
    OID slot_oid;
    int ret;

#ifdef INDEX_BUFFERING
    int issys = 0;
    int issys_next = 0;
    OID free_page_oid;
    OID nextfree_page_oid;
#endif

    if (isTemp_OID(collect->cont_id_))
    {
        collect = &temp_index_catalog->icollect_;
    }
    else
    {
        collect = &index_catalog->icollect_;
    }

  Try_Search_Again:
    free_page = Index_Free_Page_Search(collect);
    if (free_page != NULL)
    {
        char *ptr;

#ifdef INDEX_BUFFERING
        int issys1 = 0;
#endif

#ifdef INDEX_BUFFERING
        free_page_oid = collect->free_page_list_;
        SetBufIndexSegment(issys, free_page_oid);
#endif

        slot_oid = free_page->header_.free_node_id_in_page_;

        ptr = (char *) Index_OID_Point(slot_oid + (TTREE_SLOT_SIZE - 1));
        if (ptr == NULL)
        {
            return DB_E_OIDPOINTER;
        }

#ifdef INDEX_BUFFERING
        SetBufIndexSegment(issys1, slot_oid);
#endif

        if (Index_getSegmentNo(slot_oid) !=
                Index_getSegmentNo(collect->free_page_list_) ||
                Index_getPageNo(slot_oid) !=
                Index_getPageNo(collect->free_page_list_) ||
                OID_GetOffSet(slot_oid) < PAGE_HEADER_SIZE
                || (*ptr & USED_SLOT))
        {
            __SYSLOG("Recovery iPage pid(%d, %d) sid(%d, %d, %d) %d ",
                    Index_getSegmentNo(collect->free_page_list_),
                    Index_getPageNo(collect->free_page_list_),
                    Index_getSegmentNo(slot_oid), Index_getPageNo(slot_oid),
                    OID_GetOffSet(slot_oid), (*ptr & USED_SLOT));

            Collect_RecoverIPage(collect, collect->free_page_list_);

            __SYSLOG("new sid(%d, %d, %d)\n",
                    Index_getSegmentNo(free_page->header_.
                            free_slot_id_in_page_),
                    Index_getPageNo(free_page->header_.free_slot_id_in_page_),
                    OID_GetOffSet(free_page->header_.free_slot_id_in_page_));

#ifdef INDEX_BUFFERING
            UnsetBufIndexSegment(issys1);
            UnsetBufIndexSegment(issys);
#endif

            goto Try_Search_Again;
        }

#ifdef INDEX_BUFFERING
        UnsetBufIndexSegment(issys1);
#endif
    }

    if (free_page == NULL)
    {
        PageID pid;

        if (collect->free_page_list_ != NULL_PID)
        {
            MDB_SYSLOG(("Index Collect Free Page: Page Pointer Invalid \n"));
            return DB_E_PIDPOINTER;
        }

        if (isTemp_OID(collect->cont_id_))
        {
            ret = MemMgr_AllocatePage((struct Container *) collect,
                    collect, &pid);
            if (ret < 0)
            {
                *new_slot_id = NULL_OID;

                MDB_SYSLOG(("FAIL: index page allocating\n"));

                return ret;
            }
        }
        else
        {
            ret = Index_MemMgr_AllocatePage(collect, &pid);
            if (ret < 0)
            {
                *new_slot_id = NULL_OID;

                MDB_SYSLOG(("FAIL: index page allocating\n"));

                return ret;
            }
        }

        Index_Collect_InitializePage(collect, pid);

        free_page = (struct Page *) Index_PageID_Pointer(pid);
        if (free_page == NULL)
        {
            MDB_SYSLOG(("Index Alloccate Page :Page Pointer Invalid \n"));
            return DB_E_PIDPOINTER;
        }

#ifdef INDEX_BUFFERING
        free_page_oid = pid;
        SetBufIndexSegment(issys, free_page_oid);
#endif

        collect->free_page_list_ = pid;
        if (collect != &temp_index_catalog->icollect_)
        {
            SetDirtyFlag(collect->cont_id_);
        }

    }

    *new_slot_id = free_page->header_.free_node_id_in_page_;

#ifdef INDEX_BUFFERING
    nextfree_page_oid = free_page->header_.free_node_id_in_page_;
    SetBufIndexSegment(issys_next, nextfree_page_oid);
#endif

    do
    {
        char *pbuf = NULL;

        pbuf = (char *) Index_OID_Point(free_page->header_.
                free_node_id_in_page_);
        if (pbuf == NULL)
        {
            return DB_E_OIDPOINTER;
        }

        free_page->header_.free_node_id_in_page_ = *((OID *) pbuf);
    } while (0);

#ifdef INDEX_BUFFERING
    UnsetBufIndexSegment(issys_next);
#endif

    slot_state = *new_slot_id + (TTREE_SLOT_SIZE - 1);
    Index_OID_HeavyWrite(slot_state, &used_slot_byte, 1);

    free_page->header_.node_count_in_page_ += 1;

    if (free_page->header_.free_node_id_in_page_ == NULL_OID)
    {
        collect->free_page_list_ = free_page->header_.free_page_next_;

        free_page->header_.free_page_next_ = FULL_PID;
    }

    collect->item_count_ += 1;

    if (collect != &temp_index_catalog->icollect_)
    {
        SetDirtyFlag(collect->cont_id_);
    }

    Index_SetDirtyFlag(free_page->header_.self_);

#ifdef INDEX_BUFFERING
    UnsetBufIndexSegment(issys);
#endif

    return DB_SUCCESS;
}

///////////////////////////////////////////////////////////////////
//
// Function Name
// Not Need  Function ???
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
Index_Col_Remove(struct Collection *collect, OID record_oid)
{

    OID state;
    char free_slot_byte = FREE_SLOT;
    struct Page *node_delete_page;

#ifdef INDEX_BUFFERING
    int issys = 0;
#endif

    if (isTemp_OID(record_oid))
    {
        collect = &temp_index_catalog->icollect_;
    }
    else
    {
        collect = &index_catalog->icollect_;
    }

    state = (record_oid + (OID) (TTREE_SLOT_SIZE - 1));

    Index_OID_HeavyWrite(state, &free_slot_byte, 1);

    node_delete_page = (struct Page *) Index_PageID_Pointer(record_oid);

#ifdef INDEX_BUFFERING
    SetBufIndexSegment(issys, record_oid);
#endif

    Index_OID_HeavyWrite(record_oid,
            (char *) &node_delete_page->header_.free_node_id_in_page_,
            sizeof(OID));
    node_delete_page->header_.free_node_id_in_page_ = record_oid;
    node_delete_page->header_.node_count_in_page_ -= 1;

    collect->item_count_ -= 1;

    if (node_delete_page->header_.free_page_next_ == FULL_PID)
    {
        node_delete_page->header_.free_page_next_ = collect->free_page_list_;
        collect->free_page_list_ = node_delete_page->header_.self_;
    }
    if (collect != &temp_index_catalog->icollect_)
    {
        SetDirtyFlag(collect->cont_id_);
    }
    Index_SetDirtyFlag(node_delete_page->header_.self_);

#ifdef INDEX_BUFFERING
    UnsetBufIndexSegment(issys);
#endif

    return DB_SUCCESS;
}

int
Index_Link_FreePageList(void)
{
    struct Collection *collect;
    OID pid;
    struct Page *page;

    collect = &index_catalog->icollect_;

    collect->free_page_list_ = NULL_PID;

    pid = collect->page_link_.head_;
    while (pid != NULL_OID)
    {
        page = Index_PageID_Pointer(pid);
        if (page == NULL)
            break;

        if (page->header_.free_slot_id_in_page_ != NULL_OID)
        {
            page->header_.free_page_next_ = collect->free_page_list_;
            collect->free_page_list_ = page->header_.self_;
            Index_SetDirtyFlag(page->header_.self_);
        }

        pid = page->header_.link_.next_;
    }

    SetDirtyFlag(collect->cont_id_);

    return DB_SUCCESS;
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
Collect_RecoverIPage(const struct Collection *col, PageID pid)
{
    struct Page *page;
    DB_UINT32 sn;
    DB_UINT32 pn;
    DB_INT32 slot_no;
    OID slotID;
    char *pSlot;

#ifdef INDEX_BUFFERING
    int issys = 0;
#endif

    page = (struct Page *) Index_PageID_Pointer(pid);

#ifdef INDEX_BUFFERING
    SetBufIndexSegment(issys, pid);
#endif

    sn = Index_getSegmentNo(pid);
    pn = Index_getPageNo(pid);

    page->header_.free_node_id_in_page_ = NULL_OID;
    page->header_.node_count_in_page_ = 0;

    for (slot_no = SLOT_PER_IPAGE; slot_no > 0; slot_no--)
    {
        slotID = Index_OID_Cal(sn, pn,
                PAGE_HEADER_SIZE + (slot_no - 1) * TTREE_SLOT_SIZE);

        pSlot = (char *) Index_OID_Point(slotID);
        if (pSlot == NULL)
        {
#ifdef INDEX_BUFFERING
            UnsetBufIndexSegment(issys);
#endif
            return DB_E_OIDPOINTER;
        }

        if (pSlot[TTREE_SLOT_SIZE - 1] & USED_SLOT)     /* used slot */
        {
            page->header_.node_count_in_page_ += 1;
        }
        else    /* free slot */
        {
            sc_memcpy(pSlot, &(page->header_.free_node_id_in_page_),
                    sizeof(OID));

            page->header_.free_node_id_in_page_ = slotID;
        }
    }

    Index_SetDirtyFlag(page->header_.self_);

#ifdef INDEX_BUFFERING
    UnsetBufIndexSegment(issys);
#endif

    return DB_SUCCESS;
}
