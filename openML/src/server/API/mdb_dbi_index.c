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
//! dbi_Index_Create
/*! \breif  index를 생성하는 경우 사용하는 함수
 ************************************
 * \param connid(in)        :
 * \param relationName(in)  :
 * \param indexName_p(in)   :
 * \param numFields(in)     : index's fileds 갯수
 * \param pFieldDesc(in)    : index's fields description
 * \param flag(in)          : 0x80인 경우, unique index임/0x00인 경우 not unique index
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - caution
 *      INDEX를 생성하기 위해 필요한 FieldDesc의 정보는 다음과 같다
 *      1. Field's Name
 *      2. Field's order
 *      3. Field's collation
 *      이 정보들을 윗단에서 설정해서 내려보내준다
 *  - 의문점..
 *      필드 이름 대신에 Field's ID만 설정해서 내려보내주면 되지 않나(?)
 *      (시간상 이점)
 *      아니면 윗단에서 정보를 내려보내는 것이 맞을듯 한데..
 *****************************************************************************/
int
dbi_Index_Create(int connid, char *relationName, char *indexName_p,
        int numFields, FIELDDESC_T * pFieldDesc, DB_UINT8 flag)
{
    struct KeyDesc *key_desc = NULL;
    int indexNo;
    short indexType;
    int ret = -1;
    int *pTransid;
    int curr_transid = 0;
    struct Trans *trans;
    struct Container *cont;
    char *indexName;
    int issys_cont = 0;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (numFields == 0 || pFieldDesc == NULL)
        return DB_E_INVALIDPARAM;

    ret = sc_strlen(relationName);
    if (ret == 0 || ret >= REL_NAME_LENG)
        return DB_E_INVALIDPARAM;

    ret = sc_strlen(indexName_p);
    if (ret == 0 || ret >= INDEX_NAME_LENG)
        return DB_E_INVALIDPARAM;
#endif

    if (numFields > MAX_KEY_FIELD_NO)
    {
        return DB_E_NUMIDXFIEDLS;
    }

    indexName = indexName_p;

    cont = Cont_Search(relationName);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end_return;
    }

    SetBufSegment(issys_cont, cont->collect_.cont_id_);

    /* Block creating normal index on system tables. */
    if (!isTempIndex_name(indexName))
    {
        if (connid != -1 && (isSysTable(cont) || isSystemTable(cont)))
        {
            ret = DB_E_NOTPERMITTED;
            goto end_return;
        }
    }

    pTransid = (int *) CS_getspecific(my_trans_id);
    curr_transid = *pTransid;
    if (curr_transid == -1)
    {
        if (isTempIndex_name(indexName))
        {
            MDB_SYSLOG(("WARNING: trying create temp index(%s) \
                      without transaction\n", indexName));
        }
        ret = dbi_Trans_Begin(connid);
        if (ret < 0)
            goto end_return;
    }

    key_desc = (struct KeyDesc *) PMEM_ALLOC(sizeof(struct KeyDesc));
    if (key_desc == NULL)
    {
        ret = DB_E_OUTOFMEMORY;
        goto end;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    sc_memset(key_desc, 0, sizeof(struct KeyDesc));
    /* 엔진쪽에 알릴 key desc. 형태를 만듬 */
    ret = _Schema_MakeKeyDesc(*pTransid, relationName,
            numFields, pFieldDesc, key_desc);
    if (ret < 0)
    {
        goto end;
    }

    if (flag & 0x80)
        key_desc->is_unique_ = TRUE;
    else
        key_desc->is_unique_ = FALSE;

    trans->fReadOnly = 0;

    if (!isTempTable(cont) && !isTempIndex_name(indexName))
    {
        trans->fLogging = 1;
        ret = LockMgr_Lock(relationName, *pTransid, LOWPRIO,
                LK_EXCLUSIVE, WAIT);
        if (ret < 0)
            goto end;
    }

    indexNo = _Schema_InputIndex(*pTransid, relationName, indexName,
            numFields, pFieldDesc, flag);
    if (indexNo < 0)
    {
        ret = indexNo;
        goto end;
    }

    indexType = _CONT_INDEX_TTREE;
    if (isPrimaryIndex_name(indexName))
        indexType |= _CONT_PRIMARY_INDEX;

    ret = Create_Index(indexName, indexNo, (int) indexType,
            relationName, key_desc, 1, NULL);
    if (ret < 0)
    {
        goto end;
    }

    if (!isTempTable(cont) && !isTempIndex_name(indexName))
    {
        if (cont->collect_.item_count_ > 0)
            trans->fddl = 1;    /* set for normal objects */
    }

    ret = DB_SUCCESS;

  end:
    if (curr_transid == -1)
    {
        if (ret == DB_SUCCESS)
            dbi_Trans_Commit(connid);
        else
            dbi_Trans_Abort(connid);
    }

  end_return:
    UnsetBufSegment(issys_cont);
    PMEM_FREENUL(key_desc);

    return ret;
}

