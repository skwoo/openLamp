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


#include	<stdio.h>
#include	<stdlib.h>		
#include	<Winsock2.h>
#include	<process.h>

#include	"Sync.h"
#include	"state.h"

#include	"../../version.h"

#define APP_EXIT 0
typedef struct _Sync_Param{
    int sockfd;
    int count;
    char clientAddr[16];
}Sync_Param;

Sync_Param *param;

SQLCHAR		*dbuid, *dbdsn, *dbpasswd;

int dbType;
#ifdef SERVICE_CONSOLE
HANDLE g_hEvent;
#endif

static int serverSocketFd;
extern int	REMAIN_DAYS ;

void ProcessMain(HWND hWnd);


/*****************************************************************************/
//! MemoryAllocation
/*****************************************************************************/
/*! \breif  sendBuffer, receiveBuffer, recordBuffer에 메모리를 할당할 때 사용
 ************************************
 * \param	order(in)	:
 * \param	**ptr(out)	: Pointer to the Memory that allocated
 * \param	length(in)	: not used
 ************************************
 * \return	void
 ************************************
 * \note	SInitializeParameter() 함수 안에서 sendBuffer, receiveBuffer, \n
 *			recordBuffer에 메모리를 할당할 때 사용되는 함수
 *****************************************************************************/
int MemoryAllocation(int order, char **ptr, int length)
{
	*ptr = (char *)calloc(length, sizeof(char));	
	if (*ptr ==NULL)
    {
		ErrLog("SYSTEM [%d];Memory Allocation Error : size = %dbytes;\n", 
               order, length);
		return -1;
	}
	return 0;
}

/*****************************************************************************/
//! InitializeParameter
/*****************************************************************************/
/*! \breif  파라미터 초기화
 ************************************
 * \param	pNetParam(out)	:
 * \param	pOdbcParam(out)	:
 * \param	sockfd(out)		:
 * \param	order(out)		:
 ************************************
 * \return	void
 ************************************
 * \note	Network parameter와 ODBC parameter를 초기화하는 함수로 \n
 *			client로부터 connection이 맺어져 thread가 할당된 뒤인 \n
 *			ProcessThread()에서 호출된다.
 *****************************************************************************/
int InitializeParameter(NETWORK_PARAMETER *pNetParam,
                        ODBC_PARAMETER *pOdbcParam, int sockfd, int order)
{
	memset(pOdbcParam, 0x00, sizeof(ODBC_PARAMETER));	
    pNetParam->socketFd = sockfd;
    pNetParam->sendDataLength = 0 ;
    pNetParam->receiveDataLength = 0;
    pNetParam->sendBufferSize = HUGE_STRING_LENGTH;
    pNetParam->receiveBufferSize = HUGE_STRING_LENGTH;
    pNetParam->nextState = AUTHENTICATE_USER_STATE;
    pNetParam->sendBuffer = NULL;
    pNetParam->receiveBuffer = NULL;
    pNetParam->recordBuffer = NULL;
    
    pOdbcParam->dbdsn = dbdsn;
    pOdbcParam->dbuid = dbuid;
    pOdbcParam->dbpasswd = dbpasswd;
    pOdbcParam->UserDBOpened = FALSE;
    pOdbcParam->Authenticated = FALSE;
    pOdbcParam->connectTime = NULL;
    pOdbcParam->order = order;
    pOdbcParam->mode = 0;	

    pOdbcParam->dbType = dbType;
    pOdbcParam->lastSyncTime = NULL;
    pOdbcParam->hflag4Processing = 0;
    pOdbcParam->hflag4System = 0;
    
    memset(pOdbcParam->uid, 0x00, sizeof(pOdbcParam->uid));	
    if(MemoryAllocation(pOdbcParam->order, 
                        &(pNetParam->sendBuffer), HUGE_STRING_LENGTH+4+1) < 0)
    {
        return (-1);
    }
    
    if(MemoryAllocation(pOdbcParam->order, 
                        &(pNetParam->receiveBuffer), HUGE_STRING_LENGTH+1)<0)
    {
        return (-1);
    }
    
    if(MemoryAllocation(pOdbcParam->order, 
                        &(pNetParam->recordBuffer), HUGE_STRING_LENGTH+1)<0)
    {
        return (-1);
    }
    
    return (0);
}
void FreeMemories(NETWORK_PARAMETER *pNetParam, ODBC_PARAMETER *pOdbcParam)
{
    if(pNetParam->sendBuffer)	
    {
        free(pNetParam->sendBuffer);
        pNetParam->sendBuffer = NULL;
    }
    
    if(pNetParam->receiveBuffer)
    {
        free(pNetParam->receiveBuffer);
        pNetParam->receiveBuffer = NULL;
    }
    
    if(pNetParam->recordBuffer)
    {
        free(pNetParam->recordBuffer);
        pNetParam->recordBuffer = NULL;
    }
}

