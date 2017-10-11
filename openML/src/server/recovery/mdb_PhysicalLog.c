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
#include "mdb_Cont.h"
#include "mdb_Mem_Mgr.h"
#include "mdb_ContCat.h"
#include "mdb_Collect.h"
#include "mdb_LogRec.h"
#include "mdb_inner_define.h"
#include "mdb_PMEM.h"
#include "mdb_syslog.h"

int CLR_WriteCLR(struct LogRec *logrec);
int CLR_WriteCLR_For_InsDel(struct LogRec *logrec, char *log_data,
        int log_size);

int Index_Insert_In_Relation(void *Pointer, char *record, OID record_id);
int Index_Remove_In_Relation(void *Pointer, char *record, OID record_id);
int UpdateTransTable(struct LogRec *logrec);

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
PhysicalLog_Redo(struct LogRec *logrec)
{
    int size = logrec->header_.recovery_leng_ >> 1;
    int issys;

    SetBufSegment(issys, logrec->header_.relation_pointer_);

    UpdateTransTable(logrec);

    OID_LightWrite(logrec->header_.oid_, (char *) (logrec->data1 + size),
            size);

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
PhysicalLog_Undo(struct LogRec *logrec, int flag)
{
    int size = logrec->header_.recovery_leng_ >> 1;
    struct Container *Cont;
    int issys1;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    SetBufSegment(issys1, logrec->header_.relation_pointer_);

    if (logrec->header_.op_type_ == SYS_SLOTUSE_BIT)
    {
        MDB_SYSLOG(("PhysicalLog_Undo for SYS_SLOTUSE_BIT FAIL\n"));
    }

    OID_LightWrite(logrec->header_.oid_, (char *) logrec->data1, size);

    if (flag == CLR)
        CLR_WriteCLR(logrec);

    UnsetBufSegment(issys1);

    return DB_SUCCESS;
}


int
InsertLog_Undo(struct LogRec *logrec, int flag)
{
    struct Container *Cont;
    struct Collection *collection;
    struct Page *page;
    char *record;
    int issys;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont == NULL)
    {
        MDB_SYSLOG(("FAIL: Container is NULL in InsertLog_Undo.\n"));
        return DB_E_OIDPOINTER;
    }

    SetBufSegment(issys, logrec->header_.relation_pointer_);

    Cont->base_cont_.index_rebuild = 1;
    SetDirtyFlag(logrec->header_.relation_pointer_);

    page = (struct Page *) PageID_Pointer(logrec->header_.oid_);
    if (page == NULL)
    {
        UnsetBufSegment(issys);
        MDB_SYSLOG(("FAIL: Page is NULL in InsertLog_Undo.\n"));
        return DB_E_PIDPOINTER;
    }

    if (getSegmentNo(page->header_.self_) != getSegmentNo(logrec->header_.oid_)
            || getPageNo(page->header_.self_) !=
            getPageNo(logrec->header_.oid_))
    {
        MDB_SYSLOG(("InsertLog_Undo: OID value is strange !!\n"));
    }

    /* logical undo in page internal */
    if (logrec->header_.collect_index_ == -1)
        collection = &Cont->collect_;
    else
        collection = &Cont->var_collects_[logrec->header_.collect_index_];

    /* set the record as free slot */
    record = (char *) page + OID_GetOffSet(logrec->header_.oid_);

    /* check if the slot state is used state */
    if ((*(record + collection->slot_size_ - 1) & USED_SLOT) == 0)
    {
        UnsetBufSegment(issys);
        MDB_SYSLOG(("InsertLog_Undo: Free Slot State FAIL\n"));
        return DB_FAIL;
    }

    if (flag == CLR)    /* write CLR for InsertLog_Undo */
    {
        int log_size;
        char log_data[64];
        struct SlotFreeLogData *s_data;
        struct SlotUsedLogData log_data_buf;
        struct SlotUsedLogData *s_log_data;

        log_size = SLOTFREELOGDATA_SIZE;

        s_data = (struct SlotFreeLogData *) &log_data[0];
        sc_memcpy(&log_data_buf, logrec->data1 +
                (logrec->header_.recovery_leng_ - SLOTUSEDLOGDATA_SIZE),
                SLOTUSEDLOGDATA_SIZE);
        s_log_data = &log_data_buf;

        s_data->before_slot_type = s_log_data->after_slot_type;
        s_data->after_slot_type = s_log_data->before_slot_type;
        s_data->collect_new_item_count = collection->item_count_ - 1;
        s_data->page_new_record_count =
                page->header_.record_count_in_page_ - 1;
        s_data->page_cur_free_slot_id = page->header_.free_slot_id_in_page_;
        if (page->header_.free_page_next_ == FULL_PID)
            s_data->page_new_free_page_next = collection->free_page_list_;
        else
            s_data->page_new_free_page_next = NULL_PID;

#ifdef MDB_DEBUG
        sc_assert(s_data->page_new_record_count >= 0, __FILE__, __LINE__);
#endif

        CLR_WriteCLR_For_InsDel(logrec, &log_data[0], log_size);
    }

    /* update page header & collection information */
    /* connect it into the free slot list */
    *(record + collection->slot_size_ - 1) = FREE_SLOT;

    *(OID *) record = page->header_.free_slot_id_in_page_;
    page->header_.free_slot_id_in_page_ = logrec->header_.oid_;

    page->header_.record_count_in_page_ -= 1;

    if (page->header_.free_page_next_ == FULL_PID)
    {
        page->header_.free_page_next_ = collection->free_page_list_;
        collection->free_page_list_ = page->header_.self_;
    }

    collection->item_count_ -= 1;

    SetDirtyFlag(logrec->header_.oid_);
    SetDirtyFlag(collection->cont_id_);

    UnsetBufSegment(issys);

    return DB_SUCCESS;
}

