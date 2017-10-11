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
#include "ErrorCode.h"
#ifdef WIN32
#include <math.h>
#endif

unsigned int read_bytes = 0;
unsigned int write_bytes = 0;

#if !defined(MDB_FILE_NAME)
#define MDB_FILE_NAME            256
#endif

#if defined(linux) || defined(sun)
#include <pthread.h>
#endif

#define PR_ERROR(f)

#ifdef  __cplusplus
extern "C"
{
#endif

#ifdef CHECK_MALLOC_FREE
#define MAX_CHECK_MALLOC_FREE 10000
    typedef struct
    {
        char flag;
        void *ptr;
        int size;
        int level;
    } _PtrChk;
    _PtrChk gPtrChk[MAX_CHECK_MALLOC_FREE], gPtrChk2[MAX_CHECK_MALLOC_FREE];

    int gFlag = 0;
    int gFlag2 = 0;
    int gLevel = 0;
    unsigned long gCurAllocatedHeapSize = 0;
    unsigned long gMaxAllocatedHeapSize = 0;
#endif                          /* CHECK_MALLOC_FREE */

#ifdef WIN32
    static int sc_db_lock_init_flag = 0;
#endif

#if defined(psos)
    __DECL_PREFIX char mobile_lite_config[MDB_FILE_NAME] = "/ff";
    __DECL_PREFIX char SERVERLOG_PATH[] = "";
#elif defined(_AS_WIN32_MOBILE_)
    __DECL_PREFIX char mobile_lite_config[MDB_FILE_NAME] = "\\WINNT";
    __DECL_PREFIX char SERVERLOG_PATH[] = "";
#elif defined(linux) || defined(sun)
    __DECL_PREFIX char mobile_lite_config[MDB_FILE_NAME] = "";
    __DECL_PREFIX char SERVERLOG_PATH[] = "";
#else
    __DECL_PREFIX char mobile_lite_config[MDB_FILE_NAME] = "\\Windows";
    __DECL_PREFIX char SERVERLOG_PATH[] = "";
#endif

// determine in porting 
    extern int sc_strdcmp(const char *str1, const char *str2);
    extern int sc_strndcmp(const char *s1, const char *s2, int n);
    __DECL_PREFIX char *__strchr(const char *string, const char *pEnd, int ch);

    DB_INT64 _tm2time(struct db_tm *st);
    struct db_tm *localtime_as(const DB_INT64 * p_clock, struct db_tm *res);
    struct db_tm *gmtime_as(const DB_INT64 * p_clock, struct db_tm *res);
    __DECL_PREFIX DB_INT64 mktime_as(struct db_tm *timeptr);

    __DECL_PREFIX int sc_strcmp_user1(const char *str1, const char *str2);
    __DECL_PREFIX int sc_strcmp_user2(const char *str1, const char *str2);
    __DECL_PREFIX int sc_strcmp_user3(const char *str1, const char *str2);
    __DECL_PREFIX int sc_strncmp_user1(const char *s1, const char *s2, int n);
    __DECL_PREFIX int sc_strncmp_user2(const char *s1, const char *s2, int n);
    __DECL_PREFIX int sc_strncmp_user3(const char *s1, const char *s2, int n);
    __DECL_PREFIX char *sc_strstr_user1(const char *haystack,
            const char *needle);
    __DECL_PREFIX char *sc_strstr_user2(const char *haystack,
            const char *needle);
    __DECL_PREFIX char *sc_strstr_user3(const char *haystack,
            const char *needle);
    __DECL_PREFIX char *sc_strchr_user1(const char *string, const char *pEnd,
            int ch);
    __DECL_PREFIX char *sc_strchr_user2(const char *string, const char *pEnd,
            int ch);
    __DECL_PREFIX char *sc_strchr_user3(const char *string, const char *pEnd,
            int ch);

#ifdef  __cplusplus
}
#endif

#ifdef  CHECK_MALLOC_FREE
__DECL_PREFIX int
sc_uplevel_mallocfree(void)
{
    gLevel++;
}

__DECL_PREFIX int
sc_print_mallocfree(int level)
{
    int i;

    if (level == -1)
        level = gLevel;

    for (i = 0; i < MAX_CHECK_MALLOC_FREE; i++)
    {
        if (gPtrChk[i].flag == -1)
            break;
        if (gPtrChk[i].flag == 0)
            continue;
        if (gPtrChk[i].level < level)
            continue;
        printf("[%d]: %p(%d)\n", gPtrChk[i].level, gPtrChk[i].ptr,
                gPtrChk[i].size);
    }

    printf("Maximum Heap size : %lu\n", gMaxAllocatedHeapSize);
    printf("Current Heap size : %lu\n", gCurAllocatedHeapSize);
    return 0;
}
#endif

/*****************************************************************************/
//! sc_strerror
/*! \breif     \n
 *            
 ************************************
 * \param errnum : error 
 ************************************
 * \return  return_value
 ************************************
 * \note
 *****************************************************************************/
__DECL_PREFIX char *
sc_strerror(int errnum)
{
#if defined(_AS_WINCE_)  || defined(psos)
    static char msg[40];

    sc_sprintf(msg, "strerror not supported (%d)", errnum);

    return msg;
#else
    return strerror(errnum);
#endif
}

/*****************************************************************************/
//! rint
/*! \breif     
 ************************************
 * \param x : 
 ************************************
 * \return     
 ************************************
 * \note       \n
 *                 
 *****************************************************************************/
__DECL_PREFIX double
sc_rint(double x)
{
    double rem;

    rem = x - (double) ((int) x);

    if (rem < -0.5)
        return (x - rem - 1.0);
    else if (rem < 0.0)
        return (x - rem);
    else if (rem < 0.5)
        return (x - rem);

    return (x - rem + 1.0);
}

/*****************************************************************************/
//! sc_creat
/*! \breif      \n
 ************************************
 * \param path(in)        : file path
 * \param mode(in)    : file's mode
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *        
 * Do not use this function on a file descriptor with  O_NONBLOCK or O_NDELAY
 *
 *
 * man page : creat  is  equivalent  to  open  with   flags   equal   to
 *                     O_CREAT|O_WRONLY|O_TRUNC.
 *
 *****************************************************************************/
__DECL_PREFIX int
sc_creat(const char *path, unsigned int mode)
{
#if defined(psos)
    return sc_open(path, O_RDWR | O_CREAT, mode);
#else
    return sc_open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
#endif

}

/*****************************************************************************/
//! sc_open
/*! \breif  open the file
 ************************************
 * \param path(in)    : file's path
 * \param oflag(in) : file's flag 
 * \param mode(in): file's mode
 ************************************
 * \return  ret is -1 is Error.. else Success
 ************************************
 * \note  \n
 *****************************************************************************/
/* Do not use this function on a file descriptor with
   O_NONBLOCK or O_NDELAY */
__DECL_PREFIX int
sc_open(const char *path, int oflag, unsigned int mode)
{
#if defined(_AS_WINCE_) || defined(_AS_WIN32_MOBILE_)
    FILE *fp = NULL;
    char f_mode[5];
    int f_i = 0;

    /*
       #define O_RDONLY    0x0000
       #define O_WRONLY    0x0001
       #define O_RDWR      0x0002
       #define O_APPEND    0x0008
       #define O_CREAT        0x0100
       #define O_TRUNC     0x0200
       #define O_EXCL      0x0400
       #define O_BINARY    0x8000

       #define _S_IREAD    0000400
       #define _S_IWRITE   0000200
     */

    if (oflag == O_RDONLY)      /* O_RDONLY is 0 */
    {
        f_i = 0;
        f_mode[f_i++] = 'r';
    }

    if (oflag & O_CREAT)
    {
        fp = fopen(path, "r");

        if (oflag & O_EXCL)     /* creation fail if the file exists */
        {
            if (fp)
            {
                fclose(fp);
                return -1;
            }
        }

        if (fp) /* if the file exists, remove O_CREAT flag */
        {
            oflag &= ~O_CREAT;
            fclose(fp);
        }
    }


    if (oflag & O_WRONLY)
    {
        f_i = 0;
        f_mode[f_i++] = 'r';
        f_mode[f_i++] = '+';
    }

    if (oflag & O_RDWR)
    {
        f_i = 0;
        f_mode[f_i++] = 'r';
        f_mode[f_i++] = '+';
    }

    if (oflag & O_APPEND)
    {
        f_i = 0;
        f_mode[f_i++] = 'a';
        if (oflag | O_RDWR)
            f_mode[f_i++] = '+';
    }

    if (oflag & (O_CREAT | O_TRUNC))
    {
        f_i = 0;
        f_mode[f_i++] = 'w';
        if (oflag | O_RDWR)
            f_mode[f_i++] = '+';
    }

    if (oflag & O_BINARY)
    {
        if (f_i == 0)
            f_mode[f_i++] = 'r';
        f_mode[f_i++] = 'b';
    }

    f_mode[f_i] = '\0';
    fp = fopen(path, f_mode);
    if (fp == NULL)
        return -1;

    return (int) fp;
#elif defined(psos)
    return open(path, oflag);
#else
    int ret;

#if defined(MDB_DEBUG)
    umask(000);
#endif

    ret = open(path, oflag, mode);
    PR_ERROR("open");

    return ret;
#endif
}

/*****************************************************************************/
//! sc_close
/*! \breif  close the file
 ************************************
 * \param fildes(in) : file descriptor
 ************************************
 * \return  ret == 0 is Success, else Error 
 ************************************
 * \note 
 *     Do not use this function on a file descriptor with O_NONBLOCK or O_NDELAY
 *****************************************************************************/
__DECL_PREFIX int
sc_close(int fildes)
{
    int ret = 0;

    if (fildes < 0)
        return ret;
#if defined(_AS_WINCE_) || defined(_AS_WIN32_MOBILE_)
    return fclose((FILE *) fildes);
#else
    ret = close(fildes);
    PR_ERROR("close");

    return ret;
#endif
}

/*****************************************************************************/
//! sc_read
/*! \breif  read the file
 ************************************
 * \param fd(in)    :
 * \param buf(out)    :
 * \param nbyte(in): read bytes
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *          Do not use this function on a file descriptor with  O_NONBLOCK or O_NDELAY
 *****************************************************************************/
