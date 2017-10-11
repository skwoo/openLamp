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

#ifndef __MEM_MGR_H
#define __MEM_MGR_H

#include "mdb_config.h"
#include "mdb_MemBase.h"
#include "mdb_Segment.h"
#include "mdb_DBFile.h"
#include "mdb_ppthread.h"
#include "mdb_PageList.h"
#include "mdb_Server.h"

#define CONTCAT_OFFSET (1*KB)
#define INDEXCAT_OFFSET (2*KB)

#define DIV2(n) ((n) >> 1)
#define MOD2(n) ((n) & 0x01)
#define MUL2(n) ((n) << 1)

#define DIV4(n) ((n) >> 2)
#define MOD4(n) ((n) & 0x03)
#define MUL4(n) ((n) << 2)

#define DIV8(n) ((n) >> 3)
#define MOD8(n) ((n) & 0x07)
#define MUL8(n) ((n) << 3)

struct LRU_Link
{
    volatile int next;
    volatile int prev;
    DB_UINT32 LRU_no;
};

#define BIT_SEGMENT_NO  ((SEGMENT_NO-1)/8 + 1)
#define BIT_PAGE_NO     ((SEGMENT_NO*PAGE_PER_SEGMENT - 1)/8 + 1)

#define TEMPSEG_BASE_NO ((SEGMENT_NO/3) / 8 * 8 + 1)
#define TEMPSEG_END_NO  ((SEGMENT_NO*2/3) / 8 * 8 - 1)

#define TEMP_BIT_PAGE_NO (((TEMPSEG_END_NO-TEMPSEG_BASE_NO+1)*PAGE_PER_SEGMENT - 1)/8 + 1)

struct MemoryMgr
{
    unsigned long segment_memid[SEGMENT_NO];    // shm 방식에서 위치
    unsigned char bufSegment[BIT_SEGMENT_NO];   // bit per a segment for container
#if defined(MDB_DEBUG)
    int numSetSegments;
#endif
#ifndef _ONE_BDB_
    unsigned char last_backupdb[BIT_SEGMENT_NO];        // 마지막으로 write된 backup DB
#endif
    struct LRU_Link LRU_Link[SEGMENT_NO];
    int LRU_Head;
    int LRU_Tail;
    int LRU_Count;
    struct DB_File database_file_;
#ifdef _ONE_BDB_
    unsigned char dirty_flag_[1][BIT_PAGE_NO];
#else
    unsigned char dirty_flag_[2][BIT_PAGE_NO];
#endif
    unsigned char freedPage[BIT_PAGE_NO];
    unsigned char tpage_state_[TEMP_BIT_PAGE_NO];
};

extern __DECL_PREFIX struct MemBase *mem_anchor_;       // page(0,0)
extern struct Segment *segment_[SEGMENT_NO];

#define isegment_    segment_
extern DB_UINT32 LRU_no;

#define IsNormalShutdown() \
    (mem_anchor_ && \
     f_checkpoint_finished == CHKPT_CMPLT && \
     mem_anchor_->anchor_lsn.File_No_ \
     == Log_Mgr->Anchor_.Begin_Chkpt_lsn_.File_No_ && \
     mem_anchor_->anchor_lsn.Offset_ \
     == Log_Mgr->Anchor_.Begin_Chkpt_lsn_.Offset_)

extern void __SetPageDirty(int sn, int pn, int f_dirty);

#define SetPageDirty(sn, pn) __SetPageDirty(sn, pn, 1)
#define SetTempPageDirty(sn, pn) __SetPageDirty(sn, pn, 0)

#ifdef _ONE_BDB_
#define SetPageClean(sn, pn) { \
    int page_pos = (sn) * PAGE_PER_SEGMENT + (pn); \
    Mem_mgr->dirty_flag_[0][DIV8(page_pos)] &= ~(0x80U >> (MOD8(page_pos))); \
}
#else
#define SetPageClean(sn, pn) { \
    int page_pos = (sn) * PAGE_PER_SEGMENT + (pn); \
    Mem_mgr->dirty_flag_[0][DIV8(page_pos)] &= ~(0x80U >> (MOD8(page_pos))); \
    Mem_mgr->dirty_flag_[1][DIV8(page_pos)] &= ~(0x80U >> (MOD8(page_pos))); \
}
#endif

