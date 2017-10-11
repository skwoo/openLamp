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
//! dbi_Schema_GetTableInfo
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid        :
 * \param relation_name    :
 * \param pSysTable        :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Schema_GetTableInfo(int connid, char *relation_name,
        SYSTABLE_T * pSysTable)
{
    int *pTransid;
    int ret;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (relation_name[0] == '\0' || pSysTable == NULL)
        return DB_E_INVALIDPARAM;
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    ret = _Schema_GetTableInfo(relation_name, pSysTable);
    if (ret == DB_E_UNKNOWNTABLE)
    {
        struct Container *Cont = Cont_Search(relation_name);

        if (Cont != NULL)
        {
            _Schema_Cache_Free(SCHEMA_LOAD_REGU);
            _Schema_Cache_Init(SCHEMA_LOAD_REGU);
            ret = _Schema_GetTableInfo(relation_name, pSysTable);
            if (ret == DB_E_UNKNOWNTABLE)
            {
#ifdef MDB_DEBUG
                _Schema_Cache_Dump();
                sc_assert(0, __FILE__, __LINE__);
#endif
            }
        }
    }

  end:

    return ret;
}


/*****************************************************************************/
//! dbi_Schema_GetUserTablesInfo
/*! \breif USER TABLE의 정보만을 얻어온다
 ************************************
 * \param connid(in)        :
 * \param pSysTable(in/out) :
 ************************************
 * \return  return_value    success is user table's number
 ************************************
 * \note
 *  - 함수 추가
 *****************************************************************************/
extern int _Schema_GetTablesInfoEx(int mode, SYSTABLE_T * pSysTable,
        int userid);

int
dbi_Schema_GetUserTablesInfo(int connid, SYSTABLE_T ** pSysTable)
{
    return dbi_Schema_GetUserTablesInfo_byUser(connid, pSysTable, _USER_USER);
}

int
dbi_Schema_GetUserTablesInfo_byUser(int connid, SYSTABLE_T ** pSysTable,
        int userid)
{
    int *pTransid;
    int ret;
    int tableCnt;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (pSysTable == NULL)
        return DB_E_INVALIDPARAM;
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    tableCnt = dbi_Cursor_DirtyCount(connid, "systables");
    if (tableCnt <= 0)
    {
        ret = tableCnt;
        goto end;
    }

    *pSysTable = PMEM_ALLOC(sizeof(SYSTABLE_T) * tableCnt);
    if (*pSysTable == NULL)
    {
        ret = DB_E_OUTOFMEMORY;
        goto end;
    }

    ret = _Schema_GetTablesInfoEx(1, *pSysTable, userid);

  end:

    return ret;
}

/*****************************************************************************/
//! dbi_Schema_GetFieldInfo
/*! \breif  relationname에 해당하는 테이블의 필드 정보를 받아온다
 ************************************
 * \param connid(in)        : connection id
 * \param relation_name(in) : TABLE 이름
 * \param ppSysField(in/out): FIELD 정보
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Schema_GetFieldsInfo(int connid, char *relation_name,
        SYSFIELD_T ** ppSysField)
{
    int ret;
    int *pTransid;
    SYSTABLE_T sysTable;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (relation_name[0] == '\0')
        return DB_E_INVALIDPARAM;
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    ret = _Schema_GetTableInfo(relation_name, &sysTable);
    if (ret < 0)
    {
        goto end;
    }

    *ppSysField =
            (SYSFIELD_T *) PMEM_ALLOC((sizeof(SYSFIELD_T) *
                    sysTable.numFields) + 4);
    if (*ppSysField == NULL)
    {
        ret = DB_E_OUTOFMEMORY;
        goto end;
    }

    ret = _Schema_GetFieldsInfo(relation_name, *ppSysField);
    if (ret <= 0)
    {
        PMEM_FREENUL(*ppSysField);
    }

  end:

    return ret;
}

int
dbi_Schema_GetSingleFieldsInfo(int connid, char *relation_name,
        char *field_name, SYSFIELD_T * pSysField)
{
    int *pTransid;
    int ret;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (*relation_name == 0x00 || *field_name == 0x00)
        return DB_E_INVALIDPARAM;
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    ret = _Schema_GetSingleFieldsInfo(relation_name, field_name, pSysField);

  end:
    return ret;
}

int
dbi_Schema_GetTableFieldsInfo(int connid, char *relation_name,
        SYSTABLE_T * pSysTable, SYSFIELD_T ** ppSysField)
{
    int *pTransid;
    int ret;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (relation_name[0] == '\0')
        return DB_E_INVALIDPARAM;
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    ret = _Schema_GetTableInfo(relation_name, pSysTable);
    if (ret < 0)
    {
        *ppSysField = NULL;
        goto end;
    }

    *ppSysField = (SYSFIELD_T *) PMEM_ALLOC(sizeof(SYSFIELD_T) *
            pSysTable->numFields);
    if (*ppSysField == NULL)
    {
        ret = DB_E_OUTOFMEMORY;
        goto end;
    }

    ret = _Schema_GetFieldsInfo(relation_name, *ppSysField);
    if (ret <= 0)
    {       /* ret : numFields */
        PMEM_FREENUL(*ppSysField);
    }

  end:

    return ret; /* numFields */
}