__DECL_PREFIX long
sc_read(int fd, void *buf, size_t nbyte)
{
#if defined(_AS_WINCE_) || defined(_AS_WIN32_MOBILE_)
    FILE *fp = (FILE *) fd;
    int ret = -1;

    ret = fread(buf, sizeof(char), nbyte, fp);
    if (ret == 0)
    {
        if (feof(fp) != 0)
            return 0;
        else
            return -1;
    }
    return ret;
#else
    int nread;
    int nleft;
    char *ptr = (char *) buf;

    nleft = nbyte;
    while (nleft > 0)
    {
        nread = read(fd, ptr, nleft);
        PR_ERROR("read");
        if (nread == 0)
            break;
        else if (nread < 0)
        {
            if (errno == 0)
                break;

            return -1;
        }

        nleft -= nread;
        ptr += nread;
    }

    return (nbyte - nleft);
#endif
}

/*****************************************************************************/
//! sc_write
/*! \breif     \n
 *            
 ************************************
 * \param fd(in)        :
 * \param buf(in)        :
 * \param nbyte(in)    :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *        Do not use this function on a file descriptor with O_NONBLOCK or O_NDELAY
 *****************************************************************************/
__DECL_PREFIX long
sc_write(int fd, const void *buf, size_t nbyte)
{
#if defined(_AS_WINCE_) || defined(_AS_WIN32_MOBILE_)
    FILE *fp = (FILE *) fd;

    if (buf == NULL || nbyte <= 0)
    {
        return -1;
    }

    return fwrite(buf, sizeof(char), nbyte, fp);
#else
    int nleft, nwrite;
    char *ptr = (char *) buf;

    if (buf == NULL || nbyte <= 0)
        return -1;

    nleft = nbyte;
    while (nleft > 0)
    {
        nwrite = write(fd, ptr, nleft);
        PR_ERROR("write");
        if (nwrite <= 0)
        {
            if (errno == 0)
                break;

            return -1;
        }

        nleft -= nwrite;
        ptr += nwrite;
    }

    return (nbyte - nleft);
#endif
}


/*****************************************************************************/
//! sc_lseek
/*! \breif  
 ************************************
 * \param fildes(in)        : File Descriptor or File Structure
 * \param offset(in)    : Offset
 * \param whence(in)    :
 ************************************
 * \return  if ret is -1, Error... else the file position
 ************************************
 * \note  \n
 *        Do not use this function on a file descriptor with O_NONBLOCK or O_NDELAY
 *
 *
 *****************************************************************************/
__DECL_PREFIX int
sc_lseek(int fildes, int offset, int whence)
{
#if defined(_AS_WINCE_) || defined(_AS_WIN32_MOBILE_)
    FILE *fp = (FILE *) fildes;

    if (fseek(fp, offset, whence) == 0)
    {
        return ftell(fp);
    }
    else
        return -1;
#else
    int ret;

    ret = lseek(fildes, offset, whence);
    PR_ERROR("lseek");

    return ret;
#endif
}



/*****************************************************************************/
//! sc_rename
/*! \breif     \n
 *            
 ************************************
 * \param old_name(in) :
 * \param new_name(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *         Do not use this function on a file descriptor with O_NONBLOCK or O_NDELAY
 *****************************************************************************/
#ifdef _AS_WINCE_
#if !defined(MDB_FILE_NAME)
#define MDB_FILE_NAME 256
#endif
#endif
__DECL_PREFIX int
sc_rename(const char *old_name, const char *new_name)
{
#ifdef _AS_WINCE_
    TCHAR old_TCHAR[MDB_FILE_NAME + 1];
    TCHAR new_TCHAR[MDB_FILE_NAME + 1];

    mbstowcs(old_TCHAR, old_name, sc_strlen(old_name) + 1);
    mbstowcs(new_TCHAR, new_name, sc_strlen(new_name) + 1);

    if (MoveFile(old_TCHAR, new_TCHAR) == 0)
        return -1;
    else
        return 0;
#elif defined(_AS_WIN32_MOBILE_)
    if (MoveFile(old_name, new_name) == 0)
        return -1;
    else
    {
        return 0;
    }
#else
    int ret;

    ret = rename(old_name, new_name);
    PR_ERROR("rename");

    return ret;
#endif
}

/*****************************************************************************/
//! sc_unlink
/*! \breif     
 ************************************
 * \param path :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *  Do not use this function on a file descriptor with O_NONBLOCK or O_NDELAY
 *           
 *****************************************************************************/
__DECL_PREFIX int
sc_unlink(const char *path)
{
#ifdef _AS_WINCE_
    TCHAR path_TCHAR[MDB_FILE_NAME + 1];

    mbstowcs(path_TCHAR, path, sc_strlen(path) + 1);
    if (DeleteFile(path_TCHAR) == 0)
        return -1;
    else
    {
        return 0;
    }
#elif defined(_AS_WIN32_MOBILE_)
    if (DeleteFile(path) == 0)
        return -1;
    else
    {
        return 0;
    }
#elif defined(psos)
    struct stat status;

    if (stat(path, &status) < 0)
        return -1;

    if (S_ISREG(status.st_mode))
    {
        if (unlink(path) < 0)
            return -1;

        return 0;
    }

    return -1;
#else
    int ret;

    ret = unlink(path);
    PR_ERROR("unlink");

    return ret;
#endif
}

/*****************************************************************************/
//! sc_truncate
/*! \breif      
 ************************************
 * \param path :
 * \param length :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *****************************************************************************/
__DECL_PREFIX int
sc_truncate(const char *path, int length)
{
    return -1;  /* NOT supported */
}

__DECL_PREFIX int
sc_ftruncate(int fd, int length)
{
#if defined(WIN32) && !defined(_AS_WINCE_)
    return _chsize(_fileno((FILE *) fd), length);
#elif defined(_AS_WINCE_)
    int ret;

    ret = sc_lseek(fd, 0, length);
    if (ret < 0)
        return ret;
    return SetEndOfFile(fd);
#else
    int ret;

    ret = ftruncate(fd, length);
    PR_ERROR("ftruncate");

    return ret;
#endif
}

/*****************************************************************************/
//! sc_fsync
/*! \breif     \n
 *            
 ************************************
 * \param fildes :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *****************************************************************************/
__DECL_PREFIX int
sc_fsync(int fildes)
{
#ifdef DO_NOT_FSYNC     /* for test */
    return 0;
#elif defined(_AS_WINCE_) || defined(_AS_WIN32_MOBILE_)
    int ret;

    ret = fflush((FILE *) fildes);
    if (ret)
        return -1;
    return 0;
#elif defined(psos)
    return 0;
#else
#ifdef MDB_DEBUG
    return 0;   //fsync(fildes);
#else
    return fsync(fildes);
#endif
#endif
}

/*****************************************************************************/
//! sc_mkdir
/*! \breif   
 ************************************
 * \param path :
 * \param mode :
 ************************************
 * \return  return_value
0 : ok
-1: error
-2: exist
 ************************************
 * \note  \n
 *****************************************************************************/
__DECL_PREFIX int
sc_mkdir(const char *path, unsigned int mode)
{
#ifdef _AS_WINCE_
    TCHAR buf_t[MDB_FILE_NAME * 2 + 1];

    mbstowcs(buf_t, path, sc_strlen(path) + 1);
    if (CreateDirectory(buf_t, NULL) == 0)
    {
        if (GetLastError() == 183)      /* already exists */
            return -2;
        else
            return -1;
    }
    else
        return 0;
#elif defined(_AS_WIN32_MOBILE_)
    if (CreateDirectory(path, NULL) == 0)
    {
        if (GetLastError() == 183)      /* already exists */
            return -2;
        else
            return -1;
    }
    else
        return 0;
#elif defined(psos)
    struct stat st;

    if (stat(path, &st) < 0)
    {
        if (mkdir(path, 0) < 0)
            return -1;
        else
            return 0;
    }
    else
    {
        return -2;
    }
#else
    if (mkdir(path, mode) == -1)
    {
        PR_ERROR("mkdir");
        if (errno == EEXIST)
            return -2;
        else
            return -1;
    }
    else
        return 0;
#endif
}


/*****************************************************************************/
//! sc_rmdir
/*! \breif   .
 ************************************
 * \param path :
 ************************************
 * \return  0 is Success, else is Error
 ************************************
 * \note  \n
 *****************************************************************************/
#ifdef _AS_WINCE_
static DB_BOOL __remove_dir(TCHAR * path);
static DB_BOOL
__remove_dir(TCHAR * path)
{
    HANDLE hSrch;
    WIN32_FIND_DATA wfd;
    TCHAR fname[MAX_PATH];
    DB_BOOL bResult = TRUE;
    TCHAR dir[MAX_PATH];
    TCHAR newpath[MAX_PATH];

    if (wcslen(path) <= 2)
        return -1;

    wcscpy(dir, path);

    wcscpy(newpath, path);
    if (wcscmp(newpath + wcslen(newpath) - 4, L"\\*.*") != 0)
        wcscat(newpath, L"\\*.*");

    hSrch = FindFirstFile(newpath, &wfd);
    if (hSrch == INVALID_HANDLE_VALUE)
    {
        bResult = FALSE;
    }

    while (bResult)
    {
        swprintf(fname, L"%s\\%s", dir, wfd.cFileName);


        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
            SetFileAttributes(fname, FILE_ATTRIBUTE_NORMAL);

        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (wfd.cFileName[0] != '.')
            {
                swprintf(newpath, L"%s\\%s", dir, wfd.cFileName);
                __remove_dir(newpath);
            }
        }
        else
        {
            DeleteFile(fname);
        }

        bResult = FindNextFile(hSrch, &wfd);
    }

    FindClose(hSrch);

    if (RemoveDirectory(dir) == FALSE)
    {
        __SYSLOG("RemoveDirectory Error: %d\n", GetLastError());
        return -1;
    }

    return 0;
}
#endif /*_AS_WINCE_*/

