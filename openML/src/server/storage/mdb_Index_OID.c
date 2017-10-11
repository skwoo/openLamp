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
#include "mdb_typedef.h"
#include "mdb_Mem_Mgr.h"
#include "mdb_inner_define.h"
#include "mdb_syslog.h"

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
Index_OID_HeavyWrite(OID curr, char *data, DB_INT32 size)
{
    struct Page *ipage;
    char *where;

    ipage = (struct Page *) Index_PageID_Pointer(curr);

    if (ipage == NULL)
    {
        MDB_SYSLOG(
                ("Write ERROR  since OID converte to Pointer 1, 0x%x, 0x%x\n",
                        curr, data));
        return DB_FAIL;
    }

    where = (char *) ipage;
    where = where + (OID) Index_OID_GetOffSet(curr);
    sc_memcpy(where, data, size);
    SetPageDirty(Index_getSegmentNo(curr), Index_getPageNo(curr));

    return DB_SUCCESS;
}
