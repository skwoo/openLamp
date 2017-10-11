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

#ifndef __COMPAT_H
#define __COMPAT_H

#include "mdb_config.h"
#include "dbport.h"
#include "mdb_typedef.h"
#include "sc_stdio.h"
#include "systable.h"
#include "mdb_Cont.h"
#include "mdb_TTree.h"

#define mdb_malloc_fixed                sc_malloc
#define mdb_calloc_fixed                sc_calloc
#define mdb_strdup_fixed                sc_strdup
#define mdb_free_fixed                  sc_free
#define mdb_malloc_conn                 sc_malloc
#define mdb_calloc_conn                 sc_calloc
#define mdb_strdup_conn                 sc_strdup
#define mdb_free_conn                   sc_free
#define mdb_malloc_sys                  sc_malloc
#define mdb_calloc_sys                  sc_calloc
#define mdb_strdup_sys                  sc_strdup
#define mdb_free_sys                    sc_free
#ifdef  mdb_malloc
#undef  mdb_malloc
#endif
#ifdef  mdb_free
#undef  mdb_free
#endif
#define mdb_malloc                      sc_malloc
#define mdb_calloc                      sc_calloc
#define mdb_strdup                      sc_strdup
#define mdb_free                        sc_free
#define HeapManager_default_init()      {;}
#define HeapManager_server_init()       {;}
#define HeapManager_client_init()       {;}
#define HeapManager_init()              {;}
#define HeapManager_final()             {;}

/************** 내부 sc api **************/
extern void __reverse_str(char *str, int j);
extern int __values[16];

#define isLeapYear(year) ((((year) & 0x3) == 0) && (((year) % 100 != 0) || ((year) % 400 == 0)))

/* mdb_itoa와 sc_bitoa의 return 값은 string의 길이 */

__DECL_PREFIX int mdb_itoa(int value, char *str, int radix);
__DECL_PREFIX int mdb_strlcpy(char *s1, const char *s2, int n);
__DECL_PREFIX int MDB_bitoa(bigint value, char *str, int radix);
__DECL_PREFIX bigint sc_xatoll(const char *str);

__DECL_PREFIX int sc_memdcmp(const void *s1, const void *s2, size_t n);
__DECL_PREFIX int sc_strdcmp(const char *str1, const char *str2);
__DECL_PREFIX int sc_strndcmp(const char *s1, const char *s2, int n);

/************** 공통 선언 **************/
struct db_tm *gmtime_as(const DB_INT64 * p_clock, struct db_tm *res);
struct db_tm *localtime_as(const DB_INT64 * p_clock, struct db_tm *res);
__DECL_PREFIX DB_INT64 mktime_as(struct db_tm *timeptr);
DB_INT64 mktime_as_2(struct db_tm *timeptr);
__DECL_PREFIX int __check_dbname(char *dbname, char *dbfilename);
DB_INT64 _tm2time(struct db_tm *st);


/************** 특정 시스템에 없는 함수 **************/
#ifdef _AS_WINCE_
int shmctl(int shmid, int cmd, struct shmid_ds *buf);
void *shmat(int shmid, const void *shmaddr, int shmflg);
int shmdt(char *shmaddr);
#endif

#if !defined(MDB_DEBUG)
#define umask(m) {}
#endif

#if defined(sun) || defined(linux)
#define    closesocket    close
#else
#define    closesocket
#endif

extern double pow(double x, double y);
extern double rint(double x);

extern double floor(double x);
extern double ceil(double x);

extern __DECL_PREFIX void convert_dir_delimiter(char *path);
extern int sc_maxdayofmonth(int year, int month);

extern int mdb_binary2bstring(char *src, char *dst, int src_length);
extern int mdb_binary2hstring(char *src, char *dst, int src_length);

__DECL_PREFIX int mdb_debuglog(char *str, ...);


#define MDB_TOUPPER_M(c)   (  ((c) >= 'a' && (c) <= 'z') ?    (c)-32 :  (c) )
#define MDB_TOWUPPER_M(c)  (  ((c) >= L'a' && (c) <= L'z') ?  (c)-32 :  (c) )

DB_UINT32 mdb_MakeChecksum(char *buf, int buflen, int offset);
int mdb_CompareChecksum(char *buf, int buflen, int offset, void *checksum);

#define REVERSE_DELIMETER_UPPER     'P'
#define REVERSE_DELIMETER_LOWER     'p'
#define IS_REVERSE_DELIMITER(c)     ( ((c) == 'p') || ((c) == 'P') )

extern int mdb_strrcmp(const char *s1, const char *s2);
extern int mdb_strnrcmp(const char *s1, const char *s2, int n);
extern int mdb_strnrcmp2(const char *s1, int s1_len, const char *s2,
        int s2_len, int n);
extern void *mdb_memrcpy(void *dest, const void *src, size_t n);
extern char *mdb_strrstr(const char *haystack, const char *needle);
extern char *mdb_strrchr(const char *s, int c);
extern char *mdb_strrcpy(char *dest, const char *src);
extern size_t mdb_strrlen(const char *s);


extern int _timegapTZ;

extern int _mdb_db_lock_depth;

#if defined(MDB_DEBUG)
extern __DECL_PREFIX void mdb_db_lock(char *__file__, const char *func,
        int __line__);
