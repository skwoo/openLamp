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

// mSync.cpp : Defines the entry point for the application.
//

#include    <windows.h>
#include	"resource.h"
#include	<shellapi.h>
#include	<stdio.h>
#include    <direct.h>
#include	<odbcinst.h>
#include	"../mSyncDBCTL/include/mSyncDBCTL.h"
#include	"../version.h"

#define		MAX_LOADSTRING 100

// Global Variables:
HINSTANCE   hInst;						  // current instance
TCHAR		szTitle[MAX_LOADSTRING];	  // The title bar text
TCHAR		szWindowClass[MAX_LOADSTRING];// The title bar text

// Foward declarations of functions included in this code module:
//ATOM			MyRegisterClass(HINSTANCE hInstance);
//BOOL			InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL    CALLBACK About(HWND, UINT, WPARAM, LPARAM);

HWND	hMainWnd;

// Add For Tray Icon
#define			APP_EXIT 0
#define			APP_ABOUT 1

BOOL	bActive;
BOOL	isStart;
HINSTANCE  g_hInstance;

void	PopupMenu(HWND hWnd, int x, int y);
void	ProcessTrayIcon(HWND hWnd, WPARAM wParam, LPARAM lParam);
BOOL	AddIcon(HWND hWnd);
BOOL	DeleteIcon(HWND hWnd);

// for Dialog
BOOL CALLBACK MainDlgProc(HWND hDlg,UINT iMessage,WPARAM wParam,LPARAM lParam);
HWND	hDlgMain;

S_mSyncSrv_Info gSrvInfo;


void    InitDialog();
void    GetSystemDSN(HWND hWnd);
void    GetDBServerType();

DWORD WINAPI win_SyncThread(LPVOID lpParam)
{
    // mSyncDBCTL.dll에 있는 SyncMain()이 sync의 main thread
    SyncMain(&gSrvInfo, hMainWnd);
    return	0;
}

int	APIENTRY WinMain(HINSTANCE	hInstance,
                     HINSTANCE		hPrevInstance,
                     LPSTR		lpCmdLine,
                     int		nCmdShow)
{

    MSG		msg;
    HWND		hWnd;
    WNDCLASS	WndClass;
    char		szAppName[] = "Inervit SyncServer";
    
    // initialize global variable 
    memset( &gSrvInfo, 0x00, sizeof(S_mSyncSrv_Info));
    
    g_hInstance = hInstance;
    
    WndClass.style = NULL;
    WndClass.lpfnWndProc = WndProc;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hInstance = hInstance;
    WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WndClass.hbrBackground = (struct HBRUSH__ *)GetStockObject(BLACK_BRUSH);
    WndClass.lpszMenuName = NULL;
    WndClass.lpszClassName = szAppName;
    
    if (!RegisterClass(&WndClass))
        return	NULL;
    
    isStart = TRUE;
    // DSN Setting Dialog
    DialogBox(g_hInstance, 
              MAKEINTRESOURCE(IDD_DBCONNECTDLG), HWND_DESKTOP, 
              MainDlgProc);
    
    hWnd = CreateWindow(szAppName,
                        szAppName,
                        WS_OVERLAPPEDWINDOW,
                        0, 0,
                        300, 200,
                        NULL, NULL,
                        hInstance,
                        NULL);
    
    
    ShowWindow(hWnd, SW_HIDE);	
    UpdateWindow(hWnd);
    hMainWnd = hWnd;
    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }   
    
    return	msg.wParam;
}

/*******************************************************
//  FUNCTION: MyRegisterClass()
//  PURPOSE: Registers the window class.
/*******************************************************/
ATOM	MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX	wcex;
    
    wcex.cbSize		= sizeof(WNDCLASSEX); 
    wcex.style		= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= (WNDPROC)WndProc;
    wcex.cbClsExtra		= 0;
    wcex.cbWndExtra		= 0;
    wcex.hInstance		= hInstance;
    wcex.hIcon		= LoadIcon(hInstance, (LPCTSTR)IDI_SYNCSRV);
    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName	= (LPCSTR)IDC_SYNCSRV;
    wcex.lpszClassName	= szWindowClass;
    wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);
    
    return	RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
