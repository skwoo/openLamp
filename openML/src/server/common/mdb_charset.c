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
#include "mdb_FieldVal.h"
#include "db_api.h"
#include "mdb_compat.h"

#include "mdb_collation.h"

extern int sc_strcmp2(const char *s1, const char *s2);
extern int sc_strcasecmp(const char *pp_s1, const char *pp_s2);
extern int sc_strdcmp(const char *str1, const char *str2);
extern int sc_strndcmp(const char *s1, const char *s2, int n);
extern int sc_strncmp2(const char *s1, const char *s2, int n);
extern char *__strichr(const char *string, const char *pEnd, int ch);
__DECL_PREFIX char *__strchr(const char *string, const char *pEnd, int ch);
extern DB_WCHAR *__wstrichr(const DB_WCHAR * string, const DB_WCHAR * pEnd,
        DB_WCHAR ch);
extern DB_WCHAR *__wcschr(const DB_WCHAR * string, const DB_WCHAR * pEnd,
        DB_WCHAR ch);

__DECL_PREFIX int sc_strcmp_user1(const char *str1, const char *str2);
__DECL_PREFIX int sc_strcmp_user2(const char *str1, const char *str2);
__DECL_PREFIX int sc_strcmp_user3(const char *str1, const char *str2);
__DECL_PREFIX int sc_strncmp_user1(const char *s1, const char *s2, int n);
__DECL_PREFIX int sc_strncmp_user2(const char *s1, const char *s2, int n);
__DECL_PREFIX int sc_strncmp_user3(const char *s1, const char *s2, int n);
__DECL_PREFIX char *sc_strchr_user1(const char *string, const char *pEnd,
        int ch);
__DECL_PREFIX char *sc_strchr_user2(const char *string, const char *pEnd,
        int ch);
__DECL_PREFIX char *sc_strchr_user3(const char *string, const char *pEnd,
        int ch);

__DECL_PREFIX char *sc_strstr_user1(const char *haystack, const char *needle);
__DECL_PREFIX char *sc_strstr_user2(const char *haystack, const char *needle);
__DECL_PREFIX char *sc_strstr_user3(const char *haystack, const char *needle);

__DECL_PREFIX int (*sc_collate_func[]) () =
{
    NULL,   /* MDB_COL_NONE */
            NULL,       /* MDB_COL_NUMERIC */
            sc_strcasecmp,      /* MDB_COL_CHAR_NOCASE    */
            sc_strcmp2, /* MDB_COL_CHAR_CASE  */
            sc_strdcmp, /* MDB_COL_CHAR_DICORDER */
            sc_wcscmp,  /* MDB_COL_NCHAR_NOCASE */
            sc_wcscmp,  /* MDB_COL_NCHAR_CASE   */
            NULL,       /* MDB_COL_BYTE */
            sc_strcmp_user1,    /* MDB_COL_USER_TYPE1 */
            sc_strcmp_user2,    /* MDB_COL_USER_TYPE2 */
            sc_strcmp_user3,    /* MDB_COL_USER_TYPE3 */
            mdb_strrcmp,        /* MDB_COL_CHAR_REVERSE */
            NULL        /* MDB_COL_MAX */
};

int (*sc_ncollate_func[]) () =
{
    NULL,   /* MDB_COL_NONE */
            NULL,       /* MDB_COL_NUMERIC */
            sc_strncasecmp,     /* MDB_COL_CHAR_NOCASE */
            sc_strncmp2,        /* MDB_COL_CHAR_CASE */
            sc_strndcmp,        /* MDB_COL_CHAR_DICORDER */
            sc_wcsncmp, /* MDB_COL_NCHAR_NOCASE */
            sc_wcsncmp, /* MDB_COL_NCHAR_CASE */
            sc_memcmp,  /* MDB_COL_BYTE */
            sc_strncmp_user1,   /* MDB_COL_USER_TYPE1 */
            sc_strncmp_user2,   /* MDB_COL_USER_TYPE2 */
            sc_strncmp_user3,   /* MDB_COL_USER_TYPE3 */
            mdb_strnrcmp,       /* MDB_COL_CHAR_REVERSE */
            NULL        /* MDB_COL_MAX */
};

void *(*sc_collate_memcpy_func[]) () =
{
    sc_memcpy,  /* MDB_COL_NONE */
            sc_memcpy,  /* MDB_COL_NUMERIC */
            sc_memcpy,  /* MDB_COL_CHAR_NOCASE */
            sc_memcpy,  /* MDB_COL_CHAR_CASE */
            sc_memcpy,  /* MDB_COL_CHAR_DICORDER */
            sc_memcpy,  /* MDB_COL_NCHAR_NOCASE */
            sc_memcpy,  /* MDB_COL_NCHAR_CASE */
            sc_memcpy,  /* MDB_COL_BYTE */
            sc_memcpy,  /* MDB_COL_USER_TYPE1 */
            sc_memcpy,  /* MDB_COL_USER_TYPE2 */
            sc_memcpy,  /* MDB_COL_USER_TYPE3 */
            mdb_memrcpy,        /* MDB_COL_CHAR_REVERSE */
            sc_memcpy   /* MDB_COL_MAX */
};


