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

#define __DEF_TYPES_FUNC__      // type function의 define을 이곳에서만 한다

#include "mdb_Server.h"
#include "sql_func_timeutil.h"
#include "sql_util.h"
#include "sql_decimal.h"
#include "mdb_PMEM.h"
#include "mdb_compat.h"
#include "mdb_MemBase.h"
 /* INDEX_REBUILD */
#include "mdb_ContCat.h"
#include "../../../version_mmdb.h"

void OID_SaveAsBefore(short op_type, const void *, OID id_, int size);
void OID_SaveAsAfter(short Op_Type, const void *Relation, OID id_, int size);

#define CHKPNT_COMMIT_COUNT 20

#define LOGFILENUM_TO_CHKPT 3

static int logfile_num = 0;
static int commit_count = 0;

#if defined(MDB_DEBUG)
extern int CS_Get_UserName(int connid, char **conn_username);
#endif

extern int fcreatedb;

extern int LogBuffer_Flush(int f_sync);

extern void strcpy_datatype(char *buf, int bufsz, SYSFIELD_T * field);
extern int calc_func_hexbinstring(int func_type, T_VALUEDESC * str,
        T_VALUEDESC * res, MDB_INT32 is_init);

#ifdef MDB_DEBUG
#define CHECK_DBI_PARAM
#endif

#if !defined(CHECK_DBI_PARAM)
#define DBI_CHECK_SERVER_TERM()
#define DBI_CHECK_TABLE_NAME(name)
#define DBI_CHECK_INDEX_NAME(name)
#define DBI_CHECK_RECORD_DATA(record)
#define DBI_CHECK_RECORD_LENG(length)
#define DBI_CHECK_TRANSACTION_ID(id)
#else
#define DBI_CHECK_SERVER_TERM()                            \
    if (server == NULL || server->fTerminated) {                             \
        return DB_E_TERMINATED;                            \
    }

#define DBI_CHECK_TABLE_NAME(name)                         \
    if ((name) == NULL /* || name[0] == '\0' */) {         \
        return DB_E_INVALIDPARAM;                          \
    }

#define DBI_CHECK_INDEX_NAME(name)                         \
    if ((name) == NULL /* || name[0] == '\0' */) {         \
        return DB_E_INVALIDPARAM;                          \
    }

#define DBI_CHECK_RECORD_DATA(record)                      \
    if ((record) == NULL) {                                \
        MDB_SYSLOG(("FAIL: record is null...\n"));             \
        return DB_E_INVALIDPARAM;                          \
    }

#define DBI_CHECK_RECORD_LENG(length)                      \
    if ((length) <= 0) {                                   \
        MDB_SYSLOG(("FAIL: record length is zero...\n"));      \
        return DB_E_INVALIDPARAM;                          \
    }

#define DBI_CHECK_TRANSACTION_ID(id)                       \
    if ((id) == -1) {                                      \
        return DB_E_NOTTRANSBEGIN;                         \
    }
#endif


extern int Update_Field(DB_UINT32 cursor_id, struct UpdateValue *newValues);
extern int Filter_Delete(struct Filter *pFilter);

extern int Direct_Read_Internal(struct Trans *trans, struct Container *cont,
        char *indexName, struct KeyValue *findKey, OID rid_to_read,
        LOCKMODE lock, char *record, DB_UINT32 * size,
        FIELDVALUES_T * rec_values);
extern int Direct_Insert_Internal(struct Trans *trans, struct Container *cont,
        char *item, int item_leng, LOCKMODE lock, OID * insertedRid);
extern int Direct_Remove_Internal(struct Trans *trans, struct Container *cont,
        char *indexName, struct KeyValue *findKey, OID rid_to_remove,
        LOCKMODE lock, MDB_OIDARRAY * v_oidset, SYSFIELD_T * fields_info,
        int remove_msync);
extern int Direct_Update_Internal(struct Trans *trans, struct Container *cont,
        char *indexName, struct KeyValue *findKey, OID rid_to_update,
        char *data, int data_leng, LOCKMODE lock,
        struct UpdateValue *newValues, DB_UINT8 msync_flag);
extern int Direct_Update_Field_Internal(struct Trans *trans,
        struct Container *cont, char *indexName, struct KeyValue *findKey,
        struct UpdateValue *newValues, LOCKMODE lock);

extern void dbi_DataMem_Protect_RW(void);
extern void dbi_DataMem_Protect_RO(void);

extern int (*sc_ncollate_func[]) ();
extern char *(*sc_collate_strchr_func[]) ();
extern char *(*sc_collate_strstr_func[]) ();

#undef __DEF_TYPES_FUNC__
#include "mdb_like.c"
#define __DEF_TYPES_FUNC__

/*****************************************************************************/
//! dbi_Connect_Server
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param dbserver :
 * \param dbname :
 * \param username :
 * \param password :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Connect_Server(int handle, char *dbserver, char *dbname, char *username,
        char *password)
{
    int ret = DB_SUCCESS;

    return ret;
}

/*****************************************************************************/
//! dbi_Disconnect_Server
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid    :    connnection identifier
 ************************************
 * \return  SUCCESS
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Disconnect_Server(int connid)
{
    dbi_Trans_Commit(connid);

    logfile_num = 0;
    commit_count = 0;

    return DB_SUCCESS;
}

#include "mdb_dbi_trans.c"

#include "mdb_dbi_direct.c"

#include "mdb_dbi_table.c"

#include "mdb_dbi_index.c"

#include "mdb_dbi_schema.c"

/*****************************************************************************/
//! dbi_ViewDefinition_Create
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid        :
 * \param view_name        :
 * \param definition    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_ViewDefinition_Create(int connid, char *view_name, char *definition)
{
    int *pTransid;
    struct Container *cont = NULL;

    char *sysview;
    struct Trans *trans;
    OID record_id;
    int cursor, recsize;
    int ret;
    int lock_mode = LK_NOLOCK;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (view_name == NULL || view_name[0] == '\0' || definition == NULL)
        return DB_E_INVALIDPARAM;
#endif

    /* sysviews에 insert할 record 구성 */
    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    cont = Cont_Search(view_name);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }

    recsize = sizeof(int) + MAX_SYSVIEW_DEF + 40;

    sysview = (char *) PMEM_ALLOC(recsize);

    if (sysview == NULL)
    {
        ret = DB_E_OUTOFMEMORY;
        goto end;
    }

    sc_memset(sysview, 0, recsize);

    sc_memcpy(sysview, (char *) &cont->base_cont_.id_, INT_SIZE);
    sc_strcpy(sysview + INT_SIZE, definition);

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }

    /* sysviews에 insert */
    ret = LockMgr_Lock("sysviews", *pTransid, LOWPRIO, LK_EXCLUSIVE, WAIT);

    if (ret < 0)
    {
        PMEM_FREENUL(sysview);
        goto end;
    }

    lock_mode = LockMgr_Lock("systables", *pTransid, LOWPRIO, LK_EXCLUSIVE,
            WAIT);

    trans->fddl = 1;
    trans->fReadOnly = 0;
    trans->fLogging = 1;

    cursor = Open_Cursor(*pTransid, "sysviews", LK_EXCLUSIVE, -1,
            NULL, NULL, NULL);
    if (cursor < 0)
    {
        PMEM_FREENUL(sysview);
        ret = cursor;
        goto end_unlock;
    }

    ret = __Insert(cursor, (char *) sysview, recsize, &record_id, 0);

    PMEM_FREENUL(sysview);

    Close_Cursor(cursor);

  end_unlock:
    if (lock_mode != LK_NOLOCK)
    {
        struct Container *Cont = Cont_Search("systables");

        if (Cont != NULL)
        {
            LockMgr_Table_Unlock(*pTransid, Cont->base_cont_.id_);
        }

        Cont = Cont_Search("sysviews");
        if (Cont != NULL)
        {
            LockMgr_Table_Unlock(*pTransid, Cont->base_cont_.id_);
        }
    }

  end:

    return ret;
}

