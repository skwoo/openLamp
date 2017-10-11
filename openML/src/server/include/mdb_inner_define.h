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

#ifndef _S_DEFINE_H
#define _S_DEFINE_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include "mdb_config.h"
#include "mdb_define.h"
#include "mdb_typedef.h"

#define _alignedlen(len, base) ((((len) - 1) / (base) + 1) * (base))
#define _checkalign(len, base) (_alignedlen(len,base) - (len))

#define KB              1024

#define MDB_FILE_NAME   256
#define MAX_INDEX_NO    64

#define S_PAGE_SIZE     (8*KB)

#if defined(SEGMENT_NO_65536)
#define SEGMENT_NO      (65536)
#else
#define SEGMENT_NO      (8192)
#endif

#define PAGE_PER_SEGMENT    2   //4

#define SEQ_CURSOR              0
#define TTREE_FORWARD_CURSOR    1
#define TTREE_BACKWARD_CURSOR   2

#define PARTIAL_REBUILD_INDEX   1
#define ALL_REBUILD_INDEX       2
#define PARTIAL_REBUILD_DDL     3

#define MAX_DB_NAME             MDB_FILE_NAME
#define NULL_PID                0
#define FULL_PID                1

#define MAX_ALLOC_CURSORID_NUM  20

#define MAX_ALLOC_CURSORLISTID_NUM  (MAX_ALLOC_CURSORID_NUM/2)
#define MAX_CURSOR_ID               (MAX_ALLOC_CURSORID_NUM*3/2)


// 정수형의 최대값은???
#define NULL_Cursor_Position    -1
#define NULL_CURSOR_ID          -1

#define TTREE_FORWARD       1
#define TTREE_BACKWARD      2

#define MAX_PRIO            127

#define MAX_RECORD_SIZE     (4*KB)

#define MAX_MSG_SIZE (MAX_RECORD_SIZE * 2)
#define MAX_RCV_SIZE (MAX_RECORD_SIZE + 200)

    enum PRIORITY
    {
        LOWPRIO = 0,
        MIDPRIO = 1,
        HIGHPRIO = 2,
        PRIO_DUMMY = MDB_INT_MAX        /* to guarantee sizeof(enum) == 4 */
    };

#define WAIT        0
#define TIMEWAIT    1
#define NOWAIT      2

#define PARTIAL_REBUILD     1
#define ALL_REBUILD         2

#define CEIL_MACRO(numerator, denominator) \
    (((numerator) + ((denominator) - 1)) / (denominator))

#ifdef  __cplusplus
}
#endif

#endif
