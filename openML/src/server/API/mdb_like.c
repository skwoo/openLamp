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
#include "mdb_dbi.h"

static int __like_strncasecmp(char *, char *, int, char *, DB_BOOL);
static int __like_strncmp(char *, char *, int, char *, DB_BOOL);
static int __like_wcsncmp(char *, char *, int, char *, DB_BOOL);
static int __like_strncmp_user1(char *, char *, int, char *, DB_BOOL);
static int __like_strncmp_user2(char *, char *, int, char *, DB_BOOL);
static int __like_strncmp_user3(char *, char *, int, char *, DB_BOOL);

int (*__collate_like_strncmp_func[]) (char *, char *, int, char *, DB_BOOL) =
{
    NULL,   /* MDB_COL_NONE */
            NULL,       /* MDB_COL_NUMERIC */
            __like_strncasecmp, /* MDB_COL_CHAR_NOCASE */
            __like_strncmp,     /* MDB_COL_CHAR_CASE */
            __like_strncmp,     /* MDB_COL_CHAR_DICORDER */
            __like_wcsncmp /*__like_wcsncasecmp*/ ,     /* MDB_COL_NCHAR_NOCASE */
            __like_wcsncmp,     /* MDB_COL_NCHAR_CASE */
            NULL,       /* MDB_COL_BYTE */
            __like_strncmp_user1,       /* MDB_COL_USER_TYPE1 */
            __like_strncmp_user2,       /* MDB_COL_USER_TYPE2 */
            __like_strncmp_user3,       /* MDB_COL_USER_TYPE3 */
            __like_strncmp,     /* MDB_COL_CHAR_REVERSE */
            NULL        /* MDB_COL_MAX */
};

static int __like_wcsncasecmp(char *, char *, int, char *, DB_BOOL);

int (*__collate_like_strncasecmp_func[]) (char *, char *, int, char *,
        DB_BOOL) =
{
    NULL,   /* MDB_COL_NONE */
            NULL,       /* MDB_COL_NUMERIC */
            __like_strncasecmp, /* MDB_COL_CHAR_NOCASE */
            __like_strncasecmp, /* MDB_COL_CHAR_CASE */
            __like_strncasecmp, /* MDB_COL_CHAR_DICORDER */
            __like_wcsncasecmp, /* MDB_COL_NCHAR_NOCASE */
            __like_wcsncasecmp, /* MDB_COL_NCHAR_CASE */
            NULL,       /* MDB_COL_BYTE */
            __like_strncmp_user1,       /* MDB_COL_USER_TYPE1 */
            __like_strncmp_user2,       /* MDB_COL_USER_TYPE2 */
            __like_strncmp_user3,       /* MDB_COL_USER_TYPE3 */
            __like_strncasecmp, /* MDB_COL_CHAR_REVERSE */
            NULL        /* MDB_COL_MAX */
};

static void *__like_strcasestr(char *, char *, char *, DB_BOOL);
static void *__like_strstr(char *, char *, char *, DB_BOOL);
static void *__like_wcsstr(char *, char *, char *, DB_BOOL);
static void *__like_strstr_user1(char *, char *, char *, DB_BOOL);
static void *__like_strstr_user2(char *, char *, char *, DB_BOOL);
static void *__like_strstr_user3(char *, char *, char *, DB_BOOL);

void *(*__collate_like_strstr_func[]) (char *, char *, char *, DB_BOOL) =
{
    NULL,   /* MDB_COL_NONE */
            NULL,       /* MDB_COL_NUMERIC */
            __like_strcasestr,  /* MDB_COL_CHAR_NOCASE */
            __like_strstr,      /* MDB_COL_CHAR_CASE */
            __like_strstr,      /* MDB_COL_CHAR_DICORDER */
            __like_wcsstr /*__like_wcscasestr*/ ,       /* MDB_COL_NCHAR_NOCASE */
            __like_wcsstr,      /* MDB_COL_NCHAR_CASE */
            NULL,       /* MDB_COL_BYTE */
            __like_strstr_user1,        /* MDB_COL_USER_TYPE1 */
            __like_strstr_user2,        /* MDB_COL_USER_TYPE2 */
            __like_strstr_user3,        /* MDB_COL_USER_TYPE3 */
            __like_strstr,      /* MDB_COL_CHAR_REVERSE */
            NULL        /* MDB_COL_MAX */
};

static void *__like_wcscasestr(char *, char *, char *, DB_BOOL);

void *(*__collate_like_strcasestr_func[]) (char *, char *, char *, DB_BOOL) =
{
    NULL,   /* MDB_COL_NONE */
            NULL,       /* MDB_COL_NUMERIC */
            __like_strcasestr,  /* MDB_COL_CHAR_NOCASE */
            __like_strcasestr,  /* MDB_COL_CHAR_CASE */
            __like_strcasestr,  /* MDB_COL_CHAR_DICORDER */
            __like_wcscasestr,  /* MDB_COL_NCHAR_NOCASE */
            __like_wcscasestr,  /* MDB_COL_NCHAR_CASE */
            NULL,       /* MDB_COL_BYTE */
            __like_strstr_user1,        /* MDB_COL_USER_TYPE1 */
            __like_strstr_user2,        /* MDB_COL_USER_TYPE2 */
            __like_strstr_user3,        /* MDB_COL_USER_TYPE3 */
            __like_strcasestr,  /* MDB_COL_CHAR_REVERSE */
            NULL        /* MDB_COL_MAX */
};

extern int (*sc_collate_func[]) ();
extern int (*sc_ncollate_func[]) ();
extern char *(*sc_collate_strchr_func[]) ();
extern char *(*sc_collate_strstr_func[]) ();


#define LIKE_STRING    (0)
#define LIKE_UNDERBAR  (1)
#define LIKE_PERCENT   (2)

#define GetOtherCaseLetter(c)  (((c) >= 'a' && (c) <= 'z') ?   (c)-32 : \
                                   (((c) >='A' && (c)<='Z') ? (c)+32 : (c)))
#define GetOtherCaseLetterW(c) (((c) >= L'a' && (c) <= L'z') ? (c)-32 : \
                                   (((c) >= L'A' && (c)<= L'Z') ? (c)+32 :(c)))
#define GetBigCaseLetter(c)    (((c) >= 'a' && (c) <= 'z') ?   (c)-32 :(c))
#define GetBigCaseLetterW(c)   (((c) >= L'a' && (c) <= L'z') ? (c)-32 : (c))
#define GetSmallCaseLetter(c)  (((c) >= 'A' && (c) <= 'Z')   ? (c)+32 : (c))
#define GetSmallCaseLetterW(c) (((c) >= L'A' && (c) <= L'Z') ? (c)+32 : (c))

#define MC_IsEndString(s, t)     ( (t) ?  ((s) == (t)) : !(*(s))   )

__DECL_PREFIX char *
__strchr(const char *string, const char *pEnd, int ch)
{
    if (pEnd == 0x00)
        return sc_strchr(string, ch);

    do
    {
        char szPtr[2];

        szPtr[0] = (char) ch;
        szPtr[1] = 0x00;

        while (string != pEnd && string[0] != szPtr[0])
            string++;

        if (string == pEnd)
            return 0x00;

        if (string[0] == szPtr[0])
            return ((char *) string);
    } while (0);

    return 0x00;
}

char *
__strichr(const char *string, const char *pEnd, int ch)
{

    char szPtr[2];

    szPtr[0] = (char) ch;
    szPtr[1] = 0x00;

    if (pEnd == 0x00)
    {
        while (*string && mdb_tolwr(string[0]) != mdb_tolwr(szPtr[0]))
            string++;

        if (mdb_tolwr(string[0]) == mdb_tolwr(szPtr[0]))
            return ((char *) string);
    }
    else
    {
        while (string != pEnd && mdb_tolwr(string[0]) != mdb_tolwr(szPtr[0]))
            string++;

        if (string == pEnd)
            return 0x00;

        if (mdb_tolwr(string[0]) == mdb_tolwr(szPtr[0]))
            return ((char *) string);
    }

    return 0x00;
}



