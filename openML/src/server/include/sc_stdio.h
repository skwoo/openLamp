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

/*
 * @FILE            : sc_stdio.h
 * @Description    : standard library가 제공이 안되는 경우..
 *                  이를 추가하기 위해서 가지온 함수이다.
 *                     
 **/

#ifndef __SC_STDIO_H__
#define __SC_STDIO_H__

#include "mdb_config.h"
#include "dbport.h"

#define _MDB_U      0x01        /* upper */
#define _MDB_L      0x02        /* lower */
#define _MDB_D      0x04        /* digit */
#define _MDB_C      0x08        /* cntrl */
#define _MDB_P      0x10        /* punct */
#define _MDB_S      0x20        /* white space (space/lf/tab) */
#define _MDB_X      0x40        /* hex digit */
#define _MDB_SP     0x80        /* hard space (0x20) */

extern __DECL_PREFIX unsigned char sc_ctype[];

#define __sc_ismask(x)  (sc_ctype[(int)(unsigned char)(x)])

#ifndef sc_isalnum
#define sc_isalnum(c)   ((__sc_ismask(c)&(_MDB_U|_MDB_L|_MDB_D)) != 0)
#endif

#ifndef sc_isalpha
#define sc_isalpha(c)   ((__sc_ismask(c)&(_MDB_U|_MDB_L)) != 0)
#endif

#ifndef sc_iscntrl
#define sc_iscntrl(c)   ((__sc_ismask(c)&(_MDB_C)) != 0)
#endif

#ifndef sc_isdigit
#define sc_isdigit(c)   ((__sc_ismask(c)&(_MDB_D)) != 0)
#endif

#ifndef sc_isgraph
#define sc_isgraph(c)   ((__sc_ismask(c)&(_MDB_P|_MDB_U|_MDB_L|_MDB_D)) != 0)
#endif

#ifndef sc_islower
#define sc_islower(c)   ((__sc_ismask(c)&(_MDB_L)) != 0)
#endif

#ifndef sc_isprint
#define sc_isprint(c)   ((__sc_ismask(c)&(_MDB_P|_MDB_U|_MDB_L|_MDB_D|_MDB_SP)) != 0)
#endif

#ifndef sc_ispunct
#define sc_ispunct(c)   ((__sc_ismask(c)&(_MDB_P)) != 0)
#endif

#ifndef sc_isspace
#define sc_isspace(c)   ((__sc_ismask(c)&(_MDB_S)) != 0)
#endif

#ifndef sc_isupper
#define sc_isupper(c)   ((__sc_ismask(c)&(_MDB_U)) != 0)
#endif

#ifndef sc_isxdigit
#define sc_isxdigit(c)  ((__sc_ismask(c)&(_MDB_D|_MDB_X)) != 0)
#endif

#ifndef sc_isascii
#define sc_isascii(c)   (((unsigned char)(c))<=0x7f)
#endif

#ifndef sc_toascii
#define sc_toascii(c)   (((unsigned char)(c))&0x7f)
#endif

#endif