__DECL_PREFIX int
sc_rmdir(const char *path)
{
#ifdef _AS_WINCE_
    TCHAR *path_tchar = (TCHAR *) sc_malloc(sc_strlen(path) * 2 + 128);
    DB_BOOL ret;

    mbstowcs(path_tchar, path, sc_strlen(path) + 1);

    ret = __remove_dir(path_tchar);

    free(path_tchar);

    return ret;
#elif defined(_AS_WIN32_MOBILE_)
    HANDLE hSrch;
    WIN32_FIND_DATA wfd;
    TCHAR fname[MAX_PATH];
    DB_BOOL bResult = TRUE;
    char dir[MAX_PATH];
    char newpath[MAX_PATH];

    if (sc_strlen(path) <= 2)
        return -1;

    sc_strcpy(dir, path);

    sc_strcpy(newpath, path);
    if (sc_strcmp(newpath + sc_strlen(newpath) - 4, "\\*.*") != 0)
        sc_strcat(newpath, "\\*.*");

    hSrch = FindFirstFile(newpath, &wfd);
    if (hSrch == INVALID_HANDLE_VALUE)
    {
        if (unlink(path) == 0)
            return 0;

        bResult = FALSE;
    }

    while (bResult)
    {
        sc_sprintf(fname, "%s\\%s", dir, wfd.cFileName);


        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
            SetFileAttributes(fname, FILE_ATTRIBUTE_NORMAL);

        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (wfd.cFileName[0] != '.')
            {
                sc_sprintf(newpath, "%s\\%s", dir, wfd.cFileName);
                sc_rmdir(newpath);
            }
        }
        else
        {
            DeleteFile(fname);
        }

        bResult = FindNextFile(hSrch, &wfd);
    }

    FindClose(hSrch);

    if (RemoveDirectory(dir) == FALSE)
    {
        return -1;
    }

    return 0;
#elif defined(sun)||defined(linux)||defined(psos)
    DIR *pdir;
    struct dirent *pdirent;
    struct stat status;
    char subpath[256];
    int ret = 0;

    pdir = opendir(path);
    PR_ERROR("opendir");

    if (pdir == 0)
    {


        if (stat(path, &status) < 0)
        {
            PR_ERROR("stat");

            return -1;
        }

        if (S_ISREG(status.st_mode))
        {
            if (unlink(path) < 0)
            {
                PR_ERROR("unlink");

                return -1;
            }

            return 0;
        }


        return -1;
    }

    while ((pdirent = readdir(pdir)) != NULL)
    {
        PR_ERROR("readdir");

        if (sc_strcmp(pdirent->d_name, ".") == 0)
            continue;
        if (sc_strcmp(pdirent->d_name, "..") == 0)
            continue;

        sc_memset(subpath, 0, 256);
        sc_sprintf(subpath, "%s/%s", path, pdirent->d_name);
        if (stat(subpath, &status) < 0)
        {
            PR_ERROR("stat");

            ret = -1;
            break;
        }

        if (S_ISREG(status.st_mode))
        {
            if (unlink(subpath) < 0)
            {
                PR_ERROR("unlink");

                ret = -1;
                break;
            }
        }
        else if (S_ISDIR(status.st_mode))
        {
            sc_rmdir(subpath);
        }
    }
    PR_ERROR("readdir");

    if (closedir(pdir) < 0)
    {
        PR_ERROR("closedir");

        ret = -1;
    }

#if defined(psos)
    if (rmdir_(path, 0) < 0)
#else
    if (rmdir(path) < 0)
#endif
    {
        ret = -1;
    }
    PR_ERROR("rmdir");

    return ret;
#else

#error Check whether sc_rmdir is ported for this OS!!!
    return -1;
#endif
}

#ifdef _AS_WINCE_
static DB_BOOL __remove_unlink_prefix(TCHAR * path, TCHAR * prefix);
static DB_BOOL
__remove_unlink_prefix(TCHAR * path, TCHAR * prefix)
{
    HANDLE hSrch;
    WIN32_FIND_DATA wfd;
    TCHAR fname[MAX_PATH];
    DB_BOOL bResult = TRUE;
    TCHAR dir[MAX_PATH];
    TCHAR newpath[MAX_PATH];
    int prefix_len;

    prefix_len = wcslen(prefix);

    wcscpy(dir, path);

    wcscpy(newpath, path);
    if (wcscmp(newpath + wcslen(newpath) - 4, L"\\*.*") != 0)
        wcscat(newpath, L"\\*.*");

    hSrch = FindFirstFile(newpath, &wfd);
    if (hSrch == INVALID_HANDLE_VALUE)
    {
        bResult = FALSE;
    }

    while (bResult)
    {
        if (wcsncmp(wfd.cFileName, prefix, prefix_len) == 0)
        {
            swprintf(fname, L"%s\\%s", dir, wfd.cFileName);


            if (wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
                SetFileAttributes(fname, FILE_ATTRIBUTE_NORMAL);

            if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
            }
            else
            {
                DeleteFile(fname);
            }
        }

        bResult = FindNextFile(hSrch, &wfd);
    }

    FindClose(hSrch);

    return 0;
}
#endif
       /*_AS_WINCE_*/

__DECL_PREFIX int
sc_unlink_prefix(const char *path, const char *prefix)
{
#ifdef _AS_WINCE_
    TCHAR *path_tchar = (TCHAR *) sc_malloc(sc_strlen(path) * 2 + 128);
    TCHAR *prefix_tchar = (TCHAR *) sc_malloc(sc_strlen(prefix) * 2 + 128);
    DB_BOOL ret;

    mbstowcs(path_tchar, path, sc_strlen(path) + 1);
    mbstowcs(prefix_tchar, prefix, sc_strlen(prefix) + 1);

    ret = __remove_unlink_prefix(path_tchar, prefix_tchar);

    free(path_tchar);
    free(prefix_tchar);

    return ret;
#elif defined(_AS_WIN32_MOBILE_)
    HANDLE hSrch;
    WIN32_FIND_DATA wfd;
    TCHAR fname[MAX_PATH];
    DB_BOOL bResult = TRUE;
    char dir[MAX_PATH];
    char newpath[MAX_PATH];
    int prefix_len;

    prefix_len = sc_strlen(prefix);

    sc_strcpy(dir, path);

    sc_strcpy(newpath, path);
    if (sc_strcmp(newpath + sc_strlen(newpath) - 4, "\\*.*") != 0)
        sc_strcat(newpath, "\\*.*");

    hSrch = FindFirstFile(newpath, &wfd);
    if (hSrch == INVALID_HANDLE_VALUE)
    {
        if (unlink(path) == 0)
        {
            return 0;
        }

        bResult = FALSE;
    }

    while (bResult)
    {
        if (sc_strncmp(wfd.cFileName, prefix, prefix_len) == 0)
        {
            sc_sprintf(fname, "%s\\%s", dir, wfd.cFileName);


            if (wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
                SetFileAttributes(fname, FILE_ATTRIBUTE_NORMAL);

            if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
            }
            else
            {
                DeleteFile(fname);
            }
        }

        bResult = FindNextFile(hSrch, &wfd);
    }

    FindClose(hSrch);

    return 0;
#elif defined(sun)||defined(linux)||defined(psos)
    DIR *pdir;
    struct dirent *pdirent;
    struct stat status;
    char subpath[256];
    int ret = 0;
    int prefix_len;

    prefix_len = sc_strlen(prefix);

    pdir = opendir(path);
    PR_ERROR("opendir");
    if (pdir == 0)
    {

        return -1;
    }

    while ((pdirent = readdir(pdir)) != NULL)
    {
        PR_ERROR("readdir");
        if (sc_strncmp(pdirent->d_name, prefix, prefix_len) == 0)
        {
            sc_memset(subpath, 0, 256);
            sc_sprintf(subpath, "%s/%s", path, pdirent->d_name);
            if (stat(subpath, &status) < 0)
            {
                PR_ERROR("stat");
                ret = -1;
                break;
            }

            if (S_ISREG(status.st_mode))
            {
                if (unlink(subpath) < 0)
                {
                    PR_ERROR("unlink");
                    ret = -1;
                    break;
                }
            }
        }
    }
    PR_ERROR("readdir|stat|unlink");

    if (closedir(pdir) < 0)
        ret = -1;

    PR_ERROR("closedir");

    return ret;
#else

#error Check whether sc_rmdir is ported for this OS!!!
    return -1;
#endif
}

/*****************************************************************************/
//! sc_fgetc
/*! \breif
 ************************************
 * \param fd(in) : File Description
 ************************************
 * \return  return_value
 ************************************
 * \note fgetc  
 *****************************************************************************/
__DECL_PREFIX int
sc_fgetc(int fd)
{
#if defined(_AS_WINCE_) || defined(_AS_WIN32_MOBILE_)
    FILE *fp = (FILE *) fd;

    return fgetc(fp);
#else
    char ch[2];
    int ret;

    ret = sc_read(fd, ch, 1);

    if (ret == 1)
        return (int) ch[0];
    else
        return -1;      // EOF == -1
#endif
}

/*****************************************************************************/
//! sc_fputc
/*! \breif  
 ************************************
 * \param c(in)    : ungined char
 * \param fd(in)    : File Description
 ************************************
 * \return  return_value
 ************************************
 * \note fgetc  
 *****************************************************************************/
__DECL_PREFIX int
sc_fputc(int c, int fd)
{
#if defined(_AS_WINCE_) || defined(_AS_WIN32_MOBILE_)
    FILE *fp = (FILE *) fd;

    unsigned char uch = (unsigned char) c;

    return fputc(uch, fp);
#else
    unsigned char uch = (unsigned char) c;
    int ret;

    ret = sc_write(fd, &uch, 1);

    if (ret == 1)
        return (int) uch;
    else
        return -1;      // EOF is Error
#endif
}

/*****************************************************************************/
//! sc_getenv
/*! \breif  searches the environment list for a string that matches the string pointed to by name
 ************************************
 * \param str(in) : environment variable
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *        
 *****************************************************************************/
__DECL_PREFIX char *
sc_getenv(char *str)
{
    if (sc_strcmp(str, "MOBILE_LITE_CONFIG") == 0 &&
            mobile_lite_config[0] != '\0')
    {
        return mobile_lite_config;
    }
#if defined(_AS_WINCE_)
    if (sc_strcmp(str, "SystemRoot") == 0)
    {
        static char systemroot[] = "\\Windows";

        return systemroot;
    }
#endif

#if defined(WIN32_MOBILE) || defined(linux) || defined(sun)
    return getenv(str);
#else
    return NULL;
#endif
}

