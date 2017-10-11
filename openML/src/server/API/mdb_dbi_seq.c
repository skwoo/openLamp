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

static int
__check_valid_sequence(DB_SEQUENCE * pDbSequence)
{
    int ret = 0;

    // 1. error check : 이름이 0이 아니어야 한다.
    if (pDbSequence->sequenceName[0] == '\0')
    {
        ret = DB_E_SEQUENCE_INVALID_NAME;
        goto error;
    }

// 1. increment by
    if (pDbSequence->increaseNum == 0)
    {
        ret = DB_E_SEQUENCE_INVALID_INCNUM;
        goto error;
    }

    if (pDbSequence->minNum > pDbSequence->startNum)
    {
        ret = DB_E_SEQUENCE_INVALID_STARTNUM_MINNUM;
        goto error;
    }

    if (pDbSequence->minNum >= pDbSequence->maxNum)
    {
        ret = DB_E_SEQUENCE_INVALID_MINNUM_MAXNUM;
        goto error;
    }

    if (pDbSequence->maxNum < pDbSequence->startNum)
    {
        ret = DB_E_SEQUENCE_INVALID_STARTNUM_MAXNUM;
        goto error;
    }

    if (pDbSequence->increaseNum >=
            (pDbSequence->maxNum - pDbSequence->minNum))
    {
        ret = DB_E_SEQUENCE_INVALID_INCNUM_RANGE;
    }

    return DB_SUCCESS;

  error:
    return ret;

}


static int
__get_next_sequenceId(void)
{
    int cursorId;
    int transId = *(int *) CS_getspecific(my_trans_id);
    SYSSEQUENCE_REC_T sysSequenceRec;
    DB_UINT32 recSize;

    int newSequenceId;
    struct KeyValue minkey, maxkey;
    int ret;
    FIELDVALUE_T fieldvalue;

    if (server->nextSequenceId == -1)
    {
        cursorId =
                Open_Cursor_Desc(transId, "syssequences", LK_SHARED,
                0 /* $pk.syssequences */ , NULL, NULL, NULL);
        if (cursorId < 0)
        {
            return cursorId;
        }

        ret = __Read(cursorId, (char *) &sysSequenceRec, NULL, 0, &recSize, 0);
        if (ret < 0)
        {
            Close_Cursor(cursorId);
            return ret;
        }

        server->nextSequenceId = sysSequenceRec.sequenceId;

        // FIXME
        // sequence를 몇개 까지 지원할것인지를 체크해주는 루틴이 필요할듯하다.
        Close_Cursor(cursorId);
    }

//    firstSequenceId = server->nextSequenceId;
    while (1)
    {
        newSequenceId = server->nextSequenceId;

        server->nextSequenceId += 1;
        // FIXME
        // sequence를 몇개 까지 지원할것인지를 체크해주는 루틴이 필요할듯하다.
        ret = dbi_FieldValue(MDB_NN, DT_INTEGER, 0,
                mdb_offsetof(SYSSEQUENCE_REC_T, sequenceId), sizeof(int), 0,
                MDB_COL_NUMERIC, &newSequenceId, DT_INTEGER,
                sizeof(int), &fieldvalue);
        if (ret != DB_SUCCESS)
            return ret;
        dbi_KeyValue(1, &fieldvalue, &minkey);
        dbi_KeyValue(1, &fieldvalue, &maxkey);

        cursorId =
                Open_Cursor(transId, "syssequences", LK_SHARED,
                0 /* $pk.syssequences */ , &minkey, &maxkey, NULL);
        if (cursorId < 0)
        {
            return cursorId;
        }

        ret = __Read(cursorId, (char *) &sysSequenceRec, NULL, 0, &recSize, 0);

        if (ret < 0)
        {
            if (ret != DB_E_NOMORERECORDS)
            {
                Close_Cursor(cursorId);
                return ret;
            }
            /* DB_E_NOMORERECORDS: O.K. */
            Close_Cursor(cursorId);
            break;
        }
        Close_Cursor(cursorId);
    }

    return newSequenceId;
}



/*****************************************************************************/
//! dbi_Seqeunce_Create
/*! \breif  Sequence를 생성하는 함수
 ************************************
 * \param connid(in)        : connection id
 * \param pDbSeqence(in)    : Sequence에 대한 정의
 ************************************
 * \return ret < 0, is Error.
 *         ret = 0 is Success, It's New Sequence's id
 ************************************
 * \note
 *    - 내부 알고리즘\n
 *    - 유의 사항
 *        Systable에서 사용하는 Sequence에 대한 구조체는 SYSSEQUENCE_REC_T 이며
 *        Sequence를 정의하거나, 수정하는 경우에 사용하는 구조체는 DB_SEQUENCE 이다.
 *    - COLLATION INTERFACE 추가
 *
 *****************************************************************************/
