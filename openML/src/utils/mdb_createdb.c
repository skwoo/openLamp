/* 
   This file is part of openML, mobile and embedded DBMS.

   Copyright (C) 2012 Inervit Co., Ltd.
   support@inervit.com

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mdb_config.h"
#include "dbport.h"
#include "mdb_compat.h"
#include "mdb_Segment.h"
#include "mdb_define.h"
#include "mdb_typedef.h"
#include "mdb_Cont.h"
#include "mdb_ContCat.h"
#include "mdb_IndexCat.h"
#include "mdb_TTree.h"
#include "mdb_Mem_Mgr.h"
#include "systable.h"
#include "mdb_Csr.h"
#include "mdb_FieldVal.h"
#include "mdb_FieldDesc.h"
#include "mdb_Filter.h"
#include "mdb_DBFile.h"
#include "mdb_syslog.h"
#include "mdb_LogMgr.h"
#include "mdb_dbi.h"
#include "../../version_mmdb.h"

#if !defined(_UTILITY_)
static int Create_SysTables_SysFields(void);
static int Create_System_Tables(void);
static int Make_Directory(char *dbname);

#if defined(_AS_WINCE_)
static DB_BOOL Drop_DB_tchar(TCHAR * path);
#endif

static char __config_path[MDB_FILE_NAME] = "\0";
static int __config_path_len;
#endif

int fcreatedb = 0;

#if defined(_UTILITY_)
extern __DECL_PREFIX void (*__mdb_syslog) (char *format, ...);
#else
__DECL_PREFIX void (*__mdb_syslog) (char *format, ...) = __SYSLOG;
#endif

__DECL_PREFIX int __createdb(char *p_dbname, DB_CREATEDB * create_options);
int copy_lock_info(struct Server *dst, struct Server *src);

/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param argc(in)    :
 * \param argv(in)   : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
#ifdef _UTILITY_
int
main(int argc, char *argv[])
{
    DB_CREATEDB create_options;
    char *m_arg = NULL;
    int ret;

    sc_memset(&create_options, 0, sizeof(create_options));

    __mdb_syslog = __SYSLOG2;

    __mdb_syslog("\ncreatedb for %s %s\n", _DBNAME_, _DBVERSION_);
    __mdb_syslog("%s\n\n", _COPYRIGHT_);

    if (argc < 2)
    {
        __mdb_syslog("Usage: %s <database name> [No. of segments]\n", argv[0]);
        __mdb_syslog("\tNo. of segments: ");
        if (S_PAGE_SIZE * PAGE_PER_SEGMENT >= 1024 * 1024)
            __mdb_syslog("initial db size in mega bytes, default is %d MB.\n",
                    (S_PAGE_SIZE * PAGE_PER_SEGMENT) / (1024 * 1024));
        else
            __mdb_syslog("initial db size in mega bytes, default is %d KB.\n",
                    (S_PAGE_SIZE * PAGE_PER_SEGMENT) / 1024);
        __mdb_syslog("%s\n",
                "==========================================================");
        return -1;
    }

    if (argc == 2)
        create_options.num_seg = 1;
    else
        create_options.num_seg = sc_atoi(argv[2]);

    m_arg = argv[1];

    sc_db_lock_init();

    ret = __createdb(m_arg, &create_options);

/* XXX: Could have memory leak without routine below,
        but too complicated to fix under current architecture.
        Easy fix is to add __DECL_PREFIX to mdb_hq_destroy(),
        but hmm... */
    return ret;
}
#endif

/* mobile lite에서는 아래부분이 libmobiledbms에 됨 */
#if !defined(_UTILITY_)
/*****************************************************************************/
//! createdb 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param dbname(in)    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX int
createdb(char *dbname)
{
    int ret;
    DB_CREATEDB create_options;

    sc_memset(&create_options, 0, sizeof(create_options));

    fcreatedb = 1;

    sc_db_lock_init();

    sc_memset(&create_options, 0, sizeof(DB_CREATEDB));
    create_options.num_seg = CREATEDB_SEG_UNIT;
    ret = __createdb(dbname, &create_options);

    fcreatedb = 0;

    return ret;
}

/*****************************************************************************/
//! createdb2
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param dbname(in)    :
 * \param num_seg(in)   :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX int
