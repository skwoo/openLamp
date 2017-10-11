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

__DECL_PREFIX struct ContainerCatalog *container_catalog;
__DECL_PREFIX struct TempContainerCatalog *temp_container_catalog;

__DECL_PREFIX struct Container_Name_Hash *hash_cont_name = NULL;
_PRIVATE int cont_hash_name(const char *cont_name);

__DECL_PREFIX struct Container_Tableid_Hash *hash_cont_tableid = NULL;
_PRIVATE int cont_hash_tableid(int tableid);

_PRIVATE struct Container *__Cont_Search_Tableid(int tableid);

int Drop_Table_NoLock(struct Container *Cont);
int _Cache_DeleteTempTableSchema(char *tableName);
int _Cache_DeleteTempIndexSchema(char *indexName);

/*****************************************************************************/
//! ContCatalog_init_Createdb 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param cont_item_size :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
ContCatalog_init_Createdb(int cont_item_size)
{
    struct ContainerCatalog ContCat;

    MDB_SYSLOG(("sizeof(ContCat):%d\n", sizeof(ContCat)));

    sc_memset(&ContCat, 0, sizeof(struct ContainerCatalog));

    ContCat.collect_.cont_id_ = PAGE_HEADER_SIZE + CONTCAT_OFFSET;
    ContCat.collect_.item_count_ = 0;
    ContCat.collect_.page_count_ = 0;
    ContCat.collect_.item_size_ = cont_item_size;
    ContCat.collect_.slot_size_ =
            _alignedlen(ContCat.collect_.item_size_ + 1, sizeof(long));
    ContCat.collect_.page_link_.head_ = NULL_PID;
    ContCat.collect_.page_link_.tail_ = NULL_PID;
    ContCat.collect_.free_page_list_ = NULL_PID;
    ContCat.collect_.collect_index_ = -1;

    ContCat.base_cont_.id_ = 0;
    ContCat.base_cont_.name_[0] = '\0';
    ContCat.base_cont_.type_ = _CONT_TABLE;
    ContCat.base_cont_.index_rebuild = 0;
    ContCat.base_cont_.is_dirty_ = 0;
    ContCat.base_cont_.id_ = 0;
    ContCat.base_cont_.creator_ = _USER_SYS;
    ContCat.base_cont_.transid_ = 0;
    ContCat.tableid = 0;

    server->temp_tableid = _TEMP_TABLEID_BASE;

    segment_[0] = DBDataMem_V2P(Mem_mgr->segment_memid[0]);

    container_catalog = (struct ContainerCatalog *)
            (&(segment_[0]->page_[0].body_[0]) + CONTCAT_OFFSET);

    sc_memcpy(container_catalog, &ContCat, sizeof(struct ContainerCatalog));

    return DB_SUCCESS;
}

/*****************************************************************************/
//! ContCatalogServerInit 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
struct ContainerCatalog *
ContCatalogServerInit(void)
{
    segment_[0] = DBDataMem_V2P(Mem_mgr->segment_memid[0]);

    container_catalog = (struct ContainerCatalog *)
            (&(segment_[0]->page_[0].body_[0]) + CONTCAT_OFFSET);

    server->temp_tableid = _TEMP_TABLEID_BASE;

    temp_container_catalog = &server->temp_container_catalog;

    temp_container_catalog->collect_.cont_id_ = OID_Cal(TEMPSEG_BASE_NO, 0, 0);
    temp_container_catalog->collect_.slot_size_
            = container_catalog->collect_.slot_size_;
    temp_container_catalog->collect_.item_size_
            = container_catalog->collect_.item_size_;
    temp_container_catalog->collect_.record_size_
            = container_catalog->collect_.record_size_;

    temp_container_catalog->base_cont_.type_ = _CONT_TEMPTABLE;
    temp_container_catalog->base_cont_.memory_record_size_
            = container_catalog->base_cont_.memory_record_size_;

    return container_catalog;
}

/*****************************************************************************/
//! ContCatalogServerDown 
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
DB_INT32
ContCatalogServerDown(void)
{
    return 0;
}

