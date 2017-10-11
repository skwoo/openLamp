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

#include "mdb_config.h"
#include "sql_datast.h"
#include "ErrorCode.h"

#define    MAX_CNF_SIZE    100

typedef struct t_node
{
    T_OPTYPE type;
    int start;
    int end;
    struct t_node *left;
    struct t_node *right;
} T_NODE;

int sql_normalizing(int *handle, T_STATEMENT * stmt);
void clean_expression(T_EXPRESSIONDESC * expr);
T_NODE *trans_setlist2tree(T_EXPRESSIONDESC * expr);
void free_tree(T_NODE * node);