int
dbi_Sequence_Create(int connid, DB_SEQUENCE * pDbSequence)
{
    int *pTransid;
    int curr_transid = 0;
    struct Trans *pTrans;
    DB_INT32 cursorId = -1;
    struct KeyValue minkey, maxkey;
    FIELDVALUE_T fieldvalue;
    int ret;

    SYSSEQUENCE_REC_T sysSequenceRec;
    DB_UINT32 recSize;
    OID recordId;
    DB_INT32 newSequenceId = -1;
    char *lowSequenceName;

    DBI_CHECK_SERVER_TERM();

    lowSequenceName = pDbSequence->sequenceName;

    ret = __check_valid_sequence(pDbSequence);
    if (ret < 0)
    {
        // Error 발생
        return ret;
    }

    pTransid = (int *) CS_getspecific(my_trans_id);
    curr_transid = *pTransid;
    if (curr_transid == -1)
    {
        ret = dbi_Trans_Begin(connid);
        if (ret < 0)
            goto end_return;
    }

    pTrans = (struct Trans *) Trans_Search(*pTransid);
    if (pTrans == NULL)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    ret = LockMgr_Lock("syssequences", *pTransid, LOWPRIO, LK_EXCLUSIVE, WAIT);
    if (ret < 0)
    {
        goto end;
    }

    ret = dbi_FieldValue(MDB_NN, DT_CHAR, 1,
            mdb_offsetof(SYSSEQUENCE_REC_T, sequenceName), FIELD_NAME_LENG, 0,
            MDB_COL_DEFAULT_SYSTEM, lowSequenceName, DT_CHAR,
            FIELD_NAME_LENG, &fieldvalue);
    if (ret != DB_SUCCESS)
        goto end;
    dbi_KeyValue(1, &fieldvalue, &minkey);
    ret = dbi_FieldValue(MDB_NN, DT_CHAR, 1,
            mdb_offsetof(SYSSEQUENCE_REC_T, sequenceName), FIELD_NAME_LENG, 0,
            MDB_COL_DEFAULT_SYSTEM, lowSequenceName, DT_CHAR,
            FIELD_NAME_LENG, &fieldvalue);
    if (ret != DB_SUCCESS)
        goto end;
    dbi_KeyValue(1, &fieldvalue, &maxkey);

    cursorId =
            Open_Cursor(*pTransid, "syssequences", LK_EXCLUSIVE,
            1 /* $idx_syssequences */ , &minkey, &maxkey, NULL);
    if (cursorId < 0)
    {
        ret = cursorId;
        goto end;
    }

    ret = __Read(cursorId, (char *) &sysSequenceRec, NULL, 0, &recSize, 0);

    if (ret < 0)
    {
        if (ret != DB_E_NOMORERECORDS)
        {
            goto end;
        }
        /* ret == DB_E_NOMORERECORDS */
        /* O.K. => new user can be inserted */
    }
    else
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_SEQUENCE_ALREADY_EXIST,
                0);
        ret = DB_E_SEQUENCE_ALREADY_EXIST;
        goto end;
    }

    /* find the NEW SEQUENCE'ID */
    newSequenceId = __get_next_sequenceId();
    if (newSequenceId < 0)
    {
        ret = newSequenceId;
        goto end;
    }

    // SysSequence record에 값을 할당
    sc_memset(&sysSequenceRec, 0x00, sizeof(SYSSEQUENCE_REC_T));
    sysSequenceRec.sequenceId = newSequenceId;
    sc_strcpy(sysSequenceRec.sequenceName, lowSequenceName);
    sysSequenceRec.maxNum = pDbSequence->maxNum;
    sysSequenceRec.minNum = pDbSequence->minNum;
    sysSequenceRec.increaseNum = pDbSequence->increaseNum;
    sysSequenceRec.lastNum = pDbSequence->startNum;
    sysSequenceRec.cycled = pDbSequence->cycled;
    sysSequenceRec._inited = 0;

    /* set Not-readonly-transaction */
    pTrans->fReadOnly = 0;
    pTrans->fLogging = 1;

    ret = __Insert(cursorId, (char *) &sysSequenceRec,
            sizeof(SYSSEQUENCE_REC_T), &recordId, 0);
    if (ret < 0)
    {
        goto end;
    }

    ret = sysSequenceRec.sequenceId;    /* Sequence's ID */

  end:
    if (cursorId > 0)
        Close_Cursor(cursorId);

    if (curr_transid == -1)
    {
        if (ret < 0)
            dbi_Trans_Abort(connid);
        else
            dbi_Trans_Commit(connid);
    }

  end_return:
    if (ret < 0)
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);

    return ret;
}

