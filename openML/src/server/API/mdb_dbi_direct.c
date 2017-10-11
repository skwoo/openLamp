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

int
dbi_Direct_Read_ByRid(int connid, char *tableName, OID rid_to_read,
        SYSTABLE_T * tableinfo, LOCKMODE lock, char *record,
        DB_UINT32 * size, FIELDVALUES_T * rec_values)
{
    int ret;
    SYSTABLE_T real_tableinfo;
    char real_tableName[REL_NAME_LENG];
    int *pTransid;
    int curr_transid = 0;
    struct Trans *trans;

    DBI_CHECK_SERVER_TERM();

    pTransid = (int *) CS_getspecific(my_trans_id);
    curr_transid = *pTransid;
    if (curr_transid == -1)
    {
        ret = dbi_Trans_Begin(connid);
        if (ret < 0)
            goto end_return;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)  /* fix warning */
    {
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }

    if (tableName == NULL)
    {
        ret = dbi_Rid_Tablename(connid, rid_to_read, real_tableName);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            goto end;
        }
    }
    else
    {
        if (tableinfo == NULL)
        {
            ret = dbi_Schema_GetTableInfo(connid, tableName, &real_tableinfo);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                ret = ret;
                goto end;
            }
        }

        sc_strncpy(real_tableName, tableName, REL_NAME_LENG);
    }

    ret = dbi_Direct_Read(connid, real_tableName, NULL, NULL, rid_to_read,
            LK_SHARED, record, size, rec_values);

  end:
    if (curr_transid == -1)
    {
        if (ret < 0)
            dbi_Trans_Abort(connid);
        else
            dbi_Trans_Commit(connid);
    }

  end_return:

    return ret;
}

/*****************************************************************************/
//! dbi_Direct_Remove
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid    :    connection identifier
 * \param tableName    :    table name
 * \param indexName    :    index name
 * \param findKey    :    key
 * \param lock        :    lock mode
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Direct_Remove(int connid, char *tableName, char *indexName,
        struct KeyValue *findKey, OID rid_to_remove, LOCKMODE lock)
{
    int userid = _USER_USER;

#if defined(MDB_DEBUG)
    userid = check_userid(connid);
#endif

    return dbi_Direct_Remove_byUser(connid, tableName, indexName,
            findKey, rid_to_remove, lock, userid);
}

int
dbi_Direct_Remove_ByRid(int connid, char *tableName, OID rid_to_remove,
        SYSTABLE_T * tableinfo, LOCKMODE lock)
{
    int ret;
    SYSTABLE_T real_tableinfo;
    char real_tableName[REL_NAME_LENG];
    int userid = _USER_USER;

    if (tableinfo == NULL)
    {
        ret = dbi_Schema_GetTableInfo(connid, tableName, &real_tableinfo);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            goto end;
        }
    }

    sc_strncpy(real_tableName, tableName, REL_NAME_LENG);

#if defined(MDB_DEBUG)
    userid = check_userid(connid);
#endif

    ret = dbi_Direct_Remove_byUser(connid, real_tableName, NULL, NULL,
            rid_to_remove, lock, userid);

  end:

    return ret;
}

int
dbi_Direct_Remove_byUser(int connid, char *tableName, char *indexName,
        struct KeyValue *findKey, OID rid_to_remove, LOCKMODE lock, int userid)
{
    int *pTransid;
    int curr_transid = 0;
    struct Trans *trans;
    struct Container *cont;
    int ret = DB_SUCCESS;
    int issys_cont = 0;

    DBI_CHECK_SERVER_TERM();
    DBI_CHECK_TABLE_NAME(tableName);

#ifdef MDB_DEBUG
    if (rid_to_remove != NULL_OID && (indexName || findKey))
        sc_assert(0, __FILE__, __LINE__);
#endif
    if (rid_to_remove == NULL_OID)
    {
        DBI_CHECK_INDEX_NAME(indexName);
    }

    pTransid = (int *) CS_getspecific(my_trans_id);
    curr_transid = *pTransid;
    if (curr_transid == -1)
    {
        ret = dbi_Trans_Begin(connid);
        if (ret < 0)
            goto end_return;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }

    if (lock == LK_SCHEMA_SHARED || lock == LK_SHARED)
    {
        MDB_SYSLOG(("Invalid lock mode (%d) for Direct Remove.\n", lock));
        ret = DB_E_INVALIDLOCKMODE;
        goto end;
    }

    if (lock != LK_NOLOCK)
    {
        ret = LockMgr_Lock(tableName, *pTransid, LOWPRIO, lock, WAIT);
        if (ret < 0)
        {
            goto end;
        }
        lock = ret;     /* hold lock mode */
    }

