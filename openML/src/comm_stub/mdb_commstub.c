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

#include "mdb_ppthread.h"
#include "mdb_Server.h"
#include "mdb_dbi.h"
#include "ErrorCode.h"
#include "mdb_comm_stub.h"
#include "mdb_PMEM.h"
#include "mdb_syslog.h"

#ifndef DO_NOT_RAISE
int _g_pending_signal = 0;      // per process global variable
#endif


extern int gClientidx_key;

#define  DEF_INT_AS_NULL_PTR  (0xF0F1F2F3)      /* like NULL ptr */

#if defined(WIN32)
#define sigset signal
#endif

#if defined(linux) && defined(ANDROID_OS)
#define sigset signal
#endif

#ifdef linux
extern __sighandler_t sigset(int __sig, __sighandler_t __disp);
#endif

__DECL_PREFIX int _g_connid = -1;

#if defined(WIN32)
pthread_mutex_t __gCSObject_mutex;
pthread_mutex_t _struct_init_mutex;
#else
pthread_mutex_t __gCSObject_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t _struct_init_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

extern int shared_struct_init(int shmkey, int connid, int f_init);
extern int shared_struct_free(void);
extern int Process_Command(int connid, char *buf, int buf_size);

__DECL_PREFIX T_CS_OBJECT *__gCSObject = NULL;

int MAX_CONNECTION = 20;
static int volatile fInitialized = 0;

static int local_conn_type = CS_CONNECT_LIB;

#define LCH_Lock(latch)        {}
#define LCH_Unlock(latch)    {}

__DECL_PREFIX void CS_Call_atexit(void);

static int CS_ConnectSocket(T_CS_OBJECT * Obj);

static int CS_Setup_Connection(int connid);

/*****************************************************************************/
//! CS_Connect 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param dbserver :
 * \param dbname : 
 * \param userid :
 * \param passwd :
  * \param clientType :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
CS_Connect(char *dbserver, char *dbpathname,
        char *userid, char *passwd, int clientType)
{
    int connid;
    T_CS_OBJECT *Obj;
    int err;
    int connType = local_conn_type;
    int _g_connid_bk = -2;

    MDB_DB_LOCK();

    CS_Init(MAX_CONNECTION);

    connType = CS_CONNECT_LIB;

    connid = CS_GetCSObject(dbserver, dbpathname, userid, passwd,
            local_conn_type, clientType, -1);
    if (connid < 0)
    {
        MDB_DB_UNLOCK();

        return connid;  /* error code */
    }

    Obj = &__gCSObject[connid];

    Obj->connType = connType;
    Obj->pid = sc_getpid();
    Obj->thrid = sc_get_taskid();

    MUTEX_LOCK(COMM_GCSOBJECTS_MUTEX, &__gCSObject_mutex);

    if (Obj->dbport == 0)
    {
        if (sc_getenv("MOBILE_LITE_PORT") == NULL)
        {
            Obj->dbport = DEFAULT_SOCK_PORTNO;
        }
        else
        {
            Obj->dbport = sc_atoi(sc_getenv("MOBILE_LITE_PORT"));
        }
    }

    Obj->sockfd = CS_ConnectSocket(Obj);

    if (Obj->sockfd < 0)
    {
        err = Obj->sockfd;

        Obj->sockfd = -1;

        goto end;
    }

    PUSH_G_CONNID(connid);

    /* connection 별 정보. thread specific data 대신해서 사용
     * 0: thread_entry_key, 1: my_trans_id
     * 2: gKey, 3: gClientidx_key */
    TransMgr_alloc_transid_key();       /* my_trans_id */

    POP_G_CONNID();

    err = CS_Setup_Connection(connid);

    if (err < 0)
    {
        goto end;
    }

    err = DB_SUCCESS;

  end:
    ppthread_mutex_unlock(&__gCSObject_mutex);

    if (err == DB_SUCCESS)
    {
        MDB_DB_UNLOCK();
        return connid;
    }
    else
    {
        CS_FreeCSObject(connid);

        MDB_DB_UNLOCK();

        return err;
    }
}

/*****************************************************************************/
//! CS_Disconnect 
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
CS_Disconnect(int connid)
{
    int ret;
    T_CS_OBJECT *Obj = &__gCSObject[connid];

    int i;

    int _g_connid_bk = -2;

    MDB_DB_LOCK();

    er_clear();

    switch (Obj->connType)
    {
    case CS_CONNECT_LIB:
        MUTEX_LOCK(COMM_GCSOBJECTS_MUTEX, &__gCSObject_mutex);
        PUSH_G_CONNID(connid);
        Server_Down(100);
        POP_G_CONNID();
        ppthread_mutex_unlock(&__gCSObject_mutex);
        break;
    }

    ret = DB_SUCCESS;

    CS_FreeCSObject(connid);

    for (i = 0; i < MAX_CONNECTION; i++)
    {
        if (__gCSObject[i].connType != CS_CONNECT_NONE)
        {
            break;
        }
    }

    if (server == NULL)
        CS_Call_atexit();

    MDB_DB_UNLOCK();

    return ret;
}

