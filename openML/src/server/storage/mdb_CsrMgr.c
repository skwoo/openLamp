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

void OID_SaveAsBefore(short op_type, const void *, OID id_, int size);
void OID_SaveAsAfter(short op_type, const void *, OID id_, int size);
int KeyValue_Delete(struct KeyValue *pKeyValue);
int Filter_Delete(struct Filter *pFilter);
struct Trans *Trans_Search(int Trans_ID);
int LockMgr_Table_Unlock(int trans_id, OID oid);

_PRIVATE void Cursor_RemovePosition(int pos, int flag);
_PRIVATE int __close_cursor(int cursor_id, int flag);
_PRIVATE DB_UINT32 __Open_Cursor(int Trans_Num, char *cont_name,
        LOCKMODE lock_mode, DB_INT16 idx_no, int key_include,
        struct KeyValue *minkey, struct KeyValue *maxkey,
        struct Filter *f, int direction, int index_cont_oid,
        DB_UINT8 msync_flag);
_PRIVATE int __Reopen_Cursor(int cursor_id, int key_include,
        struct KeyValue *minkey, struct KeyValue *maxkey, struct Filter *f);

extern struct CursorMgr *Cursor_Mgr;

/* lib server에서 disconnect하고 connect한 경우 qsid를 처음부터 사용하지
   않도록 하기 위해서 설정 */
__DECL_PREFIX DB_UINT16 _g_qsid = 1;

_PRIVATE int whereami = 0x01;   /* library mobiledb */