    cont = Cont_Search(tableName);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }

    SetBufSegment(issys_cont, cont->collect_.cont_id_);

    if (isSysTable(cont) ||
            (userid == _USER_USER && !doesUserCreateTable(cont, userid)))
    {
        ret = DB_E_NOTPERMITTED;
        goto end;
    }

    if (!isTempTable(cont))
    {       /* normal table */
        trans->fReadOnly = 0;

        if (!isNoLogging(cont))
            trans->fLogging = 1;

        if (server->fs_full)
        {
            ret = DB_E_DISKFULL;
            goto end;
        }
    }

    trans->fDeleted = 1;

    /* remove record */
    ret = Direct_Remove_Internal(trans, cont, indexName, findKey,
            rid_to_remove, lock, NULL, NULL, 0);

  end:
    if (curr_transid == -1)
    {
        if (ret < 0)
            dbi_Trans_Abort(connid);
        else
            dbi_Trans_Commit(connid);
    }

  end_return:
    UnsetBufSegment(issys_cont);

    return ret;
}

int
dbi_Direct_Remove2_byUser(int connid, int inputCsrId,
        char *relation_name, char *indexName,
        struct KeyValue *min, struct KeyValue *max,
        struct Filter *filter, LOCKMODE lock, int userid)
{
    int *pTransid;
    struct Trans *trans;
    struct Container *cont;
    struct Iterator *pIter;
    struct Cursor *pCursor = NULL;
    MDB_OIDARRAY h_oidset = { NULL, 0, 0 };
    MDB_OIDARRAY v_oidset = { NULL, 0, 0 };
    int curr_transid = 0;
    int csrId = 0, numDeleted = 0;
    int i, issys, ret = DB_SUCCESS;
    char *record;
    int numVariables = 0;
    int nth_collect;
    int issys_cont = 0;

#if defined(MDB_DEBUG)
    userid = check_userid(connid);
#endif

    /****************************
     * STEP 1 : init.
     ****************************/
    pTransid = (int *) CS_getspecific(my_trans_id);
    curr_transid = *pTransid;
    if (curr_transid == -1)
    {
        ret = dbi_Trans_Begin(connid);
        if (ret < 0)
            goto end_return;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }

    /****************************
     * STEP 2 : cursor open.
     ****************************/
    if (inputCsrId < 0)
        csrId = dbi_Cursor_Open_byUser(connid, relation_name, indexName,
                min, max, filter, lock, userid, 0);
    else
        csrId = inputCsrId;

    if (csrId < 0)
    {
        ret = csrId;
        goto end;
    }

    pCursor = GET_CURSOR_POINTER(csrId);

