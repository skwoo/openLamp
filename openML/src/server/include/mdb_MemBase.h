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

#ifndef __MEMBASE_H
#define __MEMBASE_H


#include "mdb_config.h"
#include "mdb_typedef.h"
#include "mdb_inner_define.h"
#include "mdb_LSN.h"

struct MemBase
{
    DB_UINT32 allocated_segment_no_;
    PageID first_free_page_id_;
    DB_UINT32 free_page_count_;
    DB_UINT32 tallocated_segment_no_;   /* no logging data */
    PageID tfirst_free_page_id_;        /* no logging data */
    DB_UINT32 tfree_page_count_;        /* no logging data */
    PageID trs_first_free_page_id_;
    DB_UINT32 trs_free_page_count_;
    DB_UINT32 trs_segment_count_;       /* resident segment count */
    DB_UINT32 iallocated_segment_no_;
    IPageID ifirst_free_page_id_;
    DB_UINT32 ifree_page_count_;
    DB_UINT32 majorVer:6,       /* 0 ~ 63 */
      minorVer:5,               /* 0 ~ 31 */
      releaseVer:5,             /* 0 ~ 31 */
      buildNO:16;               /* 0 ~ 65535 */
    /* for container and index */
    DB_INT8 mode;               /* 32 bit or 64 bit, sizeof(long) 값 저장 */
    char endian_type_;          /* B(big), L(little), M(middle) */
    DB_INT32 supported_segment_no_;     /* SEGMENT_NO 저장. 이 값이 다르면 index를
                                           새로 생성 해야함. */
    struct LSN anchor_lsn;

    /* applied engine verson */
    DB_UINT32 engine_majorVer:6,        /* 0 ~ 63 */
      engine_minorVer:5,        /* 0 ~ 31 */
      engine_releaseVer:5,      /* 0 ~ 31 */
      engine_buildNO:16;        /* 0 ~ 65535 */

    DB_UINT32 idxseg_num;
    DB_UINT32 tmpseg_num;

    int db_size_fixed;
    int language_code;
};
#endif /* __MEMBASE_H */
