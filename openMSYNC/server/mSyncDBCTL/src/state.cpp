/* 
    This file is part of openMSync Server module, DB synchronization software.

    Copyright (C) 2012 Inervit Co., Ltd.
        support@inervit.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <Windows.h>

#include "Sync.h"
#include "state.h"

struct S_STATEFUNC g_stateFunc[] = {
/* NONE_STATE = -1,         */    
/* READ_MESSAGE_BODY_STATE, */    { ReadMessageBody,  "ReadMessageBody" },	
/* AUTHENTICATE_USER_STATE, */    { AuthenticateUser, "AuthenticateUser" }, 
/* WRITE_FLAG_STATE,	    */    { WriteFlag,	      "WriteFlag" },		
/* RECEIVE_FLAG_STATE,      */    { ReceiveFlag,      "ReceiveFlag" },	
/* SET_APPLICATION_STATE,   */    { SetApplication,   "SetApplication" },	
/* SET_TABLENAME_STATE,	    */    { SetTableName,     "SetTableName" },		
/* PROCESS_SCRIPT_STATE,	*/    { ProcessScript,    "ProcessScript" },		
/* UPLOAD_OK_STATE,         */    { UploadOK,         "UploadOK" },		
/* DOWNLOAD_PROCESSING_STATE, */  { DownloadProcessingMain,"DownloadProcessingMain" },
/* MAKE_DOWNLOAD_MESSAGE_STATE,*/ { MakeDownloadMessage,   "MakeDownloadMessage" },
/* WRITE_MESSAGE_BODY_STATE, */   { WriteMessageBody,   "WriteMessageBody" },	
/* END_OF_RECORD_STATE = 99,  */
/* END_OF_STATE = 100   */
};


/*****************************************************************************/
//! ReadMessageBody
/*****************************************************************************/
/*! \breif  Packet의 size 만큼 socket으로부터 packet 내용을 읽는 함수
 ************************************
 * \param	pNetParam(in,out)	: ReadFrNet에서 사용
 * \param	pOdbcParam(in)		: not used
 ************************************
 * \return	void
 ************************************
 * \note	Packet의 size 만큼 socket으로부터 packet 내용을 읽는 함수
 *			output은 pNetParam->receiveBuffer 이다
 *****************************************************************************/
int ReadMessageBody(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    int n ;
    unsigned int    hint;	
    
    n = ReadFrNet(pNetParam->socketFd, (char *)&hint, sizeof(unsigned int));
    if(n<0) 
    {
        ErrLog("SYSTEM [%d];Network Communication Error [Receive];\n", 
            pOdbcParam->order);
        return(-1);
    }
    else if(n==0) 
    {
        ErrLog("SYSTEM [%d];Network Communication Error [Receive:Timeout];\n",
            pOdbcParam->order);
        return(-1) ;
    }
    else if(n!=sizeof(unsigned int))
    {
        ErrLog("SYSTEM [%d];Network Communication Error [Receive:Message is cut off];\n", 
            pOdbcParam->order);
        return(-1) ;
    }
    
    pNetParam->receiveDataLength  = (int)ntohl(hint) ;
    memset(pNetParam->receiveBuffer, '\0', HUGE_STRING_LENGTH+1);
    
    n = ReadFrNet(pNetParam->socketFd, 
        pNetParam->receiveBuffer, pNetParam->receiveDataLength);  
    if(n<0) 
    {
        ErrLog("SYSTEM [%d];Network Communication Error [Receive];\n", 
            pOdbcParam->order);
        return(-1) ;
    }
    else if(n==0) 
    {
        ErrLog("SYSTEM [%d];Network Communication Error [Receive:Timeout];\n",
            pOdbcParam->order);
        return(-1) ;
    }
    else if(n!=pNetParam->receiveDataLength)
    {
        ErrLog("n : %d, pNetParam->receiveDataLength : %d \n", 
            n, pNetParam->receiveDataLength);
        ErrLog("SYSTEM [%d];Network Communication Error [Receive:Message is cut off];\n", 
            pOdbcParam->order);
        return(-1) ;
    }  
    
    pNetParam->receiveBuffer[pNetParam->receiveDataLength] ='\0';
    return(pNetParam->nextState);
}