int
InsertLog_Redo(struct LogRec *logrec)
{
    struct Container *Cont;
    struct Collection *collection;
    struct Page *page;
    char *record;
    struct SlotUsedLogData log_data_buf;
    struct SlotUsedLogData *s_log_data;
    int issys;

    UpdateTransTable(logrec);

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont == NULL)
    {
        MDB_SYSLOG(("FAIL: Container is NULL in InsertLog_Redo.\n"));
        return DB_E_OIDPOINTER;
    }

    SetBufSegment(issys, logrec->header_.relation_pointer_);

    page = (struct Page *) PageID_Pointer(logrec->header_.oid_);
    if (page == NULL)
    {
        UnsetBufSegment(issys);
        MDB_SYSLOG(("FAIL: page is NULL in InsertLog_Redo.\n"));
        return DB_E_PIDPOINTER;
    }

    if (getSegmentNo(page->header_.self_) != getSegmentNo(logrec->header_.oid_)
            || getPageNo(page->header_.self_) !=
            getPageNo(logrec->header_.oid_))
    {
        MDB_SYSLOG(("InsertLog_Redo: OID value is strange !!\n"));
    }

    /* logical undo in page internal */
    if (logrec->header_.collect_index_ == -1)
        collection = &Cont->collect_;
    else
        collection = &Cont->var_collects_[logrec->header_.collect_index_];

    /* if the slot in the free slot list, disconnect it */

    /* write record image including slot status flag */
    record = (char *) page + OID_GetOffSet(logrec->header_.oid_);
    sc_memcpy(record, (const char *) logrec->data1,
            (logrec->header_.recovery_leng_ - SLOTUSEDLOGDATA_SIZE));

    sc_memcpy(&log_data_buf, logrec->data1 + logrec->header_.recovery_leng_
            - SLOTUSEDLOGDATA_SIZE, SLOTUSEDLOGDATA_SIZE);
    s_log_data = &log_data_buf;

    if (s_log_data->before_slot_type == DELETE_SLOT
            && (s_log_data->after_slot_type & USED_SLOT))
    {
        *(record + collection->slot_size_ - 1) = s_log_data->after_slot_type;
        SetDirtyFlag(logrec->header_.oid_);
    }
    else
    {
        if (s_log_data->after_slot_type)
        {
            *(record + collection->slot_size_ - 1) =
                    s_log_data->after_slot_type;
        }
        else
            *(record + collection->slot_size_ - 1) = USED_SLOT;

        /* update page header & collection information */
        /* consider align problem of SlotUsedLogData. */

        if (s_log_data->page_new_record_count < 0)
            s_log_data->page_new_record_count = 0;
        page->header_.record_count_in_page_ =
                s_log_data->page_new_record_count;
        page->header_.free_slot_id_in_page_ =
                s_log_data->page_new_free_slot_id;
        if (page->header_.free_slot_id_in_page_ == NULL_OID)
        {
            collection->free_page_list_ = s_log_data->page_cur_free_page_next;
            page->header_.free_page_next_ = FULL_PID;
        }

        collection->item_count_ = s_log_data->collect_new_item_count;

        SetDirtyFlag(logrec->header_.oid_);
        SetDirtyFlag(collection->cont_id_);
    }

    UnsetBufSegment(issys);

    return DB_SUCCESS;
}