/*****************************************************************************/
//! sc_setenv
/*! \breif The setenv() function adds the variable name to the environment with
           the value value, if name does not already exist. 
 ************************************
 * \param name(in)      : name of environment variable to set
 * \param value(in)     : new value of environment variable
 * \param overwirte(in) : 0: don't change value if exists already, !0:change it
 ************************************
 * \return  int   : 0: success, -1:fail (check errno)
 ************************************
 * \note     
 *****************************************************************************/
__DECL_PREFIX int
sc_setenv(char *name, char *value, int overwrite)
{
    return 0;
}

/*****************************************************************************/
//! sc_getpid
/*! \breif  
 ************************************
 * \param void :
 ************************************
 * \return  return_value
 ************************************
 * \note     
 *****************************************************************************/
__DECL_PREFIX int
sc_getpid(void)
{
#if defined(_AS_WIN32_MOBILE_)
    return _getpid();
#elif defined(psos) || defined(_AS_WINCE_)
    return 1;
#else
    int ret;

    ret = getpid();
    PR_ERROR("getpid");

    return ret;
#endif
}

/*****************************************************************************/
//! snprintf

/*! \breif     \n
 *            
 ************************************
 * \param param_name :
 * \param param_name :
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *         unix based 
 *****************************************************************************/
#ifndef sc_snprintf
__DECL_PREFIX int
sc_snprintf(char *buffer, size_t count, const char *format, ...)
{
    int len;
    va_list args;

    va_start(args, format);
    len = sc_vsnprintf(buffer, count, format, args);
    va_end(args);
    if (len < 0)
    {
        buffer[count - 1] = '\0';
        return count - 1;
    }
    else
        return len;
}
#endif

/*****************************************************************************/
//! sc_strlwr
/*! \breif  convert a string into lower case
 ************************************
 * \param str :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *****************************************************************************/
#ifndef sc_strlwr
__DECL_PREFIX char *
sc_strlwr(char *str)
{
#ifdef WIN32
    return _strlwr(str);
#else
    register int i;

    if (str == NULL)
        return NULL;

    for (i = 0; str[i] != '\0'; i++)
        str[i] = sc_tolower(str[i]);

    return str;
#endif
}
#endif

/*****************************************************************************/
//! sc_strlower
/*! \breif  convert a string into lower case
 ************************************
 * \param str :
 ************************************
 * \return  return_value
 ************************************
 * \note  
 *****************************************************************************/
__DECL_PREFIX int
sc_strlower(char *dest, const char *src)
{
    char *dummy = (char *) src;

    while (*dummy != '\0')
    {
        *dest++ = (char) sc_tolower((int) *dummy++);
    }

    return dummy - (char *) src;
}

/*****************************************************************************/
//! sc_strupper
/*! \breif  convert a string into lower case
 ************************************
 * \param str :
 ************************************
 * \return  return_value
 ************************************
 * \note  
 *****************************************************************************/
__DECL_PREFIX int
sc_strupper(char *dest, const char *src)
{
    char *dummy = (char *) src;

    while (*dummy != '\0')
    {
        *dest++ = (char) sc_toupper((int) *dummy++);
    }

    return dummy - (char *) src;
}

/*****************************************************************************/
//! sc_wcslower
/*! \breif  convert a string into lower case
 ************************************
 * \param str :
 ************************************
 * \return  return_value
 ************************************
 * \note  
 *****************************************************************************/
__DECL_PREFIX int
sc_wcslower(DB_WCHAR * dest, const DB_WCHAR * src)
{
    DB_WCHAR *dummy = (DB_WCHAR *) src;

    while (*dummy != (DB_WCHAR) '\0')
    {
        *dest++ = (DB_WCHAR) sc_tolower((int) *dummy++);
    }

    return dummy - (DB_WCHAR *) src;
}

/*****************************************************************************/
//! sc_wcsupper
/*! \breif  convert a string into lower case
 ************************************
 * \param str :
 ***********************************
 * \return  return_value
 ************************************
 * \note  
 *****************************************************************************/
__DECL_PREFIX int
sc_wcsupper(DB_WCHAR * dest, const DB_WCHAR * src)
{
    DB_WCHAR *dummy = (DB_WCHAR *) src;

    while (*dummy != (DB_WCHAR) '\0')
    {
        *dest++ = (DB_WCHAR) sc_toupper((int) *dummy++);
    }

    return dummy - (DB_WCHAR *) src;
}


/*****************************************************************************/
//! sc_strdup

/*! \breif  allocate new memory and copy string 
 ************************************
 * \param s(in) : string
 ************************************
 * \return  NULL is Error, else Success
 ************************************
 * \note  \n
 *****************************************************************************/

__DECL_PREFIX char *
sc_strdup(const char *s)
{
#ifdef CHECK_MALLOC_FREE
    int i;
    char *p;

    p = strdup(s);
    if (p)
    {
        for (i = 0; i < MAX_CHECK_MALLOC_FREE; i++)
        {
            if (gPtrChk[i].flag == -1)
                break;
            if (gPtrChk[i].flag == 0)
                break;
        }
        gPtrChk[i].flag = 1;
        gPtrChk[i].ptr = (void *) p;
        gPtrChk[i].size = sc_strlen(s);
        gPtrChk[i].level = gLevel;

        gCurAllocatedHeapSize += gPtrChk[i].size;

        if (gCurAllocatedHeapSize > gMaxAllocatedHeapSize)
        {
            gMaxAllocatedHeapSize = gCurAllocatedHeapSize;
        }
    }

    return p;
#elif defined(_AS_WINCE_)
    return _strdup(s);
#else
    char *ret;

    ret = strdup(s);
    PR_ERROR("strdup");

    return ret;
#endif
}

/*****************************************************************************/
//! sc_atoll

/*! \breif  convert string to bigint
 ************************************
 * \param str(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *****************************************************************************/
__DECL_PREFIX bigint
sc_atoll(char *str)
{
#if defined(_AS_WIN32_MOBILE_)
    return _atoi64(str);
#elif defined(_AS_WINCE_) || defined(psos) || defined(linux)
    char *p;
    bigint bi = 0;
    int i, isNegative;

    p = str;

    if (*p == '-')
    {
        isNegative = 1;
        p++;
    }
    else if (*p == '+')
    {
        isNegative = 0;
        p++;
    }
    else
    {
        isNegative = 0;
        i = 0;
    }

    for (i = 0; *p; i++, p++)
    {
        if (i == 18)
        {
            if (isNegative)
            {
                if (bi >= MDB_LLONG_MIN_18 && *p > '8')
                    return MDB_LLONG_MIN;
            }
            else
            {
                if (bi >= MDB_LLONG_MAX_18 && *p > '7')
                    return MDB_LLONG_MAX;
            }
        }

        if (sc_isdigit(*p))
            bi = bi * 10 + (*p - '0');
        else
        {
            return bi;
        }
    }

    return (isNegative ? -bi : bi);
#else
    return atoll(str);
#endif
}

/*****************************************************************************/
//! sc_gettimeofday
/*! \breif    .
 ************************************
 * \param tp(out)        :   
 * \param temp(in)        :
 ************************************
 * \return  if ret is 0, Success... else Error 
 ************************************
 * \note  \n
 *****************************************************************************/
__DECL_PREFIX int
sc_gettimeofday(struct db_timeval *tp, void *temp)
{
#if defined(psos)
    tp->tv_sec = sc_time(0);
    tp->tv_usec = 0;

    return 0;

#elif defined(_AS_WINCE_) || defined(_AS_WIN32_MOBILE_)
    SYSTEMTIME st;
    struct db_tm _tm;
    int t;

    GetSystemTime(&st);

    _tm.tm_year = st.wYear - 1900;
    _tm.tm_mon = st.wMonth - 1;
    _tm.tm_mday = st.wDay;
    _tm.tm_hour = st.wHour;
    _tm.tm_min = st.wMinute;
    _tm.tm_sec = st.wSecond;

    t = (int) _tm2time(&_tm);

    tp->tv_sec = t;
    tp->tv_usec = st.wMilliseconds * 1000;

    return 0;
#else
    int ret;

    ret = gettimeofday((struct timeval *) tp, temp);
    PR_ERROR("gettimeofday");

    return ret;
#endif
}

/*****************************************************************************/
//! sc_time

/*! \breif
 ************************************
 * \param tloc(in) :
 ************************************
 * \return
 ************************************
 * \note  \n
 *****************************************************************************/
__DECL_PREFIX db_time_t
sc_time(db_time_t * tloc)
{
#ifdef _AS_WINCE_
    SYSTEMTIME st;
    struct db_tm _tm;
    int t;

    GetSystemTime(&st);

    _tm.tm_year = st.wYear - 1900;
    _tm.tm_mon = st.wMonth - 1;
    _tm.tm_mday = st.wDay;
    _tm.tm_hour = st.wHour;
    _tm.tm_min = st.wMinute;
    _tm.tm_sec = st.wSecond;

    t = (int) _tm2time(&_tm);

    if (tloc != NULL)
        *(int *) tloc = t;

    return t;
#else
    db_time_t ret;

    ret = (db_time_t) time((time_t *) tloc);
    PR_ERROR("time");

    return ret;
#endif
}


/*****************************************************************************/
//! sc_mktime

/*! \breif  converts a broken-down  time  structure,  expressed as local time, 
 *        to calendar time representation.
 ************************************
 * \param timeptr(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *****************************************************************************/
__DECL_PREFIX db_time_t
sc_mktime(struct db_tm * timeptr)
{
    return (db_time_t) mktime_as(timeptr);
}

/*****************************************************************************/
//! sc_localtime_r

/*! \breif
 ************************************
 * \param clock(in) :
 * \param res(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *****************************************************************************/
