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
#include "mdb_PMEM.h"

void OID_SaveAsBefore(short op_type, const void *, OID id_, int size);
void OID_SaveAsAfter(short op_type, const void *, OID id_, int size);
int OID_SlotAllocLog(short Op_Type, const void *Relation, OID curr,
        char *data, DB_INT32 size);
int
OID_InsertLog(short Op_Type, int trans_id, const void *Relation,
        int collect_index, OID id_, char *item, int size, OID next_free_oid,
        DB_UINT8 slot_type);

int OID_DeleteLog(short Op_Type, int trans_id, const void *Relation,
        int collect_index, OID id_, int size, char *rep_data, int rep_size,
        DB_UINT8 slot_type);
int OID_UpdateLog(short Op_Type, int trans_id, const void *Relation, OID curr,
        char *data, DB_INT32 size, char *rep_data, int rep_size);
int OID_UpdateFieldLog(short Op_Type, int trans_id, const void *Relation,
        OID curr, char *data, DB_INT32 size, char *rep_data, int rep_size);

struct Container *Cont_Search(const char *cont_name);
void check_this(void);

extern int KeyValue_Create(const char *record, struct KeyDesc *key_desc,
        int recSize, struct KeyValue *keyValue, int f_memory_record);
extern int KeyValue_Delete2(struct KeyValue *pKeyValue);

#define CHECK_FOUND_FREE_SLOT(page, oid)                                     \
    if ((page) == NULL || (page)->header_.free_slot_id_in_page_ != (oid))    \
    {                                                                        \
        MDB_SYSLOG(("page == NULL or free_slot_id_in_page_ != new_oid \n")); \
        return DB_E_FREESLOT;                                                \
    }

/*****************************************************************************/
//! Collect_InitializePage
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param cont_oid :
 * \param col : 
 * \param pid :
 ************************************
 * \return  return_value
 ************************************
 * \note Call By :  Collect_AllocateSlot()
 *****************************************************************************/
