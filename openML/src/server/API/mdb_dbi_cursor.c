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

/*****************************************************************************/
//! dbi_Cursor_Open
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
************************************
 * \param connid(in)        :    connection identifier
 * \param relation_name(in) :    table name
 * \param indexName(in)     :    index name
 * \param min(in)           :    min value
 * \param max(in)           :    max value
 * \param filter(in)        :    filter
 * \param lock(in)          :    lock mode
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - index를 가지고 cursor를 여는 경우라면
 *      index fileds info를 가지고 와서 요넘으로
 *      key/filter의 collation 비교 검사
 *****************************************************************************/
#if defined(MDB_DEBUG)
static int
check_userid(int connid)
{
    int userid = _USER_USER;
    char *conn_username = NULL;

    CS_Get_UserName(connid, &conn_username);
    return userid;
}
#endif

int
dbi_Cursor_Open(int connid, char *relation_name, char *indexName,
        struct KeyValue *min, struct KeyValue *max,
        struct Filter *filter, LOCKMODE lock, DB_UINT8 msync_flag)
{
    int userid = _USER_USER;

#if defined(MDB_DEBUG)
    userid = check_userid(connid);
#endif

    return dbi_Cursor_Open_byUser(connid, relation_name, indexName,
            min, max, filter, lock, userid, msync_flag);
}

int
dbi_Cursor_Open_byUser(int connid, char *relation_name, char *indexName,
        struct KeyValue *min, struct KeyValue *max,
        struct Filter *filter, LOCKMODE lock, int userid, DB_UINT8 msync_flag)
{
    SYSINDEX_T sysIndex;
    int *pTransid;
    int ret = -1;
    struct Container *cont;
    int issys_cont = 0;

    OID index_cont_oid = NULL_OID;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (lock == LK_SCHEMA_SHARED || lock > LK_SCHEMA_EXCLUSIVE)
    {
        return DB_E_INVALIDLOCKMODE;
    }

    if (relation_name == NULL || relation_name[0] == '\0')
    {
        return DB_E_INVALIDPARAM;
    }
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);

    if (pTransid == NULL || *pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    cont = Cont_Search(relation_name);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }

    SetBufSegment(issys_cont, cont->collect_.cont_id_);

    if (indexName == NULL)
    {       // index를 사용하지 않는 것으로 index 번호를 -1로 해서 서버에게 알림
        sysIndex.indexId = -1;
    }
    else
    {       // index 이름에 해당되는 id를 찾아서 서버에게 넘김
        ret = _Schema_GetIndexInfo2(indexName, &sysIndex, NULL, NULL,
                cont->collect_.cont_id_, &index_cont_oid);
        if (ret < 0)
        {
            MDB_SYSLOG(("ERROR: cursor open: Can't get index(%s) info %d\n",
                            indexName, ret));
            goto end;
        }
    }

    /* system table에 대한 write 요구 open인지 검사
       그렇다면 허용하지 않음 */
    if (lock >= LK_EXCLUSIVE)
    {
        if (isSysTable(cont) ||
                (userid == _USER_USER && !doesUserCreateTable(cont, userid)))
        {
            ret = DB_E_NOTPERMITTED;
            goto end;
        }
    }

    if (lock != LK_NOLOCK && !isTempTable_name(relation_name))
    {
        ret = LockMgr_Lock(relation_name, *pTransid, LOWPRIO, lock, WAIT);
        if (ret < 0)
        {
            goto end;
        }
        lock = ret;     /* hold lock mode */
    }

    ret = Open_Cursor2(*pTransid, relation_name, lock,
            sysIndex.indexId, min, max, filter, index_cont_oid, msync_flag);

    goto end_return;

  end:
    KeyValue_Delete(min);
    KeyValue_Delete(max);
    Filter_Delete(filter);

  end_return:
    UnsetBufSegment(issys_cont);
    return ret;
}

/*****************************************************************************/
//! dbi_Cursor_Open_Desc
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid         :    connection identifier
 * \param relation_name    :    table name
 * \param indexName     :    index name
 * \param min             :    min value
 * \param max            :    max value
 * \param filter        :    filter
 * \param lock            :    lock mode
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - index를 가지고 cursor를 여는 경우라면
 *      index fileds info를 가지고 와서 요넘으로
 *      key/filter의 collation 비교 검사
 *****************************************************************************/
