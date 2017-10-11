/* 
    This file is part of openMSync client library, DB synchronization software.

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

#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#include <Winsock.h>
#else
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#endif

#include "state.h"
#include "../../server/version.h"
#include "../include/mSyncclient.h"


PARAMETER g_Parameter;
extern int syncErrNo;


static char logFileName[256];

void
write_Log(char *format, va_list ap)
{
    static int finit = TRUE;
    FILE *fp;

    if (finit)
    {
        if ((fp = fopen((char *) logFileName, "w")) == (FILE *) NULL)
        {
            return;
        }
        finit = FALSE;
    }
    else
    {
        if ((fp = fopen((char *) logFileName, "a")) == (FILE *) NULL)
        {
            return;
        }
    }

    vfprintf(fp, format, ap);
    fclose(fp);
}

/*****************************************************************************
 mSync_ClientLog : 외부에서 호출되는 함수
*****************************************************************************/
__DECL_PREFIX_MSYNC void
mSync_ClientLog(char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    write_Log(format, ap);
    va_end(ap);
}

void
ClientLog(char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    write_Log(format, ap);
    va_end(ap);
}

/*****************************************************************************
 GetDateTime
*****************************************************************************/
char *
GetDateTime()
{
    static char datetime[100];

#ifndef WIN32
    long tm;

    time((time_t *) & tm);
    memset(datetime, 0x00, sizeof(datetime));
    sprintf(datetime, "%s", ctime((time_t *) & tm));
#else
    char *MON[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    SYSTEMTIME lpSystemTime;
    LPSYSTEMTIME tp = &lpSystemTime;

    GetLocalTime(tp);

    sprintf(datetime, "%3s %02d, %04d at %02d:%02d:%02d", MON[tp->wMonth - 1],
            tp->wDay, tp->wYear, tp->wHour, tp->wMinute, tp->wSecond);
#endif
    return (datetime);
}

char *
GetSyncError()
{
    return errorMsg[syncErrNo];
}

/*****************************************************************************
 mSync_GetSyncError
*****************************************************************************/
__DECL_PREFIX_MSYNC char *
mSync_GetSyncError()
{
    return GetSyncError();
}




/*****************************************************************************
 mSync_InitializeClientSession
*****************************************************************************/
__DECL_PREFIX_MSYNC void
mSync_InitializeClientSession(char *fileName, int timeout)
{
    char version[VERSION_LENGTH + 1];

    sprintf(version,
            "%d.%d.%d.%d", MAJOR_VER, MINOR_VER, RELEASE_VER, BUGPATCH_VER);

    sprintf(logFileName, fileName);
    ClientLog("*** mSync client API ver %s***\n", version);

    ClientLog("Sync Process Started : %s\n", GetDateTime());

    memset(g_Parameter.receiveBuffer, 0x00, HUGE_STRING_LENGTH + 1);
    memset(g_Parameter.sendBuffer, 0x00, HUGE_STRING_LENGTH + 4 + 1);
    g_Parameter.receiveDataLength = 0;
    g_Parameter.sendDataLength = 0;
    g_Parameter.socketFd = -1;
    g_Parameter.timeout = timeout;
    g_Parameter.flag = 0x00;
}

/*****************************************************************************
 mSync_ConnectToSyncServer
 - return : 0, -1
*****************************************************************************/
__DECL_PREFIX_MSYNC int
mSync_ConnectToSyncServer(char *serverAddr, int sport)
{
    PARAMETER *pParam = &g_Parameter;

#ifdef WIN32
    WSADATA wsaData;
    int on = 1;
#endif

    char serverAddress[16];
    struct sockaddr_in serv_addr;

    syncErrNo = 0;

    strcpy((char *) serverAddress, serverAddr);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(serverAddress);
    serv_addr.sin_port = htons(sport);

    if (pParam->socketFd != -1)
#ifdef WIN32
        closesocket(pParam->socketFd);
#else
        close(pParam->socketFd);
#endif

#ifdef WIN32
    if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR)
    {
        WSACleanup();

        goto WSA_ERROR;
    }
#endif


    if ((pParam->socketFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        goto WSA_ERROR;
    }
#ifdef WIN32
    setsockopt(pParam->socketFd,
            IPPROTO_TCP, TCP_NODELAY, (char *) &on, sizeof(on));
#endif

    if (connect(pParam->socketFd,
                    (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        goto WSA_ERROR;
    }

    ClientLog("mSync_ConnectToSyncServer : %s:%d \n", serverAddress, sport);
    return (0);

  WSA_ERROR:
    return -1;

}

/*****************************************************************************
 mSync_AuthRequest
 - input : uid, passwd, application, version
 - return : -1, ACK_FLAG, NACK_FLAG, XACK_FLAG, UPGRADE_FLAG
*****************************************************************************/
__DECL_PREFIX_MSYNC int
mSync_AuthRequest(char *uid, char *passwd, char *application, int version)
{
    int ret = WRITE_MESSAGE_BODY_STATE;
    PARAMETER *pParam = &g_Parameter;

    sprintf(pParam->sendBuffer + 4,
            "%s|%s|%s|%d", uid, passwd, application, version);
    pParam->sendDataLength = strlen(pParam->sendBuffer + 4);

    pParam->nextState = RECEIVE_FLAG_STATE;

    while (1)
    {
        if (ret == END_OF_STATE || ret < 0)
            break;

        ret = g_stateFunc[ret].pFunc(pParam);
    }

    ClientLog("mSync_AuthRequest : ");
    if (ret < 0)
    {
        ClientLog("Network Communication Error %s\n", GetSyncError());
        return (-1);
    }
    else
    {
        if (pParam->flag == ACK_FLAG)
            ClientLog("Success...\n");
        else if (pParam->flag == NACK_FLAG)
            ClientLog("Fail... : There is no '%s' or wrong password\n", uid);
        else if (pParam->flag == XACK_FLAG)
            ClientLog
                    ("Fail... : The Users are registered in excess of MAX USERS\n",
                    uid);
        else if (pParam->flag == UPGRADE_FLAG)
            ClientLog("There is upversion of Application\n");
        else
            ClientLog("ERROR\n");
    }
    return (pParam->flag);
}


/*****************************************************************************
 mSync_SendDSN
 - return : 0, -1
*****************************************************************************/
__DECL_PREFIX_MSYNC int
mSync_SendDSN(char *dsn)
{
    int ret = WRITE_FLAG_STATE;
    PARAMETER *pParam = &g_Parameter;

    pParam->flag = APPLICATION_FLAG;
    sprintf(pParam->sendBuffer + 4, "%d%s", DBSYNC, dsn);
    pParam->sendDataLength = strlen(pParam->sendBuffer + 4);
    pParam->nextState = END_OF_STATE;

    while (1)
    {
        if (ret == END_OF_STATE || ret < 0)
            break;
        ret = g_stateFunc[ret].pFunc(pParam);
    }
    if (ret == END_OF_STATE)
    {
        ClientLog("mSync_SendDSN [%s] : Success...\n", dsn);
        ret = 0;
    }
    else
    {
        ClientLog("mSync_SendDSN [%s] : Fail...%s\n", dsn, GetSyncError());
        ret = -1;
    }

    return (ret);
}

/*****************************************************************************
 mSync_SendTableName
 - return : 0, -1
*****************************************************************************/
__DECL_PREFIX_MSYNC int
mSync_SendTableName(int syncFlag, char *tableName)
{
    int ret = WRITE_FLAG_STATE;
    PARAMETER *pParam = &g_Parameter;

    pParam->flag = TABLE_FLAG;
    sprintf(pParam->sendBuffer + 4, "%d%s", syncFlag, tableName);
    pParam->sendDataLength = strlen(pParam->sendBuffer + 4);
    pParam->nextState = END_OF_STATE;

    while (1)
    {
        if (ret == END_OF_STATE || ret < 0)
            break;

        ret = g_stateFunc[ret].pFunc(pParam);
    }
    if (ret == END_OF_STATE)
    {
        ClientLog("mSync_SendTableName [%s] : Success...\n", tableName);
        ret = 0;
    }
    else
    {
        ClientLog("mSync_SendTableName [%s] : Fail...%s\n",
                tableName, GetSyncError());
        ret = -1;
    }
    return (ret);
}

/*****************************************************************************
 mSync_SendUploadData
 - input : flag, Data
 - return : 0, -1
*****************************************************************************/
__DECL_PREFIX_MSYNC int
mSync_SendUploadData(char flag, char *Data)
{
    PARAMETER *pParam = &g_Parameter;
    int ret = WRITE_FLAG_STATE;

    pParam->flag = flag;
    strcpy(pParam->sendBuffer + 4, Data);

    pParam->sendDataLength = strlen(pParam->sendBuffer + 4);
    pParam->nextState = END_OF_STATE;

    while (1)
    {
        if (ret == END_OF_STATE || ret < 0)
            break;
        ret = g_stateFunc[ret].pFunc(pParam);
    }

    if (ret == END_OF_STATE)
        ret = 0;
    return (ret);
}

/*****************************************************************************
 mSync_UploadOK
 - return : 0, -1
*****************************************************************************/
__DECL_PREFIX_MSYNC int
mSync_UploadOK(char *Data)
{
    PARAMETER *pParam = &g_Parameter;
    int ret = WRITE_FLAG_STATE;

    pParam->flag = END_FLAG;
    strcpy(pParam->sendBuffer + 4, Data);
    pParam->sendDataLength = strlen(pParam->sendBuffer + 4);
    pParam->nextState = RECEIVE_FLAG_STATE;

    if (!pParam->sendDataLength)
        pParam->sendDataLength = sprintf(pParam->sendBuffer + 4, "NO_PARAM");

    while (1)
    {
        if (ret == END_OF_STATE || ret == END_OF_RECORD_STATE || ret < 0)
            break;
        ret = g_stateFunc[ret].pFunc(pParam);
    }
    if (ret == END_OF_STATE)
        ret = 0;

    return (ret);
}

/*****************************************************************************
 mSync_ReceiveDownloadData
 - return : -1, length
*****************************************************************************/
__DECL_PREFIX_MSYNC int
mSync_ReceiveDownloadData(char *Data)
{
    int ret = RECEIVE_FLAG_STATE;
    PARAMETER *pParam = &g_Parameter;

    memset(pParam->receiveBuffer, 0x00, HUGE_STRING_LENGTH + 1);
    while (1)
    {
        if (ret == END_OF_STATE || ret == END_OF_RECORD_STATE || ret < 0)
            break;
        ret = g_stateFunc[ret].pFunc(pParam);
    }

    if (ret == END_OF_STATE)
    {
        memcpy((char *) Data, pParam->receiveBuffer,
                pParam->receiveDataLength + 1);
        ret = pParam->receiveDataLength;
    }
    else if (ret == END_OF_RECORD_STATE)
        ret = 0;
    else if (ret == NO_SCRIPT_STATE)
        ClientLog("There is no Script for Download");

    return (ret);
}

/*****************************************************************************
 mSync_Disconnect
 - return : 0, -1
*****************************************************************************/
__DECL_PREFIX_MSYNC int
mSync_Disconnect(int Mode)
{
    int ret = WRITE_FLAG_STATE;
    PARAMETER *pParam = &g_Parameter;

    pParam->flag = ACK_FLAG;
    pParam->nextState = RECEIVE_FLAG_STATE;

    if (Mode == NORMAL)
    {
        while (1)
        {
            if (ret == END_OF_STATE || ret == END_OF_RECORD_STATE || ret < 0)
                break;

            //ClientLog("state %d: %s\n", ret, g_stateFunc[ret].pName ) ;
            ret = g_stateFunc[ret].pFunc(pParam);
        }
    }
    else
        ret = END_OF_STATE;

#ifdef WIN32
    closesocket(pParam->socketFd);
#else
    close(pParam->socketFd);
#endif
    if (Mode == NORMAL)
        ClientLog("Sync Process Ended Normally.\n");
    else
        ClientLog("Sync Process Ended Abnormally.\n");

    if (ret == END_OF_STATE)
        ret = 0;
    return (ret);
}
