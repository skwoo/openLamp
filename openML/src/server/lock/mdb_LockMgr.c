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

#define CYCLE        100
#define NO_CYCLE    101

__DECL_PREFIX struct LockMgr *Lock_Mgr;

/*********************************************

            << Lock Compatibility Matrix >>

                  current lock mode
            -------------------------------
            |    | NO   SS   DS   DX   SX |
            -------------------------------
  request   | NO | O    O    O    O    O  |
            | SS | O    O    O    O    X  |    
  lock      | DS | O    O    O    X    X  |
            | DX | O    O    X    X    X  | 
  mode      | SX | O    X    X    X    X  |
             -----------------------------


             << Lock Conversion Matrix >>

                  current lock mode
            -------------------------------
            |    | NO   SS   DS   DX   SX |
            -------------------------------
  request   | NO | -    -    -    -    -  |
            | SS | SS   -    -    -    -  |    
  lock      | DS | DS   DS   -    -    -  |
            | DX | DX   DX   DX   -    -  | 
  mode      | SX | SX   SX   SX   SX   -  |
             -----------------------------

***********************************************/

unsigned char LK_COMP[LK_NUM_LOCKS][LK_NUM_LOCKS] = {
    {1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0},
    {1, 1, 1, 0, 0},
    {1, 1, 0, 0, 0},
    {1, 0, 0, 0, 0}
};

LOCKMODE LK_CONV[LK_NUM_LOCKS][LK_NUM_LOCKS] = {
    {LK_MODE_NO, LK_MODE_SS, LK_MODE_DS, LK_MODE_DX, LK_MODE_SX},
    {LK_MODE_SS, LK_MODE_SS, LK_MODE_DS, LK_MODE_DX, LK_MODE_SX},
    {LK_MODE_DS, LK_MODE_DS, LK_MODE_DS, LK_MODE_DX, LK_MODE_SX},
    {LK_MODE_DX, LK_MODE_DX, LK_MODE_DX, LK_MODE_DX, LK_MODE_SX},
    {LK_MODE_SX, LK_MODE_SX, LK_MODE_SX, LK_MODE_SX, LK_MODE_SX}
};

/* database action-consistency lock을 위한 변수 */
pthread_mutex_t mutex_db_action_lock;
pthread_cond_t cv_db_action_lock;
volatile int countLockedThread;


static unsigned int LockMgr_Hash_Value(OID oid);
static lock_item_t *LockMgr_GetLockItem(OID oid);
static int LockMgr_Insert_LockItem(OID oid,
        int trans_id, enum PRIORITY trans_prio, LOCKMODE granted_lock);
static int LockMgr_Delete_LockItem(OID oid);
static int LockMgr_Insert_LockHolder(lock_item_t * pLockItem,
        int trans_id, enum PRIORITY trans_prio, LOCKMODE granted_lock);
static int LockMgr_Delete_LockHolder(lock_item_t * pLockItem, int trans_id);
static int LockMgr_Insert_LockRequestor(lock_item_t * pLockItem,
        int trans_id, enum PRIORITY trans_prio, LOCKMODE req_lock_mode);
static int LockMgr_Delete_LockRequestor(lock_item_t * pLockItem, int trans_id);
static lock_holder_t *LockMgr_Get_LockHolder(lock_item_t * pLockItem,
        int trans_id);
static int LockMgr_CheckConflict(lock_item_t * pLockItem, int trans_id,
        enum PRIORITY trans_prio, LOCKMODE req_lock_mode);
static int FindCycle2(struct DLockNode *start_node, int target_transid);
static int LockMgr_Check_DeadLock(lock_item_t * lock_item, int trans_id);

static lock_item_t *LockMgr_alloc_lock_item(void);
static void LockMgr_free_lock_item(lock_item_t * pLockItem);
static lock_holder_t *LockMgr_alloc_lock_hold(void);
static void LockMgr_free_lock_hold(lock_holder_t * pHolder);
static lock_requestor_t *LockMgr_alloc_lock_rqst(void);
static void LockMgr_free_lock_rqst(lock_requestor_t * pRequestor);
static table_lock_t *LockMgr_alloc_table_lock(void);
static void LockMgr_free_table_lock(table_lock_t * pTableLock);

static int WakeupRequestores(lock_item_t * pLockItem);
static void LockMgr_PrintLockTable(void);

int LockMgr_Free(void);
struct Container *Cont_Search(const char *cont_name);
int Trans_ClearVisited(void);

/*
 * The codes below is to support mutex like operations for the lock manager in
 * the SHP or similar platforms where only a limited number of mutex can
 * be used.  It implements a spinlock like mechanism.  Each thread goes through
 * the spinlock loop, and checks whether it can wake up by checking a linked
 * list that stores that status of the threads.
 */

 /* windows용 ETIMEDOUT */
#if defined(_AS_WIN32_MOBILE_) || defined(_AS_WINCE_)
#ifndef ETIMEDOUT
#define ETIMEDOUT WAIT_TIMEOUT
#endif
#else
#ifndef ETIMEDOUT
#define ETIMEDOUT 110   /* for AROMA */
#endif
#endif

/*****************************************************************************/
//! LockMgr_Init 
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
LockMgr_Init(void)
{
    int i;
    lock_item_t *pLockItem;
    lock_holder_t *pHolder;
    lock_requestor_t *pRequestor;
    table_lock_t *pTableLock;

    if (Lock_Mgr == NULL)
    {
        return DB_FAIL;
    }

    for (i = 0; i < DEFAULT_HASHSIZE; i++)
    {
        Lock_Mgr->lockitem_List[i] = NULL;
    }

    Lock_Mgr->free_lock_item_list = NULL;
    Lock_Mgr->free_lock_hold_list = NULL;
    Lock_Mgr->free_lock_rqst_list = NULL;
    Lock_Mgr->free_table_lock_list = NULL;

    Lock_Mgr->max_free_lock_items = MAX_TRANS;
    Lock_Mgr->num_free_lock_items = Lock_Mgr->max_free_lock_items;
    for (i = 0; i < Lock_Mgr->num_free_lock_items; i++)
    {
        pLockItem = (lock_item_t *) DBMem_Alloc(sizeof(lock_item_t));
        if (pLockItem == NULL)
        {
            goto error;
        }
        pLockItem->next_ = Lock_Mgr->free_lock_item_list;
        Lock_Mgr->free_lock_item_list = DBMem_P2V(pLockItem);
    }

    Lock_Mgr->max_free_lock_holds = MAX_TRANS * 3;
    Lock_Mgr->num_free_lock_holds = Lock_Mgr->max_free_lock_holds;
    for (i = 0; i < Lock_Mgr->num_free_lock_holds; i++)
    {
        pHolder = (lock_holder_t *) DBMem_Alloc(sizeof(lock_holder_t));
        if (pHolder == NULL)
        {
            goto error;
        }
        pHolder->next_ = Lock_Mgr->free_lock_hold_list;
        Lock_Mgr->free_lock_hold_list = DBMem_P2V(pHolder);
    }

    Lock_Mgr->max_free_lock_rqsts = MAX_TRANS;
    Lock_Mgr->num_free_lock_rqsts = Lock_Mgr->max_free_lock_rqsts;
    for (i = 0; i < Lock_Mgr->num_free_lock_rqsts; i++)
    {
        pRequestor =
                (lock_requestor_t *) DBMem_Alloc(sizeof(lock_requestor_t));
        if (pRequestor == NULL)
        {
            goto error;
        }
        pRequestor->next_ = Lock_Mgr->free_lock_rqst_list;
        Lock_Mgr->free_lock_rqst_list = DBMem_P2V(pRequestor);
    }

    Lock_Mgr->max_free_table_locks = MAX_TRANS;
    Lock_Mgr->num_free_table_locks = Lock_Mgr->max_free_table_locks;
    for (i = 0; i < Lock_Mgr->num_free_table_locks; i++)
    {
        pTableLock = (table_lock_t *) DBMem_Alloc(sizeof(table_lock_t));
        if (pTableLock == NULL)
        {
            goto error;
        }
        pTableLock->next = Lock_Mgr->free_table_lock_list;
        Lock_Mgr->free_table_lock_list = DBMem_P2V(pTableLock);
    }

    return DB_SUCCESS;

  error:
    LockMgr_Free();
    return DB_FAIL;
}

