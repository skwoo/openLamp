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
#include "mdb_Server.h"

FIELDDESC_T _hiddenField[] = {
    {"nullflag", DT_TINYINT, 0, 0, 0, 1, 0, 0x2 | 0x8, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
    ,
    {"utime", DT_INTEGER, 0, 0, 0, 4, 0, 0x2 | 0x8, FIXED_VARIABLE,
                MDB_COL_NUMERIC, "\0"}
};

int _numHiddenFields = sizeof(_hiddenField) / sizeof(FIELDDESC_T);

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
GET_HIDDENFIELDSNUM(void)
{
    return _numHiddenFields;
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
GET_HIDDENFIELDLENGTH(int field_index, int real_field_num, int real_field_len)
{
    int nullbyte;
    int fieldlen;

#ifdef MDB_DEBUG
    if (field_index < -1 || field_index >= _numHiddenFields)
        return DB_FAIL;
#endif

    if (1 <= field_index && field_index < _numHiddenFields)
        return sizeof(int);

    nullbyte = ((real_field_num - 1) >> 3) + 1; /* bytes */

    fieldlen = _alignedlen(real_field_len + nullbyte, sizeof(long));


    if (field_index == 0)
        return fieldlen - real_field_len;       /* align까지 포함한 길이 */

    if (field_index == -1)      /* hidden field 전체 길이 */
    {
        return fieldlen - real_field_len + sizeof(int) * (_numHiddenFields -
                1);
    }

    return DB_FAIL;
}
