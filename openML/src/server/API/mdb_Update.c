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
#include "mdb_PMEM.h"

int Update_heap_fields(struct Container *cont, SYSFIELD_T * fields_info,
        char *new_record, struct UpdateValue *newValues, int *auto_newVal_pos);
int Remove_some_variables(struct Container *cont, SYSFIELD_T * fields_info,
        char *old_record, struct UpdateValue *newValues);

extern int Direct_Find_Record(struct Container *cont, char *indexName,
        struct KeyValue *findKey, OID * record_id);
extern int Check_NullFields(int record_size, struct KeyDesc *pKeyDesc,
        char *item, int limit_pos);
extern int make_heap_record(struct Container *Cont, SYSFIELD_T * nfields_info,
        char *record, char *h_record, int *col_list);
extern int Remove_variables(struct Container *container,
        SYSFIELD_T * nfields_info, OID rid, char *record,
        MDB_OIDARRAY * v_oidset, int *col_list, int remove_msync_slot);

void OID_SaveAsAfter(short Op_Type, const void *Relation, OID id_, int size);

/////////////////////////////////////////////////////////////////
//
// Function Name : UPDATE API
//
/////////////////////////////////////////////////////////////////

/*****************************************************************************/
//! __Update 
/*! \breif  UPDATE Function
 ************************************
 * \param cursor_id(in) :
 * \param data(in)      : memory record
 * \param data_leng(in) : memory record's byte length
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
__Update(DB_INT32 cursor_id, char *data, int data_leng, int *col_list)
{
    struct Iterator *Iter;
    SYSTABLE_T *table_info = NULL;
    SYSFIELD_T *fields_info = NULL;
    char *rec_buf = NULL;
    char *old_record = NULL;
    char *new_record;
    OID record_id;
    OID index_id;
    struct TTree *ttree;
    DB_INT32 null_count;
    DB_INT32 ret_val;
    char *new_data = NULL;
    char *pbuf = NULL;
    struct Container *Cont;
    int issys_cont;
    int issys_ttree;
    int i;
    struct Update_Idx_Id_Array *pUpd_idx_id_array = NULL;
    struct Cursor *pCursor;

    Iter = (struct Iterator *) Iter_Attach(cursor_id, 1, 0);
    if (Iter == NULL)
    {
        if (er_errid() != DB_E_TRANSINTERRUPT)
            return DB_E_INVALIDCURSORID;
        else
            return DB_E_TRANSINTERRUPT;
    }

    if (Iter->lock_mode_ == LK_SHARED || Iter->lock_mode_ == LK_SCHEMA_SHARED)
        return DB_E_INVALIDLOCKMODE;

    Cont = (struct Container *) OID_Point(Iter->cont_id_);
    SetBufSegment(issys_cont, Iter->cont_id_);

    if (Iter->type_ != SEQ_CURSOR && Iter->real_next)
    {
        TTree_Restart((struct TTreeIterator *) Iter);
        TTree_RealNext((struct TTreeIterator *) Iter, Iter->real_next);

        Iter->real_next = NULL_OID;
    }

    record_id = Iter_Current(Iter);

    if (Cont->base_cont_.is_dirty_ == 1)
    {
        Cont->base_cont_.is_dirty_ = 0;
    }

    if (record_id == NULL_OID)
    {
        ret_val = DB_E_NOMORERECORDS;
        goto end;
    }

    Cursor_get_schema_info(cursor_id, &table_info, &fields_info);

    if (Cont->base_cont_.auto_fld_pos_ > -1)
    {
        if (!DB_VALUE_ISNULL(data, Cont->base_cont_.auto_fld_pos_,
                        Cont->base_cont_.memory_record_size_))
        {
            int auto_value;

            auto_value = *(int *) (data + Cont->base_cont_.auto_fld_offset_);
            if (Cont->base_cont_.max_auto_value_ < auto_value)
            {
                Cont->base_cont_.max_auto_value_ = auto_value;
                SetDirtyFlag(Cont->collect_.cont_id_);
                OID_SaveAsAfter(AUTOINCREMENT, Cont,
                        Cont->collect_.cont_id_ +
                        (unsigned long) &Cont->base_cont_.max_auto_value_ -
                        (unsigned long) Cont,
                        sizeof(Cont->base_cont_.max_auto_value_));
            }
        }
    }

    /* new record의 variable 부분을 insert 하기 전에 처리 */
    index_id = Cont->base_cont_.index_id_;
    if (index_id)
    {
        pCursor = GET_CURSOR_POINTER(cursor_id);

        pUpd_idx_id_array = pCursor->pUpd_idx_id_array;
    }

    if (pUpd_idx_id_array)
    {
        i = 0;
        if (i < pUpd_idx_id_array->cnt)
        {
            index_id = pUpd_idx_id_array->idx_oid[i];
        }
        else
        {
            index_id = NULL_OID;
        }
    }

    if (index_id)
    {
        ttree = (struct TTree *) OID_Point(index_id);
        if (ttree == NULL)
        {
            ret_val = DB_E_OIDPOINTER;
            goto end;
        }

        if (ttree->key_desc_.is_unique_ == TRUE)
        {
            null_count = Check_NullFields(
                    /* memory record size for Check_NullFields */
                    Cont->base_cont_.memory_record_size_,
                    &(ttree->key_desc_), data, -1);

            if (null_count == -1)
            {
                ret_val = DB_E_NOTNULLFIELD;
                goto end;
            }

            SetBufSegment(issys_ttree, index_id);

            if (null_count == 0 && TTree_IsExist(ttree, data, record_id,
                            /* memory record size for Check_NullFields */
                            Cont->base_cont_.memory_record_size_,
                            1) != NULL_OID)
            {
                UnsetBufSegment(issys_ttree);
                ret_val = DB_E_DUPUNIQUEKEY;
                goto end;
            }

            UnsetBufSegment(issys_ttree);
        }

        if (pUpd_idx_id_array)
        {
            ++i;
            if (i < pUpd_idx_id_array->cnt)
            {
                index_id = pUpd_idx_id_array->idx_oid[i];
            }
            else
            {
                index_id = NULL_OID;
            }
        }
        else
        {
            index_id = ttree->base_cont_.index_id_;
        }
    }

    if (Cont->base_cont_.has_variabletype_)
    {
        rec_buf = (char *) PMEM_ALLOC(Cont->collect_.slot_size_ * 2);
        if (rec_buf == NULL)
        {
            ret_val = DB_E_OUTOFMEMORY;
            goto end;
        }

        old_record = &rec_buf[0];
        new_record = &rec_buf[Cont->collect_.slot_size_];

        /* save old heap record image */
        pbuf = (char *) OID_Point(record_id);
        if (pbuf == NULL)
        {
            ret_val = DB_E_OIDPOINTER;
            goto end;
        }

        sc_memcpy(old_record, pbuf, Cont->collect_.slot_size_);
        if (col_list)
        {
            sc_memcpy(new_record, pbuf, Cont->collect_.slot_size_);
        }

        /* make new heap record image */
        ret_val = make_heap_record(Cont, fields_info, data, new_record,
                col_list);
        if (ret_val < 0)
        {
            MDB_SYSLOG(("ERROR: Make Heap Record.\n"));
            goto end;
        }
        data = new_record;
    }
    else
    {
        if (data_leng < Cont->collect_.item_size_)
        {
            rec_buf = (char *) PMEM_ALLOC(Cont->collect_.item_size_);
            if (rec_buf == NULL)
            {
                ret_val = DB_E_OUTOFMEMORY;
                goto end;
            }
            sc_memcpy(rec_buf, data, data_leng);
            data = rec_buf;
        }
    }


    Iter_FindNext(Iter, 0);

    ret_val = Col_Update_Record(Cont, record_id, data, Iter->open_time_,
            Iter->qsid_, pUpd_idx_id_array, 0);
    if (ret_val < 0)
    {
        goto end;
    }

    /* remove previous variable record(s) of the record to update */

    if (Cont->base_cont_.has_variabletype_ && col_list)
    {
        ret_val = Remove_variables(Cont, fields_info, NULL_OID, old_record,
                NULL, col_list, 1);
        if (ret_val != DB_SUCCESS)
            goto end;
    }


    /* 해당 cursor가 update 수행되었다는걸 나타냄 */

    ret_val = DB_SUCCESS;

  end:
    UnsetBufSegment(issys_cont);
    PMEM_FREENUL(rec_buf);

    PMEM_FREENUL(new_data);

    return ret_val;
}