/*****************************************************************************/
//! dbi_ViewDefinition_Drop
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid:
 * \param view_name:
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_ViewDefinition_Drop(int connid, char *view_name)
{
    int *pTransid;
    struct Container *cont;
    struct KeyValue minkey, maxkey;
    int cursor;
    DB_UINT32 recsize;
    int ret;
    FIELDVALUE_T fieldvalue;

    struct Trans *trans;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (view_name == NULL || view_name[0] == '\0')
        return DB_E_INVALIDPARAM;
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    cont = Cont_Search(view_name);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }

    ret = dbi_FieldValue(MDB_NN, DT_INTEGER, 0,
            mdb_offsetof(SYSVIEW_T, tableId), sizeof(int), 0,
            MDB_COL_NUMERIC, &cont->base_cont_.id_, DT_INTEGER, sizeof(int),
            &fieldvalue);
    if (ret != DB_SUCCESS)
        goto end;
    dbi_KeyValue(1, &fieldvalue, &minkey);
    dbi_KeyValue(1, &fieldvalue, &maxkey);

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }

    ret = LockMgr_Lock("sysviews", *pTransid, LOWPRIO, LK_EXCLUSIVE, WAIT);
    if (ret < 0)
    {
        ret = ret;
        goto end;
    }

    cursor = Open_Cursor(*pTransid, "sysviews", LK_EXCLUSIVE, 0
            /*$pk.sysviews */ , &minkey, &maxkey, NULL);
    if (cursor < 0)
    {
        ret = cursor;
        goto end;
    }

    ret = __Read(cursor, NULL, NULL, 0, &recsize, 0);

    trans->fddl = 1;
    trans->fReadOnly = 0;
    trans->fLogging = 1;

    if (ret == DB_SUCCESS)
    {
        ret = __Remove(cursor);
    }

    Close_Cursor(cursor);

  end:
    return ret;
}

/*****************************************************************************/
//! dbi_Schema_GetViewDefinition
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid    :
 * \param view_name    :
 * \param ppSysView    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Schema_GetViewDefinition(int connid, char *view_name,
        SYSVIEW_T ** ppSysView)
{
    int *pTransid;
    struct Container *cont;
    struct KeyValue minkey, maxkey;
    int cursor;
    DB_UINT32 recsize;
    int ret;
    FIELDVALUE_T fieldvalue;

    char *sysdef;

#ifdef MDB_DEBUG
    DBI_CHECK_SERVER_TERM();
#endif

#ifdef CHECK_DBI_PARAM
    if (view_name == NULL || view_name[0] == '\0' || ppSysView == NULL)
        return DB_E_INVALIDPARAM;
#endif

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (*pTransid == -1)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    cont = Cont_Search(view_name);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }

    ret = dbi_FieldValue(MDB_NN, DT_INTEGER, 0,
            mdb_offsetof(SYSVIEW_T, tableId), sizeof(int), 0,
            MDB_COL_NUMERIC, &cont->base_cont_.id_, DT_INTEGER,
            sizeof(int), &fieldvalue);
    if (ret != DB_SUCCESS)
        goto end;
    dbi_KeyValue(1, &fieldvalue, &minkey);
    dbi_KeyValue(1, &fieldvalue, &maxkey);

    ret = LockMgr_Lock("sysviews", *pTransid, LOWPRIO, LK_SHARED, WAIT);
    if (ret < 0)
        goto end;

    cursor = Open_Cursor(*pTransid, "sysviews", LK_SHARED,
            0 /*$pk.sysviews */ ,
            &minkey, &maxkey, NULL);
    if (cursor < 0)
    {
        ret = cursor;
        goto end;
    }

    Read_Itemsize(cursor, &recsize);

    sysdef = PMEM_ALLOC(recsize);

    if (sysdef == NULL)
    {
        Close_Cursor(cursor);
        ret = DB_E_OUTOFMEMORY;
        goto end;
    }

    ret = __Read(cursor, sysdef, NULL, 0, &recsize, 0);

    if (ret < 0)
    {
        PMEM_FREENUL(sysdef);
    }
    else
    {
        *ppSysView = (SYSVIEW_T *) sysdef;
    }

    Close_Cursor(cursor);

  end:

    return ret;
}

#include "mdb_dbi_space.c"

/*****************************************************************************/
//! dbi_Set_SysStatus
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param
 * int connid  -- connection identifier
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
typedef struct sysstatus
{
    char    version[32];
    int     procid;
    int     bits;
    char    db_name[16];
    char    db_path[64];
    int     db_dsize;
    int     db_isize;
    int     num_connects;
    int     num_trans;
} SYSSTATUS_T;
 *****************************************************************************/
void
dbi_Set_SysStatus(int connid)
{
    SYSSTATUS_T *sysstatus = NULL;
    static OID sysstatus_oid = NULL_OID;

    if (sysstatus_oid == NULL_OID)
    {
        struct Container *Cont = Cont_Search("sysstatus");

        if (Cont == NULL)
            goto end;

        sysstatus_oid = Col_GetFirstDataID_Page(Cont, NULL_PID, 0);

        if (sysstatus_oid == NULL_OID)
        {
            MDB_SYSLOG(("FAIL: sysstatus records not existed\n"));
            goto end;
        }

        sysstatus = (SYSSTATUS_T *) OID_Point(sysstatus_oid);
        if (sysstatus == NULL)
        {
            goto end;
        }

        sc_memset(sysstatus, 0, sizeof(SYSSTATUS_T));

        sc_strncpy(sysstatus->version, _DBVERSION_, 31);

        sysstatus->version[31] = '\0';
        sysstatus->procid = sc_getpid();
        sysstatus->bits = sizeof(long) * 8;
        sc_strncpy(sysstatus->db_name, server->db_name_, 15);
        sysstatus->db_name[15] = '\0';

        // 아래에서 에러나는 부분
        sc_snprintf(sysstatus->db_path, 63, "%s%s%s",
                sc_getenv("MOBILE_LITE_CONFIG"), DIR_DELIM, server->db_name_);

        sysstatus->db_path[63] = '\0';
    }

    sysstatus = (SYSSTATUS_T *) OID_Point(sysstatus_oid);
    if (sysstatus == NULL)
    {
        goto end;
    }

  end:
    return;
}

void
dbi_DataMem_Protect_RO(void)
{
#if defined(_AS_WINCE_) || defined(WIN32_MOBILE)
    int sn;
    DWORD dwOldProtect;

    if (fcreatedb == 1)
        return;

    if (server == NULL)
        return;

    if (server->shparams.db_write_protection == 0)
        return;

    for (sn = 0; sn < SEGMENT_NO; sn++)
    {
        segment_[sn] = DBDataMem_V2P(Mem_mgr->segment_memid[sn]);

        if (segment_[sn] == NULL)
            continue;

        VirtualProtect(segment_[sn], SEGMENT_SIZE, PAGE_READONLY,
                &dwOldProtect);
    }
#endif

    return;
}

