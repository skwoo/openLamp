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
#include "mdb_LogMgr.h"
#include "mdb_LogRec.h"
#include "mdb_syslog.h"

extern int LogFile_AppendLog(char *log_ptr, int log_size, int f_sync);

/*****************************************************************************/
//! LogBuffer_Init 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param logbuffer :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
struct LogBuffer *
LogBuffer_Init(struct LogBuffer *logbuffer)
{
    /* LogPage has been set up in shared_struct_init() */
    logbuffer->buf_size = server->shparams.log_buffer_size;

    logbuffer->head = 0;
    logbuffer->tail = 0;
    logbuffer->last = -1;
    logbuffer->send = -1;
    logbuffer->flush = -1;
    logbuffer->create = -1;

    logbuffer->flush_needed = 0;
    logbuffer->free_limit = logbuffer->buf_size - (512 * KB);

    return logbuffer;
}

/*****************************************************************************/
//! LogBuffer_Final 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void
 ************************************
 * \return  void
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void
LogBuffer_Final(void)
{
    /* step 2: flush log buffer */
    LogBuffer_Flush(LOGFILE_SYNC_FORCE);
}

/*****************************************************************************/
//! LogBuffer_Flush 
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
LogBuffer_Flush(int f_sync)
{
    struct LogBuffer *logbuf = &Log_Mgr->Buffer_;
    int flush_head = 0, flush_tail = 0;
    int try_count = 0;

    /* logbuf->tail: updated by log writers */

  try_again:

    flush_head = logbuf->head;
    flush_tail = logbuf->tail;

    if (flush_head == flush_tail)
    {
        logbuf->flush_needed = 0;       /* reset */
        return DB_SUCCESS;
    }

    if (flush_head < flush_tail)
    {
        if (logbuf->create != -1 &&
                logbuf->create >= flush_head && logbuf->create <= flush_tail)
        {
            /* flush remaining log data */
            LogFile_AppendLog(LogPage + flush_head,
                    (logbuf->create - flush_head), f_sync);
            flush_head = logbuf->create;
            logbuf->create = -1;
            /* create new log file */
            LogFile_Create();
        }

        LogFile_AppendLog(LogPage + flush_head, (flush_tail - flush_head),
                f_sync);

        logbuf->head = flush_tail;
    }
    else    /* flush_head > flush_tail */
    {
        /* checking */
        if (logbuf->last == -1)
        {
            if (((++try_count) & 0x01) == 0)
            {
                MDB_SYSLOG(("LOGBUFFER_CHECK: logbuf->last == -1."
                                "(head=%d, send=%d, tail=%d)\n",
                                logbuf->head, logbuf->send, logbuf->tail));
                return DB_E_LOGBUFFERCHECK1;
            }
            goto try_again;
        }

        /* phase 1: */
        if (logbuf->create != -1 && logbuf->create >= flush_head)
        {
            if (logbuf->create > logbuf->last)
            {
                MDB_SYSLOG(("LOGBUFFER_CHECK: create(%d) is strange!! "
                                "(h=%d,s=%d,t=%d,l=%d)\n",
                                logbuf->create, logbuf->head, logbuf->send,
                                logbuf->tail, logbuf->last));
                return DB_E_LOGBUFFERCHECK2;
            }
            LogFile_AppendLog(LogPage + flush_head,
                    (logbuf->create - flush_head), f_sync);
            flush_head = logbuf->create;
            logbuf->create = -1;
            /* create new log file */
            LogFile_Create();
        }

        LogFile_AppendLog(LogPage + flush_head, (logbuf->last - flush_head),
                f_sync);
        flush_head = 0;
        /* logbuf->last = -1; */

        /* phase 2: */
        if (logbuf->create != -1 && logbuf->create <= flush_tail)
        {
            LogFile_AppendLog(LogPage, logbuf->create, f_sync);
            flush_head = logbuf->create;
            logbuf->create = -1;
            /* create new log file */
            LogFile_Create();
        }

        LogFile_AppendLog(LogPage + flush_head, (flush_tail - flush_head),
                f_sync);

        logbuf->last = -1;
        logbuf->head = flush_tail;
    }

    logbuf->flush_needed = 0;   /* reset */

    return DB_SUCCESS;
}