__DECL_PREFIX struct db_tm *
sc_localtime_r(const db_time_t * p_clock, struct db_tm *res)
{
#if defined(_AS_WINCE_) || defined(psos) || \
    defined(_AS_WIN32_MOBILE_)
    DB_INT64 cclock = (DB_INT64) (*p_clock);

    return (struct db_tm *) localtime_as(&cclock, res);
#else
    struct tm _tm, *p_tm;
    time_t _time = *p_clock;

    p_tm = localtime_r(&_time, &_tm);
    PR_ERROR("localtime_r");

    if (p_tm == NULL)
        return NULL;

    sc_memcpy(res, &_tm, sizeof(struct db_tm));

    return res;
#endif
}

/*
  * return : localtime - UTC time   (sec)
 */
__DECL_PREFIX int
sc_getTimeGapTZ(void)
{
#if defined(_AS_WINCE_) || defined(_AS_WIN32_MOBILE_)
    SYSTEMTIME systm;
    struct db_tm _tm;
    DB_INT64 st, lt;

    st = sc_time(NULL);

    GetLocalTime(&systm);

    _tm.tm_year = systm.wYear - 1900;
    _tm.tm_mon = systm.wMonth - 1;
    _tm.tm_mday = systm.wDay;
    _tm.tm_hour = systm.wHour;
    _tm.tm_min = systm.wMinute;
    _tm.tm_sec = systm.wSecond;

    lt = _tm2time(&_tm);

    st = lt - st;
    st /= 60;
    st *= 60;

    return (int) st;
#else
    time_t st, lt;
    struct tm _tm;

    st = time(NULL);
    PR_ERROR("time");

    gmtime_r(&st, &_tm);
    PR_ERROR("gmtime_r");

    lt = mktime(&_tm);
    PR_ERROR("mktime");

    st = st - lt;
    st /= 60;
    st *= 60;

    return (int) st;
#endif
}

/*****************************************************************************/
//! sc_strcasecmp

/*! \breif        
 ************************************
 * \param s1(in) :
 * \param s2(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *****************************************************************************/
__DECL_PREFIX int
sc_strcasecmp(const char *s1, const char *s2)
{
#if defined(WIN32)
    return _stricmp(s1, s2);
#else
    int ret;

    ret = strcasecmp(s1, s2);
    PR_ERROR("strcasecmp");

    return ret;
#endif
}

/*****************************************************************************/
//! sc_strncasecmp

/*! \breif
 ************************************
 * \param s1(in) :
 * \param s2(in) :
 * \param n(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *****************************************************************************/
__DECL_PREFIX int
sc_strncasecmp(const char *s1, const char *s2, size_t n)
{
#if defined(WIN32)
    return _strnicmp(s1, s2, n);
#else
    int ret;

    ret = strncasecmp(s1, s2, n);
    PR_ERROR("strncasecmp");

    return ret;
#endif
}

/*****************************************************************************/
//! sc_wcscmp

/*! \breif  compare two wide-character strings
 ************************************
 * \param s1(in) : wide-character string
 * \param s2(in) : wide-character string
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *****************************************************************************/
DB_INT32
sc_wcscmp(const DB_WCHAR * s1, const DB_WCHAR * s2)
{
#if defined(USE_UCS_2_0)
    for (; *s1 && *s2; s1++, s2++)
    {
        if (*s1 < *s2)
            return -1;
        if (*s1 > *s2)
            return 1;
    }

    if (!*s1 && !*s2)
        return 0;
    else if (!*s1)
        return -1;      // s2   
    else if (!*s2)
        return 1;       // s1  

    return 0;
#else
    DB_INT32 ret;

    ret = wcscmp(s1, s2);
    PR_ERROR("wcscmp");

    return ret;
#endif
}

__DECL_PREFIX DB_INT32
sc_wcscasecmp(const DB_WCHAR * s1, const DB_WCHAR * s2)
{
    DB_WCHAR ch1;
    DB_WCHAR ch2;
    int i, n;
    int len1, len2;

    len1 = sc_wcslen(s1);
    len2 = sc_wcslen(s2);

    n = len1 > len2 ? len2 : len1;

    for (i = 0; i < n; i++)
    {
        ch1 = *(DB_WCHAR *) s1++;
        if (ch1 >= 'A' && ch1 <= 'Z')
            ch1 += 'a' - 'A';

        ch2 = *(DB_WCHAR *) s2++;
        if (ch2 >= 'A' && ch2 <= 'Z')
            ch2 += 'a' - 'A';

        if (ch1 < ch2)
            return -1;
        if (ch1 > ch2)
            return 1;
    }

    if (len1 < len2)
        return -1;
    else if (len1 > len2)
        return 1;

    return 0;
}

/*****************************************************************************/
//! sc_wcsncmp

/*! \breif  compare two wide-character strings
 ************************************
 * \param s1(in) :
 * \param s2(in) :
 * \param n(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *****************************************************************************/
DB_INT32
sc_wcsncmp(const DB_WCHAR * s1, const DB_WCHAR * s2, DB_INT32 n)
{
#if defined(USE_UCS_2_0)
    for (; n > 0 && (DB_WCHAR) * s1 && (DB_WCHAR) * s2; n--, s1++, s2++)
    {
        if (*s1 < *s2)
            return -1;
        if (*s1 > *s2)
            return 1;
    }

    if (n == 0)
        return 0;

    if (!*s1 && !*s2)
        return 0;
    else if (!*s1)
        return -1;      // s2   
    else if (!*s2)
        return 1;       // s1  

    return 0;
#else
    DB_INT32 ret;

    ret = wcsncmp(s1, s2, n);
    PR_ERROR("wcsncmp");

    return ret;
#endif
}

__DECL_PREFIX DB_INT32
sc_wcsncasecmp(const DB_WCHAR * s1, const DB_WCHAR * s2, int n)
{
    DB_WCHAR ch1;
    DB_WCHAR ch2;
    int i;

    for (i = 0; i < n; i++)
    {
        ch1 = *(DB_WCHAR *) s1++;
        if (ch1 >= 'A' && ch1 <= 'Z')
            ch1 += 'a' - 'A';

        ch2 = *(DB_WCHAR *) s2++;
        if (ch2 >= 'A' && ch2 <= 'Z')
            ch2 += 'a' - 'A';

        if (ch1 < ch2)
            return -1;
        if (ch1 > ch2)
            return 1;
    }
    return 0;
}

/*****************************************************************************/
//! sc_wmemcmp

/*! \breif  compare two wide-character strings
 ************************************
 * \param s1(in) :
 * \param s2(in) :
 * \param n(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *****************************************************************************/
DB_INT32
sc_wmemcmp(const DB_WCHAR * s1, const DB_WCHAR * s2, DB_INT32 n)
{
#if defined(WIN32)
    for (; n > 0; n--, s1++, s2++)
    {
        if (*s1 < *s2)
            return -1;
        if (*s1 > *s2)
            return 1;
    }
    return 0;
#else
#if defined(USE_UCS_2_0)
    for (; n > 0; n--, s1++, s2++)
    {
        if (*s1 < *s2)
            return -1;
        if (*s1 > *s2)
            return 1;
    }
    return 0;
#else
    DB_INT32 ret;

    ret = wmemcmp(s1, s2, n);
    PR_ERROR("wmemcmp");

    return ret;
#endif
#endif
}

/*****************************************************************************/
//! sc_wcslen

/*! \breif  determine the length of a wide-character string
 ************************************
 * \param wstring(in) :
 ************************************
 * \return  return_value
 ************************************
 *
 *****************************************************************************/
DB_INT32
sc_wcslen(const DB_WCHAR * wstring)
{
#if defined(USE_UCS_2_0)
    int len = 0;

    for (; *wstring; ++wstring)
        ++len;

    return len;
#else
    DB_INT32 ret;

    ret = wcslen(wstring);
    PR_ERROR("wcslen");

    return ret;
#endif
}

/*****************************************************************************/
//! sc_wcsncpy

/*! \breif  copy a fixed-size string of wide characters
 ************************************
 * \param dest(in) :
 * \param src(in) :
 * \param n(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *
 *****************************************************************************/
DB_WCHAR *
sc_wcsncpy(DB_WCHAR * dest, const DB_WCHAR * src, DB_INT32 n)
{
#if defined(USE_UCS_2_0)
    DB_WCHAR *p1 = dest;
    DB_WCHAR *p2 = (DB_WCHAR *) src;

    for (; n > 0 && *p2; --n, ++p1, ++p2)
        *p1 = *p2;

    for (; n > 0; --n)
        *p1 = L'\0';

    return dest;
#else
    DB_WCHAR *ret;

    ret = wcsncpy(dest, src, n);
    PR_ERROR("wcsncpy");

    return ret;
#endif
}


/*****************************************************************************/
//! sc_wcscpy

/*! \breif  the wide-character equivalent of the strcpy function
 ************************************
 * \param dest(out) :
 * \param src(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *
 *****************************************************************************/
DB_WCHAR *
sc_wcscpy(DB_WCHAR * dest, const DB_WCHAR * src)
{
#if defined(USE_UCS_2_0)
    DB_WCHAR *ptr = dest;

    for (; *src; dest++, src++)
        *dest = *src;
    *dest = L'\0';

    return ptr;
#else
    DB_WCHAR *ret;

    ret = wcscpy(dest, src);
    PR_ERROR("wcscpy");

    return ret;
#endif
}

/*****************************************************************************/
//! sc_wmemcpy

/*! \breif  copy an array of wide-characters
 ************************************
 * \param dest(out) :
 * \param src(in) :
 * \param n(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *
 *****************************************************************************/
void *
sc_wmemcpy(DB_WCHAR * dest, const DB_WCHAR * src, DB_INT32 n)
{
    return sc_memcpy(dest, src, n * sizeof(DB_WCHAR));
}

/*****************************************************************************/
//! sc_wmemset

/*! \breif fill an array of wide-characters with a constant wide character
 ************************************
 * \param wcs(out) :
 * \param wc(in) :
 * \param n(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *
 *****************************************************************************/
DB_WCHAR *
sc_wmemset(DB_WCHAR * wcs, DB_WCHAR wc, DB_INT32 n)
{
#if defined(WIN32)
    DB_WCHAR *_wcs = wcs;

    for (; 0 < n; ++_wcs, --n)
        *_wcs = wc;
    return wcs;
#else
#if defined(USE_UCS_2_0)
    DB_WCHAR *_wcs = wcs;

    for (; 0 < n; ++_wcs, --n)
        *_wcs = wc;
    return wcs;
#else
    DB_WCHAR *ret ret = wmemset(wcs, wc, n);

    PR_ERROR("wmemset");

    return ret;
#endif
#endif
}