void
dbi_DataMem_Protect_RW(void)
{
#if defined(_AS_WINCE_) || defined(WIN32_MOBILE)
    int sn;
    DWORD dwOldProtect;

    if (fcreatedb == 1)
        return;

    if (server == NULL)
        return;

    if (server->shparams.db_write_protection == 0)
        return;

    for (sn = 0; sn < SEGMENT_NO; sn++)
    {
        segment_[sn] = DBDataMem_V2P(Mem_mgr->segment_memid[sn]);

        if (segment_[sn] == NULL)
            continue;

        if (!VirtualProtect
                (segment_[sn], SEGMENT_SIZE, PAGE_READWRITE, &dwOldProtect))
        {
            MDB_SYSLOG(("FAIL: protect %d\n", GetLastError()));
            break;
        }
    }
#endif

    return;
}

/* End */

#include "mdb_dbi_seq.c"
#include "mdb_dbi_str.c"

int
dbi_Rid_Tablename(int connid, OID rid, char *tablename)
{
    struct Container *cont;

    DBI_CHECK_SERVER_TERM();

    cont = Cont_get(rid);

    if (cont == NULL)
        return DB_E_PIDPOINTER;

    /*
       if (is_systable_systables(cont->base_cont_.id_))
       return DB_FAIL;
       else
     */
    sc_strcpy(tablename, cont->base_cont_.name_);

    return DB_SUCCESS;
}

int
dbi_Rid_Drop(int connid, OID rid)
{
    struct Container *cont;
    int ret = DB_SUCCESS;

    char *pbuf = NULL;

    cont = Cont_get(rid);
    if (cont == NULL)
    {
        ret = DB_E_PIDPOINTER;
        goto end;
    }

    if (is_systable_systables(cont->base_cont_.id_))
    {
        int tableid;
        int issys;

        SetBufSegment(issys, rid);

        pbuf = (char *) OID_Point(rid);
        if (pbuf == NULL)
        {
            UnsetBufSegment(issys);

            ret = DB_E_OIDPOINTER;
            goto end;
        }

        sc_memcpy(&tableid, pbuf, INT_SIZE);

        UnsetBufSegment(issys);

        cont = Cont_Search_Tableid(tableid);
        if (cont == NULL)
        {
            ret = DB_FAIL;
            goto end;
        }
#ifdef MDB_DEBUG
        __SYSLOG("%s(%ld) drop.\n", cont->base_cont_.name_, rid);
#endif
        ret = dbi_Relation_Drop(connid, cont->base_cont_.name_);
        goto end;
    }
    else
    {
#ifdef MDB_DEBUG
        __SYSLOG("record(%ld) of %s delete.\n", rid, cont->base_cont_.name_);
#endif
        ret = dbi_Direct_Remove(connid, cont->base_cont_.name_, NULL,
                NULL, rid, LK_EXCLUSIVE);
        goto end;
    }

  end:
    return ret;
}

extern int
Update_memory_fields(struct Container *cont, SYSFIELD_T * fields_info,
        char *new_record, struct UpdateValue *newValues);

/*****************************************************************************/
//! dbi_Rid_Update

/*! \breif  RID에 해당하는 record를 newValue로 변경해버린다
 ************************************
 * \param connid(in)    :
 * \param rid(in)       :
 * \param newValue(in)  : 실제 변경하려고 하는 값
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_Rid_Update(int connid, OID rid, struct UpdateValue *newValues)
{
    char *record = NULL;
    char rec_buf[DEFAULT_PACKET_SIZE];
    DB_UINT32 recleng;
    int ret;
    LOCKMODE lock = LK_EXCLUSIVE;
    struct Container *cont;
    int issys_cont = 0;

    cont = Cont_get(rid);
    if (cont == NULL)
    {
        ret = DB_E_PIDPOINTER;
        goto end;
    }

    SetBufSegment(issys_cont, cont->collect_.cont_id_);

    if (is_systable_systables(cont->base_cont_.id_))
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_NOTPERMITTED, 0);
        ret = DB_E_NOTPERMITTED;
        goto end;
    }

    /* dbi_Direct_Read */
    /* apply newValue */
    /* dbi_Direct_Update(); */

    while (1)
    {
        if (record == NULL)
        {
            record = rec_buf;
            recleng = DEFAULT_PACKET_SIZE;
        }
        ret = dbi_Direct_Read(connid, cont->base_cont_.name_, NULL, NULL, rid,
                LK_SHARED, record, &recleng, NULL);
        if (ret == DB_E_TOOSMALLRECBUF)
        {
            record = PMEM_ALLOC(recleng);
            if (record != NULL)
                continue;

            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_OUTOFMEMORY, 0);
            ret = DB_E_OUTOFMEMORY;
        }
        break;
    }

    if (ret >= 0)
        ret = Update_memory_fields(cont, NULL, record, newValues);

    if (ret >= 0)
        ret = dbi_Direct_Update(connid, cont->base_cont_.name_, NULL, NULL,
                rid, record, recleng, lock, newValues);

    if (record && record != rec_buf)
        PMEM_FREENUL(record);

    if (ret < 0)
        goto end;

    ret = DB_SUCCESS;

  end:
    UnsetBufSegment(issys_cont);
    return ret;
}

int
dbi_Table_Size(int connid, char *tablename)
{
    int *pTransid;
    struct Container *cont;
    int ret = 0;
    int i;
    int curr_transid = 0;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (tablename == NULL || tablename[0] == '\0')
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

    cont = Cont_Search(tablename);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }

    if (cont->collect_.item_count_ == 0)
    {
        ret = 0;
        goto end;
    }

    ret = cont->collect_.item_count_ * cont->collect_.slot_size_;

    for (i = 0; i < VAR_COLLECT_CNT; i++)
    {
        if (cont->var_collects_[i].record_size_ > 0)
        {
            ret += (cont->var_collects_[i].item_count_ *
                    cont->var_collects_[i].slot_size_);
        }
    }

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

extern int TTree_Get_NodeCount(struct TTree *ttree);

int
dbi_Index_Size(int connid, char *tablename)
{
    int *pTransid;
    struct Container *cont;
    int ret = 0;
    OID index_id;
    struct TTree *ttree;
    int curr_transid = 0;
    int issys_ttree;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (tablename == NULL || tablename[0] == '\0')
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

    cont = Cont_Search(tablename);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }

    if (cont->collect_.item_count_ == 0)
    {
        ret = 0;
        goto end;
    }

    ret = 0;
    index_id = cont->base_cont_.index_id_;
    while (index_id)
    {
        ttree = (struct TTree *) OID_Point(index_id);
        if (ttree == NULL)
        {
            ret = DB_E_OIDPOINTER;
            goto end;
        }

        SetBufSegment(issys_ttree, index_id);

        ret += (TTree_Get_NodeCount(ttree) * sizeof(struct TTreeNode));

        UnsetBufSegment(issys_ttree);

        index_id = ttree->base_cont_.index_id_;
    }

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

/* return the number of loaded segments since startup */
int
dbi_Get_numLoadSegments(int connid, void *buf)
{
    if (buf == NULL)
        return DB_FAIL;

    *(int *) buf = server->numLoadSegments;

    return DB_SUCCESS;
}

