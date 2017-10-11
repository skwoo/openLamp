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
dbi_Trans_ID(int connid)
{
    int *pTransid;

    int ret;

    pTransid = (int *) CS_getspecific(my_trans_id);

    if (pTransid == NULL || *pTransid == -1)
        ret = DB_E_NOTTRANSBEGIN;
    else
        ret = *pTransid;

    return ret;
}

/*****************************************************************************/
//! dbi_Trans_Begin
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid    :    connection identifier
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Trans_Begin(int connid)
{
    int *pTransid;
    int transId;
    int ret = DB_SUCCESS;
    int _g_connid_bk = -2;
    int pushed_here = 0;

    DBI_CHECK_SERVER_TERM();

    if (_g_connid == -1)
    {
        PUSH_G_CONNID(connid);
        pushed_here = 1;
    }

    pTransid = (int *) CS_getspecific(my_trans_id);

    if (pTransid == NULL)
    {
        ret = DB_E_INVALIDPARAM;
        goto end;
    }

    if (*pTransid != -1)
    {
        ret = DB_E_NOTCOMMITTED;
        goto end;
    }

    if (connid == -1)
        transId = TransMgr_InsTrans(0, -1, -1);
    else
        transId = TransMgr_InsTrans(-1, CS_Get_ClientType(connid), connid);

    if (transId > 0 || (connid == -1 && transId == 0))
    {
        *pTransid = transId;

#ifdef MDB_DEBUG
        if (_Schema_Trans_TempObjectExist(transId))
        {
            __SYSLOG("Some temp tables or temp indexes exist \
                      at start of a transaction\n");
            Cont_RemoveTransTempObjects(connid, transId);
        }
#endif

        ret = transId;
    }
    else
    {
        MDB_SYSLOG(("ERROR: init of new transaction %d\n", transId));
        ret = transId;
    }

  end:
    if (pushed_here == 1)
    {
        POP_G_CONNID();
    }
    return ret;
}

/*****************************************************************************/
//! dbi_trans_commit_internal
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid        :    connection identifier
 * \param commit_lsn    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
extern void Delete_Useless_Logfiles(void);
static int
dbi_trans_commit_internal(int connid, struct LSN *commit_lsn)
{
    int *pTransid;
    struct Trans *trans = NULL;
    int f_ddl, fReadOnly, First_LSN_File_No_;

    int fLogging;
    struct TransCommitLog trans_commit_log;
    int ret;

    int do_checkpointing = 0;

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (pTransid == NULL || *pTransid == -1)
    {
        return DB_SUCCESS;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }

    f_ddl = trans->fddl;
    fReadOnly = trans->fReadOnly;
    fLogging = trans->fLogging;
    First_LSN_File_No_ = trans->First_LSN_.File_No_;

    trans->state = TRANS_STATE_COMMIT;

    Cont_RemoveTransTempObjects(connid, *pTransid);

    /* open된 cursor를 닫는다 */
    if (trans->cursor_list)
        Cursor_Scan_Dangling(*pTransid);

    if (!trans->fReadOnly && trans->First_LSN_.File_No_ != -1)
    {
        LogRec_Init(&(trans_commit_log.logrec));

        trans_commit_log.logrec.header_.type_ = TRANS_COMMIT_LOG;
        trans_commit_log.logrec.header_.trans_id_ = *pTransid;
        LogMgr_WriteLogRecord((struct LogRec *) &trans_commit_log);
    }
    else
    {
        trans_commit_log.logrec.header_.record_lsn_.File_No_ = -1;
        trans_commit_log.logrec.header_.record_lsn_.Offset_ = -1;
    }

    if (LockMgr_Unlock(*pTransid) < 0)
    {
        MDB_SYSLOG(("unlock failed for transaction %d to be committed\n",
                        *pTransid));
    }

    if (fLogging && !fReadOnly && First_LSN_File_No_ != -1)
        commit_count++;

    if (f_ddl || (commit_count > CHKPNT_COMMIT_COUNT
                    && Log_Mgr->File_.File_No_ >
                    Log_Mgr->Anchor_.End_Chkpt_lsn_.File_No_))
    {
        do_checkpointing = 1;
    }
    else if (Log_Mgr->File_.File_No_ >=
            Log_Mgr->Anchor_.Last_Deleted_Log_File_No_ + LOGFILENUM_TO_CHKPT)
    {
        do_checkpointing = 1;
    }

    if (!trans->fReadOnly && trans->First_LSN_.File_No_ != -1)
    {
        /* commit시에 buffer flush 시킴
           backup db를 하나만 사용하기에... */
        /* memory commit이 아닌 상태(disk commit)에서 fLogging이 on 되어야
         * log flush를 하는 것이므로 &&로 변경 */
        if (fLogging && server->shparams.memory_commit == 0
                && do_checkpointing == 0)
        {
            LogBuffer_Flush(LOGFILE_SYNC_FORCE);
        }
    }

    if (commit_lsn != NULL)
    {
        *commit_lsn = trans_commit_log.logrec.header_.record_lsn_;
    }

    TransMgr_DelTrans(*pTransid);

    *pTransid = -1;

    if (do_checkpointing)
    {
        if (do_checkpointing == 2)
        {
            Delete_Useless_Logfiles();
        }
        else
        {
            _Checkpointing(0, 0);       /* 0: not slow checkpoint */
        }
        commit_count = 0;
        logfile_num = Log_Mgr->File_.File_No_;
    }
    else
    {
        /* checkpoint를 하지 않을 때는 dirty page를 내린다. */
        if (server && server->shparams.num_flush_page_commit)
            MemMgr_FlushPage(server->shparams.num_flush_page_commit);
    }

    if (logfile_num == 0)
        logfile_num = Log_Mgr->File_.File_No_;

    if (Mem_mgr)
    {
#ifdef MDB_DEBUG
        int i;

        if (Mem_mgr->bufSegment[0] != 0x80)
            MDB_SYSLOG(("bufSegment not cleared 0, 0x%hhx\n",
                            Mem_mgr->bufSegment[0]));
        for (i = 1; i < BIT_SEGMENT_NO; i++)
        {
            if (Mem_mgr->bufSegment[i] != 0)
            {
                MDB_SYSLOG(("bufSegment not cleared %d, 0x%hhx\n",
                                i, Mem_mgr->bufSegment[i]));
            }
        }
#endif
    }
    ret = DB_SUCCESS;

  end:
    return ret;
}

