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

/*
   for createdb
*/
/*****************************************************************************/
//! IndexCatalog_init_Createdb 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param index_item_size :
 ************************************
 * \return  int
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
IndexCatalog_init_Createdb(int index_item_size)
{
    struct IndexCatalog IndexCat;

    __SYSLOG("sizeof(IndexCat):%d\n", sizeof(IndexCat));

    sc_memset(&IndexCat, 0, sizeof(struct IndexCatalog));

    IndexCat.collect_.cont_id_ = PAGE_HEADER_SIZE + INDEXCAT_OFFSET;
    IndexCat.collect_.item_count_ = 0;
    IndexCat.collect_.page_count_ = 0;
    IndexCat.collect_.item_size_ = index_item_size;
    IndexCat.collect_.slot_size_ =
            _alignedlen(IndexCat.collect_.item_size_ + 1, sizeof(long));

    IndexCat.collect_.collect_index_ = -1;
    IndexCat.collect_.page_link_.head_ = NULL_PID;
    IndexCat.collect_.page_link_.tail_ = NULL_PID;
    IndexCat.collect_.free_page_list_ = NULL_PID;

    IndexCat.icollect_.cont_id_ = PAGE_HEADER_SIZE + INDEXCAT_OFFSET;
    IndexCat.icollect_.item_count_ = 0;
    IndexCat.collect_.page_count_ = 0;
    IndexCat.icollect_.item_size_ = sizeof(struct TTreeNode);
    IndexCat.icollect_.slot_size_ = TTREE_SLOT_SIZE;
    IndexCat.icollect_.collect_index_ = -1;
    IndexCat.icollect_.page_link_.head_ = NULL_PID;
    IndexCat.icollect_.page_link_.tail_ = NULL_PID;
    IndexCat.icollect_.free_page_list_ = NULL_PID;

    segment_[0] = DBDataMem_V2P(Mem_mgr->segment_memid[0]);

    index_catalog = (struct IndexCatalog *)
            (&(segment_[0]->page_[0].body_[0]) + INDEXCAT_OFFSET);

    sc_memcpy(&(segment_[0]->page_[0].body_[0]) + INDEXCAT_OFFSET,
            (char *) &IndexCat, sizeof(struct IndexCatalog));

    return DB_SUCCESS;
}

/*****************************************************************************/
//! IndexCatalogServerInit 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void
 ************************************
 * \return  struct IndexCatalog*  : 
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
struct IndexCatalog *
IndexCatalogServerInit(void)
{
    struct IndexCatalog *IndexCat_Point;

    segment_[0] = DBDataMem_V2P(Mem_mgr->segment_memid[0]);

    IndexCat_Point = (struct IndexCatalog *)
            (&(segment_[0]->page_[0].body_[0]) + INDEXCAT_OFFSET);

    temp_index_catalog = &server->temp_index_catalog;

    temp_index_catalog->collect_.cont_id_ = OID_Cal(TEMPSEG_BASE_NO, 0, 0);
    temp_index_catalog->collect_.slot_size_
            = IndexCat_Point->collect_.slot_size_;
    temp_index_catalog->collect_.item_size_
            = IndexCat_Point->collect_.item_size_;
    temp_index_catalog->collect_.record_size_
            = IndexCat_Point->collect_.record_size_;

    temp_index_catalog->base_cont_.type_ = _CONT_TEMPTABLE;
    temp_index_catalog->base_cont_.memory_record_size_
            = IndexCat_Point->base_cont_.memory_record_size_;

    sc_memcpy(&temp_index_catalog->icollect_, &temp_index_catalog->collect_,
            sizeof(temp_index_catalog->collect_));

    return IndexCat_Point;
}

extern struct IndexCatalog *index_catalog;

/*****************************************************************************/
//! IndexCatalogServerDown 
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
DB_INT32
IndexCatalogServerDown(void)
{
    return 0;
}

/*****************************************************************************/
//! TTree_Catalog_Remove 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param index :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
TTree_Catalog_Remove(IndexID index)
{
    int ret_val;
    struct IndexCatalog *idx_catalog;

    if (isTemp_OID(index))
    {
        idx_catalog = (struct IndexCatalog *) temp_index_catalog;
    }
    else
    {
        idx_catalog = index_catalog;
    }

    ret_val = Col_Remove((struct Container *) &(idx_catalog->collect_),
            -1, index, 0);
    if (ret_val < 0)
    {
        MDB_SYSLOG(("index slot remove FAIL\n"));
        return ret_val;
    }

    return DB_SUCCESS;
}

struct TTree *
TTree_Search(const OID table_oid, const int idx_no)
{
    struct Container *Cont;
    struct TTree *ttree;
    OID index_id;

    ttree = NULL;
    Cont = (struct Container *) OID_Point(table_oid);
    if (Cont == NULL)
        return NULL;
    index_id = Cont->base_cont_.index_id_;

    while (index_id)
    {
        ttree = (struct TTree *) OID_Point(index_id);
        if (ttree == NULL)
            return NULL;
        if (ttree->base_cont_.id_ == idx_no)
            break;

        index_id = ttree->base_cont_.index_id_;
        ttree = NULL;
    }

    return ttree;
}