int
dbi_Sort_rid(OID * pszOids, int nMaxCnt, DB_BOOL IsAsc)
{
    register int i, j, h, k;
    OID oid;

    if (!pszOids)
        return -1;

    if (nMaxCnt <= 0)
        return -2;

    for (h = 1; h < nMaxCnt; h = h * 3 + 1)
        ;

    if (IsAsc)
    {
        for (h = h / 3; h > 0; h = h / 3)
        {
            for (i = 0; i < h; i++)
            {
                for (j = i + h; j < nMaxCnt; j = j + h)
                {
                    oid = pszOids[j];
                    k = j;

                    while ((k > (h - 1)) && (pszOids[k - h] > oid))
                    {
                        pszOids[k] = pszOids[k - h];
                        k = k - h;
                    }   // while

                    pszOids[k] = oid;
                }       // for (j)
            }   // for (i)
        }   // for (h)
    }
    else
    {
        for (h = h / 3; h > 0; h = h / 3)
        {
            for (i = 0; i < h; i++)
            {
                for (j = i + h; j < nMaxCnt; j = j + h)
                {
                    oid = pszOids[j];
                    k = j;

                    while ((k > (h - 1)) && (pszOids[k - h] < oid))
                    {
                        pszOids[k] = pszOids[k - h];
                        k = k - h;
                    }   // while

                    pszOids[k] = oid;
                }       // for (j)
            }   // for (i)
        }   // for (h)
    }

    return 0;
}

/*****************************************************************************/
//! dbi_Table_Export
/*! \breif  특정 table의 내용을 export_file로 export 시킴
 ************************************
 * \param connid(in)        : connection identifier
 * \param export_file(in)   : exported file
 * \param table_name(in)    : table's name
 * \param f_append(in)      : 1 means the append, 0 means that create new
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - A. iSQL_table_export()의 함수를 dbi 단으로 옮김
 *      B. SQL 문장 지원
 *  - added instant transaction begin/abort/commit mechanism
 *****************************************************************************/
int
dbi_Table_Export(int connid, char *export_file, char *table_name, int f_append)
{
    T_EXPORT_FILE_HEADER header;
    SYSTABLE_T systable;
    int ret;
    int fd;
    int cursor_id;
    int offset;
    int record_len;
    char *record_buf = NULL;
    int record_cnt = 0;
    int *pTransid;
    int curr_transid = 0;
    struct Trans *trans;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (table_name == NULL || table_name[0] == '\0' ||
            export_file == NULL || export_file[0] == '\0')
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
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }

    /* interrupt checking */
    if (trans->interrupt)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_TRANSINTERRUPT, 0);
        trans->interrupt = FALSE;
        ret = DB_E_TRANSINTERRUPT;
        goto end;
    }

    ret = dbi_Schema_GetTableInfo(connid, table_name, &systable);
    if (ret < 0)
    {
        goto end;
    }

    if (f_append == 1)
        fd = sc_open(export_file, O_WRONLY | O_CREAT | O_BINARY, OPEN_MODE);
    else
        fd = sc_open(export_file, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY,
                OPEN_MODE);

    if (fd < 0)
    {
        ret = DB_E_OPENFILE;
        goto end;
    }

    if (f_append == 1)
        offset = sc_lseek(fd, 0, SEEK_END);
    else
        offset = 0;

    sc_lseek(fd, offset + sizeof(T_EXPORT_FILE_HEADER), SEEK_SET);

    cursor_id = dbi_Cursor_Open(connid, table_name,
            NULL, NULL, NULL, NULL, LK_SHARED, 0);
    if (cursor_id < 0)
    {
        sc_close(fd);
        ret = cursor_id;
        goto end;
    }

    ret = dbi_Cursor_Read(connid, cursor_id, NULL, &record_len, NULL, 0,
            NULL_OID);
    if (ret < 0)
    {
        dbi_Cursor_Close(connid, cursor_id);
        sc_close(fd);
        goto end;
    }

    record_buf = PMEM_ALLOC(record_len + 1);
    if (record_buf == NULL)
    {
        dbi_Cursor_Close(connid, cursor_id);
        sc_close(fd);
        ret = DB_E_OUTOFMEMORY;
        goto end;
    }

    while (1)
    {
        ret = dbi_Cursor_Read(connid, cursor_id, record_buf, &record_len, NULL,
                1, NULL_OID);
        if (ret < 0)
        {
            if (ret == DB_E_NOMORERECORDS)
                ret = 0;
            break;      // error가 존재하는 경우 이를 처리해주기 위해서
        }
        ++record_cnt;

        ret = sc_write(fd, record_buf, record_len);
        if (ret != record_len)
        {
            ret = DB_E_WRITEFILE;
            break;
        }
    }

    dbi_Cursor_Close(connid, cursor_id);

    // write the header's info
    header._temp[0] = '%';
    header._temp[1] = '%';
    header._temp[2] = '%';
    header._temp[3] = '%';
    sc_strncpy(header.tablename, table_name, REL_NAME_LENG);
    header.rec_len = record_len;
    header.rec_cnt = record_cnt;

    sc_lseek(fd, offset, SEEK_SET);
    sc_write(fd, &header, sizeof(T_EXPORT_FILE_HEADER));
    sc_close(fd);

    PMEM_FREENUL(record_buf);

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

int
dbi_Table_Export_Schema(int connid, char *export_file, char *table_name,
        int f_append, int isAll)
{
    SYSTABLE_T systable;
    T_RESULT_SHOW res_show;
    int ret;
    int fd = -1;
    int offset;
    int *pTransid;
    int curr_transid = 0;
    struct Trans *trans;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (table_name == NULL || table_name[0] == '\0' ||
            export_file == NULL || export_file[0] == '\0')
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
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }

    /* interrupt checking */
    if (trans->interrupt)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_TRANSINTERRUPT, 0);
        trans->interrupt = FALSE;
        ret = DB_E_TRANSINTERRUPT;
        goto end;
    }

    ret = dbi_Schema_GetTableInfo(connid, table_name, &systable);
    if (ret < 0)
    {
        goto end;
    }

    if (f_append == 1)
        fd = sc_open(export_file, O_WRONLY | O_CREAT | O_BINARY, OPEN_MODE);
    else
        fd = sc_open(export_file, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY,
                OPEN_MODE);

    if (fd < 0)
    {
        ret = DB_E_OPENFILE;
        goto end;
    }

    if (f_append == 1)
        offset = sc_lseek(fd, 0, SEEK_END);
    else
        offset = 0;

    sc_lseek(fd, offset, SEEK_SET);

    sc_memset(&res_show, 0, sizeof(T_RESULT_SHOW));
    res_show.fd = fd;

    ret = dbi_Relation_Show(connid, table_name, &res_show, isAll);

  end:
    if (curr_transid == -1)
    {
        if (ret < 0)
            dbi_Trans_Abort(connid);
        else
            dbi_Trans_Commit(connid);
    }

    if (fd >= 0)
    {
        sc_close(fd);
        fd = -1;
    }

  end_return:
    return ret;
}

/*****************************************************************************/
//! dbi_Table_Export
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid(in)        : connection identifier
 * \param import_file(in)   : exported file
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - A. iSQL_table_export()의 함수를 dbi 단으로 옮김
 *      B. SQL 문장 지원
 *  - added instant transaction begin/abort/commit mechanism
 *****************************************************************************/
