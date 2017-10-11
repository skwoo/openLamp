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

struct Container *Cont_Search(const char *cont_name);

int
OID_InsertLog(short Op_Type, int trans_id, const void *Relation,
        int collect_index, OID id_, char *item, int size, OID next_free_oid,
        DB_UINT8 slot_type);
void OID_SaveAsBefore(short op_type, const void *, OID id_, int size);
void OID_SaveAsAfter(short op_type, const void *, OID id_, int size);


////////////////////////////////////////////////////////////
////
//// Function Name :  Cont_CreateCont
//// Call By :  Create_Table(),
////
//////////////////////////////////////////////////////////////

/*****************************************************************************/
//! calculate_var_rec_sizes
/*! \breif calculate the variable type's candidate number
 ************************************
 * \param min(in)           : extended part's min-size
 * \param max(in)           : extended part's max-size
 * \param count(in)         : candidate's number
 * \param rec_sizes(in/out) : 계산된 공간(?)
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *  - referenced : Cont_CreateCont()
 *****************************************************************************/
static DB_INT32 *
calculate_var_rec_sizes(DB_INT32 min, DB_INT32 max, DB_INT32 count,
        DB_INT32 * rec_sizes)
{
    DB_INT32 increments, i;
    DB_INT32 page_size = 0;

    page_size = PAGE_BODY_SIZE - INT_SIZE;

    min = _alignedlen(min, sizeof(DB_INT32));
    max = _alignedlen(max, sizeof(DB_INT32));

    if (min == max)
        max += 4;

    if (max > page_size)
        max = page_size;

    increments = (max - min) / count;

    if (increments < 1)
        increments = 1;

    increments = _alignedlen(increments, sizeof(DB_INT32));


    rec_sizes[0] = min + increments;
    if (rec_sizes[0] >= max)
    {
        rec_sizes[0] = max;
        return rec_sizes;
    }

    for (i = 1; i < count; i++)
    {
        rec_sizes[i] = rec_sizes[i - 1] + increments;
        if (rec_sizes[i] >= max)
        {
            rec_sizes[i] = max;
            return rec_sizes;
        }
    }

    if (i == count)
        rec_sizes[i - 1] = max;

    return rec_sizes;
}

/*****************************************************************************/
//! Cont_CreateCont 
/*! \breif  Container 생성
 ************************************
 * \param cont_name(in)             :
 * \param memory_record_size(in)    : 
 * \param heap_record_size(in)      : 
 * \param container_id(in)          :
 * \param numFields(in)             :
 * \param type(in)                  :
 * \param tableId(in)               :
 * \param creator(in)               :
 * \param min_fixedpart(in)         : variable type의 후보지 결정시 필요
 * \param max_definedlength(in)     : variable type의 후보지 결정시 필요
 * \param max_records(in)           : Fixed Record 생성시 설정
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *  - var_rec_sizes[VAR_COLLECT_CNT+1] : variable type의 후보지
 *      
 *****************************************************************************/
