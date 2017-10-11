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

#ifndef __TTREEITER_H
#define __TTREEITER_H

#include "mdb_config.h"
#include "mdb_TTree.h"
#include "mdb_Iterator.h"

typedef struct oidarray
{
    OID *array;
    int count;
    int idx;
} MDB_OIDARRAY;

struct TTreeIterator
{
    struct Iterator iterator_;
    OID tree_oid_;
    DB_BOOL is_unique_;
    struct TTreePosition first_;
    struct TTreePosition last_;
    struct TTreePosition curr_;
    DB_UINT32 modified_time_;
};

struct TTreeIterator_Postion
{
    int csrID_;
    struct TTreePosition first_;
    struct TTreePosition last_;
    struct TTreePosition curr_;
};

int TTreeIter_Create(struct TTreeIterator *ttreeiter);
int TTree_RealNext(struct TTreeIterator *ttreeiter, RecordID real_next);
int TTree_Restart(struct TTreeIterator *ttreeiter);
int TTree_Set_First(DB_BOOL use_min_key, DB_BOOL use_filter,
        struct TTreePosition *first, struct TTreeIterator *ttreeiter);
int TTree_Set_Last(DB_BOOL use_max_key, DB_BOOL use_filter,
        struct TTreePosition *last, struct TTreeIterator *ttreeiter);

extern int TTree_GetOidSet(struct TTreeIterator *ttreeiter,
        MDB_OIDARRAY * oidset);

#endif