BOOL	InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND		hWnd;
    
    hInst = hInstance; // Store instance handle in our global variable
    
    hWnd = CreateWindow(szWindowClass, 
                        szTitle, 
                        WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, 
                        0, 
                        CW_USEDEFAULT, 
                        0, 
                        NULL, 
                        NULL, 
                        hInstance, 
                        NULL);
    
    if (!hWnd)		return	FALSE;
    
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    
    return	TRUE;
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
// for 메인 메세지 루프 
LRESULT	CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int		xPos, yPos;
    int		wmId, wmEvent;
    
    switch (message) 
    {
    case	WM_CREATE:
        if (isStart && !AddIcon(hWnd))
        {//트레이 아이콘을 추가한다.
            MessageBox(hWnd, "Can't create TrayIcon","", MB_OK);
            PostQuitMessage(0);
        }
        break;
        
    case	WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
        {// 최소화할때는 트레이아이콘만 남게한다
            ShowWindow(hWnd, SW_HIDE);
        }
        break;
        
    case	WM_CONTEXTMENU:
        xPos = LOWORD(lParam);
        yPos = HIWORD(lParam);
        
        PopupMenu(hWnd, xPos, yPos);
        break;
        
    case	WM_COMMAND:
        wmId    = LOWORD(wParam); 
        wmEvent = HIWORD(wParam); 
        // Parse the menu selections:
        switch (wmId)
        {
        case	IDM_ABOUT:
        case	APP_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), 
                HWND_DESKTOP, About);
            break;
            
        case	IDM_EXIT:
            DestroyWindow(hWnd);
            break;
            
        case	APP_EXIT:
            //어플리케이션 종료... 										
            ErrLog("SYSTEM;Stop openMSync™;\n");
            
            MYDeleteCriticalSection();
            DeleteIcon(hWnd); 
            PostQuitMessage(0);
            break;
            
        default:
            return	DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
        
    case	WM_PAINT:
        break;
        
    case	WM_USER + 1:
        ProcessTrayIcon(hWnd, wParam, lParam);
        break;
        
    case	WM_ACTIVATEAPP:
        bActive = (BOOL)wParam;
        break;
        
    case	WM_DESTROY :
        //종료할때는 아이콘 지우기 
        ErrLog("SYSTEM;Stopped openMSync™;\n");
        MYDeleteCriticalSection();
        
        DeleteIcon(hWnd);
        PostQuitMessage(0);
        return	FALSE;
        
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return	0;
}