/*****************************************************************************/
//! dbi_Sequence_Alter
/*! \breif  Sequence에 대한 정의를 수정하는 함수이다.
 ************************************
 * \param connid(in)        : connection id
 * \param pDbSeqence(in)    : Sequence에 대한 정의
 ************************************
 * \return    ret < 0, it's ERROR
 *        ret = DB_SUCCESS, it's SUCCESS
 ************************************
 * \note
 *    - 내부 알고리즘\n
 *    - 유의 사항
 *        1) 자료구조

 *        Systable에서 사용하는 Sequence에 대한 구조체는 SYSSEQUENCE_REC_T 이며
 *        Sequence를 정의하거나, 수정하는 경우에 사용하는 구조체는 DB_SEQUENCE 이다.
 *        2) 변경시 주의사항
 *        Sequence의 이름은 수정할 수 없다.
 *    - COLLATION INTERFACE 추가
 *
 *****************************************************************************/
int
dbi_Sequence_Alter(int connid, DB_SEQUENCE * pDbSequence)
{
    int *pTransid;
    int curr_transid = 0;
    struct Trans *pTrans = NULL;
    DB_INT32 cursorId = -1;
    struct KeyValue minkey, maxkey;
    FIELDVALUE_T fieldvalue;
    int ret;

    SYSSEQUENCE_REC_T sysSequenceRec;
    DB_UINT32 recSize;
    char *lowSequenceName;

    DBI_CHECK_SERVER_TERM();

    lowSequenceName = pDbSequence->sequenceName;

    ret = __check_valid_sequence(pDbSequence);
    if (ret < 0)
    {
        // Error 발생
        return ret;
    }

    pTransid = (int *) CS_getspecific(my_trans_id);
    curr_transid = *pTransid;
    if (curr_transid == -1)
    {
        ret = dbi_Trans_Begin(connid);
        if (ret < 0)
            goto end_return;
    }

    pTrans = (struct Trans *) Trans_Search(*pTransid);
    if (pTrans == NULL)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    ret = LockMgr_Lock("syssequences", *pTransid, LOWPRIO, LK_EXCLUSIVE, WAIT);
    if (ret < 0)
    {
        goto end;
    }

    ret = dbi_FieldValue(MDB_NN, DT_CHAR, 1,
            mdb_offsetof(SYSSEQUENCE_REC_T, sequenceName), FIELD_NAME_LENG, 0,
            MDB_COL_DEFAULT_SYSTEM, lowSequenceName, DT_CHAR,
            FIELD_NAME_LENG, &fieldvalue);
    if (ret != DB_SUCCESS)
        goto end;
    dbi_KeyValue(1, &fieldvalue, &minkey);
    ret = dbi_FieldValue(MDB_NN, DT_CHAR, 1,
            mdb_offsetof(SYSSEQUENCE_REC_T, sequenceName), FIELD_NAME_LENG, 0,
            MDB_COL_DEFAULT_SYSTEM, lowSequenceName, DT_CHAR,
            FIELD_NAME_LENG, &fieldvalue);
    if (ret != DB_SUCCESS)
        goto end;
    dbi_KeyValue(1, &fieldvalue, &maxkey);

    cursorId = Open_Cursor(*pTransid, "syssequences", LK_EXCLUSIVE,
            1 /* $idx_syssequences */ , &minkey, &maxkey, NULL);
    if (cursorId < 0)
    {
        ret = cursorId;
        goto end;
    }

    ret = __Read(cursorId, (char *) &sysSequenceRec, NULL, 0, &recSize, 0);

    if (ret < 0)
    {
        if (ret == DB_E_NOMORERECORDS)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    DB_E_SEQUENCE_INVALID_NAME, 0);
            ret = DB_E_SEQUENCE_ALREADY_EXIST;
        }
        goto end;
    }

    /* set Not-readonly-transaction */
    pTrans->fReadOnly = 0;
    pTrans->fLogging = 1;

    // update the record's contents
    sysSequenceRec.maxNum = pDbSequence->maxNum;
    sysSequenceRec.minNum = pDbSequence->minNum;
    sysSequenceRec.increaseNum = pDbSequence->increaseNum;
    sysSequenceRec.lastNum = pDbSequence->startNum;
    sysSequenceRec.cycled = pDbSequence->cycled;

    ret = __Update(cursorId, (char *) &sysSequenceRec, recSize, NULL);
    if (ret < 0)
    {
        goto end;
    }

    ret = DB_SUCCESS;

  end:
    if (cursorId > 0)
        Close_Cursor(cursorId);

    if (curr_transid == -1)
    {
        if (ret == DB_SUCCESS)
            dbi_Trans_Commit(connid);
        else
            dbi_Trans_Abort(connid);
    }

  end_return:
    if (ret < 0)
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);

    return ret;
}

