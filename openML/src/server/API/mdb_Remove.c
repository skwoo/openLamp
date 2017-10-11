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

int Remove_variables(struct Container *, SYSFIELD_T *, OID, char *,
        MDB_OIDARRAY *, int *col_list, int remove_msync_slot);

extern int Direct_Find_Record(struct Container *cont, char *indexName,
        struct KeyValue *findKey, OID * record_id);

//////////////////////////////////////////////////////////////////
//
// Function Name : REMOVE API
//
//////////////////////////////////////////////////////////////////

/*****************************************************************************/
//! __Remove
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param cursor_id(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *  - called : _Schema_RemoveIndex()
 *****************************************************************************/
int
__Remove(DB_INT32 cursor_id)
{
    struct Iterator *Iter;
    char *old_record = NULL;
    RecordID record_id;
    DB_INT32 ret_val;
    struct Container *Cont;
    int issys_cont;

    Iter = (struct Iterator *) Iter_Attach(cursor_id, 1, 0);
    if (Iter == NULL)
    {
        return DB_E_CURSORNOTATTACHED;
    }

    if (Iter->lock_mode_ == LK_SHARED || Iter->lock_mode_ == LK_SCHEMA_SHARED)
    {
        return DB_E_INVALIDLOCKMODE;
    }

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

    Iter_FindNext(Iter, 0);

    if (Cont->base_cont_.has_variabletype_)
    {
        char *pbuf = NULL;

        /* copy old record image */
        old_record = PMEM_ALLOC(Cont->collect_.slot_size_);
        if (old_record == NULL)
        {
            ret_val = DB_E_OUTOFMEMORY;
            goto end;
        }

        pbuf = (char *) OID_Point(record_id);
        if (pbuf == NULL)
        {
            ret_val = DB_E_OIDPOINTER;
            goto end;
        }

        sc_memcpy(old_record, pbuf, Cont->collect_.slot_size_);
    }

    /* remove keys and heap record */
    ret_val = Col_Remove(Cont, -1, record_id, 0);
    if (ret_val < 0)
    {
        MDB_SYSLOG((" Remove Function FAIL since Col_Remove() FAIL \n"));
        goto end;
    }

    if (Cont->base_cont_.has_variabletype_)
    {
        SYSTABLE_T *table_info;
        SYSFIELD_T *fields_info;

        /* remove variable data records */
        Cursor_get_schema_info(cursor_id, &table_info, &fields_info);

        ret_val = Remove_variables(Cont, fields_info, NULL_OID, old_record,
                NULL, NULL, 0);
        if (ret_val != DB_SUCCESS)
        {
            goto end;
        }

        PMEM_FREENUL(old_record);
    }


    ret_val = DB_SUCCESS;

  end:
    UnsetBufSegment(issys_cont);
    PMEM_FREENUL(old_record);
    return ret_val;
}

/*****************************************************************************/
//! Direct_Remove_Internal 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param cursor_id :
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
Direct_Remove_Internal(struct Trans *trans, struct Container *cont,
        char *indexName, struct KeyValue *findKey,
        OID rid_to_remove, LOCKMODE lock,
        MDB_OIDARRAY * v_oidset, SYSFIELD_T * pfields_info,
        int remove_msync_slot)
{
    char *old_record = NULL;
    OID record_id;
    DB_INT32 ret_val = DB_SUCCESS;

    if (rid_to_remove == NULL_OID && indexName == NULL)
    {
#ifdef MDB_DEBUG
        sc_assert(0, __FILE__, __LINE__);
#endif
        return -1;
    }

    if (rid_to_remove != NULL_OID)
        record_id = rid_to_remove;
    else
        ret_val = Direct_Find_Record(cont, indexName, findKey, &record_id);

    if (ret_val < 0)
    {
        return ret_val;
    }

    if (cont->base_cont_.has_variabletype_)
    {
        char *pbuf = NULL;

        /* copy old record image */
        old_record = PMEM_ALLOC(cont->collect_.slot_size_);
        if (old_record == NULL)
        {
            return DB_E_OUTOFMEMORY;
        }

        pbuf = (char *) OID_Point(record_id);
        if (pbuf == NULL)
        {
            return DB_E_OIDPOINTER;
        }

        sc_memcpy(old_record, pbuf, cont->collect_.slot_size_);
    }

    /* remove keys and heap record */
    ret_val = Col_Remove(cont, -1, record_id, remove_msync_slot);
    if (ret_val < 0)
    {
        MDB_SYSLOG((" Remove Function FAIL since Col_Remove() FAIL \n"));
        goto error;
    }

    if (cont->base_cont_.has_variabletype_)
    {
        SYSFIELD_T *fields_info;

        /* remove variable data records */

        if (pfields_info)
        {
            fields_info = pfields_info;
        }
        else
        {
            fields_info = (SYSFIELD_T *)
                    PMEM_ALLOC(sizeof(SYSFIELD_T) * cont->collect_.numfields_);
            if (fields_info == NULL)
            {
                ret_val = DB_E_OUTOFMEMORY;
                goto error;
            }
            ret_val =
                    _Schema_GetFieldsInfo(cont->base_cont_.name_, fields_info);
            if (ret_val < 0)
            {
                PMEM_FREENUL(fields_info);
                goto error;
            }
        }
        ret_val = Remove_variables(cont, fields_info, NULL_OID, old_record,
                v_oidset, NULL, remove_msync_slot);

        if (ret_val < 0)
        {
            if (pfields_info == NULL)
                PMEM_FREENUL(fields_info);
            goto error;
        }
        if (pfields_info == NULL)
            PMEM_FREENUL(fields_info);
        PMEM_FREENUL(old_record);
    }

    return DB_SUCCESS;

  error:
    PMEM_FREENUL(old_record);
    return ret_val;
}

/*****************************************************************************/
//! Remove_variables 
/*! \breif  이전 record에서 사용되었던 variable collection 공간을 제거 
 ************************************
 * \param container(in)     :
 * \param nfields_info(in)  : 
 * \param rid(in)           : RID
 * \param record(in)        : memory record(맞나?)
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *  - called : __Update()
 *  - record format reference : _Schema_CheckFieldDesc()
 *
 *****************************************************************************/
int
Remove_variables(struct Container *container, SYSFIELD_T * nfields_info,
        OID rid, char *record, MDB_OIDARRAY * v_oidset,
        int *col_list, int remove_msync_slot)
{
    char *h_record = NULL;      /* heap record */
    OID var_rec_id;
    DB_INT32 nFields;           /* number of fields */
    char *h_ptr;                /* field data of record */
    DB_INT32 nth_field;
    int ret;
    DB_INT16 nth_collect;

    int i;

    if (col_list)
    {
        nFields = col_list[0];
    }
    else
    {
        nFields = container->collect_.numfields_;
    }

    if (rid == NULL_OID)
        h_record = record;
    else
        h_record = OID_Point(rid);

    if (h_record == NULL)
    {
        return DB_E_OIDPOINTER;
    }

    if ((container->base_cont_.type_ & _CONT_MSYNC) && !remove_msync_slot)
    {
        char *p;

        p = h_record + container->collect_.slot_size_ - 1;

        if (*p == SYNCED_SLOT || *p == UPDATE_SLOT)
        {
            return DB_SUCCESS;
        }
    }

    for (i = 0; i < nFields; ++i)
    {
        if (col_list)
        {
            nth_field = col_list[i + 1];
        }
        else
        {
            nth_field = i;
        }

        if (IS_VARIABLE_DATA(nfields_info[nth_field].type,
                        nfields_info[nth_field].fixed_part))
        {
            h_ptr = h_record + nfields_info[nth_field].h_offset;

            if (IS_VARIABLE_DATA_EXTENDED(h_ptr,
                            nfields_info[nth_field].fixed_part))
            {
                // Get Extened Part's OID
                sc_memcpy((char *) &var_rec_id, h_ptr, sizeof(OID));
                if (v_oidset)
                {
#if defined(MDB_DEBUG)
                    if (v_oidset->count < v_oidset->idx)
                    {
                        sc_assert(0, __FILE__, __LINE__);
                    }
#endif
                    v_oidset->array[v_oidset->idx++] = var_rec_id;
                }
                else
                {
                    nth_collect = Col_get_collect_index(var_rec_id);

                    if (nth_collect < -1)       /* error */
                    {
                        return nth_collect;
                    }

                    ret = Col_Remove(container, nth_collect, var_rec_id, 0);
                    if (ret < 0)
                    {
                        return ret;
                    }
                }
            }
        }
    }

    return DB_SUCCESS;
}
