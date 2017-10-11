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
#include "mdb_syslog.h"
#include "mdb_compat.h"
#include "mdb_inner_define.h"
#include "mdb_collation.h"

__DECL_PREFIX DB_PARAMS DB_Params = {
    /* max_segments             */ 0,
    /* mem_segments             */ 0,
    /* mem_segments_indexing    */ 0,
    /* low_mem_segments         */ 0,
    /* memory_commit            */ 0,
    /* lock_timeout             */ 30,
    /* use_logging              */ 1,
    /* prm_no_recovery_mode     */ 0,
    /* use_shm_key              */ 0,
                                                /* log_file_size            */ (64 * 1024),
                                                /* bytes */
                                                /* log_buffer_size          */ (64 * 1024),
                                                /* bytes */
    /* max_logfile_num          */ 50,
    /* log_path                 */ "",
    /* num_candidates           */ VAR_COLLECT_CNT,
    /* default_fixed_part       */ FIXED_SIZE_FOR_VARIABLE,
                                        /* serverlog_level          */ 1,
                                        /* on */
    /* serverlog_size           */ 128 * 1024,
    /* serverlog_num            */ 1,
    /* serverlog_path           */ "log",
    /* db_write_protection      */ 0,
    /* db_lock_sleep_time       */ 10,
    /* db_lock_sleep_time2      */ 100,
    /* db_start_recovery        */ 1,
    /* num_flush_page_commit    */ 0,
    /* default_col_char         */ MDB_COL_DEFAULT_CHAR,
    /* default_col_nchar        */ MDB_COL_DEFAULT_NCHAR,
    /* DEBUG_LOGGING */
    /* debug log                */ 0,
    /* skip_index_check         */ 0,
    /* resident_table_limit     */ 0,
    /* check_free_space         */ 1,
    /* kick_dog_page_count      */ 50
};

extern MDB_COL_TYPE db_find_collation(char *identifier);

static int Conf_read_parameters_buf(char **buf, int *len,
        char *param, char *value);
static void skip_line_buf(char **buf, int *len);

/*****************************************************************************/
//! Conf_db_parameters 
/*! \breif  환경 설정 파일에서 DB의 parameter를 읽어서, 값을 셋팅해주는 함수 
 ************************************
 * \param dbname(in)    : DB의 path
 ************************************
 * \return  return_value
 ************************************
 * \note    LIBSERVER에서만 사용되는 함수이다    
 *  - SYSTEM COLLATION 추가
 *****************************************************************************/
/* db 별 configuration.
   dbname은 full path name */
