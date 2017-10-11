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
#include "ErrorCode.h"

int Check_NullFields(int record_size, struct KeyDesc *pKeyDesc, char *item,
        int limit_pos);

int make_heap_record(struct Container *Cont, SYSFIELD_T * nfields_info,
        char *record, char *h_record, int *col_list);

static int Col_Rollback_NoLoggingTable(struct Container *Cont, OID record_id,
        SYSFIELD_T * fields_info);

void OID_SaveAsAfter(short Op_Type, const void *Relation, OID id_, int size);

//////////////////////////////////////////////////////////////
//
// Function Name : INSERT API
//
//////////////////////////////////////////////////////////////

/*****************************************************************************/
//! __Insert 
/*! \breif  internal insert function
 ************************************
 * \param cursor_id(in) :
 * \param item(in)      : 
 * \param item_leng(in) :
 * \param item_leng(out): record_id(rid)
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/

int
__Insert(DB_INT32 cursor_id, char *item, int item_leng, OID * record_id,
        int isPreprocessed)
{
    struct Iterator *Iter;
    SYSTABLE_T *table_info = NULL;
    SYSFIELD_T *fields_info = NULL;
    char *new_record = NULL;
    DB_INT32 ret_val;
    DB_INT32 isHeapRec;
    struct Container *Cont;
    int issys_cont;

    Iter = (struct Iterator *) Iter_Attach(cursor_id, 0, 0);
    if (Iter == NULL)
    {
        if (er_errid() != DB_E_TRANSINTERRUPT)
            return DB_E_INVALIDCURSORID;
        else
            return DB_E_TRANSINTERRUPT;
    }

    Cont = (struct Container *) OID_Point(Iter->cont_id_);
    SetBufSegment(issys_cont, Iter->cont_id_);

    Cursor_get_schema_info(cursor_id, &table_info, &fields_info);

    if (Cont->base_cont_.is_dirty_ == 1)
    {
        Cont->base_cont_.is_dirty_ = 0;
    }

    if (Iter->lock_mode_ == LK_SHARED || Iter->lock_mode_ == LK_SCHEMA_SHARED)
    {
        ret_val = DB_E_INVALIDLOCKMODE;
        goto end;
    }

    if (IS_FIXED_RECORDS_TABLE(Cont->base_cont_.max_records_) &&
            Cont->collect_.item_count_ >= Cont->base_cont_.max_records_ &&
            isPreprocessed == 0)
    {
        if (table_info->columnName[0] == '#')
        {
            ret_val = DB_E_RECORDLIMIT_EXCEEDED;
            goto end;
        }
        ret_val = __Update(cursor_id, item, item_leng, NULL);
        goto end;
    }


    /* cursor에서 auto field의 position을 포함한 경우 
       insert 값에 대하여 autoincrement 처리 */
    if (Cont->base_cont_.auto_fld_pos_ > -1)
    {
        int *auto_value;

        auto_value = (int *) (item + Cont->base_cont_.auto_fld_offset_);
        if (DB_VALUE_ISNULL(item, Cont->base_cont_.auto_fld_pos_,
                        Cont->base_cont_.memory_record_size_))
        {
            *auto_value = Cont->base_cont_.max_auto_value_ + 1;
            DB_VALUE_SETNOTNULL(item, Cont->base_cont_.auto_fld_pos_,
                    Cont->base_cont_.memory_record_size_);
        }

        if (Cont->base_cont_.max_auto_value_ < *auto_value)
        {
            Cont->base_cont_.max_auto_value_ = *auto_value;
            SetDirtyFlag(Cont->collect_.cont_id_);
            OID_SaveAsAfter(AUTOINCREMENT, Cont,
                    Cont->collect_.cont_id_ +
                    (unsigned long) &Cont->base_cont_.max_auto_value_ -
                    (unsigned long) Cont,
                    sizeof(Cont->base_cont_.max_auto_value_));
        }
    }

    if (isPreprocessed)
    {
        isHeapRec = 1;
    }
    else
    {
        if (Cont->base_cont_.has_variabletype_)
            isHeapRec = 0;
        else
            isHeapRec = 1;
    }

    if (isHeapRec)      /* does not have variable type */
    {
        if (item_leng < Cont->collect_.item_size_)
        {
            new_record = (char *) PMEM_ALLOC(Cont->collect_.item_size_);
            if (new_record == NULL)
            {
                ret_val = DB_E_OUTOFMEMORY;
                goto end;
            }
            sc_memcpy(new_record, item, item_leng);
            item = new_record;
        }
    }
    else
    {
        new_record = (char *) PMEM_ALLOC(Cont->collect_.slot_size_);
        if (new_record == NULL)
        {
            ret_val = DB_E_OUTOFMEMORY;
            goto end;
        }

        ret_val = make_heap_record(Cont, fields_info, item, new_record, NULL);
        if (ret_val < 0)
        {
            ret_val = ret_val;
            goto end;
        }
        item = new_record;
    }


    /* At this place, uniqueness checking does not performed.
       Uniqueness checking will be performed in key insertion.
     */

    /* insert heap record */
    ret_val = Col_Insert(Cont, -1, item, Iter->open_time_, Iter->qsid_,
            record_id, 0, Iter->msync_flag);
    if (ret_val < 0)
    {
        if (ret_val == DB_E_DUPUNIQUEKEY && isNoLogging(Cont))
        {
            Col_Rollback_NoLoggingTable(Cont, *record_id, fields_info);
            *record_id = NULL_OID;
        }
        goto end;
    }

    ret_val = DB_SUCCESS;

  end:
    UnsetBufSegment(issys_cont);
    PMEM_FREENUL(new_record);
    return ret_val;
}

