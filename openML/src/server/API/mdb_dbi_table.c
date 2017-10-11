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
//! dbi_Relation_Create
/*! \breif  TABLE 생성
 ************************************
 * \param connid(in)            :
 * \param relation_name_p(in)    :
 * \param numFields(in)            :
 * \param pFieldDesc(in)        :
 * \param type(in)                :
 * \param userid(in)            :
 * \param max_records(in)        :
 * \param column_name(in)        :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - BYTE/VARBYTE 지원
 *  - DB_E_INVALID_VARIABLE_TYPE 보다 큰 경우 에러 처리
 *  - COLLATION 지원
 *****************************************************************************/
static int __dbi_Relation_Create(int connid, char *relation_name_p,
        int numFields, FIELDDESC_T * pFieldDesc,
        ContainerType type, int userid, int max_records, char *column_name);

int
dbi_Relation_Create(int connid, char *relation_name_p,
        int numFields, FIELDDESC_T * pFieldDesc,
        ContainerType type, int userid, int max_records, char *column_name)
{
    return __dbi_Relation_Create(connid, relation_name_p,
            numFields, pFieldDesc, type, userid, max_records, column_name);
}

static int
__dbi_Relation_Create(int connid, char *relation_name_p,
        int numFields, FIELDDESC_T * pFieldDesc,
        ContainerType type, int userid, int max_records, char *column_name)
{
    int record_size = 0;
    int heaprecord_size = 0;
    int tableId;
    int ret = -1;
    int *pTransid;
    int curr_transid = 0;
    struct Container *cont;
    SYSTABLE_T sysTable;
    struct Trans *trans;
    DB_BOOL has_variabletype = FALSE;

    int lock_mode = LK_NOLOCK;
    DB_INT32 i;
    char *relation_name;
    char idx4fixedtable[INDEX_NAME_LENG];

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (relation_name_p == NULL)
        return DB_E_INVALIDPARAM;
#endif

#ifdef CHECK_DBI_PARAM
    /* parameter checking */
    ret = sc_strlen(relation_name_p);

    /* normal table create일 때 check */
    if (ret == 0 || ret >= REL_NAME_LENG)
        return DB_E_INVALIDPARAM;

    if (numFields <= 0)
        return DB_E_INVALIDPARAM;

    if (pFieldDesc == NULL)
        return DB_E_INVALIDPARAM;
#endif

    if (max_records < 0)
        return DB_E_INVALIDPARAM;

    relation_name = relation_name_p;

    pTransid = (int *) CS_getspecific(my_trans_id);
    curr_transid = *pTransid;
    if (curr_transid == -1)
    {
        ret = dbi_Trans_Begin(connid);
        if (ret < 0)
        {
            goto end_return;
        }
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end_trans;
    }


    lock_mode = LockMgr_Lock("systables", *pTransid, LOWPRIO, LK_EXCLUSIVE,
            WAIT);

    if (lock_mode < 0 && lock_mode != DB_E_UNKNOWNTABLE)
    {
        goto end;
    }

    /* 같은 이름의 테이블이 있고, temp table이라면 제거 */
    cont = Cont_Search(relation_name);
    if (isTempTable(cont))
    {
        dbi_Relation_Drop(connid, relation_name);
    }

    /* 내부에 테이블이 있지만 systables에 없는 테이블이면 제거
       consistency가 깨어진 경우..  */
    if (cont != NULL && _Schema_GetTableInfo(relation_name, &sysTable) < 0)
        Drop_Table(relation_name);

    record_size = _Schema_CheckFieldDesc(numFields, pFieldDesc, FALSE);
    heaprecord_size = _Schema_CheckFieldDesc(numFields, pFieldDesc, TRUE);

    if (heaprecord_size < 0)
    {
        ret = heaprecord_size;
        goto end;
    }

    for (i = 0; i < numFields; i++)
    {
        if (IS_VARIABLE_DATA(pFieldDesc[i].type_, pFieldDesc[i].fixed_part))
        {
            has_variabletype = TRUE;

            if (pFieldDesc[i].fixed_part > MAX_FIXED_SIZE_FOR_VARIABLE)
            {
                ret = DB_E_INVALID_VARIABLE_TYPE;
                goto end;
            }
        }

        if (pFieldDesc[i].flag & NOLOGGING_BIT)
        {
            if (IS_VARIABLE(pFieldDesc[i].type_))
            {
                ret = SQL_E_INVALID_NOLOGGING_COLUMN;
                goto end;
            }
            else if (pFieldDesc[i].flag & PRI_KEY_BIT)
            {
                ret = SQL_E_NOLOGGING_COLUMN_INDEX;
                goto end;
            }
        }
    }

    if (isTempTable_flag(type))
    {
        userid = connid + _CONNID_BASE; /* temp table 이면 connid를 넣고
                                           disconnect 시점에 제거 */
        tableId = Cont_NewTempTableID(userid);
    }
    else
    {
        if (userid == _USER_SYS)
        {
            tableId = check_systable(relation_name);
            if (tableId < 0)
            {
                ret = tableId;
                goto end;
            }
        }
        else
            tableId = Cont_NewTableID(userid);
        trans->fLogging = 1;
    }

    trans->fReadOnly = 0;

    ret = Create_Table(relation_name, record_size, heaprecord_size, numFields, type, tableId, userid, max_records       /* , column_name */
            , pFieldDesc);
    if (ret < 0)
        goto end;

    /* register table & fields information */
    ret = _Schema_InputTable(*pTransid, relation_name, tableId, type,
            has_variabletype, record_size, heaprecord_size,
            numFields, pFieldDesc, max_records, column_name);
    if (ret < 0)
    {
        Drop_Table(relation_name);
        MDB_SYSLOG(("table & fields registeration FAIL %s %d %d\n",
                        relation_name, tableId, ret));
        goto end;
    }

    ret = DB_SUCCESS;

    if (!isTempTable_flag(type))
    {
        LogMgr_buffer_flush(LOGFILE_SYNC_FORCE);
    }

    if (max_records > 0 && column_name[0] != '#')       /* must be a fixed table */
    {
        FIELDDESC_T fieldDesc;

        dbi_FieldDesc(column_name, (DataType) 0, 0, 0, 0, 0, -1, &fieldDesc,
                MDB_COL_NONE);

        sc_snprintf(idx4fixedtable, INDEX_NAME_LENG, "%s%s",
                MAXREC_KEY_PREFIX, relation_name);

        ret = dbi_Index_Create(connid, relation_name,
                idx4fixedtable, 1, &fieldDesc, 0);
        if (ret < 0)
        {
            _Schema_RemoveTable(*pTransid, relation_name);
            Drop_Table(relation_name);

            goto end;
        }
    }

  end:
    if (*pTransid != -1 && lock_mode != LK_NOLOCK)
    {
        struct Container *Cont = Cont_Search("systables");

        if (Cont != NULL)
        {
            LockMgr_Table_Unlock(*pTransid, Cont->base_cont_.id_);
        }
    }

  end_trans:

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
//! dbi_Relation_Rename
/*! \breif  Rename old relation name to new relation name.
 ************************************
 * \param connid : connection identifier.
 * \param old_name : old name of the relation.
 * \param new_name : new name for the relation.
 ************************************
 * \return DB_SUCCESS or FAIL
 ************************************
 * \note  None.
 *****************************************************************************/
int
dbi_Relation_Rename(int connid, char *old_name, char *new_name_p)
{
    int userid = _USER_USER;

#if defined(MDB_DEBUG)
    userid = check_userid(connid);
#endif
    return dbi_Relation_Rename_byUser(connid, old_name, new_name_p, userid);
}

int
dbi_Relation_Rename_byUser(int connid, char *old_name, char *new_name_p,
        int userid)
{
    int *pTransid;
    int curr_transid = 0;
    SYSTABLE_T sysTable;
    struct Trans *trans;
    struct Container *old_cont = NULL;
    int issys_cont = 0;

    int ret = -1;
    char *new_name;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (old_name == NULL)
        return DB_E_INVALIDPARAM;
#endif

#ifdef CHECK_DBI_PARAM
    /* parameter checking */
    ret = sc_strlen(new_name_p);

    /* normal table create일 때 check */
    if (ret == 0 || ret >= REL_NAME_LENG)
    {
        return DB_E_INVALIDPARAM;
    }
#endif

    new_name = new_name_p;

    old_cont = Cont_Search(old_name);

    // NULL 처리 추가
    if (!old_cont)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end_return;
    }

    SetBufSegment(issys_cont, old_cont->collect_.cont_id_);

    if (isSysTable(old_cont) || isSystemTable(old_cont))
    {
        ret = DB_E_NOTPERMITTED;
        goto end_return;
    }

    if (userid == _USER_USER && !doesUserCreateTable(old_cont, userid))
    {
        ret = DB_E_NOTPERMITTED;
        goto end_return;
    }

    if (Cont_Search(new_name) != NULL)
    {
        /* 내부에 테이블이 있지만 systables에 없는 테이블이면 제거
           consistency가 깨어진 경우..  */
        if (_Schema_GetTableInfo(new_name, &sysTable) < 0)
            Drop_Table(new_name);
        else
        {
            ret = DB_E_DUPTABLENAME;
            goto end_return;
        }
    }

    pTransid = (int *) CS_getspecific(my_trans_id);
    curr_transid = *pTransid;
    if (curr_transid == -1)
    {
        ret = dbi_Trans_Begin(connid);
        if (ret < 0)
            goto end_return;
    }

    if ((trans = (struct Trans *) Trans_Search(*pTransid)) == NULL)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    ret = _Schema_GetTableInfo(old_name, &sysTable);
    if (ret < 0)
    {
        goto end;
    }

    trans->fReadOnly = 0;
    trans->fLogging = 1;

    ret = LockMgr_Lock(old_name, *pTransid, LOWPRIO,
            LK_SCHEMA_EXCLUSIVE, WAIT);

    if (ret < 0 && ret != DB_E_UNKNOWNTABLE)
        goto end;

    ret = Cont_Rename(old_cont, new_name);
    if (ret < 0)
        goto end;

    /* Rename relation name of table info */
    ret = _Schema_RenameTable(*pTransid, old_name, new_name);
    if (ret < 0)
        goto end;

    ret = DB_SUCCESS;

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

/*****************************************************************************/
//! dbi_Relation_Drop
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid        :
 * \param relation_name    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Relation_Drop(int connid, char *relation_name)
{
    int userid = _USER_USER;

#if defined(MDB_DEBUG)
    userid = check_userid(connid);
#endif
    return dbi_Relation_Drop_byUser(connid, relation_name, userid);
}

int
dbi_Relation_Drop_byUser(int connid, char *relation_name, int userid)
{
    int ret = -1;
    int *pTransid;
    int curr_transid = 0;
    struct Container *cont;
    struct Trans *trans;
    int lock_mode;

    SYSTABLE_T sysTable;
    int issys_cont = 0;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    ret = sc_strlen(relation_name);
    if (ret == 0 || ret >= REL_NAME_LENG)
        return DB_E_INVALIDPARAM;
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (pTransid == NULL || *pTransid == -1)
    {
        if (pTransid)
            curr_transid = *pTransid;
        ret = dbi_Trans_Begin(connid);
        if (ret < 0)
            goto end_return;
    }
    else
    {
        curr_transid = *pTransid;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end_trans;
    }

    /* 0은 엔진 내부 처리용. 초기화.
       temp table 제거시에 사용 */
    lock_mode = LockMgr_Lock("systables", *pTransid, LOWPRIO,
            LK_EXCLUSIVE, WAIT);
    if (lock_mode < 0 && lock_mode != DB_E_UNKNOWNTABLE)
    {
        ret = lock_mode;
        goto end_trans;
    }

    cont = Cont_Search(relation_name);
    if (cont == NULL)
    {
        /* The given relation(normally, it is temp table)
           has already been dropped. */
        ret = DB_SUCCESS;
        goto end;
    }

    SetBufSegment(issys_cont, cont->collect_.cont_id_);

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (pTransid == NULL || *pTransid == -1)
    {
        if (pTransid)
            curr_transid = *pTransid;
        ret = dbi_Trans_Begin(connid);
        if (ret < 0)
            goto end_return;
    }
    else
        curr_transid = *pTransid;

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    if (cont != NULL)
    {
        if (isSysTable(cont) || isSystemTable(cont))
        {
            ret = DB_E_NOTPERMITTED;
            goto end;
        }

        if (userid == _USER_USER && !doesUserCreateTable(cont, userid))
        {
            ret = DB_E_NOTPERMITTED;
            goto end;
        }

        ret = _Schema_GetTableInfo(relation_name, &sysTable);
        if (ret < 0)
        {
            goto end;
        }

        /* 0은 엔진 내부 처리용. 초기화.  temp table 제거시에 사용 */
        ret = LockMgr_Lock(relation_name, *pTransid, LOWPRIO,
                LK_SCHEMA_EXCLUSIVE, WAIT);
        if (ret < 0 && ret != DB_E_UNKNOWNTABLE)
        {
            goto end;
        }

        trans->fReadOnly = 0;
        if (!isTempTable(cont))
        {
            trans->fddl = 1;    /* set for normal objects */
            trans->fLogging = 1;
        }

        /* remove all schema information of table */
        ret = _Schema_RemoveTable(*pTransid, relation_name);
        if (ret < 0 && ret != DB_E_UNKNOWNTABLE)
        {
            goto end;
        }

        if (isTempTable(cont))
        {
            if (trans->state == TRANS_STATE_ABORT)
            {
                extern int Drop_Table_Temp(char *Cont_name);

                if (Drop_Table_Temp(relation_name) < 0)
                    goto end;
            }
            else
            {
                if (Drop_Table(relation_name) < 0)
                    goto end;
            }
        }
        else
        {
            if (Drop_Table(relation_name) < 0)
                goto end;
        }
    }

    ret = DB_SUCCESS;

  end:
    if (*pTransid != -1 && lock_mode != LK_NOLOCK)
    {
        struct Container *Cont = Cont_Search("systables");

        if (Cont != NULL)
        {
            LockMgr_Table_Unlock(*pTransid, Cont->base_cont_.id_);
        }
    }

  end_trans:
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

/*****************************************************************************/
//! dbi_Relation_Truncate
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid        :
 * \param relation_name    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Relation_Truncate(int connid, char *relation_name)
{
    int userid = _USER_USER;

#if defined(MDB_DEBUG)
    userid = check_userid(connid);
#endif
    return dbi_Relation_Truncate_byUser(connid, relation_name, userid);
}

int
dbi_Relation_Truncate_byUser(int connid, char *relation_name, int userid)
{
    int ret = -1;
    int *pTransid;
    int curr_transid = 0;
    struct Container *cont;
    struct Trans *trans;
    int issys_cont = 0;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    ret = sc_strlen(relation_name);
    if (ret == 0 || ret >= REL_NAME_LENG)
        return DB_E_INVALIDPARAM;
#endif

    cont = Cont_Search(relation_name);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end_return;
    }

    SetBufSegment(issys_cont, cont->collect_.cont_id_);

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
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    if (cont != NULL)
    {
        if (isSysTable(cont) || isSystemTable(cont))
        {
            ret = DB_E_NOTPERMITTED;
            goto end;
        }

        if (userid == _USER_USER && !doesUserCreateTable(cont, userid))
        {
            ret = DB_E_NOTPERMITTED;
            goto end;
        }

        ret = LockMgr_Lock(relation_name, *pTransid, LOWPRIO,
                LK_SCHEMA_EXCLUSIVE, WAIT);
        if (ret < 0)
            goto end;

        trans->fReadOnly = 0;
        if (!isTempTable(cont))
        {
            trans->fddl = 1;    /* set for normal objects */
            trans->fLogging = 1;
        }

        Truncate_Table(relation_name);

        if (cont->base_cont_.max_auto_value_)
        {
            cont->base_cont_.max_auto_value_ = 0;
            SetDirtyFlag(cont->collect_.cont_id_);
            OID_SaveAsAfter(AUTOINCREMENT, cont,
                    cont->collect_.cont_id_ +
                    (unsigned long) &cont->base_cont_.max_auto_value_ -
                    (unsigned long) cont,
                    sizeof(cont->base_cont_.max_auto_value_));
        }
    }

    ret = DB_SUCCESS;

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
