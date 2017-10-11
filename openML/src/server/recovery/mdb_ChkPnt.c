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
#include "mdb_LogMgr.h"
#include "mdb_PMEM.h"
#include "mdb_Server.h"

int log_flush_by_daemon;

extern int LogMgr_Check_And_Flush(struct LogRec *logrec);
extern int LogAnchor_WriteToDisk(struct LogAnchor *anchor);
extern void LogFile_Remove(int from_fileno, int to_fileno);
extern void TransMgr_Get_MinFirstLSN(struct LSN *minLSN);
extern int Trans_Mgr_AllTransGet(int *Count, struct Trans4Log **trans);

extern int fRecovered;

/***** static functions *******/
/*static*/ void Delete_Useless_Logfiles(void);

void LogFile_SyncFile(void);

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
/*static*/ void
Delete_Useless_Logfiles(void)
{
    struct LSN Trans_MinFirstLSN;
    int MinFileNo;
    int BeginChkpt_FileNo;

    MinFileNo = Log_Mgr->File_.File_No_;

    /* compare MinFileNo with Trans_MinFirstLSN.File_No_ */
    TransMgr_Get_MinFirstLSN(&Trans_MinFirstLSN);
    if (Trans_MinFirstLSN.File_No_ != -1 &&
            Trans_MinFirstLSN.File_No_ < MinFileNo)
    {
        MinFileNo = Trans_MinFirstLSN.File_No_;
    }

    /* compare MinFileNo with begin_chkpt_lsn.File_No_ */
    BeginChkpt_FileNo = Log_Mgr->Anchor_.Begin_Chkpt_lsn_.File_No_;
    if (BeginChkpt_FileNo != -1 && BeginChkpt_FileNo < MinFileNo)
    {
        MinFileNo = BeginChkpt_FileNo;
    }

    if ((Log_Mgr->Anchor_.Last_Deleted_Log_File_No_ + 1) < MinFileNo)
    {
        LogFile_Remove(Log_Mgr->Anchor_.Last_Deleted_Log_File_No_ + 1,
                MinFileNo - 1);
    }

    return;
}

