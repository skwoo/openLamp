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
#include "mdb_ContCat.h"
#include "mdb_IndexCat.h"
#include "mdb_Mem_Mgr.h"
#include "mdb_PageList.h"
#include "mdb_Recovery.h"
#include "mdb_Collect.h"
#include "mdb_syslog.h"
#include "mdb_define.h"
#include "mdb_Mem_Mgr.h"
#include "../../version_mmdb.h"

#if PAGE_PER_SEGMENT == 2
#define _BITS_PAGE  0xC0
#elif PAGE_PER_SEGMENT == 4
#define _BITS_PAGE  0xF0
#elif PAGE_PER_SEGMENT == 8
#define _BITS_PAGE  0xFF
#else
#error define _BITS_PAGE!
#endif

_PRIVATE PageID null_pid = NULL_PID;
struct Segment *segment_[SEGMENT_NO];
__DECL_PREFIX struct MemBase *mem_anchor_;      // page(0,0)

CURR_OP_E _g_curr_opeation = CURR_OP_NONE;

void CursorTable_MemFree(void);
int OID_AnchorHeavyWrite(OID oid, int collect_index,
        int table_id, PageID page_id, PageID tail_id, PageID next_id,
        int free_count, int page_count, OID col_free_next, int alloc_flag);
int OID_SegmentAllocLog(OID pageid, DB_UINT32 segment_id);
void CheckPoint_SegmentAppend(void);
void CheckPoint_Segment(DB_UINT32 sn);

_PRIVATE int Check_DB_Compatible(void);
DB_UINT32 LRU_no = 1;

///////////////////////////////////////////////////////////////
//
// Function Name :
//
///////////////////////////////////////////////////////////////

/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
extern MDB_ENDIAN_TYPE UtilEndianType(void);

struct MemoryMgr *
MemoryMgr_init_Createdb(DB_CREATEDB * create_options)
{
    DB_UINT32 sn;
    DB_UINT32 pn;
    struct Segment *segment;
    struct PageHeader *header;

    DB_UINT32 segment_no = create_options->num_seg;

    Mem_mgr = (struct MemoryMgr *) mdb_malloc(sizeof(struct MemoryMgr));

    sc_memset(Mem_mgr, 0, sizeof(struct MemoryMgr));

    if (Mem_mgr == NULL)
        return NULL;

    for (sn = 0; sn < SEGMENT_NO; sn++)
    {
        segment_[sn] = NULL;
        Mem_mgr->segment_memid[sn] = 0;
    }

    if (PAGE_PER_SEGMENT == 1 && segment_no == 1)
        segment_no = 2;

    for (sn = 0; sn < segment_no; sn++)
    {
        segment = (struct Segment *)
                DBDataMem_Alloc(SEGMENT_SIZE, &Mem_mgr->segment_memid[sn]);
        if (segment == NULL)
        {
            return NULL;        /*DB_E_OUTOFDBMEMORY; */
        }
        else
        {
            segment_[sn] = segment;
        }

        sc_memset((char *) segment, 0, SEGMENT_SIZE);


        for (pn = 0; pn < PAGE_PER_SEGMENT; pn++)
        {
            header = &(segment_[sn]->page_[pn].header_);
            header->self_ = PageID_Set(sn, pn);

            header->link_.next_ = (pn == PAGE_PER_SEGMENT - 1) ?
                    ((sn == segment_no - 1) ? NULL_PID : PageID_Set(sn + 1,
                            0)) : PageID_Set(sn, pn + 1);

            header->link_.prev_ = (pn == 0) ?
                    ((sn == 0) ? NULL_PID : PageID_Set(sn - 1,
                            PAGE_PER_SEGMENT - 1)) : PageID_Set(sn, pn - 1);

            header->free_slot_id_in_page_ = NULL_PID;
            header->record_count_in_page_ = 0;
            header->cont_id_ = 0;
        }
    }

    mem_anchor_ = (struct MemBase *) &(segment_[0]->page_[0].body_[0]);
    mem_anchor_->allocated_segment_no_ = segment_no;

    if (PAGE_PER_SEGMENT == 1)
        mem_anchor_->first_free_page_id_ = PageID_Set(1, 0);
    else
        mem_anchor_->first_free_page_id_ = PageID_Set(0, 1);
    mem_anchor_->free_page_count_ = segment_no * PAGE_PER_SEGMENT - 1;

    mem_anchor_->tallocated_segment_no_ = 0;
    mem_anchor_->tfirst_free_page_id_ = NULL_PID;
    mem_anchor_->tfree_page_count_ = 0;
    mem_anchor_->iallocated_segment_no_ = 0;
    mem_anchor_->ifirst_free_page_id_ = NULL_PID;
    mem_anchor_->ifree_page_count_ = 0;

    mem_anchor_->majorVer = sc_atoi(MAJOR_VER);
    mem_anchor_->minorVer = sc_atoi(MINOR_VER);
    mem_anchor_->releaseVer = sc_atoi(SUBMINOR_VER);
    mem_anchor_->buildNO = sc_atoi(BUILD_NO);
    mem_anchor_->supported_segment_no_ = SEGMENT_NO;

    {
        MDB_ENDIAN_TYPE ProcType = UtilEndianType();

        switch (ProcType)
        {
        case MDB_ENDIAN_NO_INFORMATION:
            mem_anchor_->endian_type_ = 'N';
            break;

        case MDB_ENDIAN_LITTLE:
            mem_anchor_->endian_type_ = 'L';
            break;

        case MDB_ENDIAN_MIDDLE:
            mem_anchor_->endian_type_ = 'M';
            break;

        case MDB_ENDIAN_BIG:
            mem_anchor_->endian_type_ = 'B';
            break;

        default:
            mem_anchor_->endian_type_ = 'N';
            break;
        }
    }

    if (mem_anchor_->endian_type_ == 'N' || mem_anchor_->endian_type_ == 'M')
    {
#if defined(MDB_DEBUG)
        sc_assert(0, __FILE__, __LINE__);
#else
        return NULL;
#endif
    }

    mem_anchor_->language_code = -1234;

    /* page(0, 0)는 mem_anchor로 사용하므로 제외 */

    return Mem_mgr;
}

/*****************************************************************************/
//! Init_BackupFile
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param __config_path(in) :
 * \param dbname(in)        : 
 * \param segment_no(in)    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *****************************************************************************/
__DECL_PREFIX int
Init_BackupFile(char *__config_path, char *dbname,
        DB_CREATEDB * create_options)
{
    char db_path[MDB_FILE_NAME];
    int db_idx;
    DB_UINT32 sn;
    int ret;
    int keep_segno = 0;
    int server_status;

    ppthread_mutexattr_init(&_mutex_attr);

    DBMem_Init(0, NULL);        /* shm_mutex 초기화 */

    keep_segno = create_options->num_seg;
    create_options->num_seg = 1;

    Mem_mgr = (struct MemoryMgr *) MemoryMgr_init_Createdb(create_options);
    if (Mem_mgr == NULL)
    {
        return DB_E_MEMMGRINIT;
    }

    create_options->num_seg = keep_segno;

    if (ContCatalog_init_Createdb(CONT_ITEM_SIZE) < 0)
    {
        MDB_SYSLOG(("Cont Catalog initialize ERROR\n"));

        return DB_E_CONTCATINIT;
    }

    if (IndexCatalog_init_Createdb(INDEX_ITEM_SIZE) < 0)
    {
        MDB_SYSLOG(("Index Catalog initialize ERROR\n"));

        return DB_E_IDXCATINIT;
    }

    sc_strcpy(db_path, __config_path);

    ret = DBFile_Allocate(dbname, db_path);
    if (ret < 0)
    {
        MDB_SYSLOG(("FAIL - DBFile Allocate: %s, %s\n", dbname, db_path));
        return ret;
    }

    ret = DBFile_Create(create_options->num_seg);
    if (ret < 0)
    {
        MDB_SYSLOG(("FAIL - DBFile Create: %d\n", create_options->num_seg));
        return ret;
    }

    for (db_idx = 0; db_idx <= 1; db_idx++)
    {
#ifdef _ONE_BDB_
        if (db_idx == 1)
            continue;
#endif

        ret = DBFile_Open(db_idx, -1);
        if (ret < 0)
        {
            MDB_SYSLOG(("FAIL - DBFile Open\n"));
            return ret;
        }

        for (sn = 0; sn < (DB_UINT32) mem_anchor_->allocated_segment_no_; sn++)
        {
            segment_[sn] = DBDataMem_V2P(Mem_mgr->segment_memid[sn]);
            if (DBFile_Write(db_idx, sn, 0, (void *) &segment_[sn]->page_[0],
                            PAGE_PER_SEGMENT) < 0)
            {
                DBFile_Close(db_idx, -1);
                MDB_SYSLOG(("FAIL - DBFile Write\n"));
                MDB_SYSLOG(("    Check the file system size\n"));
                return DB_E_DBFILEWRITE;
            }
        }

        server_status = server->status;
        server->status = SERVER_STATUS_SHUTDOWN;
        ret = DBFile_Close(db_idx, -1);
        if (ret < 0)
        {
            __SYSLOG2("FAIL - DBFile Close\n");
            return ret;
        }
        server->status = server_status;
    }

    MDB_MemoryFree(FALSE);

    mdb_free(Mem_mgr);
    Mem_mgr = NULL;

    return DB_SUCCESS;
}

///////////////////////////////////////////////////////////////
//
// Function Name :
//
///////////////////////////////////////////////////////////////

/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
#include "mdb_PMEM.h"