extern int Remove_variables(struct Container *container,
        SYSFIELD_T * nfields_info, OID rid, char *record,
        MDB_OIDARRAY * v_oidset, int *col_list, int remove_msync_slot);

static int
Col_Rollback_NoLoggingTable(struct Container *Cont, OID record_id,
        SYSFIELD_T * fields_info)
{
    int ret = DB_SUCCESS;

    /* variable data 제거 */
    if (fields_info && Cont->base_cont_.has_variabletype_)
    {
        ret = Remove_variables(Cont, fields_info, record_id, NULL, NULL,
                NULL, 1);
    }

    /* heap record 제거 */
    ret = Col_Remove(Cont, -1, record_id, 1);

    return ret;
}

extern int SizeOfType[];

/*****************************************************************************/
//! Insert_Variable
/*! \breif  VARIABLE TYPE FIELD를 HEAP RECORD에 맞게 구성한다
 ************************************
 * \param Cont(in)          :
 * \param fields_info(in)   : fields info
 * \param h_value(in/out)   : heap record buf
 * \param value(in)         : memory record buf
 ************************************
 * \return if ret < 0 is Error, else is Success.
 ************************************
 * \note 
 *  called : make_heap_record()
 *  heap record format reference : _Schema_CheckFieldDesc()
 *  warning : please null-check at caller 
 *  - record format 수정(for variable type)
 *
 *****************************************************************************/