static int
_Table_Checkpointing(int dbFile_idx, int slow, struct LSN *bckpt_lsn)
{
    DB_UINT32 sn = 0;
    DB_UINT32 pn = 0;
    struct Page *page;
    DB_UINT32 i, j;
    unsigned char dflag;
    int ret;

#ifndef MEM_MUTEX_REDUCED
#endif
    int write_cnt = 0;
    int cnt;

#ifdef _ONE_BDB_
    Log_Mgr->Anchor_.DB_File_Idx_ = 0;
#endif

    ret = DBFile_Open(dbFile_idx, -1);
    if (ret < 0)
    {
        MDB_SYSLOG((" CheckPoint File Open Fail %d\n", __LINE__));
        return ret;
    }

    sn = pn = 0;
    i = 0;
    while (1)
    {
        dflag = *(unsigned char *) (&Mem_mgr->dirty_flag_[dbFile_idx][i]);
        if (dflag)      // 0이 아니면 dirty page 존재
        {
            for (j = 0; j < 8; j++)
            {
                cnt = 1;
                if (dflag & (0x80U >> j))
                {
                    sn = (8 * i + j) / PAGE_PER_SEGMENT;
                    if (sn >= (DB_UINT32) (mem_anchor_->allocated_segment_no_))
                        break;
                    pn = (8 * i + j) % PAGE_PER_SEGMENT;
                    if (Mem_mgr->segment_memid[sn] != 0)
                    {
                        page = (struct Page *) PtrToPage(sn, pn);
                        if (IS_CRASHED_PAGE(sn, pn, page))
                        {
                            __SYSLOG("FAIL checkpoint. crashed page (%d)"
                                    "db:%d sn:%d, pn:%d, self:%x, link next:%x\n"
                                    "    free slotid:%x, rec_cnt:%d, next:%x"
                                    "type:%d \n",
                                    __LINE__, dbFile_idx, sn, pn,
                                    page->header_.self_,
                                    page->header_.link_.next_,
                                    page->header_.free_slot_id_in_page_,
                                    page->header_.record_count_in_page_,
                                    page->header_.free_page_next_,
                                    page->header_.cont_type_);
                            ret = DB_E_INVALIDPAGE;
                            sc_assert(FALSE, __FILE__, __LINE__);
                            goto end;
                        }

                        if (sn == 0 && pn == 0)
                            continue;   /* skip until the last */

                        {
                            /* segment의 두 페이지가 모두 dirty인 경우 한번에
                             * write */
                            if (pn == 0 && (dflag & (0x80U >> (j + 1))))
                            {
                                cnt = 2;
                            }
                            ret = DBFile_Write(dbFile_idx, sn, pn, page, cnt);
                            write_cnt += cnt;
                        }
                        if (ret < 0)
                        {
                            MDB_SYSLOG(("chkpnt Write page FAIL 2 "));
                            MDB_SYSLOG(("db:%d sn:%d, pn:%d\n",
                                            dbFile_idx, sn, pn));
                            goto end;
                        }
                        dflag ^= (0x80U >> j);
                        if (cnt == 2)
                        {
                            ++j;
                            dflag ^= (0x80U >> j);
                        }
                    }
                }
            }

            *(unsigned char *) (&Mem_mgr->dirty_flag_[dbFile_idx][i]) &= dflag;
        }

        i++;
        sn = (8 * i) / PAGE_PER_SEGMENT;
        if (sn >= (DB_UINT32) (mem_anchor_->allocated_segment_no_))
            break;
    }

    /* always do flush page_0 */
    if (Mem_mgr->dirty_flag_[0][0] & 0x80U)
    {
        if (Mem_mgr->segment_memid[0] != 0)
        {
#if defined(_AS_WINCE_) || defined(WIN32_MOBILE)
            DWORD dwOldProtect;
#endif
            /* We do sync file here, in order to keep the order of writing.
             * Page 0 should be written after flushing all dirty pages. */
            ret = DBFile_Sync(dbFile_idx, -1);
            if (ret < 0)
            {
                MDB_SYSLOG(("FAIL Table Checkpointing: "
                                "File Sync before page 0 %d\n", ret));
                goto end;
            }

#if defined(_AS_WINCE_) || defined(WIN32_MOBILE)
            /* memory protect 되어 있는 경우 풀어줌 */
            if (server->shparams.db_write_protection)
            {
                segment_[0] = DBDataMem_V2P(Mem_mgr->segment_memid[0]);

                VirtualProtect(segment_[0], SEGMENT_SIZE, PAGE_READWRITE,
                        &dwOldProtect);
            }
#endif
            mem_anchor_->anchor_lsn.File_No_ = bckpt_lsn->File_No_;
            mem_anchor_->anchor_lsn.Offset_ = bckpt_lsn->Offset_;
#if defined(_AS_WINCE_) || defined(WIN32_MOBILE)
            if (server->shparams.db_write_protection &&
                    dwOldProtect == PAGE_READONLY)
            {
                segment_[0] = DBDataMem_V2P(Mem_mgr->segment_memid[0]);

                VirtualProtect(segment_[0], SEGMENT_SIZE, PAGE_READONLY,
                        &dwOldProtect);
            }
#endif

            page = (struct Page *) PtrToPage(0, 0);
            /* place anchor_lsn at the end of page 0 */
            {
                struct LSN *anchor_lsn;

                anchor_lsn = (struct LSN *)
                        (segment_[0]->page_[0].body_ - PAGE_HEADER_SIZE +
                        S_PAGE_SIZE - sizeof(struct LSN));
                *anchor_lsn = mem_anchor_->anchor_lsn;
            }

            ret = DBFile_Write(dbFile_idx, 0, 0, page, 1);
            if (ret < 0)
            {
                MDB_SYSLOG(("chkpnt Write page FAIL %d ", __LINE__));
                MDB_SYSLOG(("db:%d sn:%d, pn:%d\n", dbFile_idx, 0, 0));
                goto end;
            }
            Mem_mgr->dirty_flag_[0][0] &= 0x7FU;
        }
    }

    SetLastBackupDBALL(dbFile_idx);

    ret = DB_SUCCESS;

  end:
    if (ret != DB_SUCCESS)
    {
        DBFile_Sync(dbFile_idx, -1);
        DBFile_Close(dbFile_idx, -1);
        return ret;
    }

    ret = DBFile_Sync(dbFile_idx, -1);
    if (ret < 0)
    {
        MDB_SYSLOG(("Table Checkpointing: File Sync FAIL %d\n", ret));
        DBFile_Close(dbFile_idx, -1);
        return ret;
    }

    ret = DBFile_Close(dbFile_idx, -1);
    if (ret < 0)
    {
        MDB_SYSLOG(("Table Checkpointing: File Close FAIL %d\n", ret));
    }

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
#ifdef CHECK_INDEX_VALIDATION
void
Index_Check_Page()
{
    DB_UINT32 isn = 0;
    DB_UINT32 pn = 0;
    struct Page *page;

    isn = SEGMENT_NO - mem_anchor_->iallocated_segment_no_;
    pn = 0;
    for (; isn < SEGMENT_NO; isn++)
    {
        for (pn = 0; pn <= 1; pn++)
        {
            page = (struct Page *) Index_PtrToPage(isn, pn);
            if (page == NULL)
                continue;
            if (IS_CRASHED_PAGE(isn, pn, page))
            {
                __SYSLOG("FAIL index checkpoint. crashed page (%d)"
                        "sn:%d, pn:%d, self:%x, link next:%x\n"
                        "    free slotid:%x, rec_cnt:%d, next:%x"
                        "type:%d\n",
                        __LINE__, isn, pn,
                        page->header_.self_,
                        page->header_.link_.next_,
                        page->header_.free_slot_id_in_page_,
                        page->header_.record_count_in_page_,
                        page->header_.free_page_next_,
                        page->header_.cont_type_);
                sc_assert(FALSE, __FILE__, __LINE__);
            }
        }
    }
}
#endif

static int
_Index_Checkpointing(int slow)
{
    DB_UINT32 isn = 0;
    DB_UINT32 pn = 0;
    struct Page *page;
    int i, j;
    unsigned char dflag;
    int ret;
    int write_cnt = 0;
    int cnt;

    ret = DBFile_Open(DBFILE_INDEX_TYPE, -1);
    if (ret < 0)
    {
        MDB_SYSLOG((" index File Open Fail %d\n", __LINE__));
        return ret;
    }

    isn = SEGMENT_NO - mem_anchor_->iallocated_segment_no_;
    pn = 0;
    i = (isn * PAGE_PER_SEGMENT) >> 3;
    while (1)
    {
        dflag = *(unsigned char *) (&Mem_mgr->dirty_flag_[0][i]);
        if (dflag)      // 0이 아니면 dirty page 존재
        {
            for (j = 0; j < 8; j++)
            {
                cnt = 1;
                isn = (8 * i + j) / PAGE_PER_SEGMENT;
                if (isn >= SEGMENT_NO)
                    break;
                pn = (8 * i + j) % PAGE_PER_SEGMENT;
                if (dflag & (0x80U >> j))
                {
                    if (Mem_mgr->segment_memid[isn] != 0)
                    {
                        page = (struct Page *) Index_PtrToPage(isn, pn);
                        if (IS_CRASHED_PAGE(isn, pn, page))
                        {
                            __SYSLOG("FAIL index checkpoint. crashed page (%d)"
                                    "sn:%d, pn:%d, self:%x, link next:%x\n"
                                    "    free slotid:%x, rec_cnt:%d, next:%x"
                                    "type:%d\n",
                                    __LINE__, isn, pn,
                                    page->header_.self_,
                                    page->header_.link_.next_,
                                    page->header_.free_slot_id_in_page_,
                                    page->header_.record_count_in_page_,
                                    page->header_.free_page_next_,
                                    page->header_.cont_type_);
                            ret = DB_E_INVALIDPAGE;
                            sc_assert(FALSE, __FILE__, __LINE__);
                            goto end;
                        }
                        {
                            /* segment의 두 페이지가 모두 dirty인 경우 한번에
                             * write */
                            if (pn == 0 && (dflag & (0x80U >> (j + 1))))
                            {
                                cnt = 2;
                            }

                            ret = DBFile_Write(DBFILE_INDEX_TYPE, isn, pn,
                                    page, cnt);
                            write_cnt += cnt;
                        }

                        if (ret < 0)
                        {
                            MDB_SYSLOG(("checkpoint Write page FAIL 3, "
                                            "isn:%d, pn:%d\n", isn, pn));
                            goto end;
                        }
                        dflag ^= (0x80U >> j);
                        if (cnt == 2)
                        {
                            j++;
                            dflag ^= (0x80U >> j);
                        }
                    }
                }
            }

            *(unsigned char *) (&Mem_mgr->dirty_flag_[0][i]) &= dflag;
        }

        i++;
        isn = (8 * i) / PAGE_PER_SEGMENT;
        if (isn >= SEGMENT_NO)
            break;

    }

    ret = DB_SUCCESS;

  end:

    if (ret != DB_SUCCESS)
    {
        DBFile_Sync(DBFILE_INDEX_TYPE, -1);
        DBFile_Close(DBFILE_INDEX_TYPE, -1);
        return ret;
    }

    ret = DBFile_Sync(DBFILE_INDEX_TYPE, -1);
    if (ret < 0)
    {
        MDB_SYSLOG(("Index Checkpointing: File Sync FAIL %d\n", ret));
        DBFile_Close(DBFILE_INDEX_TYPE, -1);
        return ret;
    }

    ret = DBFile_Close(DBFILE_INDEX_TYPE, -1);
    if (ret < 0)
    {
        MDB_SYSLOG(("Index Checkpointing: File Close FAIL %d\n", ret));
    }

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
static int
__checkpoint(struct BeginChkptLog *_beginchkptlog, int slow)
{
    struct BeginChkptLog beginchkptlog;
    struct EndChkptLog endchkptlog;
    int dbFile_idx;
    int ret;

    if (Log_Mgr == NULL)
        return DB_E_LOGMGRINIT;

    if (server->shparams.prm_no_recovery_mode == 0)
    {
        /* write Begin_Checkpoint log record */
        LogRec_Init(&(beginchkptlog.logrec));
        beginchkptlog.logrec.header_.type_ = BEGIN_CHKPT_LOG;
        beginchkptlog.logrec.header_.trans_id_ = trans_mgr->NextTransID_;
        LogMgr_WriteLogRecord((struct LogRec *) &beginchkptlog);
        if (_beginchkptlog != NULL)
        {
            sc_memcpy(_beginchkptlog, &beginchkptlog,
                    sizeof(struct BeginChkptLog));
        }

    }

    LogMgr_buffer_flush(LOGFILE_SYNC_FORCE);

#ifdef _ONE_BDB_
    dbFile_idx = 0;
#else
    dbFile_idx = (Log_Mgr->Anchor_.DB_File_Idx_ == 0) ? 1 : 0;
#endif

    /* checkpointing table data */
    ret = _Table_Checkpointing(dbFile_idx, slow,
            &(beginchkptlog.logrec.header_.record_lsn_));
    if (ret < 0)
    {
        MDB_SYSLOG(("Checkpointing Table Data Fail..\n"));
        return ret;
    }

    /* checkpointing index data */
    ret = _Index_Checkpointing(slow);
    if (ret < 0)
    {
        MDB_SYSLOG(("Checkpointing Index Data Fail..\n"));
        return ret;
    }

    if (server->shparams.prm_no_recovery_mode == 0)
    {
        struct Trans4Log *transaction;
        int Num_Of_Trans = 0;

        /* write End_Checkpoint log record */
        LogRec_Init(&(endchkptlog.logrec));
        endchkptlog.logrec.header_.type_ = END_CHKPT_LOG;
        endchkptlog.logrec.header_.trans_id_ = trans_mgr->NextTransID_;

        /* 현재 active한 transaction의 정보 기록 */
        Trans_Mgr_AllTransGet(&Num_Of_Trans, &transaction);

        endchkptlog.logrec.data1 = (char *) &Num_Of_Trans;
        endchkptlog.logrec.data2 = NULL;
        endchkptlog.logrec.header_.lh_length_ = sizeof(int) +
                sizeof(struct Trans4Log) * Num_Of_Trans;
        if (Num_Of_Trans != 0)
        {
            endchkptlog.logrec.data2 = (char *) transaction;
        }
        LogMgr_WriteLogRecord((struct LogRec *) &endchkptlog);

        if (transaction != NULL)
        {
            SMEM_FREENUL(transaction);
        }

        LogMgr_Check_And_Flush(&endchkptlog.logrec);
        LogFile_SyncFile();

        if (server->status != SERVER_STATUS_INDEXING)
        {
            Log_Mgr->Anchor_.DB_File_Idx_ = dbFile_idx;
            LSN_Set(Log_Mgr->Anchor_.Begin_Chkpt_lsn_,
                    beginchkptlog.logrec.header_.record_lsn_);
            LSN_Set(Log_Mgr->Anchor_.End_Chkpt_lsn_,
                    endchkptlog.logrec.header_.record_lsn_);
            // _Checkpointing()에서 __checkpoint() call 이후 log file을 지우는
            // 과정에서 loganchor를 write하게 되는데 그 때 write 여부에 따라서
            // 처리하도록 지연 시킴
            /* the last checkpoint LSN must be written before
             * Logfile removes. */
            LogAnchor_WriteToDisk(&Log_Mgr->Anchor_);
        }
    }



    return DB_SUCCESS;
}

/* 특정 segment를 file로 flush */
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
void
CheckPoint_Segment(DB_UINT32 p_sn)
{
    int db_idx;
    int dirty_idx;
    struct Page *page;
    unsigned char dflag;
    DB_UINT32 i;
    int j;
    DB_UINT32 pn = 0;
    DB_UINT32 sn = p_sn;
    int log_flush = 0;
    int ret;
    int cnt;

    int is_opened = 0;

    if (p_sn >= TEMPSEG_BASE_NO)
    {
        dirty_idx = 0;
#ifdef INDEX_BUFFERING
        if (p_sn >= (int) (SEGMENT_NO - mem_anchor_->iallocated_segment_no_))
            db_idx = DBFILE_INDEX_TYPE;
        else
#endif
            db_idx = DBFILE_TEMP_TYPE;
    }
    else
    {
#ifdef _ONE_BDB_
        dirty_idx = 0;
#else
        dirty_idx = (Log_Mgr->Anchor_.DB_File_Idx_ == 0) ? 1 : 0;
#endif
        db_idx = dirty_idx;
    }

    if (Mem_mgr->segment_memid[p_sn] == 0)
        goto end;

    i = (p_sn * PAGE_PER_SEGMENT) >> 3;
    j = (p_sn * PAGE_PER_SEGMENT) & 0x07;
    sn = (8 * i + j) / PAGE_PER_SEGMENT;
    pn = (8 * i + j) % PAGE_PER_SEGMENT;
    while (1)
    {
        dflag = *(unsigned char *) (&Mem_mgr->dirty_flag_[dirty_idx][i]);
        if (dflag)      // 0이 아니면 dirty page 존재
        {
            for (; j < 8; j++)
            {
                cnt = 1;
                sn = (8 * i + j) / PAGE_PER_SEGMENT;
                if (sn != p_sn)
                    break;
                pn = (8 * i + j) % PAGE_PER_SEGMENT;
                if (dflag & (0x80U >> j))
                {
                    if (db_idx != DBFILE_TEMP_TYPE && log_flush == 0)
                    {
                        LogMgr_buffer_flush(LOGFILE_SYNC_FORCE);
                        log_flush = 1;
                    }

                    page = (struct Page *) PtrToPage(sn, pn);
                    if (IS_CRASHED_PAGE(sn, pn, page))
                    {
                        __SYSLOG2("FAIL segment checkpoint. crashed page (%d)"
                                "db:%d sn:%d, pn:%d, self:%x, link next:%x\n"
                                "    free slotid:%x, rec_cnt:%d, next:%x"
                                "type:%d\n",
                                __LINE__, db_idx, sn, pn,
                                page->header_.self_,
                                page->header_.link_.next_,
                                page->header_.free_slot_id_in_page_,
                                page->header_.record_count_in_page_,
                                page->header_.free_page_next_,
                                page->header_.cont_type_);
                        ret = DB_E_INVALIDPAGE;
                        sc_assert(FALSE, __FILE__, __LINE__);
                        goto end;
                    }

                    if (!is_opened && DBFile_Open(db_idx, p_sn) < 0)
                    {
                        MDB_SYSLOG((" CheckPoint File Open Fail "
                                        "db:%d, p_sn:%d, sn:%d, %d\n",
                                        db_idx, p_sn, sn, __LINE__));
                        goto end;
                    }

                    is_opened = 1;

                    /* segment의 두 페이지가 모두 dirty인 경우 한번에
                     * write */
                    if (pn == 0 && (dflag & (0x80U >> (j + 1))))
                    {
                        cnt = 2;
                    }

                    ret = DBFile_Write(db_idx, sn, pn, page, cnt);

                    if (ret < 0)
                    {
                        MDB_SYSLOG(("checkpoint Write page FAIL 3,"));
                        MDB_SYSLOG(("db:%d sn:%d, pn:%d\n", db_idx, sn, pn));
                    }
                    dflag ^= (0x80U >> j);
                    if (cnt == 2)
                    {
                        ++j;
                        dflag ^= (0x80U >> j);
                    }
                }
            }

            *(unsigned char *) (&Mem_mgr->dirty_flag_[dirty_idx][i]) &= dflag;
        }

        i++;
        sn = (8 * i) / PAGE_PER_SEGMENT;
        if (sn != p_sn)
            break;
    }

  end:
    if (db_idx != DBFILE_TEMP_TYPE)
        SetLastBackupDB(p_sn, db_idx);

    if (is_opened)
    {
        DBFile_Close(db_idx, p_sn);
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
static int enable_checkpointing = 1;
static int do_not_checkpointing = 0;
int
_Checkpointing(int slow, int force)
{
    struct BeginChkptLog beginchkptlog;
    int ret;

    if (server == NULL || server->fTerminated)
        return DB_SUCCESS;

    if (do_not_checkpointing == 1)
    {
        __SYSLOG_TIME("Checkpoint skipped by iSQL_disconnect_nc().\n");
        return DB_SUCCESS;
    }

    if (force == 0 && enable_checkpointing == 0)
    {
        __SYSLOG_TIME("Blocked Checkpoint\n");
        return DB_SUCCESS;
    }

    /* dirty page가 없으면 수행하지 않음 */
    if (server->gDirtyBit == 0)
    {
        Delete_Useless_Logfiles();
        return DB_SUCCESS;
    }

#if defined(MDB_DEBUG)
    __SYSLOG_TIME("Check Point Start... ");
#endif


    ret = __checkpoint(&beginchkptlog, slow);

    if (server->status != SERVER_STATUS_INDEXING
            && server->status != SERVER_STATUS_RECOVERY)
    {
        /* __checkpoint()에서 하던 LogAnchor_WriteToDisk()를 이쪽으로 옮김
         * Delete_Useless_Logfiles()에서 LogAnchor_WriteToDisk()를 하기 때문에
         * 중복을 피하기 위함. loganchor write가 되지 않은 경우 즉,
         * Last_Deleted_Log_File_No_이 같은 경우에는
         * Last_Deleted_Log_File_No_()를 call하도록 함. */
        int last_del_logfile_no;

        last_del_logfile_no = Log_Mgr->Anchor_.Last_Deleted_Log_File_No_;

        Delete_Useless_Logfiles();

        if (last_del_logfile_no == Log_Mgr->Anchor_.Last_Deleted_Log_File_No_)
        {
            LogAnchor_WriteToDisk(&Log_Mgr->Anchor_);
        }
    }

#if defined(MDB_DEBUG)
    __SYSLOG("%d End (%d|%d)\n", Log_Mgr->Anchor_.DB_File_Idx_,
            Log_Mgr->File_.File_No_, Log_Mgr->File_.Offset_);
#endif

    if (ret < 0)
        return ret;

    server->gDirtyBit--;

    return DB_SUCCESS;
}

int
Checkpoint_GetDirty(void)
{
    if (server == NULL || server->gDirtyBit == 0)
        return 0;

    return server->gDirtyBit;
}