/*****************************************************************************/
//! dbi_Index_Create_With_KeyInsCond
/*! \breif  index를 생성하는 경우 사용하는 함수
 ************************************
 * \param connid(in)        : connection id
 * \param relationName(in)  : table name
 * \param indexName_p(in)   : index 이름
 * \param numFields(in)     : index의 fields 갯수
 * \param pFieldDesc(in)    : index의 fields 정보
 * \param flag(in)          : 0x80 : unique index임/0x00 : not unique index
 * \param keyins_flag(in)   :
 * \param pScanDesc(in)     :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      Same with dbi_Index_Create() excluding
 *      keyins_flag and pScanDesc are transferred when Create_index() is called.
 *
 *****************************************************************************/
int
dbi_Index_Create_With_KeyInsCond(int connid, char *relationName,
        char *indexName_p, int numFields,
        FIELDDESC_T * pFieldDesc, DB_UINT8 flag,
        int keyins_flag, SCANDESC_T * pScanDesc)
{
    struct KeyDesc *key_desc = NULL;
    int indexNo;
    short indexType;
    int ret = -1;
    int *pTransid;
    int curr_transid = 0;
    struct Trans *trans;
    struct Container *cont;
    char *indexName;
    int issys_cont = 0;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (numFields == 0 || pFieldDesc == NULL)
        return DB_E_INVALIDPARAM;
#endif

    if (numFields > MAX_KEY_FIELD_NO)
    {
        return DB_E_NUMIDXFIEDLS;
    }

#ifdef CHECK_DBI_PARAM
    ret = sc_strlen(relationName);
    if (ret == 0 || ret >= REL_NAME_LENG)
        return DB_E_INVALIDPARAM;
#endif

#ifdef CHECK_DBI_PARAM
    ret = sc_strlen(indexName_p);
    if (ret == 0 || ret >= INDEX_NAME_LENG)
        return DB_E_INVALIDPARAM;
#endif

    indexName = indexName_p;

    cont = Cont_Search(relationName);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end_return;
    }

    SetBufSegment(issys_cont, cont->collect_.cont_id_);

    if (isTempIndex_name(indexName))
    {       /* temp index */
        if ((keyins_flag == 0) || (keyins_flag && pScanDesc))
        {
            /* indexName must be temporary partial index name.
               that is, the prefix of indexName must be "#pi" */
            if (!isPartialIndex_name(indexName))
            {
                ret = DB_E_NOTPERMITTED;
                goto end_return;
            }
        }
    }
    else
    {       /* regular index */
        if ((keyins_flag == 0) || (keyins_flag && pScanDesc))
        {
            /* A regular index must be complete index. */
            /* Complete Index: have keys of all records of a table */
            /* Partial Index: have keys of some records of a table */
            ret = DB_E_NOTPERMITTED;
            goto end_return;
        }

        if (connid != -1 && (isSysTable(cont) || isSystemTable(cont)))
        {
            ret = DB_E_NOTPERMITTED;
            goto end_return;
        }
    }

    pTransid = (int *) CS_getspecific(my_trans_id);
    curr_transid = *pTransid;
    if (curr_transid == -1)
    {
        if (isTempIndex_name(indexName))
        {
            MDB_SYSLOG(("WARNING: trying create temp index(%s) \
                      without transaction\n", indexName));
        }
        ret = dbi_Trans_Begin(connid);
        if (ret < 0)
            goto end_return;
    }

    key_desc = (struct KeyDesc *) PMEM_ALLOC(sizeof(struct KeyDesc));
    if (key_desc == NULL)
    {
        ret = DB_E_OUTOFMEMORY;
        goto end;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    sc_memset(key_desc, 0, sizeof(struct KeyDesc));
    /* 엔진쪽에 알릴 key desc. 형태를 만듬 */
    ret = _Schema_MakeKeyDesc(*pTransid, relationName,
            numFields, pFieldDesc, key_desc);
    if (ret < 0)
    {
        goto end;
    }

    if (flag & 0x80)
        key_desc->is_unique_ = TRUE;
    else
        key_desc->is_unique_ = FALSE;

    trans->fReadOnly = 0;

    if (!isTempTable(cont) && !isTempIndex_name(indexName))
    {
        /* FIX_INDEX_CREATE_LOGGING */
        trans->fLogging = 1;

        ret = LockMgr_Lock(relationName, *pTransid, LOWPRIO,
                LK_EXCLUSIVE, WAIT);
        if (ret < 0)
            goto end;
    }

    indexNo = _Schema_InputIndex(*pTransid, relationName, indexName,
            numFields, pFieldDesc, flag);
    if (indexNo < 0)
    {
        ret = indexNo;
        goto end;
    }

    indexType = _CONT_INDEX_TTREE;
    if (isPrimaryIndex_name(indexName))
        indexType |= _CONT_PRIMARY_INDEX;

    ret = Create_Index(indexName, indexNo, (int) indexType,
            relationName, key_desc, keyins_flag, pScanDesc);
    if (ret < 0)
    {
        goto end;
    }

    if (!isTempTable(cont) && !isTempIndex_name(indexName))
    {
        if (cont->collect_.item_count_ > 0)
            trans->fddl = 1;    /* set for normal objects */
    }

    ret = DB_SUCCESS;

  end:
    if (curr_transid == -1)
    {
        if (ret == DB_SUCCESS)
            dbi_Trans_Commit(connid);
        else
            dbi_Trans_Abort(connid);
    }

  end_return:
    UnsetBufSegment(issys_cont);
    PMEM_FREENUL(key_desc);

    return ret;
}