////////////////////////////////////////////////////////////////////////
//
//  Function name        :  Relation Search Funtion
//  CALL Function name     : Create_Table(),
//
////////////////////////////////////////////////////////////////////////


/*****************************************************************************/
//! Cont_Search 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param cont_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
struct Container *
Cont_Search(const char *cont_name)
{
    OID cont_oid;
    struct Container *Cont = NULL;
    int hashval;
    struct Container_Name_Node *pName;

    hashval = cont_hash_name(cont_name);

    pName = DBMem_V2P(hash_cont_name->hashptr[hashval]);

    while (pName)
    {
        if (!mdb_strcmp(pName->name, cont_name))
            break;

        pName = DBMem_V2P(pName->next);
    }

    if (pName)
    {
        cont_oid = pName->cont_oid;

        /* USED SLOT이 아닌 경우 NULL return */
        /* Keep as of it, because the slot size of table is same of
           that of temp table. sizeof(struct Container) */
        if (!Col_IsDataSlot(cont_oid,
                        container_catalog->collect_.slot_size_, 0))
        {
            Cont = NULL;
        }
        else
        {
            Cont = (struct Container *) OID_Point(cont_oid);

            if (Cont == NULL || mdb_strcmp(Cont->base_cont_.name_, cont_name))
            {
                Cont = NULL;
            }
        }
    }

    return Cont;
}

/*****************************************************************************/
//! Cont_Search_Tableid 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param tableid :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
struct Container *
Cont_Search_Tableid(int tableid)
{
    struct Container *Cont = NULL;

    if (tableid == 0)
        return NULL;

    Cont = __Cont_Search_Tableid(tableid);

    return Cont;
}

/*****************************************************************************/
//! __Cont_Search_Tableid 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param tableid :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
_PRIVATE struct Container *
__Cont_Search_Tableid(int tableid)
{
    OID cont_oid;
    struct Container *Cont = NULL;
    int hashval;
    struct Container_Tableid_Node *pTableid;

    hashval = cont_hash_tableid(tableid);

    pTableid = DBMem_V2P(hash_cont_tableid->hashptr[hashval]);

    while (pTableid)
    {
        if (pTableid->tableid == tableid)
            break;

        pTableid = DBMem_V2P(pTableid->next);
    }

    if (pTableid)
    {
        cont_oid = pTableid->cont_oid;

        if (!Col_IsDataSlot(cont_oid,
                        container_catalog->collect_.slot_size_, 0))
        {
            Cont = NULL;
        }

        Cont = (struct Container *) OID_Point(cont_oid);

        if (Cont == NULL || cont_oid != Cont->collect_.cont_id_
                || Cont->base_cont_.id_ != tableid)
        {
            Cont = NULL;
        }
    }

    return Cont;
}

/* cont_name table의 item_count_ (레코드수)를 return */
/*****************************************************************************/
//! Cont_Item_Count 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param cont_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
Cont_Item_Count(const char *cont_name)
{
    OID cont_oid;
    struct Container *Cont = NULL;
    int hashval;
    struct Container_Name_Node *pName;
    int count = 0;

    hashval = cont_hash_name(cont_name);

    pName = DBMem_V2P(hash_cont_name->hashptr[hashval]);

    while (pName)
    {
        if (!mdb_strcmp(pName->name, cont_name))
            break;

        pName = DBMem_V2P(pName->next);
    }

    if (pName)
    {
        cont_oid = pName->cont_oid;

        if (!Col_IsDataSlot(cont_oid,
                        container_catalog->collect_.slot_size_, 0))
        {
            Cont = NULL;
        }

        Cont = (struct Container *) OID_Point(cont_oid);

        if (Cont == NULL || cont_oid != Cont->collect_.cont_id_
                || mdb_strcmp(Cont->base_cont_.name_, cont_name))
        {
            Cont = NULL;
        }
    }

    if (Cont != NULL)
    {
        count = (int) Cont->collect_.item_count_;

        /* 레코드 count의 값이 음수일 때 다시 계산.
           속도 향상을 위해서 page의 record_count_in_page_의 합을 구한다. */
        if (count < 0)
        {
            struct Page *temp_page;

            MDB_SYSLOG(("Recompute collection item count begin (%d)\n",
                            count));

            count = 0;

            if (Cont->collect_.page_link_.head_ != NULL_OID)
            {
                int issys_cont;

                SetBufSegment(issys_cont, Cont->collect_.cont_id_);

                temp_page = (struct Page *)
                        PageID_Pointer(Cont->collect_.page_link_.head_);

                while (1)
                {
                    if (temp_page == NULL)
                        break;

                    count += temp_page->header_.record_count_in_page_;

                    if (temp_page->header_.link_.next_ == NULL_OID)
                        break;

                    temp_page = (struct Page *)
                            PageID_Pointer(temp_page->header_.link_.next_);
                }       /* while */

                UnsetBufSegment(issys_cont);
            }

            Cont->collect_.item_count_ = count;
            SetDirtyFlag(Cont->collect_.cont_id_);

            MDB_SYSLOG(("Recompute collection item count end (%d)\n", count));
        }
    }

    if (Cont == NULL)
        return DB_E_UNKNOWNTABLE;

    return count;
}

