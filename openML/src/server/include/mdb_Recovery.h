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

#ifndef RECOVERY_H
#define RECOVERY_H

#include "mdb_config.h"

#define KB                  1024

#define    LOG_PAGE_SIZE    (32*KB)
#define LOG_FILE_SIZE       (server->shparams.log_file_size)

//########################################################################
//  Log Type
//########################################################################
#define PHYSICAL_LOG                    1
#define BEFORE_LOG                      2
#define AFTER_LOG                       3
#define INSERT_LOG                      4
#define DELETE_LOG                      5
#define UPDATE_LOG                      6
#define UPDATEFIELD_LOG                 7
#define COMPENSATION_LOG                8

#define TRANS_BEGIN_LOG                 10
#define TRANS_COMMIT_LOG                11
#define TRANS_ABORT_LOG                 12

#define BEGIN_CHKPT_LOG                 20
#define INDEX_CHKPT_LOG                 21
#define END_CHKPT_LOG                   22

#define PAGEALLOC_LOG                   30
#define PAGEDEALLOC_LOG                 31
#define SEGMENTALLOC_LOG                32

/* nologging table의 undo를 위하여 추가 */
#define INSERT_UNDO_LOG                 50
#define DELETE_UNDO_LOG                 51
#define UPDATE_UNDO_LOG                 52

#define CLR                             0
#define NO_CLR                          1

#define INSERT_HEAPREC                  100
#define REMOVE_HEAPREC                  101
#define UPDATE_HEAPREC                  102
#define INSERT_VARDATA                  103
#define REMOVE_VARDATA                  104
#define UPDATE_HEAPFLD                  105

#define TEMPRELATION_CREATE             106
#define TEMPRELATION_DROP               107
#define TEMPINDEX_CREATE                108
#define TEMPINDEX_DROP                  109
#define RELATION_CREATE                 110
#define RELATION_DROP                   111
#define INDEX_CREATE                    112
#define INDEX_DROP                      113
#define RELATION                        114
#define INDEX                           115
#define RELATION_CATALOG                116
#define INDEX_CATALOG                   117
#define RELATION_UPDATE                 118
#define AUTOINCREMENT                   119

#define SYS_ANCHOR                      120
#define SYS_MEMBASE                     121
#define SYS_PAGELINK                    122
#define SYS_SLOTLINK                    123
#define SYS_SLOTUSE_BIT                 124
#define SYS_PAGEINIT                    125

#define RELATION_CATALOG_PAGEALLOC      130
#define INDEX_CATALOG_PAGEALLOC         131
#define RELATION_PAGEALLOC              132

#define RELATION_CATALOG_PAGEDEALLOC    133
#define INDEX_CATALOG_PAGEDEALLOC       134
#define RELATION_PAGEDEALLOC            135
#define RELATION_PAGETRUNCATE           136

#define RELATION_MSYNCSLOT              138

int _Checkpointing(int slow, int force);

/* check how the last checkpoint is finished */
extern int f_checkpoint_finished;

#define CHKPT_BEGIN 0   /* begin chkpnt 만 존재. end chkpnt 없음. */
#define CHKPT_INDEX 1   /* begin~index chkpnt로 종료. 이후 다른 log 없음. */
#define CHKPT_CMPLT 2   /* begin~index~end chkpnt로 종료. 이후 다른 log 없음. */
#define CHKPT_TAILS 3   /* begin~index~end chkpnt로 종료. 이후 다른 log 있음. */

#endif