/*****************************************************************************/
//! dbi_Index_Rename
/*! \breif  Rename old index name to new index name.
 ************************************
 * \param connid : connection identifier.
 * \param old_name : old name of the index.
 * \param new_name : new name for the index.
 ************************************
 * \return DB_SUCCESS or FAIL
 ************************************
 * \note  None.
 *****************************************************************************/
int
dbi_Index_Rename(int connid, char *old_name, char *new_name_p)
{
    int userid = _USER_USER;

#if defined(MDB_DEBUG)
    userid = check_userid(connid);
#endif
    return dbi_Index_Rename_byUser(connid, old_name, new_name_p, userid);
}

int
dbi_Index_Rename_byUser(int connid, char *old_name, char *new_name_p,
        int userid)
{
    int ret = -1;
    int *pTransid;
    int curr_transid = 0;
    struct Trans *trans;
    SYSINDEX_T sysIndex;
    char relation[REL_NAME_LENG] = "";
    char *new_name;

    DBI_CHECK_SERVER_TERM();

    if (isTempIndex_name(old_name))
    {
        return DB_E_INVALIDPARAM;
    }

#ifdef CHECK_DBI_PARAM
    ret = sc_strlen(new_name_p);
    if (ret == 0 || ret >= REL_NAME_LENG)
        return DB_E_INVALIDPARAM;
#endif

    new_name = new_name_p;

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

    ret = _Schema_GetIndexInfo(new_name, &sysIndex, NULL, NULL);
    if (ret == DB_SUCCESS)
    {
        ret = DB_E_DUPINDEXNAME;
        goto end;
    }

    ret = _Schema_GetIndexInfo(old_name, &sysIndex, NULL, relation);
    if (ret < 0)
    {
        goto end;
    }

    {
        struct Container *table_cont = Cont_Search_Tableid(sysIndex.tableId);

        if (table_cont == NULL ||
                (userid == _USER_USER
                        && !doesUserCreateTable(table_cont, userid)))

        {
            ret = DB_E_NOTPERMITTED;
            goto end;
        }
    }

    ret = LockMgr_Lock(relation, *pTransid, LOWPRIO, LK_EXCLUSIVE, WAIT);
    if (ret < 0)
        goto end;

    trans->fReadOnly = 0;
    trans->fLogging = 1;

    ret = Rename_Index(relation, sysIndex.indexId, new_name);
    if (ret < 0)
        goto end;

    ret = _Schema_RenameIndex(*pTransid, old_name, new_name);
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

    return ret;
}