int
dbi_Cursor_Open_Desc(int connid, char *relation_name, char *indexName,
        struct KeyValue *min, struct KeyValue *max,
        struct Filter *filter, LOCKMODE lock)
{
    int userid = _USER_USER;

#if defined(MDB_DEBUG)
    userid = check_userid(connid);
#endif

    return dbi_Cursor_Open_Desc_byUser(connid, relation_name, indexName,
            min, max, filter, lock, userid);
}

int
dbi_Cursor_Open_Desc_byUser(int connid, char *relation_name, char *indexName,
        struct KeyValue *min, struct KeyValue *max,
        struct Filter *filter, LOCKMODE lock, int userid)
{
    SYSINDEX_T sysIndex;
    int *pTransid;
    int ret = -1;
    struct Container *cont;
    int issys_cont = 0;

    SYSINDEXFIELD_T *pSysIndexField = NULL;
    int i, j;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (lock == LK_SCHEMA_SHARED || lock > LK_SCHEMA_EXCLUSIVE)
    {
        return DB_E_INVALIDLOCKMODE;
    }

    if (relation_name == NULL || relation_name[0] == '\0')
    {
        return DB_E_INVALIDPARAM;
    }
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (pTransid == NULL || *pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    cont = Cont_Search(relation_name);

    SetBufSegment(issys_cont, cont->collect_.cont_id_);

    if (indexName == NULL)
    {       // index를 사용하지 않는 것으로 index 번호를 -1로 해서 서버에게 알림
        sysIndex.indexId = -1;
    }
    else
    {       // index 이름에 해당되는 id를 찾아서 서버에게 넘김
        ret = _Schema_GetIndexInfo(indexName, &sysIndex, NULL, NULL);
        if (ret < 0)
        {
            MDB_SYSLOG(("ERROR: cursor open: Can't get index(%s) info %d\n",
                            indexName, ret));
            goto end;
        }
        pSysIndexField =
                (SYSINDEXFIELD_T *) PMEM_ALLOC(sizeof(SYSINDEXFIELD_T) *
                sysIndex.numFields);
        if (pSysIndexField == NULL)
        {
            ret = DB_E_OUTOFMEMORY;
            goto end;
        }

        ret = _Schema_GetIndexInfo(indexName, &sysIndex, pSysIndexField, NULL);
        if (ret < 0)
        {
            MDB_SYSLOG(("ERROR: cursor open: Can't get index(%s) info %d\n",
                            indexName, ret));
            goto end;
        }

        if (min)
        {
            for (i = 0; i < sysIndex.numFields; ++i)
            {
                for (j = 0; j < min->field_count_; ++j)
                {
                    if ((pSysIndexField[i].fieldId ==
                                    min->field_value_[j].position_)
                            && ((MDB_COL_TYPE) pSysIndexField[i].collation !=
                                    min->field_value_[j].collation))
                    {
                        ret = DB_E_MISMATCH_COLLATION;
                        goto end;
                    }
                }
            }
        }

        if (max)
        {
            for (i = 0; i < sysIndex.numFields; ++i)
            {
                for (j = 0; j < max->field_count_; ++j)
                {
                    if ((pSysIndexField[i].fieldId ==
                                    max->field_value_[j].position_)
                            && ((MDB_COL_TYPE) pSysIndexField[i].collation !=
                                    max->field_value_[j].collation))
                    {
                        ret = DB_E_MISMATCH_COLLATION;
                        goto end;
                    }
                }
            }
        }

        if (filter)
        {
            for (i = 0; i < sysIndex.numFields; ++i)
            {
                for (j = 0; j < filter->expression_count_; ++j)
                {
                    if ((pSysIndexField[i].fieldId ==
                                    filter->expression_[j].position_)
                            && ((MDB_COL_TYPE) pSysIndexField[i].collation !=
                                    filter->expression_[j].collation))
                    {
                        ret = DB_E_MISMATCH_COLLATION;
                        goto end;
                    }
                }
            }
        }

        PMEM_FREENUL(pSysIndexField);
    }

    /* system table에 대한 write 요구 open인지 검사
       그렇다면 허용하지 않음 */
    if (lock >= LK_EXCLUSIVE)
    {
        if (isSysTable(cont) ||
                (userid == _USER_USER && !doesUserCreateTable(cont, userid)))
        {
            ret = DB_E_NOTPERMITTED;
            goto end;
        }
    }

    if (lock != LK_NOLOCK)
    {
        ret = LockMgr_Lock(relation_name, *pTransid, LOWPRIO, lock, WAIT);
        if (ret < 0)
        {
            goto end;
        }
        lock = ret;     /* hold lock mode */
    }

    ret = Open_Cursor_Desc(*pTransid, relation_name, lock,
            sysIndex.indexId, min, max, filter);
    goto end_return;

  end:
    PMEM_FREENUL(pSysIndexField);
    KeyValue_Delete(min);
    KeyValue_Delete(max);
    Filter_Delete(filter);

  end_return:
    UnsetBufSegment(issys_cont);
    return ret;
}

/*****************************************************************************/
//! dbi_Cursor_Close
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid    :
 * \param cursorID    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Cursor_Close(int connid, int cursorID)
{
    int retvalue;

    DBI_CHECK_SERVER_TERM();

    retvalue = Close_Cursor(cursorID);

    return retvalue;
}

/*****************************************************************************/
//! dbi_Cursor_Insert
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid        :
 * \param cursorID        :
 * \param record        :
 * \param record_leng    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
 /* INSERTED_RID */
dbi_Cursor_Insert(int connid, int cursorId, char *record, int record_leng,
        int isPreprocessed, OID * inserted_rid)
{
    int *pTransid;
    struct Trans *trans = NULL;
    int retvalue;
    OID record_id;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (record == NULL || inserted_rid == NULL || record_leng <= 0)
    {
        return DB_E_INVALIDPARAM;
    }
#endif

    *inserted_rid = NULL_OID;

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        retvalue = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        retvalue = DB_E_UNKNOWNTRANSID;
        goto end;
    }

