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

#ifndef __FILTER_H
#define __FILTER_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include "mdb_config.h"
#include "mdb_FieldVal.h"

/* The struct Filter must be kept the same as struct KeyValue */
    struct Filter
    {
        DB_UINT16 expression_count_;
        FIELDVALUE_T *expression_;
    };

    struct Filter dbi_Filter(int n, FIELDVALUE_T * expression);

#ifdef  __cplusplus
}
#endif

#endif