/*****************************************************************************/
//! dbi_Sequence_Drop
/*! \breif  Sequence를 제거하는 함수이다.
 ************************************
 * \param connid(in)            : connection id
 * \param sequenceName(in)    : 지우려는 Sequence의 이름
 ************************************
 * \return  성공 : SUCCESS( = 0)
 *          실패 : 오류코드 ( < 0 )
 ************************************
 * \note
 *    - 내부 알고리즘\n
 *    - COLLATION INTERFACE 추가
 *****************************************************************************/
int
dbi_Sequence_Drop(int connid, char *sequenceName)
{
    int *pTransid;
    int curr_transid = 0;
    struct Trans *pTrans = NULL;
    DB_INT32 cursorId = -1;
    struct KeyValue minkey, maxkey;
    FIELDVALUE_T fieldvalue;
    int ret;

    DB_UINT32 recSize;
    char *lowSequenceName;

    DBI_CHECK_SERVER_TERM();

    lowSequenceName = sequenceName;

    pTransid = (int *) CS_getspecific(my_trans_id);
    curr_transid = *pTransid;
    if (curr_transid == -1)
    {
        ret = dbi_Trans_Begin(connid);
        if (ret < 0)
            goto end_return;
    }

    pTrans = (struct Trans *) Trans_Search(*pTransid);
    if (pTrans == NULL)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    ret = LockMgr_Lock("syssequences", *pTransid, LOWPRIO, LK_EXCLUSIVE, WAIT);
    if (ret < 0)
    {
        goto end;
    }

    ret = dbi_FieldValue(MDB_NN, DT_CHAR, 1,
            mdb_offsetof(SYSSEQUENCE_REC_T, sequenceName), FIELD_NAME_LENG, 0,
            MDB_COL_DEFAULT_SYSTEM, lowSequenceName, DT_CHAR,
            FIELD_NAME_LENG, &fieldvalue);
    if (ret != DB_SUCCESS)
        goto end;
    dbi_KeyValue(1, &fieldvalue, &minkey);
    ret = dbi_FieldValue(MDB_NN, DT_CHAR, 1,
            mdb_offsetof(SYSSEQUENCE_REC_T, sequenceName), FIELD_NAME_LENG, 0,
            MDB_COL_DEFAULT_SYSTEM, lowSequenceName, DT_CHAR,
            FIELD_NAME_LENG, &fieldvalue);
    if (ret != DB_SUCCESS)
        goto end;
    dbi_KeyValue(1, &fieldvalue, &maxkey);

    cursorId =
            Open_Cursor(*pTransid, "syssequences", LK_EXCLUSIVE,
            1 /* $idx_syssequences */ , &minkey, &maxkey, NULL);
    if (cursorId < 0)
    {
        ret = cursorId;
        goto end;
    }

    ret = __Read(cursorId, NULL, NULL, 0, &recSize, 0);

    if (ret < 0)
    {
        if (ret == DB_E_NOMORERECORDS)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    DB_E_SEQUENCE_INVALID_NAME, 0);
            ret = DB_E_SEQUENCE_INVALID_NAME;
        }
        goto end;
    }

    /* set Not-readonly-transaction */
    pTrans->fReadOnly = 0;
    pTrans->fLogging = 1;

    ret = __Remove(cursorId);
    if (ret < 0)
    {
        if (ret == DB_E_NOMORERECORDS)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_SEQUENCE_NOT_EXIST,
                    0);
            ret = DB_E_SEQUENCE_NOT_EXIST;
        }
        goto end;
    }

    ret = DB_SUCCESS;

  end:
    if (cursorId > 0)
        Close_Cursor(cursorId);

    if (curr_transid == -1)
    {
        if (ret < 0)
            dbi_Trans_Abort(connid);
        else
            dbi_Trans_Commit(connid);
    }

  end_return:
    if (ret < 0)
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);

    return ret;
}

