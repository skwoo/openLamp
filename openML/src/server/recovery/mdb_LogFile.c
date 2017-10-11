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
#include "mdb_LogAnchor.h"
#include "mdb_LogFile.h"
#include "mdb_LogRec.h"
#include "mdb_Mem_Mgr.h"
#include "mdb_compat.h"
#include "mdb_PMEM.h"

extern void print_fstat(int fd);

#include "mdb_syslog.h"
#include "mdb_Server.h"

/* 기존 log file이 있다면 그대로 사용하기 위해서 O_TRUNC를 제거함 */
#define CREAT_FLAG  O_RDWR | O_CREAT | O_BINARY

extern struct LogMgr *Log_Mgr;
extern int fRecovered;

int LogRec_Redo(struct LogRec *logrec);

int LogMgr_Insert_Commit(int transid);
int LogMgr_Remove_Commit(int transid);

static int LogFile_OpenFile(int *file_len);
static int LogFile_CloseFile(void);

__DECL_PREFIX const char Str_EndOfLog[16] =
        { 'E', 'n', 'd', ' ', 'o', 'f', ' ', 'L', 'o', 'g', 'R', 'e', 'c', 'o',
            'r', 'd' };


/*****************************************************************************/
//! LogFile_Init 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param DB_Name :
 * \param logfile : 
 * \param loganchor :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 * - log file 초기화
 * loganchor에 기록된 log file이 있는지 확인.
 * 있으면 마지막 offset(file 크기)를 파악해서 저장하고,
 * 없으면 해당 log file 만들어줌 
 *****************************************************************************/
struct LogFile *
LogFile_Init(char *DB_Name, struct LogFile *logfile,
        struct LogAnchor *loganchor)
{
    char LogFile_Name[MDB_FILE_NAME];
    int fd;
    int file_no;

    sc_sprintf(LogFile_Name, "%s" DIR_DELIM "%s_logfile%d",
            Log_Mgr->log_path, DB_Name, (int) loganchor->Current_Log_File_No_);

    sc_strncpy(logfile->File_Name_, LogFile_Name, MDB_FILE_NAME);
    logfile->File_No_ = loganchor->Current_Log_File_No_;

    logfile->fd_ = sc_open(logfile->File_Name_,
            O_RDWR | O_CREAT | O_BINARY, OPEN_MODE);

    if (logfile->fd_ != -1)
    {       /* file 존재 */
        logfile->Offset_ = sc_lseek(logfile->fd_, 0, SEEK_END);

        /* log file 크기 확보, LOG_FILE_SIZE+4KB */
        if (logfile->Offset_ < LOG_FILE_SIZE + 4096)
        {
            char c = '\0';

            /* write Str_EndOfLog at log file fillup */
            sc_write(logfile->fd_, Str_EndOfLog, sizeof(Str_EndOfLog));
            sc_lseek(logfile->fd_, LOG_FILE_SIZE + 4096 - 1, SEEK_SET);
            sc_write(logfile->fd_, &c, 1);
        }

        sc_close(logfile->fd_);

        /* log file을 처음 만든 경우, 2번까지 만들어서 공간 확보를 한다. */
        /* LOGFILE_RESERVING_NUM 만큼 더 만드는 것으로 변경. */
        if (logfile->File_No_ == 1 && logfile->Offset_ == 0)
        {
            char c = '\0';

            for (file_no = 2; file_no <= LOGFILE_RESERVING_NUM; file_no++)
            {
                sc_sprintf(LogFile_Name, "%s" DIR_DELIM "%s_logfile%d",
                        Log_Mgr->log_path, DB_Name, file_no);

                fd = sc_open(LogFile_Name,
                        O_RDWR | O_CREAT | O_BINARY, OPEN_MODE);

                sc_write(fd, Str_EndOfLog, sizeof(Str_EndOfLog));

                sc_lseek(fd, LOG_FILE_SIZE + 4096 - 1, SEEK_SET);
                sc_write(fd, &c, 1);

                sc_close(fd);
            }

        }
    }
    else
    {
        /* file이 없어 새로 생성됨 */
        logfile->Offset_ = 0;
    }


    logfile->fd_ = -1;

    logfile->status_ = CLOSE;

    /* log file을 scan 하고 나서 offset을 결정함 */
    logfile->Offset_ = -1;