// for Dialog 메세지 루프 
BOOL CALLBACK MainDlgProc(HWND hDlg,UINT iMessage,WPARAM wParam,LPARAM lParam)
{
    switch (iMessage)
    {
    case	WM_INITDIALOG:
        
        hDlgMain = hDlg;
        InitDialog();
        return	TRUE;
        
    case	WM_COMMAND:
        
        switch (LOWORD(wParam))
        {
        case	IDOK:
            BOOL bTranslated;
            char buf[256];
            DWORD		dwThreadID, dwThrdParam ;
            HANDLE		hThread;
            
            UpdateWindow(hDlgMain); 	
            
            memset( &gSrvInfo, 0x00, sizeof(S_mSyncSrv_Info));		
            
            GetDlgItemText(hDlgMain, IDC_DSNNAME, 
                gSrvInfo.szDsn, MAX_ODBC_DSN_NAME_LEN);
            GetDlgItemText(hDlgMain, IDC_USERID,  
                gSrvInfo.szUid, MAX_ODBC_USERID_LEN);
            GetDlgItemText(hDlgMain, IDC_PWD,     
                gSrvInfo.szPasswd, MAX_ODBC_PASSWD_LEN);
            
            gSrvInfo.nPortNo = 
                GetDlgItemInt(hDlgMain, IDC_PORT, &bTranslated, FALSE);  
            if( !bTranslated )
            {
                SetFocus(GetDlgItem(hDlgMain, IDC_PORT));
                return TRUE;
            }                          
            gSrvInfo.ntimeout = 
                GetDlgItemInt(hDlgMain, IDC_TIMEOUT, &bTranslated, FALSE);
            if( !bTranslated )
            {
                SetFocus(GetDlgItem(hDlgMain, IDC_TIMEOUT));
                return TRUE;
            }              
            gSrvInfo.nMaxUser = 
                GetDlgItemInt(hDlgMain, IDC_MAXUSER, &bTranslated, FALSE);
            if( !bTranslated )
            {
                SetFocus(GetDlgItem(hDlgMain, IDC_MAXUSER));
                return TRUE;
            }              
            GetDBServerType();	
            
            WriteProfileString("INERVIT_CONFIG",
                               "openMSync_DSNName",gSrvInfo.szDsn);
            WriteProfileString("INERVIT_CONFIG",
                               "openMSync_UID",gSrvInfo.szUid);
            WriteProfileString("INERVIT_CONFIG",
                               "openMSync_PWD",gSrvInfo.szPasswd);
            
            sprintf(buf, "%d", gSrvInfo.nPortNo);
            WriteProfileString("INERVIT_CONFIG", "openMSync_PORT",buf);
            sprintf(buf, "%d", gSrvInfo.ntimeout);
            WriteProfileString("INERVIT_CONFIG", "openMSync_TIMEOUT",buf);
            sprintf(buf, "%d", gSrvInfo.nMaxUser);
            WriteProfileString("INERVIT_CONFIG", "openMSync_MAXUSER",buf);
            _getcwd( buf, 200);
            SetPath(buf);					                       
            
            // 
            hThread = CreateThread(NULL, 0, win_SyncThread, 
                &dwThrdParam, 0, &dwThreadID);
            if (hThread == NULL)
            {
                MessageBox(hDlgMain, "Failed begin thread!", "Error", 
                    MB_OK | MB_ICONERROR );                                
            }
            else 
            {				
                ResumeThread(hThread);
            }
            
            EndDialog(hDlg,0);
            return	TRUE;
            
        case	IDCANCEL: 
            isStart = FALSE;
            PostQuitMessage(0);	// 프로그램 종료
            return TRUE;
        }
        
        return	FALSE;
    }
    return	FALSE;
}

void InitDialog()
{
    char cCfg[201];
    int nSize = 200;
    memset(cCfg, 0x00, sizeof(cCfg));
    
    GetSystemDSN(GetDlgItem(hDlgMain, IDC_DSNNAME));	
    
    GetProfileString("INERVIT_CONFIG","openMSync_DSNName","",cCfg,nSize);
    SetDlgItemText(hDlgMain,IDC_DSNNAME, cCfg);	
    GetProfileString("INERVIT_CONFIG","openMSync_UID","",cCfg,nSize);
    SetDlgItemText(hDlgMain,IDC_USERID, cCfg);
    GetProfileString("INERVIT_CONFIG","openMSync_PWD","",cCfg,nSize);
    SetDlgItemText(hDlgMain,IDC_PWD, cCfg);
    
    GetProfileString("INERVIT_CONFIG","openMSync_PORT","",cCfg,nSize);	
    SetDlgItemText(hDlgMain,IDC_PORT, cCfg);
    
    GetProfileString("INERVIT_CONFIG","openMSync_TIMEOUT","",cCfg,nSize);	
    SetDlgItemText(hDlgMain,IDC_TIMEOUT, cCfg);
    
    GetProfileString("INERVIT_CONFIG","openMSync_MAXUSER","",cCfg,nSize);	
    SetDlgItemText(hDlgMain,IDC_MAXUSER, cCfg);
}

