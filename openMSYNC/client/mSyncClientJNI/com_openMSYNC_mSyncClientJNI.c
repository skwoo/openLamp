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
#include <stdlib.h>
#include <string.h>
#include "com_openMSYNC_mSyncClientJNI.h"
#include "ConvertString.h"
#include "../include/mSyncclient.h"


/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_InitializeClientSession
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL
Java_com_openMSYNC_mSyncClientJNI_mSync_1InitializeClientSession(JNIEnv * env,
        jclass cls, jstring jfilename, jint jtimeout)
{
    const char *filename = (*env)->GetStringUTFChars(env, jfilename, 0);

    mSync_InitializeClientSession((char *) filename, jtimeout);
    (*env)->ReleaseStringUTFChars(env, jfilename, filename);
}

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_ConnectToSyncServer
 * Signature: (Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL
Java_com_openMSYNC_mSyncClientJNI_mSync_1ConnectToSyncServer(JNIEnv * env,
        jclass cls, jstring jserveraddr, jint jserverport)
{
    int ret;
    const char *serveraddr = (*env)->GetStringUTFChars(env, jserveraddr, 0);

    ret = mSync_ConnectToSyncServer((char *) serveraddr, jserverport);
    (*env)->ReleaseStringUTFChars(env, jserveraddr, serveraddr);

    return ret;
}

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_AuthRequest
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL
Java_com_openMSYNC_mSyncClientJNI_mSync_1AuthRequest(JNIEnv * env, jclass cls,
        jstring jid, jstring jpassword, jstring japplication, jint jversion)
{
    int ret;
    const char *id = (*env)->GetStringUTFChars(env, jid, 0);
    const char *password = (*env)->GetStringUTFChars(env, jpassword, 0);
    const char *application = (*env)->GetStringUTFChars(env, japplication, 0);

    ret = mSync_AuthRequest((char *) id, (char *) password,
            (char *) application, jversion);
    (*env)->ReleaseStringUTFChars(env, jid, id);
    (*env)->ReleaseStringUTFChars(env, jpassword, password);
    (*env)->ReleaseStringUTFChars(env, japplication, application);
    return ret;
}

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_SendDSN
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL
Java_com_openMSYNC_mSyncClientJNI_mSync_1SendDSN(JNIEnv * env, jclass cls,
        jstring jdsn)
{
    int ret;
    const char *dsn = (*env)->GetStringUTFChars(env, jdsn, 0);

    ret = mSync_SendDSN((char *) dsn);
    (*env)->ReleaseStringUTFChars(env, jdsn, dsn);
    return ret;
}

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_SendTableName
 * Signature: (ILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL
Java_com_openMSYNC_mSyncClientJNI_mSync_1SendTableName(JNIEnv * env,
        jclass cls, jint jsyncmode, jstring jtablename)
{
    int ret;
    const char *tablename = (*env)->GetStringUTFChars(env, jtablename, 0);

    ret = mSync_SendTableName(jsyncmode, (char *) tablename);
    (*env)->ReleaseStringUTFChars(env, jtablename, tablename);
    return ret;
}

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_SendUploadData
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL
Java_com_openMSYNC_mSyncClientJNI_mSync_1SendUploadData(JNIEnv * env,
        jclass cls, jstring jflag, jstring jdata)
{
    int ret;
    const char *flag = (*env)->GetStringUTFChars(env, jflag, 0);
    const char *data = (*env)->GetStringUTFChars(env, jdata, 0);

    ret = mSync_SendUploadData((char) flag[0], (char *) data);
    (*env)->ReleaseStringUTFChars(env, jflag, flag);
    (*env)->ReleaseStringUTFChars(env, jdata, data);
    return ret;
}

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_UploadOK
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL
Java_com_openMSYNC_mSyncClientJNI_mSync_1UploadOK(JNIEnv * env, jclass cls,
        jstring jparameter)
{
    int ret;
    const char *parameter = (*env)->GetStringUTFChars(env, jparameter, 0);

    ret = mSync_UploadOK((char *) parameter);
    (*env)->ReleaseStringUTFChars(env, jparameter, parameter);

    return ret;
}

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_ReceiveDownloadData
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_openMSYNC_mSyncClientJNI_mSync_1ReceiveDownloadData(JNIEnv * env,
        jclass cls)
{
    jstring jreceivedata;
    char receivedata[8192];
    int ret;

    memset(receivedata, 0x00, sizeof(receivedata));
    ret = mSync_ReceiveDownloadData(receivedata);
    if (ret <= 0)
        return NULL;

    jreceivedata = (*env)->NewStringUTF(env, receivedata);
    return jreceivedata;
}


/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_Disconnect
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL
Java_com_openMSYNC_mSyncClientJNI_mSync_1Disconnect(JNIEnv * env, jclass cls,
        jint jmode)
{
    return mSync_Disconnect(jmode);
}

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_ClientLog
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_com_openMSYNC_mSyncClientJNI_mSync_1ClientLog(JNIEnv * env, jclass cls,
        jstring jmsg)
{
    const char *msg = (*env)->GetStringUTFChars(env, jmsg, 0);

    mSync_ClientLog((char *) msg);
    (*env)->ReleaseStringUTFChars(env, jmsg, msg);
}

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_GetSyncError
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL
Java_com_openMSYNC_mSyncClientJNI_mSync_1GetSyncError(JNIEnv * env, jclass cls)
{
    jstring jerrormsg;
    char *perrormsg = mSync_GetSyncError();

    jerrormsg = (*env)->NewStringUTF(env, perrormsg);
    return jerrormsg;
}



/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_SendUploadData2
 * Signature: (Ljava/lang/String;[B)I
 */
JNIEXPORT jint JNICALL
        Java_com_openMSYNC_mSyncClientJNI_mSync_1SendUploadData2
        (JNIEnv * env, jclass cls, jstring jflag, jbyteArray jdata)
{
    int ret;
    const char *flag = (*env)->GetStringUTFChars(env, jflag, 0);
    char *data;

    data = jbyteArray2cstr(env, jdata);
    ret = mSync_SendUploadData((char) flag[0], (char *) data);

    (*env)->ReleaseStringUTFChars(env, jflag, flag);
    free(data);

    return ret;
}

/*
 * Class:     com_openMSYNC_mSyncClientJNI
 * Method:    mSync_ReceiveDownloadData2
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL
        Java_com_openMSYNC_mSyncClientJNI_mSync_1ReceiveDownloadData2
        (JNIEnv * env, jclass cls)
{
    char receivedata[8192];
    int ret;
    jbyteArray jnb;

    memset(receivedata, 0x00, sizeof(receivedata));
    ret = mSync_ReceiveDownloadData(receivedata);
    if (ret <= 0)
        return NULL;

    jnb = cstr2jbyteArray(env, receivedata);
    return jnb;
}