createdb2(char *dbname, int num_seg)
{
    int ret;
    DB_CREATEDB create_options;

    sc_memset(&create_options, 0, sizeof(create_options));

    fcreatedb = 1;

    sc_db_lock_init();

    sc_memset(&create_options, 0, sizeof(DB_CREATEDB));
    create_options.num_seg = num_seg;
    ret = __createdb(dbname, &create_options);

    fcreatedb = 0;

    return ret;
}

__DECL_PREFIX int
createdb3(char *dbname, int num_seg, int idxseg_num, int tmpseg_num)
{
    int ret;
    DB_CREATEDB create_options;

    sc_memset(&create_options, 0, sizeof(create_options));

    fcreatedb = 1;

    sc_db_lock_init();

    sc_memset(&create_options, 0, sizeof(DB_CREATEDB));
    create_options.num_seg = num_seg;
    create_options.idxseg_num = idxseg_num;
    create_options.tmpseg_num = tmpseg_num;
    ret = __createdb(dbname, &create_options);

    fcreatedb = 0;

    return ret;
}

/*****************************************************************************/
//! __createdb
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param p_dbname(in)    :
 * \param num_seg(in)   :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *****************************************************************************/
int DBFile_GetVolumeSize(int ftype);

__DECL_PREFIX int
__createdb(char *p_dbname, DB_CREATEDB * create_options)
{
    DB_UINT32 segment_no;
    char dbfilename[MDB_FILE_NAME];
    int ret = DB_FAIL;
    int connid = -1;
    char dbname[MDB_FILE_NAME];

    struct Server createdb_svr;
    int num_seg = create_options->num_seg;

    if (server != NULL)
        return DB_E_ALREADYCONNECTED;

    if (p_dbname == NULL)
    {
#if defined(linux)
        __mdb_syslog("INTERNAL ERROR: %s: Invalid paramter.\n", __FUNCTION__);
#else
        __mdb_syslog("INTERNAL ERROR: __createdb(): Invalid paramter.\n");
#endif
        return DB_E_INVALIDPARAM;
    }

    /* p_dbname length must between 1 and MAX_DB_NAME */
    if (p_dbname[0] == 0 || sc_strlen(p_dbname) >= MAX_DB_NAME)
    {
        __mdb_syslog("ERROR:  Invalid DB name [%s].\n", p_dbname);
        __mdb_syslog("\tValid DB name length is between 1 and %d.\n",
                MAX_DB_NAME - 1);
        __mdb_syslog("===============================\n");
        return DB_E_INVALIDPARAM;
    }

    __mdb_syslog("\n"
            "========================================"
            "========================================\n");
#ifndef _UTILITY_
    __mdb_syslog("\ncreatedb for %s %s\n", _DBNAME_, _DBVERSION_);
    __mdb_syslog("%s\n\n", _COPYRIGHT_);
#endif

    sc_strncpy(dbname, p_dbname, MDB_FILE_NAME - 1);
    dbname[MDB_FILE_NAME - 1] = '\0';

    convert_dir_delimiter(dbname);

    ret = __check_dbname(dbname, dbfilename);
    if (ret < 0)
        return ret;

    /* check if dbfilename(DB name) starts with alphanumeric
     * character or '_'(underscore). If not, poll an error
     */
    if (!isalnum(dbfilename[0]) && dbfilename[0] != '_')
    {
        __mdb_syslog("ERROR:  Invalid DB name [%s].\n", dbfilename);
        __mdb_syslog("\tDB name must start with alphanumeric character "
                "or underscore.\n");
        return DB_E_INVALIDPARAM;
    }

    ret = sc_strlen(dbfilename);

    if (ret == 0 || ret > MAX_DB_NAME)
    {
        __mdb_syslog("ERROR:  Invalid DB name [%s].\n", dbfilename);
        __mdb_syslog("\tValid DB name length is between 1 and %d.\n",
                MAX_DB_NAME - 1);
        __mdb_syslog("===============================\n");
        return DB_E_INVALIDPARAM;
    }

    segment_no = num_seg;

    // segment_no가 uint 인데, 0보다 작아질 수 없음. 
    if (segment_no == 0 || segment_no >= SEGMENT_NO)
    {
        __mdb_syslog("ERROR:  Invalid segment number   \n");
        __mdb_syslog("\tValid segment number is between 1 and %d.\n",
                SEGMENT_NO - 1);
        return DB_E_INVALIDPARAM;
    }

    /* 설정된 환경변수를 읽어온다 */
    if (sc_getenv("MOBILE_LITE_CONFIG") == NULL)
    {
        __mdb_syslog("MOBILE_LITE_CONFIG Variable No Setting");
        return DB_E_ENVIRONVAR;
    }
    else
    {
        sc_strcpy(__config_path, sc_getenv("MOBILE_LITE_CONFIG"));
    }

    __config_path_len = sc_strlen(__config_path);

    ret = Make_Directory(dbfilename);
    if (ret < 0)
        goto done;

    sc_memset(&createdb_svr, 0, sizeof(createdb_svr));
    server = &createdb_svr;

    CS_Init(5);

    connid = CS_GetCSObject(NULL, dbname, NULL, NULL,
            CS_CONNECT_LIB, CS_CLIENT_NORMAL, -1);
    if (connid < 0)
    {
        ret = connid;
        goto done;
    }

    _g_connid = connid;

    MDB_DB_LOCK();

    ret = Init_BackupFile(__config_path, dbfilename, create_options);
    if (ret < 0)
        goto done;

    server = NULL;

    ret = SpeedMain(dbname, STATE_CREATEDB, connid);
    if (ret < 0)
        goto done;

    // copy lock info from createdb_svr to server
    copy_lock_info(server, &createdb_svr);

#define MINIMUM_SEG 20
    if (segment_no < MINIMUM_SEG)
        segment_no = MINIMUM_SEG;

    dbi_AddSegment(connid, MINIMUM_SEG, 0);

    /* Expand the volume of index and temp files, not segment */
    mem_anchor_->idxseg_num = create_options->idxseg_num;
    mem_anchor_->tmpseg_num = create_options->tmpseg_num;

    create_options->idxseg_num = mem_anchor_->idxseg_num -
            DBFile_GetVolumeSize(DBFILE_INDEX_TYPE) / SEGMENT_SIZE;
    if (create_options->idxseg_num < 0)
        create_options->idxseg_num = 0;

    create_options->tmpseg_num = mem_anchor_->tmpseg_num -
            DBFile_GetVolumeSize(DBFILE_TEMP_TYPE) / SEGMENT_SIZE;
    if (create_options->tmpseg_num < 0)
        create_options->tmpseg_num = 0;

    dbi_AddVolume(connid, 0, create_options->idxseg_num,
            create_options->tmpseg_num);

    /* 기본이 되는 'systables'와 'sysfields' 생성 */
    ret = Create_SysTables_SysFields();
    if (ret < 0)
    {
        __mdb_syslog("Create SysTables SysFields Fail...\n");
        goto done;
    }

    /* systables = 1; sysfields = 2; */
    ContCatalog_TableidInit(3);

    /* 그외 system table 생성 */
    ret = Create_System_Tables();
    if (ret < 0)
    {
        __mdb_syslog("Create System Tables Fail...\n");
        goto done;
    }

    if (SEGMENT_SIZE < 1024 * 1024)
        __mdb_syslog("Segment Size: %d KB\n\n", SEGMENT_SIZE / 1024);
    else
        __mdb_syslog("Segment Size: %d MB\n\n", SEGMENT_SIZE / 1024 / 1024);

    if (SEGMENT_SIZE < 1024 * 1024)
        __mdb_syslog("Init DB Size: %d KB (%d segment(s))\n\n",
                (segment_no * SEGMENT_SIZE) / 1024, segment_no);
    else
        __mdb_syslog("Init DB Size: %d MB (%d segment(s))\n\n",
                segment_no * (SEGMENT_SIZE / 1024 / 1024), segment_no);

    sc_memcpy(&createdb_svr, server, sizeof(struct Server));

    Server_Down(100);

    __mdb_syslog("\nCreatedb (%s) Done...\n", dbname);

    ret = DB_SUCCESS;

  done:
    MDB_DB_UNLOCK();

    if (connid >= 0)
    {
        CS_FreeCSObject(connid);
        _g_connid = -1;
    }

    if (ret < 0)
    {
        __mdb_syslog("ERROR(%d): %s\n", ret, DB_strerror(ret));

        if (ret == DB_E_DBEXISTED)
        {
            /* 이미 존재하는 경우임. */
            __mdb_syslog("\tThe DB file or directory exists!\n");
            __mdb_syslog("\tTo create db with the name %s\n", dbname);
            __mdb_syslog
                    ("\tremove the file or directory and rerun 'createdb'.\n");
        }
        else
        {
            char dir_name[MDB_FILE_NAME];

            sc_sprintf(dir_name, "%s" DIR_DELIM "%s",
                    __config_path, dbfilename);

            if (Drop_DB(dir_name) != TRUE)
            {
                __mdb_syslog("FAIL to destroy, %s\n", dir_name);
            }
        }

        __mdb_syslog("\nCreatedb Fail...\n");
    }

    server = NULL;

    return ret;
}