    if (!Cursor_isTempTable(cursorId))
    {       /* normal table */
        trans->fReadOnly = 0;

        if (!Cursor_isNologgingTable(cursorId))
            trans->fLogging = 1;

        if (server->fs_full)
        {
            retvalue = DB_E_DISKFULL;
            goto end;
        }
    }

    _g_curr_opeation = CURR_OP_INSERT;
    retvalue = __Insert(cursorId, record, record_leng, &record_id,
            isPreprocessed);
    _g_curr_opeation = CURR_OP_NONE;

    *inserted_rid = record_id;

  end:
    return retvalue;
}

/*****************************************************************************/
//! dbi_Cursor_Distinct_Insert
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid        :
 * \param cursorID        :
 * \param record        :
 * \param record_leng    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Cursor_Distinct_Insert(int connid, int cursorId, char *record,
        int record_leng)
{
    int *pTransid;
    struct Trans *trans = NULL;
    int ret;

    DBI_CHECK_SERVER_TERM();

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }

    if (!Cursor_isTempTable(cursorId))
    {       /* normal table */
        trans->fReadOnly = 0;

        if (!Cursor_isNologgingTable(cursorId))
            trans->fLogging = 1;

        if (server->fs_full)
        {
            ret = DB_E_DISKFULL;
            goto end;
        }
    }

    _g_curr_opeation = CURR_OP_INSERT;
    ret = Insert_Distinct(cursorId, record, record_leng);
    _g_curr_opeation = CURR_OP_NONE;

  end:

    return ret;
}

/*****************************************************************************/
//! dbi_Cursor_Sel_Read
/*! \breif  Selection List에 해당하는 Record를 읽어올때 사용됨
 ************************************
 * \param connid(in)        :
 * \param cursorId(in)      :
 * \param rec_values(in/out):
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - 마지막 인자 page를 없앰(사용되지 않음)
 *****************************************************************************/
int
dbi_Cursor_Sel_Read(int connid, int cursorId, FIELDVALUES_T * rec_values)
{
    int *pTransid;
    int ret;

    struct Container *Cont;

    DBI_CHECK_SERVER_TERM();

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    Cont = Cont_get(rec_values->rec_oid);
    if (Cont == NULL)
    {
        ret = DB_E_PIDPOINTER;
        goto end;
    }

    ret = Col_Read_Values(Cont->collect_.record_size_, rec_values->rec_oid,
            rec_values, 0);

  end:

    return ret;
}