int
Cont_CreateCont(char *cont_name,
        DB_UINT32 memory_record_size, DB_UINT32 heap_record_size,
        ContainerID container_id, int numFields, ContainerType type,
        int tableId, int creator, int max_records /* , char *column_name */ ,
        FIELDDESC_T * pFieldDesc)
{
    struct Container *Cont;
    DB_INT32 i, logSize, op_type;
    DB_INT32 var_rec_sizes[VAR_COLLECT_CNT + 1];
    DB_BOOL has_variabletype = FALSE;
    DB_INT32 min_fixedpart = 0;
    DB_INT32 max_definedlength = 0;
    DB_INT32 first_variable = 0;
    int auto_fld_idx = -1;

    struct Page *page;
    char *record;
    int transid = *(int *) CS_getspecific(my_trans_id);

    int ret;
    struct ContainerCatalog *cont_catalog;

    if (isTempTable_name(cont_name))
    {
        cont_catalog = (struct ContainerCatalog *) temp_container_catalog;
    }
    else
    {
        cont_catalog = container_catalog;
    }

    for (i = 0; i < numFields && pFieldDesc != NULL; i++)
    {
        if (IS_VARIABLE_DATA(pFieldDesc[i].type_, pFieldDesc[i].fixed_part))
        {
            has_variabletype = TRUE;

            first_variable += 1;

            if (first_variable == 1)
                min_fixedpart = pFieldDesc[i].fixed_part;
            else if (pFieldDesc[i].fixed_part < min_fixedpart)
                min_fixedpart = pFieldDesc[i].fixed_part;

            if (DB_BYTE_LENG(pFieldDesc[i].type_, pFieldDesc[i].length_)
                    > (unsigned int) max_definedlength)
                max_definedlength =
                        DB_BYTE_LENG(pFieldDesc[i].type_,
                        pFieldDesc[i].length_);
            if (pFieldDesc[i].fixed_part > MAX_FIXED_SIZE_FOR_VARIABLE)
                return DB_E_INVALID_VARIABLE_TYPE;
        }
        else if (pFieldDesc[i].flag & FD_AUTOINCREMENT)
        {
            auto_fld_idx = i;
        }
    }
    /* heap record will store string of size of fixedpart and
       the rest will be stored in a exteneded record
       so i can reduce min_fixedpart& max_definedlength */

    if (has_variabletype)
    {
        max_definedlength -= min_fixedpart;
        min_fixedpart /= 2;

        // 왜 8 byte를 할당해줘야 하는지.. 잘 모르겠다... -_-
        min_fixedpart += (OID_SIZE + INT_SIZE);
        max_definedlength += (OID_SIZE + INT_SIZE);
        if (min_fixedpart >= max_definedlength)
            min_fixedpart = max_definedlength - 1;
    }
    Cont = (struct Container *) PMEM_ALLOC(sizeof(struct Container));
    if (Cont == NULL)
        return DB_E_OUTOFMEMORY;

    Cont->base_cont_.id_ = tableId;
    sc_strncpy(Cont->base_cont_.name_, cont_name, REL_NAME_LENG);
    Cont->base_cont_.creator_ = creator;
    Cont->base_cont_.transid_ = -1;
    Cont->base_cont_.type_ = type;
    Cont->base_cont_.index_rebuild = 0;
    Cont->base_cont_.is_dirty_ = 1;
    if (max_definedlength > 0)
        Cont->base_cont_.has_variabletype_ = TRUE;
    else
        Cont->base_cont_.has_variabletype_ = FALSE;
    Cont->base_cont_.memory_record_size_ = memory_record_size;
    Cont->base_cont_.runtime_logging_ = 0;
    Cont->base_cont_.index_count_ = 0;
    Cont->base_cont_.index_id_ = NULL_OID;
    Cont->base_cont_.hash_next_name = NULL_OID;
    Cont->base_cont_.hash_next_id = NULL_OID;

    Cont->base_cont_.max_records_ = max_records;

    if (auto_fld_idx > -1)
    {
        Cont->base_cont_.auto_fld_pos_ = pFieldDesc[auto_fld_idx].position_;
        Cont->base_cont_.auto_fld_offset_ = pFieldDesc[auto_fld_idx].offset_;
    }
    else
    {
        Cont->base_cont_.auto_fld_pos_ = -1;
        Cont->base_cont_.auto_fld_offset_ = -1;
    }
    Cont->base_cont_.max_auto_value_ = 0;

    Cont->collect_.page_link_.head_ = NULL_OID;
    Cont->collect_.page_link_.tail_ = NULL_OID;
    Cont->collect_.cont_id_ = container_id;
    Cont->collect_.item_count_ = 0;
    Cont->collect_.page_count_ = 0;

    Cont->collect_.record_size_ = heap_record_size;
    Cont->collect_.numfields_ = numFields;

    /* hidden field 추가 */
    Cont->collect_.item_size_ = REC_ITEM_SIZE(heap_record_size, numFields);

    // qsid(2byte)와 slot flag(1byte) 용으로 4바이트 추가하고 sizeof(long)바이트 align 맞춤
    Cont->collect_.slot_size_ = REC_SLOT_SIZE(Cont->collect_.item_size_);

    Cont->collect_.collect_index_ = -1;

    /* 레코드 slot의 최소 size는 8로 해야함... free slot의 list를 위해서...
       free slot의 list에는 OID와 slot flag가 들어가기 때문임.
       이를 위해서 item_size_는 record_size를 저장하고,
       slot_size_는 최소 size가 8이상 되도록 보정한 값으로 저장함 */
    if (Cont->collect_.slot_size_ < sizeof(long) * 2)
    {
        Cont->collect_.slot_size_ = sizeof(long) * 2;
    }

    Cont->collect_.free_page_list_ = NULL_OID;

    if (isTable(Cont))
    {
        /*****
        __SYSLOG("[create table] %s (%d), memory record size %d\n",
                cont_name, tableId, memory_record_size);
        __SYSLOG("    heap record size %d, field num %d, bit num %d\n",
                heap_record_size, numFields, (numFields - 1)/8 + 1);
        __SYSLOG("    item size %d, slot size %d\n",
                Cont->collect_.item_size_, Cont->collect_.slot_size_);
        *****/
    }
    else
    {
#ifdef MDB_DEBUG
        /******
        __SYSLOG("[create temp table] %s (%d), record size %d, field num %d, bit num %d, item size %d, slot size %d\n",
                cont_name, tableId, record_size, numFields, (numFields-1)/8+1,
                Cont->collect_.item_size_, Cont->collect_.slot_size_);
        *******/
#endif
    }

    ////////////////////////
    // VARIABLE TYPE 관련
    sc_memset(var_rec_sizes, 0, sizeof(var_rec_sizes));

    if (0 < min_fixedpart && min_fixedpart < max_definedlength)
        calculate_var_rec_sizes(min_fixedpart, max_definedlength,
                server->shparams.num_candidates, var_rec_sizes);

    for (i = 0; i < VAR_COLLECT_CNT; i++)
    {
        struct Collection *var_col;

        var_col = Cont->var_collects_ + i;

        var_col->page_link_.head_ = var_col->page_link_.tail_ = NULL_OID;
        var_col->cont_id_ = container_id;
        var_col->item_count_ = 0;
        var_col->numfields_ = 1;
        var_col->record_size_ = var_col->item_size_ = var_rec_sizes[i];
        var_col->slot_size_ = REC_SLOT_SIZE(var_col->item_size_);
        var_col->collect_index_ = i;

        if (var_col->slot_size_ < sizeof(long) * 2)
        {
            var_col->slot_size_ = sizeof(long) * 2;
        }

        var_col->free_page_list_ = NULL_OID;
    }

    // 새로운 Relation을 위한 Slot를 할당 받음

    page = (struct Page *) PageID_Pointer(container_id);
    if (page == NULL)
    {
        MDB_SYSLOG(("Write ERROR  since OID converte to Pointer 3\n"));
        ret = DB_E_OIDPOINTER;
        goto end;
    }

    record = (char *) page + OID_GetOffSet(container_id);

    op_type = RELATION_CREATE;
    logSize = sizeof(struct Container);

    if (isTempTable(Cont))
    {
        op_type = TEMPRELATION_CREATE;
        logSize = OID_SIZE;     /* cont_id_ */
        goto skip_logging;
    }

    ret = OID_InsertLog(op_type, transid,
            (const void *) cont_catalog, -1, container_id,
            (char *) Cont, logSize, *(OID *) record, 0);
    if (ret < 0)
    {
        MDB_SYSLOG(("table create logging FAIL\n"));
        goto end;
    }

  skip_logging:

    page->header_.free_slot_id_in_page_ = *(OID *) record;

    page->header_.record_count_in_page_ += 1;

    sc_memcpy(record, (char *) Cont, sizeof(struct Container));
    *(record + cont_catalog->collect_.slot_size_ - 1) = USED_SLOT;

    if (page->header_.free_slot_id_in_page_ == NULL_OID)
    {
        cont_catalog->collect_.free_page_list_ = page->header_.free_page_next_;
        page->header_.free_page_next_ = FULL_PID;
    }

    cont_catalog->collect_.item_count_ += 1;

    SetDirtyFlag(container_id);
    if (cont_catalog != (struct ContainerCatalog *) temp_container_catalog)
    {
        SetDirtyFlag(cont_catalog->collect_.cont_id_);
    }

    ret = Cont_InputHashing(cont_name, tableId, container_id);

  end:
    PMEM_FREE(Cont);

    if (ret < 0)
        return ret;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! Cont_Rename
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param Cont          :
 * \param new_cont_name :
 ************************************
 * \return  int
 ************************************
 * \note  None.
 *****************************************************************************/
int
Cont_Rename(struct Container *Cont, char *new_cont_name)
{
    struct Container *new_Cont;
    int ret;
    char old_name[REL_NAME_LENG];

    new_Cont = (struct Container *) PMEM_ALLOC(sizeof(struct Container));
    if (new_Cont == NULL)
        return DB_E_OUTOFMEMORY;

#ifdef MDB_DEBUG
    if (isTable(Cont))
        MDB_SYSLOG(("[Rename table] %s --> %s\n",
                        Cont->base_cont_.name_, new_cont_name));
#endif

    sc_strncpy(old_name, Cont->base_cont_.name_, REL_NAME_LENG);

    sc_memcpy(new_Cont, Cont, sizeof(struct Container));

    sc_strncpy(new_Cont->base_cont_.name_, new_cont_name, REL_NAME_LENG);

    ret = OID_HeavyWrite(UPDATE_HEAPREC, (const void *) NULL,
            Cont->collect_.cont_id_,
            (char *) new_Cont, sizeof(struct Container));
    if (ret < 0)
    {
        MDB_SYSLOG(("table rename logging FAIL\n"));
        PMEM_FREE(new_Cont);
        return ret;
    }

    Cont_RemoveHashing(old_name, Cont->base_cont_.id_);

    ret = Cont_InputHashing(new_cont_name, Cont->base_cont_.id_,
            Cont->collect_.cont_id_);

    PMEM_FREE(new_Cont);

    if (ret < 0)
        return ret;

    return DB_SUCCESS;
}

int
Cont_ChangeProperty(struct Container *cont, ContainerType cont_type,
        int duringRuntime)
{
    if (duringRuntime)
    {
        if (cont_type & _CONT_NOLOGGING)
        {
            if (cont->base_cont_.type_ & _CONT_NOLOGGING)
                cont->base_cont_.runtime_logging_ = _RUNTIME_LOGGING_NONE;
            else
                cont->base_cont_.runtime_logging_ = _RUNTIME_LOGGING_OFF;
        }
        else
        {
            if (cont->base_cont_.type_ & _CONT_NOLOGGING)
            {
                _Checkpointing(0, 1);   /* guarantee previous work on the table */
                cont->base_cont_.runtime_logging_ = _RUNTIME_LOGGING_ON;
            }
            else
                cont->base_cont_.runtime_logging_ = _RUNTIME_LOGGING_NONE;
        }
    }
    else
    {
        if ((cont_type & _CONT_NOLOGGING) == 0)
            _Checkpointing(0, 1);       /* guarantee previous work on the table */

        OID_SaveAsBefore(RELATION, (const void *) cont,
                cont->collect_.cont_id_ + sizeof(struct Collection),
                (int) sizeof(struct BaseContainer));

        cont->base_cont_.type_ = cont_type;
        cont->base_cont_.runtime_logging_ = _RUNTIME_LOGGING_NONE;

        OID_SaveAsAfter(RELATION, (const void *) cont,
                cont->collect_.cont_id_ + sizeof(struct Collection),
                (int) sizeof(struct BaseContainer));
    }
    return DB_SUCCESS;
}

/*****************************************************************************/
//! Cont_Destroy
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param Cont       :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  int
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
Cont_Destroy(struct Container *Cont)
{
    struct Collection *collect = &Cont->collect_;
    int i;
    int ret;

    ret = MemMgr_FreePageList(collect, Cont, 1);
    if (ret < 0)
    {
        MDB_SYSLOG(("table destory free pages FAIL\n"));
        return ret;
    }

    for (i = 0; i < VAR_COLLECT_CNT; i++)
    {
        if (Cont->var_collects_[i].record_size_ > 0)
        {
            collect = &Cont->var_collects_[i];
            ret = MemMgr_FreePageList(collect, Cont, 1);
            if (ret < 0)
            {
                MDB_SYSLOG(("table destory free pages FAIL\n"));
                return ret;
            }
        }
        else
        {
            break;
        }
    }

    Cont_RemoveHashing(Cont->base_cont_.name_, Cont->base_cont_.id_);

    return DB_SUCCESS;
}


struct Container *
Cont_get(OID recordid)
{
    struct Page *page;
    struct Container *Cont;

    page = (struct Page *) PageID_Pointer(recordid);

    if (page == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_PIDPOINTER, 0);
        return NULL;
    }

    if (isFreedPage(page->header_.self_))
    {
        return NULL;
    }

    Cont = (struct Container *) OID_Point(page->header_.cont_id_);

    if (Cont == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_PIDPOINTER, 0);
        return NULL;
    }

    return Cont;
}
