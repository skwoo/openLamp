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
#include "mdb_LogAnchor.h"
#include "mdb_Server.h"
#include "mdb_syslog.h"

int LogAnchor_WriteToDisk(struct LogAnchor *anchor);
void LogFile_Remove(int from_fileno, int to_fileno);

/*****************************************************************************/
//! LogAnchor_Init 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param anchor :
 * \param Db_Name : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 * loganchor file 초기화.
 * file이 있으면 내용을 읽어 anchor에 저장하고,
 * 없으면 새로 초기화해서 만들어줌 
 *****************************************************************************/
struct LogAnchor *
LogAnchor_Init(struct LogAnchor *anchor, char *Db_Name)
{
    char Anchor_Name[MDB_FILE_NAME];
    int length = 0;
    int size = 0;
    int fd = -1;

    length = sc_strlen(Db_Name);

    if (length > MDB_FILE_NAME)
    {
        MDB_SYSLOG(("log anchor file name is too long\n"));
        return NULL;
    }

    if (anchor == NULL)
    {
        MDB_SYSLOG((" Anchor Memory Allocate FAIL\n"));
        return NULL;
    }

    sc_strncpy(anchor->Anchor_File_Name, Db_Name, length + 1);

    sc_sprintf(Anchor_Name, "%s" DIR_DELIM "%s_loganchor",
            Log_Mgr->log_path, Db_Name);

    /* O_CREAT | O_EXCL이 포함되어 있다면 file이 존재하면 fail 처리됨 */
    fd = sc_open(Anchor_Name, O_RDWR | O_CREAT | O_EXCL | O_BINARY | O_SYNC,
            OPEN_MODE);
    if (fd == -1)
    {       // LogAnchor File EXIST..
        fd = sc_open(Anchor_Name, O_RDWR | O_SYNC | O_BINARY, OPEN_MODE);
        if (fd == -1)
        {
            MDB_SYSLOG((" Anchor File(%s) Open Function Fail\n", Anchor_Name));
            return NULL;
        }

        size = sc_read(fd, anchor, sizeof(struct LogAnchor));
        if (size != sizeof(struct LogAnchor))
        {
            MDB_SYSLOG((" LogAnchor Read FAIL \n"));
            MDB_SYSLOG(("Remake Loganchor!!!\n"));
            goto remake_loganchor;
        }

        sc_close(fd);

        if (DB_Params.db_start_recovery == 0)
            goto remake_loganchor;

        if (IS_CRASHED_LOGANCHOR(anchor))
        {
            MDB_SYSLOG(("Log Anchor Init: log anchor crashed %s:%d\n",
                            __FILE__, __LINE__));
            MDB_SYSLOG(("Remake Loganchor!!!\n"));
            goto remake_loganchor;
        }
        anchor->Current_Log_File_No_ = anchor->Current_Log_File_No_;
        anchor->DB_File_Idx_ = anchor->DB_File_Idx_;
        anchor->Begin_Chkpt_lsn_.File_No_ = anchor->Begin_Chkpt_lsn_.File_No_;
        anchor->Begin_Chkpt_lsn_.Offset_ = anchor->Begin_Chkpt_lsn_.Offset_;
        anchor->End_Chkpt_lsn_.File_No_ = anchor->End_Chkpt_lsn_.File_No_;
        anchor->End_Chkpt_lsn_.Offset_ = anchor->End_Chkpt_lsn_.Offset_;

    }
    else
    {
        // LogAnchor File Not EXIST..
        sc_close(fd);

      remake_loganchor:
        anchor->Current_Log_File_No_ = 1;
        anchor->Last_Deleted_Log_File_No_ = 0;
        anchor->DB_File_Idx_ = 0;
        anchor->Begin_Chkpt_lsn_.File_No_ = -1;
        anchor->Begin_Chkpt_lsn_.Offset_ = -1;
        anchor->End_Chkpt_lsn_.File_No_ = -1;
        anchor->End_Chkpt_lsn_.Offset_ = -1;
        anchor->check1 = anchor->check2 = anchor->check3
                = LOGANCHOR_CHECKVALUE;

        if (LogAnchor_WriteToDisk(anchor) < 0)
        {
            MDB_SYSLOG(("log anchor flush FAIL\n"));
            return NULL;
        }
    }

    return anchor;
}

/*****************************************************************************/
//! LogAnchor_WriteToDisk 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param anchor :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LogAnchor_WriteToDisk(struct LogAnchor *anchor)
{
    char Anchor_Name[MDB_FILE_NAME];
    int fd = -1;
    int size = 0;

    struct LogAnchor anchor_towrite;

    if (IS_CRASHED_LOGANCHOR(anchor))
    {
        MDB_SYSLOG(("Log anchor crash: %s:%d\n", __FILE__, __LINE__));
        return DB_E_CRASHLOGANCHOR;
    }

    if (sc_memcmp(anchor, &server->anchor_bk, sizeof(struct LogAnchor)) == 0)
    {
        return DB_SUCCESS;
    }

    server->anchor_bk = *anchor;

    anchor_towrite = *anchor;
    anchor_towrite.Current_Log_File_No_ = anchor->Current_Log_File_No_;
    anchor_towrite.DB_File_Idx_ = anchor->DB_File_Idx_;
    anchor_towrite.Begin_Chkpt_lsn_.File_No_ =
            anchor->Begin_Chkpt_lsn_.File_No_;
    anchor_towrite.Begin_Chkpt_lsn_.Offset_ = anchor->Begin_Chkpt_lsn_.Offset_;
    anchor_towrite.End_Chkpt_lsn_.File_No_ = anchor->End_Chkpt_lsn_.File_No_;
    anchor_towrite.End_Chkpt_lsn_.Offset_ = anchor->End_Chkpt_lsn_.Offset_;

    sc_sprintf(Anchor_Name, "%s" DIR_DELIM "%s_loganchor",
            Log_Mgr->log_path, server->db_name_);

    fd = sc_open(Anchor_Name, O_RDWR | O_BINARY, OPEN_MODE);
    if (fd == -1)
    {
        MDB_SYSLOG(("log anchor file name: %s\n", Anchor_Name));
        MDB_SYSLOG(("log anchor open error\n"));
        return DB_E_ANCHORFILEOPEN;
    }

    sc_lseek(fd, 0, SEEK_SET);

    size = sc_write(fd, &anchor_towrite, sizeof(struct LogAnchor));
    if (size != sizeof(struct LogAnchor))
    {
        sc_close(fd);
        __SYSLOG("log anchor write FAIL\n");
        return DB_E_ANCHORFILEWRITE;
    }

    /* DO_FSYNC */
    sc_fsync(fd);
    sc_close(fd);

    return DB_SUCCESS;
}