/*****************************************************************************/
//! dbi_Trans_Commit
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid    :    connection identifier
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Trans_Commit(int connid)
{
    return dbi_trans_commit_internal(connid, NULL);
}

/*****************************************************************************/
//! dbi_Trans_Abort
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid        :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Trans_Abort(int connid)
{
    int *pTransid;
    struct Trans *trans = NULL;
    int ret;
    int /*f_ddl, */ fReadOnly, First_LSN_File_No_;

    int fLogging;

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (pTransid == NULL || *pTransid == -1)
    {
        ret = DB_SUCCESS;       /* DB_E_NOTTRANSBEGIN; */
        goto end;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }

    fReadOnly = trans->fReadOnly;
    fLogging = trans->fLogging;
    First_LSN_File_No_ = trans->First_LSN_.File_No_;

    /* 이미 abort 진행 중이면 return */
    if (trans->aborted_ != TRUE)
    {
        trans->aborted_ = TRUE;

        trans->state = TRANS_STATE_ABORT;

        Cont_RemoveTransTempObjects(connid, *pTransid);

        TransMgr_trans_abort(*pTransid);
    }

    *pTransid = -1;

    if (fLogging && !fReadOnly && First_LSN_File_No_ != -1)
        commit_count++;

    /* fix_logfile_num */
    if (Log_Mgr->File_.File_No_ >=
            Log_Mgr->Anchor_.End_Chkpt_lsn_.File_No_ + LOGFILENUM_TO_CHKPT)
    {
        _Checkpointing(0, 0);   /* 0: not slow checkpoint */
        commit_count = 0;
        logfile_num = Log_Mgr->File_.File_No_;
    }
    if (logfile_num == 0)
        logfile_num = Log_Mgr->File_.File_No_;

    if ((Log_Mgr->File_.File_No_ >=
                    Log_Mgr->Anchor_.End_Chkpt_lsn_.File_No_ +
                    LOGFILENUM_TO_CHKPT)
            || (Log_Mgr->Anchor_.Current_Log_File_No_ >=
                    Log_Mgr->Anchor_.Last_Deleted_Log_File_No_ +
                    LOGFILENUM_TO_CHKPT))
    {
        _Checkpointing(0, 0);   /* 0: not slow checkpoint */
    }

    ret = DB_SUCCESS;

  end:
    return ret;
}

