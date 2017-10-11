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
#include "mdb_ppthread.h"
#include "mdb_Server.h"
#include "../../version_mmdb.h"

#include "sql_packet_format.h"

#include "mdb_PMEM.h"

#define GET_PTHREAD_SIGMASK     0

#if defined(linux) && !defined(ANDROID_OS)
extern __sighandler_t sigset(int __sig, __sighandler_t __disp);
#elif defined(linux) && defined(ANDROID_OS)
#define sigset  signal
#endif

static int init_count = 0;

int hostid_check(void);

void _time_init(void);
int Init_Socket(void);
extern int InitializeClient(void);

int Conf_db_parameters(char *dbfullname);

int DBFile_GetVolumeSize(int ftype);

int Cont_TablesConsistency(void);
void DestroyClient(void);       /* for clear sql client struct */

extern int print_anchor_freepagelist(int flag);
int DBFile_AddVolume(int ftype, int num_data_segs);


__DECL_PREFIX struct MemoryMgr *Mem_mgr = NULL;
__DECL_PREFIX struct CursorMgr *Cursor_Mgr = NULL;
__DECL_PREFIX struct IndexCatalog *index_catalog = NULL;
__DECL_PREFIX struct TempIndexCatalog *temp_index_catalog = NULL;

__DECL_PREFIX struct Server *server = NULL;

#define DEFAULTACTTHR 4
#define DEFAULTDEADLOCK_INTERVAL 30
#define DEFAULTCHKPT_INTERVAL 10        //30

extern __DECL_PREFIX DB_UINT16 _g_qsid;

int action_processing_thread_no = DEFAULTACTTHR;

char config_path[MDB_FILE_NAME] = "\0";

static char curr_dbfilename[MAX_DB_NAME] = "\0";

int fRecovered = 0;             /* recovery 대상이 있었는지 나타냄 */

/* log에서 checkpoint가 제대로 끝났는지를 파악함
 * 0: begin chkpnt 만 존재. end chkpnt 없음.
 * 1: begin~index chkpnt로 종료. 이후 다른 log 없음.
 *    제거. index chkpnt 제거.
 * 2: begin~index~end chkpnt로 종료. 이후 다른 log 없음.
 * 3: begin~index~end chkpnt로 종료. 이후 다른 log 있음.
 * These are defined in mdb_Recovery.h
*/

int f_checkpoint_finished = CHKPT_BEGIN;
int LogMgr_Scan_CommitList(void);
int LogMgr_Delete_CommitList(void);
int Check_Index_Item_Size(void);

int shared_struct_init(int shmkey, int connid, int f_init);
int shared_struct_free(void);

pthread_mutexattr_t _mutex_attr;
pthread_condattr_t _cond_attr;

static void Make_File_DBNAME(char *dbpath);
static void Remove_File_PID_DBNAME(char *dbpath);

