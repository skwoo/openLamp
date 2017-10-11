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
#include <math.h>

#include <wchar.h>
#include <wctype.h>

#ifdef  __cplusplus
extern "C"
{
#endif

    __DECL_PREFIX int sc_strdcmp(const char *str1, const char *str2);
    __DECL_PREFIX int sc_strndcmp(const char *s1, const char *s2, int n);
    __DECL_PREFIX char *__strchr(const char *string, const char *pEnd, int ch);

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


/*! \breif  compare the string pointed to by s1 to the string pointed to by s2
 ************************************
 * \param str1(in)  :
 * \param str2(in)  :
 ************************************
 * \return 
 ************************************
 * \note 내부 알고리즘
 *  - 함수 생성
 *  - TODO :
 *      포팅시 사용자가 수정할 수 있다
 *****************************************************************************/
// UNICODE에 대해서 Case Insensitive하게 동작
// sc_strdcmp(str1, str2);와 동일한 동작
__DECL_PREFIX int
sc_strcmp_user1(const char *str1, const char *str2)
{
#if defined(ANDROID_OS)
    return 0;
#elif defined(WIN32)
    return wcsicmp((wchar_t *) str1, (wchar_t *) str2);
#else
    wchar_t *wstr1;
    wchar_t *wstr2;
    wchar_t *tmp_wstr;
    int str1_len, str2_len;
    int nCmp;
    DB_WCHAR *ptr;

#if !defined(__USE_XOPEN2K8) && defined(linux)
    int wcscasecmp(wchar_t *, wchar_t *);
#endif

    str1_len = sc_wcslen((DB_WCHAR *) str1);
    wstr1 = (wchar_t *) mdb_malloc(sizeof(wchar_t) * (str1_len + 1));
    if (!wstr1)
        return 0;

    str2_len = sc_wcslen((DB_WCHAR *) str2);
    wstr2 = (wchar_t *) mdb_malloc(sizeof(wchar_t) * (str2_len + 1));
    if (!wstr2)
    {
        mdb_free(wstr1);
        return 0;
    }

    sc_memset(wstr1, 0x00, sizeof(wchar_t) * (str1_len + 1));
    sc_memset(wstr2, 0x00, sizeof(wchar_t) * (str2_len + 1));

    ptr = (DB_WCHAR *) str1;
    tmp_wstr = wstr1;
    for (; *ptr; ++ptr, ++tmp_wstr)
        *tmp_wstr = (wchar_t) * ptr;

    ptr = (DB_WCHAR *) str2;
    tmp_wstr = wstr2;
    for (; *ptr; ++ptr, ++tmp_wstr)
        *tmp_wstr = (wchar_t) * ptr;

#if defined(linux)
    nCmp = wcscasecmp(wstr1, wstr2);
#else
    nCmp = sc_wcscasecmp(wstr1, wstr2);
#endif

    mdb_free(wstr1);
    mdb_free(wstr2);

    return nCmp;
#endif
}

/*****************************************************************************/
//! sc_strcmp_user2

/*! \breif  compare the string pointed to by s1 to the string pointed to by s2
 ************************************
 * \param str1(in)  :
 * \param str2(in)  :
 ************************************
 * \return 
 ************************************
 * \note 내부 알고리즘
 *  - 함수 생성
 *  - TODO :
 *      포팅시 사용자가 수정할 수 있다
 *****************************************************************************/
__DECL_PREFIX int
sc_strcmp_user2(const char *str1, const char *str2)
{
    return sc_strdcmp(str1, str2);
}

/*****************************************************************************/
//! sc_strcmp_user3

/*! \breif  compare the string pointed to by s1 to the string pointed to by s2
 ************************************
 * \param str1(in)  :
 * \param str2(in)  :
 ************************************
 * \return 
 ************************************
 * \note 내부 알고리즘
 *  - 함수 생성
 *  - TODO :
 *      포팅시 사용자가 수정할 수 있다
 *****************************************************************************/
__DECL_PREFIX int
sc_strcmp_user3(const char *str1, const char *str2)
{
    return sc_strdcmp(str1, str2);
}

/*****************************************************************************/
//! sc_strncmp_user1

/*! \breif  compare the string pointed to by s1 to the string pointed to by s2
 *          in n- bytes
 ************************************
 * \param s1(in)    :
 * \param s2(in)    :
 * \param n(in)    :
 ************************************
 * \return 
 ************************************
 * \note 내부 알고리즘
 *  - 함수 생성
 *  - TODO :
 *      포팅시 사용자가 수정할 수 있다
 *****************************************************************************/
// UNICODE에 대해서 Case Insensitive하게 동작
// sc_strndcmp(s1, s2, n)와 동일한 동작
__DECL_PREFIX int
sc_strncmp_user1(const char *s1, const char *s2, int n)
{
#if defined(ANDROID_OS)
    return 0;
#elif defined(WIN32)
    return wcsnicmp((wchar_t *) s1, (wchar_t *) s2, n);
#else
    wchar_t *wstr1;
    wchar_t *wstr2;
    wchar_t *tmp_wstr;
    int nCmp;
    int s1_len, s2_len;
    DB_WCHAR *ptr;

#if !defined(__USE_XOPEN2K8) && defined(linux)
    int wcsncasecmp(wchar_t *, wchar_t *, int);
#endif

    s1_len = sc_wcslen((DB_WCHAR *) s1);

    wstr1 = (wchar_t *) mdb_malloc(sizeof(wchar_t) * (s1_len + 1));
    if (!wstr1)
        return 0;

    s2_len = sc_wcslen((DB_WCHAR *) s2);

    wstr2 = (wchar_t *) mdb_malloc(sizeof(wchar_t) * (s2_len + 1));
    if (!wstr2)
    {
        mdb_free(wstr1);
        return DB_E_OUTOFMEMORY;
    }

    sc_memset(wstr1, 0x00, sizeof(wchar_t) * (s1_len + 1));
    sc_memset(wstr2, 0x00, sizeof(wchar_t) * (s2_len + 1));

    ptr = (DB_WCHAR *) s1;
    tmp_wstr = wstr1;
    for (; *ptr; ++ptr, ++tmp_wstr)
        *tmp_wstr = (wchar_t) * ptr;

    ptr = (DB_WCHAR *) s2;
    tmp_wstr = wstr2;
    for (; *ptr; ++ptr, ++tmp_wstr)
        *tmp_wstr = (wchar_t) * ptr;

#if defined(linux)
    nCmp = wcsncasecmp(wstr1, wstr2, n);
#else
    nCmp = sc_wcsncasecmp(wstr1, wstr2, n);
#endif

    mdb_free(wstr1);
    mdb_free(wstr2);

    return nCmp;
#endif
}

/*****************************************************************************/
//! sc_strncmp_user2

/*! \breif  compare the string pointed to by s1 to the string pointed to by s2
 *          in n- bytes
 ************************************
 * \param s1(in)    :
 * \param s2(in)    :
 * \param n(in)    :
 ************************************
 * \return 
 ************************************
 * \note 내부 알고리즘
 *  - 함수 생성
 *  - TODO :
 *      포팅시 사용자가 수정할 수 있다
 *****************************************************************************/
__DECL_PREFIX int
sc_strncmp_user2(const char *s1, const char *s2, int n)
{
    return sc_strndcmp(s1, s2, n);
}

/*****************************************************************************/
//! sc_strncmp_user3

/*! \breif  compare the string pointed to by s1 to the string pointed to by s2
 *          in n- bytes
 ************************************
 * \param s1(in)    :
 * \param s2(in)    :
 * \param n(in)    :
 ************************************
 * \return 
 ************************************
 * \note 내부 알고리즘
 *  - 함수 생성
 *  - TODO :
 *      포팅시 사용자가 수정할 수 있다
 *****************************************************************************/
__DECL_PREFIX int
sc_strncmp_user3(const char *s1, const char *s2, int n)
{
    return sc_strndcmp(s1, s2, n);
}

/*****************************************************************************/
//! sc_strchr_user1

/*! \breif  locate the first occurrence of ch in the string
 ************************************
 * \param string(in):
 * \param pEnd(in)  :
 * \param ch(in)    :
 ************************************
 * \return 
 ************************************
 * \note 내부 알고리즘
 *  like operation에서 사용됨
 *  - 함수 생성
 *  - TODO :
 *      포팅시 사용자가 수정할 수 있다
 *****************************************************************************/
// UNICODE에 대해서 Case Insensitive하게 동작
__DECL_PREFIX char *
sc_strchr_user1(const char *string, const char *pEnd, int ch)
{
    wchar_t *wstr1;
    wchar_t *tmp_wstr;
    wint_t wstr2 = towupper((wchar_t) pEnd);
    int string_len;
    DB_WCHAR *ptr;

    if (wstr2 == (wchar_t) '\0')
        return NULL;

    string_len = sc_wcslen((DB_WCHAR *) string);

    wstr1 = (wchar_t *) mdb_malloc(sizeof(wchar_t) * (string_len + 1));
    if (!wstr1)
        return NULL;

    sc_memset(wstr1, 0x00, sizeof(wchar_t) * (string_len + 1));

    ptr = (DB_WCHAR *) string;
    tmp_wstr = wstr1;
    for (; *ptr; ++ptr, ++tmp_wstr)
        *tmp_wstr = (wchar_t) * ptr;
    tmp_wstr = wstr1;

    for (; *tmp_wstr; tmp_wstr++, string++)
    {
        if (towupper(*tmp_wstr) == wstr2)
        {
            mdb_free(wstr1);

            return (char *) string;
        }
    }

    mdb_free(wstr1);

    return NULL;
}

/*****************************************************************************/
//! sc_strchr_user2

/*! \breif  locate the first occurrence of ch in the string
 ************************************
 * \param string(in):
 * \param pEnd(in)  :
 * \param ch(in)    :
 ************************************
 * \return 
 ************************************
 * \note 내부 알고리즘
 *  like operation에서 사용됨
 *  - 함수 생성
 *  - TODO :
 *      포팅시 사용자가 수정할 수 있다
 *****************************************************************************/
__DECL_PREFIX char *
sc_strchr_user2(const char *string, const char *pEnd, int ch)
{
    return __strchr(string, pEnd, ch);
}

/*****************************************************************************/
//! sc_strchr_user3

/*! \breif  locate the first occurrence of ch in the string
 ************************************
 * \param string(in):
 * \param pEnd(in)  :
 * \param ch(in)    :
 ************************************
 * \return 
 ************************************
 * \note 내부 알고리즘
 *  like operation에서 사용됨
 *  - 함수 생성
 *  - TODO :
 *      포팅시 사용자가 수정할 수 있다
 *****************************************************************************/
__DECL_PREFIX char *
sc_strchr_user3(const char *string, const char *pEnd, int ch)
{
    return __strchr(string, pEnd, ch);
}

// UNICODE에 대해서 Case Insensitive하게 동작
__DECL_PREFIX char *
sc_strstr_user1(const char *haystack, const char *needle)
{
    wchar_t *wstr1;
    wchar_t *wstr2;
    wchar_t *tmp_wstr;
    int haystack_len, needle_len;
    DB_WCHAR *ptr;

    haystack_len = sc_wcslen((DB_WCHAR *) haystack);

    wstr1 = (wchar_t *) mdb_malloc(sizeof(wchar_t) * (haystack_len + 1));
    if (!wstr1)
        return NULL;

    needle_len = sc_wcslen((DB_WCHAR *) needle);

    wstr2 = (wchar_t *) mdb_malloc(sizeof(wchar_t) * (needle_len + 1));
    if (!wstr2)
    {
        mdb_free(wstr1);
        return NULL;
    }

    sc_memset(wstr1, 0x00, sizeof(wchar_t) * (haystack_len + 1));
    sc_memset(wstr2, 0x00, sizeof(wchar_t) * (needle_len + 1));

    ptr = (DB_WCHAR *) haystack;
    tmp_wstr = wstr1;
    for (; *ptr; ++ptr, ++tmp_wstr)
        *tmp_wstr = (wchar_t) * ptr;

    ptr = (DB_WCHAR *) needle;
    tmp_wstr = wstr2;
    for (; *ptr; ++ptr, ++tmp_wstr)
        *tmp_wstr = (wchar_t) * ptr;


    {
        register wchar_t *p, *q;
        register wchar_t *s = (wchar_t *) wstr1;
        register wint_t n0 = towupper(wstr2[0]);
        register int f_n0_n1;

        if (n0 == '\0')
        {
            mdb_free(wstr1);
            mdb_free(wstr2);

            return (char *) haystack;
        }

        f_n0_n1 = (n0 != towupper(wstr2[1]));

        for (; *s; s++, haystack++)
        {
            if (towupper(*s) == n0)
            {
                for (p = s + 1, q = (wchar_t *) needle + 1; *q; p++, q++)
                {
                    if (*p != *q && towupper(*p) != towupper(*q))
                        break;
                }

                if (*q == '\0')
                {
                    mdb_free(wstr1);
                    mdb_free(wstr2);

                    return (char *) haystack;
                }
                if (f_n0_n1)
                    s++;
            }
        }

    }

    mdb_free(wstr1);
    mdb_free(wstr2);

    return NULL;
}

__DECL_PREFIX char *
sc_strstr_user2(const char *haystack, const char *needle)
{
    return (char *) sc_strstr(haystack, needle);
}

__DECL_PREFIX char *
sc_strstr_user3(const char *haystack, const char *needle)
{
    return (char *) sc_strstr(haystack, needle);
}