/*****************************************************************************/
//! sc_wmemmove

/*! \breif  The  wmemmove function is the wide-character equivalent of      
 *          the memmove function. It copies n wide characters from the
 *          array  starting  at src to the array starting at dest. The
 *          arrays may overlap.
 *          The programmer must ensure that there is room for at least
 *          n wide characters at dest.
 ************************************
 * \param dest(out) :
 * \param src(in) :
 * \param n(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *      
 *****************************************************************************/
DB_WCHAR *
sc_wmemmove(DB_WCHAR * dest, DB_WCHAR * src, MDB_INT32 n)
{
#if defined(WIN32)
    return (DB_WCHAR *) memmove(dest, src, n * sizeof(DB_WCHAR));
#else
#if defined(USE_UCS_2_0)
    DB_WCHAR *ret;

    ret = memmove(dest, src, n * sizeof(DB_WCHAR));
    PR_ERROR("memmove");

    return ret;
#else
    DB_WCHAR *ret;

    ret = wmemmove(dest, src, n);
    PR_ERROR("memmove");

    return ret;
#endif
#endif
}

/*****************************************************************************/
//! sc_wcsstr

/*! \breif  locate a substring in a wide-character string
 ************************************
 * \param haystack(out) :
 * \param needle(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *
 *****************************************************************************/
DB_WCHAR *
sc_wcsstr(const DB_WCHAR * haystack, const DB_WCHAR * needle)
{
#if defined(USE_UCS_2_0)
    const DB_WCHAR *p;
    const DB_WCHAR *q;
    const DB_WCHAR *r;

    if (!*needle)
        return (DB_WCHAR *) haystack;

    if (sc_wcslen(haystack) < sc_wcslen(needle))
        return NULL;

    p = haystack;
    q = needle;
    while (*p)
    {
        q = needle;
        r = p;
        while (*q)
        {
            if (*r != *q)
                break;
            q++;
            r++;
        }
        if (!*q)
        {
            return (DB_WCHAR *) p;
        }
        p++;
    }
    return NULL;
#else
    DB_WCHAR *ret;

    ret = wcsstr(haystack, needle);
    PR_ERROR("wcsstr");

    return ret;
#endif
}


/*****************************************************************************/
//! sc_wcschr

/*! \breif  search  a  wide  character  in a wide-character string
 ************************************
 * \param wcs(in) :
 * \param wc(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *
 *****************************************************************************/
DB_WCHAR *
sc_wcschr(const DB_WCHAR * wcs, DB_WCHAR wc)
{
#if defined(USE_UCS_2_0)
    const DB_WCHAR *p = wcs;
    int i;

    for (i = 0; i < sc_wcslen(wcs); i++, p++)
    {
        if (*p == (DB_WCHAR) wc)
            return (DB_WCHAR *) p;
    }
    return NULL;
#else
    DB_WCHAR *ret;

    ret = wcschr(wcs, wc);
    PR_ERROR("wcschr");

    return ret;
#endif
}

__DECL_PREFIX DB_WCHAR *
sc_wcsrchr(const DB_WCHAR * wcs, DB_WCHAR wc)
{
    register DB_WCHAR *s, *p;

    if (wcs == NULL)
        return NULL;

    p = s = (DB_WCHAR *) wcs;

    /* set p to the end of string */
    while (*p)
        p++;

    if (wc == L'\0')
        return p;

    while (p != s)
    {
        if (*p == wc)
            return p;
        p--;
    }

    if (*p == wc)
        return p;

    return NULL;
}


/*****************************************************************************/
//! sc_get_taskid

/*! \breif  get current task's id
 ************************************
 * \param void
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *****************************************************************************/
__DECL_PREFIX MDB_PID
sc_get_taskid(void)
{
#if defined(linux) || defined(sun)
    return ppthread_self();
#elif defined(WIN32)
    return GetCurrentThreadId();
#else
    return 0;
#endif
}

/*****************************************************************************/
//! sc_sleep

/*! \breif  sleep in mSec;
 ************************************
 * \param mSec(in)
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *****************************************************************************/
__DECL_PREFIX void
sc_sleep(MDB_INT32 mSec)
{
#if defined(linux) || defined(sun)
    usleep(mSec * 1000);
    PR_ERROR("usleep");

    return;
#else
    Sleep(mSec);
    return;
#endif
}



static MDB_MUTEX ghEmDBMutex;
static MDB_MUTEX ghEmIRMutex;
static MDB_PID gEDBTaskId = INIT_gEDBTaskId;

#if defined(linux) || defined(sun) || defined(WIN32)
static MDB_CV ghEmDBCond;
static MDB_CV ghEmIRCond;
#endif

__DECL_PREFIX MDB_INT32
sc_db_lock_owner_transfer()
{
    gEDBTaskId = sc_get_taskid();
    return DB_SUCCESS;
}

__DECL_PREFIX MDB_INT32
sc_db_clearlock(MDB_PID terminated_taskid)
{
#if defined(linux)
#ifdef MDB_DEBUG
    __SYSLOG("sc_db_clearlock(): terminated task=[%u] = mutex owner=[%u].\n",
            terminated_taskid, gEDBTaskId);
#endif

    if (terminated_taskid == gEDBTaskId)
    {
        gEDBTaskId = INIT_gEDBTaskId;
        ppthread_cond_signal(&ghEmDBCond);
        ppthread_mutex_unlock(&ghEmDBMutex);

        return DB_SUCCESS;
    }

#else
    /* other than linux platform are not ready yet */

    sc_assert(0, __FILE__, __LINE__);

#endif
    return -1;
}

// TODO : WIN32  

/*****************************************************************************/
//! sc_db_lock_init

/*! \breif  initilize the db's lock 
 ************************************
 * \param void
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *****************************************************************************/
__DECL_PREFIX MDB_INT32
sc_db_lock_init(void)
{
#if defined(linux) || defined(sun)
    pthread_mutexattr_t _mutex_attr;
    pthread_condattr_t _cond_attr;

    ppthread_mutexattr_init(&_mutex_attr);
    ppthread_condattr_init(&_cond_attr);

    ppthread_mutexattr_setpshared(&_mutex_attr, PTHREAD_PROCESS_SHARED);
    ppthread_condattr_setpshared(&_cond_attr, PTHREAD_PROCESS_SHARED);

    ppthread_mutex_init(&ghEmDBMutex, &_mutex_attr);
    ppthread_mutex_init(&ghEmIRMutex, &_mutex_attr);

    ppthread_cond_init(&ghEmDBCond, &_cond_attr);
    ppthread_cond_init(&ghEmIRCond, &_cond_attr);

    return 0;

#elif defined(WIN32)

#ifdef WIN32
    if (sc_db_lock_init_flag != 0)
    {
        return 0;
    }
#endif

    InitializeCriticalSection(&ghEmDBMutex);
    InitializeCriticalSection(&ghEmIRMutex);

#ifdef WIN32
    sc_db_lock_init_flag = 1;
#endif

    return 0;
#else
    return 0;
#endif
}


/*****************************************************************************/
//! sc_db_lock_destroy

/*! \breif  destroy the db's lock 
 ************************************
 * \param void
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *****************************************************************************/
MDB_INT32
sc_db_lock_destroy(void)
{
#if defined(linux) || defined(sun)
    MDB_INT32 ret;

    ret = ppthread_mutex_destroy(&ghEmDBMutex);
    if (ret < 0)
        return -1;

    return 0;

#elif defined(WIN32)

    DeleteCriticalSection(&ghEmDBMutex);
    DeleteCriticalSection(&ghEmIRMutex);

#ifdef WIN32
    sc_db_lock_init_flag = 0;
#endif

    return 0;
#else
    return 0;
#endif
}


extern int DB_Params_db_lock_sleep_time();
extern int DB_Params_db_lock_sleep_time2();

/*****************************************************************************/
//! sc_db_lock

/*! \breif  lock the internal mutex 
 ************************************
 * \param void
 ************************************
 * \return RET is 1 -> locking
 *         RET is 2 -> skip
 ************************************
 * \note  \n
 *****************************************************************************/
#ifdef MDB_DEBUG
extern int dbi_Trans_ID(int connid);
extern int _g_connid;
#endif

#ifdef MDB_DEBUG
__DECL_PREFIX MDB_INT32
sc_db_lock_debug(char *file, int line)
#else
__DECL_PREFIX MDB_INT32
sc_db_lock(void)
#endif
{
#if defined(linux) || defined(sun)
    MDB_PID currentTaskId = -1;
    MDB_INT32 isSleep = 1;
    MDB_INT32 ret = MDB_LOCK_UNKNOWN;

    currentTaskId = sc_get_taskid();
    ppthread_mutex_lock(&ghEmDBMutex);  // LOCK    

    while (1)
    {
        if (gEDBTaskId == INIT_gEDBTaskId)
        {
            gEDBTaskId = currentTaskId;
            isSleep = 0;
            ret = MDB_LOCK_SET;
        }
        else if (gEDBTaskId == currentTaskId)
        {
            isSleep = 0;
            ret = MDB_LOCK_SKIP;
        }

        if (isSleep == 0)
        {
            ppthread_mutex_unlock(&ghEmDBMutex);
            break;
        }

        ppthread_cond_wait(&ghEmDBCond, &ghEmDBMutex);
    }

    return ret;

#elif defined(WIN32)

    MDB_PID currentTaskId = -1;
    MDB_INT32 isSleep = 1;
    MDB_INT32 ret = MDB_LOCK_UNKNOWN;

    currentTaskId = sc_get_taskid();
    while (1)
    {
        EnterCriticalSection(&ghEmDBMutex);     // LOCK    

        if (gEDBTaskId == INIT_gEDBTaskId)
        {
            gEDBTaskId = currentTaskId;
            isSleep = 0;
            ret = MDB_LOCK_SET;
        }
        else if (gEDBTaskId == currentTaskId)
        {
            isSleep = 0;
            ret = MDB_LOCK_SKIP;
        }

        LeaveCriticalSection(&ghEmDBMutex);

        if (isSleep == 0)
            break;

        sc_sleep(10);
    }

    return ret;
#else
    return 0;
#endif
}