DB_WCHAR *
__wcschr(const DB_WCHAR * string, const DB_WCHAR * pEnd, DB_WCHAR ch)
{
    DB_WCHAR *chPtr = &ch;

    if (pEnd == (DB_WCHAR *) L'\0')
        return sc_wcschr(string, ch);

    while (string != pEnd && sc_wcsncmp(string, chPtr, 1))
        string++;

    if (string == pEnd)
        return (DB_WCHAR *) L'\0';

    if (sc_wcsncmp(string, chPtr, 1) == 0)
        return ((DB_WCHAR *) string);

    return (DB_WCHAR *) L'\0';
}


DB_WCHAR *
__wstrichr(const DB_WCHAR * string, const DB_WCHAR * pEnd, DB_WCHAR ch)
{

    DB_WCHAR *chPtr = &ch;

    if (pEnd == (DB_WCHAR *) L'\0')
    {
        while (*string && sc_wcsncasecmp(string, chPtr, 1))
            string++;

        if (sc_wcsncasecmp(string, chPtr, 1) == 0)
            return ((DB_WCHAR *) string);
    }
    else
    {
        while (string != pEnd && sc_wcsncasecmp(string, chPtr, 1))
            string++;

        if (string == pEnd)
            return (DB_WCHAR *) L'\0';

        if (sc_wcsncasecmp(string, chPtr, 1) == 0)
            return ((DB_WCHAR *) string);
    }

    return (DB_WCHAR *) L'\0';
}

static void *
__like_strstr(char *str, char *likestr, char *pEnd, DB_BOOL IsNChar)
{
    if (pEnd == 0x00)
        return sc_strstr(str, likestr);
    else
    {
        char *cp = (char *) str;
        char *s1, *s2;

        if (!*likestr)
            return ((char *) str);
        while (!MC_IsEndString(cp, pEnd))
        {
            s1 = cp;
            s2 = (char *) likestr;

            while (!MC_IsEndString(s1, pEnd) && *s2 && (s1[0] == s2[0]))
                s1++, s2++;

            if (!*s2)
                return (cp);

            cp++;
        }

    }

    return (0x00);
}

static void *
__like_wcsstr(char *p_str, char *p_likestr, char *p_pEnd, DB_BOOL IsNChar)
{
    DB_WCHAR *str = (DB_WCHAR *) p_str;
    DB_WCHAR *likestr = (DB_WCHAR *) p_likestr;
    DB_WCHAR *pEnd = (DB_WCHAR *) p_pEnd;

    if (pEnd == 0x00)
        return sc_wcsstr(str, likestr);
    else
    {
        DB_WCHAR *cp = (DB_WCHAR *) str;
        DB_WCHAR *s1, *s2;


        if (!*likestr)
            return ((DB_WCHAR *) str);

        while (!MC_IsEndString(cp, pEnd))
        {
            s1 = cp;
            s2 = (DB_WCHAR *) likestr;

            while (!MC_IsEndString(s1, pEnd) && *s2
                    && (sc_wcsncmp(s1, s2, 1) == 0))
                s1++, s2++;

            if (!*s2)
                return (cp);

            cp++;
        }
    }

    return ((DB_WCHAR *) 0x00);
}

static void *
__like_strcasestr(char *str, char *likestr, char *pEnd, DB_BOOL IsNChar)
{
    if (pEnd == 0x00)
        return sc_strcasestr(str, likestr);
    else
    {
        char *cp = (char *) str;
        char *s1, *s2;

        if (!*likestr)
            return ((char *) str);

        while (!MC_IsEndString(cp, pEnd))
        {
            s1 = cp;
            s2 = (char *) likestr;

            while (!MC_IsEndString(s1, pEnd) && *s2 &&
                    mdb_tolwr(s1[0]) == mdb_tolwr(s2[0]))
                s1++, s2++;

            if (!*s2)
                return (cp);

            cp++;
        }
    }

    return (0x00);
}

static void *
__like_wcscasestr(char *p_str, char *p_likestr, char *p_pEnd, DB_BOOL IsNChar)
{
    DB_WCHAR *str = (DB_WCHAR *) p_str;
    DB_WCHAR *likestr = (DB_WCHAR *) p_likestr;
    DB_WCHAR *pEnd = (DB_WCHAR *) p_pEnd;

    if (pEnd == 0x00)
    {
        ;
    }
    else
    {
        DB_WCHAR *cp = (DB_WCHAR *) str;
        DB_WCHAR *s1, *s2;

        if (!*likestr)
            return ((DB_WCHAR *) str);

        while (!MC_IsEndString(cp, pEnd))
        {
            s1 = cp;
            s2 = (DB_WCHAR *) likestr;

            while (!MC_IsEndString(s1, pEnd) && *s2
                    && (sc_wcsncasecmp(s1, s2, 1) == 0))
                s1++, s2++;

            if (!*s2)
                return (cp);

            cp++;
        }
    }

    return ((DB_WCHAR *) 0x00);
}

static int
__like_strncmp(char *str, char *likestr, int len, char *pEnd, DB_BOOL IsNChar)
{
    int l2;

    if (pEnd == 0x00)
        return mdb_strncmp(str, likestr, len);

    l2 = pEnd - str;
    if (len <= l2)
        return mdb_strncmp(str, likestr, len);

    if ((len = mdb_strncmp(str, likestr, l2)) == 0)
        return -1;
    else
        return len;
}

static int
__like_wcsncmp(char *p_str, char *p_likestr, int len, char *p_pEnd,
        DB_BOOL IsNChar)
{
    int l2;
    DB_WCHAR *str = (DB_WCHAR *) p_str;
    DB_WCHAR *likestr = (DB_WCHAR *) p_likestr;
    DB_WCHAR *pEnd = (DB_WCHAR *) p_pEnd;

    if (pEnd == 0x00)
        return sc_wcsncmp(str, likestr, len);

    l2 = pEnd - str;
    if (len <= l2)
        return sc_wcsncmp(str, likestr, len);

    if ((len = sc_wcsncmp(str, likestr, l2)) == 0)
        return -1;
    else
        return len;
}

static int
__like_strncasecmp(char *str, char *likestr, int len, char *pEnd,
        DB_BOOL IsNChar)
{
    int l2;

    if (pEnd == 0x00)
        return mdb_strncasecmp(str, likestr, len);

    l2 = pEnd - str;
    if (len <= l2)
        return mdb_strncasecmp(str, likestr, len);

    len = mdb_strncasecmp(str, likestr, l2);
    if (len == 0)
        return -1;
    else
        return len;
}

static int
__like_wcsncasecmp(char *p_str, char *p_likestr, int len, char *p_pEnd,
        DB_BOOL IsNChar)
{
    int l2;
    DB_WCHAR *str = (DB_WCHAR *) p_str;
    DB_WCHAR *likestr = (DB_WCHAR *) p_likestr;
    DB_WCHAR *pEnd = (DB_WCHAR *) p_pEnd;

    if (pEnd == 0x00)
        return sc_wcsncasecmp(str, likestr, len);

    l2 = pEnd - str;
    if (len <= l2)
        return sc_wcsncasecmp(str, likestr, len);

    if ((len = sc_wcsncasecmp(str, likestr, l2)) == 0)
        return -1;
    else
        return len;
}

static void *
MC_like_strstr_user123(char *str, char *likestr, char *pEnd, DB_BOOL IsNChar,
        MDB_COL_TYPE coltype)
{
    char *cp = (char *) (str);
    char *s1, *s2;
    int incr_x;

    if (!*(likestr))
        return ((char *) (str));

    incr_x = (IsNChar) ? sizeof(DB_WCHAR) : sizeof(char);
    while (!MC_IsEndString(cp, pEnd))
    {
        s1 = cp;
        s2 = (char *) (likestr);
        while (!MC_IsEndString(s1, (pEnd)) && *s2 &&
                (sc_ncollate_func[(coltype)] (s1, s2, 1) == 0))
        {
            s1 += incr_x;
            s2 += incr_x;
        }

        if (!*s2)
            return (char *) (cp);

        cp += incr_x;
    }

    return NULL;
}

