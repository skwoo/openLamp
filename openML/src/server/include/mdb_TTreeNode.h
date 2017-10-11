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

#ifndef __TTREENODE_H
#define __TTREENODE_H

#include "mdb_config.h"
#include "mdb_typedef.h"
#include "mdb_inner_define.h"

#define ROOT_NODE   0x00
#define LEFT_CHILD  0x10
#define RIGHT_CHILD 0x01

/* TTREE NODE MAX VALUE */
#define TNODE_MAX_ITEM 121
#define TNODE_MIN_ITEM (TNODE_MAX_ITEM-1)

#define TNODE_KEYVAL_LEN    8

struct TTreeNode
{
    NodeID self_;
    NodeID parent_;
    NodeID left_;
    NodeID right_;
    DB_INT16 item_count_;       /* 16 bit */
    Height height_;             /* 8 bit */
    NodeType type_;             /* 8 bit */
    RecordID item_[TNODE_MAX_ITEM];
};

#define TTREE_SLOT_SIZE _alignedlen(sizeof(struct TTreeNode) + 1, sizeof(long))

extern struct TTreeNode *TTree_NextNode(struct TTreeNode *node);
extern struct TTreeNode *TTree_PrevNode(struct TTreeNode *node);

#endif