/*****************************************************************************/
//! AuthenticateUser
/*****************************************************************************/
/*! \breif  Packet의 size 만큼 socket으로부터 packet 내용을 읽는 함수
 ************************************
 * \param	pNetParam(in)	: receiveBuffer (UserID|PASSWD|application|version)이 input으로 사용
 * \param	pOdbcParam(in)	: DB 연결을 위해서 사용함
 ************************************
 * \return	0 (valid), -1 (invalid)\n
 *			ACK_FLAG, NACK_FLAG, UPGRADE_FLAG (valid), -1 (error)
 ************************************
 * \note	Socket으로부터 읽은 receiveBuffer 안에는 \n
 *			userid|password|application|version 정보가 들어 있다. 이 값을 \n
 *			parsing 한 뒤에 먼저 현재 등록된 사용자 수가MaxUser를 초과하지 \n
 *			않는지 확인하고(CheckCurrentUserCount()) openMSYNC_User table로부터 \n
 *			userid와 password를 확인한 뒤(CheckUserInfo()), application의 \n
 *			version을 체크한다(CheckVersion()).
 *			이와 같은 체크 사항의 결과에 맞게 사용자 인증에 성공하면 \n
 *			ACK_FLAG, 실패하면 NACK_FLAG, 사용자 수가 초과하면 XACK_FLAG, \n
 *			upgrade할 version이 존재하면 UPGRADE_FLAG를 client에 전달하도록\n
 *			WRITE_FLAG를 return하여 단계를 진행한다. 
 *****************************************************************************/
int AuthenticateUser(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    SQLCHAR		passwd[PASSWD_LEN+1], application[APPLICATION_ID_LEN+1];
    int version;
    int ret;
    
    /* ReceiveBuffer에는 UserID|PASSWD|application|version 들어 있음 */	
    /* 1. SystemDB open */
    if(OpenSystemDB(pOdbcParam)<0) 
        return (-1);
   
    /* 2. 전달 받은 Data(receiveBuffer) Parsing */
    if(ParseData(pNetParam->receiveBuffer, 
                (char *)pOdbcParam->uid, (char *)passwd, 
                (char *)application, &version)<0)
    {	// 각 값들을 parsing해서 얻음
        ErrLog("%d_%s;[Authenticate User]Data Format is Wrong;\n", 
               pOdbcParam->order, pOdbcParam->uid);
        return (-1);
    }
   
    pNetParam->nextState = RECEIVE_FLAG_STATE;
    
    /* 3. User count check */
    ret = CheckCurrentUserCount(pOdbcParam);
    if(ret<0)  
        return (-1);
    else if(ret == EXCESS_USER)
    {
        pOdbcParam->event = XACK_FLAG;
        return (WRITE_FLAG_STATE);
    }

    /* 4. User ID/passwd check */
    ret = CheckUserInfo(pOdbcParam, passwd);
    if(ret<0)
    {
        if(ret == INVALID_USER) 
        {
            ErrLog("%d_%s;[Authenticate User]Invalid User : Password is wrong!;\n",
                   pOdbcParam->order, pOdbcParam->uid);
        }
        else if(ret == NO_USER) 
        {
            ErrLog("%d_%s;[Authenticate User]There is no user ID[%s];\n", 
                   pOdbcParam->order, pOdbcParam->uid, pOdbcParam->uid);
        }
        else
        {
            ErrLog("%d_%s;[Authenticate User]System Error;\n", 
                   pOdbcParam->order, pOdbcParam->uid);
            return (-1);
        }
        
        pOdbcParam->event = NACK_FLAG;
        return (WRITE_FLAG_STATE);	// userID로 passwd검증
    }

    /* 5. Version Check & versionID 구하기 */
    ret = CheckVersion(pOdbcParam, application, version);
    if(ret <0)
    {
        pOdbcParam->event = NACK_FLAG;
        return (WRITE_FLAG_STATE);
    }
    else if(ret >0)
    {
        pOdbcParam->event = UPGRADE_FLAG;
        ErrLog("%d_%s;업그레이드를 위한 Application '%s'의 버전 '%d'이 존재합니다.;\n", 
               pOdbcParam->order, pOdbcParam->uid, application, ret);
        return (WRITE_FLAG_STATE);
    }
    /* 6. ConnectionFlag를 CONNET_STATE로 update */
    if(UpdateConnectFlag(pOdbcParam, CONNECT_STATE)<0)
    {
        ErrLog("%d_%s;[Authenticate User]System Error;\n", 
               pOdbcParam->order, pOdbcParam->uid);
        return -1;
    }
    pOdbcParam->Authenticated = TRUE;
    
    ErrLog("%d_%s;[%s:%d] sync start.;\n", 
           pOdbcParam->order, pOdbcParam->uid, application, version);
    pOdbcParam->event = ACK_FLAG;
    return (WRITE_FLAG_STATE); 
}