int
MemoryMgr_initFromDBFile(int db_idx)
{
    DB_UINT32 segment_no;
    int ret;

    SetLastBackupDBALL(db_idx);

    for (segment_no = 0; segment_no < SEGMENT_NO; segment_no++)
    {
        segment_[segment_no] = NULL;
        Mem_mgr->LRU_Link[segment_no].next = -1;
        Mem_mgr->LRU_Link[segment_no].prev = -1;
        Mem_mgr->LRU_Link[segment_no].LRU_no = 0;
    }

    Mem_mgr->LRU_Head = -1;
    Mem_mgr->LRU_Tail = -1;
    Mem_mgr->LRU_Count = 0;

    LRU_no = 1;

    segment_[0] = (struct Segment *)
            DBDataMem_Alloc(SEGMENT_SIZE, &Mem_mgr->segment_memid[0]);

    if (segment_[0] == NULL)
    {
        return DB_E_OUTOFDBMEMORY;
    }

    mem_anchor_ = NULL;

    ret = DBFile_Read_OC(db_idx, 0, 0, (void *) &(segment_[0]->page_[0]),
            PAGE_PER_SEGMENT);
    if (ret < 0)
    {
        MDB_SYSLOG(("DBFile_Read_OC(%d, 0, 0, %d) FAIL: %d)\n",
                        db_idx, PAGE_PER_SEGMENT, ret));
        return ret;
    }

    mem_anchor_ = (struct MemBase *) &(segment_[0]->page_[0].body_[0]);

    /* place anchor_lsn at the end of page 0 */
    /* read & replace anchor_lsn */
    {
        struct LSN *anchor_lsn;

        anchor_lsn = (struct LSN *)
                (segment_[0]->page_[0].body_ - PAGE_HEADER_SIZE +
                S_PAGE_SIZE - sizeof(struct LSN));

        if (anchor_lsn->File_No_ == 0 && anchor_lsn->Offset_ == 0)
        {
            /* do noting for backward compatibility */
        }
        else
        {
            if (anchor_lsn->File_No_ < 0 || anchor_lsn->Offset_ < 0 ||
                    anchor_lsn->File_No_ > mem_anchor_->anchor_lsn.File_No_)
            {
                anchor_lsn->File_No_ = 0;
                anchor_lsn->Offset_ = 0;
#ifdef MDB_DEBUG
                sc_assert(0, __FILE__, __LINE__);
#endif
            }

            mem_anchor_->anchor_lsn = *anchor_lsn;
        }
    }

    __SYSLOG("\nDB Ver %d.%d.%d patch %d (%s endian)\n\n",
            mem_anchor_->majorVer,
            mem_anchor_->minorVer,
            mem_anchor_->releaseVer,
            mem_anchor_->buildNO,
            mem_anchor_->endian_type_ == 'L' ? "Little" : "Big");

    ret = Check_DB_Compatible();
    if (ret < 0)
    {
        if (ret == DB_E_ENDIANNOTCOMPATIBLE)
        {
            MDB_ENDIAN_TYPE ProcType = UtilEndianType();
            char *EndianTypeName[] =
                    { "NO Information", "Little", "Middle", "Big" };

            __SYSLOG2("\n## Compatibility Notice ##\n");

            __SYSLOG2("Current SERVER is a %s-Endian compatable Version on "
                    "%s-Endian processor.\n",
                    mem_anchor_->endian_type_ == 'B' ? "Little" : "Big",
                    EndianTypeName[ProcType]);
            __SYSLOG2("It is not compatible to %s-Endian DB.\n\n",
                    mem_anchor_->endian_type_ == 'B' ? "Big" : "Little");
            return DB_E_ENDIANNOTCOMPATIBLE;
        }
        else
        {
            __SYSLOG2("\n## Compatibility Notice ##\n");
            __SYSLOG2("Current SERVER Ver %d.%d.%d.%d is not compatible to ",
                    sc_atoi(MAJOR_VER), sc_atoi(MINOR_VER),
                    sc_atoi(SUBMINOR_VER), sc_atoi(BUILD_NO));
            __SYSLOG2("MobileLite V%d.%d.%d patch %d.\n",
                    mem_anchor_->majorVer, mem_anchor_->minorVer,
                    mem_anchor_->releaseVer, mem_anchor_->buildNO);
            __SYSLOG2("Please check the compatibility of versions.\n\n");
            return DB_E_VERNOTCOMPATIBLE;
        }
    }

    /* memory에 올라가는 segment 수가 정해져 있지 않는 상황에서는
       모든 segment를 미리 메모리에 올린다. */

    if (server->shparams.mem_segments == 0)
    {
        ret = DBFile_Open(db_idx, -1);
        if (ret < 0)
        {
            MDB_SYSLOG(("IN initFrom DBFile File Open Error 2\n"));
            return ret;
        }

        for (segment_no = 1;
                segment_no < (DB_UINT32) mem_anchor_->allocated_segment_no_;
                segment_no += 1)
        {
            segment_[segment_no] =
                    DBDataMem_V2P(Mem_mgr->segment_memid[segment_no]);

            segment_[segment_no] = (struct Segment *)
                    DBDataMem_Alloc(SEGMENT_SIZE,
                    &Mem_mgr->segment_memid[segment_no]);

            if (segment_[segment_no] == NULL)
            {
                DBFile_Close(db_idx, -1);
                /* 추가, close가 되든 안되든 FAIL 처리
                   check할 필요 없음 */
                return DB_E_OUTOFDBMEMORY;
            }

            sc_memset((char *) segment_[segment_no], 0, SEGMENT_SIZE);

            ret = DBFile_Read(db_idx, segment_no, 0,
                    (void *) &(segment_[segment_no]->page_[0]),
                    PAGE_PER_SEGMENT);
            if (ret < 0)
            {
                /* file lock 풀어줌 */
                DBFile_Close(db_idx, -1);
                MDB_SYSLOG(("FAIL: read segment %d\n", segment_no));
                return ret;
            }

            if (segment_no % 100 == 0)
            {
                MDB_SYSLOG(("Loaded to Segment %d\n", segment_no));
            }
        }   /* for */

        __SYSLOG("DB file loading... Finished\n");

        ret = DBFile_Close(db_idx, -1);
        if (ret < 0)
        {
            MDB_SYSLOG(("File Close Error\n"));

            return ret; /* file close가 안되면 나중에 문제가 생길 수
                           있으므로 fail 처리 */
        }
    }

    if (1)
    {
        DB_UINT32 sn;
        DB_UINT32 pn;
        struct PageHeader *header;

        /* initialize temp files */
        ret = DBFile_InitTemp();
        if (ret < 0)
        {
            MDB_SYSLOG(("Initialize Temp DB Files Error\n"));
            return ret;
        }

        sn = TEMPSEG_BASE_NO;

        segment_[sn] = (struct Segment *) DBDataMem_Alloc(SEGMENT_SIZE,
                &Mem_mgr->segment_memid[sn]);

        if (segment_[sn] == NULL)
        {
            return DB_E_OUTOFDBMEMORY;
        }

        sc_memset(segment_[sn], 0, SEGMENT_SIZE);

        for (pn = 0; pn < PAGE_PER_SEGMENT; pn++)
        {
            header = &(segment_[sn]->page_[pn].header_);
            header->self_ = PageID_Set(sn, pn);
            /* first temp segment flush */
            SetTempPageDirty(sn, pn);
        }
        if (server->shparams.mem_segments)
            LRU_AppendSegment(sn);
        mem_anchor_->tallocated_segment_no_ = 1;
        mem_anchor_->tfirst_free_page_id_ = PageID_Set(sn, 0);
        mem_anchor_->tfree_page_count_ = 1 * PAGE_PER_SEGMENT;
        mem_anchor_->trs_first_free_page_id_ = NULL_OID;
        mem_anchor_->trs_free_page_count_ = 0;
        mem_anchor_->trs_segment_count_ = 0;
    }

    /* applied engine verson */
    mem_anchor_->engine_majorVer = sc_atoi(MAJOR_VER);
    mem_anchor_->engine_minorVer = sc_atoi(MINOR_VER);
    mem_anchor_->engine_releaseVer = sc_atoi(SUBMINOR_VER);
    mem_anchor_->engine_buildNO = sc_atoi(BUILD_NO);

    return DB_SUCCESS;
}

/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
memmgr_ReadFromDBTempFile(DB_INT32 sn, DB_INT32 pn)
{
    DB_INT32 last_pn;
    int open_flag = 0;
    int i, j, ret;
    unsigned char dflag;
    struct PageHeader *header;
    int ftype;

    if (sn < TEMPSEG_BASE_NO)
    {
        MDB_SYSLOG(("FAIL - ReadFromDBTempFile : sn = %d\n", sn));
        return DB_FAIL;
    }

    ftype = DBFILE_TEMP_TYPE;

    if (pn == -1)
    {       /* read one segment */
        pn = 0;
        last_pn = PAGE_PER_SEGMENT - 1;
    }
    else
    {       /* read one page */
        last_pn = pn;
    }

    while (pn <= last_pn)
    {
        i = (((sn - TEMPSEG_BASE_NO) * PAGE_PER_SEGMENT) + pn) >> 3;
        j = (((sn - TEMPSEG_BASE_NO) * PAGE_PER_SEGMENT) + pn) & 0x07;

        dflag = *(unsigned char *) (&Mem_mgr->tpage_state_[i]);

        for (; j < 8; j++)
        {
            segment_[sn] = DBDataMem_V2P(Mem_mgr->segment_memid[sn]);

            if (dflag && (dflag & (0x80U >> j)))
            {
                if (!open_flag)
                {
                    ret = DBFile_Open(ftype, sn);
                    if (ret < 0)
                    {
                        return ret;
                    }
                    open_flag = 1;
                }

                ret = DBFile_Read(ftype, sn, pn,
                        (void *) &(segment_[sn]->page_[pn]), 1);
                if (ret < 0)
                {
                    DBFile_Close(ftype, sn);
                    return ret;
                }

                header = &(segment_[sn]->page_[pn].header_);
                if (sn != (int) getSegmentNo(header->self_) ||
                        pn != (int) getPageNo(header->self_))
                {
                    DBFile_Close(ftype, sn);
                    MDB_SYSLOG(("FAIL: Page(%d|%d) has damaged.\n", sn, pn));
                    return DB_FAIL;
                }
            }
            else        /* clean(free) page */
            {
                sc_memset((char *) &segment_[sn]->page_[pn], 0, S_PAGE_SIZE);
                header = &(segment_[sn]->page_[pn].header_);
                header->self_ = PageID_Set(sn, pn);
            }

            pn += 1;
            if (pn > last_pn)
                break;
        }
    }

    if (open_flag)
        DBFile_Close(ftype, sn);

    return DB_SUCCESS;
}

