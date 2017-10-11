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

#ifndef _CHARSET_H_
#define _CHARSET_H_

#include "mdb_config.h"
#include "dbport.h"
#include "mdb_compat.h"
#include "mdb_FieldVal.h"

#define DB_BYTE_LENG(_type, _length)    \
    (IS_NBS(_type)?                     \
     (unsigned int)(_length) * sizeof(DB_WCHAR) : (unsigned int)(_length))

extern __DECL_PREFIX int (*sc_collate_func[]) (const char *s1, const char *s2);
extern int (*sc_ncollate_func[]) (const char *s1, const char *s2, int n);
extern int (*sc_collate_memcpy_func[]) (void *s1, void *s2, int n);

#endif