/*****************************************************************************/
//! WriteFlag
/*****************************************************************************/
/*! \breif  Flags 값을 socket에 써주는 단계이다.
 ************************************
 * \param	pNetParam(in)	:							
 * \param	pOdbcParam(in)	: DB 연결을 위해서 사용함
 ************************************
 * \return	0 (valid), -1 (invalid)
 ************************************
 * \note	Flag 값를 socket에 써주는 함수이다.\n
 *			이때 보낼 packet이 있는 경우 WRITE_MESSAGE_SIZE 단계로 진행하고 \n
 *			packet을 보낼 필요가 없는 경우는 pNetParam->nextState에 지정된 \n
 *			단계로 진행한다. 
 *****************************************************************************/
int WriteFlag(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    int ret;
    char a[2] = {0x00,};
    
    sprintf(a, "%c", pOdbcParam->event);
    
    ret = WriteToNet(pNetParam->socketFd, a, sizeof(a));
    if(ret < 0) 
    {
        ErrLog("%d_%s;Network Communication Error;\n", 
               pOdbcParam->order, pOdbcParam->uid);
        return(-1);
    }
    
    if(a[0]==OK_FLAG)	
        return (END_OF_STATE);    
    if(a[0] == NACK_FLAG || a[0] == ACK_FLAG  || a[0] == XACK_FLAG || 
       a[0] == UPGRADE_FLAG || a[0] == END_OF_TRANSACTION || a[0] == END_FLAG)
    { // MSG를 보낼 필요가 없는 경우 지정된 state로 이동
        return (pNetParam->nextState);
    }
    
    return (WRITE_MESSAGE_BODY_STATE);
}

/*****************************************************************************/
//! ReceiveFlag
/*****************************************************************************/
/*! \breif  Socket으로부터 flag 값을 읽어, 각 FLAG에 따라 적절한 다음 단계로 이동\n
 ************************************
 * \param	pNetParam(in)	:							
 * \param	pOdbcParam(in)	: DB 연결을 위해서 사용함
 ************************************
 * \return	0 (valid), -1 (invalid)
 ************************************
 * \note	Socket으로부터 flag 값을 읽는 함수로 각 flag에 따라 적절히 다음 \n
 *			단계로 진행한다.
 *****************************************************************************/
int ReceiveFlag(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    int     n;
    char    buf[2];
    int     cnt=0;
    
re :
    memset(buf, 0x00, sizeof(buf));
    n = ReadFrNet(pNetParam->socketFd, buf, 2);
    if( n<0 ) 
    {
        ErrLog("%d_%s;Network Communication Error [Receive];\n", 
               pOdbcParam->order, pOdbcParam->uid);
        return(-1) ;
    }
    else if( n==0 ) 
    {
        ErrLog("%d_%s;Network Communication Error [Receive:Timeout];\n", 
               pOdbcParam->order, pOdbcParam->uid);
        return(-1) ;
    }
    else if(n!=2)
    {
        ErrLog("SYSTEM [%d];Network Communication Error [Receive:Message is cut off];\n", 
               pOdbcParam->order);
        return(-1) ;
    }

    if(buf[0] == APPLICATION_FLAG)
    { //'S'
        return (SET_APPLICATION_STATE);
    }
    else if(buf[0] == TABLE_FLAG)
    { // 'M'
        return (SET_TABLENAME_STATE);
    }
    else if(buf[0] == UPDATE_FLAG || 
            buf[0] == INSERT_FLAG || buf[0] == DELETE_FLAG)
    { // 'U', 'I', 'D'
        pOdbcParam->event = buf[0];
        return (PROCESS_SCRIPT_STATE);
    }
    else if( buf[0]== END_FLAG )
    { // 'E'
        return(UPLOAD_OK_STATE);
    }
    else if(buf[0] == ACK_FLAG)
    { // 'A'
        pOdbcParam->event = END_OF_TRANSACTION;
        pNetParam->nextState = END_OF_STATE;
        return (WRITE_FLAG_STATE);
    }
    else 
    {
        ErrLog("%d_%s;Network Communication Error [Invalid Flag:%c];\n", 
               pOdbcParam->order, pOdbcParam->uid, buf[0]);
        if( cnt<5 ) 
        {
            cnt++;
            goto re;
        }
        return(-1) ;
    }		
}