int
Conf_db_parameters(char *dbname)
{
    int fd;
    char configfile[256];
    int res;
    int file_size;
    char *config_buf;
    char *ptr;

#if defined(_AS_WINCE_) || defined(_AS_WIN32_MOBILE_)
    /* wince에서는 mem_segments를 default로 10 MB 사용 */
    DB_Params.mem_segments = (5 * 1024 * 1024) / SEGMENT_SIZE + 1;
#else
    /* default 2MB */
    DB_Params.mem_segments = (2 * 1024 * 1024) / SEGMENT_SIZE + 1;
#endif

    DB_Params.mem_segments_indexing = DB_Params.mem_segments;

//윈도우 기본 디렉토리(\\WINNT or \\WINDOWS)를 알아와서 자동으로 넣어주도록 변경
#if defined(_AS_WINCE_) || defined(_AS_WIN32_MOBILE_)
    sc_sprintf(configfile, "%s" DIR_DELIM "%s",
            sc_getenv("SystemRoot"), "openml.cfg");
#else
    sc_sprintf(configfile, DIR_DELIM "%s", "openml.cfg");
#endif
    fd = sc_open(configfile, O_RDONLY | O_BINARY, OPEN_MODE);
    if (fd >= 0)
        goto read_params;

#if defined(_AS_WINCE_) || defined(_AS_WIN32_MOBILE_)
    sc_sprintf(configfile, "%s" DIR_DELIM "openml.cfg", dbname);
#else
    sc_sprintf(configfile, "/Debug" DIR_DELIM "openml.cfg");
#endif
    fd = sc_open(configfile, O_RDONLY | O_BINARY, OPEN_MODE);
    if (fd >= 0)
        goto read_params;

#if defined(_AS_WINCE_) || defined(_AS_WIN32_MOBILE_)
#else
    sc_sprintf(configfile, "%s" DIR_DELIM "openml.cfg",
            sc_getenv("MOBILE_LITE_CONFIG"));
    fd = sc_open(configfile, O_RDONLY | O_BINARY, OPEN_MODE);
#endif
    if (fd < 0)
        return -1;

  read_params:
    {
        char param[256];
        char value[256];

        file_size = sc_lseek(fd, 0, SEEK_END);
        if (file_size <= 0)
        {
            sc_close(fd);
            __SYSLOG2("configuration file has no contents! "
                    "Please check openml.cfg in MOBILE_LITE_CONFIG\n");
            return -1;
        }

        config_buf = mdb_malloc(file_size + 1);
        if (config_buf == NULL)
        {
            sc_close(fd);
            return DB_E_OUTOFMEMORY;
        }

        sc_lseek(fd, 0, 0);
        if (sc_read(fd, config_buf, file_size) != file_size)
        {
            mdb_free(config_buf);
            sc_close(fd);
            __SYSLOG2("configuration file read fail!");
            return -1;
        }
        config_buf[file_size] = '\0';

        sc_close(fd);

        ptr = config_buf;
        while (1)
        {
            res = Conf_read_parameters_buf(&ptr, &file_size, param, value);
            if (res > 0)
                break;

#ifdef MDB_DEBUG
            __SYSLOG("PARAM: %s = %s\n", param, value);
#endif

#ifndef REDUCED_CFG_PARAMETERS
            if (!mdb_strcasecmp(param, "MAX_SEGMENTS"))
                DB_Params.max_segments = sc_atoi(value);
            else if (!mdb_strcasecmp(param, "MEM_SEGMENTS"))
                DB_Params.mem_segments = sc_atoi(value);
            else
#endif // REDUCED_CFG_PARAMETERS
            if (!mdb_strcasecmp(param, "DB_SIZE"))
            {
                /* KB 단위 지원, 0.5 */
                double mem_MB = sc_atof(value);

                if (mem_MB < 0.3 && mem_MB > 0)
                    mem_MB = 0.3;

                if (mem_MB <= 0)
                    DB_Params.max_segments = 0;
                else
                {
                    DB_Params.max_segments = (int)
                            ((mem_MB * 1024.0 * 1024.0) /
                            (double) SEGMENT_SIZE + 1);
                }
            }
            else if (!mdb_strcasecmp(param, "NUM_CANDIDATES"))
            {
                DB_Params.num_candidates = sc_atoi(value);

                if (DB_Params.num_candidates < MIN_VAR_COLLECT_CNT)
                    DB_Params.num_candidates = MIN_VAR_COLLECT_CNT;
                else if (DB_Params.num_candidates > VAR_COLLECT_CNT)
                    DB_Params.num_candidates = VAR_COLLECT_CNT;
            }
#ifndef REDUCED_CFG_PARAMETERS
#endif // REDUCED_CFG_PARAMETERS
            else if (!mdb_strcasecmp(param, "DATA_BUFCACHE_SIZE"))
            {
//     DATA_BUFCACHE_SIZE =1.5 를 지원 
                double mem_MB = sc_atof(value);

                if (mem_MB <= 0)
                    DB_Params.mem_segments = 0;
                else
                {
                    DB_Params.mem_segments = (int)
                            ((mem_MB * 1024 * 1024) / (double) SEGMENT_SIZE +
                            1);
                }
            }
#ifndef REDUCED_CFG_PARAMETERS
            else if (!mdb_strcasecmp(param, "DATA_BUFCACHE_LOW_SIZE"))
            {
                double mem_MB = sc_atof(value);

                if (mem_MB <= 0)
                    DB_Params.low_mem_segments = 0;
                else
                {
                    DB_Params.low_mem_segments = (int)
                            ((mem_MB * 1024 * 1024) / (double) SEGMENT_SIZE);
                }
            }
#endif // REDUCED_CFG_PARAMETERS
            else if (!mdb_strcasecmp(param,
                            "DATA_BUFCACHE_SIZE_INDEX_REBUILD"))
            {
                double mem_MB = sc_atof(value);

                if (mem_MB <= 0)
                    DB_Params.mem_segments_indexing = 0;
                else
                {
                    DB_Params.mem_segments_indexing = (int)
                            ((mem_MB * 1024 * 1024) / (double) SEGMENT_SIZE +
                            1);
                }
            }
#ifndef REDUCED_CFG_PARAMETERS
            else if (!mdb_strcasecmp(param, "DB_WRITE_PROTECTION"))
            {
                DB_Params.db_write_protection = sc_atoi(value);
            }
#endif // REDUCED_CFG_PARAMETERS
            else if (!mdb_strcasecmp(param, "DB_LOCK_SLEEP_TIME"))
            {
                DB_Params.db_lock_sleep_time = sc_atoi(value);
            }
            else if (!mdb_strcasecmp(param, "DB_LOCK_SLEEP_TIME2"))
            {
                DB_Params.db_lock_sleep_time2 = sc_atoi(value);
            }
#ifndef REDUCED_CFG_PARAMETERS
            else if (!mdb_strcasecmp(param, "FIXED_PART_FOR_VARIABLE"))
            {
                DB_Params.default_fixed_part = sc_atoi(value);
                if (DB_Params.default_fixed_part <= FIXED_VARIABLE)
                    DB_Params.default_fixed_part = FIXED_VARIABLE;
                else if (DB_Params.default_fixed_part < MINIMUM_FIXEDPART)
                    DB_Params.default_fixed_part = MINIMUM_FIXEDPART;
            }
#endif // REDUCED_CFG_PARAMETERS
            else if (!mdb_strcasecmp(param, "DB_START_RECOVERY"))
            {
                DB_Params.db_start_recovery = sc_atoi(value);
            }
            else if (!mdb_strcasecmp(param, "NUM_FLUSH_PAGE_COMMIT"))
                DB_Params.num_flush_page_commit = sc_atoi(value);
#ifndef REDUCED_CFG_PARAMETERS
            else if (!mdb_strcasecmp(param, "DEFAULT_COLLATION_CHAR"))
            {
                MDB_COL_TYPE collation;

                collation = db_find_collation(value);
                if (collation != MDB_COL_NONE)
                    DB_Params.default_col_char = collation;
            }
            else if (!mdb_strcasecmp(param, "DEFAULT_COLLATION_NCHAR"))
            {
                MDB_COL_TYPE collation;

                collation = db_find_collation(value);
                if (collation != MDB_COL_NONE)
                    DB_Params.default_col_nchar = collation;
            }
#endif // REDUCED_CFG_PARAMETERS
            else if (!mdb_strcasecmp(param, "LOCK_TIME_OUT"))
            {
                res = sc_atoi(value);
                if (res >= 0)
                    DB_Params.lock_timeout = res;
            }
#ifndef REDUCED_CFG_PARAMETERS
            else if (!mdb_strcasecmp(param, "RESIDENT_TABLE_LIMIT"))
            {
                double mem_MB = sc_atof(value);

                if (mem_MB <= 0)
                    DB_Params.resident_table_limit = 0;
                else
                {
                    DB_Params.resident_table_limit = (int)
                            ((mem_MB * 1024 * 1024) / (double) SEGMENT_SIZE +
                            1);
                }
            }
            else if (!mdb_strcasecmp(param, "SERVERLOG_LEVEL"))
                DB_Params.serverlog_level = sc_atoi(value);
            else if (!mdb_strcasecmp(param, "SERVERLOG_SIZE"))
            {
                int size = sc_atoi(value);

                if (16 <= size && size <= 1024)
                {
                    DB_Params.serverlog_size = size * 1024;
                }
            }
            else if (!mdb_strcasecmp(param, "SERVERLOG_NUM"))
            {
                int num = sc_atoi(value);

                if (1 <= num && num <= 64)
                {
                    DB_Params.serverlog_num = num;
                }
            }
            else if (!mdb_strcasecmp(param, "MAX_LOGFILE_NUM"))
            {
                DB_Params.max_logfile_num = sc_atoi(value);
                if (DB_Params.max_logfile_num < 0)
                    DB_Params.max_logfile_num = 50;
            }
            else if (!mdb_strcasecmp(param, "CHECK_FREE_SPACE"))
            {
                DB_Params.check_free_space = sc_atoi(value);
                if (DB_Params.check_free_space != 0)
                {
                    DB_Params.check_free_space = 1;
                }
            }
            else if (!mdb_strcasecmp(param, "KICK_DOG_PAGE_COUNT"))
            {
                DB_Params.kick_dog_page_count = sc_atoi(value);
                if (DB_Params.kick_dog_page_count < 0)
                    DB_Params.kick_dog_page_count = 0;
            }
#endif // REDUCED_CFG_PARAMETERS
            else if (!mdb_strcasecmp(param, "LOG_BUFFER_SIZE"))
            {
                int log_buffer_size_in_KB = sc_atoi(value);

                if (log_buffer_size_in_KB < LOG_PAGE_SIZE / 1024)
                {
                    MDB_SYSLOG(("Too small LOG_BUFFER_SIZE(%d KB). "
                                    "Set to %d KB.\n",
                                    log_buffer_size_in_KB,
                                    LOG_PAGE_SIZE / 1024));
                    log_buffer_size_in_KB = 64;
                }
                if (log_buffer_size_in_KB > 256)
                {
                    MDB_SYSLOG(("Too large LOG_BUFFER_SIZE(%d KB). "
                                    "Set to 256 KB.\n",
                                    log_buffer_size_in_KB));
                    log_buffer_size_in_KB = 256;
                }
                DB_Params.log_buffer_size = (log_buffer_size_in_KB * 1024);
            }
            else if (!mdb_strcasecmp(param, "LOG_FILE_SIZE"))
            {
                int log_file_size_in_KB = sc_atoi(value);

                /* log file size must not less than log buffer size */
                if (log_file_size_in_KB * 1024 < DB_Params.log_buffer_size)
                {
                    MDB_SYSLOG(("Too small LOG_FILE_SIZE(%d KB). "
                                    "Set to %d KB.\n",
                                    log_file_size_in_KB,
                                    DB_Params.log_buffer_size / 1024));
                    log_file_size_in_KB = DB_Params.log_buffer_size / 1024;
                }
                if (log_file_size_in_KB * 1024 < LOG_PAGE_SIZE)
                {
                    MDB_SYSLOG(("Too small LOG_FILE_SIZE(%d KB). "
                                    "Set to %d KB.\n",
                                    log_file_size_in_KB,
                                    LOG_PAGE_SIZE / 1024));
                    log_file_size_in_KB = LOG_PAGE_SIZE / 1024;
                }
                if (log_file_size_in_KB > 256)
                {
                    MDB_SYSLOG(("Too large LOG_FILE_SIZE(%d KB). "
                                    "Set to 256 KB.\n", log_file_size_in_KB));
                    log_file_size_in_KB = 256;
                }
                DB_Params.log_file_size = (log_file_size_in_KB * 1024);
            }
            else if (!mdb_strcasecmp(param, "SKIP_INDEX_CHECK"))
            {
                DB_Params.skip_index_check = sc_atoi(value);
            }
        }

        mdb_free(config_buf);
    }

    return res;
}