extern __DECL_PREFIX void mdb_db_unlock(char *__file__, const char *func,
        int __line__);
#ifdef WIN32
#define MDB_DB_LOCK()   mdb_db_lock(__FILE__, "Unknown", __LINE__)
#define MDB_DB_UNLOCK() mdb_db_unlock(__FILE__, "Unknown", __LINE__)
#else // STX removed
#define MDB_DB_LOCK()   mdb_db_lock(__FILE__, __func__, __LINE__)
#define MDB_DB_UNLOCK() mdb_db_unlock(__FILE__, __func__, __LINE__)
#endif // WIN32
#else
extern __DECL_PREFIX void mdb_db_lock(void);
extern __DECL_PREFIX void mdb_db_unlock(void);

#define MDB_DB_LOCK()   mdb_db_lock()
#define MDB_DB_UNLOCK() mdb_db_unlock()
#endif // MDB_DEBUG

#define MDB_DB_ISLOCKED()                                               \
    do                                                                  \
    {                                                                   \
        sc_assert(server->_mdb_db_lock_depth != 0, __FILE__, __LINE__); \
        sc_assert((server->_mdb_db_owner_thrid == ppthread_self() &&    \
                    server->_mdb_db_owner_pid == sc_getpid()),          \
                __FILE__, __LINE__);                                    \
    } while (0);

#define MDB_DB_LOCK_API()       MDB_DB_LOCK()
#define MDB_DB_UNLOCK_API()     MDB_DB_UNLOCK()
#define MDB_DB_LOCK_SQL_API()   MDB_DB_LOCK()
#define MDB_DB_UNLOCK_SQL_API() MDB_DB_UNLOCK()

#define POP_G_CONNID()          { _g_connid = _g_connid_bk;}
#define POP_G_CONNID_SQL_API()  { _g_connid = _g_connid_bk;}

__DECL_PREFIX int get_mdb_db_lock_depth(void);

#define PUSH_G_CONNID(connid)                   \
    {                                           \
        int get_mdb_db_lock_depth(void);        \
        if (get_mdb_db_lock_depth() == 0)       \
            sc_assert(0, __FILE__, __LINE__);   \
        _g_connid_bk = _g_connid;               \
        if ((connid) != -1)                     \
            _g_connid = (connid);               \
    }
#define PUSH_G_CONNID_SQL_API(connid)           \
    {                                           \
        int get_mdb_db_lock_depth(void);        \
        if (get_mdb_db_lock_depth() == 0)       \
            sc_assert(0, __FILE__, __LINE__);   \
        _g_connid_bk = _g_connid;               \
        if ((connid) != -1)                     \
            _g_connid = (connid);               \
    }

extern int mdb_sort_array(void *pszOids[], int nMaxCnt,
        int (*comp_func) (const void *, const void *), int IsAsc);

__DECL_PREFIX int MDB_FLOAT2STR(char *str, int buf_size, float f);
__DECL_PREFIX int MDB_DOUBLE2STR(char *str, int buf_size, double f);

extern int mdb_check_numeric(char *src, DB_BOOL * is_negative_p,
        DB_BOOL * is_float_p);
extern bigint mdb_char2bigint(char *str);

__DECL_PREFIX extern const unsigned char lower_array[];

#define mdb_tolwr(str_) lower_array[(unsigned char)(str_)]

#define mdb_strcmp(s1_, s2_)                                            \
    ((((unsigned char *)(s1_))[0] == ((unsigned char *)(s2_))[0]) ?     \
     sc_strcmp((s1_), (s2_)) :                                          \
     (((unsigned char *)(s1_))[0] - ((unsigned char *)(s2_))[0]))

#define mdb_strncmp(s1_, s2_, n_)                                       \
    ((n_) < 1 ? 0 : ((n_) > 1 &&                                        \
        (((unsigned char *)(s1_))[0] == ((unsigned char *)(s2_))[0]) ?  \
        sc_strncmp((s1_), (s2_), (n_)) :                                \
        (((unsigned char *)(s1_))[0] - ((unsigned char *)(s2_))[0])))

#define mdb_strcasecmp(s1_, s2_)                                        \
    ((mdb_tolwr(((unsigned char *)(s1_))[0]) ==                         \
      mdb_tolwr(((unsigned char *)(s2_))[0])) ?                         \
     sc_strcasecmp((s1_), (s2_)) :                                      \
     (mdb_tolwr(((unsigned char *)(s1_))[0]) -                          \
      mdb_tolwr(((unsigned char *)(s2_))[0])))

#define mdb_strncasecmp(s1_, s2_, n_)                                   \
    ((n_) < 1 ? 0 : ((n_) > 1 &&                                        \
        (mdb_tolwr(((unsigned char *)(s1_))[0]) ==                      \
         mdb_tolwr(((unsigned char *)(s2_))[0])) ?                      \
        sc_strncasecmp((s1_), (s2_), (n_)) :                            \
        (mdb_tolwr(((unsigned char *)(s1_))[0]) -                       \
         mdb_tolwr(((unsigned char *)(s2_))[0]))))

extern char *mdb_strlwr(char *str);

#endif /* __COMPAT_H */