/*****************************************************************************/
//! Update_Field 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param cursor_id :
 * \param newValues : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - called : dbi_Cursor_Update_Field()
 *      및 기타 설명
 *****************************************************************************/
int
Update_Field(DB_UINT32 cursor_id, struct UpdateValue *newValues)
{
    struct Iterator *Iter;
    SYSTABLE_T *table_info = NULL;
    SYSFIELD_T *fields_info = NULL;
    char *rec_buf = NULL;
    char *old_record = NULL;
    char *new_record;
    OID record_id;
    OID index_id;
    struct TTree *ttree;
    DB_INT32 null_count;
    DB_INT32 ret_val;
    DB_INT32 i;
    struct Update_Idx_Id_Array *pUpd_idx_id_array = NULL;
    struct Cursor *pCursor;
    char *pbuf = NULL;
    struct Container *Cont;
    int issys_cont;
    int issys_ttree;

    int auto_newVal_pos = -1;

    Iter = (struct Iterator *) Iter_Attach(cursor_id, 1, 0);
    if (Iter == NULL)
    {
        if (er_errid() != DB_E_TRANSINTERRUPT)
            return DB_E_INVALIDCURSORID;
        else
            return DB_E_TRANSINTERRUPT;
    }

    if (Iter->lock_mode_ == LK_SHARED || Iter->lock_mode_ == LK_SCHEMA_SHARED)
        return DB_E_INVALIDLOCKMODE;

    Cont = (struct Container *) OID_Point(Iter->cont_id_);
    SetBufSegment(issys_cont, Iter->cont_id_);

    if (Iter->type_ != SEQ_CURSOR && Iter->real_next)
    {
        TTree_Restart((struct TTreeIterator *) Iter);
        TTree_RealNext((struct TTreeIterator *) Iter, Iter->real_next);
        Iter->real_next = NULL_OID;
    }

    record_id = Iter_Current(Iter);

    if (Cont->base_cont_.is_dirty_ == 1)
    {
        Cont->base_cont_.is_dirty_ = 0;
    }

    if (record_id == NULL_OID)
    {
        ret_val = DB_E_NOMORERECORDS;
        goto end;
    }

    Cursor_get_schema_info(cursor_id, &table_info, &fields_info);
    if (table_info == NULL)
    {
        struct Cursor *pCursor;

        pCursor = &(Cursor_Mgr->cursor[cursor_id - 1]);

        pCursor->table_info = (SYSTABLE_T *) PMEM_ALLOC(sizeof(SYSTABLE_T));
        if (pCursor->table_info == NULL)
        {
            ret_val = CS_E_OUTOFMEMORY;
            goto end;
        }

        ret_val = _Schema_GetTableInfo(Cont->base_cont_.name_,
                pCursor->table_info);
        if (ret_val < 0)
        {
            PMEM_FREENUL(pCursor->table_info);
            goto end;
        }

        pCursor->fields_info = (SYSFIELD_T *)
                PMEM_ALLOC(sizeof(SYSFIELD_T) *
                pCursor->table_info->numFields);
        if (pCursor->fields_info == NULL)
        {
            PMEM_FREENUL(pCursor->table_info);
            ret_val = CS_E_OUTOFMEMORY;
            goto end;
        }

        ret_val = _Schema_GetFieldsInfo(Cont->base_cont_.name_,
                pCursor->fields_info);
        if (ret_val < 0)
        {
            PMEM_FREENUL(pCursor->table_info);
            PMEM_FREENUL(pCursor->fields_info);
            goto end;
        }

        table_info = pCursor->table_info;
        fields_info = pCursor->fields_info;
    }

    if (Cont->base_cont_.has_variabletype_)
    {
        rec_buf = (char *) PMEM_ALLOC(Cont->collect_.slot_size_ * 2);
        if (rec_buf == NULL)
        {
            ret_val = DB_E_OUTOFMEMORY;
            goto end;
        }
        old_record = &rec_buf[0];
        new_record = &rec_buf[Cont->collect_.slot_size_];

        /* save old heap record image */
        pbuf = (char *) OID_Point(record_id);
        if (pbuf == NULL)
        {
            ret_val = DB_E_OIDPOINTER;
            goto end;
        }

        sc_memcpy(old_record, pbuf, Cont->collect_.slot_size_);
    }
    else    /* does not have variable type field */
    {
        rec_buf = (char *) PMEM_ALLOC(Cont->collect_.slot_size_);
        if (rec_buf == NULL)
        {
            ret_val = DB_E_OUTOFMEMORY;
            goto end;
        }
        new_record = &rec_buf[0];
    }

    /* make new heap record image */
    pbuf = (char *) OID_Point(record_id);
    if (pbuf == NULL)
    {
        ret_val = DB_E_OIDPOINTER;
        goto end;
    }

    sc_memcpy(new_record, pbuf, Cont->collect_.slot_size_);

    ret_val = Update_heap_fields(Cont, fields_info, new_record,
            newValues, &auto_newVal_pos);
    if (ret_val < 0)
    {
        goto end;
    }

    if (auto_newVal_pos > -1)
    {
        if (!DB_VALUE_ISNULL(new_record,
                        newValues->update_field_value[auto_newVal_pos].pos,
                        Cont->base_cont_.memory_record_size_))
        {
            int auto_value;

            auto_value = *(int *)
                    (newValues->update_field_value[auto_newVal_pos].data);
            if (Cont->base_cont_.max_auto_value_ < auto_value)
            {
                Cont->base_cont_.max_auto_value_ = auto_value;
                SetDirtyFlag(Cont->collect_.cont_id_);
                OID_SaveAsAfter(AUTOINCREMENT, Cont,
                        Cont->collect_.cont_id_ +
                        (unsigned long) &Cont->base_cont_.max_auto_value_ -
                        (unsigned long) Cont,
                        sizeof(Cont->base_cont_.max_auto_value_));
            }
        }
    }

    /* find to-be-updated indexes while uniqueness checking */
    index_id = Cont->base_cont_.index_id_;
    if (index_id)
    {
        pCursor = GET_CURSOR_POINTER(cursor_id);

        pUpd_idx_id_array = pCursor->pUpd_idx_id_array;
    }

    if (pUpd_idx_id_array)
    {
        i = 0;
        if (i < pUpd_idx_id_array->cnt)
        {
            index_id = pUpd_idx_id_array->idx_oid[i];
        }
        else
        {
            index_id = NULL_OID;
        }
    }

    if (index_id)
    {
        ttree = (struct TTree *) OID_Point(index_id);
        if (ttree == NULL)
        {
            ret_val = DB_E_OIDPOINTER;
            goto end;
        }

        if (ttree->key_desc_.is_unique_ == TRUE)
        {
            null_count = Check_NullFields(Cont->collect_.record_size_,
                    &(ttree->key_desc_), new_record, -1);
            if (null_count == -1)
            {
                ret_val = DB_E_NOTNULLFIELD;
                goto end;
            }

            SetBufSegment(issys_ttree, index_id);

            if (null_count == 0 && TTree_IsExist(ttree, new_record, record_id,
                            Cont->collect_.record_size_, 0) != NULL_OID)
            {
                UnsetBufSegment(issys_ttree);
                ret_val = DB_E_DUPUNIQUEKEY;
                goto end;
            }

            UnsetBufSegment(issys_ttree);
        }

        if (pUpd_idx_id_array)
        {
            ++i;
            if (i < pUpd_idx_id_array->cnt)
            {
                index_id = pUpd_idx_id_array->idx_oid[i];
            }
            else
            {
                index_id = NULL_OID;
            }
        }
        else
        {
            index_id = ttree->base_cont_.index_id_;
        }
    }

    Iter_FindNext(Iter, 0);

    ret_val = Col_Update_Field(Cont, fields_info, newValues,
            record_id, new_record, Iter->open_time_, Iter->qsid_,
            pUpd_idx_id_array);
    if (ret_val < 0)
    {
        goto end;
    }

    if (Cont->base_cont_.has_variabletype_)
    {
        /* remove old variable data record(s) of the to-be-updated record */
        ret_val = Remove_some_variables(Cont, fields_info,
                old_record, newValues);
        if (ret_val < 0)
            goto end;
    }


    /* 해당 cursor가 update 수행되었다는걸 나타냄 */

    ret_val = DB_SUCCESS;

  end:
    UnsetBufSegment(issys_cont);

    PMEM_FREENUL(rec_buf);
    return ret_val;
}

