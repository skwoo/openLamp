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

#ifndef __DBPORT_H
#define __DBPORT_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include "mdb_config.h"
#include "sys_inc.h"

#if defined(WIN32)
#define I64_FORMAT  "%I64d"
#define I64x_FORMAT "%I64x"
#define I64_FORMAT3 "%03I64d"
#define MDB_NEWLINE "\r\n"

#else
#define I64_FORMAT  "%lld"
#define I64x_FORMAT "%llx"
#define I64_FORMAT3 "%03lld"
#define MDB_NEWLINE "\r\n"      /* unify every OS */
#endif

    typedef unsigned char uchar;

#if defined(WIN32)
    typedef signed __int64 DB_INT64;
    typedef unsigned __int64 DB_UINT64;
#else
    typedef signed long long DB_INT64;
    typedef unsigned long long DB_UINT64;
#endif

    typedef signed char DB_INT8;
    typedef short DB_INT16;
    typedef int DB_INT32;
    typedef long DB_LONG;
    typedef unsigned char DB_UINT8;
    typedef unsigned short DB_UINT16;
    typedef unsigned int DB_UINT32;
    typedef unsigned long DB_ULONG;

    typedef DB_UINT8 DB_BOOL;

#ifndef DB_TRUE
#define DB_TRUE             1
#endif

#ifndef DB_FALSE
#define DB_FALSE            0
#endif

    typedef DB_INT64 bigint;

#if defined(_GCC)
#define MDB_INLINE inline
#else
#define MDB_INLINE
#endif

#ifndef byte_defined
#define byte_defined
    typedef unsigned char DB_BYTE;      /* Smallest addressable unit */
#endif

#ifdef USE_WCHAR_H
    typedef wchar_t DB_WCHAR;
#else
    typedef unsigned short DB_WCHAR;
#endif

#define DB_WCHAR_INTMAX     MDB_INT_MAX
#ifdef USE_WCHAR_H
#define DB_WCHAR_SHRTMAX    MDB_SHRT_MAX
#else
#define DB_WCHAR_SHRTMAX    0xFFFF
#endif

#define DB_WCHAR_INTMIN     0
#define DB_WCHAR_SHRTMIN    0

#define MDB_INT64   DB_INT64
#define MDB_UINT64  DB_UINT64
#define MDB_INT64   DB_INT64
#define MDB_INT8    DB_INT8
#define MDB_INT16   DB_INT16
#define MDB_INT32   DB_INT32
#define MDB_UINT8   DB_UINT8
#define MDB_UINT16  DB_UINT16
#define MDB_UINT32  DB_UINT32
#define MDB_BOOL    DB_BOOL
#define MDB_FALSE   DB_FALSE
#define MDB_TRUE    DB_TRUE
#define MDB_BYTE    DB_BYTE

#define    MDB_LOCK_UNKNOWN     -1
#define    MDB_LOCK_SET         1
#define    MDB_LOCK_SKIP        2


#define DEFAULT_SOCK_PORTNO     (6600)

#if defined(linux) || defined(sun)
#define MDB_PID         MDB_INT32
#define MDB_MUTEX       pthread_mutex_t
#define MDB_CV          pthread_cond_t
#elif defined(WIN32)
#define MDB_PID         DWORD
#define MDB_MUTEX       CRITICAL_SECTION
#define MDB_CV          pthread_cond_t
#else
#define MDB_PID         MDB_INT32
#define MDB_MUTEX       MDB_INT32
#endif

#ifndef MDB_CHAR_MAX
#define MDB_CHAR_MAX        127
#endif

#ifndef MDB_CHAR_MIN
#define MDB_CHAR_MIN        0
#endif

#ifndef MDB_UCHAR_MAX
#define MDB_UCHAR_MAX       255
#endif

#ifndef MDB_TINYINT_MIN
#define MDB_TINYINT_MIN     -128
#endif

#ifndef MDB_TINYINT_MAX
#define MDB_TINYINT_MAX     127
#endif

#ifndef MDB_SHRT_MAX
#define MDB_SHRT_MAX        32767
#endif

#ifndef MDB_SHRT_MIN
#define MDB_SHRT_MIN        (-32768)
#endif

#ifndef MDB_INT_MIN
#define MDB_INT_MIN         (-2147483647-1)
#endif

#ifndef MDB_INT_MAX
#define MDB_INT_MAX         2147483647
#endif

#ifndef MDB_UINT_MAX
#define MDB_UINT_MAX        0xffffffff
#endif

#ifndef MDB_FLT_MAX
#define MDB_FLT_MAX         3.402823466e+38F
#endif

#ifndef MDB_FLT_MIN
#define MDB_FLT_MIN         1.175494351e-38F    /* min positive value */
#endif
#ifndef MDB_DBL_MAX
#define MDB_DBL_MAX         1.7976931348623158e+308     /* max value */
#endif