static void *
__like_strstr_user1(char *str, char *likestr, char *pEnd, DB_BOOL IsNChar)
{
    if (pEnd == 0x00)
        return (char *) sc_collate_strstr_func[MDB_COL_USER_TYPE1] (str,
                likestr);

    return MC_like_strstr_user123(str, likestr, pEnd, IsNChar,
            MDB_COL_USER_TYPE1);
}

static void *
__like_strstr_user2(char *str, char *likestr, char *pEnd, DB_BOOL IsNChar)
{
    if (pEnd == 0x00)
        return (char *) sc_collate_strstr_func[MDB_COL_USER_TYPE2] (str,
                likestr);

    return MC_like_strstr_user123(str, likestr, pEnd, IsNChar,
            MDB_COL_USER_TYPE2);
}

static void *
__like_strstr_user3(char *str, char *likestr, char *pEnd, DB_BOOL IsNChar)
{
    if (pEnd == 0x00)
        return (char *) sc_collate_strstr_func[MDB_COL_USER_TYPE3] (str,
                likestr);

    return MC_like_strstr_user123(str, likestr, pEnd, IsNChar,
            MDB_COL_USER_TYPE3);
}

static int
__like_strncmp_user1(char *str, char *likestr, int len, char *pEnd,
        DB_BOOL IsNChar)
{
    int l2;

    if (pEnd == 0x00)
        return sc_ncollate_func[MDB_COL_USER_TYPE1] (str, likestr, len);

    l2 = pEnd - str;
    if (len <= l2)
        return sc_ncollate_func[MDB_COL_USER_TYPE1] (str, likestr, len);

    if ((len = sc_ncollate_func[MDB_COL_USER_TYPE1] (str, likestr, l2)) == 0)
        return -1;
    else
        return len;
}

static int
__like_strncmp_user2(char *str, char *likestr, int len, char *pEnd,
        DB_BOOL IsNChar)
{
    int l2;

    if (pEnd == 0x00)
        return sc_ncollate_func[MDB_COL_USER_TYPE2] (str, likestr, len);

    l2 = pEnd - str;
    if (len <= l2)
        return sc_ncollate_func[MDB_COL_USER_TYPE2] (str, likestr, len);

    if ((len = sc_ncollate_func[MDB_COL_USER_TYPE2] (str, likestr, l2)) == 0)
        return -1;
    else
        return len;
}


static int
__like_strncmp_user3(char *str, char *likestr, int len, char *pEnd,
        DB_BOOL IsNChar)
{
    int l2;

    if (pEnd == 0x00)
        return sc_ncollate_func[MDB_COL_USER_TYPE3] (str, likestr, len);

    l2 = pEnd - str;
    if (len <= l2)
        return sc_ncollate_func[MDB_COL_USER_TYPE3] (str, likestr, len);

    if ((len = sc_ncollate_func[MDB_COL_USER_TYPE3] (str, likestr, l2)) == 0)
        return -1;
    else
        return len;
}

// definition struct
typedef struct _SLikeListTag SLikeList;
struct _SLikeListTag
{
    union
    {
        DB_WCHAR *_pWStr;
        char *_pStr;
    } u;

    int nLen;
    int sType;
    SLikeList *next;
};

typedef struct
{
    char *pnewlike;
    int revbuf_pos;
    short nLeastLen;
    short nwild;
    SLikeList *pList;
} SLikeInfo;

typedef struct
{
    union
    {
        DB_WCHAR *_pWvalue;
        char *_pvalue;
    } u;

    union
    {
        DB_WCHAR *_pWEnd;
        char *_pEnd;
    } ue;

    union
    {
        DB_WCHAR *_pWX;
        char *_pX;
    } ux;

    OID _nextOid;
} SDividedPart;

#define MC_GetPos_SLikeInfo(l,s) (_alignedlen((sizeof(int) + (((l)+1)*(s))) , sizeof(long)))

typedef struct
{
    MDB_COL_TYPE collation;
    int Is_ILIKE;
} SLikeOption;

#define MC_STRSTR(i, c, v, p, e )                                            \
  ( ((i) == 0) ? __collate_like_strstr_func[(c)]( (char*)(v), (char*)(p), (char*)(e), DB_FALSE ) :\
    __collate_like_strcasestr_func[(c)]( (char*)(v), (char*)(p), (char*)(e), DB_FALSE ) )
#define MC_NSTRSTR(i, c, v, p, e )                                           \
  ( ((i) == 0) ? __collate_like_strstr_func[(c)]( (char*)(v), (char*)(p), (char*)(e), DB_TRUE ) : \
    __collate_like_strcasestr_func[(c)]( (char*)(v), (char*)(p), (char*)(e), DB_TRUE ) )

#define MC_STRNCMP(i, c, v, p, l, e )                                        \
  (((i) == 0) ? __collate_like_strncmp_func[(c)]((char*)(v),(char*)(p),(l),(char*)(e),DB_FALSE ) :\
    __collate_like_strncasecmp_func[(c)]( (char*)(v), (char*)(p), (l), (char*)(e), DB_FALSE ) )

#define MC_NSTRNCMP(i, c, v, p, l, e)                                        \
  (((i) == 0) ? __collate_like_strncmp_func[(c)]((char*)(v),(char*)(p),(l),(char*)(e),DB_TRUE ) : \
    __collate_like_strncasecmp_func[(c)]( (char*)(v), (char*)(p), (l), (char*)(e), DB_TRUE ) )

/* ----------------------------------------------------------------------------------------
**  buffer에 escape 문자를 허용랄 것인가 예를 들면 
** \t, \n, \v , \b, \f, \r, \a, \\, \?, \', \", \OOO, \xhhhh. \0 등.
** \ooo 는  ooo를 8진수로 읽어라.
** \xhhhh는 hhhh를 16진수로 읽어라.
** 처리하는 것은 다음과 같다.
** \\, \%, \_ 
** 실제 문자 %, _를 찾길 원한다면 \%, \_로 나타내면 \은 와일드 캐릭터와 실제 문자를 구분하기 위한 
** escape sequence 이다. \는 \\로 나타낸다.
-------------------------------------------------------------------------------------------- */
DB_BOOL
__Calc_likeLength(char *plikestr, DB_BOOL bNChar, int *pLength, int *pCount)
{
    int percent, underbar, string;

    // rule 1) %%%%<str>%%%<str>%%%% ==> %<str>%<str>%
    // rule 2) <str>_%_<str>         ==> <str>__%<str>     
    *pLength = 0;
    *pCount = 0;

    if (bNChar)
    {
        DB_WCHAR *p = (DB_WCHAR *) plikestr;

        if (*p == L'_' || *p == L'%')
            string = 0;
        else
        {
            string = 1;
            *pCount = 1;
        }

        while (*p)
        {
            if (*p == L'_' || *p == L'%')
            {
                if (string)
                    string = 0;

                percent = underbar = 0;
                do
                {
                    if (*p == L'_')
                    {
                        underbar = 1;
                        (*pLength)++;
                    }
                    else
                        percent = 1;

                    p++;
                } while (*p == L'_' || *p == L'%');

                if (underbar)
                    (*pCount)++;

                if (percent)
                {
                    (*pCount)++;
                    (*pLength)++;
                }
            }
            else
            {
                if (*p == L'\\')
                {
                    if (*(p + 1) != L'%' && *(p + 1) != L'_'
                            && *(p + 1) != L'\\')
                        return FALSE;

                    p += 2;
                }
                else
                    p++;

                (*pLength) += 2;
                if (string == 0)
                {
                    (*pCount)++;
                    string = 1;
                }
            }
        }
    }
    else
    {
        char *p = plikestr;

        if (*p == '_' || *p == '%')
            string = 0;
        else
        {
            string = 1;
            *pCount = 1;
        }

        while (*p)
        {
            if (*p == '_' || *p == '%')
            {
                if (string)
                    string = 0;

                percent = underbar = 0;
                do
                {
                    if (*p == '_')
                    {
                        underbar = 1;
                        (*pLength)++;
                    }
                    else
                        percent = 1;

                    p++;
                } while (*p == '_' || *p == '%');

                if (underbar)
                    (*pCount)++;

                if (percent)
                {
                    (*pCount)++;
                    (*pLength)++;
                }
            }
            else
            {
                if (*p == '\\')
                {
                    if (*(p + 1) != '%' && *(p + 1) != '_' && *(p + 1) != '\\')
                        return FALSE;

                    p += 2;
                }
                else
                    p++;

                (*pLength) += 2;        //(*pLength)++;
                if (string == 0)
                {
                    (*pCount)++;
                    string = 1;
                }
            }
        }
    }

    return TRUE;
}

