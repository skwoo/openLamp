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
#include "mdb_LogRec.h"
#include "mdb_inner_define.h"
#include "ErrorCode.h"
#include "mdb_compat.h"
#include "mdb_syslog.h"
#include "mdb_Server.h"

extern int fRecovered;

int AfterLog_Undo(struct LogRec *logrec, int flag);
int AfterLog_Redo(struct LogRec *logrec);

int BeforeLog_Undo(struct LogRec *logrec, int flag);
int BeforeLog_Redo(struct LogRec *logrec);

int BeginChkptLog_Undo(struct LogRec *logrec, int flag);
int BeginChkptLog_Redo(struct LogRec *logrec);

int PhysicalLog_Redo(struct LogRec *logrec);
int PhysicalLog_Undo(struct LogRec *logrec, int flag);

int InsertLog_Undo(struct LogRec *logrec, int flag);
int InsertLog_Redo(struct LogRec *logrec);

int DeleteLog_Undo(struct LogRec *logrec, int flag);
int DeleteLog_Redo(struct LogRec *logrec);

int UpdateLog_Undo(struct LogRec *logrec, int flag);
int UpdateLog_Redo(struct LogRec *logrec);

int UpdateFieldLog_Undo(struct LogRec *logrec, int flag);
int UpdateFieldLog_Redo(struct LogRec *logrec);

int CompensationLog_Redo(struct LogRec *logrec);
int CompensationLog_Undo(struct LogRec *logrec);

int TransAbortLog_Undo(struct LogRec *logrec, int flag);
int TransAbortLog_Redo(struct LogRec *logrec);

int TransBeginLog_Undo(struct LogRec *logrec, int flag);
int TransBeginLog_Redo(struct LogRec *logrec);

int TransCommitLog_Redo(struct LogRec *logrec);


/* for f_checkpoint_finished */
int LogMgr_Check_Commit(int transid);

int MemAnchorLog_Undo(struct LogRec *logrec, int flag);
int MemAnchorLog_Redo(struct LogRec *logrec);
int SegmentAllocLog_Redo(struct LogRec *logrec);

int InsertLog_Index_Insert_Redo(struct LogRec *logrec);
int InsertLog_Index_Insert_Undo(struct LogRec *logrec);
int DeleteLog_Index_Remove_Redo(struct LogRec *logrec);
int DeleteLog_Index_Remove_Undo(struct LogRec *logrec);
int UpdateLog_Index_Remove_Old_Redo(struct LogRec *logrec);
int UpdateLog_Index_Insert_New_Redo(struct LogRec *logrec);
int UpdateLog_Index_Insert_New_Undo(struct LogRec *logrec);
int UpdateLog_Index_Remove_Old_Undo(struct LogRec *logrec);
int UpdateFieldLog_Index_Remove_Old_Redo(struct LogRec *logrec);
int UpdateFieldLog_Index_Insert_New_Redo(struct LogRec *logrec);
int UpdateFieldLog_Index_Insert_New_Undo(struct LogRec *logrec);
int UpdateFieldLog_Index_Remove_Old_Undo(struct LogRec *logrec);

