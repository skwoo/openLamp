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

#ifndef __BASECONT_H
#define __BASECONT_H

#include "mdb_config.h"
#include "mdb_inner_define.h"
#include "mdb_typedef.h"

struct BaseContainer
{
    int id_;                    /* table id or index no */
    char name_[CONT_NAME_LENG];
    int creator_;
    int transid_;
    ContainerType type_;

    DB_INT8 index_rebuild;
    DB_INT8 is_dirty_;
    DB_INT8 has_variabletype_;

    DB_UINT8 runtime_logging_;

    DB_UINT8 index_count_;

    DB_UINT32 memory_record_size_;

    OID index_id_;
    OID hash_next_name;
    OID hash_next_id;
    DB_UINT32 max_records_;
    DB_INT32 auto_fld_pos_;
    DB_INT32 auto_fld_offset_;
    DB_LONG max_auto_value_;
};

/* container type for table */
#define _CONT_TABLE             0x0001
#define _CONT_VIEW              0x0002

#define _CONT_TEMPTABLE_        0x0008

/* container type for index */
#define _CONT_INDEX_TTREE       0x0010
#define _CONT_INDEX_TTREE_SPLIT 0x0020
#define _CONT_PRIMARY_INDEX     0x0040

#define _CONT_NOLOGGING         0x0100
#define _CONT_NOLOCKING         0x0200
#define _CONT_HIDDEN            0x0400

#define _CONT_RESIDENT          0x0800
#define _CONT_RESIDENT_TABLE    (_CONT_TABLE|_CONT_NOLOGGING|_CONT_RESIDENT)
 /* for sysdummy */
#define _CONT_HIDDENTABLE       (_CONT_TABLE|_CONT_HIDDEN)
 /* for nologging table */
#define _CONT_NOLOGGINGTABLE    (_CONT_TABLE|_CONT_NOLOGGING)

#define _CONT_TEMPTABLE         (_CONT_TEMPTABLE_|_CONT_NOLOGGING|_CONT_NOLOCKING|_CONT_HIDDEN)

#define _CONT_MSYNC             0x1000
#define _CONT_MSYNC_TABLE       (_CONT_TABLE|_CONT_MSYNC)

#define SCHEMA_REGU_INDEX       1
#define SCHEMA_TEMP_INDEX       2
#define SCHEMA_ALL_INDEX        3

/* user */
#define _USER_SYS               0
#define _USER_SYSTEM            1
#define _USER_USER              100

#define _USER_TABLEID_BASE      200
#define _USER_TABLEID_END       (_TEMP_TABLEID_BASE-1)


#define _TEMP_TABLEID_BASE      10000000

#define _CONNID_BASE            10000000

#define _RUNTIME_LOGGING_NONE   0
#define _RUNTIME_LOGGING_ON     1
#define _RUNTIME_LOGGING_OFF    2

#define isNoLogging(cont)                                                   \
    ((cont) != NULL &&                                                      \
     (((cont)->base_cont_.runtime_logging_ == _RUNTIME_LOGGING_OFF ||       \
       ((cont)->base_cont_.runtime_logging_ == _RUNTIME_LOGGING_NONE &&     \
        (cont)->base_cont_.type_ & _CONT_NOLOGGING))))
#define isNoLocking(cont)   (cont != NULL && (cont->base_cont_.type_ & _CONT_NOLOCKING))
#define isHidden(cont)      (cont != NULL && (cont->base_cont_.type_ & _CONT_HIDDEN))
#define isTable(cont)       (cont != NULL && (cont->base_cont_.type_ & _CONT_TABLE))
#define isTempTable(cont)   (cont != NULL && (cont->base_cont_.type_ & _CONT_TEMPTABLE_))

#define isSysTable(cont)    (cont != NULL && (cont->base_cont_.creator_) == _USER_SYS)
#define isSysTable2(creator) ((creator) == _USER_SYS)
#define isSystemTable(cont) (cont != NULL && (cont->base_cont_.creator_) == _USER_SYSTEM)
#define isSystemTable2(creator) ((creator) == _USER_SYSTEM)
#define isUserTable(cont)   (isTable(cont) && (cont->base_cont_.creator_) >= _USER_USER)

#define doesUserCreateTable(cont, userid)   \
    (cont != NULL && (isTempTable(cont) || (cont->base_cont_.creator_) == userid))

#define isViewTable(cont)   (cont != NULL && (cont->base_cont_.type_ & _CONT_VIEW))

#define isResidentTable(cont)   (cont != NULL && (cont->base_cont_.type_ & _CONT_RESIDENT))

#define isTempTable_flag(flag)  ((flag & _CONT_TEMPTABLE) == _CONT_TEMPTABLE)
#define isHidden_flag(flag)     (flag & _CONT_HIDDEN)

#define isTempTable_name(name)  (name[0] == '#')
#define isTempIndex_name(name)  (name[0] == '#')
#define isTempField_name(name)  (name[0] == '#')

#define isTempTable_ID(id)      (id >= _TEMP_TABLEID_BASE)

#define isPrimaryIndex_name(name)   \
        (name[0] == '$' && name[1] == 'p' && name[2] == 'k')

#define isPartialIndex_name(name)   \
        (name[0] == '#' && name[1] == 'p' && name[2] == 'i')

#define IS_FIXED_RECORDS_TABLE(num) (num > 0)

#define isSequenceTable(tableid)        (tableid == DEFAULT_SYSSEQUENCES_ID)
#define is_systable_systables(tableid)  (tableid == DEFAULT_SYSTABLES_ID)

#define isTemp_SEGMENT(no)  (TEMPSEG_BASE_NO <= (no) && (no) < TEMPSEG_END_NO)
#define isTemp_OID(oid)     isTemp_SEGMENT(getSegmentNo(oid))

#define ismSyncTable(cont)  (cont != NULL && (cont->base_cont_.type_ & _CONT_MSYNC))

#endif