#ifndef MDB_DBL_MIN
#define MDB_DBL_MIN         2.2250738585072014e-308     /* min positive value */
#endif

#if defined(WIN32)

#ifndef MDB_LLONG_MAX
#define MDB_LLONG_MAX       9223372036854775807
#endif

#ifndef MDB_LLONG_MIN
#define MDB_LLONG_MIN       (-9223372036854775807-1)
#endif

#define MDB_LLONG_MAX_18    922337203685477580
#define MDB_LLONG_MIN_18    922337203685477580

#else

#ifndef MDB_LLONG_MAX
#define MDB_LLONG_MAX       9223372036854775807LL
#endif

#ifndef MDB_LLONG_MIN
#define MDB_LLONG_MIN       (-9223372036854775807LL-1LL)
#endif

#define MDB_LLONG_MAX_18    922337203685477580LL
#define MDB_LLONG_MIN_18    922337203685477580LL

#endif

#if defined(_AS_WINCE_)

#define O_RDONLY        0x0000
#define O_WRONLY        0x0001
#define O_RDWR          0x0002
#define O_APPEND        0x0008
#define O_CREAT         0x0100
#define O_TRUNC         0x0200
#define O_EXCL          0x0400
#define O_BINARY        0x8000
#define _S_IREAD        0000400
#define _S_IWRITE       0000200

#endif


#if !defined(O_SYNC)
#define O_SYNC              0
#endif

#if !defined(O_BINARY)
#define O_BINARY            0
#endif

#ifndef S_IRWXU
#define S_IRWXU             0
#endif

#ifndef S_IRWXG
#define S_IRWXG             0
#endif

#ifndef S_IRWXO
#define S_IRWXO             0
#endif

#ifndef EINVAL
#define EINVAL  -1
#endif

#ifndef ENOMEM
#define ENOMEM  -1
#endif

#ifdef USE_ERROR
    __DECL_PREFIX extern int errno;
#endif

#ifndef SEEK_SET
#define SEEK_SET 0              /* start of stream (see fseek) */
#endif

#ifndef SEEK_CUR
#define SEEK_CUR 1              /* current position in stream (see fseek) */
#endif

#ifndef SEEK_END
#define SEEK_END 2              /* end of stream (see fseek) */
#endif

#if defined(_AS_WINCE_) || defined(_AS_WIN32_MOBILE_)
#define OPEN_MODE    _S_IREAD | _S_IWRITE
#else
#define OPEN_MODE    S_IRWXU | S_IRWXG | S_IRWXO
#endif

/************** db structures **************/
    struct db_timeval
    {
        long tv_sec;            /* seconds since Jan. 1, 1970 */
        long tv_usec;           /* and microseconds */
    };
    typedef struct db_timeval T_DB_TIMEVAL;

    struct db_tm
    {
        int tm_sec;             /* seconds after the minute - [0,59] */
        int tm_min;             /* minutes after the hour - [0,59] */
        int tm_hour;            /* hours since midnight - [0,23] */
        int tm_mday;            /* day of the month - [1,31] */
        int tm_mon;             /* months since January - [0,11] */
        int tm_year;            /* years since 1900 */
        int tm_wday;            /* days since Sunday - [0,6] */
        int tm_yday;            /* days since January 1 - [0,365] */
        int tm_isdst;           /* daylight savings time flag */
    };
/* localtime_as and gmtime_as do not set tm_wday and tm_yday */

    typedef time_t db_time_t;

    typedef struct db_timespec
    {
        db_time_t tv_sec;       /* seconds */
        long tv_nsec;           /* and nanoseconds */
    } db_timespec_t;

    typedef enum
    {
        MDB_ENDIAN_NO_INFORMATION = 0,
        MDB_ENDIAN_LITTLE,
        MDB_ENDIAN_MIDDLE,
        MDB_ENDIAN_BIG,
        MDB_ENDIAN_DUMMY = MDB_INT_MAX  /* to guarantee sizeof(enum) == 4 */
    } MDB_ENDIAN_TYPE;
    typedef struct _createdboption
    {
        int num_seg;
        int idxseg_num;
        int tmpseg_num;
        MDB_ENDIAN_TYPE endian_type;
    } DB_CREATEDB;

