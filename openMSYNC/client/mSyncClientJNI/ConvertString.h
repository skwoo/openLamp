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


#ifndef _Included_ConvertString
#define _Included_ConvertString
#ifdef __cplusplus
extern "C"
{
#endif
    char *jbyteArray2cstr(JNIEnv * env, jbyteArray javaBytes);
    jbyteArray javaGetBytes(JNIEnv * env, jstring str);

    jstring javaNewString(JNIEnv * env, jbyteArray javaBytes);
    jbyteArray cstr2jbyteArray(JNIEnv * env, const char *nativeStr);
#ifdef __cplusplus
}
#endif
#endif
