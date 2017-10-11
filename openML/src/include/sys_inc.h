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

#ifndef _SYS_INCLDUE_H_
#define _SYS_INCLDUE_H_

#ifdef  __cplusplus
extern "C"
{
#endif

#include "mdb_config.h"

#ifdef USE_WCHAR_H
#include <wchar.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#if defined(_AS_WIN32_MOBILE_)

#include <Windows.h>
#include <process.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>              // Needed only for _O_RDWR definition
#include <signal.h>
#include <ctype.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <io.h>
#include <sys/timeb.h>
#include <errno.h>
#include <float.h>

#elif defined(_AS_WINCE_)

#include <Winbase.h>
#include <Winsock.h>
#include <types.h>
#include <float.h>

#elif defined(psos)

#include <unistd.h>
#include <sys/stat.h>
#include <types.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <sys/pollcom.h>
#include <ctype.h>
#include <dirent.h>
#include <float.h>

#elif defined(sun) || defined(linux)

#include <unistd.h>
#include <sys/mman.h>
#include <strings.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/ipc.h>
#if defined(ANDROID_OS)
#include <linux/shm.h>
#include <linux/msg.h>
#else
#include <sys/shm.h>
#include <sys/msg.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sched.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <assert.h>
#if !defined(ANDROID_OS)
#include <sys/statvfs.h>
#endif
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <semaphore.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>         // IPPROTO_IP
#include <sys/socket.h>         // socket()
#include <net/if.h>             // struct ifreq
#include <sys/ioctl.h>          // ioctl() and SIOCGIFHWADDR
#include <unistd.h>             // close()
#include <time.h>               // mktime()
#include <wchar.h>

#ifdef linux
#include <malloc.h>
#if !defined(ANDROID_OS)
#include <values.h>
#endif
#endif

#if defined(sun)
#include <netinet/in_systm.h>
#endif

#ifdef sun
#include <sys/un.h>
#include <inet/ip.h>
#include <inet/tcp.h>
#elif defined(linux)
#include <netinet/tcp.h>
#include <sys/un.h>
#endif

#include <dirent.h>

#endif                          // sun || linux

#ifdef  __cplusplus
}
#endif

#endif                          // _SYS_INCLDUE_H_