int
InsertLog_Index_Insert_Redo(struct LogRec *logrec)
{
    struct Container *Cont;
    char *record;
    extern struct Container *Log_Cont;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont != NULL)
    {
        int issys;

        SetBufSegment(issys, logrec->header_.relation_pointer_);

        Log_Cont = Cont;
        record = (char *) OID_Point(logrec->header_.oid_);
        if ((*(record + Cont->collect_.slot_size_ - 1) & USED_SLOT) == 0)
        {
            extern int fRecovered;

            fRecovered |= ALL_REBUILD;
            return DB_E_REMOVEDRECORD;
        }

        Index_Insert_In_Relation(Cont, (char *) logrec->data1,
                logrec->header_.oid_);

        Log_Cont = NULL;

        UnsetBufSegment(issys);
    }

    return DB_SUCCESS;
}

int
InsertLog_Index_Insert_Undo(struct LogRec *logrec)
{
    struct Container *Cont;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont != NULL)
    {
        int issys;

        SetBufSegment(issys, logrec->header_.relation_pointer_);

        Index_Remove_In_Relation(Cont, (char *) logrec->data1,
                logrec->header_.oid_);
        UnsetBufSegment(issys);
    }

    return DB_SUCCESS;
}

int
InsertLog_Schema_Cache_Undo(struct LogRec *logrec)
{
    struct Container *Cont;
    int issys;
    int ret = DB_SUCCESS;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont != NULL)
    {
        SetBufSegment(issys, logrec->header_.relation_pointer_);
        switch (Cont->base_cont_.id_)
        {
        case 1:        /* systables */
            ret = _Schema_Cache_InsertTable_Undo(logrec->header_.oid_,
                    logrec->data1);
            break;
        case 3:        /* sysindexes */
            ret = _Schema_Cache_InsertIndex_Undo(logrec->header_.oid_,
                    logrec->data1);
            break;
        }
        UnsetBufSegment(issys);
    }
    return ret;
}