int
dbi_Log_Buffer_Flush(int connid)
{
    int ret;

    DBI_CHECK_SERVER_TERM();

    if (server->shparams.memory_commit)
    {
        LogMgr_buffer_flush(LOGFILE_SYNC_FORCE);
    }
    ret = DB_SUCCESS;

    return ret;
}

int
dbi_FlushBuffer(void)
{
    int ret;

    DBI_CHECK_SERVER_TERM();

    MemMgr_DeallocateAllSegments(1);

    ret = DB_SUCCESS;

    return ret;
}

/*****************************************************************************/
//! dbi_Trans_Implicit_Savepoint
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid        :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Trans_Implicit_Savepoint(int connid)
{
    int *pTransid;
    int ret;

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    ret = TransMgr_Implicit_Savepoint(*pTransid);
    if (ret < 0)
        goto end;

    ret = DB_SUCCESS;

  end:
    return ret;
}

/*****************************************************************************/
//! dbi_Trans_Implicit_Savepoint_Release
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid        :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Trans_Implicit_Savepoint_Release(int connid)
{
    int ret;
    int *pTransid;

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    ret = TransMgr_Implicit_Savepoint_Release(*pTransid);

    if (ret >= 0)
        ret = DB_SUCCESS;

  end:
    return ret;
}

/*****************************************************************************/
//! dbi_Trans_Implicit_Partial_Rollback
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid        :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Trans_Implicit_Partial_Rollback(int connid)
{
    int *pTransid;
    int ret;

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    ret = TransMgr_Implicit_Partial_Rollback(*pTransid);

    if (ret >= 0)
        ret = DB_SUCCESS;

  end:
    return ret;
}

int
dbi_Trans_Set_Query_Timeout(int connid, MDB_UINT32 query_timeout)
{
    return DB_SUCCESS;
}

/*****************************************************************************/
//! dbi_Trans_NextTempName
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param table_flag:    which is temp table name or temp index name
 * \param temp_name    :    temp table name or temp index name
 * \param suffix    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Trans_NextTempName(int connid, int table_flag, char *temp_name,
        char *suffix)
{
    int trans_idx;
    int temp_serial_num;
    int ret;

#ifdef CHECK_DBI_PARAM
    if (temp_name == NULL)
        return DB_E_INVALIDPARAM;
#endif

    ret = TransMgr_NextTempSerialNum(&trans_idx, &temp_serial_num);

    if (ret < 0)
        return ret;

    sc_snprintf(temp_name, table_flag ? REL_NAME_LENG : INDEX_NAME_LENG,
            "%s_%d_%d%s",
            table_flag ? INTERNAL_TABLE_PREFIX : INTERNAL_INDEX_PREFIX,
            trans_idx, temp_serial_num, suffix ? suffix : "_");

    return temp_serial_num;
}

#include "mdb_dbi_cursor.c"

/*****************************************************************************/
//! dbi_Direct_Read
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid    :    connection identifier
 * \param tableName    :    table name
 * \param indexName    :    index name
 * \param findKey    :    key
 * \param lock        :    lock mode
 * \param record    :    record buffer
 * \param size        :    record size
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Direct_Read(int connid, char *tableName,
        char *indexName, struct KeyValue *findKey,
        OID rid_to_read, LOCKMODE lock,
        char *record, DB_UINT32 * size, FIELDVALUES_T * rec_values)
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
    if (rid_to_read != NULL_OID && (indexName || findKey))
        sc_assert(0, __FILE__, __LINE__);
#endif

    if (rid_to_read == NULL_OID)
    {
        if (indexName == NULL || findKey == NULL || findKey->field_count_ == 0)
            return DB_E_INVALIDPARAM;
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

    cont = Cont_Search(tableName);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }

    SetBufSegment(issys_cont, cont->collect_.cont_id_);

    if (lock != LK_NOLOCK && lock != LK_SHARED)
    {
        ret = DB_E_INVALIDLOCKMODE;
        goto end;
    }

    /* locking */
    if (lock != LK_NOLOCK)
    {
        ret = LockMgr_Lock(tableName, *pTransid, LOWPRIO, lock, WAIT);
        if (ret < 0)
            goto end;

        lock = ret;     /* hold lock mode */
    }

    if (rid_to_read)
    {
        if (Col_check_valid_rid(NULL, rid_to_read) < 0)
        {
            ret = DB_E_INVALID_RID;
            goto end;
        }
    }

    ret = Direct_Read_Internal(trans, cont, indexName, findKey,
            rid_to_read, lock, record, size, rec_values);

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