int
MemoryMgr_LoadSegment(DB_INT32 sn)
{
    int ret;

    /* memory에 있는 segment 수가 limit이면 tail segment를 deallocate 시킨다. */
    if (server->shparams.mem_segments &&
            Mem_mgr->LRU_Count >= server->shparams.mem_segments)
    {
        MemMgr_DeallocateSegment();
    }

    segment_[sn] = DBDataMem_V2P(Mem_mgr->segment_memid[sn]);

    if (segment_[sn] != NULL ||
            (Mem_mgr->LRU_Link[sn].next != -1 &&
                    Mem_mgr->LRU_Link[sn].prev != -1))
    {
        MDB_SYSLOG(("error loading segment %d\n", sn));
#ifdef MDB_DEBUG
        sc_assert(0, __FILE__, __LINE__);
#endif
        return DB_FAIL;
    }

    segment_[sn] = (struct Segment *)
            DBDataMem_Alloc(SEGMENT_SIZE, &Mem_mgr->segment_memid[sn]);

    if (segment_[sn] == NULL)
    {
        if (server->shparams.mem_segments)
        {   /* buffering을 하는 상태에서는 deallocate하고 나서 한 번 더 시도함. */
            MemMgr_DeallocateSegment();
            segment_[sn] = (struct Segment *)
                    DBDataMem_Alloc(SEGMENT_SIZE, &Mem_mgr->segment_memid[sn]);
        }

        if (segment_[sn] == NULL)
        {
            return DB_E_OUTOFDBMEMORY;
        }
    }

    if (sn >= TEMPSEG_BASE_NO)  /* temp segment */
    {
        ret = memmgr_ReadFromDBTempFile(sn, -1);
        if (ret < 0)
        {
            goto error;
        }
    }
    else    /* data segment */
    {
        ret = DBFile_Read_OC(GetLastBackupDB(sn), sn, 0,
                (void *) &(segment_[sn]->page_[0]), PAGE_PER_SEGMENT);
        if (ret < 0)
        {
            goto error;
        }
    }

    if (server->shparams.mem_segments)
        LRU_AppendSegment(sn);

    server->numLoadSegments += 1;

    return DB_SUCCESS;

  error:
    DBDataMem_Free(Mem_mgr->segment_memid[sn]);
    segment_[sn] = NULL;
    Mem_mgr->segment_memid[sn] = 0;
    return ret;
}

///////////////////////////////////////////////////////////////
//
// Function Name :
//
///////////////////////////////////////////////////////////////

/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
DB_INT32
MDB_MemoryFree(DB_BOOL server_down)
{
    DB_UINT32 sn;
    void dbi_DataMem_Protect_RW(void);

    if (Mem_mgr != NULL)
    {
        int db_idx;

        for (db_idx = 0; db_idx < DBFILE_TYPE_COUNT; db_idx++)
        {
#ifdef _ONE_BDB_
            if (db_idx == 1)
                continue;
#endif

            if (DBFile_Close(db_idx, -1) < 0)
            {
                MDB_SYSLOG(("Memory Free: File Close (%d, -1) FAIL\n",
                                db_idx));
            }
        }

        dbi_DataMem_Protect_RW();

        for (sn = 0; sn < SEGMENT_NO; sn++)
        {
            segment_[sn] = DBDataMem_V2P(Mem_mgr->segment_memid[sn]);

            if (segment_[sn] != NULL)
            {
                DBDataMem_Free(Mem_mgr->segment_memid[sn]);
                segment_[sn] = NULL;
                Mem_mgr->segment_memid[sn] = 0;
            }
        }
    }

    DBDataMem_All_Free();

    mem_anchor_ = NULL;
    container_catalog = NULL;
    index_catalog = NULL;

    return DB_SUCCESS;
}

//############################################################################
//
// Function Name : MemMgr_AllocateSegment
// Call By : MemMgr_AllocatePage()
//  New Segment allocate memory Function
//
//
//############################################################################

/* sn이 0이 아니면 segment 할당 redo를 할 때 */
/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
MemMgr_AllocateSegment(PageID pageid, DB_UINT32 sn, int seg_type)
{
    int is_temp = 0;
    int is_resident = 0;
    DB_UINT32 Last_Segment_No;
    DB_UINT32 pn;
    struct Page *temp_page;
    struct PageHeader *header;

#ifdef _ONE_BDB_
    int ret;
#endif

    if (seg_type == SEGMENT_TEMP)
        is_temp = 1;
    else if (seg_type == SEGMENT_RESI)
        is_resident = 1;

    if (pageid != NULL_PID && getSegmentNo(pageid) < TEMPSEG_BASE_NO)
    {
        /* allocate data segment */
        is_temp = 0;
        Last_Segment_No = mem_anchor_->allocated_segment_no_;
        if (sn == 0 && Last_Segment_No >= TEMPSEG_BASE_NO)
        {
            MDB_SYSLOG(
                    ("FAIL: Allocate Data Segment %d, due to memory limit\n",
                            Last_Segment_No));
            MemMgr_DumpMemAnchor();
            return DB_E_LIMITSIZE;
        }
    }
    else if (is_resident || is_temp ||
            (pageid != NULL_PID && getSegmentNo(pageid) <= TEMPSEG_END_NO))
    {
        /* allocate temp segment */
        is_temp = 1;
        Last_Segment_No = TEMPSEG_BASE_NO +
                mem_anchor_->tallocated_segment_no_;
        if (sn == 0 && Last_Segment_No > TEMPSEG_END_NO)
        {
            MDB_SYSLOG(("FAIL: Allocate Temp Segment %d, "
                            "due to memory limit\n", Last_Segment_No));
            MemMgr_DumpMemAnchor();
            return DB_E_LIMITSIZE;
        }
    }
    else if (pageid != NULL_PID && getSegmentNo(pageid) > TEMPSEG_END_NO)
    {
        is_temp = 0;
        Last_Segment_No = -1;
        return DB_FAIL;
    }
    else
    {
        sc_assert(0, __FILE__, __LINE__);
        Last_Segment_No = -1;
        return DB_FAIL;
    }

    /* temp volume인 경우 fixed를 체크하지 않고 허용함 */
    if (!is_temp && mem_anchor_->db_size_fixed == 1)
    {
        MDB_SYSLOG(("FAIL: Allocate Segment %d, due to db size fixed\n",
                        Last_Segment_No));
        return DB_E_FIXEDDBSIZE;
    }

    if (server->shparams.max_segments != 0 &&
            (DB_UINT32) (mem_anchor_->allocated_segment_no_ +
                    mem_anchor_->tallocated_segment_no_ +
                    mem_anchor_->iallocated_segment_no_)
            >= (DB_UINT32) server->shparams.max_segments)
    {
        MDB_SYSLOG(("FAIL: Allocate Segment %d, due to memory limit\n",
                        Last_Segment_No));
        MemMgr_DumpMemAnchor();
        return DB_E_LIMITSIZE;
    }

    if (sn != 0 && sn != Last_Segment_No)
    {
        MDB_SYSLOG(("FAIL: Allocate Segment %d, last: %d\n", sn,
                        Last_Segment_No));
        Last_Segment_No = sn;
    }

    /* memory에 있는 segment 수가 limit이면 tail segment를 deallocate 시킨다. */
    if (is_resident)
    {
        if (server->shparams.resident_table_limit &&
                mem_anchor_->trs_segment_count_ >=
                server->shparams.resident_table_limit)
        {
            MDB_SYSLOG(("FAIL: Allocate Segment %d, "
                            "due to resident memory limit\n",
                            Last_Segment_No));
            MemMgr_DumpMemAnchor();
            return DB_E_LIMITSIZE;
        }
    }
    else if (server->shparams.mem_segments &&
            Mem_mgr->LRU_Count >= server->shparams.mem_segments)
    {
        MemMgr_DeallocateSegment();
    }

    segment_[Last_Segment_No] =
            DBDataMem_V2P(Mem_mgr->segment_memid[Last_Segment_No]);

    if (Last_Segment_No >= SEGMENT_NO ||
            (sn == 0 && segment_[Last_Segment_No] != NULL))
    {
        MDB_SYSLOG(("FAIL:Allocate Segment %d, full of memory\n",
                        Last_Segment_No));
        MemMgr_DumpMemAnchor();
        return DB_E_SEGALLOCFAIL;
    }

#ifdef MDB_DEBUG
    __SYSLOG("Mem Mgr Allocate Segment pageid=0x%x, Segment_No=%d\n",
            pageid, Last_Segment_No);
#endif

    if (segment_[Last_Segment_No] == NULL)
    {
        segment_[Last_Segment_No] = (struct Segment *)
                DBDataMem_Alloc(SEGMENT_SIZE,
                &Mem_mgr->segment_memid[Last_Segment_No]);
    }

    if (segment_[Last_Segment_No] == NULL)
    {
        /* As resident tables uses a separate buffer, just return error
         * at out of memory */
        if (is_resident)
            return DB_E_OUTOFDBMEMORY;

        if (server->shparams.mem_segments)
        {   /* buffering을 하는 상태에서는 deallocate하고 나서 한 번 더 시도함. */
            MemMgr_DeallocateSegment();
            segment_[Last_Segment_No] = (struct Segment *)
                    DBDataMem_Alloc(SEGMENT_SIZE,
                    &Mem_mgr->segment_memid[Last_Segment_No]);
        }

        if (segment_[Last_Segment_No] == NULL)
        {
            return DB_E_OUTOFDBMEMORY;
        }
    }

    sc_memset((char *) segment_[Last_Segment_No], 0, SEGMENT_SIZE);

    if (is_temp)        /* allocate temp segment */
    {
        for (pn = 0; pn < PAGE_PER_SEGMENT; pn++)
        {
            header = &(segment_[Last_Segment_No]->page_[pn].header_);
            header->self_ = PageID_Set(Last_Segment_No, pn);
            if (is_resident)
            {
                if (pn < PAGE_PER_SEGMENT - 1)
                    header->link_.next_ = PageID_Set(Last_Segment_No, pn + 1);
                else    /* last page */
                    header->link_.next_ = NULL_OID;

                if (pn == 0)
                    header->link_.prev_ = NULL_OID;
                else
                    header->link_.prev_ = PageID_Set(Last_Segment_No, pn - 1);

                SetTempPageUsed(Last_Segment_No, pn);
                SetPageDirty(Last_Segment_No, pn);
            }
        }

        mem_anchor_->tallocated_segment_no_ += 1;
        mem_anchor_->tfree_page_count_ += PAGE_PER_SEGMENT;
        if (pageid == NULL_PID)
        {
            if (is_resident)
                mem_anchor_->trs_first_free_page_id_ =
                        PageID_Set(Last_Segment_No, 0);
            else
                mem_anchor_->tfirst_free_page_id_ =
                        PageID_Set(Last_Segment_No, 0);
        }
    }
    else    /* allocate data segment */
    {
#if !defined(DO_NOT_CHECK_DISKFREESPACE)
        if (DBFile_GetVolumeSize(DBFILE_DATA0_TYPE)
                < (Last_Segment_No + 1) * SEGMENT_SIZE
                && SEGMENT_SIZE * 2 > DBFile_DiskFreeSpace(DBFILE_DATA0_TYPE))
        {
            MDB_SYSLOG(("MemMgr Segment Allocate FAIL(disk full!!!)\n"));
            DBDataMem_Free(Mem_mgr->segment_memid[Last_Segment_No]);
            segment_[Last_Segment_No] = NULL;
            Mem_mgr->segment_memid[Last_Segment_No] = 0;
            return DB_E_DISKFULL;
        }
#endif
        if (sn == 0 && OID_SegmentAllocLog(pageid, Last_Segment_No) < 0)
        {
            MDB_SYSLOG(("MemMgr Segment Allocate Logging FAIL\n"));
            DBDataMem_Free(Mem_mgr->segment_memid[Last_Segment_No]);
            segment_[Last_Segment_No] = NULL;
            Mem_mgr->segment_memid[Last_Segment_No] = 0;
            return DB_E_SEGALLOCLOGFAIL;
        }

        for (pn = 0; pn < PAGE_PER_SEGMENT; pn++)
        {
            header = &(segment_[Last_Segment_No]->page_[pn].header_);
            header->self_ = PageID_Set(Last_Segment_No, pn);
            header->link_.next_ = (pn == PAGE_PER_SEGMENT - 1) ?
                    NULL_PID : PageID_Set(Last_Segment_No, pn + 1);
            header->link_.prev_ = (pn == 0) ?
                    PageID_Set(Last_Segment_No - 1, PAGE_PER_SEGMENT - 1) :
                    PageID_Set(Last_Segment_No, pn - 1);
            header->cont_id_ = 0;
#if !defined(_ONE_BDB_) /* DO_Segment_Append */
            SetPageDirty(Last_Segment_No, pn);
#endif
            SetPageFreed(header->self_);
        }

        /* _ONE_BDB_ Do it below CheckPoint_SegmentAppend() --> changed */
        /* file system에 여유 공간이 있는지를 점검하기 위해서
           segment append를 시도 */

        temp_page = (struct Page *) &(segment_[Last_Segment_No]->page_[0]);
        temp_page->header_.link_.prev_ = pageid;
        SetPageDirty(Last_Segment_No, 0);

#ifdef _ONE_BDB_        /* DO_Segment_Append */
        ret = DBFile_Write_OC(DBFILE_DATA0_TYPE, Last_Segment_No, 0,
                (void *) &(segment_[Last_Segment_No]->page_[0]),
                PAGE_PER_SEGMENT, 0);

        if (ret != DB_SUCCESS)
        {
            DBDataMem_Free(Mem_mgr->segment_memid[Last_Segment_No]);
            segment_[Last_Segment_No] = NULL;
            Mem_mgr->segment_memid[Last_Segment_No] = 0;
            MDB_SYSLOG(("MemMgr Segment Allocate Append FAIL\n"));
            return ret;
        }
#endif

        temp_page = (struct Page *) PageID_Pointer(pageid);
        if (temp_page == NULL)
        {
            DBDataMem_Free(Mem_mgr->segment_memid[Last_Segment_No]);
            segment_[Last_Segment_No] = NULL;
            Mem_mgr->segment_memid[Last_Segment_No] = 0;
            return DB_E_OIDPOINTER;
        }
        temp_page->header_.link_.next_ =
                segment_[Last_Segment_No]->page_[0].header_.self_;
        SetPageDirty(getSegmentNo(pageid), getPageNo(pageid));

        /* 이 내용은 CheckPoint_SegmentAppend()에서 disk로 기록됨 */

        if (Last_Segment_No == (DB_UINT32) mem_anchor_->allocated_segment_no_)
        {
            mem_anchor_->allocated_segment_no_ = Last_Segment_No + 1;
            mem_anchor_->free_page_count_ += PAGE_PER_SEGMENT;
        }
    }

    SetPageDirty(0, 0);

    if (server->shparams.mem_segments && seg_type != SEGMENT_RESI)
        LRU_AppendSegment(Last_Segment_No);

#ifdef MDB_DEBUG
    __SYSLOG("\tNew allocated segment: 0x%x\n", segment_[Last_Segment_No]);
    MemMgr_DumpMemAnchor();
#endif

    return DB_SUCCESS;
}

