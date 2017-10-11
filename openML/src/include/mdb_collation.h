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

#ifndef __MDB_COLLATION_H__
#define __MDB_COLLATION_H__

#ifdef  __cplusplus
extern "C"
{
#endif

#include "mdb_config.h"
#include "sys_inc.h"
#include "dbport.h"

    typedef enum
    {
        MDB_COL_NONE = 0,
        MDB_COL_NUMERIC,
        MDB_COL_CHAR_NOCASE,
        MDB_COL_CHAR_CASE,
        MDB_COL_CHAR_DICORDER,
        MDB_COL_NCHAR_NOCASE,
        MDB_COL_NCHAR_CASE,
        MDB_COL_BYTE,
        MDB_COL_USER_TYPE1,
        MDB_COL_USER_TYPE2,
        MDB_COL_USER_TYPE3,
        MDB_COL_CHAR_REVERSE,
        MDB_COL_MAX = MDB_INT_MAX
    } MDB_COL_TYPE;

#define IS_REVERSE(_coltype) ((_coltype) == MDB_COL_CHAR_REVERSE)
#define IS_CHARCASE(_coltype) ((_coltype) == MDB_COL_CHAR_CASE)

#define MDB_COL_DEFAULT_SYSTEM  MDB_COL_CHAR_DICORDER
#define MDB_COL_DEFAULT_CHAR    MDB_COL_CHAR_DICORDER
#define MDB_COL_DEFAULT_NCHAR   MDB_COL_NCHAR_CASE
#define MDB_COL_DEFAULT_BYTE    MDB_COL_BYTE


#ifdef  __cplusplus
}
#endif

#endif                          // __MDB_COLLATION_H__