int
Collect_InitializePage(ContainerID cont_oid, const struct Collection *col,
        PageID pid)
{
    struct Page *page;
    char *page_body;
    DB_UINT32 sn;
    DB_UINT32 pn;
    DB_INT32 slot_no;
    OID next_oid;
    OID Last_OID = NULL_OID;
    DB_INT32 slot_size = col->slot_size_;

    page = (struct Page *) PageID_Pointer(pid);
    if (page == NULL)
    {
        return DB_E_OIDPOINTER;
    }
    sn = getSegmentNo(pid);
    pn = getPageNo(pid);

    page->header_.free_slot_id_in_page_ = NULL_OID;
    page->header_.record_count_in_page_ = 0;
    page->header_.cont_id_ = cont_oid;
    page->header_.free_page_next_ = NULL_OID;
    page->header_.collect_index_ = col->collect_index_;

    page_body = (char *) (page->body_);
    sc_memset(page_body, 0, PAGE_BODY_SIZE);

    for (slot_no = 0; slot_no < SLOT_PER_PAGE(col->slot_size_) - 1; slot_no++)
    {
        next_oid =
                OID_Cal(sn, pn, PAGE_HEADER_SIZE + (slot_no + 1) * slot_size);

        sc_memcpy(&(page->body_[slot_no * slot_size]), &next_oid, sizeof(OID));

        page->body_[slot_no * slot_size + slot_size - 1] = FREE_SLOT;
    }

    sc_memcpy(&(page->body_[(SLOT_PER_PAGE(col->slot_size_) - 1) * slot_size]),
            &Last_OID, sizeof(OID));
    page->body_[SLOT_PER_PAGE(col->slot_size_) * slot_size - 1] = FREE_SLOT;

    page->header_.free_slot_id_in_page_ = OID_Cal(sn, pn, PAGE_HEADER_SIZE);

    SetPageDirty(sn, pn);

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
_PRIVATE int
Collect_RecoverPage(const struct Collection *col, PageID pid)
{
    struct Page *page;
    DB_UINT32 sn;
    DB_UINT32 pn;
    DB_INT32 slot_no;
    OID slotID;
    char *pSlot;
    int issys;

    page = (struct Page *) PageID_Pointer(pid);
    if (page == NULL)
    {
        return DB_E_OIDPOINTER;
    }
    sn = getSegmentNo(pid);
    pn = getPageNo(pid);

    page->header_.free_slot_id_in_page_ = NULL_OID;
    page->header_.record_count_in_page_ = 0;

    SetBufSegment(issys, pid);

    page = (struct Page *) PageID_Pointer(pid);
    if (page == NULL)
    {
        UnsetBufSegment(issys);
        return DB_E_OIDPOINTER;
    }

    for (slot_no = SLOT_PER_PAGE(col->slot_size_); slot_no > 0; slot_no--)
    {
        slotID = OID_Cal(sn, pn,
                PAGE_HEADER_SIZE + (slot_no - 1) * col->slot_size_);
        pSlot = (char *) OID_Point(slotID);
        if (pSlot == NULL)
        {
            UnsetBufSegment(issys);
            return DB_E_OIDPOINTER;
        }

        if (pSlot[col->slot_size_ - 1] & USED_SLOT)     /* used slot */
        {
            page->header_.record_count_in_page_ += 1;
        }
        else    /* free slot */
        {
            sc_memcpy(pSlot, &(page->header_.free_slot_id_in_page_),
                    sizeof(OID));

            page->header_.free_slot_id_in_page_ = slotID;

        }
    }

    SetPageDirty(sn, pn);

    UnsetBufSegment(issys);

    return DB_SUCCESS;
}

int
__Col_IsDataSlot(OID curr, DB_INT32 slot_size, DB_UINT8 msync_flag)
{
    unsigned char *p;

    p = (unsigned char *) OID_Point(curr);
    if (p == NULL)
    {
        return 0;
    }

    if (msync_flag)
    {
        return (*((unsigned char *) p + slot_size - 1) == msync_flag);
    }
    else
    {
        return (*((unsigned char *) p + slot_size - 1) & USED_SLOT);
    }
}

//===================================================================
//
//Function Name        : Col_GetNextDataID
//Call Funtion Name : Cont_Search(), Iter_FindNext()
//
//===================================================================
void
__Col_GetNextDataID(OID * curr, DB_INT32 slot_size, PageID * tail,
        DB_UINT8 msync_flag)
{
    register unsigned long _offset;
    register struct Page *_page;

    if (*curr == NULL_OID)
        return;

    _offset = OID_GetOffSet(*curr) + slot_size - 1;
    _page = (struct Page *) PageID_Pointer(*curr);
    if (_page == NULL)
    {
        *curr = INVALID_OID;
        return;
    }

    while (1)
    {
        /* does next_id exist in the same page with current_id? */
        if (_offset + slot_size <= S_PAGE_SIZE)
        {
            *curr += slot_size;
            _offset += slot_size;
        }
        else if (getPageOid(*curr) == *tail)
        {
            *curr = NULL_OID;
            return;
        }
        else
        {
            if ((*curr = _page->header_.link_.next_) == NULL_OID)
                return;
            *curr += PAGE_HEADER_SIZE;
            _page = (struct Page *) PageID_Pointer(*curr);
            if (_page == NULL)
            {
                *curr = INVALID_OID;
                return;
            }
            _offset = PAGE_HEADER_SIZE + slot_size - 1;
        }
        /* next_id is data slot? */
        if (msync_flag)
        {
            if (*((unsigned char *) _page + _offset) == msync_flag)
            {
                return;
            }
        }
        else if (*((unsigned char *) _page + _offset) & USED_SLOT)
        {
            return;
        }
    }
}

int
Collect_reconstruct_freepagelist(struct Collection *collect)
{
    struct Page *nextPage = NULL;
    struct Page *freePage = NULL;

    OID freePageId = NULL_PID;
    OID nextPageId = collect->page_link_.head_;

    unsigned int offset;
    int foundCount = 0;

    __SYSLOG("Alert!!! FreePageList reconstruction Start\n");

    collect->free_page_list_ = NULL_OID;
    SetDirtyFlag(collect->cont_id_);

    while (nextPageId != NULL_PID)
    {
        nextPage = (struct Page *) PageID_Pointer(nextPageId);
        if (nextPage == NULL)
        {
            MDB_SYSLOG(("FreePageList reconstruction Fail\n"));
            return DB_E_INVALID_RID;
        }

        /* Do you have any bad feeling about the page? 
           Do it, RecoveryPage routine */
        offset = OID_GetOffSet(nextPage->header_.free_slot_id_in_page_);

        if (offset > S_PAGE_SIZE ||
                nextPage->header_.record_count_in_page_ >
                SLOT_PER_PAGE(collect->slot_size_))
        {
            Collect_RecoverPage(collect, nextPageId);
        }

        nextPage = (struct Page *) PageID_Pointer(nextPageId);
        if (nextPage == NULL)
        {
            return DB_E_OIDPOINTER;
        }
        nextPage->header_.free_page_next_ = NULL_OID;
        SetDirtyFlag(nextPageId);

        if (nextPage->header_.free_slot_id_in_page_ != NULL_OID)
        {
            if (foundCount == 0)
            {
                collect->free_page_list_ = nextPageId;
                freePageId = nextPageId;
                SetDirtyFlag(collect->cont_id_);
            }
            else
            {
                freePage = (struct Page *) PageID_Pointer(freePageId);
                if (freePage == NULL)
                {
                    return DB_E_OIDPOINTER;
                }

                freePage->header_.free_page_next_ = nextPageId;
#ifdef MDB_DEBUG
                sc_assert(freePage->header_.self_ !=
                        freePage->header_.free_page_next_, __FILE__, __LINE__);
#endif
                SetDirtyFlag(freePage->header_.self_);
                freePageId = nextPageId;
            }

            foundCount += 1;
        }
        else
        {   /* full page */
            nextPage->header_.free_page_next_ = FULL_PID;
        }

        if (nextPageId == collect->page_link_.tail_)
            break;

        nextPage = (struct Page *) PageID_Pointer(nextPageId);
        if (nextPage == NULL)
        {
            return DB_E_OIDPOINTER;
        }
        nextPageId = nextPage->header_.link_.next_;
    }


    MDB_SYSLOG(("cont_id(%d) has %d free page(s).\n", collect->cont_id_,
                    foundCount));

    return 0;
}

/*****************************************************************************/
//! Free_Page_Search 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param collect(in/out)   :
 ************************************
 * \return  NULL is Error.. else is Success
 ************************************
 * \note 내부 알고리즘\n
 *  Call by :  Create_Table(), Col_Insert()
 *****************************************************************************/
struct Page *
Free_Page_Search(struct Collection *collect)
{
    struct Page *page;
    unsigned int sn, pn;

    while (collect->free_page_list_ != NULL_PID)
    {
        sn = getSegmentNo(collect->free_page_list_);
        pn = getPageNo(collect->free_page_list_);

        if (sn > SEGMENT_NO || pn > PAGE_PER_SEGMENT ||
                collect->free_page_list_ == FULL_PID)
        {
            Collect_reconstruct_freepagelist(collect);
        }

        if (collect->free_page_list_ == NULL_PID)
            break;

        page = (struct Page *) PageID_Pointer(collect->free_page_list_);
        if (page == NULL)
        {
            MDB_SYSLOG((" Free Page Search FAIL \n"));
            return NULL;
        }

        if (page->header_.cont_id_ != collect->cont_id_)
        {
            collect->free_page_list_ = NULL_OID;
            SetDirtyFlag(collect->cont_id_);
            return NULL;
        }

        if (page->header_.free_slot_id_in_page_ != NULL_OID)
            return page;

        MDB_SYSLOG(
                ("Collect first free page(%d|%d) does not have free slots.\n",
                        getSegmentNo(collect->free_page_list_),
                        getPageNo(collect->free_page_list_)));
        /* Actually, the page has no free slots */
        collect->free_page_list_ = page->header_.free_page_next_;
        page->header_.free_page_next_ = FULL_PID;

        SetDirtyFlag(page->header_.self_);
        SetDirtyFlag(collect->cont_id_);
    }

    /* no page that has free slots */
    return NULL;
}

/*****************************************************************************/
//! Collect_AllocateSlot 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param Cont :
 * \param collect : 
 * \param new_slot_id :
 * \param find_only_flag :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
Collect_AllocateSlot(struct Container *Cont, struct Collection *collect,
        OID * new_slot_id, int find_only_flag)
{
    struct Page *free_page;
    OID slot_oid;
    int ret;
    int issys;

    if (collect == NULL)
        collect = &(Cont->collect_);

  Try_Search_Again:

    free_page = Free_Page_Search(collect);
    if (free_page != NULL)
    {
        char *ptr;

        /*
           Collect_RecoverPage()를 호출하는 경우는 page의 free list head에
           문제가 발생되어 있을 때임.
           Free_Page_Search()를 수행 후 collect->last_accessed_pageId는
           free_page를 가르키게 됨.
           그런데, free_page->header_.free_slot_id_in_page_의
           값이 깨어지는 경우가 발생함.
           rollback이 정확하게 되지 않아서 발생한 듯.
           이 경우 page의 free list를 다시 정비하는 작업을 위해서
           Collect_RecoverPage()를 호출함.

           used slot이 free list에 남아있게 되는 경우 발생?
           정확하지는 않지만 이런 경우가 발생함. 아마도 temp table drop 처리가
           제대로 안된 상황의 코드에서 발생한 듯... 이런 상태의 DB도 지원하기
           할당되어질 free list의 첫번째 slot이 USED_SLOT이면 page 내 free list
           다시 정렬하기 위해 추가함.
         */
        if (IS_CRASHED_PAGE(getSegmentNo(free_page->header_.self_),
                        getPageNo(free_page->header_.self_), free_page))
        {
            return DB_E_INVALIDPAGE;
        }
        slot_oid = free_page->header_.free_slot_id_in_page_;
        ptr = (char *) OID_Point(slot_oid);
        if (ptr == NULL)
        {
            return DB_E_OIDPOINTER;
        }

        ptr = (char *) ptr + collect->slot_size_ - 1;

        if (getSegmentNo(slot_oid) != getSegmentNo(collect->free_page_list_) ||
                getPageNo(slot_oid) != getPageNo(collect->free_page_list_) ||
                OID_GetOffSet(slot_oid) < PAGE_HEADER_SIZE ||
                OID_GetOffSet(slot_oid) >= S_PAGE_SIZE || (*ptr & USED_SLOT))
        {
            __SYSLOG("Collect info. cont name = %s collect_index = %d\n",
                    Cont->base_cont_.name_, collect->collect_index_);

            MDB_SYSLOG(("Recovery Page pid(%d, %d) sid(%d, %d, %d)",
                            getSegmentNo(collect->free_page_list_),
                            getPageNo(collect->free_page_list_),
                            getSegmentNo(slot_oid), getPageNo(slot_oid),
                            OID_GetOffSet(slot_oid)));

            Collect_RecoverPage(collect, collect->free_page_list_);

            MDB_SYSLOG(("new sid(%d, %d, %d)\n",
                            getSegmentNo(free_page->header_.
                                    free_slot_id_in_page_),
                            getPageNo(free_page->header_.
                                    free_slot_id_in_page_),
                            OID_GetOffSet(free_page->header_.
                                    free_slot_id_in_page_)));

            goto Try_Search_Again;
        }
    }

    if (free_page == NULL)
    {
        PageID pid;

        ret = MemMgr_AllocatePage(Cont, collect, &pid);
        if (ret < 0)
        {
            *new_slot_id = NULL_OID;
            return ret;
        }

        free_page = (struct Page *) PageID_Pointer(pid);
        if (free_page == NULL)
        {
            *new_slot_id = NULL_OID;
            return DB_E_OIDPOINTER;
        }

    }

    SetBufSegment(issys, free_page->header_.self_);

    *new_slot_id = free_page->header_.free_slot_id_in_page_;

    if (!find_only_flag)
    {
        OID slot_state;
        char used_slot_byte = USED_SLOT;
        char *where;
        char *pbuf = NULL;

        /* logging */
        slot_state = *new_slot_id + (collect->slot_size_ - 1);
        OID_SlotAllocLog(SYS_SLOTUSE_BIT, Cont, slot_state,
                &used_slot_byte, 1);

        /* disconnect the slot from free slot list */
        pbuf = (char *) OID_Point(free_page->header_.free_slot_id_in_page_);
        if (pbuf == NULL)
        {
            UnsetBufSegment(issys);
            return DB_E_OIDPOINTER;
        }

        free_page->header_.free_slot_id_in_page_ = *((OID *) pbuf);

        free_page->header_.record_count_in_page_ += 1;

        /* set the slot status flag as USED_SLOT */
        where = (char *) free_page + OID_GetOffSet(slot_state);
        *where = used_slot_byte;

        if (free_page->header_.free_slot_id_in_page_ == NULL_OID)
        {
            collect->free_page_list_ = free_page->header_.free_page_next_;
            free_page->header_.free_page_next_ = FULL_PID;
        }

        collect->item_count_ += 1;

        SetDirtyFlag(slot_state);

        SetDirtyFlag(collect->cont_id_);

    }

    UnsetBufSegment(issys);

    return DB_SUCCESS;
}

//===================================================================
//
// Function  Name:
//
//===================================================================
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
Col_GetFirstDataID(struct Container * Cont, DB_UINT8 msync_flag)
{
    PageID head;
    PageID tail;
    OID record_oid;
    DB_INT32 size;

    if (Cont == NULL)
        return NULL_OID;

    head = Cont->collect_.page_link_.head_;
    tail = Cont->collect_.page_link_.tail_;

    size = Cont->collect_.slot_size_;

    if (head == NULL_PID)
        return NULL_OID;

    record_oid =
            OID_Cal(getSegmentNo(head), getPageNo(head), PAGE_HEADER_SIZE);

    /* 사용 중인 첫번째 data slot(record)을 찾는다 */
    if (Col_IsDataSlot(record_oid, size, msync_flag))
    {
        return record_oid;
    }
    else
    {
        Col_GetNextDataID(record_oid, size, tail, msync_flag);
        if (record_oid == INVALID_OID)
        {
            return DB_E_OIDPOINTER;
        }
        return record_oid;
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
OID
Col_GetFirstDataID_Page(struct Container * Cont, OID page, DB_UINT8 msync_flag)
{
    PageID head;
    PageID tail;
    OID record_oid;
    DB_INT32 size;

    if (page == NULL_PID)
    {
        head = Cont->collect_.page_link_.head_;
        tail = Cont->collect_.page_link_.tail_;
    }
    else
    {
        head = tail = page;
    }

    if (head == NULL_PID)
        return NULL_OID;


    size = Cont->collect_.slot_size_;
    record_oid =
            OID_Cal(getSegmentNo(head), getPageNo(head), PAGE_HEADER_SIZE);

    /* 사용 중인 첫번째 data slot(record)을 찾는다 */
    if (Col_IsDataSlot(record_oid, size, msync_flag))
    {
        return record_oid;
    }
    else
    {
        Col_GetNextDataID(record_oid, size, tail, msync_flag);
        if (record_oid == INVALID_OID)
        {
            return DB_E_OIDPOINTER;
        }
        return record_oid;
    }
}

/*****************************************************************************/
//! Col_Choose_collection
/*! \breif  EXTENDED 공간을 찾아주는 함수
 ************************************
 * \param Cont(in)          :
 * \param var_size(in)      : 
 * \param collect(in/out)   :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *  - called : Col_Insert_Variable()
 * 
 *****************************************************************************/
DB_INT32
Col_Choose_collection(struct Container * Cont, DB_INT32 var_size,
        struct Collection ** collect)
{
    struct Collection *collections = Cont->var_collects_;
    DB_INT32 i;

    if (var_size <= collections[0].record_size_)
    {
        *collect = &collections[0];
        return 0;
    }

    if (collections[0].record_size_ >= var_size)
    {
        *collect = &collections[0];
        return 0;
    }

    /* later, make more good algorithm^^ */
    for (i = 1; i < VAR_COLLECT_CNT; i++)
    {

        if (collections[i - 1].record_size_ < var_size &&
                var_size <= collections[i].record_size_)
        {
            *collect = &collections[i];
            return i;
        }
    }

    return -1;
}

/*****************************************************************************/
//! Col_Insert 
/*! \breif  VARIABLE FIELD를 INSERT한다
 ************************************
 * \param Cont(in/out)      : containter's info
 * \param collect_index(in) : -1 is heap record insert, else is variable data
 * \param item(in)          : field buffer(extended 된 부분)
 * \param opentime()        :
 * \param qsid()            :
 * \param record_id(in/out) : record's OID
 * \param val_len(in)       : value's byte length(extended part의 length)
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  Call By : __Insert(), Col_Insert_Variable()
 *  - record format reference : _Schema_CheckFieldDesc()
 *  - algorithm
 *      WAL(Write Ahead Logging)의 원칙에 따르면 아래와 같은 순서로 진행되어야 한다
 *          step 1 : collection에서 slot을 하나 결정()
 *          step 2 : logging for insertion 
 *          step 3 : allocate the slot 
 *          step 4 : write the record image into the slot 
 *          step 5 : set the dirty flag 
 *      하지만 extended 된 영역의 앞 부분에 4 byte의 length를 넣어 주기 위해서..
 *      WAL의 원칙을 약간 느슨하게 적용하였다
 *          step 1 : collection에서 slot을 하나 결정()
 *          step 2 : allocate the slot 
 *          step 3 : write the record image into the slot 
 *          step 4 : logging for insertion 
 *          step 5 : set the dirty flag 
 *  
 *  - extended 된 영역의 앞 부분에 4 byte의 length를 넣어 주기 위해서..
 *      algorith을 수정함
 *
 *****************************************************************************/
int
Col_Insert(struct Container *Cont, int collect_index,
        char *item, DB_UINT32 opentime, DB_UINT16 qsid, OID * record_id,
        int val_len, DB_UINT8 msync_flag)
{
    struct Collection *collection;
    short op_type;
    OID new_slot_oid;
    int hf_offset;
    struct Page *page;
    char *record;
    int ret_val;
    int issys = 0;
    char *rec_buf;
    int trans_id = *(int *) CS_getspecific(my_trans_id);
    OID next_free_oid;
    DB_INT8 is_msync_insert = 0;

    if (collect_index == -1)
    {
        /* heap record insert */
        collection = &(Cont->collect_);
        op_type = INSERT_HEAPREC;
    }
    else
    {
        /* varchar data insert */
        collection = &(Cont->var_collects_[collect_index]);
        op_type = INSERT_VARDATA;
    }

    opentime = opentime;
    qsid = qsid;

    rec_buf = item;

    if (op_type == INSERT_HEAPREC)
    {
        if ((Cont->base_cont_.type_ & _CONT_MSYNC)
                && (msync_flag & SYNCED_SLOT))
        {
            is_msync_insert = 1;
        }

        /* utime 설정 */
        hf_offset = collection->slot_size_ - 8;
        sc_memcpy(rec_buf + hf_offset, &opentime, sizeof(DB_UINT32));
    }

    /* step 1 */

    ret_val = Collect_AllocateSlot(Cont, collection, &new_slot_oid, 1);
    if (ret_val < 0)
    {
        MDB_SYSLOG(("find free new slot FAIL %s, %d\n", __FILE__, __LINE__));
        *record_id = NULL_OID;

        return ret_val;
    }

    page = (struct Page *) PageID_Pointer(new_slot_oid);
    if (page == NULL || page->header_.free_slot_id_in_page_ != new_slot_oid)
    {
        MDB_SYSLOG(("free_slot_id_in_page_ != new_slot_oid\n"));
        *record_id = NULL_OID;
        return DB_E_VERIFYNEWSLOT;
    }

    if (op_type == INSERT_HEAPREC)
    {
        SetBufSegment(issys, new_slot_oid);
    }

    /* step 2 : get the next free slot oid of new_slot_oid */
    record = (char *) page + OID_GetOffSet(new_slot_oid);
    next_free_oid = *(OID *) record;

    /* step 3 : write the record image into the slot */
    if (val_len)
    {
        if (op_type == INSERT_VARDATA)
        {
            // 아래에서 val_len은 length까지 포함된 길이임
            DB_EXT_PART_LEN ext_length = (DB_EXT_PART_LEN)
                    (val_len - MDB_EXT_LEN_SIZE);

            sc_memcpy(record, (char *) (&ext_length), MDB_EXT_LEN_SIZE);
            sc_memcpy(record + MDB_EXT_LEN_SIZE, rec_buf, ext_length);
        }
        else
        {
            sc_memcpy(record, rec_buf, val_len);
        }
    }
    else
        sc_memcpy(record, rec_buf, collection->item_size_);

    /* step 4 : logging for insertion */
    {
        DB_UINT8 slot_type = 0;

        if (is_msync_insert)
        {
            slot_type = msync_flag;
        }

        if (val_len)
        {
            OID_InsertLog(op_type, trans_id, (const void *) Cont,
                    collect_index, new_slot_oid, record, val_len,
                    next_free_oid, slot_type);
        }
        else
        {
            OID_InsertLog(op_type, trans_id, (const void *) Cont,
                    collect_index, new_slot_oid, record,
                    collection->item_size_, next_free_oid, slot_type);
        }

    }

    /* step 2 : allocate the slot */
    page->header_.free_slot_id_in_page_ = next_free_oid;

    page->header_.record_count_in_page_ += 1;

    /* step 5: finally, mark it used slot */
    if (op_type == INSERT_HEAPREC)
        sc_memcpy(record + collection->slot_size_ - 4, &qsid, sizeof(qsid));
    if (is_msync_insert)
    {
        *(record + collection->slot_size_ - 1) = msync_flag;
    }
    else
    {
        *(record + collection->slot_size_ - 1) = USED_SLOT;
    }

    if (page->header_.free_slot_id_in_page_ == NULL_OID)
    {
        collection->free_page_list_ = page->header_.free_page_next_;
        page->header_.free_page_next_ = FULL_PID;
    }

    SetDirtyFlag(new_slot_oid);

    collection->item_count_ += 1;       /* hint info */
    SetDirtyFlag(collection->cont_id_);

    /* container catalog나 index catalog인 경우 index remove skip */
    if (op_type == INSERT_HEAPREC)
    {
        OID index_id;
        struct TTree *ttree;
        int issys_ttree;

        index_id = Cont->base_cont_.index_id_;
        while (index_id != NULL_OID)
        {
            ttree = (struct TTree *) OID_Point(index_id);
            if (ttree == NULL)
            {
                ret_val = DB_E_OIDPOINTER;
                goto done;
            }

            if (isPartialIndex_name(ttree->base_cont_.name_))
            {
                index_id = ttree->base_cont_.index_id_;
                continue;
            }
            SetBufSegment(issys_ttree, index_id);
            ret_val = TTree_InsertRecord(ttree, &rec_buf[0], new_slot_oid, 1);
            UnsetBufSegment(issys_ttree);
            if (ret_val < 0)
            {
                if (ret_val != DB_E_DUPUNIQUEKEY)
                {
                    MDB_SYSLOG(("TTree_Insert FAIL %d\n", ret_val));
                }
                *record_id = new_slot_oid;
                goto done;
            }

            index_id = ttree->base_cont_.index_id_;
        }
    }

    *record_id = new_slot_oid;

    /* insert가 성공한 경우 해당 table cache 정보 중 modified_times를 증가 */
    if (!isTempTable_name(Cont->base_cont_.name_))
    {
        ret_val = _Schema_SetTable_modifiedtimes(Cont->base_cont_.name_);
        if (ret_val < 0)
        {
            ret_val = DB_E_UNKNOWNTABLE;
            goto done;
        }
    }

    ret_val = DB_SUCCESS;

  done:
    if (op_type == INSERT_HEAPREC)
    {
        UnsetBufSegment(issys);
    }

    return ret_val;
}

/*****************************************************************************/
//! Col_Insert_Variable
/*! \breif  store partial value in exteded part
 ************************************
 * \param Cont(in)          :
 * \param value(in)         : extend buffer
 * \param val_len(in)       : extened buffer's length
 * \param record_id(in/out) : RECORD ID(RID)
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘
 *  - called : Insert_Variable()
 *  - heap record format reference : _Schema_CheckFieldDesc()
 *  - extened part의 image가 수정됨
 *      collection 후보지를 선택시 length bytes(4 bytes)를 더해줘야 한다
 *
 *****************************************************************************/
int
Col_Insert_Variable(struct Container *Cont, char *value, DB_INT32 val_len,
        OID * record_id)
{
    struct Collection *collect;
    char *ptr = value;
    int ret;
    DB_INT16 nth_collect;

    val_len += MDB_EXT_LEN_SIZE;        // length bytes in extended-part

    nth_collect = (DB_INT16) Col_Choose_collection(Cont, val_len, &collect);
    if (nth_collect < 0)
    {
        MDB_SYSLOG(("ERROR: Choose Collection for Varchar Data(size=%d).\n",
                        val_len));
        *record_id = NULL_OID;
        return DB_FAIL;
    }

    // 윗 부분에서 val_len += MDB_EXT_LEN_SIZE : 안쪽에서 빼줘야 한다
    ret = Col_Insert(Cont, nth_collect, ptr, 0, 0, record_id, val_len, 0);
    if (ret < 0)
    {
        *record_id = NULL_OID;
        return ret;
    }

    return DB_SUCCESS;
}

//===================================================================
//
// Function Name: Col_Read
// Call By : __Read(),
//
//===================================================================
/*****************************************************************************/
//! Col_Read
/*! \breif  Container에서 Data를 바로 읽어옴
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  Call By : __Read()
 *      
 *****************************************************************************/
int
Col_Read(DB_INT32 item_size, OID record_oid, char *item)
{
    char *src;

    src = (char *) OID_Point(record_oid);

    if (src == NULL)
    {
        return DB_E_INVALIDPARAM;
    }

    if (item != NULL)
    {
        sc_memcpy(item, src, item_size);
    }

    return DB_SUCCESS;
}

extern char *dbi_extractcpy_variable(FIELDVALUE_T * fld_val, char *value,
        SYSFIELD_T * fld_info);

/* this function be called so many times, so i could not avoid a little dirty(?)^^. */
/*****************************************************************************/
//! Col_Read_Values 
/*! \breif  record_oid에 해당하는 record를 field 별로 rec_values에 할당함
 ************************************
 * \param Cont(in)          :
 * \param record_oid(in)    : 
 * \param nfields_info(in)  :
 * \param rec_values(in/out): record의 내용을 각 field 별로 할당되는 공간
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - called : __Read()
 *  - record format reference : _Schema_CheckFieldDesc()
 *  - FIELDVALUE_T의 경우 memory record format이지만, 
 *      BYTE/VARBYTE TYPE에서 4 BYTES 를 더 할당할 필요는 없을 것 같다
 *  - record format 수정
 *  - 기존에는 dest's byte length를 리턴했으나..
 *      함수가 애매 모호해져서, 안쪽에서 변환한다
 *  - alloc_mem_to_value의 인터페이스 수정
 *  - mdb_dbi.h의 macro를 사용하도록 인터페이스 수정함
 *  
 *****************************************************************************/
int
Col_Read_Values(DB_UINT16 record_size, OID record_oid,
        FIELDVALUES_T * rec_values, int eval)
{
    char *h_record;
    char *val_ptr;

    FIELDVALUE_T *fld_val;
    MDB_UINT16 fld_cnt;
    MDB_UINT16 *fld_pos;

    int defined_len;            // memory record's field byte length
    int i;
    int issys = 0;

#if defined(MDB_DEBUG)
    if (record_oid != rec_values->rec_oid)
        sc_assert(0, __FILE__, __LINE__);
#endif

    if (eval)
    {
        fld_cnt = rec_values->eval_fld_cnt;
        fld_pos = rec_values->eval_fld_pos;
    }
    else
    {
        fld_cnt = rec_values->sel_fld_cnt;
        fld_pos = rec_values->sel_fld_pos;
    }

    if (fld_cnt == 0)
        return DB_SUCCESS;

    h_record = (char *) OID_Point(record_oid);
    if (h_record == NULL)
        return DB_E_INVALIDPARAM;

    SetBufSegment(issys, record_oid);

    for (i = 0; i < fld_cnt; ++i)
    {
#ifdef CHECK_VARCHAR_EXTLEN
        if (i > 0)
        {
            fld_val = &(rec_values->fld_value[fld_pos[i - 1]]);
            if (fld_val->type_ == DT_VARCHAR && !fld_val->is_null)
                sc_assert(sc_strlen(fld_val->value_.pointer_) ==
                        fld_val->value_length_, __FILE__, __LINE__);
        }
#endif
        fld_val = &(rec_values->fld_value[fld_pos[i]]);

        if (DB_VALUE_ISNULL(h_record, fld_val->position_, record_size))
        {
            fld_val->is_null = TRUE;
            continue;
        }
        fld_val->is_null = FALSE;

        switch (fld_val->type_)
        {
        case DT_DECIMAL:
        case DT_CHAR:
        case DT_NCHAR:
        case DT_BYTE:
            ALLOC_MEM_TO_VALUE(defined_len, fld_val);
            if (defined_len < 0)
            {
                UnsetBufSegment(issys);
                return DB_FAIL;
            }
            val_ptr = fld_val->value_.pointer_;
            break;

        case DT_VARCHAR:
        case DT_NVARCHAR:
        case DT_VARBYTE:

            ALLOC_MEM_TO_VALUE(defined_len, fld_val);
            if (defined_len < 0)
            {
                UnsetBufSegment(issys);
                return DB_FAIL;
            }

            if (fld_val->fixed_part > FIXED_VARIABLE)
            {   // extended 된 경우
                fld_val->value_length_ =
                        dbi_strcpy_variable(fld_val->value_.pointer_,
                        h_record + fld_val->h_offset_,
                        fld_val->fixed_part, fld_val->type_, defined_len);
                continue;

            }

            val_ptr = fld_val->value_.pointer_;
            break;

        default:
            val_ptr = (char *) &(fld_val->value_);
            defined_len = fld_val->length_;
            break;
        }

        if (fld_val->type_ == DT_BYTE)
        {
            sc_memcpy(&fld_val->value_length_, h_record + fld_val->h_offset_,
                    INT_SIZE);
            sc_memcpy(val_ptr, (h_record + fld_val->h_offset_ + INT_SIZE),
                    fld_val->value_length_);
        }
        else if (fld_val->type_ == DT_VARBYTE)
        {
#if defined(MDB_DEBUG)
            if (fld_val->fixed_part != FIXED_VARIABLE)
                sc_assert(0, __FILE__, __LINE__);
#endif
            sc_memcpy(&fld_val->value_length_, h_record + fld_val->h_offset_,
                    INT_SIZE);
            sc_memcpy(val_ptr, (h_record + fld_val->h_offset_ + INT_SIZE),
                    fld_val->value_length_);
        }
        else
        {
            sc_memcpy(val_ptr, h_record + fld_val->h_offset_ /* src_ptr */ ,
                    defined_len);

            if (IS_BS_OR_NBS(fld_val->type_))
                fld_val->value_length_ = strlen_func[fld_val->type_] (val_ptr);
        }
    }
#ifdef CHECK_VARCHAR_EXTLEN
    if (i > 0)
    {
        fld_val = &(rec_values->fld_value[fld_pos[i - 1]]);
        if (fld_val->type_ == DT_VARCHAR && !fld_val->is_null)
            sc_assert(sc_strlen(fld_val->value_.pointer_) ==
                    fld_val->value_length_, __FILE__, __LINE__);
    }
#endif

    UnsetBufSegment(issys);

    return DB_SUCCESS;
}

/*****************************************************************************/
//! Col_Update 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param Cont():
 * \param record_id(in) : 
 * \param item(in)      : memory record
 * \param open_time(in) :
 * \param qsid(in)      :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  Call By :  __Update()
 *
 *****************************************************************************/
static int
Col_Update(struct Container *Cont, OID record_id,
        char *item, char *record, DB_UINT16 qsid,
        struct Update_Idx_Id_Array *upd_idx_id_array, DB_UINT8 msync_flag)
{
    struct TTree *ttree;
    DB_INT32 h_offset;
    OID updated_ttree_oid[MAX_INDEX_NO];
    int i, num_updated_ttree = 0;
    int ret_val;
    int issys_ttree = 0;
    OID index_id;

    if (!Cont)
    {
        ret_val = DB_E_INVALIDPARAM;
        goto done;
    }

    index_id = Cont->base_cont_.index_id_;

    if (index_id && upd_idx_id_array)
    {
        i = 0;
        if (i < upd_idx_id_array->cnt)
        {
            index_id = upd_idx_id_array->idx_oid[i];
        }
        else
        {
            index_id = NULL_OID;
        }
    }

    /* remove old keys */
    while (index_id)
    {
        ttree = (struct TTree *) OID_Point(index_id);
        if (ttree == NULL)
        {
            ret_val = DB_E_OIDPOINTER;
            goto done;
        }

        if (isPartialIndex_name(ttree->base_cont_.name_))
        {
            continue;
        }

        SetBufSegment(issys_ttree, index_id);

        if (KeyDesc_Compare(&(ttree->key_desc_), item, record,
                        ttree->collect_.record_size_, 0) != 0)
        {
            if (Cont->collect_.item_count_ > TNODE_MAX_ITEM * 3)
            {
                ret_val = TTree_RemoveRecord(ttree, record, record_id);
            }
            else
            {
                ret_val = TTree_RemoveRecord(ttree, NULL, record_id);
            }

            if (ret_val < 0)
            {
                UnsetBufSegment(issys_ttree);
                MDB_SYSLOG((" ERROR TTree Remove Record in Update\n"));
                ret_val = DB_FAIL;
                goto done;
            }

            updated_ttree_oid[num_updated_ttree] = index_id;
            num_updated_ttree += 1;
        }

        UnsetBufSegment(issys_ttree);

        if (upd_idx_id_array)
        {
            ++i;
            if (i < upd_idx_id_array->cnt)
            {
                index_id = upd_idx_id_array->idx_oid[i];
            }
            else
            {
                index_id = NULL_OID;
            }
        }
        else
        {
            index_id = ttree->base_cont_.index_id_;
        }
    }

    /* update the heap record */
    sc_memcpy(record, item, Cont->collect_.item_size_);

    /* set qsid */
    h_offset = Cont->collect_.slot_size_ - 4;
    *(DB_UINT16 *) (record + h_offset) = qsid;
    if (*(record + Cont->collect_.slot_size_ - 1) == SYNCED_SLOT)
    {
        *(record + Cont->collect_.slot_size_ - 1) = UPDATE_SLOT;
    }
    else if (msync_flag & SYNCED_SLOT)
    {
        *(record + Cont->collect_.slot_size_ - 1) = SYNCED_SLOT;
    }

    SetDirtyFlag(record_id);

    /* insert new keys */
    for (i = 0; i < num_updated_ttree; i++)
    {
        /* Uniqueness checking has been performed ahead */
        ttree = (struct TTree *) OID_Point(updated_ttree_oid[i]);

        SetBufSegment(issys_ttree, updated_ttree_oid[i]);

        ret_val = TTree_InsertRecord(ttree, item, record_id, 0);

        UnsetBufSegment(issys_ttree);
        if (ret_val < 0)
        {
            MDB_SYSLOG(("ERROR TTree Insert In Update\n"));
            ret_val = DB_FAIL;
            goto done;
        }
    }

    /* update가 성공한 경우 해당 table cache 정보 중 modified_times를 증가 */
    if (!isTempTable_name(Cont->base_cont_.name_))
    {
        ret_val = _Schema_SetTable_modifiedtimes(Cont->base_cont_.name_);
        if (ret_val < 0)
        {
            ret_val = DB_E_UNKNOWNTABLE;
            goto done;
        }
    }

    ret_val = DB_SUCCESS;

  done:

    return ret_val;
}

int
Col_Update_Record(struct Container *Cont, OID record_id, char *item,
        DB_UINT32 open_time, DB_UINT16 qsid,
        struct Update_Idx_Id_Array *upd_idx_id_array, DB_UINT8 msync_flag)
{
    char *record;
    int trans_id = *(int *) CS_getspecific(my_trans_id);
    int ret_val;
    int issys;

    /* get old record */
    record = (char *) OID_Point(record_id);
    if (record == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    /* set utime (open_time) */
    *(DB_UINT32 *) (item + Cont->collect_.slot_size_ - 8) = open_time;

    SetBufSegment(issys, record_id);

    /* logging for update row operation */
    if (*(record + Cont->collect_.slot_size_ - 1) == SYNCED_SLOT)
    {
        DB_UINT8 slot_type = SYNCED_SLOT;

        OID_UpdateLog(UPDATE_HEAPREC, trans_id, (const void *) Cont,
                record_id, item, Cont->collect_.item_size_,
                (void *) &slot_type, 1);
    }
    else if (msync_flag & SYNCED_SLOT)
    {
        DB_UINT8 slot_type = USED_SLOT;

        OID_UpdateLog(UPDATE_HEAPREC, trans_id, (const void *) Cont,
                record_id, item, Cont->collect_.item_size_,
                (void *) &slot_type, 1);
    }
    else
    {
        OID_UpdateLog(UPDATE_HEAPREC, trans_id, (const void *) Cont,
                record_id, item, Cont->collect_.item_size_, NULL, 0);
    }

    ret_val = Col_Update(Cont, record_id, item, record, qsid, upd_idx_id_array,
            msync_flag);
    if (ret_val < 0)
    {
        ret_val = DB_FAIL;
        goto done;
    }

    ret_val = DB_SUCCESS;

  done:
    UnsetBufSegment(issys);
    return ret_val;
}

int
Col_Update_Field(struct Container *Cont, SYSFIELD_T * fields_info,
        struct UpdateValue *newValues, OID record_id, char *item,
        DB_UINT32 open_time, DB_UINT16 qsid,
        struct Update_Idx_Id_Array *upd_idx_id_array)
{
    char *record;
    DB_INT16 pos;
    DB_INT32 log_space_size = 1024;     /* align log_space */
    union
    {
        char log_space[1024];
        long temp;
    } un;
    char *log_data = &un.log_space[0];
    DB_INT32 log_leng;
    DB_UINT32 i;
    struct UpdateFieldInfo fldinfo;
    int trans_id = *(int *) CS_getspecific(my_trans_id);
    int ret_val;
    int issys;
    int log_field_cnt = 0;
    DB_UINT32 upd_field_count;

    /* get old heap record pointer */
    record = (char *) OID_Point(record_id);
    if (record == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    SetBufSegment(issys, record_id);

    /* set utime (open_time) */
    *(DB_UINT32 *) (item + Cont->collect_.slot_size_ - 8) = open_time;

    /* build log data and logging */
    log_leng = 4;       /* num of updated fields */
    for (i = 0; i <= newValues->update_field_count; i++)
    {
        if (i < newValues->update_field_count)
        {
            pos = newValues->update_field_value[i].pos;
            if (fields_info[pos].flag & FD_NOLOGGING)
            {
                continue;
            }
            ++log_field_cnt;
            fldinfo.offset = fields_info[pos].h_offset;
            if (pos < (Cont->collect_.numfields_ - 1))
            {
                fldinfo.length =
                        fields_info[pos + 1].h_offset - fldinfo.offset;
            }
            else
            {
                fldinfo.length = Cont->collect_.record_size_ - fldinfo.offset;
            }
        }
        else    /* i == newValues->update_field_count */
        {
            fldinfo.offset = Cont->collect_.record_size_;
            fldinfo.length = Cont->collect_.item_size_ - fldinfo.offset;
        }

        if ((log_leng + 4 + (fldinfo.length * 2)) > log_space_size)
        {
            char *temp_ptr = log_data;

            /* remove invalid write */
            do
            {
                log_space_size += 1024;
            } while ((log_leng + 4 + (fldinfo.length * 2)) > log_space_size);

            log_data = mdb_malloc(log_space_size);
            if (log_data == NULL)
            {
                if (temp_ptr != &un.log_space[0])
                {
                    mdb_free(temp_ptr);
                    temp_ptr = NULL;
                }
                ret_val = DB_E_OUTOFMEMORY;
                goto done;
            }

            sc_memcpy(log_data, temp_ptr, log_leng);

            if (temp_ptr != &un.log_space[0])
            {
                mdb_free(temp_ptr);
                temp_ptr = NULL;
            }
        }

        sc_memcpy(log_data + log_leng, &fldinfo, 4);

        log_leng += 4;
        sc_memcpy(log_data + log_leng, record + fldinfo.offset,
                fldinfo.length);
        log_leng += fldinfo.length;
        sc_memcpy(log_data + log_leng, item + fldinfo.offset, fldinfo.length);
        log_leng += fldinfo.length;
    }

    upd_field_count = log_field_cnt + 1;

    sc_memcpy(log_data, (char *) &upd_field_count, sizeof(upd_field_count));

    /* If all update fields are nologging field, then don't write log. */
    if (log_field_cnt == 0)
    {
    }
    else

        OID_UpdateFieldLog(UPDATE_HEAPFLD, trans_id, Cont, record_id,
                log_data, log_leng, NULL, 0);

#ifdef MDB_DEBUG
    sc_assert(*(DB_UINT32 *) log_data == upd_field_count, __FILE__, __LINE__);
#endif

#ifdef MDB_DEBUG
    {
        DB_UINT32 field_count;

        sc_memcpy((char *) &field_count, log_data, sizeof(field_count));

        if (field_count != upd_field_count)
        {
            __SYSLOG("ERROR Col_Update_Field %d %d\n",
                    field_count, upd_field_count);
            ret_val = DB_E_UPDATEFIELDCOUNT;
            goto done;
        }
    }
#endif

    if (log_data != &un.log_space[0])
    {
        mdb_free(log_data);
        log_data = NULL;
    }

    ret_val = Col_Update(Cont, record_id, item, record, qsid, upd_idx_id_array,
            0);
    if (ret_val < 0)
    {
        ret_val = DB_FAIL;
        goto done;
    }

    ret_val = DB_SUCCESS;

  done:
    UnsetBufSegment(issys);
    return ret_val;
}

/*****************************************************************************/
//! Col_Remove
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param Cont()            :
 * \param collect_index()   : 
 * \param record_oid()      :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  Call By : __Remove()
 *
 *****************************************************************************/
int
Col_Remove(struct Container *Cont, int collect_index, OID record_oid,
        int remove_msync_slot)
{
    struct Collection *collection;
    short op_type;
    struct Page *free_page;
    char *record;
    int trans_id = *(int *) CS_getspecific(my_trans_id);
    int ret_val;
    int issys = 0;
    int logsize = 0;
    DB_UINT8 slot_type = 0;

    if (collect_index == -1)
    {       /* heap record delete */
        collection = &(Cont->collect_);
        if (&(Cont->collect_) == &(container_catalog->collect_))
            op_type = RELATION_DROP;
        else if (&(Cont->collect_) == &(index_catalog->collect_))
            op_type = INDEX_DROP;
        else
            op_type = REMOVE_HEAPREC;
    }
    else
    {       /* varchar data delete */
        collection = &(Cont->var_collects_[collect_index]);
        op_type = REMOVE_VARDATA;
    }

    free_page = (struct Page *) PageID_Pointer(record_oid);
    if (free_page == NULL)
    {
        MDB_SYSLOG((" Remove Record : Page Pointer Invalid \n"));
        return DB_E_PIDPOINTER;
    }

    /* get record pointer */
    record = (char *) free_page + OID_GetOffSet(record_oid);

    if (op_type == REMOVE_HEAPREC)
    {
        SetBufSegment(issys, record_oid);
    }

    logsize = collection->item_size_;

    if (op_type == REMOVE_HEAPREC)
    {
        slot_type = *(record + collection->slot_size_ - 1);
        if (slot_type == USED_SLOT)
        {
            slot_type = 0;
        }

        if (!remove_msync_slot)
        {
            if (slot_type == DELETE_SLOT)
            {
                ret_val = DB_SUCCESS;
                goto done;
            }
        }
    }
    else if (op_type == REMOVE_VARDATA)
    {
        DB_EXT_PART_LEN ext_length;

        sc_memcpy((char *) (&ext_length), record, MDB_EXT_LEN_SIZE);

        if (ext_length & MDB_EXTENDED_FLAG_BIT)
            logsize =
                    MDB_EXT_LEN_SIZE + (ext_length & ~MDB_EXTENDED_FLAG_BIT) +
                    OID_SIZE;
        else
            logsize = MDB_EXT_LEN_SIZE + ext_length;

        logsize = _alignedlen(logsize, sizeof(long));

        if (logsize > collection->item_size_)
        {
#if defined(MDB_DEBUG)
            sc_assert(0, __FILE__, __LINE__);
#else
            logsize = collection->item_size_;
#endif
        }
    }
    else if (op_type == RELATION_DROP)
    {
        struct Container *cont = (struct Container *) record;

        if (isTempTable(cont))
        {
            op_type = TEMPRELATION_DROP;
            logsize = OID_SIZE; /* cont_id_ */
        }
    }
    else if (op_type == INDEX_DROP)
    {
        struct TTree *ttree = (struct TTree *) record;

        if (isTempIndex_name(ttree->base_cont_.name_))
        {
            op_type = TEMPINDEX_DROP;
            logsize = OID_SIZE; /* cont_id_ */
        }
    }

    OID_DeleteLog(op_type, trans_id, (const void *) Cont, collect_index,
            record_oid, logsize, NULL, 0, slot_type);

    /* remove keys */
    /* container catalog나 index catalog인 경우 index remove skip */
    if (op_type == REMOVE_HEAPREC)
    {
        OID index_id;
        struct TTree *ttree;
        int issys_ttree;

        index_id = Cont->base_cont_.index_id_;
        while (index_id != NULL_OID)
        {
            ttree = (struct TTree *) OID_Point(index_id);
            if (ttree == NULL)
            {
                ret_val = DB_E_OIDPOINTER;
                if (op_type == REMOVE_HEAPREC)
                {
                    UnsetBufSegment(issys);
                }
                return ret_val;
            }

            if (isPartialIndex_name(ttree->base_cont_.name_))
            {
                index_id = ttree->base_cont_.index_id_;
                continue;
            }

            SetBufSegment(issys_ttree, index_id);

            if (Cont->collect_.item_count_ > TNODE_MAX_ITEM * 3)
                ret_val = TTree_RemoveRecord(ttree, record, record_oid);
            else
                ret_val = TTree_RemoveRecord(ttree, NULL, record_oid);

            UnsetBufSegment(issys_ttree);

            if (ret_val < 0)
            {
                /* continue */
            }
            index_id = ttree->base_cont_.index_id_;
        }
    }

    if (slot_type == SYNCED_SLOT || slot_type == UPDATE_SLOT)
    {
        *(record + collection->slot_size_ - 1) = DELETE_SLOT;

        SetDirtyFlag(record_oid);
    }
    else
    {
        /* set FREE_SLOT */
        *(record + collection->slot_size_ - 1) = FREE_SLOT;

        /* insert the free slot into free slot list */
        *(OID *) record = free_page->header_.free_slot_id_in_page_;
        free_page->header_.free_slot_id_in_page_ = record_oid;
        if (free_page->header_.record_count_in_page_ > 0)
            free_page->header_.record_count_in_page_ -= 1;

        if (free_page->header_.free_page_next_ == FULL_PID)
        {
            free_page->header_.free_page_next_ = collection->free_page_list_;
            collection->free_page_list_ = free_page->header_.self_;
        }

        SetDirtyFlag(record_oid);

        collection->item_count_ -= 1;

        if (collection != &temp_container_catalog->collect_ &&
                collection != &temp_index_catalog->collect_)
        {
            SetDirtyFlag(collection->cont_id_);
        }
    }

    /* remove가 성공한 경우 해당 table cache 정보 중 modified_times를 증가 */
    if (!isTempTable_name(Cont->base_cont_.name_))
    {
        ret_val = _Schema_SetTable_modifiedtimes(Cont->base_cont_.name_);
        if (ret_val < 0)
        {
            ret_val = DB_E_UNKNOWNTABLE;
            goto done;
        }
    }

    ret_val = DB_SUCCESS;

  done:

    ret_val = DB_SUCCESS;

    if (op_type == REMOVE_HEAPREC)
    {
        UnsetBufSegment(issys);
    }
    return ret_val;
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
Col_Check_Exist(struct Container *Cont,
        SYSTABLE_T * table_info, SYSFIELD_T * fields_info, char *newrecord)
{
    OID recordId;
    char *record;
    char *m_record = NULL;
    DB_INT32 m_rec_len = 0;
    DB_BOOL has_variabletype = FALSE;
    DB_INT32 item_size = 0;
    int issys;

    if (table_info)
        m_rec_len = table_info->recordLen;
    else
        m_rec_len = Cont->collect_.record_size_;

    if (table_info && table_info->has_variabletype)
    {
        has_variabletype = TRUE;
        item_size = REC_ITEM_SIZE(m_rec_len, table_info->numFields);

        m_record = PMEM_ALLOC(item_size);
        if (m_record == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFDBMEMORY, 0);
            return DB_E_OUTOFDBMEMORY;
        }
    }

    m_rec_len += ((Cont->collect_.numfields_ - 1) >> 3) + 1;

    recordId = Col_GetFirstDataID(Cont, 0);

    while (1)
    {
        if (recordId == INVALID_OID)
        {
            if (m_record)
            {
                PMEM_FREENUL(m_record);
            }
            return DB_E_OIDPOINTER;
        }

        if (recordId == NULL_OID)
            break;

        record = (char *) OID_Point(recordId);
        if (record == NULL)
        {
            if (m_record)
            {
                PMEM_FREENUL(m_record);
            }
            return DB_E_OIDPOINTER;
        }

        if (has_variabletype)
        {
            SetBufSegment(issys, recordId);

            record = make_memory_record(Cont->collect_.numfields_,
                    Cont->collect_.item_size_,
                    Cont->collect_.record_size_,
                    Cont->base_cont_.memory_record_size_,
                    fields_info, m_record, record, 0, NULL);

            UnsetBufSegment(issys);
        }

        if (sc_memcmp(newrecord, record, m_rec_len) == 0)
        {
            if (m_record)
                PMEM_FREENUL(m_record);
            return TRUE;
        }

        Col_GetNextDataID(recordId, Cont->collect_.slot_size_,
                Cont->collect_.page_link_.tail_, 0);
    }

    if (m_record)
        PMEM_FREENUL(m_record);

    return FALSE;
}

/* return value -1 is not error */

DB_INT32
Col_get_collect_index(OID recordid)
{
    struct Page *page = NULL;

    page = (struct Page *) PageID_Pointer(recordid);
    if (page == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_PIDPOINTER, 0);
        return DB_E_PIDPOINTER;
    }

    if (page->header_.collect_index_ == -1
            || page->header_.collect_index_ < VAR_COLLECT_CNT)
    {
        return page->header_.collect_index_;
    }
    else
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_INVALIDCOLLECTIDX, 0);
        return DB_E_INVALIDCOLLECTIDX;
    }
}

struct Container *Cont_get(OID recordid);
int
Col_check_valid_rid(char *tablename, OID rid)
{
    char *record = NULL;
    struct Container *cont;
    int slot_size;

    if (isInvalidRid(rid))
    {
        return DB_E_INVALID_RID;
    }

    /* check rid set_here */
    cont = Cont_get(rid);
    if (cont == NULL)
        return DB_E_INVALID_RID;

    if (cont->collect_.item_count_ == 0)
    {
        return DB_E_INVALID_RID;
    }

    if (tablename != NULL)
    {
        if (mdb_strcmp(tablename, cont->base_cont_.name_))
            return DB_E_INVALID_RID;
    }


    slot_size = cont->collect_.slot_size_;

    record = OID_Point(rid);
    if (record == NULL)
        return DB_E_INVALID_RID;

    if (*(record + slot_size - 1) & USED_SLOT)
    {
        return DB_SUCCESS;
    }

    return DB_E_INVALID_RID;    /* FREE_SLOT */
}