/*****************************************************************************/
//! ProcessThread
/*****************************************************************************/
/*! \breif  Client로부터 connection이 맺어진 뒤 thread나 child process가	\n
 *			할당되고 매핑되는 함수 \n
 *			sync 과정을 담당.
 ************************************
 * \param	param(in,out)	: network paramter
 ************************************
 * \return	void
 ************************************
 * \note	Client로부터 connection이 맺어진 뒤 thread나 child process가\n
 *			할당되고 매핑되는 함수로 sync 과정을 처리하는 thread의 main \n
 *			함수인 셈이다. 
 *			Client에서 connect 다음에 호출되는 것이 mSync_AuthRequest()이므로\n
 *			ret 변수에는READ_MESSAGE_SIZE로 setting하여 이 단계부터 \n
 *			시작하도록 한다. ReadMessageSize() 함수는  READ_MESSAGE_BODY를 \n
 *			return하므로 다음 단계는 packet 내용을 읽는 ReadMessageBody()를 \n
 *			호출하게 되고 socket으로부터 읽은 정보를 가지고 \n
 *			pNetParam->nextState에 지정된 대로 인증 처리를 하는 \n
 *			AUTHENTICATE_USER를 호출한다. 이처럼 state.cpp에 구현되어 있는 \n
 *			각 state들을 client의 API에 해당하는 순서대로 처리한다.\n
 *			Sync의 모든 단계가 종료되면 DB에 대한 handle들을 해제하고 
 *			할당했던 메모리들도 해제하고 thread를 종료한다.
 *****************************************************************************/
