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

#ifndef  LOGBUFFER_H
#define  LOGBUFFER_H

#include "mdb_config.h"
#include "mdb_Trans.h"
#include "mdb_Recovery.h"
#include "mdb_LogRec.h"

struct LogBuffer
{
    volatile int head;          /* the start position in the log buffer */
    volatile int tail;          /* the end position in the log buffer */
    volatile int last;          /* the last position in the log buffer */
    volatile int send;          /* can be used when REP sync mode is turned on */
    volatile int flush;         /* flush position when repmode = 2 */
    volatile int create;
    volatile int flush_needed;
    volatile int free_limit;
    volatile int buf_size;      /* log buffer size */
};

struct LogBuffer *LogBuffer_Init(struct LogBuffer *logbuffer);
int LogBuffer_reflect_replogs(struct LogBuffer *logbuffer,
        struct Trans *trans);
int LogBuffer_Flush(int f_sync);
int LogBuffer_WriteLogRecord(struct LogBuffer *, struct Trans *,
        struct LogRec *);
int LogBuffer_ReadLogRecord(struct LSN *lsn, struct LogRec *);
extern void LogBuffer_Final(void);

#endif
