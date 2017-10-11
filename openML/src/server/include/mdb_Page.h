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

#ifndef __PAGE_H
#define __PAGE_H

#include "mdb_config.h"
#include "mdb_typedef.h"
#include "mdb_inner_define.h"

struct PageLink
{
    PageID prev_;
    PageID next_;
};

#define LINK_SIZE sizeof(struct PageLink)

struct PageHeader
{
    PageID self_;
    struct PageLink link_;
    PageID free_page_next_;
    OID free_slot_id_in_page_;
#define free_node_id_in_page_ free_slot_id_in_page_
    DB_UINT32 record_count_in_page_;
#define node_count_in_page_ record_count_in_page_
    ContainerType cont_type_;
    MDB_INT16 collect_index_;
    ContainerID cont_id_;
};

#define  PAGE_HEADER_SIZE   sizeof(struct PageHeader)
#define  PAGE_BODY_SIZE     (S_PAGE_SIZE - PAGE_HEADER_SIZE )

struct Page
{
    struct PageHeader header_;
    char body_[PAGE_BODY_SIZE];
};

extern int IS_CRASHED_PAGE(int sn, int pn, struct Page *page);

#endif