/*****************************************************************************/
//! Direct_Update_Internal 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param trans :
 * \param cont : 
 * \param indexName :
 * \param findKey :
 * \param data : 
 * \param data_leng :
 * \param lock :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/

int
Direct_Update_Internal(struct Trans *trans, struct Container *cont,
        char *indexName, struct KeyValue *findKey, OID rid_to_update,
        char *data, int data_leng, LOCKMODE lock,
        struct UpdateValue *newValues, DB_UINT8 msync_flag)
{
    SYSFIELD_T *fields_info = NULL;
    char *rec_buf = NULL;
    char *old_record = NULL;
    char *new_record;
    OID record_id;
    OID index_id;
    struct TTree *ttree;
    DB_INT32 null_count;
    DB_INT32 ret_val = DB_SUCCESS;
    char *pbuf = NULL;
    int issys_ttree;
    int i, j;
    DB_INT32 index_changed;
    struct Update_Idx_Id_Array upd_idx_id_array;
    struct Update_Idx_Id_Array *pUpd_idx_id_array = NULL;
    int *col_list = NULL;

    if (rid_to_update != NULL_OID)
        record_id = rid_to_update;
    else
        ret_val = Direct_Find_Record(cont, indexName, findKey, &record_id);

    if (ret_val < 0)
    {
        return ret_val;
    }

