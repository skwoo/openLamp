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

#ifndef TRANS_H
#define TRANS_H

#include "mdb_config.h"
#include "mdb_LSN.h"
#include "mdb_inner_define.h"
#include "mdb_LockMgrPI.h"
#include "mdb_ppthread.h"
#include "mdb_UndoTransTable.h"

#define MAX_TRANS             21
#define MAX_TRANS_ID         (MAX_TRANS * 1000000)

#define TRANS_STATE_BEGIN   1
#define TRANS_STATE_COMMIT  2
#define TRANS_STATE_ABORT   3
#define TRANS_STATE_PARTIAL_ROLLBACK 4

/*
fReadOnly
    - logging 하지 않음
        LogMgr_WriteLogRecord()
    - undo 하지않음
        LogMgr_PartailUndoTransaction()
        LogMgr_UndoTransaction()
    - 다음 레코드를 읽을 때 트랜잭션 내 같은 cursor에 의해서 insert/update가 
      수행된 경우 레코드를 읽어 utime과 qsid 값이 같으면 skip을 해야되는데 
      이 때 레코드를 읽는 부담을 줄이기 위해서 fReadOnly로 미리 판단함.
      즉, fReadOnly == 1 이면 insert/update된 레코드가 없기 때문에 레코드를
      읽어서 utime과 qsid를 비교할 필요가 없음. 단, filter가 없는 경우
        Iter_FindNext()
    - commit/abort 시 log buffer flush를 하지 않음
      체크포인트를 하기 위한 트랜잭션 수에 포함되지 않음
        dbi_Trans_Commit(), dbi_Trans_Abort()
    - 기본값은 1로 되어 있고 insert/update/remove 등 operation이 발생하면
      0으로 설정됨. 단, temp table인 경우 0으로 설정되지 않음
        dbi_Cursor_Insert(), dbi_Cursor_Distinct_Insert()
        dbi_Cursor_SetNullField()
        dbi_Cursor_Remove(), dbi_Cursor_Update(), dbi_Cursor_Update_Field()
        dbi_Direct_Remove2()
        dbi_Direct_Remove(), dbi_Direct_Update()
        dbi_Direct_Update_Field(), dbi_Cursor_Upsert()
      DDL은 temp table 상관없이 0으로 설정함.
        dbi_Relation_Create()
        dbi_Relation_Rename(), dbi_Relation_Drop(), dbi_Relation_Truncate()
        dbi_Index_Create(), dbi_Index_Create_With_KeyInsCond()
        dbi_Index_Rename(), dbi_Index_Drop(), dbi_Index_Rebuild()
        dbi_Sequence_Create(), dbi_Sequence_Alter(), dbi_Sequence_Drop()
        dbi_Sequence_Read(), dbi_Sequence_Nextval(), dbi_Relation_Logging()
    - 0번 trans는 내부 트랜잭션으로 0으로 설정되어 있음.
        TransMgr_Init()
      
fLogging
    - commit/abort 시에 log buffer flush를 결정함
      체크포인트를 하기 위한 트랜잭션 수 설정에 포함됨
        dbi_Trans_Commit(), dbi_Trans_Abort()
    - 기본값은 0이고, write operaton이 발생하면 1로 설정됨
      단, temp table과 nologging table이 아닌 경우 설정됨
        dbi_Cursor_Insert(), dbi_Cursor_Distinct_Insert(),
        dbi_Cursor_SetNullField(), dbi_Cursor_Remove(), dbi_Cursor_Update()
        dbi_Cursor_Update_Field(), dbi_Direct_Remove()
        dbi_Direct_Remove2(), dbi_Direct_Update(), dbi_Direct_Update_Field()
        dbi_Relation_Create(), dbi_Relation_Rename(), dbi_Relation_Drop()
        dbi_Relation_Truncate()
        dbi_Index_Create(), dbi_Index_Create_With_KeyInsCond()
        dbi_Index_Rename(), dbi_Index_Drop(), dbi_Index_Rebuild()
        dbi_Sequence_Create(), dbi_Sequence_Alter(), dbi_Sequence_Drop()
        dbi_Sequence_Read(), dbi_Sequence_Nextval(), dbi_Relation_Logging()
        dbi_Cursor_Upsert()
        
fDeleted (사용하는 곳 없음)
    - 기본값은 0이고, remove operaton 발생시 1로 설정됨
        dbi_Cursor_Remove(), dbi_Direct_Remove(), dbi_Direct_Remove2()
        dbi_Relation_Sync_removeResidue()
    - 0번 trans는 내부 트랜잭션으로 1로 설정되어 있음
        TransMgr_Init()

fBeginLogged
    - begin transacton log가 write 되었는지를 표시함.
      read only인 transaction인 경우 로그를 기록하지 않기 때문에 필요.
      기본값은 0이고, write 되었다면 1로 설정됨.
        LogMgr_WriteLogRecord()
    - 0번 trans는 내부 트랜잭션으로 1로 설정되어 있음
        TransMgr_Init()

fddl
    - 기본값은 0이고 DDL 수행되면 1로 설정됨
    - 설정되어 있으면 commit/abort 시 체크포인트 발생 시킴        
        dbi_Trans_Commit(), dbi_Trans_Abort()
    - DDL operaton 중에서도 필요한 곳에서 사용
      temp table이 아닐 때 설정. relation create와 같이 log에 의해서
      recovery가 가능한 곳에서는 fddl 설정을 제거함.
        dbi_Relation_Drop(), dbi_Relation_Truncate(), dbi_Index_Create()
        dbi_Index_Create_With_KeyInsCond()
        dbi_Index_Drop(), dbi_Index_Rebuild()

fRU
    - for READ UNCOMMITTED isolation transaction.
      default 0. 1 if TX is set to READ_UNCOMMITTED.

fUseResult
    - for READ UNCOMMITTED isolation transaction.
      default 0. 1 if fetch using USE RESULT method

      NOTE: Combination of USE RESULT and index scan can cause invalid TTree
            node access under READ UNCOMMITTED transaction. Therefore, we
            should be carefull when cursor open time is older than TTree
            modification time when fUseResult are set to 1.
*/

