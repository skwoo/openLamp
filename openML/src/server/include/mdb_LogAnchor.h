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

#ifndef LOGANCHOR_H
#define LOGANCHOR_H

#include "mdb_config.h"
#include "mdb_LSN.h"
#include "mdb_Recovery.h"
#include "mdb_inner_define.h"

struct LogAnchor
{
    int check1;
    int Current_Log_File_No_;   /* 현재 로그 파일로 사용되는 로그 파일명 지시자 */
    int Last_Deleted_Log_File_No_;
    int check2;
    char Anchor_File_Name[MDB_FILE_NAME];       /* Anchor 정보를 저장하는 파일명 */
    unsigned int DB_File_Idx_;  /* 최근 완료된 체크포인트의 백업 데이터베이스 지시자 */
    struct LSN Begin_Chkpt_lsn_;        /* 체크포인트 시작시의 LSN */
    int check3;
    struct LSN End_Chkpt_lsn_;  /* 체크포인트 완료시의 LSN */
};

#define LOGANCHOR_CHECKVALUE (0x59595959)

#define IS_CRASHED_LOGANCHOR(lar)                       \
    ((((lar)->check1 & (lar)->check2 & (lar)->check3)   \
      != LOGANCHOR_CHECKVALUE) ? 1 : 0)

struct LogAnchor *LogAnchor_Init(struct LogAnchor *, char *);
int LogAnchor_WriteToDisk(struct LogAnchor *anchor);
void LogAnchor_Dump(struct LogAnchor *anchor, char *caller);

#endif