    cont = (struct Container *) OID_Point(pCursor->cont_id_);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto END_OF_OPERATION;
    }

    SetBufSegment(issys_cont, cont->collect_.cont_id_);

    if (isSysTable(cont))
    {
        ret = DB_E_NOTPERMITTED;
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        goto END_OF_OPERATION;
    }

    if (!isTempTable(cont))     /* normal table */
    {
        trans->fReadOnly = 0;

        if (!isNoLogging(cont))
            trans->fLogging = 1;

        if (server->fs_full)
        {
            ret = DB_E_DISKFULL;
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            goto END_OF_OPERATION;
        }
    }

    trans->fDeleted = 1;

    /****************************
     * STEP 3 : find rids
     ****************************/

    pIter = (struct Iterator *) (pCursor->chunk_);

    if (pIter->type_ == SEQ_CURSOR)
    {
        ret = DB_E_NOTSUPPORTED;
        goto END_OF_OPERATION;
    }

    ret = TTreeIter_Create((struct TTreeIterator *) pIter);
    if (ret < 0)
    {
        goto END_OF_OPERATION;
    }

    ret = TTree_GetOidSet((struct TTreeIterator *) pIter, &h_oidset);
    if (ret < 0)
    {
        goto END_OF_OPERATION;
    }

    ret = dbi_Sort_rid(h_oidset.array, h_oidset.count, 1);
    if (ret < 0)
    {
        goto END_OF_OPERATION;
    }

    for (i = 0; i < pCursor->table_info->numFields; i++)
    {
        if (IS_VARIABLE_DATA(pCursor->fields_info[i].type,
                        pCursor->fields_info[i].fixed_part))
            numVariables += 1;
    }

    v_oidset.count = h_oidset.count * numVariables + 1;

    v_oidset.array = PMEM_ALLOC(v_oidset.count * OID_SIZE);
    if (v_oidset.array == NULL)
    {
#if defined(MDB_DEBUG)
        sc_assert(0, __FILE__, __LINE__);
#else
        ret = CS_E_OUTOFMEMORY;
        goto END_OF_OPERATION;
#endif
    }

    /******************************
     * STEP 4 : remove heap records
     ******************************/
    for (i = 0; i < h_oidset.count; i++)
    {
        if (pIter->filter_.expression_count_)
        {
            record = (char *) OID_Point(h_oidset.array[i]);
            if (record == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto END_OF_OPERATION;
            }

            SetBufSegment(issys, h_oidset.array[i]);

            if (Filter_Compare(&(pIter->filter_), record,
                            cont->collect_.record_size_) != DB_SUCCESS)
            {
                UnsetBufSegment(issys);
                continue;
            }

            UnsetBufSegment(issys);
        }

        /*
           __SYSLOG("h_oid(%4d/%4d/%4d)\n", getSegmentNo(h_oidset.array[i]),
           getPageNo(h_oidset.array[i]), OID_GetOffSet(h_oidset.array[i]));
         */

        ret = Direct_Remove_Internal(trans, cont, NULL, NULL,
                h_oidset.array[i], lock, &v_oidset, pCursor->fields_info, 0);

        if (ret < 0)
            break;

        numDeleted += 1;
    }

    /*****************************
     * STEP 5 : remove var records
     *****************************/
    if (v_oidset.idx)
    {
        ret = dbi_Sort_rid(v_oidset.array, v_oidset.idx, 1);
        if (ret < 0)
        {
            goto END_OF_OPERATION;
        }

        nth_collect = Col_get_collect_index(v_oidset.array[0]);
        if (nth_collect < -1)   /* error */
        {
            ret = nth_collect;
            goto END_OF_OPERATION;
        }

        ret = Col_Remove(cont, nth_collect, v_oidset.array[0], 1);
        if (ret < 0)
            goto END_OF_OPERATION;

        for (i = 1; i < v_oidset.idx; i++)
        {
            /*
               __SYSLOG("v_oid(%4d/%4d/%4d)\n", getSegmentNo(v_oidset.array[i]),
               getPageNo(v_oidset.array[i]), OID_GetOffSet(v_oidset.array[i]));
             */

            if (!IsSamePage(v_oidset.array[i - 1], v_oidset.array[i]))
            {
                nth_collect = Col_get_collect_index(v_oidset.array[i]);
                if (nth_collect < -1)   /* error */
                {
                    ret = nth_collect;
                    goto END_OF_OPERATION;
                }
            }

            ret = Col_Remove(cont, nth_collect, v_oidset.array[i], 1);
            if (ret < 0)
                goto END_OF_OPERATION;
        }
    }

    /****************************
     * STEP 5 : end of operation
     ****************************/

  END_OF_OPERATION:
    if (inputCsrId < 0)
        dbi_Cursor_Close(connid, csrId);

  end:
    if (curr_transid == -1)
    {
        if (ret < 0)
            dbi_Trans_Abort(connid);
        else
            dbi_Trans_Commit(connid);
    }

    PMEM_FREENUL(h_oidset.array);
    PMEM_FREENUL(v_oidset.array);

  end_return:
    UnsetBufSegment(issys_cont);

    if (ret == DB_SUCCESS)
        return numDeleted;
    else
        return ret;
}

/*****************************************************************************/
//! dbi_Direct_Update
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid        :    connection identifier
 * \param tableName        :    table name
 * \param indexName        :    index name
 * \param findKey        :    key
 * \param record        :    record
 * \param record_leng    :    record length
 * \param lock            :    lock mode
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Direct_Update(int connid, char *tableName, char *indexName,
        struct KeyValue *findKey, OID rid_to_update, char *record,
        int record_leng, LOCKMODE lock, struct UpdateValue *newValues)
{
    int userid = _USER_USER;

#if defined(MDB_DEBUG)
    userid = check_userid(connid);
#endif

    return dbi_Direct_Update_byUser(connid, tableName, indexName, findKey,
            rid_to_update, record, record_leng, lock, userid, newValues);
}

int
dbi_Direct_Update_byUser(int connid, char *tableName, char *indexName,
        struct KeyValue *findKey, OID rid_to_update, char *record,
        int record_leng, LOCKMODE lock, int userid,
        struct UpdateValue *newValues)
{
    int *pTransid;
    int curr_transid = 0;
    struct Trans *trans;
    struct Container *cont;
    int ret = DB_SUCCESS;
    int issys_cont = 0;

    DBI_CHECK_SERVER_TERM();
    DBI_CHECK_TABLE_NAME(tableName);

#ifdef MDB_DEBUG
    if (rid_to_update != NULL_OID && (indexName || findKey))
        sc_assert(0, __FILE__, __LINE__);
#endif
    if (rid_to_update == NULL_OID)
    {
        DBI_CHECK_INDEX_NAME(indexName);
    }

