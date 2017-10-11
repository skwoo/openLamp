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

#ifndef NET_H
#define NET_H

typedef struct _network_parameter{
    int socketFd ;
    int sendDataLength ;
    int receiveDataLength ;
    char *sendBuffer ;
    char *receiveBuffer ;
    char *recordBuffer;
    int sendBufferSize ;
    int receiveBufferSize ;
    int	nextState ;
} NETWORK_PARAMETER ;

int	 InitServerSocket(int sport );
int	 ProcessNetwork(char *clientAddr);
int  WriteToNet(int fd, char *buf, size_t byte);
void SetTimeout(int timeout);
int	 ReadFrNet(int sock, char *buf, size_t msgsz);

#endif