// Mesage handler for about box.
BOOL CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    char Version[100];
    
    switch (message)
    {
    case WM_INITDIALOG:
        {
            char szDb[64];
            switch(gSrvInfo.nDbType)
            {
            case DBSRV_TYPE_ORACLE:
                strcpy(szDb, "Oracle");
                break;
            case DBSRV_TYPE_MSSQL:
                strcpy(szDb, "SQL Server");
                break;
            case DBSRV_TYPE_MYSQL:
                strcpy(szDb, "MySQL");
                break;
            case DBSRV_TYPE_ACCESS:
                strcpy(szDb, "MS Access");
                break;
            case DBSRV_TYPE_SYBASE:
                strcpy(szDb, "Sybase");
                break;
            case DBSRV_TYPE_CUBRID:
                strcpy(szDb, "CUBRID");
                break;
            case DBSRV_TYPE_DB2:
                strcpy(szDb, "DB2");
                break;
            }
            
            
            sprintf(Version, "ver %d.%d.%d.%d for %s", 
                MAJOR_VER, MINOR_VER, RELEASE_VER, BUGPATCH_VER, 
                szDb);
            SetDlgItemText(hDlg,IDC_VERSION, Version);	
        }
        return TRUE;
        
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
        {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    
    return	FALSE;
}



// Add in about TrayIcon Function
// PopupMenu context menu
void	PopupMenu(HWND hWnd, int x, int y)
{
    HMENU		hMenu;
    //팝업메뉴를 만든다. 
    hMenu = CreatePopupMenu();
    
    AppendMenu(hMenu, MF_STRING, APP_ABOUT, "About");
    AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
    AppendMenu(hMenu, MF_STRING, APP_EXIT, "Exit");
    
    // setting for WM_RBUTTONDOWN(마우스 오른쪽버튼 클릭)
    SetForegroundWindow(hWnd);
    TrackPopupMenu (hMenu,
                    TPM_LEFTALIGN|TPM_TOPALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
                    x, 
                    y,
                    0,
                    hWnd,
                    NULL);
}

// ProcessTrayIcon:TrayIcon notify message 처리
void	ProcessTrayIcon(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HMENU		hMenu = NULL;
    POINT		pos;
    LPARAM		mousepos;
    
    if (lParam == WM_RBUTTONDOWN)
    {//트레이 아이콘에 마우스 오른쪽 버튼이 눌림. -> CONTEXT MENU
        GetCursorPos(&pos);
        mousepos = pos.y<<16;
        mousepos |= pos.x;
        
        //여기서 바로 메뉴 팝업하는게 아니라 메시지를 보내게 했다.   
        SendMessage(hWnd, WM_CONTEXTMENU, (WPARAM)hWnd, mousepos);
    }
}

// AddIcon : 트레이 아이콘을 추가합니다.
BOOL	 AddIcon(HWND hWnd)
{
    NOTIFYICONDATA		nid;
    
    ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uFlags = NIF_ICON|NIF_MESSAGE|NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    sprintf(nid.szTip, "= openMSync™ ver %d.%d.%d.%d =", 
        MAJOR_VER, MINOR_VER, RELEASE_VER, BUGPATCH_VER);
    
    nid.uID = 0;
    nid.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_TRAY1));
    
    if (Shell_NotifyIcon(NIM_ADD, &nid) == 0)
        return		FALSE;
    
    return			TRUE;
}

