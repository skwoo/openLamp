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
#include "mdb_ppthread.h"
#include "mdb_Cont.h"
#include "mdb_IndexCat.h"
#include "mdb_KeyDesc.h"
#include "mdb_TTree.h"
#include "mdb_Recovery.h"
#include "ErrorCode.h"
#include "mdb_syslog.h"
#include "mdb_Server.h"

#include "mdb_PMEM.h"

int TTree_Create(char *indexName, int indexNo, int IndexType, OID record_oid,
        struct KeyDesc *keydesc, struct Container *Cont);
void OID_SaveAsAfter(short op_type, const void *, OID id_, int size);
void OID_SaveAsBefore(short op_type, const void *, OID id_, int size);

extern struct IndexCatalog *index_catalog;

////////////////////////////////////////////////////////////////////////
//
// Function Name : TTree Index Create API
//
////////////////////////////////////////////////////////////////////////
/*****************************************************************************/
//! Build_Index
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param Cont(in)          : Container
 * \param new_ttree_oid(in) : 
 * \param scandesc(in/out)  : scan 정보(?)
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *  - collation type 추가
 *****************************************************************************/
static int
Build_Index(struct Container *Cont, OID new_ttree_oid, SCANDESC_T * scandesc)
{
    int sscan_flag;
    OID index_id;
    struct TTree *use_ttree = NULL;
    struct TTree *new_ttree;
    struct KeyValue *minkey = NULL;
    struct KeyValue *maxkey = NULL;
    struct Filter *filter = NULL;
    char *record;
    OID rec_oid;
    int rec_size;
    int ret, i, j;
    int issys;
    int issys_ttree;
    int comp_res;

    /* Cont->collect_.item_count_ > 0 */

    if (scandesc == NULL || scandesc->idxname == NULL)
    {
        sscan_flag = 1; /* do sequential scan to build the new index */
    }
    else    /* scandesc != NULL && scandesc->idxname != NULL */
    {
        sscan_flag = 0; /* do index scan to build the new index */

        /* find ttree to be scanned */
        index_id = Cont->base_cont_.index_id_;
        while (index_id != NULL_OID)
        {
            use_ttree = (struct TTree *) OID_Point(index_id);
            if (use_ttree == NULL)
            {
                return DB_E_OIDPOINTER;
            }

            if (!mdb_strcmp(use_ttree->base_cont_.name_, scandesc->idxname))
                break;
            index_id = use_ttree->base_cont_.index_id_;
        }
        if (index_id == NULL_OID)
        {   /* Not Found */
            MDB_SYSLOG(("Build Index: IndexName(%s) of ScanDesc FAIL\n",
                            scandesc->idxname));
            return DB_FAIL;
        }

        minkey = &scandesc->minkey;
        maxkey = &scandesc->maxkey;

        for (i = 0; i < minkey->field_count_; i++)
        {
            for (j = 0; j < use_ttree->key_desc_.field_count_; j++)
            {
                if (minkey->field_value_[i].position_
                        == use_ttree->key_desc_.field_desc_[j].position_)
                {
                    minkey->field_value_[i].order_
                            = use_ttree->key_desc_.field_desc_[j].order_;
                    minkey->field_value_[i].h_offset_
                            = use_ttree->key_desc_.field_desc_[j].h_offset_;
                    minkey->field_value_[i].fixed_part
                            = use_ttree->key_desc_.field_desc_[j].fixed_part;
                    // 안정화가 되면 아래는 지운다
#if defined(MDB_DEBUG)
                    if (use_ttree->key_desc_.field_desc_[j].collation ==
                            MDB_COL_NONE)
                        sc_assert(0, __FILE__, __LINE__);
#endif
                    minkey->field_value_[i].collation
                            = use_ttree->key_desc_.field_desc_[j].collation;
                    break;
                }
            }
        }
        for (i = 0; i < maxkey->field_count_; i++)
        {
            for (j = 0; j < use_ttree->key_desc_.field_count_; j++)
            {
                if (maxkey->field_value_[i].position_
                        == use_ttree->key_desc_.field_desc_[j].position_)
                {
                    maxkey->field_value_[i].order_
                            = use_ttree->key_desc_.field_desc_[j].order_;
                    maxkey->field_value_[i].h_offset_
                            = use_ttree->key_desc_.field_desc_[j].h_offset_;
                    maxkey->field_value_[i].fixed_part
                            = use_ttree->key_desc_.field_desc_[j].fixed_part;
                    // 안정화가 되면 아래는 지운다
#if defined(MDB_DEBUG)
                    if (use_ttree->key_desc_.field_desc_[j].collation ==
                            MDB_COL_NONE)
                        sc_assert(0, __FILE__, __LINE__);
#endif
                    maxkey->field_value_[i].collation
                            = use_ttree->key_desc_.field_desc_[j].collation;
                    break;
                }
            }
        }
    }

    if (scandesc != NULL && scandesc->filter.expression_count_ > 0)
    {
        SYSTABLE_T sysTable;
        SYSFIELD_T *fields = NULL;

        ret = _Schema_GetTableInfo(Cont->base_cont_.name_, &sysTable);
        if (ret != DB_SUCCESS)
        {
            MDB_SYSLOG(("Build_Index: Get Table Schema Info FAIL\n"));
            return ret;
        }

        fields = (SYSFIELD_T *)
                PMEM_ALLOC(sizeof(SYSFIELD_T) * sysTable.numFields);
        if (fields == NULL)
        {
            MDB_SYSLOG(("Build_Index: Allocate Memory FAIL\n"));
            return DB_E_OUTOFMEMORY;
        }

        ret = _Schema_GetFieldsInfo(Cont->base_cont_.name_, fields);
        if (ret < 0)
        {
            PMEM_FREENUL(fields);
            MDB_SYSLOG(("Build_Index: Get Fields Schema Info FAIL\n"));
            return ret;
        }

        filter = &scandesc->filter;
        for (i = 0; i < filter->expression_count_; i++)
        {
            for (j = 0; j < sysTable.numFields; j++)
            {
                if (filter->expression_[i].position_ == fields[j].position)
                {
                    filter->expression_[i].h_offset_ = fields[j].h_offset;
                    filter->expression_[i].fixed_part = fields[j].fixed_part;
                    filter->expression_[i].length_ = fields[j].length;
                    // 안정화가 되면 지우자
#if defined(MDB_DEBUG)
                    if (fields[j].collation == MDB_COL_NONE)
                        sc_assert(0, __FILE__, __LINE__);
#endif
                    filter->expression_[i].collation = fields[j].collation;
                    break;
                }
            }
        }

        PMEM_FREENUL(fields);
    }

    rec_size = Cont->collect_.record_size_;

    new_ttree = (struct TTree *) OID_Point(new_ttree_oid);
    if (new_ttree == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    SetBufSegment(issys_ttree, new_ttree_oid);

  Build_Again:
    if (sscan_flag)     /* do sequentail scan */
    {
        rec_oid = Col_GetFirstDataID(Cont, 0);
        while (rec_oid != NULL_OID)
        {
            if (rec_oid == INVALID_OID)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }

            record = (char *) OID_Point(rec_oid);
            if (record == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }

            SetBufSegment(issys, rec_oid);

            for (i = 0; i < 1; i++)
            {
                if (filter)
                {       /* do filtering */
                    if (Filter_Compare(filter, record, rec_size) != DB_SUCCESS)
                        break;
                }


                ret = TTree_InsertRecord(new_ttree, record, rec_oid, 1);

                if (ret != DB_SUCCESS)
                {
                    UnsetBufSegment(issys);
                    MDB_SYSLOG(("Build_Index: Insert Key FAIL %d %d %d\n", i,
                                    rec_oid, ret));
                    goto end;
                }
            }

            UnsetBufSegment(issys);

            Col_GetNextDataID(rec_oid, Cont->collect_.slot_size_,
                    Cont->collect_.page_link_.tail_, 0);
        }
    }
    else    /* sscan_flag == 0 : do index scan */
    {
        struct TTreePosition first, curr, last;

        if (minkey && minkey->field_value_[0].mode_ != MDB_AL)
        {
            if (TTree_Get_First_Ge(use_ttree, minkey, &first) < 0)
            {
                sscan_flag = 1;
                goto Build_Again;
            }
        }
        else
        {
            first.node_ = (struct TTreeNode *) TTree_SmallestChild(use_ttree);
            if (first.node_ == NULL)
            {
#ifdef INDEX_BUFFERING
                first.self_ = NULL_OID;
#endif
                sscan_flag = 1;
                goto Build_Again;
            }
#ifdef INDEX_BUFFERING
            first.self_ = first.node_->self_;
#endif
            first.idx_ = 0;
        }

        if (maxkey && maxkey->field_value_[0].mode_ != MDB_AL)
        {
            if (TTree_Get_Last_Le(use_ttree, maxkey, &last) < 0)
            {
                sscan_flag = 1;
                goto Build_Again;
            }
        }
        else
        {
            last.node_ = (struct TTreeNode *) TTree_GreatestChild(use_ttree);
            if (last.node_ == NULL)
            {
#ifdef INDEX_BUFFERING
                last.self_ = NULL_OID;
#endif
                sscan_flag = 1;
                goto Build_Again;
            }
#ifdef INDEX_BUFFERING
            last.self_ = last.node_->self_;
#endif
            last.idx_ = last.node_->item_count_ - 1;
        }

        /* check key range */
        if (minkey && minkey->field_value_[0].mode_ != MDB_AL)
        {
#ifdef INDEX_BUFFERING
            last.node_ = (struct TTreeNode *) Index_OID_Point(last.self_);
            if (last.node_ == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }
#endif
            rec_oid = last.node_->item_[last.idx_];
            record = (char *) OID_Point(rec_oid);
            if (record == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }

            SetBufSegment(issys, rec_oid);

            comp_res = KeyValue_Compare(minkey, record, rec_size);

            UnsetBufSegment(issys);

            if (comp_res > 0)
            {
                ret = DB_SUCCESS;       /* empty result */
                goto end;
            }
        }

        curr = first;
        while (1)
        {
#ifdef INDEX_BUFFERING
            curr.node_ = (struct TTreeNode *) Index_OID_Point(curr.self_);
            if (curr.node_ == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }
#endif
            rec_oid = curr.node_->item_[curr.idx_];
            record = (char *) OID_Point(rec_oid);
            if (record == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }

            SetBufSegment(issys, rec_oid);

            for (i = 0; i < 1; i++)
            {
                if (filter)
                {       /* do data filtering */
                    if (Filter_Compare(filter, record, rec_size) != DB_SUCCESS)
                        break;
                }



                ret = TTree_InsertRecord(new_ttree, record, rec_oid, 1);


                if (ret != DB_SUCCESS)
                {
                    UnsetBufSegment(issys);
                    MDB_SYSLOG(("Build_Index: Insert Key FAIL\n"));
                    goto end;
                }
            }

            UnsetBufSegment(issys);

            if (TTree_Find_Next_Without_Filter(&curr, &last) < 0)
            {
                /* No more records */
                break;
            }
        }
    }

    ret = DB_SUCCESS;

  end:
    UnsetBufSegment(issys_ttree);

    return ret;
}

