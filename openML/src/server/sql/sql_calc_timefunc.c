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

#include "isql.h"
#include "sql_datast.h"
#include "ErrorCode.h"
#include "sql_util.h"
#include "sql_func_timeutil.h"
#include "mdb_er.h"

#include "mdb_Server.h"

static char *month_names[] = { "January", "February", "March", "April",
    "May", "June", "July", "August", "September",
    "October", "November", "December", "Unknown"
};

static char *day_names[] = { "Monday", "Tuesday", "Wednesday", "Thursday",
    "Friday", "Saturday", "Sunday", "Unknown"
};

#if defined(WIN32)
#define START_TIMESTAMP4TIME -144768289996800
#define END_TIMESTAMP4TIME   -144762627750937
#else
#define START_TIMESTAMP4TIME -144768289996800LL
#define END_TIMESTAMP4TIME   -144762627750937LL
#endif


/*****************************************************************************/
//! value2astime 

/*! \breif SQL의 인자를 time으로 변환해주는 함수
 ************************************
 * \param arg(in)        : SQL's argument
 * \param sec(out)        : seconds
 * \param msec(out)    : milli seconds
 ************************************
 * \return  RET_ERROR or RET_SUCCESS
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
value2astime(T_VALUEDESC * arg /*in */ , t_astime * sec /*out */ ,
        t_astime * msec /*out */ )
{
    char datetime[MAX_DATETIME_STRING_LEN];
    char date[MAX_DATE_STRING_LEN];
    t_astime *ptr;

    if (arg == NULL || sec == NULL || msec == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
        return RET_ERROR;
    }

    if (arg->valuetype == DT_CHAR)
    {
        ptr = char2timestamp(sec, arg->u.ptr, sc_strlen(arg->u.ptr));
        if (ptr == NULL)
        {
            /* 
             * note:
             * 일단, 변환이 안되면 0을 반환.
             * value2astime 함수를 여러 곳에서 호출하므로,
             * 호출하는 쪽을 파악하여 적절하게 변경하도록 하자.
             * 현재까지 함수 반환 검사를  < 0 으로만 하니
             * 기존 구현 변경에 영향을 미치지 않도록 하는 범위에서
             * 임시적으로 0을 반환하여 문제 상황을 해결.
             */
            return 0;
        }
    }
    else if (arg->valuetype == DT_VARCHAR)
    {
        ptr = char2timestamp(sec, arg->u.ptr, sc_strlen(arg->u.ptr));
        if (ptr == NULL)
        {
            return 0;
        }
    }
    else if (arg->valuetype == DT_DATETIME)
    {
        datetime2char(datetime, arg->u.datetime);
        ptr = char2timestamp(sec, datetime, sc_strlen(datetime));
        if (ptr == NULL)
        {
            return 0;
        }
    }
    else if (arg->valuetype == DT_DATE)
    {
        date2char(date, arg->u.date);
        ptr = char2timestamp(sec, date, sc_strlen(date));
        if (ptr == NULL)
        {
            return 0;
        }
    }
    /* DT_TIME cannot convert to timestamp,
     *  but ... */
    else if (arg->valuetype == DT_TIME)
    {
        time2timestamp(sec, arg->u.time);
    }
    else if (arg->valuetype == DT_TIMESTAMP)
    {
        sc_memcpy(sec, &arg->u.timestamp, MAX_TIMESTAMP_FIELD_LEN);
    }

    *msec = MSEC_OF_TS(*sec);
    *sec = SEC_OF_TS(*sec);

    return RET_SUCCESS;
}


/*****************************************************************************/
//! get_format_length 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param format :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static unsigned int
get_format_length(const char *format)
{
    unsigned int size = 0;

    for (; *format != '\0'; format++)
    {
        if (*format != '%')
            size++;
        else
        {
            switch (*++format)
            {
            case 'D':  // Day of the month [00 ~ 31]
            case 'd':  // Day of the month [1 ~ 31]
            case 'H':  // Hour of day [00 ~ 23]
            case 'h':  // Hour of day [01 ~ 12]
            case 'k':  // Hour of day [0 ~ 23]
            case 'l':  // Hour of day [1 ~ 12]
            case 'I':  // Minute of hour [00 ~ 59]
            case 'i':  // Minute of hour [0 ~ 59]
            case 'p':  // AM or PM
            case 'S':  // Second of minute [00 ~ 59]
            case 's':  // Second of minute [0 ~ 59]
            case 'M':  // Month [01 ~ 12]
            case 'm':  // Month [1 ~ 12]
            case 'y':  // Year within century [00 ~ 99]
                size += 2;
                break;
            case 'a':  // Abbreviated weekday name [Sun ~ Sat]
            case 'b':  // Abbreviated month name [Jan ~ Dec]
            case 'E':  // milli-second of second [000 ~ 999]
            case 'e':  // milli-second of second [0 ~ 999]
            case 'j':  // Day number of year [001 ~ 366]
                size += 3;
                break;
            case 'Y':  // Year including century [0000 ~ 9999]
                size += 4;
                break;
            case 'A':  // Full weekday name [Sunday ~ Saturday]
            case 'B':  // Full month name [January ~ December]
                size += 9;
                break;
            case 'w':  // Day of week [0:Sunday ~ 6:Saturday]
            case '%':
                size++;
                break;
            }
        }
    }
    ++size;

    return size;
}

