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

#include "mdb_compat.h"
#include "sql_datast.h"
#include "sql_util.h"

#include "sql_func_timeutil.h"

#include "isql.h"
#include "mdb_er.h"

static iSQL_TIME __zero_timest = { 0, 1, 1, 0, 0, 0, 0 };
static uchar __zero_datetime[] = { "\000\000\001\001\000\000\000\000" };
static uchar __zero_date[] = { "\000\000\001\001" };
static uchar __zero_time[] = { "\000\000\000\000" };
static t_astm __zero_timestamp = { 0, 0, 0, 1, 0, -1900, 0, 0, 0 };

/*****************************************************************************/
//! strtol_limited 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param str :
 * \param limit : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static long int
strtol_limited(char *str, int limit)
{
    long int num = 0L;
    int i;

    if (limit < 0)
        limit = MDB_INT_MAX;
    for (i = 0; i < limit && str[i]; i++)
    {
        if (!sc_isdigit(str[i]))
        {
            return MDB_INT_MAX;
        }
        else
        {
            num = num * 10 + (str[i] - '0');
        }
    }

    return num;
}

/*****************************************************************************/
//! find_delimiter 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param str :
 * \param len : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
find_delimiter(char *str, int len)
{
    int i;

    for (i = 0; i < len && str[i]; i++)
        if (!sc_isdigit((int) str[i]))
        {
            return i;
        }

    return -1;
}

/*****************************************************************************/
//! get_timestamp_format 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param str :
 * \param len : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
get_timestamp_format(char *str, int len)
{
    int i;

    for (i = 0; i < len && str[i]; i++)
    {
        if (str[i] == '-')
            return 1;
        else if (str[i] == ':')
            return 3;
        else
            continue;
    }
    return 0;
}

/*****************************************************************************/
//! get_interval_value 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param val :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
T_INTERVAL
get_interval_value(T_VALUEDESC * val)
{
    char *p;

    p = val->u.ptr;

    if (!mdb_strcasecmp(p, "millisecond"))
        return INTERVAL_MILLI_SECOND;
    if (!mdb_strcasecmp(p, "milli-second"))
        return INTERVAL_MILLI_SECOND;
    if (!mdb_strcasecmp(p, "msec"))
        return INTERVAL_MILLI_SECOND;

    if (!mdb_strcasecmp(p, "second"))
        return INTERVAL_SECOND;
    if (!mdb_strcasecmp(p, "sec"))
        return INTERVAL_SECOND;

    if (!mdb_strcasecmp(p, "minute"))
        return INTERVAL_MINUTE;
    if (!mdb_strcasecmp(p, "min"))
        return INTERVAL_MINUTE;

    if (!mdb_strcasecmp(p, "hour"))
        return INTERVAL_HOUR;

    if (!mdb_strcasecmp(p, "day"))
        return INTERVAL_DAY;

    if (!mdb_strcasecmp(p, "month"))
        return INTERVAL_MONTH;
    if (!mdb_strcasecmp(p, "mon"))
        return INTERVAL_MONTH;

    if (!mdb_strcasecmp(p, "year"))
        return INTERVAL_YEAR;

    return INTERVAL_NONE;
}