struct Trans
{
    int id_;

    enum PRIORITY prio_;

    trans_lock_t trans_lock;

    int procId;                 // 이 트랜잭션을 발생시킨 프로세스 번호 for shm connection
    pthread_t thrId;

    int clientType;
    int connid;                 /* connection 종료될 때 temp table 제거하기 위해서 */

    unsigned volatile int fRU:1,        // default 0. 1 if TX is set to READ_UNCOMMITTED.
      fUseResult:1,             // default 0. 1 if TX uses USE RESULT method
      fLogging:1,               //초기값은 0이고 logging table 연산시 1으로 변경
      fReadOnly:1,              //초기값은 1이고 update/insert/update 발생시 0으로 변경
      fDeleted:1,               //초기값은 0이고 delete 발생시 1로 변경
      fBeginLogged:1,           //초기값은 0이고 begin log를 남겼으면 1로 변경
      fddl:1;                   //초기값은 0이고 ddl을 수행했으면 1로 변경
    unsigned volatile char fUsed;       //사용중인지를 나타냄

    DB_BOOL visited_;

    struct LSN First_LSN_;      // 트랜잭션의 첫번째 LSN
    struct LSN Last_LSN_;       // 마지막 LSN
    struct LSN UndoNextLSN_;    // undo를 위한 다음 LSN, 역방향?

    struct LSN Implicit_SaveLSN_;       // implicit savepoint LSN for stmt atomicity 

    volatile DB_BOOL timeout_;
    volatile DB_BOOL aborted_;

    volatile DB_BOOL trans_lock_wait_;
    volatile DB_BOOL dbact_lock_wait_;

    unsigned char state;        /* TRANS_STATE_BEGIN, 
                                   TRANS_STATE_COMMIT, TRANS_STATE_ABORT */
    unsigned char interrupt;
    /* int num_operation; */

    int temp_serial_num;

    int tran_next;

    struct Cursor *cursor_list;

    pthread_mutex_t trans_mutex;        /* for sleeping (lock waiting) */
    pthread_cond_t wait_cond_;  /* for sleeping (lock waiting) */
};

struct Trans4Log
{
    int trans_id_;
    struct LSN Last_LSN_;
    struct LSN UndoNextLSN_;
};

struct TransMgr
{
    int NextTransID_;
    int Trans_Count_;
};

extern __DECL_PREFIX struct TransMgr *trans_mgr;
extern __DECL_PREFIX struct Trans *Trans_;

extern __DECL_PREFIX int my_trans_id;
extern __DECL_PREFIX struct Trans _trans_0;

int TransMgr_Init(void);
void Trans_Mgr_Free(void);
int *TransMgr_alloc_transid_key(void);
int TransMgr_InsTrans(int Trans_Num, int clientType, int connid);
int TransMgr_DelTrans(int);
void TransMgr_Set_NextTransID(int Trans_Num);
struct Trans *Trans_Search(int Trans_ID);
int Trans_Get_State(int Trans_ID);

int Trans_GetTransID_pid(int pid);

int TransMgr_trans_abort(int transid);

int TransMgr_Implicit_Savepoint(int transid);
int TransMgr_Implicit_Savepoint_Release(int transid);
int TransMgr_Implicit_Partial_Rollback(int transid);
int TransMgr_NextTempSerialNum(int *TransIdx, int *TempSerialNum);
int TransMgr_Find_By_ConnId(int connid);

int Trans_Mgr_AllTransGet(int *Count, struct Trans4Log **trans);
int TransTable_Insert(struct UndoTransTable *trans_table, struct LSN *lsn);
void TransTable_Remove_LSN(struct UndoTransTable *trans_table,
        struct LSN *lsn);
void TransTable_Replace_LSN(struct UndoTransTable *trans_table,
        struct LSN *lsn, struct LSN *return_lsn);
int Trans_Mgr_ALLTransDel(void);

#endif
