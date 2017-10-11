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

#include <jni.h>
#include "isql.h"
#include "ConvertString.h"
static jclass class_String;
static jmethodID mid_getBytes;
static jmethodID mid_newString;

char *
jbyteArray2cstr(JNIEnv * env, jbyteArray javaBytes)
{
    size_t len = (*env)->GetArrayLength(env, javaBytes);
    jbyte *nativeBytes = (*env)->GetByteArrayElements(env, javaBytes, 0);
    char *nativeStr = (char *) sc_malloc(len + 2);

    sc_strncpy(nativeStr, (char *) nativeBytes, len);
    nativeStr[len] = '\0';
    nativeStr[len + 1] = '\0';
    (*env)->ReleaseByteArrayElements(env, javaBytes, nativeBytes, JNI_ABORT);
    return nativeStr;
}

jbyteArray
javaGetBytes(JNIEnv * env, jstring str)
{
    if (mid_getBytes == 0)
    {
        if (class_String == 0)
        {
            jclass cls = (*env)->FindClass(env, "java/lang/String");

            if (cls == 0)
                return 0;
            class_String = (*env)->NewGlobalRef(env, cls);
            if (class_String == 0)
                return 0;
        }
        mid_getBytes =
                (*env)->GetMethodID(env, class_String, "getBytes", "()[B");
        if (mid_getBytes == 0)
            return 0;
    }
    return (*env)->CallObjectMethod(env, str, mid_getBytes);
}

jstring
javaNewString(JNIEnv * env, jbyteArray javaBytes)
{
    if (mid_newString == 0)
    {
        if (class_String == 0)
        {
            jclass cls = (*env)->FindClass(env, "java/lang/String");

            if (cls == 0)
                return 0;
            class_String = (*env)->NewGlobalRef(env, cls);
            if (class_String == 0)
                return 0;
        }
        mid_newString =
                (*env)->GetMethodID(env, class_String, "<init>", "([B)V");
        if (mid_newString == 0)
            return 0;
    }
    return (*env)->NewObject(env, class_String, mid_newString, javaBytes);
}

jbyteArray
cstr2jbyteArray(JNIEnv * env, const char *nativeStr)
{
    jbyteArray javaBytes;
    int len = sc_strlen(nativeStr);

    javaBytes = (*env)->NewByteArray(env, len);
    (*env)->SetByteArrayRegion(env, javaBytes, 0, len, (jbyte *) nativeStr);
    return javaBytes;
}