int
dbi_Table_Import(int connid, char *import_file)
{
    T_EXPORT_FILE_HEADER header;
    int fd = -1;
    int ret;
    int cursor_id;
    int record_cnt;
    char *record_buf;
    OID inserted_rid;
    int *pTransid;
    int curr_transid = 0;
    struct Trans *trans;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (import_file == NULL || import_file[0] == '\0')
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

    // get transaction information
    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_UNKNOWNTRANSID;
        goto end;
    }

    /* interrupt checking */
    if (trans->interrupt)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_TRANSINTERRUPT, 0);
        trans->interrupt = FALSE;
        ret = DB_E_TRANSINTERRUPT;
        goto end;
    }

    fd = sc_open(import_file, O_RDONLY | O_BINARY, OPEN_MODE);
    if (fd < 0)
    {
        ret = DB_E_OPENFILE;
        goto end;
    }

    /* file main loop */
    while (1)
    {
        ret = sc_read(fd, &header, sizeof(T_EXPORT_FILE_HEADER));
        if (ret != sizeof(T_EXPORT_FILE_HEADER))
        {
            if (ret != 0)
                ret = DB_E_READFILE;
            break;
        }

        if (sc_memcmp(header._temp, "%%%%%%%%", 4) != 0)
        {
            sc_close(fd);
            fd = -1;
            ret = DB_E_INVALIDFILE;
            goto end;
        }

        record_cnt = header.rec_cnt;
        if (record_cnt == 0)
            continue;

        record_buf = PMEM_ALLOC(header.rec_len);
        if (record_buf == NULL)
        {
            sc_close(fd);
            fd = -1;
            ret = DB_E_OUTOFMEMORY;
            goto end;
        }

        cursor_id = dbi_Cursor_Open(connid, header.tablename,
                NULL, NULL, NULL, NULL, LK_EXCLUSIVE, 0);
        if (cursor_id < 0)
        {
            PMEM_FREENUL(record_buf);
            sc_close(fd);
            fd = -1;
            ret = cursor_id;
            goto end;
        }

        while (record_cnt)
        {
            ret = sc_read(fd, record_buf, header.rec_len);
            if (ret != header.rec_len)
            {
                dbi_Cursor_Close(connid, cursor_id);
                PMEM_FREENUL(record_buf);
                sc_close(fd);
                fd = -1;
                ret = DB_E_READFILE;
                goto end;
            }


            /* INSERTED_RID */
            ret = dbi_Cursor_Insert(connid, cursor_id, record_buf,
                    header.rec_len, 0, &inserted_rid);

            if (ret < 0)
            {
                dbi_Cursor_Close(connid, cursor_id);
                PMEM_FREENUL(record_buf);
                sc_close(fd);
                fd = -1;
                ret = ret;
                goto end;

            }

            --record_cnt;
        }

        dbi_Cursor_Close(connid, cursor_id);
        PMEM_FREENUL(record_buf);
    }

    // under instant transaction mode, commit if everything O.K. or rollback
  end:
    if (curr_transid == -1)
    {
        if (ret < 0)
            dbi_Trans_Abort(connid);
        else
            dbi_Trans_Commit(connid);
    }

    if (fd >= 0)
    {
        sc_close(fd);
        fd = -1;
    }

  end_return:
    return ret;
}

extern __DECL_PREFIX char SERVERLOG_PATH[];
void
dbi_GetServerLogPath(char *pSrvPath)
{
    pSrvPath[0] = 0x00;

    if (SERVERLOG_PATH[0] == '\0')
    {
#if defined(SERVERLOG_PATH_ON_SYSTEMROOT)
        sc_strcpy(pSrvPath, sc_getenv("SystemRoot"));
#else
        sc_strcpy(pSrvPath, sc_getenv("MOBILE_LITE_CONFIG"));
#endif
    }
    else
        sc_strcpy(pSrvPath, SERVERLOG_PATH);
}

