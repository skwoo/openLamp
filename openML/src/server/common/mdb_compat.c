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

/* errno */
#ifdef USE_ERROR
__DECL_PREFIX int errno;
#endif

#include "mdb_syslog.h"
#include "mdb_inner_define.h"
#include "mdb_compat.h"
#include "db_api.h"
#include "ErrorCode.h"
#include "mdb_Server.h"

#define TERM_YEARTIME    (50)
#define NUM_YEARTIME    (10000 / TERM_YEARTIME)
__DECL_PREFIX struct
{
    DB_INT64 time_;
    int year_;
} _yeartime[NUM_YEARTIME];

__DECL_PREFIX int _yeartime1951 = 0;
__DECL_PREFIX int _yeartime2001 = 0;

static int f_time_init = 0;
int _timegapTZ = -1;
static unsigned char daysofmonth[12] =
        { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static unsigned short sum_daysofmonth[12] =
        { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

static int setTimeGapTZ(void);
static int daynumofyear(int year, int month, int day);

static int _time2tm(const DB_INT64 * p_clock, struct db_tm *res, int flag);
static int getyear_yeartime(DB_INT64 time_, DB_INT64 * ytime, int flag);

/*****************************************************************************/
//! _time_init 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void
_time_init(void)
{
    int i, year;
    struct db_tm _tm;

    if (f_time_init == 1)
        return;

    if (_timegapTZ == -1)
        setTimeGapTZ();

    _tm.tm_mon = 0;
    _tm.tm_mday = 1;
    _tm.tm_hour = 0;
    _tm.tm_min = 0;
    _tm.tm_sec = 0;

    year = 1;

    i = 0;
    while (1)
    {
        if (i >= NUM_YEARTIME)
            break;

        _tm.tm_year = year - 1900;

        _yeartime[i].year_ = year;
        _yeartime[i].time_ = _tm2time(&_tm);

        if (year == 1951)
            _yeartime1951 = i;
        else if (year == 2001)
            _yeartime2001 = i;

        year += TERM_YEARTIME;
        i++;
    }

    f_time_init = 1;

    return;
}

/*****************************************************************************/
//! getyear_yeartime 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param time_ :
 * \param ytime : 
 * \param flag    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
getyear_yeartime(DB_INT64 time_, DB_INT64 * ytime, int flag)
{
    int i;
    MDB_INT64 diff;

#if (TERM_YEARTIME == 50)
    if (time_ == 0)
    {
        *ytime = _yeartime[_yeartime1951].time_;

        return _yeartime[_yeartime1951].year_;
    }
#endif


    /* binary search */
    if (time_ >= _yeartime[NUM_YEARTIME - 1].time_)
    {
        i = NUM_YEARTIME - 1;
        goto end;
    }
    else
    {
        int f, l, m;
        int f_start;

        if (time_ >= _yeartime[_yeartime2001].time_)
            f = f_start = _yeartime2001;
        else if (time_ >= 0)
            f = f_start = _yeartime1951;
        else
            f = f_start = 0;
        l = NUM_YEARTIME - 1;

        while (1)
        {
            m = (f + l) >> 1;

            if (m == f_start || m == NUM_YEARTIME - 1)
            {
                i = m;
                goto end;
            }

            diff = time_ - _yeartime[m].time_;

            if (diff < 0)
                l = m - 1;
            else if (diff == 0)
            {
                i = m;
                goto end;
            }
            else
            {
                if (time_ < _yeartime[m + 1].time_)
                {
                    i = m;
                    goto end;
                }
                else
                    f = m + 1;
            }
        }
    }
    /* End of  binary search */

  end:
    *ytime = _yeartime[i].time_;

    return _yeartime[i].year_;
}

extern __DECL_PREFIX char mobile_lite_config[256];

/* dbname에 path가 포함되어 있는지 점검하고
   path가 포함되어 있다면 마지막 db 이름을 제외한 path를
   mobile_lite_config 값으로, 뒤쪽을 dbfilename으로 설정한다.
   없으면 dbname 전체를 dbfilename으로 copy.
*/

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
__DECL_PREFIX int
__check_dbname(char *dbname, char *dbfilename)
{
    int i;

    if (dbname == NULL || dbname[0] == 0 || dbfilename == NULL)
        return DB_E_INVALIDDBPATH;

    i = sc_strlen(dbname) - 1;
    if (dbname[i] == DIR_DELIM_CHAR)
    {
        // no DB name in dbname
        return DB_E_INVALIDDBPATH;
    }

    while (1)
    {
        if (i < 0)
        {
            /* dbname이 root부터의 path를 포함하지 않는 경우
               mobile_lite_config를 . 로 변경.
               단, linux와 sun은 환경변수인 MOBILE_LITE_CONFIG 설정값을 이용 */
            if (sc_getenv("MOBILE_LITE_CONFIG") != NULL)
            {
                if (mobile_lite_config[0] == '\0')
                    sc_strcpy(mobile_lite_config,
                            sc_getenv("MOBILE_LITE_CONFIG"));
            }
            else
            {
                mobile_lite_config[0] = '.';
                mobile_lite_config[1] = '\0';
            }
            sc_strcpy(dbfilename, dbname);
            break;
        }

        if (dbname[i] == DIR_DELIM_CHAR)
        {   /* path 포함 */

            sc_strcpy(dbfilename, dbname + i + 1);

            if (dbname[0] == DIR_DELIM_CHAR)
            {   /* root 부터 path 있는 경우
                   mobile_lite_config와 dbfilename 분리 */
                if (i == 0)
                    sc_strcpy(mobile_lite_config, DIR_DELIM);
                else
                {
                    sc_strncpy(mobile_lite_config, dbname, i);
                    mobile_lite_config[i] = '\0';
                }
            }
            else
            {
#if defined(linux) || defined(sun)
#if defined(ANDROID_OS)
                if (i == 0)
                    sc_strcpy(mobile_lite_config, DIR_DELIM);
                else
                {
                    sc_strncpy(mobile_lite_config, dbname, i);
                    mobile_lite_config[i] = '\0';
                }
#endif
#else
                if (i == 0)
                    sc_strcpy(mobile_lite_config, DIR_DELIM);
                else
                {
                    sc_strncpy(mobile_lite_config, dbname, i);
                    mobile_lite_config[i] = '\0';
                }
#endif
            }

            break;
        }
        i--;
    }

    return 0;
}

/*****************************************************************************/
//! _tm2time 
/*! \breif        convert struct db_tm to db_time_t
 ************************************
 * \param st(in)        : tm 구조체
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *        - time은 1970년부터 시작된다
 *****************************************************************************/
DB_INT64
_tm2time(struct db_tm * st)
{
    int syear, eyear;
    DB_INT64 days;
    register int i;
    DB_INT64 resTime;
    int year, month, day;
    register int numLeapYear;

    year = st->tm_year + 1900;
    month = st->tm_mon + 1;
    day = st->tm_mday;

    if (year < 1970)
    {
        syear = year + 1;
        eyear = 1969;

        /* 시작하는 날짜에서 연말까지 계산 */
        if (isLeapYear(year))
            days = 366 - daynumofyear(year, month, day);
        else
            days = 365 - daynumofyear(year, month, day);

        /* 현재시간부터 연말까지 시간이니까 현재날짜까지 시간을
           구한 다음에 현재 시간을 빼야함 */
        resTime = (days + 1) * 24 * 3600 -
                (st->tm_hour * 3600 + st->tm_min * 60 + st->tm_sec);
    }
    else
    {
        syear = 1970;
        eyear = year - 1;

        /* 마지막 날짜에서 연초부터 계산 */
        days = daynumofyear(year, month, day);

        resTime = (days - 1) * 24 * 3600 + st->tm_hour * 3600 +
                st->tm_min * 60 + st->tm_sec;
    }

    days = 0;
    numLeapYear = 0;

    /* 시작연도 다음해부터 끝연도 전해까지 계산 */
    i = syear;
    while (eyear - i >= 400)
    {
        if (isLeapYear(i))
            numLeapYear += 98;
        else
            numLeapYear += 97;

        i += 401;
    }

    for (; i <= eyear; i++)
    {
        if (isLeapYear(i))
        {
            numLeapYear++;
            if (i + 2 <= eyear)
                i += 2;
        }
    }

    days = (eyear - syear + 1) * 365 + numLeapYear;
    resTime += days * 24 * 3600;

    if (year < 1970)
        return resTime * (-1);
    else
        return resTime;
}

/*****************************************************************************/
//! daynumofyear 
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
/* 연도에서 날짜가 해당되는 날 수. 1월1일은 1 */
static int
daynumofyear(int year, int month, int day)
{
    int daynum;
    int leapyear = isLeapYear(year);

    daynum = sum_daysofmonth[month - 1] + day;
    if (month > 2 && leapyear)
        daynum++;

    return daynum;
}

/*****************************************************************************/
//! _time2tm 
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
 *  from db_time_t to struct db_tm 
 *  flag = 0 *clock is UTC tme, 1  local time 
 *****************************************************************************/
static int
_time2tm(const DB_INT64 * p_clock, struct db_tm *res, int flag)
{
    int dyear, dmon, dday, dhour, dmin, dsec;
    DB_INT64 l_clock = *p_clock;

    int myear;
    DB_INT64 mtime;

    myear = getyear_yeartime(l_clock, &mtime, flag);

    l_clock -= mtime;

    dsec = (int) (l_clock % 60);        /* sec */
    res->tm_sec = dsec;

    l_clock /= 60;
    dmin = (int) (l_clock % 60);
    res->tm_min = dmin; /* min */

    l_clock /= 60;
    dhour = (int) (l_clock % 24);
    res->tm_hour = dhour;       /* hour */

    l_clock /= 24;
    l_clock += 1;

    dyear = myear;
    while (1)
    {
        /* fix_for_leapyear */
        if (isLeapYear(dyear))
        {
            if (l_clock < 366)
                break;
            l_clock -= 366;
        }
        else
        {
            if (l_clock < 365)
                break;
            l_clock -= 365;
        }
        dyear++;
    }

    for (dmon = 0; dmon < 12; dmon++)
    {
        if (dmon == 1 && isLeapYear(dyear))
        {
            if ((int) l_clock <= (daysofmonth[dmon] + 1))
                break;

            l_clock -= (DB_INT64) (daysofmonth[dmon] + 1);

        }
        else if ((int) l_clock <= daysofmonth[dmon])
            break;
        else
            l_clock -= (DB_INT64) daysofmonth[dmon];
    }

    dday = (int) l_clock;

    if (dday == 0)
    {
        if (dmon == 0)
        {
            dmon = 11;
            dday = daysofmonth[dmon];
            dyear -= 1;
        }
        else
        {
            dmon -= 1;

            if (dmon == 1 && isLeapYear(dyear))
                dday = daysofmonth[dmon] + 1;
            else
                dday = daysofmonth[dmon];
        }
    }

    res->tm_mday = dday;
    res->tm_mon = dmon;
    res->tm_year = dyear - 1900;

    // 1969/12/31 => 719162
    res->tm_wday = ((*p_clock / 86400) + 719162 + 1) % 7;
    res->tm_yday = 0;

    res->tm_isdst = 0;

    return 0;
}

/*****************************************************************************/
//! setTimeGapTZ 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘  및 기타 설명
 *****************************************************************************/
static int
setTimeGapTZ(void)
{
    if (_timegapTZ == -1)
    {
        _timegapTZ = sc_getTimeGapTZ();
    }

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
struct db_tm *
gmtime_as(const DB_INT64 * p_clock, struct db_tm *res)
{
    if (p_clock == NULL || res == NULL)
        return NULL;

    /* 1-1-1 0:0:0 ~ 9999:12:31 23:59:59 */
#ifdef WIN32
    if (*p_clock < -62135596800 || 253402300799 < *p_clock)
#else
    if (*p_clock < -62135596800LL || 253402300799LL < *p_clock)
#endif
        return NULL;

    _time2tm(p_clock, res, 0);

    return res;
}

/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param clock(in) :
 * \param res(out) : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
/* clock에 해당되는 local datetime으로 변경 */
struct db_tm *
localtime_as(const DB_INT64 * p_clock, struct db_tm *res)
{
    DB_INT64 lclock = *p_clock;

    if (p_clock == NULL || res == NULL)
        return NULL;

    /* 1-1-1 0:0:0 ~ 9999:12:31 23:59:59 */
#ifdef WIN32
    if (*p_clock < -62135596800 || 253402300799 < *p_clock)
#else
    if (*p_clock < -62135596800LL || 253402300799LL < *p_clock)
#endif
        return NULL;

    lclock += _timegapTZ;

    _time2tm(&lclock, res, 1);

    return res;
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
/* struct db_tm은 local datetime */
__DECL_PREFIX DB_INT64
mktime_as(struct db_tm * timeptr)
{
    DB_INT64 l_clock;

    if (timeptr == NULL)
        return 0;

    if (timeptr->tm_year + 1900 < 1 || timeptr->tm_year + 1900 > 9999)
        return 0;

    if (_timegapTZ == -1)
        setTimeGapTZ();

    l_clock = _tm2time(timeptr);

    l_clock -= _timegapTZ;

    return l_clock;
}

DB_INT64
mktime_as_2(struct db_tm * timeptr)
{
    DB_INT64 l_clock;

    if (timeptr == NULL)
    {
#if defined(WIN32)
        return -62167219200;
#else
        return -62167219200LL;
#endif
    }

    if (timeptr->tm_year + 1900 < 1 || timeptr->tm_year + 1900 > 9999)
    {
        /* 날짜 범위를 벗어난 경우 0000-01-01 에 해당하는 값을 반환 */
#if defined(WIN32)
        return -62167219200;
#else
        return -62167219200LL;
#endif
    }

    l_clock = _tm2time(timeptr);

    return l_clock;
}

#if defined(_AS_WINCE_) || defined(ANDROID_OS)
/*** shared memory operation ***/
/*****************************************************************************/
//! shmget 
/*! \breif  shared memory 관련 
 ************************************
 * \param key :
 * \param size : 
 * \param shmflg :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
shmget(int key, size_t size, int shmflg)
{
    return 0;
}

/*****************************************************************************/
//! shmctl 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param shmid :
 * \param cmd : 
 * \param buf :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
shmctl(int shmid, int cmd, struct shmid_ds *buf)
{
    return 0;
}

/*****************************************************************************/
//! shmat 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param shmid :
 * \param shmaddr : 
 * \param shmflg :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void *
shmat(int shmid, const void *shmaddr, int shmflg)
{
    return NULL;
}

/*****************************************************************************/
//! shmdt 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param shmaddr :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
#if defined(_AS_WINCE_)
shmdt(char *shmaddr)
#elif defined(ANDROID_OS)
shmdt(const void *shmaddr)
#endif
{
    return 0;
}

#endif

__DECL_PREFIX int
MDB_FLOAT2STR(char *str, int buf_size, float f)
{
    int len;
    char buf[512];

    /* len = sc_snprintf(buf, buf_size, "%f", f); */
    len = sc_sprintf(buf, "%f", f);

    if (sc_strchr(buf, '.'))
    {
        while (len > 0 && buf[--len] == '0')
            ;

        if (buf[len] != '.')
            ++len;

        buf[len] = '\0';
    }

    if (len >= 15 /* CSIZE_FLOAT */ )
        len = sc_sprintf(str, "%e", f);
    else
        sc_strncpy(str, buf, 15);


    return len;
}

__DECL_PREFIX int
MDB_DOUBLE2STR(char *str, int buf_size, double f)
{
    int len;
    char buf[512];

    len = sc_sprintf(buf, "%f", f);

    if (sc_strchr(buf, '.'))
    {
        while (len > 0 && buf[--len] == '0')
            ;

        if (buf[len] != '.')
            ++len;

        buf[len] = '\0';
    }

    if (len >= 15 /* CSIZE_DOUBLE */ )
        len = sc_sprintf(str, "%e", f);
    else
        sc_strncpy(str, buf, 15);

    return len;
}

void __reverse_str(char *str, int j);

int __values[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F'
};

__DECL_PREFIX unsigned char sc_ctype[256] = {
    _MDB_C, _MDB_C, _MDB_C, _MDB_C, _MDB_C, _MDB_C, _MDB_C, _MDB_C,     /* 0-7 */
    _MDB_C, _MDB_C | _MDB_S, _MDB_C | _MDB_S, _MDB_C | _MDB_S, _MDB_C | _MDB_S, _MDB_C | _MDB_S, _MDB_C, _MDB_C,        /* 8-15 */
    _MDB_C, _MDB_C, _MDB_C, _MDB_C, _MDB_C, _MDB_C, _MDB_C, _MDB_C,     /* 16-23 */
    _MDB_C, _MDB_C, _MDB_C, _MDB_C, _MDB_C, _MDB_C, _MDB_C, _MDB_C,     /* 24-31 */
    _MDB_S | _MDB_SP, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P,   /* 32-39 */
    _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P,     /* 40-47 */
    _MDB_D, _MDB_D, _MDB_D, _MDB_D, _MDB_D, _MDB_D, _MDB_D, _MDB_D,     /* 48-55 */
    _MDB_D, _MDB_D, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P,     /* 56-63 */
    _MDB_P, _MDB_U | _MDB_X, _MDB_U | _MDB_X, _MDB_U | _MDB_X, _MDB_U | _MDB_X, _MDB_U | _MDB_X, _MDB_U | _MDB_X, _MDB_U,       /* 64-71 */
    _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U,     /* 72-79 */
    _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U,     /* 80-87 */
    _MDB_U, _MDB_U, _MDB_U, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P,     /* 88-95 */
    _MDB_P, _MDB_L | _MDB_X, _MDB_L | _MDB_X, _MDB_L | _MDB_X, _MDB_L | _MDB_X, _MDB_L | _MDB_X, _MDB_L | _MDB_X, _MDB_L,       /* 96-103 */
    _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L,     /* 104-111 */
    _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L,     /* 112-119 */
    _MDB_L, _MDB_L, _MDB_L, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_C,     /* 120-127 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     /* 128-143 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     /* 144-159 */
    _MDB_S | _MDB_SP, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P,   /* 160-175 */
    _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P, _MDB_P,     /* 176-191 */
    _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U,     /* 192-207 */
    _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_P, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_U, _MDB_L,     /* 208-223 */
    _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L,     /* 224-239 */
    _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_P, _MDB_L,
    _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L, _MDB_L
};          /* 240-255 */

 // -2147483648의 경우를 처리
__DECL_PREFIX int
mdb_itoa(int value, char *str, int radix)
{
    register int _i;

    if (value < -2147483646)
    {
        if (value == -2147483647)
            sc_strcpy(str, "-2147483647");
        else
            sc_strcpy(str, "-2147483648");
        return 11;
    }
    else
    {
        _i = 0;
        if (value < 0)
        {
            str[0] = '-';
            _i++;
            value = -value;
        }
        do
        {
            str[_i] = __values[value % radix];
            _i++;
            value /= radix;
        } while (value > 0);

        __reverse_str(str, _i);

        str[_i] = '\0';

        return _i;
    }
}

static int __radix[17] = {
    0,
    1,
    128,
    2187,
    16384,
    78125,
    279936,
    823543,
    2097152,
    4782969,
    1000000000,
    19487171,
    35831808,
    62748517,
    105413504,
    170859375,
    268435456
};

static int __radix_num[17] = {
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    9,
    7,
    7,
    7,
    7,
    7,
    7
};

static int __mdb_itoa(int value, char *str, int radix);

__DECL_PREFIX int
MDB_bitoa(bigint value, char *str, int radix)
{
    register int r;
    register int i;
    int len;
    MDB_UINT64 temp;

    if (-2147483647 <= value && value <= 2147483647)
    {
        return mdb_itoa((int) value, str, radix);
    }

    i = 0;
    if (value < 0)
    {
        str[0] = '-';
        i++;
        temp = -value;
    }
    else
        temp = value;

    while (1)
    {
        r = (int) (temp % __radix[radix]);
        temp /= __radix[radix];

        if (temp == 0)
        {
            i += __mdb_itoa(r, str + i, radix);
            break;
        }
        else
        {
            if (r == 0)
            {
                sc_memset(str + i, '0', __radix_num[radix]);
                i += __radix_num[radix];
            }
            else
            {
                len = __mdb_itoa(r, str + i, radix);
                if (len < __radix_num[radix])
                    sc_memset(str + len + i, '0', __radix_num[radix] - len);
                i += __radix_num[radix];
            }
        }
    }

    __reverse_str(str, i);
    str[i] = '\0';

    return i;
}

void
__reverse_str(char *str, int j)
{
    register int i;             /*, j = len - 1; */
    register int t;

    j--;
    i = 0;
    if (str[0] == '-')
        i++;

    for (; i < j; i++)
    {
        t = str[i];
        str[i] = str[j];
        str[j--] = t;
    }
}

__DECL_PREFIX int
mdb_strlcpy(char *s1, const char *s2, int n)
{
    register int i, t;

    i = 0;
    while (i < n)
    {
        t = *(s2++);
        if (t == '\0')
            break;
        *(s1++) = t;
        i++;
    }
    *s1 = '\0';

    return i;
}

static int
__mdb_itoa(int value, char *str, int radix)
{
    register int i;

    i = 0;
    do
    {
        str[i] = __values[value % radix];
        i++;
        value /= radix;
    } while (value > 0);

    str[i] = '\0';

    return i;
}

/*****************************************************************************/
//! sc_memdcmp
/*! \breif  memcmp in dicorder
 ************************************
 * \param   s1(in)  : string1
 * \param   s2(in)  : string2
 * \param   n(in)   : compare n bytes
 ************************************
 * \return  return_value
 ************************************
 * \note
 *****************************************************************************/
int
sc_memdcmp(const void *s1, const void *s2, size_t n)
{
    int nCmp = mdb_strncasecmp(s1, s2, n);

    if (nCmp == 0)
    {
        return mdb_strncmp(s1, s2, n);
    }

    return nCmp;
}

/*****************************************************************************/
//! sc_strdcmp
/*! \breif  memcmp in dicorder
 ************************************
 * \param   s1(in)  : string1
 * \param   s2(in)  : string2
 ************************************
 * \return  return_value
 ************************************
 * \note
 *****************************************************************************/
int
sc_strdcmp(const char *str1, const char *str2)
{
    int nCmp = mdb_strcasecmp(str1, str2);

    if (nCmp == 0)
    {
        return mdb_strcmp(str1, str2);
    }

    return nCmp;
}

/*****************************************************************************/
//! sc_strndcmp
/*! \breif  memcmp in dicorder
 ************************************
 * \param   s1(in)  : string1
 * \param   s2(in)  : string2
 * \param   n(in)   : compare n bytes
 ************************************
 * \return  return_value
 ************************************
 * \note
 *****************************************************************************/
int
sc_strndcmp(const char *s1, const char *s2, int n)
{
    int nCmp = mdb_strncasecmp(s1, s2, n);

    if (nCmp == 0)
    {
        return mdb_strncmp(s1, s2, n);
    }

    return nCmp;
}

/* from googling */
MDB_ENDIAN_TYPE
UtilEndianType(void)
{
    MDB_ENDIAN_TYPE EndianType = MDB_ENDIAN_NO_INFORMATION;
    unsigned long Value = 0x12345678;
    unsigned char *cPtr = (unsigned char *) &Value;

    if (*cPtr == 0x12 && *(cPtr + 1) == 0x34 && *(cPtr + 2) == 0x56
            && *(cPtr + 3) == 0x78)
        EndianType = MDB_ENDIAN_BIG;
    else if (*cPtr == 0x78 && *(cPtr + 1) == 0x56 && *(cPtr + 2) == 0x34 &&
            *(cPtr + 3) == 0x12)
        EndianType = MDB_ENDIAN_LITTLE;
    else if (*cPtr == 0x34 && *(cPtr + 1) == 0x12 && *(cPtr + 2) == 0x78 &&
            *(cPtr + 3) == 0x56)
        EndianType = MDB_ENDIAN_MIDDLE;
    return EndianType;
}

#include "mdb_Cont.h"
#include "mdb_TTree.h"
#include "mdb_PMEM.h"
#include "mdb_dbi.h"
#include "mdb_Server.h"

__DECL_PREFIX void
convert_dir_delimiter(char *path)
{
    /* char rev_dir_delim; unused */
    int i;


    i = 0;
    while (1)
    {
        switch (path[i])
        {
        case '\0':
            return;
        case '/':
            if (DIR_DELIM_CHAR == '\\')
                path[i] = '\\';
            break;
        case '\\':
            if (DIR_DELIM_CHAR == '/')
                path[i] = '/';
            break;
        default:
            break;
        }
        i++;

        if (i == MDB_FILE_NAME)
            break;
    }

    return;
}

/* year, month must be valid */
int
sc_maxdayofmonth(int year, int month)
{
    int daynum = 0;
    int leapyear = isLeapYear(year);

    daynum += daysofmonth[month - 1];
    if (month == 2 && leapyear)
        daynum++;

    return daynum;
}

int
mdb_binary2bstring(char *src, char *dst, int src_length)
{
    int conveted, i, dst_len = 0, exp;
    int len = src_length;
    unsigned char value;

    for (conveted = 0; conveted < len; ++conveted)
    {
        exp = 128;
        value = src[conveted];

        for (i = 0; i < 8; i++)
        {
            dst_len = i + (conveted * 8);

            if ((value / exp))
            {
                dst[dst_len] = '1';
                value -= exp;
            }
            else
                dst[dst_len] = '0';

            exp /= 2;
        }
    }

    return dst_len + 1;
}

int
mdb_binary2hstring(char *src, char *dst, int src_length)
{
    int conveted = 0;
    int len = src_length;
    unsigned char value, ch;

    for (conveted = 0; conveted < len; ++conveted)
    {
        value = src[conveted];

        ch = value >> 4;
        if (ch > 9)
            dst[conveted * 2] = ch + 'a' - 10;
        else
            dst[conveted * 2] = ch + '0';

        ch = value - (ch * 16);
        if (ch > 9)
            dst[conveted * 2 + 1] = ch + 'a' - 10;
        else
            dst[conveted * 2 + 1] = ch + '0';
    }
    return conveted * 2;
}

DB_UINT32
mdb_MakeChecksum(char *buf, int buflen, int offset)
{
    DB_UINT32 checksum = 0, val;
    int i;

    for (i = 0; i < buflen - 4; i += offset)
    {
        sc_memcpy(&val, buf + i, 4);

        checksum += val;
    }

    return checksum;
}

/* checksum은  4 바이트라고 가정 ! */
int
mdb_CompareChecksum(char *buf, int buflen, int offset, void *checksum)
{
    DB_UINT32 chksum, prev_chksum;


    chksum = mdb_MakeChecksum(buf, buflen, offset);

    sc_memcpy(&prev_chksum, checksum, 4);

    if (chksum == prev_chksum)
        return 0;

    return 1;
}


#if !defined(DO_NOT_USE_MDB_DEBUGLOG)
static char *_wday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

static char *_mon[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

__DECL_PREFIX int
mdb_debuglog(char *str, ...)
{
    va_list ap;
    char debug_buf[256];
    int debuglog_fd = -1;
    static char filename[256] = "";

    char time_buf[64];
    struct db_tm tm;
    db_time_t aclock;
    int n;

    if (server == NULL || server->shparams.debug_log == 0)
        return 0;

    if (filename[0] == '\0')
    {
        sc_sprintf(filename, "%s" DIR_DELIM "debuglog.txt",
                sc_getenv("MOBILE_LITE_CONFIG"));
    }

    debuglog_fd = sc_open(filename, O_RDWR | O_CREAT | O_APPEND | O_BINARY,
            OPEN_MODE);
    if (debuglog_fd < 0)
    {
        MDB_SYSLOG(("mdb_debuglog open FAIL %s, %d\n", filename, debuglog_fd));
        return debuglog_fd;
    }

    aclock = sc_time(NULL);     /* Get time in seconds */
    sc_localtime_r(&aclock, &tm);       /* Convert time to struct */
    /* tm form */

    sc_sprintf(time_buf, "%s %s %d %02d:%02d:%02d %d",
            _wday[tm.tm_wday], _mon[tm.tm_mon], tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_year + 1900);

    n = sc_sprintf(debug_buf, "%08x:%s:", sc_get_taskid(), time_buf);

    va_start(ap, str);
    sc_vsnprintf(debug_buf + n, sizeof(debug_buf) - n - 2, str, ap);
    va_end(ap);

    debug_buf[sizeof(debug_buf) - 1] = '\0';

    sc_write(debuglog_fd, debug_buf, sc_strlen(debug_buf));

    sc_close(debuglog_fd);

    return 0;
}
#endif

int
mdb_strrcmp(const char *s1, const char *s2)
{
    unsigned char *offset1 = (unsigned char *) s1;
    unsigned char *offset2 = (unsigned char *) s2;

    // REVERSE_DELIMETER_UPPER == 'P','p' or '\0' Search
    while (*offset1 != '\0' && !IS_REVERSE_DELIMITER(*offset1))
    {
        offset1++;
    }
    while (*offset2 != '\0' && !IS_REVERSE_DELIMITER(*offset2))
    {
        offset2++;
    }

    // ex) "123P" , *offset1 == P  --> *offset1 == 3 Back
    if (0 < (offset1 - (unsigned char *) s1))
        offset1--;

    if (0 < (offset2 - (unsigned char *) s2))
        offset2--;

    // reserve string compare, ( 'right to left' with NOT NULL )
    while (*offset1 == *offset2 && offset1 > (unsigned char *) s1
            && offset2 > (unsigned char *) s2)
    {
        offset1--;
        offset2--;
    }

    //  not same
    if (*offset1 != *offset2 && !IS_REVERSE_DELIMITER(*offset1)
            && !IS_REVERSE_DELIMITER(*offset2))
    {
        if (*offset1 > *offset2)
            return 1;
        else
            return -1;
    }

    //  same
    if (*offset1 == *offset2)
        return 0;

    // ex) "P" == "P", "p" == "p", "" == "", "p" = "", "" = "p", "P" == "", "" == "P", "" == ""
    if (*offset1 != '\0' && *offset2 != '\0' && IS_REVERSE_DELIMITER(*offset1)
            && !(IS_REVERSE_DELIMITER(*offset2)))
        return -1;
    if (*offset2 != '\0' && *offset2 != '\0' && IS_REVERSE_DELIMITER(*offset2)
            && !(IS_REVERSE_DELIMITER(*offset1)))
        return 1;

    return 0;

}

int
mdb_strnrcmp(const char *s1, const char *s2, int n)
{
    unsigned char *offset1 = (unsigned char *) s1;
    unsigned char *offset2 = (unsigned char *) s2;

    while (*offset1 != '\0' && !IS_REVERSE_DELIMITER(*offset1))
    {
        offset1++;
    }
    while (*offset2 != '\0' && !IS_REVERSE_DELIMITER(*offset2))
    {
        offset2++;
    }

    if (0 < (offset1 - (unsigned char *) s1))
        offset1--;

    if (0 < (offset2 - (unsigned char *) s2))
        offset2--;

    while (--n > 0 && *offset1 == *offset2 && offset1 > (unsigned char *) s1
            && offset2 > (unsigned char *) s2)
    {
        offset1--;
        offset2--;
    }

    if (*offset1 != *offset2 && !IS_REVERSE_DELIMITER(*offset1)
            && !IS_REVERSE_DELIMITER(*offset2))
    {
        return *offset1 - *offset2;
    }

    if (n <= 0 && !IS_REVERSE_DELIMITER(*offset1)
            && !IS_REVERSE_DELIMITER(*offset2))
    {
        return *offset1 - *offset2;
    }

    if ((offset1 - (unsigned char *) s1) > 0
            && (offset2 - (unsigned char *) s2) == 0)
    {
        if (IS_REVERSE_DELIMITER(*offset2)
                && !(IS_REVERSE_DELIMITER(*offset1)))
            return *offset1;
        else
            return *(offset1 - 1);
    }
    else if ((offset2 - (unsigned char *) s2) > 0
            && (offset1 - (unsigned char *) s1) == 0)
    {
        if (IS_REVERSE_DELIMITER(*offset1)
                && !(IS_REVERSE_DELIMITER(*offset2)))
            return '\0' - *offset2;
        else
            return '\0' - *(offset2 - 1);
    }

    if (IS_REVERSE_DELIMITER(*offset2) && !(IS_REVERSE_DELIMITER(*offset1)))
        return *offset1;

    if (IS_REVERSE_DELIMITER(*offset1) && !(IS_REVERSE_DELIMITER(*offset2)))
        return '\0' - *offset2;

    return 0;
}

int
mdb_strnrcmp2(const char *s1, int s1_len, const char *s2, int s2_len, int n)
{
    unsigned char *offset1 = (unsigned char *) s1;
    unsigned char *offset2 = (unsigned char *) s2;

    while (*offset1 != '\0' && !IS_REVERSE_DELIMITER(*offset1))
    {
        offset1++;
    }
    while (*offset2 != '\0' && !IS_REVERSE_DELIMITER(*offset2))
    {
        offset2++;
    }

    s1_len--;
    s2_len--;

    if (0 < (offset1 - (unsigned char *) s1))
        offset1--;

    if (0 < (offset2 - (unsigned char *) s2))
        offset2--;

    if (0 < s1_len && s1_len < (offset1 - (unsigned char *) s1))
        offset1 = (unsigned char *) s1 + (s1_len);

    if (0 < s2_len && s2_len < (offset2 - (unsigned char *) s2))
        offset2 = (unsigned char *) s2 + (s2_len);

    while (--n > 0 && *offset1 == *offset2 && offset1 > (unsigned char *) s1
            && offset2 > (unsigned char *) s2)
    {
        offset1--;
        offset2--;
    }

    if (*offset1 != *offset2 && !IS_REVERSE_DELIMITER(*offset1)
            && !IS_REVERSE_DELIMITER(*offset2))
    {
        return *offset1 - *offset2;
    }

    if (n <= 0 && !IS_REVERSE_DELIMITER(*offset1)
            && !IS_REVERSE_DELIMITER(*offset2))
    {
        return *offset1 - *offset2;
    }

    if ((offset1 - (unsigned char *) s1) > 0
            && (offset2 - (unsigned char *) s2) == 0)
    {
        if (IS_REVERSE_DELIMITER(*offset2)
                && !(IS_REVERSE_DELIMITER(*offset1)))
            return *offset1;
        else
            return *(offset1 - 1);
    }
    else if ((offset2 - (unsigned char *) s2) > 0
            && (offset1 - (unsigned char *) s1) == 0)
    {
        if (IS_REVERSE_DELIMITER(*offset1)
                && !(IS_REVERSE_DELIMITER(*offset2)))
            return '\0' - *offset2;
        else
            return '\0' - *(offset2 - 1);
    }

    if (IS_REVERSE_DELIMITER(*offset2) && !(IS_REVERSE_DELIMITER(*offset1)))
        return *offset1;

    if (IS_REVERSE_DELIMITER(*offset1) && !(IS_REVERSE_DELIMITER(*offset2)))
        return '\0' - *offset2;

    return 0;
}

void *
mdb_memrcpy(void *dest, const void *src, size_t n)
{

    char *offset1 = (char *) dest;
    char *offset2 = (char *) src;
    int length;

    while (*offset2 && !IS_REVERSE_DELIMITER(*offset2))
    {
        offset2++;
    }

    length = offset2 - (char *) src;

    if (length < n)
    {
        offset2 -= length;
        n = length;
    }
    else
    {
        offset2 -= n;
    }

    while (n--)
    {
        *offset1++ = *offset2++;
    }

    return dest;
}


char *
mdb_strrchr(const char *s, int c)
{
    char c0 = c;
    char c1;

    if (c == 0)
    {
        while (*s++)
        {;
        }

        return (char *) (s - 1);
    }

    while ((c1 = *s++) != '\0')
    {
        if (c1 == c0 || IS_REVERSE_DELIMITER(c1))
            return (char *) (s - 1);

    }

    return (0);
}

char *
mdb_strrcpy(char *dest, const char *src)
{
    char *offset1 = (char *) dest;
    char *offset2 = (char *) src;

    while (*offset2 && !IS_REVERSE_DELIMITER(*offset2))
    {
        *offset1++ = *offset2++;
    }

    *offset1 = '\0';

    return dest;
}

size_t
mdb_strrlen(const char *s)
{
    char *offset = (char *) s;

    while (*offset && !IS_REVERSE_DELIMITER(*offset))
    {
        offset++;
    }

    return (offset - s);
}

extern __DECL_PREFIX MDB_INT32 sc_db_islocked(void);

int _mdb_db_lock_depth = 0;
MDB_PID _mdb_db_owner_thrid;
int _mdb_db_owner_pid;
MDB_PID _mdb_db_owner;

void
increase_mdb_db_lock_depth()
{
    if (server != NULL)
        server->_mdb_db_lock_depth++;

    _mdb_db_lock_depth++;

    return;
}

void
decrease_mdb_db_lock_depth()
{
    if (server != NULL)
        server->_mdb_db_lock_depth--;

    _mdb_db_lock_depth--;

    return;
}

__DECL_PREFIX int
get_mdb_db_lock_depth(void)
{
    if (server != NULL)
        return server->_mdb_db_lock_depth;
    else
        return _mdb_db_lock_depth;
}

void
set_mdb_db_owner()
{
    if (server != NULL)
    {
        _mdb_db_owner_pid = server->_mdb_db_owner_pid = sc_getpid();
        _mdb_db_owner_thrid = server->_mdb_db_owner_thrid = sc_get_taskid();
        _mdb_db_owner = sc_get_taskid();
    }
    else
    {
        _mdb_db_owner_pid = sc_getpid();
        _mdb_db_owner_thrid = sc_get_taskid();
        _mdb_db_owner = sc_get_taskid();

    }

    return;
}

void
clear_mdb_db_owner()
{
    if (server != NULL)
    {
        _mdb_db_owner_pid = server->_mdb_db_owner_pid = 0;
        _mdb_db_owner_thrid = server->_mdb_db_owner_thrid = 0;
        _mdb_db_owner = 0;
        _mdb_db_lock_depth = server->_mdb_db_lock_depth = 0;
    }
    else
    {
        _mdb_db_owner_pid = 0;
        _mdb_db_owner_thrid = 0;
        _mdb_db_owner = 0;
        _mdb_db_lock_depth = 0;
    }

    return;
}

// Check is mdb_db_owner_thrid/pid is set to null or not
// returns 1 if mdb_db_owner_thrid/pid is null, 0 otherwise.
int
is_mdb_db_owner_null()
{
    if (server != NULL)
    {
        if (server->_mdb_db_owner_pid == 0 && server->_mdb_db_owner_thrid == 0)
            return 1;

        if (_mdb_db_owner_pid == 0 && _mdb_db_owner_thrid == 0)
            return 1;
    }
    else
    {
        if (_mdb_db_owner_pid == 0 && _mdb_db_owner_thrid == 0)
            return 1;

        if (_mdb_db_owner == 0)
            return 1;
    }

    return 0;
}


// Check if mdb_db_lock owner is pid, thrid or not.
// returns 1 if owner is pid, thrid, 0 otherwise
int
mdb_db_owner_is(int pid, MDB_PID thrid)
{
    if (server != NULL)
    {
        if (server->_mdb_db_owner_pid == pid &&
                server->_mdb_db_owner_thrid == thrid)
            return 1;
    }
    else
    {
        if (_mdb_db_owner_pid == pid && _mdb_db_owner_thrid == thrid)
            return 1;

        if (_mdb_db_owner == thrid)
            return 1;
    }

    return 0;
}


// Check if mdb_db_lock owner is myself(in thread level)
// returns 1 if owner is myself, 0 otherwise
#ifdef MDB_DEBUG
int
mdb_db_owner_is_myself()
{
    if (server != NULL)
    {
        if (server->_mdb_db_owner_pid == sc_getpid() &&
                server->_mdb_db_owner_thrid == sc_get_taskid())
            return 1;
    }
    else
    {
        if (_mdb_db_owner_pid == sc_getpid() &&
                _mdb_db_owner_thrid == sc_get_taskid())
            return 1;

        if (_mdb_db_owner == sc_get_taskid())
            return 1;
    }

    return 0;
}
#endif

MDB_INT32
mdb_db_lock_owner_transfer()
{
    _mdb_db_owner_pid = sc_getpid();
    _mdb_db_owner_thrid = sc_get_taskid();

    return DB_SUCCESS;
}

// for __createdb()
__DECL_PREFIX int
copy_lock_info(struct Server *dst, struct Server *src)
{
    dst->_mdb_db_lock_depth = src->_mdb_db_lock_depth;
    dst->_mdb_db_owner_pid = src->_mdb_db_owner_pid;
    dst->_mdb_db_owner_thrid = src->_mdb_db_owner_thrid;
    dst->gEDBTaskId_pid = src->gEDBTaskId_pid;
    dst->gEDBTaskId_thrid = src->gEDBTaskId_thrid;

    return DB_SUCCESS;
}

#if defined(MDB_DEBUG)
__DECL_PREFIX void
mdb_db_lock(char *__file__, const char *func, int __line__)
#else
__DECL_PREFIX void
mdb_db_lock()
#endif
{
    // when shared struct not yet called.
    sc_db_lock();

    increase_mdb_db_lock_depth();


    if (get_mdb_db_lock_depth() == 1)
    {
        set_mdb_db_owner();
    }
    else
    {
#ifdef MDB_DEBUG
        if (mdb_db_owner_is_myself() != 1)
            sc_assert(0, __file__, __line__);
#endif
    }

#ifdef MDB_DEBUG
    if (get_mdb_db_lock_depth() > 20)
        sc_assert(0, __file__, __line__);
#endif

    return;
}

#if defined(MDB_DEBUG)
__DECL_PREFIX void
mdb_db_unlock(char *__file__, const char *func, int __line__)
#else
__DECL_PREFIX void
mdb_db_unlock()
#endif
{
    if (get_mdb_db_lock_depth() == 0)
    {
        if (is_mdb_db_owner_null() == 1)
            return;
    }

#ifdef MDB_DEBUG
    if (mdb_db_owner_is_myself() != 1)
    {
        sc_assert(0, __file__, __line__);
    }
#endif

    decrease_mdb_db_lock_depth();

#ifdef MDB_DEBUG
    if (get_mdb_db_lock_depth() < 0)
        sc_assert(0, __file__, __line__);
#endif

    if (get_mdb_db_lock_depth() == 0)
    {
        clear_mdb_db_owner();
        // when shared struct not yet called.
        sc_db_unlock();
    }

    return;
}

/* from dbi_Sort_Rid */
int
mdb_sort_array(void *pszOids[], int nMaxCnt,
        int (*comp_func) (const void *, const void *), int IsAsc)
{
    register int i, j, h, k;
    void *oid;

    if (!pszOids)
        return -1;

    if (nMaxCnt <= 0)
        return -2;

    for (h = 1; h < nMaxCnt; h = h * 3 + 1)
        ;

    if (IsAsc)
    {
        for (h = h / 3; h > 0; h = h / 3)
        {
            for (i = 0; i < h; i++)
            {
                for (j = i + h; j < nMaxCnt; j = j + h)
                {
                    oid = pszOids[j];
                    k = j;

                    while ((k > (h - 1)) && comp_func(pszOids[k - h], oid) > 0)
                    {
                        pszOids[k] = pszOids[k - h];
                        k = k - h;
                    }   // while

                    pszOids[k] = oid;
                }       // for (j)
            }   // for (i)
        }   // for (h)
    }
    else
    {
        for (h = h / 3; h > 0; h = h / 3)
        {
            for (i = 0; i < h; i++)
            {
                for (j = i + h; j < nMaxCnt; j = j + h)
                {
                    oid = pszOids[j];
                    k = j;

                    while ((k > (h - 1)) && comp_func(pszOids[k - h], oid) < 0)
                    {
                        pszOids[k] = pszOids[k - h];
                        k = k - h;
                    }   // while

                    pszOids[k] = oid;
                }       // for (j)
            }   // for (i)
        }   // for (h)
    }

    return 0;
}

int
mdb_check_numeric(char *src, DB_BOOL * is_negative_p, DB_BOOL * is_float_p)
{
    int i;
    DB_BOOL is_negative = 0, is_float = 0;

    if (src == NULL)
        return SQL_E_INVALIDARGUMENT;

    for (i = 0; src[i] != '\0'; i++)
    {
        if (!sc_isdigit((int) src[i]))
        {
            if (src[i] == ' ' || src[i] == '\t')
                continue;
            if (src[i] == '-')
            {
                if (!is_negative)
                    is_negative = 1;
                else
                    return SQL_E_NOTNUMBER;
            }
            else if (src[i] == '.')
            {
                if (is_float)
                    return SQL_E_NOTNUMBER;
                else
                    is_float = 1;
            }
            else
                return SQL_E_NOTNUMBER;
        }
        else
            is_negative = (DB_BOOL) - 1;
    }

    if (is_negative_p)
        *is_negative_p = is_negative;
    if (is_float_p)
        *is_float_p = is_float;

    return DB_SUCCESS;
}

bigint
mdb_char2bigint(char *str)
{
    int i, j;

    for (i = 0, j = 0; str[i] != '\0'; i++)
    {
        if (sc_isdigit((int) str[i]))
            str[j++] = str[i];
    }
    str[j] = '\0';
    return sc_atoll(str);
}

__DECL_PREFIX const unsigned char lower_array[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53,
    54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 97, 98, 99, 100, 101, 102, 103,
    104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118,
            119, 120, 121,
    122, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106,
            107,
    108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
            123, 124, 125,
    126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140,
            141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158,
            159, 160, 161,
    162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176,
            177, 178, 179,
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
            195, 196, 197,
    198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212,
            213, 214, 215,
    216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230,
            231, 232, 233,
    234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248,
            249, 250, 251,
    252, 253, 254, 255
};

char *
mdb_strlwr(char *str)
{
    register DB_UINT16 i = 0;

    if (str == NULL)
    {
        return NULL;
    }

    while (str[i])
    {
        str[i] = (char) lower_array[(unsigned char) str[i]];
        i++;
    }

    return str;
}