/*****************************************************************************/
//! Make_Directory 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param dbname :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
Make_Directory(char *dbname)
{
    /* 0: db, 1: sys, 2: file */
    char dirname[3][MDB_FILE_NAME];
    int numDirName = 3;
    int i;

    // __config_path directory 없으면 생성 
    {
        char dir_path[MDB_FILE_NAME];
        int err;

        i = 1;

#if defined(WIN32)
        if (__config_path[1] == ':' && __config_path[2] == DIR_DELIM_CHAR)
        {
            i = 3;
        }
#endif
        while (1)
        {
            if (__config_path[i] == DIR_DELIM_CHAR || __config_path[i] == '\0')
            {
                sc_memset(dir_path, 0, sizeof(dir_path));
                sc_memcpy(dir_path, __config_path, i);

                err = sc_mkdir(dir_path, OPEN_MODE);
                if (err == -1)
                    return DB_E_DBPATHMAKE;

                if (__config_path[i] == '\0')
                    break;
            }

            i++;
        }
    }

    /* db file이 있는지 점검 */
    {
        int fd;
        char datafile[MDB_FILE_NAME];

        sc_sprintf(datafile, "%s" DIR_DELIM "%s_data.00",
                __config_path, dbname);
        fd = sc_open(datafile, O_RDONLY, OPEN_MODE);
        if (fd >= 0)
        {   /* 이미 db file이 존재. 오류 처리! */
            sc_close(fd);

            __mdb_syslog("Error: db already exists. (%s)\n", datafile);

            return DB_E_DBEXISTED;
        }
    }

    /* dbname directory를 만들지 않으므로 log path를 만드는 곳으로 jump! */
    goto make_log_path;

    if (__config_path_len + sc_strlen(dbname) * 2 + 10 > MDB_FILE_NAME - 1)
    {
        __mdb_syslog(" ERROR : dir_name Very Long ");
        return DB_E_INVALIDPARAM;
    }

    /* db directory 설정 */
    sc_sprintf(dirname[0], "%s" DIR_DELIM "%s", __config_path, dbname);

    /* db/backupDB directory */
    sc_sprintf(dirname[1], "%s" DIR_DELIM "%s" DIR_DELIM "backupDB",
            __config_path, dbname);

    /* db/logs directory */
    sc_sprintf(dirname[2], "%s" DIR_DELIM "%s" DIR_DELIM "logs",
            __config_path, dbname);

    for (i = 0; i < numDirName; i++)
    {
        if (sc_mkdir(dirname[i], OPEN_MODE) < 0)
        {
            if (sc_strcmp(dirname[i], dirname[0]) == 0)
                continue;

            __mdb_syslog("Error: making %s\n", dirname[i]);

            return DB_E_DBPATHMAKE;
        }

        __mdb_syslog("making %s... ok\n", dirname[i]);
    }

  make_log_path:

/* lib_server 경우, log_path[]가 없으면 만들어줌 */
    if (log_path[0] != '\0')
    {
        // Set_Path_Log()에 의해 log_path가 지정되면 해당 log_path에 대해서도
        //   있는지 점검하고 없으면 만들어준다 
        char dir_path[MDB_FILE_NAME];
        int err = 0;

        i = 1;
        while (1)
        {
            if (log_path[i] == DIR_DELIM_CHAR || log_path[i] == '\0')
            {
                sc_memset(dir_path, 0, sizeof(dir_path));
                sc_memcpy(dir_path, log_path, i);

                err = sc_mkdir(dir_path, OPEN_MODE);
                if (err == -1)
                    break;

                if (log_path[i] == '\0')
                    break;
            }

            i++;
        }

        if (err == -1)
        {
            __mdb_syslog("Error: making %s\n", log_path);
            return DB_E_MAKELOGPATH;
        }
        else
        {   // db 디렉토리 아래 log_path를 file로 저장한다 
            int fd;

            sc_sprintf(dir_path, "%s" DIR_DELIM "logpath", dirname[0]);

            fd = sc_open(dir_path, O_WRONLY | O_BINARY, OPEN_MODE);
            sc_write(fd, log_path, sc_strlen(log_path));

            sc_fsync(fd);
            sc_close(fd);
        }
    }


    return DB_SUCCESS;
}