#define UNKNOWN_DAY_OF_WEEK                7

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
get_weekday(int year, int month, int day)
{
    int cent, days;

    if (day < 1)
        return UNKNOWN_DAY_OF_WEEK;
    if (month == 2 && !isLeapYear(year) && day > 28)
        return UNKNOWN_DAY_OF_WEEK;
    else if ((month == 1 || month == 3 || month == 5 || month == 7 ||
                    month == 8 || month == 10 || month == 12) && day > 31)
        return UNKNOWN_DAY_OF_WEEK;
    else if ((month == 4 || month == 6 ||
                    month == 9 || month == 11) && day > 30)
        return UNKNOWN_DAY_OF_WEEK;

    /* adjust months so February is the last one */
    month -= 2;
    if (month < 1)
    {
        month += 12;
        --year;
    }

    /* split by century */
    cent = year / 100;
    year %= 100;
    days = ((26 * month - 2) / 10 + day + year
            + (year >> 2) + (cent >> 2) - 2 * cent) % 7;
    days += (days < 0) ? 7 : 0;

    days = (days + 6) % 7;

    return days;
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
int
get_day_of_year(int year, int month, int day)
{
    int days_of_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int past_days = 0;
    int i;

    for (i = 0; i < month - 1; i++)
        past_days += days_of_month[i];
    if (isLeapYear(year) && month > 2)
        past_days++;

    return (past_days + day);
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
char *
date2char(char *str_date, char *date)
{
    int i, j;

    for (i = 0, j = 0; i < MAX_DATE_FIELD_LEN; i++)
    {
        str_date[j++] = ((date[i] & 0xF0) >> 4) + '0';
        str_date[j++] = (date[i] & 0x0F) + '0';

        switch (j)
        {   // insert delimiter
        case 4:
        case 7:
            str_date[j++] = '-';
            break;
        }
    }
    str_date[j] = '\0';

    return str_date;
}

/*****************************************************************************/
//! char2date
//
/*! \breif  String 형태의 날짜 형식을 DATE FIELD (4 bytes) 형식으로 변환
 *         
 ************************************
 * \param date      : output
 * \param str_date  : input (string)
 * \param strlength : input string length
 ************************************
 * \return  변환된 4 bytes DATE FIELD
 ************************************
 * \note bit 연산을 통해 4 bytes 에 Year, Month, Day 정보를 채워넣음
 *****************************************************************************/
__DECL_PREFIX char *
char2date(char *date, char *str_date, int strlength)
{
    char *ptr;
    int i, success = 0;
    int num, deli;
    int year = 0, month = 0;

    ptr = date;
    sc_memset(ptr, 0, MAX_DATE_FIELD_LEN);

    if (!strlength)
    {
        return NULL;
    }

    // search any delimiter.
    deli = find_delimiter(str_date, strlength);
    if (deli < 0)
    {       // delimiter없이 연결되어 있는 경우.
        /* MAX_TIMPESTAMP_STRING_LEN - the number of delimiters - 1 */
        if (strlength > (MAX_TIMESTAMP_STRING_LEN - 7))
        {
            return NULL;
        }
        i = 0;
        while (1)
        {
            *date |= (str_date[i++] - '0') << 4;
            if (i == 5)
            {   // month
                if ((*date & 0xF0) >> 4 > 1)
                    goto RETURN_LABEL;
            }
            else if (i == 7)
            {   // day
                if ((*date & 0xF0) >> 4 > 3)
                    goto RETURN_LABEL;
            }
            if (i == strlength || i >= 8)
            {
                date++; // 상위 4bit이 이미 assign 되었으므로 next로 이동
                break;
            }

            *date |= (str_date[i++] - '0') & 0x0F;
            if (i == 6)
            {   // month
                if ((*date & 0x0F) >> 4 > 9)
                    goto RETURN_LABEL;
            }
            else if (i == 8)
            {   // day
                if ((*date & 0x0F) >> 4 > 9)
                    goto RETURN_LABEL;
            }

            // checking validation
            // year는 제외하자.
            if (i > 4)
            {
                if (i == 6)
                {       // month
                    num = ((*date & 0xF0) >> 4) * 10 + (*date & 0x0F);
                    if (num > 12)
                        goto RETURN_LABEL;
                }
                else if (i == 8)
                {       // day
                    num = ((*date & 0xF0) >> 4) * 10 + (*date & 0x0F);
                    if (num > 31)
                        goto RETURN_LABEL;
                    if (year > 0 && num > sc_maxdayofmonth(year, month))
                        goto RETURN_LABEL;
                }
            }

            date++;
            if (i == strlength || i >= 8)
                break;
        }
    }
    else
    {       // delimiter로 구분되어 있는 경우.
        if (strlength > MAX_TIMESTAMP_STRING_LEN - 1)
        {
            return NULL;
        }
        if (str_date[deli] != '-')
            return NULL;

        if (deli == 0)
            date += 2;  // bypass the year
        else
        {
            // year
            num = TO_NUMBER(str_date, deli);
            if (num > 9999)
                return NULL;

            year = num;

            *date |= (num / 1000) << 4;
            num %= 1000;
            *date |= (num / 100);
            num %= 100;
            date++;

            *date |= (num / 10) << 4;
            *date |= num % 10;
            date++;
        }

        str_date = str_date + deli + 1;
        strlength -= deli + 1;
        if (strlength <= 0)
        {
            sc_memcpy(date, &__zero_date[date - ptr],
                    MAX_DATE_FIELD_LEN - (int) (date - ptr + 1));
            return ptr;
        }

        // month
        deli = find_delimiter(str_date, strlength);
        if (str_date[deli] != '-')
            return NULL;

        num = TO_NUMBER(str_date, deli);
        if (num < 1 || num > 12)
            goto RETURN_LABEL;

        month = num;
        *date |= (num / 10) << 4;
        *date |= num % 10;
        date++;
        if (deli < 0)
        {
            sc_memcpy(date, &__zero_date[date - ptr],
                    MAX_DATE_FIELD_LEN - (int) (date - ptr));
            return ptr;
        }

        str_date += deli + 1;
        strlength -= deli + 1;
        if (strlength <= 0)
        {
            sc_memcpy(date, &__zero_date[date - ptr],
                    MAX_DATE_FIELD_LEN - (int) (date - ptr));
            return ptr;
        }

        // day
        deli = find_delimiter(str_date, strlength);
        num = TO_NUMBER(str_date, deli);
        if (num < 1 || num > 31)
            goto RETURN_LABEL;
        if (year > 0 && num > sc_maxdayofmonth(year, month))
            goto RETURN_LABEL;
        *date |= (num / 10) << 4;
        *date |= num % 10;
        date++;
    }

    /* check '0000' year */
    if (*(short int *) ptr == 0x0)
    {
        return NULL;
    }

    success = 1;

  RETURN_LABEL:
    if (!success)
    {
        sc_memset(ptr, 0, MAX_DATE_FIELD_LEN);
        return NULL;
    }
    else
    {
        if (date - ptr + 1 < MAX_DATE_FIELD_LEN)
            sc_memcpy(date, &__zero_date[date - ptr],
                    MAX_DATE_FIELD_LEN - (int) (date - ptr));
    }

    return ptr;
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
char *
time2char(char *str_time, char *p_time)
{
    int i, j;

    for (i = 0, j = 0; i < MAX_TIME_FIELD_LEN - 1; i++)
    {
        str_time[j++] = ((p_time[i] & 0xF0) >> 4) + '0';
        str_time[j++] = (p_time[i] & 0x0F) + '0';

        switch (j)
        {   // insert delimiter
        case 2:
        case 5:
            str_time[j++] = ':';
            break;
        }
    }
    str_time[j] = '\0';

    return str_time;
}

/*****************************************************************************/
//! char2time

/*! \breif  String 으로 이뤄진 시간 형식을 4 bytes 의 TIME FIELD 에 저장
 ************************************
 * \param time      : output
 * \param str_time  : input
 * \param strlength : input string length
 ************************************
 * \return  4 bytes TIME FIELD
 ************************************
 * \note String 을 parse 하여 Hour, Minute, Second 를 TIME FIELD 에 저장
 *****************************************************************************/
__DECL_PREFIX char *
char2time(char *time, char *str_time, int strlength)
{
    char *ptr;
    int i, success = 0;
    int num, deli;

    ptr = time;
    sc_memset(ptr, 0, MAX_TIME_FIELD_LEN);

    if (!strlength)
    {
        return NULL;
    }

    // search any delimiter.
    deli = find_delimiter(str_time, strlength);
    if (deli < 0)
    {       // delimiter없이 연결되어 있는 경우.
        /*  MAX_TIMESTAMP_STRING_LEN - the number of delimiter - 1 */
        if (strlength > (MAX_TIMESTAMP_STRING_LEN - 7))
        {
            return NULL;
        }
        else if (strlength > 6 && strlength < 9)
        {
            return NULL;
        }

        /* remove date (YYYYMMDD) */
        if (strlength > 8)
        {
            strlength -= 8;
            str_time += 8;
        }

        i = 0;
        while (1)
        {
            *time |= (str_time[i++] - '0') << 4;        // hour
            if (i == 3)
            {   // minute
                if ((*time & 0xF0) >> 4 > 5)
                    goto RETURN_LABEL;
            }
            else if (i == 5)
            {   // second
                if ((*time & 0xF0) >> 4 > 5)
                    goto RETURN_LABEL;
            }
            if (i == strlength || i >= 6)
            {
                time++; // 상위 4bit이 이미 assign 되었으므로 next로 이동
                break;
            }

            *time |= (str_time[i++] - '0') & 0x0F;      // hour
            if (i == 4)
            {   // minute
                if ((*time & 0x0F) >> 4 > 9)
                    goto RETURN_LABEL;
            }
            else if (i == 6)
            {   // second
                if ((*time & 0x0F) >> 4 > 9)
                    goto RETURN_LABEL;
            }

            // checking validation
            if (i == 2)
            {   // hour
                num = ((*time & 0xF0) >> 4) * 10 + (*time & 0x0F);
                if (num > 23)
                    goto RETURN_LABEL;
            }
            else if (i == 4)
            {   // minute
                num = ((*time & 0xF0) >> 4) * 10 + (*time & 0x0F);
                if (num > 59)
                    goto RETURN_LABEL;
            }
            else if (i == 6)
            {   // second
                num = ((*time & 0xF0) >> 4) * 10 + (*time & 0x0F);
                if (num > 59)
                    goto RETURN_LABEL;
            }

            time++;
            if (i == strlength || i >= 6)
                break;
        }
    }
    else
    {       // delimiter로 구분되어 있는 경우.
        int type;

        type = get_timestamp_format(str_time, strlength);
        if (type == 1)
        {
            if (strlength > MAX_TIMESTAMP_STRING_LEN - 1)
            {
                return NULL;
            }

            /* remove date (YYYY-MM-DD) */
            if (strlength > MAX_DATE_STRING_LEN)
            {
                strlength -= MAX_DATE_STRING_LEN;
                str_time += MAX_DATE_STRING_LEN;
                deli = find_delimiter(str_time, strlength);
            }
        }
        else
        {
            /* 00:00:00 */
            if (strlength > MAX_TIME_STRING_LEN - 1)
            {
                return NULL;
            }
        }

        // hour
        if (str_time[deli] != ':')
            return NULL;

        num = TO_NUMBER(str_time, deli);
        if (num > 23)
            goto RETURN_LABEL;
        *time |= (num / 10) << 4;
        *time |= num % 10;
        time++;
        if (deli < 0)
            return ptr;

        str_time += deli + 1;
        strlength -= deli + 1;
        if (strlength <= 0)
            return ptr;

        // minute
        deli = find_delimiter(str_time, strlength);
        if (str_time[deli] != ':')
            return NULL;

        num = TO_NUMBER(str_time, deli);
        if (num > 59)
            goto RETURN_LABEL;
        *time |= (num / 10) << 4;
        *time |= num % 10;
        time++;
        if (deli < 0)
            return ptr;

        str_time += deli + 1;
        strlength -= deli + 1;
        if (strlength <= 0)
            return ptr;

        // second
        num = TO_NUMBER(str_time, 2);
        if (num > 59)
            goto RETURN_LABEL;
        *time |= (num / 10) << 4;
        *time |= num % 10;
        time++;
    }

    success = 1;

  RETURN_LABEL:
    if (!success)
    {
        sc_memset(ptr, 0, MAX_TIME_FIELD_LEN);
        return NULL;
    }
    else
    {
        if (time - ptr + 1 < MAX_TIME_FIELD_LEN)
            sc_memcpy(time, &__zero_time[time - ptr],
                    MAX_TIME_FIELD_LEN - (int) (time - ptr + 1));
    }

    return ptr;
}

/*****************************************************************************/
//! datetime2char 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param str_datetime : output 
 * \param datetime : input
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
char *
datetime2char(char *str_datetime, char *datetime)
{
    int i, j;

    for (i = 0, j = 0; i < MAX_DATETIME_FIELD_LEN - 1; i++)
    {
        str_datetime[j++] = ((datetime[i] & 0xF0) >> 4) + '0';
        str_datetime[j++] = (datetime[i] & 0x0F) + '0';

        switch (j)
        {   // insert delimiter
        case 4:
        case 7:
            str_datetime[j++] = '-';
            break;
        case 10:
            str_datetime[j++] = ' ';
            break;
        case 13:
        case 16:
            str_datetime[j++] = ':';
            break;
        }
    }
    str_datetime[j] = '\0';

    return str_datetime;
}

/*****************************************************************************/
//! char2datetime 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param datetime :
 * \param str_datetime : 
 * \param strlength :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX char *
char2datetime(char *datetime, char *str_datetime, int strlength)
{
    char *ptr;
    int i, success = 0;
    int num, deli;
    int year = 0, month = 0;

    ptr = datetime;
    sc_memset(ptr, 0, MAX_DATETIME_FIELD_LEN);

    if (!strlength)
    {
        return NULL;
    }

    // search any delimiter.
    deli = find_delimiter(str_datetime, strlength);
    if (deli < 0)
    {       // delimiter없이 연결되어 있는 경우.
        /* MAX_TIMESTAMP_STRING_LEN - the number of delimiter -1 -1(space) */
        if (strlength > (MAX_TIMESTAMP_STRING_LEN - 7))
        {
            return NULL;
        }
        i = 0;
        while (1)
        {
            *datetime |= (str_datetime[i++] - '0') << 4;

            if (i == 5)
            {   // month
                if ((*datetime & 0xF0) >> 4 > 1)
                    goto RETURN_LABEL;
            }
            else if (i == 7)
            {   // day
                if ((*datetime & 0xF0) >> 4 > 3)
                    goto RETURN_LABEL;
            }
            else if (i == 9)
            {   // hour
                if ((*datetime & 0xF0) >> 4 > 2)
                    goto RETURN_LABEL;
            }
            else if (i == 11)
            {   // minute
                if ((*datetime & 0xF0) >> 4 > 5)
                    goto RETURN_LABEL;
            }
            else if (i == 13)
            {   // second
                if ((*datetime & 0xF0) >> 4 > 5)
                    goto RETURN_LABEL;
            }
            if (i == strlength || i >= 14)
            {
                datetime++;     // 상위 4bit이 이미 assign 되었으므로 next로 이동
                break;
            }

            *datetime |= (str_datetime[i++] - '0') & 0x0F;
            if (i == 6)
            {   // month
                if ((*datetime & 0x0F) >> 4 > 9)
                    goto RETURN_LABEL;
            }
            else if (i == 8)
            {   // day
                if ((*datetime & 0x0F) >> 4 > 9)
                    goto RETURN_LABEL;
            }
            else if (i == 10)
            {   // hour
                if ((*datetime & 0x0F) >> 4 > 9)
                    goto RETURN_LABEL;
            }
            else if (i == 12)
            {   // minute
                if ((*datetime & 0x0F) >> 4 > 9)
                    goto RETURN_LABEL;
            }
            else if (i == 14)
            {   // second
                if ((*datetime & 0x0F) >> 4 > 9)
                    goto RETURN_LABEL;
            }

            // checking validation
            // year는 제외하자.
            /* move codes to check '0000' year 
             * before RETURN_LABLE */
            if (i > 4)
            {
                if (i == 6)
                {       // month
                    num = ((*datetime & 0xF0) >> 4) * 10 + (*datetime & 0x0F);
                    if (num < 1 || num > 12)
                        goto RETURN_LABEL;
                }
                else if (i == 8)
                {       // day
                    num = ((*datetime & 0xF0) >> 4) * 10 + (*datetime & 0x0F);
                    if (num < 1 || num > 31)
                        goto RETURN_LABEL;
                    if (year > 0 && num > sc_maxdayofmonth(year, month))
                        goto RETURN_LABEL;
                }
                else if (i == 10)
                {       // hour
                    num = ((*datetime & 0xF0) >> 4) * 10 + (*datetime & 0x0F);
                    if (num > 23)
                        goto RETURN_LABEL;
                }
                else if (i == 12)
                {       // minute
                    num = ((*datetime & 0xF0) >> 4) * 10 + (*datetime & 0x0F);
                    if (num > 59)
                        goto RETURN_LABEL;
                }
                else if (i == 14)
                {       // second
                    num = ((*datetime & 0xF0) >> 4) * 10 + (*datetime & 0x0F);
                    if (num > 59)
                        goto RETURN_LABEL;
                }
            }

            datetime++;
            if (i == strlength || i >= 14)
                break;
        }
    }
    else
    {       // delimiter로 구분되어 있는 경우.
        int type;

        if (str_datetime[deli] != '-' && str_datetime[deli] != ':')
        {
            return NULL;
        }

        type = get_timestamp_format(str_datetime, strlength);
        if (type == 1)
        {

            if (strlength > MAX_TIMESTAMP_STRING_LEN - 1)
            {
                return NULL;
            }

            if (deli == 0)
                datetime += 2;  // bypass the year
            else
            {
                // year
                num = TO_NUMBER(str_datetime, deli);
                if (num < 1 || num > 9999)
                    return NULL;

                year = num;

                *datetime |= (num / 1000) << 4;
                num %= 1000;
                *datetime |= (num / 100);
                num %= 100;
                datetime++;

                *datetime |= (num / 10) << 4;
                *datetime |= num % 10;
                datetime++;
            }

            str_datetime = str_datetime + deli + 1;
            strlength -= deli + 1;
            if (strlength <= 0)
            {
                sc_memcpy(datetime,
                        &__zero_datetime[datetime - ptr],
                        MAX_DATETIME_FIELD_LEN - (int) (datetime - ptr + 1));
                return ptr;
            }

            // month
            deli = find_delimiter(str_datetime, strlength);
            if (str_datetime[deli] != '-')
                return NULL;

            num = TO_NUMBER(str_datetime, deli);
            if (num < 1 || num > 12)
                goto RETURN_LABEL;

            month = num;
            *datetime |= (num / 10) << 4;
            *datetime |= num % 10;
            datetime++;
            if (deli < 0)
            {
                sc_memcpy(datetime,
                        &__zero_datetime[datetime - ptr],
                        MAX_DATETIME_FIELD_LEN - (int) (datetime - ptr + 1));
                return ptr;
            }

            str_datetime += deli + 1;
            strlength -= deli + 1;
            if (strlength <= 0)
            {
                sc_memcpy(datetime,
                        &__zero_datetime[datetime - ptr],
                        MAX_DATETIME_FIELD_LEN - (int) (datetime - ptr + 1));
                return ptr;
            }

            // day
            deli = find_delimiter(str_datetime, strlength);
            num = TO_NUMBER(str_datetime, deli);
            if (num < 1 || num > 31)
                goto RETURN_LABEL;
            if (year > 0 && num > sc_maxdayofmonth(year, month))
                goto RETURN_LABEL;
            *datetime |= (num / 10) << 4;
            *datetime |= num % 10;
            datetime++;
            if (deli < 0)
            {
                sc_memcpy(datetime,
                        &__zero_datetime[datetime - ptr],
                        MAX_DATETIME_FIELD_LEN - (int) (datetime - ptr + 1));
                return ptr;
            }

            str_datetime += deli + 1;
            strlength -= deli + 1;
            if (strlength <= 0)
                return ptr;

        }   /* if (type == 1) {} */
        else if (type == 3)
        {
            /* 12 = '00:00:00.000' */
            if (strlength > 12)
            {
                return NULL;
            }

            /* set 1900-01-01 */
            *datetime++ = 25;
            *datetime++ = 0;
            *datetime++ = 1;
            *datetime++ = 1;
        }
        else
        {
            goto RETURN_LABEL;
        }

        // hour
        deli = find_delimiter(str_datetime, strlength);
        if (deli != -1 && str_datetime[deli] != ':')
            return NULL;

        num = TO_NUMBER(str_datetime, deli);
        if (num > 23)
            goto RETURN_LABEL;
        *datetime |= (num / 10) << 4;
        *datetime |= num % 10;
        datetime++;
        if (deli < 0)
            return ptr;

        str_datetime += deli + 1;
        strlength -= deli + 1;
        if (strlength <= 0)
            return ptr;

        // minute
        deli = find_delimiter(str_datetime, strlength);
        if (str_datetime[deli] != ':')
            return NULL;

        num = TO_NUMBER(str_datetime, deli);
        if (num > 59)
            goto RETURN_LABEL;
        *datetime |= (num / 10) << 4;
        *datetime |= num % 10;
        datetime++;
        if (deli < 0)
            return ptr;

        str_datetime += deli + 1;
        strlength -= deli + 1;
        if (strlength <= 0)
            return ptr;

        // second
        num = TO_NUMBER(str_datetime, 2);
        if (num > 59)
            goto RETURN_LABEL;
        *datetime |= (num / 10) << 4;
        *datetime |= num % 10;
        datetime++;
    }

    /* check '0000' year */
    if (*(short int *) ptr == 0x0)
    {
        return NULL;
    }

    success = 1;

  RETURN_LABEL:
    if (!success)
    {
        sc_memset(ptr, 0, MAX_DATETIME_FIELD_LEN);
        return NULL;
    }
    else
    {
        if (datetime - ptr + 1 < MAX_DATETIME_FIELD_LEN)
            sc_memcpy(datetime,
                    &__zero_datetime[datetime - ptr],
                    MAX_DATETIME_FIELD_LEN - (int) (datetime - ptr + 1));
    }

    return ptr;
}

/*****************************************************************************/
//! timestamp2char 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param str_timestamp :
 * \param timestamp : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
char *
timestamp2char(char *str_timestamp, t_astime * timestamp)
{
    t_astm astm;
    t_astime ast_sec;
    int posi = 0;
    int num;

    ast_sec = SEC_OF_TS(*timestamp);
    if (gmtime_as(&ast_sec, &astm) == NULL)
        sc_memset(&astm, 0, sizeof(t_astm));

    // year
    num = astm.tm_year + DEFAULT_START_YEAR;
    str_timestamp[posi++] = num / 1000 + '0';
    num = num % 1000;
    str_timestamp[posi++] = num / 100 + '0';
    num = num % 100;
    str_timestamp[posi++] = num / 10 + '0';
    str_timestamp[posi++] = num % 10 + '0';
    str_timestamp[posi++] = '-';

    // month since January
    str_timestamp[posi++] = (astm.tm_mon + 1) / 10 + '0';
    str_timestamp[posi++] = (astm.tm_mon + 1) % 10 + '0';
    str_timestamp[posi++] = '-';

    // day of the month
    str_timestamp[posi++] = (astm.tm_mday) / 10 + '0';
    str_timestamp[posi++] = (astm.tm_mday) % 10 + '0';
    str_timestamp[posi++] = ' ';

    // hour since midnight
    str_timestamp[posi++] = (astm.tm_hour) / 10 + '0';
    str_timestamp[posi++] = (astm.tm_hour) % 10 + '0';
    str_timestamp[posi++] = ':';

    // minute after the hour
    str_timestamp[posi++] = (astm.tm_min) / 10 + '0';
    str_timestamp[posi++] = (astm.tm_min) % 10 + '0';
    str_timestamp[posi++] = ':';

    // second after the minute
    str_timestamp[posi++] = (astm.tm_sec) / 10 + '0';
    str_timestamp[posi++] = (astm.tm_sec) % 10 + '0';

    str_timestamp[posi++] = '.';

    // millisecond after the second
    num = MSEC_OF_TS(*timestamp);
    str_timestamp[posi++] = num / 100 + '0';
    num = num % 100;
    str_timestamp[posi++] = num / 10 + '0';
    str_timestamp[posi++] = num % 10 + '0';
    str_timestamp[posi] = '\0';

    return str_timestamp;
}

#define CHECK_TOKEN_SIZE(t_, s_) \
    do { \
        if (t_ != s_) { \
            success = 0; \
            goto RETURN_LABEL; } \
    } while(0)

#define CHECK_TOKEN_RANGE(t_, a_, b_) \
    do { \
        if (t_ < a_ || t_ > b_) { \
            success = 0; \
            goto RETURN_LABEL; } \
    } while(0)

/*****************************************************************************/
//! time2timestamp 

/*! \breif  TIME STAMP 형태의 STRING을  t_astime로 변환한다 
 ************************************
 * \param timestamp(out) :
 * \param str_timestamp(in) : 
 * \param strlength(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX t_astime *
time2timestamp(t_astime * timestamp, char *time)
{
    t_astm astm = { 0, 0, 0, 1, 0, 0, 0, 0, 0 };
    t_astime astm_ms = 0;

    sc_memset(timestamp, 0, sizeof(t_astime));

    astm.tm_hour = ((time[0] & 0xF0) >> 4) * 10 + (time[0] & 0x0F);
    astm.tm_min = ((time[1] & 0xF0) >> 4) * 10 + (time[1] & 0x0F);
    astm.tm_sec = ((time[2] & 0xF0) >> 4) * 10 + (time[2] & 0x0F);

    if (astm.tm_mday > sc_maxdayofmonth(astm.tm_year + DEFAULT_START_YEAR,
                    astm.tm_mon + 1))
        return NULL;
    *timestamp = mktime_as_2(&astm);
    *timestamp = (*timestamp) << 16 | astm_ms;
    return timestamp;
}

/*****************************************************************************/
//! char2timestamp 

/*! \breif  TIME STAMP 형태의 STRING을  t_astime로 변환한다 
 ************************************
 * \param timestamp(out) :
 * \param str_timestamp(in) : 
 * \param strlength(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX t_astime *
char2timestamp(t_astime * timestamp, char *str_timestamp, int strlength)
{
    t_astm astm = __zero_timestamp;
    t_astime astm_ms = 0;
    int success = 1;
    int deli, num;
    int type;

    sc_memset(timestamp, 0, sizeof(t_astime));

    if (!strlength)
    {
        return NULL;
    }

    type = get_timestamp_format(str_timestamp, strlength);

    if (type == 0)
    {       // delimiter없이 연결되어 있는 경우.
        /* MAX_TIMESTAMP_STRING_LEN - the number of delimiters -1(space) -1 */
        if (strlength > (MAX_TIMESTAMP_STRING_LEN - 7))
        {
            strlength = sc_strlen(str_timestamp);
            if (strlength > (MAX_TIMESTAMP_STRING_LEN - 7))
            {
                return NULL;
            }
        }
        if (strlength == 1)
            astm.tm_year =
                    TO_NUMBER(str_timestamp, 1) * 1000 - DEFAULT_START_YEAR;
        else if (strlength == 2)
            astm.tm_year =
                    TO_NUMBER(str_timestamp, 2) * 100 - DEFAULT_START_YEAR;
        else if (strlength == 3)
            astm.tm_year =
                    TO_NUMBER(str_timestamp, 3) * 10 - DEFAULT_START_YEAR;
        else
            astm.tm_year = TO_NUMBER(str_timestamp, 4) - DEFAULT_START_YEAR;

        if (astm.tm_year + DEFAULT_START_YEAR < 1
                || astm.tm_year + DEFAULT_START_YEAR > 9999)
        {
            return NULL;
        }

        str_timestamp += 4;
        strlength -= 4;
        if (strlength <= 0)
            goto RETURN_LABEL;

        if (strlength == 1)
            num = TO_NUMBER(str_timestamp, 1) * 10;
        else
            num = TO_NUMBER(str_timestamp, 2);

        if (num < 1 || num > 12)
        {
            success = 0;
            goto RETURN_LABEL;
        }

        astm.tm_mon = num - 1;

        str_timestamp += 2;
        strlength -= 2;
        if (strlength <= 0)
            goto RETURN_LABEL;

        if (strlength == 1)
            num = TO_NUMBER(str_timestamp, 1) * 10;
        else
            num = TO_NUMBER(str_timestamp, 2);
        if (num < 1 || num > 31)
        {
            success = 0;
            goto RETURN_LABEL;
        }
        else
            astm.tm_mday = num;

        str_timestamp += 2;
        strlength -= 2;
        if (strlength <= 0)
            goto RETURN_LABEL;

        if (strlength == 1)
            num = TO_NUMBER(str_timestamp, 1) * 10;
        else
            num = TO_NUMBER(str_timestamp, 2);
        if (num > 23)
        {
            success = 0;
            goto RETURN_LABEL;
        }
        else
            astm.tm_hour = num;

        str_timestamp += 2;
        strlength -= 2;
        if (strlength <= 0)
            goto RETURN_LABEL;

        if (strlength == 1)
            num = TO_NUMBER(str_timestamp, 1) * 10;
        else
            num = TO_NUMBER(str_timestamp, 2);
        if (num > 59)
        {
            success = 0;
            goto RETURN_LABEL;
        }
        else
            astm.tm_min = num;

        str_timestamp += 2;
        strlength -= 2;
        if (strlength <= 0)
            goto RETURN_LABEL;

        if (strlength == 1)
            num = TO_NUMBER(str_timestamp, 1) * 10;
        else
            num = TO_NUMBER(str_timestamp, 2);
        if (num > 59)
        {
            success = 0;
            goto RETURN_LABEL;
        }
        else
            astm.tm_sec = num;

        str_timestamp += 2;
        strlength -= 2;
        if (strlength <= 0)
            goto RETURN_LABEL;

        if (strlength == 1)
            num = TO_NUMBER(str_timestamp, 1) * 100;
        if (strlength == 2)
            num = TO_NUMBER(str_timestamp, 2) * 10;
        else
            num = TO_NUMBER(str_timestamp, 3);
        if (num > 999)
        {
            success = 0;
            goto RETURN_LABEL;
        }
        else
            astm_ms = num;
    }
    else if (type == 1)
    {       // delimiter로 구분되어 있는 경우.
        if (strlength > MAX_TIMESTAMP_STRING_LEN - 1)
        {
            strlength = sc_strlen(str_timestamp);
            if (strlength > (MAX_TIMESTAMP_STRING_LEN - 1))
            {
                return NULL;
            }
        }

        deli = find_delimiter(str_timestamp, strlength);
/* CHECK YEAR RANGE */
        if (str_timestamp[deli] != '-')
            return NULL;

        if (deli == 0)  // bypass the year
            num = DEFAULT_START_YEAR * (-1);
        else
        {
            num = TO_NUMBER(str_timestamp, deli);
            if (num < 1 || num > 9999)
            {
                success = 0;
                goto RETURN_LABEL;
            }
            else
                num -= DEFAULT_START_YEAR;
        }
        astm.tm_year = num;

        str_timestamp += deli + 1;
        strlength -= deli + 1;
        if (strlength <= 0)
            goto RETURN_LABEL;

        // month
        deli = find_delimiter(str_timestamp, strlength);
        if (str_timestamp[deli] != '-')
            return NULL;

        num = TO_NUMBER(str_timestamp, deli);
        if (num < 1 || num > 12)
        {
            success = 0;
            goto RETURN_LABEL;
        }
        else
            astm.tm_mon = num - 1;
        if (deli < 0)
            goto RETURN_LABEL;

        str_timestamp += deli + 1;
        strlength -= deli + 1;
        if (strlength <= 0)
            goto RETURN_LABEL;

        // day
        deli = find_delimiter(str_timestamp, strlength);
        num = TO_NUMBER(str_timestamp, deli);
        if (num < 1 || num > 31)
        {
            success = 0;
            goto RETURN_LABEL;
        }
        else
            astm.tm_mday = num;
        if (deli < 0)
            goto RETURN_LABEL;

        str_timestamp += deli + 1;
        strlength -= deli + 1;
        if (strlength <= 0)
            goto RETURN_LABEL;

        // hour
        deli = find_delimiter(str_timestamp, strlength);
        if (deli != -1 && str_timestamp[deli] != ':')
            return NULL;

        num = TO_NUMBER(str_timestamp, deli);
        if (num > 23)
        {
            success = 0;
            goto RETURN_LABEL;
        }
        else
            astm.tm_hour = num;
        if (deli < 0)
            goto RETURN_LABEL;

        str_timestamp += deli + 1;
        strlength -= deli + 1;
        if (strlength <= 0)
            goto RETURN_LABEL;

        // minute
        deli = find_delimiter(str_timestamp, strlength);
        if (str_timestamp[deli] != ':')
            return NULL;

        num = TO_NUMBER(str_timestamp, deli);
        if (num > 59)
        {
            success = 0;
            goto RETURN_LABEL;
        }
        else
            astm.tm_min = num;
        if (deli < 0)
            goto RETURN_LABEL;

        str_timestamp += deli + 1;
        strlength -= deli + 1;
        if (strlength <= 0)
            goto RETURN_LABEL;

        // second
        deli = find_delimiter(str_timestamp, strlength);
        if (deli == -1 && str_timestamp[deli] == ':')
        {
        }
        else if (deli != -1 && str_timestamp[deli] == '.')
        {
        }
        else
            return NULL;

        num = TO_NUMBER(str_timestamp, deli);
        if (num > 59)
        {
            success = 0;
            goto RETURN_LABEL;
        }
        else
            astm.tm_sec = num;
        if (deli < 0)
            goto RETURN_LABEL;

        str_timestamp += deli + 1;
        strlength -= deli + 1;
        if (strlength <= 0)
            goto RETURN_LABEL;

        // milli-second
        num = TO_NUMBER(str_timestamp, 3);
        if (num > 999)
        {
            success = 0;
            goto RETURN_LABEL;
        }
        else
            astm_ms = num;
    }
    else if (type == 3)
    {       /* 00:00:00 */
        /* 12 is '00:00:00.000' */
        if (strlength > 12)
        {
            return NULL;
        }
        astm.tm_year = 0;
        // hour
        deli = find_delimiter(str_timestamp, strlength);
        num = TO_NUMBER(str_timestamp, deli);
        if (num > 23)
        {
            success = 0;
            goto RETURN_LABEL;
        }
        else
            astm.tm_hour = num;
        if (deli < 0)
            goto RETURN_LABEL;

        str_timestamp += deli + 1;
        strlength -= deli + 1;
        if (strlength <= 0)
            goto RETURN_LABEL;

        // minute
        deli = find_delimiter(str_timestamp, strlength);
        num = TO_NUMBER(str_timestamp, deli);
        if (num > 59)
        {
            success = 0;
            goto RETURN_LABEL;
        }
        else
            astm.tm_min = num;
        if (deli < 0)
            goto RETURN_LABEL;

        str_timestamp += deli + 1;
        strlength -= deli + 1;
        if (strlength <= 0)
            goto RETURN_LABEL;

        // second
        deli = find_delimiter(str_timestamp, strlength);
        num = TO_NUMBER(str_timestamp, deli);
        if (num > 59)
        {
            success = 0;
            goto RETURN_LABEL;
        }
        else
            astm.tm_sec = num;
    }
    else
    {
        return NULL;
    }

  RETURN_LABEL:
    if (success)
    {

        if (astm.tm_mday > sc_maxdayofmonth(astm.tm_year + DEFAULT_START_YEAR,
                        astm.tm_mon + 1))
            return NULL;
        *timestamp = mktime_as_2(&astm);
        *timestamp = (*timestamp) << 16 | astm_ms;
        return timestamp;
    }
    else
    {
        return NULL;
    }
}


/*****************************************************************************/
//! char2timest 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param timest : output
 * \param stime :  input
 * \param sleng :  input length
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
iSQL_TIME *
char2timest(iSQL_TIME * timest, char *stime, int sleng)
{
    int deli;

    sc_memcpy(timest, &__zero_timest, sizeof(iSQL_TIME));

    if (!sleng)
        return timest;

    // search any delimiter.
    deli = find_delimiter(stime, sleng);
    if (deli < 0)
        return timest;
    else
    {       // delimiter로 구분되어 있는 경우.
        if (stime[deli] != ':') // support time type "HH:MM:SS"
        {
            if (deli != 0)
                timest->year = TO_NUMBER(stime, deli);

            stime += deli + 1;
            sleng -= deli + 1;
            deli = find_delimiter(stime, sleng);

            timest->month = TO_NUMBER(stime, deli);

            stime += deli + 1;
            sleng -= deli + 1;
            deli = find_delimiter(stime, sleng);

            timest->day = TO_NUMBER(stime, deli);

            stime += deli + 1;
            sleng -= deli + 1;
            deli = find_delimiter(stime, sleng);
        }
        else
        {
            timest->year = timest->month = timest->day = 0;
        }
        if (deli < 0)
            return timest;
        timest->hour = TO_NUMBER(stime, deli);

        stime += deli + 1;
        sleng -= deli + 1;
        deli = find_delimiter(stime, sleng);

        if (deli < 0)
            return timest;
        timest->minute = TO_NUMBER(stime, deli);

        stime += deli + 1;
        sleng -= deli + 1;
        deli = find_delimiter(stime, sleng);

        timest->second = TO_NUMBER(stime, deli);

        if (deli < 0)
            return timest;      // only for DATETIME

        stime += deli + 1;

        timest->fraction = TO_NUMBER(stime, 3);
    }

    return timest;
}

iSQL_TIME *
timestamp2timest(iSQL_TIME * timest, t_astime * timestamp)
{
    t_astm astm;
    t_astime ast_sec;

    ast_sec = SEC_OF_TS(*timestamp);
    if (gmtime_as(&ast_sec, &astm) == NULL)
    {
        sc_memcpy(timest, &__zero_timest, sizeof(iSQL_TIME));
        return timest;
    }

    timest->year = astm.tm_year + DEFAULT_START_YEAR;
    timest->month = astm.tm_mon + 1;
    timest->day = astm.tm_mday;
    timest->hour = astm.tm_hour;
    timest->minute = astm.tm_min;
    timest->second = astm.tm_sec;
    timest->fraction = MSEC_OF_TS(*timestamp);

    return timest;
}

int
calc_interval_by_gregorian(T_INTERVAL int_type,
        bigint interval, t_astime * sec, t_astime * msec)
{
    t_astm ast_tm;
    t_astime ast_sec, ast_msec;

    ast_sec = *sec;
    ast_msec = *msec;

    if (int_type == INTERVAL_MILLI_SECOND)
    {
        ast_msec += interval;
        if (ast_msec < 0)
        {
            ast_sec--;
            ast_msec += 1000;
        }
        else
        {
            ast_sec += ast_msec / 1000;
            ast_msec %= 1000;
        }
    }
    else if (int_type == INTERVAL_SECOND)
    {
        ast_sec += interval;
    }
    else if (int_type == INTERVAL_MINUTE)
    {
        ast_sec += interval * 60;
    }
    else if (int_type == INTERVAL_HOUR)
    {
        ast_sec += interval * 3600;
    }
    else if (int_type == INTERVAL_DAY)
    {
        ast_sec += interval * 86400;
    }
    else if (int_type == INTERVAL_MONTH)
    {
        if (gmtime_as(&ast_sec, &ast_tm) != NULL)
        {
            ast_tm.tm_mon += interval;
            if (ast_tm.tm_mon == 0)
            {
                ast_tm.tm_mon = 0;
            }
            else if (ast_tm.tm_mon > 0)
            {
                ast_tm.tm_year += ast_tm.tm_mon / 12;
                ast_tm.tm_mon = ast_tm.tm_mon % 12;
            }
            else if (ast_tm.tm_mon < 0)
            {
                ast_tm.tm_year += ast_tm.tm_mon / 12 - 1;
                ast_tm.tm_mon = 12 + ast_tm.tm_mon % 12;
            }

            ast_sec = mktime_as_2(&ast_tm);
        }
        else
        {
            return RET_ERROR;
        }
    }
    else if (int_type == INTERVAL_YEAR)
    {
        if (gmtime_as(&ast_sec, &ast_tm) != NULL)
        {
            ast_tm.tm_year += (int) interval;
            ast_sec = mktime_as_2(&ast_tm);
        }
        else
        {
            return RET_ERROR;
        }
    }

    *sec = ast_sec;
    *msec = ast_msec;

    return RET_SUCCESS;
}

int
add_interval_to_timestamp(T_INTERVAL int_type,
        bigint interval, t_astime * sec, t_astime * msec)
{
    return calc_interval_by_gregorian(int_type, interval, sec, msec);
}

int
add_interval_by_convert(T_INTERVAL int_type, T_VALUEDESC * value,
        char *dest, int len)
{
    int ret;
    t_astime sec, msec;
    bigint interval = -1;
    t_astm astm = { 0, 0, 0, 1, 0, 0, 0, 0, 0 };
    char timestamp[MAX_TIMESTAMP_STRING_LEN];

    sc_memset(&sec, 0x0, sizeof(t_astime));
    sc_memset(&msec, 0x0, sizeof(t_astime));
    sc_memset(timestamp, 0x0, MAX_TIMESTAMP_STRING_LEN);

    sec = mktime_as_2(&astm);

    switch (value->valuetype)
    {
    case DT_TINYINT:
    case DT_SMALLINT:
    case DT_INTEGER:
    case DT_BIGINT:
        GET_INT_VALUE(interval, bigint, value);
        break;
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDARGUMENT, 0);
        return SQL_E_INVALIDARGUMENT;
    }

    if (int_type == INTERVAL_SECOND)
    {
        if (interval > 86399)
        {
            return RET_ERROR;
        }
    }

    /* now, supported for gregorian */
    ret = add_interval_to_timestamp(int_type, interval, &sec, &msec);
    if (ret < 0)
    {
        return RET_ERROR;
    }

    sec <<= 16;
    sec |= msec & 0x000000000000FFFF;

    timestamp2char(timestamp, &sec);

    if (int_type == INTERVAL_SECOND)
    {
        sc_memcpy(dest, timestamp + MAX_DATE_STRING_LEN, len - 1);
    }
    else
    {
        /* the len includes a null terminator.
         * so copies the data from timestamp array. 
         */
        sc_memcpy(dest, timestamp, len - 1);
    }

    return RET_SUCCESS;
}