/*****************************************************************************/
//! SetApplication
/*****************************************************************************/
/*! \breif  flag가 APPLICATION_FLAG인 경우 진행되는 단계
 ************************************
 * \param	pNetParam(in)	: network 파라미터							
 * \param	pOdbcParam(in)	: DB 연결을 위해서 사용함
 ************************************
 * \return	0 (valid), -1 (invalid)
 ************************************
 * \note	RECEIVE_FLAG로부터 호출되는 함수로 읽은 flag가 APPLICATION_FLAG인\n
 *			경우 진행되는 단계로 READ_MESSAGE_SIZE부터 packet의 내용을 읽는 \n
 *			READ_MESSAGE_BODY까지 진행하면 pNetParam->nextState가 \n
 *			END_OF_STATE이므로 while loop를 빠져 나온다.\n
 *			ReceiveBuffer에는 DBSync+Dsn 값이 저장되어 있으므로 \n
 *          application Type에 맞춰 userDB를 open하거나 \n
 *			DB connection time을 읽어 오는 일을 해준다. 
 *			DB sync인 경우 다음 단계는 table 이름을 읽기 위해 RECEIVE_FLAG로\n
 *			진행한다.\n
 *****************************************************************************/
int SetApplication(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    int ret = READ_MESSAGE_BODY_STATE;
    SQLCHAR dsn[DSN_LEN + 1];
    SQLCHAR dsnuid[DSN_UID_LEN + 1];
    SQLCHAR dsnpasswd[DSN_PASSWD_LEN + 1];
    char appType[2];
    int  applicationType;
    
    pNetParam->nextState = END_OF_STATE;
    while(1) 
    {
        if(ret == END_OF_STATE || ret<0)
            break ;		
        ret = g_stateFunc[ret].pFunc(pNetParam, pOdbcParam);		
    }	

    if(ret < 0)	
        return -1;
    
    // receiveBuffer에는 
    // DBSYNC인 경우 DBSYNC+DSN    
    sprintf(appType, "%1.1s", &pNetParam->receiveBuffer[0]);
    applicationType = atoi(appType);
    
    if(applicationType == DBSYNC)
    {
        if(pOdbcParam->UserDBOpened)
        { // 현재 열려 있는 디비의 세션을 닫아 준다.
            FreeHandles(USERDB_ONLY, pOdbcParam);  
            pOdbcParam->UserDBOpened = FALSE;
        }

        // 서버에 셋팅되어 있는 DSN 
        if(GetDSN(pOdbcParam, 
                 (char *)pNetParam->receiveBuffer+1, dsn, dsnuid, dsnpasswd)<0)
        {
            return (-1);
        }

        if(OpenUserDB(pOdbcParam, dsn, dsnuid, dsnpasswd)<0) 
        {
            return (-1); 
        }

        pOdbcParam->UserDBOpened = TRUE;
        pOdbcParam->mode = pOdbcParam->mode | DBSYNC;

        if(GetDBConnectTime(pOdbcParam, 
                            &(pOdbcParam->hstmt4Processing), 
                            PROCESSING, &(pOdbcParam->connectTime))<0)
        {
            return (-1);
        }

        ErrLog("%d_%s;[DBSync] DSN : %s start.;\n", 
               pOdbcParam->order, pOdbcParam->uid, dsn);
    }
    
    return (RECEIVE_FLAG_STATE);
}