    if (cont->base_cont_.auto_fld_pos_ > -1)
    {
        if (!DB_VALUE_ISNULL(data, cont->base_cont_.auto_fld_pos_,
                        cont->base_cont_.memory_record_size_))
        {
            int auto_value;

            auto_value = *(int *) (data + cont->base_cont_.auto_fld_offset_);
            if (cont->base_cont_.max_auto_value_ < auto_value)
            {
                int issys_cont;

                SetBufSegment(issys_cont, cont->collect_.cont_id_);
                cont->base_cont_.max_auto_value_ = auto_value;
                SetDirtyFlag(cont->collect_.cont_id_);
                OID_SaveAsAfter(AUTOINCREMENT, cont,
                        cont->collect_.cont_id_ +
                        (unsigned long) &cont->base_cont_.max_auto_value_ -
                        (unsigned long) cont,
                        sizeof(cont->base_cont_.max_auto_value_));
                UnsetBufSegment(issys_cont);
            }
        }
    }

    /* new record의 variable 부분을 insert 하기 전에 처리 */
    index_id = cont->base_cont_.index_id_;

    if (index_id && newValues)
    {
        upd_idx_id_array.cnt = 0;
        pUpd_idx_id_array = &upd_idx_id_array;
    }

    while (index_id)
    {
        ttree = (struct TTree *) OID_Point(index_id);
        if (ttree == NULL)
        {
            ret_val = DB_E_OIDPOINTER;
            goto error;
        }

        if (newValues)
        {
            index_changed = 0;

            for (i = 0; i < (ttree->key_desc_.field_count_); i++)
            {
                for (j = 0; j < (int) newValues->update_field_count; j++)
                {
                    if (ttree->key_desc_.field_desc_[i].position_
                            == newValues->update_field_value[j].pos)
                    {
                        index_changed = 1;
                        break;
                    }
                }

                if (index_changed)
                {
                    break;
                }
            }
        }
        else
        {
            index_changed = 1;
        }

        if (index_changed)
        {
            if (newValues)
            {
                pUpd_idx_id_array->idx_oid[pUpd_idx_id_array->cnt] = index_id;
                pUpd_idx_id_array->cnt += 1;
            }

            if (ttree->key_desc_.is_unique_ == TRUE)
            {
                null_count = Check_NullFields(
                        /* memory record size for Check_NullFields */
                        cont->base_cont_.memory_record_size_,
                        &(ttree->key_desc_), data, -1);
                if (null_count == -1)
                {
                    ret_val = DB_E_NOTNULLFIELD;
                    goto error;
                }
                SetBufSegment(issys_ttree, index_id);
                if (null_count == 0 && TTree_IsExist(ttree, data, record_id,
                                /* memory record size for Check_NullFields */
                                cont->base_cont_.memory_record_size_,
                                1) != NULL_OID)
                {
                    UnsetBufSegment(issys_ttree);
                    ret_val = DB_E_DUPUNIQUEKEY;
                    goto error;
                }
                UnsetBufSegment(issys_ttree);
            }
        }
        index_id = ttree->base_cont_.index_id_;
    }

