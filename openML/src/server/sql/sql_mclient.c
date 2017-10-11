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

#include "mdb_compat.h"

#include "sql_mclient.h"
#include "mdb_er.h"

#include "mdb_comm_stub.h"

static int MAX_CLIENT_NUM = 20;

__DECL_PREFIX T_CLIENT *gClients = NULL;

#if defined(WIN32)
pthread_mutex_t gClientsLock;
#else
pthread_mutex_t gClientsLock = PTHREAD_MUTEX_INITIALIZER;
#endif

#if defined(sun)
static pthread_once_t once_init = { PTHREAD_ONCE_INIT };
#else
static pthread_once_t once_init = PTHREAD_ONCE_INIT;
#endif
int gClientidx_key = 3;

void __DECL_PREFIX DestroyClient(void);
int SQL_cleanup(int *, T_STATEMENT *, int, int);

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
static void
destroy_gClientidx_key(void)
{
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
static void
alloc_gClientidx(void)
{
    gClientidx_key = 3;
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
static void *
alloc_gbuffer(unsigned int s)
{
    void *tData;

    if (CS_setspecific_int(gClientidx_key, &tData))
        return NULL;

    return tData;
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

static int e_ptr = -1;
__DECL_PREFIX void *
get_thread_global_area(void)
{
    void *g_ptr;

    g_ptr = CS_getspecific(gClientidx_key);
    if (g_ptr == NULL)
    {
        g_ptr = alloc_gbuffer(sizeof(int));
        if (!g_ptr)
            return &e_ptr;

        *(int *) g_ptr = -1;
    }

    return g_ptr;
}

/*****************************************************************************/
//! 

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
#define LCH_Lock(latch)     {}
#define LCH_Unlock(latch)   {}

__DECL_PREFIX int
InitializeClient(void)
{
    int i;

#ifdef WIN32
    static int fLockInit = 0;
#endif

    if (gClients != NULL)
        return 1;

#ifdef WIN32
    if (fLockInit == 0)
    {
        LCH_Lock((long *) &latchvalue);

        if (fLockInit == 0)
        {
            ppthread_mutex_init(&gClientsLock, NULL);

            fLockInit = 1;
        }

        LCH_Unlock(latchvalue);
    }
#endif

    MUTEX_LOCK(SQL_GCLIENTS_MUTEX, &gClientsLock);

    if (gClients != NULL)
    {
        ppthread_mutex_unlock(&gClientsLock);
        return 1;
    }

    gClients = SMEM_ALLOC(sizeof(T_CLIENT) * MAX_CLIENT_NUM);
    if (gClients == NULL)
    {
        ppthread_mutex_unlock(&gClientsLock);
        return DB_E_OUTOFMEMORY;
    }

    sc_memset(gClients, 0, sizeof(T_CLIENT) * MAX_CLIENT_NUM);

    for (i = 0; i < MAX_CLIENT_NUM; i++)
        gClients[i].status = iSQL_STAT_NOT_READY;


    if (ppthread_once(&once_init, alloc_gClientidx))
    {
        ppthread_mutex_unlock(&gClientsLock);
        return -1;
    }

    ppthread_mutex_unlock(&gClientsLock);

    return 1;
}

/*****************************************************************************/
//! DestroyClient 

/*! \breif  Server가 Shutdown 될 경우, Client 들에게 할당된 메모리 및 정보들을 제거하는 함수
 ************************************
 * \param void 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *  FIXU : 여기에도 stmt를 보고 할것..
 *  - 자료 구조를 잘못 이해함
 *****************************************************************************/
__DECL_PREFIX void
DestroyClient(void)
{
    T_STATEMENT *stmt;
    int i, j;

    if (gClients == NULL)
        return;

    MUTEX_LOCK(SQL_GCLIENTS_MUTEX, &gClientsLock);

    for (i = 0; i < MAX_CLIENT_NUM; i++)
    {
        if (gClients[i].status == iSQL_STAT_NOT_READY)
            continue;

        stmt = gClients[i].sql->stmts;

        for (j = 0; j < gClients[i].sql->stmt_num; j++)
        {
            if (stmt->status == iSQL_STAT_IN_USE ||
                    stmt->status == iSQL_STAT_USE_RESULT ||
                    stmt->status == iSQL_STAT_PREPARED ||
                    stmt->status == iSQL_STAT_EXECUTED)
                SQL_cleanup(&gClients[i].DBHandle, stmt, MODE_AUTO, 1);

            PMEM_FREENUL(stmt->query);
            PMEM_FREENUL(stmt->fields);

            if (stmt->result)
            {
                RESULT_LIST_Destroy(stmt->result->list);
                PMEM_FREENUL(stmt->result->field_datatypes);
                PMEM_FREENUL(stmt->result);
            }

            stmt->parsing_memory = sql_mem_destroy_chunk(stmt->parsing_memory);
#ifdef MDB_DEBUG
            if (stmt->parsing_memory)
                sc_assert(0, __FILE__, __LINE__);
#endif

            if (j)
            {
                gClients[i].sql->stmts->next = stmt->next;
                PMEM_FREE(stmt);
                stmt = gClients[i].sql->stmts->next;
            }
            else
                stmt = stmt->next;

        }

        if (gClients[i].sql)
        {
            mdb_free(gClients[i].sql);
            gClients[i].sql = NULL;
        }
    }

    if (sc_memcmp(&once_init, &__once_init__, sizeof(pthread_once_t)))
    {
        destroy_gClientidx_key();
        CS_setspecific_int(gClientidx_key, NULL);

        once_init = __once_init__;
    }

    ppthread_mutex_unlock(&gClientsLock);

    SMEM_FREENUL(gClients);

    return;
}

/*****************************************************************************/
//! InsertClient

/*! \breif  Server에 Client가 추가될때 호출되는 함수
 ************************************
 * \param dbhost(in)    :
 * \param dbname(in)    : 
 * \param dbuser(in)    :
 * \param dbpassword(in):
 * \param flags(in)     :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - 새로운 Client 구조체를 생성할때 마다
 *      CUNK 공간을 초기화 시켜준다
 *
 *****************************************************************************/
__DECL_PREFIX int
InsertClient(char *dbhost, char *dbname, char *dbuser, char *dbpassword,
        int flags)
{
    int i;

    MUTEX_LOCK(SQL_GCLIENTS_MUTEX, &gClientsLock);

    for (i = 0; i < MAX_CLIENT_NUM; i++)
    {
        if (gClients[i].status == iSQL_STAT_NOT_READY)
            break;
    }

    if (i == MAX_CLIENT_NUM)
    {
// Under MCONN, MAX_CLIENT_NUM should not be modifiable since it is related with
// max. transaction number and max. connection number.
        ppthread_mutex_unlock(&gClientsLock);
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, E_MAXCLIENTS_EXEEDED, 0);
        return DB_FAIL;
    }

    _g_connid = i;

    sc_memset(&gClients[i], 0, sizeof(T_CLIENT));

    gClients[i].status = iSQL_STAT_CONNECTED;

    ppthread_mutex_unlock(&gClientsLock);

    THREAD_HANDLE = i;

    gClients[i].DBHandle = -1;

    gClients[i].flags = flags;

    if (gClients[i].sql == NULL)
    {
        gClients[i].sql = mdb_malloc(sizeof(T_SQL));
        if (gClients[i].sql == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return DB_FAIL;
        }
    }
    sc_memset(gClients[i].sql, 0, sizeof(T_SQL));
    gClients[i].sql->stmts->prev = gClients[i].sql->stmts;
    gClients[i].sql->stmts->next = gClients[i].sql->stmts;

    gClients[i].last_query = ST_NONE;
    gClients[i].store_or_use_result = MDB_UINT_MAX;
    gClients[i].trans_info.opened = 0;
    gClients[i].trans_info.when_to_open = -1;

    gClients[i].trans_info.query_timeout = 0;


    /* ENHANCEMENT
     * 추후에 login을 위한 internal call이 추가된다면,
     * 여기에서 해당 login funcion을 call해야 한다.
     */
    gClients[i].status = iSQL_STAT_READY;
    gClients[i].sql->stmts[0].status = iSQL_STAT_READY;

/* iSQL_begin_transaction */
    gClients[i].sql->stmts[0].trans_info = &gClients[i].trans_info;

    gClients[i].sql->stmts->parsing_memory = NULL;

    return i;
}

/*****************************************************************************/
//!  RemoveClient

/*! \breif  Client가 제거될때(iSQL_disconnect)가 일어날때 호출됨
 ************************************
 * \param void
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - Client의 Stmt에 할당된 Chunk를 해제 시킨다
 *
 *****************************************************************************/
int
RemoveClient(void)
{
    int thrd_handle = THREAD_HANDLE;

    if (gClients[thrd_handle].status == iSQL_STAT_NOT_READY)
        return -1;

    gClients[thrd_handle].DBHandle = -1;

    {
        void *ptr = CS_getspecific(gClientidx_key);

        if (ptr)
        {
            CS_setspecific_int(gClientidx_key, NULL);
        }
    }

    gClients[thrd_handle].sql->stmts->parsing_memory =
            sql_mem_destroy_chunk(gClients[thrd_handle].sql->stmts->
            parsing_memory);

#ifdef MDB_DEBUG
    if (gClients[thrd_handle].sql->stmts->parsing_memory != NULL)
        sc_assert(0, __FILE__, __LINE__);
#endif

    mdb_free(gClients[thrd_handle].sql);
    gClients[thrd_handle].sql = NULL;

    gClients[thrd_handle].status = iSQL_STAT_NOT_READY;

    return 1;
}
