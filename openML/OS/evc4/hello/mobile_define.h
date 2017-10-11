/* 
   This file is part of openML, mobile and embedded DBMS.

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

#ifndef __MOBILE_DEFINE_H__
#define __MOBILE_DEFINE_H__

#include "mdb_config.h"
#include "isql.h"


#if 0
#if defined(_AS_WINCE_)
#pragma comment(lib, ".\\ARMV4\\openML.lib")
#endif
#endif

#if defined(_AS_WINCE_)
#define E_aslitePath            "\\Windows\\openml.cfg"
#else
#define E_aslitePath            "/openml.cfg"
#endif

#define	SQL_OK		0
#define SQL_ERROR	-1
#define	SQL_NO_DATA	NULL


#ifndef QUERY_SIZE
#define QUERY_SIZE		(4*1024)
#endif

#if defined(_AS_WINCE_)
#define TIME_VARIABLE				long
#define TIME_DURATION_VARIABLE	long
#elif defined(_AS_WIN32_MOCHA_) || defined(alessi)
#define TIME_VARIABLE				TmMilliSec
#define TIME_DURATION_VARIABLE	long
#endif

#if defined(_AS_WINCE_)
#define TIME_CHECK_FUNC(x)		{ x = GetTickCount(); }
#define TIME_DURATION_FUNC(start, end)	(start - end) 
#elif defined(_AS_WIN32_MOCHA_) || defined(alessi)
#define TIME_CHECK_FUNC(x)		{ TmGetMilliSeconds(&x); }
#define TIME_DURATION_FUNC(start, end)	((end.second*1000+end.millisecond) - (start.second*1000+start.millisecond))
#endif

#if defined(_AS_WINCE_)
#ifndef BOOL
#define BOOL	bool
#endif
#elif defined(_AS_WIN32_MOCHA_) || defined(alessi)
#ifndef BOOL
#define BOOL	unsigned char
#endif
#endif

#ifndef true
#define true	TRUE
#endif

#ifndef false
#define false	FALSE
#endif

#endif
