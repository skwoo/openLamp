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

#ifndef LOCKMGRPI_H
#define LOCKMGRPI_H

#include "mdb_config.h"
#include "mdb_ppthread.h"
#include "mdb_inner_define.h"

#define DEFAULT_HASHSIZE            1
#define MAX_TABLE_LOCK_PER_TRANS    32
#define NUM_REC_LOCK_PER_TABLE      256

#define LK_UPGRADE                  0x10

typedef struct rec_lock_struct rec_lock_t;
struct rec_lock_struct
{
    DB_UINT16 count;
    OID recid[NUM_REC_LOCK_PER_TABLE];
    rec_lock_t *next;
};

typedef struct table_lock_struct table_lock_t;
struct table_lock_struct
{
    OID tableid;
    DB_INT8 lock_mode;
    rec_lock_t *rec_lock_head;
    table_lock_t *next;
};

typedef struct trans_lock_struct trans_lock_t;
struct trans_lock_struct
{
    DB_INT8 lock_granul;

    DB_INT16 table_lock_count;
    table_lock_t *table_lock_list;

    int transid;

    int req_tableid;
    OID req_rec_oid;
};

/* 일반적으로 transaction에서 lock을 한 횟수는 필요하지 않으나,
   system table을 운용하기 위해서 추가함.
   system table의 경우 cursor_close()를 할 때 lock을 풀어주어야
   다른 transaction에서 사용을 하므로 cursor_open()의 횟수를
   기록해 두었다가 lock_count_가 0이 되는 시점에 lock item을 없앰.
   이를 위해서 lock_count_를 추가함. */
typedef struct lock_holder_struct lock_holder_t;
struct lock_holder_struct
{
    int trans_id_;              /* lock을 transaction id */
    enum PRIORITY trans_prio_;  /* transaction의 priority */
    LOCKMODE hld_lock_mode_;
    int lock_count_;            /* transaction에서 lock을 한 횟수 */
    lock_holder_t *next_;
};

typedef struct lock_requestor_struct lock_requestor_t;
struct lock_requestor_struct
{
    int trans_id_;
    enum PRIORITY trans_prio_;
    LOCKMODE req_lock_mode_;
    DB_UINT8 lock_upgraded;
    pthread_t curr_thr_id_;
    lock_requestor_t *next_;
};

typedef struct lock_item_struct lock_item_t;
struct lock_item_struct
{
    OID oid_;
    LOCKMODE granted_lock_;
    int wait_trans_count_;
    int holder_count_;
    DB_BOOL visited_;
    lock_requestor_t *requestor_list_;
    lock_holder_t *holder_list_;
    lock_item_t *next_;
};

/*
 * Queue of nodes:{is_locked, transaction_id}, It is conntected from within
 * the table_list_head lists, for each table, i.e. each table maintains its
 * own lock_request_queue.  is_locked variable is initially set to 0, but
 * once the thread before it finishes executing, the finishing thread sets
 * it to 1.  When the thread that was waiting sees that its is_locked is
 * set to 1, it deletes its own node from the queue, and stops watiting.
 */
typedef struct lock_request_queue_s
{
    volatile int is_locked;
    int transaction_id;
    struct lock_request_queue_s *next;
} lock_request_queue_t;

/*
 * List that connects the all lock_request_queues.  The header node is
 * "table_list_head" (see below).  The lockmgr_cond_wait() and
 * lockmgr_cond_signal() uses this function (again see below) to access
 * the lock_request_queues.
 */
typedef struct table_list_s
{
    OID table_id;
    lock_request_queue_t *lock_request_queue;
    struct table_list_s *next;
} table_list_t;

struct LockMgr
{
    lock_item_t *lockitem_List[DEFAULT_HASHSIZE];
    pthread_mutex_t lockmgr_mutex;
    int max_free_lock_items;
    int num_free_lock_items;
    lock_item_t *free_lock_item_list;   /* free lock item list */
    /* pthread_mutex_t    free_lock_item_mutex; : might be used later */
    int max_free_lock_holds;
    int num_free_lock_holds;
    lock_holder_t *free_lock_hold_list; /* free lock holder list */
    /* pthread_mutex_t    free_lock_hold_mutex; : might be used later */
    int max_free_lock_rqsts;
    int num_free_lock_rqsts;
    lock_requestor_t *free_lock_rqst_list;      /* free lock requestor list */
    /* pthread_mutex_t    free_lock_rqst_mutex; : might be used later */
    int max_free_table_locks;
    int num_free_table_locks;
    table_lock_t *free_table_lock_list; /* free table lock list */
    /* pthread_mutex_t    free_table_lock_mutex; : might be used later */
};

struct DLockNode
{
    int trans_id_;
    int priority_;
};

extern __DECL_PREFIX struct LockMgr *Lock_Mgr;

/* lock granularity */
#define LK_GRANUL_ROW        0  /* default */
#define LK_GRANUL_TABLE        1

/* table-level lock mode */
/* defined in define.h
#define LK_NOLOCK        0
#define LK_SHARED        1
#define LK_EXCLUSIVE    2
*/
#define LK_INTENT_S        3
#define LK_INTENT_X        4

/* row-level lock mode */
#define LK_ROW_NONE        0
#define LK_ROW_S        1
#define LK_ROW_I        2
#define LK_ROW_U        3
#define LK_ROW_D        4

#define NOCONFLICT 0
#define CONFLICT  -1

int LockMgr_Init(void);
int LockMgr_Free(void);
int LockMgr_Lock(char *relation_name, int trans_id, enum PRIORITY trans_prio,
        LOCKMODE req_lock_mode, LCRP waiting_flag);
int LockMgr_Unlock(int trans_id);
int LockMgr_Table_Unlock(int trans_id, OID oid);
int LockMgr_trans_lock_init(trans_lock_t * trans_lock, int transid);
int LockMgr_trans_insert_table_lock(trans_lock_t * trans_lock, int tableid,
        DB_INT8 lock_mode);
int LockMgr_trans_delete_table_lock(trans_lock_t * trans_lock, int tableid);
int LockMgr_delete_row_lock(rec_lock_t * rec_lock);
int LockMgr_row_lock_compat(int hold_m, int request_m);

extern volatile int dbActionLock;

#endif