/*****************************************************************************/
//! LockMgr_Free 
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
LockMgr_Free(void)
{
    int i;
    lock_item_t *pLockItem;
    lock_holder_t *pLockHolder;
    lock_requestor_t *pLockRequestor;
    table_lock_t *pTableLock;

    for (i = 0; i < DEFAULT_HASHSIZE; i++)
    {
        while (Lock_Mgr->lockitem_List[i] != NULL)
        {
            pLockItem = DBMem_V2P(Lock_Mgr->lockitem_List[i]);

            /* requestor list 제거 */
            while (pLockItem->requestor_list_ != NULL)
            {
                pLockRequestor = DBMem_V2P(pLockItem->requestor_list_);

                pLockItem->requestor_list_ = pLockRequestor->next_;

                DBMem_Free(pLockRequestor);
                pLockRequestor = NULL;
            }

            /* holder list 제거 */
            while (pLockItem->holder_list_ != NULL)
            {
                pLockHolder = DBMem_V2P(pLockItem->holder_list_);

                pLockItem->holder_list_ = pLockHolder->next_;

                DBMem_Free(pLockHolder);
                pLockHolder = NULL;
            }

            Lock_Mgr->lockitem_List[i] = pLockItem->next_;

            DBMem_Free(pLockItem);
            pLockItem = NULL;
        }
    }

    while (Lock_Mgr->free_lock_item_list != NULL)
    {
        pLockItem = DBMem_V2P(Lock_Mgr->free_lock_item_list);
        Lock_Mgr->free_lock_item_list = pLockItem->next_;
        DBMem_Free(pLockItem);
    }
    while (Lock_Mgr->free_lock_hold_list != NULL)
    {
        pLockHolder = DBMem_V2P(Lock_Mgr->free_lock_hold_list);
        Lock_Mgr->free_lock_hold_list = pLockHolder->next_;
        DBMem_Free(pLockHolder);
    }
    while (Lock_Mgr->free_lock_rqst_list != NULL)
    {
        pLockRequestor = DBMem_V2P(Lock_Mgr->free_lock_rqst_list);
        Lock_Mgr->free_lock_rqst_list = pLockRequestor->next_;
        DBMem_Free(pLockRequestor);
    }
    while (Lock_Mgr->free_table_lock_list != NULL)
    {
        pTableLock = DBMem_V2P(Lock_Mgr->free_table_lock_list);
        Lock_Mgr->free_table_lock_list = pTableLock->next;
        DBMem_Free(pTableLock);
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LockMgr_Lock 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param relation_name :
 * \param trans_id : 
 * \param trans_prio :
 * \param req_lock_mode :
 * \param waiting_flag :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LockMgr_Lock(char *relation_name, int trans_id, enum PRIORITY trans_prio,
        LOCKMODE req_lock_mode, LCRP waiting_flag)
{
    struct Container *Relation = NULL;
    struct Trans *trans = NULL;
    lock_item_t *lockitem = NULL;
    lock_holder_t *pLockHolder;
    int oid;
    int result = DB_FAIL;

#if defined(WIN32) || defined(USE_PPTHREAD)
    struct db_timespec timeout;
#else
    struct timespec timeout;
#endif
    int lock_wait_time = 0;
    int f_checked = 0;
    int req_lock_upgrade = 0;

    if (relation_name[0] == '\0')
        return DB_E_INVALIDPARAM;

    //---------------------------------------------------------------
    // table이 있는지 먼저 점검
    //---------------------------------------------------------------
    Relation = (struct Container *) Cont_Search(relation_name);

    if (Relation == NULL)
    {
        return DB_E_UNKNOWNTABLE;
    }

    /* temp table인지 점검.
       temp table이면 lock을 하지 않고 그냥 SUCCESS return */
    if (isNoLocking(Relation))
        return DB_SUCCESS;

    oid = Relation->base_cont_.id_;

    //---------------------------------------------------------------
    // 제대로된 Tx id인지 점검
    //---------------------------------------------------------------
    trans = (struct Trans *) Trans_Search(trans_id);

    if (trans == NULL)
    {
        return DB_E_UNKNOWNTRANSID;
    }

    if (trans->fRU == 1 && req_lock_mode == LK_SHARED)
    {
        req_lock_mode = LK_SCHEMA_SHARED;
    }

    //---------------------------------------------------------------
    // lock item이 있는지 점검해서, 없으면 새로 넣고 SUCCESS
    //---------------------------------------------------------------
    lockitem = LockMgr_GetLockItem(oid);

    if (lockitem == NULL)       // 기존에 relation_name에 대한 lock이 없음
    {
        result = LockMgr_Insert_LockItem(oid, trans_id, trans_prio,
                req_lock_mode);

        if (result < 0)
        {
            __SYSLOG(" Insert LockItem FAIL\n");
            goto End_Processing;
        }

        result = LockMgr_trans_insert_table_lock(&trans->trans_lock, oid,
                req_lock_mode);
        if (result < 0)
        {
            __SYSLOG(" Insert trans LockItem FAIL\n");
            LockMgr_Delete_LockItem(oid);
            goto End_Processing;
        }

        result = req_lock_mode;

        goto End_Processing;
    }

    //---------------------------------------------------------------
    // lock item이 있는 경우.. 먼저 이미 lock을 가지고 있는 경우
    //---------------------------------------------------------------
    //동일한 트랜잭션에의해 동일한 테이블에  라킹이 들어왔을경우
    pLockHolder = LockMgr_Get_LockHolder(lockitem, trans_id);
    if (pLockHolder != NULL)
    {
        /* get new request lock mode */
        req_lock_mode = LK_CONV[req_lock_mode][pLockHolder->hld_lock_mode_];

        if (req_lock_mode == pLockHolder->hld_lock_mode_)
        {
            /* weak or equal lock request */
            pLockHolder->lock_count_++;
            result = req_lock_mode;
            goto End_Processing;

        }

        /* strong lock request */
        if (lockitem->holder_count_ == 1 ||
                LockMgr_CheckConflict(lockitem, trans_id, trans_prio,
                        req_lock_mode) == NOCONFLICT)
        {
            lockitem->granted_lock_
                    = LK_CONV[req_lock_mode][lockitem->granted_lock_];
            pLockHolder->hld_lock_mode_ = req_lock_mode;
            pLockHolder->lock_count_++;
            result = req_lock_mode;
            goto End_Processing;
        }

        /* lock upgrade를 표시하고 바로 requestor list에 추가 */
        req_lock_upgrade = LK_UPGRADE;
        goto insert_requestor;
    }

    /*---------------------------------------------------------------
      Here, this transaction does not have a lock on the lockitem.
      Check whether there is a requestor on the lockitme.
      If yes, make the transaction a requestor.
    ---------------------------------------------------------------*/
    /* pLockHolder == NULL */

    //---------------------------------------------------------------
    // 이미 lock holder가 있으나 conflict는 아닌 상황
    //---------------------------------------------------------------
    if (LockMgr_CheckConflict(lockitem, -1, trans_prio, req_lock_mode)
            == NOCONFLICT)
    {
        result = LockMgr_Insert_LockHolder(lockitem, trans_id, trans_prio,
                req_lock_mode);
        if (result < 0)
        {
            __SYSLOG("insert lock holder fail\n");
            goto End_Processing;
        }

        result = LockMgr_trans_insert_table_lock(&trans->trans_lock, oid,
                req_lock_mode);
        if (result < 0)
        {
            __SYSLOG("insert lockitem to trans fail\n");
            goto End_Processing;
        }

        result = req_lock_mode;
        goto End_Processing;
    }

  insert_requestor:
    //---------------------------------------------------------------
    // lock conflict 발생, no wait이면 바로 return
    //---------------------------------------------------------------
    if (waiting_flag == NOWAIT)
    {
        goto End_Processing;
    }

    //---------------------------------------------------------------
    // lock conflict 발생, waiting 조건
    //---------------------------------------------------------------
    if (LockMgr_Insert_LockRequestor(lockitem, trans_id, trans->prio_,
                    (LOCKMODE) (req_lock_mode | req_lock_upgrade)) < 0)
    {
        goto End_Processing;
    }

    trans->trans_lock.req_tableid = oid;

    //---------------------------------------------------------------
    // (7) If there is any conflict, and waiting_flag != NOWAIT,
    //---------------------------------------------------------------
    // (7-4) If time out, return eTimeOut
    //---------------------------------------------------------------
    lock_wait_time = 0;
    trans->timeout_ = FALSE;
    trans->aborted_ = FALSE;

    trans->dbact_lock_wait_ = TRUE;

    while (1)
    {
        int i;
        int _g_connid_bk = -2;
        extern int get_mdb_db_lock_depth();
        int lock_depth = get_mdb_db_lock_depth();

/* DEADLOCK_PREVENTION */
        if (f_checked == 0)
        {
            if (LockMgr_Check_DeadLock(lockitem, trans_id))
            {
                __SYSLOG("Deadlock detected:transid:%d, mode:%d, upgrade:%d\n",
                        trans_id, req_lock_mode, req_lock_upgrade);
                LockMgr_PrintLockTable();

                LockMgr_Delete_LockRequestor(lockitem, trans_id);

                trans->trans_lock.req_tableid = 0;

                result = DB_E_DEADLOCKOPER;
                goto End_Processing;
            }
            f_checked = 1;
        }

        /* 시간 다시 설정하고 cv wait */
        timeout.tv_sec = sc_time(NULL) + 5;
        timeout.tv_nsec = 0;

        trans->trans_lock_wait_ = TRUE;

        PUSH_G_CONNID(-1);      // save current _g_connid

        for (i = 0; i < lock_depth; i++)
            MDB_DB_UNLOCK();

        ppthread_cond_timedwait(&trans->wait_cond_,
                &trans->trans_mutex, &timeout);

        for (i = 0; i < lock_depth; i++)
            MDB_DB_LOCK();

        POP_G_CONNID(); // restore saved _g_connid

        trans->trans_lock_wait_ = FALSE;
        lock_wait_time += 5;

        if (trans->timeout_ || trans->aborted_)
            break;

        if (trans->interrupt)
        {
            LockMgr_Delete_LockRequestor(lockitem, trans_id);
            trans->trans_lock.req_tableid = 0;
            trans->interrupt = FALSE;

            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_TRANSINTERRUPT, 0);
            result = DB_E_DEADLOCKOPER;
            goto End_Processing;
        }

        if (server->fTerminated)
        {
#ifdef MDB_DEBUG
            __SYSLOG("LockMgr_Lock: Server terminated\n");
#endif
            result = DB_E_TERMINATED;
            goto End_Processing;
        }

        /* DEADLOCK_PREVENTION */
        /* lock_wait_time이 설정된 lock timeout 시간을 초과하였다면 */
        if (server->shparams.lock_timeout > 0 &&
                lock_wait_time >= server->shparams.lock_timeout)
        {
            LockMgr_Delete_LockRequestor(lockitem, trans_id);
            trans->trans_lock.req_tableid = 0;
            if (trans->timeout_ == 1 || trans->aborted_ == 1)
            {
                __SYSLOG("trans->timeout_ or trans->aborted is 1.\n");
            }
            result = DB_E_LOCKTIMEOUT;
            __SYSLOG("PID[%d,%p] Table[%s] LockMgr_Lock timeout. "
                    "Waited [%d] sec. limit=[%d] sec.\n",
                    sc_getpid(), sc_get_taskid(), relation_name,
                    lock_wait_time, server->shparams.lock_timeout);
            goto End_Processing;
        }
    }

    result = req_lock_mode;

  End_Processing:
    trans->dbact_lock_wait_ = FALSE;

    /* 누군가 내가 lock 사용 가능하도록 한 상태 */
    if (trans->timeout_)
    {
        trans->timeout_ = FALSE;
    }

    /* abort 된 경우 */
    if (trans->aborted_ && trans->state != TRANS_STATE_ABORT)
    {
        int *pTransid = (int *) CS_getspecific(my_trans_id);
        struct Trans *_my_trans;

#ifdef MDB_DEBUG
        __SYSLOG("LockMgr_Lock: Tx %d aborted\n", *pTransid);
#endif

        _my_trans = (struct Trans *) Trans_Search(*pTransid);
        if (_my_trans == NULL)
        {
            return DB_E_UNKNOWNTRANSID;
        }

        LockMgr_Delete_LockRequestor(lockitem, trans_id);
        _my_trans->trans_lock.req_tableid = 0;

        _my_trans->state = TRANS_STATE_ABORT;

        Cont_RemoveTransTempObjects(_my_trans->connid, _my_trans->id_);

        TransMgr_trans_abort(_my_trans->id_);
        *pTransid = -1;

        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_DEADLOCKABORT, 0);
        return DB_E_DEADLOCKABORT;
    }

    return result;
}