/*****************************************************************************/
//! Server_Down 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param exitcode :
 *        0: 정상 shutdown
 *       -1: startup fail
 *      100: db가 바뀌는 경우
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX void
Server_Down(int exitcode)
{
    MDB_DB_LOCK();

#ifdef MDB_DEBUG
    __SYSLOG("Server Down: clients %d remained..\n", init_count - 1);
#endif
    if (init_count - 1 > 0)
    {
        _Checkpointing(0, 1);
        init_count--;

        MDB_DB_UNLOCK();

        return;
    }
    if (curr_dbfilename[0] == '\0')
    {
        MDB_DB_UNLOCK();

        return;
    }

    if (server != NULL && curr_dbfilename[0] != '\0')
    {
        server->status = SERVER_STATUS_SHUTDOWN;
        __SYSLOG("Checkpointing... Please wait...\n");
        MDB_DB_LOCK();
        /* page(0,0) flush를 위하여 */
        SetDirtyFlag(PAGE_HEADER_SIZE);
        _Checkpointing(0, 1);   /* 0: not slow checkpoint */
        MDB_DB_UNLOCK();
    }

    DestroyClient();

    if (curr_dbfilename[0] != '\0')
        Remove_File_PID_DBNAME(curr_dbfilename);

    if (server != NULL)
    {
        CurMgr_CursorTableFree();
        MDB_SYSLOG(("Cursor table free OK\n"));

        LogMgr_Free();
        MDB_SYSLOG(("log manager free OK\n"));

        Trans_Mgr_Free();
        MDB_SYSLOG(("transaction manager free OK\n"));

        LockMgr_Free();
        MDB_SYSLOG(("lock manager free OK\n"));

        Cont_FreeHashing();
        MDB_SYSLOG(("catalog hashing free OK\n"));

        MDB_DB_LOCK();
        _Schema_Cache_Free(SCHEMA_LOAD_ALL);
        MDB_DB_UNLOCK();

        IndexCatalogServerDown();
        ContCatalogServerDown();

        MDB_MemoryFree(TRUE);
        MDB_SYSLOG(("DB memory freeOK\n"));

        DBFile_Free();

        shared_struct_free();
        MDB_SYSLOG(("shared memory freeOK\n"));
    }

    if (exitcode != 100 && curr_dbfilename[0] != '\0')
        __SYSLOG2_TIME(" ##### MobileDBS Down ##### \n");

    curr_dbfilename[0] = '\0';
    log_path[0] = '\0';

    __SYSLOG_close();

    MDB_DB_UNLOCK();
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
/* f_state: 0 - client/library
            1 - DB server
            2 - for createdb 
 */

int G_f_state;

/*****************************************************************************/
//! SpeedMain 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param dbname(in)     :
 * \param f_state(in)    : 0-client/library, 1-DB server, 2-for createdb
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  f_state: 0 - client/library
             1 - DB server
             2 - for createdb 
 *****************************************************************************/
__DECL_PREFIX int
SpeedMain(char *p_dbname, int f_state, int connid)
{
    int db_index_num;
    char db_path[MDB_FILE_NAME];
    char dbfilename[MDB_FILE_NAME];
    char dbfullpath[MDB_FILE_NAME];
    int ret;

#if defined(MDB_DEBUG)
    extern int MAX_CONNECTION;
#endif

    int index_file_size = 0;
    char dbname[MDB_FILE_NAME];
    int f_check_syssegment = 0;
    int _g_connid_bk = -2;

    sc_strncpy(dbname, p_dbname, MDB_FILE_NAME - 1);
    dbname[MDB_FILE_NAME - 1] = '\0';

    convert_dir_delimiter(dbname);

    MDB_DB_LOCK();

    G_f_state = f_state;

    if (dbname == NULL || dbname[0] == '\0')
    {
        __SYSLOG2("Error: Database name does not specified.\n");
        ret = DB_E_DBNAMENOTSPECIFIED;
        goto end;
    }

    ret = __check_dbname(dbname, dbfilename);
    if (ret < 0)
        goto end;

    sc_sprintf(dbfullpath, "%s" DIR_DELIM "%s",
            sc_getenv("MOBILE_LITE_CONFIG"), dbfilename);

#ifdef MDB_DEBUG
    if (connid < 0 || connid >= MAX_CONNECTION)
        sc_assert(0, __func__, __LINE__);
#endif

    /* 이전에 connect 되어 open한 적이 있는지 검사 */
    if (curr_dbfilename[0] != '\0')
    {
        if (!mdb_strcasecmp(curr_dbfilename, dbfullpath))
        {
            ret = shared_struct_init(0, connid, 0);
            goto end;
        }
        else    // db가 바뀌는 경우
        {
            Server_Down(100);   /* LIB의 경우 점검 */
            curr_dbfilename[0] = '\0';
        }
    }
    Conf_db_parameters(dbfullpath);

    if (f_state != STATE_CREATEDB)
    {
        if (sc_getenv("MOBILE_LITE_CONFIG") == NULL)
        {
            __SYSLOG2("ERROR: Enviroment Variable ");
            __SYSLOG2("MOBILE_LITE_CONFIG' not set..\n");
            ret = DB_E_ENVIRONVAR;
            goto end;
        }

        __SYSLOG("\n========================================"
                "========================================\n\n");

        __SYSLOG("%s %s", _DBNAME_, _DBVERSION_);

#ifdef MDB_DEBUG
        __SYSLOG(" - DEBUG MODE\n");
#else
        __SYSLOG("\n");
#endif

        __SYSLOG("--------------------------------------------------\n");
        __SYSLOG("%s\n\n", _COPYRIGHT_);
        __SYSLOG("[Compilation Time: %s %s]\n", __DATE__, __TIME__);

        __SYSLOG_TIME("\n");
    }

    __SYSLOG_close();

    if (f_state == STATE_CREATEDB)
    {
        DB_Params.use_shm_key = 0;
        DB_Params.prm_no_recovery_mode = 0;
        DB_Params.db_start_recovery = 1;
        DB_Params.num_flush_page_commit = 0;
    }

    if (sc_getenv("MOBILE_LITE_CONFIG") == NULL)
    {
        __SYSLOG2
                ("ERROR: The MOBILE_LITE_CONFIG environment variable not set.");
        ret = DB_E_ENVIRONVAR;
        goto end;
    }

    if (sc_strlen(sc_getenv("MOBILE_LITE_CONFIG")) < MDB_FILE_NAME - 1)
    {

    }
    else
    {
        __SYSLOG2("ERROR: MOBILE_LITE_CONFIG length too long.\n");
        ret = DB_E_ENVIRONVAR;
        goto end;
    }

    if (sc_strlen(sc_getenv("MOBILE_LITE_CONFIG")) + 6 < MDB_FILE_NAME - 1)
    {
        int fd;
        char filename[256];

        sc_strcpy(db_path, sc_getenv("MOBILE_LITE_CONFIG"));

        /* db path와 db가 있는지 점검 */
        sc_sprintf(filename, "%s" DIR_DELIM "%s_data.00", db_path, dbfilename);
        fd = sc_open(filename, O_RDONLY, OPEN_MODE);
        if (fd < 0)
        {
            __SYSLOG2("\nError: Check DB path, %s\n\n", db_path);
            ret = DB_E_INVALIDDBPATH;
            goto end;
        }

        sc_close(fd);
    }
    else
    {
        __SYSLOG2("ERROR: MOBILE_LITE_CONFIG length too long.\n");
        ret = DB_E_ENVIRONVAR;
        goto end;
    }

    ret = shared_struct_init(DB_Params.use_shm_key, connid, 1);
    if (ret < 0)
        goto end;

    server->start_time = (int) sc_time(NULL);
    server->server_uid = 1;
    server->server_gid = 1;
    server->fTerminated = FALSE;
    server->fs_full = 0;
    server->server_pid = sc_getpid();
    server->f_checkpointing = 0;

    /* FIX_qsid_value_between_anthor_connection */
    {
        struct db_timeval tp;

        sc_gettimeofday(&tp, NULL);
        _g_qsid = server->_g_qsid = tp.tv_usec % 65536;
    }

    sc_memcpy(server->db_name_, dbfilename, MAX_DB_NAME - 1);
    server->db_name_[MAX_DB_NAME - 1] = '\0';
    server->status = SERVER_STATUS_NORMAL;

    sc_strcpy(config_path, sc_getenv("MOBILE_LITE_CONFIG"));

    if (DB_Params.max_segments == 0)
        __SYSLOG("CONF. Max DB Size: unlimited\n");
    else
    {
        /* KB 단위 지원 */
        if (DB_Params.max_segments * SEGMENT_SIZE < 1024 * 1024)
            __SYSLOG("CONF. Max DB Size: %d KB\n",
                    (DB_Params.max_segments * SEGMENT_SIZE) / 1024);
        else
            __SYSLOG("CONF. Max DB Size: %d MB\n",
                    (DB_Params.max_segments * SEGMENT_SIZE) / 1024 / 1024);
    }

    if (DB_Params.mem_segments == 0)
        __SYSLOG("CONF. Buffer Size: unlimited\n");
    {
        if (SEGMENT_SIZE < 1024 * 1024)
            __SYSLOG("CONF. Buffer Size: %d KB\n",
                    DB_Params.mem_segments * (SEGMENT_SIZE / 1024));
        else
            __SYSLOG("CONF. Buffer Size: %d MB\n",
                    DB_Params.mem_segments * (SEGMENT_SIZE / 1024 / 1024));
    }

    if (DB_Params.db_start_recovery == 0)
        __SYSLOG("Skip start recovery\n");

#ifdef MDB_DEBUG
    __SYSLOG("Volume Info.\n");
    __SYSLOG("  SEGMENT_NO: %d\n", SEGMENT_NO);
    __SYSLOG("  TEMPSEG_BASE_NO: %d\n", TEMPSEG_BASE_NO);
    __SYSLOG("  TEMPSEG_END_NO: %d\n", TEMPSEG_END_NO);
    __SYSLOG("  data space: %d segments, %d MB\n",
            TEMPSEG_BASE_NO - 1,
            ((TEMPSEG_BASE_NO - 1) * SEGMENT_SIZE) / 1024 / 1024);
    __SYSLOG("  temp space: %d segments, %d MB\n",
            TEMPSEG_END_NO - TEMPSEG_BASE_NO + 1,
            ((TEMPSEG_END_NO - TEMPSEG_BASE_NO +
                            1) * SEGMENT_SIZE) / 1024 / 1024);
    __SYSLOG("  index space: %d segments, %d MB\n",
            SEGMENT_NO - TEMPSEG_END_NO - 1,
            ((SEGMENT_NO - TEMPSEG_END_NO - 1) * SEGMENT_SIZE) / 1024 / 1024);
#endif

    MDB_DB_LOCK();
    PUSH_G_CONNID(connid);
    ret = TransMgr_Init();
    POP_G_CONNID();
    MDB_DB_UNLOCK();

    if (ret < 0)
    {
        __SYSLOG2("Transaction Manager Init FAIL\n");
        goto end;
    }
    else
        __SYSLOG("Transaction Manager Init OK\n");

    ret = LogMgr_init(dbfilename);
    if (ret < 0)
    {
        __SYSLOG2("Log Manager Init FAIL \n");
        goto end;
    }
    else
    {
        MDB_SYSLOG(("Log Manager Init OK\n"));
    }

    db_index_num = LogMgr_Get_DB_Idx_Num();

    if (sc_strlen(db_path) + sc_strlen(dbfilename) + 6 >= (MDB_FILE_NAME - 1))
    {
        __SYSLOG2("ERROR database name PATH VERY LONG");
        ret = DB_E_INVALIDLONGDBPATH;
        goto end;
    }

    ret = DBFile_Allocate(dbfilename, db_path);
    if (ret < 0)
    {
        __SYSLOG2("DBFile Allocate FAIL\n");
        goto end;
    }
    else
    {
        MDB_SYSLOG(("DB File Allocate OK\n"));
    }

    __SYSLOG("DB file path: %s\n", db_path);

    ret = MemoryMgr_initFromDBFile(db_index_num);
    if (ret < 0)
    {
        __SYSLOG2("MemoryMgr initialize ERROR (%d)\n", ret);
        goto end;
    }

    /* cursor 구조 초기화 */
    ret = CurMgr_CursorTableInit();
    if (ret < 0)
    {
        __SYSLOG2("Cursor Manager Init FAIL \n");
        goto end;
    }

    if (ContCatalogServerInit() == NULL)
    {
        __SYSLOG2("container allocate FAIL \n");
        ret = DB_E_CONTCATINIT;
        goto end;
    }

    index_catalog = (struct IndexCatalog *) IndexCatalogServerInit();
    if (index_catalog == NULL)
    {
        __SYSLOG2(" index container allocate FAIL \n");
        ret = DB_E_IDXCATINIT;
        goto end;
    }

    if (mem_anchor_->supported_segment_no_ != SEGMENT_NO)
    {
        /* 현재 db file의 SEGMENT_NO와 현재 db engine의 SEGMENT_NO가
         * 다른 경우 index를 새로 생성하도록 한다.*/
        __SYSLOG("Index rebuild due to SEGMENT_NO changed(%d --> %d)\n",
                mem_anchor_->supported_segment_no_, SEGMENT_NO);
        f_checkpoint_finished = CHKPT_BEGIN;
        fRecovered = ALL_REBUILD;
        mem_anchor_->supported_segment_no_ = SEGMENT_NO;
        SetDirtyFlag(0);

        LogMgr_Scan_CommitList();
        LogMgr_Delete_CommitList();
    }
    else if (f_state != STATE_CREATEDB)
    {
        if (DB_Params.db_start_recovery == 0)
            goto next;

        f_checkpoint_finished = CHKPT_BEGIN;

        if (LogMgr_Scan_CommitList() < 0)
            f_checkpoint_finished = CHKPT_BEGIN;


        if (f_checkpoint_finished)
        {
            if (Check_Index_Item_Size() == DB_SUCCESS)
            {
                int ialloced_segment_size;

                ialloced_segment_size =
                        mem_anchor_->iallocated_segment_no_ * SEGMENT_SIZE;

                if (index_file_size == -1)
                {
                    f_checkpoint_finished = CHKPT_BEGIN;
                    fRecovered = ALL_REBUILD;
                }
                else
                {
                    index_file_size = DBFile_GetVolumeSize(DBFILE_INDEX_TYPE);
                    if (index_file_size <= 0 ||
                            index_file_size < ialloced_segment_size ||
                            Index_MemoryMgr_initFromDBFile() != DB_SUCCESS)
                    {
                        f_checkpoint_finished = CHKPT_BEGIN;
                        fRecovered = ALL_REBUILD;
                    }
                }
            }
            else
            {
                f_checkpoint_finished = CHKPT_BEGIN;
                fRecovered = ALL_REBUILD;
            }
        }

        if (f_checkpoint_finished == CHKPT_BEGIN)
            LogMgr_Delete_CommitList();
    }

  next:
    if (DB_Params.db_start_recovery == 0)
    {
        f_checkpoint_finished = CHKPT_CMPLT;
        fRecovered = ALL_REBUILD;
    }

    f_check_syssegment = 0;
    if (f_checkpoint_finished)
    {
        f_check_syssegment = 1;
    }

    if (DB_Params.prm_no_recovery_mode == 0)
    {
        MDB_DB_LOCK();
        PUSH_G_CONNID(connid);

        server->status = SERVER_STATUS_RECOVERY;
        ret = LogMgr_Restart();
        server->status = SERVER_STATUS_NORMAL;
        if (ret < 0 || (fRecovered & ALL_REBUILD))
        {
            Index_MemoryMgr_FreeSegment();
            f_checkpoint_finished = CHKPT_BEGIN;
            fRecovered = ALL_REBUILD;
        }

        POP_G_CONNID();
        MDB_DB_UNLOCK();
    }

    /* index를 전체 새로 만들어야 하는 경우라면(f_checkpoint_finished ==
     * CHKPT_BEGIN)
     * index segment 제거하고(초기화) index file을 지우고, checkpoint를
     * 수행하여 필요없는 log file들을 제거하도록 처리한다. 이렇게 하지
     * 않으면 index 재생성 중에 checkpointing으로 begin_chkpt와 end_check
     * log들이 계속해서 쌓아게 되어 log file들이 계속해서 늘어날 수 있다. */
    if (f_checkpoint_finished == CHKPT_BEGIN)
    {
        char fname[MDB_FILE_NAME];
        int ffd;
        int i;

        for (i = 0; i < MAX_FILE_ID; ++i)
        {
            if (Mem_mgr->database_file_.file_id_[DBFILE_INDEX_TYPE][i] == -1)
            {
                break;
            }

            sc_close(Mem_mgr->database_file_.file_id_[DBFILE_INDEX_TYPE][i]);
            Mem_mgr->database_file_.file_id_[DBFILE_INDEX_TYPE][i] = -1;
        }

        Index_MemoryMgr_FreeSegment();

        sc_sprintf(fname, "%s0",
                Mem_mgr->database_file_.file_name_[DBFILE_INDEX_TYPE]);
        ffd = sc_open(fname, O_RDONLY, OPEN_MODE);
        if (ffd >= 0)
        {
            sc_close(ffd);
            sc_unlink(fname);
        }

        mem_anchor_->iallocated_segment_no_ = 0;
        mem_anchor_->ifirst_free_page_id_ = 0;
        mem_anchor_->ifree_page_count_ = 0;

        server->gDirtyBit = INIT_gDirtyBit;
        _Checkpointing(0, 1);
    }

    /* If max_logfile_num is specified and the number of remained log files
     * is greater than the specified number, then all useless logfiles are
     * deleted without respect to xla_lsn. After that, xla_lsn is set to
     * {0, 0} to make FTS do the full indexing. */
    if (server->shparams.max_logfile_num &&
            Log_Mgr->File_.File_No_ -
            (Log_Mgr->Anchor_.Last_Deleted_Log_File_No_ + 1) + 1
            > server->shparams.max_logfile_num)
    {
        void Delete_Useless_Logfiles(void);

        MDB_SYSLOG(("Too many logfiles, %d. Removed\n",
                        Log_Mgr->File_.File_No_ -
                        (Log_Mgr->Anchor_.Last_Deleted_Log_File_No_ + 1) + 1));

        SetDirtyFlag(PAGE_HEADER_SIZE);

        _Checkpointing(0, 1);

        Delete_Useless_Logfiles();
    }

    /* If the last checkpoint is not completed or there are some log records
       related page allocation/deallocate,
       do check the invalidness of free page list. */
    if (f_checkpoint_finished < CHKPT_CMPLT ||
            server->f_page_list_updated == 1)
    {
        extern int MemMgr_Set_LastFreePageID(void);

        ret = MemMgr_Set_LastFreePageID();
        if (ret < 0)
        {
            __SYSLOG2("FAIL: MemMgr_Set_LastFreePageID %d\n", ret);
            goto end;
        }
    }

#ifdef CHECK_INDEX_VALIDATION
    Index_Check_Page();
#endif


    LogMgr_Delete_CommitList();

    if (DB_Params.db_start_recovery == 1 && f_checkpoint_finished)
    {
        extern int ContCat_Index_Check(void);
        extern int Index_MemoryMgr_FreeSegment(void);

        if (index_file_size && ContCat_Index_Check() < 0)
        {
            Index_MemoryMgr_FreeSegment();
            f_checkpoint_finished = CHKPT_BEGIN;
            fRecovered = ALL_REBUILD;
        }
    }

    /* db file size 점검 */
    ret = DBFile_GetVolumeSize(DBFILE_DATA0_TYPE);
    if (ret < (int) (mem_anchor_->allocated_segment_no_ * SEGMENT_SIZE))
    {
        __SYSLOG2("DB file size (%d) less than no. segment (%d, %d) \n",
                ret, mem_anchor_->allocated_segment_no_,
                mem_anchor_->allocated_segment_no_ * SEGMENT_SIZE);
        ret = DBFile_GetLastSegment(DBFILE_DATA0_TYPE, ret);
        if (ret <= 0)
        {
            __SYSLOG2(" FAIL: setting the number of segments\n");
            ret = DB_E_DBFILESIZE;
            goto end;
        }
        mem_anchor_->allocated_segment_no_ = ret + 1;
        __SYSLOG2("Setting the number of segments : %d\n\n",
                mem_anchor_->allocated_segment_no_);
    }

    if (f_check_syssegment == 0 || fRecovered)
    {
        f_check_syssegment = 1;
    }

#ifdef CHECK_INDEX_VALIDATION
    Index_Check_Page();
#endif

    /* index rebuild 하는 동안 buffer flush가 발생이 되는데
     *      * 이 때는 전체 buffer를 내리기 위해서 0으로 설정 함 */
    {
        int buf_low_size = server->shparams.low_mem_segments;

        server->shparams.low_mem_segments = 0;

        ret = Index_MemoryMgr_init(0);

        server->shparams.low_mem_segments = buf_low_size;

        if (ret < 0)
        {
            __SYSLOG2("INDEX Memory Manager initialize ERROR\n");
            goto end;
        }
    }

#ifdef MDB_DEBUG
    {
        extern int ContCat_Index_Check(void);

        fRecovered = 0;

        if (ContCat_Index_Check() < 0)
        {
            __SYSLOG_syncbuf(0);
            sc_assert(0, __FILE__, __LINE__);
        }

        if (fRecovered != 0)
        {
            __SYSLOG_syncbuf(0);
            sc_assert(0, __FILE__, __LINE__);
        }
    }
#endif

    if (f_checkpoint_finished != CHKPT_CMPLT)
    {
        extern int ContCat_Container_Check(void);

        ret = ContCat_Container_Check();
        if (ret < 0)
        {
            __SYSLOG_syncbuf(0);
            goto end;
        }
    }

#ifdef CHECK_INDEX_VALIDATION
    Index_Check_Page();
#endif

    /* index rebuild */
    /* Index_MemoryMgr_init() 내부로... */

    PUSH_G_CONNID(connid);
    ret = Cont_CreateHashing();
    POP_G_CONNID();
    if (ret < 0)
    {
        __SYSLOG2("FAIL: cont hashing\n");
        goto end;
    }

    ret = MemMgr_Check_FreeSegment();
    if (ret < 0)
    {
        __SYSLOG2("FAIL: free segment allocation\n");
        goto end;
    }

    if (f_state != STATE_CREATEDB)
    {
        PUSH_G_CONNID(connid);
        ret = Cont_TablesConsistency();
        POP_G_CONNID();
        if (ret < 0)
        {
            __SYSLOG2("FAIL: table consistency\n");
            goto end;
        }
    }

    LockMgr_Init();

    /* time 변환 함수 관련 초기화 */
    _time_init();

    /* 스키마 caching */
    {
        int schema_load_flag = SCHEMA_LOAD_ALL; /* default */

        /* schema cache initialize */
        if (f_state == STATE_CREATEDB)
            schema_load_flag = 0;

        MDB_DB_LOCK();
        PUSH_G_CONNID(connid);
        ret = _Schema_Cache_Init(schema_load_flag);
        POP_G_CONNID();
        MDB_DB_UNLOCK();

        if (ret < 0)
        {
            __SYSLOG2("Error: schema initializeation...\n");
            goto end;
        }
    }

#ifdef CHECK_INDEX_VALIDATION
    Index_Check_Page();
#endif

    Cont_RemoveTempTables(-1);

#ifdef CHECK_INDEX_VALIDATION
    Index_Check_Page();
#endif

    if (mem_anchor_->idxseg_num)
    {
        int file_size;

        file_size = DBFile_GetVolumeSize(DBFILE_INDEX_TYPE);

        if ((file_size / SEGMENT_SIZE) < mem_anchor_->idxseg_num)
        {
            DBFile_AddVolume(DBFILE_INDEX_TYPE,
                    mem_anchor_->idxseg_num - (file_size / SEGMENT_SIZE));
        }
    }

    if (mem_anchor_->tmpseg_num)
    {
        int file_size;

        file_size = DBFile_GetVolumeSize(DBFILE_TEMP_TYPE);

        if ((file_size / SEGMENT_SIZE) < mem_anchor_->tmpseg_num)
        {
            DBFile_AddVolume(DBFILE_TEMP_TYPE,
                    mem_anchor_->tmpseg_num - (file_size / SEGMENT_SIZE));
        }
    }

    /* log가 begin checkpoint ~ end checkpoint로 끝이 난 경우
     * _Checkpointing을 하지 않도록 처리 함 */
    if (f_checkpoint_finished != CHKPT_CMPLT)
    {
        /* page(0,0) flush를 위하여 */
        SetDirtyFlag(PAGE_HEADER_SIZE);

        _Checkpointing(0, 1);   /* 0: not slow checkpoint */
#ifndef _ONE_BDB_
        _Checkpointing(0, 1);
#endif
    }

    if (f_state != STATE_CREATEDB)
    {
        __SYSLOG("##### %s StartUp (%s) #####\n\n", _DBNAME_, dbfilename);
    }

    sc_strcpy(curr_dbfilename, dbfullpath);

    Make_File_DBNAME(dbfullpath);

    if (f_state != STATE_CREATEDB)
    {
        void dbi_Set_SysStatus(int connid);

        dbi_Set_SysStatus(0);
    }

    ret = DB_SUCCESS;

  end:

    /* fix memory-leak */
    LogMgr_Delete_CommitList();

    MDB_DB_UNLOCK();

    __SYSLOG_syncbuf(0);

    if (ret < 0)
    {
        if (server != NULL)
        {
            int _g_connid_bk = -2;

            MDB_DB_LOCK();

            LogMgr_Free();
            Cont_FreeHashing();
            /* DestroyClient uses CS_getspecific. */
            PUSH_G_CONNID(connid) DestroyClient();
            POP_G_CONNID();
            _Schema_Cache_Free(SCHEMA_LOAD_ALL);
            MDB_MemoryFree(TRUE);

            PUSH_G_CONNID(connid);
            shared_struct_free();
            POP_G_CONNID();
            MDB_DB_UNLOCK();
        }
    }

    return ret;
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
int
shared_struct_init(int shmkey, int connid, int f_init)
{
    unsigned int size = 0;
    char *ptr = NULL;
    int ret;

    InitializeClient(); // SQL을 위한 internal structure initialization

    if (server != NULL)
    {
        goto end;
    }

    ppthread_mutexattr_init(&_mutex_attr);
    ppthread_condattr_init(&_cond_attr);

    ret = DBMem_Init(shmkey, &ptr);
    if (ret < 0)
    {
        __SYSLOG2("Fail: Database memory initialization, shmkey=%d\n", shmkey);
        return ret;
    }

    if (f_init == 1)
    {
        size = _alignedlen(sizeof(struct Server), 8);   // 1976
        size += _alignedlen(sizeof(struct MemoryMgr), 8);       // 138296
        size += _alignedlen(sizeof(struct CursorMgr), 8);       //  4176
        size += _alignedlen(sizeof(struct TransMgr), 8);        // 184
        size += _alignedlen(sizeof(struct Trans) * MAX_TRANS, 8);       // 5344
        size += _alignedlen(sizeof(struct LockMgr), 8); // 120
        size += _alignedlen(sizeof(struct LogMgr), 8);  // 1144
        size += _alignedlen(DB_Params.log_buffer_size, 8);      // 65536 (LOG_BUFFER_SIZE)
        size += _alignedlen(sizeof(struct Container_Name_Hash), 8);     // 48
        size += _alignedlen(sizeof(struct Container_Tableid_Hash), 8);  // 48
        size += _alignedlen(sizeof(SCHEMA_CACHE_T), 8); // 13344

        ptr = (char *) DBMem_Alloc(size);
        if (ptr == NULL)
        {
            __SYSLOG2("fail on DB memory Initialization\n");
            return DB_E_DBMEMINIT;
        }

        sc_memset(ptr, 0, size);
    }

    server = (struct Server *) ptr;
    ptr += _alignedlen(sizeof(struct Server), 8);
    Mem_mgr = (struct MemoryMgr *) ptr;
    ptr += _alignedlen(sizeof(struct MemoryMgr), 8);
    Cursor_Mgr = (struct CursorMgr *) ptr;
    ptr += _alignedlen(sizeof(struct CursorMgr), 8);
    trans_mgr = (struct TransMgr *) ptr;
    ptr += _alignedlen(sizeof(struct TransMgr), 8);
    Trans_ = (struct Trans *) ptr;
    ptr += _alignedlen(sizeof(struct Trans) * MAX_TRANS, 8);
    Lock_Mgr = (struct LockMgr *) ptr;
    ptr += _alignedlen(sizeof(struct LockMgr), 8);
    Log_Mgr = (struct LogMgr *) ptr;
    ptr += _alignedlen(sizeof(struct LogMgr), 8);
    if (f_init == 1)
    {
        extern int _mdb_db_owner_pid;
        extern pthread_t _mdb_db_owner_thrid;

        server->_mdb_db_lock_depth = _mdb_db_lock_depth;
        server->_mdb_db_owner_pid = _mdb_db_owner_pid;
        server->_mdb_db_owner_thrid = _mdb_db_owner_thrid;
        server->gEDBTaskId_pid = INIT_gEDBTaskId_pid;
        server->gEDBTaskId_thrid = INIT_gEDBTaskId_thrid;

        LogPage = (char *) ptr;
        ptr += _alignedlen(DB_Params.log_buffer_size, 8);
    }
    else
    {
        LogPage = (char *) ptr;
        ptr += _alignedlen(Log_Mgr->Buffer_.buf_size, 8);
    }

    hash_cont_name = (struct Container_Name_Hash *) ptr;
    ptr += _alignedlen(sizeof(struct Container_Name_Hash), 8);
    hash_cont_tableid = (struct Container_Tableid_Hash *) ptr;
    ptr += _alignedlen(sizeof(struct Container_Tableid_Hash), 8);
    Schema_Cache = (SCHEMA_CACHE_T *) ptr;
    ptr += _alignedlen(sizeof(SCHEMA_CACHE_T), 8);

    if (f_init != 1)
    {
        int sn;

        sc_memset(segment_, 0, sizeof(segment_));

        for (sn = 0; sn < SEGMENT_NO; sn++)
        {
            segment_[sn] =
                    (struct Segment *) DBDataMem_V2P(Mem_mgr->
                    segment_memid[sn]);
        }

        mem_anchor_ = (struct MemBase *) &(segment_[0]->page_[0].body_[0]);

        container_catalog = (struct ContainerCatalog *)
                (segment_[0]->page_[0].body_ + CONTCAT_OFFSET);

        index_catalog = (struct IndexCatalog *)
                (&(segment_[0]->page_[0].body_[0]) + INDEXCAT_OFFSET);

        _time_init();
    }

    if (f_init == 1)
    {
        server->shparams = DB_Params;
    }

    __SYSLOG("shared struct size:%d %d\n", size, sizeof(struct MemoryMgr));

  end:
    fRecovered = 0;

    init_count++;

#ifdef MDB_DEBUG
    __SYSLOG("%d clients connected\n", init_count);
#endif

    return DB_SUCCESS;
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
int
shared_struct_free(void)
{
    init_count--;

    if (init_count == 0)
    {
        DestroyClient();
        DBMem_Free(server);
        server = NULL;

        DBDataMem_All_Free();

        DBMem_AllFree();

        Mem_mgr = NULL;
        Cursor_Mgr = NULL;
        trans_mgr = NULL;
        Trans_ = NULL;
        Lock_Mgr = NULL;
        Log_Mgr = NULL;
        LogPage = NULL;
        hash_cont_name = NULL;
        hash_cont_tableid = NULL;
        Schema_Cache = NULL;
        mem_anchor_ = NULL;
        container_catalog = NULL;
        index_catalog = NULL;
    }

    return DB_SUCCESS;
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
Make_File_DBNAME(char *dbpath)
{
    char pidfile[MDB_FILE_NAME];
    int fd;

    sc_sprintf(pidfile, "%s_mobile_lite.db", dbpath);

    fd = sc_open(pidfile, O_RDWR | O_CREAT | O_BINARY, OPEN_MODE);
    if (fd < 0)
    {
        __SYSLOG2("dbname file open fail \n");
        return;
    }

    sc_write(fd, server->db_name_, sc_strlen(server->db_name_) + 1);
    sc_close(fd);
    return;
}

static void
Remove_File_PID_DBNAME(char *dbpath)
{
    char pidfile[MDB_FILE_NAME];

    sc_sprintf(pidfile, "%s_mobile_lite.db", dbpath);

    if (sc_unlink(pidfile) == -1)
    {
#if defined(WIN32)
        MDB_SYSLOG(("DBName file: %s deleted, %d\n", pidfile, GetLastError()));
#else
        MDB_SYSLOG(("DBName file: %s deleted, %d\n", pidfile, errno));
#endif
    }

    return;
}
