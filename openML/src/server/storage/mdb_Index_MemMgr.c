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
#include "mdb_PageList.h"
#include "mdb_Collect.h"
#include "mdb_Server.h"
#include "mdb_syslog.h"

int Index_OID_HeavyWrite(OID, char *, DB_INT32);
int ContCat_Index_Rebuild(int);
int ContCat_Cont_Counting(void);
void CheckPoint_SegmentAppend(void);

static int Index_Check_Mapping_ICatalog(int f_null);

extern int fRecovered;

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
Index_MemoryMgr_init(DB_UINT32 isegment_no)
{
    DB_UINT32 isn;
    DB_UINT32 ipn;
    struct Segment *isegment;
    struct PageHeader *header;
    int ret;
    extern int Check_Index_Item_Size(void);

    if (f_checkpoint_finished == CHKPT_BEGIN)
        fRecovered = ALL_REBUILD;

    if (mem_anchor_->iallocated_segment_no_ != 0
            && !(fRecovered & ALL_REBUILD))
    {
        /* index node item 수가 변경된 경우를 대비해서 점검!
           item 수가 변경된 경우(DB 바이너리 변경), index를 다시 만든다.
           TNODE_MIN_ITEM & TNODE_MAX_ITEM */
        if (Check_Index_Item_Size() == DB_SUCCESS)
        {
            if (Index_MemoryMgr_initFromDBFile() == DB_SUCCESS)
            {
                MDB_SYSLOG(("Index init from file %s\n",
                                Mem_mgr->database_file_.
                                file_name_[DBFILE_INDEX_TYPE]));

#ifdef CHECK_INDEX_VALIDATION
                Index_Check_Page();
#endif
                if (fRecovered == PARTIAL_REBUILD)
                {
                    ret = ContCat_Index_Rebuild(PARTIAL_REBUILD);
                    if (ret < 0)
                    {
                        __SYSLOG2("INDEX Memory Manager initialize ERROR\n");
                        return ret;
                    }
                }
                goto L_return;
            }
        }
    }

    fRecovered = ALL_REBUILD;

    isegment_no = 1;

    for (isn = SEGMENT_NO - 1; isn >= SEGMENT_NO - isegment_no; isn--)
    {
        isegment = (struct Segment *)
                DBDataMem_Alloc(SEGMENT_SIZE, &Mem_mgr->segment_memid[isn]);

        if (isegment == NULL)
        {
            if (server->shparams.mem_segments)
            {
                MemMgr_DeallocateSegment();
                isegment = (struct Segment *)
                        DBDataMem_Alloc(SEGMENT_SIZE,
                        &Mem_mgr->segment_memid[isn]);
            }
            if (isegment == NULL)
                return DB_E_OUTOFDBMEMORY;
        }
        else
        {
            isegment_[isn] = isegment;
        }

        sc_memset((char *) isegment, 0, SEGMENT_SIZE);


        for (ipn = 0; ipn < PAGE_PER_SEGMENT; ipn++)
        {
            header = &(isegment_[isn]->page_[ipn].header_);

            header->self_ = Index_PageID_Set(isn, ipn);

            header->link_.next_ = (ipn == PAGE_PER_SEGMENT - 1) ?
                    ((isn == SEGMENT_NO -
                            isegment_no) ? NULL_PID : Index_PageID_Set(isn - 1,
                            0)) : Index_PageID_Set(isn, ipn + 1);

            header->link_.prev_ = (ipn == 0) ?
                    ((isn == SEGMENT_NO -
                            1) ? NULL_PID : Index_PageID_Set(isn + 1,
                            PAGE_PER_SEGMENT - 1)) : Index_PageID_Set(isn,
                    ipn - 1);

            SetPageDirty(isn, ipn);
        }
    }

    mem_anchor_->iallocated_segment_no_ = isegment_no;
    mem_anchor_->ifirst_free_page_id_ = Index_PageID_Set(SEGMENT_NO - 1, 0);
    mem_anchor_->ifree_page_count_ = PAGE_PER_SEGMENT;

    index_catalog->icollect_.page_link_.head_ = NULL_OID;
    index_catalog->icollect_.page_link_.tail_ = NULL_OID;
    index_catalog->icollect_.free_page_list_ = NULL_OID;
    index_catalog->icollect_.item_count_ = 0;
    index_catalog->icollect_.page_count_ = 0;

    {
        SetPageDirty(0, 0);
    }

    /* index file 제거 하고 다시 생성되도록 처리 */
    {
        char fname[MDB_FILE_NAME];
        int ffd;
        int i;

        for (i = 0; i < MAX_FILE_ID; ++i)
        {
            if (Mem_mgr->database_file_.file_id_[DBFILE_INDEX_TYPE][i] == -1)
            {
                break;
            }

            sc_close(Mem_mgr->database_file_.file_id_[DBFILE_INDEX_TYPE][i]);
            Mem_mgr->database_file_.file_id_[DBFILE_INDEX_TYPE][i] = -1;
        }

        sc_sprintf(fname, "%s0",
                Mem_mgr->database_file_.file_name_[DBFILE_INDEX_TYPE]);
        ffd = sc_open(fname, O_RDONLY, OPEN_MODE);
        if (ffd >= 0)
        {
            sc_close(ffd);
            sc_unlink(fname);
        }
    }

    ret = DBFile_Write_OC(DBFILE_INDEX_TYPE, (SEGMENT_NO - 1), 0,
            (void *) &(isegment_[SEGMENT_NO - 1]->page_[0]), PAGE_PER_SEGMENT,
            1);
    if (ret < 0)
    {
        MDB_SYSLOG(("index segment append FAIL\n"));
        return ret;
    }

    /* index를 새로 만듬 */
    ret = ContCat_Index_Rebuild(ALL_REBUILD);
    if (ret < 0)
    {
        MDB_SYSLOG(("INDEX Memory Manager initialize ERROR\n"));
        return ret;
    }

    /* Flush buffer cache... */
    if (server->shparams.mem_segments)
    {
        MemMgr_DeallocateAllSegments(1);
    }

  L_return:
    fRecovered = 0;

    MDB_SYSLOG(("mem anchor:\n"));

    MemMgr_DumpMemAnchor();

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
int
Index_MemoryMgr_initFromDBFile(void)
{
    DB_UINT32 isn;
    int ret;

    isn = (DB_UINT32) (SEGMENT_NO - mem_anchor_->iallocated_segment_no_);

#if !defined(INDEX_BUFFERING)
    if (isegment_[SEGMENT_NO - 1] != NULL)
        return DB_SUCCESS;

    if (DBFile_Open(DBFILE_INDEX_TYPE, -1) < 0)
    {
        MDB_SYSLOG(("Index File Open Error\n"));
        return DB_FAIL;
    }

    for (isn = SEGMENT_NO - 1;
            isn >=
            (DB_UINT32) (SEGMENT_NO - mem_anchor_->iallocated_segment_no_);
            isn--)
    {
        isegment_[isn] =
                DBDataMem_Alloc(SEGMENT_SIZE, &Mem_mgr->segment_memid[isn]);

        if (isegment_[isn] == NULL)
        {
            DBFile_Close(DBFILE_INDEX_TYPE, -1);
            ret = DB_E_OUTOFDBMEMORY;
            goto error;
        }

        if (DBFile_Read(DBFILE_INDEX_TYPE, isn, 0,
                        (void *) &(isegment_[isn]->page_[0]),
                        PAGE_PER_SEGMENT) < 0)
        {
            DBFile_Close(DBFILE_INDEX_TYPE, -1);
            MDB_SYSLOG(("Index read from file FAIL\n"));
            ret = DB_FAIL;
            goto error;
        }
    }

    if (DBFile_Close(DBFILE_INDEX_TYPE, -1) < 0)
    {
        MDB_SYSLOG(("Index File Close Error\n"));
        ret = DB_FAIL;
        goto error;
    }

#endif /* !defined(INDEX_BUFFERING) */

    ret = Index_Check_Mapping_ICatalog(0);
    if (ret != DB_SUCCESS)
    {
        MDB_SYSLOG(("Index_Check_Mapping_ICatalog FAIL %d\n", ret));
        goto error;
    }

    if (mem_anchor_->ifirst_free_page_id_)
    {
        struct Page *page;

        page = (struct Page *)
                Index_OID_Point(mem_anchor_->ifirst_free_page_id_);
        if (page == NULL || page->header_.link_.prev_)
        {
            MDB_SYSLOG(("Index free page link error %x, %x\n",
                            mem_anchor_->ifirst_free_page_id_,
                            (page == NULL) ? 0 : page->header_.link_.prev_));
            ret = DB_FAIL;
            goto error;
        }
    }

    return DB_SUCCESS;

  error:
    for (; isn < SEGMENT_NO; isn++)
    {
        if (Mem_mgr->segment_memid[isn])
        {
            DBDataMem_Free(Mem_mgr->segment_memid[isn]);
            Mem_mgr->segment_memid[isn] = 0;
        }
        isegment_[isn] = NULL;
    }

    Index_Check_Mapping_ICatalog(1);

    return ret;
}

static int
Index_Check_Mapping_ICatalog(int f_null)
{
    OID index_oid;
    struct TTree *ttree;
    struct TTreeNode *node;
    OID ttree_root_;
    int ret;
    int issys_ttree = 0;

    if (index_catalog == NULL)
        return DB_SUCCESS;

    if (DB_Params.skip_index_check)
    {
        extern int f_checkpoint_finished;

        if (IsNormalShutdown())
        {
            __SYSLOG("Skip Index Check, Index_Check_Mapping_ICatalog\n");
            return DB_SUCCESS;
        }
    }

    index_oid = Col_GetFirstDataID((struct Container *) index_catalog, 0);
    while (index_oid != NULL_OID)
    {
        if (index_oid == INVALID_OID)
        {
            return DB_E_OIDPOINTER;
        }

        /* 잘못된 oid index가 있는 경우 오류 처리 */
        ttree = (struct TTree *) OID_Point(index_oid);
        if (ttree == NULL)
            return DB_E_INVLAIDCONTOID;

        SetBufSegment(issys_ttree, index_oid);

        if (f_null)
        {
            ttree->root_[0] = NULL_OID;
            SetDirtyFlag(ttree->collect_.cont_id_);
            goto next_index;
        }

        ttree_root_ = ttree->root_[0];

        /* root node가 없는 경우, skip to next */
        if (ttree_root_ == NULL_OID)
            goto next_index;

        node = (struct TTreeNode *) Index_OID_Point(ttree_root_);

        if (node == NULL)
        {
            ret = (__LINE__ * -1);
            goto end;
        }

        /* root node의 oid가 ttree 내 정보와 맞지 않을 때 오류 처리 */
        if (node->self_ != ttree_root_)
        {
            ret = (__LINE__ * -1);
            goto end;
        }

        /* root의 type이 ROOT_NODE가 아닌 경우 오류 처리 */
        if (node->type_ != ROOT_NODE)
        {
            ret = (__LINE__ * -1);
            goto end;
        }

        /* root의 parent가 존재하는 경우 오류 처리 */
        if (node->parent_ != NULL_OID)
        {
            ret = (__LINE__ * -1);
            goto end;
        }

        /* root의 parent가 존재하는 경우 오류 처리 */
        if (node->parent_ != NULL_OID)
        {
            ret = (__LINE__ * -1);
            goto end;
        }

      next_index:
        UnsetBufSegment(issys_ttree);
        Col_GetNextDataID(index_oid,
                index_catalog->collect_.slot_size_,
                index_catalog->collect_.page_link_.tail_, 0);
    }

    ret = DB_SUCCESS;

  end:
    UnsetBufSegment(issys_ttree);

    return ret;
}

int
Index_MemoryMgr_FreeSegment(void)
{
    DB_UINT32 isn;

    for (isn = SEGMENT_NO - 1;
            isn >=
            (DB_UINT32) (SEGMENT_NO - mem_anchor_->iallocated_segment_no_);
            isn--)
    {
        if (Mem_mgr->segment_memid[isn])
        {
            DBDataMem_Free(Mem_mgr->segment_memid[isn]);
            Mem_mgr->segment_memid[isn] = 0;
        }
        isegment_[isn] = NULL;
    }

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
int
Index_MemMgr_AllocateSegment(PageID pageid)
{
    DB_UINT32 Last_ISegment_No;
    PageID ipn;
    struct Page *temp_page;
    struct PageHeader *header;

    Last_ISegment_No = SEGMENT_NO - 1 - mem_anchor_->iallocated_segment_no_;

    if (Last_ISegment_No <= TEMPSEG_END_NO)
    {
        MDB_SYSLOG(("FAIL: Allocate Temp Segment %d, due to memory limit\n",
                        Last_ISegment_No));
        MemMgr_DumpMemAnchor();
        return DB_E_LIMITSIZE;
    }

    if (server->shparams.max_segments != 0 &&
            (DB_UINT32) (mem_anchor_->allocated_segment_no_ +
                    mem_anchor_->iallocated_segment_no_)
            >= (DB_UINT32) server->shparams.max_segments)
    {
        MDB_SYSLOG(("FAIL: Allocate Index Segment %d, due to memory limit\n",
                        Last_ISegment_No));
        MemMgr_DumpMemAnchor();
        return DB_E_LIMITSIZE;
    }

    if (Last_ISegment_No == 0 || isegment_[Last_ISegment_No] != NULL)
    {
        MDB_SYSLOG(("FAIL: Allocate Index Segment %d, full of memory\n",
                        Last_ISegment_No));
        MemMgr_DumpMemAnchor();
        return DB_E_IDXSEGALLOCFAILL;
    }

#ifdef MDB_DEBUG
    MDB_SYSLOG(("Index MemMgr Allocate pageid=0x%x, Index Segment No=%d\n",
                    pageid, Last_ISegment_No));
#endif

#ifdef INDEX_BUFFERING
    /* memory에 있는 segment 수가 limit이면 tail segment를 deallocate 시킨다. */
    if (server->shparams.mem_segments &&
            Mem_mgr->LRU_Count >= server->shparams.mem_segments)
    {
        MemMgr_DeallocateSegment();
    }
#endif

    /* file이 volume을 이미 확보하고 있으면 그대로 사용하도록 처리.
     * 즉, file volume이 segment 확장할 만큼 크기가 안될 때
     * disk free space를 점검하도록 처리함. */
    if (DBFile_GetVolumeSize(DBFILE_INDEX_TYPE) <
            (mem_anchor_->iallocated_segment_no_ + 1) * SEGMENT_SIZE
            && SEGMENT_SIZE * 2 > DBFile_DiskFreeSpace(DBFILE_INDEX_TYPE))
    {
        MDB_SYSLOG(("Index MemMgr Segment Allocate FAIL(disk full!!!)\n"));
        return DB_E_DISKFULL;
    }

    isegment_[Last_ISegment_No] = (struct Segment *)
            DBDataMem_Alloc(SEGMENT_SIZE,
            &Mem_mgr->segment_memid[Last_ISegment_No]);

    if (isegment_[Last_ISegment_No] == NULL)
    {
        if (server->shparams.mem_segments)
        {   /* buffering을 하는 상태에서는 deallocate하고 나서 한 번 더 시도함. */
            MemMgr_DeallocateSegment();
            isegment_[Last_ISegment_No] = (struct Segment *)
                    DBDataMem_Alloc(SEGMENT_SIZE,
                    &Mem_mgr->segment_memid[Last_ISegment_No]);
        }

        if (isegment_[Last_ISegment_No] == NULL)
        {
            return DB_E_OUTOFDBMEMORY;
        }
    }

    sc_memset((char *) isegment_[Last_ISegment_No], 0, SEGMENT_SIZE);

    for (ipn = 0; ipn < PAGE_PER_SEGMENT; ipn++)
    {
        header = &(isegment_[Last_ISegment_No]->page_[ipn].header_);
        header->self_ = Index_PageID_Set(Last_ISegment_No, ipn);
        header->link_.next_ = (ipn == PAGE_PER_SEGMENT - 1) ?
                NULL_PID : Index_PageID_Set(Last_ISegment_No, ipn + 1);
        header->link_.prev_ = (ipn == 0) ?
                Index_PageID_Set(Last_ISegment_No + 1, PAGE_PER_SEGMENT - 1) :
                Index_PageID_Set(Last_ISegment_No, ipn - 1);

#ifndef INDEX_BUFFERING /* After return, the allocated segment
                           is written to the file right then. */
        SetPageDirty(Last_ISegment_No, ipn);
#endif
    }

    temp_page = (struct Page *) Index_PageID_Pointer(pageid);
    temp_page->header_.link_.next_ =
            isegment_[Last_ISegment_No]->page_[0].header_.self_;
    SetPageDirty(getSegmentNo(pageid), getPageNo(pageid));

    temp_page = (struct Page *) &(isegment_[Last_ISegment_No]->page_[0]);
    temp_page->header_.link_.prev_ = pageid;
    SetPageDirty(Last_ISegment_No, 0);

    SetPageDirty(0, 0);

    mem_anchor_->iallocated_segment_no_ += 1;
    mem_anchor_->ifree_page_count_ += PAGE_PER_SEGMENT;

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
int
Index_MemMgr_SetNextPageID(IPageID curr, IPageID prev)
{
    curr += sizeof(IPageID) * 2;
    Index_OID_HeavyWrite(curr, (char *) &prev, sizeof(IPageID));
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
int
Index_MemMgr_SetPageLink(PageID curr, struct PageLink link)
{
    curr += sizeof(IPageID);
    Index_OID_HeavyWrite(curr, (char *) &link, LINK_SIZE);
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
int
Index_MemMgr_SetPrevPageID(IPageID curr, IPageID prev)
{
    curr += sizeof(IPageID);
    Index_OID_HeavyWrite(curr, (char *) &prev, sizeof(IPageID));
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
int
Index_MemMgr_AllocatePage(struct Collection *collect, IPageID * new_page_id)
{
    IPageID next;
    IPageID tail;
    struct Page *ipage;
    struct PageLink link;
    struct MemBase anchor;
    struct PageList *pagelist;

#ifdef INDEX_BUFFERING
    int issys_ipage;
#endif
    int ret;

    collect = &index_catalog->icollect_;

    pagelist = &collect->page_link_;

    *new_page_id = mem_anchor_->ifirst_free_page_id_;

    ipage = (struct Page *) (Index_PageID_Pointer(*new_page_id));

    if (ipage->header_.link_.next_ == NULL_PID)
    {
#ifdef MDB_DEBUG
        MDB_SYSLOG(("Index MemMgr Allocate Page self=0x%x, new page id=0x%x\n",
                        ipage->header_.self_, *new_page_id));
#endif

        ret = Index_MemMgr_AllocateSegment(*new_page_id);
        if (ret < 0)
        {
            MDB_SYSLOG(("MemMory allocate iSegment FAIL\n"));

            return ret;
        }

#ifdef INDEX_BUFFERING
        {
            int last_isn;
            int issys_next;

            ipage = (struct Page *) (Index_PageID_Pointer(*new_page_id));
            SetBufIndexSegment(issys_ipage, *new_page_id);
            next = ipage->header_.link_.next_;
            ipage = (struct Page *) (Index_PageID_Pointer(next));
            SetBufIndexSegment(issys_next, next);

            ipage->header_.link_.prev_ = NULL_OID;

            last_isn = Index_getSegmentNo(next);

            ret = DBFile_Write_OC(DBFILE_INDEX_TYPE, last_isn, 0,
                    (void *) &(segment_[last_isn]->page_[0]),
                    PAGE_PER_SEGMENT, 0);
            UnsetBufIndexSegment(issys_ipage);
            UnsetBufIndexSegment(issys_next);

            DBDataMem_Free(Mem_mgr->segment_memid[last_isn]);
            segment_[last_isn] = NULL;
            Mem_mgr->segment_memid[last_isn] = 0;

            if (ret < 0)
            {
                MDB_SYSLOG(("MemMgr Segment Allocate Append FAIL\n"));
                return ret;
            }
        }
#endif
    }

    ipage = (struct Page *) (Index_PageID_Pointer(*new_page_id));
    next = ipage->header_.link_.next_;

    SetBufIndexSegment(issys_ipage, *new_page_id);

    if (next != NULL_PID)
    {
        Index_MemMgr_SetPrevPageID(next, NULL_PID);
    }

    UnsetBufIndexSegment(issys_ipage);

    link = ipage->header_.link_;
    link.prev_ = link.next_ = NULL_PID;
    tail = pagelist->tail_;

    if (tail != NULL_PID)
    {
        link.prev_ = tail;
        Index_MemMgr_SetPageLink(*new_page_id, link);
        Index_MemMgr_SetNextPageID(tail, *new_page_id);
        pagelist->tail_ = *new_page_id;
    }
    else
    {
        link.prev_ = NULL_PID;
        Index_MemMgr_SetPageLink(*new_page_id, link);
        pagelist->head_ = *new_page_id;
        pagelist->tail_ = *new_page_id;
    }

    SetDirtyFlag(collect->cont_id_);

    anchor = *(mem_anchor_);
    anchor.ifree_page_count_ -= 1;
    anchor.ifirst_free_page_id_ = next;
    MemMgr_SetMemAnchor(&anchor);

    return DB_SUCCESS;
}

int
Index_MemMgr_FreeAllNodes(struct TTree *ttree)
{
    IPageID node_id, parent_id;
    int i;
    struct TTreeNode *node, *parent;

#ifdef INDEX_BUFFERING
    int issys_node = 0;
    OID oid_node = NULL_OID;
#endif

    for (i = 0; i < NUM_TTREE_BUCKET; i++)
    {
        if (ttree->root_[i] == NULL_OID)
            continue;

        node_id = ttree->root_[i];
        while (node_id)
        {
#ifdef INDEX_BUFFERING
            UnsetBufIndexSegment(issys_node);
            oid_node = node_id;
            SetBufIndexSegment(issys_node, oid_node);
#endif
            node = (struct TTreeNode *) Index_OID_Point(node_id);
            if (node == NULL)
            {
#ifdef INDEX_BUFFERING
                UnsetBufIndexSegment(issys_node);
#endif
                return DB_E_OIDPOINTER;
            }
            if (node->left_)
                node_id = node->left_;
            else if (node->right_)
                node_id = node->right_;
            else        /* left and right are null */
            {
                NodeType node_type = node->type_;

                parent_id = node->parent_;

                Index_Col_Remove(NULL, node_id);

                if (parent_id == NULL_OID)
                    break;

                parent = (struct TTreeNode *) Index_OID_Point(parent_id);
                if (parent == NULL)
                {
                    return DB_E_OIDPOINTER;
                }
                if (node_type == LEFT_CHILD)
                {
                    parent->left_ = NULL_OID;
                    Index_SetDirtyFlag(parent_id);
                }
                else if (node_type == RIGHT_CHILD)
                {
                    parent->right_ = NULL_OID;
                    Index_SetDirtyFlag(parent_id);
                }


                node_id = parent_id;
            }
        }
#ifdef INDEX_BUFFERING
        UnsetBufIndexSegment(issys_node);
#endif

        ttree->root_[i] = NULL_OID;

        SetDirtyFlag(ttree->collect_.cont_id_);
    }

    ttree->modified_time_++;

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

#ifdef INDEX_BUFFERING
int
__SetBufIndexSegment(OID oid)
{
    int issys;
    int __sn = Index_getSegmentNo((oid));

    if (!isBufIndexSegment(__sn))
    {
        issys = __sn;
        ((Mem_mgr->bufSegment[DIV8(__sn)]) |= (0x80U >> (MOD8(__sn))));
        ++count_SetBufIndexSegment;
    }
    else
    {
        issys = 0;
    }

    return issys;
}

int
__UnsetBufIndexSegment(int issys)
{
    if (issys)
    {
        ((Mem_mgr->bufSegment[DIV8(issys)]) &= ~(0x80U >> (MOD8(issys))));
        --count_SetBufIndexSegment;
        issys = 0;
    }

    return issys;
}
#endif /* INDEX_BUFFERING */
