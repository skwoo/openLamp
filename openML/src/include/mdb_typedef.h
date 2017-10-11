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

#ifndef __TYPEDEF_H
#define __TYPEDEF_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include "dbport.h"

// 1. defines for ppthread
#if defined(psos)
    typedef unsigned int WORD;
    typedef unsigned long DWORD;
    typedef void VOID;
#endif

#if defined(sun) || defined(linux)
    typedef unsigned int WORD;
    typedef unsigned long DWORD;
    typedef void VOID;
#endif

#if !defined(WIN32)
    typedef void *HANDLE;
#ifdef USE_DWORD_T
    typedef unsigned long DWORD;
#endif
#endif

#if defined(psos)
    typedef struct _SYSTEMTIME
    {
        WORD wYear;
        WORD wMonth;
        WORD wDayOfWeek;
        WORD wDay;
        WORD wHour;
        WORD wMinute;
        WORD wSecond;
        WORD wMilliseconds;
    } SYSTEMTIME, *LPSYSTEMTIME;

    typedef struct _shmid_ds
    {
        int temp;
    } shmid_ds;
#endif

#ifdef _64BIT
    typedef DB_UINT64 OID;
#else
    typedef unsigned long OID;
#endif

    typedef DB_INT32 Position;

    typedef DB_UINT16 ContainerType;
    typedef OID ContainerID;

    typedef DB_INT32 errCode;

    typedef OID SegmentID;
    typedef OID PageID;
    typedef DB_BOOL DirtyFlag;

    typedef DB_INT8 CursorType;

// type that related with the index 
    typedef OID ISegmentID;
    typedef OID IPageID;

    typedef OID IndexID;
    typedef DB_UINT8 IndexType;

// TTREE NODE Type
    typedef OID NodeID;
    typedef DB_UINT8 Height;
    typedef DB_UINT8 NodeType;
    typedef OID RecordID;
// Field Type

// lock type // C/S structure
    typedef DB_UINT8 LCRP;
    typedef DB_UINT8 LOCKMODE;
    typedef DB_UINT8 RelationType;

#define SHORT_SIZE          sizeof(DB_INT16)
#define INT_SIZE            sizeof(DB_INT32)
#define OID_SIZE            sizeof(OID)
#define WCHAR_SIZE          sizeof(DB_WCHAR)

#define NULL_OID 0
#ifdef _64BIT
#define INVALID_OID         0xFFFFFFFFFFFFFFFF
#else
#define INVALID_OID         0xFFFFFFFF
#endif

    typedef DB_UINT32 RecordSize;

#ifdef  __cplusplus
}
#endif

#endif