#define SetTempPageUsed(sn, pn) { \
    int page_pos = ((sn)-TEMPSEG_BASE_NO) * PAGE_PER_SEGMENT + (pn);        \
    Mem_mgr->tpage_state_[DIV8(page_pos)] |= (0x80U >> (MOD8(page_pos)));   \
}

#define SetTempPageFree(sn, pn) { \
    int page_pos = ((sn)-TEMPSEG_BASE_NO) * PAGE_PER_SEGMENT + (pn);        \
    Mem_mgr->tpage_state_[DIV8(page_pos)] &= ~(0x80U >> (MOD8(page_pos)));  \
}

#ifdef _ONE_BDB_
#define GetLastBackupDB(sn) (0)
#define SetLastBackupDB(sn, no) {;}
#define SetLastBackupDBALL(dbFile_idx) {;}
#else
#define GetLastBackupDB(sn) \
    ((Mem_mgr->last_backupdb[DIV8(sn)] & (0x80U >> (MOD8(sn)))) ? 1 : 0)
#define SetLastBackupDB(sn, no) {                                   \
    if (no == 1)                                                    \
        Mem_mgr->last_backupdb[DIV8(sn)] |= (0x80U >> (MOD8(sn)));  \
    else                                                            \
        Mem_mgr->last_backupdb[DIV8(sn)] &= ~(0x80U >> (MOD8(sn))); \
}
#define SetLastBackupDBALL(dbFile_idx) {                            \
    if (dbFile_idx == 0)                                            \
        sc_memset(Mem_mgr->last_backupdb, 0, sizeof(Mem_mgr->last_backupdb)); \
    else                                                            \
        sc_memset(Mem_mgr->last_backupdb, 0xFF, sizeof(Mem_mgr->last_backupdb)); \
}
#endif

#define isBufSegment(sn)    ((Mem_mgr->bufSegment[DIV8(sn)]) & (0x80U >> (MOD8(sn))))

#define SetBufSegment(issys, oid)                                       \
    do {                                                                \
        int sn = getSegmentNo(oid);                                     \
        if (!isBufSegment(sn)) {                                        \
            issys = sn;                                                 \
            ((Mem_mgr->bufSegment[DIV8(sn)]) |= (0x80U >> (MOD8(sn)))); \
        }                                                               \
        else issys = 0;                                                 \
    } while (0)

#define UnsetBufSegment(issys)                                          \
    do {                                                                \
        if (issys) {                                                    \
            ((Mem_mgr->bufSegment[DIV8(issys)]) &= ~(0x80U >> (MOD8(issys)))); \
            issys = 0;                                                  \
        }                                                               \
    } while (0)

#define SetBufSegmentSN(issys, sn)                                      \
    do {                                                                \
        if (!isBufSegment(sn)) {                                        \
            issys = sn;                                                 \
            ((Mem_mgr->bufSegment[DIV8(sn)]) |= (0x80U >> (MOD8(sn)))); \
        }                                                               \
        else issys = 0;                                                 \
    } while (0)

extern int count_SetBufIndexSegment;

#ifdef INDEX_BUFFERING
#define isBufIndexSegment(sn) \
    ((Mem_mgr->bufSegment[DIV8(sn)]) & (0x80U >> (MOD8(sn))))

extern int __SetBufIndexSegment(OID oid);
extern int __UnsetBufIndexSegment(int issys);

#define SetBufIndexSegment(issys, sn)   issys = __SetBufIndexSegment(sn)
#define UnsetBufIndexSegment(issys)     issys = __UnsetBufIndexSegment(issys)
#else
#define isBufIndexSegment(sn)
#define SetBufIndexSegment(issys, oid)
#define UnsetBufIndexSegment(issys)
#endif

#define SetPageFreed(oid) {                                                 \
    int page_pos = getSegmentNo(oid) * PAGE_PER_SEGMENT + getPageNo(oid);   \
    Mem_mgr->freedPage[DIV8(page_pos)] |= (0x80U >> (MOD8(page_pos)));      \
}

#define UnsetPageFreed(oid) {                                               \
    int page_pos = getSegmentNo(oid) * PAGE_PER_SEGMENT + getPageNo(oid);   \
    Mem_mgr->freedPage[DIV8(page_pos)] &= ~(0x80U >> (MOD8(page_pos)));     \
}

#define isFreedPage(oid) (Mem_mgr->freedPage[DIV8(getSegmentNo(oid) * PAGE_PER_SEGMENT + getPageNo(oid))] & (0x80U >> (MOD8(getSegmentNo(oid) * PAGE_PER_SEGMENT + getPageNo(oid)))))

