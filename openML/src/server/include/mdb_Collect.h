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

#ifndef __COLLECT_H
#define __COLLECT_H

#include "mdb_config.h"
#include "mdb_PageList.h"
#include "mdb_Page.h"
#include "mdb_FieldDesc.h"

#define SLOT_PER_PAGE(slot_size) (DB_INT32)(PAGE_BODY_SIZE / (slot_size))

#define SLOT_PER_IPAGE          (DB_INT32)(PAGE_BODY_SIZE / TTREE_SLOT_SIZE)

typedef struct Collection
{
    OID cont_id_;               /* 이 자료구조가 포함되어 있는 container나
                                   ttree의 OID */
    DB_UINT32 item_count_;
    DB_UINT32 page_count_;
    DB_INT32 slot_size_;
    DB_INT32 item_size_;
    DB_UINT16 numfields_;
    DB_UINT16 record_size_;
    DB_INT16 collect_index_;
    PageID free_page_list_;
    /* insert할 빈 slot이 있는 위치를 쉽게 찾아가기 위해서 사용.
       insert 발생시 해당 page, delete 발생시 head로 이동 */
    struct PageList page_link_;
} Collection_t;

#define Col_GetNextPageID(pid) {                                \
    struct Page* page = (struct Page*)PageID_Pointer(pid);      \
    if(page == NULL)                                            \
    {                                                           \
        pid = INVALID_OID;                                      \
    }                                                           \
    else                                                        \
    {                                                           \
        pid = page->header_.link_.next_;                        \
    }                                                           \
}

#define Col_GetNextID(curr, size, tail) {                       \
    if (curr != NULL_OID)                                       \
    {                                                           \
        if ((OID_GetOffSet(curr) + size * 2) <= S_PAGE_SIZE)    \
        {                                                       \
            curr = curr + size;                                 \
        }                                                       \
        else                                                    \
        {                                                       \
            if (getPageOid(curr) == tail)                       \
            {                                                   \
                   curr = NULL_OID;                             \
            }                                                   \
            else                                                \
            {                                                   \
                Col_GetNextPageID(curr);                        \
                curr += PAGE_HEADER_SIZE;                       \
            }                                                   \
        }                                                       \
    }                                                           \
}

int __Col_IsDataSlot(OID curr, DB_INT32 slot_size, DB_UINT8 msync_flag);

#define Col_IsDataSlot(curr, slot_size, msync_flag)             \
    __Col_IsDataSlot((curr), (slot_size), (msync_flag))

void __Col_GetNextDataID(OID * curr, DB_INT32 slot_size, PageID * tail,
        DB_UINT8 msync_flag);

#define Col_GetNextDataID(curr, slot_size, tail, msync_type)                \
    __Col_GetNextDataID(&(curr), slot_size, &(tail), (msync_type))

int Collect_InitializePage(ContainerID cont_oid, const struct Collection *col,
        PageID pid);
struct Page *Free_Page_Search(struct Collection *collect);

int MemMgr_DelPage(struct Collection *collect, struct PageHeader *pageheader);

int Index_MemMgr_FreePageList(struct Collection *collect);

int Col_Read_Values(DB_UINT16 record_size, OID record_oid,
        FIELDVALUES_T * rec_values, int eval);

#endif /* __COLLECT_H */
