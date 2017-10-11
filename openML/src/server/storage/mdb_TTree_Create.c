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
#include "mdb_Mem_Mgr.h"
#include "mdb_TTree.h"
#include "mdb_KeyDesc.h"
#include "mdb_Recovery.h"

int
OID_InsertLog(short Op_Type, int trans_id, const void *Relation,
        int collect_index, OID id_, char *item, int size, OID next_free_oid,
        DB_UINT8 slot_type);

/////////////////////////////////////////////////////////////////
//
// Function Name :
//
/////////////////////////////////////////////////////////////////

/*****************************************************************************/
//! TTree_Create
/*! \breif  TTree 생성
 ************************************
 * \param indexName(in)     : index's name
 * \param indexNo(in)       : index's number
 * \param indexType(in)     : index's type
 * \param record_oid(in)    : record oid
 * \param keydesc(in)       : key's field description
 * \param Cont(in)          : Containter
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *  - called : Create_Index()
 *****************************************************************************/
int
TTree_Create(char *indexName, int indexNo, int indexType, OID record_oid,
        struct KeyDesc *keydesc, struct Container *Cont)
{
    struct TTree ttree;
    int recSize = Cont->collect_.record_size_;
    struct Page *page;
    char *record;
    int transid = *(int *) CS_getspecific(my_trans_id);
    int ret;
    int op_type, log_size;
    struct IndexCatalog *idx_catalog;

#ifdef MDB_DEBUG
    sc_memset(&ttree, 0, sizeof(ttree));
#endif

    ttree.collect_.cont_id_ = record_oid;
    ttree.collect_.item_count_ = 0;
    ttree.collect_.page_count_ = 0;
    ttree.collect_.free_page_list_ = NULL_OID;
    ttree.collect_.numfields_ = 0;
    ttree.collect_.collect_index_ = -1;

    ttree.base_cont_.id_ = indexNo;
    sc_strcpy(ttree.base_cont_.name_, indexName);
    ttree.base_cont_.creator_ = 0;
    ttree.base_cont_.transid_ = -1;
    ttree.base_cont_.type_ = indexType;
    ttree.base_cont_.index_count_ = 0;
    ttree.base_cont_.index_id_ = NULL_OID;
    ttree.base_cont_.hash_next_name = NULL_OID;
    ttree.base_cont_.hash_next_id = NULL_OID;
    ttree.base_cont_.index_rebuild = 0;
    ttree.base_cont_.is_dirty_ = 1;     // dirty flag를 세팅해야 한다.
    ttree.base_cont_.max_auto_value_ = 0;

    ttree.key_desc_ = *keydesc;

    ttree.modified_time_ = 0;

    ttree.collect_.item_size_ = sizeof(struct TTreeNode);
    ttree.collect_.slot_size_ = TTREE_SLOT_SIZE;

    /* index되는 레코드의 길이... */
    ttree.collect_.record_size_ = recSize;

    ttree.collect_.page_link_.head_ = NULL_PID;
    ttree.collect_.page_link_.tail_ = NULL_PID;

    sc_memset(ttree.root_, 0, sizeof(ttree.root_[0]) * NUM_TTREE_BUCKET);

    /* 새로운 index node를 table의 index list 제일 앞쪽에 넣는다 */

    if (Cont->base_cont_.index_id_ != NULL_OID)
        ttree.base_cont_.index_id_ = Cont->base_cont_.index_id_;

    Cont->base_cont_.index_id_ = record_oid;
    Cont->base_cont_.index_count_++;

    SetDirtyFlag(Cont->collect_.cont_id_);

    page = (struct Page *) PageID_Pointer(record_oid);
    if (page == NULL)
    {
        MDB_SYSLOG(("Write ERROR  since OID converte to Pointer\n"));
        return DB_E_OIDPOINTER;
    }

    record = (char *) page + OID_GetOffSet(record_oid);

    idx_catalog = index_catalog;

    if (isTempIndex_name(ttree.base_cont_.name_))
    {
        log_size = OID_SIZE;    /* cont_id_ */
        op_type = TEMPINDEX_CREATE;
        idx_catalog = (struct IndexCatalog *) temp_index_catalog;
        goto skip_logging;
    }
    else
    {
        log_size = sizeof(ttree);
        op_type = INDEX_CREATE;
    }
    ret = OID_InsertLog(op_type, transid,
            (const void *) idx_catalog, -1, record_oid,
            (char *) &ttree, log_size, *(OID *) record, 0);

    if (ret < 0)
    {
        MDB_SYSLOG(("index create logging FAIL\n"));
        return ret;
    }

  skip_logging:

    page->header_.free_slot_id_in_page_ = *(OID *) record;
    page->header_.record_count_in_page_ += 1;

    sc_memcpy(record, (char *) &ttree, sizeof(struct TTree));
    *(record + idx_catalog->collect_.slot_size_ - 1) = USED_SLOT;

    if (page->header_.free_slot_id_in_page_ == NULL_OID)
    {
        idx_catalog->collect_.free_page_list_ = page->header_.free_page_next_;
        page->header_.free_page_next_ = FULL_PID;
    }
    idx_catalog->collect_.item_count_ += 1;

    SetDirtyFlag(record_oid);
    if (idx_catalog != (struct IndexCatalog *) temp_index_catalog)
    {
        SetDirtyFlag(idx_catalog->collect_.cont_id_);
    }

    return DB_SUCCESS;
}