/*****************************************************************************/
//! Create_Index
/*! \breif  index를 생성시 사용
 ************************************
 * \param indexName(in)     : index 이름
 * \param indexNo(in)       :
 * \param indexType(in)     : 
 * \param cont_name(in)     : 
 * \param keydesc(in)       : 
 * \param keyins_flag(in)   : 
 * \param scandesc(in)      : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *  - called : dbi_Index_Create()
 *      
 *****************************************************************************/
int
Create_Index(char *indexName, int indexNo, int indexType, char *cont_name,
        struct KeyDesc *keydesc, int keyins_flag, SCANDESC_T * scandesc)
{
    OID new_slot_oid;
    struct TTree *ttree;
    struct Container *Cont;
    OID index_id;
    int ret;
    int issys_cont;
    struct IndexCatalog *idx_catalog;

    Cont = (struct Container *) Cont_Search(cont_name);
    if (Cont == NULL)
        return DB_E_UNKNOWNTABLE;

    SetBufSegment(issys_cont, Cont->collect_.cont_id_);

    ttree = NULL;
    index_id = Cont->base_cont_.index_id_;
    while (index_id != NULL_OID)
    {
        ttree = (struct TTree *) OID_Point(index_id);
        if (ttree == NULL)
        {
            ret = DB_E_OIDPOINTER;
            goto end;
        }

        if (ttree->base_cont_.id_ == indexNo)
            break;

        index_id = ttree->base_cont_.index_id_;
        ttree = NULL;
    }