int
dbi_Relation_Logging(int connid, char *relation_name,
        int logging_on, int during_runtime, int userid)
{
    int *pTransid;
    int curr_transid = 0;
    struct Trans *trans;
    struct Container *cont = NULL;
    int ret, propertyChanged = 0;
    MDB_UINT16 cont_type = _CONT_TABLE;
    int issys_cont = 0;

    /* check parameters */
    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (relation_name == NULL)
        return DB_E_INVALIDPARAM;

    ret = sc_strlen(relation_name);
    if (ret == 0 || ret >= REL_NAME_LENG)
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

    cont = Cont_Search(relation_name);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }

    SetBufSegment(issys_cont, cont->collect_.cont_id_);

    if (isSysTable(cont) || isSystemTable(cont) || isTempTable(cont) ||
            (userid == _USER_USER && !doesUserCreateTable(cont, userid)))
    {
        ret = DB_E_NOTPERMITTED;
        goto end;
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    trans->fReadOnly = 0;
    trans->fLogging = 1;

    /* hold schema execlusive lock on table */
    ret = LockMgr_Lock(relation_name, *pTransid, LOWPRIO, LK_SCHEMA_EXCLUSIVE,
            WAIT);
    if (ret < 0)
    {
        goto end;
    }

    if (logging_on)
    {
        cont_type = _CONT_TABLE;
        if (isNoLogging(cont))
        {
            propertyChanged = 1;
        }
    }
    else
    {
        cont_type = _CONT_NOLOGGINGTABLE;
        if (!isNoLogging(cont))
        {
            propertyChanged = 1;
        }
    }
    if (!during_runtime
            && cont->base_cont_.runtime_logging_ != _RUNTIME_LOGGING_NONE)
    {
        propertyChanged = 1;
    }

    if (propertyChanged || during_runtime)
    {
        ret = Cont_ChangeProperty(cont, cont_type, during_runtime);
        if (ret < 0)
            goto end;

        if (!during_runtime)
            ret = _Schema_ChangeTableProperty(*pTransid, relation_name,
                    cont_type);

        if (ret < 0)
        {
            goto end;
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

char *
dbi_get_collation(int ctype)
{
    static char col_type[32] = " COLLATION ";

    col_type[0] = ' ';

    switch (ctype)
    {
    case 2:
        sc_sprintf(col_type + 11, "char_ci");
        break;
    case 3:
        sc_sprintf(col_type + 11, "char_cs");
        break;
    case 4:
        sc_sprintf(col_type + 11, "char_dic");
        break;
    case 6:
        sc_sprintf(col_type + 11, "nchar_cs");
        break;
    case 8:
        sc_sprintf(col_type + 11, "user1");
        break;
    case 9:
        sc_sprintf(col_type + 11, "user2");
        break;
    case 10:
        sc_sprintf(col_type + 11, "user3");
        break;
    case 11:
        sc_sprintf(col_type + 11, "char_reverse");
        break;
    default:
        col_type[0] = '\0';
    }

    return col_type;
}

static int
print_show_result(T_RESULT_SHOW * res_show, char *str, int pos,
        DB_BOOL is_tbl_start)
{
    char *rec_buf = NULL;
    int ret;

    if (res_show->fd >= 0)
    {
        ret = sc_write(res_show->fd, str, pos);
        if (ret != pos)
        {
            return DB_E_WRITEFILE;
        }
        return 0;
    }

    if (res_show->rec_pos + pos > res_show->max_rec_len || is_tbl_start)
    {
        if (res_show->max_rec_len)
        {
            res_show->rec_pos = res_show->max_rec_len;
        }

        res_show->max_rec_len += MAX_SHOW_RECSIZE;

        rec_buf = (char *) PMEM_ALLOC(res_show->max_rec_len);
        if (rec_buf == NULL)
        {
            return SQL_E_OUTOFMEMORY;
        }

        sc_memset(rec_buf, 0, res_show->max_rec_len);
        sc_memcpy(rec_buf, res_show->rec_buf, res_show->rec_pos);

        PMEM_FREENUL(res_show->rec_buf);
        res_show->rec_buf = rec_buf;
    }

    sc_memcpy((res_show->rec_buf + res_show->rec_pos), str, pos);

    res_show->rec_pos += pos;

    return 0;
}

int
dbi_Relation_Show(int connid, char *tablename, T_RESULT_SHOW * res_show,
        int isAll)
{
    SYSTABLE_T table_info;
    SYSFIELD_T *field_info = NULL;
    SYSINDEX_T *index_info = NULL;
    SYSVIEW_T *view_info = NULL;
    struct Container *cont;
    T_VALUEDESC str, res;

    char *buf;
    int pos, i, j, k;
    int numFields, numIndexes;

    /* unused int check_varbyte = 0; */
    int check_pri = 0;
    int ret = -1;
    int issys_cont = 0;

    ret = _Schema_GetTableInfo(tablename, &table_info);
    if (ret < 0)
    {
        goto end_return;
    }

    cont = Cont_Search(table_info.tableName);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end_return;
    }

    SetBufSegment(issys_cont, cont->collect_.cont_id_);

    buf = (char *) PMEM_ALLOC(MAX_SHOW_RECSIZE);
    if (buf == NULL)
    {
        ret = DB_E_OUTOFMEMORY;
        goto end_return;
    }

    if (isNoLogging(cont))
    {
        pos = sc_sprintf(buf, "\nCREATE NOLOGGING TABLE %s (", tablename);
    }
    else if (ismSyncTable(cont))
    {
        pos = sc_sprintf(buf, "\nCREATE MSYNC %s (", tablename);
    }
    else if (isTable(cont))
    {
        pos = sc_sprintf(buf, "\nCREATE TABLE %s (", tablename);
    }
    else if (isViewTable(cont))
    {
        ret = dbi_Schema_GetViewDefinition(connid, tablename, &view_info);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDVIEW, 0);
            goto RETURN_LABEL;
        }

        pos = sc_sprintf(buf, "\nCREATE VIEW %s AS\n%s", tablename,
                view_info->definition);
    }
    else
        goto RETURN_LABEL;

    pos = print_show_result(res_show, buf, pos, DB_TRUE);
    if (pos < 0)
    {
        ret = pos;
        goto RETURN_LABEL;
    }

    if (isViewTable(cont))
    {
        ret = pos;
        goto RETURN_LABEL;
    }

    field_info =
            (SYSFIELD_T *) PMEM_ALLOC(sizeof(SYSFIELD_T) *
            table_info.numFields);
    if (field_info == NULL)
    {
        ret = DB_E_OUTOFMEMORY;
        goto RETURN_LABEL;
    }

    numFields = _Schema_GetFieldsInfo(tablename, field_info);
    if (numFields < 1)
    {
        ret = numFields;
        goto RETURN_LABEL;
    }

    for (i = 0; i < numFields; i++)
    {
        pos += sc_sprintf(buf + pos, "\n%s ", field_info[i].fieldName);
        strcpy_datatype(buf + pos, MAX_SHOW_RECSIZE - pos, &field_info[i]);
        pos += sc_strlen(buf + pos);
        pos += sc_sprintf(buf + pos, "%s",
                dbi_get_collation(field_info[i].collation));

        if (!DB_DEFAULT_ISNULL(field_info[i].defaultValue))
        {
            switch (field_info[i].type)
            {
            case DT_VARCHAR:
            case DT_NVARCHAR:
            case DT_CHAR:
            case DT_NCHAR:
                pos += sc_sprintf(buf + pos, " DEFAULT \'%s\'",
                        field_info[i].defaultValue);
                break;
            case DT_DATETIME:
            case DT_TIMESTAMP:
            case DT_DATE:
            case DT_TIME:
                if (!mdb_strcmp(field_info[i].defaultValue, "sysdate"))
                {
                    pos += sc_sprintf(buf + pos, " DEFAULT SYSDATE");
                }
                else if (!mdb_strcmp(field_info[i].defaultValue,
                                "current_timestamp"))
                {
                    pos += sc_sprintf(buf + pos, " DEFAULT CURRENT_TIMESTAMP");
                }
                else if (!mdb_strcmp(field_info[i].defaultValue,
                                "current_date"))
                {
                    pos += sc_sprintf(buf + pos, " DEFAULT CURRENT_DATE");
                }
                else if (!mdb_strcmp(field_info[i].defaultValue,
                                "current_time"))
                {
                    pos += sc_sprintf(buf + pos, " DEFAULT CURRENT_TIME");
                }
                else
                {
                    pos += sc_sprintf(buf + pos, " DEFAULT \'%s\'",
                            field_info[i].defaultValue);
                }
                break;
            case DT_BYTE:
            case DT_VARBYTE:
                sc_memset(&res, 0, sizeof(T_VALUEDESC));
                sc_memset(&str, 0, sizeof(T_VALUEDESC));
                ret = get_defaultvalue(&str.attrdesc, &field_info[i]);
                if (ret != RET_SUCCESS)
                {
                    goto RETURN_LABEL;
                }
                str.u = str.attrdesc.defvalue.u;
                str.valuetype = field_info[i].type;
                str.attrdesc.len = field_info[i].length;
                str.value_len = str.attrdesc.defvalue.value_len;

                ret = calc_func_hexbinstring(SFT_HEXASTRING, &str, &res, 0);
                if (ret != RET_SUCCESS)
                {
                    sql_value_ptrFree(&res);
                    PMEM_FREENUL(str.u.ptr);
                    goto RETURN_LABEL;
                }

                pos += sc_sprintf(buf + pos, " DEFAULT x\'%s\'", res.u.ptr);

                sql_value_ptrFree(&res);
                PMEM_FREENUL(str.u.ptr);
                break;
            default:
                pos += sc_sprintf(buf + pos, " DEFAULT %s",
                        field_info[i].defaultValue);
                break;
            }
        }

        if (field_info[i].flag & AUTO_BIT)
        {
            pos += sc_sprintf(buf + pos, "%s", " PRIMARY KEY AUTOINCREMENT");
        }
        else if (field_info[i].flag & PRI_KEY_BIT)
        {
            check_pri = 1;
        }
        else
        {
            if (!(field_info[i].flag & NULL_BIT))
            {
                pos += sc_sprintf(buf + pos, " NOT NULL");
            }

            if (field_info[i].flag & NOLOGGING_BIT)
            {
                pos += sc_sprintf(buf + pos, " NOLOGGING");
            }
        }

        if (i < numFields - 1 || check_pri)
        {
            pos += sc_sprintf(buf + pos, ",");

            pos = print_show_result(res_show, buf, pos, DB_FALSE);
            if (pos < 0)
            {
                ret = pos;
                goto RETURN_LABEL;
            }
        }
    }

    numIndexes = cont->base_cont_.index_count_;
    if (numIndexes > 0)
    {
        index_info =
                (SYSINDEX_T *) PMEM_ALLOC(sizeof(SYSINDEX_T) * numIndexes + 1);
        if (index_info == NULL)
        {
            ret = DB_E_OUTOFMEMORY;
            goto RETURN_LABEL;
        }

        numIndexes =
                _Schema_GetTableIndexesInfo(tablename, SCHEMA_REGU_INDEX,
                index_info);

        for (i = 0; i < numIndexes; i++)
        {
            if (!mdb_strncmp(index_info[i].indexName, PRIMARY_KEY_PREFIX,
                            sc_strlen(PRIMARY_KEY_PREFIX)))
            {
                break;
            }
        }

        if (i < numIndexes && check_pri)
        {
            SYSINDEXFIELD_T *indexfield_info;

            pos += sc_sprintf(buf + pos, "\nPRIMARY KEY(");

            indexfield_info =
                    (SYSINDEXFIELD_T *) PMEM_ALLOC(sizeof(SYSINDEXFIELD_T) *
                    index_info[i].numFields);
            if (indexfield_info == NULL)
            {
                ret = DB_E_OUTOFMEMORY;
                goto RETURN_LABEL;
            }

            ret = _Schema_GetIndexInfo(index_info[i].indexName, &index_info[i],
                    indexfield_info, NULL);
            if (ret < 0)
            {
                PMEM_FREENUL(indexfield_info);
                goto RETURN_LABEL;
            }

            for (j = 0; j < index_info[i].numFields; j++)
            {
                for (k = 0; k < numFields; k++)
                {
                    if (indexfield_info[j].fieldId == field_info[k].fieldId)
                    {
                        pos += sc_sprintf(buf + pos, "%s",
                                field_info[k].fieldName);

                        if (j < index_info[i].numFields - 1)
                        {
                            pos += sc_sprintf(buf + pos, ", ",
                                    field_info[k].fieldName);
                        }

                        break;
                    }
                }
            }

            pos += sc_sprintf(buf + pos, ")");

            PMEM_FREENUL(indexfield_info);
        }
    }

    pos += sc_sprintf(buf + pos, ")");

    if (IS_FIXED_RECORDS_TABLE(table_info.maxRecords))
    {
        pos = print_show_result(res_show, buf, pos, DB_FALSE);
        if (pos < 0)
        {
            ret = pos;
            goto RETURN_LABEL;
        }

        if (table_info.columnName[0] == '#')
        {
            pos += sc_sprintf(buf + pos, "\nLIMIT BY %d",
                    table_info.maxRecords);
        }
        else
        {
            pos += sc_sprintf(buf + pos, "\nLIMIT BY %d USING %s",
                    table_info.maxRecords, table_info.columnName);
        }
    }

    pos += sc_sprintf(buf + pos, ";");

    switch (cont->base_cont_.runtime_logging_)
    {
    case 1:
        pos += sc_sprintf(buf + pos, "\n    -- runtime logging on");
        break;
    case 2:
        pos += sc_sprintf(buf + pos, "\n    -- runtime logging off");
        break;
    }

    pos = print_show_result(res_show, buf, pos, DB_FALSE);
    if (pos < 0)
    {
        ret = pos;
        goto RETURN_LABEL;
    }

    for (i = 0; i < numIndexes; i++)
    {
        SYSINDEXFIELD_T *info;

        if (!mdb_strncmp(index_info[i].indexName,
                        PRIMARY_KEY_PREFIX, sc_strlen(PRIMARY_KEY_PREFIX)))
        {
            continue;
        }

        if (!mdb_strncmp(index_info[i].indexName,
                        MAXREC_KEY_PREFIX, sc_strlen(MAXREC_KEY_PREFIX)))
        {
            continue;
        }

        pos += sc_sprintf(buf + pos, "\nCREATE%s INDEX %s ON %s(",
                ((index_info[i].type & UNIQUE_INDEX_BIT) ? " UNIQUE" : ""),
                index_info[i].indexName, tablename);

        for (j = 0; j < index_info[i].numFields; j++)
        {
            info = (SYSINDEXFIELD_T *) PMEM_ALLOC(sizeof(SYSINDEXFIELD_T) *
                    index_info[i].numFields);
            if (info == NULL)
            {
                ret = DB_E_OUTOFMEMORY;
                goto RETURN_LABEL;
            }

            ret = _Schema_GetIndexInfo(index_info[i].indexName, &index_info[i],
                    info, NULL);
            if (ret < 0)
            {
                PMEM_FREENUL(info);
                goto RETURN_LABEL;
            }

            for (k = 0; k < numFields; k++)
            {
                if (info[j].fieldId == field_info[k].fieldId)
                {
                    pos += sc_sprintf(buf + pos, "%s",
                            field_info[k].fieldName);
                    if (info[j].order == 'D')
                    {
                        pos += sc_sprintf(buf + pos, " DESC");
                    }

                    if (info[j].collation != field_info[k].collation)
                    {
                        pos += sc_sprintf(buf + pos, "%s",
                                dbi_get_collation(info[j].collation));
                    }

                    if (j < index_info[i].numFields - 1)
                    {
                        pos += sc_sprintf(buf + pos, ", ");
                    }

                    break;
                }
            }
            PMEM_FREENUL(info);
        }

        pos += sc_sprintf(buf + pos, ");");

        pos = print_show_result(res_show, buf, pos, DB_FALSE);
        if (pos < 0)
        {
            ret = pos;
            goto RETURN_LABEL;
        }
    }

    pos += sc_sprintf(buf + pos, "\n");

    pos = print_show_result(res_show, buf, pos, DB_FALSE);
    if (pos < 0)
    {
        ret = pos;
        goto RETURN_LABEL;
    }

    ret = pos;

  RETURN_LABEL:

    PMEM_FREENUL(index_info);
    PMEM_FREENUL(field_info);
    PMEM_FREENUL(buf);

  end_return:
    UnsetBufSegment(issys_cont);
    return ret;
}

