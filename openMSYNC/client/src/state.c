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
#ifdef WIN32
#include <Winsock.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>
#include <time.h>
#include <wchar.h>
#endif

#include "state.h"

extern void ClientLog(char *format, ...);

int syncErrNo = 0;


struct S_STATEFUNC g_stateFunc[] = {
    /* NONE_STATE = -1,          */
    /* WRITE_MESSAGE_BODY_STATE, */ {WriteMessageBody, "WriteMessageBody"},
    /* WRITE_FLAG_STATE,         */ {WriteFlag, "WriteFlag"},
    /* RECEIVE_FLAG_STATE,       */ {ReceiveFlag, "ReceiveFlag"},
    /* READ_MESSAGE_BODY_STATE,  */ {ReadMessageBody, "ReadMessageBody"},
    /* END_OF_RECORD_STATE=99,   */
    /* END_OF_STATE = 100, */
    /* NO_SCRIPT_STATE = -2      */
};

typedef enum
{
    SEND_ERROR = 1,
    RECEIVE_ERROR,
    TIMEOUT,
    CUT_OFF
} SYNC_ERROR;

char *errorMsg[] = {
    "No Error msg",
    "Send Error",
    "Receive Error",
    "Timeout",
    "Message is cut off"
};


/*****************************************************************************
 WriteToNet 
 - return : 0, -1
*****************************************************************************/
int
WriteToNet(int fd, char *buf, size_t byte)
{
#ifdef WIN32
    if (send(fd, (char *) buf, byte, 0) == SOCKET_ERROR)
    {
        ClientLog("Network Communication Error[Send]:%d\n", WSAGetLastError());
        return -1;
    }
#else
    if (send(fd, (char *) buf, byte, 0) < 0)
    {
        ClientLog("Network Communication Error[Send]:%d\n", errno);
        return -1;
    }
#endif

    return (0);
}



/*****************************************************************************
 ReadFrNet
 - return : 0, -1, 읽은 length
*****************************************************************************/
int
ReadFrNet(int sock, char *buf, size_t msgsz, int timeout)
{
    int retval;
    int nleft, nread;
    char *ptr = (char *) buf;
    struct timeval tv;
    fd_set rfds;


    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);

    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    retval = select(sock + 1, &rfds, NULL, NULL, &tv);
    nleft = msgsz;
    *buf = '\0';

    if (retval > 0)
    {
        while (nleft > 0)
        {
            nread = recv(sock, ptr, nleft, 0);
#ifdef WIN32
            if (nread == SOCKET_ERROR || nread < 0)
#else
            if (nread < 0)
#endif
            {
                nleft = msgsz + 1;
                syncErrNo = RECEIVE_ERROR;
                goto ReadFrNet_END;     // return -1
            }
            else if (nread == 0)
            {
                nleft = msgsz + 1;
                syncErrNo = RECEIVE_ERROR;
                goto ReadFrNet_END;     // return -1
            }
            nleft -= nread;
            ptr += nread;
        }

        if (nleft != 0)
            syncErrNo = CUT_OFF;
    }
    else if (retval < 0)
    {
        nleft = msgsz + 1;
        syncErrNo = RECEIVE_ERROR;
        goto ReadFrNet_END;     // return -1
    }
    else
    {
        syncErrNo = TIMEOUT;
        goto ReadFrNet_END;     // return 0
    }

  ReadFrNet_END:
    FD_CLR((u_int) sock, &rfds);

    return (msgsz - nleft);     // 정상인 경우 nleft=0니까 msgsz
}




/*****************************************************************************
 WriteFlag
*****************************************************************************/
int
WriteFlag(PARAMETER * pParam)
{
    char a[PACKET_OP_CODE_SIZE];
    int ret;

    syncErrNo = 0;
    sprintf(a, "%c", pParam->flag);
    ret = WriteToNet(pParam->socketFd, (char *) a, PACKET_OP_CODE_SIZE);
    if (ret < 0)
    {
        syncErrNo = SEND_ERROR;
        return (-1);
    }

    if (pParam->flag == ACK_FLAG || pParam->flag == UPGRADE_FLAG)
        return (pParam->nextState);

    return (WRITE_MESSAGE_BODY_STATE);
}

/*****************************************************************************
 WriteMessageBody
*****************************************************************************/
int
WriteMessageBody(PARAMETER * pParam)
{
    int ret;
    unsigned int tmpSize;

    syncErrNo = 0;
    if (pParam->sendDataLength >
            (sizeof(pParam->sendBuffer) - PACKET_BODY_LENGTH_SIZE))
    {
        ClientLog("Network Communication Error: Too Large Message\n");
        return (-1);
    }

    tmpSize = htonl((unsigned int) pParam->sendDataLength);
    memcpy(pParam->sendBuffer, (char *) &tmpSize, PACKET_BODY_LENGTH_SIZE);

    ret = WriteToNet(pParam->socketFd,
            (char *) pParam->sendBuffer,
            pParam->sendDataLength + PACKET_BODY_LENGTH_SIZE);
    if (ret < 0)
    {
        syncErrNo = SEND_ERROR;
        return (-1);
    }
    return (pParam->nextState);
}

/*****************************************************************************
 ReceiveFlag
*****************************************************************************/
int
ReceiveFlag(PARAMETER * pParam)
{
    int n;
    char buf[256];
    int cnt = 0;

  reF:
    syncErrNo = 0;
    memset(buf, 0x00, sizeof(buf));
    n = ReadFrNet(pParam->socketFd,
            (char *) buf, PACKET_OP_CODE_SIZE, pParam->timeout);
    if (n <= 0)
    {
        return (-1);
    }


    if (buf[0] == ACK_FLAG || buf[0] == NACK_FLAG || buf[0] == XACK_FLAG
            || buf[0] == UPGRADE_FLAG || buf[0] == OK_FLAG)
    {
        pParam->flag = buf[0];
        return (END_OF_STATE);
    }
    else if (buf[0] == DOWNLOAD_FLAG || buf[0] == DOWNLOAD_DELETE_FLAG)
        return (READ_MESSAGE_BODY_STATE);
    else if (buf[0] == END_FLAG)
        return (END_OF_RECORD_STATE);
    else if (buf[0] == END_OF_TRANSACTION)
        return (END_OF_STATE);
    else if (buf[0] == NO_SCRIPT_FLAG)
        return (NO_SCRIPT_STATE);
    else
    {
        ClientLog("Network Communication Error: Invalid Flag : %c\n", buf[0]);
        if (cnt < 5)
        {
            cnt++;
            goto reF;
        }
        return (-1);
    }
}

/*****************************************************************************
 ReadMessageBody
*****************************************************************************/
int
ReadMessageBody(PARAMETER * pParam)
{
    int n;
    unsigned int hint;

    n = ReadFrNet(pParam->socketFd,
            (char *) &hint, PACKET_BODY_LENGTH_SIZE, pParam->timeout);
    if (n <= 0)
    {
        syncErrNo = RECEIVE_ERROR;
        return (-1);
    }

    pParam->receiveDataLength = (int) ntohl(hint);
    memset(pParam->receiveBuffer, '\0', HUGE_STRING_LENGTH + 1);

    n = ReadFrNet(pParam->socketFd,
            pParam->receiveBuffer, pParam->receiveDataLength, pParam->timeout);

    if (n < 0)
    {
        syncErrNo = RECEIVE_ERROR;
        return (-1);
    }

    pParam->receiveBuffer[pParam->receiveDataLength] = '\0';
    return (END_OF_STATE);
}