/*****************************************************************************/
//! dbi_Schema_GetIndexInfo
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid            :
 * \param indexName            :
 * \param pSysIndex            :
 * \param ppSysIndexField(in/out)   : index's fields description
 * \param relationName        :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Schema_GetIndexInfo(int connid, char *indexName,
        SYSINDEX_T * pSysIndex,
        SYSINDEXFIELD_T ** ppSysIndexField, char *relationName)
{
    int *pTransid;
    int ret;
    SYSINDEX_T tmpSysIndex;
    char __relationName[REL_NAME_LENG] = "$";

#ifdef MDB_DEBUG
    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (indexName[0] == '\0')
        return DB_E_INVALIDPARAM;
#endif
#endif

    if (pSysIndex == NULL)
        pSysIndex = &tmpSysIndex;

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    if (relationName == NULL)
        relationName = __relationName;
    else
        relationName[0] = '\0';

    ret = _Schema_GetIndexInfo(indexName, pSysIndex, NULL, relationName);
    if (ret < 0)
        goto end;

    if (ppSysIndexField != NULL)
    {
        *ppSysIndexField =
                (SYSINDEXFIELD_T *) PMEM_ALLOC(sizeof(SYSINDEXFIELD_T) *
                pSysIndex->numFields);
        if (*ppSysIndexField == NULL)
        {
            ret = DB_E_OUTOFMEMORY;
            goto end;
        }

        sc_memset(*ppSysIndexField, 0,
                sizeof(SYSINDEXFIELD_T) * pSysIndex->numFields);

        ret = _Schema_GetIndexInfo(indexName, pSysIndex, *ppSysIndexField,
                NULL);
    }

    if (ret < 0)
    {
        if (ppSysIndexField != NULL)
            PMEM_FREENUL(*ppSysIndexField);

        goto end;
    }

    ret = DB_SUCCESS;

  end:

    return ret;
}

/*****************************************************************************/
//! dbi_Schema_GetTableIndexesInfo
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid        :
 * \param relation_name    :
 * \param schIndexType    :
 * \param ppSysIndex    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Schema_GetTableIndexesInfo(int connid, char *relation_name,
        int schIndexType, SYSINDEX_T ** ppSysIndex)
{
    int *pTransid;
    int ret;

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

    if (ppSysIndex == NULL)
    {
        ret = _Schema_GetTableIndexesInfo(relation_name, schIndexType, NULL);
        goto end;
    }
    else
    {
        if (schIndexType == SCHEMA_ALL_INDEX)
        {
            *ppSysIndex = (SYSINDEX_T *) PMEM_ALLOC(sizeof(SYSINDEX_T)
                    * (2 * MAX_INDEX_NO));
        }
        else
        {   /* SCHEMA_REGU_INDEX | SCHEMA_TEMP_INDEX */
            *ppSysIndex = (SYSINDEX_T *) PMEM_ALLOC(sizeof(SYSINDEX_T)
                    * MAX_INDEX_NO);
        }

        if (*ppSysIndex == NULL)
        {
            ret = DB_E_OUTOFMEMORY;
            goto end;
        }

        ret = _Schema_GetTableIndexesInfo(relation_name, schIndexType,
                *ppSysIndex);
    }

    if (ret <= 0)
    {
        PMEM_FREENUL(*ppSysIndex);
    }

  end:

    return ret;
}