    if (cont->base_cont_.has_variabletype_)
    {
        /* get fields information */
        fields_info = (SYSFIELD_T *)
                PMEM_ALLOC(sizeof(SYSFIELD_T) * cont->collect_.numfields_);
        if (fields_info == NULL)
        {
            return DB_E_OUTOFMEMORY;
        }

        ret_val = _Schema_GetFieldsInfo(cont->base_cont_.name_, fields_info);
        if (ret_val < 0)
        {
            MDB_SYSLOG(("ERROR: Unknown Table %s.\n", cont->base_cont_.name_));
            PMEM_FREENUL(fields_info);
            return ret_val;
        }

        rec_buf = (char *) PMEM_ALLOC(cont->collect_.slot_size_ * 2);
        if (rec_buf == NULL)
        {
            ret_val = DB_E_OUTOFMEMORY;
            goto error;
        }

        old_record = &rec_buf[0];
        new_record = &rec_buf[cont->collect_.slot_size_];

        /* save old heap record image */
        pbuf = (char *) OID_Point(record_id);
        if (pbuf == NULL)
        {
            ret_val = DB_E_OIDPOINTER;
            goto error;
        }

        sc_memcpy(old_record, pbuf, cont->collect_.slot_size_);

        if (newValues)
        {
            int k;

            col_list = (int *)
                    PMEM_ALLOC(sizeof(int) * (newValues->update_field_count +
                            1));
            if (col_list == NULL)
            {
                ret_val = DB_E_OUTOFMEMORY;
                goto error;
            }

            col_list[0] = newValues->update_field_count;

            for (k = 0; k < newValues->update_field_count; ++k)
            {
                col_list[k + 1] = newValues->update_field_value[k].pos;
            }
        }

        /* make new heap record image */
        ret_val = make_heap_record(cont, fields_info, data, new_record,
                col_list);
        if (ret_val < 0)
        {
            MDB_SYSLOG(("ERROR: Make Heap Record.\n"));
            goto error;
        }
        data = new_record;
    }
    else    /* does not have variable type field */
    {
        if (data_leng < cont->collect_.item_size_)
        {
            rec_buf = PMEM_ALLOC(cont->collect_.item_size_);
            if (rec_buf == NULL)
            {
                return DB_E_OUTOFMEMORY;
            }
            sc_memcpy(rec_buf, data, data_leng);
            data = rec_buf;
        }
    }

    ret_val =
            Col_Update_Record(cont, record_id, data, (DB_UINT32) sc_time(NULL),
            0, pUpd_idx_id_array, msync_flag);
    if (ret_val < 0)
    {
        goto error;
    }

    /* remove previous variable record(s) of the record to update */
    if (cont->base_cont_.has_variabletype_ && col_list)
    {
        ret_val = Remove_variables(cont, fields_info, NULL_OID, old_record,
                NULL, col_list, 1);
        if (ret_val != DB_SUCCESS)
            goto error;

    }

    PMEM_FREENUL(col_list);
    PMEM_FREENUL(fields_info);
    PMEM_FREENUL(rec_buf);
    return DB_SUCCESS;