/*********************************************************/

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
CurMgr_CursorTableInit()
{
    int cursor_num;


    Cursor_Mgr->free_first_ = 0;

    for (cursor_num = 0; cursor_num < MAX_ALLOC_CURSORID_NUM; cursor_num++)
    {
        // cursor slot에 대해서 하나의 cursor 번호를 고정시킴
        // 이렇게 되면 빈 slot을 찾으면 cursor 번호가 결정되고
        // next_cursorid는 필요없음...
        //cursor id는 cursor position + 1 값으로 사용
        Cursor_Mgr->cursor[cursor_num].fAllocated_ = 0;
        Cursor_Mgr->cursor[cursor_num].whereami = 0;
        Cursor_Mgr->cursor[cursor_num].trans_id_ = 0;
        Cursor_Mgr->cursor[cursor_num].trans_ptr = NULL;
        if (cursor_num < (MAX_ALLOC_CURSORID_NUM - 1))
            Cursor_Mgr->cursor[cursor_num].next_ = cursor_num + 1;
        else
            Cursor_Mgr->cursor[cursor_num].next_ = NULL_Cursor_Position;
        sc_memset((char *) Cursor_Mgr->cursor[cursor_num].chunk_, 0,
                sizeof(char) * CURSOR_CHUNK_SIZE);
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
CurMgr_CursorTableFree()
{
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
static struct Cursor *
CsrMgr_AllocateCursor(DB_UINT32 * cursor_id, DB_UINT16 * qsid,
        int readonly_cursor_flag)
{
    int cursor_p;

    if (Cursor_Mgr->free_first_ == NULL_Cursor_Position)
    {
        int cursor_num;

        /* cursor list에서 제대로 free가 안 된 것이 있는지 찾아보자 */
        for (cursor_num = 0; cursor_num < MAX_ALLOC_CURSORID_NUM; cursor_num++)
        {
            if (Trans_Search(Cursor_Mgr->cursor[cursor_num].trans_id_) == NULL)
            {
                Cursor_RemovePosition(cursor_num, 0);
            }
        }

        if (Cursor_Mgr->free_first_ == NULL_Cursor_Position)
        {
            return (struct Cursor *) NULL;
        }
    }

    /* get a cursor from the free cursor list */
    cursor_p = Cursor_Mgr->free_first_;
    Cursor_Mgr->free_first_ = Cursor_Mgr->cursor[cursor_p].next_;

    *cursor_id = cursor_p + 1;
    Cursor_Mgr->cursor[cursor_p].fAllocated_ = 1;
    Cursor_Mgr->cursor[cursor_p].whereami = whereami;
    Cursor_Mgr->cursor[cursor_p].pUpd_idx_id_array = NULL;
    Cursor_Mgr->cursor[cursor_p].trans_next = NULL;

    if (++server->_g_qsid == 0) /* 0은 reserved로 사용하기 위하여 */
        server->_g_qsid++;

    *qsid = server->_g_qsid;

    _g_qsid = *qsid;

    return &(Cursor_Mgr->cursor[cursor_p]);
}

////////////////////////////////////////////////////////////////////////
////
//// Function name:  Cursor Open API
////
//////////////////////////////////////////////////////////////////////////

/*****************************************************************************/
//! Open_Cursor
/*! \breif  cursor를 열때 사용하는 함수
 ************************************
 * \param Trans_Num(in) : transaction id
 * \param cont_name(in) : relation name
 * \param lock_mode(in) : lock mode
 * \param idx_no(in)    : index's number
 * \param minkey(in)    : index's minimum key
 * \param maxkey(in)    : index's maximum key
 * \param f(in)         : filter's value
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - reference function : __Open_Cursor()
 *  Cursor를 여는 방법은 3가지가 존재함
 *  1. index 없음, filter도 없음
 *  2. index 있음. filter는 없음
 *  3. index 없음. filter는 있음
 *  - COLLATION 지원
 *****************************************************************************/
DB_UINT32
Open_Cursor(int Trans_Num, char *cont_name, LOCKMODE lock_mode,
        DB_INT16 idx_no,
        struct KeyValue * minkey, struct KeyValue * maxkey, struct Filter * f)
{
    int ret;

    ret = __Open_Cursor(Trans_Num, cont_name, lock_mode, idx_no, 1,
            minkey, maxkey, f, 0, NULL_OID, 0);
    if (ret < 0)
    {
        KeyValue_Delete(minkey);
        KeyValue_Delete(maxkey);
        Filter_Delete(f);
    }

    return ret;
}

DB_UINT32
Open_Cursor2(int Trans_Num, char *cont_name, LOCKMODE lock_mode,
        DB_INT16 idx_no, struct KeyValue * minkey, struct KeyValue * maxkey,
        struct Filter * f, OID index_cont_oid, DB_UINT8 msync_flag)
{
    int ret;

    ret = __Open_Cursor(Trans_Num, cont_name, lock_mode, idx_no, 1,
            minkey, maxkey, f, 0, index_cont_oid, msync_flag);
    if (ret < 0)
    {
        KeyValue_Delete(minkey);
        KeyValue_Delete(maxkey);
        Filter_Delete(f);
    }

    return ret;
}

/*****************************************************************************/
//! Open_Cursor_Desc
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param Trans_Num(in)     : transaction id
 * \param cont_name(in)     : relation name      
 * \param lock_mode(in)     : lock mode
 * \param idx_no(in)        : index's number
 * \param key_include(in)   : always 1(Open_Cursor에서 호출되는 경우)
 * \param minkey(in)        : index's minimum
 * \param maxkey(in)        : index's maximum
 * \param f(in)             : filter's value
 * \param direction(in)     : cursor's direction
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - 20061016
 *      COLLATION 지원
 *****************************************************************************/
DB_UINT32
Open_Cursor_Desc(int Trans_Num, char *cont_name, LOCKMODE lock_mode,
        DB_INT16 idx_no, struct KeyValue * minkey, struct KeyValue * maxkey,
        struct Filter * f)
{
    int ret;

    ret = __Open_Cursor(Trans_Num, cont_name, lock_mode, idx_no, 1,
            minkey, maxkey, f, 1, NULL_OID, 0);
    if (ret < 0)
    {
        KeyValue_Delete(minkey);
        KeyValue_Delete(maxkey);
        Filter_Delete(f);
    }

    return ret;
}

/*****************************************************************************/
//! __Open_Cursor
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param Trans_Num(in)     : transaction id
 * \param cont_name(in)     : relation name      
 * \param lock_mode(in)     : lock mode
 * \param idx_no(in)        : index's number
 * \param key_include(in)   : always 1(Open_Cursor에서 호출되는 경우)
 * \param minkey(in)        : index's minimum
 * \param maxkey(in)        : index's maximum
 * \param f(in)             : filter's value
 * \param direction(in)     : cursor's direction
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - COLLATION 지원
 *****************************************************************************/
extern int G_f_state;

_PRIVATE DB_UINT32
__Open_Cursor(int Trans_Num, char *cont_name, LOCKMODE lock_mode,
        DB_INT16 idx_no, int key_include,
        struct KeyValue *minkey, struct KeyValue *maxkey,
        struct Filter *f, int direction, int index_cont_oid,
        DB_UINT8 msync_flag)
{
    struct Cursor *cursor = NULL;
    struct Container *Cont;
    DB_UINT32 cursor_id = -1;
    int result = 0;
    static DB_UINT32 open_time = 0;
    DB_UINT16 qsid;

    DB_INT32 i, j;

    DB_INT32 readonly_flag = 0;
    struct Iterator *iter = NULL;
    int issys_cont = 0;
    int issys_ttree = 0;

    if (open_time == 0)
        open_time = (DB_UINT32) sc_time(NULL);
    else
        open_time++;

    Cont = (struct Container *) Cont_Search(cont_name);

    if (Cont == NULL)
    {
        result = DB_E_UNKNOWNTABLE;
        goto End_Processing;
    }

    SetBufSegment(issys_cont, Cont->collect_.cont_id_);

    if (lock_mode == LK_SCHEMA_SHARED)
        readonly_flag = 1;

    cursor = (struct Cursor *) CsrMgr_AllocateCursor(&cursor_id, &qsid,
            readonly_flag);
    if (cursor == NULL)
    {
        result = DB_E_NOMORECURSOR;
        goto End_Processing;
    }

    cursor->cont_type = Cont->base_cont_.type_;
    cursor->cont_id_ = Cont->collect_.cont_id_;
    cursor->trans_id_ = Trans_Num;
    cursor->trans_ptr = Trans_Search(Trans_Num);
    cursor->lock_mode_ = lock_mode;
    cursor->table_info = NULL;
    cursor->fields_info = NULL;

    /* 이 transaction이 이미 open한 cursor 중에 같은 테이블이 있는지 점검
       If exists, assign open_time and qsid of new cursor as that cursor's */
    if (!isTempTable_ID(Cont->base_cont_.id_))
    {
        register struct Cursor *pCursor = cursor->trans_ptr->cursor_list;

        iter = NULL;

        while (pCursor)
        {
            iter = (struct Iterator *) pCursor->chunk_;

            if (Cont->collect_.cont_id_ == iter->cont_id_)
                break;

            pCursor = pCursor->trans_next;
        }

        if (pCursor)
        {
            open_time = iter->open_time_;
            qsid = iter->qsid_;
        }

        if (cursor->trans_ptr->cursor_list == NULL)
        {
            cursor->trans_ptr->cursor_list = cursor;
        }
        else
        {   /* insert into the front of the cursor list */
            cursor->trans_next = cursor->trans_ptr->cursor_list;
            cursor->trans_ptr->cursor_list = cursor;
        }
    }

    if (G_f_state != STATE_CREATEDB)
    {
        cursor->table_info = (SYSTABLE_T *) PMEM_ALLOC(sizeof(SYSTABLE_T));
        if (cursor->table_info == NULL)
        {
            result = CS_E_OUTOFMEMORY;
            goto End_Processing;
        }

        result = dbi_Schema_GetTableFieldsInfo(-1, Cont->base_cont_.name_,
                cursor->table_info, &cursor->fields_info);
        if (result < 0)
            goto End_Processing;
    }


    if (key_include && f != NULL && f->expression_count_ > 0)
    {
        if (cursor->table_info)
        {
            for (i = 0; i < f->expression_count_; i++)
            {
                for (j = 0; j < cursor->table_info->numFields; j++)
                {
                    if (f->expression_[i].position_ ==
                            cursor->fields_info[j].position)
                    {
                        f->expression_[i].h_offset_ =
                                cursor->fields_info[j].h_offset;
                        f->expression_[i].fixed_part =
                                cursor->fields_info[j].fixed_part;
                        f->expression_[i].length_ =
                                cursor->fields_info[j].length;
                        break;
                    }
                }
            }
        }
        else
        {
            for (i = 0; i < f->expression_count_; i++)
            {
                f->expression_[i].h_offset_ = f->expression_[i].offset_;
                f->expression_[i].fixed_part = FIXED_VARIABLE;
            }
        }
    }

    if (idx_no < 0)
    {
        register struct ContainerIterator *ContIter =
                (struct ContainerIterator *) cursor->chunk_;

        sc_memset(cursor->chunk_, 0, CURSOR_CHUNK_SIZE);

        iter = (struct Iterator *) cursor->chunk_;

        ContIter->iterator_.cont_id_ = Cont->collect_.cont_id_;
        ContIter->iterator_.trans_ = Trans_Num;
        ContIter->iterator_.trans_ptr = cursor->trans_ptr;
        ContIter->iterator_.rec_leng = 0;
        ContIter->iterator_.min_.field_count_ = 0;
        ContIter->iterator_.min_.field_value_ = NULL;
        ContIter->iterator_.max_.field_count_ = 0;
        ContIter->iterator_.max_.field_value_ = NULL;
        ContIter->iterator_.msync_flag = msync_flag;

        ContIter->iterator_.creator_ = Cont->base_cont_.creator_;
        ContIter->iterator_.id_ = Cont->base_cont_.id_;
        ContIter->iterator_.slot_size_ = Cont->collect_.slot_size_;
        ContIter->iterator_.item_size_ = Cont->collect_.item_size_;
        ContIter->iterator_.record_size_ = Cont->collect_.record_size_;
        ContIter->iterator_.numfields_ = Cont->collect_.numfields_;
        ContIter->iterator_.memory_record_size_ =
                Cont->base_cont_.memory_record_size_;

        if (f == NULL || f->expression_count_ == 0)
        {
            ContIter->iterator_.filter_.expression_count_ = 0;
            ContIter->iterator_.filter_.expression_ = NULL;
        }
        else
        {
            ContIter->iterator_.filter_ = *f;
        }

        ContIter->iterator_.key_include_ = key_include;

        ContIter->iterator_.type_ = SEQ_CURSOR;
        ContIter->iterator_.lock_mode_ = lock_mode;
        ContIter->iterator_.open_time_ = open_time;
        ContIter->iterator_.qsid_ = qsid;

        ContIter->iterator_.fInit = 0;
    }
    else
    {
        // createdb 수행시 systable을 만들 때 이 함수를 호출하는데
        // T-tree cursor로는 open을 하지 않음.
        register struct TTreeIterator *ttreeiter =
                (struct TTreeIterator *) cursor->chunk_;
        struct TTree *ttree;
        OID index_id;

        sc_memset(cursor->chunk_, 0, CURSOR_CHUNK_SIZE);

        iter = (struct Iterator *) cursor->chunk_;

        ttreeiter->iterator_.cont_id_ = Cont->collect_.cont_id_;
        ttreeiter->iterator_.trans_ = Trans_Num;
        ttreeiter->iterator_.trans_ptr = cursor->trans_ptr;
        ttreeiter->iterator_.rec_leng = 0;

        if (index_cont_oid)
        {
            ttree = (struct TTree *) OID_Point(index_cont_oid);
        }
        else
        {
            index_id = Cont->base_cont_.index_id_;
            ttree = NULL;
            while (index_id != NULL_OID)
            {
                ttree = (struct TTree *) OID_Point(index_id);
                if (ttree == NULL)
                {
                    result = DB_E_OIDPOINTER;
                    goto End_Processing;
                }

                if (ttree->base_cont_.id_ == idx_no)
                    break;

                index_id = ttree->base_cont_.index_id_;
                ttree = NULL;
            }
        }

        if (ttree == NULL)
        {
            MDB_SYSLOG(
                    ("[Error] TTree is NOt Exist on idx no %d on table %s\n",
                            idx_no, Cont->base_cont_.name_));

            result = -1;
            goto End_Processing;
        }

        SetBufSegment(issys_ttree, ttree->collect_.cont_id_);

        if (minkey == NULL || minkey->field_count_ == 0)
        {
            ttreeiter->iterator_.min_.field_count_ = 0;
            ttreeiter->iterator_.min_.field_value_ = NULL;
        }
        else
        {
            if (key_include)
            {
                int set_count = 0;

                if (minkey->field_count_ > ttree->key_desc_.field_count_)
                {
                    result = DB_E_CURSORKEYDESC;
                    goto End_Processing;
                }

                for (i = 0; i < minkey->field_count_; i++)
                {
                    for (j = 0; j < (ttree->key_desc_.field_count_); j++)
                    {
                        if (minkey->field_value_[i].position_ ==
                                ttree->key_desc_.field_desc_[j].position_)
                        {
                            minkey->field_value_[i].order_ =
                                    ttree->key_desc_.field_desc_[j].order_;
                            minkey->field_value_[i].h_offset_ =
                                    ttree->key_desc_.field_desc_[j].h_offset_;
                            minkey->field_value_[i].fixed_part =
                                    ttree->key_desc_.field_desc_[j].fixed_part;
                            minkey->field_value_[i].collation =
                                    ttree->key_desc_.field_desc_[j].collation;
                            set_count++;
                            break;
                        }
                    }
                }

                if (minkey->field_count_ != set_count)
                {
                    result = DB_E_CURSORKEYDESC;
                    goto End_Processing;
                }
            }
            ttreeiter->iterator_.min_ = *minkey;
        }

        if (maxkey == NULL || maxkey->field_count_ == 0)
        {
            ttreeiter->iterator_.max_.field_count_ = 0;
            ttreeiter->iterator_.max_.field_value_ = NULL;
        }
        else
        {
            if (key_include)
            {
                int set_count = 0;

                if (maxkey->field_count_ > ttree->key_desc_.field_count_)
                {
                    result = DB_E_CURSORKEYDESC;
                    goto End_Processing;
                }

                for (i = 0; i < maxkey->field_count_; i++)
                {
                    for (j = 0; j < (ttree->key_desc_.field_count_); j++)
                    {
                        if (maxkey->field_value_[i].position_ ==
                                ttree->key_desc_.field_desc_[j].position_)
                        {
                            maxkey->field_value_[i].order_ =
                                    ttree->key_desc_.field_desc_[j].order_;
                            maxkey->field_value_[i].h_offset_ =
                                    ttree->key_desc_.field_desc_[j].h_offset_;
                            maxkey->field_value_[i].fixed_part =
                                    ttree->key_desc_.field_desc_[j].fixed_part;
                            maxkey->field_value_[i].collation =
                                    ttree->key_desc_.field_desc_[j].collation;
                            set_count++;
                            break;
                        }
                    }
                }

                if (maxkey->field_count_ != set_count)
                {
                    result = DB_E_CURSORKEYDESC;
                    goto End_Processing;
                }
            }
            ttreeiter->iterator_.max_ = *maxkey;
        }

        ttreeiter->iterator_.msync_flag = msync_flag;
        ttreeiter->iterator_.creator_ = Cont->base_cont_.creator_;
        ttreeiter->iterator_.id_ = Cont->base_cont_.id_;
        ttreeiter->iterator_.slot_size_ = Cont->collect_.slot_size_;
        ttreeiter->iterator_.item_size_ = Cont->collect_.item_size_;
        ttreeiter->iterator_.record_size_ = Cont->collect_.record_size_;
        ttreeiter->iterator_.numfields_ = Cont->collect_.numfields_;
        ttreeiter->iterator_.memory_record_size_ =
                Cont->base_cont_.memory_record_size_;


        if (f == NULL || f->expression_count_ == 0)
        {
            ttreeiter->iterator_.filter_.expression_count_ = 0;
            ttreeiter->iterator_.filter_.expression_ = NULL;
        }
        else
        {
            ttreeiter->iterator_.filter_ = *f;
        }

        ttreeiter->iterator_.key_include_ = key_include;

        if (direction == 0)
            ttreeiter->iterator_.type_ = TTREE_FORWARD_CURSOR;
        else
            ttreeiter->iterator_.type_ = TTREE_BACKWARD_CURSOR;

        ttreeiter->iterator_.lock_mode_ = lock_mode;
        ttreeiter->iterator_.open_time_ = open_time;
        ttreeiter->iterator_.qsid_ = qsid;

        ttreeiter->tree_oid_ = ttree->collect_.cont_id_;
        ttreeiter->is_unique_ = ttree->key_desc_.is_unique_;

        ttreeiter->iterator_.fInit = 0;

        ttreeiter->modified_time_ = ttree->modified_time_;
        // Now that it's new cursor open, reset flag which means the second
        // bulk read was occurred.
        cursor->trans_ptr->fUseResult = 0;
    }

  End_Processing:
    UnsetBufSegment(issys_cont);
    UnsetBufSegment(issys_ttree);

    if (result < 0)
    {
        if (iter)
        {
            iter->min_.field_count_ = 0;
            iter->min_.field_value_ = NULL;
            iter->max_.field_count_ = 0;
            iter->max_.field_value_ = NULL;
            iter->filter_.expression_count_ = 0;
            iter->filter_.expression_ = NULL;
        }

        if (cursor)
        {
            PMEM_FREENUL(cursor->table_info);
            PMEM_FREENUL(cursor->fields_info);
        }

        return result;
    }
    else
        return cursor_id;
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
Reopen_Cursor(int cursor_id,
        struct KeyValue *minkey, struct KeyValue *maxkey, struct Filter *f)
{
    int ret_val;
    int key_include = 1;        /* must be set to 1 at this place */

    ret_val = __Reopen_Cursor(cursor_id, key_include, minkey, maxkey, f);
    if (ret_val < 0)
    {
        goto error;
    }

    return DB_SUCCESS;

  error:
    if (key_include)
    {
        KeyValue_Delete(minkey);
        KeyValue_Delete(maxkey);
        Filter_Delete(f);
    }
    return ret_val;
}

/*****************************************************************************/
//! __Reopen_Cursor
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param cursor_id(in)     :
 * \param key_include(in)   : 
 * \param minkey(in)        : key's minimum value
 * \param maxkey(in)        : key's maximum value
 * \param f(in)             : filter
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - collation 설정
 *****************************************************************************/
_PRIVATE int
__Reopen_Cursor(int cursor_id, int key_include,
        struct KeyValue *minkey, struct KeyValue *maxkey, struct Filter *f)
{
    int result = DB_FAIL;
    Position pos;
    struct Iterator *Iter;

    DB_UINT16 qsid;
    struct Cursor *cursor;

    DB_INT32 i, j;

    if (cursor_id <= 0)
        return DB_FAIL;

    pos = CursorPosition(cursor_id);
    if (pos == NULL_Cursor_Position)
    {
#ifdef MDB_DEBUG
        MDB_SYSLOG((" Reopen_Cursor: Not Find Cursor ID %d\n", cursor_id));
#endif
        Iter = NULL;
        result = DB_FAIL;
        goto End_Processing;
    }

    cursor = &Cursor_Mgr->cursor[pos];

    /* insert하고 reopen하여 read를 할 때 insert한 값이 read되기 위해서는
       reopen한 경우 다른 cursor로 변경하기 위해서 qsid를 변경시켜줌. */

    if (++server->_g_qsid == 0) /* 0은 reserved로 사용하기 위하여 */
        server->_g_qsid++;

    qsid = server->_g_qsid;

    _g_qsid = qsid;

    Iter = (struct Iterator *) &Cursor_Mgr->cursor[pos].chunk_;

    if (key_include && f != NULL && f->expression_count_ > 0)
    {
        if (cursor->table_info)
        {
            for (i = 0; i < f->expression_count_; i++)
            {
                for (j = 0; j < cursor->table_info->numFields; j++)
                {
                    if (f->expression_[i].position_ ==
                            cursor->fields_info[j].position)
                    {
                        f->expression_[i].h_offset_ =
                                cursor->fields_info[j].h_offset;
                        f->expression_[i].fixed_part =
                                cursor->fields_info[j].fixed_part;
                        break;
                    }
                }
            }
        }
        else
        {
            for (i = 0; i < f->expression_count_; i++)
            {
                f->expression_[i].h_offset_ = f->expression_[i].offset_;
                f->expression_[i].fixed_part = FIXED_VARIABLE;
#if defined(MDB_DEBUG)
                if (f->expression_[i].collation == MDB_COL_NONE)
                    sc_assert(0, __FILE__, __LINE__);
#endif
            }
        }
    }

    if (Iter->type_ == SEQ_CURSOR)
    {
        struct ContainerIterator *ContIter = (struct ContainerIterator *) Iter;

        if (ContIter->iterator_.key_include_)
            Filter_Delete(&ContIter->iterator_.filter_);

        if (f == NULL || f->expression_count_ == 0)
        {
            ContIter->iterator_.filter_.expression_count_ = 0;
            ContIter->iterator_.filter_.expression_ = NULL;
        }
        else
        {
            ContIter->iterator_.filter_ = *f;
        }

        ContIter->iterator_.key_include_ = key_include;

        /* reduce the overhead of calling sc_time */
        ContIter->iterator_.open_time_--;
        ContIter->iterator_.qsid_ = qsid;

        ContIter_Create(ContIter, 0);
    }
    else
    {
        // createdb 수행시 systable을 만들 때 이 함수를 호출하는데
        // T-tree cursor로는 open을 하지 않음.
        struct TTreeIterator *ttreeiter = (struct TTreeIterator *) Iter;
        struct TTree *ttree = (struct TTree *) OID_Point(ttreeiter->tree_oid_);
        int issys_ttree;

        SetBufSegment(issys_ttree, ttreeiter->tree_oid_);

        if (ttreeiter->iterator_.key_include_)
        {
            KeyValue_Delete(&ttreeiter->iterator_.min_);
            KeyValue_Delete(&ttreeiter->iterator_.max_);
            Filter_Delete(&ttreeiter->iterator_.filter_);
        }

        if (minkey == NULL || minkey->field_count_ == 0)
        {
            ttreeiter->iterator_.min_.field_count_ = 0;
            ttreeiter->iterator_.min_.field_value_ = NULL;
        }
        else
        {
            if (key_include)
            {
                for (i = 0; i < minkey->field_count_; i++)
                {
                    for (j = 0; j < ttree->key_desc_.field_count_; j++)
                    {
                        if (minkey->field_value_[i].position_ ==
                                ttree->key_desc_.field_desc_[j].position_)
                        {
                            minkey->field_value_[i].order_ =
                                    ttree->key_desc_.field_desc_[j].order_;
                            minkey->field_value_[i].h_offset_ =
                                    ttree->key_desc_.field_desc_[j].h_offset_;
                            minkey->field_value_[i].fixed_part =
                                    ttree->key_desc_.field_desc_[j].fixed_part;
                            minkey->field_value_[i].collation =
                                    ttree->key_desc_.field_desc_[j].collation;
                            break;
                        }
                    }
                }
            }
            ttreeiter->iterator_.min_ = *minkey;
        }

        if (maxkey == NULL || maxkey->field_count_ == 0)
        {
            ttreeiter->iterator_.max_.field_count_ = 0;
            ttreeiter->iterator_.max_.field_value_ = NULL;
        }
        else
        {
            if (key_include)
            {
                for (i = 0; i < maxkey->field_count_; i++)
                {
                    for (j = 0; j < ttree->key_desc_.field_count_; j++)
                    {
                        if (maxkey->field_value_[i].position_ ==
                                ttree->key_desc_.field_desc_[j].position_)
                        {
                            maxkey->field_value_[i].order_ =
                                    ttree->key_desc_.field_desc_[j].order_;
                            maxkey->field_value_[i].h_offset_ =
                                    ttree->key_desc_.field_desc_[j].h_offset_;
                            maxkey->field_value_[i].fixed_part =
                                    ttree->key_desc_.field_desc_[j].fixed_part;
                            maxkey->field_value_[i].collation =
                                    ttree->key_desc_.field_desc_[j].collation;
                            break;
                        }
                    }
                }
            }
            ttreeiter->iterator_.max_ = *maxkey;
        }

        UnsetBufSegment(issys_ttree);

        if (f == NULL || f->expression_count_ == 0)
        {
            ttreeiter->iterator_.filter_.expression_count_ = 0;
            ttreeiter->iterator_.filter_.expression_ = NULL;
        }
        else
        {
            ttreeiter->iterator_.filter_ = *f;
        }

        ttreeiter->iterator_.key_include_ = key_include;

        /* reduce the overhead of calling sc_time */
        ttreeiter->iterator_.open_time_--;
        ttreeiter->iterator_.qsid_ = qsid;

        ttreeiter->iterator_.real_next = NULL_OID;

        TTreeIter_Create(ttreeiter);
    }

    Iter->fInit = 1;

    result = DB_SUCCESS;

  End_Processing:
    if (result < 0)
    {
        if (Iter)
        {
            Iter->min_.field_count_ = 0;
            Iter->min_.field_value_ = NULL;
            Iter->max_.field_count_ = 0;
            Iter->max_.field_value_ = NULL;
            Iter->filter_.expression_count_ = 0;
            Iter->filter_.expression_ = NULL;
        }

        return DB_FAIL;
    }
    else
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
Get_CursorPos(int cursorId, struct Cursor_Position *cursorPos)
{
    struct Iterator *Iter;

    /* second argument value 2: do not check interrupt */
    Iter = (struct Iterator *) Iter_Attach(cursorId, 2, 0);
    if (Iter == NULL)
        return DB_FAIL;

    if (Iter->type_ == SEQ_CURSOR)
    {
        struct ContainerIterator *ContIter = (struct ContainerIterator *) Iter;

        cursorPos->cp.contIter.first_ = ContIter->first_;
        cursorPos->cp.contIter.last_ = ContIter->last_;
        cursorPos->cp.contIter.curr_ = ContIter->curr_;
    }
    else
    {
        struct TTreeIterator *ttreeiter = (struct TTreeIterator *) Iter;

        cursorPos->cp.ttreeIter.first_ = ttreeiter->first_;
        cursorPos->cp.ttreeIter.last_ = ttreeiter->last_;
        cursorPos->cp.ttreeIter.curr_ = ttreeiter->curr_;
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
Set_CursorPos(int cursorId, struct Cursor_Position *cursorPos)
{
    int cursorpos;
    struct Iterator *Iter;

    cursorpos = CursorPosition(cursorId);
    if (cursorpos == NULL_Cursor_Position)
    {
        return DB_FAIL;
    }

    Iter = (struct Iterator *) &Cursor_Mgr->cursor[cursorpos].chunk_;

    if (Iter->type_ == SEQ_CURSOR)
    {
        struct ContainerIterator *ContIter = (struct ContainerIterator *) Iter;

        ContIter->first_ = cursorPos->cp.contIter.first_;
        ContIter->last_ = cursorPos->cp.contIter.last_;
        ContIter->curr_ = cursorPos->cp.contIter.curr_;
    }
    else
    {
        struct TTreeIterator *ttreeiter = (struct TTreeIterator *) Iter;

        ttreeiter->first_ = cursorPos->cp.ttreeIter.first_;
        ttreeiter->last_ = cursorPos->cp.ttreeIter.last_;
        ttreeiter->curr_ = cursorPos->cp.ttreeIter.curr_;
        Iter->real_next = NULL_OID;
    }

    return DB_SUCCESS;
}

int
Make_CursorPos(int cursorId, OID rid_to_make)
{
    struct Cursor_Position cursorPos;

    struct Iterator *Iter;
    struct Container *ridCont = NULL;
    struct TTreeIterator *ttreeiter;
    DB_UINT16 ridCont_record_size = 0;

    int csrID, ret;
    int tfound = 0;
    int issys_ttree;
    struct TTree *ttree;

    FIELDVALUES_T rec_values;
    RecordSize dummy_size;

    ret = Get_CursorPos(cursorId, &cursorPos);
    if (ret < 0)
        return ret;

    csrID = cursorId;

    /* get single-table's or real-table-cursors' position
       of first great than equan the value of the rid */
    while (csrID != -1)
    {
        Iter = (struct Iterator *) Iter_Attach(csrID, 2, 0);
        if (Iter->type_ == SEQ_CURSOR)
        {
            cursorPos.cp.contIter.curr_ = rid_to_make;
            tfound = 1;
        }
        else
        {
            ret = Col_check_valid_rid(NULL, rid_to_make);
            if (ret < 0)
                return DB_E_INVALID_RID;

            ttreeiter = (struct TTreeIterator *) Iter;

            if (!ttreeiter->first_.node_ && !ttreeiter->last_.node_ &&
                    !ttreeiter->curr_.node_)
            {
                ret = DB_E_NOMORERECORDS;
            }
            else
            {
                if (ridCont == NULL)
                {
                    ridCont = Cont_get(rid_to_make);
                    if (ridCont == NULL)
                        return DB_E_INVALID_RID;

                    ridCont_record_size = ridCont->collect_.record_size_;
                }

                SetBufSegment(issys_ttree, ttreeiter->tree_oid_);

                ttree = (struct TTree *) OID_Point(ttreeiter->tree_oid_);

                ret = TTree_Get_Position(ttree, rid_to_make,
                        Iter->type_, ridCont_record_size,
                        &cursorPos.cp.ttreeIter.curr_);

                UnsetBufSegment(issys_ttree);
                if (ret < 0)
                {
                    if (ret != DB_E_NOMORERECORDS)
                        return ret;
                }
                else
                    tfound = 1;
            }

            if (ret != DB_E_NOMORERECORDS)
            {
                Set_CursorPos(csrID, &cursorPos);
                if (ret >= DB_SUCCESS &&
                        ttreeiter->iterator_.filter_.expression_count_)
                {
                    OID rid = Iter_Current(Iter);
                    char *record = (char *) OID_Point(rid);
                    int issys;

                    if (record == NULL)
                    {
                        return DB_E_OIDPOINTER;
                    }

                    SetBufSegment(issys, rid);

                    ret = Filter_Compare(&(ttreeiter->iterator_.filter_),
                            record, ttreeiter->iterator_.record_size_);
                    UnsetBufSegment(issys);
                    if (ret != DB_SUCCESS)
                        __Read(csrID, NULL, NULL, 1, &dummy_size, 0);
                }
                Get_CursorPos(csrID, &cursorPos);
            }
        }

        break;
    }

    if (!tfound)
        return DB_E_NOMORERECORDS;

    /* set the positions which may be previous */
    ret = Set_CursorPos(cursorId, &cursorPos);
    if (ret < 0)
        return ret;

    /* find the rid & set cursor there */
    sc_memset(&rec_values, 0, sizeof(FIELDVALUES_T));
    rec_values.fld_cnt = 1;     /* it hasn't mean, just for doing __Read() */
    while (rec_values.rec_oid != rid_to_make)
    {
        ret = __Read(cursorId, NULL, &rec_values, 1, &dummy_size, 0);
        if (ret < 0)
        {
            if (ret == DB_E_NOMORERECORDS || ret == DB_E_INDEXCHANGED)
                return DB_E_INVALID_RID;
            return ret;
        }
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
struct Cursor *
Cursor_get_schema_info(DB_INT32 cursor_id,
        SYSTABLE_T ** table_info, SYSFIELD_T ** fields_info)
{
    struct Cursor *pCursor;

    pCursor = &(Cursor_Mgr->cursor[cursor_id - 1]);

    if (table_info)
        *table_info = pCursor->table_info;

    if (fields_info)
        *fields_info = pCursor->fields_info;

    return pCursor;
}

////////////////////////////////////////////////////////////////////////
////
//// Function name:  Cursor Close API
////
//////////////////////////////////////////////////////////////////////////

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
Close_Cursor(DB_INT32 cursor_id)
{
    return __close_cursor(cursor_id, 1);
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
_PRIVATE int
__close_cursor(DB_INT32 cursor_id, int flag)
{
    Position pos;
    struct Iterator *Iter;
    int transid = *(int *) CS_getspecific(my_trans_id);
    int ret = DB_SUCCESS;
    int itsmine = 0;
    LOCKMODE lock_mode = LK_NOLOCK;

    if (cursor_id == 0)
        return DB_FAIL;

    pos = CursorPosition(cursor_id);

    if (pos == NULL_Cursor_Position)
    {
        return DB_FAIL;
    }

    if (Cursor_Mgr->cursor[pos].fAllocated_ == 0)
    {
        ret = DB_SUCCESS;
        goto unlock;
    }

    if (Cursor_Mgr->cursor[pos].trans_id_ != transid)
    {
        MDB_SYSLOG(("FAIL: Cursor close, transid mismatch, %d, %d\n",
                        Cursor_Mgr->cursor[pos].trans_id_, transid));
        ret = DB_FAIL;
        goto unlock;
    }

    if (Cursor_Mgr->cursor[pos].table_info != NULL)
    {
        PMEM_FREENUL(Cursor_Mgr->cursor[pos].table_info);
        Cursor_Mgr->cursor[pos].table_info = NULL;
    }

    if (Cursor_Mgr->cursor[pos].fields_info != NULL)
    {
        PMEM_FREENUL(Cursor_Mgr->cursor[pos].fields_info);
        Cursor_Mgr->cursor[pos].fields_info = NULL;
    }

    Iter = (struct Iterator *) &Cursor_Mgr->cursor[pos].chunk_;
    lock_mode = Cursor_Mgr->cursor[pos].lock_mode_;

    itsmine = 1;

    if (Cursor_Mgr->cursor[pos].pUpd_idx_id_array != NULL)
    {
        PMEM_FREENUL(Cursor_Mgr->cursor[pos].pUpd_idx_id_array);
        Cursor_Mgr->cursor[pos].pUpd_idx_id_array = NULL;
    }

    if (flag == 1)
    {
        struct Cursor *cursor;
        struct Cursor *prev;

        cursor = Cursor_Mgr->cursor[pos].trans_ptr->cursor_list;
        prev = NULL;
        while (1)
        {
            if (cursor == NULL)
                break;

            if (cursor == &Cursor_Mgr->cursor[pos])
            {
                if (prev == NULL)
                    Cursor_Mgr->cursor[pos].trans_ptr->cursor_list =
                            cursor->trans_next;
                else
                    prev->trans_next = cursor->trans_next;

                cursor->trans_next = NULL;

                break;
            }

            prev = cursor;
            cursor = cursor->trans_next;
        }
    }

    Cursor_RemovePosition(pos, flag);

  unlock:
    if (itsmine)
    {
        /* system table인 경우 lock item 제거 */
        if (isSysTable2(Iter->creator_))
        {
            int trans_id;

            trans_id = *(int *) CS_getspecific(my_trans_id);

            if (trans_id != -1 && lock_mode != LK_NOLOCK)
                LockMgr_Table_Unlock(trans_id, Iter->id_);
        }
    }

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
_PRIVATE void
Cursor_RemovePosition(Position pos, int flag)
{
    struct Iterator *Iter;

    Iter = (struct Iterator *) &Cursor_Mgr->cursor[pos].chunk_;

    if (Iter->key_include_)
    {
        KeyValue_Delete(&Iter->min_);
        KeyValue_Delete(&Iter->max_);
        Filter_Delete(&Iter->filter_);
    }
    else
    {
        sc_memset(&Iter->min_, 0, sizeof(struct KeyValue));
        sc_memset(&Iter->max_, 0, sizeof(struct KeyValue));
        sc_memset(&Iter->filter_, 0, sizeof(struct Filter));
    }

    Cursor_Mgr->cursor[pos].fAllocated_ = 0;
    Cursor_Mgr->cursor[pos].whereami = 0;
    Cursor_Mgr->cursor[pos].trans_id_ = 0;
    Cursor_Mgr->cursor[pos].trans_ptr = NULL;

    /* connect it into the free cursor list */
    Cursor_Mgr->cursor[pos].next_ = Cursor_Mgr->free_first_;
    Cursor_Mgr->free_first_ = pos;

    return;
}

/* transaction commit/abort시나 client의 connection이 끊어졌을 때
   불리어져서 열려있는 cursor를 닫는다 */
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
void
Cursor_Scan_Dangling(int transId)
{
    int position;

    for (position = 0; position < MAX_ALLOC_CURSORID_NUM; position++)
    {
        if (Cursor_Mgr->cursor[position].fAllocated_)
        {
            if (Cursor_Mgr->cursor[position].trans_id_ == transId)
            {
                __close_cursor(position + 1, 0);
            }
        }
    }

    return;
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
Cursor_isTempTable(int cursorid)
{
    Position pos;

    pos = CursorPosition(cursorid);

    if (pos == NULL_Cursor_Position)
        return DB_E_INVALIDCURSORID;

    if (Cursor_Mgr->cursor[pos].cont_type & _CONT_TEMPTABLE_)
        return TRUE;

    return FALSE;
}

int
Cursor_isNologgingTable(int cursorid)
{
    Position pos;

    pos = CursorPosition(cursorid);

    if (pos == NULL_Cursor_Position)
        return DB_E_INVALIDCURSORID;

    if (Cursor_Mgr->cursor[pos].cont_type & _CONT_NOLOGGING)
        return TRUE;

    return FALSE;
}
