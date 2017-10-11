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

/*****************************************************************************/
//! decimal2char 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param str_decimal :
 * \param dec : 
 * \param pres :
 * \param scale :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX char *
decimal2char(char *str_decimal, decimal dec, int pres, int scale)
{
    char *p, format[32];
    bigint pw;
    int n;

    if (dec == MDB_DBL_MIN)
    {
        p = str_decimal;
        *p++ = '-';
        while ((pres--) - scale)
            *p++ = '9';
        *p++ = '.';
        while (scale--)
            *p++ = '9';
        *p = '\0';

        return str_decimal;
    }
    else if (dec == MDB_DBL_MAX)
    {
        p = str_decimal;
        *p++ = ' ';
        while ((pres--) - scale)
            *p++ = '9';
        *p++ = '.';
        while (scale--)
            *p++ = '9';
        *p = '\0';

        return str_decimal;
    }

    if (dec < 0.0)
    {
        str_decimal[0] = '-';
        dec *= -1.0;
    }
    else
        str_decimal[0] = ' ';

    pw = (bigint) sc_pow(10, pres - scale);
    if (dec > (decimal) pw)
        dec -= ((bigint) dec / pw) * pw;

    /* REMOVE_SNPRINTF */
    sc_sprintf(format, "%%%d.%df", pres + (scale ? 1 : 0), scale);
    n = sc_snprintf(str_decimal + 1, MAX_DECIMAL_STRING_LEN - 1, format, dec);

    /* see man of snprintf */
    if (n > MAX_DECIMAL_STRING_LEN - 1)
        str_decimal[MAX_DECIMAL_STRING_LEN] = '\0';
    else
        str_decimal[n + 1] = '\0';

    return str_decimal;
}

/*****************************************************************************/
//! char2decimal 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param dec :
 * \param str_decimal : 
 * \param len :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
decimal
char2decimal(decimal * dec, char *str_decimal, int len)
{
    decimal floating;
    char buf[CSIZE_DECIMAL + 1];
    ibool negative = (*str_decimal++ == '-' ? 1 : 0);
    int i;

    while (*str_decimal == ' ')
        str_decimal++;
    for (i = 0, *dec = 0; *str_decimal && i < len; i++, str_decimal++)
    {
        if (*str_decimal == '.')
        {
            str_decimal++;
            break;
        }
        *dec = *dec * 10 + (*str_decimal - '0');
    }

    for (floating = 0.1; *str_decimal && i < len; i++, str_decimal++)
    {
        if (*str_decimal == '.')
        {
            str_decimal++;
            break;
        }
        *dec += (*str_decimal - '0') * floating;
        floating /= 10;
    }

    if (negative)
        *dec *= -1.0;

    sc_snprintf(buf, CSIZE_DECIMAL, "%f", *dec);
    *dec = sc_atof(buf);

    return *dec;
}