// DeleteIcon : 아이콘을 지웁니다.
BOOL	DeleteIcon(HWND hWnd)
{
    NOTIFYICONDATA		nid;
    
    ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
    nid.cbSize	= sizeof(NOTIFYICONDATA);
    nid.hWnd	= hWnd;
    nid.uFlags	= NULL;
    
    if(Shell_NotifyIcon(NIM_DELETE, &nid) == 0)
        return	FALSE;
    
    return	TRUE;
}
// DB server의 type을 알아서 DB마다 다른 내용을 적용한다.
// 가령 sql 문장이나 datetime을 알아내는 함수등...
void GetDBServerType()
{

    /*    HKEY_LOCAL_MACHINE\Software\ODBC\ODBC.INI */
    LONG		lResult;
    HKEY		hKey;
    CHAR		achClass[MAX_PATH] = "";
    DWORD		cchClassName = MAX_PATH,cSubKeys,cbMaxSubKey,cchMaxClass;
    DWORD		cValues, cchMaxValue, cbMaxValueData, cbSecurityDescriptor;
    FILETIME	ftLastWriteTime;
    
    RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\ODBC\\ODBC.INI\\ODBC Data Sources",
        0,KEY_READ,&hKey);
    
    RegQueryInfoKey(hKey,
				    achClass,
                    &cchClassName,
                    NULL,&cSubKeys,
                    &cbMaxSubKey,
                    &cchMaxClass,
                    &cValues,
                    &cchMaxValue,
                    &cbMaxValueData,
                    &cbSecurityDescriptor,
                    &ftLastWriteTime);
    
    DWORD		dwBuffer, dwType;
    char		szSubKeyName[MAX_PATH], szSubValueName[MAX_PATH];
    
    for (DWORD nCount=0 ; nCount < cValues ; nCount++)
    {
        dwBuffer = MAX_PATH;
        lResult = RegEnumValue(hKey,nCount,(LPTSTR)szSubKeyName,
            &dwBuffer,NULL,NULL,NULL, NULL);
        if(!strcmp(szSubKeyName, gSrvInfo.szDsn)){
            dwBuffer = MAX_PATH;
            RegQueryValueEx (hKey, (LPSTR)szSubKeyName, NULL,  &dwType, 
                (BYTE *)szSubValueName, &dwBuffer);
            if(strstr(szSubValueName, "Oracle"))
                gSrvInfo.nDbType = DBSRV_TYPE_ORACLE;	
            else if(strstr(szSubValueName, "SQL Server"))
                gSrvInfo.nDbType = DBSRV_TYPE_MSSQL;	
            else if(strstr(szSubValueName, "MySQL"))
                gSrvInfo.nDbType = DBSRV_TYPE_MYSQL;			
            else if(strstr(szSubValueName, "Access"))
                gSrvInfo.nDbType = DBSRV_TYPE_ACCESS;				
            else if(strstr(szSubValueName, "Adaptive Server Enterprise") ||
                    strstr(szSubValueName, "ASE"))
                gSrvInfo.nDbType = DBSRV_TYPE_SYBASE;	
            else if(strstr(szSubValueName, "CUBRID"))
                gSrvInfo.nDbType = DBSRV_TYPE_CUBRID;	
            else if(strstr(szSubValueName, "DB2"))
                gSrvInfo.nDbType = DBSRV_TYPE_DB2;	
            
            break;
        }		
    }
    RegCloseKey(hKey);
}
void	GetSystemDSN(HWND hWnd)
{
    /*    HKEY_LOCAL_MACHINE\Software\ODBC\ODBC.INI */
    LONG		lResult;
    HKEY		hKey;
    CHAR		achClass[MAX_PATH] = "";
    DWORD		cchClassName = MAX_PATH,cSubKeys,cbMaxSubKey,cchMaxClass;
    DWORD		cValues, cchMaxValue, cbMaxValueData, cbSecurityDescriptor;
    FILETIME	ftLastWriteTime;
    
    RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\ODBC\\ODBC.INI\\ODBC Data Sources",
        0,KEY_READ,&hKey);
    
    RegQueryInfoKey(hKey,
				    achClass,
                    &cchClassName,
                    NULL,&cSubKeys,
                    &cbMaxSubKey,
                    &cchMaxClass,
                    &cValues,
                    &cchMaxValue,
                    &cbMaxValueData,
                    &cbSecurityDescriptor,
                    &ftLastWriteTime);
    
    DWORD		dwBuffer;
    char		szSubKeyName[MAX_PATH];
    
    for (DWORD nCount=0 ; nCount < cValues ; nCount++)
    {
        dwBuffer = MAX_PATH;
        lResult = RegEnumValue(hKey,nCount,(LPTSTR)szSubKeyName,
            &dwBuffer,NULL,NULL,NULL, NULL);
        SendMessage(hWnd, CB_ADDSTRING, 0, (long)szSubKeyName); 
    }
    
    RegCloseKey(hKey);
}