void
__preprocessing_likestr(SLikeInfo * pLikeInfo, char *psource, DB_BOOL bNChar,
        int mode)
{
    int flag, size;
    SLikeList *pTmp;

    pLikeInfo->nLeastLen = 0;
    size = sizeof(SLikeList);

    if (bNChar)
    {
        DB_WCHAR *pNew, *p;

        // STEP 1: make new string
        pNew = (DB_WCHAR *) pLikeInfo->pnewlike;
        p = (DB_WCHAR *) psource;

        while (*p)
        {
            if (*p == L'\\')
            {
                *pNew++ = *p++;
                *pNew++ = *p++;
                pLikeInfo->nLeastLen++;
            }
            else if (*p == L'_' || *p == L'%')
            {
                flag = 0;
                do
                {
                    if (*p == L'_')
                    {
                        *pNew++ = *p;
                        pLikeInfo->nLeastLen++;
                    }
                    else
                        flag = 1;
                    p++;
                } while (*p == L'_' || *p == L'%');

                if (flag)
                    *pNew++ = L'%';
            }
            else
            {
                if (mode == MDB_ILIKE)
                {
                    *pNew = GetSmallCaseLetterW(*p);    // convert small letter
                    pNew++;
                    p++;
                }
                else
                    *pNew++ = *p++;

                pLikeInfo->nLeastLen++;
            }
        }
        *pNew = 0x00;

        // STEP 2: split like sting       
        pLikeInfo->pList = (SLikeList *) (pLikeInfo + 1);
        pTmp = pLikeInfo->pList;
        sc_memset(pTmp, 0x00, size);
        p = (DB_WCHAR *) pLikeInfo->pnewlike;

        while (*p)
        {
            if (*p == L'%')
            {
                pTmp->sType = LIKE_PERCENT;
                *p = 0x00;
                p++;
            }
            else if (*p == L'_')
            {
                pTmp->sType = LIKE_UNDERBAR;
                *p = 0x00;
                pTmp->nLen = 0;
                do
                {
                    p++;
                    pTmp->nLen++;
                } while (*p == L'_');
            }
            else
            {
                pTmp->u._pWStr = p;
                pTmp->sType = LIKE_STRING;
                pTmp->nLen = 0;

                pNew = p;
                do
                {
                    if (*p == L'\\')
                    {
                        *pNew = *(p + 1);
                        p += 2;
                    }
                    else
                        *pNew = *p++;

                    pNew++;
                    pTmp->nLen++;

                } while (*p && *p != L'_' && *p != L'%');

                if (pNew != p)  // \%, \\, \_를 하나라도 만난경우.
                    *pNew = 0x00;
            }

            //
            if (*p != 0x00)
            {
                pTmp->next = pTmp + 1;
                pTmp = pTmp->next;
                sc_memset(pTmp, 0x00, size);
            }
        }
    }
    else    //if( bNChar )
    {
        char *pNew, *p;

        // STEP 1: make new string
        pNew = pLikeInfo->pnewlike;
        p = psource;
        while (*p)
        {
            if (*p == '\\')
            {
                *pNew++ = *p++;
                *pNew++ = *p++;
                pLikeInfo->nLeastLen++;
            }
            else if (*p == '_' || *p == '%')
            {
                flag = 0;
                do
                {
                    if (*p == '_')
                    {
                        *pNew++ = *p;
                        pLikeInfo->nLeastLen++;
                    }
                    else
                        flag = 1;
                    p++;
                } while (*p == '_' || *p == '%');

                if (flag)
                    *pNew++ = '%';
            }
            else
            {
                if (mode == MDB_ILIKE)
                {
                    *pNew = GetSmallCaseLetter(*p);     // convert small letter
                    pNew++;
                    p++;
                }
                else
                    *pNew++ = *p++;

                pLikeInfo->nLeastLen++;
            }
        }
        *pNew = 0x00;

        // STEP 2: split like sting       
        pLikeInfo->pList = (SLikeList *) (pLikeInfo + 1);
        pTmp = pLikeInfo->pList;
        sc_memset(pTmp, 0x00, size);
        p = pLikeInfo->pnewlike;

        while (*p)
        {
            if (*p == '%')
            {
                pTmp->sType = LIKE_PERCENT;
                *p = 0x00;
                p++;
            }
            else if (*p == '_')
            {
                pTmp->sType = LIKE_UNDERBAR;
                *p = 0x00;
                pTmp->nLen = 0;
                do
                {
                    p++;
                    pTmp->nLen++;
                } while (*p == '_');
            }
            else
            {
                pTmp->u._pStr = p;
                pTmp->sType = LIKE_STRING;
                pTmp->nLen = 0;

                pNew = p;
                do
                {
                    if (*p == '\\')
                    {
                        *pNew = *(p + 1);
                        p += 2;
                    }
                    else
                        *pNew = *p++;

                    pNew++;
                    pTmp->nLen++;

                } while (*p && *p != '_' && *p != '%');

                if (pNew != p)  // \%, \\, \_를 하나라도 만난경우.
                    *pNew = 0x00;
            }

            //
            if (*p != 0x00)
            {
                pTmp->next = pTmp + 1;
                pTmp = pTmp->next;
                sc_memset(pTmp, 0x00, size);
            }
        }
    }
}


  /*  -----------------------------------------------------------
   **** 아래와 같은 Automata를 정의할 수 있다.
   < string > := LIKE_STRING
   < % >      := LIKE_PERCENT
   < _ >      := LIKE_UNDERBAR
   \0         := NULL

   < start > ----------------> < string >
   |                   
   ------------> < % > LIKE_PERCENT
   |
   ------------> < _ > LIKE_UNDERBAR

   < string >-----------------------------------------------> \0 
   |                   
   ------------> < % > LIKE_PERCENT
   |
   ------------> < _ > LIKE_UNDERBAR

   < % > --------------------------------------------------> \0
   |
   ------------> < string > 
   < _ > --------------------------------------------------> \0 
   |                   
   ------------> < % >
   |
   ------------> < string >
   *** < % > 이후에 < _ >이 오는 경우는 없다.     
   ----------------------------------------------------------- */