FIELDDESC_T _systablesFD[] = {
    {"tableid", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"tablename", DT_CHAR, 0, 0, REL_NAME_LENG, 0, 0, 0x1, FIXED_VARIABLE,
                MDB_COL_DEFAULT_SYSTEM, "\0"}
    ,
    {"numfields", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
    ,
    {"recordlen", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
    ,
    {"h_recordlen", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
    ,
    {"numrecords", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
    ,
    {"nextid", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"maxrecords", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
    ,
    {"column_name", DT_CHAR, 0, 0, REL_NAME_LENG, 0, 0, 1, FIXED_VARIABLE,
                MDB_COL_DEFAULT_SYSTEM, "\0"}
    ,
    {"flag", DT_SHORT, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC, "\0"}
    ,
    {"has_variabletype", DT_TINYINT, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
};

int systablesNum = sizeof(_systablesFD) / sizeof(FIELDDESC_T);

FIELDDESC_T _sysfieldsFD[] = {
    {"fieldid", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"tableid", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"position", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"fieldname", DT_CHAR, 0, 0, FIELD_NAME_LENG, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_DEFAULT_SYSTEM, "\0"}
    ,
    {"offset", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"h_offset", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"length", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"scale", DT_TINYINT, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"type", DT_TINYINT, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"flag", DT_TINYINT, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"collation_type", DT_TINYINT, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
    ,
    {"defaultvalue", DT_CHAR, 0, 0, DEFAULT_VALUE_LEN, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_DEFAULT_SYSTEM, "\0"}
    ,
    {"fixed_part", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
};

int sysfieldsNum = sizeof(_sysfieldsFD) / sizeof(FIELDDESC_T);

FIELDDESC_T _sysindexesFD[] = {
    {"tableid", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"indexid", DT_TINYINT, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"type", DT_TINYINT, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"numfields", DT_TINYINT, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
    ,
    {"indexname", DT_CHAR, 0, 0, INDEX_NAME_LENG, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_DEFAULT_SYSTEM, "\0"}
};

int sysindexesNum = sizeof(_sysindexesFD) / sizeof(FIELDDESC_T);

FIELDDESC_T _sysindexfieldsFD[] = {
    {"tableid", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"indexid", DT_TINYINT, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"keyposition", DT_TINYINT, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
    ,
    {"ordering", DT_CHAR, 0, 0, 1, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_DEFAULT_SYSTEM, "\0"}
    ,
    {"collation_type", DT_TINYINT, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
    ,
    {"fieldid", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
};

int sysindexfieldsNum = sizeof(_sysindexfieldsFD) / sizeof(FIELDDESC_T);

/* check SYSSTATUS_T, dbi_Set_SysStatus() */
FIELDDESC_T _sysstatusFD[] = {
    {"version", DT_CHAR, 0, 0, 32, 0, 0, 0x2, FIXED_VARIABLE,
                MDB_COL_DEFAULT_SYSTEM, "\0"}
    ,
    {"procid", DT_INTEGER, 0, 0, 0, 0, 0, 0x2, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"bits", DT_INTEGER, 0, 0, 0, 0, 0, 0x2, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"db_name", DT_CHAR, 0, 0, 16, 0, 0, 0x2, FIXED_VARIABLE,
                MDB_COL_DEFAULT_SYSTEM, "\0"}
    ,
    {"db_path", DT_CHAR, 0, 0, 64, 0, 0, 0x2, FIXED_VARIABLE,
                MDB_COL_DEFAULT_SYSTEM, "\0"}
    ,
    {"db_dsize", DT_INTEGER, 0, 0, 0, 0, 0, 0x2, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
    ,
    {"db_isize", DT_INTEGER, 0, 0, 0, 0, 0, 0x2, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
    ,
    {"num_connects", DT_INTEGER, 0, 0, 0, 0, 0, 0x2, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
    ,
    {"num_trans", DT_INTEGER, 0, 0, 0, 0, 0, 0x2, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
};

int sysstatusNum = sizeof(_sysstatusFD) / sizeof(FIELDDESC_T);

FIELDDESC_T _sysdummyFD[] = {
    {"$w_fInternal_dummy", DT_CHAR, 0, 0, 1, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_DEFAULT_SYSTEM, "\0"}
};

int sysdummyNum = sizeof(_sysdummyFD) / sizeof(FIELDDESC_T);

typedef struct sysdummy
{
    char dummy;
} SYSDUMMY_T;

FIELDDESC_T _sysviewsFD[] = {
    {"tableId", DT_INTEGER, 0, 0, 0, 0, 0, 1, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"},
    {"definition", DT_VARCHAR, 0, 0, MAX_SYSVIEW_DEF, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_DEFAULT_SYSTEM, "\0"}
};

int sysviewsNum = sizeof(_sysviewsFD) / sizeof(FIELDDESC_T);

FIELDDESC_T _syssequencesFD[] = {
    {"sequenceId", DT_INTEGER, 0, 0, 0, 0, 0, 1, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
    ,
    {"sequenceName", DT_CHAR, 0, 0, FIELD_NAME_LENG, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_DEFAULT_SYSTEM, "\0"}
    ,
    {"min_value", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
    ,
    {"max_value", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
    ,
    {"increment_by", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
    ,
    {"last_number", DT_INTEGER, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
    ,
    {"cycled", DT_TINYINT, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
    ,
    {"_inited", DT_TINYINT, 0, 0, 0, 0, 0, 0, FIXED_VARIABLE, MDB_COL_NUMERIC,
                "\0"}
};

int syssequencesNum = sizeof(_syssequencesFD) / sizeof(FIELDDESC_T);

/*****************************************************************************/
//! Create_SysTables_SysFields 
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
static int
Create_SysTables_SysFields(void)
{
    int systables_size, sysfields_size;
    int transid;

    transid = dbi_Trans_Begin(-1);
    if (transid < 0)
    {
        __mdb_syslog("Trans Begin Fail [%s:%d] %d\n", __FILE__, __LINE__,
                transid);
        return DB_FAIL;
    }

    systables_size = _Schema_CheckFieldDesc(systablesNum, _systablesFD, FALSE);
    systables_size = _Schema_CheckFieldDesc(systablesNum, _systablesFD, TRUE);

    if (Create_Table("systables", systables_size, systables_size, systablesNum,
                    _CONT_TABLE, 1, _USER_SYS, 0, NULL) < 0)
    {
        __mdb_syslog("Create Table sysfields Fail...\n");
        return DB_FAIL;
    }

    sysfields_size = _Schema_CheckFieldDesc(sysfieldsNum, _sysfieldsFD, FALSE);
    sysfields_size = _Schema_CheckFieldDesc(sysfieldsNum, _sysfieldsFD, TRUE);

    if (Create_Table("sysfields", sysfields_size, sysfields_size, sysfieldsNum,
                    _CONT_TABLE, 2, _USER_SYS, 0, NULL) < 0)
    {
        __mdb_syslog("Create Table sysfields Fail...\n");
        return DB_FAIL;
    }

    /* 'systables'와 'sysfields' systables에 넣기 */
    if (_Schema_InputTable(transid, "systables", 1, _CONT_TABLE, FALSE,
                    systables_size, systables_size,
                    systablesNum, _systablesFD, 0, NULL) < 0)
    {
        __mdb_syslog("FAIL: systables schema input\n");
        return DB_FAIL;
    }

    if (_Schema_InputTable(transid, "sysfields", 2, _CONT_TABLE, FALSE,
                    sysfields_size, sysfields_size,
                    sysfieldsNum, _sysfieldsFD, 0, NULL) < 0)
    {
        __mdb_syslog("FAIL: systables schema input\n");
        return DB_FAIL;
    }

    dbi_Trans_Commit(-1);

    return DB_SUCCESS;
}

/*****************************************************************************/
//! Create_System_Tables 
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
static int
Create_System_Tables(void)
{
    int transid;

    if (dbi_Relation_Create(-1, "sysindexes", sysindexesNum,
                    _sysindexesFD, _CONT_TABLE, _USER_SYS, 0, NULL) < 0)
    {
        __mdb_syslog("FAIL: sysindexes create\n");

        return DB_FAIL;
    }

    if (dbi_Relation_Create(-1, "sysindexfields", sysindexfieldsNum,
                    _sysindexfieldsFD, _CONT_TABLE, _USER_SYS, 0, NULL) < 0)
    {
        __mdb_syslog("FAIL: sysindexfields create\n");

        return DB_FAIL;
    }

    if (1)
    {
        int Cursor_ID;
        OID record_id;
        union
        {
            char buf[sizeof(SYSSTATUS_T) * 2];
            SYSSTATUS_T __tmp;
        }
        un;
        SYSSTATUS_T *sysStatus = (SYSSTATUS_T *) & un.__tmp;

        sc_memset(un.buf, 0, sizeof(un.buf));

        if (dbi_Relation_Create(-1, "sysstatus", sysstatusNum,
                        _sysstatusFD, _CONT_TABLE, _USER_SYS, 0, NULL) < 0)
        {
            __mdb_syslog("FAIL: sysstatus create\n");

            return DB_FAIL;
        }

        transid = dbi_Trans_Begin(-1);
        if (transid < 0)
        {
            __mdb_syslog("FAIL: trans begin [%s:%d] %d\n",
                    __FILE__, __LINE__, transid);
            return DB_FAIL;
        }

        Cursor_ID = Open_Cursor(transid, "sysstatus", LK_NOLOCK, -1,
                NULL, NULL, NULL);

        __Insert(Cursor_ID, (char *) sysStatus, sizeof(SYSSTATUS_T) * 2,
                &record_id, 0);

        Close_Cursor(Cursor_ID);

        dbi_Trans_Commit(-1);
    }

    /* systables의 tablename index */
    if (dbi_Index_Create(-1, "systables", "$pk.systables", 1,
                    _systablesFD + 1, 0x80) < 0)
    {
        __mdb_syslog("FAIL: systables index create\n");

        return DB_FAIL;
    }

    /* create unique index on systables(tableId) */
    if (dbi_Index_Create(-1, "systables", "$idx_systables_tblid",
                    1, _systablesFD, 0x80) < 0)
    {
        __mdb_syslog("FAIL: $idx_systables_tblid index create\n");
        return DB_FAIL;
    }

    /* sysfields의 tableId index */
    if (dbi_Index_Create(-1, "sysfields", "$idx_sysfields_tblid", 1,
                    _sysfieldsFD + 1, 0x00) < 0)
    {
        __mdb_syslog("FAIL: sysfields index create\n");

        return DB_FAIL;
    }

    /* create unique index on sysindexes(tableId, indexId) */
    if (dbi_Index_Create(-1, "sysindexes", "$idx_sysindexes_tblid_idxid",
                    2, _sysindexesFD, 0x80) < 0)
    {
        __mdb_syslog("FAIL: $idx_systables_tblid index create\n");
        return DB_FAIL;
    }

    /* create index on sysindexfields(tableId, indexId) */
    if (dbi_Index_Create(-1, "sysindexfields",
                    "$idx_sysindexfields_tblid_idxid", 2, _sysindexfieldsFD,
                    0x00) < 0)
    {
        __mdb_syslog("FAIL: $idx_systables_tblid index create\n");
        return DB_FAIL;
    }

    if (1)
    {
        int Cursor_ID;
        OID record_id;
        char buf[256];
        SYSDUMMY_T *sysDummy = (SYSDUMMY_T *) buf;

        sc_memset(buf, 0, sizeof(buf));

        if (dbi_Relation_Create(-1, "sysdummy", sysdummyNum,
                        _sysdummyFD, _CONT_TABLE, _USER_SYS, 0, NULL) < 0)
        {
            return DB_FAIL;
        }

        transid = dbi_Trans_Begin(-1);
        if (transid < 0)
        {
            __mdb_syslog("FAIL: trans begin [%s:%d] %d\n",
                    __FILE__, __LINE__, transid);
            return DB_FAIL;
        }

        Cursor_ID = Open_Cursor(transid, "sysdummy", LK_NOLOCK, -1,
                NULL, NULL, NULL);

        sysDummy->dummy = 'x';

        __Insert(Cursor_ID, (char *) sysDummy, 256, &record_id, 0);

        Close_Cursor(Cursor_ID);

        dbi_Trans_Commit(-1);
    }

    if (dbi_Relation_Create(-1, "sysviews", sysviewsNum,
                    _sysviewsFD, _CONT_TABLE, _USER_SYS, 0, NULL) < 0)
    {
        __mdb_syslog("FAIL: sysviews table create\n");

        return DB_FAIL;
    }

    /* sysviews tableId index */
    if (dbi_Index_Create(-1, "sysviews", "$pk.sysviews", 1,
                    _sysviewsFD, 0x80) < 0)
    {
        _sysviewsFD[1].length_ = MAX_SYSVIEW_DEF;
        __mdb_syslog("FAIL: sysviews index create\n");

        return DB_FAIL;
    }
    _sysviewsFD[1].length_ = MAX_SYSVIEW_DEF;

    if (dbi_Relation_Create(-1, "syssequences", syssequencesNum,
                    _syssequencesFD, _CONT_TABLE, _USER_SYS, 0, NULL) < 0)
    {
        __mdb_syslog("FAIL: syssequences table create\n");

        return DB_FAIL;
    }

    /* syssequences tableId index */
    if (dbi_Index_Create(-1, "syssequences", "$pk.syssequences", 1,
                    _syssequencesFD, 0x80) < 0)
    {
        __mdb_syslog("FAIL: syssequences index create\n");

        return DB_FAIL;
    }

    if (dbi_Index_Create(-1, "syssequences", "$idx_syssequences", 1,
                    &(_syssequencesFD[1]), 0x80) < 0)
    {
        __mdb_syslog("FAIL: syssequences index create\n");

        return DB_FAIL;
    }

    ContCatalog_TableidInit(_USER_TABLEID_BASE);

    return DB_SUCCESS;
}

/*****************************************************************************/
//! Drop_DB 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param path : DB Path
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX DB_BOOL
Drop_DB(char *p_path)
{
    int ret;
    char db_path[MDB_FILE_NAME];
    char db_prefix[MDB_FILE_NAME];
    int i;
    char path[MDB_FILE_NAME];

    Server_Down(100);

    sc_strncpy(path, p_path, MDB_FILE_NAME - 1);
    path[MDB_FILE_NAME - 1] = '\0';

    convert_dir_delimiter(path);

    i = sc_strlen(path);
    if (i == 0)
        return FALSE;

    while (1)
    {
        i--;
        if (i < 0)
            break;

        if (path[i] == DIR_DELIM_CHAR)
            break;
    }

    sc_memset(db_path, 0, MDB_FILE_NAME);
    sc_memset(db_prefix, 0, MDB_FILE_NAME);

    if (i < 0)
    {
        sc_strcpy(db_path, ".");
    }
    else if (i == 0)    /* root */
    {
        sc_strcpy(db_path, DIR_DELIM);
    }
    else
    {
        sc_strncpy(db_path, path, i);
        db_path[i] = '\0';
    }

    sc_sprintf(db_prefix, "%s_temp.", path + i + 1);
    ret = sc_unlink_prefix(db_path, db_prefix);

    sc_sprintf(db_prefix, "%s_indx.", path + i + 1);
    ret = sc_unlink_prefix(db_path, db_prefix);

    sc_sprintf(db_prefix, "%s_logfile", path + i + 1);
    ret = sc_unlink_prefix(db_path, db_prefix);

    sc_sprintf(db_prefix, "%s_loganchor", path + i + 1);
    ret = sc_unlink_prefix(db_path, db_prefix);

    sc_sprintf(db_prefix, "%s_data.", path + i + 1);
    ret = sc_unlink_prefix(db_path, db_prefix);
    if (ret < 0)
    {
        __mdb_syslog("\nDrop DB Error : %s\n", path);
        return FALSE;
    }
    else
    {
        __mdb_syslog("\nDrop DB Success : %s\n", path);
        return TRUE;
    }
}

#endif /* !defined(_UTILITY_) */
