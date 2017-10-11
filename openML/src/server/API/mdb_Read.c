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
#include "mdb_TTreeNode.h"
#include "mdb_PMEM.h"

#include "sql_datast.h"
#include "mdb_FieldDesc.h"

OID Iter_Current(struct Iterator *iter);

//////////////////////////////////////////////////////////////////
//
// Function Name : __READ API
//
//////////////////////////////////////////////////////////////////
/*****************************************************************************/
//! __Read 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param cursor_id(in)     :
 * \param record(in/out)    : 
 * \param rec_values(in/out): 
 * \param direction         :
 * \param size(in/out)      :
 * \param page(in)          :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - called : dbi_Cursor_Read()
 *****************************************************************************/
int
__Read(DB_INT32 cursor_id, char *record, FIELDVALUES_T * rec_values,
        int direction, DB_UINT32 * size, OID page)
{
    register OID record_id;
    register struct Iterator *Iter;
    SYSTABLE_T *table_info = NULL;
    SYSFIELD_T *fields_info = NULL;
    int ret;

    Iter = (struct Iterator *) Iter_Attach(cursor_id, 1, page);
    if (Iter == NULL)
    {
        if (er_errid() != DB_E_TRANSINTERRUPT)
            return DB_E_INVALIDCURSORID;
        else
            return DB_E_TRANSINTERRUPT;
    }

    if (Iter->type_ == TTREE_FORWARD_CURSOR ||
            Iter->type_ == TTREE_BACKWARD_CURSOR)
    {
        struct TTreeIterator *ttreeIter = (struct TTreeIterator *) Iter;
        struct TTree *ttree = (struct TTree *)
                OID_Point(((struct TTreeIterator *) Iter)->tree_oid_);

        // TTreeIter time  == TTree time --> OK. TTree not changed. Go on.
        // otherwise we might not find next OID. So, be carefull. 
        if (ttreeIter->modified_time_ != ttree->modified_time_)
        {
            struct Trans *trans = Iter->trans_ptr;

            // check if this transaction is READ UNCOMMITTED
            // & USE RESULT
            if (trans->fRU == 1 && trans->fUseResult == 1)
            {
                // Now, we faced that READ UNCOMMITTED index scan and the
                // index changed after last read. Segment fault might occur
                // if we go on. Let's withdraw.
                MDB_SYSLOG(
                        ("Index modification occurred during READ UNCOMMITTED transaction. Abandon SELECT operation with ISCAN.\n"));

                return DB_E_INDEXCHANGED;
            }
        }

        // note: someday, we could utilize TTree_Restart(), TTree_RealNext()
        //       with READ UNCOMMITTED read operation.
    }

    /* get real table schema */
    table_info = Cursor_Mgr->cursor[cursor_id - 1].table_info;
    fields_info = Cursor_Mgr->cursor[cursor_id - 1].fields_info;

    if (Iter->rec_leng)
    {
        *size = Iter->rec_leng;
    }
    else if (table_info && table_info->has_variabletype)
    {
        Iter->rec_leng =
                *size =
                REC_ITEM_SIZE(table_info->recordLen, table_info->numFields);
    }
    else
    {
        Iter->rec_leng = *size = Iter->item_size_;
    }

    /* if record is NULL, just return after attaching cursor and
     * setting record item size */
    if (record == NULL && rec_values == NULL && direction == 0)
        return DB_SUCCESS;      /* size 설정후 return */

    record_id = Iter_Current(Iter);

    if (Iter->type_ != SEQ_CURSOR && Iter->real_next)
    {
        if (Iter->real_next != record_id)
        {
            TTree_Restart((struct TTreeIterator *) Iter);
            TTree_RealNext((struct TTreeIterator *) Iter, Iter->real_next);

            record_id = Iter_Current(Iter);
        }

        Iter->real_next = NULL_OID;
    }


    if (record_id == NULL_OID)
        return DB_E_NOMORERECORDS;

    if (record)
    {
        ret = Col_Read(Iter->item_size_, record_id, record);
        if (ret < 0)
            goto end;
    }

    if (rec_values && rec_values->fld_cnt > 0)
    {
        rec_values->rec_oid = record_id;
        // evaluation 값만 가져간다
        ret = Col_Read_Values(Iter->record_size_, record_id, rec_values, 1);
        if (ret < 0)
            goto end;
    }

    switch (direction)
    {
    case -2:   /* backward circular */
        Iter_FindPrev(Iter, page);
        if (Iter_Current(Iter) == NULL_OID)
            Iter_FindLast(Iter);
        break;
    case -1:   /* backward */
        Iter_FindPrev(Iter, page);
        break;
    case 0:    /* do not move */
        break;
    case 1:    /* forward */
        Iter_FindNext(Iter, page);
        break;
    case 2:    /* forward circular */
        Iter_FindNext(Iter, page);
        if (Iter_Current(Iter) == NULL_OID)
            Iter_FindFirst(Iter);
        break;
    }

    if (record)
    {
        if (table_info && table_info->has_variabletype)
        {
            make_memory_record(Iter->numfields_, Iter->item_size_,
                    Iter->record_size_, Iter->memory_record_size_,
                    fields_info, record, record, Iter->rec_leng, NULL);
        }
    }

    ret = DB_SUCCESS;

  end:
    return ret;
}