/* ------------------------------------------------------------
** return  >= 0 : success
**        DB_E_INVALIDPARAM : likestr is 0x00 or empty
**        DB_E_INVALIDPARAM :  result param is invalid
**        DB_E_INVALIDLIKESYNTAX : syntax error
**        DB_E_OUTOFMEMORY : fail malloc() 
** notice  ppLikeInfo parameter는 초기화되어진 변수이어야 한다.     use memset()    
           return value가 0 이상인 경우 함수는 성공한것이다.
           이때 특별히, return value가  0인 경우는 실제 검색 할 필요가 없이 널이 아닌 모든 레코드가 채택되면 된다. 
           이러한 경우는 like '%' 인 경우 뿐이다.           
------------------------------------------------------------  */
int
dbi_prepare_like(char *likestr, DataType type, int mode, int nfixedbytelen,
        void **ppLikeInfo)
{
    int nLength, nCount;
    int newlen, pos;
    DB_BOOL bNChar;
    char *pNewMemory;
    SLikeInfo *pLikeInfo;

    /* support like '' */
    if (likestr == 0x00)
        return DB_E_INVALIDPARAM;

    if (ppLikeInfo == 0x00)
        return DB_E_INVALIDPARAM;

    if (*ppLikeInfo != 0x00)
    {
        mdb_free(*ppLikeInfo);
        *ppLikeInfo = 0x00;
    }

    if (nfixedbytelen < 0)
        nfixedbytelen = 0;
    else
    {
        nfixedbytelen += 2;     // nchar 대비용.
    }

    bNChar = IS_NBS(type) ? 1 : 0;

    /* support like '' */
    if ((bNChar && *(DB_WCHAR *) likestr == 0x0000) || (!bNChar
                    && *likestr == 0x00))
    {
        nLength = 0;
        pNewMemory = (char *) mdb_malloc(sizeof(int) + nfixedbytelen);
        if (pNewMemory == 0x00)
            return DB_E_OUTOFMEMORY;

        *ppLikeInfo = (void *) pNewMemory;
        sc_memcpy(pNewMemory, &nLength, sizeof(int));
        return 1;       // 0을 리턴하면 like '%'의 처리가 될 가능성 있음.
    }

    if (!__Calc_likeLength(likestr, bNChar, &nLength, &nCount))
        return DB_E_INVALIDLIKESYNTAX;

    pNewMemory = 0x00;

    if (bNChar)
    {
        newlen = sizeof(int)    /* 새로운 스트링의 길이 정보 */
                + (nLength + 1) * sizeof(DB_WCHAR)      /* 새로 생성될 스트링 길이 */
                + sizeof(SLikeInfo) + (nCount * sizeof(SLikeList));

        newlen += sizeof(long);

        pNewMemory = (char *) mdb_malloc(newlen + nfixedbytelen);
        if (pNewMemory == 0x00)
            return DB_E_OUTOFMEMORY;

        pos = MC_GetPos_SLikeInfo(nLength, sizeof(DB_WCHAR));
    }
    else
    {
        newlen = sizeof(int)    /* 새로운 스트링의 길이 정보 */
                + (nLength + 1) /* 새로 생성될 스트링 길이 */
                + sizeof(SLikeInfo) + (nCount * sizeof(SLikeList));

        newlen += sizeof(long);

        pNewMemory = (char *) mdb_malloc(newlen + nfixedbytelen);
        if (pNewMemory == 0x00)
            return DB_E_OUTOFMEMORY;

        pos = MC_GetPos_SLikeInfo(nLength, sizeof(char));
    }

    *ppLikeInfo = (void *) pNewMemory;
    sc_memcpy(pNewMemory, &nLength, sizeof(int));

    pLikeInfo = (SLikeInfo *) (pNewMemory + pos);
    sc_memset(pLikeInfo, 0x00, sizeof(SLikeInfo));

    pLikeInfo->revbuf_pos = newlen;

    pLikeInfo->pnewlike = pNewMemory + sizeof(int);

    __preprocessing_likestr(pLikeInfo, likestr, bNChar, mode);

    return pLikeInfo->nLeastLen;
}

int
_compare_like_rec(char *value, char *pEnd, SLikeList * pLike,
        SLikeOption * pOpt)
{
    char *pSrc;
    int i;
    SLikeList *pTmp;

    while (pLike)
    {
        if (pLike->sType == LIKE_PERCENT)
        {
            if (pLike->next == 0x00)
                return 0;

            pLike = pLike->next;
            // ppList->sType은 반드시 LIKE_STRING 이다.    
            pSrc = MC_STRSTR(pOpt->Is_ILIKE, pOpt->collation, value,
                    pLike->u._pStr, pEnd);
            if (pSrc == 0x00)
                return -1;

            value = pSrc + pLike->nLen;
            if (MC_IsEndString(value, pEnd))
            {
                for (pTmp = pLike->next; pTmp; pTmp = pTmp->next)
                {
                    if (pTmp->sType != LIKE_PERCENT)
                        break;
                }

                if (pTmp == 0x00)
                    return 0;
                else
                    return -1;
            }

            do
            {
                if (_compare_like_rec(value, pEnd, pLike->next, pOpt) == 0)
                    return 0;

                // why pSrc+1 ?  consider like %aa_c%, data aaabc
                value = pSrc + 1;

                pSrc = MC_STRSTR(pOpt->Is_ILIKE, pOpt->collation, value,
                        pLike->u._pStr, pEnd);
                if (pSrc == 0x00)
                    return -1;

                value = pSrc + pLike->nLen;
            } while (!MC_IsEndString(value, pEnd));

            for (pTmp = pLike->next; pTmp; pTmp = pTmp->next)
            {
                if (pTmp->sType != LIKE_PERCENT)
                    break;
            }

            if (pTmp == 0x00)
                return 0;
            else
                return -1;
        }
        else if (pLike->sType == LIKE_UNDERBAR)
        {
            //value += pLike->nLen;    
            value++;
            for (i = 1; i < pLike->nLen; i++)
            {
                if (MC_IsEndString(value, pEnd))
                    return -1;
                value++;
            }
        }
        else
        {
            if (MC_STRNCMP(pOpt->Is_ILIKE, pOpt->collation, value,
                            pLike->u._pStr, pLike->nLen, pEnd) == 0)
            {
                value += pLike->nLen;
            }
            else
                return -1;
        }

        pLike = pLike->next;
    }

    if (MC_IsEndString(value, pEnd))
        return 0;

    return -1;
}


int
_compare_likeN_rec(DB_WCHAR * value, DB_WCHAR * pEnd, SLikeList * pLike,
        SLikeOption * pOpt)
{
    DB_WCHAR *pSrc;
    int i;
    SLikeList *pTmp;

    while (pLike)
    {
        if (pLike->sType == LIKE_PERCENT)
        {
            if (pLike->next == 0x00)
                return 0;

            pLike = pLike->next;
            // ppList->sType은 반드시 LIKE_STRING 이다.    
            pSrc = MC_NSTRSTR(pOpt->Is_ILIKE, pOpt->collation, value,
                    pLike->u._pWStr, pEnd);
            if (pSrc == 0x00)
                return -1;

            value = pSrc + pLike->nLen;
            if (MC_IsEndString(value, pEnd))
            {
                for (pTmp = pLike->next; pTmp; pTmp = pTmp->next)
                {
                    if (pTmp->sType != LIKE_PERCENT)
                        break;
                }

                if (pTmp == 0x00)
                    return 0;
                else
                    return -1;
            }

            do
            {
                if (_compare_likeN_rec(value, pEnd, pLike->next, pOpt) == 0)
                    return 0;

                // why pSrc+1 ?  consider like %aa_c%, data aaabc
                value = pSrc + 1;

                pSrc = MC_NSTRSTR(pOpt->Is_ILIKE, pOpt->collation, value,
                        pLike->u._pWStr, pEnd);
                if (pSrc == 0x00)
                    return -1;

                value = pSrc + pLike->nLen;
            } while (!MC_IsEndString(value, pEnd));

            for (pTmp = pLike->next; pTmp; pTmp = pTmp->next)
            {
                if (pTmp->sType != LIKE_PERCENT)
                    break;
            }

            if (pTmp == 0x00)
                return 0;
            else
                return -1;
        }
        else if (pLike->sType == LIKE_UNDERBAR)
        {
            //value += pLike->nLen;    
            value++;
            for (i = 1; i < pLike->nLen; i++)
            {
                if (MC_IsEndString(value, pEnd))
                    return -1;
                value++;
            }
        }
        else
        {
            if (MC_NSTRNCMP(pOpt->Is_ILIKE, pOpt->collation, value,
                            pLike->u._pWStr, pLike->nLen, pEnd) == 0)
            {
                value += pLike->nLen;
            }
            else
                return -1;
        }

        pLike = pLike->next;
    }

    if (MC_IsEndString(value, pEnd))
        return 0;

    return -1;
}

#ifdef _64BIT
#define SEGMENT_NO_TYPE DB_INT64
#else
#define SEGMENT_NO_TYPE DB_INT32
#endif