/*****************************************************************************/
//! SetTableName
/*****************************************************************************/
/*! \breif  flag가 TABLE_FLAG인 경우 진행
 ************************************
 * \param	pNetParam(in)	: network 파라미터							
 * \param	pOdbcParam(in)	: DB 연결을 위해서 사용함
 ************************************
 * \return	0 (valid), -1 (invalid)
 ************************************
 * \note	RECEIVE_FLAG로부터 호출되는 함수\n
 *			이때 읽은 flag가 TABLE_FLAG인 경우 진행되는 단계로 
 *			SetApplication() 함수와 같이 READ_MESSAGE_SIZE, READ_MESSAGE_BODY\n
 *			단계를 거쳐 syncMode와 tableName 값을 얻는다.\n
 *			Upload data stream을 기대하며 RECEIVE_FLAG로 진행한다.
 *****************************************************************************/
int	SetTableName(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    char syncFlag[2];
    int	 ret = READ_MESSAGE_BODY_STATE;
    
    pNetParam->nextState = END_OF_STATE;
    while(1) 
    {
        if(ret == END_OF_STATE || ret<0)
            break ;		
        ret = g_stateFunc[ret].pFunc(pNetParam, pOdbcParam);		
    }	

    if(ret < 0)	
        return -1;
    
    sprintf(syncFlag, "%1.1s", &pNetParam->receiveBuffer[0]);
    pOdbcParam->syncFlag = atoi(syncFlag);
    
    
    // TO DO:
    //    table 별로 ALL, MOD sync 가능하도록 수정 
    // if(pOdbcParam->syncFlag==ALL_SYNC)
    //     strcpy(pOdbcParam->lastSyncTime,"1900-01-01");
    
    strcpy((char *)pOdbcParam->tableName, (char *)pNetParam->receiveBuffer+1);
    
    return (RECEIVE_FLAG_STATE);
}

/*****************************************************************************/
//! ProcessScript
/*****************************************************************************/
/*! \breif  flag가 UPDATE_FLAG, DELETE_FLAG, INSERT_FLAG인 경우 진행\n
 *			Script가 없는 경우 메세지만 로깅하고 받은 데이타는 디비에 \n
 *			반영되지 않으며 다음 단계로 진행
 ************************************
 * \param	pNetParam(in)	: network 파라미터							
 * \param	pOdbcParam(in)	: DB 연결을 위해서 사용함
 ************************************
 * \return	0 (valid), -1 (invalid)
 ************************************
 * \note	RECEIVE_FLAG로부터 호출되는 함수로 읽은 flag가 UPDATE_FLAG, \n
 *			DELETE_FLAG, INSERT_FLAG인 경우 진행되는 단계로 record들의 \n
 *			packet을 socket으로부터 읽어(READ_MESSAGE_SIZE, READ_MESSAGE_BODY)\n
 *			loop를 빠져나온 뒤 GetStatement() 함수로 반영할 문장을 구하고 \n
 *			ProcessStmt() 함수에서 서버 데이터베이스에 각 record들을 적절히 
 *			반영한다. 다음 packet을 기대하며 RECEIVE_FLAG로 진행한다. 
 *****************************************************************************/
int ProcessScript(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    SQLCHAR	Statement[SCRIPT_LEN+1];
    int	ret = READ_MESSAGE_BODY_STATE;
    
    pNetParam->nextState = END_OF_STATE;
    while(1) 
    {
        if(ret == END_OF_STATE || ret<0)
            break ;		
        ret = g_stateFunc[ret].pFunc(pNetParam, pOdbcParam);
    }	

    if(ret < 0)	
        return (-1);
    
    memset(Statement, 0x00, SCRIPT_LEN+1);
    if((ret = GetStatement(pOdbcParam, Statement))<0) 
        return (-1);
    
    if(ret == SQL_NO_DATA)
    { // script가 없는 경우 
        if(pOdbcParam->event==UPDATE_FLAG)
        {
            ErrLog("%d_%s;There is No script for Upload_Update;\n", 
                   pOdbcParam->order, pOdbcParam->uid);
        }
        else if(pOdbcParam->event==DELETE_FLAG)
        {
            ErrLog("%d_%s;There is No script for Upload_Delete;\n",
                   pOdbcParam->order, pOdbcParam->uid);
        }
        else if(pOdbcParam->event==INSERT_FLAG)
        {
            ErrLog("%d_%s;There is No script for Upload_Insert;\n", 
                   pOdbcParam->order, pOdbcParam->uid);	
        }
    }
    else
    {        
        if(ProcessStmt(pNetParam->receiveBuffer, 
                       pNetParam->receiveDataLength, pOdbcParam, Statement)<0)
            return (-1);
    }
    
    return (RECEIVE_FLAG_STATE);
}