extern __DECL_PREFIX struct MemoryMgr *Mem_mgr;

#if defined(SEGMENT_NO_65536)
#define PAGE_BIT        0xF     /* 15 */
#define OFFSET_BIT      0xD     /* 13 */
#else
#define PAGE_BIT        0x10    /* 16 */
#define OFFSET_BIT      0xE     /* 14 */
#endif

#ifdef _64BIT
#define SEGMENT_MASK    0xFFFFFFFFFFFF0000UL
#define DB_PAGE_MASK    0x000000000000C000UL
#define OFFSET_MASK     0x0000000000003FFFUL
#else
#if defined(SEGMENT_NO_65536)
#define SEGMENT_MASK    0x7FFF8000UL    // 65536 segment
#define DB_PAGE_MASK    0x00006000UL    // 4 page
#define OFFSET_MASK     0x00001FFFUL    // 8KB page
#else
#define SEGMENT_MASK    0x7FFF0000UL
#define DB_PAGE_MASK    0x0000C000UL
#define OFFSET_MASK     0x00003FFFUL
#endif
#endif

#define getSegmentNo(pid)   (((OID)(pid) & SEGMENT_MASK) >> PAGE_BIT)
#define getPageNo(pid)      (((OID)(pid) & DB_PAGE_MASK) >> OFFSET_BIT)
#define OID_GetOffSet(id)   ((OID)(id) & OFFSET_MASK)

#define OID_Cal(sn, pn, offset) \
    (((OID)(sn) << PAGE_BIT) | ((OID)(pn) << OFFSET_BIT) | (offset))

#define PageID_Set(sn, pn)  \
    (((OID)(sn) << PAGE_BIT) | ((OID)(pn) << OFFSET_BIT))

#ifdef MDB_DEBUG
char *_OidToPtr(DB_INT32 sn, DB_INT32 pn, DB_INT32 offset,
        char *file, int line);
#else
char *_OidToPtr(DB_INT32 sn, DB_INT32 pn, DB_INT32 offset);
#endif

#ifdef MDB_DEBUG
#define OidToPtr(sn, pn, offset)    \
    _OidToPtr((int)(sn),(int)(pn),(int)offset,__FILE__,__LINE__)
#else
#define OidToPtr(sn, pn, offset) _OidToPtr((int)(sn),(int)(pn),(int)offset)
#endif

#define PtrToPage(sn, pn)   ((struct Page *)OidToPtr((int)(sn),(int)(pn),0))

#define getPageOid(oid)     (oid & ~OFFSET_MASK)

#define PageID_Pointer(id)  PtrToPage(getSegmentNo(id), getPageNo(id))

#define OID_Point(id)       ((id) ? (char *)OidToPtr(getSegmentNo(id), getPageNo(id), OID_GetOffSet(id)) : NULL)

#define INDEX_PAGE_BIT      PAGE_BIT
#define INDEX_OFFSET_BIT    OFFSET_BIT
#if defined(SEGMENT_NO_65536)
#define INDEX_SEGMENT_MASK  (SEGMENT_MASK | 0x80000000)
#else
#define INDEX_SEGMENT_MASK  0xFFFF0000UL
#endif
#define INDEX_PAGE_MASK     DB_PAGE_MASK
#define INDEX_OFFSET_MASK   OFFSET_MASK

#define Index_getSegmentNo(pid) (((OID)(pid) & INDEX_SEGMENT_MASK) >> INDEX_PAGE_BIT)
#define Index_getPageNo(pid)    (((OID)(pid) & INDEX_PAGE_MASK) >> INDEX_OFFSET_BIT)
#define Index_OID_GetOffSet(id) ((OID)(id) & INDEX_OFFSET_MASK)

#define Index_OID_Cal(sn, pn, offset) (((OID)(sn) << INDEX_PAGE_BIT) | ((OID)(pn) << INDEX_OFFSET_BIT) | (OID)(offset))

#define Index_PageID_Set(sn, pn) (((OID)(sn) << INDEX_PAGE_BIT) | ((OID)(pn) << INDEX_OFFSET_BIT))

#define Index_PtrToPage(sn, pn) \
    ((sn) < TEMPSEG_BASE_NO ? NULL : PtrToPage(sn,pn))