int
_check_both_string(char **pvalue, int restLen, SDividedPart * pDivNextPart,
        SLikeList * pPat, SLikeOption * pOpt)
{
    char *pEnd;
    char *pNext, *pattern;

    pEnd = (*pvalue) + restLen;
    if (*pEnd == 0x00)
        pEnd = 0x00;
    pNext = pDivNextPart->u._pvalue;
    pattern = pPat->u._pStr;

    while (restLen > 0)
    {
        if (MC_STRNCMP(pOpt->Is_ILIKE, pOpt->collation, *pvalue,
                        pattern, restLen, pEnd) == 0)
        {
            if (MC_STRNCMP(pOpt->Is_ILIKE, pOpt->collation, pNext,
                            pattern + restLen, pPat->nLen - restLen,
                            pDivNextPart->ue._pEnd) == 0)
                return 0;
        }

        restLen--;
        (*pvalue)++;
    }

    return -1;
}

int
_check_both_Nstring(DB_WCHAR ** pvalue, int restLen,
        SDividedPart * pDivNextPart, SLikeList * pPat, SLikeOption * pOpt)
{
    DB_WCHAR *pEnd;
    DB_WCHAR *pNext, *pattern;

    pEnd = (*pvalue) + restLen;
    if (*pEnd == 0x00)
        pEnd = 0x00;

    pNext = pDivNextPart->u._pWvalue;
    pattern = pPat->u._pWStr;

    while (restLen > 0)
    {
        if (MC_NSTRNCMP(pOpt->Is_ILIKE, pOpt->collation, *pvalue,
                        pattern, restLen, pEnd) == 0)
        {
            if (MC_NSTRNCMP(pOpt->Is_ILIKE, pOpt->collation, pNext,
                            pattern + restLen, pPat->nLen - restLen,
                            pDivNextPart->ue._pWEnd) == 0)
            {
                return 0;
            }
        }

        restLen--;
        (*pvalue)++;
    }

    return -1;
}

#define MC_Lock_EXT_PART(issys, nextOid, DivNextPart,length )  {       \
    SetBufSegment(issys1, (nextOid));                                         \
    (DivNextPart)._nextOid = NULL_OID;                                        \
    DBI_GET_EXTENDPART((nextOid), (DivNextPart).u._pvalue, (length));         \
    (DivNextPart).ue._pEnd = 0x00; /*(DivNextPart).u._pvalue + length;*/      \
}

#define MC_Release_EXT_PART(issys)     { UnsetBufSegment((issys));  }

int
_compare_like_divided(SDividedPart * pDivCur, SLikeList * pLike,
        SLikeOption * pOpt)
{
    int remainder, length;
    SDividedPart DivNextPart;
    SLikeList sList;

    while (pLike)
    {
        if (pLike->sType == LIKE_PERCENT)
        {
            if (pLike->next == 0x00)
                return 0;
            else
                pLike = pLike->next;
            //pList->sType은 반드시 LIKE_STRING 이다.                

            if (pDivCur->ue._pEnd)
                remainder = pDivCur->ue._pEnd - pDivCur->u._pvalue;
            else
            {
                remainder = pDivCur->ux._pX - pDivCur->u._pvalue;
            }

            if (pLike->nLen <= remainder)
            {
                char *pSrc;

                if (pDivCur->_nextOid == NULL_OID)
                {
                    do
                    {
                        pSrc = MC_STRSTR(pOpt->Is_ILIKE, pOpt->collation,
                                pDivCur->u._pvalue, pLike->u._pStr,
                                pDivCur->ue._pEnd);
                        if (pSrc == 0x00)
                            return -1;

                        pDivCur->u._pvalue = pSrc + pLike->nLen;
                        if (_compare_like_divided(pDivCur, pLike->next,
                                        pOpt) == 0)
                        {
                            return 0;
                        }

                        // why pSrc+1 ?  consider like %aa_c%, data aaabc
                        pDivCur->u._pvalue = pSrc + 1;

                    } while (!MC_IsEndString(pDivCur->u._pvalue,
                                    pDivCur->ue._pEnd)
                            /*&& pDivCur->_nextOid == NULL_OID */ );

                    return -1;
                }
                else
                {       //-----------------------------------------------------------
                    do
                    {
                        pSrc = MC_STRSTR(pOpt->Is_ILIKE, pOpt->collation,
                                pDivCur->u._pvalue, pLike->u._pStr,
                                pDivCur->ue._pEnd);
                        if (pSrc == 0x00)
                        {
                            pDivCur->u._pvalue +=
                                    ((remainder - pLike->nLen) + 1);
                            sc_memset(&sList, 0x00, sizeof(SLikeList));
                            sList.sType = LIKE_PERCENT;
                            sList.next = pLike;
                            return _compare_like_divided(pDivCur, &sList,
                                    pOpt);
                        }

                        pDivCur->u._pvalue = pSrc + pLike->nLen;
                        if (_compare_like_divided(pDivCur, pLike->next,
                                        pOpt) == 0)
                        {
                            return 0;
                        }

                        // why pSrc+1 ?  consider like %aa_c%, data aaabc
                        pDivCur->u._pvalue = pSrc + 1;
                    } while (!MC_IsEndString(pDivCur->u._pvalue,
                                    pDivCur->ue._pEnd)
                            /*&& pDivCur->_nextOid == NULL_OID */ );
                    if (pDivCur->_nextOid != NULL_OID)
                    {
                        sc_memset(&sList, 0x00, sizeof(SLikeList));
                        sList.sType = LIKE_PERCENT;
                        sList.next = pLike;
                        return _compare_like_divided(pDivCur, &sList, pOpt);
                    }
                    else
                        return -1;
                }
            }   // if( pLike->nLen <= remainder )           
            else
            {
                /*************************************************************
                **  남아있는 스트링의 길이보다 패턴의 길이가 더 길다.                
                ******************************************************* */
                if (pDivCur->_nextOid == NULL_OID)
                {
                    return -1;
                }
                else
                {
                    int issys1 = 0;

                    //----------------------
                    MC_Lock_EXT_PART(issys1, pDivCur->_nextOid,
                            DivNextPart, length);

                    DivNextPart.ux._pX = DivNextPart.u._pvalue + (length - 1);

                    do
                    {
                        // check both                         
                        if (_check_both_string(&(pDivCur->u._pvalue),
                                        remainder, &DivNextPart, pLike,
                                        pOpt) != 0)
                        {
                            break;
                        }

                        if (pDivCur->ue._pEnd)
                            remainder = pDivCur->ue._pEnd - pDivCur->u._pvalue;
                        else
                        {
                            remainder = pDivCur->ux._pX - pDivCur->u._pvalue;
                        }

                        DivNextPart.u._pvalue += (pLike->nLen - remainder);

                        if (_compare_like_divided(&DivNextPart, pLike->next,
                                        pOpt) == 0)
                        {
                            //----------------------
                            MC_Release_EXT_PART(issys1);
                            return 0;
                        }

                        remainder--;
                        pDivCur->u._pvalue++;
                    } while (remainder > 0);

                    sc_memset(&sList, 0x00, sizeof(SLikeList));
                    sList.sType = LIKE_PERCENT;
                    sList.next = pLike;
                    length = _compare_like_divided(&DivNextPart, &sList, pOpt);
                    //----------------------
                    MC_Release_EXT_PART(issys1);

                    return length;
                }
            }
        }   // if( pLike->sType == LIKE_PERCENT )
        else
        {
            /***************************************************************
            **  Underbar 또는 string을 처리 함.
            **  여기서는 반드시 일치되는 상태이어야 한다.
            ***************************************************************/
            if (pDivCur->ue._pEnd)
                remainder = pDivCur->ue._pEnd - pDivCur->u._pvalue;
            else
            {
                remainder = pDivCur->ux._pX - pDivCur->u._pvalue;
            }

            if (pLike->nLen <= remainder)
            {   // CASE: Length(pattern) <= Length(string)
                if (pLike->sType == LIKE_UNDERBAR)
                    pDivCur->u._pvalue += pLike->nLen;
                else
                {
                    if (MC_STRNCMP(pOpt->Is_ILIKE, pOpt->collation,
                                    pDivCur->u._pvalue, pLike->u._pStr,
                                    pLike->nLen, pDivCur->ue._pEnd) == 0)
                    {
                        pDivCur->u._pvalue += pLike->nLen;
                    }
                    else
                        return -1;
                }
            }   //if( pLike->nLen <= remainder )
            else
            {
                if (pDivCur->_nextOid == NULL_OID)
                {       // CASE: Length(pattern) > Length(string) 
                    // and no more string-block
                    return -1;
                }
                else
                {       // CASE: Length(pattern) > Length(string)  
                    // and more string-block
                    int issys1 = 0;

                    if (pLike->sType == LIKE_UNDERBAR)
                        pDivCur->u._pvalue += remainder;
                    else
                    {
                        if (MC_STRNCMP(pOpt->Is_ILIKE, pOpt->collation,
                                        pDivCur->u._pvalue, pLike->u._pStr,
                                        remainder, pDivCur->ue._pEnd) == 0)
                        {
                            pDivCur->u._pvalue += remainder;    //pLike->nLen;            
                        }
                        else
                            return -1;

                    }

                    sc_memcpy(&sList, pLike, sizeof(SLikeList));
                    sList.nLen -= remainder;
                    sList.u._pStr += remainder;

                    //----------------------
                    MC_Lock_EXT_PART(issys1, pDivCur->_nextOid,
                            DivNextPart, length);

                    DivNextPart.ux._pX = DivNextPart.u._pvalue + (length - 1);

                    length = _compare_like_divided(&DivNextPart, &sList, pOpt);
                    //----------------------
                    MC_Release_EXT_PART(issys1);

                    return length;
                }
            }
        }

        pLike = pLike->next;
    }

    if (MC_IsEndString(pDivCur->u._pvalue, pDivCur->ue._pEnd) &&
            pDivCur->_nextOid == NULL_OID)
    {
        return 0;
    }

    return -1;
}