/*****************************************************************************/
//! LockMgr_Unlock 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param trans_id :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
/* trans_id의 모든 lock을 다 풀어준다.
   transaction commit시에 사용 */
int
LockMgr_Unlock(int trans_id)
{
    struct Trans *trans = NULL;
    lock_item_t *pLockItem;
    table_lock_t *table_lock;
    rec_lock_t *rec_lock;
    int result = DB_SUCCESS;

    trans = (struct Trans *) Trans_Search(trans_id);

    if (trans == NULL)
    {
        /* Like elsewhere, let FAIL it. */
        return DB_E_UNKNOWNTRANSID;
    }

    if (trans->trans_lock.req_tableid != 0)
    {
        pLockItem = (lock_item_t *)
                LockMgr_GetLockItem(trans->trans_lock.req_tableid);

        LockMgr_Delete_LockRequestor(pLockItem, trans_id);

        trans->trans_lock.req_tableid = 0;
    }

    while (trans->trans_lock.table_lock_list)
    {
        table_lock = DBMem_V2P(trans->trans_lock.table_lock_list);
        trans->trans_lock.table_lock_list = table_lock->next;

        /* hash table에서 제거 */
        pLockItem = LockMgr_GetLockItem(table_lock->tableid);
        if (pLockItem)
        {
            result = LockMgr_Delete_LockHolder(pLockItem, trans->id_);
            if (result < 0)
            {
                __SYSLOG("Warning: unlock... delete lock holder fail\n");
                goto End_Processing;
            }
            if (pLockItem->wait_trans_count_ == 0)
            {
                if (pLockItem->holder_count_ == 0)
                    LockMgr_Delete_LockItem(pLockItem->oid_);
            }
            else        /* pLockItem->wait_trans_count_ > 0 */
            {
                result = WakeupRequestores(pLockItem);
                if (result < 0)
                {
                    __SYSLOG("Warning: unlock wakeup lock requestors fail\n");
                    goto End_Processing;
                }
            }
        }

        /* trans lock table에서 제거 */
        while (table_lock->rec_lock_head)
        {
            rec_lock = DBMem_V2P(table_lock->rec_lock_head);
            table_lock->rec_lock_head = rec_lock->next;
            LockMgr_delete_row_lock(rec_lock);
            DBMem_Free(rec_lock);
            rec_lock = NULL;
        }

        LockMgr_free_table_lock(table_lock);
        table_lock = NULL;
    }

    trans->trans_lock.table_lock_count = 0;

  End_Processing:

    return result;
}