/*****************************************************************************/
//! Read_Itemsize 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param cursor_id :
 * \param size : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
Read_Itemsize(DB_INT32 cursor_id, DB_UINT32 * size)
{
    struct Iterator *Iter;
    SYSTABLE_T *table_info = NULL;
    SYSFIELD_T *fields_info = NULL;

    Iter = (struct Iterator *) Iter_Attach(cursor_id, 0, 0);
    if (Iter == NULL)
    {
        if (er_errid() != DB_E_TRANSINTERRUPT)
            return DB_E_INVALIDCURSORID;
        else
            return DB_E_TRANSINTERRUPT;
    }

    Cursor_get_schema_info(cursor_id, &table_info, &fields_info);

    if (Iter->rec_leng)
    {
        *size = Iter->rec_leng;
    }
    else if (table_info && table_info->has_variabletype)
    {
        Iter->rec_leng =
                *size =
                REC_ITEM_SIZE(table_info->recordLen, table_info->numFields);
    }
    else
    {
        Iter->rec_leng = *size = Iter->item_size_;
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! make_memory_record
/*! \breif Convert heap record to memeory record.
 ************************************
 * \param table_info    : internal table's schema infomation.
 * \param nfields_info  : internal fields' schema infomations. 
 * \param m_record      : dest. record.
 * \param h_record      : src. record(to be converted).
 ************************************
 * \return m_record: memory format record. 
 ************************************
 * \note
 *  - 참고
 *      memory record : DB API에서 사용하는 record format
 *      heap record : DB Engine에서 저장시 record format
 *  - reference record format : _Schema_CheckFieldDesc()
 *  - memory record format 수정
 *****************************************************************************/
char *
make_memory_record(DB_UINT16 cont_numfields, DB_INT32 cont_item_size,
        DB_UINT16 cont_record_size,
        DB_UINT32 cont_memory_record_size, SYSFIELD_T * nfields_info,
        char *m_record, char *h_record, int m_item_size, int *copy_not_cols)
{
    DB_INT32 nFields;
    DB_INT32 h_rec_len;         /* heap record length */
    DB_INT32 rec_len;           /* record length      */
    DB_INT32 h_item_size;       /* heap item size     */
    DB_INT32 item_size;         /* item size          */
    char *ptr;
    char *h_ptr;
    char *alloc_h_record = NULL;

    /* char       alloc_h_record[PAGE_BODY_SIZE]; */
    DB_INT32 i;
    DB_INT32 defined_len;

    if (copy_not_cols == NULL)
    {
        alloc_h_record = (char *) PMEM_ALLOC(PAGE_BODY_SIZE);
        if (alloc_h_record == NULL)
            return NULL;
    }

    nFields = cont_numfields;
    h_item_size = cont_item_size;
    if (m_item_size)
    {
        item_size = m_item_size;
    }
    else
    {
        item_size = REC_ITEM_SIZE(cont_memory_record_size, nFields);
    }
    h_rec_len = cont_record_size;
    rec_len = cont_memory_record_size;

    if (copy_not_cols == NULL)
    {
        if (m_record == h_record)
        {
            sc_memcpy(alloc_h_record, h_record, h_item_size);
            h_record = alloc_h_record;
        }

        sc_memset(m_record, 0, item_size);
        sc_memcpy(m_record + rec_len, h_record + h_rec_len,
                h_item_size - h_rec_len);
    }

    for (i = 0; i < nFields; i++)
    {
        ptr = m_record + nfields_info[i].offset;
        h_ptr = h_record + nfields_info[i].h_offset;
        defined_len = nfields_info[i].length;

        if (copy_not_cols && copy_not_cols[i])
            continue;

        if (DB_VALUE_ISNULL(h_record, nfields_info[i].position, h_rec_len))
        {
            DB_VALUE_SETNULL(m_record, nfields_info[i].position, rec_len);
            continue;
        }

        DB_VALUE_SETNOTNULL(m_record, nfields_info[i].position, rec_len);
        if (IS_VARIABLE_DATA(nfields_info[i].type, nfields_info[i].fixed_part))
        {
            int buflen = DB_BYTE_LENG(nfields_info[i].type, defined_len);

            /* fix varbyte record */
            if (nfields_info[i].type == DT_VARBYTE)
            {
                defined_len = dbi_strcpy_variable(ptr + INT_SIZE, h_ptr,
                        nfields_info[i].fixed_part, nfields_info[i].type,
                        buflen);
                sc_memcpy(ptr, &defined_len, INT_SIZE);
            }
            else
                dbi_strcpy_variable(ptr, h_ptr, nfields_info[i].fixed_part,
                        nfields_info[i].type, buflen);
        }
        else
        {
            if (nfields_info[i].type == DT_DECIMAL)
                defined_len += 3;
            else if (IS_BYTE(nfields_info[i].type))
                defined_len += 4;

            defined_len = DB_BYTE_LENG(nfields_info[i].type, defined_len);
            sc_memcpy(ptr, h_ptr, defined_len);
        }
    }

    if (copy_not_cols == NULL)
        PMEM_FREENUL(alloc_h_record);

    return m_record;
}

int
Find_Seek(DB_INT32 cursor_id, int offset)
{
    struct Iterator *Iter;
    struct TTreeIterator *ttreeiter;

    Iter = (struct Iterator *) Iter_Attach(cursor_id, 1, 0);

    if (Iter == NULL)
    {
        if (er_errid() != DB_E_TRANSINTERRUPT)
            return DB_E_INVALIDCURSORID;
        else
            return DB_E_TRANSINTERRUPT;
    }

    if (Iter->type_ == SEQ_CURSOR)
    {
        return DB_E_INVALIDPARAM;
    }

    ttreeiter = (struct TTreeIterator *) Iter;

    if (ttreeiter->iterator_.type_ == TTREE_BACKWARD)
    {
        struct Cursor *csr;
        struct Container *cont;

        csr = GET_CURSOR_POINTER(cursor_id);

        cont = (struct Container *) OID_Point(csr->cont_id_);
        if (cont == NULL)
        {
            return DB_E_UNKNOWNTABLE;
        }

        offset = cont->collect_.item_count_ - offset - 1;
    }

    return TTree_Seek_Offset((struct TTree *) OID_Point(ttreeiter->tree_oid_),
            &ttreeiter->curr_, offset);
}

/*****************************************************************************/
//! Direct_Find_Record 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param cont :
 * \param indexName : 
 * \param findKey :
 * \param record_id :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
Direct_Find_Record(struct Container *cont, char *indexName,
        struct KeyValue *findKey, OID * record_id)
{
    OID index_id;
    int i, j;
    struct TTree *ttree;
    struct TTreePosition first;
    int issys_ttree;

    /* find index oid from index name */
    index_id = cont->base_cont_.index_id_;
    while (index_id != NULL_OID)
    {
        ttree = (struct TTree *) OID_Point(index_id);
        if (ttree == NULL)
        {
            return DB_E_OIDPOINTER;
        }
        if (ttree->key_desc_.is_unique_)
        {
            if (!mdb_strcmp(indexName, ttree->base_cont_.name_))
                break;
        }
        index_id = ttree->base_cont_.index_id_;
    }
    if (index_id == NULL_OID)
    {
        return DB_E_UNKNOWNINDEX;
    }

    if (findKey->field_count_ != ttree->key_desc_.field_count_)
    {
        return DB_E_CURSORKEYDESC;
    }

    for (i = 0; i < findKey->field_count_; i++)
    {
        if (findKey->field_value_[i].mode_ != MDB_NN)
            return DB_E_INVALIDPARAM;

        for (j = 0; j < (ttree->key_desc_.field_count_); j++)
        {
            if (findKey->field_value_[i].position_ ==
                    ttree->key_desc_.field_desc_[j].position_)
            {
                findKey->field_value_[i].order_
                        = ttree->key_desc_.field_desc_[j].order_;
                findKey->field_value_[i].h_offset_
                        = ttree->key_desc_.field_desc_[j].h_offset_;
                findKey->field_value_[i].fixed_part
                        = ttree->key_desc_.field_desc_[j].fixed_part;
            }
        }
    }

    SetBufSegment(issys_ttree, index_id);

    i = TTree_Get_First_Eq_Unique(ttree, findKey, &first);

    UnsetBufSegment(issys_ttree);

    if (i < 0)
        return DB_E_NOMORERECORDS;

#ifdef INDEX_BUFFERING
    first.node_ = (struct TTreeNode *) Index_OID_Point(first.self_);
    if (first.node_ == NULL)
    {
        return DB_E_OIDPOINTER;
    }
#endif

    *record_id = first.node_->item_[first.idx_];
    return DB_SUCCESS;
}

/*****************************************************************************/
//! Direct_Read_Internal 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param trans :
 * \param cont :
 * \param indexName : 
 * \param findKey :
 * \param lock :
 * \param record :
 * \param size :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - __Read는 항상 evaluation 값을 가져간다
 *****************************************************************************/
int
Direct_Read_Internal(struct Trans *trans, struct Container *cont,
        char *indexName, struct KeyValue *findKey,
        OID rid_to_read,
        LOCKMODE lock, char *record, DB_UINT32 * size,
        FIELDVALUES_T * rec_values)
{
    DB_UINT32 item_size;
    OID record_id;
    int result = DB_SUCCESS;
    SYSFIELD_T *fields_info;

    if (cont->base_cont_.has_variabletype_)
    {
        item_size = REC_ITEM_SIZE(cont->base_cont_.memory_record_size_,
                cont->collect_.numfields_);
    }
    else
    {
        item_size = cont->collect_.item_size_;
    }

    if (record == NULL && rec_values == NULL)
    {
        *size = item_size;
        return DB_SUCCESS;
    }

    if (*size > 0 && *size < item_size)
    {
        *size = item_size;
        return DB_E_TOOSMALLRECBUF;
    }

    *size = item_size;

    if (rid_to_read != NULL_OID)
        record_id = rid_to_read;
    else
        result = Direct_Find_Record(cont, indexName, findKey, &record_id);

    if (result < 0)
    {
        return result;
    }

    /* get fields information */
    fields_info = (SYSFIELD_T *)
            PMEM_ALLOC(sizeof(SYSFIELD_T) * cont->collect_.numfields_);

    if (fields_info == NULL)
    {
        return DB_E_OUTOFMEMORY;
    }

    result = _Schema_GetFieldsInfo(cont->base_cont_.name_, fields_info);

    if (result < 0)
    {
        PMEM_FREENUL(fields_info);
        return result;
    }

    if (rec_values && rec_values->fld_cnt > 0)
    {
        rec_values->rec_oid = record_id;
        // evaluation 값을 가지고 온다
        result = Col_Read_Values(cont->collect_.record_size_, record_id,
                rec_values, 1);
    }

    if (!record)
    {
        PMEM_FREENUL(fields_info);
        return result;
    }

    result = Col_Read(cont->collect_.item_size_, record_id, record);

    if (result < 0)
    {
        PMEM_FREENUL(fields_info);
        return result;
    }
    if (cont->base_cont_.has_variabletype_)
    {
        make_memory_record(cont->collect_.numfields_,
                cont->collect_.item_size_,
                cont->collect_.record_size_,
                cont->base_cont_.memory_record_size_,
                fields_info, record, record, 0, NULL);
    }

    PMEM_FREENUL(fields_info);

    return DB_SUCCESS;
}

int
Find_GetRid(DB_INT32 cursor_id, OID * rid)
{
    struct Iterator *Iter;

    Iter = (struct Iterator *) Iter_Attach(cursor_id, 1, 0);

    if (Iter == NULL)
    {
        if (er_errid() != DB_E_TRANSINTERRUPT)
            return DB_E_INVALIDCURSORID;
        else
            return DB_E_TRANSINTERRUPT;
    }

    *rid = Iter_Current(Iter);

    return DB_SUCCESS;
}
