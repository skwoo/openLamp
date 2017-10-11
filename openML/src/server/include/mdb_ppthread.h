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

// pthread 함수의 래핑 함수를 만듬 
// pthread_xxx 에서 ppthread_xxx로 변경됨 

#ifndef __PPTHREADNT_H
#define __PPTHREADNT_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include "mdb_config.h"
#include "mdb_typedef.h"

/* mutex type */
    enum MUTEX_TYPE
    {
        SC_TAB_INFO_MUTEX = 0,
        SC_IDX_INFO_MUTEX,
        SC_TAB_IDX_INFO_MUTEX,
        DB_ACTION_MUTEX,
        THRD_MGR_MUTEX,
        TRANS_MGR_MUTEX,
        TRANS_ENTRY_MUTEX,
        SHARED_MEM_MUTEX,
        LOCK_MGR_MUTEX,
        LOG_MGR_MUTEX,
        LOG_FLUSH_MUTEX,        /* 10 */
        CONT_CAT_MUTEX,
        CURSOR_MUTEX,
        IDX_CAT_MUTEX,
        MEM_MGR_MUTEX,
        DB_FILE_MUTEX,
        SQL_GCLIENTS_MUTEX,
        SQL_FLEX_MUTEX,
        SQL_MW_MUTEX,
        SQL_TABLELIST_MUTEX,
        COMM_GCSOBJECTS_MUTEX,  /* 20 */
        LOCK_ITEM_FREE_MUTEX,
        LOCK_HOLD_FREE_MUTEX,
        LOCK_RQST_FREE_MUTEX,
        TABLE_LOCK_FREE_MUTEX,
        CHKPNT_WAKEUP_MUTEX,
        STRUCT_INIT_MUTEX,
        REP_DUMMY = MDB_INT_MAX /* to guarantee sizeof(enum) == 4 */
    };

#define MUTEX_TOTAL_COUNT   35

#define MUTEX_INIT(a, b)    ppthread_mutex_init((a),(b))
#define MUTEX_DESTROY(a)    ppthread_mutex_destroy(a)
#define MUTEX_UNLOCK(a)     ppthread_mutex_unlock(a)
#define MUTEX_LOCK(a, b)    ppthread_mutex_lock(b)

#if defined(linux) && defined(USE_PPTHREAD)
#include <pthread.h>
#elif defined(USE_PPTHREAD) && !defined(psos)

    struct __pthread_mutex_t
    {
        char mutexname[64];
        HANDLE hMutex;
        int status;
    };

    struct __pthread_cond_t
    {
        char condname[64];
        HANDLE hEvent;
        int status;
    };

#define PTHREAD_ONCE_INIT 0

#define PTHREAD_PROCESS_SHARED 1

#define PTHREAD_SCOPE_SYSTEM 0x01
#define PTHREAD_CREATE_DETACHED 0x04

// linux에서 ppthread를 사용하는 경우
// pthread.h를 include 한다.. -_-
#if !defined(linux)
    typedef DWORD pthread_t;
    typedef struct __pthread_cond_t pthread_cond_t;

    typedef struct __pthread_mutex_t pthread_mutex_t;

    typedef DWORD pthread_attr_t;
    typedef int pthread_key_t;
    typedef DWORD pthread_once_t;
    typedef DWORD pthread_mutexattr_t;
    typedef DWORD pthread_condattr_t;
#endif

#elif !defined(WIN32)
#include <pthread.h>
#endif

    extern pthread_once_t __once_init__;

