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

#ifndef __KEYVALUE_H
#define __KEYVALUE_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include "mdb_config.h"
#include "mdb_FieldVal.h"

/* The struct KeyValue must be kept the same as struct Filter*/
    struct KeyValue
    {
        DB_UINT16 field_count_;
        struct FieldValue *field_value_;
    };

    int KeyField_Compare(struct FieldValue *, char *, DB_INT32);

    int dbi_KeyValue(int n, FIELDVALUE_T * fieldValue,
            struct KeyValue *keyvalue);

#ifdef  __cplusplus
}
#endif

#endif