/*****************************************************************************/
//! CS_Get_ClientType 
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
__DECL_PREFIX int
CS_Get_ClientType(int connid)
{
    if (!fInitialized)
        return CS_E_NOTINITAILIZED;

    if (connid < 0 || MAX_CONNECTION <= connid)
        return CS_E_INVALIDCONNID;

    return __gCSObject[connid].clientType;
}

#if defined(MDB_DEBUG)
__DECL_PREFIX int
CS_Get_UserName(int connid, char **conn_username)
{
    if (!fInitialized)
        return CS_E_NOTINITAILIZED;

    if (connid < 0 || MAX_CONNECTION <= connid)
        return CS_E_INVALIDCONNID;

    *conn_username = __gCSObject[connid].userid;

    return 0;
}
#endif

int
CS_Check_Connid(int connid)
{
    T_CS_OBJECT *Obj;

    if (!fInitialized)
        return CS_E_NOTINITAILIZED;

    if (connid < 0 || MAX_CONNECTION <= connid)
        return CS_E_INVALIDCONNID;

    Obj = &__gCSObject[connid];

    if (Obj->connType == CS_CONNECT_NONE)
        return CS_E_ALREADYDISCONNECTED;

    return 0;
}

/**************** static functions *******************/
/*****************************************************************************/
//! CS_Init 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param numConn :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int fLockInit = 0;       /* for __gCSObject_mutex */
int
CS_Init(int numConn)
{
    int i;

    MDB_DB_LOCK();

    if (fInitialized == 1)
    {
        MDB_DB_UNLOCK();
        return 0;
    }

#ifdef WIN32
    if (fLockInit == 0)
    {
        LCH_Lock((long *) &latchvalue);

        if (fLockInit == 0)
        {
            ppthread_mutex_init(&__gCSObject_mutex, &_mutex_attr);

            fLockInit = 1;
        }

        LCH_Unlock(latchvalue);
    }
#endif

    /* Comm Stub 자료 구조 초기화 */
    MUTEX_LOCK(COMM_GCSOBJECTS_MUTEX, &__gCSObject_mutex);

    if (fInitialized == 1)
    {
        ppthread_mutex_unlock(&__gCSObject_mutex);
        MDB_DB_UNLOCK();

        return DB_SUCCESS;
    }

    if (__gCSObject == NULL)
        __gCSObject =
                (T_CS_OBJECT *) SMEM_XCALLOC(sizeof(T_CS_OBJECT) *
                MAX_CONNECTION);

    if (__gCSObject == NULL)
    {
        ppthread_mutex_unlock(&__gCSObject_mutex);
        MDB_DB_UNLOCK();

        return CS_E_OUTOFMEMORY;
    }

    for (i = 0; i < MAX_CONNECTION; i++)
    {
        __gCSObject[i].connType = CS_CONNECT_NONE;
        __gCSObject[i].specific[my_trans_id] = (void *) DEF_INT_AS_NULL_PTR;
        __gCSObject[i].specific[gClientidx_key] = (void *) DEF_INT_AS_NULL_PTR;
    }

    fInitialized = 1;

    ppthread_mutex_unlock(&__gCSObject_mutex);
    MDB_DB_UNLOCK();

    return DB_SUCCESS;
}

/*****************************************************************************/
//! CS_GetCSObject 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param dbserver :
 * \param dbname : 
 * \param userid :
 * \param passwd :
 * \param connType :
 * \param clientType :
 * \param sockfd :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX int
CS_GetCSObject(char *dbserver, char *dbname,
        char *userid, char *passwd, int connType, int clientType, int sockfd)
{
    int i;
    T_CS_OBJECT *Obj;

    if (!fInitialized)
        return CS_E_NOTINITAILIZED;

    MDB_DB_LOCK();

    MUTEX_LOCK(COMM_GCSOBJECTS_MUTEX, &__gCSObject_mutex);

    /* 빈 slot 하나 선택 */
    for (i = 0; i < MAX_CONNECTION; i++)
    {
        if (__gCSObject[i].connType == CS_CONNECT_NONE)
        {
            sc_memset(&__gCSObject[i], 0, sizeof(T_CS_OBJECT));
            break;
        }
    }

    if (i == MAX_CONNECTION)    /* connection object 확장 */
    {
        ppthread_mutex_unlock(&__gCSObject_mutex);
        MDB_DB_UNLOCK();
        return CS_E_NOMOREHANDLE;
    }

    Obj = &__gCSObject[i];

    Obj->bufsize = 0;

    Obj->connType = connType;

    ppthread_mutex_unlock(&__gCSObject_mutex);

    Obj->sockfd = sockfd;
    Obj->clientType = clientType;

    if (dbserver == NULL)
    {
        Obj->dbserver[0] = '\0';
        Obj->dbport = 0;
    }
    else
    {
        int j;
        int len = sc_strlen(dbserver);

        for (j = 0; j < len; j++)
            if (dbserver[j] == ':')
                break;

        if (j < len)
        {
            sc_memcpy(Obj->dbserver, dbserver, j);
            Obj->dbserver[j] = '\0';
            Obj->dbport = sc_atoi(dbserver + j + 1);
        }
        else
        {
            sc_strcpy(Obj->dbserver, dbserver);
            Obj->dbport = 0;
        }
    }

    if (dbname == NULL)
        Obj->dbname[0] = '\0';
    else
        sc_strcpy(Obj->dbname, dbname);

    if (userid == NULL)
        Obj->userid[0] = '\0';
    else
        sc_strcpy(Obj->userid, userid);

    if (passwd == NULL)
        Obj->passwd[0] = '\0';
    else
        sc_strcpy(Obj->passwd, passwd);

    Obj->read_pos = Obj->write_pos = 0;
    Obj->status = 0;
    Obj->interrupt = 0;
    Obj->isql = NULL;

    MDB_DB_UNLOCK();

    return i;
}