    /* 현재 log file 뒤에 reserved된 log file이 더 있는지를 확인함
     * 하나 더 있는지 점검. 나중에 reserved 갯수는 설정하도록 해야 될 듯 */
    Log_Mgr->reserved_File_No_ = logfile->File_No_;

    for (file_no = 1; file_no <= LOGFILE_RESERVING_NUM; file_no++)
    {
        sc_sprintf(LogFile_Name, "%s" DIR_DELIM "%s_logfile%d",
                Log_Mgr->log_path, DB_Name, logfile->File_No_ + file_no);

        fd = sc_open(LogFile_Name, O_RDONLY | O_BINARY, OPEN_MODE);
        if (fd >= 0)
        {
            Log_Mgr->reserved_File_No_ = logfile->File_No_ + file_no;
            sc_close(fd);
        }
    }

    return logfile;
}

/*****************************************************************************/
//! LogFile_Final 
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
LogFile_Final(void)
{
    if (Log_Mgr->File_.status_ == OPEN)
    {
        LogFile_CloseFile();
    }
}

/*****************************************************************************/
//! LogFile_OpenFile 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param file_len :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
LogFile_OpenFile(int *file_len)
{
    register int fd;

    if (Log_Mgr->File_.status_ == OPEN)
    {
        fd = Log_Mgr->File_.fd_;
        goto check_size;
    }

    fd = sc_open(Log_Mgr->File_.File_Name_, O_WRONLY | O_BINARY, OPEN_MODE);

    if (fd == -1)
    {
        MDB_SYSLOG((" LogFile_OpenFile() - FAIL open\n"));

        return fd;
    }

  check_size:
    *file_len = sc_lseek(fd, 0, SEEK_END);

    if (*file_len < 0)
    {
        int file_errno = sc_file_errno();

        sc_close(fd);
        fd = -1;
        Log_Mgr->File_.status_ = CLOSE;
        Log_Mgr->File_.fd_ = -1;
        MDB_SYSLOG(("FAIL: log file seek %d\n", file_errno));
        return -1;
    }
    else
        Log_Mgr->File_.status_ = OPEN;

    Log_Mgr->File_.fd_ = fd;

    sc_lseek(fd, Log_Mgr->File_.Offset_, SEEK_SET);

    return fd;
}