#define Index_PageID_Pointer(id) (Index_getSegmentNo(id) < TEMPSEG_BASE_NO ? NULL : Index_PtrToPage(Index_getSegmentNo(id), Index_getPageNo(id)))

#define Index_OID_Point(id) ((id) ? (Index_getSegmentNo(id) < TEMPSEG_BASE_NO ? NULL : (char *)OidToPtr(Index_getSegmentNo(id), Index_getPageNo(id), Index_OID_GetOffSet(id))) : NULL)

#define IsSamePage(oid1,oid2)   \
    ((getSegmentNo(oid1)==getSegmentNo(oid2)) && \
     (getPageNo(oid1)==getPageNo(oid2)))

#define isInvalidRid(rid) \
    (getSegmentNo(rid) >= SEGMENT_NO ||  \
     getPageNo(rid) >= PAGE_PER_SEGMENT || \
     OID_GetOffSet(rid) >= S_PAGE_SIZE || \
     (getSegmentNo(rid) < TEMPSEG_BASE_NO && \
      getSegmentNo(rid) >= mem_anchor_->allocated_segment_no_) || \
     (getSegmentNo(rid) <= TEMPSEG_END_NO && \
      getSegmentNo(rid) >= \
      TEMPSEG_BASE_NO + mem_anchor_->tallocated_segment_no_) || \
     (getSegmentNo(rid) > TEMPSEG_END_NO && \
      (getSegmentNo(rid) < (SEGMENT_NO - mem_anchor_->iallocated_segment_no_))))

/* used for seg_type of MemMgr_AllocateSegment() */
#define SEGMENT_UNKN    0       /* unknown, determine with pageid and sn */
#define SEGMENT_DATA    1
#define SEGMENT_TEMP    2
#define SEGMENT_RESI    3
#define SEGMENT_INDX    4

struct MemoryMgr *MemoryMgr_init_Createdb(DB_CREATEDB * create_options);
extern __DECL_PREFIX int Init_BackupFile(char *__config_path, char *dbname,
        DB_CREATEDB * create_options);

int MemoryMgr_initFromDBFile(int db_idx);
int MemoryMgr_LoadSegment(DB_INT32 sn);
int MDB_MemoryFree(DB_BOOL server_down);
int MemMgr_AllocateSegment(PageID pageid, DB_UINT32 sn, int seg_type);
int MemMgr_Check_FreeSegment(void);
int MemMgr_DeallocateSegment(void);
int MemMgr_SetMemAnchor(struct MemBase *anchor);
int MemMgr_DeallocateAllSegments(int f_dealloc_bufseg);
void MemMgr_DumpMemAnchor(void);
int MemMgr_FlushPage(int count);
int MemMgr_SetNextPageID(PageID curr, PageID prev);
int MemMgr_SetPrevPageID(PageID curr, PageID prev);
int MemMgr_SetPageLink(PageID curr, struct PageLink link);

int LRU_MoveToHead(int sn);
int LRU_AppendSegment(int sn);
int LRU_RemoveSegment(int sn);

int Index_MemoryMgr_init(DB_UINT32 isegment_no);
int Index_MemoryMgr_initFromDBFile(void);
int Index_MemoryMgr_FreeSegment(void);

int Index_MemMgr_AllocateSegment(PageID pageid);

#define SetDirtyFlag(oid) SetPageDirty(getSegmentNo(oid), getPageNo(oid))

#define Index_SetDirtyFlag(oid) SetPageDirty(getSegmentNo(oid), getPageNo(oid))

int OID_LightWrite(OID curr, const char *data, DB_INT32 size);
int OID_LightWrite_msync(OID curr, const char *data, DB_INT32 size,
        struct Container *Cont, DB_UINT8 slot_type);
int OID_HeavyWrite(short op_type, const void *, OID, char *,
        DB_INT32 DataSize);

int
MemMgr_AllocatePage(struct Container *cont, struct Collection *collect,
        PageID * new_page_id);

#ifdef CHECK_INDEX_VALIDATION
extern void Index_Check_Page();
#endif

int MemMgr_FreePageList(struct Collection *collect, struct Container *Cont,
        int f_pagelist_attach);

typedef enum
{
    CURR_OP_NONE = 0,
    CURR_OP_SELECT,
    CURR_OP_INSERT,
    CURR_OP_UPDATE,
    CURR_OP_DELETE,
    CURR_OP_DUMMY = MDB_INT_MAX
} CURR_OP_E;

extern CURR_OP_E _g_curr_opeation;
#endif