/*****************************************************************************/
//! dbi_Cursor_Read
/*! \breif  Record를 읽어올때 사용됨
 ************************************
 * \param connid(in)        :
 * \param cursorId(in)      :
 * \param record(in/out)    : memory record
 * \param record_len(in/out): memory record's len
 * \param rec_values(in/out):
 * \param direct(in)        :
 * \param page(in)          :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Cursor_Read(int connid, int cursorId, char *record, int *record_leng,
        FIELDVALUES_T * rec_values, int direct, OID page)
{
    int *pTransid;
    int ret;

    DBI_CHECK_SERVER_TERM();

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    ret = __Read(cursorId, record, rec_values, direct,
            (DB_UINT32 *) record_leng, page);

  end:

    return ret;
}

int
dbi_Cursor_GetRid(int connid, int cursorId, OID * rid)
{
    int *pTransid;
    int ret;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (rid == NULL)
    {
        return DB_E_INVALIDPARAM;
    }
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    ret = Find_GetRid(cursorId, rid);

  end:
    return ret;
}

int
dbi_Cursor_Seek(int connid, int cursorId, int offset)
{
    int *pTransid;
    int ret = 0;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (offset < 0)
        return DB_E_INVALIDPARAM;
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    ret = Find_Seek(cursorId, offset);

  end:

    return ret;
}

/*****************************************************************************/
//! dbi_Cursor_Reopen
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid(in)    : connection id
 * \param cursorId(in)  : cursorID
 * \param min(in)       : key's min value
 * \param max(in)       : key's max value
 * \param filter(in)    : filter
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - called : reopen_cursor()
 *****************************************************************************/
int
dbi_Cursor_Reopen(int connid, int cursorId, struct KeyValue *min,
        struct KeyValue *max, struct Filter *filter)
{
    int ret;
    int *pTransid;

#ifdef CHECK_DBI_PARAM
    if (server == NULL || server->fTerminated)
    {
        KeyValue_Delete(min);
        KeyValue_Delete(max);
        Filter_Delete(filter);

        return DB_E_TERMINATED;
    }
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        KeyValue_Delete(min);
        KeyValue_Delete(max);
        Filter_Delete(filter);

        return DB_E_NOTTRANSBEGIN;
    }

    ret = Reopen_Cursor(cursorId, min, max, filter);

    return ret;
}

/*****************************************************************************/
//! dbi_Cursor_Remove
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid    :
 * \param cursorId    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Cursor_Remove(int connid, int cursorId)
{
    int *pTransid;
    int ret = DB_SUCCESS;
    struct Trans *trans = NULL;

    DBI_CHECK_SERVER_TERM();

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }


    if (!Cursor_isTempTable(cursorId))
    {       /* normal table */
        trans->fReadOnly = 0;

        if (!Cursor_isNologgingTable(cursorId))
            trans->fLogging = 1;

        if (server->fs_full)
        {
            ret = DB_E_DISKFULL;
            goto end;
        }
    }

    trans->fDeleted = 1;

    ret = __Remove(cursorId);

  end:

    return ret;
}

/*****************************************************************************/
//! dbi_Cursor_Update
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid        :
 * \param cursorId        :
 * \param record        :
 * \param record_leng    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Cursor_Update(int connid, int cursorId, char *record, int record_leng,
        int *col_list)
{
    int *pTransid;
    struct Trans *trans = NULL;
    int ret;

    DBI_CHECK_SERVER_TERM();

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }


    if (!Cursor_isTempTable(cursorId))
    {       /* normal table */
        trans->fReadOnly = 0;

        if (!Cursor_isNologgingTable(cursorId))
            trans->fLogging = 1;

        if (server->fs_full)
        {
            ret = DB_E_DISKFULL;
            goto end;
        }
    }

    _g_curr_opeation = CURR_OP_UPDATE;
    ret = __Update(cursorId, record, record_leng, col_list);
    _g_curr_opeation = CURR_OP_NONE;

  end:

    return ret;
}

/*****************************************************************************/
//! dbi_Cursor_Update_Field
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid    :
 * \param cursorId    :
 * \param newValues    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Cursor_Update_Field(int connid, int cursorId,
        struct UpdateValue *newValues)
{
    int *pTransid;
    struct Trans *trans = NULL;
    int ret;

    DBI_CHECK_SERVER_TERM();

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }


    if (!Cursor_isTempTable(cursorId))
    {       /* normal table */
        trans->fReadOnly = 0;

        if (!Cursor_isNologgingTable(cursorId))
            trans->fLogging = 1;

        if (server->fs_full)
        {
            ret = DB_E_DISKFULL;
            goto end;
        }
    }

    _g_curr_opeation = CURR_OP_UPDATE;
    ret = Update_Field(cursorId, newValues);
    _g_curr_opeation = CURR_OP_NONE;

  end:

    return ret;
}

