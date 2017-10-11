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
#include "mdb_ppthread.h"
#include "mdb_typedef.h"
#include "mdb_inner_define.h"
#include "mdb_ContCat.h"
#include "mdb_Cont.h"
#include "mdb_IndexCat.h"
#include "mdb_Page.h"
#include "mdb_TTree.h"
#include "mdb_Recovery.h"
#include "mdb_Mem_Mgr.h"
#include "ErrorCode.h"
#include "mdb_Server.h"
#include "mdb_PMEM.h"
#include "mdb_syslog.h"

int Cont_Catalog_Remove(ContainerID);
int Cont_Destroy(struct Container *Cont);
int TTree_Catalog_Remove(IndexID);
int TTree_Destory(IndexID);

void OID_SaveAsAfter(short op_type, const void *, OID id_, int size);
void OID_SaveAsBefore(short op_type, const void *, OID id_, int size);

////////////////////////////////////////////////////////////////////////
//
// Function name:  Relation Create API
//
////////////////////////////////////////////////////////////////////////
/*****************************************************************************/
//! Create_Table 
/*! \breif  Table에 관련된 공간을 할당
 ************************************
 * \param Cont_name(in)         : table(or relation)'s name
 * \param memory_record_size(in): table(or relation)'s memory record size
 * \param heap_record_size(in)  : table(or relation)'s heap record size
 * \param numFields(in)         : number fields
 * \param type(in)              :
 * \param tableid(in)           : 
 * \param creator(in)           :
 * \param min_fixedpart(in)     : if -1 is not variable type
 * \param max_definedlength(in) :
 * \param max_records(in)       : record's max number
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘
 *  - DML은 Auto-Commit 모드
 *
 *****************************************************************************/