/************** sc api **************/
    __DECL_PREFIX int sc_creat(const char *path, unsigned int mode);
    __DECL_PREFIX int sc_open(const char *path, int oflag, unsigned int mode);
    __DECL_PREFIX int sc_close(int fildes);
    __DECL_PREFIX long sc_read(int fd, void *buf, size_t nbyte);
    __DECL_PREFIX long sc_write(int fd, const void *buf, size_t nbyte);
    __DECL_PREFIX int sc_rename(const char *old_name, const char *new_name);
    __DECL_PREFIX int sc_unlink(const char *path);
    __DECL_PREFIX int sc_unlink_prefix(const char *path, const char *prefix);
    __DECL_PREFIX int sc_truncate(const char *path, int length);
    __DECL_PREFIX int sc_ftruncate(int fd, int length);
    __DECL_PREFIX int sc_lseek(int fildes, int offset, int whence);
    __DECL_PREFIX int sc_fsync(int fildes);

    __DECL_PREFIX char *sc_strlwr(char *str);

    __DECL_PREFIX int sc_strlower(char *dest, const char *src);
    __DECL_PREFIX int sc_strupper(char *dest, const char *src);
    __DECL_PREFIX int sc_wcslower(DB_WCHAR * dest, const DB_WCHAR * src);
    __DECL_PREFIX int sc_wcsupper(DB_WCHAR * dest, const DB_WCHAR * src);

    __DECL_PREFIX int sc_mkdir(const char *path, unsigned int mode);
    __DECL_PREFIX int sc_rmdir(const char *path);
    __DECL_PREFIX double sc_rint(double x);
    __DECL_PREFIX int sc_gettimeofday(struct db_timeval *tp, void *temp);
    __DECL_PREFIX db_time_t sc_time(db_time_t * tloc);
    __DECL_PREFIX bigint sc_atoll(char *str);
    __DECL_PREFIX int sc_getpid(void);
    __DECL_PREFIX char *sc_getenv(char *str);

    __DECL_PREFIX char *sc_strdup(const char *s);
    __DECL_PREFIX char *sc_strerror(int errnum);
    __DECL_PREFIX int sc_setenv(char *name, char *value, int overwrite);
/*
 * sc_gmtime_r()
 *    used only in sc_getTimeGapTZ(). if not used in sc_getTimeGapTZ(), this
 *    function has not to be implemented.
 */
    __DECL_PREFIX struct db_tm *sc_gmtime_r(const db_time_t * p_clock,
            struct db_tm *res);
    __DECL_PREFIX db_time_t sc_mktime(struct db_tm *timeptr);
    __DECL_PREFIX int sc_strcasecmp(const char *s1, const char *s2);
    __DECL_PREFIX int sc_strncasecmp(const char *s1, const char *s2, size_t n);
    __DECL_PREFIX struct db_tm *sc_localtime_r(const db_time_t * p_clock,
            struct db_tm *res);
    __DECL_PREFIX int sc_getTimeGapTZ(void);
    __DECL_PREFIX DB_INT32 sc_wcscmp(const DB_WCHAR * s1, const DB_WCHAR * s2);
    __DECL_PREFIX DB_INT32 sc_wcsncmp(const DB_WCHAR * s1, const DB_WCHAR * s2,
            DB_INT32 n);
    __DECL_PREFIX DB_INT32 sc_wcscasecmp(const DB_WCHAR * s1,
            const DB_WCHAR * s2);
    __DECL_PREFIX DB_INT32 sc_wcsncasecmp(const DB_WCHAR * s1,
            const DB_WCHAR * s2, int n);
    __DECL_PREFIX DB_INT32 sc_wmemcmp(const DB_WCHAR * s1, const DB_WCHAR * s2,
            DB_INT32 n);
    __DECL_PREFIX DB_INT32 sc_wcslen(const DB_WCHAR * wstring);
    __DECL_PREFIX DB_WCHAR *sc_wcsncpy(DB_WCHAR * dest, const DB_WCHAR * src,
            DB_INT32 n);
    __DECL_PREFIX DB_WCHAR *sc_wcscpy(DB_WCHAR * dest, const DB_WCHAR * src);
    __DECL_PREFIX void *sc_wmemcpy(DB_WCHAR * dest, const DB_WCHAR * src,
            DB_INT32 n);
    __DECL_PREFIX DB_WCHAR *sc_wmemset(DB_WCHAR * wcs, DB_WCHAR wc,
            DB_INT32 n);
    __DECL_PREFIX DB_WCHAR *sc_wmemmove(DB_WCHAR * dest, DB_WCHAR * src,
            DB_INT32 n);
    __DECL_PREFIX DB_WCHAR *sc_wcsstr(const DB_WCHAR * haystack,
            const DB_WCHAR * needle);
    __DECL_PREFIX DB_WCHAR *sc_wcschr(const DB_WCHAR * wcs, DB_WCHAR wc);
    __DECL_PREFIX DB_WCHAR *sc_wcsrchr(const DB_WCHAR * wcs, DB_WCHAR wc);

    extern __DECL_PREFIX MDB_PID sc_get_taskid(void);

    extern __DECL_PREFIX void sc_sleep(MDB_INT32 mSec);
    __DECL_PREFIX MDB_INT32 sc_db_lock_init(void);
    MDB_INT32 sc_db_lock_destroy(void);