/* sn이 0이면 startup 시에 호출. 그외는 redo 시
   data segment의 free page list에 page가 없는 경우 segment를 할당해서
   연결시켜준다.
*/
/*****************************************************************************/
//! MemMgr_Check_FreeSegment 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param sn :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
MemMgr_Check_FreeSegment(void)
{
    DB_UINT32 Last_Segment_No;
    DB_UINT32 pn;
    struct Page *temp_page;
    struct PageHeader *header;

    int ret;

    if (mem_anchor_->first_free_page_id_ != NULL_OID)
        return DB_SUCCESS;

    Last_Segment_No = mem_anchor_->allocated_segment_no_;

    if (server->shparams.max_segments != 0 &&
            (DB_UINT32) (mem_anchor_->allocated_segment_no_ +
                    mem_anchor_->iallocated_segment_no_)
            >= (DB_UINT32) server->shparams.max_segments)
    {
        MDB_SYSLOG(("FAIL: free Segment %d, due to memory limit\n",
                        Last_Segment_No));
        MemMgr_DumpMemAnchor();
        return DB_E_LIMITSIZE;
    }

    /* memory에 있는 segment 수가 limit이면 tail segment를 deallocate 시킨다. */
    if (server->shparams.mem_segments &&
            Mem_mgr->LRU_Count >= server->shparams.mem_segments)
    {
        MemMgr_DeallocateSegment();
    }

    segment_[Last_Segment_No] =
            DBDataMem_V2P(Mem_mgr->segment_memid[Last_Segment_No]);

    if (Last_Segment_No >= SEGMENT_NO || segment_[Last_Segment_No] != NULL)
    {
        MDB_SYSLOG(("FAIL: free Segment %d, full of memory\n",
                        Last_Segment_No));
        MemMgr_DumpMemAnchor();
        return DB_E_SEGALLOCFAIL;
    }

#ifdef MDB_DEBUG
    __SYSLOG("Mem Mgr free Segment Segment_No=%d\n", Last_Segment_No);
#endif

    segment_[Last_Segment_No] = (struct Segment *)
            DBDataMem_Alloc(SEGMENT_SIZE,
            &Mem_mgr->segment_memid[Last_Segment_No]);

    if (segment_[Last_Segment_No] == NULL)
    {
        if (server->shparams.mem_segments)
        {   /* buffering을 하는 상태에서는 deallocate하고 나서 한 번 더 시도함. */
            MemMgr_DeallocateSegment();
            segment_[Last_Segment_No] = (struct Segment *)
                    DBDataMem_Alloc(SEGMENT_SIZE,
                    &Mem_mgr->segment_memid[Last_Segment_No]);
        }

        if (segment_[Last_Segment_No] == NULL)
        {
            return DB_E_OUTOFDBMEMORY;
        }
    }

    sc_memset((char *) segment_[Last_Segment_No], 0, SEGMENT_SIZE);

    ret = OID_SegmentAllocLog(0, Last_Segment_No);
    if (ret < 0)
    {
        MDB_SYSLOG(("MemMgr Segment Allocate Logging FAIL\n"));
        DBDataMem_Free(Mem_mgr->segment_memid[Last_Segment_No]);
        segment_[Last_Segment_No] = NULL;
        Mem_mgr->segment_memid[Last_Segment_No] = 0;

        return ret;
    }

    for (pn = 0; pn < PAGE_PER_SEGMENT; pn++)
    {
        header = &(segment_[Last_Segment_No]->page_[pn].header_);

        header->self_ = PageID_Set(Last_Segment_No, pn);
        header->link_.next_ = (pn == PAGE_PER_SEGMENT - 1) ?
                NULL_PID : PageID_Set(Last_Segment_No, pn + 1);
        header->link_.prev_ = (pn == 0) ?
                PageID_Set(Last_Segment_No - 1, PAGE_PER_SEGMENT - 1) :
                PageID_Set(Last_Segment_No, pn - 1);

        header->cont_id_ = 0;

#if !defined(_ONE_BDB_) /* DO_Segment_Append */
        SetPageDirty(Last_Segment_No, pn);
#endif
    }

    /* _ONE_BDB_ Do it below CheckPoint_SegmentAppend() --> changed */
    /* file system에 여유 공간이 있는지를 점검하기 위해서
       segment append를 시도 */

    temp_page = (struct Page *) &(segment_[Last_Segment_No]->page_[0]);
    temp_page->header_.link_.prev_ = NULL_OID;
    SetPageDirty(Last_Segment_No, 0);

#ifdef _ONE_BDB_        /* DO_Segment_Append */
    ret = DBFile_Write_OC(DBFILE_DATA0_TYPE, Last_Segment_No, 0,
            (void *) &(segment_[Last_Segment_No]->page_[0]),
            PAGE_PER_SEGMENT, 1);
    if (ret != DB_SUCCESS)
    {
        DBDataMem_Free(Mem_mgr->segment_memid[Last_Segment_No]);
        segment_[Last_Segment_No] = NULL;
        Mem_mgr->segment_memid[Last_Segment_No] = 0;
        MDB_SYSLOG(("MemMgr Segment Allocate Append FAIL\n"));

        return ret;
    }
#endif

    /* segment의 첫번째 page를 free page list에 연결 */
    mem_anchor_->first_free_page_id_ =
            segment_[Last_Segment_No]->page_[0].header_.self_;
    SetPageDirty(0, 0);


    /* 이 내용은 CheckPoint_SegmentAppend()에서 disk로 기록됨 */
    mem_anchor_->allocated_segment_no_ = Last_Segment_No + 1;
    mem_anchor_->free_page_count_ = PAGE_PER_SEGMENT;

    SetPageDirty(0, 0);

    if (server->shparams.mem_segments)
        LRU_AppendSegment(Last_Segment_No);

#ifdef MDB_DEBUG
    __SYSLOG("\tNew allocated free segment: 0x%x\n",
            segment_[Last_Segment_No]);
    MemMgr_DumpMemAnchor();
#endif

    return DB_SUCCESS;
}

/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int IS_DIRTY_SEGMENT(int sn);