int
DeleteLog_Undo(struct LogRec *logrec, int flag)
{
    struct Container *Cont;
    struct Collection *collection;
    struct Page *page;
    char *record;
    OID prev_slot;
    OID curr_slot;
    DB_UINT32 offset;
    int issys = 0, issys1 = 0;
    int ret;
    struct SlotFreeLogData log_data_buf;
    struct SlotFreeLogData *s_log_data;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont == NULL)
    {
        MDB_SYSLOG(("FAIL: Container is NULL in DeleteLog_Undo.\n"));
        return DB_E_OIDPOINTER;
    }

    SetBufSegment(issys1, logrec->header_.relation_pointer_);

    Cont->base_cont_.index_rebuild = 1;
    SetDirtyFlag(logrec->header_.relation_pointer_);

    page = (struct Page *) PageID_Pointer(logrec->header_.oid_);
    if (page == NULL)
    {
        MDB_SYSLOG(("FAIL: page is NULL in DeleteLog_Undo.\n"));
        ret = DB_E_OIDPOINTER;
        goto end;
    }

    SetBufSegment(issys, logrec->header_.oid_);

    if (getSegmentNo(page->header_.self_) != getSegmentNo(logrec->header_.oid_)
            || getPageNo(page->header_.self_) !=
            getPageNo(logrec->header_.oid_))
    {
        MDB_SYSLOG(("DeleteLog_Undo: OID value is strange !!\n"));
        MDB_SYSLOG(("DeleteLog_Undo: page=(%d|%d|%d), oid=(%d|%d|%d)\n",
                        getSegmentNo(page->header_.self_),
                        getPageNo(page->header_.self_),
                        OID_GetOffSet(page->header_.self_),
                        getSegmentNo(logrec->header_.oid_),
                        getPageNo(logrec->header_.oid_),
                        OID_GetOffSet(logrec->header_.oid_)));
    }

    if (logrec->header_.collect_index_ == -1)
        collection = &Cont->collect_;
    else
        collection = &Cont->var_collects_[logrec->header_.collect_index_];

    sc_memcpy(&log_data_buf, logrec->data1 +
            (logrec->header_.recovery_leng_ - SLOTFREELOGDATA_SIZE),
            SLOTFREELOGDATA_SIZE);
    s_log_data = &log_data_buf;

    if (!s_log_data->before_slot_type)
    {
        prev_slot = NULL_OID;
        curr_slot = page->header_.free_slot_id_in_page_;
        while (curr_slot != NULL_OID)
        {
            if (curr_slot == logrec->header_.oid_)
                break;
            if (getSegmentNo(curr_slot) > SEGMENT_NO ||
                    getSegmentNo(curr_slot) !=
                    getSegmentNo(page->header_.self_) ||
                    getPageNo(curr_slot) >= PAGE_PER_SEGMENT ||
                    getPageNo(curr_slot) != getPageNo(page->header_.self_) ||
                    OID_GetOffSet(curr_slot) & 0x3 /* must be 4-byte align */ )
            {
                __SYSLOG("Invalid free slot(%d|%d|%d) page(%d|%d) LSN(%d,%d)\n", getSegmentNo(curr_slot), getPageNo(curr_slot), offset, getSegmentNo(page->header_.self_), getPageNo(page->header_.self_), logrec->header_.record_lsn_.File_No_, logrec->header_.record_lsn_.Offset_);
                ret = DB_FAIL;
                goto end;
            }
            offset = OID_GetOffSet(curr_slot);
            if (offset == 0)
            {
                MDB_SYSLOG(("Inconsistent slot %d(%d|%d|%d) page(%d|%d) "
                                "LSN(%d, %d)FAIL\n",
                                curr_slot, getSegmentNo(curr_slot),
                                getPageNo(curr_slot), offset,
                                getSegmentNo(page->header_.self_),
                                getPageNo(page->header_.self_),
                                logrec->header_.record_lsn_.File_No_,
                                logrec->header_.record_lsn_.Offset_));
                ret = DB_FAIL;
                goto end;
            }
            prev_slot = curr_slot;
            curr_slot = *(OID *) ((char *) page + offset);
        }

        if (curr_slot != logrec->header_.oid_)
        {
            if (server->status == SERVER_STATUS_NORMAL)
            {
                /* Record Delete Process is like followings. 
                   1) logging Delete Log
                   2) remove keys
                   3) remove record.
                   If delete operation fails at the 2nd phase,
                   the record will be in used slot state.
                   In that case, writing CLR only is performed.
                 */
                MDB_SYSLOG(
                        ("DeleteLog_Undo: record is in used slot state.\n"));

                if (flag == CLR)
                {
                    /* write dummy CLR */
                    logrec->header_.recovery_leng_ = 0;
                    logrec->header_.lh_length_ = 0;
                    CLR_WriteCLR_For_InsDel(logrec, NULL, 0);
                }
                ret = DB_SUCCESS;
                goto end;
            }
            else        /* recovery undo */
            {
                MDB_SYSLOG(
                        ("DeleteLog_Undo: Used Slot State FAIL LSN(%d,%d)\n",
                                logrec->header_.record_lsn_.File_No_,
                                logrec->header_.record_lsn_.Offset_));
                ret = DB_FAIL;
                goto end;
            }
        }

        if (prev_slot != NULL_OID)
        {
            MDB_SYSLOG(
                    ("DeleteLog_Undo: record is not the first free slot. FAIL\n"));
            ret = DB_FAIL;
            goto end;
        }
    }

    if (flag == CLR)
    {
        int log_size;
        char log_data[64];
        struct SlotUsedLogData *s_data;

        log_size = OID_SIZE + SLOTUSEDLOGDATA_SIZE;
        sc_memcpy(&log_data[0], logrec->data1, OID_SIZE);

        s_data = (struct SlotUsedLogData *) &log_data[OID_SIZE];

        s_data->before_slot_type = s_log_data->after_slot_type;
        s_data->after_slot_type = s_log_data->before_slot_type;

        s_data->collect_new_item_count = collection->item_count_ + 1;
        s_data->page_new_record_count =
                page->header_.record_count_in_page_ + 1;
        s_data->page_new_free_slot_id =
                *(OID *) ((char *) page + OID_GetOffSet(logrec->header_.oid_));
        s_data->page_cur_free_page_next = page->header_.free_page_next_;

        CLR_WriteCLR_For_InsDel(logrec, &log_data[0], log_size);
    }

    /* disconnect current slot from the free slot list */
    record = (char *) page + OID_GetOffSet(logrec->header_.oid_);

    /* check the record exists. then skip undo */
    if (*(record + collection->slot_size_ - 1) & USED_SLOT)
    {
        ret = DB_SUCCESS;
        goto end;
    }

    if (s_log_data->before_slot_type & USED_SLOT)
    {
        *(record + collection->slot_size_ - 1) = s_log_data->before_slot_type;
        SetDirtyFlag(logrec->header_.oid_);
    }
    else
    {
        page->header_.free_slot_id_in_page_ = *(OID *) record;
        page->header_.record_count_in_page_ += 1;

        /* restore record image including slot status flag */
        sc_memcpy(record, (const char *) logrec->data1,
                logrec->header_.recovery_leng_ - SLOTFREELOGDATA_SIZE);
        if (s_log_data->before_slot_type == DELETE_SLOT)
        {
            *(record + collection->slot_size_ - 1) = DELETE_SLOT;
        }
        else
        {
            *(record + collection->slot_size_ - 1) = USED_SLOT;
        }

        /* update page header & collection information */
        if (page->header_.free_slot_id_in_page_ == NULL_OID)
        {
            if (collection->free_page_list_ == page->header_.self_)
            {
                collection->free_page_list_ = page->header_.free_page_next_;
                page->header_.free_page_next_ = FULL_PID;
            }
            else
            {
                MDB_SYSLOG(("DeleteLog_Undo: "
                                "free_page_list(%d|%d) != page(%d|%d)\n",
                                getSegmentNo(collection->free_page_list_),
                                getPageNo(collection->free_page_list_),
                                getSegmentNo(page->header_.self_),
                                getPageNo(page->header_.self_)));
            }
        }

        collection->item_count_ += 1;
        SetDirtyFlag(logrec->header_.oid_);
        SetDirtyFlag(collection->cont_id_);
    }

    ret = DB_SUCCESS;

  end:
    UnsetBufSegment(issys);
    UnsetBufSegment(issys1);

    return ret;
}

