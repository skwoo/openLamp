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
#include "mdb_UndoTransTable.h"
#include "mdb_PMEM.h"
#include "mdb_ppthread.h"

__DECL_PREFIX struct LogMgr *Log_Mgr;
__DECL_PREFIX char *LogPage;

extern int fRecovered;

unsigned long CommitLog_Count = 0;

char log_path[MDB_FILE_NAME];

int LogMgr_error = 0;

/*****************************************************************************/
//! LogMgr_Free 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void
LogMgr_Free(void)
{
    if (Log_Mgr != NULL)
    {
        LogBuffer_Final();
        LogFile_Final();
        Log_Mgr = NULL;
    }
}

/*****************************************************************************/
//! LogMgr_Get_DB_Idx_Num 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LogMgr_Get_DB_Idx_Num(void)
{
#ifdef _ONE_BDB_
    /* win ce에서는 하나의 backup DB 사용 */
    return 0;
#else
    return Log_Mgr->Anchor_.DB_File_Idx_;
#endif
}

/*****************************************************************************/
//! LogMgr_init 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param Db_Name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LogMgr_init(char *Db_Name)
{
    if (Log_Mgr == NULL)
    {
        MDB_SYSLOG((" Log_Mgr Memory Allocate FAIL in LogMgr_init\n"));
        return DB_E_LOGMGRINIT;
    }

    {
        int fd;
        char file[MAX_DB_NAME];

        sc_sprintf(file, "%s" DIR_DELIM "%s_logpath",
                sc_getenv("MOBILE_LITE_CONFIG"), Db_Name);

        fd = sc_open(file, O_RDONLY | O_BINARY, OPEN_MODE);
        if (fd < 0)
            log_path[0] = '\0';
        else
        {
            sc_read(fd, log_path, sizeof(log_path));
            sc_close(fd);
        }
    }

    if (log_path[0] == '\0')
    {
        sc_strcpy(Log_Mgr->log_path, sc_getenv("MOBILE_LITE_CONFIG"));
    }
    else
    {
        sc_strcpy(Log_Mgr->log_path, log_path);
    }

    __SYSLOG("Log Path: %s\n", Log_Mgr->log_path);

    LogMgr_Check_LogPath();

    LogAnchor_Init(&Log_Mgr->Anchor_, Db_Name);

    LogFile_Init(Db_Name, &Log_Mgr->File_, &Log_Mgr->Anchor_);

    LogBuffer_Init(&Log_Mgr->Buffer_);

    Log_Mgr->Current_LSN_.File_No_ = 1;
    Log_Mgr->Current_LSN_.Offset_ = 0;

    return DB_SUCCESS;
}


/*****************************************************************************/
//! LogMgr_Check_LogPath 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void
 ************************************
 * \return  int
 ************************************
 * \note Log_Mgr->log_path 없으면 만들어 준다.                \n
 * 일단은 wince 대상으로 하자.                                \n
 *  wince에서는 log를 volatile 영역에 만들므로 이를 대비하기 위해서    \n
 * 메모리가 초기화 된 경우 새로 만들어주는 부분 추가! 
 *****************************************************************************/