int
MemMgr_DeallocateSegment(void)  // This Function need  File Append
{
    DB_INT32 sn;
    DB_INT32 delsn, first_delsn;
    DB_UINT32 delsn_LRU_no = 0;
    int is_retry = 0;
    int count = 0;

#ifdef INDEX_BUFFERING
    int count_idx_seg;
#endif
    int count_dirty;
    int count_total_dirty;
    int first_dirty_data_sn;
    int segment_limit;

    if (server->shparams.mem_segments < (1024 * 1024) / SEGMENT_SIZE)
    {
        segment_limit = 9;
    }
    else
    {
        segment_limit = 10;
    }

  retry:
#ifdef INDEX_BUFFERING
    count_idx_seg = 0;
#endif
    count_dirty = 0;
    count_total_dirty = 0;
    first_dirty_data_sn = -1;
    count = 0;
    first_delsn = -1;
    delsn = sn = Mem_mgr->LRU_Tail;
    while (1)
    {
        sn = Mem_mgr->LRU_Link[sn].prev;
        if (sn == -1)
        {
            if (first_delsn != -1)
            {
                delsn = first_delsn;
                break;
            }
            if (is_retry == 0)
            {
                is_retry = 1;
                _Checkpointing(0, 1);
                goto retry;
            }

            if (isBufSegment(delsn))
            {
                __SYSLOG("SEG BUFFER) Replace buffered segment(%d)\n", delsn);
            }
            else
            {
                __SYSLOG("SEG BUFFER) del segment(%d)\n", delsn);
            }
            if (1)
            {
                int real_count = 0;
                DB_INT32 cur_sn;

                __SYSLOG("LRU Head=%d Tail=%d Count=%d \n",
                        Mem_mgr->LRU_Head, Mem_mgr->LRU_Tail,
                        Mem_mgr->LRU_Count);

                cur_sn = Mem_mgr->LRU_Head;
                while (cur_sn != -1)
                {
                    real_count += 1;
                    if ((real_count % 5) == 0)
                    {
                        MDB_SYSLOG(("%05d(%d:%d:%d)\n", cur_sn, 0,
                                        isBufSegment(cur_sn),
                                        IS_DIRTY_SEGMENT(cur_sn)));
                    }
                    else
                    {
                        MDB_SYSLOG(("%05d(%d:%d:%d) ", cur_sn, 0,
                                        isBufSegment(cur_sn),
                                        IS_DIRTY_SEGMENT(cur_sn)));
                    }
                    cur_sn = Mem_mgr->LRU_Link[cur_sn].next;
                }
                if ((real_count % 5) != 0)
                {
                    MDB_SYSLOG(("\n"));
                }
                MDB_SYSLOG(("LRU Real Count = %d\n", real_count));
            }

#ifdef MDB_DEBUG
            __SYSLOG_syncbuf(0);
            sc_assert(0, __FILE__, __LINE__);
#endif
            break;
        }

        if (!isBufSegment(sn))
        {
            if (IS_DIRTY_SEGMENT(sn))
            {
                ++count_total_dirty;
                if (_g_curr_opeation == CURR_OP_UPDATE)
                {
                    ++count_dirty;
                }
#ifdef INDEX_BUFFERING
                if (first_delsn == -1 ||
                        count_dirty == (server->shparams.mem_segments - 10))
                {
                    /* data segment인 경우 후보 */
                    if (sn < (int) (SEGMENT_NO -
                                    mem_anchor_->iallocated_segment_no_))
                    {
                        first_delsn = sn;
                        delsn_LRU_no = 9999999; /* fix buffering; assume max */
                        if (first_dirty_data_sn == -1)
                        {
                            first_dirty_data_sn = sn;
                        }
                    }
                    else        /* index segment */
                    {
                      count_idx:
                        count_idx_seg++;

                        /* index segment 이고, 현재 index segment의 갯수가
                         * 버퍼의 1/2을 넘은 경우 checkpoint 발생시키고
                         * victim으로 선정 */

                        if (server->shparams.mem_segments &&
                                count_idx_seg >
                                server->shparams.mem_segments / 2)
                        {
                            if (first_delsn != -1)
                            {
                                delsn = first_delsn;
                            }
                            else
                            {
                                delsn = sn;
                            }
                            if (IS_DIRTY_SEGMENT(delsn))
                            {
                                if (delsn >=
                                        (int) (SEGMENT_NO -
                                                mem_anchor_->
                                                iallocated_segment_no_))
                                {
                                    _Checkpointing(0, 0);
                                }
                                else
                                {
                                    break;
                                }
                            }

                            goto remove_segment;
                        }
                    }
                }
#else
                if (first_delsn == -1 ||
                        count_dirty == (server->shparams.mem_segments - 10))
                {
                    first_delsn = sn;
                    delsn_LRU_no = 9999999;     /* fix buffering; assume max */
                }
#endif
                if (sn < (int) (SEGMENT_NO -
                                mem_anchor_->iallocated_segment_no_)
                        && count_total_dirty ==
                        (server->shparams.mem_segments - segment_limit)
                        && _g_curr_opeation != CURR_OP_INSERT)
                {
                    if (first_dirty_data_sn != -1)
                    {
                        delsn = first_dirty_data_sn;
                    }
                    else
                    {
                        delsn = sn;
                    }

                    if (count <= segment_limit)
                    {
                        if (isTemp_SEGMENT(delsn))
                        {
                            CheckPoint_Segment(delsn);
                        }

                        _Checkpointing(0, 0);
                        goto remove_segment;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            {
                count++;
#ifdef INDEX_BUFFERING
                if (sn >=
                        (int) (SEGMENT_NO -
                                mem_anchor_->iallocated_segment_no_))
                {
                    if (first_delsn == -1)
                    {
                        first_delsn = sn;
                    }
                    goto count_idx;
                }
#endif

                delsn = sn;
                if (first_delsn == -1)
                {
                    first_delsn = sn;
                    delsn_LRU_no = Mem_mgr->LRU_Link[sn].LRU_no;
                    goto remove_segment;        /* fix buffering */
                }
                else
                {
                    if (Mem_mgr->LRU_Link[sn].LRU_no < delsn_LRU_no)
                    {
                        first_delsn = sn;
                        delsn_LRU_no = Mem_mgr->LRU_Link[sn].LRU_no;
                        goto remove_segment;    /* fix buffering */
                    }

                }
            }
        }
    }

    if (IS_DIRTY_SEGMENT(delsn))
    {
        /* flush container page when the victim is a dirty page. */
        /* consider PAGE_PER_SEGMENT */
        DB_INT32 delsn_cont[PAGE_PER_SEGMENT];
        int i;

#ifdef MDB_DEBUG
        sc_assert(delsn <
                (int) (SEGMENT_NO - mem_anchor_->iallocated_segment_no_),
                __FILE__, __LINE__);
#endif
        /* In case of temp segment, first flush the segment */
        if (isTemp_SEGMENT(delsn))
        {
            CheckPoint_Segment(delsn);
        }
        /* do checkpoint when the victim is a dirty page. */
        /* flush container page when the victim is a dirty page. */
        segment_[delsn] = DBDataMem_V2P(Mem_mgr->segment_memid[delsn]);
        for (i = 0; i < PAGE_PER_SEGMENT; ++i)
        {
            delsn_cont[i] =
                    getSegmentNo(segment_[delsn]->page_[i].header_.cont_id_);
        }

        for (i = 0; i < PAGE_PER_SEGMENT; ++i)
        {
            if (delsn_cont[i] && Mem_mgr->segment_memid[delsn_cont[i]])
            {
                CheckPoint_Segment(delsn_cont[i]);
            }
        }
        CheckPoint_Segment(delsn);
    }

  remove_segment:
    LRU_RemoveSegment(delsn);

    segment_[delsn] = DBDataMem_V2P(Mem_mgr->segment_memid[delsn]);

    if (segment_[delsn] == NULL)
    {
        MDB_SYSLOG(("deallocate segment: %d NULL\n", sn));
        return DB_SUCCESS;
    }

#ifdef MDB_DEBUG
    if (isBufSegment(delsn))
        sc_assert(0, __FILE__, __LINE__);
#endif

    DBDataMem_Free(Mem_mgr->segment_memid[delsn]);

    segment_[delsn] = NULL;
    Mem_mgr->segment_memid[delsn] = 0;

    {
        int issys = delsn;

        UnsetBufSegment(issys);
    }

    return DB_SUCCESS;
}

/* 
 * segment sn이 dirty인지 판별.
 * segment내 page 수(PAGE_PER_SEGMENT)는
 * 8개 이하라고 가정하고 작성함.
 */
static int
IS_DIRTY_SEGMENT(int sn)
{
    int i, j;
    unsigned char dflag;

    i = (sn * PAGE_PER_SEGMENT) >> 3;

    dflag = Mem_mgr->dirty_flag_[0][i];
    if (dflag == 0)
        return 0;

#if PAGE_PER_SEGMENT == 8
    return dflag;
#endif

    j = (sn * PAGE_PER_SEGMENT) & 0x07;

    return dflag & (_BITS_PAGE >> j);
}

#ifdef MDB_DEBUG
int bufseg_count = 0;
#endif
int
MemMgr_DeallocateAllSegments(int f_dealloc_bufseg)
{
    extern void DBDataMem_RealFree(long v);
    DB_UINT32 delsn, delsn_next;

    // commit_flush를 할 때는 전체를 다 내려야 함.
    // 일부만 내리거나 순서가 맞지 않는 경우 free page list에 loop가
    // 발생될 가능성이 있음
    _Checkpointing(0, 0);


    delsn_next = Mem_mgr->LRU_Head;

    while (1)
    {
        if (delsn_next == -1)
            break;

        delsn = delsn_next;
        delsn_next = Mem_mgr->LRU_Link[delsn].next;

        if (Mem_mgr->LRU_Count <= server->shparams.low_mem_segments)
            break;

        if (!f_dealloc_bufseg && isBufSegment(delsn))
            continue;

        CheckPoint_Segment(delsn);

        LRU_RemoveSegment(delsn);

        segment_[delsn] = DBDataMem_V2P(Mem_mgr->segment_memid[delsn]);

        if (segment_[delsn] == NULL)
        {
            continue;
        }

        DBDataMem_Free(Mem_mgr->segment_memid[delsn]);
        segment_[delsn] = NULL;
        Mem_mgr->segment_memid[delsn] = 0;

        if (isBufSegment(delsn))
        {
            int issys = delsn;

            UnsetBufSegment(issys);
        }
    }

#ifdef MDB_DEBUG
    if (f_dealloc_bufseg && !server->shparams.low_mem_segments)
        sc_assert(Mem_mgr->LRU_Head == -1, __FILE__, __LINE__);
#endif

    return DB_SUCCESS;
}

/* Must call this function after commit.
 * return the number of flushed pages */
int
MemMgr_FlushPage(int count)
{
    DB_INT32 sn, pn;
    unsigned char dflag;
    int i, j;
    int db_idx;
    int flush_count = 0;
    int ret;

    if (segment_ == NULL || Mem_mgr->LRU_Tail == -1)
        return 0;

    if (server->gDirtyBit == 0)
        return 0;

    sn = Mem_mgr->LRU_Head;
    while (1)
    {
        if (sn == -1)
            break;

        /* index, temp segment는 제외 */
        if (sn >= (int) (SEGMENT_NO - mem_anchor_->iallocated_segment_no_) ||
                (isTemp_SEGMENT(sn)))
            goto loop;

        dflag = IS_DIRTY_SEGMENT(sn);
        if (dflag == 0)
            goto loop;

        if (sn < TEMPSEG_BASE_NO)
            db_idx = 0;

        /* sn 시작 bit 위치 */
        i = (sn * PAGE_PER_SEGMENT) >> 3;
        j = (sn * PAGE_PER_SEGMENT) & 0x07;

        for (pn = 0; pn < PAGE_PER_SEGMENT; pn++, j++)
        {
            if (dflag & (0x80U >> j))
            {
                ret = DBFile_Write_OC(db_idx, sn, pn, &segment_[sn]->page_[pn],
                        1, 0);
                if (ret == DB_SUCCESS)
                {
                    Mem_mgr->dirty_flag_[0][i] ^= (0x80U >> j);

                    flush_count++;

                    if (flush_count == count)
                        return flush_count;
                }
            }
        }

      loop:
        sn = Mem_mgr->LRU_Link[sn].next;
    }

    return flush_count;
}

///////////////////////////////////////////////////////////////
//
// Function Name : MemMgr_SetMemAnchor()
// Call By : MemMgr_AllocatePage()
//
///////////////////////////////////////////////////////////////

/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
MemMgr_SetMemAnchor(struct MemBase *anchor)
{
    PageID curr;

    curr = OID_Cal(0, 0, PAGE_HEADER_SIZE);

    OID_LightWrite(curr, (char *) anchor, sizeof(struct MemBase));

    return DB_SUCCESS;
}

void
MemMgr_DumpMemAnchor(void)
{
    __SYSLOG("\tdSeg No.:%d, dfree page id:0x%lx, dcount: %d\n",
            mem_anchor_->allocated_segment_no_,
            mem_anchor_->first_free_page_id_, mem_anchor_->free_page_count_);
    __SYSLOG("\ttSeg No.:%d, tfree page id:0x%lx, tcount: %d\n",
            mem_anchor_->tallocated_segment_no_,
            mem_anchor_->tfirst_free_page_id_, mem_anchor_->tfree_page_count_);
    __SYSLOG("\tiSeg No.:%d, ifree page id:0x%lx, icount: %d\n",
            mem_anchor_->iallocated_segment_no_,
            mem_anchor_->ifirst_free_page_id_, mem_anchor_->ifree_page_count_);
    __SYSLOG("\tanchor lsn: %d %d\n",
            mem_anchor_->anchor_lsn.File_No_, mem_anchor_->anchor_lsn.Offset_);
}

///////////////////////////////////////////////////////////////
//
// Function Name : MemMgr_AllocatePage
// Call by     : Collect_AllocateSlot(),
//
///////////////////////////////////////////////////////////////
/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static PageID
memmgr_GetNextFreeTempPageID(PageID pageid)
{
    DB_UINT32 sn, last_sn;
    DB_UINT32 pn;
    int i, j;
    unsigned char dflag;
    PageID next_pageid;
    int SEG_BASE_NO;

    SEG_BASE_NO = TEMPSEG_BASE_NO;

    sn = getSegmentNo(pageid);
    pn = getPageNo(pageid);
    if (pn < (PAGE_PER_SEGMENT - 1))
    {
        pn++;
    }
    else
    {
        sn++;
        pn = 0;
    }

#ifdef MDB_DEBUG
    sc_assert(sn >= TEMPSEG_BASE_NO, __FILE__, __LINE__);
#endif

    last_sn = SEG_BASE_NO + mem_anchor_->tallocated_segment_no_ - 1;

    next_pageid = NULL_PID;

    while (sn <= last_sn && next_pageid == NULL_PID)
    {
        i = (((sn - TEMPSEG_BASE_NO) * PAGE_PER_SEGMENT) + pn) >> 3;
        j = (((sn - TEMPSEG_BASE_NO) * PAGE_PER_SEGMENT) + pn) & 0x07;

        dflag = *(unsigned char *) (&Mem_mgr->tpage_state_[i]);
        if (dflag == 0)
        {
            next_pageid = PageID_Set(sn, pn);
            break;
        }

        for (; j < 8; j++)
        {
            if (dflag & (0x80U >> j))
            {
                pn++;
                if (pn >= PAGE_PER_SEGMENT)
                {
                    sn += 1;
                    pn = 0;
                    if (sn > last_sn)
                        break;
                }
            }
            else
            {
                next_pageid = PageID_Set(sn, pn);
                break;
            }
        }
    }

    return next_pageid;
}

/* seek 범위를 줄이기 위한 tuning:
 * seek한 마지막 segment 번호를 기록한다.
 * SetTempPageFree()가 불려서 사용되면 이값을 다시 TEMPSEG_BASE_NO으로
 * 설정한다. */
int SEEK_RESIDENT_SEG_BASE_NO = TEMPSEG_BASE_NO;

/* page가 모두 free인 segment를 찾는다 */
static int
memmgr_SeekTempSegment_Resident(void)
{
    DB_UINT32 sn, first_free_sn, last_sn;
    int i = 0, j;
    unsigned char dflag;
    PageID next_pageid;
    struct Page *page;

    sn = SEEK_RESIDENT_SEG_BASE_NO + 1;

    first_free_sn = getSegmentNo(mem_anchor_->tfirst_free_page_id_);

    last_sn = TEMPSEG_BASE_NO + mem_anchor_->tallocated_segment_no_ - 1;

    next_pageid = NULL_PID;

    while (sn <= last_sn && next_pageid == NULL_PID)
    {
        /* mem_anchor_->tfirst_free_page_id_ 는 skip
         * SetTempPageUsed()를 하지 않고 연결되어 있음 */
        if (sn == first_free_sn)
        {
            sn++;
            continue;
        }

        i = ((sn - TEMPSEG_BASE_NO) * PAGE_PER_SEGMENT) >> 3;

        dflag = *(unsigned char *) (&Mem_mgr->tpage_state_[i]);
        if (dflag == 0)
        {   /* found */
            next_pageid = PageID_Set(sn, 0);
            break;
        }

        j = ((sn - TEMPSEG_BASE_NO) * PAGE_PER_SEGMENT) & 0x07;

        for (; j < 8; j += _BITS_PAGE)
        {
            if (dflag & (_BITS_PAGE >> j))      /* B0..0 인것을 찾음 */
            {   /* 사용중. next skip */
                sn++;
                if (sn > last_sn)
                    break;
            }
            else
            {   /* found!!! */
                next_pageid = PageID_Set(sn, 0);
                break;
            }
        }
    }

    if (next_pageid)
    {
        /* trs page list에 연결 */
        mem_anchor_->trs_first_free_page_id_ = next_pageid;

        for (j = 0; j < PAGE_PER_SEGMENT; j++)
        {
            next_pageid = PageID_Set(sn, j);

            page = (struct Page *) (PageID_Pointer(next_pageid));
            if (page == NULL)
            {
                return DB_E_OIDPOINTER;
            }

            if (j < PAGE_PER_SEGMENT - 1)
                page->header_.link_.next_ = PageID_Set(sn, j + 1);
            else        /* last page */
                page->header_.link_.next_ = NULL_OID;

            if (j == 0)
                page->header_.link_.prev_ = NULL_OID;
            else
                page->header_.link_.prev_ = PageID_Set(sn, j - 1);

            SetTempPageUsed(sn, j);
            SetPageDirty(sn, j);
        }

        return DB_SUCCESS;
    }

    return DB_FAIL;
}

int
MemMgr_AllocatePage(struct Container *cont, struct Collection *collect,
        PageID * new_page_id)
{
    OID oid = cont->collect_.cont_id_;
    int is_temp = isTempTable(cont) ? 1 : 0;
    int is_resident = isResidentTable(cont);
    int table_id = cont->base_cont_.id_;
    PageID next = NULL_OID;
    PageID tail;
    struct Page *page;
    struct PageLink link;
    struct MemBase anchor;
    struct PageList *pagelist = &collect->page_link_;
    int issys = 0;
    int ret;
    int get_freepage = 0;

    if (isTemp_OID(collect->cont_id_))
    {
        is_temp = 1;
    }

    if (is_resident)
        is_temp = 1;

    if (is_resident)
    {
        *new_page_id = mem_anchor_->trs_first_free_page_id_;

        if (*new_page_id == NULL_PID)
        {
            /* seek a segment from temp free page list */
            ret = memmgr_SeekTempSegment_Resident();
            if (ret < 0)
            {
                /* allocate new temp segment */
                ret = MemMgr_AllocateSegment(NULL_PID, 0, SEGMENT_RESI);
                if (ret < 0)
                {
                    MDB_SYSLOG(("MemMgr allocate Resident Temp Segment FAIL "
                                    "%d, %d line\n", ret, __LINE__));
                    goto end;
                }
            }

            /* add the count of page & segment */
            mem_anchor_->trs_segment_count_ += 1;
            mem_anchor_->trs_free_page_count_ += 2;
            *new_page_id = mem_anchor_->trs_first_free_page_id_;
        }
    }
    else if (is_temp)   /* allocate temp page */
    {
        *new_page_id = mem_anchor_->tfirst_free_page_id_;

        if (mem_anchor_->tfree_page_count_ == 1 || *new_page_id == NULL_PID)
        {
            if (*new_page_id == NULL_PID)
            {
                MDB_SYSLOG(("NOTE: old tfirst_free_page_id_ = NULL_PID\n"));
            }

            /* allocate new temp segment */
            ret = MemMgr_AllocateSegment(*new_page_id, 0, SEGMENT_TEMP);
            if (ret < 0)
            {
                MDB_SYSLOG(("MemMory allocate Temp Segment FAIL %d, %d\n",
                                __LINE__, ret));
                goto end;
            }

            if (*new_page_id == NULL_PID)
            {
                *new_page_id = mem_anchor_->tfirst_free_page_id_;
                MDB_SYSLOG(("NOTE: new tfirst_free_page_id_ = (%d|%d)\n",
                                (int) getSegmentNo(*new_page_id),
                                (int) getPageNo(*new_page_id)));
            }
        }
    }
    else    /* allocate data page */
    {
        *new_page_id = mem_anchor_->first_free_page_id_;

        page = (struct Page *) (PageID_Pointer(*new_page_id));

        if (*new_page_id && page == NULL)
        {
            MDB_SYSLOG(("MemMgr_AllocatePage oid error 0x%x, %d\n",
                            *new_page_id, __LINE__));
            ret = DB_E_OIDPOINTER;
            goto end;
        }

        /* free_page_id의 page가 마지막 free page라면 새로운 segment를 할당해서
           연결시켜준다. */
        if (*new_page_id == NULL_OID || page->header_.link_.next_ == NULL_OID)
        {
            /* segment 할당이 성공하면 새로 할당된 segment의 page들이
               page 다음으로 linked list로 연결되어 return 됨 */
            ret = MemMgr_AllocateSegment(page->header_.self_, 0, SEGMENT_DATA);
            if (ret < 0)
            {   // 추후 함수 포함
                MDB_SYSLOG(("MemMory allocate Data Segment FAIL %d, %d\n",
                                __LINE__, ret));
                goto end;
            }
        }
    }

    page = (struct Page *) (PageID_Pointer(*new_page_id));
    if (page == NULL)
    {
        ret = DB_E_OIDPOINTER;
        goto end;
    }

    SetBufSegment(issys, *new_page_id);

    if (is_temp || is_resident)
    {
        /* initialize the page */
        Collect_InitializePage(oid, collect, *new_page_id);

        if (is_resident)
            next = page->header_.link_.next_;

        if (pagelist->tail_ != NULL_PID)
        {
            page->header_.link_.prev_ = pagelist->tail_;
            page->header_.link_.next_ = NULL_PID;
            OID_LightWrite((pagelist->tail_ + sizeof(PageID) * 2),
                    (char *) new_page_id, sizeof(PageID));
            pagelist->tail_ = *new_page_id;
        }
        else
        {
            page->header_.link_.prev_ = NULL_PID;
            page->header_.link_.next_ = NULL_PID;
            pagelist->head_ = *new_page_id;
            pagelist->tail_ = *new_page_id;
        }
    }
    else    /* allocate data page */
    {
        /* page 할당 logging */
        OID_AnchorHeavyWrite(oid, (int) collect->collect_index_,
                table_id, *new_page_id, pagelist->tail_,
                page->header_.link_.next_,
                mem_anchor_->free_page_count_ - 1,
                collect->page_count_ + 1, collect->free_page_list_, 1);

        /* initialize the page */
        Collect_InitializePage(oid, collect, *new_page_id);

        next = page->header_.link_.next_;
        if (next != NULL_PID)
        {
            /* next page의 prev를 NULL로 */
            OID_LightWrite((next + sizeof(PageID)), (char *) &null_pid,
                    sizeof(PageID));
        }

        link = page->header_.link_;
        link.prev_ = link.next_ = NULL_PID;
        tail = pagelist->tail_;
        if (tail != NULL_PID)
        {
            link.prev_ = tail;
            OID_LightWrite(((*new_page_id) + sizeof(PageID)), (char *) &link,
                    LINK_SIZE);

            OID_LightWrite((tail + sizeof(PageID) * 2), (char *) new_page_id,
                    sizeof(PageID));
            pagelist->tail_ = *new_page_id;
        }
        else
        {
            link.prev_ = NULL_PID;
            OID_LightWrite((*new_page_id + sizeof(PageID)), (char *) &link,
                    LINK_SIZE);
            pagelist->head_ = *new_page_id;
            pagelist->tail_ = *new_page_id;
        }
    }

    if (collect->free_page_list_ != NULL_OID)
        page->header_.free_page_next_ = collect->free_page_list_;

    /* Container Type on Page Header */
    page->header_.cont_type_ = cont->base_cont_.type_;

    UnsetBufSegment(issys);

    collect->free_page_list_ = *new_page_id;

    collect->page_count_++;

    SetDirtyFlag(collect->cont_id_);

    if (is_resident)
    {
        if (next == NULL_PID)
        {
            mem_anchor_->trs_free_page_count_ = 0;
            mem_anchor_->trs_first_free_page_id_ = NULL_PID;
        }
        else
        {
            mem_anchor_->trs_free_page_count_--;
            mem_anchor_->trs_first_free_page_id_ = next;
        }
    }
    else if (is_temp)
    {
        next = memmgr_GetNextFreeTempPageID(*new_page_id);
        if (next == NULL_PID)
        {
            mem_anchor_->tfirst_free_page_id_ = NULL_PID;
        }
        else
        {
            mem_anchor_->tfree_page_count_--;
            mem_anchor_->tfirst_free_page_id_ = next;
#ifdef MDB_DEBUG
            sc_assert(getPageNo(mem_anchor_->tfirst_free_page_id_)
                    < PAGE_PER_SEGMENT, __FILE__, __LINE__);
#endif
        }
    }
    else
    {
        anchor = *(mem_anchor_);
        anchor.free_page_count_ -= 1;
        if (get_freepage == 0)
        {
            anchor.first_free_page_id_ = next;
        }
        MemMgr_SetMemAnchor(&anchor);
    }

    if (is_temp && !is_resident)
    {
        SetTempPageUsed(getSegmentNo(*new_page_id), getPageNo(*new_page_id));
    }

    UnsetPageFreed(*new_page_id);

    ret = DB_SUCCESS;

  end:
    UnsetBufSegment(issys);

    return ret;
}

/*
 * f_pagelist_attach: multi-volume을 지원하기 위한 flag.
 * 1 이면 free page 들을 mem_anchor_의 free page list에 연결하고,
 * 0 이면 container 내용만 제거한다.
 * 0인 경우는 volume이 변경된 후에만 사용되는 것으로,
 * dbi_Set_Volume을 통하여 호출된다.
 */
int
MemMgr_FreePageList(struct Collection *collect, struct Container *Cont,
        int f_pagelist_attach)
{
    PageID head;
    PageID tail;
    struct MemBase anchor;
    int pageCount;
    PageID nextPageID;
    struct Page *pPage;
    struct PageList *page_list = &collect->page_link_;
    int table_id = Cont ? Cont->base_cont_.id_ : 0;
    int is_resident = isResidentTable(Cont);

    if (page_list->head_ == NULL_OID)
    {
        return DB_SUCCESS;
    }

    if (page_list->head_ == NULL_OID)
    {
        return DB_SUCCESS;
    }

    head = page_list->head_;
    tail = page_list->tail_;

    if (is_resident)
    {
        /* temp page와는 달리 MemBase의 trs page list에 연결한다 */
        pageCount = collect->page_count_;

        nextPageID = head;
        while (nextPageID)
        {
            pPage = (struct Page *) PageID_Pointer(nextPageID);
            if (pPage == NULL)
            {
                break;
            }

            SetPageFreed(pPage->header_.self_);

            nextPageID = pPage->header_.link_.next_;
        }

        /* page list tail = first free page */
        OID_LightWrite(tail + sizeof(PageID) * 2,
                (char *) &mem_anchor_->trs_first_free_page_id_,
                sizeof(PageID));

        /* page list head = NULL OID */
        OID_LightWrite(head + sizeof(PageID),
                (char *) &null_pid, sizeof(PageID));

        if (mem_anchor_->trs_first_free_page_id_ != NULL_PID)
        {
            /* first free page's prev = page list tail */
            OID_LightWrite(mem_anchor_->trs_first_free_page_id_ +
                    sizeof(PageID), (char *) &tail, sizeof(PageID));
        }

        mem_anchor_->trs_free_page_count_ += pageCount;

        mem_anchor_->trs_first_free_page_id_ = head;
    }
    else if (getSegmentNo(head) >= TEMPSEG_BASE_NO)
    {
        PageID minPageID;

        if (f_pagelist_attach)
        {
            /* Free pages of temp container */
            pageCount = 0;
            minPageID = head;
            nextPageID = head;
            while (nextPageID != NULL_PID)
            {
                pPage = (struct Page *) PageID_Pointer(nextPageID);
                if (pPage == NULL)
                {
                    MDB_SYSLOG(("FAIL: Page(%d|%d) Pointer is NULL\n",
                                    getSegmentNo(nextPageID),
                                    getPageNo(nextPageID)));
                    return DB_E_PIDPOINTER;
                }

                SetPageClean(getSegmentNo(nextPageID), getPageNo(nextPageID));

                SetTempPageFree(getSegmentNo(nextPageID),
                        getPageNo(nextPageID));

                SetPageFreed(pPage->header_.self_);

                pageCount++;

                if (nextPageID < minPageID)
                    minPageID = nextPageID;

                nextPageID = pPage->header_.link_.next_;
            }

            mem_anchor_->tfree_page_count_ += pageCount;
            if (minPageID < mem_anchor_->tfirst_free_page_id_)
                mem_anchor_->tfirst_free_page_id_ = minPageID;

            SEEK_RESIDENT_SEG_BASE_NO = TEMPSEG_BASE_NO;
#ifdef MDB_DEBUG
            sc_assert(getPageNo(mem_anchor_->tfirst_free_page_id_)
                    < PAGE_PER_SEGMENT, __FILE__, __LINE__);
#endif
        }
    }
    else    /* regular container */
    {
        pageCount = collect->page_count_;
        nextPageID = head;
        while (nextPageID)
        {
            pPage = (struct Page *) PageID_Pointer(nextPageID);
            if (pPage == NULL)
            {
                break;
            }

            SetPageFreed(pPage->header_.self_);

            nextPageID = pPage->header_.link_.next_;
        }

        /* page dealloc logging */
        OID_AnchorHeavyWrite(collect->cont_id_,
                (int) collect->collect_index_, table_id,
                head, tail, mem_anchor_->first_free_page_id_,
                mem_anchor_->free_page_count_ + pageCount, 0, NULL_OID, 0);

        OID_LightWrite((tail + sizeof(PageID) * 2),
                (char *) &mem_anchor_->first_free_page_id_, sizeof(PageID));

        OID_LightWrite((head + sizeof(PageID)), (char *) &null_pid,
                sizeof(PageID));

        if (mem_anchor_->first_free_page_id_ != NULL_PID)
        {
            OID_LightWrite((mem_anchor_->first_free_page_id_ + sizeof(PageID)),
                    (char *) &tail, sizeof(PageID));
        }

        anchor = *mem_anchor_;
        anchor.free_page_count_ += pageCount;
        anchor.first_free_page_id_ = head;
        MemMgr_SetMemAnchor(&anchor);
    }

    page_list->head_ = NULL_PID;
    page_list->tail_ = NULL_PID;

    collect->item_count_ = 0;
    collect->page_count_ = 0;
    collect->free_page_list_ = NULL_OID;

    SetDirtyFlag(collect->cont_id_);

    return DB_SUCCESS;
}

/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LRU_AppendSegment(int sn)
{
#ifndef INDEX_BUFFERING
#ifdef MDB_DEBUG
    if (mem_anchor_ &&
            sn >= (int) (SEGMENT_NO - mem_anchor_->iallocated_segment_no_))
        sc_assert(0, __FILE__, __LINE__);
#endif
#endif

    if (sn == 0)
        return DB_SUCCESS;

    if (Mem_mgr->LRU_Link[sn].next != -1 || Mem_mgr->LRU_Link[sn].prev != -1)
    {
        return DB_SUCCESS;
    }

    if (segment_[sn] &&
            (segment_[sn]->page_[0].header_.cont_type_ & _CONT_RESIDENT))
    {
        return DB_SUCCESS;      /* ignore */
    }

    Mem_mgr->LRU_Link[sn].next = Mem_mgr->LRU_Head;
    Mem_mgr->LRU_Link[sn].prev = -1;

    if (Mem_mgr->LRU_Head != -1)
    {
        Mem_mgr->LRU_Link[Mem_mgr->LRU_Head].prev = sn;
    }

    Mem_mgr->LRU_Head = sn;

    if (Mem_mgr->LRU_Tail == -1)
        Mem_mgr->LRU_Tail = sn;

    Mem_mgr->LRU_Count++;

    return DB_SUCCESS;
}

/* 해당되는 segment를 없앤다 */
/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LRU_RemoveSegment(int sn)
{
    register int next_sn, prev_sn;
    int f_removed;

    if (sn == 0)
        return -1;

    if (sn == -1)
    {
        sn = Mem_mgr->LRU_Tail;
        if (sn == -1)
            return -1;
    }

    prev_sn = Mem_mgr->LRU_Link[sn].prev;
    next_sn = Mem_mgr->LRU_Link[sn].next;

    f_removed = 0;

    if (Mem_mgr->LRU_Head == sn)
    {
        Mem_mgr->LRU_Head = next_sn;
        f_removed = 1;
    }

    if (Mem_mgr->LRU_Tail == sn)
    {
        Mem_mgr->LRU_Tail = prev_sn;
        f_removed = 1;
    }

    if (next_sn != -1)
        Mem_mgr->LRU_Link[next_sn].prev = prev_sn;

    if (prev_sn != -1)
        Mem_mgr->LRU_Link[prev_sn].next = next_sn;

    if (prev_sn != -1 && next_sn != -1)
        f_removed = 1;

    Mem_mgr->LRU_Link[sn].prev = -1;
    Mem_mgr->LRU_Link[sn].next = -1;

    if (f_removed)
        Mem_mgr->LRU_Count--;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LRU_MoveToHead(int sn)
{
    if (sn <= 0)
        return -1;

    if (Mem_mgr->LRU_Head == sn)
        return DB_SUCCESS;

    LRU_RemoveSegment(sn);

    LRU_AppendSegment(sn);

    return DB_SUCCESS;
}

int
MemMgr_Set_LastFreePageID(void)
{
    struct Page *page;
    PageID pageid;
    int count = 0;
    int ret = DB_SUCCESS;

    /* check page link loop */
    int max_page_count;

    max_page_count = DBFile_GetVolumeSize(DBFILE_DATA0_TYPE);
    max_page_count += S_PAGE_SIZE;
    max_page_count /= S_PAGE_SIZE;
    if (max_page_count < mem_anchor_->allocated_segment_no_ * PAGE_PER_SEGMENT)
    {
        max_page_count = mem_anchor_->allocated_segment_no_ * PAGE_PER_SEGMENT;
    }

    sc_memset(Mem_mgr->freedPage, 0, sizeof(Mem_mgr->freedPage));

    pageid = mem_anchor_->first_free_page_id_;

    /* check valid first_free_page_id */
    if (isInvalidRid(pageid))
    {
        __SYSLOG("first_free_page_id_ crashed.. initialized..\n");
        mem_anchor_->first_free_page_id_ = NULL_OID;
        mem_anchor_->free_page_count_ = 0;
        SetDirtyFlag(0);

        return DB_SUCCESS;
    }

    while (pageid != NULL_OID)
    {
        ++count;

        page = (struct Page *) (PageID_Pointer(pageid));
        if (page == NULL)
        {
            return DB_E_OIDPOINTER;
        }

        SetPageFreed(page->header_.self_);

        if (page->header_.link_.next_ == NULL_OID)
        {
            break;
        }

        /* check page link loop */
        if (page->header_.link_.next_ == page->header_.self_ ||
                count > max_page_count)
        {
            page = (struct Page *)
                    (PageID_Pointer(mem_anchor_->first_free_page_id_));
            if (page == NULL)
            {
                return DB_E_OIDPOINTER;
            }
            page->header_.link_.next_ = NULL_OID;
            count = 1;
            SetDirtyFlag(page->header_.self_);
            MDB_SYSLOG(("MemMgr_Set_LastFreePageID: loop found\n"));
        }

        pageid = page->header_.link_.next_;
    }

    if (count != mem_anchor_->free_page_count_)
    {
        MDB_SYSLOG(("MemMgr_Set_LastFreePageID: "
                        "count(%d) != mem_anchor_->free_page_count_(%d)\n",
                        count, mem_anchor_->free_page_count_));
        mem_anchor_->free_page_count_ = count;
        SetDirtyFlag(0);
    }

    return ret;
}

/*****************************************************************************/
//! Check_DB_Compatible 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - DB image가 수정(DT_NBYTES -> DT_NCHAR, DT_BYTES -> DT_CHAR,
 *                  DT_STRING -> DT_VARCHAR, DT_NSTRING -> DT_NVARCHAR)
 *      DB image가 수정(DT_BYTE, DT_VARBYTE 추가)
 *****************************************************************************/
_PRIVATE int
Check_DB_Compatible(void)
{
    int DBFILEVER = MKVERSION(mem_anchor_->majorVer, mem_anchor_->minorVer,
            mem_anchor_->releaseVer, mem_anchor_->buildNO);

    MDB_ENDIAN_TYPE ProcType = UtilEndianType();

    if (DBFILEVER < MKVERSION(6, 1, 0, 0) &&
            MKVERSION(6, 1, 0, 0) <= MMDB_VERSION)
    {
        return DB_E_VERNOTCOMPATIBLE;
    }

    if (DBFILEVER < MKVERSION(6, 0, 0, 0) &&
            MKVERSION(6, 0, 0, 0) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    if (DBFILEVER < MKVERSION(5, 2, 0, 0) &&
            MKVERSION(5, 2, 0, 0) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    if (DBFILEVER < MKVERSION(5, 1, 0, 0) &&
            MKVERSION(5, 1, 0, 0) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    if (DBFILEVER < MKVERSION(5, 0, 0, 0) &&
            MKVERSION(5, 0, 0, 0) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    if (DBFILEVER < MKVERSION(4, 6, 0, 0) &&
            MKVERSION(4, 6, 0, 0) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    if (DBFILEVER < MKVERSION(4, 5, 0, 0) &&
            MKVERSION(4, 5, 0, 0) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    if (DBFILEVER < MKVERSION(4, 3, 1, 0) &&
            MKVERSION(4, 3, 1, 0) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    if (DBFILEVER < MKVERSION(4, 3, 0, 0) &&
            MKVERSION(4, 3, 0, 0) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    if (DBFILEVER < MKVERSION(4, 2, 0, 0) &&
            MKVERSION(4, 2, 0, 0) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    if (DBFILEVER < MKVERSION(4, 1, 2, 2) &&
            MKVERSION(4, 1, 2, 2) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    if (DBFILEVER < MKVERSION(4, 1, 2, 1) &&
            MKVERSION(4, 1, 2, 1) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    if (DBFILEVER < MKVERSION(4, 0, 0, 1) &&
            MKVERSION(4, 0, 0, 1) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    if (DBFILEVER < MKVERSION(4, 0, 0, 0) &&
            MKVERSION(4, 0, 0, 0) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    if (DBFILEVER < MKVERSION(3, 3, 5, 2) &&
            MKVERSION(3, 3, 5, 2) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    if (DBFILEVER < MKVERSION(3, 3, 5, 1) &&
            MKVERSION(3, 3, 5, 1) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    if (DBFILEVER < MKVERSION(3, 3, 4, 0) &&
            MKVERSION(3, 3, 4, 0) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    if (DBFILEVER < MKVERSION(3, 3, 3, 8) &&
            MKVERSION(3, 3, 3, 8) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    if (DBFILEVER < MKVERSION(3, 3, 3, 7) &&
            MKVERSION(3, 3, 3, 7) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    if (DBFILEVER < MKVERSION(3, 3, 3, 0) &&
            MKVERSION(3, 3, 3, 0) <= MMDB_VERSION)
        return DB_E_VERNOTCOMPATIBLE;

    switch (ProcType)
    {
    case MDB_ENDIAN_NO_INFORMATION:
        if (mem_anchor_->endian_type_ != 'N')
            return DB_E_ENDIANNOTCOMPATIBLE;
        break;

    case MDB_ENDIAN_LITTLE:
        if (mem_anchor_->endian_type_ != 'L')
            return DB_E_ENDIANNOTCOMPATIBLE;
        break;

    case MDB_ENDIAN_MIDDLE:
        if (mem_anchor_->endian_type_ != 'M')
            return DB_E_ENDIANNOTCOMPATIBLE;
        break;

    case MDB_ENDIAN_BIG:
        if (mem_anchor_->endian_type_ != 'B')
            return DB_E_ENDIANNOTCOMPATIBLE;
        break;

    default:
        return DB_E_ENDIANNOTCOMPATIBLE;
    }

    return DB_SUCCESS;
}

void
__SetPageDirty(int sn, int pn, int f_dirty)
{
    Mem_mgr->dirty_flag_[0][DIV8(sn * PAGE_PER_SEGMENT + pn)] |=
            (0x80U >> (MOD8(sn * PAGE_PER_SEGMENT + pn)));
#ifndef _ONE_BDB_
    Mem_mgr->dirty_flag_[1][DIV8(sn * PAGE_PER_SEGMENT + pn)] |=
            (0x80U >> (MOD8(sn * PAGE_PER_SEGMENT + pn)));
#endif
    if (f_dirty)
    {
        server->gDirtyBit = INIT_gDirtyBit;
    }
}