__DECL_PREFIX int
Create_Table(char *Cont_name, DB_UINT32 memory_record_size, DB_UINT32 heap_record_size, int numFields, ContainerType type, int tableid, int creator, int max_records    /* , char *column_name */
        , FIELDDESC_T * pFieldDesc)
{
    ContainerID container_id;
    int ret;
    struct Container *cont;
    struct ContainerCatalog *cont_catalog;

    if (isTempTable_flag(type))
    {
        cont_catalog = (struct ContainerCatalog *) temp_container_catalog;
    }
    else
    {
        cont_catalog = container_catalog;
    }

// ########## 테이블 생성을 하나의 트랜잭션으로 봄
// 테이블을 생성한 트랜잭션이 abort 되어도 테이블 생성이 rollback되지 않는다는
// 의미???

    cont = (struct Container *) Cont_Search(Cont_name);
    // 동일한 이름이 존재할 경우 Error Return

    if (cont != NULL)
    {
        return DB_E_DUPTABLENAME;
    }

    ret = Collect_AllocateSlot((struct Container *) cont_catalog,
            &cont_catalog->collect_, &container_id, 1);
    if (ret < 0)
    {
        MDB_SYSLOG(("Collect_AllocateSlot For Create Table FAIL %d\n", ret));
        return ret;
    }

#ifdef MDB_DEBUG
    if ((isTempTable_name(Cont_name) && !isTemp_OID(container_id)) ||
            (!isTempTable_name(Cont_name) && isTemp_OID(container_id)))
    {
        sc_assert(0, __FILE__, __LINE__);
    }
#endif

    // 이 함수 내부에서  Before Log & After Log를 쓴다
    ret = Cont_CreateCont(Cont_name, memory_record_size, heap_record_size, container_id, numFields, type, tableid, creator, max_records /* , column_name */
            , pFieldDesc);
    if (ret < 0)
    {
        MDB_SYSLOG(("Container Create FAIL %d\n", ret));
        return ret;
    }

    return DB_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
//
// Function name:    Relation Drop API
//
////////////////////////////////////////////////////////////////////////

/*****************************************************************************/
//!  Drop_Table
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param Cont_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
Drop_Table(char *Cont_name)
{
    struct Container *Cont;
    int result = DB_SUCCESS;
    int issys_cont;

    Cont = (struct Container *) Cont_Search(Cont_name);

    if (Cont == NULL)
    {
        return DB_SUCCESS;
    }

    SetBufSegment(issys_cont, Cont->collect_.cont_id_);

    if (Cont->base_cont_.index_count_ > 0)
    {
        OID index_id_next;
        struct TTree *ttree;

        while (Cont->base_cont_.index_id_ != NULL_OID)
        {
            ttree = (struct TTree *) OID_Point(Cont->base_cont_.index_id_);
            if (ttree == NULL)
            {
                result = DB_E_OIDPOINTER;
                goto Done_Processing;
            }

            index_id_next = ttree->base_cont_.index_id_;

            if (TTree_Destory(Cont->base_cont_.index_id_) < 0)
            {
                MDB_SYSLOG((" Index Drop destory FAIL\n"));

                result = DB_FAIL;
                goto Done_Processing;
            }

            if (TTree_Catalog_Remove(Cont->base_cont_.index_id_) < 0)
            {
                MDB_SYSLOG((" Index catalog Drop FAIL\n"));

                result = DB_FAIL;
                goto Done_Processing;
            }

            Cont->base_cont_.index_id_ = index_id_next;
        }

        SetDirtyFlag(2048);
    }

    if (Cont_Destroy(Cont) < 0)
    {
        MDB_SYSLOG(("table cont destory FAIL.\n"));

        result = DB_FAIL;
        goto Done_Processing;
    }

    if (Cont_Catalog_Remove(Cont->collect_.cont_id_) < 0)
    {
        MDB_SYSLOG(("table catalog remove FAIL.\n"));

        result = DB_FAIL;
        goto Done_Processing;
    }

    SetDirtyFlag(1024);

  Done_Processing:
    UnsetBufSegment(issys_cont);

    return result;
}

int
Drop_Table_Temp(char *Cont_name)
{
    struct Container *Cont;
    int result = DB_SUCCESS;
    int issys_cont;

    Cont = (struct Container *) Cont_Search(Cont_name);

    if (Cont == NULL)
    {
        return DB_SUCCESS;
    }

    SetBufSegment(issys_cont, Cont->collect_.cont_id_);

    if (Cont->base_cont_.index_count_ > 0)
    {
        OID index_id_next;
        struct TTree *ttree;

        while (Cont->base_cont_.index_id_ != NULL_OID)
        {
            ttree = (struct TTree *) OID_Point(Cont->base_cont_.index_id_);
            if (ttree == NULL)
            {
                result = DB_E_OIDPOINTER;
                goto Done_Processing;
            }

            index_id_next = ttree->base_cont_.index_id_;

            if (TTree_Destory(Cont->base_cont_.index_id_) < 0)
            {
                MDB_SYSLOG((" Index Drop destory FAIL\n"));

                result = DB_FAIL;
                goto Done_Processing;
            }

            Cont->base_cont_.index_id_ = index_id_next;
        }

        SetDirtyFlag(2048);
    }

    if (Cont_Destroy(Cont) < 0)
    {
        MDB_SYSLOG(("table cont destory FAIL.\n"));

        result = DB_FAIL;
        goto Done_Processing;
    }

    SetDirtyFlag(1024);

  Done_Processing:
    UnsetBufSegment(issys_cont);

    return result;
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
__Truncate_Table(struct Container *Cont, int f_pagelist_attach)
{
    int result = DB_SUCCESS;
    int i;

    if (Cont->base_cont_.index_count_ > 0)
    {
        OID index_id;
        struct TTree *ttree;
        int issys_ttree;


        index_id = Cont->base_cont_.index_id_;
        while (index_id != NULL_OID)
        {
            ttree = (struct TTree *) OID_Point(index_id);
            if (ttree == NULL)
            {
                result = DB_E_OIDPOINTER;
                goto Done_Processing;
            }

            SetBufSegment(issys_ttree, index_id);

            if (Index_MemMgr_FreeAllNodes(ttree) < 0)
            {
                UnsetBufSegment(issys_ttree);
                MDB_SYSLOG(("FAIL: table truncate index free pages\n"));
                result = DB_FAIL;
                goto Done_Processing;
            }
            SetDirtyFlag(index_id);

            UnsetBufSegment(issys_ttree);

            index_id = ttree->base_cont_.index_id_;
        }

        SetDirtyFlag(2048);
    }

    if (MemMgr_FreePageList(&Cont->collect_, Cont, f_pagelist_attach) < 0)
    {
        MDB_SYSLOG(("FAIL: table truncate free pages\n"));
        return DB_FAIL;
    }

    for (i = 0; i < VAR_COLLECT_CNT; i++)
    {
        if (MemMgr_FreePageList(&Cont->var_collects_[i], Cont,
                        f_pagelist_attach) < 0)
        {
            MDB_SYSLOG(("FAIL: table truncate var free pages, %d\n", i));
            return DB_FAIL;
        }
    }

    SetDirtyFlag(Cont->collect_.cont_id_);

    SetDirtyFlag(1024);

  Done_Processing:

    return result;
}

int
Truncate_Table(char *Cont_name)
{
    struct Container *Cont;
    int ret;
    int issys_cont;

    Cont = (struct Container *) Cont_Search(Cont_name);

    if (Cont == NULL)
        return DB_FAIL;

    SetBufSegment(issys_cont, Cont->collect_.cont_id_);

    ret = __Truncate_Table(Cont, 1);

    UnsetBufSegment(issys_cont);

    return ret;
}

////////////////////////////////////////////////////////////////////////
//
// Function name:      TTree Index Drop API
//
////////////////////////////////////////////////////////////////////////

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
Drop_Index(char *Cont_name, DB_INT16 index_no)
{
    struct Container *Cont;
    struct TTree *ttree;
    struct TTree *ttree_prev;
    OID ttree_prev_oid;
    OID index_id;
    int ret;
    int issys_cont;

    // Relation Catalog에 대한 Lock를 유지 해야 하는것 아닌가???

    Cont = (struct Container *) Cont_Search(Cont_name);
    if (Cont == NULL)
        return DB_E_UNKNOWNTABLE;

    SetBufSegment(issys_cont, Cont->collect_.cont_id_);

    index_id = Cont->base_cont_.index_id_;
    ttree = NULL;
    ttree_prev_oid = NULL_OID;
    while (index_id != NULL_OID)
    {
        ttree = (struct TTree *) OID_Point(index_id);
        if (ttree == NULL)
        {
            ret = DB_E_OIDPOINTER;
            goto end;
        }

        if (ttree->base_cont_.id_ == index_no)
        {
            break;
        }

        ttree_prev_oid = index_id;
        index_id = ttree->base_cont_.index_id_;
        ttree = NULL;
    }

    if (ttree != NULL)
    {
        if (!isTempTable(Cont))
        {
            OID_SaveAsBefore(RELATION, (const void *) NULL,
                    (Cont->collect_.cont_id_ + sizeof(struct Collection)),
                    (int) sizeof(struct BaseContainer));
        }

        Cont->base_cont_.index_count_--;
        if (ttree_prev_oid == NULL_OID)
        {
            Cont->base_cont_.index_id_ = ttree->base_cont_.index_id_;
            SetDirtyFlag(Cont->collect_.cont_id_);
        }
        else
        {
            int issys_ttree;

            SetBufSegment(issys_ttree, ttree->collect_.cont_id_);
            ttree_prev = (struct TTree *) OID_Point(ttree_prev_oid);
            ttree_prev->base_cont_.index_id_ = ttree->base_cont_.index_id_;
            UnsetBufSegment(issys_ttree);
            SetDirtyFlag(ttree_prev_oid);
        }

        ret = TTree_Destory(index_id);
        if (ret < 0)
        {
            goto end;
        }

        ret = TTree_Catalog_Remove(index_id);
        if (ret < 0)
        {
            goto end;
        }

        SetDirtyFlag(2048);

        if (!isTempTable(Cont))
        {
            OID_SaveAsAfter(RELATION, (const void *) Cont,
                    (Cont->collect_.cont_id_ + sizeof(struct Collection)),
                    (int) sizeof(struct BaseContainer));
        }

    }
    else
    {
        MDB_SYSLOG((" ERROR : Index is not exist in Container \n"));
    }

    ret = DB_SUCCESS;

  end:

    UnsetBufSegment(issys_cont);

    return ret;
}

int
Drop_Index_Temp(char *Cont_name, DB_INT16 index_no)
{
    struct Container *Cont;
    struct TTree *ttree, *ttree_prev;
    OID ttree_prev_oid;
    OID index_id;
    int ret;
    int issys_cont;

    // Relation Catalog에 대한 Lock를 유지 해야 하는것 아닌가???

    Cont = (struct Container *) Cont_Search(Cont_name);

    if (Cont == NULL)
    {
        MDB_SYSLOG(("table %s not found\n", Cont_name));
        return DB_FAIL;
    }

    SetBufSegment(issys_cont, Cont->collect_.cont_id_);

    index_id = Cont->base_cont_.index_id_;
    ttree = NULL;
    ttree_prev_oid = NULL_OID;
    while (index_id != NULL_OID)
    {
        ttree = (struct TTree *) OID_Point(index_id);
        if (ttree == NULL)
        {
            ret = DB_E_OIDPOINTER;
            goto end;
        }

        if (ttree->base_cont_.id_ == index_no)
        {
            if (ttree_prev_oid == NULL_OID)
            {
                Cont->base_cont_.index_id_ = index_id;
                SetDirtyFlag(Cont->collect_.cont_id_);
            }
            else
            {
                ttree_prev = (struct TTree *) OID_Point(ttree_prev_oid);
                ttree_prev->base_cont_.index_id_ = index_id;
                SetDirtyFlag(ttree_prev->collect_.cont_id_);
            }

            break;
        }

        index_id = ttree->base_cont_.index_id_;
        ttree_prev = ttree;
    }

    if (index_id != NULL_OID)
    {
        if (TTree_Destory(index_id) < 0)
        {
            MDB_SYSLOG((" Index Drop FAIL\n"));

            ret = DB_FAIL;
            goto end;
        }

        SetDirtyFlag(2048);
    }
    else
    {
        MDB_SYSLOG((" ERROR : Index is not exist in Container \n"));
    }

    ret = DB_SUCCESS;

  end:
    UnsetBufSegment(issys_cont);

    return ret;
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
ContCat_Index_Rebuild(int rebuild_mode)
{
    OID cont_oid;
    OID rec_oid;
    OID index_id;
    PageID page_id;
    DB_INT32 slot_size;
    struct Container *Cont;
    struct TTree *ttree;
    struct Page *page;
    char *record;
    char *rec_buf = NULL;
    DB_UINT32 rec_offset;
    int issys;
    int ret_val;
    int remove_duplicate_key_flag = 1;
    DB_UINT32 total_row_count;
    DB_UINT32 page_count;       /* sign match */
    OID prev_page_id;
    int max_page_count;
    int mem_segments;
    int server_status = server->status;
    int accum_page_count;
    DB_LONG max_av;
    int f_offset_av;
    int issys_ttree = 0;
    int issys_cont = 0;

    /* free page list는 hint 정보로 recovery 시점에 다시 구성되어야 한다. */
    extern int Index_Link_FreePageList(void);

    Index_Link_FreePageList();

    mem_segments = server->shparams.mem_segments;
    if (server->shparams.mem_segments_indexing &&
            server->shparams.mem_segments_indexing >
            server->shparams.mem_segments)
    {
        server->shparams.mem_segments = server->shparams.mem_segments_indexing;
    }

    max_page_count = DBFile_GetVolumeSize(DBFILE_DATA0_TYPE);
    max_page_count += S_PAGE_SIZE;
    max_page_count /= S_PAGE_SIZE;

    server->status = SERVER_STATUS_INDEXING;

    if (server->shparams.mem_segments)  /* PR29 fix */
        rec_buf = mdb_malloc(S_PAGE_SIZE);

    cont_oid = Col_GetFirstDataID((struct Container *) container_catalog, 0);

    accum_page_count = 0;
    if (server->shparams.kick_dog_page_count)
        sc_kick_dog();

    while (cont_oid != NULL_OID)
    {
        if (cont_oid == INVALID_OID)
        {
            ret_val = DB_E_OIDPOINTER;
            goto error;
        }

        Cont = (struct Container *) OID_Point(cont_oid);
        if (Cont == NULL)
        {
            ret_val = DB_E_INVLAIDCONTOID;
            goto error;
        }

        SetBufSegment(issys_cont, cont_oid);

        /* check autoincrement max value */
        /* init variables */
        max_av = 0;     /* auto value starts from 1 */
        f_offset_av = -1;       /* will set h_offset if auto increment'ed */

        if (rebuild_mode == PARTIAL_REBUILD ||
                rebuild_mode == PARTIAL_REBUILD_DDL)
        {
            if (Cont->base_cont_.index_rebuild == 0)
            {
                MDB_SYSLOG(("Skip index build for table %s\n",
                                Cont->base_cont_.name_));
                goto next_container;
            }
        }
        Cont->base_cont_.index_rebuild = 0;
        SetDirtyFlag(cont_oid);

        issys_ttree = 0;

        index_id = Cont->base_cont_.index_id_;
        while (index_id != NULL_OID)
        {
            UnsetBufSegment(issys_ttree);
            ttree = (struct TTree *) OID_Point(index_id);
            if (ttree == NULL)
            {
                ret_val = DB_E_OIDPOINTER;
                goto error;
            }

            SetBufSegment(issys_ttree, index_id);

#ifdef CHECK_INDEX_VALIDATION
            {
                void TTree_Check_Root(struct TTree *ttree);

                if (rebuild_mode != ALL_REBUILD)
                    TTree_Check_Root(ttree);
            }
#endif

            if (rebuild_mode == PARTIAL_REBUILD_DDL &&
                    ttree->base_cont_.index_rebuild == 0)
            {
                index_id = ttree->base_cont_.index_id_;
                continue;
            }

            if (rebuild_mode == PARTIAL_REBUILD ||
                    rebuild_mode == PARTIAL_REBUILD_DDL)
            {
                ret_val = Index_MemMgr_FreeAllNodes(ttree);
                if (ret_val < 0)
                {
                    MDB_SYSLOG(("TTree Destroy MMDB_FAIL\n"));
                    goto error;
                }
            }
            ttree->collect_.page_link_.head_ = NULL_OID;
            ttree->collect_.page_link_.tail_ = NULL_OID;
            ttree->collect_.cont_id_ = index_id;
            ttree->collect_.item_count_ = 0;
            ttree->collect_.page_count_ = 0;
            ttree->collect_.free_page_list_ = NULL_PID;

            /* if T-tree node size is changed... */
            ttree->collect_.item_size_ = sizeof(struct TTreeNode);
            ttree->collect_.slot_size_ = TTREE_SLOT_SIZE;
            sc_memset(ttree->root_, 0,
                    sizeof(ttree->root_[0]) * NUM_TTREE_BUCKET);

            /* check autoincrement max value */
            /* The auto increment index has only one column of
             * integer data type. */
            if (ttree->key_desc_.field_desc_[0].flag_ & FD_AUTOINCREMENT)
            {
                max_av = 0;
                f_offset_av = ttree->key_desc_.field_desc_[0].h_offset_;
            }

            SetDirtyFlag(index_id);
            index_id = ttree->base_cont_.index_id_;
        }

        UnsetBufSegment(issys_ttree);

        __SYSLOG("Index build for table %s, %d\n", Cont->base_cont_.name_,
                Cont->base_cont_.id_);

        /********************************************************
          Recompute follwing values while building indexes.
            1) For container, Cont->collect_.item_count_ 
            2) For each page, page->header_.record_count_in_page_
        *********************************************************/

        total_row_count = 0;
        page_count = 0;
        slot_size = Cont->collect_.slot_size_;
        page_id = Cont->collect_.page_link_.head_;

        prev_page_id = NULL_OID;

        while (page_id != NULL_PID)
        {
            page = (struct Page *) PageID_Pointer(page_id);
            if (page == NULL)
            {
                ret_val = DB_E_OIDPOINTER;
                goto error;
            }

            if (cont_oid != page->header_.cont_id_)
            {
                MDB_SYSLOG(("[MISMATCH] ContCat_Index_Rebuild: "
                                "cont oid, %d, page cont id %d\n",
                                cont_oid, page->header_.cont_id_));

                page = (struct Page *) PageID_Pointer(prev_page_id);
                if (page == NULL)
                {
                    ret_val = DB_E_OIDPOINTER;
                    goto error;
                }

                page->header_.link_.next_ = NULL_OID;
                SetDirtyFlag(prev_page_id);
                break;
            }

            accum_page_count++;
            if (server->shparams.kick_dog_page_count &&
                    accum_page_count > server->shparams.kick_dog_page_count)
            {
                accum_page_count = 0;
                sc_kick_dog();
            }
            page_count++;

            if ((prev_page_id && page->header_.link_.next_ == prev_page_id) ||
                    page_count > max_page_count)
            {
                /* looping */
                MDB_SYSLOG(
                        ("Error: index rebuilding. [%s] has page link loop\n",
                                Cont->base_cont_.name_));
#ifdef MDB_DEBUG
                sc_assert(0, __FILE__, __LINE__);
#endif

                if (page->header_.link_.next_ == prev_page_id)
                {
                    MDB_SYSLOG(("    self loop %d\n", prev_page_id));
                    page->header_.link_.next_ = NULL_OID;
                    SetDirtyFlag(page->header_.self_);
                    break;
                }

                ret_val = DB_E_PAGELINKLOOP;
                goto error;
            }

            rec_offset = PAGE_HEADER_SIZE;
            record = (char *) page + rec_offset;

            SetBufSegment(issys, page_id);

            while ((rec_offset + slot_size) <= S_PAGE_SIZE)
            {
                if (!(*(record + slot_size - 1) & USED_SLOT))
                {
                    rec_offset += slot_size;
                    record += slot_size;
                    continue;
                }

                if (Cont->base_cont_.index_id_ == NULL_OID)
                {
                    rec_offset += slot_size;
                    record += slot_size;
                    total_row_count++;
                    continue;
                }

                if (server->shparams.mem_segments)
                    sc_memcpy(rec_buf, record, slot_size);
                else
                    rec_buf = record;

                rec_oid = page_id + rec_offset;

                ret_val = 0;

                index_id = Cont->base_cont_.index_id_;
                while (index_id != NULL_OID)
                {
                    ttree = (struct TTree *) OID_Point(index_id);
                    if (ttree == NULL)
                    {
                        ret_val = DB_E_OIDPOINTER;
                        goto error;
                    }

                    if (rebuild_mode == PARTIAL_REBUILD_DDL &&
                            ttree->base_cont_.index_rebuild == 0)
                    {
                        index_id = ttree->base_cont_.index_id_;
                        continue;
                    }
                    if (isPartialIndex_name(ttree->base_cont_.name_))
                    {
                        index_id = ttree->base_cont_.index_id_;
                        continue;
                    }

                    SetBufSegment(issys_ttree, index_id);

                    ret_val = TTree_InsertRecord(ttree, rec_buf, rec_oid, 1);

                    UnsetBufSegment(issys_ttree);

                    if (ret_val < 0)
                    {
                        if (ret_val == DB_E_DUPUNIQUEKEY)
                        {
                            MDB_SYSLOG(("Index Rebuild Uniqueness FAIL %s\n",
                                            ttree->base_cont_.name_));
                            break;
                        }

                        MDB_SYSLOG(("Index Rebuild Insert FAIL %s\n",
                                        ttree->base_cont_.name_));

                        UnsetBufSegment(issys);
                        goto error;
                    }

                    index_id = ttree->base_cont_.index_id_;
                }

                if (ret_val == DB_E_DUPUNIQUEKEY)
                {
                    OID saved_index_id = index_id;

                    if (remove_duplicate_key_flag == 0)
                    {
                        UnsetBufSegment(issys);
                        goto error;
                    }

                    /* remove_duplicate_key_flag == 1 */
                    MDB_SYSLOG(("Remove Current Record \n"));

                    /* remove the inserted keys */
                    index_id = Cont->base_cont_.index_id_;
                    while (index_id != NULL_OID && index_id != saved_index_id)
                    {
                        ttree = (struct TTree *) OID_Point(index_id);
                        if (ttree == NULL)
                        {
                            ret_val = DB_E_OIDPOINTER;
                            UnsetBufSegment(issys);
                            goto error;
                        }

                        if (isPartialIndex_name(ttree->base_cont_.name_))
                        {
                            index_id = ttree->base_cont_.index_id_;
                            continue;
                        }

                        SetBufSegment(issys_ttree, index_id);
                        if (total_row_count > TNODE_MAX_ITEM * 3)
                            ret_val =
                                    TTree_RemoveRecord(ttree, rec_buf,
                                    rec_oid);
                        else
                            ret_val = TTree_RemoveRecord(ttree, NULL, rec_oid);
                        UnsetBufSegment(issys_ttree);
                        if (ret_val < 0 && ret_val != DB_E_NOMORERECORDS)
                        {
                            MDB_SYSLOG(
                                    ("Index Rebuild Remove FAIL %s,rid 0x%x\n",
                                            ttree->base_cont_.name_, rec_oid));
                            UnsetBufSegment(issys);
                            goto error;
                        }
                        index_id = ttree->base_cont_.index_id_;
                    }

                    /* remove the record */
                    *(record + slot_size - 1) = FREE_SLOT;
                    /* set dirty flag */
                    SetDirtyFlag(page_id);
                }
                else    /* ret_val != DB_E_DUPUNIQUEKEY */
                {
                    int rec_av;

                    /* get av from record and compare them */
                    if (f_offset_av != -1)
                    {
                        sc_memcpy(&rec_av, rec_buf + f_offset_av, sizeof(int));

                        if (max_av < rec_av)
                            max_av = rec_av;
                    }
                    total_row_count++;
                }

                /* get next record */
                rec_offset += slot_size;
                record += slot_size;
            }


            UnsetBufSegment(issys);

            prev_page_id = page_id;

            /* get next page id */
            page_id = page->header_.link_.next_;
        }

        if (Cont->collect_.page_count_ != page_count)
            Cont->collect_.page_count_ = page_count;

        /* sign match */
        if ((DB_UINT32) (Cont->collect_.item_count_) != total_row_count)
        {
            Cont->collect_.item_count_ = total_row_count;
        }

        index_id = Cont->base_cont_.index_id_;
        while (index_id != NULL_OID)
        {
            ttree = (struct TTree *) OID_Point(index_id);
            if (ttree == NULL)
            {
                ret_val = DB_E_OIDPOINTER;
                goto error;
            }

            if (rebuild_mode == PARTIAL_REBUILD ||
                    (rebuild_mode == PARTIAL_REBUILD_DDL &&
                            ttree->base_cont_.index_rebuild))
            {
                MDB_SYSLOG(("\t%s,%d\n", ttree->base_cont_.name_,
                                ttree->base_cont_.id_));
            }
            ttree->base_cont_.index_rebuild = 0;
            SetDirtyFlag(ttree->collect_.cont_id_);
            index_id = ttree->base_cont_.index_id_;
        }

        Cont->base_cont_.index_rebuild = 0;
        SetDirtyFlag(Cont->collect_.cont_id_);

      next_container:

        if (f_offset_av != -1)
        {
            if (Cont->base_cont_.max_auto_value_ < max_av)
            {
                Cont->base_cont_.max_auto_value_ = max_av + 1;
                SetDirtyFlag(Cont->collect_.cont_id_);
            }
        }

        UnsetBufSegment(issys_cont);

#ifdef CHECK_INDEX_VALIDATION
        Index_Check_Page();
#endif

        MemMgr_DeallocateAllSegments(1);

        Col_GetNextDataID(cont_oid,
                container_catalog->collect_.slot_size_,
                container_catalog->collect_.page_link_.tail_, 0);
    }       /* while - containers' loop */

    ret_val = DB_SUCCESS;

  error:
    if (server->shparams.mem_segments)
        SMEM_FREENUL(rec_buf);

    UnsetBufSegment(issys_ttree);
    UnsetBufSegment(issys_cont);

    server->shparams.mem_segments = mem_segments;
    MemMgr_DeallocateAllSegments(1);
    server->status = server_status;

    return ret_val;
}

int
ContCat_Index_Check(void)
{
    OID cont_oid;
    OID index_id;
    struct Container *Cont;
    struct TTree *ttree;
    int table_cnt, index_cnt;
    extern int TTree_Get_ItemCount(struct TTree *ttree);
    int issys = 0, issys_ttree = 0;
    int ret;

    /* anchor lsn과 anchor file의 begin checkpoint lsn이 같은 경우는
     * 정상적인 shutdown 이므로 점검 하지 않음 */
    if (IsNormalShutdown())
    {
        return DB_SUCCESS;
    }

    cont_oid = Col_GetFirstDataID((struct Container *) container_catalog, 0);

    while (cont_oid != NULL_OID)
    {
        if (cont_oid == INVALID_OID)
        {
            ret = DB_E_OIDPOINTER;
            goto end;
        }

        Cont = (struct Container *) OID_Point(cont_oid);
        if (Cont == NULL)
        {
            ret = DB_E_INVLAIDCONTOID;
            goto end;
        }

        SetBufSegment(issys, cont_oid);

        table_cnt = Cont->collect_.item_count_;

#ifdef MDB_DEBUG
        MDB_SYSLOG(("No. records of table <%s,%d>: %d\n",
                        Cont->base_cont_.name_, Cont->base_cont_.id_,
                        Cont->collect_.item_count_));
#endif

        index_id = Cont->base_cont_.index_id_;
        while (index_id != NULL_OID)
        {
            ttree = (struct TTree *) OID_Point(index_id);
            if (ttree == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }

            SetBufSegment(issys_ttree, index_id);

            index_cnt = TTree_Get_ItemCount(ttree);

#ifdef MDB_DEBUG
            MDB_SYSLOG(("  No. records of index <%s,%d>: %d\n",
                            ttree->base_cont_.name_, ttree->base_cont_.id_,
                            index_cnt));
#endif
            if (table_cnt != index_cnt)
            {
                MDB_SYSLOG(("MISMATCH: No. of record of table <%s> and "
                                "index <%s>: %d, %d\n",
                                Cont->base_cont_.name_,
                                ttree->base_cont_.name_, table_cnt,
                                index_cnt));
                if (index_cnt == -2)
                {
                    ret = -1;
                    goto end;
                }
                Cont->base_cont_.index_rebuild = 1;
                SetDirtyFlag(cont_oid);
                {
                    extern int fRecovered;

                    fRecovered |= PARTIAL_REBUILD;
                }
            }

            UnsetBufSegment(issys_ttree);

            index_id = ttree->base_cont_.index_id_;
        }

        /* free_page_list 점검 */
        if (f_checkpoint_finished == CHKPT_BEGIN)
        {
            struct Page *pPage1;
            PageID pageid;
            unsigned int sn, pn;
            DB_UINT32 page_count;       /* sign match */

          RETRACE_FREEPAGELIST:
            pageid = Cont->collect_.free_page_list_;
            page_count = 0;

            while (1)
            {
                if (pageid == NULL_OID)
                    break;

                sn = getSegmentNo(pageid);
                pn = getPageNo(pageid);

                if (sn > SEGMENT_NO || pn > PAGE_PER_SEGMENT ||
                        pageid == FULL_PID ||
                        page_count >
                        mem_anchor_->allocated_segment_no_ * PAGE_PER_SEGMENT)
                {
                    MDB_SYSLOG(("%s's freePageList being reconstructing\n",
                                    Cont->base_cont_.name_));
                    Collect_reconstruct_freepagelist(&Cont->collect_);
                    goto RETRACE_FREEPAGELIST;
                }

                pPage1 = (struct Page *) OID_Point(pageid);
                if (pPage1 == NULL)
                {
                    ret = DB_E_OIDPOINTER;
                    goto end;
                }

#ifdef MDB_DEBUG
                sc_assert(pPage1->header_.self_ !=
                        pPage1->header_.free_page_next_, __FILE__, __LINE__);
#endif

                pageid = pPage1->header_.free_page_next_;
                page_count++;
            }
        }

        UnsetBufSegment(issys);

        Col_GetNextDataID(cont_oid,
                container_catalog->collect_.slot_size_,
                container_catalog->collect_.page_link_.tail_, 0);
    }       /* while - containers' loop */

    ret = DB_SUCCESS;

  end:
    UnsetBufSegment(issys);
    UnsetBufSegment(issys_ttree);

    return ret;
}

int
ContCat_Container_Check(void)
{
    OID cont_oid;
    struct Container *Cont;
    DB_UINT32 count;            /* sign match */
    struct Page *pPage1;
    PageID pageid;
    unsigned int sn, pn;
    int issys_cont;

    cont_oid = Col_GetFirstDataID((struct Container *) container_catalog, 0);

    while (cont_oid != NULL_OID)
    {
        if (cont_oid == INVALID_OID)
        {
            return DB_E_OIDPOINTER;
        }

        Cont = (struct Container *) OID_Point(cont_oid);
        if (Cont == NULL)
            return DB_E_INVLAIDCONTOID;

        SetBufSegment(issys_cont, cont_oid);

        if (isResidentTable(Cont) || isTempTable(Cont))
            goto next_cont;

        /* page_count 점검 */
        pageid = Cont->collect_.page_link_.head_;
        count = 0;

        while (1)
        {
            if (pageid == NULL_OID)
                break;

            pPage1 = (struct Page *) OID_Point(pageid);
            if (pPage1 == NULL)
            {
                UnsetBufSegment(issys_cont);
                return DB_E_OIDPOINTER;
            }

#ifdef MDB_DEBUG
            sc_assert(pPage1->header_.cont_id_ == Cont->collect_.cont_id_,
                    __FILE__, __LINE__);
#endif

            count++;

            pageid = pPage1->header_.link_.next_;
        }

        if (Cont->collect_.page_count_ != count)
        {
            Cont->collect_.page_count_ = count;
            SetDirtyFlag(Cont->collect_.cont_id_);
        }

        /* free_page_list 점검 */
      RETRACE_FREEPAGELIST:

        /* check only nologging tables. */
        if (!isNoLogging(Cont))
            goto next_cont;

        pageid = Cont->collect_.free_page_list_;
        count = 0;

        while (1)
        {
            if (pageid == NULL_OID)
                break;

            count++;

            sn = getSegmentNo(pageid);
            pn = getPageNo(pageid);

            if (sn > SEGMENT_NO || pn > PAGE_PER_SEGMENT || pageid == FULL_PID
                    || count > Cont->collect_.page_count_)
            {
                MDB_SYSLOG(("%s's freePageList being reconstructing\n",
                                Cont->base_cont_.name_));
                Collect_reconstruct_freepagelist(&Cont->collect_);
                goto RETRACE_FREEPAGELIST;
            }

            pPage1 = (struct Page *) OID_Point(pageid);
            if (pPage1 == NULL)
            {
                UnsetBufSegment(issys_cont);
                return DB_E_OIDPOINTER;
            }

#ifdef MDB_DEBUG
            sc_assert(pPage1->header_.self_ !=
                    pPage1->header_.free_page_next_, __FILE__, __LINE__);
#endif

            pageid = pPage1->header_.free_page_next_;
        }

      next_cont:
        UnsetBufSegment(issys_cont);
        Col_GetNextDataID(cont_oid,
                container_catalog->collect_.slot_size_,
                container_catalog->collect_.page_link_.tail_, 0);

    }       /* while - containers' loop */
    return DB_SUCCESS;
}

int
Check_Index_Item_Size(void)
{
    PageID contcat_pageid;
    OID cont_oid;
    struct Container *Cont;
    struct TTree *ttree;
    OID index_id;

    contcat_pageid = (PageID) (container_catalog->collect_.page_link_.head_);

    if (contcat_pageid == NULL_PID)
        return DB_FAIL;

    // 릴레이션 페이지의 첫번째 Slot 물리적 주소를 계산한다.
    cont_oid = OID_Cal(getSegmentNo(contcat_pageid), getPageNo(contcat_pageid),
            PAGE_HEADER_SIZE);

    if (!Col_IsDataSlot(cont_oid, container_catalog->collect_.slot_size_, 0))
    {
        Col_GetNextDataID(cont_oid,
                container_catalog->collect_.slot_size_,
                container_catalog->collect_.page_link_.tail_, 0);
    }
    while (cont_oid != NULL_OID)
    {
        if (cont_oid == INVALID_OID)
        {
            return DB_E_OIDPOINTER;
        }

        Cont = (struct Container *) OID_Point(cont_oid);

        if (Cont == NULL)
        {
            return DB_FAIL;
        }

        index_id = Cont->base_cont_.index_id_;
        if (index_id != NULL_OID)
        {
            ttree = (struct TTree *) OID_Point(index_id);
            if (ttree == NULL)
            {
                return DB_E_OIDPOINTER;
            }

            if (ttree->collect_.item_size_ != sizeof(struct TTreeNode))
                return DB_FAIL;
            else
                return DB_SUCCESS;
        }

        Col_GetNextDataID(cont_oid,
                container_catalog->collect_.slot_size_,
                container_catalog->collect_.page_link_.tail_, 0);

    }       /* while - containers' loop */

    return DB_SUCCESS;
}


/*****************************************************************************/
//! Rename_Index 
/*! \breif  change ttree's name.
 ************************************
 * \param Cont_name : container name of ttree to rename.
 * \param index_no  : index_no of ttree to rename.
 * \param new_name  : new name for ttree.
 ************************************
 * \return  SUCCESS or FAIL.
 ************************************
 * \note  None.
 *****************************************************************************/
int
Rename_Index(char *Cont_name, DB_INT16 index_no, char *new_name)
{
    struct Container *Cont;
    struct TTree *ttree;
    OID index_id;

    // Relation Catalog에 대한 Lock를 유지 해야 하는것 아닌가???

    Cont = (struct Container *) Cont_Search(Cont_name);

    if (Cont == NULL)
    {
        MDB_SYSLOG(("table %s not found\n", Cont_name));
        return DB_FAIL;
    }

    index_id = Cont->base_cont_.index_id_;
    ttree = NULL;

    while (index_id != NULL_OID)
    {
        ttree = (struct TTree *) OID_Point(index_id);
        if (ttree == NULL)
        {
            return DB_E_OIDPOINTER;
        }

        if (ttree->base_cont_.id_ == index_no)
            break;

        index_id = ttree->base_cont_.index_id_;
    }

    if (ttree != NULL)
    {
        OID_SaveAsBefore(RELATION, (const void *) NULL,
                (ttree->collect_.cont_id_ + sizeof(struct Collection)),
                (int) sizeof(struct BaseContainer));
        sc_strncpy(ttree->base_cont_.name_, new_name, REL_NAME_LENG);

        OID_SaveAsAfter(RELATION, (const void *) NULL,
                (ttree->collect_.cont_id_ + sizeof(struct Collection)),
                (int) sizeof(struct BaseContainer));
    }
    else
    {
        MDB_SYSLOG((" ERROR : Index is not exist in Container \n"));
    }

    return DB_SUCCESS;
}
