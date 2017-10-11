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

// MSyncIPBar.cpp: implementation of the CMSyncIPBar class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Admin.h"
#include "MSyncIPBar.h"
#include	<Winsock2.h>
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMSyncIPBar::CMSyncIPBar()
{

}

CMSyncIPBar::~CMSyncIPBar()
{

}

void CMSyncIPBar::MakeEdit(int pos, int toolid, CSize size)
{
	CRect rect(0,0,190,20);    
	CString serverIP ;

	m_Edit.Create("TEST", SS_LEFT,rect,this,toolid);
	GetItemRect(pos,rect);
	m_Edit.SetWindowPos(NULL,
						rect.left+10, 10, 0, 0,
						SWP_NOZORDER|SWP_NOACTIVATE|
						SWP_NOSIZE|SWP_NOCOPYBITS);
	serverIP = GetServerIP();
	m_Edit.SetWindowText("mSync IP : " + serverIP);	
#ifdef WINDOWS	
	m_Edit.ShowWindow(SW_SHOW);
#endif
}

CString CMSyncIPBar::GetServerIP()
{    
    struct hostent* myhost; 
    char		szHostName [40];
    int		n = 0;
    CString serverIP;
    
    WSADATA			wsaData;
    if (WSAStartup(0x202,&wsaData) == SOCKET_ERROR) 
    {
        return "xxx.xxx.xxx.xxx";
    }
    
    //컴 로그인이름
    memset(szHostName, '\0', sizeof(szHostName));
    
    n = gethostname (szHostName, sizeof (szHostName));
    if (n == SOCKET_ERROR) 
    { 
        serverIP.Format("%s", "xxx.xxx.xxx.xxx");
        goto clean_ret;
    }
    if (!strcmp (szHostName, ""))
    { 
        serverIP.Format("%s", "xxx.xxx.xxx.xxx");
        goto clean_ret;
    }
    
    myhost = gethostbyname (szHostName);
    if (myhost == NULL) 
    {
        serverIP.Format("%s", "xxx.xxx.xxx.xxx");
        goto clean_ret;
    }   	

    if( myhost->h_addr_list [0] == NULL )
        serverIP.Format("%s", "xxx.xxx.xxx.xxx");
    else
        serverIP.Format("%s",inet_ntoa (*((struct in_addr *)myhost->h_addr)));
    
clean_ret:
    WSACleanup();
    
    return serverIP;
}

