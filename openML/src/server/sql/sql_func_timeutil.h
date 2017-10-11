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

#ifndef _FUNC_TIMEUTIL_H
#define _FUNC_TIMEUTIL_H

#include "sql_datast.h"

typedef enum
{
    INTERVAL_NONE = 0,
    INTERVAL_MILLI_SECOND,
    INTERVAL_SECOND,
    INTERVAL_MINUTE,
    INTERVAL_HOUR,
    INTERVAL_DAY,
    INTERVAL_MONTH,
    INTERVAL_YEAR,
    INTERVAL_DUMMY = MDB_INT_MAX        /* to guarantee sizeof(enum) == 4 */
} T_INTERVAL;

extern int get_weekday(int year, int month, int day);
extern int get_day_of_year(int year, int month, int day);
extern char *date2char(char *str_date, char *date);
extern __DECL_PREFIX char *char2date(char *date, char *str_date, int str_len);
extern char *time2char(char *str_time, char *p_time);
extern __DECL_PREFIX char *char2time(char *p_time, char *str_time,
        int str_len);
extern char *datetime2char(char *str_datetime, char *datetime);
extern __DECL_PREFIX char *char2datetime(char *datetime, char *str_datetime,
        int str_len);
extern char *timestamp2char(char *str_timestamp, t_astime * timestamp);
extern __DECL_PREFIX t_astime *time2timestamp(t_astime * timestamp,
        char *time);
extern __DECL_PREFIX t_astime *char2timestamp(t_astime * timestamp,
        char *str_timestamp, int str_len);
extern T_INTERVAL get_interval_value(T_VALUEDESC * val);

extern __DECL_PREFIX iSQL_TIME *char2timest(iSQL_TIME * timest, char *stime,
        int sleng);

extern iSQL_TIME *timestamp2timest(iSQL_TIME * timest, t_astime * timestamp);


#define TO_NUMBER(s,n)                (int)strtol_limited(s,n)

#define DEFAULT_START_YEAR            1900
#define SEC_OF_TS(t)                ((t)>>16)
#define MSEC_OF_TS(t)                (short int)((t)&0xFFFF)

#if defined(WIN32)
#define START_SEC    (-62135596800)     /* 0001-01-01 00:00:00.000 sec */
#define LAST_SEC    (253402300799)      /* 9999-12-31 23:59.59.999  sec */
#else
#define START_SEC    (-62135596800LL)   /* 0001-01-01 00:00:00.000 sec */
#define LAST_SEC    (253402300799LL)    /* 9999-12-31 23:59.59.999  sec */
#endif

int calc_interval_by_gregorian(T_INTERVAL, bigint, t_astime *, t_astime *);
int add_interval_to_timestamp(T_INTERVAL, bigint, t_astime *, t_astime *);
int add_interval_by_convert(T_INTERVAL, T_VALUEDESC *, char *, int);

#endif // _FUNC_TIMEUTIL_H
