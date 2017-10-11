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

#if !defined(MSYNCCLIENT_EXPORTS)
#include "protocol.h"
#endif

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
    __DECL_PREFIX_MSYNC int mSync_FileDownload(int syncFlag);
    __DECL_PREFIX_MSYNC int mSync_ApplicationUpgrade();
    __DECL_PREFIX_MSYNC int mSync_GetFileList(char *svrPath, char *FileList);
    __DECL_PREFIX_MSYNC int mSync_GetFile(char *svrFileName,
            char *cliFileName);
    __DECL_PREFIX_MSYNC int mSync_Disconnect(int MODE);
    __DECL_PREFIX_MSYNC void mSync_ClientLog(char *format, ...);
    __DECL_PREFIX_MSYNC char *mSync_GetSyncError();


#ifdef __cplusplus
}
#endif
#endif
