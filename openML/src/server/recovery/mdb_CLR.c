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
#include "mdb_LogMgr.h"
#include "mdb_CLR.h"
#include "mdb_Trans.h"
#include "mdb_inner_define.h"
#include "mdb_compat.h"

int TransMgr_UpdateLastLSN(int TransID, struct LSN *lsn);
int UpdateTransTable(struct LogRec *logrec);
int LogFile_ReadFromBackup(struct LSN *lsn, struct LogRec *);
int LogRec_Undo(struct LogRec *logrec, int flag);

int InsertLog_Redo(struct LogRec *logrec);
int DeleteLog_Redo(struct LogRec *logrec);

/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
CLR_WriteCLR(struct LogRec *logrec)
{
    struct CLRLog clrlog;

    TransMgr_UpdateLastLSN(logrec->header_.trans_id_,
            &logrec->header_.trans_prev_lsn_);

    LogRec_Init(&(clrlog.logrec));

    clrlog.logrec.header_.type_ = COMPENSATION_LOG;
    clrlog.logrec.header_.op_type_ = logrec->header_.type_;     /* NOTE */
    clrlog.logrec.header_.trans_id_ = logrec->header_.trans_id_;
    clrlog.logrec.header_.oid_ = logrec->header_.oid_;
    clrlog.logrec.header_.lh_length_ = sizeof(struct LSN);
    clrlog.logrec.header_.recovery_leng_ = sizeof(struct LSN);
    clrlog.logrec.data1 = (char *) &logrec->header_.record_lsn_;

    LogMgr_WriteLogRecord((struct LogRec *) &clrlog);

    return DB_SUCCESS;
}

int
CLR_WriteCLR_For_InsDel(struct LogRec *logrec, char *log_data, int log_size)
{
    struct CLRLog clrlog;

    TransMgr_UpdateLastLSN(logrec->header_.trans_id_,
            &logrec->header_.trans_prev_lsn_);

    LogRec_Init(&(clrlog.logrec));

    clrlog.logrec.header_.type_ = COMPENSATION_LOG;
    clrlog.logrec.header_.op_type_ = logrec->header_.type_;     /* NOTE */
    clrlog.logrec.header_.trans_id_ = logrec->header_.trans_id_;
    clrlog.logrec.header_.oid_ = logrec->header_.oid_;
    clrlog.logrec.header_.relation_pointer_ =
            logrec->header_.relation_pointer_;
    clrlog.logrec.header_.collect_index_ = logrec->header_.collect_index_;
    clrlog.logrec.header_.tableid_ = logrec->header_.tableid_;

    clrlog.logrec.header_.recovery_leng_ = log_size;
    clrlog.logrec.header_.lh_length_ = log_size;
    clrlog.logrec.data1 = log_data;

    LogMgr_WriteLogRecord((struct LogRec *) &clrlog);

    return DB_SUCCESS;
}

/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
CompensationLog_Undo(struct LogRec *logrec)
{
    return DB_SUCCESS;
}

/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
CompensationLog_Redo(struct LogRec *logrec)
{
    if (logrec->header_.type_ != COMPENSATION_LOG)
    {
        return DB_FAIL;
    }

    if (logrec->header_.op_type_ == INSERT_LOG)
    {
        if (logrec->header_.recovery_leng_ > 0)
            DeleteLog_Redo(logrec);
        else
            UpdateTransTable(logrec);
    }
    else if (logrec->header_.op_type_ == DELETE_LOG)
    {
        if (logrec->header_.recovery_leng_ > 0)
            InsertLog_Redo(logrec);
        else
            UpdateTransTable(logrec);
    }
    else    /* other cases */
    {
        struct LogRec logrec_back;

        logrec_back.data1 = mdb_malloc(16 * 1024);
        sc_memset(logrec_back.data1, 0, 16 * 1024);
        logrec_back.data2 = NULL;

        LogFile_ReadFromBackup((struct LSN *) logrec->data1, &logrec_back);

        LogRec_Undo(&logrec_back, NO_CLR);
        UpdateTransTable(logrec);

        mdb_free(logrec_back.data1);
        logrec_back.data1 = NULL;
    }

    return DB_SUCCESS;
}