DB_INT32
Insert_Variable(struct Container *Cont, SYSFIELD_T * field_info,
        char *h_value, char *value)
{
    DataType type;
    OID var_rec_id = NULL_OID;
    DB_INT32 var_len, var_bytelen, ret;

    DB_FIX_PART_LEN blength_value;
    DB_EXT_PART_LEN exted_bytelen;

    type = field_info->type;
    var_len = strlen_func[type] (value);

    if (var_len > field_info->length)
    {
        MDB_SYSLOG(("ERROR: String Size(%d) > defiend length(%d)\n",
                        var_len, field_info->length));
        return DB_FAIL;
    }

    var_bytelen = var_len * SizeOfType[type];

    if (field_info->type == DT_VARBYTE)
        value += INT_SIZE;

    sc_memset(h_value, 0, field_info->fixed_part);

    if (var_bytelen <= field_info->fixed_part)
    {       /* CASE A. can store entire value in heap. */
        blength_value = (DB_FIX_PART_LEN) var_bytelen;

#if defined(MDB_DEBUG)
        if (var_bytelen >= MAX_FIXED_SIZE_FOR_VARIABLE)
            sc_assert(0, __FILE__, __LINE__);
#endif
        // real value in the fixed-part
        sc_memcpy(h_value, value, var_bytelen);

        // length info in length flag
        sc_memcpy(h_value + field_info->fixed_part, &blength_value,
                MDB_FIX_LEN_SIZE);
    }
    else
    {       /* CASE B. store partial value in heap. */
        blength_value = (DB_FIX_PART_LEN) MDB_UCHAR_MAX;

        // real value in the fixed-part
        sc_memcpy(h_value + OID_SIZE, value,
                (field_info->fixed_part - OID_SIZE));

        // length info in length flag
        sc_memcpy(h_value + field_info->fixed_part, &blength_value,
                MDB_FIX_LEN_SIZE);

        value += (field_info->fixed_part - OID_SIZE);

        exted_bytelen = var_bytelen - (field_info->fixed_part - OID_SIZE);
        /* the last extended var record must have a null character */
        if (IS_N_STRING(type))
            exted_bytelen += SizeOfType[type];  /* null character */

#ifdef CHECK_VARCHAR_EXTLEN
        if (type == DT_VARCHAR)
        {
            sc_assert(sc_strlen(value) + 1 == exted_bytelen, __FILE__,
                    __LINE__);
        }
#endif

        ret = Col_Insert_Variable(Cont, value, exted_bytelen, &var_rec_id);
        if (ret < 0)
        {
            return ret;
        }

        // set OID in fixed-part
        sc_memcpy(h_value, (char *) &var_rec_id, OID_SIZE);
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! Insert_Distinct 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param cursor_id :
 * \param item : 
 * \param item_leng :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *  및 기타 설명
 *****************************************************************************/
int
Insert_Distinct(DB_INT32 cursor_id, char *item, int item_leng)
{
    struct Iterator *Iter;
    SYSTABLE_T *table_info = NULL;
    SYSFIELD_T *fields_info = NULL;
    char *new_record = NULL;
    OID record_id;
    OID index_id;
    struct TTree *ttree;
    DB_INT32 f_unique_index = 0;
    DB_INT32 record_size;
    DB_INT32 ret_val;
    char *srcItem = item;
    struct Container *Cont;
    int issys_cont;
    int issys_ttree;

    Iter = (struct Iterator *) Iter_Attach(cursor_id, 0, 0);
    if (Iter == NULL)
    {
        if (er_errid() != DB_E_TRANSINTERRUPT)
            return DB_E_INVALIDCURSORID;
        else
            return DB_E_TRANSINTERRUPT;
    }

    Cont = (struct Container *) OID_Point(Iter->cont_id_);
    SetBufSegment(issys_cont, Iter->cont_id_);

    record_size = Cont->collect_.record_size_;

    if (Cont->base_cont_.is_dirty_ == 1)
        Cont->base_cont_.is_dirty_ = 0;

    if (Iter->lock_mode_ == LK_SHARED || Iter->lock_mode_ == LK_SCHEMA_SHARED)
    {
        ret_val = DB_E_INVALIDLOCKMODE;
        goto end;
    }

    Cursor_get_schema_info(cursor_id, &table_info, &fields_info);

    /* make heap record */
    if (Cont->base_cont_.has_variabletype_)
    {
        new_record = (char *) PMEM_ALLOC(Cont->collect_.slot_size_);
        if (new_record == NULL)
        {
            ret_val = DB_E_OUTOFMEMORY;
            goto end;
        }

        ret_val = make_heap_record(Cont, fields_info, item, new_record, NULL);
        if (ret_val < 0)
        {
            ret_val = ret_val;
            goto end;
        }

    }
    else    /* does not have variable type */
    {
        if (item_leng < Cont->collect_.item_size_)
        {
            new_record = (char *) PMEM_ALLOC(Cont->collect_.item_size_);
            if (new_record == NULL)
            {
                ret_val = DB_E_OUTOFMEMORY;
                goto end;
            }
            sc_memcpy(new_record, item, item_leng);
        }
        else
            new_record = item;
    }

    /* if has a unique index, check with new_record, 
       if hasn't any unique index, check with item */
    index_id = Cont->base_cont_.index_id_;
    while (index_id != NULL_OID)
    {
        ttree = (struct TTree *) OID_Point(index_id);
        if (ttree == NULL)
        {
            ret_val = DB_E_OIDPOINTER;
            goto end;
        }
        if (ttree->key_desc_.is_unique_ == TRUE)
        {
            f_unique_index = 1;

            SetBufSegment(issys_ttree, index_id);

            ret_val = TTree_IsExist(ttree, new_record, NULL_OID,
                    record_size, 0);

            UnsetBufSegment(issys_ttree);
            if (ret_val != NULL_OID)
            {
                ret_val = DB_E_DUPDISTRECORD;   /*SUCCESS; */
                goto end;
            }
        }
        index_id = ttree->base_cont_.index_id_;
    }

    /* unique index가 없는 경우에 레코드 비교 */
    if (f_unique_index == 0 &&
            Col_Check_Exist(Cont, table_info, fields_info, srcItem) == TRUE)
    {
        ret_val = DB_E_DUPDISTRECORD;   /*SUCCESS; */
        goto end;
    }

    /* insert heap record */
    ret_val = Col_Insert(Cont, -1, new_record, Iter->open_time_,
            Iter->qsid_, &record_id, 0, Iter->msync_flag);
    if (ret_val < 0)
    {
        if (ret_val == DB_E_DUPUNIQUEKEY)
            ret_val = DB_E_DUPDISTRECORD;       /*SUCCESS; */
        else
        {
            MDB_SYSLOG(("Data Insert FAIL"));
        }
        goto end;
    }

    ret_val = DB_SUCCESS;

  end:
    UnsetBufSegment(issys_cont);
    if (new_record != item)
        PMEM_FREENUL(new_record);

    return ret_val;
}


/*****************************************************************************/
//! Direct_Insert_Internal 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param trans :
 * \param cont : 
 * \param item(in)      : memory record
 * \param item_leng(in) : memory record's length
 * \param lock :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
Direct_Insert_Internal(struct Trans *trans, struct Container *cont,
        char *item, int item_leng, LOCKMODE lock, OID * insertedRid)
{
    char *new_record = NULL;
    OID record_id;
    DB_INT32 ret_val;
    SYSFIELD_T *fields_info = NULL;

    if (cont->base_cont_.auto_fld_pos_ > -1)
    {
        int *auto_value;

        auto_value = (int *) (item + cont->base_cont_.auto_fld_offset_);
        if (DB_VALUE_ISNULL(item, cont->base_cont_.auto_fld_pos_,
                        cont->base_cont_.memory_record_size_))
        {
            *auto_value = cont->base_cont_.max_auto_value_ + 1;
            DB_VALUE_SETNOTNULL(item, cont->base_cont_.auto_fld_pos_,
                    cont->base_cont_.memory_record_size_);
        }

        if (cont->base_cont_.max_auto_value_ < *auto_value)
        {
            int issys_cont;

            SetBufSegment(issys_cont, cont->collect_.cont_id_);
            cont->base_cont_.max_auto_value_ = *auto_value;
            SetDirtyFlag(cont->collect_.cont_id_);
            OID_SaveAsAfter(AUTOINCREMENT, cont,
                    cont->collect_.cont_id_ +
                    (unsigned long) &cont->base_cont_.max_auto_value_ -
                    (unsigned long) cont,
                    sizeof(cont->base_cont_.max_auto_value_));
            UnsetBufSegment(issys_cont);
        }
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
            PMEM_FREENUL(fields_info);
            MDB_SYSLOG(("ERROR: Unknown Table %s.\n", cont->base_cont_.name_));
            return ret_val;
        }

        new_record = (char *) PMEM_ALLOC(cont->collect_.slot_size_);
        if (new_record == NULL)
        {
            PMEM_FREENUL(fields_info);
            return DB_E_OUTOFMEMORY;
        }

        ret_val = make_heap_record(cont, fields_info, item, new_record, NULL);
        if (ret_val < 0)
        {
            PMEM_FREENUL(fields_info);
            PMEM_FREENUL(new_record);
            return ret_val;
        }
        item = new_record;

    }
    else    /* does not have variable type */
    {
        if (item_leng < cont->collect_.item_size_)
        {
            new_record = PMEM_ALLOC(cont->collect_.item_size_);
            if (new_record == NULL)
            {
                return DB_E_OUTOFMEMORY;
            }
            sc_memcpy(new_record, item, item_leng);
            item = new_record;
        }
    }

    /* At this place, uniqueness checking does not performed.
       Uniqueness checking will be performed in key insertion.
     */

    /* insert heap record */
    ret_val = Col_Insert(cont, -1, item, (DB_UINT32) sc_time(NULL), 0,
            &record_id, 0, 0);
    if (ret_val < 0)
    {
        if (ret_val != DB_E_DUPUNIQUEKEY)
            MDB_SYSLOG(("Data Insert FAIL"));
        else if (isNoLogging(cont))
        {
            Col_Rollback_NoLoggingTable(cont, record_id, fields_info);
        }
        goto error;
    }

    *insertedRid = record_id;

    PMEM_FREENUL(fields_info);
    PMEM_FREENUL(new_record);
    return DB_SUCCESS;

  error:
    PMEM_FREENUL(fields_info);
    PMEM_FREENUL(new_record);
    return ret_val;
}


/*****************************************************************************/
//! Check_NullFields 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param pKeyDesc : 
 * \param item :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
/* not null field에 NULL 값이 설정되었는지 점검.
   not null field에 NULL이 있으면 -1,
   그외는 null field 갯수 */

int
Check_NullFields(int record_size, struct KeyDesc *pKeyDesc, char *item,
        int limit_pos)
{
    int i;
    int count = 0;

    for (i = 0; i < pKeyDesc->field_count_; i++)
    {
        if (limit_pos > -1 && pKeyDesc->field_desc_[i].position_ > limit_pos)
        {
            continue;
        }

        if (DB_VALUE_ISNULL(item, pKeyDesc->field_desc_[i].position_,
                        record_size))
        {   /* null field */
            if (pKeyDesc->field_desc_[i].flag_ & 0x02)
                count++;
            else
                return -1;      /* not null field에 null이 설정되었음 */
        }
    }

    return count;
}

/*****************************************************************************/
//! make_heap_record 
/*! \breif  heap record를 생성함(memory record to heap record)
 ************************************
 * \param Cont(in)          : Container
 * \param nfields_info(in)  : field's info
 * \param record(in)        : memory record
 * \param h_record(in/out)  : heap record
 ************************************
 * \return  if ret < 0 is Error, else is Success
 ************************************
 * \note 
 *  - 참고
 *      memory record : DB API에서 사용되는 record
 *      heap record : db에서 저장시 사용되는 record
 *  - record format reference : _Schema_CheckFieldDesc()
 *  - record format이 변경
 *      variable type이 extend 된 경우 length byte의 값은 MDB_UCHAR_MAX(255)
 *
 *****************************************************************************/
int
make_heap_record(struct Container *Cont, SYSFIELD_T * nfields_info,
        char *record, char *h_record, int *col_list)
{
    DB_INT32 h_item_len;        /* heap item length */
    DB_INT32 h_record_len;      /* heap record length */

    DB_INT32 record_len;        /* heap record length */
    DB_INT32 nFields;           /* number of fields */
    char *value;                /* field data of record */
    char *h_value;              /* field data of heap record */
    DB_INT32 defined_len;       /* defined length of field */
    DB_INT32 i;
    DB_INT32 ret;

    int cnt, pos;

    nFields = Cont->collect_.numfields_;
    record_len = Cont->base_cont_.memory_record_size_;
    h_record_len = Cont->collect_.record_size_;
    h_item_len = REC_ITEM_SIZE(h_record_len, nFields);

    if (col_list)
    {
        sc_memcpy(h_record + h_record_len, record + record_len,
                h_item_len - h_record_len);
        cnt = col_list[0];
    }
    else
    {
        sc_memset(h_record, 0, h_item_len);
        /* copy hidden fileds */
        sc_memcpy(h_record + h_record_len, record + record_len,
                h_item_len - h_record_len);
        cnt = nFields;
    }

    for (i = 0; i < cnt; i++)
    {
        if (col_list)
        {
            pos = col_list[i + 1];
        }
        else
        {
            pos = i;
        }

        value = record + nfields_info[pos].offset;
        h_value = h_record + nfields_info[pos].h_offset;
        defined_len = nfields_info[pos].length;

        if (nfields_info[pos].type == DT_DECIMAL)
        {
            defined_len += 3;
        }
        else if (nfields_info[pos].type == DT_BYTE)
        {
            defined_len += 4;
        }
        else if (nfields_info[pos].type == DT_VARBYTE &&
                nfields_info[pos].fixed_part == FIXED_VARIABLE)
        {
            defined_len += 4;
        }

        defined_len = DB_BYTE_LENG(nfields_info[pos].type, defined_len);

        if (IS_VARIABLE_DATA(nfields_info[pos].type,
                        nfields_info[pos].fixed_part))
        {
            if (DB_VALUE_ISNULL(record, nfields_info[pos].position,
                            record_len))
            {
                continue;
            }

            ret = Insert_Variable(Cont, &nfields_info[pos], h_value, value);
            if (ret != DB_SUCCESS)
            {
                return ret;
            }
        }
        else
        {
            sc_memcpy(h_value, value, defined_len);
        }
    }

    return DB_SUCCESS;
}

int
Exist_Record(DB_INT32 cursor_id, char *item, int item_leng)
{
    struct Iterator *Iter;
    SYSTABLE_T *table_info = NULL;
    SYSFIELD_T *fields_info = NULL;
    char *new_record = NULL;
    OID index_id;
    struct TTree *ttree;
    DB_INT32 f_unique_index = 0;
    DB_INT32 record_size;
    DB_INT32 ret_val;
    char *srcItem = item;
    struct Container *Cont;
    int issys_cont;
    int issys_ttree;

    Iter = (struct Iterator *) Iter_Attach(cursor_id, 0, 0);
    if (Iter == NULL)
    {
        if (er_errid() != DB_E_TRANSINTERRUPT)
        {
            return DB_E_INVALIDCURSORID;
        }
        else
        {
            return DB_E_TRANSINTERRUPT;
        }
    }

    Cont = (struct Container *) OID_Point(Iter->cont_id_);
    SetBufSegment(issys_cont, Iter->cont_id_);

    record_size = Cont->collect_.record_size_;

    if (Cont->base_cont_.is_dirty_ == 1)
    {
        Cont->base_cont_.is_dirty_ = 0;
    }

    Cursor_get_schema_info(cursor_id, &table_info, &fields_info);

    /* make heap record */
    if (Cont->base_cont_.has_variabletype_)
    {
        new_record = (char *) PMEM_ALLOC(Cont->collect_.slot_size_);
        if (new_record == NULL)
        {
            ret_val = DB_E_OUTOFMEMORY;
            goto end;
        }

        ret_val = make_heap_record(Cont, fields_info, item, new_record, NULL);
        if (ret_val < 0)
        {
            goto end;
        }
    }
    else    /* does not have variable type */
    {
        if (item_leng < Cont->collect_.item_size_)
        {
            new_record = (char *) PMEM_ALLOC(Cont->collect_.item_size_);
            if (new_record == NULL)
            {
                ret_val = DB_E_OUTOFMEMORY;
                goto end;
            }
            sc_memcpy(new_record, item, item_leng);
        }
        else
        {
            new_record = item;
        }
    }

    /* if has a unique index, check with new_record, 
       if hasn't any unique index, check with item */
    index_id = Cont->base_cont_.index_id_;
    while (index_id != NULL_OID)
    {
        ttree = (struct TTree *) OID_Point(index_id);
        if (ttree == NULL)
        {
            ret_val = DB_E_OIDPOINTER;
            goto end;
        }
        if (ttree->key_desc_.is_unique_ == TRUE)
        {
            f_unique_index = 1;

            SetBufSegment(issys_ttree, index_id);
            ret_val = TTree_IsExist(ttree, new_record, NULL_OID,
                    record_size, 0);
            UnsetBufSegment(issys_ttree);
            if (ret_val != NULL_OID)
            {
                ret_val = DB_E_DUPDISTRECORD;   /*SUCCESS; */
                goto end;
            }
        }

        index_id = ttree->base_cont_.index_id_;
    }

    /* unique index가 없는 경우에 레코드 비교 */
    if (f_unique_index == 0 &&
            Col_Check_Exist(Cont, table_info, fields_info, srcItem) == TRUE)
    {
        ret_val = DB_E_DUPDISTRECORD;   /*SUCCESS; */
        goto end;
    }

    ret_val = DB_SUCCESS;

  end:

    UnsetBufSegment(issys_cont);

    if (new_record != item)
    {
        PMEM_FREENUL(new_record);
    }

    return ret_val;
}