int
dbi_All_Relation_Show(int connid, T_RESULT_SHOW * res_show)
{
    SYSTABLE_T *pSysTable = NULL;

    int ret = 0;
    int i, j;
    int tableCnt;

    tableCnt = Cont_Item_Count("systables");

    pSysTable = (SYSTABLE_T *) PMEM_ALLOC(sizeof(SYSTABLE_T) * tableCnt);
    if (pSysTable == NULL)
    {
        ret = DB_E_OUTOFMEMORY;
        goto end_return;
    }

    ret = _Schema_GetTablesInfoEx(1, pSysTable, _USER_USER);
    if (ret < 0)
    {
        goto end_return;
    }

    for (j = 0; j < 2; j++)
    {
        for (i = 0; i < tableCnt; i++)
        {
            if (pSysTable[i].tableId > _USER_TABLEID_BASE
                    && ((j == 0 && pSysTable[i].flag != 4)
                            || (j == 1 && pSysTable[i].flag == 4)))
            {
                ret = dbi_Relation_Show(connid, pSysTable[i].tableName,
                        res_show, 1);
                if (ret < 0)
                {
                    goto end_return;
                }
            }
        }
    }

  end_return:

    PMEM_FREENUL(pSysTable);

    return ret;
}

int
dbi_check_valid_rid(char *tablename, OID rid)
{
    int ret;
    struct Container *cont = NULL;

    if (tablename == NULL)
    {
        ret = Col_check_valid_rid(tablename, rid);
    }

    cont = Cont_Search(tablename);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }

    ret = Col_check_valid_rid(tablename, rid);

  end:
    return ret;
}

int
dbi_does_table_has_rid(char *tablename, OID rid)
{
    int ret;

    int sno = getSegmentNo(rid);
    int pno = getPageNo(rid);
    int offset = OID_GetOffSet(rid);

    struct Container *cont_rid;
    struct Container *cont_table;
    int issys_cont_rid = 0;
    int issys_cont_table = 0;

#ifdef CHECK_DBI_PARAM
    if (tablename == NULL)
        return DB_E_INVALIDPARAM;
#endif

    if (sno > (int) mem_anchor_->allocated_segment_no_
            || pno > PAGE_PER_SEGMENT || offset > S_PAGE_SIZE)
    {
        return DB_E_INVALID_RID;
    }

    if (OID_Point(rid) == NULL)
    {
        ret = DB_E_INVALID_RID;
        goto end;
    }

    cont_table = Cont_Search(tablename);
    if (cont_table == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }
    SetBufSegment(issys_cont_table, cont_table->collect_.cont_id_);

    cont_rid = Cont_get(rid);
    if (cont_rid == NULL)
    {
        ret = DB_E_INVALID_RID;
        goto end;
    }
    SetBufSegment(issys_cont_rid, cont_rid->collect_.cont_id_);

    if (!mdb_strcmp(tablename, cont_rid->base_cont_.name_))
    {
        ret = DB_SUCCESS;
        goto end;
    }

    ret = DB_E_INVALID_RID;

  end:
    UnsetBufSegment(issys_cont_table);
    UnsetBufSegment(issys_cont_rid);

    return ret;
}

