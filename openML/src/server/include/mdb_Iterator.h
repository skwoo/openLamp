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

#ifndef __ITERATOR_H
#define __ITERATOR_H

#include "mdb_config.h"
#include "mdb_Cont.h"

struct Iterator
{
    int trans_;
    struct Trans *trans_ptr;
    int rec_leng;               /* memory record length */
    OID cont_id_;
    struct Filter filter_;
    struct KeyValue min_;
    struct KeyValue max_;
    int creator_;
    int id_;
    DB_INT32 slot_size_;
    DB_INT32 item_size_;
    DB_UINT32 memory_record_size_;
    DB_UINT16 record_size_;
    DB_UINT16 numfields_;
    DB_UINT8 fInit;
    DB_UINT8 key_include_;
    LOCKMODE lock_mode_;
    CursorType type_;
    DB_UINT16 qsid_;
    DB_UINT32 open_time_;
    OID real_next;
    DB_UINT8 msync_flag;
};

struct Iterator *Iter_Attach(DB_INT32 cursor_id, int optype, OID page);
OID Iter_Current(struct Iterator *Iter);
int Iter_FindPrev(struct Iterator *Iter, OID page);
int Iter_FindNext(struct Iterator *Iter, OID page);
int Iter_FindFirst(struct Iterator *);
int Iter_FindLast(struct Iterator *);

#endif