/*****************************************************************************/
//! dbi_Cursor_Count
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid        :
 * \param relation_name    :
 * \param indexName        :
 * \param min            :
 * \param max            :
 * \param filter        :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Cursor_Count(int connid, char *relation_name, char *indexName,
        struct KeyValue *min, struct KeyValue *max, struct Filter *filter)
{
    int *pTransid;
    int ret;

    struct Container *cont;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (relation_name == NULL || relation_name[0] == '\0')
        return DB_E_INVALIDPARAM;
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    cont = Cont_Search(relation_name);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }

    ret = LockMgr_Lock(relation_name, *pTransid, LOWPRIO, LK_SHARED, WAIT);
    if (ret < 0)
    {
        goto end;
    }

    ret = Cont_Item_Count(relation_name);

  end:
    KeyValue_Delete(min);
    KeyValue_Delete(max);
    Filter_Delete(filter);

    return ret;
}

int
dbi_Cursor_DirtyCount(int connid, char *relation_name)
{
    int *pTransid;
    int ret;

    struct Container *cont;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (relation_name == NULL || relation_name[0] == '\0')
        return DB_E_INVALIDPARAM;
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    cont = Cont_Search(relation_name);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }

    ret = Cont_Item_Count(relation_name);

  end:

    return ret;
}

int
dbi_Cursor_Count2(int connid, char *relation_name, char *indexName,
        struct KeyValue *min, struct KeyValue *max, struct Filter *filter)
{
    int *pTransid;
    int cursorid = -1;
    int dummy_len;
    int ret = DB_SUCCESS;
    int tot_count = 0;
    struct Container *cont;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (relation_name == NULL || relation_name[0] == '\0')
        return DB_E_INVALIDPARAM;
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    cont = Cont_Search(relation_name);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }

    cursorid = dbi_Cursor_Open(connid, relation_name,
            indexName, min, max, filter, LK_SHARED, 0);

    if (cursorid < 0)
    {
        ret = cursorid;
        goto end;
    }

    if (indexName && filter == NULL)
    {
        struct Iterator *Iter;
        struct TTreeIterator *ttreeiter;
        int issys_ttree;

        Iter = (struct Iterator *) Iter_Attach(cursorid, 1, 0);
        if (Iter == NULL)
        {
            ret = DB_E_INVALIDCURSORID;
            goto end;
        }

        ttreeiter = (struct TTreeIterator *) Iter;

        SetBufSegment(issys_ttree, ttreeiter->tree_oid_);

        ret = TTree_Get_ItemCount_Interval(
                (struct TTree *) OID_Point(ttreeiter->tree_oid_),
                &(ttreeiter->first_), &(ttreeiter->last_), Iter->type_);

        UnsetBufSegment(issys_ttree);

        goto end;
    }

    while (1)
    {
        ret = __Read(cursorid, NULL, NULL, 1, (DB_UINT32 *) & dummy_len, 0);
        if (ret < 0)
        {
            if (ret == DB_E_NOMORERECORDS || ret == DB_E_INDEXCHANGED)
                ret = DB_SUCCESS;
            break;
        }
        tot_count += 1;
    }

    if (ret == DB_SUCCESS)
        ret = tot_count;

  end:
    if (cursorid > 0)
        dbi_Cursor_Close(connid, cursorid);

    return ret;
}

/*****************************************************************************/
//! dbi_Cursor_GetPosition
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid    :
 * \param cursorId    :
 * \param cursorPos    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Cursor_GetPosition(int connid, int cursorId,
        struct Cursor_Position *cursorPos)
{
    int ret;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (cursorPos == NULL)
        return DB_E_INVALIDPARAM;
#endif

    ret = Get_CursorPos(cursorId, cursorPos);

    return ret;
}

/*****************************************************************************/
//! dbi_Cursor_SetPosition
/*! \breif  간단하게 어떤 함수인지스트용 \n
 ************************************
 * \param connid    :
 * \param cursorId    :
 * \param cursorPos    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Cursor_SetPosition(int connid, int cursorId,
        struct Cursor_Position *cursorPos)
{
    int ret;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (cursorPos == NULL)
        return DB_E_INVALIDPARAM;
#endif

    ret = Set_CursorPos(cursorId, cursorPos);

    return ret;
}

int
dbi_Cursor_SetPosition_UnderRid(int connid, int cursorId, OID set_here)
{
    int ret = DB_SUCCESS;

    DBI_CHECK_SERVER_TERM();

    /* make cursorPos by set_here */
    ret = Make_CursorPos(cursorId, set_here);
    if (ret < 0)
    {
        goto end;
    }

  end:
    return ret;
}

