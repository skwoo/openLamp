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

#ifndef __CONTITER_H
#define __CONTITER_H

#include "mdb_config.h"
#include "mdb_Iterator.h"

struct ContainerIterator
{
    struct Iterator iterator_;
    OID first_;
    OID last_;
    OID curr_;
};

struct ContainerIterator_Postion
{
    int csrID_;
    OID first_;
    OID last_;
    OID curr_;
};

int ContIter_Create(struct ContainerIterator *ContIter, OID page);

#endif
