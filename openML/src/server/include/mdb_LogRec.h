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

#ifndef LOGREC_H
#define LOGREC_H

#include "mdb_config.h"
#include "mdb_Recovery.h"

#include "mdb_typedef.h"
#include "mdb_LSN.h"

struct LogHeader
{
    short op_type_;
    short type_;                /* 로그 레코드 종류 */
    int tableid_;
    OID relation_pointer_;
    int check1;
    int collect_index_;
    int trans_id_;              /* 생성한 트랜잭션 번호 */
    OID oid_;                   /* 실제 데이터가 저장된 논리 주소 */
    int check2;
    int recovery_leng_;
    int lh_length_;             /* 로그레코드 크기 */
    struct LSN record_lsn_;
    /* 현재 로그 레코드의 LSN, redo시 사용 */
    struct LSN trans_prev_lsn_; /* 이전 로그 레코드의 LSN, undo시 사용 */
};

struct LogRec
{
    struct LogHeader header_;   /* 로그레코드 정보 */
    char *data1;                /* 로그 레코드 내용 */
    char *data2;
    char *data3;
};

#define LOGREC_MAGIC_NUM 0x59595959

#define LOG_HDR_SIZE  sizeof(struct LogHeader)
#define LOG_PADDING_SIZE    8
#define LOG_CHKSUM_OFFSET   128

#define IS_CRASHED_LOGRECORD(hdr)                       \
    (((hdr)->check1 != LOGREC_MAGIC_NUM                 \
      || (hdr)->check2 != LOGREC_MAGIC_NUM              \
      || (hdr)->lh_length_ < 0                          \
      || (hdr)->lh_length_ > S_PAGE_SIZE*2) ? 1 : 0)

extern __DECL_PREFIX const char Str_EndOfLog[16];

struct SlotUsedLogData          /* Log Data For Slot Used */
{
    int collect_new_item_count;
    int page_new_record_count;
    OID page_new_free_slot_id;
    PageID page_cur_free_page_next;
    int before_slot_type;
    int after_slot_type;
};

struct SlotFreeLogData          /* Log Data For Slot Free */
{
    int collect_new_item_count;
    int page_new_record_count;
    OID page_cur_free_slot_id;
    PageID page_new_free_page_next;
    int before_slot_type;
    int after_slot_type;
};

#define SLOTUSEDLOGDATA_SIZE    sizeof(struct SlotUsedLogData)
#define SLOTFREELOGDATA_SIZE    sizeof(struct SlotFreeLogData)

struct PageallocateLogData
{
    OID tail_id;
    OID next_id;
    int free_count;
    int page_count;
    OID collect_free_page_list;
};

struct AfterLog
{
    struct LogRec logrec;
};

struct BeforeLog
{
    struct LogRec logrec;
};

struct PhysicalLog
{
    struct LogRec logrec;
};

struct TransBeginLog
{
    struct LogRec logrec;
};

struct TransCommitLog
{
    struct LogRec logrec;
};

struct TransAbortLog
{
    struct LogRec logrec;
};

struct MemAnchorLog
{
    struct LogRec logrec;
};

struct BeginChkptLog
{
    struct LogRec logrec;
};

struct IndexChkptLog
{
    struct LogRec logrec;
};

struct EndChkptLog
{
    struct LogRec logrec;
};

void LogRec_Init(struct LogRec *logrec);

struct UpdateFieldInfo
{
    DB_UINT16 offset;
    DB_UINT16 length;
};

int LogRec_Undo_Trans(struct LogRec *logrec, int flag);
int LogRec_Undo(struct LogRec *logrec, int flag);

#endif /* LOGREC_H */