    if (ttree != NULL)
    {
        MDB_SYSLOG(("Index Create FAIL since index_id[%d] already used\n",
                        indexNo));
        MDB_SYSLOG(("\t Recovery...\n"));

        Drop_Index(cont_name, (DB_INT16) indexNo);
    }

    if (!isTempTable(Cont))
    {
        OID_SaveAsBefore(RELATION, (const void *) Cont,
                Cont->collect_.cont_id_ + sizeof(struct Collection),
                (int) sizeof(struct BaseContainer));
    }

    if (isTempIndex_name(indexName))
    {
        idx_catalog = (struct IndexCatalog *) temp_index_catalog;
    }
    else
    {
        idx_catalog = index_catalog;
    }

    ret = Collect_AllocateSlot((struct Container *) idx_catalog,
            &idx_catalog->collect_, &new_slot_oid, 1);
    if (ret < 0)
    {
        MDB_SYSLOG(("Index Collect Allocate Slot FAIL \n"));

        goto end;
    }

#ifdef MDB_DEBUG
    if ((isTempIndex_name(indexName) && !isTemp_OID(new_slot_oid)) ||
            (!isTempIndex_name(indexName) && isTemp_OID(new_slot_oid)))
    {
        sc_assert(0, __FILE__, __LINE__);
    }
#endif