int
DeleteLog_Redo(struct LogRec *logrec)
{
    struct Container *Cont;
    struct Collection *collection;
    struct Page *page;
    char *record;
    struct SlotFreeLogData log_data_buf;
    struct SlotFreeLogData *s_log_data;
    int issys;

    UpdateTransTable(logrec);

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont == NULL)
    {
        MDB_SYSLOG(("FAIL: Container is NULL in DeleteLog_Redo.\n"));
        return DB_E_OIDPOINTER;
    }

    SetBufSegment(issys, logrec->header_.relation_pointer_);


    page = (struct Page *) PageID_Pointer(logrec->header_.oid_);
    if (page == NULL)
    {
        UnsetBufSegment(issys);
        MDB_SYSLOG(("FAIL: page is NULL in DeleteLog_Redo.\n"));
        return DB_E_OIDPOINTER;
    }

    if (getSegmentNo(page->header_.self_) != getSegmentNo(logrec->header_.oid_)
            || getPageNo(page->header_.self_) !=
            getPageNo(logrec->header_.oid_))
    {
        MDB_SYSLOG(("DeleteLog_Redo: OID value is strange !!\n"));
        MDB_SYSLOG(("DeleteLog_Redo: page=(%d|%d|%d), oid=(%d|%d|%d)\n",
                        getSegmentNo(page->header_.self_),
                        getPageNo(page->header_.self_),
                        OID_GetOffSet(page->header_.self_),
                        getSegmentNo(logrec->header_.oid_),
                        getPageNo(logrec->header_.oid_),
                        OID_GetOffSet(logrec->header_.oid_)));
    }

    if (logrec->header_.collect_index_ == -1)
        collection = &Cont->collect_;
    else
        collection = &Cont->var_collects_[logrec->header_.collect_index_];

    record = (char *) page + OID_GetOffSet(logrec->header_.oid_);

    sc_memcpy(&log_data_buf, logrec->data1 +
            (logrec->header_.recovery_leng_ - SLOTFREELOGDATA_SIZE),
            SLOTFREELOGDATA_SIZE);
    s_log_data = &log_data_buf;

    if ((s_log_data->before_slot_type & USED_SLOT)
            && s_log_data->after_slot_type == DELETE_SLOT)
    {
        *(record + collection->slot_size_ - 1) = DELETE_SLOT;
        SetDirtyFlag(logrec->header_.oid_);
    }
    else
    {
        *(record + collection->slot_size_ - 1) = FREE_SLOT;

        /* insert the free slot into free slot list */
        *(OID *) record = s_log_data->page_cur_free_slot_id;
        page->header_.free_slot_id_in_page_ = logrec->header_.oid_;
        page->header_.record_count_in_page_ =
                s_log_data->page_new_record_count;

#ifdef MDB_DEBUG
        sc_assert(s_log_data->page_new_record_count >= 0, __FILE__, __LINE__);
#endif

        if (page->header_.free_page_next_ == FULL_PID)
        {
            page->header_.free_page_next_ =
                    s_log_data->page_new_free_page_next;
            if (collection->free_page_list_ != page->header_.self_)
                collection->free_page_list_ = page->header_.self_;
        }

        collection->item_count_ = s_log_data->collect_new_item_count;

        SetDirtyFlag(logrec->header_.oid_);
        SetDirtyFlag(collection->cont_id_);
    }

    UnsetBufSegment(issys);

    return DB_SUCCESS;
}