#ifdef USE_PPTHREAD
    __DECL_PREFIX int ppthread_create(pthread_t * tid,
            const pthread_attr_t * attr, void *(*start) (void *), void *arg);
    __DECL_PREFIX int ppthread_kill(pthread_t thread, int sig);
    __DECL_PREFIX pthread_t ppthread_self(void);
    __DECL_PREFIX void ppthread_exit(void *retval);
    __DECL_PREFIX int ppthread_attr_setscope(pthread_attr_t * attr, int scope);
    __DECL_PREFIX int ppthread_attr_init(pthread_attr_t * attr);
    __DECL_PREFIX int ppthread_attr_setdetachstate(pthread_attr_t * attr,
            int detachstate);
    __DECL_PREFIX int ppthread_attr_setstacksize(pthread_attr_t * attr,
            size_t stacksize);
    __DECL_PREFIX int ppthread_setconcurrency(int new_level);
    __DECL_PREFIX int ppthread_join(pthread_t thread, void **value_ptr);
    __DECL_PREFIX int ppthread_mutexattr_init(pthread_mutexattr_t * attr);
    __DECL_PREFIX int ppthread_mutex_init(pthread_mutex_t * mutex,
            const pthread_mutexattr_t * mutexattr);
    __DECL_PREFIX int ppthread_mutexattr_setpshared(pthread_mutexattr_t * attr,
            int pshared);
    __DECL_PREFIX int ppthread_mutex_destroy(pthread_mutex_t * mutex);
    __DECL_PREFIX int ppthread_mutex_lock(pthread_mutex_t * mutex);
    __DECL_PREFIX int ppthread_mutex_unlock(pthread_mutex_t * mutex);
    __DECL_PREFIX int ppthread_cond_init(pthread_cond_t * cond,
            const pthread_condattr_t * attr);
    __DECL_PREFIX int ppthread_condattr_init(pthread_condattr_t * attr);
    __DECL_PREFIX int ppthread_condattr_setpshared(pthread_condattr_t * attr,
            int pshared);
    __DECL_PREFIX int ppthread_cond_wait(pthread_cond_t * cond,
            pthread_mutex_t * mutex);
    __DECL_PREFIX int ppthread_cond_timedwait(pthread_cond_t * cond,
            pthread_mutex_t * mutex, const struct db_timespec *abstime);
    __DECL_PREFIX int ppthread_cond_signal(pthread_cond_t * cond);
    __DECL_PREFIX int ppthread_cond_broadcast(pthread_cond_t * cond);
    __DECL_PREFIX int ppthread_cond_destroy(pthread_cond_t * cond);
    __DECL_PREFIX int ppthread_key_create(pthread_key_t * key,
            void (*destructor) (void *));
    __DECL_PREFIX int ppthread_key_delete(pthread_key_t key);
    __DECL_PREFIX int ppthread_once(pthread_once_t * once_control,
            void (*init_routine) (void));
#else                           /* !USE_PPTHREAD */

#define ppthread_create                 pthread_create
#define ppthread_kill                   pthread_kill
#define ppthread_self                   pthread_self
#define ppthread_exit                   pthread_exit
#define ppthread_attr_setscope          pthread_attr_setscope
#define ppthread_attr_init              pthread_attr_init
#define ppthread_attr_setdetachstate    pthread_attr_setdetachstate
#define ppthread_attr_setstacksize      pthread_attr_setstacksize
#define ppthread_setconcurrency         pthread_setconcurrency
#define ppthread_join                   pthread_join
#define ppthread_mutexattr_init         pthread_mutexattr_init
#define ppthread_mutex_init             pthread_mutex_init
#define ppthread_mutexattr_setpshared   pthread_mutexattr_setpshared
#define ppthread_mutex_destroy          pthread_mutex_destroy
#define ppthread_mutex_lock             pthread_mutex_lock
#define ppthread_mutex_unlock           pthread_mutex_unlock
#define ppthread_cond_init              pthread_cond_init
#define ppthread_condattr_init          pthread_condattr_init
#define ppthread_condattr_setpshared    pthread_condattr_setpshared
#define ppthread_cond_wait              pthread_cond_wait
#define ppthread_cond_timedwait         pthread_cond_timedwait
#define ppthread_cond_signal            pthread_cond_signal
#define ppthread_cond_broadcast         pthread_cond_broadcast
#define ppthread_cond_destroy           pthread_cond_destroy
#define ppthread_key_create             pthread_key_create
#define ppthread_key_delete             pthread_key_delete
#define ppthread_once                   pthread_once

#endif                          /* !USE_PPTHREAD */

#ifdef  __cplusplus
}
#endif

#endif                          //__PPTHREADNT_H