int
dbi_Cursor_Insert_Variable(int connid, int cursorId, char *value,
        int exted_bytelen, OID * var_rid)
{
    int *pTransid;
    struct Trans *trans = NULL;

    struct Iterator *Iter;
    int ret = -1;
    struct Container *Cont;
    int issys_cont;

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }

    Iter = (struct Iterator *) Iter_Attach(cursorId, 0, 0);

    if (!Cursor_isTempTable(cursorId))
    {       /* normal table */
        trans->fReadOnly = 0;

        if (!Cursor_isNologgingTable(cursorId))
            trans->fLogging = 1;

        if (server->fs_full)
        {
            ret = DB_E_DISKFULL;
            goto end;
        }
    }

    Cont = (struct Container *) OID_Point(Iter->cont_id_);
    SetBufSegment(issys_cont, Iter->cont_id_);

    _g_curr_opeation = CURR_OP_INSERT;
    ret = Col_Insert_Variable(Cont, value, exted_bytelen, var_rid);
    _g_curr_opeation = CURR_OP_NONE;

    UnsetBufSegment(issys_cont);

  end:
    return ret;
}

/* except와 intersect는 record자체가 존재하는지 확인이 필요하기 때문에
   record 단위 비교를 하는 함수가 필요 */
int
dbi_Cursor_Exist_by_Record(int connid, int cursorId, char *record,
        int record_leng)
{
    int *pTransid;
    struct Trans *trans = NULL;
    int ret;

    DBI_CHECK_SERVER_TERM();

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }

    ret = Exist_Record(cursorId, record, record_leng);

  end:
    return ret;
}

int
dbi_Cursor_Set_updidx(int connid, int cursorId, int *col_pos)
{
    int *pTransid;
    struct Cursor *pCursor;
    struct Container *Cont;
    struct TTree *ttree;
    int index_id;
    int i, j, ret;
    struct Update_Idx_Id_Array *pArray;
    int csr_id;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (col_pos == NULL)
    {
        return DB_E_INVALIDPARAM;
    }
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    csr_id = cursorId;

    while (csr_id != -1)
    {
        pCursor = GET_CURSOR_POINTER(csr_id);
        if (pCursor == NULL)
        {
            ret = DB_E_INVALIDCURSORID;
            goto end;
        }

        Cont = (struct Container *) OID_Point(pCursor->cont_id_);
        if (Cont == NULL)
        {
            ret = DB_E_OIDPOINTER;
            goto end;
        }

        index_id = Cont->base_cont_.index_id_;
        if (index_id)
        {
            pArray = PMEM_ALLOC(sizeof(struct Update_Idx_Id_Array));
            if (pArray == NULL)
            {
                ret = DB_E_OUTOFMEMORY;
                goto end;
            }

            pArray->cnt = 0;

            while (index_id)
            {
                ttree = (struct TTree *) OID_Point(index_id);
                if (ttree == NULL)
                {
                    PMEM_FREENUL(pArray);
                    ret = DB_E_OIDPOINTER;
                    goto end;
                }

                for (i = 0; i < (ttree->key_desc_.field_count_); i++)
                {
                    for (j = 1; j <= col_pos[0]; j++)
                    {
                        if (ttree->key_desc_.field_desc_[i].position_ ==
                                col_pos[j])
                        {
                            pArray->idx_oid[pArray->cnt] = index_id;
                            pArray->cnt += 1;
                            break;
                        }
                    }

                    if (j < col_pos[0])
                    {
                        break;
                    }
                }

                index_id = ttree->base_cont_.index_id_;
            }

            pCursor->pUpd_idx_id_array = pArray;
        }

        break;
    }

    ret = DB_SUCCESS;

  end:
    return ret;
}