/*****************************************************************************/
//! LockMgr_Table_Unlock 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param trans_id :
 * \param oid : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
/* trans_id의 relation_name에 대한 lock을 풀어준다.
   system table 운용을 위해서 사용, 즉, system table에 대한 lock은
   cursor close시에 풀어준다. 따라서, 이 function은 system table에
   대해서만 제한적으로 사용한다. relation_name에 내용 점검!!! */
int
LockMgr_Table_Unlock(int trans_id, OID oid)
{
    int result = DB_SUCCESS;
    lock_item_t *pLockItem;
    lock_holder_t *pLockHolder;

    pLockItem = LockMgr_GetLockItem(oid);
    if (pLockItem == NULL)
    {
        /* If oid is a system table, just pass! */
        if (oid < _USER_TABLEID_BASE)
        {
            goto End_Processing;
        }
        __SYSLOG("LOCK_CHECK: lock item (oid:%d) not found\n", oid);
        result = DB_FAIL;
        goto End_Processing;
    }

    pLockHolder = LockMgr_Get_LockHolder(pLockItem, trans_id);
    if (pLockHolder == NULL)
    {
        __SYSLOG("LOCK_CHECK: lock holder (trans_id:%d) not found\n",
                trans_id);
        result = DB_FAIL;
        goto End_Processing;
    }

    /* decrement lock count */
    pLockHolder->lock_count_--;

    if (pLockHolder->lock_count_ == 0)
    {
        struct Trans *trans = NULL;

        trans = (struct Trans *) Trans_Search(trans_id);
        if (trans == NULL)
        {
            __SYSLOG("During unlock, transaction(%d) not found\n", trans_id);
            return DB_FAIL;
        }

        result = LockMgr_Delete_LockHolder(pLockItem, trans_id);
        if (result < 0)
        {
            __SYSLOG("Warning: delete lock holder fail table %d\n", oid);
            goto End_Processing;
        }

        if (pLockItem->wait_trans_count_ == 0)
        {
            if (pLockItem->holder_count_ == 0)
            {
                LockMgr_Delete_LockItem(oid);
            }
        }
        else    /* pLockItem->wait_trans_count_ > 0 */
        {
            result = WakeupRequestores(pLockItem);
            if (result < 0)
            {
                __SYSLOG("Warning: wakeup lock requestors fail\n");
                goto End_Processing;
            }
        }

        LockMgr_trans_delete_table_lock(&trans->trans_lock, oid);
    }

  End_Processing:

    return result;
}

