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
#include "mdb_Mem_Mgr.h"
#include "mdb_memmgr.h"

int count_SetBufIndexSegment = 0;

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
#ifdef MDB_DEBUG
char *
_OidToPtr(DB_INT32 sn, DB_INT32 pn, DB_INT32 offset, char *file, int line)
#else
char *
_OidToPtr(DB_INT32 sn, DB_INT32 pn, DB_INT32 offset)
#endif
{
    if (sn < SEGMENT_NO && pn < PAGE_PER_SEGMENT && offset < S_PAGE_SIZE)
    {
        /* OK */
    }
    else
    {
        extern int fRecovered;

        fRecovered = ALL_REBUILD;
        return NULL;
    }

    /* check ifirst_free_page_id_ */
    if (sn == 0 && pn == 0 && offset == 1)
    {
        return NULL;
    }

    segment_[sn] = DBDataMem_V2P(Mem_mgr->segment_memid[sn]);

    if (segment_[sn])
    {
        Mem_mgr->LRU_Link[sn].LRU_no++;
        if ((Mem_mgr->LRU_Link[sn].LRU_no & 0x0f) == 0x0)
        {
            if (sn < (int) (SEGMENT_NO - mem_anchor_->iallocated_segment_no_))
            {
                LRU_MoveToHead(sn);
            }
        }
    }
    else
    {
        {
            if (Mem_mgr->segment_memid[sn] == 0)
            {
                int ret = -1;

                ret = MemoryMgr_LoadSegment(sn);
                if (ret < DB_SUCCESS)
                {
                    return NULL;
                }
            }

#ifdef MDB_DEBUG
            if (Mem_mgr->segment_memid[sn] == 0 &&
                    sn <
                    (int) (SEGMENT_NO - mem_anchor_->iallocated_segment_no_))
                sc_assert(0, file, line);
#endif
            Mem_mgr->LRU_Link[sn].LRU_no = 0;

            segment_[sn] = DBDataMem_V2P(Mem_mgr->segment_memid[sn]);

            if (segment_[sn] == NULL)
                return NULL;
        }
    }

    return segment_[sn]->page_[pn].body_ + offset - PAGE_HEADER_SIZE;
}

int
IS_CRASHED_PAGE(int sn, int pn, struct Page *page)
{
    if (((page->header_.self_ == 0 && sn != 0 && pn != 0) ||
                    OID_GetOffSet(page->header_.self_) ||
                    OID_GetOffSet(page->header_.link_.next_)) ? 1 : 0 ||
            getSegmentNo(page->header_.free_slot_id_in_page_) > SEGMENT_NO ||
            getPageNo(page->header_.free_slot_id_in_page_) >= PAGE_PER_SEGMENT
            || OID_GetOffSet(page->header_.free_slot_id_in_page_) & 0x3 ||
            page->header_.record_count_in_page_ > S_PAGE_SIZE / 4 ||
            getSegmentNo(page->header_.free_page_next_) > SEGMENT_NO)
    {
        return 1;
    }

    return 0;
}
