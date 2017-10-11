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

#include <string.h>
#include <stdio.h>

#ifdef WIN32
#include <windows.h>
#include <Nb30.h>
#include <direct.h>
#else
#include <inet/tcp.h>
#include <sys/socket.h>  
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>  
#endif

#include "Sync.h"

struct timeval	tv;
static int		serverSocketFd;

/*****************************************************************************/
//! InitServerSocket
/*****************************************************************************/
/*! \breif  Server의 소켓 초기화 부분
 ************************************
 * \param	text(in) : 서버의 port
 ************************************
 * \return	int :  \n
 *			0 < return : Error \n
 *			0 > retrun : Success
 ************************************
 * \note	Socket을 초기화하고 할당 받아 미리 지정한 port에 bind, listen시킴\n
 *****************************************************************************/
int	InitServerSocket(int sport ) /* for server */
{
    struct	sockaddr_in	serv_addr;
#ifdef WIN32
    WSADATA wsaData;
#endif
    int		opton = 1;
    
    memset (&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons( sport );
    
#ifdef WIN32
    if (WSAStartup(0x202,&wsaData) == SOCKET_ERROR) 
    {
        ErrLog("SYSTEM;There is some problems in Socket.;\n");
        WSACleanup();
        return (-1);
    }
#endif

    if (serverSocketFd != -1)
    {    
#ifdef WIN32
        closesocket(serverSocketFd);
#else
        close(serverSocketFd);
#endif
    }
    
    if ((serverSocketFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)  
    {
        ErrLog("SYSTEM;There is some problems in socket();\n");	
#ifdef WIN32        
        WSACleanup();
#endif
        return(-1);
	}

#ifndef WIN32   
    // Windeow에서는 SO_REUSEADDR 사용시 동일 port에 대해서 bind error가 
    // 즉시 검출되지 않아서 서버가 여러개 뜨는 경우가 있어 제외함.
	setsockopt(serverSocketFd, 
               SOL_SOCKET, SO_REUSEADDR, (char *)&opton, sizeof(opton));
#endif

	setsockopt(serverSocketFd, 
               IPPROTO_TCP, TCP_NODELAY, (char *)&opton, sizeof(opton));
	{
        int bufsize = 1024 * 1024;
        // RCVBUF 설정으로 local net에서의 upload시간이 줄어듬.
        setsockopt(serverSocketFd, 
                   SOL_SOCKET, SO_RCVBUF, (char *)&bufsize, sizeof(bufsize));        
	}

    if (bind(serverSocketFd, 
             (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) 
    {
#ifdef WIN32
        closesocket(serverSocketFd);
        WSACleanup();		
#else
        close(serverSocketFd);		
#endif
        ErrLog("SYSTEM;There is some problems in bind();\n");
        serverSocketFd = -1;
        return	(-1);
    }

    if (listen(serverSocketFd, 10000) == -1) 
    {    /* why 10 ? */
#ifdef WIN32
        closesocket(serverSocketFd);
        WSACleanup();		
#else
        close(serverSocketFd);		
#endif
        ErrLog("SYSTEM;There is some problems in listen();\n");
        serverSocketFd = -1;
        return	(-1);
    }
    
    return	(serverSocketFd);
}


/*****************************************************************************/
//! ProcessNetwork
/*****************************************************************************/
/*! \breif  새로운 커넥트를 연결 
 ************************************
 * \param	clientAddr(out)	: 클라이언트의 주소
 ************************************
 * \return	int :  \n
 *			0 < return : Error \n
 *			0 > retrun : Success
 ************************************
 * \note	Client로부터 connection이 맺어져서 accept가 성공하면 \n
 *			새로운 socket fd를 return한다.
 *****************************************************************************/
int	ProcessNetwork(char *clientAddr)
{
    struct sockaddr_in	saddr ;
    int	   length, newSocketFd;
    
    
    length = sizeof(saddr);	
#ifdef WIN32
    newSocketFd = accept(serverSocketFd, (struct sockaddr *)&saddr, &length);
    if(newSocketFd == INVALID_SOCKET) 
#else
        newSocketFd = accept(serverSocketFd, 
        (struct sockaddr *)&saddr, (socklen_t *)&length);
    if(newSocketFd <0)
#endif
    {
#ifdef WIN32
        WSACleanup();
#endif
        return	-1;
    }
    
    memset(clientAddr, (char)0, sizeof(clientAddr));
    strcpy(clientAddr, (char *)inet_ntoa(saddr.sin_addr));
    
    return	newSocketFd;
}

/*****************************************************************************/
//! WriteToNet
/*****************************************************************************/
/*! \breif  데이타 전송시 사용
 ************************************
 * \param	fd(in)	: 소켓 디스크립션
 * \param	buf(in) : 보낼 데이터
 * \param	byte(in): 보낼 바이트의 갯수
 ************************************
 * \return	int :  \n
 *			0 < return : Error \n
 *			0 = retrun : Success
 ************************************
 * \note	Socket에 data를 write할 때 사용하는 함수로 내부적으로는 \n
 *			send()를 호출하며 write 실패시 -1을 return한다
 *****************************************************************************/

int WriteToNet(int fd, char *buf, size_t byte)
{	
	int		ret, left, offset;

	left = byte;
	offset = 0;
	while(left > 0) 
    {
		ret = send(fd, (char* )(buf + offset), left, 0);
#ifdef WIN32
		if (ret == SOCKET_ERROR) {
#else
		if (ret < 0) {
#endif
			return -1;
		}
		
		offset += ret;
		left -= ret;
	}

	return(0);
}


/*****************************************************************************/
//! SetTimeout
/*****************************************************************************/
/*! \breif  Auth.ini에 지정해 놓은 timeout 값을 setting 한다.
 ************************************
 * \param	timeout(in)	: 
 ************************************
 * \return	int :  \n
 *			0 < return : Error \n
 *			0 = retrun : Success
 ************************************
 * \note	Auth.ini에 지정해 놓은 timeout 값으로 셋팅됨 
 *****************************************************************************/
void SetTimeout(int timeout) 
{
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
}

/*****************************************************************************/
//! ReadFrNet
/*****************************************************************************/
/*! \breif  데이타를 받을 때에 사용됨
 ************************************
 * \param	sock(in)	: 소켓 디스크립션
 * \param	buf(out)	: 받을 데이타의 버퍼
 * \param	byte(msgsz)	: 받을 데이타의 사이즈
 ************************************
 * \return	0 : timeout : 1시간 (3600초)
 *			>0 : bytes read
 *			<0 : error
 ************************************
 * \note	Socket에서 data를 read할 때 사용하는 함수로 내부적으로는 \n
 *			timeout을 체크하기 위해 select()와 recv()를 호출하며 \n
 *			read 실패 시 -1을 return 하고 성공 시에는 읽은 data length를 \n
 *			return 한다. 또한 timeout 이 발생했을 때에는 0을 return 한다. \n
 *			\n-- by jinki -- \n
 *			타임아웃이 되면 무조건 에러이다
 *****************************************************************************/
int	ReadFrNet(int sock, char *buf, size_t msgsz)
{
    int nleft, nread;
    char *ptr = (char *)buf;
    fd_set rfds;
    
    int retval;
    
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    
    retval = select(sock+1, &rfds, NULL, NULL, &tv);
    nleft = msgsz;
    *buf = '\0';
    if(retval>0)
    {	
        while (nleft > 0)
        {
            nread = recv(sock, ptr, nleft, 0);            
#ifdef WIN32
            if (nread == SOCKET_ERROR || nread <0){
#else
            if (nread <0){
#endif
                nleft = msgsz+1;
                goto ReadFrNet_END;	// return -1
            }
            else if (nread == 0){
                nleft = msgsz+1;
                goto ReadFrNet_END;	// return -1
            }
            nleft -= nread;
            ptr += nread;
        }
    }
    else if(retval<0){
        nleft = msgsz+1;
        goto ReadFrNet_END;	// return -1
    }
    else {
        goto ReadFrNet_END;	// return 0
    }
    
ReadFrNet_END:
    FD_CLR((u_int)sock, &rfds);
    return (msgsz - nleft); // 정상인 경우 nleft=0
}

