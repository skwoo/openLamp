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

#ifndef __CSR_H
#define __CSR_H

#include "mdb_config.h"
#include "mdb_typedef.h"
#include "mdb_ContIter.h"
#include "mdb_TTreeIter.h"
#include "mdb_BaseCont.h"
#include "mdb_ppthread.h"
#include "systable.h"
#include "mdb_Trans.h"

#define _CI_SIZE    sizeof(struct ContainerIterator)
#define _TI_SIZE    sizeof(struct TTreeIterator)
#define CURSOR_CHUNK_SIZE  _CI_SIZE > _TI_SIZE ? _CI_SIZE : _TI_SIZE

#define CURSOR_FORWARD_DIRECTION    1
#define CURSOR_BACKWARD_DIRECTION   2
#define CURSOR_ASC_ORDER            1
#define CURSOR_DESC_ORDER           2

#define GET_CURSOR_POINTER(csrID)                                       \
    (((csrID) <= 0 || (csrID) > MAX_ALLOC_CURSORID_NUM) ?               \
     NULL : (&(Cursor_Mgr->cursor[(csrID)-1])))

struct Cursor
{
    DB_INT8 fAllocated_;
    DB_INT8 whereami;           /* allocation 한 위치. 0x01: 서버, 0x0f: process */
    LOCKMODE lock_mode_;
    ContainerType cont_type;
    OID cont_id_;
    Position next_;
    int trans_id_;
    struct Trans *trans_ptr;
    SYSTABLE_T *table_info;
    SYSFIELD_T *fields_info;

    struct Update_Idx_Id_Array *pUpd_idx_id_array;

    struct Cursor *trans_next;  /* cursor list for transaction */

    char chunk_[CURSOR_CHUNK_SIZE];
};

struct CursorInfo
{
    char cont_name[CONT_NAME_LENG];
    DB_INT16 index_no;
    LOCKMODE hold_lock;
};

/* cursor_type: SEQ_CURSOR, TTREE_FORWARD_CURSOR, TTREE_BACKWARD_CURSOR */
/* direction  : CURSOR_FORWARD_DIRECTION, CURSOR_BACKWARD_DIRECTION */

struct CursorMgr
{
    Position free_first_;
    struct Cursor cursor[MAX_ALLOC_CURSORID_NUM];
};

struct Cursor_Position
{
    union
    {
        struct ContainerIterator_Postion contIter;
        struct TTreeIterator_Postion ttreeIter;
    } cp;
};

#define CursorPosition(cursor_id)                               \
    ((0 < cursor_id && cursor_id <= MAX_ALLOC_CURSORID_NUM) ?   \
     cursor_id - 1 : NULL_Cursor_Position)

extern __DECL_PREFIX struct CursorMgr *Cursor_Mgr;

int CurMgr_CursorTableInit(void);
int CurMgr_CursorTableFree(void);
DB_UINT32 Open_Cursor(int Trans_Num, char *cont_name, LOCKMODE,
        DB_INT16 idx_no, struct KeyValue *minkey,
        struct KeyValue *maxkey, struct Filter *f);
DB_UINT32 Open_Cursor_Desc(int Trans_Num, char *cont_name,
        LOCKMODE, DB_INT16 idx_no, struct KeyValue *minkey,
        struct KeyValue *maxkey, struct Filter *f);

DB_UINT32 Open_Cursor2(int Trans_Num, char *cont_name, LOCKMODE,
        DB_INT16 idx_no, struct KeyValue *minkey, struct KeyValue *maxkey,
        struct Filter *f, OID index_cont_oid, DB_UINT8 msync_flag);

int Close_Cursor(DB_INT32 cursor_id);

char *Cursor_TableName(int cursorid);
int Cursor_isTempTable(int cursorid);
int Cursor_isNologgingTable(int cursorid);

int Reopen_Cursor(int cursor_id, struct KeyValue *minkey,
        struct KeyValue *maxkey, struct Filter *f);
int Reposition_Cursor(int cursor_id, struct KeyValue *findkey);

int Get_CursorPos(int cursorId, struct Cursor_Position *cursorPos);
int Set_CursorPos(int cursorId, struct Cursor_Position *cursorPos);
void Cursor_Scan_Dangling(int transId);

int Make_CursorPos(int cursorId, OID rid_to_make);

extern struct Cursor *Cursor_get_schema_info(DB_INT32 cursor_id,
        SYSTABLE_T ** table_info, SYSFIELD_T ** fields_info);
#endif /* __CSR_H */