// Record의 내용을 비교하여 동일한지 다른지 검사
// 동일 한 경우 Upsert 명령은 아무것도 수행하지 않음.
// 다른 경우 Upsert 명령은 Insert 수행
static int
compare_record(char *memory_record, OID rid, SYSTABLE_T * table_info,
        SYSFIELD_T * fields_info, int *compareFields)
{
    int i, ret = 0;

    char *value, *h_value;
    char *heap_record;
    struct FieldValue field;
    int issys = TRUE;

    SetBufSegment(issys, rid);

    heap_record = (char *) OID_Point(rid);
    if (heap_record == NULL)
    {
        ret = DB_E_OIDPOINTER;
        goto RETURN_LABEL;
    }

    for (i = 0; i < table_info->numFields; i++)
    {
        if (compareFields != NULL)
        {
            if (!compareFields[i])
                continue;
        }

        if (DB_VALUE_ISNULL
                (memory_record, fields_info[i].position,
                        table_info->recordLen))
        {
            if (DB_VALUE_ISNULL
                    (heap_record, fields_info[i].position,
                            table_info->h_recordLen))
            {
                continue;
            }
            else
            {
                ret = 1;
                goto RETURN_LABEL;
            }
        }
        else if (DB_VALUE_ISNULL
                (heap_record, fields_info[i].position,
                        table_info->h_recordLen))
        {
            ret = 1;
            goto RETURN_LABEL;
        }

        // 성능 개선을 위해서 Code 위치 이동
        value = memory_record + fields_info[i].offset;
        h_value = heap_record + fields_info[i].h_offset;

        if (IS_VARIABLE_DATA(fields_info[i].type, fields_info[i].fixed_part))
        {
            field.value_.pointer_ = value;
            field.mode_ = MDB_EQ;
            field.type_ = fields_info[i].type;
            field.collation = fields_info[i].collation;
            field.length_ = fields_info[i].length;
            field.value_length_ = strlen_func[fields_info[i].type] (value);
            field.h_offset_ = fields_info[i].h_offset;
            field.fixed_part = fields_info[i].fixed_part;
            ret = KeyField_Compare(&field, heap_record, table_info->recordLen);
            if (ret != 0)
            {
                goto RETURN_LABEL;
            }
        }
        else
        {
            ret = sc_memcmp(value, h_value, fields_info[i].length);
            if (ret != 0)
            {
                goto RETURN_LABEL;
            }
        }

    }

  RETURN_LABEL:

    UnsetBufSegment(issys);

    return ret;
}