/* Get the value of the specified parameter from configuration file . */

/*****************************************************************************/
//! Conf_read_parameters 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param fd(in)        : File Description
 * \param param(out)    : 변수 이름 
 * \param value(out)    : 변수의 값 
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *      
 *****************************************************************************/
static int
Conf_read_parameters_buf(char **buf, int *len, char *param, char *value)
{
    char ch;
    int i, j;

    value[0] = '\0';

    i = 0;
    param[0] = '\0';

    while (1)
    {
        if (*len == 0)
            break;

        ch = *(*buf);
        (*buf)++;
        (*len)--;

        if (ch == '#')
        {
            skip_line_buf(buf, len);

            continue;
        }

        if (ch == '\r' || ch == '\n')
        {
            i = 0;
            param[0] = '\0';
            continue;
        }

        if (ch == ' ' || ch == '\t')
            continue;

        if (ch == '=')
        {
            if (i != 0)
            {
                char tmpvalue[256];

                sc_memset(tmpvalue, 0, 256);

                // paramter의 이름이 들어가는 건 완료
                param[i] = '\0';

                // value 이름 찾기 시작 : fscanf를 사용 못해서 직접 대용을 만듬

                j = 0;
                while (1)
                {
                    if (*len == 0)
                    {
                        if (j == 0)
                            return -1;
                        else
                            break;
                    }

                    ch = *(*buf);
                    (*buf)++;
                    (*len)--;

                    if (ch == ' ')
                        continue;

                    if (ch == '\n' || ch == '\0' || ch == '\r')
                        break;

                    tmpvalue[j++] = ch;

                    if (j >= 255)
                    {
                        j = 255;
                        tmpvalue[255] = '\0';
                        break;
                    }
                }
                // 값 복사     
                sc_strncpy(value, tmpvalue, sc_strlen(tmpvalue));
                value[j] = '\0';

                return 0;
            }
            skip_line_buf(buf, len);
        }
        else
            param[i++] = ch;
    }

    return 1;
}

/*****************************************************************************/
//! skip_line 
/*! \breif 주석 문장을 skip 시켜 버리는 함수 
 ************************************
 * \param fd(in) : file description
 ************************************
 * \return  void
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static void
skip_line_buf(char **buf, int *len)
{
    char ch;

    while ((*len) > 0)
    {
        ch = *(*buf);
        (*buf)++;
        (*len)--;
        if (ch == '\r' || ch == '\n' || ch == 0xa || ch == 0xd)
            break;
    }
}

int
DB_Params_db_lock_sleep_time(void)
{
    return DB_Params.db_lock_sleep_time;
}

int
DB_Params_db_lock_sleep_time2(void)
{
    return DB_Params.db_lock_sleep_time2;
}
