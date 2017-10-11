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

#ifndef VERSION_MMDB_H
#define VERSION_MMDB_H

#define MAJOR_VER "0"
#define MINOR_VER "1"
#define SUBMINOR_VER "0"
#define BUILD_NO "0"

#define _DBNAME_    "openML"

#if defined(VERSION_NICKNAME)
#define _DBVERSION "Ver "MAJOR_VER"."MINOR_VER"."SUBMINOR_VER"."BUILD_NO" "VERSION_NICKNAME
#else
#define _DBVERSION "Ver "MAJOR_VER"."MINOR_VER"."SUBMINOR_VER"."BUILD_NO
#endif

#if defined(ENDIAN_COMPATIBLE)

#ifdef _64BIT
#define _DBVERSION_ _DBVERSION" (64 bit, Endian Compatible)"
#else
#define _DBVERSION_ _DBVERSION" (32 bit, Endian Compatible)"
#endif

#else

#ifdef _64BIT
#define _DBVERSION_ _DBVERSION" (64 bit)"
#else
#define _DBVERSION_ _DBVERSION" (32 bit)"
#endif

#endif

#define _COPYRIGHT_ "Copyright (C) 2012 INERVIT Co., Ltd. All rights reserved."

#define MKVERSION(M, m, s, p) (M * 100000000 + m * 1000000 + s * 10000 + p)

#define MMDB_VERSION	MKVERSION(sc_atoi(MAJOR_VER), sc_atoi(MINOR_VER), sc_atoi(SUBMINOR_VER), sc_atoi(BUILD_NO))

#endif /* VERSION_MMDB_H */
