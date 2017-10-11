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

#ifndef __MSYNC_CLIENT_H_
#define __MSYNC_CLIENT_H_

#if defined(MSYNCCLIENT_EXPORTS)
#define __DECL_PREFIX_MSYNC     __declspec(dllexport)
#elif defined(MSYNC_CLIENT_IMPORTS)
#define __DECL_PREFIX_MSYNC     __declspec(dllimport)
#elif defined(linux)
#define __DECL_PREFIX_MSYNC     __attribute__((visibility("default")))
#else
#define __DECL_PREFIX_MSYNC
#endif

//#if !defined(MSYNCCLIENT_EXPORTS)
//#   include "protocol.h"
//#endif

#ifdef __cplusplus
extern "C"
{
#endif
    __DECL_PREFIX_MSYNC void mSync_InitializeClientSession(char *fileName,
            int timeout);
    __DECL_PREFIX_MSYNC int mSync_ConnectToSyncServer(char *serverAddr,
            int sport);
    __DECL_PREFIX_MSYNC int mSync_AuthRequest(char *uid, char *passwd,
            char *application, int version);
    __DECL_PREFIX_MSYNC int mSync_SendDSN(char *dsn);
    __DECL_PREFIX_MSYNC int mSync_SendTableName(int syncFlag, char *tableName);
    __DECL_PREFIX_MSYNC int mSync_SendUploadData(char flag, char *Data);
    __DECL_PREFIX_MSYNC int mSync_UploadOK(char *parameter);
    __DECL_PREFIX_MSYNC int mSync_ReceiveDownloadData(char *Data);
    __DECL_PREFIX_MSYNC int mSync_Disconnect(int MODE);
    __DECL_PREFIX_MSYNC void mSync_ClientLog(char *format, ...);
    __DECL_PREFIX_MSYNC char *mSync_GetSyncError();

#ifdef __cplusplus
}
#endif
#endif