unsigned  __stdcall	ProcessThread(Sync_Param *param)
{	
    NETWORK_PARAMETER	networkParameter;
    ODBC_PARAMETER		odbcParameter;
    int		ret = READ_MESSAGE_BODY_STATE;
    int		state;
        
    // 변수 초기화 및 메모리 할당
    if(InitializeParameter(&networkParameter, 
                           &odbcParameter, param->sockfd, param->count) < 0)
    {
        ret = -1;
        goto SYNC_END;
    }
    
    ErrLog("SYSTEM;%dth client[%s] is Connected;\n", 
           odbcParameter.order, param->clientAddr);
    
    // sync의 각 과정을 처리 : 자세한 내용은 state.cpp에 구현
    // 각 function의 이름은 func[]에 기술    
    while(1) 
    {
        if (ret == END_OF_STATE || ret < 0)	break ;
        ret = g_stateFunc[ret].pFunc(&networkParameter, &odbcParameter);		
    }	
    
    // sync 단계 종료 : ret <0이면 에러이고 END_OF_STATE면 정상 종료
    if (ret < 0)
        state = DISCONNECT_ERROR_STATE;
    else 
        state = DISCONNECT_STATE;
        
#if  defined(COMMIT_AFTER_SYNC_ALL)
    // 전체 동기화 후에 rollback이나 commit을 취한다.
    if(odbcParameter.UserDBOpened)
    {
        if(ret < 0)
        {		
            ret = SQLEndTran(SQL_HANDLE_DBC, 
                             odbcParameter.hdbc4Processing, SQL_ROLLBACK);
            if(ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
            {
                DisplayError(odbcParameter.hstmt4Processing, 
                             odbcParameter.uid, odbcParameter.order, ret); 	
                ret = -1;
            }
        }	
        else 
        {
            ret = SQLEndTran(SQL_HANDLE_DBC, 
                             odbcParameter.hdbc4Processing, SQL_COMMIT);
            if(ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
            {
                DisplayError(odbcParameter.hstmt4Processing, 
                             odbcParameter.uid, odbcParameter.order, ret);
                state = DISCONNECT_ERROR_STATE;
                ret = -1;
            }
        }	
    }
#endif

    // user가 있는 경우에 update
    // firstUserDB가 True인건 invalid user이거나 no user이거나 update에 에러난 경우
    // 인증 받고 openMSYNC_User 테이블에 update가 성공되면 firstUserDB는 TRUE가 된다.
    if(odbcParameter.Authenticated == TRUE ) 
    {
        if(UpdateConnectFlag(&odbcParameter, state) <0) 
            ret = -1;
    }
    // 각 handle을 해제하고 DB와의 연결을 해제한다
    FreeHandles(ALL_HANDLES, &odbcParameter);
    
SYNC_END:
    if(ret <0)
    {
        ErrLog("SYSTEM;%dth client[%s] is Disconnected => Error Occurred;\n", 
               odbcParameter.order, param->clientAddr);
    }
    else
    {
        ErrLog("SYSTEM;%dth client[%s] is Disconnected;\n", 
               odbcParameter.order, param->clientAddr);
    }
    
    closesocket(networkParameter.socketFd);    
    
    FreeMemories(&networkParameter, &odbcParameter);

    return	0;
}


DWORD WINAPI win_ProcessThread(LPVOID param)
{	
	Sync_Param* pSyncParam =  *((Sync_Param **)param);

	param = 0x00;
	ProcessThread(pSyncParam);
	free(pSyncParam);
	
	_endthread();
	return 0;
}

/*****************************************************************************/
//! SyncMain
/*****************************************************************************/
/*! \breif  Server Socket을 initialize하고 bind()시점에 thread 생성 처리
 ************************************
 * \param	svUid(in)		:
 * \param	svDsn(in)		:
 * \param	svPort(in)		:
 * \parram	hWnd(in)		:
 ************************************
 * \return	int
 ************************************
 * \note	Server를 띄우고 auth.ini 파일을 읽어 사용 가능한 버전인지 \n
 *			확인한 뒤 ProcessMain()을 호출한다. \n 
 *****************************************************************************/
/*****************************************************************************
 Server Socket을 initialize하고 client의 connect가 연결되면 
 thread 생성 처리하도록 ProcessMain()을 호출한다. 
 실질적인 부분은 ProcessMain()에 있음.  
*****************************************************************************/
__DECL_PREFIX_MSYNC int SyncMain(S_mSyncSrv_Info *pSrvInfo, HWND hWnd)
{
    int		    SERVER_PORT;
    char		version[VERSION_LENGTH+1];
    
    SERVER_PORT = pSrvInfo->nPortNo;
    
    sprintf(version,
            "%d.%d.%d.%d", MAJOR_VER, MINOR_VER, RELEASE_VER, BUGPATCH_VER);
    
    // socket을 초기화하고 서버로 띄운 뒤
    if ((serverSocketFd = InitServerSocket(SERVER_PORT)) < 0) 
    {
        ErrLog("SYSTEM;Start Up mSync ver %s [port:%d]: FAIL.;\n", 
               version, SERVER_PORT);
        SendMessage(hWnd, WM_COMMAND, APP_EXIT, 0);
        exit (0);
    }

    ErrLog("SYSTEM;Start Up mSync ver %s [port:%d];\n", version, SERVER_PORT);
    ErrLog("SYSTEM;최대 사용자 수는 %d명입니다;\n", pSrvInfo->nMaxUser);
    
    // max User count와 timeout 값을 셋팅한다.
    SetMaxUserCount(pSrvInfo->nMaxUser);
    SetTimeout(pSrvInfo->ntimeout);
    
    dbuid = (SQLCHAR *)pSrvInfo->szUid;
    dbpasswd = (SQLCHAR *)pSrvInfo->szPasswd;
    dbdsn = (SQLCHAR *)pSrvInfo->szDsn;
    
    dbType = pSrvInfo->nDbType;
    
    ProcessMain(hWnd);
    return	0;
}

#ifdef SERVICE_CONSOLE
void SetEventHandle(HANDLE gEvent){
	g_hEvent = gEvent;
}
#endif

/*****************************************************************************
// Sync Process의 Main 함수 : client가 접속되면 thread를 하나 만들어
// ProcessThread()에서 sync 과정을 담당하도록 한다.
*****************************************************************************/
void ProcessMain(HWND hWnd)
{
	char	clientAddr[16];
	int		newsockfd;
	int		ttt=1;	
	int		childPid = -1;

#ifdef SERVICE_CONSOLE
	// 프로그램으로 처리할 때에는 timeout을 둬서 주기적으로 
	// service가 stop /되었는지 그 이벤트를 확인한다.
	DWORD dwWait;
	struct timeval tv;
	fd_set rset;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
#endif

	while(1)
	{		
		// 서비스 프로그램인 경우는 주기적으로 이벤트를 확인하고
		// Unix나 windows용은 client가 들어올 때까지 
        // ProcessNetwork()에서 계속 기다린다. 
#ifdef SERVICE_CONSOLE
		dwWait = WaitForSingleObject(g_hEvent, 0);
		if(WAIT_OBJECT_0 == dwWait)
		{
			break;
		}
	
		FD_ZERO(&rset);
		FD_SET((u_int)serverSocketFd, &rset);
		if(select(serverSocketFd+1, &rset, NULL, NULL, &tv)<=0)
			continue;
#endif

		if ((newsockfd = ProcessNetwork(clientAddr)) < 0)	
			continue;

		HANDLE		hThread;

		param = (Sync_Param *)malloc(sizeof(Sync_Param));
		param->sockfd = newsockfd;
		param->count = ttt++;
		strcpy(param->clientAddr,clientAddr);
				
		hThread = (HANDLE)_beginthread(
            (void(__cdecl *)(void *))win_ProcessThread,
		    0,
			(void*)(&param));
		
		if (hThread == NULL)
		{
			free(param);
			param = NULL;

			ErrLog("SYSTEM;Thread Creation ERROR;\n");
			closesocket(newsockfd);
		}
	}
}
