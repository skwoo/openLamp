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
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include "mdb_define.h"
#include "mdb_inner_define.h"
#include "mdb_compat.h"
#include "db_api.h"
#include "../../version_mmdb.h"

__DECL_PREFIX DB_BOOL Drop_DB(char *);

static int get_serverpid();
static int get_dbname(char *dbname);

static char config_path[MDB_FILE_NAME] = "\0";
int config_path_len = 0;

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
main(int argc, char *argv[])
{
    char dir_name[MDB_FILE_NAME];
    int answer;
    int pid;
    DB_BOOL force = FALSE;
    int i;
    char *input_dbname;
    char dbfilename[MAX_DB_NAME] = "";

    printf("\ndestroydb for %s %s\n", _DBNAME_, _DBVERSION_);
    printf("%s\n\n", _COPYRIGHT_);

    for (i = 1; i < argc; i++)
    {
        if (*argv[i] == '-' && (*(argv[i] + 1) == 'f'
                        || *(argv[i] + 1) == 'F'))
        {
            force = TRUE;
            break;
        }
    }

    if (argc < 2 || argc > 3 || (argc == 3 && force == FALSE)
            || (argc == 2 && force == TRUE))
    {
        printf("ERROR: Invalid arguments\n");
        printf("USAGE: argv[1] -  database_name\n");
        printf("===============================\n");
        return DB_FAIL;
    }
    else
    {
        int len;

        if (force)
        {
            if (i == 2)
                input_dbname = argv[1];
            else
                input_dbname = argv[2];
        }
        else
            input_dbname = argv[1];

        len = sc_strlen(input_dbname);

        if (len == 0 || len > MAX_DB_NAME - 1)
        {
            printf("ERROR: invalid database name\n");
            printf("USAGE: valid database name length = 1 ~ %d\n",
                    MAX_DB_NAME - 1);
            printf("===============================\n");
            return DB_FAIL;
        }
    }

    convert_dir_delimiter(input_dbname);

    {
        int ret;

        ret = __check_dbname(input_dbname, dbfilename);
        if (ret < 0)
        {
            printf("FAIL check dbname: %d\n", ret);
            return ret;
        }
    }

    if (sc_getenv("MOBILE_LITE_CONFIG") == NULL)
    {
        printf("ERROR: Environment variable MOBILE_LITE_CONFIG not set.\n");

        return DB_FAIL;
    }
    else
    {
        sc_strcpy(config_path, sc_getenv("MOBILE_LITE_CONFIG"));
    }

    config_path_len = sc_strlen(config_path);

    pid = get_serverpid();
    if (pid > 0)
    {
        char dbname[MAX_DB_NAME] = "";

        get_dbname(dbname);

        if (sc_strcmp(dbname, input_dbname) == 0)
        {
            printf("Unable to destroy [%s] database while Server (PID %d) is "
                    "running on it.\n", dbname, pid);
            exit(-1);
        }
    }

    sc_strcpy(dir_name, config_path);

    {
        struct stat buf;

#if !defined(ANDROID_OS)
        extern int errno;
#endif
        char dbfullpath[MAX_DB_NAME];

        if (stat(dir_name, &buf) < 0 && errno == ENOENT)
        {
            fprintf(stderr, "\tDatabase[%s] does not exist!!!\n\n",
                    input_dbname);

            exit(0);
        }

        sc_sprintf(dbfullpath, "%s" DIR_DELIM "%s_data.00",
                config_path, dbfilename);
        if (stat(dbfullpath, &buf) < 0 && errno == ENOENT)
        {
            fprintf(stderr, "\tDatabase[%s] does not exist!!!\n\n",
                    input_dbname);

            exit(0);
        }
    }

    if (!force)
    {
        printf("You are destroying the db file, %s" DIR_DELIM "%s\n",
                dir_name, dbfilename);
        printf("Are you sure? (y/n) ");
        answer = getchar();

        if (sc_toupper(answer) != 'Y')
        {
            return DB_FAIL;
        }
    }

    sc_sprintf(dir_name, "%s" DIR_DELIM "%s", config_path, dbfilename);
    if (Drop_DB(dir_name) != TRUE)
        printf("FAIL to destroy [%s] dabatase\n", dbfilename);
    else
        printf("\nDestroying [%s] database done.\n", dbfilename);

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
static int
get_serverpid()
{
    FILE *fp;
    char pidfile[MAX_DB_NAME];
    int pid;

    sc_sprintf(pidfile, "%s" DIR_DELIM "mobile_lite.pid", config_path);

    fp = fopen(pidfile, "r");
    if (fp == NULL)
        return -1;

    fscanf(fp, "%d", &pid);
    fclose(fp);

#ifdef WIN32
    if (OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid))
        return pid;
#else
    if (kill(pid, 0) == 0)
        return pid;
#endif

    return -1;
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
static int
get_dbname(char *dbname)
{
    FILE *fp;
    char dbnamefile[MAX_DB_NAME];

    sc_sprintf(dbnamefile, "%s" DIR_DELIM "mobile_lite.db", config_path);

    fp = fopen(dbnamefile, "r");
    if (fp == NULL)
        return -1;

    fscanf(fp, "%s", dbname);
    fclose(fp);

    return 0;
}