int
dbi_Trans_Set_RU_fUseResult(int connid)
{
    int ret = DB_SUCCESS;
    int *pTransid;
    struct Trans *trans;

    DBI_CHECK_SERVER_TERM();

    pTransid = (int *) CS_getspecific(my_trans_id);

    if (pTransid == NULL || *pTransid == -1)
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

    trans->fUseResult = 1;

  end:
    return ret;
}

int
dbi_Relation_mSync_Alter(int connid, char *tablename, int enable)
{
    int *pTransid;
    int curr_transid = 0;
    struct Trans *trans;
    struct Container *cont = NULL;
    int ret;
    char *relation_name;
    MDB_UINT16 cont_type = _CONT_TABLE;
    int issys_cont = 0;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (tablename == NULL)
    {
        return DB_E_INVALIDPARAM;
    }

    ret = sc_strlen(tablename);
    if (ret == 0 || ret >= REL_NAME_LENG)
    {
        return DB_E_INVALIDPARAM;
    }
#endif

    relation_name = tablename;

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

    cont = Cont_Search(relation_name);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }

    SetBufSegment(issys_cont, cont->collect_.cont_id_);

    if (!isUserTable(cont))
    {
        ret = DB_E_NOTPERMITTED;
        goto end;
    }

    if (enable)
    {
        if (cont->base_cont_.type_ & _CONT_MSYNC
                || cont->base_cont_.type_ & _CONT_NOLOGGING
                || cont->base_cont_.type_ & _CONT_RESIDENT
                || cont->base_cont_.type_ & _CONT_VIEW)
        {
            ret = DB_E_NOTPERMITTED;
            goto end;
        }
    }
    else
    {
        if ((cont->base_cont_.type_ & _CONT_MSYNC) == 0)
        {
            ret = DB_E_NOTPERMITTED;
            goto end;
        }
    }

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    trans->fReadOnly = 0;
    trans->fLogging = 1;

    /* hold schema execlusive lock on table */
    ret = LockMgr_Lock(relation_name, *pTransid, LOWPRIO,
            LK_SCHEMA_EXCLUSIVE, WAIT);
    if (ret < 0)
    {
        goto end;
    }

    if (enable)
    {
        cont_type = _CONT_MSYNC_TABLE;
    }

    if (cont->base_cont_.type_ == cont_type)
    {
        ret = RET_SUCCESS;
        goto end;
    }

    ret = Cont_ChangeProperty(cont, cont_type, 0);
    if (ret < 0)
    {
        goto end;
    }

    ret = _Schema_ChangeTableProperty(*pTransid, relation_name, cont_type);
    if (ret < 0)
    {
        goto end;
    }

    ret = DB_SUCCESS;

  end:
    if (curr_transid == -1)
    {
        if (ret < 0)
        {
            dbi_Trans_Abort(connid);
        }
        else
        {
            dbi_Trans_Commit(connid);
        }
    }

  end_return:
    UnsetBufSegment(issys_cont);

    return ret;
}

int
dbi_Relation_mSync_Flush(int connid, char *tablename, int flag)
{
    int ret;
    int *pTransid = NULL;
    int curr_transid = 0;
    struct Container *cont = NULL;
    struct Collection *collect;
    struct Trans *trans;
    char *relation_name;
    unsigned long _offset;
    struct Page *_page;
    OID curr;
    int slot_size;
    unsigned char *p;

    int issys_cont = 0;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (tablename == NULL)
    {
        return DB_E_INVALIDPARAM;
    }

    ret = sc_strlen(tablename);
    if (ret == 0 || ret >= REL_NAME_LENG)
    {
        return DB_E_INVALIDPARAM;
    }
#endif

    relation_name = tablename;

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

    cont = Cont_Search(relation_name);
    if (cont == NULL)
    {
        ret = DB_E_UNKNOWNTABLE;
        goto end;
    }

    collect = &cont->collect_;

    if (collect->page_link_.head_ == NULL_OID)
    {
        ret = RET_SUCCESS;
        goto end;
    }

    if (!isUserTable(cont))
    {
        ret = DB_E_NOTPERMITTED;
        goto end;
    }

    if ((cont->base_cont_.type_ & _CONT_MSYNC) == 0)
    {
        ret = DB_E_NOTPERMITTED;
        goto end;
    }

    SetBufSegment(issys_cont, cont->collect_.cont_id_);

    trans = (struct Trans *) Trans_Search(*pTransid);
    if (trans == NULL)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    trans->fReadOnly = 0;
    trans->fLogging = 1;

    curr = collect->page_link_.head_;
    slot_size = collect->slot_size_;

    _page = (struct Page *) PageID_Pointer(curr);
    if (_page == NULL)
    {
        ret = DB_E_INVALID_RID;
        goto end;
    }

    curr = OID_Cal(getSegmentNo(curr), getPageNo(curr), PAGE_HEADER_SIZE);

    _offset = OID_GetOffSet(curr) + slot_size - 1;

    while (1)
    {
        p = (unsigned char *) _page + _offset;
        if (*p == DELETE_SLOT)
        {
            ret = Direct_Remove_Internal(trans, cont, NULL, NULL,
                    curr, LK_EXCLUSIVE, NULL, NULL, 1);
            if (ret < 0)
            {
                goto end;
            }
        }
        else if (flag)
        {
            if (*p == USED_SLOT || *p == UPDATE_SLOT)
            {
                OID_SaveAsBefore(RELATION_MSYNCSLOT,
                        (const void *) cont, curr, 1);

                *p = SYNCED_SLOT;

                OID_SaveAsAfter(RELATION_MSYNCSLOT,
                        (const void *) cont, curr, 1);

                SetDirtyFlag(curr);
            }
        }
        else
        {
            if (*p == SYNCED_SLOT || *p == UPDATE_SLOT)
            {
                OID_SaveAsBefore(RELATION_MSYNCSLOT,
                        (const void *) cont, curr, 1);

                *p = USED_SLOT;

                OID_SaveAsAfter(RELATION_MSYNCSLOT,
                        (const void *) cont, curr, 1);

                SetDirtyFlag(curr);
            }
        }

        if (_offset + slot_size <= S_PAGE_SIZE)
        {
            curr += slot_size;
            _offset += slot_size;
        }
        else if (getPageOid(curr) == collect->page_link_.tail_)
        {
            break;
        }
        else
        {
            curr = _page->header_.link_.next_;
            if (curr == NULL_OID)
            {
                break;
            }

            curr += PAGE_HEADER_SIZE;
            _page = (struct Page *) PageID_Pointer(curr);
            if (_page == NULL)
            {
                ret = DB_E_INVALID_RID;
                break;
            }
            _offset = PAGE_HEADER_SIZE + slot_size - 1;
        }
    }

    ret = RET_SUCCESS;

  end:
    if (curr_transid == -1)
    {
        if (ret >= 0)
        {
            dbi_Trans_Commit(connid);
        }
        else
        {
            dbi_Trans_Abort(connid);
        }
    }

  end_return:
    UnsetBufSegment(issys_cont);

    return ret;
}