/*****************************************************************************/
//! dbi_Index_Drop
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid    :
 * \param indexName    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Index_Drop(int connid, char *indexName)
{
    int userid = _USER_USER;

#if defined(MDB_DEBUG)
    userid = check_userid(connid);
#endif
    return dbi_Index_Drop_byUser(connid, indexName, userid);
}

int
dbi_Index_Drop_byUser(int connid, char *indexName, int userid)
{
    SYSINDEX_T sysIndex;
    int ret;
    int *pTransid;
    int curr_transid = 0;
    char relation_name[REL_NAME_LENG] = "";
    struct Trans *trans;
    struct Container *cont;
    int issys_cont = 0;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    ret = sc_strlen(indexName);
    if (ret == 0 || ret >= REL_NAME_LENG)
        return DB_E_INVALIDPARAM;
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);
    curr_transid = *pTransid;
    if (curr_transid == -1)
    {
        if (isTempIndex_name(indexName))
        {
            /* The given temp index had already been dropped
               at the end of the transaction that created the temp index.
               So, The given temp index does not exist.
             */
            ret = DB_SUCCESS;
            goto end_return;
        }
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

    ret = _Schema_GetIndexInfo(indexName, &sysIndex, NULL, relation_name);
    if (ret < 0)
    {
        MDB_SYSLOG(("ERROR: index drop: Can't get index(%s) info %d\n",
                        indexName, ret));
        goto end;
    }

    cont = Cont_Search(relation_name);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }

    SetBufSegment(issys_cont, cont->collect_.cont_id_);

    if (!isTempIndex_name(indexName) &&
            userid == _USER_USER && !doesUserCreateTable(cont, userid))
    {
        ret = DB_E_NOTPERMITTED;
        goto end;
    }

    trans->fReadOnly = 0;

    if (!isTempTable(cont) && !isTempIndex_name(indexName))
    {
        trans->fLogging = 1;
        ret = LockMgr_Lock(relation_name, *pTransid, LOWPRIO,
                LK_EXCLUSIVE, WAIT);
        if (ret < 0)
            goto end;

        trans->fddl = 1;        /* set for normal objects */
    }

    if (isTempIndex_name(indexName))
    {
        if (trans->state == TRANS_STATE_ABORT)
        {
            extern int Drop_Index_Temp(char *Cont_name, DB_INT16 index_no);

            ret = Drop_Index_Temp(relation_name, sysIndex.indexId);
            if (ret < 0)
                goto end;
        }
        else
        {
            ret = Drop_Index(relation_name, sysIndex.indexId);
            if (ret < 0)
                goto end;
        }
    }
    else
    {
        ret = Drop_Index(relation_name, sysIndex.indexId);
        if (ret < 0)
            goto end;
    }

    _Schema_RemoveIndex(*pTransid, indexName);

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