#ifdef MDB_DEBUG
#define sc_db_lock() sc_db_lock_debug(__FILE__, __LINE__)
#define sc_db_unlock() sc_db_unlock_debug(__FILE__, __LINE__)
    extern __DECL_PREFIX MDB_INT32 sc_db_lock_debug(char *file, int line);
    extern __DECL_PREFIX MDB_INT32 sc_db_unlock_debug(char *file, int line);
#else
    extern __DECL_PREFIX MDB_INT32 sc_db_lock(void);
    extern __DECL_PREFIX MDB_INT32 sc_db_unlock(void);
#endif

#if defined(linux)
#define sc_assert(a,b,c) assert((a))
#else
    __DECL_PREFIX void sc_assert(int expression, char *file, int line);
#endif

    __DECL_PREFIX int sc_wcstombs(char *dest, const DB_WCHAR * src, int n);
    __DECL_PREFIX int sc_mbstowcs(DB_WCHAR * dest, const char *src, int n);

    __DECL_PREFIX int sc_vsprintf(char *buffer, const char *format,
            va_list ap);

    __DECL_PREFIX int sc_sprintf(char *buffer, const char *format, ...);

    __DECL_PREFIX int sc_vsnprintf(char *buf, int size, const char *format,
            va_list ap);

    __DECL_PREFIX int sc_snprintf(char *buffer, size_t count,
            const char *format, ...);


#if defined(sun)
#define sc_malloc(size) memalign(8, size)
#else
    __DECL_PREFIX void *sc_malloc(size_t size);
#endif
    __DECL_PREFIX void sc_free(void *ptr);
    __DECL_PREFIX void *sc_calloc(size_t nmemb, size_t size);
    __DECL_PREFIX void *sc_realloc(void *ptr, size_t size);

    __DECL_PREFIX int sc_memcmp(const void *s1, const void *s2, size_t n);

#if defined(linux)
#define sc_memcpy   memcpy
#else
    __DECL_PREFIX void *sc_memcpy(void *s1, const void *s2, size_t len);
#endif

#if defined(linux)
#define sc_memset   memset
#else
    __DECL_PREFIX void *sc_memset(void *s, int c, size_t n);
#endif

    __DECL_PREFIX void *sc_memmove(void *dest, const void *src, size_t n);

    __DECL_PREFIX char *sc_strcat(char *dest, const char *src);

    __DECL_PREFIX char *sc_strchr(const char *s, int c);

    __DECL_PREFIX char *sc_strrchr(const char *s, int c);

    __DECL_PREFIX int sc_strcmp(const char *str1, const char *str2);

    __DECL_PREFIX int sc_strncmp(const char *s1, const char *s2, int n);

    __DECL_PREFIX char *sc_strcpy(char *s1, const char *s2);

    __DECL_PREFIX char *sc_strncpy(char *s1, const char *s2, int n);

    __DECL_PREFIX int sc_strlen(const char *str);

    __DECL_PREFIX char *sc_strstr(const char *haystack, const char *needle);

    __DECL_PREFIX char *sc_strcasestr(const char *haystack,
            const char *needle);

    __DECL_PREFIX char *sc_strtok_r(char *s, const char *delim, char **ptrptr);

    __DECL_PREFIX int sc_atoi(const char *nptr);

    __DECL_PREFIX long sc_atol(const char *nptr);

    __DECL_PREFIX double sc_atof(const char *nptr);

    __DECL_PREFIX long int sc_strtol(const char *nptr, char **endptr,
            int base);

#if defined(linux)
#define sc_toupper toupper
#else
    __DECL_PREFIX int sc_toupper(int c);
#endif

    __DECL_PREFIX int sc_tolower(int c);

    __DECL_PREFIX double sc_pow(double x, double y);

    __DECL_PREFIX int sc_rand(void);

    __DECL_PREFIX void sc_srand(unsigned int seed);

    __DECL_PREFIX int sc_printf(const char *format, ...);
    __DECL_PREFIX double sc_floor(double x);
    __DECL_PREFIX double sc_ceil(double x);
    __DECL_PREFIX int sc_abs(int j);

    int sc_bstring2binary(char *src, char *dst);
    int sc_hstring2binary(char *src, char *dst);

#define INIT_gEDBTaskId -100
#define INIT_gEDBTaskId_pid INIT_gEDBTaskId
#define INIT_gEDBTaskId_thrid -99

    __DECL_PREFIX int sc_get_disk_free_space(const char *path);

    __DECL_PREFIX int sc_fgetc(int fd);
    __DECL_PREFIX void sc_kick_dog(void);

#if defined WIN32
#define __func__    "N/A"
#endif

    __DECL_PREFIX int sc_file_errno(void);

    __DECL_PREFIX void sc_add_errmsg2queue(char *pmsg);


#ifdef  __cplusplus
}
#endif

#endif