  error:
    PMEM_FREENUL(col_list);
    PMEM_FREENUL(rec_buf);
    PMEM_FREENUL(fields_info);
    return ret_val;
}

/*****************************************************************************/
//! Direct_Update_Field_Internal 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param trans :
 * \param cont : 
 * \param indexName :
 * \param findKey :
 * \param newValues :
 * \param lock :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
Direct_Update_Field_Internal(struct Trans *trans, struct Container *cont,
        char *indexName, struct KeyValue *findKey,
        struct UpdateValue *newValues, LOCKMODE lock)
{
    SYSFIELD_T *fields_info;
    char *rec_buf = NULL;
    char *old_record = NULL;
    char *new_record;
    OID record_id;
    OID index_id;
    struct TTree *ttree;
    DB_INT32 index_changed;
    struct Update_Idx_Id_Array upd_idx_id_array;
    struct Update_Idx_Id_Array *pUpd_idx_id_array = NULL;
    DB_INT32 i, pos;
    DB_UINT32 j;
    DB_INT32 null_count;
    DB_INT32 ret_val;
    char *pbuf = NULL;
    int issys_ttree;
    int auto_newVal_pos = -1;

    /* find to-be-updated record */
    ret_val = Direct_Find_Record(cont, indexName, findKey, &record_id);
    if (ret_val < 0)
    {
        return ret_val;
    }

    /* get fields information */
    fields_info = (SYSFIELD_T *)
            PMEM_ALLOC(sizeof(SYSFIELD_T) * cont->collect_.numfields_);
    if (fields_info == NULL)
    {
        return DB_E_OUTOFMEMORY;
    }

    ret_val = _Schema_GetFieldsInfo(cont->base_cont_.name_, fields_info);
    if (ret_val < 0)
    {
        PMEM_FREENUL(fields_info);
        return ret_val;
    }

    if (cont->base_cont_.has_variabletype_)
    {
        rec_buf = (char *) PMEM_ALLOC(cont->collect_.slot_size_ * 2);
        if (rec_buf == NULL)
        {
            PMEM_FREENUL(fields_info);
            return DB_E_OUTOFMEMORY;
        }
        old_record = &rec_buf[0];
        new_record = &rec_buf[cont->collect_.slot_size_];

        /* save old heap record image */
        pbuf = (char *) OID_Point(record_id);
        if (pbuf == NULL)
        {
            ret_val = DB_E_OIDPOINTER;
            goto error;
        }

        sc_memcpy(old_record, pbuf, cont->collect_.slot_size_);
    }
    else
    {
        rec_buf = (char *) PMEM_ALLOC(cont->collect_.slot_size_);
        if (rec_buf == NULL)
        {
            PMEM_FREENUL(fields_info);
            return DB_E_OUTOFMEMORY;
        }
        new_record = &rec_buf[0];
    }

    /* make new heap record image */
    pbuf = (char *) OID_Point(record_id);
    if (pbuf == NULL)
    {
        ret_val = DB_E_OIDPOINTER;
        goto error;
    }

    sc_memcpy(new_record, pbuf, cont->collect_.slot_size_);

    ret_val = Update_heap_fields(cont, fields_info, new_record,
            newValues, &auto_newVal_pos);

    if (ret_val < 0)
    {
        goto error;
    }

    if (auto_newVal_pos > -1)
    {
        if (!DB_VALUE_ISNULL(new_record,
                        newValues->update_field_value[auto_newVal_pos].pos,
                        cont->base_cont_.memory_record_size_))
        {
            int auto_value;

            auto_value = *(int *)
                    (newValues->update_field_value[auto_newVal_pos].data);
            if (cont->base_cont_.max_auto_value_ < auto_value)
            {
                int issys_cont;

                SetBufSegment(issys_cont, cont->collect_.cont_id_);
                cont->base_cont_.max_auto_value_ = auto_value;
                SetDirtyFlag(cont->collect_.cont_id_);
                OID_SaveAsAfter(AUTOINCREMENT, cont,
                        cont->collect_.cont_id_ +
                        (unsigned long) &cont->base_cont_.max_auto_value_ -
                        (unsigned long) cont,
                        sizeof(cont->base_cont_.max_auto_value_));
                UnsetBufSegment(issys_cont);
            }
        }
    }

    /* find to-be-updated indexes while uniqueness checking */
    index_id = cont->base_cont_.index_id_;

    if (index_id)
    {
        upd_idx_id_array.cnt = 0;
        pUpd_idx_id_array = &upd_idx_id_array;
    }