    // TTREE를 생성한다.
    ret = TTree_Create(indexName, indexNo, indexType, new_slot_oid,
            keydesc, Cont);
    if (ret < 0)
    {
        MDB_SYSLOG((" TTree Create FAIL \n"));

        goto end;
    }

    if (!isTempTable(Cont))
    {
        OID_SaveAsAfter(RELATION, (const void *) Cont,
                Cont->collect_.cont_id_ + sizeof(struct Collection),
                (int) sizeof(struct BaseContainer));
    }

    if (keyins_flag)
    {
        ret = Build_Index(Cont, new_slot_oid, scandesc);
        if (ret < 0)
        {
            MDB_SYSLOG(("Build Index After Create FAIL \n"));
            Drop_Index(cont_name, (DB_INT16) indexNo);
            goto end;
        }
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
Index_Remove_In_Relation(void *Pointer, char *record, OID record_id)
{
    struct Container *Cont;
    struct TTree *ttree;
    int result = DB_SUCCESS;
    OID index_id;
    int end_of_rem_flag;
    int issys_ttree;

    Cont = (struct Container *) Pointer;

    end_of_rem_flag = 0;

    index_id = Cont->base_cont_.index_id_;
    while (index_id != NULL_OID)
    {
        if (isTemp_OID(index_id))
        {
            break;
        }

        ttree = (struct TTree *) OID_Point(index_id);
        if (ttree == NULL)
        {
            return DB_E_OIDPOINTER;
        }

        if (isPartialIndex_name(ttree->base_cont_.name_))
        {
            index_id = ttree->base_cont_.index_id_;
            continue;
        }

        SetBufSegment(issys_ttree, index_id);

        if (record && Cont->collect_.item_count_ > TNODE_MAX_ITEM * 3)
            result = TTree_RemoveRecord(ttree, record, record_id);
        else
            result = TTree_RemoveRecord(ttree, NULL, record_id);

        UnsetBufSegment(issys_ttree);

        if (result < 0)
        {
            if (result == DB_E_NOMORERECORDS)
            {
                if (ttree->key_desc_.is_unique_ != TRUE)
                {
                    MDB_SYSLOG(
                            ("NOTE: To-Be-Removed Key Not Found. (%s %s %lu)\n",
                                    Cont->base_cont_.name_,
                                    ttree->base_cont_.name_, record_id));
                }
                end_of_rem_flag = 1;
            }
            else
            {
            }
        }
        else
        {   /* success */
            if (end_of_rem_flag)
            {
                MDB_SYSLOG(("NOTE: Hole Index in Removing Keys. (%s %s %lu)\n",
                                Cont->base_cont_.name_,
                                ttree->base_cont_.name_, record_id));
            }
        }

        index_id = ttree->base_cont_.index_id_;
    }


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
Index_Insert_In_Relation(void *Pointer, char *record, OID record_id)
{
    struct Container *Cont;
    struct TTree *ttree;
    OID index_id;
    int result, found;
    int try_count = 0;
    int need_retry_flag;
    int end_of_ins_flag;
    int issys_ttree;

    Cont = (struct Container *) Pointer;

  again:
    try_count += 1;
    need_retry_flag = 0;
    end_of_ins_flag = 0;

    index_id = Cont->base_cont_.index_id_;
    while (index_id != NULL_OID)
    {
        if (isTemp_OID(index_id))
        {
            break;
        }

        ttree = (struct TTree *) OID_Point(index_id);
        if (ttree == NULL)
        {
            return DB_E_OIDPOINTER;
        }

        if (isPartialIndex_name(ttree->base_cont_.name_))
        {
            index_id = ttree->base_cont_.index_id_;
            continue;
        }

        SetBufSegment(issys_ttree, index_id);

        result = TTree_FindRecord(ttree, record, record_id, &found);
        if (result < 0)
        {
            MDB_SYSLOG(("Find a Key in Relation FAIL. (%s %s %d %d)\n",
                            Cont->base_cont_.name_, ttree->base_cont_.name_,
                            record_id, result));
            if (result == DB_E_INDEXNODELOOP)
            {
                extern int f_checkpoint_finished;

                f_checkpoint_finished = CHKPT_BEGIN;
                return DB_SUCCESS;
            }
            need_retry_flag = 1;
        }
        else
        {
            if (found == TRUE)
            {
                MDB_SYSLOG(("NOTE: To-Be-Inserted Key Found. (%s %s %d)\n",
                                Cont->base_cont_.name_,
                                ttree->base_cont_.name_, record_id));
                end_of_ins_flag = 1;
            }
            else
            {
                if (end_of_ins_flag == 1)
                {
                    MDB_SYSLOG(
                            ("NOTE: Hole Index in Inserting Keys. (%s %s %d)\n",
                                    Cont->base_cont_.name_,
                                    ttree->base_cont_.name_, record_id));
                }
                result = TTree_InsertRecord(ttree, record, record_id, 0);
                if (result < 0)
                {
                    MDB_SYSLOG(
                            ("Insert a Key in Relation FAIL. (%s %s %d %d)\n",
                                    Cont->base_cont_.name_,
                                    ttree->base_cont_.name_, record_id,
                                    result));
                    need_retry_flag = 1;
                }
            }
        }

        UnsetBufSegment(issys_ttree);

        index_id = ttree->base_cont_.index_id_;
    }

    if (need_retry_flag && try_count < 3)
    {
        MDB_SYSLOG(("Retry(count=%d) Inserting Keys in Relation.\n",
                        try_count));
        goto again;
    }

    return DB_SUCCESS;
}
