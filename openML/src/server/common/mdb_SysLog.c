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
#include "mdb_compat.h"
#include "db_api.h"
#include "mdb_ppthread.h"
#include "mdb_inner_define.h"
#include "mdb_syslog.h"
#include "../../version_mmdb.h"
#include "mdb_PMEM.h"
#include "mdb_Server.h"
#ifdef WIN32_MOBILE
#include "Shlobj.h"
#endif

#define LOGFILENAME "serverlog.txt"

void rotate_log_file(char *logfile);

static int syslog_fInit = FALSE;
static char syslog_file[256];

#define SYSLOGBUFSZ     2048
static int syslogbuf_pos = 0;
static char *syslogbuf = NULL;

static int __SYSLOG_Init(void);
static void __SYSLOG_SETTIME(char *buf, int bufsize);

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
__DECL_PREFIX void
__SYSLOG(char *format, ...)
{
    va_list ap;
    int fd, n;
    char buf[256], *p_buf;
    int p_size;

    if (DB_Params.serverlog_level == 0)
        return;

    fd = -1;

    p_buf = buf;
    p_size = sizeof(buf);

    while (1)
    {
        va_start(ap, format);

        n = sc_vsnprintf(p_buf, p_size - 1, format, ap);

        va_end(ap);


#if defined(WIN32)
        if (n == -1)
#else
        if (n > p_size - 1)
#endif
        {
            char *ptr;

            ptr = (char *) PMEM_ALLOC(n + 2);
            if (ptr == NULL)
            {
                p_buf[p_size - 2] = '\n';
                p_buf[p_size - 1] = '\0';
                n = p_size;
                break;
            }
            else
            {
                if (p_buf != buf)
                    PMEM_FREE(p_buf);
                p_buf = ptr;
                p_size = n + 2;
                p_buf[p_size - 1] = '\0';
            }
        }
        else if (n < 0)
        {
            p_buf[p_size - 2] = '\n';
            p_buf[p_size - 1] = '\0';
            n = p_size;
            break;
        }
        else
            break;
    }

  retry:
    if (syslogbuf == NULL)
    {
        if (syslogbuf)
            goto retry;

        fd = __SYSLOG_Init();
        if (fd < 0)
            return;

        sc_write(fd, p_buf, n);
    }
    else
    {
        if (syslogbuf_pos + n > SYSLOGBUFSZ)
        {
            fd = __SYSLOG_Init();
            if (fd < 0)
                return;
            sc_write(fd, syslogbuf, syslogbuf_pos);
            syslogbuf_pos = 0;
        }

        sc_memcpy(syslogbuf + syslogbuf_pos, p_buf, n);
        syslogbuf_pos += n;
    }

    if (syslog_file[0] != '\0')
    {
        if (fd >= 0)
            sc_close(fd);
    }


    if (p_buf != buf)
        PMEM_FREE(p_buf);
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
#if defined(WIN32_MOBILE) || defined(linux) || defined(sun)
__DECL_PREFIX void
__SYSLOG2(char *format, ...)
{
    va_list ap;
    int fd, n;
    char buf[256], *p_buf;
    int p_size;

    if (DB_Params.serverlog_level == 0)
        return;

    p_buf = buf;
    p_size = sizeof(buf);

    while (1)
    {
        va_start(ap, format);

        n = sc_vsnprintf(p_buf, p_size - 1, format, ap);

        va_end(ap);


#if defined(WIN32)
        if (n == -1)
#else
        if (n > p_size - 1)
#endif
        {
            char *ptr;

            ptr = (char *) PMEM_ALLOC(n + 2);
            if (ptr == NULL)
            {
                p_buf[p_size - 2] = '\n';
                p_buf[p_size - 1] = '\0';
                n = p_size;
                break;
            }
            else
            {
                if (p_buf != buf)
                    PMEM_FREE(p_buf);
                p_buf = ptr;
                p_size = n + 2;
                p_buf[p_size - 1] = '\0';
            }
        }
        else if (n < 0)
        {
            p_buf[p_size - 2] = '\n';
            p_buf[p_size - 1] = '\0';
            n = p_size;
            break;
        }
        else
            break;
    }

    fd = __SYSLOG_Init();
    if (fd < 0)
    {
        fprintf(stderr, "%s", p_buf);
        return;
    }

    if (syslogbuf == NULL)
    {
        sc_write(fd, p_buf, n);
    }
    else
    {
        if (syslogbuf_pos + n > SYSLOGBUFSZ)
        {
            sc_write(fd, syslogbuf, syslogbuf_pos);
            syslogbuf_pos = 0;
        }

        sc_memcpy(syslogbuf + syslogbuf_pos, p_buf, n);
        syslogbuf_pos += n;
    }

    if (fd != fileno(stderr))
        fprintf(stderr, "%s", p_buf);

    if (syslog_file[0] != '\0')
        sc_close(fd);


    if (p_buf != buf)
        PMEM_FREE(p_buf);

}
#endif

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
extern void dbi_GetServerLogPath(char *pSrvPath);
extern __DECL_PREFIX char SERVERLOG_PATH[];

static int
__SYSLOG_Init(void)
{
    int seek_file;
    int fd = -1;

    if (syslog_fInit == FALSE)
    {
        if (sc_getenv("MOBILE_LITE_CONFIG") == NULL)
        {
            syslog_file[0] = '\0';

            return -1;
        }
        else
        {
            static char __serverlog_path[256] = "";

#ifdef WIN32_MOBILE
            int f_repath = 0;
#endif

            if (__serverlog_path[0] == '\0')
            {
                dbi_GetServerLogPath(__serverlog_path);
            }

#ifdef WIN32_MOBILE
          repath_serverlog:
#endif

            sc_sprintf(syslog_file, "%s" DIR_DELIM "%s",
                    __serverlog_path, LOGFILENAME);

            fd = sc_open(syslog_file, O_RDWR | O_CREAT | O_APPEND | O_BINARY,
                    OPEN_MODE);

            if (fd < 0)
            {
#ifdef WIN32_MOBILE
                if (f_repath == 0)
                {
                    SHGetSpecialFolderPath(NULL, __serverlog_path,
                            CSIDL_DESKTOP, FALSE);
                    f_repath = 1;
                    goto repath_serverlog;
                }
#endif
                syslog_file[0] = '\0';
                return fd;
            }
        }

        if (syslogbuf == NULL)
            syslogbuf = (char *) SMEM_ALLOC(SYSLOGBUFSZ);
        syslogbuf_pos = 0;
        syslog_fInit = TRUE;

        return fd;
    }

    if (syslog_file[0] == '\0')
    {
        return -1;
    }

    fd = sc_open(syslog_file, O_RDWR | O_CREAT | O_APPEND | O_BINARY,
            OPEN_MODE);
    if (fd < 0)
        return -1;

    seek_file = sc_lseek(fd, 0, SEEK_END);

    if (seek_file > DB_Params.serverlog_size)
    {
        static unsigned int file_no = 1;
        char buf[128];

        __SYSLOG_SETTIME(buf, sizeof(buf));

        /* 새로 만듬 */
        sc_close(fd);

        rotate_log_file(syslog_file);

        fd = sc_open(syslog_file, O_RDWR | O_CREAT | O_TRUNC | O_BINARY,
                OPEN_MODE);
        if (fd >= 0)
        {
            int n;

            n = sc_sprintf(buf, "\n%s %s", _DBNAME_, _DBVERSION_);
            sc_write(fd, buf, n);

#ifdef MDB_DEBUG
            n = sc_sprintf(buf, " - DEBUG MODE\n");
#else
            n = sc_sprintf(buf, "\n");
#endif
            sc_write(fd, buf, n);

            n = sc_sprintf(buf,
                    "--------------------------------------------\n");
            sc_write(fd, buf, n);
            n = sc_sprintf(buf, "%s\n\n", _COPYRIGHT_);
            sc_write(fd, buf, n);
            n = sc_sprintf(buf, "[Compilation Time: %s %s]\n\n",
                    __DATE__, __TIME__);
            sc_write(fd, buf, n);
            n = sc_sprintf(buf, "[%d]\n\n", file_no);
            sc_write(fd, buf, n);
            file_no++;
        }
        else
            return -1;
    }

    return fd;
}

void
__SYSLOG_syncbuf(int f_discard)
{
    int fd;

    if (syslogbuf == NULL)
    {
        syslogbuf_pos = 0;
        return;
    }

    if (syslogbuf_pos == 0)
    {
        SMEM_FREENUL(syslogbuf);
        return;
    }

    if (f_discard == 0)
    {
        fd = __SYSLOG_Init();
        if (fd < 0)
        {
            syslogbuf_pos = 0;
        }
        else
        {
            sc_write(fd, syslogbuf, syslogbuf_pos);

            if (syslog_file[0] != '\0')
                sc_close(fd);
        }
    }

    SMEM_FREENUL(syslogbuf);
    syslogbuf_pos = 0;
}

void
__SYSLOG_close(void)
{
    int fd;

    if (syslogbuf == NULL)
    {
        syslogbuf_pos = 0;
        syslog_fInit = FALSE;
        return;
    }

    if (syslogbuf_pos == 0)
    {
        SMEM_FREENUL(syslogbuf);
        syslog_fInit = FALSE;
        return;
    }

    fd = __SYSLOG_Init();
    if (fd >= 0)
    {
        sc_write(fd, syslogbuf, syslogbuf_pos);
        if (syslog_file[0] != '\0')
            sc_close(fd);
    }

    SMEM_FREENUL(syslogbuf);
    syslogbuf_pos = 0;
    syslog_fInit = FALSE;

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
void
rotate_log_file(char *logfile)
{
    char oldFileName[256];
    char newFileName[256];
    int i;

    if (DB_Params.serverlog_num <= 1)
        return;

    for (i = DB_Params.serverlog_num - 2; i > 0; i--)
    {
        /* ".txt" 제거 */
        sc_sprintf(oldFileName, "%s.%d", logfile, i - 1);
        sc_sprintf(newFileName, "%s.%d", logfile, i);

        sc_unlink(newFileName);
        sc_rename(oldFileName, newFileName);
    }
    sc_sprintf(newFileName, "%s.0", logfile);

    sc_unlink(newFileName);
    sc_rename(logfile, newFileName);
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
__DECL_PREFIX void
__SYSLOG_TIME(char *format, ...)
{
    va_list ap;
    char buf[64];

    __SYSLOG_SETTIME(buf, sizeof(buf));

    /* Print local time as a string */
    __SYSLOG("[%s]", buf);

    va_start(ap, format);

    __SYSLOG(format, ap);

    va_end(ap);
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
#if defined(WIN32_MOBILE) || defined(linux) || defined(sun)
void
__SYSLOG2_TIME(char *format, ...)
{
    va_list ap;
    char buf[64];

    __SYSLOG_SETTIME(buf, sizeof(buf));

    /* Print local time as a string */
    __SYSLOG2("[%s]", buf);

    va_start(ap, format);

    __SYSLOG2(format, ap);

    va_end(ap);
}
#endif

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
static char *_wday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

static char *_mon[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static void
__SYSLOG_SETTIME(char *buf, int bufsize)
{
    struct db_tm tm;
    db_time_t aclock;

    aclock = sc_time(NULL);     /* Get time in seconds */
    sc_localtime_r(&aclock, &tm);       /* Convert time to struct */
    /* tm form */

    sc_sprintf(buf, "%s %s %d %02d:%02d:%02d %d",
            _wday[tm.tm_wday], _mon[tm.tm_mon], tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_year + 1900);

    /*sc_asctime_r(&newtime, buf);

       buf[sc_strlen(buf)-1] = '\0'; */

    return;
}
