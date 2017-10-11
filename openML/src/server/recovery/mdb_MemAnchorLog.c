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
#include "mdb_inner_define.h"
#include "mdb_compat.h"
#include "mdb_Cont.h"
#include "mdb_LogRec.h"
#include "mdb_Mem_Mgr.h"
#include "mdb_syslog.h"

static PageID null_pid = NULL_PID;

int CLR_WriteCLR(struct LogRec *logrec);

int UpdateTransTable(struct LogRec *logrec);

int Catalog_PageAllocate_Redo(OID SystemCatalog, PageID pageid,
        struct PageallocateLogData *log_data);
int Relation_PageAllocate_Redo(OID SystemCatalog, int collect_index,
        PageID pageid, struct PageallocateLogData *log_data);
int Catalog_PageDeallocate_Redo(OID SystemCatalog, PageID pageid,
        struct PageallocateLogData *log_data);
int Relation_PageDeallocate_Redo(OID SystemCatalog, int collect_index,
        PageID pageid, struct PageallocateLogData *log_data);
int PageAllocate_Redo(struct Collection *, PageID, PageID, PageID, int);
int PageDeallocate_Redo(struct Collection *, PageID, PageID, PageID, int);
void Print_Page(PageID pid);

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
MemAnchorLog_Redo(struct LogRec *logrec)
{
    OID cont_id;
    struct PageallocateLogData log_data;
    PageID pageid;

    UpdateTransTable(logrec);

    cont_id = (OID) (logrec->header_.relation_pointer_);
    pageid = (PageID) logrec->header_.oid_;

    sc_memcpy(&log_data, logrec->data1, sizeof(log_data));

    switch (logrec->header_.op_type_)
    {
    case RELATION_CATALOG_PAGEALLOC:
    case INDEX_CATALOG_PAGEALLOC:
        Catalog_PageAllocate_Redo(cont_id, pageid, &log_data);
        break;
    case RELATION_PAGEALLOC:
        Relation_PageAllocate_Redo(cont_id, logrec->header_.collect_index_,
                pageid, &log_data);
        break;
    case RELATION_CATALOG_PAGEDEALLOC:
    case INDEX_CATALOG_PAGEDEALLOC:
        Catalog_PageDeallocate_Redo(cont_id, pageid, &log_data);
        break;
    case RELATION_PAGETRUNCATE:
    case RELATION_PAGEDEALLOC:
        Relation_PageDeallocate_Redo(cont_id,
                logrec->header_.collect_index_, pageid, &log_data);
        break;
    default:
        return DB_E_INVALIDLOGTYPE;
    }

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
SegmentAllocLog_Redo(struct LogRec *logrec)
{
    PageID pageid;
    OID sn;
    int ret;

    sc_memcpy(&pageid, logrec->data1, sizeof(OID));
    sc_memcpy(&sn, logrec->data1 + sizeof(OID), sizeof(OID));

    if (pageid == NULL_OID)
        ret = MemMgr_Check_FreeSegment();
    else
    {
        ret = MemMgr_AllocateSegment(pageid, (DB_UINT32) sn, SEGMENT_UNKN);
    }

    if (ret < 0)
        return ret;

    MDB_SYSLOG(("Segment Allocation Redo: segment %d, page %d\n", sn, pageid));

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
Catalog_PageAllocate_Redo(OID SystemCatalog, PageID pageid,
        struct PageallocateLogData *log_data)
{
    struct Collection *collect;
    int issys;
    PageID tail_pid = log_data->tail_id;
    PageID next_pid = log_data->next_id;
    int free_count = log_data->free_count;

    collect = (struct Collection *) OID_Point(SystemCatalog);
    if (collect == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    SetBufSegment(issys, SystemCatalog);

    PageAllocate_Redo(collect, pageid, tail_pid, next_pid, free_count);

    Collect_InitializePage(SystemCatalog, collect, pageid);

    collect->page_count_ = log_data->page_count;
    SetDirtyFlag(collect->cont_id_);

    UnsetBufSegment(issys);

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
Relation_PageAllocate_Redo(OID SystemCatalog, int collect_index,
        PageID pageid, struct PageallocateLogData *log_data)
{
    struct Container *Cont;
    struct Collection *collect;
    int issys;
    PageID tail_pid = log_data->tail_id;
    PageID next_pid = log_data->next_id;
    int free_count = log_data->free_count;
    OID collect_free_page_list = log_data->collect_free_page_list;

    Cont = (struct Container *) OID_Point(SystemCatalog);
    if (Cont == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    if (collect_index == -1)
        collect = &(Cont->collect_);
    else
        collect = &Cont->var_collects_[collect_index];

    SetBufSegment(issys, SystemCatalog);

    PageAllocate_Redo(collect, pageid, tail_pid, next_pid, free_count);

    Collect_InitializePage(SystemCatalog, collect, pageid);

    /* fix free_page_list recovery */
    if (collect->free_page_list_ != NULL_OID)
    {
        struct Page *page;

        page = (struct Page *) (PageID_Pointer(pageid));
        if (page == NULL)
        {
            UnsetBufSegment(issys);
            return DB_E_OIDPOINTER;
        }

        if (page->header_.free_page_next_ != collect_free_page_list)
        {
            page->header_.free_page_next_ = collect_free_page_list;
            SetDirtyFlag(pageid);
        }
    }

    collect->page_count_ = log_data->page_count;
    SetDirtyFlag(collect->cont_id_);

    UnsetBufSegment(issys);

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
Catalog_PageDeallocate_Redo(OID SystemCatalog, PageID pageid,
        struct PageallocateLogData *log_data)
{
    struct Collection *collect;
    int issys;
    PageID tail_pid = log_data->tail_id;
    PageID next_pid = log_data->next_id;
    int free_count = log_data->free_count;

    collect = (struct Collection *) OID_Point(SystemCatalog);
    if (collect == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    SetBufSegment(issys, SystemCatalog);

    PageDeallocate_Redo(collect, pageid, tail_pid, next_pid, free_count);

    collect->page_count_ = log_data->page_count;
    SetDirtyFlag(collect->cont_id_);

    UnsetBufSegment(issys);

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
Relation_PageDeallocate_Redo(OID SystemCatalog, int collect_index,
        PageID pageid, struct PageallocateLogData *log_data)
{
    struct Container *Cont;
    struct Collection *collect;
    int issys;
    PageID tail_pid = log_data->tail_id;
    PageID next_pid = log_data->next_id;
    int free_count = log_data->free_count;
    int issys_ttree;

    Cont = (struct Container *) OID_Point(SystemCatalog);
    if (Cont == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    if (collect_index == -1)
        collect = &Cont->collect_;
    else
        collect = &Cont->var_collects_[collect_index];

    SetBufSegment(issys, SystemCatalog);

    if (Cont->base_cont_.index_count_ > 0 &&
            f_checkpoint_finished >= CHKPT_CMPLT)
    {
        OID index_id;
        struct TTree *ttree;

        index_id = Cont->base_cont_.index_id_;
        while (index_id != NULL_OID)
        {
            ttree = (struct TTree *) OID_Point(index_id);
            if (ttree == NULL)
            {
                UnsetBufSegment(issys);
                return DB_E_OIDPOINTER;
            }

            SetBufSegment(issys_ttree, index_id);

            if (Index_MemMgr_FreeAllNodes(ttree) < 0)
            {
                f_checkpoint_finished = CHKPT_BEGIN;
            }
            SetDirtyFlag(index_id);

            UnsetBufSegment(issys_ttree);

            index_id = ttree->base_cont_.index_id_;
        }
    }

    PageDeallocate_Redo(collect, pageid, tail_pid, next_pid, free_count);

    collect->page_count_ = log_data->page_count;
    SetDirtyFlag(collect->cont_id_);

    UnsetBufSegment(issys);

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
PageAllocate_Redo(Collection_t * collect, PageID new_page_id,
        PageID tail_pid, PageID next_pid, int free_count)
{
    struct PageLink link;
    struct PageList *pagelist = &collect->page_link_;

    if (next_pid != NULL_PID)   /* 다음 page의 prev_를 NULL로 만듬 */
    {
        OID_LightWrite((next_pid + sizeof(PageID)), (const char *) &null_pid,
                sizeof(PageID));
    }

    link.prev_ = link.next_ = NULL_PID;

    if (tail_pid != NULL_PID)
    {
        link.prev_ = tail_pid;
        OID_LightWrite((new_page_id + sizeof(PageID)), (const char *) &link,
                LINK_SIZE);
        OID_LightWrite((tail_pid + sizeof(PageID) * 2),
                (const char *) &new_page_id, sizeof(PageID));
        pagelist->tail_ = new_page_id;
    }
    else
    {
        link.prev_ = NULL_PID;
        OID_LightWrite((new_page_id + sizeof(PageID)), (const char *) &link,
                LINK_SIZE);
        pagelist->head_ = new_page_id;
        pagelist->tail_ = new_page_id;
    }

    collect->free_page_list_ = new_page_id;

    SetDirtyFlag(collect->cont_id_);

    mem_anchor_->free_page_count_ = free_count;
    mem_anchor_->first_free_page_id_ = next_pid;
    SetDirtyFlag(PAGE_HEADER_SIZE);

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
 * \note table page 전체 해제를 고려함
 *****************************************************************************/
int
PageDeallocate_Redo(struct Collection *collect, PageID page_id,
        PageID tail_pid, PageID next_pid, int free_count)
{
    struct PageList *pagelist = &collect->page_link_;
    int f_make_link = 1;

    if (f_make_link)
    {
        OID_LightWrite((tail_pid + sizeof(PageID) * 2), (char *) &next_pid,
                sizeof(PageID));
        OID_LightWrite((page_id + sizeof(PageID)), (char *) &null_pid,
                sizeof(PageID));
        if (next_pid != NULL_PID)
        {
            OID_LightWrite((next_pid + sizeof(PageID)), (char *) &tail_pid,
                    sizeof(PageID));
        }
    }

    mem_anchor_->free_page_count_ = free_count;
    mem_anchor_->first_free_page_id_ = page_id;
    SetDirtyFlag(PAGE_HEADER_SIZE);
    if (page_id != NULL_OID)
    {
        struct Page *page;

        page = (struct Page *) (PageID_Pointer(page_id));
        if (page)
        {
            page->header_.link_.prev_ = NULL_OID;
            SetDirtyFlag(page_id);
        }
    }

    pagelist->head_ = NULL_OID;
    pagelist->tail_ = NULL_OID;

    collect->item_count_ = 0;
    collect->free_page_list_ = NULL_PID;

    SetDirtyFlag(collect->cont_id_);

    return DB_SUCCESS;
}
