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

#ifndef    _UCS2_VS_UTF_
#define    _UCS2_VS_UTF_

#ifdef  __cplusplus
extern "C"
{
#endif

#include "mdb_config.h"
#include "dbport.h"
#include "mdb_define.h"

/* **************************************************************************************************** 
** Notice!
** UCS2는 utf16으로 표현되는 문자셋중에서 BMP에 해당하는 영역이며, surrogate 영역을 이용하지 않는다.
** 때문에 이 UCS2는 2바이트 크기로 하나의 문자를 표현하게 된다.
** Convert_UCS2toUTF8(), Convert_UTF8toUCS2() 함수는 오직 BMP영역인 2바이트 unicode에 대해서만 처리한다.
******************************************************************************************************** */

    typedef unsigned long UTF32;        /* at least 32 bits */
    typedef unsigned short UTF16;       /* at least 16 bits */
    typedef unsigned char UTF8; /* at least 8 bits */
    typedef unsigned short UCS2;        /* typically 16 bits */

#define     IRS_CONVERT_SOURCE1             -1001460
#define     IRS_CONVERT_SOURCE2             -1001461
#define     IRS_CONVERT_ALLOC1              -1001462
#define     IRS_CONVERT_ALLOC2              -1001463
#define     IRS_CONVERT_INVALIDPARAM1       -1001464
#define     IRS_CONVERT_INVALIDPARAM2       -1001465
#define     IRS_CONVERT_OVERBMP1            -1001466
#define     IRS_CONVERT_OVERBMP2            -1001467
#define     IRS_CONVERT_ILLEGAL1            -1001468
#define     IRS_CONVERT_ILLEGAL2            -1001469
#define     IRS_CONVERT_DESTSIZE1           -1001470
#define     IRS_CONVERT_DESTSIZE2           -1001471
#define     IRS_CONVERT_UNKNOWN1            -1001472
#define     IRS_CONVERT_UNKNOWN2            -1001473

#define    __GetUTF8ByteSize_Internal(c)  ( (!((c) & 0x80)) ? 1 : (((c) >> 5) == 0x06) ? 2 : (((c) >> 4) == 0x0e) ? 3 : 0 )

#define    MC_GetUTF8ByteSize(c)  ( __GetUTF8ByteSize_Internal((u_char)(c))  )

#define    MC_Is3UTF8SpecialChar(ch, ch2, ch3) \
(\
    ((ch) == 0xe2 &&\
        (((ch2) == 0x80 && ((ch3) == 0x98 || (ch3) == 0x99 || (ch3) == 0x9c ||\
                         (ch3) == 0x9d || (ch3) == 0xa6 || (ch3) == 0xbb )) ||\
        ((ch2) == 0x97 && (ch3) == 0x8e))\
    ) ||\
    ((ch) == 0xe3 && (ch2) == 0x80 &&\
        ((ch3) == 0x81 || (ch3) == 0x82 || (ch3) == 0x88 || (ch3) == 0x89 ||\
         (ch3) == 0x8a || (ch3) == 0x8b || (ch3) == 0x8c || (ch3) == 0x8d ||\
         (ch3) == 0x8e || (ch3) == 0x8f || (ch3) == 0x90 || (ch3) == 0x91  )\
    ) ||\
    ((ch) == 0xef &&\
        (((ch2) == 0xb9 && ((ch3) == 0x9b || (ch3) == 0x9c || (ch3) == 0x9d ||\
                         (ch3) == 0x9e || (ch3) == 0x9f || (ch3) == 0xa1 )) ||\
        ((ch2) == 0xbc && ((ch3) == 0x81 || (ch3) == 0x85 || (ch3) == 0x86 ||\
                         (ch3) == 0x88 || (ch3) == 0x89 || (ch3) == 0x8b ||\
                         (ch3) == 0x8c || (ch3) == 0x8d || (ch3) == 0x9a ||\
                         (ch3) == 0x9b || (ch3) == 0x9d || (ch3) == 0x9f )) ||\
        ((ch2) == 0xbf && (ch3) == 0xa5))\
    )\
)

#define    MC_Is2UTF8SpecialChar(ch, ch2)\
(\
    ((ch) == 0xc2 && (ch2) == 0xa7) ||\
    ((ch) == 0xc3 && ((ch2) == 0x97 || (ch2) == 0xb7)) ||\
    ((ch) == 0xd4 && (ch2) >= 0xb0) ||\
    ((ch) == 0xd5) ||\
    ((ch) == 0xd6 && (ch2) <= 0x8f)\
)



    __DECL_PREFIX int MDB_UCS2toUTF8(UCS2 * source, UTF8 ** dest,
            int *pdestsize);
    __DECL_PREFIX int MDB_UTF8toUCS2(UTF8 * source, UCS2 ** dest,
            int *pdestsize);
    DB_BOOL MDB_IsLegalUTF8(const UTF8 * source);



#ifdef  __cplusplus
}
#endif

#endif                          //    !_UCS2_VS_UTF_