    while (index_id)
    {
        ttree = (struct TTree *) OID_Point(index_id);
        if (ttree == NULL)
        {
            ret_val = DB_E_OIDPOINTER;
            goto error;
        }

        index_changed = 0;
        for (i = 0; i < ttree->key_desc_.field_count_; i++)
        {
            pos = ttree->key_desc_.field_desc_[i].position_;
            for (j = 0; j < newValues->update_field_count; j++)
            {
                if (pos == newValues->update_field_value[j].pos)
                {
                    index_changed = 1;
                    break;
                }
            }
            if (index_changed)
                break;
        }

        if (index_changed)
        {
            upd_idx_id_array.idx_oid[upd_idx_id_array.cnt] = index_id;
            upd_idx_id_array.cnt += 1;

            if (ttree->key_desc_.is_unique_ == TRUE)
            {
                null_count = Check_NullFields(cont->collect_.record_size_,
                        &(ttree->key_desc_), new_record, -1);
                if (null_count == -1)
                {
                    ret_val = DB_E_NOTNULLFIELD;
                    goto error;
                }
                SetBufSegment(issys_ttree, index_id);
                if (null_count == 0 &&
                        TTree_IsExist(ttree, new_record, record_id,
                                cont->collect_.record_size_, 0) != NULL_OID)
                {
                    UnsetBufSegment(issys_ttree);
                    ret_val = DB_E_DUPUNIQUEKEY;
                    goto error;
                }
                UnsetBufSegment(issys_ttree);
            }
        }

        /* get next index id */
        index_id = ttree->base_cont_.index_id_;
    }

    ret_val = Col_Update_Field(cont, fields_info, newValues,
            record_id, new_record, 0, 0, pUpd_idx_id_array);
    if (ret_val < 0)
    {
        goto error;
    }

    if (cont->base_cont_.has_variabletype_)
    {
        /* remove old variable data record(s) of the to-be-updated record */
        ret_val = Remove_some_variables(cont, fields_info, old_record,
                newValues);
        if (ret_val < 0)
            goto error;
    }

    PMEM_FREENUL(rec_buf);
    PMEM_FREENUL(fields_info);
    return DB_SUCCESS;

  error:
    PMEM_FREENUL(rec_buf);
    PMEM_FREENUL(fields_info);
    return ret_val;
}

/*****************************************************************************/
//! Update_heap_fields
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param cont(in)          :
 * \param fields_info(in)   : 
 * \param new_record(in)    :
 * \param newValues(in):
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - called : Update_Field()
 *  - reference record format : _Schema_CheckFieldDesc()
 *  - record format 수정
 *****************************************************************************/