/*****************************************************************************/
//! UploadOK
/*****************************************************************************/
/*! \breif  flag가 END_FLAG인 경우\n
 *			upload가 성공한 뒤에는 OK_FLAG 'K'를 보내고 client로부터 \n
 *			OK_FLAG 'K'를 보낸 뒤에 download로 넘어간다.
 ************************************
 * \param	pNetParam(in)	: network 파라미터							
 * \param	pOdbcParam(in)	: DB 연결을 위해서 사용함
 ************************************
 * \return	0 (valid), -1 (invalid)
 ************************************
 * \note	RECEIVE_FLAG에서 읽은 flag가 END_FLAG인 경우 upload가 끝났음을 \n
 *			알 수 있으며 이때 호출되는 함수다. Download script를 실행할 때에\n
 *			필요한 parameter가 있는지 확인하기 위해 socket으로부터 packet을 \n
 *			받는다. \n
 *			Parameter가 없으면 NO_PARAM이라는 메시지를 얻게 된다 \n
 *			(READ_MESSAGE_SIZE, READ_MESSAGE_BODY). \n
 *			성공적으로 이 단계가 끝나면 pNetParam->nextState에 지정된 것처럼\n
 *			WRITE_FLAG(OK_FLAG) 단계로 진행하고 END_OF_STATE로while loop를 \n
 *			빠져 나온다. 각 단계에서 실패 시에는 upload 단계에 시작한 \n
 *			transaction을 rollback으로 처리하고 성공 시에는commit으로 처리한\n
 *			뒤 DOWNLOAD 단계로 진행한다.
 *****************************************************************************/
