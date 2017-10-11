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

#ifndef __CONTCAT_H
#define __CONTCAT_H

#include "mdb_config.h"
#include "mdb_ppthread.h"
#include "mdb_Collect.h"
#include "mdb_Cont.h"

struct ContainerCatalog
{
    struct Collection collect_;
    struct BaseContainer base_cont_;
    unsigned int tableid;
    OID hash_next_name[11];
    OID hash_next_id[11];
};

extern __DECL_PREFIX struct ContainerCatalog *container_catalog;

struct TempContainerCatalog
{
    struct Collection collect_;
    struct BaseContainer base_cont_;
};

extern __DECL_PREFIX struct TempContainerCatalog *temp_container_catalog;

struct ContainerCatalog *ContCatalogServerInit(void);
DB_INT32 ContCatalogServerDown(void);
__DECL_PREFIX int ContCatalog_TableidInit(int tableid);
int ContCatalog_init_Createdb(int cont_item_size);
int Cont_Dirty_Bit_Init(void);
int Cont_Item_Count(const char *cont_name);

/* INDEX_REBUILD */
int ContCat_Index_Rebuild(int rebuild_mode);

#endif