__DECL_PREFIX MDB_INT32
sc_db_islocked(void)
{
    int lock_status = 1;

#if defined(linux) || defined(sun)

    ppthread_mutex_lock(&ghEmDBMutex);  // LOCK    

    if (gEDBTaskId == INIT_gEDBTaskId)
        lock_status = 0;

    ppthread_mutex_unlock(&ghEmDBMutex);

#elif defined(WIN32)

    EnterCriticalSection(&ghEmDBMutex); // LOCK    

    if (gEDBTaskId == INIT_gEDBTaskId)
        lock_status = 0;

    LeaveCriticalSection(&ghEmDBMutex);

#else

    lock_status = 0;

#endif

    return lock_status;
}

/*****************************************************************************/
//! sc_db_unlock

/*! \breif  unlock the internal mutex 
 ************************************
 * \param void
 ************************************
 * \return 
 ************************************
 * \note  \n
 *****************************************************************************/
#ifdef MDB_DEBUG
__DECL_PREFIX MDB_INT32
sc_db_unlock_debug(char *file, int line)
#else
__DECL_PREFIX MDB_INT32
sc_db_unlock(void)
#endif
{
#if defined(linux) || defined(sun)
    MDB_PID currentTaskId = sc_get_taskid();
    MDB_INT32 isError = 0;

    ppthread_mutex_lock(&ghEmDBMutex);

    if (gEDBTaskId == currentTaskId)
    {
        gEDBTaskId = INIT_gEDBTaskId;
    }
    else if (gEDBTaskId != INIT_gEDBTaskId)
        isError = 1;

    ppthread_cond_signal(&ghEmDBCond);

    ppthread_mutex_unlock(&ghEmDBMutex);

    if (isError)
    {
        return -1;
    }
    else
    {
        return 0;
    }
#elif defined(WIN32)
    MDB_PID currentTaskId = sc_get_taskid();
    MDB_INT32 isError = 0;

    EnterCriticalSection(&ghEmDBMutex);

    if (gEDBTaskId == currentTaskId)
        gEDBTaskId = INIT_gEDBTaskId;
    else if (gEDBTaskId != INIT_gEDBTaskId)
        isError = 1;
    LeaveCriticalSection(&ghEmDBMutex);

    if (isError)
        return -1;
    else
    {
        return 0;
    }

#else
    return 0;
#endif
}

/*****************************************************************************/
//! sc_assert

/*! \breif  unlock the internal mutex 
 ************************************
 * \param expression    :
 * \param file(in)        :
 * \param line(in)        :
 ************************************
 * \return 
 ************************************
 * \note  \n
 *****************************************************************************/
#if !defined(sc_assert)
__DECL_PREFIX void
sc_assert(int expression, char *file, int line)
{
#if !defined(_WIN32_WCE)
    assert(expression);
#endif
}
#endif

/*****************************************************************************/
//! wcstomsb

/*! \breif  CONVERT WIDE-CHRACTER TO CHARACTER
 ************************************
 * \param dest(out) :
 * \param src(in) :
 * \param n(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note  \n
 *  - DB_WCHAR  wchar_t   
 *  - valgrind invalid read 
 *
 *****************************************************************************/
__DECL_PREFIX int
sc_wcstombs(char *dest, const DB_WCHAR * src, int n)
{
#if defined(ANDROID_OS)
    unsigned int i = 0;

    DB_WCHAR tt = src[0];
    char tc = NULL;


    if (dest == NULL || src == NULL || n == 0)
    {
        return 0;
    }

    for (i = 0; i < n; ++i)
    {
        dest[i] = (char) src[i];
    }

    dest[n] = '\0';
    return i;
#elif defined(USE_UCS_2_0)
    int i = 0;
    char ch;
    wchar_t wch[2];

    sc_memset((char *) wch, 0, sizeof(wch));

    for (i = 0; n > 0; --n, ++i, ++src, ++dest)
    {
        wch[0] = (wchar_t) * src;
        wcstombs(&ch, wch, 1);
        PR_ERROR("wcstombs");

        *dest = ch;
    }
    return i;
#else
    int ret;

    ret = wcstombs(dest, src, n);
    PR_ERROR("wcstombs");

    return ret;
#endif
}

#if !defined(ANDROID_OS)
__DECL_PREFIX int
sc_mbstowcs(DB_WCHAR * dest, const char *src, int n)
{
#if defined(USE_UCS_2_0)
    int i;
    char ch;
    wchar_t wch;

    if (n < 0)
    {
        n = sc_strlen(src);
    }

    for (i = 0; n > 0; --n, ++i, ++src, ++dest)
    {
        ch = *src;
        mbstowcs(&wch, &ch, 1);
        PR_ERROR("mbstowcs");

        *dest = wch;
    }
    *dest = L'\0';

    return i;
#else
    int ret;

    ret = mbstowcs(dest, src, n);
    PR_ERROR("mbstowcs");

    return ret;
#endif
}
#endif

#ifndef sc_memcpy
void *
sc_memcpy(void *s1, const void *s2, size_t len)
{
    void *ret;

    ret = memcpy(s1, s2, len);
    PR_ERROR("memcpy");

    return ret;
}
#endif

#ifndef sc_strcmp
int
sc_strcmp(const char *str1, const char *str2)
{
    int ret;

    ret = strcmp(str1, str2);
    PR_ERROR("strcmp");

    return ret;
}
#endif

char *
sc_strcpy(char *s1, const char *s2)
{
    char *ret;

    ret = strcpy(s1, s2);
    PR_ERROR("strcpy");

    return ret;
}

#ifndef sc_strlen
int
sc_strlen(const char *str)
{
#if defined(linux)
    register int i;

    i = 0;
    while (str[i])
        i++;

    return i;
#else
    return strlen(str);
#endif
}
#endif

#ifndef sc_strncmp
int
sc_strncmp(const char *s1, const char *s2, int n)
{
    int ret;

    ret = strncmp(s1, s2, n);
    PR_ERROR("strncmp");

    return ret;
}
#endif

#ifndef sc_strncpy
char *
sc_strncpy(char *s1, const char *s2, int n)
{
#if defined(_AS_WIN32_MOBILE_) || defined(_AS_WINCE_)
    int i;

    for (i = 0; i < n && (s1[i] = s2[i]) != '\0'; i++);
    return s1;
#else
    char *ret;

    ret = strncpy(s1, s2, n);
    PR_ERROR("strncpy");

    return ret;
#endif
}
#endif


#ifndef sc_malloc
__DECL_PREFIX void *
sc_malloc(size_t size)
{
#ifdef CHECK_MALLOC_FREE
    int i;
    void *p;

    gCurAllocatedHeapSize += size;

    if (gCurAllocatedHeapSize > gMaxAllocatedHeapSize)
    {
        gMaxAllocatedHeapSize = gCurAllocatedHeapSize;
    }

    if (gFlag == 0)
    {
        gFlag = 1;
        for (i = 0; i < MAX_CHECK_MALLOC_FREE; i++)
        {
            gPtrChk[i].flag = -1;
            gPtrChk[i].ptr = NULL;
            gPtrChk[i].size = 0;
            gPtrChk[i].level = -1;
        }
    }

    p = malloc(size);
    if (p)
    {
        for (i = 0; i < MAX_CHECK_MALLOC_FREE; i++)
        {
            if (gPtrChk[i].flag == -1)
                break;
            if (gPtrChk[i].flag == 0)
                break;
        }
        gPtrChk[i].flag = 1;
        gPtrChk[i].ptr = p;
        gPtrChk[i].size = size;
        gPtrChk[i].level = gLevel;
    }

    return p;

#else /* else of CHECK_MALLOC_FREE */
#if defined(MDB_DEBUG)
    if (size == 0 || size > 5 * 1024 * 1024)
        sc_assert(0, __FILE__, __LINE__);
#endif

    {
        void *ret;

        ret = malloc(size);
        PR_ERROR("malloc");

        return ret;
    }
#endif /* end of CHECK_MALLOC_FREE */
}
#endif

#ifndef sc_free
__DECL_PREFIX void
sc_free(void *ptr)
{
#ifdef CHECK_MALLOC_FREE
    int i;

    for (i = 0; i < 10000; i++)
    {
        if (gPtrChk[i].flag == 0)
            continue;
        if (gPtrChk[i].ptr == ptr)
        {
            gCurAllocatedHeapSize -= gPtrChk[i].size;
            gPtrChk[i].flag = 0;
            gPtrChk[i].ptr = NULL;
            goto FREE;
        }
    }

    printf("free error !! [%p]\n", ptr);

    assert(0);

  FREE:
#endif /* CHECK_MALLOC_FREE */
    free(ptr);
    PR_ERROR("free");

    return;
}
#endif

#ifndef sc_calloc
__DECL_PREFIX void *
sc_calloc(size_t nmemb, size_t size)
{
    void *ret;

    ret = calloc(nmemb, size);
    PR_ERROR("calloc");

    return ret;
}
#endif

#ifndef sc_realloc
__DECL_PREFIX void *
sc_realloc(void *ptr, size_t size)
{
    void *ret;

    ret = realloc(ptr, size);
    PR_ERROR("realloc");

    return ret;
}
#endif

#ifndef sc_memcmp
__DECL_PREFIX int
sc_memcmp(const void *s1, const void *s2, size_t n)
{
    int ret;

    ret = memcmp(s1, s2, n);
    PR_ERROR("memcmp");

    return ret;
}
#endif

#ifndef sc_memset
__DECL_PREFIX void *
sc_memset(void *s, int c, size_t n)
{
    void *ret;

    ret = memset(s, c, n);
    PR_ERROR("memset");

    return ret;
}
#endif

#ifndef sc_memmove
__DECL_PREFIX void *
sc_memmove(void *dest, const void *src, size_t n)
{
    void *ret;

    ret = memmove(dest, src, n);
    PR_ERROR("memmove");

    return ret;
}
#endif