int
_compare_likeN_divided(SDividedPart * pDivCur, SLikeList * pLike,
        SLikeOption * pOpt)
{
    int remainder, length;
    SDividedPart DivNextPart;
    SLikeList sList;

    while (pLike)
    {
        if (pLike->sType == LIKE_PERCENT)
        {
            if (pLike->next == 0x00)
                return 0;
            else
                pLike = pLike->next;
            //pList->sType은 반드시 LIKE_STRING 이다.                

            if (pDivCur->ue._pEnd)
                remainder = pDivCur->ue._pWEnd - pDivCur->u._pWvalue;
            else
            {
                remainder = pDivCur->ux._pWX - pDivCur->u._pWvalue;
            }

            if (pLike->nLen <= remainder)
            {
                DB_WCHAR *pSrc;

                if (pDivCur->_nextOid == NULL_OID)
                {
                    do
                    {
                        pSrc = MC_NSTRSTR(pOpt->Is_ILIKE, pOpt->collation,
                                pDivCur->u._pWvalue, pLike->u._pWStr,
                                pDivCur->ue._pWEnd);
                        if (pSrc == 0x00)
                            return -1;

                        pDivCur->u._pWvalue = pSrc + pLike->nLen;
                        if (_compare_likeN_divided(pDivCur, pLike->next,
                                        pOpt) == 0)
                        {
                            return 0;
                        }

                        // why pSrc+1 ?  consider like %aa_c%, data aaabc
                        pDivCur->u._pWvalue = pSrc + 1;

                    } while (!MC_IsEndString(pDivCur->u._pWvalue,
                                    pDivCur->ue._pWEnd)
                            /*&& pDivCur->_nextOid == NULL_OID */ );
                    return -1;
                }
                else
                {       //-----------------------------------------------------------
                    do
                    {
                        pSrc = MC_NSTRSTR(pOpt->Is_ILIKE, pOpt->collation,
                                pDivCur->u._pWvalue, pLike->u._pWStr,
                                pDivCur->ue._pWEnd);
                        if (pSrc == 0x00)
                        {
                            pDivCur->u._pWvalue +=
                                    ((remainder - pLike->nLen) + 1);

                            sc_memset(&sList, 0x00, sizeof(SLikeList));
                            sList.sType = LIKE_PERCENT;
                            sList.next = pLike;
                            return _compare_likeN_divided(pDivCur, &sList,
                                    pOpt);
                        }

                        pDivCur->u._pWvalue = pSrc + pLike->nLen;
                        if (_compare_likeN_divided(pDivCur, pLike->next,
                                        pOpt) == 0)
                        {
                            return 0;
                        }

                        pDivCur->u._pWvalue = pSrc + 1; // why pSrc+1 ?  consider like %aa_c%, data aaabc                                        
                    } while (!MC_IsEndString(pDivCur->u._pWvalue,
                                    pDivCur->ue.
                                    _pWEnd)
                            /*&& pDivCur->_nextOid == NULL_OID */ );
                    if (pDivCur->_nextOid != NULL_OID)
                    {
                        sc_memset(&sList, 0x00, sizeof(SLikeList));
                        sList.sType = LIKE_PERCENT;
                        sList.next = pLike;
                        return _compare_likeN_divided(pDivCur, &sList, pOpt);
                    }
                    else
                        return -1;
                }
            }
            else
            {
                /***************************************************************************************
                **  남아있는 스트링의 길이보다 패턴의 길이가 더 길다.                
                ************************************************************************************** */
                if (pDivCur->_nextOid == NULL_OID)
                {
                    return -1;
                }
                else
                {
                    int issys1 = 0;

                    //----------------------
                    MC_Lock_EXT_PART(issys1, pDivCur->_nextOid, DivNextPart,
                            length);

                    DivNextPart.ux._pWX =
                            DivNextPart.u._pWvalue +
                            (length / sizeof(DB_WCHAR) - 1);

                    do
                    {
                        // check both                         
                        if (_check_both_Nstring(&(pDivCur->u._pWvalue),
                                        remainder, &DivNextPart, pLike,
                                        pOpt) != 0)
                        {
                            break;
                        }

                        if (pDivCur->ue._pEnd)
                        {
                            remainder =
                                    pDivCur->ue._pWEnd - pDivCur->u._pWvalue;
                        }
                        else
                        {
                            remainder = pDivCur->ux._pWX - pDivCur->u._pWvalue;
                        }

                        DivNextPart.u._pWvalue += (pLike->nLen - remainder);

                        if (_compare_likeN_divided(&DivNextPart, pLike->next,
                                        pOpt) == 0)
                        {
                            //----------------------
                            MC_Release_EXT_PART(issys1);
                            return 0;
                        }

                        remainder--;
                        pDivCur->u._pWvalue++;
                    } while (remainder > 0);

                    sc_memset(&sList, 0x00, sizeof(SLikeList));
                    sList.sType = LIKE_PERCENT;
                    sList.next = pLike;
                    length = _compare_likeN_divided(&DivNextPart, &sList,
                            pOpt);
                    //----------------------
                    MC_Release_EXT_PART(issys1);

                    return length;
                }
            }
        }
        else
        {
            /***************************************************************************************
            **  Underbar 또는 string을 처리 함.
            **  여기서는 반드시 일치되는 상태이어야 한다.
            ************************************************************************************** */
            if (pDivCur->ue._pEnd)
            {
                remainder = pDivCur->ue._pWEnd - pDivCur->u._pWvalue;
            }
            else
            {
                remainder = pDivCur->ux._pWX - pDivCur->u._pWvalue;
            }

            if (pLike->nLen <= remainder)
            {   // CASE: Length(pattern) <= Length(string)
                if (pLike->sType == LIKE_UNDERBAR)
                    pDivCur->u._pWvalue += pLike->nLen;
                else
                {
                    if (MC_NSTRNCMP(pOpt->Is_ILIKE, pOpt->collation,
                                    pDivCur->u._pWvalue, pLike->u._pWStr,
                                    pLike->nLen, pDivCur->ue._pWEnd) == 0)
                    {
                        pDivCur->u._pWvalue += pLike->nLen;
                    }
                    else
                        return -1;
                }
            }
            else
            {
                if (pDivCur->_nextOid == NULL_OID)
                {       // CASE: Length(pattern) > Length(string)  and no more string-block
                    return -1;
                }
                else
                {       // CASE: Length(pattern) > Length(string)  and more string-block
                    int issys1 = 0;

                    if (pLike->sType == LIKE_UNDERBAR)
                        pDivCur->u._pWvalue += remainder;
                    else
                    {
                        if (MC_NSTRNCMP(pOpt->Is_ILIKE, pOpt->collation,
                                        pDivCur->u._pWvalue, pLike->u._pWStr,
                                        remainder, pDivCur->ue._pWEnd) == 0)
                        {
                            pDivCur->u._pWvalue += remainder;   //pLike->nLen;            
                        }
                        else
                            return -1;

                    }

                    sc_memcpy(&sList, pLike, sizeof(SLikeList));
                    sList.nLen -= remainder;
                    sList.u._pWStr += remainder;

                    //----------------------
                    MC_Lock_EXT_PART(issys1, pDivCur->_nextOid, DivNextPart,
                            length);

                    DivNextPart.ux._pWX =
                            DivNextPart.u._pWvalue +
                            (length / sizeof(DB_WCHAR) - 1);
                    length = _compare_likeN_divided(&DivNextPart, &sList,
                            pOpt);
                    //----------------------
                    MC_Release_EXT_PART(issys1);

                    return length;
                }
            }
        }

        pLike = pLike->next;
    }

    if (MC_IsEndString(pDivCur->u._pWvalue, pDivCur->ue._pWEnd)
            && pDivCur->_nextOid == NULL_OID)
        return 0;

    return -1;
}