/*****************************************************************************/
//! LogFile_CloseFile 
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
static int
LogFile_CloseFile(void)
{
    if (Log_Mgr->File_.fd_ != -1)
    {
        sc_close(Log_Mgr->File_.fd_);
        Log_Mgr->File_.fd_ = -1;
    }

    Log_Mgr->File_.status_ = CLOSE;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LogFile_AppendLog 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param log_ptr :
 * \param log_size : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
#define LOGFILE_WRITE_RETRY_COUNT 3
int
LogFile_AppendLog(char *log_ptr, int log_size, int f_sync)
{
    int write_byte = 0;
    int num_retry = 0;
    int file_len;
    int f_eol_appended = 0;
    char backup_Str_EndOfLog[sizeof(Str_EndOfLog)];

    if (sc_memcmp(log_ptr, Str_EndOfLog, sizeof(Str_EndOfLog)) == 0)
    {
        sc_assert(0, __FILE__, __LINE__);
    }

    if (log_size < 0)
    {
        MDB_SYSLOG(("Log flush data size is negative value (%d).\n",
                        log_size));
        return DB_E_INVALIDLOGDATASIZE;
    }

    if (log_size == 0)
    {
        return DB_SUCCESS;
    }

    if (LogPage > log_ptr ||
            log_ptr + log_size > LogPage + DB_Params.log_buffer_size)
    {
        MDB_SYSLOG(("LogFile_AppendLog: LogPage %x, log_pr %x, log_size %d "
                        "log buffer size %d\n",
                        LogPage, log_ptr, log_size,
                        DB_Params.log_buffer_size));
    }

#ifdef MDB_DEBUG
    sc_assert(LogPage <= log_ptr, __FILE__, __LINE__);
    sc_assert(log_ptr + log_size <= LogPage + DB_Params.log_buffer_size,
            __FILE__, __LINE__);
#endif

#ifdef MDB_DEBUG
    {
        struct LogHeader *logheader = (struct LogHeader *) log_ptr;
        int num;
        char *pNum1, *pNum2;

        pNum1 = (char *) &num;
        pNum2 = (char *) &logheader->record_lsn_.File_No_;
        pNum1[0] = pNum2[0];
        pNum1[1] = pNum2[1];
        pNum1[2] = pNum2[2];
        pNum1[3] = pNum2[3];
        if (num != Log_Mgr->File_.File_No_)
        {
            MDB_SYSLOG(
                    ("LogFile_AppendLog: num %d, file no: %d, logsize: %d\n",
                            num, Log_Mgr->File_.File_No_, log_size));
            sc_assert(num == Log_Mgr->File_.File_No_, __FILE__, __LINE__);
        }

        pNum1 = (char *) &num;
        pNum2 = (char *) &logheader->record_lsn_.Offset_;
        pNum1[0] = pNum2[0];
        pNum1[1] = pNum2[1];
        pNum1[2] = pNum2[2];
        pNum1[3] = pNum2[3];
        if (num != Log_Mgr->File_.Offset_)
        {
            MDB_SYSLOG(
                    ("LogFile_AppendLog: num %d, file offset: %d, logsize: %d\n",
                            num, Log_Mgr->File_.Offset_, log_size));
            sc_assert(num == Log_Mgr->File_.Offset_, __FILE__, __LINE__);
        }
    }
#endif

    if (log_ptr + log_size + sizeof(Str_EndOfLog) <
            LogPage + Log_Mgr->Buffer_.buf_size)
    {
        sc_memcpy(backup_Str_EndOfLog, log_ptr + log_size,
                sizeof(Str_EndOfLog));
        sc_memcpy(log_ptr + log_size, Str_EndOfLog, sizeof(Str_EndOfLog));
        log_size += sizeof(Str_EndOfLog);
        f_eol_appended = 1;
    }

  write_retry:
    if (++num_retry > LOGFILE_WRITE_RETRY_COUNT)
    {
        if (f_eol_appended == 1)
        {
            sc_memcpy(log_ptr + log_size - sizeof(Str_EndOfLog),
                    backup_Str_EndOfLog, sizeof(Str_EndOfLog));
            log_size -= sizeof(Str_EndOfLog);
        }
        Log_Mgr->File_.Offset_ += log_size;
        return DB_E_LOGFILEWRITE;
    }

    if (LogFile_OpenFile(&file_len) == -1)
    {
        MDB_SYSLOG((" File Open Error in log flush: %s\n",
                        Log_Mgr->File_.File_Name_));
        LogFile_CloseFile();
        goto write_retry;
    }

    if (file_len > (LOG_FILE_SIZE + 8 * 1024))
    {
        MDB_SYSLOG(("LogFile_AppendLog(%d): file_len %d\n", __LINE__,
                        file_len));
    }

    /* depence code */
    write_byte =        /* temporary used for the result of lseek */
            sc_lseek(Log_Mgr->File_.fd_, Log_Mgr->File_.Offset_, SEEK_SET);
    if (write_byte != Log_Mgr->File_.Offset_)
    {
        LogFile_CloseFile();
        goto write_retry;
    }

    write_byte = sc_write(Log_Mgr->File_.fd_, log_ptr, log_size);
    if (write_byte != log_size)
    {
        int err = sc_file_errno();

        if (num_retry + 1 == LOGFILE_WRITE_RETRY_COUNT)
        {
            __SYSLOG(" Log Buffer append Log File Write Error: %s, %d %d\n",
                    Log_Mgr->File_.File_Name_, Log_Mgr->File_.fd_, err);
            __SYSLOG("write byte = %d, log_size = %d \n", write_byte,
                    log_size);
        }

        LogFile_CloseFile();
        goto write_retry;
    }

    if (f_eol_appended == 1)
    {
        sc_memcpy(log_ptr + log_size - sizeof(Str_EndOfLog),
                backup_Str_EndOfLog, sizeof(Str_EndOfLog));
        log_size -= sizeof(Str_EndOfLog);
    }
    else    /* f_eol_appended == 0 */
    {
        sc_write(Log_Mgr->File_.fd_, Str_EndOfLog, sizeof(Str_EndOfLog));
    }

#ifdef MDB_DEBUG
    file_len = sc_lseek(Log_Mgr->File_.fd_, 0, SEEK_END);
    if (file_len > (LOG_FILE_SIZE + 8 * 1024))
    {
        __SYSLOG("LogFile_AppendLog(%d): file_len %d\n", __LINE__, file_len);
    }
#endif

    if (f_sync)
    {
        sc_fsync(Log_Mgr->File_.fd_);
    }

    Log_Mgr->File_.Offset_ += log_size;

    return DB_SUCCESS;
}

void
LogFile_SyncFile(void)
{
    if (Log_Mgr->File_.status_ == CLOSE)
        return;

    sc_fsync(Log_Mgr->File_.fd_);

    LogFile_CloseFile();

    return;
}

/*****************************************************************************/
//! LogFile_Create 
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
LogFile_Create(void)
{
    char LogFile_Name[MDB_FILE_NAME];

    /* 이전 log file이 열려져 있는지 점검 */
    LogFile_CloseFile();

    sc_sprintf(LogFile_Name, "%s" DIR_DELIM "%s_logfile%d",
            Log_Mgr->log_path, server->db_name_,
            (int) Log_Mgr->File_.File_No_ + 1);
    sc_strncpy(Log_Mgr->File_.File_Name_, LogFile_Name, MDB_FILE_NAME);

    Log_Mgr->File_.fd_ = sc_open(Log_Mgr->File_.File_Name_,
            CREAT_FLAG, OPEN_MODE);

    if (Log_Mgr->File_.fd_ == -1)
    {
        MDB_SYSLOG((" Log File create Fail\n"));
        return DB_E_LOGFILECREATE;
    }

    /* 새로 만들어진 경우 file 영역을 확보함 */
    if (sc_lseek(Log_Mgr->File_.fd_, 0, SEEK_END) < LOG_FILE_SIZE + 4096)
    {
        char c = '\0';

        sc_lseek(Log_Mgr->File_.fd_, LOG_FILE_SIZE + 4096 - 1, SEEK_SET);
        sc_write(Log_Mgr->File_.fd_, &c, 1);
    }

    sc_close(Log_Mgr->File_.fd_);

    Log_Mgr->File_.fd_ = -1;

    Log_Mgr->File_.status_ = CLOSE;

    if (Log_Mgr->File_.File_No_ + 1 > Log_Mgr->reserved_File_No_)
        Log_Mgr->reserved_File_No_ = Log_Mgr->File_.File_No_ + 1;

    Log_Mgr->Anchor_.Current_Log_File_No_ = Log_Mgr->File_.File_No_ + 1;
    LogAnchor_WriteToDisk(&Log_Mgr->Anchor_);

    Log_Mgr->File_.Offset_ = 0;
    Log_Mgr->File_.File_No_ += 1;

    return DB_SUCCESS;

}

/*****************************************************************************/
//! LogFile_Remove 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param from_fileno :
 * \param to_fileno : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void
LogFile_Remove(int from_fileno, int to_fileno)
{
    int i, mem_fileno = -1;
    char Del_LogFile_Name[MDB_FILE_NAME];
    char Del_LogFile_Name_Next[MDB_FILE_NAME];

    if (from_fileno > to_fileno)
    {
        MDB_SYSLOG(("WARNING: LogFile Delete is strange.. (%d - %d)\n",
                        from_fileno, to_fileno));
        return;
    }

    if (to_fileno <= Log_Mgr->Anchor_.Last_Deleted_Log_File_No_)
    {
        return;
    }

    if (from_fileno == 0)
        from_fileno++;

    for (i = from_fileno; i <= to_fileno; i++)
    {
        sc_sprintf(Del_LogFile_Name, "%s" DIR_DELIM "%s_logfile%d",
                Log_Mgr->log_path, server->db_name_, i);

        /* reserved file 번호가 현재 사용 중인 file 번호보다 작다면
         * 같이 맞춤 */
        if (Log_Mgr->reserved_File_No_ < Log_Mgr->File_.File_No_)
            Log_Mgr->reserved_File_No_ = Log_Mgr->File_.File_No_;
        /* 지워지는 파일에 대해서 1개까지 reserving을 함.
         * 총 2개 log file이 생김. */
        /* reserving 갯수를 LOGFILE_RESERVING_NUM으로 대체 */
        if (Log_Mgr->reserved_File_No_
                < Log_Mgr->File_.File_No_ + LOGFILE_RESERVING_NUM - 1)
        {
            sc_sprintf(Del_LogFile_Name_Next, "%s" DIR_DELIM "%s_logfile%d",
                    Log_Mgr->log_path, server->db_name_,
                    Log_Mgr->reserved_File_No_ + 1);
            if (sc_rename(Del_LogFile_Name, Del_LogFile_Name_Next) < 0)
            {   /* FAIL */
                /* rename이 안된 경우 log file 제거 */
                goto do_unlink;
            }
            else
            {   /* SUCCES */
                Log_Mgr->reserved_File_No_++;
                continue;
            }
        }
        else
        {
          do_unlink:
            if (sc_unlink(Del_LogFile_Name) < 0)
            {
                if (mem_fileno == -1)
                    mem_fileno = i;
                continue;
            }
        }
    }       /* for */

    if (mem_fileno == -1)
    {
        __SYSLOG("\tDelete DiskLogFile(%d ~ %d), MemLogFile(none)\n",
                from_fileno, to_fileno);
    }
    else if (mem_fileno == from_fileno)
    {
        __SYSLOG("\tDelete DiskLogFile(none), MemLogFile(%d ~ %d)\n",
                from_fileno, to_fileno);
    }
    else
    {
        __SYSLOG("\tDelete DiskLogFile(%d ~ %d), MemLogFile(%d ~ %d)\n",
                from_fileno, mem_fileno - 1, mem_fileno, to_fileno);
    }

    Log_Mgr->Anchor_.Last_Deleted_Log_File_No_ = to_fileno;
    LogAnchor_WriteToDisk(&Log_Mgr->Anchor_);
}

/*****************************************************************************/
//! LogFile_ReadLogRecord 
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
LogFile_ReadLogRecord(struct LSN *lsn, struct LogRec *logrec)
{
    int offset = 0;
    long size = 0;
    int retry_count = 0;

  retry:
    if (Log_Mgr->File_.status_ == OPEN)
    {
        LogFile_CloseFile();
    }

    if (Log_Mgr->File_.status_ == CLOSE)
    {
        Log_Mgr->File_.fd_ = sc_open(Log_Mgr->File_.File_Name_,
                O_RDONLY | O_BINARY, OPEN_MODE);
        if (Log_Mgr->File_.fd_ == -1)
        {
            MDB_SYSLOG((" File Open Error in read log: %s\n",
                            Log_Mgr->File_.File_Name_));
            return DB_E_LOGFILEOPEN;
        }
        Log_Mgr->File_.status_ = OPEN;
    }

    /* file 크기 점검 */
    offset = sc_lseek(Log_Mgr->File_.fd_, 0, SEEK_END);
    if (offset <= lsn->Offset_)
    {
        MDB_SYSLOG(("strange log file size - %d, %d, retry %d\n",
                        offset, lsn->Offset_, retry_count));
        LogFile_CloseFile();
        retry_count++;
        if (retry_count == 1)
            goto retry;
        return DB_E_LOGFILESEEK;
    }

    offset = sc_lseek(Log_Mgr->File_.fd_, lsn->Offset_, SEEK_SET);

    if (offset != lsn->Offset_)
    {
        LogFile_CloseFile();
        MDB_SYSLOG(("log file read -  Fail lseek\n"));
        return DB_E_LOGFILESEEK;
    }

    size = sc_read(Log_Mgr->File_.fd_, &logrec->header_, LOG_HDR_SIZE);
    if (size != LOG_HDR_SIZE)
    {
        LogFile_CloseFile();
        MDB_SYSLOG(("Log file: %s, lsn offset:%d\n",
                        Log_Mgr->File_.File_Name_, (int) lsn->Offset_));
        MDB_SYSLOG(("log file read -  Fail Log Head read\n"));

        fRecovered = ALL_REBUILD;

        return DB_E_LOGFILEREAD;
    }

    if (offset != logrec->header_.record_lsn_.Offset_ ||
            lsn->File_No_ != logrec->header_.record_lsn_.File_No_)
    {
        LogFile_CloseFile();
        MDB_SYSLOG(("log record offset mismatch %d - (%d,%d) %s:%d\n",
                        offset,
                        logrec->header_.record_lsn_.File_No_,
                        logrec->header_.record_lsn_.Offset_,
                        __FILE__, __LINE__));
        return DB_E_LOGOFFSETMISMATCH;
    }

    if (IS_CRASHED_LOGRECORD(&logrec->header_))
    {
        LogFile_CloseFile();
        MDB_SYSLOG(("log record crashed (%d,%d) - %s:%d\n",
                        lsn->File_No_, lsn->Offset_, __FILE__, __LINE__));
        return DB_E_CRASHLOGRECORD;
    }

    size = sc_read(Log_Mgr->File_.fd_, logrec->data1,
            logrec->header_.lh_length_);

    if (size != logrec->header_.lh_length_)
    {
        LogFile_CloseFile();
        MDB_SYSLOG(("log file read -  Fail Log Body read\n"));

        return DB_E_LOGFILEREAD;
    }

    LogFile_CloseFile();

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LogFile_ReadFromBackup 
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
LogFile_ReadFromBackup(struct LSN *lsn, struct LogRec *logrec)
{
    int fd = -1;
    char LogFile_Name[MDB_FILE_NAME];
    int file_no = 0;
    long size = 0;
    int offset = 0;
    int retry_count = 0;

    file_no = lsn->File_No_;

    sc_sprintf(LogFile_Name, "%s" DIR_DELIM "%s_logfile%d",
            Log_Mgr->log_path, server->db_name_, file_no);

  retry:
    fd = sc_open(LogFile_Name, O_RDONLY | O_BINARY, OPEN_MODE);

    if (fd == -1)
    {
        MDB_SYSLOG(("File Open Error in read log from backup:%s\n",
                        LogFile_Name));

        return DB_E_LOGFILEOPEN;
    }

    /* file 크기 점검 */
    offset = sc_lseek(fd, 0, SEEK_END);
    if (offset <= lsn->Offset_)
    {
        sc_close(fd);
        MDB_SYSLOG(("strange log file size from backup - %d, %d, retry %d\n",
                        offset, lsn->Offset_, retry_count));
        retry_count++;
        if (retry_count == 1)
            goto retry;
        return DB_E_LOGFILESEEK;
    }

    offset = sc_lseek(fd, lsn->Offset_, SEEK_SET);

    if (offset != lsn->Offset_)
    {
        sc_close(fd);
        MDB_SYSLOG(("log file read -  Fail lseek\n"));
        return DB_E_LOGFILESEEK;
    }

    size = sc_read(fd, &logrec->header_, LOG_HDR_SIZE);

    if (size != LOG_HDR_SIZE)
    {
        sc_close(fd);
        MDB_SYSLOG(("backup log file read -  Fail Log Head read\n"));

        fRecovered = ALL_REBUILD;

        return DB_E_LOGFILEREAD;
    }

    if (offset != logrec->header_.record_lsn_.Offset_ ||
            file_no != logrec->header_.record_lsn_.File_No_)
    {
        sc_close(fd);
        MDB_SYSLOG(("log record offset mismatch %d - (%d,%d) %s:%d\n",
                        offset,
                        logrec->header_.record_lsn_.File_No_,
                        logrec->header_.record_lsn_.Offset_,
                        __FILE__, __LINE__));
        return DB_E_LOGOFFSETMISMATCH;
    }

    if (IS_CRASHED_LOGRECORD(&logrec->header_))
    {
        sc_close(fd);
        MDB_SYSLOG(("log record crashed (%d,%d) - %s:%d\n",
                        lsn->File_No_, lsn->Offset_, __FILE__, __LINE__));
        return DB_E_CRASHLOGRECORD;
    }

    size = sc_read(fd, logrec->data1, logrec->header_.lh_length_);

    if (size != logrec->header_.lh_length_)
    {
        sc_close(fd);
        MDB_SYSLOG(("%s\n", "log file read -  Fail Log Body read"));
        return DB_E_LOGFILEREAD;
    }

    sc_close(fd);

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LogFile_ReadLogBlock 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param fileno :
 * \param offset : 
 * \param buf :
 * \param length :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/

/*****************************************************************************/
//! LogFile_Redo 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param lsn :
 * \param end : 
 * \param Current_LSN :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LogFile_Redo(struct LSN *lsn, int end, struct LSN *Current_LSN, int mode)
{
    int fd = -1;
    int size = 0;
    char LogFile_Name[MDB_FILE_NAME];
    struct LogRec redo_logrec;
    int File_Offset = 0, seek_file = 0;

    redo_logrec.data1 = (char *) mdb_malloc(16 * 1024);
    if (redo_logrec.data1 == NULL)
        return DB_E_OUTOFMEMORY;
    sc_memset(redo_logrec.data1, 0, 16 * 1024);
    redo_logrec.data2 = NULL;

    while (lsn->File_No_ <= end)
    {
        Current_LSN->File_No_ = lsn->File_No_;
        Current_LSN->Offset_ = lsn->Offset_;

        sc_sprintf(LogFile_Name, "%s" DIR_DELIM "%s_logfile%d",
                Log_Mgr->log_path, server->db_name_, (int) lsn->File_No_);

        fd = sc_open(LogFile_Name, O_RDONLY | O_BINARY, OPEN_MODE);

        if (fd < 0)
        {
            MDB_SYSLOG(("Redo FAIL -- LogFile Open FAIL, %s\n", LogFile_Name));
            SMEM_FREENUL(redo_logrec.data1);
            return DB_E_LOGFILEOPEN;
        }

        /* File_Offset은 화일 크기를 가짐 */
        File_Offset = sc_lseek(fd, 0, SEEK_END);

        /* log 화일 크기가 0인 경우 */
        if (File_Offset == 0 || File_Offset <= lsn->Offset_)
        {
            sc_close(fd);
            lsn->File_No_ += 1;
            lsn->Offset_ = 0;
            continue;
        }

        seek_file = sc_lseek(fd, lsn->Offset_, SEEK_SET);

        /* 가지고 있는 offset보다 log 화일이 작은 경우 */
        if (seek_file != lsn->Offset_)
        {
            sc_close(fd);
            MDB_SYSLOG(("%s\n", LogFile_Name));
            MDB_SYSLOG(("LogFile Redo - Fail lseek\n"));

            SMEM_FREENUL(redo_logrec.data1);
            return DB_E_LOGFILESEEK;
        }


        while (lsn->Offset_ < File_Offset)
        {
            size = sc_read(fd, &redo_logrec.header_, LOG_HDR_SIZE);

            /* Str_EndOfLog를 읽은 경우 log file의 끝으로 인식 */
            if (sc_memcmp(&redo_logrec.header_, Str_EndOfLog,
                            sizeof(Str_EndOfLog)) == 0)
            {
                if (mode == 0)
                {
                    /* log file을 scan한 마지막 lsn인 currlsn의 file이
                     * 현재 log file과 같다면 currlsn의 offset을 file의
                     * offset으로 설정한다. */
                    if (Log_Mgr->File_.File_No_ == lsn->File_No_)
                        Log_Mgr->File_.Offset_ = lsn->Offset_;
                }

                break;
            }

            if (size != LOG_HDR_SIZE)
            {
                sc_close(fd);
                MDB_SYSLOG(("#%s#", LogFile_Name));
                MDB_SYSLOG(
                        ("Log File Redo - Log Record Header Reading FAIL\n"));

                SMEM_FREENUL(redo_logrec.data1);
                return DB_E_LOGFILEREAD;
            }

            if (lsn->Offset_ != redo_logrec.header_.record_lsn_.Offset_ ||
                    lsn->File_No_ != redo_logrec.header_.record_lsn_.File_No_)
            {
                sc_close(fd);
                SMEM_FREENUL(redo_logrec.data1);
                MDB_SYSLOG(("log record offset mismatch %d - (%d,%d) %s:%d\n",
                                lsn->Offset_,
                                redo_logrec.header_.record_lsn_.File_No_,
                                redo_logrec.header_.record_lsn_.Offset_,
                                __FILE__, __LINE__));
                return DB_E_LOGOFFSETMISMATCH;
            }

            if (IS_CRASHED_LOGRECORD(&redo_logrec.header_))
            {
                sc_close(fd);
                SMEM_FREENUL(redo_logrec.data1);
                MDB_SYSLOG(("log record crashed (%d,%d) - %s:%d\n",
                                lsn->File_No_, lsn->Offset_, __FILE__,
                                __LINE__));
                return DB_E_CRASHLOGRECORD;
            }


            if (redo_logrec.header_.lh_length_ != 0)
            {
                char *check_ptr;

                size = sc_read(fd, redo_logrec.data1,
                        redo_logrec.header_.lh_length_);

                if (size != redo_logrec.header_.lh_length_)
                {
                    sc_close(fd);
                    MDB_SYSLOG(("%s\n", LogFile_Name));
                    MDB_SYSLOG(("Log File Redo - Log Record Body Read FAIL, "
                                    "%d, %d\n", redo_logrec.header_.lh_length_,
                                    size));
                    MDB_SYSLOG(("LOGFILE_CHECK: File_Offset = %d\n",
                                    File_Offset));
                    SMEM_FREENUL(redo_logrec.data1);
                    return DB_E_LOGFILEREAD;
                }
                check_ptr = redo_logrec.data1 + redo_logrec.header_.lh_length_
                        - 4;
                if (check_ptr[0] != 0xE || check_ptr[1] != 0xD ||
                        check_ptr[2] != 0xE || check_ptr[3] != 0xD)
                {
                    sc_close(fd);
                    SMEM_FREENUL(redo_logrec.data1);
                    MDB_SYSLOG(("log record crashed (%d,%d) - %s:%d\n",
                                    redo_logrec.header_.record_lsn_.File_No_,
                                    redo_logrec.header_.record_lsn_.Offset_,
                                    __FILE__, __LINE__));
                    return DB_E_CRASHLOGRECORD;
                }

                if (mdb_CompareChecksum(redo_logrec.data1,
                                redo_logrec.header_.lh_length_ - 8,
                                LOG_CHKSUM_OFFSET, check_ptr - 4))
                {
                    sc_close(fd);
                    SMEM_FREENUL(redo_logrec.data1);
                    MDB_SYSLOG(("log record crashed checksum fail (%d,%d) - "
                                    "%s:%d\n",
                                    redo_logrec.header_.record_lsn_.File_No_,
                                    redo_logrec.header_.record_lsn_.Offset_,
                                    __FILE__, __LINE__));
                    return DB_E_LOGCHECKSUM;
                }
            }

            if (mode)
            {
                LogRec_Redo(&redo_logrec);
                if (redo_logrec.header_.type_ == TRANS_COMMIT_LOG)
                {
                    LogMgr_Remove_Commit(redo_logrec.header_.trans_id_);
                }
            }
            else
            {

                if (redo_logrec.header_.type_ == TRANS_COMMIT_LOG)
                {
                    LogMgr_Insert_Commit(redo_logrec.header_.trans_id_);
                }
                else if (redo_logrec.header_.type_ == BEGIN_CHKPT_LOG)
                {
                    f_checkpoint_finished = CHKPT_BEGIN;
                }
                else if (redo_logrec.header_.type_ == INDEX_CHKPT_LOG)
                {
                    f_checkpoint_finished = CHKPT_INDEX;
                }
                else if (redo_logrec.header_.type_ == END_CHKPT_LOG)
                {
                    /* do not admit the last complete checkpoint,
                     * if the LSN is not same that LSN in log anchor */
                    if (redo_logrec.header_.record_lsn_.File_No_ ==
                            Log_Mgr->Anchor_.End_Chkpt_lsn_.File_No_
                            &&
                            redo_logrec.header_.record_lsn_.Offset_ ==
                            Log_Mgr->Anchor_.End_Chkpt_lsn_.Offset_)
                    {
                        f_checkpoint_finished = CHKPT_CMPLT;
                    }
                    else
                    {
                        f_checkpoint_finished = CHKPT_BEGIN;
                    }
                }
                else
                {
                    if (f_checkpoint_finished)
                    {
                        f_checkpoint_finished = CHKPT_TAILS;
                    }
                }
            }

            lsn->Offset_ += redo_logrec.header_.lh_length_ + LOG_HDR_SIZE;

            Current_LSN->Offset_ = lsn->Offset_;
        }

        sc_close(fd);

        if (mode == 0 && lsn->File_No_ == end)
        {
            if (Log_Mgr->File_.Offset_ == -1)
                Log_Mgr->File_.Offset_ = File_Offset;
        }

        lsn->File_No_ += 1;
        lsn->Offset_ = 0;
    }

    SMEM_FREENUL(redo_logrec.data1);

    return DB_SUCCESS;
}