/*****************************************************************************/
//! logbuf_set_logfilecreate 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param logbuffer :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
logbuf_set_logfilecreate(struct LogBuffer *logbuffer)
{
    if (logbuffer->create != -1)
    {
        MDB_SYSLOG(("Log File Create Pointer(%d) of Log Buffer "
                        "has already been set.\n", logbuffer->create));
        return DB_E_LOGFILECREATEPTR;
    }

    logbuffer->create = logbuffer->tail;

    /* update Log_Mgr->Current_LSN_ */
    Log_Mgr->Current_LSN_.File_No_ += 1;
    Log_Mgr->Current_LSN_.Offset_ = 0;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LogBuffer_WriteLogRecord 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param logbuffer :
 * \param trans : 
 * \param logrec :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LogBuffer_WriteLogRecord(struct LogBuffer *logbuffer,
        struct Trans *trans, struct LogRec *logrec)
{
    register int logrec_length;
    register int free_length;
    register int fix_head;
    register int fix_send;
    int ret;
    DB_UINT32 checksum;

    /* we assume that logrec_length < logbuffer->buf_size */
    /* 4 bytes for checksum, 4 bytes for end mark */
    logrec->header_.lh_length_ += LOG_PADDING_SIZE;
    logrec_length = LOG_HDR_SIZE + logrec->header_.lh_length_;

    if (Log_Mgr->Current_LSN_.Offset_ >= LOG_FILE_SIZE)
    {
        ret = logbuf_set_logfilecreate(logbuffer);
        if (ret < 0)
            return ret;
    }

    /* set the LSN of given log record */
    LSN_Set(logrec->header_.record_lsn_, Log_Mgr->Current_LSN_);

  check_again:
    fix_head = logbuffer->head;
    if (fix_head == logbuffer->tail)
    {
        /*** case 1: *************************************
        |------------------------------------------------|
        |                                                |
        |------------------------------------------------|
                   ^                    
                   |                   
                  head                
                  tail
        **************************************************/
        if (logrec_length >= (logbuffer->buf_size - logbuffer->tail))
        {
            logbuffer->last = logbuffer->tail;
            logbuffer->tail = 0;
            goto check_again;
        }
    }
    else if (fix_head < logbuffer->tail)
    {
        /*** case 2: *************************************
        |------------------------------------------------|
        |          ######################                |
        |------------------------------------------------|
                   ^                    ^
                   |                    |
                  head                 tail
        **************************************************/
        free_length = (logbuffer->buf_size - logbuffer->tail);
        if (logrec_length >= free_length)
        {
            if (fix_head > 0)
            {
                logbuffer->last = logbuffer->tail;
                logbuffer->tail = 0;
                goto check_again;
            }
            /* No enough free space. What should we do ? */
            /* 1) check if logbuffer->send is -1 */
            if (logbuffer->send == -1)
            {
                LogMgr_buffer_flush(LOGFILE_SYNC_SKIP);
                goto check_again;
            }
            /* 2) check if free space can be made when flushed */
            if (logbuffer->send > logrec_length)
            {
                LogMgr_buffer_flush(LOGFILE_SYNC_SKIP);
                goto check_again;
            }
            /* 3) Really, no free space. */
            if (logbuffer->flush != -1)
            {
                logbuffer->head = logbuffer->flush;
                logbuffer->flush = -1;
                if (logbuffer->send > logbuffer->tail &&
                        logbuffer->head < logbuffer->tail)
                    logbuffer->last = -1;
            }
            logbuffer->send = -1;       // clear
            LogBuffer_Flush(LOGFILE_SYNC_SKIP);
            goto check_again;
        }
        else
        {
            if (logbuffer->flush_needed == 0)
            {
                if ((free_length + fix_head) < logbuffer->free_limit)
                    logbuffer->flush_needed = 1;
            }
        }
    }
    else
    {       /* fix_head > logbuffer->tail */
        /*** case 3: *************************************
        |------------------------------------------------|
        |###########                    ###############  |
        |------------------------------------------------|
                   ^                    ^             ^
                   |                    |             |
                  tail                 head          last
        **************************************************/
        free_length = fix_head - logbuffer->tail;
        if (logrec_length >= free_length)
        {
            /* 1) check if logbuffer->send is -1 */
            fix_send = logbuffer->send;
            if (fix_send == -1)
            {
                LogMgr_buffer_flush(LOGFILE_SYNC_SKIP);
                goto check_again;
            }
            /* 2) check if free space can be made when flushed */
            if (fix_send > logbuffer->tail)
            {
                if ((fix_send - logbuffer->tail) > logrec_length)
                {
                    LogMgr_buffer_flush(LOGFILE_SYNC_SKIP);
                    goto check_again;
                }
            }
            else
            {   /* fix_send <= logbuffer->tail */
                if ((logbuffer->buf_size - logbuffer->tail) > logrec_length ||
                        fix_send > logrec_length)
                {
                    LogMgr_buffer_flush(LOGFILE_SYNC_SKIP);
                    goto check_again;
                }
            }
            /* 3) Really, no free space */
            if (logbuffer->flush != -1)
            {
                logbuffer->head = logbuffer->flush;
                logbuffer->flush = -1;
                if (logbuffer->send > logbuffer->tail &&
                        logbuffer->head < logbuffer->tail)
                    logbuffer->last = -1;
            }
            logbuffer->send = -1;       // clear
            LogBuffer_Flush(LOGFILE_SYNC_SKIP);
            goto check_again;
        }
        else
        {
            if (logbuffer->flush_needed == 0)
            {
                if (free_length < logbuffer->free_limit)
                    logbuffer->flush_needed = 1;
            }
        }
    }

    /* write log record header */
    sc_memcpy(LogPage + logbuffer->tail, logrec, LOG_HDR_SIZE);

    /* write log record body */
    switch (logrec->header_.type_)
    {
    case PHYSICAL_LOG:
    case SEGMENTALLOC_LOG:
        if (logrec->data1 == NULL || logrec->data2 == NULL)
        {
            MDB_SYSLOG(("Log(%d) Data is strange.\n", logrec->header_.type_));
            return DB_E_INVALIDLOGDATA;
        }
        /* PAGEALLOC_LOG에서는 recovery_leng_ 에 free_count 값을
         * 넣어서 남기기 때문에 아래 length_를 recovery_leng_로
         * 변경하면 안됨. */
        sc_memcpy(LogPage + logbuffer->tail + LOG_HDR_SIZE,
                logrec->data1,
                (logrec->header_.lh_length_ - LOG_PADDING_SIZE) / 2);
        sc_memcpy(LogPage + logbuffer->tail + LOG_HDR_SIZE +
                ((logrec->header_.lh_length_ - LOG_PADDING_SIZE) / 2),
                logrec->data2,
                (logrec->header_.lh_length_ - LOG_PADDING_SIZE) / 2);
        break;

    case UPDATE_UNDO_LOG:
    case UPDATE_LOG:
        if (logrec->data1 == NULL || logrec->data2 == NULL)
        {
            MDB_SYSLOG(("Log(%d) Data is strange.\n", logrec->header_.type_));
            return DB_E_INVALIDLOGDATA;
        }
        /* recovery log data */
        sc_memcpy(LogPage + logbuffer->tail + LOG_HDR_SIZE,
                logrec->data1, logrec->header_.recovery_leng_ / 2);
        sc_memcpy(LogPage + logbuffer->tail + LOG_HDR_SIZE
                + (logrec->header_.recovery_leng_ / 2),
                logrec->data2, logrec->header_.recovery_leng_ / 2);
        if (logrec->data3 != NULL)
        {
            sc_memcpy(LogPage + logbuffer->tail + LOG_HDR_SIZE
                    + logrec->header_.recovery_leng_,
                    logrec->data3,
                    logrec->header_.lh_length_ - LOG_PADDING_SIZE
                    - logrec->header_.recovery_leng_);
        }
        break;

    case BEFORE_LOG:
    case AFTER_LOG:
    case COMPENSATION_LOG:
    case TRANS_BEGIN_LOG:
    case TRANS_COMMIT_LOG:
    case PAGEALLOC_LOG:
        if (logrec->data1 != NULL)
        {
            sc_memcpy(LogPage + logbuffer->tail + LOG_HDR_SIZE,
                    logrec->data1,
                    logrec->header_.lh_length_ - LOG_PADDING_SIZE);
        }
        break;

    case INSERT_UNDO_LOG:
    case DELETE_UNDO_LOG:
    case INSERT_LOG:
    case DELETE_LOG:
        {
            int data_size = logrec->header_.recovery_leng_
                    - SLOTUSEDLOGDATA_SIZE;

            /* recovery log data */
            sc_memcpy(LogPage + logbuffer->tail + LOG_HDR_SIZE,
                    logrec->data1, data_size);
            sc_memcpy(LogPage + logbuffer->tail + LOG_HDR_SIZE + data_size,
                    logrec->data2, SLOTUSEDLOGDATA_SIZE);
            if (logrec->data3 != NULL)
            {
                sc_memcpy(LogPage + logbuffer->tail + LOG_HDR_SIZE
                        + logrec->header_.recovery_leng_,
                        logrec->data3,
                        logrec->header_.lh_length_ - LOG_PADDING_SIZE
                        - logrec->header_.recovery_leng_);
            }
        }
        break;

    case UPDATEFIELD_LOG:
        if (logrec->data1 == NULL)
        {
            MDB_SYSLOG(("Log(%d) Data is strange.\n", logrec->header_.type_));
            return DB_E_INVALIDLOGDATA;
        }
        /* recovery log data */
        sc_memcpy(LogPage + logbuffer->tail + LOG_HDR_SIZE,
                logrec->data1, logrec->header_.recovery_leng_);
        if (logrec->data3 != NULL)
        {
            sc_memcpy(LogPage + logbuffer->tail + LOG_HDR_SIZE
                    + logrec->header_.recovery_leng_,
                    logrec->data3,
                    logrec->header_.lh_length_ - LOG_PADDING_SIZE
                    - logrec->header_.recovery_leng_);
        }
        break;

    case BEGIN_CHKPT_LOG:      /* no image */
    case INDEX_CHKPT_LOG:
    case TRANS_ABORT_LOG:
        break;

    case END_CHKPT_LOG:
        if (logrec->data1 == NULL)
        {
            MDB_SYSLOG(("Log(%d) Data is strange.\n", logrec->header_.type_));
            return DB_E_INVALIDLOGDATA;
        }
        sc_memcpy(LogPage + logbuffer->tail + LOG_HDR_SIZE,
                logrec->data1, sizeof(int));
        if (logrec->data2 != NULL)
        {
            sc_memcpy(LogPage + logbuffer->tail + LOG_HDR_SIZE
                    + sizeof(int),
                    logrec->data2,
                    logrec->header_.lh_length_ - LOG_PADDING_SIZE -
                    sizeof(int));
        }
        break;
    default:
        goto after_tail_move;
    }

    /* log record body의 checksum을 구한다.
     * 첫번째 4 바이트 + 512 간격마다 4 바이트 + ...
     * checksum은 unsigned int에 sum을 하여 구함.
     * 현재 log record의 시작은 logbuffer->tail부터 시작!
     */
    checksum = mdb_MakeChecksum(LogPage + logbuffer->tail + LOG_HDR_SIZE,
            logrec->header_.lh_length_ - LOG_PADDING_SIZE, LOG_CHKSUM_OFFSET);

    logbuffer->tail += (LOG_HDR_SIZE + logrec->header_.lh_length_);
    sc_memcpy(&LogPage[logbuffer->tail - 8], &checksum, 4);
    LogPage[logbuffer->tail - 4] = 0xE;
    LogPage[logbuffer->tail - 3] = 0xD;
    LogPage[logbuffer->tail - 2] = 0xE;
    LogPage[logbuffer->tail - 1] = 0xD;
  after_tail_move:

    // LogMgr의 Current_LSN의 설정을 바꾼다.
    Log_Mgr->Current_LSN_.Offset_ += logrec_length;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LogBuffer_ReadLogRecord 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param lsn :
 * \param logrec : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LogBuffer_ReadLogRecord(struct LSN *lsn, struct LogRec *logrec)
{
    struct LogBuffer *logbuf = &Log_Mgr->Buffer_;
    int offset;
    int head_pos;
    char *bufptr = NULL;

    if (logbuf->flush == -1)
        head_pos = logbuf->head;
    else
        head_pos = logbuf->flush;

    if (head_pos == logbuf->tail)
    {
        MDB_SYSLOG(("undo log record: doesn't exist in log buffer.\n"));
        return DB_E_READLOGRECORD;
    }

    if (lsn->File_No_ == Log_Mgr->File_.File_No_)
    {
        offset = lsn->Offset_ - Log_Mgr->File_.Offset_;
        if (head_pos < logbuf->tail)
        {
            if (offset < (logbuf->tail - head_pos))
            {
                bufptr = LogPage + head_pos + offset;
                goto read_record;
            }
        }
        else
        {
            if (offset < (logbuf->last - head_pos))
            {
                bufptr = LogPage + head_pos + offset;
                goto read_record;
            }
            offset -= (logbuf->last - head_pos);
            if (offset < logbuf->tail)
            {
                bufptr = LogPage + offset;
                goto read_record;
            }
        }
    }
    else
    {       /* lsn->File_No_ > Log_Mgr->File_.File_No_ */
        offset = lsn->Offset_;
        if (logbuf->create != -1)
        {
            if (logbuf->create < logbuf->tail)
            {
                if (offset < (logbuf->tail - logbuf->create))
                {
                    bufptr = LogPage + logbuf->create + offset;
                    goto read_record;
                }
            }
            else
            {   /* logbuf->create > logbuf->tail */
                if (offset < (logbuf->last - logbuf->create))
                {
                    bufptr = LogPage + logbuf->create + offset;
                    goto read_record;
                }
                offset -= (logbuf->last - logbuf->create);
                if (offset < logbuf->tail)
                {
                    bufptr = LogPage + offset;
                    goto read_record;
                }
            }
        }
    }

    if (bufptr == NULL)
    {
        MDB_SYSLOG(("undo log record: doesn't exist in log buffer.\n"));
        return DB_E_READLOGRECORD;
    }

  read_record:
    sc_memcpy(&logrec->header_, bufptr, LOG_HDR_SIZE);
    if (IS_CRASHED_LOGRECORD(&logrec->header_))
        return DB_E_CRASHLOGRECORD;
    sc_memcpy(logrec->data1, bufptr + LOG_HDR_SIZE,
            logrec->header_.lh_length_);

    return DB_SUCCESS;
}