/*****************************************************************************/
//! dbi_Sequence_Read
/*! \breif  특정 Sequence의 내용을 읽는 함수(for alter)
 ************************************
 * \param connid(in)        : connection id
 * \param sequenceName(in)    : Sequence의 이름
 * \param pSequence(out)    : Sequence의 정보
 ************************************
 * \return  성공 : SUCCESS( = 0)
 *          실패 : 오류코드 ( < 0 )
 ************************************
 * \note
 *    - 내부 알고리즘\n
 *    - 참고
 *        dbi_Sequence_Alter()를 위한 함수이다
 *    - COLLATION INTERFACE 추가
 *****************************************************************************/
int
dbi_Sequence_Read(int connid, char *sequenceName, DB_SEQUENCE * pSequence)
{
    int *pTransid;
    int curr_transid = 0;
    struct Trans *pTrans = NULL;
    DB_INT32 cursorId = -1;
    struct KeyValue minkey, maxkey;
    FIELDVALUE_T fieldvalue;
    int ret;

    SYSSEQUENCE_REC_T sysSequenceRec;
    DB_UINT32 recSize;
    char *lowSequenceName;

    DBI_CHECK_SERVER_TERM();

    lowSequenceName = sequenceName;

    pTransid = (int *) CS_getspecific(my_trans_id);
    curr_transid = *pTransid;
    if (curr_transid == -1)
    {
        ret = dbi_Trans_Begin(connid);
        if (ret < 0)
            goto end_return;
    }

    pTrans = (struct Trans *) Trans_Search(*pTransid);
    if (pTrans == NULL)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    ret = LockMgr_Lock("syssequences", *pTransid, LOWPRIO, LK_SHARED, WAIT);
    if (ret < 0)
    {
        goto end;
    }

    ret = dbi_FieldValue(MDB_NN, DT_CHAR, 1,
            mdb_offsetof(SYSSEQUENCE_REC_T, sequenceName), FIELD_NAME_LENG, 0,
            MDB_COL_DEFAULT_SYSTEM, lowSequenceName, DT_CHAR,
            FIELD_NAME_LENG, &fieldvalue);
    if (ret != DB_SUCCESS)
        goto end;
    dbi_KeyValue(1, &fieldvalue, &minkey);
    ret = dbi_FieldValue(MDB_NN, DT_CHAR, 1,
            mdb_offsetof(SYSSEQUENCE_REC_T, sequenceName), FIELD_NAME_LENG, 0,
            MDB_COL_DEFAULT_SYSTEM, lowSequenceName, DT_CHAR,
            FIELD_NAME_LENG, &fieldvalue);
    if (ret != DB_SUCCESS)
        goto end;
    dbi_KeyValue(1, &fieldvalue, &maxkey);

    cursorId =
            Open_Cursor(*pTransid, "syssequences", LK_SHARED,
            1 /* $idx_syssequences */ , &minkey, &maxkey, NULL);
    if (cursorId < 0)
    {
        ret = cursorId;
        goto end;
    }

    ret = __Read(cursorId, (char *) &sysSequenceRec, NULL, 0, &recSize, 0);

    if (ret < 0)
    {
        if (ret == DB_E_NOMORERECORDS)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    DB_E_SEQUENCE_NOT_EXIST, 0);
            ret = DB_E_SEQUENCE_NOT_EXIST;
        }
        goto end;
    }

#ifdef MDB_DEBUG
    if (mdb_strcmp(lowSequenceName, sysSequenceRec.sequenceName))
    {
        sc_assert(0, __FILE__, __LINE__);
    }
#endif


    // 바깥으로 보내려는 값
    sc_memset(pSequence, 0x00, sizeof(DB_SEQUENCE));
    pSequence->startNum = sysSequenceRec.lastNum;
    pSequence->increaseNum = sysSequenceRec.increaseNum;
    pSequence->maxNum = sysSequenceRec.maxNum;
    pSequence->minNum = sysSequenceRec.minNum;
    pSequence->cycled = sysSequenceRec.cycled;

    ret = DB_SUCCESS;

  end:
    if (cursorId > 0)
        Close_Cursor(cursorId);

    if (curr_transid == -1)
    {
        if (ret < 0)
            dbi_Trans_Abort(connid);
        else
            dbi_Trans_Commit(connid);
    }

  end_return:
    if (ret < 0)
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);

    return ret;
}

