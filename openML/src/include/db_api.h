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

#ifndef DB_API_H
#define DB_API_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include "mdb_config.h"
#include "dbport.h"
#include "mdb_define.h"
#include "mdb_typedef.h"
#include "ErrorCode.h"
#include "systable.h"

/* rec: record, fieldPos: position of field, nullFlagOffset: size of record */
#define DB_VALUE_ISNULL(rec,fieldPos,nullFlagOffset)             \
    ((rec)[(nullFlagOffset)+(((fieldPos)>>3))] & (0x80U>>((fieldPos)&0x07U)))
#define DB_VALUE_SETNULL(rec,fieldPos,nullFlagOffset)            \
    (rec)[(nullFlagOffset)+(((fieldPos)>>3))] |= (0x80>>((fieldPos)&0x7))
#define DB_VALUE_SETNOTNULL(rec,fieldPos,nullFlagOffset)      \
    (rec)[(nullFlagOffset)+(((fieldPos)>>3))] &= ~(0x80>>((fieldPos)&0x7))

#ifndef mdb_offsetof
#define mdb_offsetof(__TYPE, __MEMBER) ((size_t) &((__TYPE *)0)->__MEMBER)
#endif

    __DECL_PREFIX DB_BOOL Drop_DB(char *path);
    __DECL_PREFIX int createdb(char *dbfilename);
    __DECL_PREFIX int createdb2(char *dbname, int num_seg);
    __DECL_PREFIX int createdb3(char *dbname, int dbseg_num,
            int idxseg_num, int tmpseg_num);

    __DECL_PREFIX const char *DB_strerror(int errorno);

    __DECL_PREFIX void chartohex(unsigned char *str, unsigned char *hex,
            unsigned int size);

#define DB_GET_BYTE_LENGTH(field_, length_)     (sc_memcpy(&length_, field_, 4))
#define DB_SET_BYTE_LENGTH(field_, length_)     (sc_memcpy(field_,&length_, 4))
#define DB_GET_BYTE_VALUE(field_)               ((char*)((char*)(field_)+4))
#define DB_GET_BYTE(field_, buf_, length_)                      \
    do {                                                        \
        DB_GET_BYTE_LENGTH(field_, length_);                    \
        sc_memcpy(buf_, (((char*)(char*)field_)+4), length_);   \
    } while(0)
#define DB_SET_BYTE(field_, buf_, length_)                      \
    do {                                                        \
        DB_SET_BYTE_LENGTH(field_, length_);                    \
        sc_memcpy(((char*)((char*)field_)+4), buf_, length_);   \
    } while(0)

#define DB_DEFAULT_ISNULL(defval)                           \
    ((unsigned char)(defval[DEFAULT_VALUE_LEN-1]) == 0xFF)
#define DB_DEFAULT_SETNULL(defval)                          \
    (defval[DEFAULT_VALUE_LEN-1] = 0xFF)

#ifdef  __cplusplus
}
#endif

#endif                          /* #ifndef DB_API_H */