int
dbi_like_compare(char *value_, DataType type, int fixed_part,
        void *pLikeVal, int mode, MDB_COL_TYPE collation)
{
    int pos;
    SLikeInfo *pLikeInfo;
    char *within;
    int within_blen, length;
    char *t_value;
    unsigned char ch1_bk, ch2_bk;

    SLikeOption sLikeOpt;

    sLikeOpt.collation = collation;
    sLikeOpt.Is_ILIKE = (mode == MDB_ILIKE);

    sc_memcpy(&length, pLikeVal, sizeof(int));

    if (collation == MDB_COL_CHAR_REVERSE)
    {       // only support char(n) 
#ifdef MDB_DEBUG
        if (IS_NBS(type))
            sc_assert(0, __FILE__, __LINE__);

        if (IS_VARIABLE_DATA(type, fixed_part))
            sc_assert(0, __FILE__, __LINE__);
#endif

        if (length == 0)
        {
            return ((mdb_strrlen(value_) > 0) ? -1 : 0);
            // ((*value_ != 0x00) ? -1 :  0);                          
        }
        else
        {
            SLikeList *pls;

            pos = MC_GetPos_SLikeInfo(length, sizeof(char));
            pLikeInfo = (SLikeInfo *) (((char *) pLikeVal) + pos);

            /* customize '%str' */
            pls = pLikeInfo->pList;
            if (pls != 0x00 && pls->sType == LIKE_PERCENT &&
                    pls->next != 0x00 && pls->next->next == 0x00 &&
                    pls->next->sType == LIKE_STRING)
            {

                pls = pls->next;
                if (mdb_strnrcmp(value_, pls->u._pStr, pls->nLen) == 0)
                {
                    return 0;
                }
                else
                {
                    return -1;
                }
            }

            t_value = mdb_strrcpy(((char *) pLikeVal) + pLikeInfo->revbuf_pos,
                    value_);
        }
    }
    else
    {
        t_value = value_;
    }

    /* support like '' */
    if (length == 0)
    {       // like ''
        if (IS_NBS(type))
        {
            if (!IS_VARIABLE_DATA(type, fixed_part))
            {   // value is a complete string 
                if (*(DB_WCHAR *) t_value != 0x0000)
                {
                    return -1;
                }
            }
            else
            {
                SDividedPart DivCurPart;

                /* value is an incomplete string. */
                DBI_GET_FIXEDPART(t_value, fixed_part, within, within_blen,
                        DivCurPart._nextOid);
                if (*(DB_WCHAR *) within != 0x0000)
                {
                    return -1;
                }
            }
        }
        else
        {
            if (!IS_VARIABLE_DATA(type, fixed_part))
            {   // value is a complete string 
                if (*t_value != 0x00)
                {
                    return -1;
                }
            }
            else
            {
                SDividedPart DivCurPart;

                /* value is an incomplete string. */
                DBI_GET_FIXEDPART(t_value, fixed_part, within, within_blen,
                        DivCurPart._nextOid);
                if (*within != 0x00)
                {
                    return -1;
                }
            }
        }

        return 0;
    }
    /* End of  support like '' */

    if (IS_NBS(type))
    {
        pos = MC_GetPos_SLikeInfo(length, sizeof(DB_WCHAR));

        pLikeInfo = (SLikeInfo *) (((char *) pLikeVal) + pos);
        if (pLikeInfo->nLeastLen == 0)
        {
            return 0;   // 무조건 참이다. '%' 이런 형태의 likevalue 였다.
        }

        if (!IS_VARIABLE_DATA(type, fixed_part))
        {   // value is a complete string 
            return _compare_likeN_rec((DB_WCHAR *) t_value, 0x00,
                    pLikeInfo->pList, &sLikeOpt);
        }
        else
        {
            SDividedPart DivCurPart;

            /* value is an incomplete string. */
            DBI_GET_FIXEDPART(t_value, fixed_part, within, within_blen,
                    DivCurPart._nextOid);

            DivCurPart.ux._pWX = ((DB_WCHAR *) within) +
                    (within_blen / sizeof(DB_WCHAR));

            ch1_bk = within[within_blen];
            ch2_bk = within[within_blen + 1];
            within[within_blen] = 0x00;
            within[within_blen + 1] = 0x00;
            if (DivCurPart._nextOid == 0x00)
            {
                pos = _compare_likeN_rec((DB_WCHAR *) within, 0x00,
                        pLikeInfo->pList, &sLikeOpt);
            }
            else
            {
                DivCurPart.u._pvalue = within;
                DivCurPart.ue._pEnd = 0x00;
                pos = _compare_likeN_divided(&DivCurPart, pLikeInfo->pList,
                        &sLikeOpt);
            }
            within[within_blen] = ch1_bk;
            within[within_blen + 1] = ch2_bk;

            return pos;
        }
    }
    else
    {
        pos = MC_GetPos_SLikeInfo(length, sizeof(char));

        pLikeInfo = (SLikeInfo *) (((char *) pLikeVal) + pos);
        if (pLikeInfo->nLeastLen == 0)
        {
            return 0;   // 무조건 참이다. '%' 이런 형태의 likevalue 였다.
        }

        if (!IS_VARIABLE_DATA(type, fixed_part))
        {   // value is a complete string              
            return _compare_like_rec(t_value, 0x00, pLikeInfo->pList,
                    &sLikeOpt);
        }
        else
        {
            SDividedPart DivCurPart;

            /* value is an incomplete string. */
            DBI_GET_FIXEDPART(t_value, fixed_part, within, within_blen,
                    DivCurPart._nextOid);

            DivCurPart.ux._pX = within + within_blen;

            ch1_bk = within[within_blen];
            within[within_blen] = 0x00;

            if (DivCurPart._nextOid == 0x00)
            {
                pos = _compare_like_rec(within, 0x00, pLikeInfo->pList,
                        &sLikeOpt);
            }
            else
            {
                DivCurPart.u._pvalue = within;
                DivCurPart.ue._pEnd = 0x00;
                pos = _compare_like_divided(&DivCurPart, pLikeInfo->pList,
                        &sLikeOpt);
            }
            within[within_blen] = ch1_bk;

            return pos;
        }
    }

    //return 0;
}