/*****************************************************************************/
//! Cont_Catalog_Remove 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param container_id :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
Cont_Catalog_Remove(ContainerID container_id)
{
    struct ContainerCatalog *cont_catalog;

    if (isTemp_OID(container_id))
    {
        cont_catalog = (struct ContainerCatalog *) temp_container_catalog;
    }
    else
    {
        cont_catalog = container_catalog;
    }

    if (Col_Remove((struct Container *) &(cont_catalog->collect_),
                    -1, container_id, 0) < 0)
    {
        MDB_SYSLOG(("table slot remove FAIL\n"));
        return DB_FAIL;
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! Cont_CreateHashing 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
Cont_CreateHashing(void)
{
    PageID contcat_pageid;
    OID cont_oid;
    struct Container *Cont;
    int ret;
    int issys;

    sc_memset(hash_cont_name, 0, sizeof(struct Container_Name_Hash));
    sc_memset(hash_cont_tableid, 0, sizeof(struct Container_Tableid_Hash));

    contcat_pageid = (PageID) container_catalog->collect_.page_link_.head_;

    if (contcat_pageid == NULL_PID)
        goto end;

    // 릴레이션 페이지의 첫번째 Slot 물리적 주소를 계산한다.
    cont_oid = OID_Cal(getSegmentNo(contcat_pageid),
            getPageNo(contcat_pageid), PAGE_HEADER_SIZE);

    if (!Col_IsDataSlot(cont_oid, container_catalog->collect_.slot_size_, 0))
    {
        Col_GetNextDataID(cont_oid,
                container_catalog->collect_.slot_size_,
                container_catalog->collect_.page_link_.tail_, 0);
    }

    while (cont_oid != NULL_OID)
    {
        if (cont_oid == INVALID_OID)
        {
            return DB_E_OIDPOINTER;
        }

        Cont = (struct Container *) OID_Point(cont_oid);

        if (Cont == NULL)
        {
            return DB_E_INVLAIDCONTOID;
        }

        SetBufSegment(issys, cont_oid);

        Cont->base_cont_.runtime_logging_ = _RUNTIME_LOGGING_NONE;

        if (isResidentTable(Cont))
            Cont_ResidentTable_Init(Cont);

        {
            int var_idx;

            for (var_idx = 0; var_idx < VAR_COLLECT_CNT; var_idx++)
            {
                if (Cont->var_collects_[var_idx].slot_size_
                        !=
                        REC_SLOT_SIZE(Cont->var_collects_[var_idx].item_size_))
                {
                    Cont->var_collects_[var_idx].slot_size_
                            =
                            REC_SLOT_SIZE(Cont->var_collects_[var_idx].
                            item_size_);
                    SetDirtyFlag(Cont->collect_.cont_id_);
                }
            }
        }

        ret = Cont_InputHashing(Cont->base_cont_.name_, Cont->base_cont_.id_,
                cont_oid);

        UnsetBufSegment(issys);

        if (ret < 0)
            return ret;

        Col_GetNextDataID(cont_oid,
                container_catalog->collect_.slot_size_,
                container_catalog->collect_.page_link_.tail_, 0);
    }

  end:
    return DB_SUCCESS;
}

/*****************************************************************************/
//! Cont_FreeHashing 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
Cont_FreeHashing(void)
{
    int i;
    struct Container_Name_Node *pName;
    struct Container_Tableid_Node *pTableid;

    if (hash_cont_name == NULL && hash_cont_tableid == NULL)
        return DB_SUCCESS;

    for (i = 0; i < CONTAINER_HASH_SIZE; i++)
    {
        if (hash_cont_name)
        {
            while (hash_cont_name->hashptr[i])
            {
                pName = DBMem_V2P(hash_cont_name->hashptr[i]);
                hash_cont_name->hashptr[i] = pName->next;
                DBMem_Free(pName);
            }
        }

        if (hash_cont_tableid)
        {
            while (hash_cont_tableid->hashptr[i])
            {
                pTableid = DBMem_V2P(hash_cont_tableid->hashptr[i]);
                hash_cont_tableid->hashptr[i] = pTableid->next;
                DBMem_Free(pTableid);
            }
        }
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! Cont_InputHashing 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param cont_name :
 * \param tableId : 
 * \param cont_oid :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
Cont_InputHashing(const char *cont_name, const int tableId, const OID cont_oid)
{
    int hashval;
    struct Container_Name_Node *pName;
    struct Container_Tableid_Node *pTableid;

    if (cont_name == NULL || cont_name[0] == '\0')
        return DB_SUCCESS;

#ifdef MDB_DEBUG
    {
        /* hash checking */
        int Cont_CheckHashing(const char *cont_name, const OID cont_oid);

        if (Cont_CheckHashing(cont_name, cont_oid) < 0)
            return DB_SUCCESS;
    }
#endif

    pName = (struct Container_Name_Node *)
            DBMem_Alloc(sizeof(struct Container_Name_Node));
    if (pName == NULL)
    {
        MDB_SYSLOG(("cont hashing; no more memory\n"));

        return DB_E_OUTOFMEMORY;
    }

    pTableid = (struct Container_Tableid_Node *)
            DBMem_Alloc(sizeof(struct Container_Tableid_Node));
    if (pTableid == NULL)
    {
        DBMem_Free(pName);

        MDB_SYSLOG(("cont hashing; no more memory\n"));

        return DB_E_OUTOFMEMORY;
    }

    /* for name */
    sc_strcpy(pName->name, cont_name);
    pName->cont_oid = cont_oid;

    hashval = cont_hash_name(cont_name);

    /* 젤 앞쪽에 넣음 */
    pName->next = hash_cont_name->hashptr[hashval];
    hash_cont_name->hashptr[hashval] = DBMem_P2V(pName);

    pTableid->tableid = tableId;
    pTableid->cont_oid = cont_oid;

    hashval = cont_hash_tableid(tableId);

    /* 젤 앞쪽에 넣음 */
    pTableid->next = hash_cont_tableid->hashptr[hashval];
    hash_cont_tableid->hashptr[hashval] = DBMem_P2V(pTableid);

    return DB_SUCCESS;
}

/*****************************************************************************/
//! Cont_RemoveHashing 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param cont_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
Cont_RemoveHashing(const char *cont_name, const int tableid)
{
    int hashval;

    if (1)
    {
        struct Container_Name_Node *pNode, *pPrev;

        hashval = cont_hash_name(cont_name);

        pNode = DBMem_V2P(hash_cont_name->hashptr[hashval]);
        pPrev = NULL;

        while (pNode)
        {
            if (!mdb_strcmp(pNode->name, cont_name))
            {
                if (pPrev == NULL)
                    hash_cont_name->hashptr[hashval] = pNode->next;
                else
                    pPrev->next = pNode->next;

                DBMem_Free(pNode);

                if (pPrev == NULL)
                    pNode = DBMem_V2P(hash_cont_name->hashptr[hashval]);
                else
                    pNode = DBMem_V2P(pPrev->next);

                continue;
            }

            pPrev = pNode;
            pNode = DBMem_V2P(pNode->next);
        }
    }

    if (1)
    {
        struct Container_Tableid_Node *pNode, *pPrev;

        hashval = cont_hash_tableid(tableid);

        pNode = DBMem_V2P(hash_cont_tableid->hashptr[hashval]);
        pPrev = NULL;

        while (pNode)
        {
            if (pNode->tableid == tableid)
            {
                if (pPrev == NULL)
                    hash_cont_tableid->hashptr[hashval] = pNode->next;
                else
                    pPrev->next = pNode->next;

                DBMem_Free(pNode);

                if (pPrev == NULL)
                    pNode = DBMem_V2P(hash_cont_tableid->hashptr[hashval]);
                else
                    pNode = DBMem_V2P(pPrev->next);

                continue;
            }

            pPrev = pNode;
            pNode = DBMem_V2P(pNode->next);
        }
    }

#ifdef MDB_DEBUG
    {
        /* hash checking */
        int Cont_CheckHashing(const char *cont_name, const OID cont_oid);

        Cont_CheckHashing(cont_name, 0);
    }
#endif
    return DB_SUCCESS;
}

#ifdef MDB_DEBUG
int
Cont_CheckHashing(const char *cont_name, const OID cont_oid)
{
    int hashval;

    if (1)
    {
        struct Container_Name_Node *pNode;

        hashval = cont_hash_name(cont_name);

        pNode = DBMem_V2P(hash_cont_name->hashptr[hashval]);

        while (pNode)
        {
            if (!mdb_strcmp(pNode->name, cont_name))
            {
                sc_assert(0, __FILE__, __LINE__);
            }

            pNode = DBMem_V2P(pNode->next);
        }
    }

    if (cont_oid)
    {
        struct Container_Tableid_Node *pNode;
        struct Container *Cont = (struct Container *) OID_Point(cont_oid);

        if (Cont == NULL)
        {
            return DB_E_OIDPOINTER;
        }

        hashval = cont_hash_tableid(Cont->base_cont_.id_);

        pNode = DBMem_V2P(hash_cont_tableid->hashptr[hashval]);

        while (pNode)
        {
            if (pNode->tableid == Cont->base_cont_.id_)
            {
                MDB_SYSLOG(("Cont:%s,%d,0x%x, Node:%s,%d,0x%x\n",
                                Cont->base_cont_.name_, Cont->base_cont_.id_,
                                Cont->collect_.cont_id_,
                                cont_name, pNode->tableid, pNode->cont_oid));
                sc_assert(0, __FILE__, __LINE__);
            }

            pNode = DBMem_V2P(pNode->next);
        }
    }

    return 0;
}
#endif

/*****************************************************************************/
//! cont_hash_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param cont_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
_PRIVATE int
cont_hash_name(const char *cont_name)
{
    register unsigned int hash_val = 0;
    register DB_UINT32 count = 0;
    register char c;

    for (count = 0;; count++)
    {
        c = cont_name[count];

        if (c == '\0')
            break;

        hash_val += mdb_tolwr(c);
    }

    return (hash_val % CONTAINER_HASH_SIZE);
}

/*****************************************************************************/
//! cont_hash_tableid 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param tableid :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
_PRIVATE int
cont_hash_tableid(int tableid)
{
    return (tableid % CONTAINER_HASH_SIZE);
}

/*****************************************************************************/
//! Cont_RemoveTempTables 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
Cont_RemoveTempTables(int connid)
{
    PageID contcat_pageid;
    OID cont_oid;
    struct Container *Cont;
    DB_UINT32 tallocated_segment_no_ = mem_anchor_->tallocated_segment_no_;
    PageID tfirst_free_page_id_ = mem_anchor_->tfirst_free_page_id_;
    DB_UINT32 tfree_page_count_ = mem_anchor_->tfree_page_count_;
    int _g_connid_bk = -2;
    int ret = DB_SUCCESS;
    extern int Drop_Table_Temp(char *Cont_name);
    int issys_cont;

    contcat_pageid = (PageID) container_catalog->collect_.page_link_.head_;

    if (contcat_pageid == NULL_PID)
        goto end;

    // 릴레이션 페이지의 첫번째 Slot 물리적 주소를 계산한다.
    cont_oid = OID_Cal(getSegmentNo(contcat_pageid), getPageNo(contcat_pageid),
            PAGE_HEADER_SIZE);

    if (!Col_IsDataSlot(cont_oid, container_catalog->collect_.slot_size_, 0))
    {
        Col_GetNextDataID(cont_oid,
                container_catalog->collect_.slot_size_,
                container_catalog->collect_.page_link_.tail_, 0);
    }

    while (cont_oid != NULL_OID)
    {
        if (cont_oid == INVALID_OID)
        {
            ret = DB_E_OIDPOINTER;
            goto end;
        }

        Cont = (struct Container *) OID_Point(cont_oid);

        if (Cont == NULL)
        {
            break;
        }

        SetBufSegment(issys_cont, cont_oid);

        Col_GetNextDataID(cont_oid,
                container_catalog->collect_.slot_size_,
                container_catalog->collect_.page_link_.tail_, 0);

        /* temp table 제거 */
        if (isTempTable(Cont))
        {
            if (connid == -1 ||
                    Cont->base_cont_.creator_ == (connid + _CONNID_BASE))
            {
                MDB_SYSLOG(("WARNING: Remaining temp table %s will be "
                                "dropped.\n", Cont->base_cont_.name_));

                PUSH_G_CONNID(0);

                ret = Drop_Table(Cont->base_cont_.name_);
                if (ret < 0)
                {
                    MDB_SYSLOG(("Remaining temp table %s FAIL. %d\n",
                                    Cont->base_cont_.name_, ret));
                }


                POP_G_CONNID();
            }
        }
        else
        {
            OID index_id;

            index_id = Cont->base_cont_.index_id_;

#ifdef MDB_DEBUG
            if (isTemp_OID(index_id))
            {
                sc_assert(0, __FILE__, __LINE__);
            }
#endif
        }
        UnsetBufSegment(issys_cont);
    }       /* while */

  end:
    mem_anchor_->tallocated_segment_no_ = tallocated_segment_no_;
    mem_anchor_->tfirst_free_page_id_ = tfirst_free_page_id_;
    mem_anchor_->tfree_page_count_ = tfree_page_count_;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! Cont_RemoveTransTempObjects 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid :
 * \param transid : 
  ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
Cont_RemoveTransTempObjects(int connid, int transid)
{
    char relation_name[REL_NAME_LENG];
    char index_name[INDEX_NAME_LENG];

    /* remove temp indexes of regular tables */
    while (_Schema_Trans_FirstTempIndexName(transid, index_name) == DB_SUCCESS)
    {
        dbi_Index_Drop(connid, index_name);
        _Cache_DeleteTempIndexSchema(index_name);
    }

    /* remove temp tables and temp indexes of the temp tables */
    while (_Schema_Trans_FirstTempTableName(transid,
                    relation_name) == DB_SUCCESS)
    {
        dbi_Relation_Drop(connid, relation_name);
        _Cache_DeleteTempTableSchema(relation_name);
    }

    return DB_SUCCESS;
}

extern int __Truncate_Table(struct Container *Cont, int f_pagelist_attach);

/*****************************************************************************/
//! Cont_TablesConsistency 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
Cont_TablesConsistency(void)
{
    struct Container *Cont;
    OID record_id;
    SYSTABLE_T *pSysTable;
    int slot_size;
    OID link_tail;

    Cont = Cont_Search("systables");
    if (Cont == NULL)
    {
        MDB_SYSLOG(("FAIL: can not find systables\n"));
        return DB_E_SYSTABLECONT;
    }

    slot_size = Cont->collect_.slot_size_;
    link_tail = Cont->collect_.page_link_.tail_;

    record_id = Col_GetFirstDataID(Cont, 0);

    while (1)
    {
        if (record_id == INVALID_OID)
        {
            return DB_E_OIDPOINTER;
        }

        if (record_id == NULL_OID)
            break;

        pSysTable = (SYSTABLE_T *) OID_Point(record_id);
        if (pSysTable == NULL)
        {
            return DB_E_OIDPOINTER;
        }

        /* 해당 table이 없음. 스키마 정보 제거 */
        if (Cont_Search(pSysTable->tableName) == NULL)
        {
            dbi_Relation_Drop(-1, pSysTable->tableName);
        }

        Col_GetNextDataID(record_id, slot_size, link_tail, 0);
    }

    {
        char *_systable[] = {
            "sysfields", "sysindexes", "sysindexfields",
            "sysstatus", "sysdummy", "syssequences", NULL
        };
        int i = 0;

        while (_systable[i])
        {
            if (Cont_Search(_systable[i]) == NULL)
            {
                __SYSLOG("FAIL: can not find %s\n", _systable[i]);
                return DB_E_SYSTABLECONT;
            }
            ++i;
        }
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! ContCatalog_TableidInit 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param tableid :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX int
ContCatalog_TableidInit(int tableid)
{
    container_catalog->tableid = tableid;

    SetPageDirty(0, 0);

    return DB_SUCCESS;
}

/*****************************************************************************/
//! Cont_NewTableID 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
Cont_NewTableID(int userid)
{
    int tableid;

    tableid = container_catalog->tableid;

    if (userid > _USER_SYSTEM && tableid < _USER_TABLEID_BASE)
        tableid = _USER_TABLEID_BASE;

    while (1)
    {
        if (__Cont_Search_Tableid(tableid) == NULL)
            break;
        if (tableid + 1 == _USER_TABLEID_END)
            tableid = _USER_TABLEID_BASE;
        else
            tableid++;
    }

    if (tableid + 1 >= _USER_TABLEID_END)
        container_catalog->tableid = _USER_TABLEID_BASE;
    else
        container_catalog->tableid = tableid + 1;

    SetPageDirty(0, 0);

    return tableid;
}

/*****************************************************************************/
//! Cont_NewTempTableID 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
Cont_NewTempTableID(int userid)
{
    int tableid;

    tableid = server->temp_tableid;

    if (userid > _USER_SYSTEM && tableid < _TEMP_TABLEID_BASE)
        tableid = _TEMP_TABLEID_BASE;

    while (1)
    {
        if (__Cont_Search_Tableid(tableid) == NULL)
            break;
        tableid++;
    }

    if (tableid + 1 == MDB_INT_MAX)
    {
        server->temp_tableid = _TEMP_TABLEID_BASE;
    }
    else
    {
        server->temp_tableid = tableid + 1;
    }

    return tableid;
}

int
Cont_ResidentTable_Init(struct Container *cont)
{
    int i;
    OID index_id;
    struct TTree *ttree;

    /* Table collect init */
    cont->collect_.page_count_ = 0;
    cont->collect_.item_count_ = 0;
    cont->collect_.free_page_list_ = NULL_OID;
    cont->collect_.page_link_.head_ = NULL_OID;
    cont->collect_.page_link_.tail_ = NULL_OID;

    /* variable collect init */
    for (i = 0; i < VAR_COLLECT_CNT; i++)
    {
        cont->var_collects_[i].page_count_ = 0;
        cont->var_collects_[i].item_count_ = 0;
        cont->var_collects_[i].free_page_list_ = NULL_OID;
        cont->var_collects_[i].page_link_.head_ = NULL_OID;
        cont->var_collects_[i].page_link_.tail_ = NULL_OID;
    }

    /* Index Container init */
    if (cont->base_cont_.index_count_ > 0)
    {
        index_id = cont->base_cont_.index_id_;
        while (index_id != NULL_OID)
        {
            ttree = (struct TTree *) OID_Point(index_id);
            if (ttree == NULL)
            {
                return DB_E_OIDPOINTER;
            }

            for (i = 0; i < NUM_TTREE_BUCKET; i++)
                ttree->root_[i] = NULL_OID;

            ttree->collect_.page_count_ = 0;
            ttree->collect_.item_count_ = 0;
            ttree->collect_.free_page_list_ = NULL_OID;
            ttree->collect_.page_link_.head_ = NULL_OID;
            ttree->collect_.page_link_.tail_ = NULL_OID;

            SetDirtyFlag(index_id);

            index_id = ttree->base_cont_.index_id_;
        }
    }

    return DB_SUCCESS;
}