/*****************************************************************************/
//! dbi_Sequence_Currval
/*! \breif  특정 Sequence의 내부 변수의 현재 값을 가져오는 함수
 ************************************
 * \param connid(in)            : connection id
 * \param sequenceName(in)    : Sequence의 이름
 * \param pCurVal(out)            : Sequence의 내부 변수의 현재 값
 ************************************
 * \return  성공 : SUCCESS( = 0)
 *          실패 : 오류코드 ( < 0 )
 ************************************
 * \note
 *    - 내부 알고리즘\n
 *    - COLLATION INTERFACE 추가
 *
 *****************************************************************************/
int
dbi_Sequence_Currval(int connid, char *sequenceName, int *pCurVal)
{
    int *pTransid;
    int curr_transid = 0;
    struct Trans *pTrans = NULL;
    DB_INT32 cursorId = -1;
    struct KeyValue minkey, maxkey;
    FIELDVALUE_T fieldvalue;
    int ret;

    SYSSEQUENCE_REC_T sysSequenceRec;
    DB_UINT32 recSize;
    char *lowSequenceName;

    DBI_CHECK_SERVER_TERM();

    lowSequenceName = sequenceName;

    pTransid = (int *) CS_getspecific(my_trans_id);
    curr_transid = *pTransid;
    if (curr_transid == -1)
    {
        ret = dbi_Trans_Begin(connid);
        if (ret < 0)
            goto end_return;
    }

    pTrans = (struct Trans *) Trans_Search(*pTransid);
    if (pTrans == NULL)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    ret = LockMgr_Lock("syssequences", *pTransid, LOWPRIO, LK_SHARED, WAIT);
    if (ret < 0)
    {
        goto end;
    }

    ret = dbi_FieldValue(MDB_NN, DT_CHAR, 1,
            mdb_offsetof(SYSSEQUENCE_REC_T, sequenceName), FIELD_NAME_LENG, 0,
            MDB_COL_DEFAULT_SYSTEM, lowSequenceName, DT_CHAR,
            FIELD_NAME_LENG, &fieldvalue);
    if (ret != DB_SUCCESS)
        goto end;
    dbi_KeyValue(1, &fieldvalue, &minkey);
    ret = dbi_FieldValue(MDB_NN, DT_CHAR, 1,
            mdb_offsetof(SYSSEQUENCE_REC_T, sequenceName), FIELD_NAME_LENG, 0,
            MDB_COL_DEFAULT_SYSTEM, lowSequenceName, DT_CHAR,
            FIELD_NAME_LENG, &fieldvalue);
    if (ret != DB_SUCCESS)
        goto end;
    dbi_KeyValue(1, &fieldvalue, &maxkey);

    cursorId =
            Open_Cursor(*pTransid, "syssequences", LK_SHARED,
            1 /* $idx_syssequences */ , &minkey, &maxkey, NULL);
    if (cursorId < 0)
    {
        ret = cursorId;
        goto end;
    }

    ret = __Read(cursorId, (char *) &sysSequenceRec, NULL, 0, &recSize, 0);

    if (ret < 0)
    {
        if (ret == DB_E_NOMORERECORDS)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_SEQUENCE_NOT_EXIST,
                    0);
            ret = DB_E_SEQUENCE_NOT_EXIST;
        }
        goto end;
    }

#ifdef MDB_DEBUG
    if (mdb_strcmp(lowSequenceName, sysSequenceRec.sequenceName))
    {
        sc_assert(0, __FILE__, __LINE__);
    }
#endif

    // NEXTVAL을 호출하기 전까지는 CURRVAL을 호출하지 못함
    if (!sysSequenceRec._inited)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_SEQUENCE_NOT_INIT, 0);
        ret = DB_E_SEQUENCE_NOT_INIT;
        goto end;
    }


    // 현제 값을 가져옴
    *pCurVal = (int) sysSequenceRec.lastNum;

    ret = DB_SUCCESS;

  end:
    if (cursorId > 0)
        Close_Cursor(cursorId);

    if (curr_transid == -1)
    {
        if (ret < 0)
            dbi_Trans_Abort(connid);
        else
            dbi_Trans_Commit(connid);
    }

  end_return:
    if (ret < 0)
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);

    return ret;
}