#ifndef sc_strcat
__DECL_PREFIX char *
sc_strcat(char *dest, const char *src)
{
    char *ret;

    ret = strcat(dest, src);
    PR_ERROR("strcat");

    return ret;
}
#endif

#ifndef sc_strchr
__DECL_PREFIX char *
sc_strchr(const char *s, int c)
{
    char *ret;

    ret = strchr(s, c);
    PR_ERROR("strchr");

    return ret;
}
#endif

#ifndef sc_strrchr
__DECL_PREFIX char *
sc_strrchr(const char *s, int c)
{
    char *ret;

    ret = strrchr(s, c);
    PR_ERROR("strrchr");

    return ret;
}
#endif

#ifndef sc_strstr
__DECL_PREFIX char *
sc_strstr(const char *haystack, const char *needle)
{
    char *ret;

    ret = strstr(haystack, needle);
    PR_ERROR("strstr");

    return ret;
}
#endif

#define _to_upper(c) (((c) >= 'a' && (c) <= 'z') ? (c)-32 : (c))

__DECL_PREFIX char *
sc_strcasestr(const char *haystack, const char *needle)
{
    register char *p, *q;
    register char *s = (char *) haystack;
    register char n0 = _to_upper(needle[0]);


    if (n0 == '\0')
        return s;


    for (; *s; s++)
    {
        if (_to_upper(*s) == n0)
        {
            for (p = s + 1, q = (char *) needle + 1; *q; p++, q++)
            {
                if (*p != *q && _to_upper(*p) != _to_upper(*q))
                    break;
            }
            if (*q == '\0')
                return s;
        }
    }
    return NULL;
}

#ifndef sc_strtok_r
__DECL_PREFIX char *
sc_strtok_r(char *s, const char *delim, char **ptrptr)
{
#if defined(WIN32)
    return strtok(s, delim);
#else
    char *ret;

    ret = strtok_r(s, delim, ptrptr);
    PR_ERROR("strtok_r");

    return ret;
#endif
}
#endif

#ifndef sc_atoi
__DECL_PREFIX int
sc_atoi(const char *nptr)
{
    int ret;

    ret = atoi(nptr);
    PR_ERROR("atoi");

    return ret;
}
#endif

#ifndef sc_atol
__DECL_PREFIX long
sc_atol(const char *nptr)
{
    long ret;

    ret = atol(nptr);
    PR_ERROR("atol");

    return ret;
}
#endif

#ifndef sc_atof
__DECL_PREFIX double
sc_atof(const char *nptr)
{
    double ret;

    ret = atof(nptr);
    PR_ERROR("atof");

    return ret;
}
#endif

#ifndef sc_strtol
__DECL_PREFIX long int
sc_strtol(const char *nptr, char **endptr, int base)
{
    long int ret;

    ret = strtol(nptr, endptr, base);
    PR_ERROR("strtol");

    return ret;
}
#endif

#ifndef sc_toupper
__DECL_PREFIX int
sc_toupper(int c)
{
    return toupper(c);
}
#endif

#ifndef sc_tolower
__DECL_PREFIX int
sc_tolower(int c)
{
    return tolower(c);
}
#endif

#ifndef sc_pow
__DECL_PREFIX double
sc_pow(double x, double y)
{
    double ret;

    ret = pow(x, y);
    PR_ERROR("pow");

    return ret;
}
#endif

#ifndef sc_rand
__DECL_PREFIX int
sc_rand(void)
{
#if defined(sun)
    return random();
#else
    int ret;

    ret = rand();
    PR_ERROR("rand");

    return ret;
#endif
}
#endif

#ifndef sc_srand
__DECL_PREFIX void
sc_srand(unsigned int seed)
{
#if defined(sun)
    srandom(seed);
#else
    srand(seed);
#endif
}
#endif

#ifndef sc_floor
__DECL_PREFIX double
sc_floor(double x)
{
    double ret;

    ret = floor(x);
    PR_ERROR("floor");

    return ret;
}
#endif

#ifndef sc_ceil
__DECL_PREFIX double
sc_ceil(double x)
{
    double ret;

    ret = ceil(x);
    PR_ERROR("ceil");

    return ret;
}
#endif

#ifndef sc_abs
__DECL_PREFIX int
sc_abs(int j)
{
    int ret;

    ret = abs(j);
    PR_ERROR("abs");

    return ret;
}
#endif

#ifndef sc_printf
__DECL_PREFIX int
sc_printf(const char *format, ...)
{
#if defined(MDB_DEBUG)
    int len;
    va_list args;

    va_start(args, format);
    len = vprintf(format, args);
    PR_ERROR("vprintf");

    va_end(args);
    return len;
#else
    return 0;
#endif
}
#endif

#ifndef sc_sprintf
__DECL_PREFIX int
sc_sprintf(char *buffer, const char *format, ...)
{
    int len;
    va_list args;

    va_start(args, format);
    len = sc_vsnprintf(buffer, MDB_INT_MAX, format, args);
    va_end(args);
    return len;
}
#endif

#ifndef sc_vsnprintf
__DECL_PREFIX int
sc_vsnprintf(char *buf, int size, const char *format, va_list ap)
{
#if defined(WIN32)
    if (buf == 0x00 && size == 0)
    {
        char _buf[8192];

        _vsnprintf(_buf, sizeof(_buf), format, ap);
        return sc_strlen(_buf);
    }

    return _vsnprintf(buf, size, format, ap);
#else
    int ret;

    ret = vsnprintf(buf, size, format, ap);
    PR_ERROR("vsnprintf");

    return ret;
#endif
}
#endif /* !sc_vsnprintf */

// determine in porting 
extern int sc_strdcmp(const char *str1, const char *str2);
extern int sc_strndcmp(const char *s1, const char *s2, int n);
__DECL_PREFIX char *__strchr(const char *string, const char *pEnd, int ch);

__DECL_PREFIX int
sc_get_disk_free_space(const char *path)
{
#if defined(WIN32)
    DB_BOOL fResult;
    DB_INT64 i64FreeBytesToCaller;
    DB_INT64 i64TotalBytes;
    DB_INT64 i64FreeBytes;

    int len = 0;
    char dir[MAX_PATH];
    char *ptr;

    /* the path includes a file name */
    sc_strcpy(dir, path);
    ptr = sc_strrchr(dir, DIR_DELIM_CHAR);
    if (ptr != NULL)
    {
        *ptr = '\0';
    }

    fResult = GetDiskFreeSpaceEx(dir,
            (PULARGE_INTEGER) & i64FreeBytesToCaller,
            (PULARGE_INTEGER) & i64TotalBytes,
            (PULARGE_INTEGER) & i64FreeBytes);

    if (!fResult)
    {
        __SYSLOG("FAIL : GetDiskFreeSpaceEx, %d\n", GetLastError());
        return (-1);
    }

    return (int) i64FreeBytesToCaller;

#else
#if defined(ANDROID_OS)
    return (1024 * 1024 * 1024);
#else
    int error;
    int freebytes;
    struct statvfs buf;

    error = statvfs(path, &buf);

    if (error < 0)
    {
        __SYSLOG("FAIL : statvfs, %d\n", error);
        return (-1);
    }

    freebytes = (int) ((DB_INT64) buf.f_frsize * (DB_INT64) buf.f_bavail);

    if (freebytes < 0)
        return (-1);
    else
        return freebytes;
#endif

#endif
}

#include "sc_collation_main.c"

__DECL_PREFIX void
sc_kick_dog(void)
{
    return;
}

__DECL_PREFIX int
sc_calc_system()
{
    return 0;
}

__DECL_PREFIX int
sc_file_errno(void)
{
#if defined(WIN32)
    return GetLastError();
#else
    return errno;
#endif
}

/* add to error message for RAM-DUMP */
#if defined(WRITE_ERROR_MSG_2_MEMORY)
#define MAX_ERRORMSG_QUEUE  (5)
typedef struct
{
    int _initiate;
    int nNewPos;
    MDB_PID task_id[MAX_ERRORMSG_QUEUE];
    int allocSize[MAX_ERRORMSG_QUEUE];
    char *pErrMsg[MAX_ERRORMSG_QUEUE];
} SErrMsgQueue;

SErrMsgQueue g_EMsgQueue = { 0, 0, {0,}, {0,}, {NULL,} };
#endif


__DECL_PREFIX void
sc_add_errmsg2queue(char *pmsg)
{
#if defined(WRITE_ERROR_MSG_2_MEMORY)
    int len;
    char *p;

    if (g_EMsgQueue._initiate == 0)
    {
        sc_memset(&g_EMsgQueue, 0x00, sizeof(SErrMsgQueue));
        g_EMsgQueue._initiate = 1;      /* set initiate */
    }

    len = sc_strlen(pmsg);
    p = NULL;
    if (g_EMsgQueue.pErrMsg[g_EMsgQueue.nNewPos])
    {
        if (g_EMsgQueue.allocSize[g_EMsgQueue.nNewPos] <= len)
        {
            sc_free(g_EMsgQueue.pErrMsg[g_EMsgQueue.nNewPos]);
            g_EMsgQueue.pErrMsg[g_EMsgQueue.nNewPos] = NULL;
            g_EMsgQueue.allocSize[g_EMsgQueue.nNewPos] = 0;
        }
        else
            p = g_EMsgQueue.pErrMsg[g_EMsgQueue.nNewPos];
    }

    if (!p)
    {
        p = (char *) sc_malloc(len + 1);
    }

    g_EMsgQueue.task_id[g_EMsgQueue.nNewPos] = sc_get_taskid();
    if (p)
    {
        sc_strcpy(p, pmsg);
        g_EMsgQueue.pErrMsg[g_EMsgQueue.nNewPos] = p;
        g_EMsgQueue.allocSize[g_EMsgQueue.nNewPos] = len + 1;
    }
    else
    {
        g_EMsgQueue.pErrMsg[g_EMsgQueue.nNewPos] = NULL;
        g_EMsgQueue.allocSize[g_EMsgQueue.nNewPos] = 0;
    }

    g_EMsgQueue.nNewPos = (g_EMsgQueue.nNewPos + 1) % MAX_ERRORMSG_QUEUE;
#else
    return;
#endif
}