int
DeleteLog_Index_Remove_Redo(struct LogRec *logrec)
{
    struct Container *Cont;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont != NULL)
    {
        int issys;

        SetBufSegment(issys, logrec->header_.relation_pointer_);

        /* find and remove a key using only RID */
        Index_Remove_In_Relation(Cont, NULL, logrec->header_.oid_);

        UnsetBufSegment(issys);
    }

    return DB_SUCCESS;
}

int
DeleteLog_Index_Remove_Undo(struct LogRec *logrec)
{
    struct Container *Cont;
    char *pbuf = NULL;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont != NULL)
    {
        int issys;

        SetBufSegment(issys, logrec->header_.relation_pointer_);
        pbuf = (char *) OID_Point(logrec->header_.oid_);

        if (pbuf == NULL)
        {
            UnsetBufSegment(issys);
            return DB_E_OIDPOINTER;
        }

        Index_Insert_In_Relation(Cont, pbuf, logrec->header_.oid_);

        UnsetBufSegment(issys);
    }

    return DB_SUCCESS;
}

int
DeleteLog_Schema_Cache_Undo(struct LogRec *logrec)
{
    struct Container *Cont;
    int issys;
    int ret = DB_SUCCESS;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont != NULL)
    {
        SetBufSegment(issys, logrec->header_.relation_pointer_);

        switch (Cont->base_cont_.id_)
        {
        case 1:        /* systables */
            ret = _Schema_Cache_DeleteTable_Undo(logrec->header_.oid_);
            break;
        case 3:        /* sysindexes */
            ret = _Schema_Cache_DeleteIndex_Undo(logrec->header_.oid_);
            break;
        }

        UnsetBufSegment(issys);
    }

    return ret;
}

int
UpdateLog_Undo(struct LogRec *logrec, int flag)
{
    int size = logrec->header_.recovery_leng_ >> 1;
    struct Container *Cont;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont == NULL)
    {
        __SYSLOG("FAIL: Container is NULL in UpdateLog_Undo.\n");
        return DB_E_OIDPOINTER;
    }

    Cont->base_cont_.index_rebuild = 1;
    SetDirtyFlag(logrec->header_.relation_pointer_);

    if (isSequenceTable(logrec->header_.tableid_))
        return DB_SUCCESS;

    if (logrec->header_.lh_length_ ==
            logrec->header_.recovery_leng_ + LOG_PADDING_SIZE + 1
            && ((DB_UINT8) (*(logrec->data1 + (size * 2))) == SYNCED_SLOT))
    {
        OID_LightWrite_msync(logrec->header_.oid_, (char *) logrec->data1,
                size, Cont, SYNCED_SLOT);
    }
    else if (logrec->header_.lh_length_ ==
            logrec->header_.recovery_leng_ + LOG_PADDING_SIZE + 1
            && ((DB_UINT8) (*(logrec->data1 + (size * 2))) == USED_SLOT))
    {
        OID_LightWrite_msync(logrec->header_.oid_,
                (char *) (logrec->data1 + size), size, Cont, USED_SLOT);
    }
    else
    {
        OID_LightWrite(logrec->header_.oid_, (char *) logrec->data1, size);
    }

    if (flag == CLR)
        CLR_WriteCLR(logrec);

    return DB_SUCCESS;
}

int
UpdateLog_Redo(struct LogRec *logrec)
{
    int size = logrec->header_.recovery_leng_ >> 1;
    int issys;
    struct Container *Cont;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont == NULL)
    {
        __SYSLOG("FAIL: Container is NULL in UpdateLog_Undo.\n");
        return DB_E_OIDPOINTER;
    }

    SetBufSegment(issys, logrec->header_.relation_pointer_);

    UpdateTransTable(logrec);

    if (logrec->header_.lh_length_ ==
            logrec->header_.recovery_leng_ + LOG_PADDING_SIZE + 1
            && ((DB_UINT8) (*(logrec->data1 + (size * 2))) == SYNCED_SLOT))
    {
        OID_LightWrite_msync(logrec->header_.oid_,
                (char *) (logrec->data1 + size), size, Cont, UPDATE_SLOT);
    }
    else if (logrec->header_.lh_length_ ==
            logrec->header_.recovery_leng_ + LOG_PADDING_SIZE + 1
            && ((DB_UINT8) (*(logrec->data1 + (size * 2))) == USED_SLOT))
    {
        OID_LightWrite_msync(logrec->header_.oid_,
                (char *) (logrec->data1 + size), size, Cont, SYNCED_SLOT);
    }
    else
    {
        OID_LightWrite(logrec->header_.oid_, (char *) (logrec->data1 + size),
                size);
    }

    UnsetBufSegment(issys);

    return DB_SUCCESS;
}