int InsertLog_Schema_Cache_Undo(struct LogRec *logrec);
int DeleteLog_Schema_Cache_Undo(struct LogRec *logrec);
int UpdateLog_Schema_Cache_Undo(struct LogRec *logrec);

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
LogRec_Undo(struct LogRec *logrec, int flag)
{
    switch (logrec->header_.type_)
    {
    case PHYSICAL_LOG:
        PhysicalLog_Undo(logrec, flag);
        fRecovered |= PARTIAL_REBUILD;
        break;
    case BEFORE_LOG:
        BeforeLog_Undo(logrec, flag);
        fRecovered |= PARTIAL_REBUILD;
        break;
    case AFTER_LOG:
        AfterLog_Undo(logrec, flag);
        fRecovered |= PARTIAL_REBUILD;
        break;
    case INSERT_UNDO_LOG:
        if (server->status == SERVER_STATUS_RECOVERY)
            break;
        /* fallthrough */
    case INSERT_LOG:
        InsertLog_Undo(logrec, flag);
        fRecovered |= PARTIAL_REBUILD;
        break;
    case DELETE_UNDO_LOG:
        if (server->status == SERVER_STATUS_RECOVERY)
            break;
        /* fallthrough */
    case DELETE_LOG:
        DeleteLog_Undo(logrec, flag);
        fRecovered |= PARTIAL_REBUILD;
        break;
    case UPDATE_UNDO_LOG:
        if (server->status == SERVER_STATUS_RECOVERY)
            break;
        /* fallthrough */
    case UPDATE_LOG:
        UpdateLog_Undo(logrec, flag);
        fRecovered |= PARTIAL_REBUILD;
        break;
    case UPDATEFIELD_LOG:
        UpdateFieldLog_Undo(logrec, flag);
        fRecovered |= PARTIAL_REBUILD;
        break;
    case COMPENSATION_LOG:
        CompensationLog_Undo(logrec);
        fRecovered |= PARTIAL_REBUILD;
        break;
    case TRANS_BEGIN_LOG:
        TransBeginLog_Undo(logrec, flag);
        break;
    case TRANS_ABORT_LOG:
        TransAbortLog_Undo(logrec, flag);
        break;
    case BEGIN_CHKPT_LOG:
        break;
    case INDEX_CHKPT_LOG:
        break;
    case END_CHKPT_LOG:
        break;
    default:
        break;
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
struct Container *Log_Cont;

int
LogRec_Redo(struct LogRec *logrec)
{
    if (logrec->header_.record_lsn_.File_No_ <
            mem_anchor_->anchor_lsn.File_No_)
        goto end;

    if (logrec->header_.record_lsn_.File_No_ ==
            mem_anchor_->anchor_lsn.File_No_
            && logrec->header_.record_lsn_.Offset_ <
            mem_anchor_->anchor_lsn.Offset_)
        goto end;

    Log_Cont = NULL;

    switch (logrec->header_.type_)
    {
    case PHYSICAL_LOG:
        {
            int f_check = 0;

            if (f_checkpoint_finished >= CHKPT_CMPLT &&
                    (fRecovered & ALL_REBUILD) == 0 &&
                    logrec->header_.op_type_ == UPDATE_HEAPREC &&
                    LogMgr_Check_Commit(logrec->header_.trans_id_))
            {
                UpdateLog_Index_Remove_Old_Redo(logrec);
#ifdef CHECK_INDEX_VALIDATION
                Index_Check_Page();
#endif
                f_check = 1;
            }
            PhysicalLog_Redo(logrec);
            fRecovered |= PARTIAL_REBUILD;
            if (f_check)
            {
                UpdateLog_Index_Insert_New_Redo(logrec);
#ifdef CHECK_INDEX_VALIDATION
                Index_Check_Page();
#endif
            }
            break;
        }

    case BEFORE_LOG:
        BeforeLog_Redo(logrec);
        fRecovered |= PARTIAL_REBUILD;
        break;

    case AFTER_LOG:
        AfterLog_Redo(logrec);
        fRecovered |= PARTIAL_REBUILD;
        break;

    case INSERT_LOG:
        InsertLog_Redo(logrec);
        fRecovered |= PARTIAL_REBUILD;
        if (f_checkpoint_finished >= CHKPT_CMPLT &&
                (fRecovered & ALL_REBUILD) == 0 &&
                logrec->header_.op_type_ == INSERT_HEAPREC &&
                LogMgr_Check_Commit(logrec->header_.trans_id_))
        {
            InsertLog_Index_Insert_Redo(logrec);
#ifdef CHECK_INDEX_VALIDATION
            Index_Check_Page();
#endif
        }
        break;

    case DELETE_LOG:
        if (f_checkpoint_finished >= CHKPT_CMPLT &&
                (fRecovered & ALL_REBUILD) == 0 &&
                logrec->header_.op_type_ == REMOVE_HEAPREC &&
                LogMgr_Check_Commit(logrec->header_.trans_id_))
        {
            DeleteLog_Index_Remove_Redo(logrec);
#ifdef CHECK_INDEX_VALIDATION
            Index_Check_Page();
#endif
        }
        DeleteLog_Redo(logrec);
        fRecovered |= PARTIAL_REBUILD;
        break;

    case UPDATE_LOG:
        {
            int f_check = 0;

            if (f_checkpoint_finished >= CHKPT_CMPLT &&
                    (fRecovered & ALL_REBUILD) == 0 &&
                    logrec->header_.op_type_ == UPDATE_HEAPREC &&
                    LogMgr_Check_Commit(logrec->header_.trans_id_))
            {
                UpdateLog_Index_Remove_Old_Redo(logrec);
#ifdef CHECK_INDEX_VALIDATION
                Index_Check_Page();
#endif
                f_check = 1;
            }
            UpdateLog_Redo(logrec);
            fRecovered |= PARTIAL_REBUILD;
            if (f_check)
            {
                UpdateLog_Index_Insert_New_Redo(logrec);
#ifdef CHECK_INDEX_VALIDATION
                Index_Check_Page();
#endif
            }
            break;
        }

    case UPDATEFIELD_LOG:
        {
            int f_check = 0;

            if (f_checkpoint_finished >= CHKPT_CMPLT &&
                    (fRecovered & ALL_REBUILD) == 0 &&
                    logrec->header_.op_type_ == UPDATE_HEAPFLD &&
                    LogMgr_Check_Commit(logrec->header_.trans_id_))
            {
                UpdateFieldLog_Index_Remove_Old_Redo(logrec);
#ifdef CHECK_INDEX_VALIDATION
                Index_Check_Page();
#endif
                f_check = 1;
            }
            UpdateFieldLog_Redo(logrec);
            fRecovered |= PARTIAL_REBUILD;
            if (f_check)
            {
                UpdateFieldLog_Index_Insert_New_Redo(logrec);
#ifdef CHECK_INDEX_VALIDATION
                Index_Check_Page();
#endif
            }
            break;
        }

    case COMPENSATION_LOG:
        CompensationLog_Redo(logrec);
        fRecovered |= PARTIAL_REBUILD;
        break;
    case TRANS_BEGIN_LOG:
        TransBeginLog_Redo(logrec);
        break;
    case TRANS_ABORT_LOG:
        TransAbortLog_Redo(logrec);
        break;
    case TRANS_COMMIT_LOG:
        TransCommitLog_Redo(logrec);
        break;
    case BEGIN_CHKPT_LOG:
        break;
    case INDEX_CHKPT_LOG:
        break;
    case END_CHKPT_LOG:
        break;
    case PAGEALLOC_LOG:
        MemAnchorLog_Redo(logrec);
        server->f_page_list_updated = 1;
        fRecovered |= PARTIAL_REBUILD;
        break;
    case SEGMENTALLOC_LOG:
        SegmentAllocLog_Redo(logrec);
        fRecovered |= PARTIAL_REBUILD;
        break;
    default:
        break;
    }

  end:

    Log_Cont = NULL;

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
LogRec_Undo_Trans(struct LogRec *logrec, int Flag)
{
    switch (logrec->header_.op_type_)
    {
    case INSERT_HEAPREC:
        InsertLog_Index_Insert_Undo(logrec);
        LogRec_Undo(logrec, Flag);

        InsertLog_Schema_Cache_Undo(logrec);

        fRecovered |= PARTIAL_REBUILD;
        break;

    case INSERT_VARDATA:
        LogRec_Undo(logrec, Flag);
        fRecovered |= PARTIAL_REBUILD;
        break;

    case REMOVE_HEAPREC:
        LogRec_Undo(logrec, Flag);
        DeleteLog_Index_Remove_Undo(logrec);
        DeleteLog_Schema_Cache_Undo(logrec);

        fRecovered |= PARTIAL_REBUILD;
        break;

    case REMOVE_VARDATA:
        LogRec_Undo(logrec, Flag);
        fRecovered |= PARTIAL_REBUILD;
        break;

    case UPDATE_HEAPREC:
        UpdateLog_Index_Insert_New_Undo(logrec);
        LogRec_Undo(logrec, Flag);
        UpdateLog_Index_Remove_Old_Undo(logrec);
        UpdateLog_Schema_Cache_Undo(logrec);

        fRecovered |= PARTIAL_REBUILD;
        break;

    case UPDATE_HEAPFLD:
        UpdateFieldLog_Index_Insert_New_Undo(logrec);
        LogRec_Undo(logrec, Flag);
        UpdateFieldLog_Index_Remove_Old_Undo(logrec);
        fRecovered |= PARTIAL_REBUILD;
        break;

    case RELATION_CREATE:
        {
            struct BaseContainer base_cont;

            sc_memcpy(&base_cont, logrec->data1 + sizeof(struct Collection),
                    sizeof(struct BaseContainer));
            LogRec_Undo(logrec, Flag);
            Cont_RemoveHashing(base_cont.name_, base_cont.id_);
            break;
        }

    case RELATION_DROP:
        {
            struct BaseContainer base_cont;

            sc_memcpy(&base_cont, logrec->data1 + sizeof(struct Collection),
                    sizeof(struct BaseContainer));
            LogRec_Undo(logrec, Flag);
            Cont_InputHashing(base_cont.name_, base_cont.id_,
                    logrec->header_.oid_);
            break;
        }

    default:
        LogRec_Undo(logrec, Flag);
    }

    return DB_SUCCESS;
}

void
LogRec_Init(struct LogRec *logrec)
{
    logrec->header_.op_type_ = -1;
    logrec->header_.type_ = -1;
    logrec->header_.tableid_ = 0;
    logrec->header_.relation_pointer_ = NULL_OID;
    logrec->header_.collect_index_ = -1;
    logrec->header_.trans_id_ = -1;
    logrec->header_.oid_ = NULL_OID;
    logrec->header_.lh_length_ = 0;
    logrec->header_.recovery_leng_ = 0;
    LogLSN_Init(&(logrec->header_.record_lsn_));
    LogLSN_Init(&(logrec->header_.trans_prev_lsn_));
    logrec->data1 = logrec->data2 = logrec->data3 = NULL;
    logrec->header_.check1 = logrec->header_.check2 = LOGREC_MAGIC_NUM;
}