char *
tmp_sc_wcsstr(const DB_WCHAR * haystack, const DB_WCHAR * needle)
{
    return (char *) sc_wcsstr(haystack, needle);
}

char *(*sc_collate_strstr_func[]) () =
{
    NULL,   /* MDB_COL_NONE */
            NULL,       /* MDB_COL_NUMERIC */
            sc_strcasestr,      /* MDB_COL_CHAR_NOCASE */
            sc_strstr,  /* MDB_COL_CHAR_CASE */
            sc_strstr,  /* MDB_COL_CHAR_DICORDER */
            tmp_sc_wcsstr,      /* MDB_COL_NCHAR_NOCASE */
            tmp_sc_wcsstr,      /* MDB_COL_NCHAR_CASE */
            NULL,       /* MDB_COL_BYTE */
            sc_strstr_user1,    /* MDB_COL_USER_TYPE1 */
            sc_strstr_user2,    /* MDB_COL_USER_TYPE2 */
            sc_strstr_user3,    /* MDB_COL_USER_TYPE3 */
            sc_strstr,  /* MDB_COL_CHAR_REVERSE */
            NULL        /* MDB_COL_MAX */
};

char *
tmp_wstrichr(const DB_WCHAR * string, const DB_WCHAR * pEnd, int ch)
{
    return (char *) __wstrichr(string, pEnd, (DB_WCHAR) ch);
}

char *
tmp_wcschr(const DB_WCHAR * string, const DB_WCHAR * pEnd, int ch)
{
    return (char *) __wcschr(string, pEnd, (DB_WCHAR) ch);
}

char *(*sc_collate_strchr_func[]) () =
{
    NULL,   /* MDB_COL_NONE */
            NULL,       /* MDB_COL_NUMERIC */
            __strichr,  /* MDB_COL_CHAR_NOCASE */
            __strchr,   /* MDB_COL_CHAR_CASE */
            __strchr,   /* MDB_COL_CHAR_DICORDER */
            tmp_wstrichr, tmp_wcschr, NULL,     /* MDB_COL_BYTE */
            sc_strchr_user1,    /* MDB_COL_USER_TYPE1 */
            sc_strchr_user2,    /* MDB_COL_USER_TYPE2 */
            sc_strchr_user3,    /* MDB_COL_USER_TYPE3 */
            NULL        /* MDB_COL_MAX */
};


/*****************************************************************************/
//! db_find_collation

/*! \breif identifier를 비교하여 collation type을 리턴함
 ************************************
 * \param identifier(in)    : identifier
 ************************************
 * \return  return_value    MDB_COL_TYPE type
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - converting rule
 *      char_ci  : MDB_COL_CHAR_NOCASE
 *      char_cs  : MDB_COL_CHAR_CASE
 *      char_dic : MDB_COL_CHAR_DICORDER
 *      nchar_ci : MDB_COL_NCHAR_NOCASE
 *      nchar_cs : MDB_COL_NCHAR_CASE
 *      user1    : MDB_COL_USER_TYPE1
 *      user2    : MDB_COL_USER_TYPE2
 *      user3    : MDB_COL_USER_TYPE3
 *  - 함수 생성
 *****************************************************************************/
__DECL_PREFIX MDB_COL_TYPE
db_find_collation(char *identifier)
{
    if (!mdb_strcasecmp(identifier, "char_ci"))
        return MDB_COL_CHAR_NOCASE;
    else if (!mdb_strcasecmp(identifier, "char_cs"))
        return MDB_COL_CHAR_CASE;
    else if (!mdb_strcasecmp(identifier, "char_dic"))
        return MDB_COL_CHAR_DICORDER;
    else if (!mdb_strcasecmp(identifier, "nchar_cs"))
        return MDB_COL_NCHAR_CASE;
    else if (!mdb_strcasecmp(identifier, "user1"))
        return MDB_COL_USER_TYPE1;
    else if (!mdb_strcasecmp(identifier, "user2"))
        return MDB_COL_USER_TYPE2;
    else if (!mdb_strcasecmp(identifier, "user3"))
        return MDB_COL_USER_TYPE3;
    else if (!mdb_strcasecmp(identifier, "char_reverse"))
        return MDB_COL_CHAR_REVERSE;
    else
        return MDB_COL_NONE;
}