/* num: obj_name에 설정된 object 수,
 * obj_name: 테이블 이름 또는 인덱스 이름
 *           테이블 이름은 foo 와 같은 형태로 들어오고,
 *           인덱스 이름은 foo.idx1 과 같은 형태로 들어와야 한다.
 *           즉, '.' 으로 테이블이름인지 인덱스 이름인지 구별함 */
/* num == 0 && obj_name == NULL : REBUILD INDEX ALL */
int
dbi_Index_Rebuild(int connid, int num, char **obj_name)
{
    int *pTransid;
    int curr_transid = 0;
    int ret = DB_SUCCESS;
    int i;
    char tablename[REL_NAME_LENG];
    char indexname[INDEX_NAME_LENG];
    char *p;
    int len;
    OID cont_oid, index_oid;
    struct Container *cont = NULL;
    struct TTree *ttree;
    struct Trans *trans;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (num <= 0 || obj_name == NULL)
        if (!(num == 0 && obj_name == NULL))
            return DB_E_INVALIDPARAM;
#endif

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
        goto end_trans;
    }

    trans->fReadOnly = 0;
    trans->fLogging = 1;
    trans->fddl = 1;

    /* clean index_rebuild of all tables */
    cont_oid = Col_GetFirstDataID((struct Container *) container_catalog, 0);
    while (cont_oid != NULL_OID)
    {
        if (cont_oid == INVALID_OID)
        {
            ret = DB_E_OIDPOINTER;
            goto end_trans;
        }

        cont = (struct Container *) OID_Point(cont_oid);
        if (cont == NULL)
        {
            ret = DB_E_INVLAIDCONTOID;
            goto end_trans;
        }

        if (num == 0 && obj_name == NULL)
            cont->base_cont_.index_rebuild = 1;
        else
            cont->base_cont_.index_rebuild = 0;

        SetDirtyFlag(cont_oid);

        Col_GetNextDataID(cont_oid,
                container_catalog->collect_.slot_size_,
                container_catalog->collect_.page_link_.tail_, 0);

    }

    /* clean index_rebuild of all indexes */
    index_oid = Col_GetFirstDataID((struct Container *) index_catalog, 0);
    while (index_oid != NULL_OID)
    {
        if (index_oid == INVALID_OID)
        {
            ret = DB_E_OIDPOINTER;
            goto end_trans;
        }

        ttree = (struct TTree *) OID_Point(index_oid);
        if (ttree == NULL)
        {
            ret = DB_E_INVLAIDCONTOID;
            goto end_trans;
        }

        if (num == 0 && obj_name == NULL)
        {
            if (ttree->base_cont_.index_rebuild == 0)
            {
                SetDirtyFlag(index_oid);
                ttree->base_cont_.index_rebuild = 1;
            }
        }
        else
        {
            if (ttree->base_cont_.index_rebuild == 1)
            {
                SetDirtyFlag(index_oid);
                ttree->base_cont_.index_rebuild = 0;
            }
        }

        Col_GetNextDataID(index_oid,
                index_catalog->collect_.slot_size_,
                index_catalog->collect_.page_link_.tail_, 0);
    }

    /* check to-be-rebuilded indexes */
    for (i = 0; i < num; i++)
    {
        if (obj_name[i] == NULL || obj_name[i][0] == '\0')
        {
            ret = DB_E_INVALIDPARAM;
            goto end;
        }

        p = sc_strchr(obj_name[i], '.');
        if (p)
        {   /* table.index */
            len = (unsigned long) p - (unsigned long) obj_name[i];
            if (len >= REL_NAME_LENG)
            {
                ret = DB_E_INVALIDPARAM;
                goto end;
            }

            sc_memcpy(tablename, obj_name[i], len);
            tablename[len] = '\0';
            sc_strncpy(indexname, p + 1, INDEX_NAME_LENG - 1);
            indexname[INDEX_NAME_LENG - 1] = '\0';
        }
        else
        {   /* table */
            sc_strncpy(tablename, obj_name[i], REL_NAME_LENG - 1);
            tablename[REL_NAME_LENG - 1] = '\0';
            indexname[0] = '\0';
        }

        if (indexname[0] != '\0')
        {
            if (!mdb_strcmp(indexname, "primary"))
            {
                sc_snprintf(indexname, INDEX_NAME_LENG - 1, "$pk.%s",
                        tablename);
                indexname[INDEX_NAME_LENG - 1] = '\0';
            }
        }

        cont = Cont_Search(tablename);
        if (cont == NULL)
        {
            ret = DB_E_UNKNOWNTABLE;
            goto end;
        }

        cont->base_cont_.index_rebuild = 1;
        SetDirtyFlag(cont->collect_.cont_id_);

        index_oid = cont->base_cont_.index_id_;
        while (index_oid != NULL_OID)
        {
            ttree = (struct TTree *) OID_Point(index_oid);
            if (ttree == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }

            /* skip for temp or partial index */
            if (isPartialIndex_name(ttree->base_cont_.name_))
            {
                index_oid = ttree->base_cont_.index_id_;
                continue;
            }

            if (indexname[0] == '\0')   /* check all indexes */
            {
                if (ttree->base_cont_.index_rebuild == 0)
                {
                    SetDirtyFlag(index_oid);
                    ttree->base_cont_.index_rebuild = 1;
                }
            }
            else
            {   /* check the index */
                if (!mdb_strcmp(ttree->base_cont_.name_, indexname))
                {
                    if (ttree->base_cont_.index_rebuild == 0)
                    {
                        SetDirtyFlag(index_oid);
                        ttree->base_cont_.index_rebuild = 1;
                    }
                    break;
                }
            }

            index_oid = ttree->base_cont_.index_id_;
        }

        if (indexname[0] != '\0' && index_oid == NULL_OID)
        {
            ret = DB_E_UNKNOWNINDEX;
            goto end;
        }
    }

    /* do checkpoint */
    _Checkpointing(0, 1);

    /* do rebuild indexes by DDL */
    ret = ContCat_Index_Rebuild(PARTIAL_REBUILD_DDL);

    /* do checkpoint */
    if (ret == DB_SUCCESS)
    {
        server->gDirtyBit = INIT_gDirtyBit;
        _Checkpointing(0, 1);
        goto end_trans;
    }
    else    /* rollback index - read from index file */
    {
        Index_MemoryMgr_FreeSegment();
        Index_MemoryMgr_initFromDBFile();
    }

  end:
    /* clean index_rebuild of all tables */
    cont_oid = Col_GetFirstDataID((struct Container *) container_catalog, 0);
    while (cont_oid != NULL_OID)
    {
        if (cont_oid == INVALID_OID)
        {
            ret = DB_E_OIDPOINTER;
            goto end_trans;
        }

        cont = (struct Container *) OID_Point(cont_oid);
        if (cont == NULL)
            break;

        cont->base_cont_.index_rebuild = 0;
        SetDirtyFlag(cont_oid);

        Col_GetNextDataID(cont_oid,
                container_catalog->collect_.slot_size_,
                container_catalog->collect_.page_link_.tail_, 0);
    }

    /* clean index_rebuild of all indexes */
    index_oid = Col_GetFirstDataID((struct Container *) index_catalog, 0);
    while (index_oid != NULL_OID)
    {
        if (index_oid == INVALID_OID)
        {
            ret = DB_E_OIDPOINTER;
            goto end_trans;
        }
        ttree = (struct TTree *) OID_Point(index_oid);
        if (ttree == NULL)
            break;

        if (ttree->base_cont_.index_rebuild == 1)
        {
            SetDirtyFlag(index_oid);
            ttree->base_cont_.index_rebuild = 0;
        }

        Col_GetNextDataID(index_oid,
                index_catalog->collect_.slot_size_,
                index_catalog->collect_.page_link_.tail_, 0);
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
