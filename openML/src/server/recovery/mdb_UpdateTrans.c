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
#include "dbport.h"
#include "mdb_LogRec.h"
#include "mdb_Trans.h"
#include "mdb_inner_define.h"
#include "mdb_UndoTransTable.h"
#include "mdb_comm_stub.h"

int TransTable_Upheep_LSN(struct UndoTransTable *trans_table, int index);

/*****************************************************************************/
//! UpdateTransTable 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param logrec :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
UpdateTransTable(struct LogRec *logrec)
{
    struct Trans *trans;
    int trans_num;

    if ((trans = (struct Trans *) Trans_Search(logrec->header_.trans_id_)) ==
            NULL)
    {
        trans_num = TransMgr_InsTrans(logrec->header_.trans_id_,
                CS_CLIENT_NORMAL, -1);

        trans = (struct Trans *) Trans_Search(trans_num);

    }

    LSN_Set(trans->Last_LSN_, logrec->header_.record_lsn_);
    LSN_Set(trans->UndoNextLSN_, logrec->header_.trans_prev_lsn_);

    return DB_SUCCESS;
}

/*****************************************************************************/
//! TransTable_Insert 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param trans_table :
 * \param lsn : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
TransTable_Insert(struct UndoTransTable *trans_table, struct LSN *lsn)
{
    trans_table->Num_Of_LSN_ += 1;
    trans_table->max_lsn_[trans_table->Num_Of_LSN_] = *lsn;
    TransTable_Upheep_LSN(trans_table, trans_table->Num_Of_LSN_);
    return DB_SUCCESS;
}

/*****************************************************************************/
//! TransTable_Upheep_LSN 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param trans_table :
 * \param index : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
TransTable_Upheep_LSN(struct UndoTransTable *trans_table, int index)
{
    struct LSN tempLSN;

    tempLSN = trans_table->max_lsn_[index];
    trans_table->max_lsn_[0].File_No_ = MDB_INT_MAX;
    trans_table->max_lsn_[0].Offset_ = MDB_INT_MAX;

    while (trans_table->max_lsn_[index / 2].File_No_ <= tempLSN.File_No_
            && trans_table->max_lsn_[index / 2].Offset_ <= tempLSN.Offset_)
    {
        trans_table->max_lsn_[index] = trans_table->max_lsn_[index / 2];
        index = index / 2;
    }

    trans_table->max_lsn_[index] = tempLSN;
    return DB_SUCCESS;
}

/*****************************************************************************/
//! TransTable_Downheap_LSN 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param trans_table :
 * \param index : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void
TransTable_Downheap_LSN(struct UndoTransTable *trans_table, int index)
{
    struct LSN tempLSN = trans_table->max_lsn_[index];

    while (index <= trans_table->Num_Of_LSN_ / 2)
    {
        int i;

        i = index + index;
        if (i < trans_table->Num_Of_LSN_
                && ((trans_table->max_lsn_[i].File_No_ <=
                                trans_table->max_lsn_[i + 1].File_No_)
                        && (trans_table->max_lsn_[i].Offset_ <=
                                trans_table->max_lsn_[i + 1].Offset_)))
        {
            i++;
        }
        if ((tempLSN.File_No_ >= trans_table->max_lsn_[i].File_No_)
                && (tempLSN.Offset_ >= trans_table->max_lsn_[i].Offset_))
        {
            break;
        }
        trans_table->max_lsn_[index] = trans_table->max_lsn_[i];
        index = i;
    }
    trans_table->max_lsn_[index] = tempLSN;
}

/*****************************************************************************/
//! TransTable_Replace_LSN 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param trans_table :
 * \param lsn : 
 * \param return_lsn :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void
TransTable_Replace_LSN(struct UndoTransTable *trans_table, struct LSN *lsn,
        struct LSN *return_lsn)
{
    trans_table->max_lsn_[0] = *lsn;
    TransTable_Downheap_LSN(trans_table, 0);
    *return_lsn = trans_table->max_lsn_[0];
}

/*****************************************************************************/
//! TransTable_Remove_LSN 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param trans_table :
 * \param lsn : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void
TransTable_Remove_LSN(struct UndoTransTable *trans_table, struct LSN *lsn)
{
    struct LSN tempLSN;

    tempLSN = trans_table->max_lsn_[1];
    trans_table->max_lsn_[1] = trans_table->max_lsn_[trans_table->Num_Of_LSN_];
    trans_table->Num_Of_LSN_--;
    TransTable_Downheap_LSN(trans_table, 1);
    *lsn = tempLSN;
}
