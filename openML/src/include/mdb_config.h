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

#ifndef __MDB__CONFIG_H
#define __MDB__CONFIG_H

#ifdef  __cplusplus
extern "C"
{
#endif

// case1. compile in windows visual studio(win32).
#ifdef _WIN32
    // for Windows 32
#ifndef WIN32
#define WIN32
#endif

#ifdef _WIN32_WCE
    // case. 1. 1. WINCE
#define _AS_WINCE_
#define _ONE_BDB_               /* one backup db */
    /* use one backup db in buffer replacement environment */
#else
#if defined(WIN32_MOBILE)
    // case. 1.2.2. WIN32 MOBILE MOCE
#define _AS_WIN32_MOBILE_
#define _ONE_BDB_               /* one backup db */
#endif
#endif
#endif                          /* _WIN32 */

    /* Change__DECL_PREFIX */
#if defined(MOBILEDB_EXPORTS)
#define __DECL_PREFIX       __declspec(dllexport)
#elif defined(MOBILEDB_IMPORTS)
#define __DECL_PREFIX       __declspec(dllimport)
#elif defined(HIDDEN__DECL_PREFIX)
#define __DECL_PREFIX
#elif defined(linux)
#define __DECL_PREFIX       __attribute__((visibility("default")))
#else
#define __DECL_PREFIX
#endif

// case2. compile in Unix(SUN)
#if defined(__sun) && !defined(sun)
#define sun
#endif

// case3. compile in Unix(LINUX)
#if defined(__linux) && !defined(linux)
#define linux
#endif

// case4. compile in psos
#if defined(__psos) && !defined(psos)
#define psos
#endif

#if defined(ANDROID_OS)
#ifndef linux                   // avoid 'redefined' warning
#define linux
#endif
#endif


/*******************************************************************
defines for functions
#define _ONE_BDB_
#define USE_PPTHREAD        : pthread
#define USE_ERROR            : if system has not errno
#define USE_WCHAR_H            : wchar_t
#define USE_DWORD_T
#define USE_PACKET_SIZE_256 : DEFAULT_PACKET_SIZE
#define CREATEDB_SEG_UNIT n
#define DIR_DELIM   "\\" or "/"
#define DB_LOCALCODE_SIZE_MAX   : multibyte string's maxlength
#define USE_UCS_2_0
*******************************************************************/

#if defined(sun)
#define _ONE_BDB_
#define USE_WCHAR_H
#define CREATEDB_SEG_UNIT   1
#define DIR_DELIM                   "/"
#define DIR_DELIM_CHAR        '/'
#define DB_LOCALCODE_SIZE_MAX 3
#define USE_UCS_2_0
#endif

#if defined(linux)
#define USE_PPTHREAD
//#      define USE_WCHAR_H
#define CREATEDB_SEG_UNIT  1
#define DIR_DELIM           "/"
#define DIR_DELIM_CHAR        '/'
#define USE_PACKET_SIZE_256
#define _ONE_BDB_               /* one backup db */
#define DB_LOCALCODE_SIZE_MAX 3
#define USE_UCS_2_0
#endif

#if defined(_AS_WINCE_)
#define _ONE_BDB_
#define USE_PPTHREAD
#define USE_ERROR
#define CREATEDB_SEG_UNIT   1
#define DIR_DELIM           "\\"
#define DIR_DELIM_CHAR        '\\'
#define DB_LOCALCODE_SIZE_MAX 3
#define SERVERLOG_PATH_ON_SYSTEMROOT
#elif defined(_AS_WIN32_MOBILE_)
#define _ONE_BDB_
#define USE_PPTHREAD
#define CREATEDB_SEG_UNIT   1
#define DIR_DELIM           "\\"
#define DIR_DELIM_CHAR        '\\'
#define DB_LOCALCODE_SIZE_MAX 3
#define SERVERLOG_PATH_ON_SYSTEMROOT
#elif defined(psos)
#define _ONE_BDB_
#define USE_PPTHREAD
#define USE_WCHAR_H
#define CREATEDB_SEG_UNIT   1
#define DIR_DELIM           "/"
#define DIR_DELIM_CHAR        '/'
#define DB_LOCALCODE_SIZE_MAX 3
#endif

#if !defined(_ONE_BDB_)
#define _ONE_BDB_
#endif

    __DECL_PREFIX void __SYSLOG(char *format, ...);
    __DECL_PREFIX void __SYSLOG_TIME(char *format, ...);

// NCHAR -> 2 Bytes
#ifdef USE_WCHAR_H
#undef USE_WCHAR_H
#endif

#if !defined(DO_NOT_USE_DESC_COMPILE_OPT)
//#define USE_DESC_COMPILE_OPT
#if defined(USE_DESC_COMPILE_OPT)
    static const char *const compileOpt[] = {
#ifdef CHECK_INDEX_VALIDATION
        "CHECK_INDEX_VALIDATION",
#endif
#ifdef CHECK_MALLOC_FREE
        "CHECK_MALLOC_FREE",
#endif
#ifdef CHECK_VARCHAR_EXTLEN
        "CHECK_VARCHAR_EXTLEN",
#endif
#ifdef DO_NOT_CHECK_DISKFREESPACE
        "DO_NOT_CHECK_DISKFREESPACE",
#endif
#ifdef DO_NOT_FSYNC
        "DO_NOT_FSYNC",
#endif
#ifdef DO_NOT_SUPPORT_MDB_SYSLOG
        "DO_NOT_SUPPORT_MDB_SYSLOG",
#endif
#ifdef DO_NOT_USE_ERROR_DESC
        "DO_NOT_USE_ERROR_DESC",
#endif
#ifdef DO_NOT_USE_MDB_DEBUGLOG
        "DO_NOT_USE_MDB_DEBUGLOG",
#endif
#ifdef HIDDEN__DECL_PREFIX
        "HIDDEN__DECL_PREFIX",
#endif
#ifdef INDEX_BUFFERING
        "INDEX_BUFFERING",
#endif
#ifdef SEGMENT_NO_65536
        "SEGMENT_NO_65536",
#endif
#ifdef USE_DESC_COMPILE_OPT
        "USE_DESC_COMPILE_OPT",
#endif
#ifdef _64BIT
        "_64BIT",
#endif
        0x00
    };
#endif /* USE_DESC_COMPILE_OPT */
#endif

#ifdef WIN32
#define sleep(t)  Sleep((t)*1000)
#endif

#define LOGFILE_RESERVING_NUM   3

#ifdef  __cplusplus
}
#endif

#endif
