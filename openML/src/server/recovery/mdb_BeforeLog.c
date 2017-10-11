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
#include "mdb_Mem_Mgr.h"
#include "mdb_inner_define.h"
#include "mdb_LogRec.h"

int CLR_WriteCLR(struct LogRec *logrec);
int UpdateTransTable(struct LogRec *logrec);

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
BeforeLog_Undo(struct LogRec *logrec, int flag)
{
    if (flag == CLR)
        CLR_WriteCLR(logrec);

    if (logrec->header_.op_type_ == RELATION_MSYNCSLOT)
    {
        struct Container *Cont;

        Cont = (struct Container *) OID_Point(logrec->header_.
                relation_pointer_);
        if (Cont == NULL)
        {
            return DB_E_OIDPOINTER;
        }

        OID_LightWrite_msync(logrec->header_.oid_, NULL,
                logrec->header_.recovery_leng_, Cont,
                (DB_UINT8) (*logrec->data1));
    }
    else
    {
        OID_LightWrite(logrec->header_.oid_,
                (const char *) logrec->data1, logrec->header_.recovery_leng_);
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
int
BeforeLog_Redo(struct LogRec *logrec)
{
    UpdateTransTable(logrec);

    return DB_SUCCESS;
}