/*****************************************************************************/
//! calc_func_current_date 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
calc_func_current_date(T_VALUEDESC * res, MDB_INT32 is_init)
{
    struct db_tm ltm;
    db_time_t l_clock;
    char date[MAX_DATE_STRING_LEN];
    int pos;

    struct db_timeval tv;

    res->valueclass = SVC_CONSTANT;
    res->valuetype = DT_DATE;
    res->is_null = 0;
    res->attrdesc.collation = MDB_COL_NUMERIC;
    if (is_init)
        return RET_SUCCESS;

    sc_gettimeofday(&tv, NULL);
    l_clock = tv.tv_sec;

    sc_localtime_r(&l_clock, &ltm);

    pos = sc_sprintf(date, "%.4d", ltm.tm_year + 1900);
    pos += sc_sprintf(date + pos, "-%02d", ltm.tm_mon + 1);
    pos += sc_sprintf(date + pos, "-%02d", ltm.tm_mday);

    char2date(res->u.date, date, sc_strlen(date));
    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_current_time 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
calc_func_current_time(T_VALUEDESC * res, MDB_INT32 is_init)
{
    struct db_tm ltm;
    db_time_t l_clock;
    char time[MAX_TIME_STRING_LEN];
    int pos;

    struct db_timeval tv;

    res->valueclass = SVC_CONSTANT;
    res->valuetype = DT_TIME;
    res->is_null = 0;
    res->attrdesc.collation = MDB_COL_NUMERIC;
    if (is_init)
        return RET_SUCCESS;

    sc_gettimeofday(&tv, NULL);
    l_clock = tv.tv_sec;

    sc_localtime_r(&l_clock, &ltm);

    pos = sc_sprintf(time, "%02d", ltm.tm_hour);
    pos += sc_sprintf(time + pos, ":%02d", ltm.tm_min);
    pos += sc_sprintf(time + pos, ":%02d", ltm.tm_sec);

    char2time(res->u.time, time, sc_strlen(time));
    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_current_timestamp 

/*! \breif  CURRENT_TIMESTAMP를 구하는 함수
 ************************************
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE :  res's type check, MDB_FALSE : 실제 수행
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *  - execution시에 time이 결정되도록 수정함
 *  - collation 할당
 *****************************************************************************/
int
calc_func_current_timestamp(T_VALUEDESC * res, MDB_INT32 is_init)
{
    struct db_tm ltm;
    db_time_t l_clock;
    int msec;
    char timestamp[MAX_TIMESTAMP_STRING_LEN];
    int pos;

    struct db_timeval tv;

    res->valueclass = SVC_CONSTANT;
    res->valuetype = DT_TIMESTAMP;
    res->is_null = 0;
    res->attrdesc.collation = MDB_COL_NUMERIC;
    if (is_init)
        return RET_SUCCESS;

    sc_gettimeofday(&tv, NULL);
    l_clock = tv.tv_sec;
    msec = (int) (tv.tv_usec / 1000);

    sc_localtime_r(&l_clock, &ltm);

    pos = sc_sprintf(timestamp, "%.4d", ltm.tm_year + 1900);
    pos += sc_sprintf(timestamp + pos, "-%02d", ltm.tm_mon + 1);
    pos += sc_sprintf(timestamp + pos, "-%02d", ltm.tm_mday);
    pos += sc_sprintf(timestamp + pos, " %02d", ltm.tm_hour);
    pos += sc_sprintf(timestamp + pos, ":%02d", ltm.tm_min);
    pos += sc_sprintf(timestamp + pos, ":%02d", ltm.tm_sec);
    pos += sc_sprintf(timestamp + pos, ".%03d", msec);

    char2timestamp(&res->u.timestamp, timestamp, sc_strlen(timestamp));

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_date_add

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param arg1(in)      :
 * \param arg2(in)      : 
 * \param arg3(in)      :
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(res's type check)/MDB_FALSE(실제 수행)
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - execution시에 time이 결정되도록 수정함
 *  - collation 추가
 *  - parameter binding시 res type이 NULL로 넘어가는 문제 해결
 *****************************************************************************/
int
calc_func_date_add(T_VALUEDESC * arg1, T_VALUEDESC * arg2,
        T_VALUEDESC * arg3, T_VALUEDESC * res, MDB_INT32 is_init)
{
    t_astime ast_sec, ast_msec;
    T_INTERVAL int_type;
    bigint interval;
    char timestamp[MAX_TIMESTAMP_STRING_LEN];
    int ret = -1;

    int_type = get_interval_value(arg1);
    if (int_type == INTERVAL_NONE)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDARGUMENT, 0);
        return SQL_E_INVALIDARGUMENT;
    }

    if (arg1->valueclass == SVC_VARIABLE
            || arg2->valueclass == SVC_VARIABLE
            || arg3->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    if (IS_BS(arg3->valuetype))
        res->valuetype = DT_TIMESTAMP;
    else if (arg3->valuetype == DT_DATETIME)
        res->valuetype = DT_DATETIME;
    else if (arg3->valuetype == DT_DATE)
        res->valuetype = DT_DATE;
    else if (arg3->valuetype == DT_TIME)
        res->valuetype = DT_TIME;
    else
        res->valuetype = DT_TIMESTAMP;

    res->attrdesc.collation = arg3->attrdesc.collation;

    if (arg2->is_null || arg3->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (is_init)
        return RET_SUCCESS;

    switch (arg2->valuetype)
    {
    case DT_TINYINT:
    case DT_SMALLINT:
    case DT_INTEGER:
    case DT_BIGINT:
        GET_INT_VALUE(interval, bigint, arg2);
        break;
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDARGUMENT, 0);
        return SQL_E_INVALIDARGUMENT;
    }

    if (res->valuetype == DT_TIME
            && !(int_type == INTERVAL_MILLI_SECOND
                    || int_type == INTERVAL_SECOND))
    {
        goto RESULT_PROCESSING;
    }

    ret = value2astime(arg3, &ast_sec, &ast_msec);
    if (ret < 0)
    {
        return RET_ERROR;
    }
    else if (ret == 0)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    ret = add_interval_to_timestamp(int_type, interval, &ast_sec, &ast_msec);
    if (ret < 0)
    {
        return RET_ERROR;
    }

    if (ast_sec < START_SEC || ast_sec > LAST_SEC)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

  RESULT_PROCESSING:

    if (res->valuetype == DT_DATETIME)
    {
        ast_sec <<= 16;
        ast_sec |= ast_msec & 0x000000000000FFFF;

        timestamp2char(timestamp, &ast_sec);
        char2datetime(res->u.datetime, timestamp, sc_strlen(timestamp));
    }
    else if (res->valuetype == DT_DATE)
    {
        ast_sec <<= 16;
        ast_sec |= ast_msec & 0x000000000000FFFF;

        timestamp2char(timestamp, &ast_sec);
        char2date(res->u.date, timestamp, sc_strlen(timestamp));
    }
    else if (res->valuetype == DT_TIME)
    {
        if (int_type == INTERVAL_MILLI_SECOND || int_type == INTERVAL_SECOND)
        {
            ast_sec <<= 16;
            ast_sec |= ast_msec & 0x000000000000FFFF;

            if (ast_sec < START_TIMESTAMP4TIME || ast_sec > END_TIMESTAMP4TIME)
            {
                res->is_null = 1;
                return RET_SUCCESS;
            }


            timestamp2char(timestamp, &ast_sec);
            char2time(res->u.time, timestamp, sc_strlen(timestamp));
        }
        else
        {
            sc_memcpy(res->u.time, arg3->u.time, MAX_TIME_FIELD_LEN);
        }
    }
    else if (res->valuetype == DT_TIMESTAMP)
    {
        res->u.timestamp = (ast_sec << 16) | (ast_msec & 0x000000000000FFFF);
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_date_diff

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param arg1(in)      :
 * \param arg2(in)      : 
 * \param arg3(in)      :
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(res's type check)/MDB_FALSE(실제 수행)
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - execution시에 time이 결정되도록 수정함
 *  - collation 추가
 *****************************************************************************/
int
calc_func_date_diff(T_VALUEDESC * arg1,
        T_VALUEDESC * arg2, T_VALUEDESC * arg3,
        T_VALUEDESC * res, MDB_INT32 is_init)
{
    t_astm left_tm, right_tm;
    t_astime left_sec, left_msec, right_sec, right_msec;
    T_INTERVAL int_type;

    int_type = get_interval_value(arg1);
    if (int_type == INTERVAL_NONE)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDARGUMENT, 0);
        return SQL_E_INVALIDARGUMENT;
    }

    if (arg1->valueclass == SVC_VARIABLE
            || arg2->valueclass == SVC_VARIABLE
            || arg3->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->valuetype = DT_BIGINT;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (arg2->is_null || arg3->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (is_init)
        return RET_SUCCESS;

    left_sec = 0;
    left_msec = 0;
    right_sec = 0;
    right_msec = 0;

    value2astime(arg2, &right_sec, &right_msec);
    value2astime(arg3, &left_sec, &left_msec);

    if (int_type == INTERVAL_MILLI_SECOND)
    {
        res->u.i64 =
                (left_sec * 1000 + left_msec) - (right_sec * 1000 +
                right_msec);
    }
    else if (int_type == INTERVAL_SECOND)
    {
        res->u.i64 =
                ((left_sec * 1000 + left_msec) - (right_sec * 1000 +
                        right_msec)) / 1000;
    }
    else if (int_type == INTERVAL_MINUTE)
    {
        res->u.i64 =
                ((left_sec * 1000 + left_msec) - (right_sec * 1000 +
                        right_msec)) / 60000;
    }
    else if (int_type == INTERVAL_HOUR)
    {
        res->u.i64 =
                ((left_sec * 1000 + left_msec) - (right_sec * 1000 +
                        right_msec)) / 3600000;
    }
    else if (int_type == INTERVAL_DAY)
    {
        res->u.i64 =
                ((left_sec * 1000 + left_msec) - (right_sec * 1000 +
                        right_msec)) / 86400000;
    }
    else if (int_type == INTERVAL_MONTH)
    {
        gmtime_as(&left_sec, &left_tm);
        gmtime_as(&right_sec, &right_tm);
        res->u.i64 =
                ((left_tm.tm_year + DEFAULT_START_YEAR) * 12 + left_tm.tm_mon +
                1) - ((right_tm.tm_year + DEFAULT_START_YEAR) * 12 +
                right_tm.tm_mon + 1);
    }
    else if (int_type == INTERVAL_YEAR)
    {
        gmtime_as(&left_sec, &left_tm);
        gmtime_as(&right_sec, &right_tm);
        res->u.i64 = left_tm.tm_year - right_tm.tm_year;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_date_format

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param arg1(in)      :
 * \param arg2(in)      : 
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(res's type check)/MDB_FALSE(실제 수행)
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - 초기화 변수를 추가한다
 *  - collation 추가
 *****************************************************************************/
int
calc_func_date_format(T_VALUEDESC * arg1, T_VALUEDESC * arg2,
        T_VALUEDESC * res, MDB_INT32 is_init)
{
    t_astm ast_tm;
    t_astime ast_sec, ast_msec;
    char datetime[MAX_DATETIME_STRING_LEN];
    char date[MAX_DATE_STRING_LEN];
    char timestamp[MAX_TIMESTAMP_STRING_LEN];
    char *ptr, *end, *str;
    char *format;
    unsigned int posi, max_len;
    int weekday, day_of_year;
    int num;

    switch (arg2->valuetype)
    {
    case DT_CHAR:
    case DT_VARCHAR:
        format = arg2->u.ptr;
        break;
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDARGUMENT, 0);
        return SQL_E_INVALIDARGUMENT;
    }

    max_len = get_format_length(format) + 1;

    if (arg1->valueclass == SVC_VARIABLE || arg2->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->valuetype = DT_VARCHAR;
    res->attrdesc.len = max_len - 1;
    res->attrdesc.collation = DB_Params.default_col_char;

    if (arg1->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (is_init)
        return RET_SUCCESS;

    if (sql_value_ptrXcalloc(res, max_len) < 0)
        return SQL_E_OUTOFMEMORY;

    ptr = timestamp;
    switch (arg1->valuetype)
    {
    case DT_TINYINT:
    case DT_SMALLINT:
    case DT_INTEGER:
    case DT_BIGINT:
    case DT_FLOAT:
    case DT_DOUBLE:
    case DT_DECIMAL:
        print_value_as_string(arg1, timestamp, MAX_TIMESTAMP_STRING_LEN);
        break;
    case DT_CHAR:
    case DT_VARCHAR:
        ptr = arg1->u.ptr;
        break;
    case DT_DATETIME:
        datetime2char(datetime, arg1->u.datetime);
        ptr = datetime;
        break;
    case DT_DATE:
        date2char(date, arg1->u.date);
        ptr = date;
        break;
        /* DT_TIME cannot convert to timestamp */
    case DT_TIMESTAMP:
        ast_sec = arg1->u.timestamp;
        break;
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDARGUMENT, 0);
        return SQL_E_INVALIDARGUMENT;
    }

    if (arg1->valuetype != DT_TIMESTAMP)
        char2timestamp(&ast_sec, ptr, sc_strlen(ptr));

    ast_msec = MSEC_OF_TS(ast_sec);
    ast_sec = SEC_OF_TS(ast_sec);

    if (gmtime_as(&ast_sec, &ast_tm) == NULL)
        return RET_ERROR;

    str = format;
    ptr = res->u.ptr;
    end = ptr + sc_strlen(ptr);
    posi = 0;
    for (; *str != '\0'; str++)
    {
        if (*str != '%' || str == end - 1)
            ptr[posi++] = *str;
        else
        {
            switch (*++str)
            {
            case 'D':  // Day of the month [00 ~ 31]
                ptr[posi++] = ast_tm.tm_mday / 10 + '0';
                ptr[posi++] = ast_tm.tm_mday % 10 + '0';
                break;
            case 'd':  // Day of the month [1 ~ 31]
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%d", ast_tm.tm_mday);
                break;
            case 'H':  // Hour of day [00 ~ 23]
                ptr[posi++] = ast_tm.tm_hour / 10 + '0';
                ptr[posi++] = ast_tm.tm_hour % 10 + '0';
                break;
            case 'h':  // Hour of day [01 ~ 12]
                num = (ast_tm.tm_hour + 11) % 12 + 1;
                ptr[posi++] = num / 10 + '0';
                ptr[posi++] = num % 10 + '0';
                break;
            case 'k':  // Hour of day [0 ~ 23]
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%d", ast_tm.tm_hour);
                break;
            case 'l':  // Hour of day [1 ~ 12]
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%d", (ast_tm.tm_hour + 11) % 12 + 1);
                break;
            case 'I':  // Minute of hour [00 ~ 59]
                ptr[posi++] = ast_tm.tm_min / 10 + '0';
                ptr[posi++] = ast_tm.tm_min % 10 + '0';
                break;
            case 'i':  // Minute of hour [0 ~ 59]
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%d", ast_tm.tm_min);
                break;
            case 'p':  // AM or PM
                if (ast_tm.tm_hour < 12)
                    ptr[posi++] = 'A';
                else
                    ptr[posi++] = 'P';
                ptr[posi++] = 'M';
                break;
            case 'S':  // Second of minute [00 ~ 59]
                ptr[posi++] = ast_tm.tm_sec / 10 + '0';
                ptr[posi++] = ast_tm.tm_sec % 10 + '0';
                break;
            case 's':  // Second of minute [0 ~ 59]
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%d", ast_tm.tm_sec);
                break;
            case 'M':  // Month [01 ~ 12]
                ptr[posi++] = (ast_tm.tm_mon + 1) / 10 + '0';
                ptr[posi++] = (ast_tm.tm_mon + 1) % 10 + '0';
                break;
            case 'm':  // Month [1 ~ 12]
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%d", ast_tm.tm_mon + 1);
                break;
            case 'y':  // Year within century [00 ~ 99]
                num = (ast_tm.tm_year + DEFAULT_START_YEAR) % 100;
                ptr[posi++] = num / 10 + '0';
                ptr[posi++] = num % 10 + '0';
                break;
            case 'a':  // Abbreviated weekday name [Sun ~ Sat]
                weekday = get_weekday(ast_tm.tm_year + DEFAULT_START_YEAR,
                        ast_tm.tm_mon + 1, ast_tm.tm_mday);
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%.*s", 3, day_names[weekday]);
                break;
            case 'E':  // milli-second of second [000 ~ 999]
                posi += sc_snprintf(ptr + posi, max_len - posi, I64_FORMAT3,
                        ast_msec);
                break;
            case 'e':  // milli-second of second [0 ~ 999]
                posi += sc_snprintf(ptr + posi, max_len - posi, I64_FORMAT,
                        ast_msec);
                break;
            case 'j':  // Day number of year [001 ~ 366]
                day_of_year =
                        get_day_of_year(ast_tm.tm_year + DEFAULT_START_YEAR,
                        ast_tm.tm_mon + 1, ast_tm.tm_mday);
                num = day_of_year;
                ptr[posi++] = num / 100 + '0';
                num = num % 100;
                ptr[posi++] = num / 10 + '0';
                ptr[posi++] = num % 10 + '0';
                break;
            case 'Y':  // Year including century [0000 ~ 9999]
                num = ast_tm.tm_year + DEFAULT_START_YEAR;
                ptr[posi++] = num / 1000 + '0';
                num = num % 1000;
                ptr[posi++] = num / 100 + '0';
                num = num % 100;
                ptr[posi++] = num / 10 + '0';
                ptr[posi++] = num % 10 + '0';
                break;
            case 'A':  // Full weekday name [Sunday ~ Saturday]
                weekday = get_weekday(ast_tm.tm_year + DEFAULT_START_YEAR,
                        ast_tm.tm_mon + 1, ast_tm.tm_mday);
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%s", day_names[weekday]);
                break;
            case 'B':  // Full month name [January ~ December]
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%s", month_names[ast_tm.tm_mon]);
                break;
            case 'b':  // Abbreviated month name [Jan ~ Dec]
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%.*s", 3, month_names[ast_tm.tm_mon]);
                break;
            case 'w':  // day of the week [0:Sunday ~ 6;Saturday]
                weekday = get_weekday(ast_tm.tm_year + DEFAULT_START_YEAR,
                        ast_tm.tm_mon + 1, ast_tm.tm_mday);
                posi += sc_snprintf(ptr + posi, max_len - posi, "%d", weekday);
                break;
            case '%':
                ptr[posi++] = *str;
                break;
            }
        }
    }

    res->value_len = sc_strlen(ptr);

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_date_sub 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param arg1(in)  :
 * \param arg2(in)  : 
 * \param arg3(in)  :
 * \param res(in/out) :
 * \param is_init(in) : MDB_TRUE :  res's type check, MDB_FALSE : 실제 수행
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - 실제 수행은 1번만 일어나도록 수정
 *  - collation 추가
 *  - parameter binding시 res type이 NULL로 넘어가는 문제 해결
 *****************************************************************************/
int
calc_func_date_sub(T_VALUEDESC * arg1,
        T_VALUEDESC * arg2, T_VALUEDESC * arg3,
        T_VALUEDESC * res, MDB_INT32 is_init)
{
    t_astime ast_sec, ast_msec;
    T_INTERVAL int_type;
    bigint interval;
    char timestamp[MAX_TIMESTAMP_STRING_LEN];
    int ret = -1;

    int_type = get_interval_value(arg1);
    if (int_type == INTERVAL_NONE)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDARGUMENT, 0);
        return SQL_E_INVALIDARGUMENT;
    }

    if (arg1->valueclass == SVC_VARIABLE
            || arg2->valueclass == SVC_VARIABLE
            || arg3->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    if (IS_BS(arg3->valuetype))
        res->valuetype = DT_TIMESTAMP;
    else if (arg3->valuetype == DT_DATETIME)
        res->valuetype = DT_DATETIME;
    else if (arg3->valuetype == DT_DATE)
        res->valuetype = DT_DATE;
    else if (arg3->valuetype == DT_TIME)
        res->valuetype = DT_TIME;
    else
        res->valuetype = DT_TIMESTAMP;

    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (arg2->is_null || arg3->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (is_init)
        return RET_SUCCESS;

    switch (arg2->valuetype)
    {
    case DT_TINYINT:
    case DT_SMALLINT:
    case DT_INTEGER:
    case DT_BIGINT:
        GET_INT_VALUE(interval, bigint, arg2);
        break;
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDARGUMENT, 0);
        return SQL_E_INVALIDARGUMENT;
    }

    if (res->valuetype == DT_TIME
            && !(int_type == INTERVAL_MILLI_SECOND
                    || int_type == INTERVAL_SECOND))
    {
        goto RESULT_PROCESSING;
    }

    ret = value2astime(arg3, &ast_sec, &ast_msec);
    if (ret < 0)
    {
        return RET_ERROR;
    }
    else if (ret == 0)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    ret = add_interval_to_timestamp(int_type, -(interval),
            &ast_sec, &ast_msec);
    if (ret < 0)
    {
        return RET_ERROR;
    }

    if (ast_sec < START_SEC || ast_sec > LAST_SEC)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

  RESULT_PROCESSING:

    if (res->valuetype == DT_DATETIME)
    {
        ast_sec <<= 16;
        ast_sec |= ast_msec & 0x000000000000FFFF;
        timestamp2char(timestamp, &ast_sec);
        char2datetime(res->u.datetime, timestamp, sc_strlen(timestamp));
    }
    else if (res->valuetype == DT_DATE)
    {
        ast_sec <<= 16;
        ast_sec |= ast_msec & 0x000000000000FFFF;
        timestamp2char(timestamp, &ast_sec);
        char2date(res->u.date, timestamp, sc_strlen(timestamp));
    }
    else if (res->valuetype == DT_TIME)
    {
        if (int_type == INTERVAL_MILLI_SECOND || int_type == INTERVAL_SECOND)
        {
            ast_sec <<= 16;
            ast_sec |= ast_msec & 0x000000000000FFFF;

            if (ast_sec < START_TIMESTAMP4TIME || ast_sec > END_TIMESTAMP4TIME)
            {
                res->is_null = 1;
                return RET_SUCCESS;
            }

            timestamp2char(timestamp, &ast_sec);
            char2time(res->u.time, timestamp, sc_strlen(timestamp));
        }
        else
        {
            sc_memcpy(res->u.time, arg3->u.time, MAX_TIME_FIELD_LEN);
        }
    }
    else if (res->valuetype == DT_TIMESTAMP)
        res->u.timestamp = (ast_sec << 16) | (ast_msec & 0x000000000000FFFF);

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_time_add

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param arg1(in)      :
 * \param arg2(in)      : 
 * \param arg3(in)      :
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(res's type check)/MDB_FALSE(실제 수행)
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - execution시에 time이 결정되도록 수정함
 *  - collation 추가
 *  - parameter binding시 res type이 NULL로 넘어가는 문제 해결
 *****************************************************************************/
int
calc_func_time_add(T_VALUEDESC * arg1, T_VALUEDESC * arg2,
        T_VALUEDESC * arg3, T_VALUEDESC * res, MDB_INT32 is_init)
{
    t_astime ast_sec, ast_msec;
    T_INTERVAL int_type;
    bigint interval;
    char timestamp[MAX_TIMESTAMP_STRING_LEN];

    int_type = get_interval_value(arg1);
    switch (int_type)
    {
    case INTERVAL_HOUR:
    case INTERVAL_MINUTE:
    case INTERVAL_SECOND:
        break;
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDARGUMENT, 0);
        return SQL_E_INVALIDARGUMENT;
    }

    if (arg1->valueclass == SVC_VARIABLE
            || arg2->valueclass == SVC_VARIABLE
            || arg3->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    if (IS_BS(arg3->valuetype))
        res->valuetype = DT_TIMESTAMP;
    else if (arg3->valuetype == DT_DATETIME)
        res->valuetype = DT_DATETIME;
    else if (arg3->valuetype == DT_DATE)
        res->valuetype = DT_DATE;
    else if (arg3->valuetype == DT_TIME)
        res->valuetype = DT_TIME;
    else
        res->valuetype = DT_TIMESTAMP;


    res->attrdesc.collation = arg3->attrdesc.collation;

    if (arg2->is_null || arg3->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (is_init)
        return RET_SUCCESS;

    switch (arg2->valuetype)
    {
    case DT_TINYINT:
    case DT_SMALLINT:
    case DT_INTEGER:
    case DT_BIGINT:
        GET_INT_VALUE(interval, bigint, arg2);
        break;
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDARGUMENT, 0);
        return SQL_E_INVALIDARGUMENT;
    }

    if (value2astime(arg3, &ast_sec, &ast_msec) < 0)
        return RET_ERROR;

    if (int_type == INTERVAL_MILLI_SECOND)
    {
        ast_msec += interval;
        ast_sec += ast_msec / 1000;
        ast_msec %= 1000;
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

    if (res->valuetype == DT_DATETIME)
    {
        ast_sec <<= 16;
        ast_sec |= ast_msec & 0x000000000000FFFF;
        timestamp2char(timestamp, &ast_sec);
        char2datetime(res->u.datetime, timestamp, sc_strlen(timestamp));
    }
    else if (res->valuetype == DT_DATE)
    {
        ast_sec <<= 16;
        ast_sec |= ast_msec & 0x000000000000FFFF;
        timestamp2char(timestamp, &ast_sec);
        char2date(res->u.date, timestamp, sc_strlen(timestamp));
    }
    else if (res->valuetype == DT_TIME)
    {
        ast_sec <<= 16;
        ast_sec |= ast_msec & 0x000000000000FFFF;
        timestamp2char(timestamp, &ast_sec);
        char2time(res->u.time, timestamp, sc_strlen(timestamp));
    }
    else if (res->valuetype == DT_TIMESTAMP)
        res->u.timestamp = (ast_sec << 16) | (ast_msec & 0x000000000000FFFF);

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_time_diff

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param arg1(in)      :
 * \param arg2(in)      : 
 * \param arg3(in)      :
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(res's type check)/MDB_FALSE(실제 수행)
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - execution시에 time이 결정되도록 수정함
 *  - collation 추가
 *****************************************************************************/
int
calc_func_time_diff(T_VALUEDESC * arg1,
        T_VALUEDESC * arg2, T_VALUEDESC * arg3,
        T_VALUEDESC * res, MDB_INT32 is_init)
{
    t_astime left_sec, left_msec, right_sec, right_msec;
    T_INTERVAL int_type;

    int_type = get_interval_value(arg1);
    switch (int_type)
    {
    case INTERVAL_HOUR:
    case INTERVAL_MINUTE:
    case INTERVAL_SECOND:
        break;
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDARGUMENT, 0);
        return SQL_E_INVALIDARGUMENT;
    }

    if (arg1->valueclass == SVC_VARIABLE
            || arg2->valueclass == SVC_VARIABLE
            || arg3->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->valuetype = DT_BIGINT;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (arg2->is_null || arg3->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (is_init)
        return RET_SUCCESS;

    value2astime(arg2, &right_sec, &right_msec);
    value2astime(arg3, &left_sec, &left_msec);

    if (int_type == INTERVAL_MILLI_SECOND)
    {
        res->u.i64 =
                (left_sec * 1000 + left_msec) - (right_sec * 1000 +
                right_msec);
    }
    else if (int_type == INTERVAL_SECOND)
    {
        res->u.i64 =
                ((left_sec * 1000 + left_msec) - (right_sec * 1000 +
                        right_msec)) / 1000;
    }
    else if (int_type == INTERVAL_MINUTE)
    {
        res->u.i64 =
                ((left_sec * 1000 + left_msec) - (right_sec * 1000 +
                        right_msec)) / 60000;
    }
    else if (int_type == INTERVAL_HOUR)
    {
        res->u.i64 =
                ((left_sec * 1000 + left_msec) - (right_sec * 1000 +
                        right_msec)) / 3600000;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_time_format

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param arg1(in)      :
 * \param arg2(in)      : 
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(res's type check)/MDB_FALSE(실제 수행)
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - 초기화 변수를 추가한다
 *  - collation 추가
 *****************************************************************************/
int
calc_func_time_format(T_VALUEDESC * arg1, T_VALUEDESC * arg2,
        T_VALUEDESC * res, MDB_INT32 is_init)
{
    t_astm ast_tm;
    t_astime ast_sec;
    char timestamp[MAX_TIMESTAMP_STRING_LEN];
    char *ptr, *end, *str;
    char format[MDB_BUFSIZ];
    unsigned int posi, max_len;

    switch (arg2->valuetype)
    {
    case DT_CHAR:
    case DT_VARCHAR:
        sc_strncpy(format, arg2->u.ptr, sizeof(format) - 1);
        break;
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDARGUMENT, 0);
        return SQL_E_INVALIDARGUMENT;
    }

    max_len = get_format_length(format) + 1;

    if (arg1->valueclass == SVC_VARIABLE || arg2->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->valuetype = DT_VARCHAR;
    res->attrdesc.len = max_len - 1;
    res->attrdesc.collation = DB_Params.default_col_char;

    if (arg1->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (is_init)
        return RET_SUCCESS;

    if (sql_value_ptrXcalloc(res, max_len) < 0)
        return SQL_E_OUTOFMEMORY;


    ptr = timestamp;

    switch (arg1->valuetype)
    {
    case DT_TINYINT:
    case DT_SMALLINT:
    case DT_INTEGER:
    case DT_BIGINT:
    case DT_FLOAT:
    case DT_DOUBLE:
    case DT_DECIMAL:
        print_value_as_string(arg1, timestamp, MAX_TIMESTAMP_STRING_LEN);
        break;
    case DT_CHAR:
    case DT_VARCHAR:
        ptr = arg1->u.ptr;
        break;
    case DT_TIME:
        time2timestamp(&ast_sec, arg1->u.time);
        break;
    default:
        /*  DATE, DATETIME and TIMESTAMP can not convert to TIME */
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDARGUMENT, 0);
        return SQL_E_INVALIDARGUMENT;
    }

    if (arg1->valuetype != DT_TIME)
        char2timestamp(&ast_sec, ptr, sc_strlen(ptr));

    ast_sec = SEC_OF_TS(ast_sec);

    if (gmtime_as(&ast_sec, &ast_tm) == NULL)
        return RET_ERROR;

    str = format;
    ptr = res->u.ptr;
    end = ptr + sc_strlen(ptr);
    posi = 0;
    for (; *str != '\0'; str++)
    {
        if (*str != '%' || str == end - 1)
            ptr[posi++] = *str;
        else
        {
            switch (*++str)
            {
            case 'H':  // Hour of day [00 ~ 23]
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%02d", ast_tm.tm_hour);
                break;
            case 'h':  // Hour of day [01 ~ 12]
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%02d",
                        (ast_tm.tm_hour + 11) % 12 + 1);
                break;
            case 'k':  // Hour of day [0 ~ 23]
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%d", ast_tm.tm_hour);
                break;
            case 'l':  // Hour of day [1 ~ 12]
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%d", (ast_tm.tm_hour + 11) % 12 + 1);
                break;
            case 'I':  // Minute of hour [00 ~ 59]
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%02d", ast_tm.tm_min);
                break;
            case 'i':  // Minute of hour [0 ~ 59]
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%d", ast_tm.tm_min);
                break;
            case 'p':  // AM or PM
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%s",
                        (ast_tm.tm_hour < 12 ? "AM" : "PM"));
                break;
            case 'S':  // Second of minute [00 ~ 59]
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%02d", ast_tm.tm_sec);
                break;
            case 's':  // Second of minute [0 ~ 59]
                posi += sc_snprintf(ptr + posi,
                        max_len - posi, "%d", ast_tm.tm_sec);
                break;
            case '%':
                ptr[posi++] = *str;
                break;
            }
        }
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_time_sub 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param arg1(in)  :
 * \param arg2(in)  : 
 * \param arg3(in)  :
 * \param res(in/out) :
 * \param is_init(in) : MDB_TRUE :  res's type check, MDB_FALSE : 실제 수행
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - 실제 수행은 1번만 일어나도록 수정
 *  - collation 추가
 *  - parameter binding시 res type이 NULL로 넘어가는 문제 해결
 *****************************************************************************/
int
calc_func_time_sub(T_VALUEDESC * arg1,
        T_VALUEDESC * arg2, T_VALUEDESC * arg3,
        T_VALUEDESC * res, MDB_INT32 is_init)
{
    t_astime ast_sec, ast_msec;
    T_INTERVAL int_type;
    bigint interval;
    char timestamp[MAX_TIMESTAMP_STRING_LEN];

    int_type = get_interval_value(arg1);
    switch (int_type)
    {
    case INTERVAL_HOUR:
    case INTERVAL_MINUTE:
    case INTERVAL_SECOND:
        break;
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDARGUMENT, 0);
        return SQL_E_INVALIDARGUMENT;
    }

    if (arg1->valueclass == SVC_VARIABLE
            || arg2->valueclass == SVC_VARIABLE
            || arg3->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    if (IS_BS(arg3->valuetype))
        res->valuetype = DT_TIMESTAMP;
    else if (arg3->valuetype == DT_DATETIME)
        res->valuetype = DT_DATETIME;
    else if (arg3->valuetype == DT_DATE)
        res->valuetype = DT_DATE;
    else if (arg3->valuetype == DT_TIME)
        res->valuetype = DT_TIME;
    else
        res->valuetype = DT_TIMESTAMP;

    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (arg2->is_null || arg3->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (is_init)
        return RET_SUCCESS;

    switch (arg2->valuetype)
    {
    case DT_TINYINT:
    case DT_SMALLINT:
    case DT_INTEGER:
    case DT_BIGINT:
        GET_INT_VALUE(interval, bigint, arg2);
        break;
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDARGUMENT, 0);
        return SQL_E_INVALIDARGUMENT;
    }

    if (value2astime(arg3, &ast_sec, &ast_msec) < 0)
        return RET_ERROR;

    if (int_type == INTERVAL_MILLI_SECOND)
    {
        ast_msec -= interval;
        if (ast_msec < 0)
        {
            ast_sec--;
            ast_msec += 1000;
        }
    }
    else if (int_type == INTERVAL_SECOND)
    {
        ast_sec -= interval;
    }
    else if (int_type == INTERVAL_MINUTE)
    {
        ast_sec -= interval * 60;
    }
    else if (int_type == INTERVAL_HOUR)
    {
        ast_sec -= interval * 3600;
    }

    if (res->valuetype == DT_DATETIME)
    {
        ast_sec <<= 16;
        ast_sec |= ast_msec & 0x000000000000FFFF;
        timestamp2char(timestamp, &ast_sec);
        char2datetime(res->u.datetime, timestamp, sc_strlen(timestamp));
    }
    else if (res->valuetype == DT_DATE)
    {
        ast_sec <<= 16;
        ast_sec |= ast_msec & 0x000000000000FFFF;
        timestamp2char(timestamp, &ast_sec);
        char2date(res->u.date, timestamp, sc_strlen(timestamp));
    }
    else if (res->valuetype == DT_TIME)
    {
        ast_sec <<= 16;
        ast_sec |= ast_msec & 0x000000000000FFFF;
        timestamp2char(timestamp, &ast_sec);
        char2time(res->u.time, timestamp, sc_strlen(timestamp));
    }
    else if (res->valuetype == DT_TIMESTAMP)
        res->u.timestamp = (ast_sec << 16) | (ast_msec & 0x000000000000FFFF);

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_sysdate 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *  - execution시에 time이 결정되도록 수정함
 *****************************************************************************/
int
calc_func_sysdate(T_VALUEDESC * res, MDB_INT32 is_init)
{
    return calc_func_current_timestamp(res, is_init);
}