/*****************************************************************************/
//! dbi_Sequence_Nextval
/*! \breif  특정 Sequence의 내부 변수의 현재 값에서 increase_num 만큼 증가된 값
 ************************************
 * \param connid(in)            : connection id
 * \param sequenceName(in)    : Sequence의 이름
 * \param pCurVal(out)            : Sequence의 내부 변수의 값에서 increase_num 만큼 증가된 값
 ************************************
 * \return  성공 : SUCCESS( = 0)
 *          실패 : 오류코드 ( < 0 )
 ************************************
 * \note
 *    - 내부 알고리즘\n
 *        특정 Sequence의 내부 변수의 현재 값에서 increase_num 만큼 증가 시키고,
 *        그 값을 pNextVal에 넣어둔다.
 *    - COLLATION INTERFACE 추가
 *****************************************************************************/
int
dbi_Sequence_Nextval(int connid, char *sequenceName, int *pNextVal)
{
    int *pTransid;
    int curr_transid = 0;
    struct Trans *pTrans = NULL;
    DB_INT32 cursorId = -1;
    struct KeyValue minkey, maxkey;
    FIELDVALUE_T fieldvalue;
    int ret;

    SYSSEQUENCE_REC_T sysSequenceRec;
    DB_UINT32 recSize;
    char *lowSequenceName;
    DB_INT32 sequenceNextVal = -1;
    struct UpdateValue upd_values;
    struct UpdateFieldValue update_field_value;
    MDB_BOOL is_inited = MDB_FALSE;

    DBI_CHECK_SERVER_TERM();

    lowSequenceName = sequenceName;

    pTransid = (int *) CS_getspecific(my_trans_id);
    curr_transid = *pTransid;
    if (curr_transid == -1)
    {
        ret = dbi_Trans_Begin(connid);
        if (ret < 0)
            goto end_return;
    }

    pTrans = (struct Trans *) Trans_Search(*pTransid);
    if (pTrans == NULL)
    {
        ret = DB_E_NOTTRANSBEGIN;
        goto end;
    }

    ret = LockMgr_Lock("syssequences", *pTransid, LOWPRIO, LK_EXCLUSIVE, WAIT);
    if (ret < 0)
    {
        goto end;
    }

    ret = dbi_FieldValue(MDB_NN, DT_CHAR, 1,
            mdb_offsetof(SYSSEQUENCE_REC_T, sequenceName), FIELD_NAME_LENG, 0,
            MDB_COL_DEFAULT_SYSTEM, lowSequenceName, DT_CHAR,
            FIELD_NAME_LENG, &fieldvalue);
    if (ret != DB_SUCCESS)
        goto end;
    dbi_KeyValue(1, &fieldvalue, &minkey);
    ret = dbi_FieldValue(MDB_NN, DT_CHAR, 1,
            mdb_offsetof(SYSSEQUENCE_REC_T, sequenceName), FIELD_NAME_LENG, 0,
            MDB_COL_DEFAULT_SYSTEM, lowSequenceName, DT_CHAR,
            FIELD_NAME_LENG, &fieldvalue);
    if (ret != DB_SUCCESS)
        goto end;
    dbi_KeyValue(1, &fieldvalue, &maxkey);

    cursorId =
            Open_Cursor(*pTransid, "syssequences", LK_EXCLUSIVE,
            1 /* $idx_syssequences */ , &minkey, &maxkey, NULL);
    if (cursorId < 0)
    {
        ret = cursorId;
        goto end;
    }

    ret = __Read(cursorId, (char *) &sysSequenceRec, NULL, 0, &recSize, 0);

    if (ret < 0)
    {
        if (ret == DB_E_NOMORERECORDS)
        {
            /* index scan으로 sequence 찾기가 실패한 경우
             * serial scan으로 다시 시도함 */
            struct Filter filter;

            Close_Cursor(cursorId);

            ret = dbi_FieldValue(MDB_EQ, DT_CHAR, 1,
                    mdb_offsetof(SYSSEQUENCE_REC_T, sequenceName),
                    FIELD_NAME_LENG, 0, MDB_COL_DEFAULT_SYSTEM,
                    lowSequenceName, DT_CHAR, FIELD_NAME_LENG, &fieldvalue);
            if (ret != DB_SUCCESS)
                goto end;

            filter = dbi_Filter(1, &fieldvalue);
            cursorId = Open_Cursor(*pTransid, "syssequences", LK_EXCLUSIVE,
                    -1, NULL, NULL, &filter);
            if (cursorId < 0)
            {
                ret = cursorId;
                goto end;
            }
            ret = __Read(cursorId, (char *) &sysSequenceRec, NULL, 0,
                    &recSize, 0);
            if (ret < 0)
            {
                if (ret == DB_E_NOMORERECORDS)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            DB_E_SEQUENCE_NOT_EXIST, 0);
                    ret = DB_E_SEQUENCE_NOT_EXIST;
                    MDB_SYSLOG(("Error: Sequence(%s) does not exists\n",
                                    lowSequenceName));
                }
            }
            else
            {
                goto ok_continue;
            }
        }
        goto end;
    }

