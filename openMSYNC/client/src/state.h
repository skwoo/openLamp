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

#ifndef _MSYNC_STATE_H_
#define _MSYNC_STATE_H_


#include "../../server/protocol.h"

typedef struct _parameter
{
    int socketFd;
    char receiveBuffer[PACKET_BUFFER_SIZE + 1];
    char sendBuffer[PACKET_BUFFER_SIZE + 1];
    int receiveDataLength;
    int sendDataLength;
    int nextState;
    int timeout;
    char flag;
    FILE *fp;
    short isOpened;
} PARAMETER;

typedef enum
{
    NONE_STATE = -1,
    WRITE_MESSAGE_BODY_STATE, WRITE_FLAG_STATE,
    RECEIVE_FLAG_STATE,
    READ_MESSAGE_BODY_STATE,
    END_OF_RECORD_STATE = 99, END_OF_STATE = 100, NO_SCRIPT_STATE = -2
} MAIN_STATE;

typedef int (*MyStateFunc) (PARAMETER *);
struct S_STATEFUNC
{
    MyStateFunc pFunc;
    char *pName;
};
extern struct S_STATEFUNC g_stateFunc[];

extern char *errorMsg[];
extern int syncErrNo;

int WriteMessageBody(PARAMETER *);
int WriteFlag(PARAMETER *);
int ReceiveFlag(PARAMETER *);
int ReadMessageBody(PARAMETER *);


#ifndef WIN32
#define TRUE  (1)
#define FALSE (0)
#endif

#endif // _MSYNC_STATE_H_