/*****************************************************************************/
//! WakeupRequestores 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param pLockItem :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
WakeupRequestores(lock_item_t * pLockItem)
{
    struct Trans *trans;
    lock_holder_t *pHolder;
    lock_requestor_t *pRequestor;

    if (pLockItem->wait_trans_count_ == 0)
    {
        __SYSLOG("LOCK_CHECK: No transaction to wake up.\n");
        return DB_SUCCESS;
    }

    while (pLockItem->requestor_list_)
    {
        pRequestor = DBMem_V2P(pLockItem->requestor_list_);
        if (!LK_COMP[pRequestor->req_lock_mode_][pLockItem->granted_lock_])
        {
            /* JHPAKR_LOCK_UPGRADE_WAKEUP_DEBUG */
            if (pRequestor->lock_upgraded == 0 ||
                    LockMgr_CheckConflict(pLockItem, pRequestor->trans_id_,
                            pRequestor->trans_prio_,
                            pRequestor->req_lock_mode_) == CONFLICT)
                break;
        }

        trans = (struct Trans *) Trans_Search(pRequestor->trans_id_);
        if (trans == NULL)
        {
            __SYSLOG("trans null trans_id=%d\n", pRequestor->trans_id_);
            return DB_E_UNKNOWNTRANSID;
        }

        pHolder = LockMgr_Get_LockHolder(pLockItem, pRequestor->trans_id_);
        if (pHolder != NULL)
        {
            pHolder->hld_lock_mode_ = pRequestor->req_lock_mode_;
            pHolder->lock_count_++;
            /* fix_locking */
            pLockItem->granted_lock_
                    =
                    LK_CONV[pHolder->hld_lock_mode_][pLockItem->granted_lock_];
        }
        else
        {
            if (LockMgr_Insert_LockHolder(pLockItem, pRequestor->trans_id_,
                            pRequestor->trans_prio_,
                            pRequestor->req_lock_mode_) < 0)
            {
                __SYSLOG("insert lock holder fail\n");
                return DB_FAIL;
            }

            LockMgr_trans_insert_table_lock(&trans->trans_lock,
                    pLockItem->oid_, pRequestor->req_lock_mode_);
        }

        if (LockMgr_Delete_LockRequestor(pLockItem, pRequestor->trans_id_) < 0)
        {
            __SYSLOG("delete lock requestor fail\n");
            return DB_FAIL;
        }

        trans->trans_lock.req_tableid = 0;

        /* wake up the transaction */
        trans->timeout_ = TRUE;

        if (trans->trans_lock_wait_)
        {
            MUTEX_LOCK(TRANS_ENTRY_MUTEX, &trans->trans_mutex);
            ppthread_cond_signal(&trans->wait_cond_);
            ppthread_mutex_unlock(&trans->trans_mutex);
        }
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LockMgr_Check_DeadLock 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param lock_item :
 * \param req_trans_id : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
/* DEADLOCK_PREVENTION */
static int
LockMgr_Check_DeadLock(lock_item_t * lock_item, int req_trans_id)
{
    int ret = 0;
    lock_holder_t *lock_holder = NULL;
    int trans_id = 0;
    struct Trans *trans = NULL;
    struct DLockNode start_node;

    if (Trans_ClearVisited() < 0)       /* 트랜잭션이 없거나 하나인 경우 */
    {
        return 0;
    }

    lock_holder = DBMem_V2P(lock_item->holder_list_);
    for (; lock_holder != NULL; lock_holder = DBMem_V2P(lock_holder->next_))
    {
        trans_id = lock_holder->trans_id_;

        if (trans_id == req_trans_id)
            continue;

        trans = (struct Trans *) Trans_Search(trans_id);
        if (trans == NULL)
        {
            __SYSLOG("Unknown trans_id %d\n", trans_id);
            continue;
        }

        if (trans->trans_lock.req_tableid != 0 &&
                trans->visited_ == FALSE && trans->aborted_ == FALSE)
        {
            start_node.trans_id_ = trans_id;
            start_node.priority_ = trans->prio_;

            if (FindCycle2(&start_node, req_trans_id) == CYCLE)
            {
                ret = 1;
                break;
            }
            if (lock_holder == NULL)
            {
                ret = 0;
                break;
            }
            else
                continue;
        }

        trans->visited_ = TRUE;
    }

    return ret;
}

/*****************************************************************************/
//! FindCycle2 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param start_node :
 * \param target_trans_id : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
FindCycle2(struct DLockNode *start_node, int target_trans_id)
{
    struct Trans *trans = NULL;
    lock_item_t *lock_item = NULL;
    struct DLockNode next_node;
    struct Container *Relation;
    lock_holder_t *lock_holder = NULL;

    trans = (struct Trans *) Trans_Search(start_node->trans_id_);
    if (trans == NULL)
        return NO_CYCLE;

    if (trans->trans_lock.req_tableid == 0)
        return NO_CYCLE;

    Relation = (struct Container *)
            Cont_Search_Tableid(trans->trans_lock.req_tableid);

    if (Relation == NULL)
    {
        __SYSLOG(" Relation(%d) Not Found in find cycle 2\n",
                trans->trans_lock.req_tableid);
        return NO_CYCLE;
    }

    lock_item = (lock_item_t *) LockMgr_GetLockItem(Relation->base_cont_.id_);
    if (lock_item == NULL)
    {
        __SYSLOG(" Lock Item Search FAIL  in FindCycle  \n");
        return NO_CYCLE;
    }

    if (lock_item->visited_ == TRUE)
        return NO_CYCLE;

    lock_item->visited_ = TRUE;

    lock_holder = DBMem_V2P(lock_item->holder_list_);
    for (; lock_holder != NULL; lock_holder = DBMem_V2P(lock_holder->next_))
    {
        if (lock_holder->trans_id_ == start_node->trans_id_)
        {   /* lock upgraded */
            continue;
        }

        if (lock_holder->trans_id_ == target_trans_id)
        {
            lock_item->visited_ = FALSE;
            return CYCLE;
        }

        trans = (struct Trans *) Trans_Search(lock_holder->trans_id_);
        if (trans == NULL)
        {
            __SYSLOG("Unknown lock holder trans_id %d\n",
                    lock_holder->trans_id_);
            continue;
        }

        next_node.trans_id_ = lock_holder->trans_id_;
        next_node.priority_ = lock_holder->trans_prio_;

        if (FindCycle2(&next_node, target_trans_id) == CYCLE)
        {
            lock_item->visited_ = FALSE;
            return CYCLE;
        }

        trans->visited_ = TRUE;
    }

    lock_item->visited_ = FALSE;

    return NO_CYCLE;
}

/*****************************************************************************/
//! LockMgr_Hash_Value 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param oid
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static unsigned int
LockMgr_Hash_Value(OID oid)
{
    return (oid % DEFAULT_HASHSIZE);
}

/*****************************************************************************/
//! LockMgr_GetLockItem 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param oid :
 ************************************
 * \return  lock_item_t* : 
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static lock_item_t *
LockMgr_GetLockItem(OID oid)
{
    lock_item_t *pLockItem;
    int hashval;

    hashval = LockMgr_Hash_Value(oid);
    pLockItem = DBMem_V2P(Lock_Mgr->lockitem_List[hashval]);
    while (pLockItem != NULL)
    {
        if (pLockItem->oid_ == oid)
            return pLockItem;

        pLockItem = DBMem_V2P(pLockItem->next_);
    }

    return NULL;
}

/*****************************************************************************/
//! LockMgr_Insert_LockItem 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param oid :
 * \param trans_id : 
 * \param trans_prio :
 * \param granted_lock :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
LockMgr_Insert_LockItem(OID oid,
        int trans_id, enum PRIORITY trans_prio, LOCKMODE granted_lock)
{
    lock_item_t *pLockItem;
    int hashval = LockMgr_Hash_Value(oid);

    pLockItem = LockMgr_alloc_lock_item();
    if (pLockItem == NULL)
    {
        return DB_E_OUTOFLOCKMEMORY;
    }

    sc_memset(pLockItem, 0, sizeof(lock_item_t));

    pLockItem->granted_lock_ = LK_NOLOCK;
    if (LockMgr_Insert_LockHolder(pLockItem, trans_id, trans_prio,
                    granted_lock) < 0)
    {
        return DB_FAIL;
    }

    /* list 앞쪽에 연결 */
    pLockItem->next_ = Lock_Mgr->lockitem_List[hashval];
    Lock_Mgr->lockitem_List[hashval] = DBMem_P2V(pLockItem);

    pLockItem->oid_ = oid;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LockMgr_Delete_LockItem 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param oid :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
LockMgr_Delete_LockItem(OID oid)
{
    lock_item_t *pLockItem, *pPrev;
    lock_holder_t *pLockHolder;
    lock_requestor_t *pLockRequestor;
    int hashval = LockMgr_Hash_Value(oid);

    pLockItem = DBMem_V2P(Lock_Mgr->lockitem_List[hashval]);
    pPrev = NULL;
    while (pLockItem != NULL)
    {
        if (pLockItem->oid_ == oid)
        {
            if (pPrev == NULL)
                Lock_Mgr->lockitem_List[hashval] = pLockItem->next_;
            else
                pPrev->next_ = pLockItem->next_;

            /* requestor list 제거 */
            while (pLockItem->requestor_list_ != NULL)
            {
                pLockRequestor = DBMem_V2P(pLockItem->requestor_list_);

                pLockItem->requestor_list_ = pLockRequestor->next_;

                LockMgr_free_lock_rqst(pLockRequestor);
                pLockRequestor = NULL;
            }

            /* holder list 제거 */
            while (pLockItem->holder_list_ != NULL)
            {
                pLockHolder = DBMem_V2P(pLockItem->holder_list_);

                pLockItem->holder_list_ = pLockHolder->next_;

                LockMgr_free_lock_hold(pLockHolder);
                pLockHolder = NULL;
            }
            /* requestor list 제거 */

            LockMgr_free_lock_item(pLockItem);
            pLockItem = NULL;

            return DB_SUCCESS;
        }

        pPrev = pLockItem;
        pLockItem = DBMem_V2P(pLockItem->next_);
    }

    return DB_FAIL;
}

/*****************************************************************************/
//! LockMgr_Insert_LockHolder 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param pLockItem :
 * \param trans_id : 
 * \param trans_prio :
 * \param granted_lock :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
LockMgr_Insert_LockHolder(lock_item_t * pLockItem,
        int trans_id, enum PRIORITY trans_prio, LOCKMODE granted_lock)
{
    lock_holder_t *pHolder, *pCurr, *pPrev;

    pHolder = LockMgr_alloc_lock_hold();
    if (pHolder == NULL)
    {
        return DB_FAIL;
    }

    pHolder->trans_id_ = trans_id;
    pHolder->trans_prio_ = trans_prio;
    pHolder->hld_lock_mode_ = granted_lock;
    pHolder->lock_count_ = 1;
    pHolder->next_ = NULL;

    pLockItem->granted_lock_ = LK_CONV[granted_lock][pLockItem->granted_lock_];

    pLockItem->holder_count_++;

    if (pLockItem->holder_list_ == NULL)
    {
        pLockItem->holder_list_ = DBMem_P2V(pHolder);

        return DB_SUCCESS;
    }

    pCurr = DBMem_V2P(pLockItem->holder_list_);
    pPrev = NULL;
    while (1)
    {
        if (pCurr == NULL || trans_prio > pCurr->trans_prio_)
        {
            pHolder->next_ = DBMem_P2V(pCurr);
            if (pPrev == NULL)
                pLockItem->holder_list_ = DBMem_P2V(pHolder);
            else
                pPrev->next_ = DBMem_P2V(pHolder);

            break;
        }

        pPrev = pCurr;
        pCurr = DBMem_V2P(pCurr->next_);
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LockMgr_Delete_LockHolder 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param pLockItem :
 * \param trans_id : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
LockMgr_Delete_LockHolder(lock_item_t * pLockItem, int trans_id)
{
    lock_holder_t *pCurr, *pPrev;

    if (pLockItem == NULL || pLockItem->holder_list_ == NULL)
    {
        __SYSLOG("lock holder already deleted\n");
        return DB_SUCCESS;
    }

    pCurr = DBMem_V2P(pLockItem->holder_list_);
    pPrev = NULL;
    while (pCurr != NULL)
    {
        if (trans_id == pCurr->trans_id_)
        {
            if (pPrev == NULL)
                pLockItem->holder_list_ = pCurr->next_;
            else
                pPrev->next_ = pCurr->next_;

            LockMgr_free_lock_hold(pCurr);
            pCurr = NULL;

            pLockItem->holder_count_--;

            if (pLockItem->holder_count_ == 0)
            {
                pLockItem->granted_lock_ = LK_NOLOCK;
            }
            else
            {
                LOCKMODE group_mode = LK_NOLOCK;

                pCurr = DBMem_V2P(pLockItem->holder_list_);
                while (pCurr != NULL)
                {
                    group_mode = LK_CONV[pCurr->hld_lock_mode_][group_mode];
                    pCurr = DBMem_V2P(pCurr->next_);
                }

                pLockItem->granted_lock_ = group_mode;
            }
            break;
        }
        else
        {
            pPrev = pCurr;
            pCurr = DBMem_V2P(pCurr->next_);
        }
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LockMgr_Insert_LockRequestor 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param pLockItem :
 * \param trans_id : 
 * \param trans_prio :
 * \param req_lock_mode :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
LockMgr_Insert_LockRequestor(lock_item_t * pLockItem,
        int trans_id, enum PRIORITY trans_prio, LOCKMODE req_lock_mode)
{
    lock_requestor_t *pRequestor, *pCurr, *pPrev;

    pRequestor = LockMgr_alloc_lock_rqst();
    if (pRequestor == NULL)
    {
        return DB_FAIL;
    }

    pRequestor->trans_id_ = trans_id;
    pRequestor->trans_prio_ = trans_prio;
    pRequestor->req_lock_mode_ = req_lock_mode & 0x0F;
    pRequestor->lock_upgraded = req_lock_mode & 0xF0;
    pRequestor->curr_thr_id_ = ppthread_self();
    pRequestor->next_ = NULL;

    pLockItem->wait_trans_count_++;

    if (pLockItem->requestor_list_ == NULL)
    {
        pLockItem->requestor_list_ = DBMem_P2V(pRequestor);

        return DB_SUCCESS;
    }

    pCurr = DBMem_V2P(pLockItem->requestor_list_);
    pPrev = NULL;
    while (1)
    {
        /* lock upgrade하려는 트랜잭션을 먼저 수행시킨다. */
        if (pRequestor->lock_upgraded > pCurr->lock_upgraded ||
                trans_prio > pCurr->trans_prio_)
        {
            pRequestor->next_ = DBMem_P2V(pCurr);
            if (pPrev == NULL)
                pLockItem->requestor_list_ = DBMem_P2V(pRequestor);
            else
                pPrev->next_ = DBMem_P2V(pRequestor);

            break;
        }

        pPrev = pCurr;
        pCurr = DBMem_V2P(pCurr->next_);

        if (pCurr == NULL)
        {
            pPrev->next_ = DBMem_P2V(pRequestor);
            break;
        }
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LockMgr_Delete_LockRequestor 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param pLockItem :
 * \param trans_id : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
LockMgr_Delete_LockRequestor(lock_item_t * pLockItem, int trans_id)
{
    lock_requestor_t *pCurr, *pPrev;

    if (pLockItem->requestor_list_ == NULL)
    {
        __SYSLOG("lock requestor already deleted\n");
        return DB_SUCCESS;
    }

    pCurr = DBMem_V2P(pLockItem->requestor_list_);
    pPrev = NULL;
    while (pCurr != NULL)
    {
        if (trans_id == pCurr->trans_id_)
        {
            if (pPrev == NULL)
                pLockItem->requestor_list_ = pCurr->next_;
            else
                pPrev->next_ = pCurr->next_;

            LockMgr_free_lock_rqst(pCurr);
            pCurr = NULL;

            pLockItem->wait_trans_count_--;
            if (pLockItem->wait_trans_count_ == 0 &&
                    pLockItem->requestor_list_ != NULL)
            {
                __SYSLOG("LOCK_CHECK: (request) list != NULL, count = 0\n");
            }
            break;
        }
        else
        {
            pPrev = pCurr;
            pCurr = DBMem_V2P(pCurr->next_);
        }
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LockMgr_Get_LockHolder 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param pLockItem :
 * \param trans_id : 
 ************************************
 * \return  lock_holder_t* : 
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static lock_holder_t *
LockMgr_Get_LockHolder(lock_item_t * pLockItem, int trans_id)
{
    lock_holder_t *pHolder;

    pHolder = DBMem_V2P(pLockItem->holder_list_);
    while (pHolder != NULL)
    {
        if (pHolder->trans_id_ == trans_id)
            break;

        pHolder = DBMem_V2P(pHolder->next_);
    }

    return pHolder;
}

/*****************************************************************************/
//! LockMgr_CheckConflict 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param pLockItem :
 * \param trans_id : 
 * \param trans_prio :
 * \param req_lock_mode :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
LockMgr_CheckConflict(lock_item_t * pLockItem, int trans_id,
        enum PRIORITY trans_prio, LOCKMODE req_lock_mode)
{
    if (trans_id != -1)
    {
        lock_holder_t *pLockHolder;

        /* check compatibility with lock holders excluding itself */

        pLockHolder = DBMem_V2P(pLockItem->holder_list_);
        while (pLockHolder != NULL)
        {
            if (pLockHolder->trans_id_ != trans_id)
            {
                if (!LK_COMP[req_lock_mode][pLockHolder->hld_lock_mode_])
                    return CONFLICT;
            }

            pLockHolder = DBMem_V2P(pLockHolder->next_);
        }
    }
    else    /* trans_id == -1 */
    {
        lock_requestor_t *pRequestor;

        /* check compatibility with all lock holders and requesters */

        if (!LK_COMP[req_lock_mode][pLockItem->granted_lock_])
            return CONFLICT;

        pRequestor = DBMem_V2P(pLockItem->requestor_list_);
        while (pRequestor != NULL)
        {
            if (!LK_COMP[req_lock_mode][pRequestor->req_lock_mode_])
                return CONFLICT;

            /* requestor에 높은 priority의 transaction이 기다리고 있으면
               CONFLICT 처리하여 shared lock을 허용하지 않는다 */
            /* If trans is Read Uncommitted, than do not compare prioity. */
            if (req_lock_mode != LK_MODE_SS &&
                    pRequestor->trans_prio_ > trans_prio)
                return CONFLICT;

            pRequestor = DBMem_V2P(pRequestor->next_);
        }
    }

    return NOCONFLICT;
}

/*****************************************************************************/
//! LockMgr_alloc_lock_item 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void 
 ************************************
 * \return  lock_item_t*
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static lock_item_t *
LockMgr_alloc_lock_item(void)
{
    lock_item_t *pLockItem = NULL;

    if (Lock_Mgr->num_free_lock_items > 0)
    {
        if (Lock_Mgr->free_lock_item_list != NULL)
        {
            pLockItem = DBMem_V2P(Lock_Mgr->free_lock_item_list);
            Lock_Mgr->free_lock_item_list = pLockItem->next_;
            Lock_Mgr->num_free_lock_items -= 1;
        }
    }

    if (pLockItem == NULL)
    {
        pLockItem = (lock_item_t *) DBMem_Alloc(sizeof(lock_item_t));
        if (pLockItem == NULL)
        {
            __SYSLOG("lock item alloc FAIL\n");
        }
    }

    return pLockItem;
}

/*****************************************************************************/
//! LockMgr_free_lock_item 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param pLockItem :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static void
LockMgr_free_lock_item(lock_item_t * pLockItem)
{
    if (pLockItem == NULL)
        return;

    if (Lock_Mgr->num_free_lock_items < Lock_Mgr->max_free_lock_items)
    {
        pLockItem->next_ = Lock_Mgr->free_lock_item_list;
        Lock_Mgr->free_lock_item_list = DBMem_P2V(pLockItem);
        Lock_Mgr->num_free_lock_items += 1;
    }
    else
    {
        DBMem_Free(pLockItem);
    }
}

/*****************************************************************************/
//! LockMgr_alloc_lock_hold 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void 
 ************************************
 * \return  lock_holder_t*
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static lock_holder_t *
LockMgr_alloc_lock_hold(void)
{
    lock_holder_t *pHolder = NULL;

    if (Lock_Mgr->num_free_lock_holds > 0)
    {
        if (Lock_Mgr->free_lock_hold_list != NULL)
        {
            pHolder = DBMem_V2P(Lock_Mgr->free_lock_hold_list);
            Lock_Mgr->free_lock_hold_list = pHolder->next_;
            Lock_Mgr->num_free_lock_holds -= 1;
        }
    }

    if (pHolder == NULL)
    {
        pHolder = (lock_holder_t *) DBMem_Alloc(sizeof(lock_holder_t));
        if (pHolder == NULL)
        {
            __SYSLOG("lock holder alloc FAIL\n");
        }
    }

    return pHolder;
}

/*****************************************************************************/
//! LockMgr_free_lock_hold 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param pHolder :
 ************************************
 * \return  void
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static void
LockMgr_free_lock_hold(lock_holder_t * pHolder)
{
    if (pHolder == NULL)
        return;

    if (Lock_Mgr->num_free_lock_holds < Lock_Mgr->max_free_lock_holds)
    {
        pHolder->next_ = Lock_Mgr->free_lock_hold_list;
        Lock_Mgr->free_lock_hold_list = DBMem_P2V(pHolder);
        Lock_Mgr->num_free_lock_holds += 1;
    }
    else
    {
        DBMem_Free(pHolder);
    }
}

/*****************************************************************************/
//! LockMgr_alloc_lock_rqst 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void :
 ************************************
 * \return  lock_requestor_t*
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static lock_requestor_t *
LockMgr_alloc_lock_rqst(void)
{
    lock_requestor_t *pRequestor = NULL;

    if (Lock_Mgr->num_free_lock_rqsts > 0)
    {
        if (Lock_Mgr->free_lock_rqst_list != NULL)
        {
            pRequestor = DBMem_V2P(Lock_Mgr->free_lock_rqst_list);
            Lock_Mgr->free_lock_rqst_list = pRequestor->next_;
            Lock_Mgr->num_free_lock_rqsts -= 1;
        }
    }

    if (pRequestor == NULL)
    {
        pRequestor =
                (lock_requestor_t *) DBMem_Alloc(sizeof(lock_requestor_t));
        if (pRequestor == NULL)
        {
            __SYSLOG("lock requestor alloc FAIL\n");
        }
    }

    return pRequestor;
}

/*****************************************************************************/
//! LockMgr_free_lock_rqst 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param pRequestor :
 ************************************
 * \return  void
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static void
LockMgr_free_lock_rqst(lock_requestor_t * pRequestor)
{
    if (pRequestor == NULL)
        return;

    if (Lock_Mgr->num_free_lock_rqsts < Lock_Mgr->max_free_lock_rqsts)
    {
        pRequestor->next_ = Lock_Mgr->free_lock_rqst_list;
        Lock_Mgr->free_lock_rqst_list = DBMem_P2V(pRequestor);
        Lock_Mgr->num_free_lock_rqsts += 1;
    }
    else
    {
        DBMem_Free(pRequestor);
    }
}

/*****************************************************************************/
//! LockMgr_alloc_table_lock 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void
 ***********************************
 * \return  table_lock_t*
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static table_lock_t *
LockMgr_alloc_table_lock(void)
{
    table_lock_t *pTableLock = NULL;

    if (Lock_Mgr->num_free_table_locks > 0)
    {
        if (Lock_Mgr->free_table_lock_list != NULL)
        {
            pTableLock = DBMem_V2P(Lock_Mgr->free_table_lock_list);
            Lock_Mgr->free_table_lock_list = pTableLock->next;
            Lock_Mgr->num_free_table_locks -= 1;
        }
    }

    if (pTableLock == NULL)
    {
        pTableLock = (table_lock_t *) DBMem_Alloc(sizeof(table_lock_t));
        if (pTableLock == NULL)
        {
            __SYSLOG("table lock alloc FAIL\n");
        }
    }

    return pTableLock;
}

/*****************************************************************************/
//! LockMgr_free_table_lock 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param pTableLock :
 ************************************
 * \return  void
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static void
LockMgr_free_table_lock(table_lock_t * pTableLock)
{
    if (pTableLock == NULL)
        return;

    if (Lock_Mgr->num_free_table_locks < Lock_Mgr->max_free_table_locks)
    {
        pTableLock->next = Lock_Mgr->free_table_lock_list;
        Lock_Mgr->free_table_lock_list = DBMem_P2V(pTableLock);
        Lock_Mgr->num_free_table_locks += 1;
    }
    else
    {
        DBMem_Free(pTableLock);
    }
}

/*****************************************************************************/
//! LockMgr_trans_lock_init 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param trans_lock :
 * \param transid : 
 ************************************
 * \return  int :
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LockMgr_trans_lock_init(trans_lock_t * trans_lock, int transid)
{
    trans_lock->lock_granul = LK_GRANUL_ROW;

    trans_lock->transid = transid;

    trans_lock->table_lock_count = 0;
    trans_lock->table_lock_list = NULL;

    trans_lock->req_tableid = 0;
    trans_lock->req_rec_oid = 0;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LockMgr_trans_insert_table_lock 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param trans_lock :
 * \param tableid : 
 * \param lock_mode :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LockMgr_trans_insert_table_lock(trans_lock_t * trans_lock, int tableid,
        DB_INT8 lock_mode)
{
    table_lock_t *table_lock;

    table_lock = LockMgr_alloc_table_lock();
    if (table_lock == NULL)
        return DB_E_OUTOFLOCKMEMORY;

    table_lock->tableid = tableid;
    table_lock->lock_mode = lock_mode;

    table_lock->rec_lock_head = NULL;

    table_lock->next = trans_lock->table_lock_list;
    trans_lock->table_lock_list = DBMem_P2V(table_lock);

    trans_lock->table_lock_count++;

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LockMgr_trans_delete_table_lock 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param trans_lock :
 * \param tableid : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
LockMgr_trans_delete_table_lock(trans_lock_t * trans_lock, int tableid)
{
    table_lock_t *table_lock;
    table_lock_t *prev_lock;

    table_lock = DBMem_V2P(trans_lock->table_lock_list);
    prev_lock = NULL;

    while (table_lock)
    {
        if (table_lock->tableid == (DB_UINT32) tableid)
        {
            if (prev_lock == NULL)
                trans_lock->table_lock_list = table_lock->next;
            else
                prev_lock->next = table_lock->next;

            trans_lock->table_lock_count--;

            LockMgr_free_table_lock(table_lock);
            table_lock = NULL;

            return DB_SUCCESS;
        }

        prev_lock = table_lock;
        table_lock = DBMem_V2P(table_lock->next);
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LockMgr_row_lock 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param trans_lock :
 * \param tableid : 
 * \param rid :
 * \param lock_mode :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/

/*****************************************************************************/
//! LockMgr_delete_row_lock 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param rec_lock :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
/* delete row lock for table id */
int
LockMgr_delete_row_lock(rec_lock_t * rec_lock)
{
    int i;
    rec_header_t *rec_header;

    for (i = 0; i < rec_lock->count; i++)
    {
        rec_header = (rec_header_t *) OID_Point(rec_lock->recid[i]);
        if (rec_header == NULL)
        {
            return DB_E_OIDPOINTER;
        }

        rec_header->lock_mode = LK_ROW_NONE;
        rec_header->transid = -1;
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! LockMgr_PrintLockTable 
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
static void
LockMgr_PrintLockTable(void)
{
    int i;
    lock_requestor_t *requestor;
    lock_holder_t *holder;
    struct Container *cont;
    lock_item_t *lockitem;

    __SYSLOG(">>> Lock Table <<<\n");
    for (i = 0; i < DEFAULT_HASHSIZE; i++)
    {
        lockitem = DBMem_V2P(Lock_Mgr->lockitem_List[i]);
        while (lockitem)
        {
            requestor = DBMem_V2P(lockitem->requestor_list_);
            holder = DBMem_V2P(lockitem->holder_list_);

            cont = Cont_Search_Tableid(lockitem->oid_);
            if (cont != NULL)
            {
                __SYSLOG("%s: lk_mode %d, %d holders, %d waiters\n",
                        cont->base_cont_.name_,
                        lockitem->granted_lock_,
                        lockitem->holder_count_, lockitem->wait_trans_count_);

                __SYSLOG("    Holder: ");
                while (holder)
                {
                    __SYSLOG("%d[%d-%d]", holder->trans_id_,
                            holder->hld_lock_mode_, holder->lock_count_);
                    holder = DBMem_V2P(holder->next_);
                }
                __SYSLOG("\n");

                __SYSLOG("    Waiter: ");
                while (requestor)
                {
                    __SYSLOG("%d[%d-%d]    ", requestor->trans_id_,
                            requestor->req_lock_mode_,
                            requestor->lock_upgraded);
                    requestor = DBMem_V2P(requestor->next_);
                }
                __SYSLOG("\n");
            }

            lockitem = DBMem_V2P(lockitem->next_);
        }
    }
    __SYSLOG(">>>>>>>>>><<<<<<<<<<\n");
}