#ifdef MDB_DEBUG
    if (mdb_strcmp(lowSequenceName, sysSequenceRec.sequenceName))
    {
        sc_assert(0, __FILE__, __LINE__);
    }
#endif

  ok_continue:

    /* set Not-readonly-transaction */
    pTrans->fReadOnly = 0;
    pTrans->fLogging = 1;

    // SEQUENCE가 생성된 후, 첫번째 NEXTVAL이 호출되는 경우 STARTNUM가 리턴된다.
    if (!sysSequenceRec._inited)
    {
        sequenceNextVal = sysSequenceRec.lastNum;
        // 초기화
        sysSequenceRec._inited = 1;
        is_inited = MDB_TRUE;
    }
    else
    {
        sequenceNextVal = sysSequenceRec.lastNum + sysSequenceRec.increaseNum;
        if (sysSequenceRec.increaseNum > 0)
        {
            if (sysSequenceRec.lastNum > 0 && sequenceNextVal < 0)
            {
                if (sequenceNextVal < sysSequenceRec.minNum)
                {
                    if (sysSequenceRec.cycled)
                    {
                        sequenceNextVal = sysSequenceRec.minNum +
                                (MDB_INT_MAX - sysSequenceRec.maxNum) +
                                (sequenceNextVal - MDB_INT_MIN);
                    }
                    else
                    {
                        ret = DB_E_SEQUENCE_EXCESS_LIMIT;
                        goto end;
                    }
                }
            }
            else
            {
                if (sequenceNextVal > sysSequenceRec.maxNum)
                {
                    if (sysSequenceRec.cycled)
                    {
                        sequenceNextVal =
                                (sequenceNextVal - 1) - sysSequenceRec.maxNum +
                                sysSequenceRec.minNum;
                    }
                    else
                    {
                        ret = DB_E_SEQUENCE_EXCESS_LIMIT;
                        goto end;
                    }
                }
            }
        }
        else
        {
            if (sysSequenceRec.lastNum < 0 && sequenceNextVal > 0)
            {
                if (sequenceNextVal > sysSequenceRec.maxNum)
                {
                    if (sysSequenceRec.cycled)
                    {
                        sequenceNextVal = sysSequenceRec.maxNum -
                                (sysSequenceRec.minNum - MDB_INT_MIN) -
                                (MDB_INT_MAX - sequenceNextVal);
                    }
                    else
                    {
                        ret = DB_E_SEQUENCE_EXCESS_LIMIT;
                        goto end;
                    }
                }
            }
            else
            {
                if (sequenceNextVal < sysSequenceRec.minNum)
                {
                    if (sysSequenceRec.cycled)
                    {
                        sequenceNextVal =
                                sequenceNextVal + sysSequenceRec.maxNum -
                                sysSequenceRec.minNum + 1;
                    }
                    else
                    {
                        ret = DB_E_SEQUENCE_EXCESS_LIMIT;
                        goto end;
                    }
                }
            }
        }
    }

    // 다음 값으로 할당
    *pNextVal = sysSequenceRec.lastNum = sequenceNextVal;

    // update the record's contents
    upd_values.update_field_count = 1;
    upd_values.update_field_value = &update_field_value;

    update_field_value.isnull = 0;
    update_field_value.Blength = 0;
    update_field_value.data = &(update_field_value.space[0]);

    if (is_inited)
    {
        update_field_value.pos = 7;
        *(char *) update_field_value.data = (char) sysSequenceRec._inited;
    }
    else
    {
        update_field_value.pos = 5;
        *(int *) update_field_value.data = sysSequenceRec.lastNum;
    }

    ret = Update_Field(cursorId, &upd_values);
    if (ret < 0)
    {
        goto end;
    }

    ret = DB_SUCCESS;

  end:
    if (cursorId > 0)
        Close_Cursor(cursorId);

    if (curr_transid == -1)
    {
        if (ret < 0)
            dbi_Trans_Abort(connid);
        else
            dbi_Trans_Commit(connid);
    }

  end_return:
    if (ret < 0)
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);

    return ret;
}
