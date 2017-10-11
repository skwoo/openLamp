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
#include "mdb_ContIter.h"
#include "mdb_Csr.h"
#include "mdb_dbi.h"
#include "mdb_syslog.h"
#include "sql_mclient.h"

int Filter_Compare(struct Filter *, char *, int);

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
ContIter_Create(struct ContainerIterator *ContIter, OID page)
{
    int issys_cont;
    int issys;
    int ret;
    OID first_oid;
    struct Container *Cont =
            (struct Container *) OID_Point(ContIter->iterator_.cont_id_);

    SetBufSegment(issys_cont, ContIter->iterator_.cont_id_);

    first_oid = Col_GetFirstDataID_Page(Cont, page,
            ContIter->iterator_.msync_flag);

    {
        char *record;
        DB_UINT32 utime;
        DB_UINT16 qsid;
        int utime_offset;
        int qsid_offset;

        utime_offset = Cont->collect_.slot_size_ - 8;
        qsid_offset = Cont->collect_.slot_size_ - 4;

        /* 레코드의 utime이 cursor open 시간 이전 것을 찾는다 */
        issys = 0;
        while (first_oid != NULL_OID)
        {
            if (first_oid == INVALID_OID)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }

            record = (char *) OID_Point(first_oid);
            if (record == NULL)
            {
                ret = DB_E_OIDPOINTER;
                goto end;
            }

            SetBufSegment(issys, first_oid);

            utime = *(DB_UINT32 *) (record + utime_offset);
            qsid = *(DB_UINT16 *) (record + qsid_offset);

            /* table lock이므로 s-x, x-x lock이 걸릴 수 없어
               != 만 점검을 해도 될 듯 */
            if (utime == ContIter->iterator_.open_time_ &&
                    ContIter->iterator_.qsid_ == qsid)
            {
            }
            else
            {
                if (ContIter->iterator_.filter_.expression_count_ == 0)
                {       //filter가 없으면 첫번째 찾고 stop
                    UnsetBufSegment(issys);
                    break;
                }

                /* filter가 있으며 비교해서 만족하면 stop */
                if (Filter_Compare(&(ContIter->iterator_.filter_), record,
                                Cont->collect_.record_size_) == DB_SUCCESS)
                {
                    UnsetBufSegment(issys);
                    break;
                }
            }

            UnsetBufSegment(issys);

            Col_GetNextDataID(first_oid, Cont->collect_.slot_size_,
                    Cont->collect_.page_link_.tail_,
                    ContIter->iterator_.msync_flag);
        }   /* while */
    }

    ContIter->first_ = first_oid;
    ContIter->curr_ = first_oid;

    ret = DB_SUCCESS;
  end:
    UnsetBufSegment(issys);
    UnsetBufSegment(issys_cont);

    return ret;
}
