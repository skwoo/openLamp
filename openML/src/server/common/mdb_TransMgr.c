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

#include "mdb_Server.h"
#include "mdb_compat.h"

__DECL_PREFIX int my_trans_id = 1;

__DECL_PREFIX struct TransMgr *trans_mgr;
__DECL_PREFIX struct Trans *Trans_;

static void create_trans_id_key(void);
static void destroy_trans_id_key(void);

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
TransMgr_Init()
{
    int i;

    trans_mgr->NextTransID_ = 1;
    trans_mgr->Trans_Count_ = 0;

    sc_memset(Trans_, 0, sizeof(struct Trans) * MAX_TRANS);

    for (i = 0; i < MAX_TRANS; i++)
    {
        Trans_[i].id_ = i;
        if (ppthread_mutex_init(&Trans_[i].trans_mutex, &_mutex_attr))
        {
            MDB_SYSLOG(("Trans[%d] Mutex Locking Initialize FAIL\n", i));
            return DB_E_INITMUTEX;
        }
        if (ppthread_cond_init(&Trans_[i].wait_cond_, &_cond_attr))
        {
            MDB_SYSLOG(("Trans[%d] wait cond Initialize FAIL\n", i));
            return DB_E_INITCOND;
        }
    }

    Trans_[0].fUsed = 0;
    Trans_[0].fReadOnly = 0;
    Trans_[0].fDeleted = 1;
    Trans_[0].fBeginLogged = 1;
    Trans_[0].fddl = 0;

    TransMgr_alloc_transid_key();

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
int *
TransMgr_alloc_transid_key()
{
    int *p_transid;

    create_trans_id_key();

    p_transid = CS_getspecific(my_trans_id);
    if (p_transid == NULL)
    {
        CS_setspecific_int(my_trans_id, (void **) &p_transid);
    }

    *p_transid = -1;

    return p_transid;
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
static void
create_trans_id_key(void)
{
    my_trans_id = 1;
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
static void
destroy_trans_id_key(void)
{
    return;
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
void
TransMgr_Set_NextTransID(int Trans_Num)
{
    Trans_Num++;
    if (Trans_Num >= MAX_TRANS_ID)
        trans_mgr->NextTransID_ = 1;
    else
        trans_mgr->NextTransID_ = Trans_Num;
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
TransMgr_InsTrans(int Trans_Num, int clientType, int connid)
{
    struct Trans *trans;

  start:
    if (Trans_Num == 0 && connid == -1) /* 내부용. trans id 0 할당 */
    {
        if (Trans_[0].fUsed)    /* wait */
        {
            goto start;
        }
    }

    if (Trans_Num == -1)
    {
        int i, j;

        j = 0;

        while (1)
        {
            i = trans_mgr->NextTransID_ % MAX_TRANS;

            if (i != 0 && Trans_[i].fUsed == 0)
            {
                Trans_[i].fUsed = 1;

                trans = &Trans_[i];
                trans->id_ = trans_mgr->NextTransID_;

                trans_mgr->Trans_Count_++;
                TransMgr_Set_NextTransID(trans_mgr->NextTransID_);

                break;
            }

            TransMgr_Set_NextTransID(trans_mgr->NextTransID_);

            j++;

            if (j > MAX_TRANS)
            {
                MDB_SYSLOG(("Fail: trans insert, no more tx accepted\n"));
                return DB_E_NOMORETRANS;
            }
        }
    }
    else
    {
        Trans_[Trans_Num % MAX_TRANS].fUsed = 1;
        trans = &Trans_[Trans_Num % MAX_TRANS];
        trans->id_ = Trans_Num;
        trans_mgr->Trans_Count_++;

        /* trans id는 restart 후에도 계속 증가하도록 함.
           row-level lock에서 restart 후 record에 남아 있는
           trans id를 무시하도록 하기 위해서... */
        if (Trans_Num != 0)
            TransMgr_Set_NextTransID(Trans_Num);
    }

    trans->connid = connid;

    trans->fLogging = 0;
    trans->fReadOnly = 1;
    trans->fDeleted = 0;
    trans->fBeginLogged = 0;
    trans->fddl = 0;

    trans->First_LSN_.File_No_ = -1;
    trans->First_LSN_.Offset_ = -1;

    trans->Last_LSN_.File_No_ = -1;
    trans->Last_LSN_.Offset_ = -1;

    trans->UndoNextLSN_.File_No_ = -1;
    trans->UndoNextLSN_.Offset_ = -1;

    /* initialize implicit savepoint LSN */
    trans->Implicit_SaveLSN_.File_No_ = -2;
    trans->Implicit_SaveLSN_.Offset_ = -2;

    trans->prio_ = LOWPRIO;

    LockMgr_trans_lock_init(&trans->trans_lock, trans->id_);

    trans->procId = sc_getpid();        /* shm connection */
    trans->thrId = sc_get_taskid();     /* shm connection */
    trans->clientType = clientType;

    trans->timeout_ = FALSE;
    trans->aborted_ = FALSE;

    trans->trans_lock_wait_ = FALSE;
    trans->dbact_lock_wait_ = FALSE;

    trans->state = TRANS_STATE_BEGIN;

    trans->interrupt = FALSE;

    trans->temp_serial_num = 0;

    trans->tran_next = -1;

    trans->cursor_list = NULL;

    return trans->id_;
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
TransMgr_DelTrans(int Trans_ID)
{
    struct Trans *trans = NULL;

    trans = &Trans_[Trans_ID % MAX_TRANS];

    if (trans->fUsed == 0)
    {
        return DB_E_UNKNOWNTRANSID;
    }

    trans->aborted_ = 0;
    trans->fddl = 0;
    trans->cursor_list = NULL;
    trans->fUsed = 0;

    trans->procId = 0;
    trans->thrId = 0;

    trans->fLogging = 0;

    trans_mgr->Trans_Count_--;

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
struct Trans *
Trans_Search(int Trans_ID)
{
    struct Trans *trans;

    trans = &Trans_[Trans_ID % MAX_TRANS];

    if (trans->fUsed == 0)
        trans = NULL;

    return trans;
}

int
Trans_Get_State(int Trans_ID)
{
    struct Trans *trans;

    if (Trans_ID == -1)
    {       /* current transaction */
        Trans_ID = *(int *) CS_getspecific(my_trans_id);
    }

    trans = &Trans_[Trans_ID % MAX_TRANS];

    if (trans->fUsed == 0)
        return -1;

    return (int) (trans->state);
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
/* lock하고 사용됨 */
int
Trans_Mgr_Trans_Count(void)
{
    int i;
    int Count = 0;

    for (i = 1; i < MAX_TRANS; i++)
    {
        if (Trans_[i].fUsed == 1)
            Count++;
    }

    return Count;
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
Trans_Mgr_ALLTransDel()
{
    int i;

    for (i = 1; i < MAX_TRANS; i++)
    {
        Trans_[i].fUsed = 0;
    }

    trans_mgr->Trans_Count_ = 0;

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
Trans_Mgr_AllTransGet(int *Count, struct Trans4Log **trans)
{
    struct Trans *p;
    int Temp_Count = 0;
    int i;

    server->del_file_No = Log_Mgr->Current_LSN_.File_No_;

    *Count = Trans_Mgr_Trans_Count();

    if (*Count == 0)
    {
        *trans = NULL;
        return DB_SUCCESS;
    }

    *trans = (struct Trans4Log *) mdb_malloc(sizeof(struct Trans4Log) *
            (*Count));

    if (*trans == NULL)
    {
        return DB_E_UNKNOWNTRANSID;
    }

    for (i = 1; i < MAX_TRANS; i++)
    {
        p = &Trans_[i];

        if (p->fUsed == 0)
            continue;

        (*trans)[Temp_Count].trans_id_ = p->id_;
        (*trans)[Temp_Count].Last_LSN_ = p->Last_LSN_;
        (*trans)[Temp_Count].UndoNextLSN_ = p->UndoNextLSN_;
        Temp_Count++;

        /* 현재 트랜잭션들중 가장 작은 LSN을 구한다.
           log file 제거시 사용. fReadOnly인 Tr은 LSN이 -1로 되어 있어
           의미 없음... */
        if (p->fReadOnly || p->First_LSN_.File_No_ == -1)
            continue;

        if (p->First_LSN_.File_No_ < server->del_file_No)
            server->del_file_No = p->First_LSN_.File_No_;
    }

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
TransMgr_UpdateLastLSN(int TransID, struct LSN *lsn)
{
    struct Trans *trans = NULL;

    trans = &Trans_[TransID % MAX_TRANS];

    if (trans->fUsed == 0)
    {
        return DB_E_UNKNOWNTRANSID;
    }

    trans->UndoNextLSN_ = *lsn;

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
void
Trans_Mgr_Free()
{
    int i;
    void *ptr = NULL;

    if (trans_mgr != NULL)
    {
        for (i = 0; i < MAX_TRANS; i++)
        {
            ppthread_mutex_destroy(&Trans_[i].trans_mutex);
        }
    }

    ptr = CS_getspecific(my_trans_id);
    if (ptr != NULL)
    {
        destroy_trans_id_key();
        CS_setspecific_int(my_trans_id, NULL);
    }

    return;
}

/*
   deadlock detection에서 사용.
   트랜잭션수가 2 이상일 때 SUCCESS return
*/
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
Trans_ClearVisited()
{
    int count;
    int i;

    count = trans_mgr->Trans_Count_;    //Trans_Mgr_Trans_Count();

    if (count < 2)
    {
        return -1;
    }

    for (i = 1; i < MAX_TRANS; i++)
    {
        Trans_[i].visited_ = FALSE;
    }

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
TransMgr_trans_abort(int transid)
{
    /* open된 cursor를 닫는다 */
    Cursor_Scan_Dangling(transid);

    LogMgr_UndoTransaction(transid);

    if (LockMgr_Unlock(transid) < 0)
    {
        MDB_SYSLOG(("unlock failed for transaction %d to be aborted\n",
                        transid));
    }

    TransMgr_DelTrans(transid);

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
TransMgr_Implicit_Savepoint(int transid)
{
    struct Trans *trans;

    trans = (struct Trans *) Trans_Search(transid);
    if (trans == NULL)
    {
        return DB_E_NOTTRANSBEGIN;
    }

    LSN_Set(trans->Implicit_SaveLSN_, trans->Last_LSN_);

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
TransMgr_Implicit_Savepoint_Release(int transid)
{
    struct Trans *trans;

    trans = (struct Trans *) Trans_Search(transid);
    if (trans == NULL)
    {
        return DB_E_NOTTRANSBEGIN;
    }

    /* initialize implicit savepoint LSN */
    trans->Implicit_SaveLSN_.File_No_ = -2;
    trans->Implicit_SaveLSN_.Offset_ = -2;

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
TransMgr_Implicit_Partial_Rollback(int transid)
{
    struct Trans *trans;
    int ret;

    trans = (struct Trans *) Trans_Search(transid);
    if (trans == NULL)
    {
        return DB_E_UNKNOWNTRANSID;
    }

    trans->state = TRANS_STATE_PARTIAL_ROLLBACK;

    ret = LogMgr_PartailUndoTransaction(transid, &trans->Implicit_SaveLSN_);

    trans->state = TRANS_STATE_BEGIN;

    /* initialize implicit savepoint LSN */
    trans->Implicit_SaveLSN_.File_No_ = -2;
    trans->Implicit_SaveLSN_.Offset_ = -2;

    return ret;
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
TransMgr_NextTempSerialNum(int *TransIdx, int *TempSerialNum)
{
    int transid = *(int *) CS_getspecific(my_trans_id);
    struct Trans *trans = NULL;

    if (transid == -1)
        return DB_E_NOTTRANSBEGIN;

    trans = &Trans_[transid % MAX_TRANS];
    if (trans->fUsed == 0)
        return DB_E_UNKNOWNTRANSID;

    trans->temp_serial_num++;

    *TransIdx = transid;
    *TempSerialNum = trans->temp_serial_num;

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
void
TransMgr_Get_MinFirstLSN(struct LSN *minLSN)
{
    int i;

    /* Currently, Log_Mgr->LogMgr->Mutex_ is held. */

    minLSN->File_No_ = -1;
    minLSN->Offset_ = -1;

    for (i = 0; i < MAX_TRANS; i++)
    {
        if (Trans_[i].fUsed == 0)
            continue;

        if (Trans_[i].First_LSN_.File_No_ == -1)
            continue;

        if ((minLSN->File_No_ == -1) ||
                (minLSN->File_No_ > Trans_[i].First_LSN_.File_No_) ||
                (minLSN->File_No_ == Trans_[i].First_LSN_.File_No_ &&
                        minLSN->Offset_ > Trans_[i].First_LSN_.Offset_))
        {
            minLSN->File_No_ = Trans_[i].First_LSN_.File_No_;
            minLSN->Offset_ = Trans_[i].First_LSN_.Offset_;
        }
    }
}