int UploadOK(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    int	ret = READ_MESSAGE_BODY_STATE;
    
    pNetParam->nextState = WRITE_FLAG_STATE;
    pOdbcParam->event = OK_FLAG;
    while(1) 
    {
        if(ret == END_OF_STATE || ret<0)
            break ;		
        ret = g_stateFunc[ret].pFunc(pNetParam, pOdbcParam);		
    }	

    if(ret < 0)
    {
#if defined(COMMIT_AFTER_SYNC_ALL)
       /* no Action */; // 전체 동기화 후에 rollback이나 commit을 취한다.
#else
        ret = SQLEndTran(SQL_HANDLE_DBC, 
                         pOdbcParam->hdbc4Processing, SQL_ROLLBACK);
        if(ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
        {
            DisplayError(pOdbcParam->hstmt4Processing, 
                         pOdbcParam->uid, pOdbcParam->order, ret); 	
        }
#endif
        return -1;
    }	
    
#if defined(COMMIT_AFTER_SYNC_ALL)
    /* no Action */; // 전체 동기화 후에 rollback이나 commit을 취한다.
#else
    ret = SQLEndTran(SQL_HANDLE_DBC, pOdbcParam->hdbc4Processing, SQL_COMMIT);
    if(ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        DisplayError(pOdbcParam->hstmt4Processing, 
                     pOdbcParam->uid, pOdbcParam->order, ret);
        return (-1);
    }	
#endif
    
    pOdbcParam->event = DOWNLOAD_FLAG;	
    ErrLog("%d_%s;Upload processing [%s] is completed;\n", 
           pOdbcParam->order, pOdbcParam->uid, pOdbcParam->tableName);
    
    return (DOWNLOAD_PROCESSING_STATE);
}

/*****************************************************************************/
//! MakeDownloadMessage
/*****************************************************************************/
/*! \breif  DownloadProcessingMain()에서 호출되는 한 단계 
 ************************************
 * \param	pNetParam(in)	: network 파라미터							
 * \param	pOdbcParam(in)	: DB 연결을 위해서 사용함
 ************************************
 * \return	0 (valid), -1 (invalid)
 ************************************
 * \note	DownloadProcessingMain()에서 호출되는 한 단계로 FetchResult()를\n
 *			통해 sendBuffer에 보낼 packet을 만들어 socket에 download packet을\n
 *			쓸 수 있도록 WRITE_FLAG 단계로 진행한다.
 *****************************************************************************/
int 
MakeDownloadMessage(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    int ret;
    memset(pNetParam->sendBuffer, '\0', HUGE_STRING_LENGTH+4+1);
    ret = FetchResult(pOdbcParam, pNetParam->sendBuffer+4, pNetParam->recordBuffer);
    
    // 에러인 경우 return <0
    // 데이타가 있으면 return 1
    // 데이타가 없는 경우 return 2
    pNetParam->sendDataLength = strlen(pNetParam->sendBuffer+4);
    
    if(ret <0)			
        return (-1);
    else if(ret == 2) 
    {
        pOdbcParam->event = END_FLAG;
        pNetParam->nextState = END_OF_RECORD_STATE;
    }
    else
        pNetParam->nextState = MAKE_DOWNLOAD_MESSAGE_STATE;
    
    return (WRITE_FLAG_STATE);
}

/*****************************************************************************/
//! DownloadProcessingMain
/*****************************************************************************/
/*! \breif  Upload가 성공적으로 끝난 뒤 진행되는 단계
 ************************************
 * \param	pNetParam(in)	: network 파라미터							
 * \param	pOdbcParam(in)	: DB 연결을 위해서 사용함
 ************************************
 * \return	0 (valid), -1 (invalid)
 ************************************
 * \note	Upload가 성공적으로 끝난 뒤 진행되는 단계로 download script를\n
 *			DB로부터 얻어(GetStatement()) cursor를 prepare하고 \n
 *			MAKE_DOWNLOAD_MESSAGE 단계부터 시작하여 socket에 download stream을\n
 *			만들어 써준다(WRITE_FLAG, WRITE_MESSAGE_SIZE, WRITE_MESSAGE_BODY)\n
 *			보낼 데이터가 더 이상 없거나 script가 없는 경우는 END_FLAG를 \n
 *			전달하고 END_OF_RECORD를 return하게 되어 while loop를 빠져 나온다.\n 
 *			성공적으로 download 단계를 마치면 deleted record들을 download하기\n
 *			위해서 pNetParam->event를 DOWNLOAD_DELETE_FLAG로 하여 다시 한번 \n
 *			이 단계를 진행하도록 하는데(DOWNLOAD_PROCESSING) 만약 deleted \n
 *			record를 처리하지 않는 경우라면 client에서 
 *			mSync_ReceiveDownloadData()를 한번만 호출하게 되고 \n
 *			download_delete의 script도 작성되지 않았으므로 바로 RECEIVE_FLAG \n
 *			단계로 진행한다.
 *****************************************************************************/
int 
DownloadProcessingMain(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    int		ret;	
    SQLCHAR	Statement[SCRIPT_LEN+1];
    char	event = pOdbcParam->event;
    
    memset(Statement, 0x00, SCRIPT_LEN+1);
    pOdbcParam->NumCols = 0;
    pOdbcParam->NumParams = 0;
    
    if((ret = GetStatement(pOdbcParam, Statement))<0) 
        return (-1);
    if(ret != SQL_NO_DATA)
    {	// script가 있는 경우
        if(AllocStmtHandle(pOdbcParam, PROCESSING)<0) 
            return (-1);	
        if(PrepareDownload(pNetParam->receiveBuffer, pOdbcParam, 
                           pNetParam->receiveDataLength, Statement) < 0)
        {
            ret = -1;
            goto Download_End;
        }
        
        ret = MAKE_DOWNLOAD_MESSAGE_STATE;
        while(1) 
        {
            if(ret == END_OF_RECORD_STATE || ret<0)
                break ;	
            ret = g_stateFunc[ret].pFunc(pNetParam, pOdbcParam);			
        }	
        
Download_End:
        if(pOdbcParam->NumRows)	
            SQLCloseCursor(pOdbcParam->hstmt4Processing);
        FreeStmtHandle(pOdbcParam, PROCESSING);
        FreeArrays(pOdbcParam);
        
        if(ret < 0) 
            return -1;

        if(event == DOWNLOAD_FLAG)
        {
            pOdbcParam->event = DOWNLOAD_DELETE_FLAG;
            pNetParam->nextState = DOWNLOAD_PROCESSING_STATE;
            if(pOdbcParam->syncFlag == ALL_SYNC)
            {
                ErrLog("%d_%s;Download ALL_SYNC processing [%s] is"
                       " completed => %d rows downloaded;\n", 
                       pOdbcParam->order, pOdbcParam->uid, 
                       pOdbcParam->tableName, pOdbcParam->NumRows);
            }
            else
            {
                ErrLog("%d_%s;Download MOD_SYNC processing [%s] is"
                       " completed => %d rows downloaded;\n", 
                       pOdbcParam->order, pOdbcParam->uid, 
                       pOdbcParam->tableName, pOdbcParam->NumRows);			
            }
        }
        else
        {
            pNetParam->nextState = RECEIVE_FLAG_STATE;
            ErrLog("%d_%s;Download_Delete processing [%s] is"
                   " completed => %d rows downloaded;\n", 
                   pOdbcParam->order, pOdbcParam->uid, 
                   pOdbcParam->tableName, pOdbcParam->NumRows);
        }
    }
    else
    {	// script가 없는 경우
        ret = WRITE_FLAG_STATE;		
        pOdbcParam->event = END_FLAG;
        pNetParam->nextState = END_OF_RECORD_STATE;
        
        ErrLog("%d_%s;There is no Script for Download;\n", pOdbcParam->order, pOdbcParam->uid);		
        while(1) 
        {
            if(ret == END_OF_RECORD_STATE || ret<0)
                break ;	
            ret = g_stateFunc[ret].pFunc(pNetParam, pOdbcParam);
        }
        // DOWNLOAD_FLAG('F')인 경우엔 DOWNLOAD_DELETE_FLAG로 이동하고
        // DOWNLOAD_DELETE_FLAG('R')인 경우엔 바로 RECEIVE_ACK로 이동
        if(ret < 0) 
            return -1;
        if(event == DOWNLOAD_DELETE_FLAG)            
            return (RECEIVE_FLAG_STATE);
        
        pOdbcParam->event = DOWNLOAD_DELETE_FLAG;
        pNetParam->nextState = DOWNLOAD_PROCESSING_STATE;		
    }
    return (pNetParam->nextState);
}

/*****************************************************************************/
//! WriteMessageBody
/*****************************************************************************/
/*! \breif  SendBuffer를 socket에 써주는 함수로 packet의 size + content를 보냄
 ************************************
  * \param	pNetParam(in,out)	: network 파라미터							
 * \param	pOdbcParam(in,out)	: DB 연결을 위해서 사용함
 ************************************
 * \return	0 (valid), -1 (invalid)
 ************************************
 * \note	SendBuffer를 socket에 써주는 함수로 packet의 size + content가 \n
 *			보내진다.
 ****************************************************************************/
int 
WriteMessageBody(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    int ret ;
    unsigned int tmpSize ;
    
    if(pNetParam->sendDataLength > pNetParam->sendBufferSize) 
    {
        ErrLog("%d_%s;Network Communication Error;\n", 
               pOdbcParam->order, pOdbcParam->uid);
        return(-1);
    }
    tmpSize = htonl((unsigned int)pNetParam->sendDataLength) ;
    memcpy(pNetParam->sendBuffer, (char *)&tmpSize, sizeof(unsigned int));
    
    ret = WriteToNet(pNetParam->socketFd, 
                     pNetParam->sendBuffer, pNetParam->sendDataLength+4 );
    if(ret < 0) 
    {
        ErrLog("%d_%s;Network Communication Error;\n", 
               pOdbcParam->order, pOdbcParam->uid);
        return(-1) ;
    }
    
    return(pNetParam->nextState) ;
}