int
UpdateLog_Index_Remove_Old_Redo(struct LogRec *logrec)
{
    struct Container *Cont;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont != NULL)
    {
        int issys;

        SetBufSegment(issys, logrec->header_.relation_pointer_);
        /* find and remove an old key using only RID */
        if (Index_Remove_In_Relation(Cont, NULL, logrec->header_.oid_) < 0)
        {
            MDB_SYSLOG(("Error: index remove\n"));
        }
        UnsetBufSegment(issys);
    }

    return DB_SUCCESS;
}

int
UpdateLog_Index_Insert_New_Redo(struct LogRec *logrec)
{
    struct Container *Cont;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont != NULL)
    {
        int issys;

        SetBufSegment(issys, logrec->header_.relation_pointer_);
        Index_Insert_In_Relation(Cont,
                logrec->data1 + (logrec->header_.recovery_leng_ >> 1),
                logrec->header_.oid_);
        UnsetBufSegment(issys);
    }

    return DB_SUCCESS;
}

int
UpdateLog_Index_Insert_New_Undo(struct LogRec *logrec)
{
    struct Container *Cont;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont != NULL)
    {
        int issys;

        SetBufSegment(issys, logrec->header_.relation_pointer_);
        if (Index_Remove_In_Relation(Cont,
                        logrec->data1 + logrec->header_.recovery_leng_ / 2,
                        logrec->header_.oid_) < 0)
        {
            MDB_SYSLOG(("Error: index remove\n"));
        }
        UnsetBufSegment(issys);
    }

    return DB_SUCCESS;
}

int
UpdateLog_Index_Remove_Old_Undo(struct LogRec *logrec)
{
    struct Container *Cont;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont != NULL)
    {
        int issys;

        SetBufSegment(issys, logrec->header_.relation_pointer_);
        Index_Insert_In_Relation(Cont, logrec->data1, logrec->header_.oid_);
        UnsetBufSegment(issys);
    }

    return DB_SUCCESS;
}

int
UpdateLog_Schema_Cache_Undo(struct LogRec *logrec)
{
    struct Container *Cont;
    int issys;
    int ret = DB_SUCCESS;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont != NULL)
    {
        SetBufSegment(issys, logrec->header_.relation_pointer_);
        switch (Cont->base_cont_.id_)
        {
        case 1:        /* systables */
            ret = _Schema_Cache_UpdateTable_Undo(logrec->header_.oid_,
                    logrec->data1, logrec->header_.recovery_leng_);
            break;
        case 3:        /* sysindexes */
            ret = _Schema_Cache_UpdateIndex_Undo(logrec->header_.oid_,
                    logrec->data1, logrec->header_.recovery_leng_);
            break;
        }

        UnsetBufSegment(issys);
    }
    return ret;
}

int
UpdateFieldLog_Undo(struct LogRec *logrec, int flag)
{
    char *record;
    DB_UINT32 field_count, i;
    int log_offset;
    struct UpdateFieldInfo fldinfo;
    struct Container *Cont;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont == NULL)
    {
        __SYSLOG("FAIL: Container is NULL in UpdateFieldLog_Undo.\n");
        return DB_E_OIDPOINTER;
    }

    Cont->base_cont_.index_rebuild = 1;
    SetDirtyFlag(logrec->header_.relation_pointer_);

    if (isSequenceTable(logrec->header_.tableid_))
    {
        return DB_SUCCESS;
    }

    record = (char *) OID_Point(logrec->header_.oid_);
    if (record == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    sc_memcpy((char *) &field_count, logrec->data1, 4);
    log_offset = 4;

    for (i = 0; i < field_count; i++)
    {
        sc_memcpy((char *) &fldinfo, logrec->data1 + log_offset, 4);
        log_offset += 4;

        sc_memcpy(record + fldinfo.offset,
                logrec->data1 + log_offset, fldinfo.length);
        log_offset += (fldinfo.length * 2);
    }

    if (logrec->header_.lh_length_ ==
            logrec->header_.recovery_leng_ + LOG_PADDING_SIZE + 1)
    {
        if ((DB_UINT8) (*(logrec->data1 + logrec->header_.recovery_leng_))
                == SYNCED_SLOT)
        {
            *(record + Cont->collect_.slot_size_ - 1) = UPDATE_SLOT;
        }
    }

    SetDirtyFlag(logrec->header_.oid_);

    if (flag == CLR)
        CLR_WriteCLR(logrec);

    return DB_SUCCESS;
}

