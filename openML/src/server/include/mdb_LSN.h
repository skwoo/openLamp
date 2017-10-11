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

#ifndef LSN_H
#define LSN_H

#include "mdb_config.h"

struct LSN
{
    int File_No_;
    int Offset_;
};

#define LogLSN_Set() { \
    Log_Mgr->Current_LSN_.File_No_ = Log_Mgr->File_.File_No_; \
    Log_Mgr->Current_LSN_.Offset_ = Log_Mgr->File_.Offset_; \
}

#define LogLSN_Init(lsn) { \
    (lsn)->File_No_ = -1; \
    (lsn)->Offset_ = -1; \
}

#define LSN_Set(LogRecLSN, Curr)    (LogRecLSN = Curr)

#endif