int
__Upsert(int connid, int cursorId, struct Container *cont, char *record,
        int record_leng, int *upsert_cols, OID * inserted_rid,
        UPSERT_CMPMODE compareMode)
{
    int *pTransid;
    struct Trans *trans = NULL;

    OID index_id = NULL_OID;
    OID upsertRid = NULL_OID;

    SYSTABLE_T table_info_space;
    SYSTABLE_T *table_info = &table_info_space;
    SYSFIELD_T *fields_info = NULL;

    struct TTree *ttree = NULL;

    int *compareFields = NULL;
    int retvalue = 0;
    UPSERT_RESULT status = -1;  /* init *//* exist: 0, insert: 1, update: 2 */
    int issys_ttree;

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
        return DB_E_NOTTRANSBEGIN;

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
        return DB_E_UNKNOWNTRANSID;

    upsertRid = NULL_OID;

    index_id = cont->base_cont_.index_id_;

    // Primary Key를 찾는 작업
    while (index_id)
    {
        ttree = (struct TTree *) OID_Point(index_id);
        if (ttree == NULL)
        {
            retvalue = DB_E_OIDPOINTER;
            goto end;
        }
        // 검색하고 대상이 "$pk."로 시작하는지 검사 -> Primary Key
        if (!mdb_strncmp(PRIMARY_KEY_PREFIX, ttree->base_cont_.name_,
                        sc_strlen(PRIMARY_KEY_PREFIX)))
            break;
        // Tree 내에서 다음 Node 값 읽어 옴.
        index_id = ttree->base_cont_.index_id_;
    }

    // Primary Key를 찾지 못하는 경우
    if (!index_id)
        return DB_E_NOTFOUND_PRIMARYKEY;

    SetBufSegment(issys_ttree, ttree->collect_.cont_id_);

    // 작업 대상이 되는 Rid가 TTree에 있는지 검사
    upsertRid = TTree_IsExist(ttree, record, NULL_OID,
            cont->base_cont_.memory_record_size_, 1);

    UnsetBufSegment(issys_ttree);

    // 동일한 Rid가 존재하지 않는 경우
    if (upsertRid == NULL_OID)
    {
        status = UPSERT_INSERT;
    }
    else
    {
        status = UPSERT_EXIST;

        // 작업 대상이 되는 Rid의 Table과 Field 정보 Load
        if (cursorId < 0)       /* direct api */
            dbi_Schema_GetTableFieldsInfo(connid, cont->base_cont_.name_,
                    table_info, &fields_info);
        else
            Cursor_get_schema_info(cursorId, &table_info, &fields_info);

        if (compareMode != UPSERT_CMP_SKIP)
        {
            // Record의 Data가 동일한지 검사
            if (compare_record(record, upsertRid, table_info,
                            fields_info, compareFields) != 0)
            {
                status = UPSERT_UPDATE;
            }
        }
    }

    if (status == UPSERT_EXIST) /* just update utime of the record */
    {
        int utime_offset = cont->collect_.slot_size_ - 8;
        struct Iterator *pIter;
        char *pbuf = NULL;

        pIter = (struct Iterator *) &(Cursor_Mgr->cursor[cursorId - 1]);
        pbuf = (char *) OID_Point(upsertRid);
        if (pbuf == NULL)
        {
            retvalue = DB_E_OIDPOINTER;
            goto end;
        }

        *(DB_UINT32 *) (pbuf + utime_offset) = pIter->open_time_;
    }
    else
    {
        // Temp Table 인지 검사
        // 갱신 작업이 있는 경우 수행
        if (!Cursor_isTempTable(cursorId))
        {   // normal table
            trans->fReadOnly = 0;       // 갱신 작업이 일어나는 경우 설정
            if (!Cursor_isNologgingTable(cursorId))     // Logging, Nologgin 검사
                trans->fLogging = 1;
            if (server->fs_full)
            {
                if (cursorId < 0)       /* direct api */
                    PMEM_FREENUL(fields_info);
                return DB_E_DISKFULL;
            }
        }

        if (status == UPSERT_INSERT)
        {
            if (cursorId < 0)   /* direct api */
                retvalue = Direct_Insert_Internal(trans, cont, record,
                        record_leng, LK_EXCLUSIVE, &upsertRid);
            else
                retvalue =
                        __Insert(cursorId, record, record_leng, &upsertRid, 0);

            *inserted_rid = upsertRid;
        }
        else    // if (status == UPSERT_UPDATE) 
        {
            struct Iterator *pIter = NULL;
            DB_UINT8 msync_flag = 0;

            if (cursorId > -1)
            {
                pIter = (struct Iterator *) Iter_Attach(cursorId, 0, 0);
                if (pIter == NULL)
                {
                    if (er_errid() != DB_E_TRANSINTERRUPT)
                        return DB_E_INVALIDCURSORID;
                    else
                        return DB_E_TRANSINTERRUPT;
                }

                if (pIter->msync_flag)
                {
                    msync_flag = pIter->msync_flag;
                }
            }
            if (upsert_cols)
            {
                char *h_record;

                h_record = (char *) OID_Point(upsertRid);
                if (h_record == NULL)
                {
                    retvalue = DB_E_OIDPOINTER;
                    goto end;
                }
                make_memory_record(cont->collect_.numfields_,
                        cont->collect_.item_size_,
                        cont->collect_.record_size_,
                        cont->base_cont_.memory_record_size_,
                        fields_info, record, h_record, 0, upsert_cols);
            }
            // Data 다른 경우 Update 수행
            retvalue = Direct_Update_Internal(trans, cont, NULL, NULL,
                    upsertRid, record, record_leng, LK_EXCLUSIVE, NULL,
                    msync_flag);
        }
    }

  end:
    if (cursorId < 0)   /* direct api */
        PMEM_FREENUL(fields_info);

    if (retvalue < 0)
        return retvalue;
    else
        return status;
}

int
dbi_Cursor_Upsert(int connid, int cursorId, char *record, int record_leng,
        int *upsert_cols, OID * inserted_rid, UPSERT_CMPMODE compareMode)
{
    int *pTransid;
    struct Trans *trans = NULL;
    struct Iterator *Iter = NULL;
    int ret;
    struct Container *Cont;
    int issys_cont;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (record == NULL)
    {
        return DB_E_INVALIDPARAM;
    }

    if (record_leng <= 0)
    {
        return DB_E_INVALIDPARAM;
    }
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }

    Iter = Iter_Attach(cursorId, 0, 0);

    Cont = (struct Container *) OID_Point(Iter->cont_id_);
    SetBufSegment(issys_cont, Iter->cont_id_);

    ret = __Upsert(connid, cursorId, Cont, record, record_leng, upsert_cols,
            inserted_rid, compareMode);

    UnsetBufSegment(issys_cont);

  end:
    return ret;
}