int
LogMgr_Check_LogPath(void)
{
    int i = 1;
    int err;

    while (1)
    {
        if (sc_strncmp(Log_Mgr->log_path + i, DIR_DELIM, 1) == 0 ||
                Log_Mgr->log_path[i] == '\0')
        {
            err = sc_mkdir(Log_Mgr->log_path, OPEN_MODE);
            if (err == -1)
                return DB_E_MAKELOGPATH;

            if (Log_Mgr->log_path[i] == '\0')
                break;
        }

        i++;
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LogMgr_WriteLogRecord 
/*! \breif  LOG Buffer에 log를 write 하는 함수 
 ************************************
 * \param logrec :
 ************************************
 * \return  int
 ************************************
 * \note 
 *****************************************************************************/
int
LogMgr_WriteLogRecord(struct LogRec *logrec)
{
    struct Trans *transaction = NULL;
    int ret;
    short type = logrec->header_.type_;

    if (server->status == SERVER_STATUS_RECOVERY)
    {
        return DB_SUCCESS;
    }

#ifdef MDB_DEBUG
    if (isTemp_OID(logrec->header_.oid_))
    {
        sc_assert(0, __FILE__, __LINE__);
    }
#endif

    if (IS_CRASHED_LOGRECORD(&(logrec->header_)))
    {
        MDB_SYSLOG(("log record crashed - %s:%d\n", __FILE__, __LINE__));
        MDB_SYSLOG(("    0x%x, 0x%x, %d\n",
                        logrec->header_.check1, logrec->header_.check2,
                        logrec->header_.lh_length_));
        sc_assert(0, __FILE__, __LINE__);
        return DB_E_CRASHLOGRECORD;
    }

    if (!server->shparams.use_logging)
        return DB_SUCCESS;

    if (type < BEGIN_CHKPT_LOG || type > END_CHKPT_LOG)
    {
        transaction = (struct Trans *) Trans_Search(logrec->header_.trans_id_);

        if (transaction == NULL)
        {
            MDB_SYSLOG(("tx search FAIL %d, type %d\n",
                            logrec->header_.trans_id_, type));
            return DB_E_UNKNOWNTRANSID;
        }

        /* read only이면 그냥 return */
        if (transaction->fReadOnly)
            return DB_SUCCESS;
    }

    /* read only transaction이 아니고, 현재 begin transaction log를
       남기지 않았다면 남기고 계속 진행 */
    if (type < BEGIN_CHKPT_LOG || type > END_CHKPT_LOG)
    {
        if (!(transaction->fBeginLogged))
        {
            LogMgr_WriteBeginTransLogRecord(transaction);

            transaction->fBeginLogged = 1;
        }
    }

    if (type < BEGIN_CHKPT_LOG || type > END_CHKPT_LOG)
    {
        // 트랜잭션 테이블을 이용하여 트랜잭션의 이전 로그레코드의 LSN를
        // 설정한다.
        // 현재 로그 레코드의 LSN를 트랜잭션 테이블의 트랜잭션에 추가한다.
        if (type == COMPENSATION_LOG)
        {
            LSN_Set(logrec->header_.trans_prev_lsn_,
                    transaction->UndoNextLSN_);
        }
        else
        {
            LSN_Set(logrec->header_.trans_prev_lsn_, transaction->Last_LSN_);
        }
    }

    ret = LogBuffer_WriteLogRecord(&Log_Mgr->Buffer_, transaction, logrec);
    if (ret < 0)
    {
        return ret;
    }

    if (type < BEGIN_CHKPT_LOG || type > END_CHKPT_LOG)
    {
        /* First_LSN이 설정되어 있지 않으면 설정시켜준다. */
        if (transaction->First_LSN_.File_No_ == -1)
            LSN_Set(transaction->First_LSN_, logrec->header_.record_lsn_);

        LSN_Set(transaction->Last_LSN_, logrec->header_.record_lsn_);
    }


    return DB_SUCCESS;
}

int TransBeginLog_Init(struct TransBeginLog *trans_begin_log);

/*****************************************************************************/
//! LogMgr_WriteBeginTransLogRecord 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param transaction :
 ************************************
 * \return  int
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LogMgr_WriteBeginTransLogRecord(struct Trans *transaction)
{
    struct TransBeginLog transbeginLogrecord;

    LogRec_Init(&(transbeginLogrecord.logrec));

    transbeginLogrecord.logrec.header_.type_ = TRANS_BEGIN_LOG;

    transbeginLogrecord.logrec.header_.trans_id_ = transaction->id_;

    LSN_Set(transbeginLogrecord.logrec.header_.trans_prev_lsn_,
            transaction->Last_LSN_);

    LogBuffer_WriteLogRecord(&Log_Mgr->Buffer_, transaction,
            (struct LogRec *) &transbeginLogrecord);

    /* First_LSN이 설정되어 있지 않으면 설정시켜준다. */
    if (transaction->First_LSN_.File_No_ == -1)
        LSN_Set(transaction->First_LSN_,
                transbeginLogrecord.logrec.header_.record_lsn_);

    LSN_Set(transaction->Last_LSN_,
            transbeginLogrecord.logrec.header_.record_lsn_);

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LogMgr_PartailUndoTransaction 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param Trans_ID :
 * \param Rollback_LSN : 
 ************************************
 * \return  int
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LogMgr_PartailUndoTransaction(int Trans_ID, struct LSN *Rollback_LSN)
{
    struct Trans *transaction;
    struct LSN Trans_Last_LSN;
    struct LogRec logrec;
    int search_logrec_result;

    transaction = (struct Trans *) Trans_Search(Trans_ID);
    if (transaction == NULL)
    {
        return DB_E_UNKNOWNTRANSID;
    }

    /* read only Tx인 경우 undo 없음 */
    if (transaction->fReadOnly)
    {
        return DB_SUCCESS;
    }

    if (Rollback_LSN->File_No_ == -2 && Rollback_LSN->Offset_ == -2)
    {
        /* savepoint has not been set. */
        return DB_SUCCESS;
    }

    logrec.data1 = mdb_malloc(16 * 1024);
    sc_memset(logrec.data1, 0, 16 * 1024);
    logrec.data2 = NULL;

    LSN_Set(Trans_Last_LSN, transaction->Last_LSN_);

    while (Trans_Last_LSN.File_No_ != Rollback_LSN->File_No_ ||
            Trans_Last_LSN.Offset_ != Rollback_LSN->Offset_)
    {
        sc_memset(&logrec, 0, LOG_HDR_SIZE);
        sc_memset(logrec.data1, 0, 16 * 1024);

        search_logrec_result = LogMgr_ReadLogRecord(&Trans_Last_LSN, &logrec);

        if (search_logrec_result < 0)
        {
            MDB_SYSLOG(("read log Record FAIL \n"));

            SMEM_FREENUL(logrec.data1);
            return search_logrec_result;
        }

        LogRec_Undo_Trans(&logrec, CLR);

        LSN_Set(Trans_Last_LSN, logrec.header_.trans_prev_lsn_);
    };

    SMEM_FREENUL(logrec.data1);

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LogMgr_UndoTransaction 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param Trans_ID :
 ************************************
 * \return  int
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LogMgr_UndoTransaction(int Trans_ID)
{
    struct Trans *transaction;
    struct LSN Trans_Last_LSN;
    struct LogRec logrec;
    int search_logrec_result;
    struct TransAbortLog trans_abort_log;

    transaction = (struct Trans *) Trans_Search(Trans_ID);
    if (transaction == NULL)
    {
        return DB_E_UNKNOWNTRANSID;
    }

    /* read only Tx인 경우 undo 없음 */
    if (transaction->fReadOnly)
    {
        return DB_SUCCESS;
    }

    /* log를 아직 기록하기 전이면 그냥 return */
    if (transaction->Last_LSN_.File_No_ == -1 &&
            transaction->Last_LSN_.Offset_ == -1)
    {
        return DB_SUCCESS;
    }

    logrec.data1 = mdb_malloc(16 * 1024);
    sc_memset(logrec.data1, 0, 16 * 1024);
    logrec.data2 = NULL;

    LSN_Set(Trans_Last_LSN, transaction->Last_LSN_);

    do
    {
        sc_memset(&logrec, 0, LOG_HDR_SIZE);
        sc_memset(logrec.data1, 0, 16 * 1024);

        search_logrec_result = LogMgr_ReadLogRecord(&Trans_Last_LSN, &logrec);

        if (search_logrec_result < 0)
        {
            MDB_SYSLOG(("read log Record FAIL \n"));

            SMEM_FREENUL(logrec.data1);
            return search_logrec_result;
        }

        LogRec_Undo_Trans(&logrec, CLR);

        LSN_Set(Trans_Last_LSN, logrec.header_.trans_prev_lsn_);
    } while (Trans_Last_LSN.File_No_ != -1 && Trans_Last_LSN.Offset_ != -1);

    SMEM_FREENUL(logrec.data1);

    LogRec_Init(&(trans_abort_log.logrec));
    trans_abort_log.logrec.header_.type_ = TRANS_ABORT_LOG;
    trans_abort_log.logrec.header_.trans_id_ = Trans_ID;
    LogMgr_WriteLogRecord((struct LogRec *) &trans_abort_log);

    return DB_SUCCESS;
}


/*****************************************************************************/
//! LogMgr_ReadLogRecord 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param lsn :
 * \param logrec : 
 ************************************
 * \return  int
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LogMgr_ReadLogRecord(struct LSN *lsn, struct LogRec *logrec)
{
    int ret;
    int fileno = lsn->File_No_;
    int offset = lsn->Offset_;

    if (fileno > Log_Mgr->Current_LSN_.File_No_)
    {
        MDB_SYSLOG(("read log record - Invalid LSN (%d : %d)n",
                        fileno, Log_Mgr->Current_LSN_.File_No_));
        return DB_E_INVALIDREADLSN;
    }

    if (fileno < Log_Mgr->File_.File_No_)
    {
        ret = LogFile_ReadFromBackup(lsn, logrec);
        if (ret < 0)
        {
            MDB_SYSLOG(("read log record - FAIL (from backup log file)\n"));
            return ret;
        }
        return DB_SUCCESS;
    }

    if (fileno == Log_Mgr->File_.File_No_ && offset < Log_Mgr->File_.Offset_)
    {
        ret = LogFile_ReadLogRecord(lsn, logrec);
        if (ret < 0)
        {
            MDB_SYSLOG(("read log record - FAIL (from current log file)\n"));
            return ret;
        }
    }
    else
    {
        /* (fileno==Log_Mgr->File_.File_No_&&offset>=Log_Mgr->File_.Offset_) 
           || (fileno>Log_Mgr->File_.File_No_) */
        ret = LogBuffer_ReadLogRecord(lsn, logrec);
        if (ret < 0)
        {
            MDB_SYSLOG(("read log record - FAIL (from log buffer)\n"));
            return ret;
        }
    }

    return DB_SUCCESS;
}


/*****************************************************************************/
//! LogMgr_Restart 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void 
 ************************************
 * \return  int
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LogMgr_Restart(void)
{
    struct LSN Chk_Start_lsn;
    int ret_redo, ret_undo;
    int ret = DB_SUCCESS;

    LogMgr_error = 0;

    // Log File이 존재 하지 않는다면, Redo Undo룰 수행하지 않는다.

    __SYSLOG("RESTART Begin: logfiles(%d-%d), Chkpt_LSN(%d|%d - %d|%d)\n",
            Log_Mgr->Anchor_.Last_Deleted_Log_File_No_ + 1,
            Log_Mgr->Anchor_.Current_Log_File_No_,
            Log_Mgr->Anchor_.Begin_Chkpt_lsn_.File_No_,
            Log_Mgr->Anchor_.Begin_Chkpt_lsn_.Offset_,
            Log_Mgr->Anchor_.End_Chkpt_lsn_.File_No_,
            Log_Mgr->Anchor_.End_Chkpt_lsn_.Offset_);

    Chk_Start_lsn.File_No_ = Log_Mgr->Anchor_.Begin_Chkpt_lsn_.File_No_;
    Chk_Start_lsn.Offset_ = Log_Mgr->Anchor_.Begin_Chkpt_lsn_.Offset_;

// logBuffer의 첫부분에 LSN를 위해 Current_LSN를 세팅

    if (server->shparams.db_start_recovery == 1)
    {
        ret_redo = LogMgr_Redo(&Chk_Start_lsn);

        __SYSLOG("REDO end: status = %d\n", fRecovered);

        ret_undo = LogMgr_Undo();

        __SYSLOG("UNDO end: status = %d\n", fRecovered);

        if (LogMgr_error != 0)
        {
            return DB_E_OIDPOINTER;
        }

        if ((DB_E_LOGFILEREAD <= ret_redo && ret_redo <= DB_E_LOGFILEOPEN) ||
                (DB_E_LOGFILEREAD <= ret_undo &&
                        ret_undo <= DB_E_LOGFILEOPEN) ||
                ret_redo == DB_E_CRASHLOGRECORD ||
                ret_undo == DB_E_CRASHLOGRECORD)
        {
            /* LogMgr_Scan_CommitList()에서 이미 처리가 되었기에
             *          * 여기서도 확인 작업 수행 */
            if (Log_Mgr->File_.File_No_ <= Chk_Start_lsn.File_No_)
            {
                Log_Mgr->Buffer_.create = -1;
                LogFile_Create();
            }
            ret = DB_FAIL;
        }
    }

    /* 지금까지 offset이 결정되지 않았다면 log file crash 발생된 것임
     *      * 현재 log file의 offset 0부터 write를 하는 것으로 처리. */
    if (Log_Mgr->File_.Offset_ == -1)
    {
        Log_Mgr->File_.Offset_ = 0;
#ifdef MDB_DEBUG
        MDB_SYSLOG(("set log file offset to 0, (%d,%d)\n",
                        Log_Mgr->Current_LSN_.File_No_,
                        Log_Mgr->Current_LSN_.Offset_));
#endif
    }

    Log_Mgr->Current_LSN_.File_No_ = Log_Mgr->File_.File_No_;
    Log_Mgr->Current_LSN_.Offset_ = Log_Mgr->File_.Offset_;

    /* Log_Mgr->Buffer_.send == -1 */
    LogMgr_buffer_flush(LOGFILE_SYNC_FORCE);

    __SYSLOG("RESTART End: Current_LSN(%d|%d)\n\n",
            Log_Mgr->Current_LSN_.File_No_, Log_Mgr->Current_LSN_.Offset_);

    /* reset trans id */
    TransMgr_Set_NextTransID(0);

    return ret;
}


/*****************************************************************************/
//! LogMgr_Redo 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param Chkpt_Start_lsn :
 ************************************
 * \return  int
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LogMgr_Redo(struct LSN *Chkpt_Start_lsn)
{
    // 마지막 End 체크포인트 지점의 LSN를 가지고 온다.
    struct LSN last_chkpt_lsn;
    struct EndChkptLog last_chkpt_log;
    int LOG_CHK = 0;
    int i = 0;
    struct Trans4Log *transaction = NULL;
    int Trans_Count = 0;
    int Trans_Num = 0;
    struct Trans *trans = NULL;
    int ret;

    last_chkpt_log.logrec.data1 = (char *) mdb_malloc(16 * 1024);
    sc_memset(last_chkpt_log.logrec.data1, 0, 16 * 1024);
    last_chkpt_log.logrec.data2 = NULL;

    transaction = NULL;
    last_chkpt_lsn.File_No_ = Log_Mgr->Anchor_.End_Chkpt_lsn_.File_No_;
    last_chkpt_lsn.Offset_ = Log_Mgr->Anchor_.End_Chkpt_lsn_.Offset_;

    if (last_chkpt_lsn.File_No_ == -1 && last_chkpt_lsn.Offset_ == -1)
    {
        *(int *) (last_chkpt_log.logrec.data1) = 0;
        Chkpt_Start_lsn->File_No_ = 1;
        Chkpt_Start_lsn->Offset_ = 0;

        Log_Mgr->Current_LSN_.File_No_ = Chkpt_Start_lsn->File_No_;
        Log_Mgr->Current_LSN_.Offset_ = Chkpt_Start_lsn->Offset_;

        SMEM_FREENUL(last_chkpt_log.logrec.data1);

        return DB_E_LASTCHKPTLSN;
    }

    /* last_chkpt_lsn 뒤 로그레코드를 읽어 온다. */
    else if (last_chkpt_lsn.File_No_ < Log_Mgr->Anchor_.Current_Log_File_No_)
    {
        ret = LogFile_ReadFromBackup(&last_chkpt_lsn, &last_chkpt_log.logrec);
    }
    else
    {
        ret = LogFile_ReadLogRecord(&last_chkpt_lsn, &last_chkpt_log.logrec);
    }

    if (ret != DB_SUCCESS)
    {
        Log_Mgr->Current_LSN_.File_No_ = Chkpt_Start_lsn->File_No_;
        Log_Mgr->Current_LSN_.Offset_ = Chkpt_Start_lsn->Offset_;

        SMEM_FREENUL(last_chkpt_log.logrec.data1);

        return ret;
    }

    TransMgr_Set_NextTransID(last_chkpt_log.logrec.header_.trans_id_);

    /* end checkpoint log에 들어 있는 active transaction 정보를 가져온다. */
    Trans_Count = *(int *) (last_chkpt_log.logrec.data1);

    __SYSLOG("REDO start: num of trans = %d, status = %d\n",
            Trans_Count, fRecovered);

    if (Trans_Count > 0)
    {
        transaction = (struct Trans4Log *)
                (last_chkpt_log.logrec.data1 + sizeof(int));
    }

    /* end chkpt log 작성시 active했던 transaction 복귀 */
    for (i = 0; i < Trans_Count; i++)
    {
        Trans_Num = TransMgr_InsTrans(transaction[i].trans_id_,
                CS_CLIENT_NORMAL, -1);
        trans = (struct Trans *) Trans_Search(Trans_Num);
        if (trans == NULL)
            continue;

        LSN_Set(trans->Last_LSN_, transaction[i].Last_LSN_);
        LSN_Set(trans->UndoNextLSN_, transaction[i].UndoNextLSN_);
    }

    Log_Mgr->Current_LSN_.File_No_ = Chkpt_Start_lsn->File_No_;
    Log_Mgr->Current_LSN_.Offset_ = Chkpt_Start_lsn->Offset_;

    SMEM_FREENUL(last_chkpt_log.logrec.data1);

    ret = LOG_CHK = LogFile_Redo(Chkpt_Start_lsn,
            Log_Mgr->Anchor_.Current_Log_File_No_, &(Log_Mgr->Current_LSN_),
            1);

    if (LOG_CHK == 20)
    {
        return LOG_CHK;
    }

    if (ret < 0)
        return ret;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LogMgr_Undo 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param int :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LogMgr_Undo(void)
{
    struct Trans4Log *transaction = NULL;
    int Num_Of_Trans = 0;

    struct UndoTransTable *trans_table = NULL;
    struct LogRec logrec;
    struct LSN lsn, stop_lsn;
    int i;
    int ret = 0;

    logrec.data1 = (char *) mdb_malloc(16 * 1024);
    sc_memset(logrec.data1, 0, 16 * 1024);
    logrec.data2 = NULL;

    Trans_Mgr_AllTransGet(&Num_Of_Trans, &transaction);

    __SYSLOG("UNDO start: num of trans = %d, status = %d\n",
            Num_Of_Trans, fRecovered);

    if (Num_Of_Trans == 0)
    {
        SMEM_FREENUL(logrec.data1);
        return DB_SUCCESS;
    }
    else if (transaction == NULL)
    {
        SMEM_FREENUL(logrec.data1);
        return DB_E_UNKNOWNTRANSID;
    }

    trans_table = (struct UndoTransTable *)
            mdb_malloc(sizeof(struct UndoTransTable));

    if (trans_table == NULL)
    {
        SMEM_FREENUL(logrec.data1);
        SMEM_FREENUL(transaction);
        return DB_E_OUTOFMEMORY;
    }
    trans_table->Num_Of_LSN_ = 0;
    trans_table->max_lsn_ = (struct LSN *)
            mdb_malloc(sizeof(struct LSN) * Num_Of_Trans + sizeof(struct LSN));

    if (trans_table->max_lsn_ == NULL)
    {
        MDB_SYSLOG(("Error:  max lsn Memory Malloc FAIL In TransTable\n"));
    }

    for (i = 0; i < Num_Of_Trans; i++)
    {
        TransTable_Insert(trans_table, &(transaction[i].Last_LSN_));
    }

    SMEM_FREENUL(transaction);

    if (Num_Of_Trans > 0)
    {
        TransTable_Remove_LSN(trans_table, &lsn);
    }
    else
    {
        lsn.File_No_ = -1;
        lsn.Offset_ = -1;
    }

    stop_lsn.File_No_ = -1;
    stop_lsn.Offset_ = -1;

    while ((lsn.File_No_ != stop_lsn.File_No_) &&
            (lsn.Offset_ != stop_lsn.Offset_))
    {
        {
            ret = LogFile_ReadFromBackup(&lsn, &logrec);
            if (ret < 0)
            {
                MDB_SYSLOG(("read log read FAIL for undo %d %d\n", __LINE__,
                                ret));
                break;
            }
        }

        if (LogRec_Undo(&logrec, NO_CLR) < 0)
        {
            SMEM_FREENUL(trans_table->max_lsn_);
            SMEM_FREENUL(trans_table);
            SMEM_FREENUL(logrec.data1);
            MDB_SYSLOG(("Undo Fail\n"));
            return DB_E_UNDOLOGRECORD;
        }

        TransTable_Replace_LSN(trans_table, &(logrec.header_.trans_prev_lsn_),
                &lsn);
    }

    SMEM_FREENUL(trans_table->max_lsn_);

    Trans_Mgr_ALLTransDel();

    SMEM_FREENUL(trans_table);

    SMEM_FREENUL(logrec.data1);

    if (ret < 0)
        return ret;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LogMgr_buffer_flush 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void
 ************************************
 * \return  int    :
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LogMgr_buffer_flush(int f_sync)
{
    LogBuffer_Flush(f_sync);

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LogMgr_Check_And_Flush 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param logrec :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LogMgr_Check_And_Flush(struct LogRec *logrec)
{
    struct LSN *flush_lsn;

    flush_lsn = &logrec->header_.record_lsn_;

    /**** NOTE: LSN comparison ************* 
    For checking if the given LSN is smaller than LogFile LSN.
    However, following orders must be kept in the below two cases.

    A. Updating LogFile LSN.
        (1) Log_Mgr->File_.Offset_  = offset_value;
        (2) Log_Mgr->File_.File_No_ = fileno_value;
    B. Checking if given LSN is smaller than LogFile LSN.
        (1) File_No_ comparison
        (2) Offset_ comparison

    According to the above comparison method, following facts occur.
    Case 1: The given LSN <  LogFile LSN
            In this case, it might make an incorrect result
                            like the given LSN >= LogFile LSN.
    Case 2: The given LSN >= LogFile LSN
            In this case, it always make a correct result.
    Even though the occurence of some misjudgement of case 1,
    they do not break down the stability of system.
    So, the above method can be used in safety.
    *******************************************/

    if ((flush_lsn->File_No_ < Log_Mgr->File_.File_No_) ||
            (flush_lsn->File_No_ == Log_Mgr->File_.File_No_ &&
                    flush_lsn->Offset_ < Log_Mgr->File_.Offset_))
    {
        /* The given log record has already been flushed. */
        return DB_SUCCESS;
    }

    if ((flush_lsn->File_No_ < Log_Mgr->File_.File_No_) ||
            (flush_lsn->File_No_ == Log_Mgr->File_.File_No_ &&
                    flush_lsn->Offset_ < Log_Mgr->File_.Offset_))
    {
        /* The given log record has already been flushed. */
        return DB_SUCCESS;
    }

    LogBuffer_Flush(LOGFILE_SYNC_FORCE);

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LogMgr_IsFlushed 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param flush_lsn :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/

// functions below are called only at startup
// (recovery) stage. Therefore, there is no need to be concerned about
// multithreaded situation.
static int *commitList = NULL;
static int numList = 0;
static int posList = 0;
static int curList = 0;

int
LogMgr_Scan_CommitList(void)
{
    struct LSN lsn, currlsn;
    int ret;

    SMEM_FREENUL(commitList);
    commitList = (int *) SMEM_ALLOC(sizeof(int) * 50);
    if (commitList == NULL)
        return DB_E_OUTOFMEMORY;

    numList = 50;
    posList = 0;
    curList = 0;

    lsn.File_No_ = Log_Mgr->Anchor_.Begin_Chkpt_lsn_.File_No_;
    lsn.Offset_ = Log_Mgr->Anchor_.Begin_Chkpt_lsn_.Offset_;

    ret = LogFile_Redo(&lsn, Log_Mgr->Anchor_.Current_Log_File_No_,
            &(currlsn), 0);

    if (ret < 0)
    {
        /* LogMgr_Restart()에 의해서 log write가 발생할 수도 있으므로
         *          * LogMgr_Restart에서 처리하는 것을 이곳에서도 처리함.
         *                   * 현재 log file이 깨어진 것이라면 다음 logfile로
         *                   교체 */
        if ((DB_E_LOGFILEREAD <= ret && ret <= DB_E_LOGFILEOPEN) ||
                ret == DB_E_CRASHLOGRECORD)
        {
            if (Log_Mgr->File_.File_No_ <= currlsn.File_No_)
            {
                Log_Mgr->Buffer_.create = -1;
                LogFile_Create();
            }
        }

        return ret;
    }
    else
    {
        if (Log_Mgr->File_.Offset_ == -1)
            Log_Mgr->File_.Offset_ = lsn.Offset_;
    }

    return DB_SUCCESS;
}

int
LogMgr_Delete_CommitList(void)
{
    SMEM_FREENUL(commitList);
    numList = posList = curList = 0;

    return DB_SUCCESS;
}

int
LogMgr_Insert_Commit(int transid)
{
    if (commitList == NULL)
    {
        commitList = (int *) SMEM_ALLOC(sizeof(int) * 50);
        if (commitList == NULL)
            return DB_E_OUTOFMEMORY;

        numList = 50;
        posList = 0;
        curList = 0;
    }

    commitList[posList] = transid;
    posList++;
    if (posList == numList)
    {
        int *newlist = (int *) SMEM_ALLOC(sizeof(int) * (numList + 10));

        if (newlist == NULL)
            return DB_E_OUTOFMEMORY;

        sc_memcpy(newlist, commitList, sizeof(int) * numList);
        SMEM_FREE(commitList);
        commitList = newlist;
        numList += 10;
    }

    return DB_SUCCESS;
}

int
LogMgr_Remove_Commit(int transid)
{
    if (commitList == NULL)
        return DB_FAIL;

    if (curList >= posList || commitList[curList] != transid)
        return DB_FAIL;

    curList += 1;

    return DB_SUCCESS;
}

int
LogMgr_Check_Commit(int transid)
{
    int i;

    if (commitList == NULL)
        return 0;

    for (i = curList; i < posList; i++)
    {
        if (commitList[i] == transid)
            return 1;
    }

    return 0;
}
