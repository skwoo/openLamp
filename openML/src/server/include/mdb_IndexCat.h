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

#ifndef __INDEXCAT_H
#define __INDEXCAT_H

#include "mdb_config.h"
#include "mdb_Collect.h"
#include "mdb_Cont.h"

struct IndexCatalog
{
    struct Collection collect_;
    struct BaseContainer base_cont_;
    struct Collection icollect_;        /* index free page list */
};

extern __DECL_PREFIX struct IndexCatalog *index_catalog;

struct TempIndexCatalog
{
    struct Collection collect_;
    struct BaseContainer base_cont_;
    struct Collection icollect_;        /* index free page list */
};

extern __DECL_PREFIX struct TempIndexCatalog *temp_index_catalog;

int IndexCatalog_init_Createdb(int index_item_size);
struct IndexCatalog *IndexCatalogServerInit(void);
int Index_Col_Remove(struct Collection *collect, OID record_oid);
DB_INT32 IndexCatalogServerDown(void);

struct TTree *TTree_Search(const OID table_oid, const int idx_no);

#endif
