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
#include "mdb_KeyDesc.h"
#include "mdb_Collect.h"
#include "mdb_dbi.h"

/* ttree내 레코드 위치를 정하기 위해서 사용하는 function이라
   NULL 값 관련 검사는 하지 않음 */
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
KeyDesc_Compare(struct KeyDesc *keydesc, const char *r1, const char *r2,
        int recSize1, int recSize2)
{
    register DB_UINT16 n;
    int r;
    int isnull1, isnull2;

    if (recSize2 == 0)
        recSize2 = recSize1;

    for (n = 0; n < keydesc->field_count_; n++)
    {
        isnull1 = DB_VALUE_ISNULL((char *) r1,
                keydesc->field_desc_[n].position_, recSize1);

        isnull2 = DB_VALUE_ISNULL((char *) r2,
                keydesc->field_desc_[n].position_, recSize2);
        if (isnull1 && isnull2)
            r = 0;
        else if (isnull1)
        {
            if (keydesc->field_desc_[n].order_ == 'D')
                return -1;
            return 1;
        }
        else if (isnull2)
        {
            if (keydesc->field_desc_[n].order_ == 'D')
                return 1;
            return -1;
        }
        else
        {
            r = FieldDesc_Compare(&keydesc->field_desc_[n], r1, r2);
            if (r)
                return r;
        }
    }

    return 0;
}

int
KeyDesc_Compare2(struct TTree *ttree, const char *r1, const char *r2,
        int *isnull1)
{
    register struct KeyDesc *keydesc;
    register DB_UINT16 n;
    int r;
    int isnull2;
    int recSize;

    keydesc = &(ttree->key_desc_);
    recSize = ttree->collect_.record_size_;

    for (n = 0; n < keydesc->field_count_; n++)
    {
        isnull2 = DB_VALUE_ISNULL((char *) r2,
                keydesc->field_desc_[n].position_, recSize);
        if (isnull1[n] && isnull2)
            r = 0;
        else if (isnull1[n])
        {
            if (keydesc->field_desc_[n].order_ == 'D')
                return -1;
            return 1;
        }
        else if (isnull2)
        {
            if (keydesc->field_desc_[n].order_ == 'D')
                return 1;
            return -1;
        }
        else
        {
            r = FieldDesc_Compare(&keydesc->field_desc_[n], r1, r2);
            if (r)
                return r;
        }
    }

    return 0;
}