    DBI_CHECK_RECORD_DATA(record);
    DBI_CHECK_RECORD_LENG(record_leng);

    pTransid = (int *) CS_getspecific(my_trans_id);
    curr_transid = *pTransid;
    if (curr_transid == -1)
    {
        ret = dbi_Trans_Begin(connid);
        if (ret < 0)
            goto end_return;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }

    if (lock == LK_SCHEMA_SHARED || lock == LK_SHARED)
    {
        ret = DB_E_INVALIDLOCKMODE;
        goto end;
    }

    if (lock != LK_NOLOCK)
    {
        ret = LockMgr_Lock(tableName, *pTransid, LOWPRIO, lock, WAIT);
        if (ret < 0)
        {
            goto end;
        }
        lock = ret;     /* hold lock mode */
    }

    cont = Cont_Search(tableName);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }

    SetBufSegment(issys_cont, cont->collect_.cont_id_);

    if (isSysTable(cont) ||
            (userid == _USER_USER && !doesUserCreateTable(cont, userid)))
    {
        ret = DB_E_NOTPERMITTED;
        goto end;
    }

    if (!isTempTable(cont))
    {       /* normal table */
        trans->fReadOnly = 0;

        if (!isNoLogging(cont))
            trans->fLogging = 1;

        if (server->fs_full)
        {
            ret = DB_E_DISKFULL;
            goto end;
        }
    }

    ret = Direct_Update_Internal(trans, cont, indexName, findKey,
            rid_to_update, record, record_leng, lock, newValues, 0);

  end:
    if (curr_transid == -1)
    {
        if (ret < 0)
            dbi_Trans_Abort(connid);
        else
            dbi_Trans_Commit(connid);
    }

    UnsetBufSegment(issys_cont);

  end_return:

    return ret;
}

/*****************************************************************************/
//! dbi_Direct_Update_Field
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid    :    connection identifier
 * \param tableName    :    table name
 * \param indexName    :    index name
 * \param findKey    :    key
 * \param newValue    :    newValues
 * \param lock        :    lock mode
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Direct_Update_Field(int connid, char *tableName,
        char *indexName, struct KeyValue *findKey,
        struct UpdateValue *newValues, LOCKMODE lock)
{
    int userid = _USER_USER;

#if defined(MDB_DEBUG)
    userid = check_userid(connid);
#endif
    return dbi_Direct_Update_Field_byUser(connid, tableName,
            indexName, findKey, newValues, lock, userid);
}

int
dbi_Direct_Update_Field_byUser(int connid, char *tableName,
        char *indexName, struct KeyValue *findKey,
        struct UpdateValue *newValues, LOCKMODE lock, int userid)
{
    int *pTransid;
    int curr_transid = 0;
    struct Trans *trans;
    struct Container *cont;
    int ret = DB_SUCCESS;
    int issys_cont = 0;

    DBI_CHECK_SERVER_TERM();
    DBI_CHECK_TABLE_NAME(tableName);
    DBI_CHECK_INDEX_NAME(indexName);

    pTransid = (int *) CS_getspecific(my_trans_id);
    curr_transid = *pTransid;
    if (curr_transid == -1)
    {
        ret = dbi_Trans_Begin(connid);
        if (ret < 0)
            goto end_return;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }

    if (lock == LK_SCHEMA_SHARED || lock == LK_SHARED)
    {
        ret = DB_E_INVALIDLOCKMODE;
        goto end;
    }

    if (lock != LK_NOLOCK)
    {
        ret = LockMgr_Lock(tableName, *pTransid, LOWPRIO, lock, WAIT);
        if (ret < 0)
        {
            goto end;
        }
        lock = ret;     /* hold lock mode */
    }

    cont = Cont_Search(tableName);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }

    SetBufSegment(issys_cont, cont->collect_.cont_id_);

    if (isSysTable(cont) ||
            (userid == _USER_USER && !doesUserCreateTable(cont, userid)))
    {
        ret = DB_E_NOTPERMITTED;
        goto end;
    }

    if (!isTempTable(cont))
    {       /* normal table */
        trans->fReadOnly = 0;

        if (!isNoLogging(cont))
            trans->fLogging = 1;

        if (server->fs_full)
        {
            ret = DB_E_DISKFULL;
            goto end;
        }
    }

    ret = Direct_Update_Field_Internal(trans, cont, indexName, findKey,
            newValues, lock);

  end:
    if (curr_transid == -1)
    {
        if (ret < 0)
            dbi_Trans_Abort(connid);
        else
            dbi_Trans_Commit(connid);
    }

    UnsetBufSegment(issys_cont);

  end_return:

    return ret;
}
