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

#ifndef STATE_H
#define STATE_H

#include "net.h"
#include "ODBC.h"

typedef int (*MyStateFunc )(NETWORK_PARAMETER *, ODBC_PARAMETER *);
struct S_STATEFUNC {
    MyStateFunc pFunc;
    char* pName;
};
extern struct S_STATEFUNC g_stateFunc[];

typedef enum {
	NONE_STATE = -1,
    READ_MESSAGE_BODY_STATE, 	
    AUTHENTICATE_USER_STATE,
    WRITE_FLAG_STATE,			
    RECEIVE_FLAG_STATE,			
    SET_APPLICATION_STATE,
	SET_TABLENAME_STATE,		
    PROCESS_SCRIPT_STATE,
    UPLOAD_OK_STATE, 	        
    DOWNLOAD_PROCESSING_STATE,
    MAKE_DOWNLOAD_MESSAGE_STATE,
	WRITE_MESSAGE_BODY_STATE,	
    END_OF_RECORD_STATE = 99,	
    END_OF_STATE = 100
} MAIN_STATE;

int ReadMessageBody(NETWORK_PARAMETER *, ODBC_PARAMETER *) ;
int AuthenticateUser(NETWORK_PARAMETER *, ODBC_PARAMETER *) ;
int	WriteFlag(NETWORK_PARAMETER *, ODBC_PARAMETER *) ;
int ReceiveFlag(NETWORK_PARAMETER *, ODBC_PARAMETER *) ;
int	SetApplication(NETWORK_PARAMETER *, ODBC_PARAMETER *) ;
int	SetTableName(NETWORK_PARAMETER *, ODBC_PARAMETER *) ;
int	ProcessScript(NETWORK_PARAMETER *, ODBC_PARAMETER *) ;
int UploadOK(NETWORK_PARAMETER *, ODBC_PARAMETER *);
int	DownloadProcessingMain(NETWORK_PARAMETER *, ODBC_PARAMETER *) ;
int MakeDownloadMessage(NETWORK_PARAMETER *, ODBC_PARAMETER *);
int WriteMessageBody(NETWORK_PARAMETER *, ODBC_PARAMETER *) ;

#endif