int
Update_heap_fields(struct Container *cont, SYSFIELD_T * fields_info,
        char *new_record, struct UpdateValue *newValues, int *auto_newVal_pos)
{
    DB_UINT16 i, pos;

    /* OID       var_rec_id; unused */
    DB_INT32 ret;

    /* cont->collect_.record_size_ == table_info->h_recordLen */

    for (i = 0; i < newValues->update_field_count; i++)
    {
        pos = newValues->update_field_value[i].pos;

        if (fields_info[pos].flag & FD_AUTOINCREMENT)
        {
            *auto_newVal_pos = i;
        }

        /* var_rec_id = NULL_OID; */

        if (newValues->update_field_value[i].isnull)
        {
            if (DB_VALUE_ISNULL(new_record, pos, cont->collect_.record_size_))
            {
                /* nothing to do */
            }
            else        /* NOT NULL field */
            {
                DB_VALUE_SETNULL(new_record, pos, cont->collect_.record_size_);
                if (IS_VARIABLE_DATA(fields_info[pos].type,
                                fields_info[pos].fixed_part))
                {
                    sc_memset(new_record + fields_info[pos].h_offset +
                            fields_info[pos].fixed_part, 0, MDB_FIX_LEN_SIZE);
                }
            }
            continue;
        }

        /* newValues->update_field_value[i].isnull == FALSE */

        if (DB_VALUE_ISNULL(new_record, pos, cont->collect_.record_size_))
        {
            DB_VALUE_SETNOTNULL(new_record, pos, cont->collect_.record_size_);
        }

        if (IS_FIXED_DATA(fields_info[pos].type, fields_info[pos].fixed_part))
        {
            if (IS_BYTE(fields_info[pos].type))
            {
                sc_memcpy(new_record + fields_info[pos].h_offset,
                        newValues->update_field_value[i].data,
                        newValues->update_field_value[i].Blength);
            }
            else
            {
                int copy_len;

                if (IS_N_STRING(fields_info[pos].type))
                    copy_len = newValues->update_field_value[i].Blength;
                else if (fields_info[pos].type == DT_DECIMAL)
                    copy_len = fields_info[pos].length + 3;
                else
                    copy_len = fields_info[pos].length;
                if (fields_info[pos].type == DT_NCHAR)
                    copy_len *= WCHAR_SIZE;
                sc_memcpy(new_record + fields_info[pos].h_offset,
                        newValues->update_field_value[i].data, copy_len);
            }
        }
        else    /* IS_VARIABLE_DATA */
        {
            ret = Insert_Variable(cont, &fields_info[pos],
                    new_record + fields_info[pos].h_offset,
                    newValues->update_field_value[i].data);

            if (ret < 0)
            {
                MDB_SYSLOG(("ERROR: Insert Varchar Record.\n"));
                return ret;
            }
        }
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! Update_memory_fields

/*! \breif  
 ************************************
 * \param connid(in)    :
 * \param rid(in)       : 
 * \param newValue(in)  : 실제 변경하려고 하는 값
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  see update_heap_fields, this is from it. 
 *****************************************************************************/
int
Update_memory_fields(struct Container *cont, SYSFIELD_T * fields_info,
        char *new_record, struct UpdateValue *newValues)
{
    DB_UINT16 i, pos;
    DB_INT32 ret = 0;
    DB_INT32 info_alloced = 0;

    /* get fields information */
    if (fields_info == NULL)
    {
        fields_info = (SYSFIELD_T *)
                PMEM_ALLOC(sizeof(SYSFIELD_T) * cont->collect_.numfields_);

        if (fields_info == NULL)
            return DB_E_OUTOFMEMORY;

        info_alloced = 1;

        ret = _Schema_GetFieldsInfo(cont->base_cont_.name_, fields_info);
    }

    if (ret < 0)
    {
        if (info_alloced)
            PMEM_FREENUL(fields_info);
        return ret;
    }

    for (i = 0; i < newValues->update_field_count; i++)
    {
        pos = newValues->update_field_value[i].pos;

        if (newValues->update_field_value[i].isnull)
        {
            DB_VALUE_SETNULL(new_record, pos,
                    cont->base_cont_.memory_record_size_);
            if (IS_VARIABLE_DATA(fields_info[pos].type,
                            fields_info[pos].fixed_part))
            {
                sc_memset(new_record + fields_info[pos].h_offset +
                        fields_info[pos].fixed_part, 0, MDB_FIX_LEN_SIZE);
            }
            continue;
        }

        if (DB_VALUE_ISNULL(new_record, pos,
                        cont->base_cont_.memory_record_size_))
            DB_VALUE_SETNOTNULL(new_record, pos,
                    cont->base_cont_.memory_record_size_);

        if (IS_BYTE(fields_info[pos].type))
            memcpy_func[fields_info[pos].type] (new_record +
                    fields_info[pos].offset,
                    newValues->update_field_value[i].data,
                    fields_info[pos].length + 4);
        else
            memcpy_func[fields_info[pos].type] (new_record +
                    fields_info[pos].offset,
                    newValues->update_field_value[i].data,
                    (fields_info[pos].type !=
                            DT_DECIMAL ? fields_info[pos].
                            length : fields_info[pos].length + 3));
    }

    if (info_alloced)
        PMEM_FREENUL(fields_info);

    return DB_SUCCESS;
}

/*****************************************************************************/
//! Remove_some_variables 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param cont :
 * \param fields_info : 
 * \param old_record :
 * \param newValues :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - called : Update_Field(), Direct_Update_Field_Internal()
 *  - reference record format : _Schema_CheckFieldDesc()
 *  - record format이 변경됨
 *****************************************************************************/
int
Remove_some_variables(struct Container *cont, SYSFIELD_T * fields_info,
        char *old_record, struct UpdateValue *newValues)
{
    DB_INT32 page_size;
    DB_INT16 pos;
    DB_UINT32 i;
    char *h_ptr;                /* field data of record */
    OID var_rec_id;
    OID var_next_id;
    DB_INT16 nth_collect;
    DB_INT16 npages = 0;
    DB_INT16 nth_page;
    int ret;

    /* next var_rec_id *//* hidden fields *//* slot flags */
    page_size = PAGE_BODY_SIZE - OID_SIZE
            - (GET_HIDDENFIELDSNUM() * INT_SIZE) - INT_SIZE;

    for (i = 0; i < newValues->update_field_count; i++)
    {
        pos = newValues->update_field_value[i].pos;

        if (IS_FIXED_DATA(fields_info[pos].type, fields_info[pos].fixed_part))
            continue;

        h_ptr = old_record + fields_info[pos].h_offset;
        if (!IS_VARIABLE_DATA_EXTENDED(h_ptr, fields_info[pos].fixed_part))
            continue;

        sc_memcpy((char *) &var_rec_id, h_ptr, OID_SIZE);
        nth_collect = Col_get_collect_index(var_rec_id);

        if (nth_collect < -1)   /* error */
            return nth_collect;

        for (nth_page = 0; nth_page < npages; nth_page++)
        {
            h_ptr = (char *) OID_Point(var_rec_id);
            if (h_ptr == NULL)
            {
                return DB_E_OIDPOINTER;
            }

            sc_memcpy((char *) &var_next_id, h_ptr + page_size, sizeof(OID));

            ret = Col_Remove(cont, nth_collect, var_next_id, 0);
            if (ret < 0)
                return ret;
        }

        ret = Col_Remove(cont, nth_collect, var_rec_id, 0);
        if (ret < 0)
            return ret;
    }

    return DB_SUCCESS;
}