int
UpdateFieldLog_Redo(struct LogRec *logrec)
{
    char *record;
    DB_UINT32 field_count, i;
    int log_offset;
    struct UpdateFieldInfo fldinfo;
    int issys;
    struct Container *Cont;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont == NULL)
    {
        __SYSLOG("FAIL: Container is NULL in UpdateLog_Undo.\n");
        return DB_E_OIDPOINTER;
    }

    UpdateTransTable(logrec);

    SetBufSegment(issys, logrec->header_.relation_pointer_);

    record = (char *) OID_Point(logrec->header_.oid_);
    if (record == NULL)
    {
        UnsetBufSegment(issys);
        return DB_E_OIDPOINTER;
    }

    sc_memcpy((char *) &field_count, logrec->data1, 4);
    log_offset = 4;

    for (i = 0; i < field_count; i++)
    {
        sc_memcpy((char *) &fldinfo, logrec->data1 + log_offset, 4);
        log_offset += 4;

        sc_memcpy(record + fldinfo.offset,
                logrec->data1 + log_offset + fldinfo.length, fldinfo.length);
        log_offset += (fldinfo.length * 2);
    }

    if (logrec->header_.lh_length_ ==
            logrec->header_.recovery_leng_ + LOG_PADDING_SIZE + 1)
    {
        if ((DB_UINT8) (*(logrec->data1 + logrec->header_.recovery_leng_))
                == SYNCED_SLOT)
        {
            *(record + Cont->collect_.slot_size_ - 1) = SYNCED_SLOT;
        }
    }

    SetDirtyFlag(logrec->header_.oid_);

    UnsetBufSegment(issys);

    return DB_SUCCESS;
}

int
UpdateFieldLog_Index_Remove_Old_Redo(struct LogRec *logrec)
{
    struct Container *Cont;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont != NULL)
    {
        int issys;

        SetBufSegment(issys, logrec->header_.relation_pointer_);
        /* find and remove an old key using only RID */
        if (Index_Remove_In_Relation(Cont, NULL, logrec->header_.oid_) < 0)
        {
            MDB_SYSLOG(("Error: index remove\n"));
        }
        UnsetBufSegment(issys);
    }

    return DB_SUCCESS;
}

int
UpdateFieldLog_Index_Insert_New_Redo(struct LogRec *logrec)
{
    struct Container *Cont;

    /* Need Re-implementation !!! */

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont != NULL)
    {
        char *old_record;
        int issys, issys1;

        SetBufSegment(issys, logrec->header_.relation_pointer_);

        old_record = (char *) OID_Point(logrec->header_.oid_);
        if (old_record == NULL)
        {
            UnsetBufSegment(issys);
            return DB_E_OIDPOINTER;
        }

        SetBufSegment(issys1, logrec->header_.oid_);

        if (old_record != NULL)
        {
            Index_Insert_In_Relation(Cont, old_record, logrec->header_.oid_);
        }

        UnsetBufSegment(issys1);
        UnsetBufSegment(issys);
    }

    return DB_SUCCESS;
}

int
UpdateFieldLog_Index_Insert_New_Undo(struct LogRec *logrec)
{
    struct Container *Cont;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont != NULL)
    {
        char *new_record;
        int issys, issys1;

        SetBufSegment(issys, logrec->header_.relation_pointer_);

        new_record = (char *) OID_Point(logrec->header_.oid_);
        if (new_record == NULL)
        {
            UnsetBufSegment(issys);
            return DB_E_OIDPOINTER;
        }

        SetBufSegment(issys1, logrec->header_.oid_);

        if (new_record != NULL)
        {
            if (Index_Remove_In_Relation(Cont, new_record,
                            logrec->header_.oid_) < 0)
            {
                MDB_SYSLOG(("Error: index remove\n"));
            }
        }

        UnsetBufSegment(issys1);
        UnsetBufSegment(issys);
    }

    return DB_SUCCESS;
}

int
UpdateFieldLog_Index_Remove_Old_Undo(struct LogRec *logrec)
{
    struct Container *Cont;

    Cont = (struct Container *) OID_Point(logrec->header_.relation_pointer_);
    if (Cont != NULL)
    {
        char *old_record;
        int issys, issys1;

        SetBufSegment(issys, logrec->header_.relation_pointer_);

        old_record = (char *) OID_Point(logrec->header_.oid_);
        if (old_record == NULL)
        {
            UnsetBufSegment(issys);
            return DB_E_OIDPOINTER;
        }

        SetBufSegment(issys1, logrec->header_.oid_);

        if (old_record != NULL)
        {
            Index_Insert_In_Relation(Cont, old_record, logrec->header_.oid_);
        }

        UnsetBufSegment(issys1);
        UnsetBufSegment(issys);
    }

    return DB_SUCCESS;
}