/*****************************************************************************/
//! CS_FreeCSObject 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid :
 ************************************
 * \return  void
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX void
CS_FreeCSObject(int connid)
{
    int i;

    if (!fInitialized)
        return;

    MUTEX_LOCK(COMM_GCSOBJECTS_MUTEX, &__gCSObject_mutex);
    __gCSObject[connid].sockfd = -1;

    SMEM_FREENUL(__gCSObject[connid].buf);

    for (i = 0; i < NUM_SPECIFIC; i++)
    {
        if (i == my_trans_id || i == gClientidx_key)
        {
            __gCSObject[connid].specific[i] = (void *) DEF_INT_AS_NULL_PTR;
            continue;
        }

        SMEM_FREENUL(__gCSObject[connid].specific[i]);
    }

    __gCSObject[connid].connType = CS_CONNECT_NONE;
    ppthread_mutex_unlock(&__gCSObject_mutex);
}

/*****************************************************************************/
//! CS_Call_atexit 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void 
 ************************************
 * \return  void
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX void
CS_Call_atexit(void)
{
    if (fLockInit)
    {
        ppthread_mutex_destroy(&__gCSObject_mutex);
        fLockInit = 0;
    }

    SMEM_FREENUL(__gCSObject);

    fInitialized = 0;

    return;
}

#ifdef WIN32
#define in_addr_t  unsigned long
#endif

/*****************************************************************************/
//! CS_ConnectSocket 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param Obj :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
CS_ConnectSocket(T_CS_OBJECT * Obj)
{
    return 0;
}

/*****************************************************************************/
//! CS_Setup_Connection 
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
static int
CS_Setup_Connection(int connid)
{
    T_CS_OBJECT *Obj = &__gCSObject[connid];

    if (Obj->connType == CS_CONNECT_LIB)
    {
        return SpeedMain(Obj->dbname, STATE_LIB, connid);
    }
    else
    {
        return DB_E_NOTSUPPORTED;
    }
}

void *
CS_getspecific(int _key)
{
    if (CS_Check_Connid(_g_connid) < 0)
    {
        __SYSLOG("Invalid: CS_getspecific() _g_connid=%d, _key=%d\n",
                _g_connid, _key);
        return NULL;
    }

    if (_key == my_trans_id || _key == gClientidx_key)
    {
        if (__gCSObject[_g_connid].specific[_key] ==
                (void *) DEF_INT_AS_NULL_PTR)
            return NULL;

        return &(__gCSObject[_g_connid].specific[_key]);
    }

    return __gCSObject[_g_connid].specific[_key];
}

int
CS_setspecific(int _key, const void *value)
{
    if (CS_Check_Connid(_g_connid) < 0)
#if defined(MDB_DEBUG)
    {
        /* writing __SYSLOG, instead of assertion */
        MDB_SYSLOG(("returning -1 %s:%d\n", __func__, __LINE__));
        return -1;
    }
#else
        return -1;
#endif

    if (_key < 0 || NUM_SPECIFIC <= _key)
        return -1;

    if (_key == my_trans_id && _key == gClientidx_key)
    {
        return -2;
    }

    __gCSObject[_g_connid].specific[_key] = (void *) value;

    return 0;
}

int
CS_setspecific_int(int _key, void **value)
{
    if (CS_Check_Connid(_g_connid) < 0)
#if defined(MDB_DEBUG)
    {
        /* writing __SYSLOG, instead of assertion */
        MDB_SYSLOG(("returning -1 %s:%d\n", __func__, __LINE__));
        return -1;
    }
#else
        return -1;
#endif

    if (_key != my_trans_id && _key != gClientidx_key)
        return -3;

    if (value == NULL)
    {
        __gCSObject[_g_connid].specific[_key] = (void *) DEF_INT_AS_NULL_PTR;
    }
    else
    {
        *value = &(__gCSObject[_g_connid].specific[_key]);
    }

    return 0;
}

int
CS_Set_iSQL(int handle, void *isql)
{
#ifdef MDB_DEBUG
    /* check validity of handle(=connid) */
    if (handle < 0 || handle >= MAX_CONNECTION ||
            __gCSObject[handle].connType == CS_CONNECT_NONE)
        sc_assert(0, __FILE__, __LINE__);
#endif

    __gCSObject[handle].isql = isql;

    return DB_SUCCESS;
}
