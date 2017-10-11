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

#ifndef SERVER_H
#define SERVER_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include "mdb_config.h"
#include "mdb_compat.h"
#include "mdb_ppthread.h"
#include "mdb_inner_define.h"
#include "mdb_collation.h"

#include "mdb_ContCat.h"
#include "mdb_IndexCat.h"
#include "mdb_LogAnchor.h"
#include "mdb_er.h"

#define MAX_SERIALNUM 999999

    typedef struct DB_Parameters
    {
        int max_segments;       /* shared */
        int mem_segments;       /* shared */
        int mem_segments_indexing;      /* shared */
        int low_mem_segments;
        int memory_commit;      /* shared */
        int lock_timeout;       /* shared */
        int use_logging;        /* shared */
        int prm_no_recovery_mode;       /* 0 or 1 */
        int use_shm_key;        /* all threads & clients */
        int log_file_size;
        int log_buffer_size;
        int max_logfile_num;
        char log_path[MDB_FILE_NAME * 2];
        int num_candidates;     /* the number of condidates for variable record */
        int default_fixed_part;
        int serverlog_level;
        int serverlog_size;
        int serverlog_num;
        char serverlog_path[MDB_FILE_NAME * 2];
        int db_write_protection;
        int db_lock_sleep_time;
        int db_lock_sleep_time2;
        int db_start_recovery;
        int num_flush_page_commit;
        MDB_COL_TYPE default_col_char;
        MDB_COL_TYPE default_col_nchar;
        int debug_log;
        int skip_index_check;
        int resident_table_limit;
        int check_free_space;
        int kick_dog_page_count;
    } DB_PARAMS;

#define SERVER_STATUS_NORMAL    1
#define SERVER_STATUS_RECOVERY  2
#define SERVER_STATUS_INDEXING  3
#define SERVER_STATUS_SHUTDOWN  4

#define MDB_IS_SHUTDOWN_MODE() \
    (server == NULL || server->status == SERVER_STATUS_SHUTDOWN)

    struct Server
    {
        char db_name_[MAX_DB_NAME];
        volatile int status;
        volatile int server_pid;
        volatile int server_uid;
        volatile int server_gid;
        volatile int fTerminated;
        volatile int gDirtyBit;
        volatile int del_file_No;
        volatile int fs_full;
        volatile DB_UINT16 _g_qsid;
        int start_time;
        volatile int f_checkpointing;
        DB_PARAMS shparams;

        DB_INT32 nextSequenceId;

        DB_INT32 numLoadSegments;

        int _mdb_db_lock_depth;
        MDB_PID _mdb_db_owner_pid;
        MDB_PID gEDBTaskId_pid;
#ifdef WIN32
        MDB_PID _mdb_db_owner_thrid;
        MDB_PID gEDBTaskId_thrid;
#else
        pthread_t _mdb_db_owner_thrid;
        pthread_t gEDBTaskId_thrid;
#endif
        DB_BYTE f_page_list_updated;
        struct TempContainerCatalog temp_container_catalog;
        struct TempIndexCatalog temp_index_catalog;
        unsigned int temp_tableid;
        struct LogAnchor anchor_bk;
    };

#define INIT_gDirtyBit 2        /* for ping-pong checkpoint */

    extern __DECL_PREFIX struct Server *server; /* Speed.c */
    extern __DECL_PREFIX DB_PARAMS DB_Params;   /* mdb_configure.c */

/* DB configuration variables.
 * Refer openml.cfg and mdb_configure.c for initial values .*/

    extern char log_path[];

    extern pthread_mutexattr_t  _mutex_attr;
    extern pthread_condattr_t   _cond_attr;

#ifdef  __cplusplus
}
#endif

#endif

#include "mdb_Cont.h"
#include "mdb_ContCat.h"
#include "mdb_ContIter.h"
#include "mdb_Csr.h"
#include "mdb_dbi.h"
#include "ErrorCode.h"
#include "mdb_FieldDesc.h"
#include "mdb_Filter.h"
#include "mdb_IndexCat.h"
#include "mdb_Iterator.h"
#include "mdb_LogMgr.h"
#include "mdb_Mem_Mgr.h"
#include "mdb_Page.h"
#include "mdb_ppthread.h"
#include "mdb_Recovery.h"
#include "mdb_syslog.h"
#include "systable.h"
#include "mdb_Trans.h"
#include "mdb_TTree.h"
#include "mdb_TTreeIter.h"
#include "mdb_typedef.h"
#include "mdb_comm_stub.h"
#include "mdb_memmgr.h"
#include "mdb_LockMgrPI.h"
#include "mdb_LogRec.h"
#include "mdb_schema_cache.h"
#include "mdb_charset.h"
