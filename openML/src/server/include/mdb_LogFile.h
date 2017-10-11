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

#ifndef LOGFILE_H
#define LOGFILE_H

#include "mdb_config.h"
#include "mdb_inner_define.h"

#define CLOSE   0
#define OPEN    1

#define LOGFILE_SYNC_FORCE    1
#define LOGFILE_SYNC_SKIP     0

struct mem_log_file
{
    struct mem_log_file *next;
    int fileno;
    int offset;
    char space[1];
};

struct mem_log_file_fixed_
{
    struct mem_log_file *next;
    int fileno;
    int offset;
};

#define MEMLOGFILE_FIXED_SIZE sizeof(struct mem_log_file_fixed_)

typedef struct mem_log_file T_MEMLOGFILE;

struct LogFile
{
    char File_Name_[MDB_FILE_NAME];     /* 현재 로그 파일의 이름 */
    int File_No_;               /* 로그 파일에 대한 인덱스 번호 */
    int Offset_;                /* 로그 버퍼 내용을 저장할 위치 */
    int fd_;
    int status_;                /* OPEN or CLOSE */
    int max_backup_cnt;
    int cur_backup_cnt;
    T_MEMLOGFILE *active;
    T_MEMLOGFILE *backup;
};

int LogFile_Create(void);
struct LogFile *LogFile_Init(char *, struct LogFile *, struct LogAnchor *);
extern void LogFile_Remove(int from_fileno, int to_fileno);
int LogFile_Create(void);
int LogFile_ReadFromBackup(struct LSN *lsn, struct LogRec *);
int LogFile_ReadLogRecord(struct LSN *lsn, struct LogRec *logrec);
extern void LogFile_Final(void);
int LogFile_Redo(struct LSN *lsn, int end, struct LSN *Current_LSN, int mode);

#endif
