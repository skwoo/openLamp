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

// RightListView.cpp : implementation file
//

#include "stdafx.h"
#include "Admin.h"
#include "RightListView.h"
#include "MainFrm.h"		
#include "DBConnectDlg.h"	
#include "UserDlg.h"
#include "SQLDirect.h"
#include "LeftTreeView.h"	
#include "ApplicationDlg.h"
#include "ModifyDSNDlg.h"
#include "ModifyTableDlg.h"
#include "ModifyScriptDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRightListView

IMPLEMENT_DYNCREATE(CRightListView, CListView)

CRightListView::CRightListView()
{
	m_bFileExist = TRUE;
}

CRightListView::~CRightListView()
{
}


BEGIN_MESSAGE_MAP(CRightListView, CListView)
	//{{AFX_MSG_MAP(CRightListView)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	ON_COMMAND(ID_MODIFY_USER, OnModifyUser)
	ON_COMMAND(ID_DELETE_USER, OnDeleteUser)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnRclick)
	ON_COMMAND(ID_DELETE_APP, OnDeleteApp)
	ON_COMMAND(ID_DELETE_DSN, OnDeleteDsn)
	ON_COMMAND(ID_DELETE_SCRIPT, OnDeleteScript)
	ON_COMMAND(ID_DELETE_TABLE, OnDeleteTable)
	ON_COMMAND(ID_MODIFY_APP, OnModifyApp)
	ON_COMMAND(ID_MODIFY_DSN, OnModifyDsn)
	ON_COMMAND(ID_MODIFY_SCRIPT, OnModifyScript)
	ON_COMMAND(ID_MODIFY_TABLE, OnModifyTable)
	ON_COMMAND(ID_COPY_APP, OnCopyApp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRightListView drawing

void CRightListView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CRightListView diagnostics

#ifdef _DEBUG
void CRightListView::AssertValid() const
{
	CListView::AssertValid();
}

void CRightListView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CRightListView message handlers
BOOL CRightListView::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	cs.style |= LVS_SHOWSELALWAYS | LVS_REPORT;

	return CListView::PreCreateWindow(cs);
}
// supiajin 3.25 PreCreateWindow에서 지정한 style로 list control을 만든다
void CRightListView::OnInitialUpdate() 
{
	// TODO: Add your specialized code here and/or call the base class	

	CListView::OnInitialUpdate();
	
	// Gain a reference to the list control itself
	CListCtrl& theCtrl = GetListCtrl();
	
	// change listview's Style 
	theCtrl.SetExtendedStyle(theCtrl.GetExtendedStyle()|LVS_EX_GRIDLINES
        |LVS_EX_FULLROWSELECT| LVS_EX_ONECLICKACTIVATE| LVS_EX_FLATSB );

	m_ImageList.Create(IDB_TABLE,32,32,RGB(255,255,255));
	m_ImageList.SetBkColor(GetSysColor(COLOR_WINDOW));

	// 메인프레임 포인트 얻어서 m_pMainTreeView를 가져온다
	m_pMainFrame=(CMainFrame *)AfxGetMainWnd(); 	

	
}

BOOL CRightListView::AddStringToList(char *cColName[])
{
    int ListNum = GetListCtrl().GetItemCount();	
    GetListCtrl().InsertItem( ListNum,(LPCTSTR) cColName[0] );
    for(int i=1;i<m_nColSize;i++)
	{
		if(!GetListCtrl().SetItem(
            ListNum,i,LVIF_TEXT,(LPCTSTR) cColName[i],-1,0,0,NULL ))
        {        
			return FALSE;
        }
	}
	return	TRUE;
}

void CRightListView::DisplayUserMgr()
{
	char *szCol1[] = {"User Profile"};
	char *szCol2[] = {"Connected Users"};
	GetListCtrl().DeleteAllItems();
	m_nColSize = 1;
	AddStringToList(szCol1);
	AddStringToList(szCol2);
}

void CRightListView::DisplayAllUsers()
{
	GetListCtrl().DeleteAllItems();
    Fetch_N_MakeList("SELECT UserID, UserName, DBSyncTime  "
                     "FROM openMSYNC_User ORDER BY UserID");
}

void CRightListView::DisplayConnectedUsers()
{
	GetListCtrl().DeleteAllItems();
    Fetch_N_MakeList("SELECT UserID, UserName, DBSyncTime FROM openMSYNC_User "
                     "WHERE ConnectionFlag = '1' ORDER BY UserID");
}

void CRightListView::DisplayApplications()
{
	GetListCtrl().DeleteAllItems();

	int wid[] = {0, 500};
	char *szColName[] = {"Application", ""};
	InitializeColumn(2, szColName, wid);
	
	char *szColValue[2];
	CString  strCol, strApp;
	CSQLDirect aSD;

	if(aSD.ConnectToDB() == FALSE) return ;

	if(aSD.ExecuteSQL("SELECT Application, Version, VersionID "
                        "FROM openMSYNC_Version") == SQL_SUCCESS)
	{
		while(aSD.FetchSQL() == SQL_SUCCESS)
		{

			CString strApp = aSD.GetColumnValue(1);
            // Application.Version
			strCol.Format("%s.%s", strApp, aSD.GetColumnValue(2));	
			szColValue[0] = new char[strCol.GetLength()+1];
			strcpy(szColValue[0], strCol.GetBuffer(0));
			strCol = aSD.GetColumnValue(3);
			szColValue[1] = new char[strCol.GetLength()+1];
            // VersionID
			strcpy(szColValue[1], strCol.GetBuffer(0));	
			
			AddStringToList(szColValue);
			for(int i=0 ; i<2 ; i++)			
				delete szColValue[i];
		}
	}
}

void CRightListView::DisplayDSNs(HTREEITEM hItem, CString strAppVer)
{
	GetListCtrl().DeleteAllItems();

	int wid[] = {0, 500};
	char *szColName[] = {"DSN", ""};
	InitializeColumn(2, szColName, wid);
	
	char *szColValue[2];
	CString  strCol, strVer, strApp;

	char szQuery[QUERY_MAX_LEN];
	CSQLDirect aSD;
	
	if(aSD.ConnectToDB() == FALSE) return ;

	GetApplicationVersion(strAppVer, &strApp, &strVer);
	sprintf(szQuery, "SELECT SvrDSN, DBID FROM openMSYNC_Version v, "
        "openMSYNC_DSN d WHERE v.VersionID = d.VersionID "
        "AND v.Application = '%s' AND v.Version = %s",
		strApp, strVer);
	if(aSD.ExecuteSQL(szQuery) == SQL_SUCCESS)
	{
		while(aSD.FetchSQL() == SQL_SUCCESS)
		{
			for(int i=0 ; i<2 ; i++){
				strCol = aSD.GetColumnValue(i+1);
				szColValue[i] = new char[strCol.GetLength()+1];
				strcpy(szColValue[i], strCol.GetBuffer(0));
			}
			AddStringToList(szColValue);
			for(i=0 ; i<2 ; i++)			
				delete szColValue[i];
		}
	}
}

void CRightListView::DisplayTables(HTREEITEM hItem, CString strDSN)
{
	GetListCtrl().DeleteAllItems();
	
	int wid[] = {0, 500};
	char *szColName[] = { "Tables", ""};
	InitializeColumn(2, szColName, wid);

	char *szColValue[2];
	CString  strCol, strApp, strVer;
	char szQuery[QUERY_MAX_LEN];
	CSQLDirect aSD;

	if(aSD.ConnectToDB() == FALSE) return ;

	GetApplicationVersion(hItem, &strApp, &strVer);
	sprintf(szQuery, "SELECT SvrTableName, TableID FROM openMSYNC_Table t, "
        "openMSYNC_Version v, openMSYNC_DSN d WHERE v.VersionID = d.VersionID "
        "AND v.Application = '%s' AND v.Version = %s AND d.DBID = t.DBID "
        "AND d.SvrDSN = '%s'", strApp, strVer, strDSN);
	if(aSD.ExecuteSQL(szQuery) == SQL_SUCCESS)
	{
		while(aSD.FetchSQL() == SQL_SUCCESS)
		{
			for(int i=0 ; i<2 ; i++){
				strCol = aSD.GetColumnValue(i+1);
				szColValue[i] = new char[strCol.GetLength()+1];
				strcpy(szColValue[i], strCol.GetBuffer(0));
			}
			AddStringToList(szColValue);
			for(i=0 ; i<2 ; i++)			
				delete szColValue[i];
		}
	}
}

void CRightListView::DisplayScripts(HTREEITEM hItem, CString strTable)
{
	GetListCtrl().DeleteAllItems();

	int wid[] = {0, 0, 120, 500};
	char *szColName[] = {"", "", "Event", "Script"};
	char *szColValue[4];

	InitializeColumn(4, szColName, wid);
	CString  strCol, strApp, strVer, strDSN;
	char szQuery[QUERY_MAX_LEN];
	CSQLDirect aSD;
	
	if(aSD.ConnectToDB() == FALSE) return ;

	GetApplicationVersionDSN(hItem, &strApp, &strVer, &strDSN);
	sprintf(szQuery, "SELECT s.TableID, Event, Event, Script \
		FROM openMSYNC_Script s, openMSYNC_Table t, openMSYNC_Version v, openMSYNC_DSN d \
		WHERE s.TableID = t.TableID AND t.SvrTableName = '%s' \
		AND t.DBID = d.DBID AND d.SvrDSN = '%s' \
		AND d.VersionID = v.VersionID AND v.Application = '%s' AND v.Version = %s", 
		strTable, strDSN, strApp, strVer);
	if(aSD.ExecuteSQL(szQuery) == SQL_SUCCESS)
	{
		while(aSD.FetchSQL() == SQL_SUCCESS)
		{
			for(int i=0 ; i<4 ; i++){
				strCol = aSD.GetColumnValue(i+1);
				if(i==2)	// 'U' => Upload_Update 이런 식으로
					strCol = ChangeEventName(strCol);
				szColValue[i] = new char[strCol.GetLength()+1];
				strcpy(szColValue[i], strCol.GetBuffer(0));
			}
			AddStringToList(szColValue);
			for(i=0 ; i<4 ; i++)			
				delete szColValue[i];
		}
	}

}

void CRightListView::DisplayLogs()
{
	GetListCtrl().DeleteAllItems();
	
	FILE *fp;
	if((fp = OpenFiles()) == (FILE *)NULL)  return;

	char *pBuf = new char[8193];
	char *ptr;

	/* DateTime, Client, Content */
	int wid[] = {150, 100, 500};
	char *szColName[] = {"DateTime", "Client", "Content"};
	char *szColValue[3];
	InitializeColumn(3, szColName, wid);

	while(fgets(pBuf, 8192, fp) != NULL)
	{
		ptr = strtok(pBuf, ";");
		szColValue[0] = new char[strlen(ptr)+1];
		strcpy(szColValue[0], ptr);
		ptr = strtok(NULL, ";");
		szColValue[1] = new char[strlen(ptr)+1];
		strcpy(szColValue[1], ptr);
		ptr = strtok(NULL, ";");
		szColValue[2] = new char[strlen(ptr)+1];
		strcpy(szColValue[2], ptr);
		AddStringToList(szColValue);
		delete szColValue[0];
		delete szColValue[1];
		delete szColValue[2];
	}
	fclose(fp);
	delete pBuf;
}

void CRightListView::Fetch_N_MakeList(char *szQuery)
{
	/* UserID, Name, DBSyncTime */
	int wid[] = {100, 100, 200};

    char *szColName[] = {"ID", "Name", "DB Sync Time"};
	char *szColValue[3];
    CString  strCol;
	int	i;

	InitializeColumn(3, szColName, wid);
	
	CSQLDirect aSD;
	if(aSD.ConnectToDB() == FALSE) return;
	if(aSD.ExecuteSQL(szQuery) == SQL_SUCCESS)
	{
		while(aSD.FetchSQL() == SQL_SUCCESS)
		{
			for(i=0 ; i<3 ; i++)
			{
				strCol = aSD.GetColumnValue(i+1);
				szColValue[i] = new char[strCol.GetLength()+1];
				strcpy(szColValue[i], strCol.GetBuffer(0));
			}
			AddStringToList(szColValue);
			for(i=0 ; i<3 ; i++)			
				delete szColValue[i];
		}
	}	
}

void CRightListView::InitializeColumn(int nColSize, char *szColName[], int width[])
{
	int i;
	LVCOLUMN lvcol;
	CListCtrl& theCtrl = GetListCtrl();

	while(theCtrl.DeleteColumn(0));

	lvcol.mask = LVCF_FMT|LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
	lvcol.fmt = LVCFMT_LEFT;

	for(i=0 ; i<nColSize; i++)
	{
		lvcol.iSubItem = i;
		lvcol.pszText = szColName[i];
		lvcol.cx = width[i];
		theCtrl.InsertColumn(i, &lvcol);
	}
	m_nColSize = nColSize;
}

FILE * CRightListView::OpenFiles()
{
	CString strFileName;
	CString szAppPath;
	CString strBuf;	CString str;
	char pbuf[1001]="";
	int pos = 0;

	::GetModuleFileName(AfxGetInstanceHandle(), 
        szAppPath.GetBuffer(_MAX_PATH),_MAX_PATH);
	szAppPath.ReleaseBuffer();
	int nLen = szAppPath.ReverseFind('\\');
	szAppPath = szAppPath.Left(nLen);

	strFileName.Format("%s\\ServerLog\\log.txt",szAppPath);

	FILE *fp; 
	fp = fopen(strFileName, "r"); 

	if(fp == NULL)
	{
		if(m_bFileExist == TRUE)
		{
			AfxMessageBox("mSync 로그 파일이 없습니다.");
			m_bFileExist = FALSE;
		}
		return NULL; 
	}
	m_bFileExist = TRUE;
	return fp;
}

void CRightListView::GetApplicationVersion(CString strSel, 
                                           CString *strApplication, 
                                           CString *strVersion)
{
	int idx = strSel.Find(".",0);
	*strApplication = strSel.Left(idx);
	*strVersion = strSel.Right(strSel.GetLength()-(idx+1));
}

void CRightListView::GetApplicationVersion(HTREEITEM hItem, 
                                           CString *strApplication, CString *strVersion)
{
	HTREEITEM hParent = m_pMainFrame->m_pLeftTreeView->GetTreeCtrl().GetNextItem(hItem, TVGN_PARENT);
	CString strSel = m_pMainFrame->m_pLeftTreeView->GetTreeCtrl().GetItemText(hParent);
	GetApplicationVersion(strSel, strApplication, strVersion);	
}

void CRightListView::GetApplicationVersionDSN(HTREEITEM hItem, CString *strApplication, 
                                              CString *strVersion, CString *strDSN)
{
	HTREEITEM hParent = m_pMainFrame->m_pLeftTreeView->GetTreeCtrl().GetNextItem(hItem, TVGN_PARENT);
	*strDSN = m_pMainFrame->m_pLeftTreeView->GetTreeCtrl().GetItemText(hParent);
	hParent = m_pMainFrame->m_pLeftTreeView->GetTreeCtrl().GetNextItem(hParent, TVGN_PARENT);
	CString strSel = m_pMainFrame->m_pLeftTreeView->GetTreeCtrl().GetItemText(hParent);
	GetApplicationVersion(strSel, strApplication, strVersion);	
}

CString CRightListView::ChangeEventName(CString strEvent)
{
	CString	strBuf;
	if(strEvent == "I")
		strBuf = "Upload_Insert";
	else if(strEvent == "U")
		strBuf = "Upload_Update";
	else if(strEvent == "D")
		strBuf = "Upload_Delete";
	else if(strEvent == "F")
		strBuf = "Download_Select";
	else if(strEvent == "R")
		strBuf = "Download_Delete";
	
	return strBuf;
}

void CRightListView::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
    // TODO: Add your control notification handler code here
    HTREEITEM	hItem;
    hItem = m_pMainFrame->m_pLeftTreeView->GetTreeCtrl().GetSelectedItem();
    CString  strSel = 
        m_pMainFrame->m_pLeftTreeView->GetTreeCtrl().GetItemText(hItem);
    
    int nIndex = m_pMainFrame->m_pLeftTreeView->GetMenuIndex(strSel);
    m_pMainFrame->ChangeTitleText(strSel);	
    switch(nIndex)
    {
    case MENU_USER_MGR:	
    case MENU_CONNECTED_USER:
    case MENU_LOG_MGR:
        break;
    case MENU_USER_PROFILE:
        {
            nIndex = GetListCtrl().GetNextItem(-1, LVNI_FOCUSED);
            if(nIndex>=0)
            {
                CUserDlg pUserDlg(m_pMainFrame);
                // UserID
                pUserDlg.m_strUserID = GetListCtrl().GetItemText(nIndex, 0); 
                pUserDlg.SetState(ID_MODIFY_USER, MODIFY_LIST);
                pUserDlg.DoModal();
            }
        }
        break;
    case MENU_SYNC_SCRIPT_MGR:
        break;
    default:	// syncScript Manager의 sub menu
        int nLevel = m_pMainFrame->m_pLeftTreeView->GetTreeLevel(hItem);
        if(nLevel == APP_LEVEL)
        {
            /* none */; 
        }
        else if (nLevel == DSN_LEVEL)
        {
            /* none */;
        }
        else { // Script Modify 화면 넣기
            OnModifyScript();
        }
        break;
    }
    *pResult = 0;
}

int CRightListView::FindIndex(char *cColName)
{	
	LVFINDINFO info;
	info.flags = LVFI_STRING;
	info.psz = cColName;

	return (GetListCtrl().FindItem(&info));
}

void CRightListView::ModifyColValue(int row, int column, LPCSTR lpszStr)
{
	GetListCtrl().SetItemText(row, column, lpszStr);
}

void CRightListView::OnModifyUser() 
{
	// TODO: Add your command handler code here
	int nIndex = GetListCtrl().GetNextItem(-1, LVNI_FOCUSED);
	CUserDlg pUserDlg(m_pMainFrame);
	pUserDlg.m_strUserID = GetListCtrl().GetItemText(nIndex, 0); // UserID
	pUserDlg.SetState(ID_MODIFY_USER, MODIFY_LIST);
	pUserDlg.DoModal();
}

void CRightListView::OnDeleteUser() 
{
	// TODO: Add your command handler code here
	int nIndex = GetListCtrl().GetNextItem(-1, LVNI_FOCUSED);
	CString strUserID = GetListCtrl().GetItemText(nIndex, 0); // UserID	
	
	if(strUserID != "")
	{
		if(AfxMessageBox("선택하신 사용자 '" + strUserID 
            + "' 항목을 삭제하시겠습니까?", MB_YESNO|MB_ICONSTOP)==IDYES)
		{
			CSQLDirect aSD;
	
			if(aSD.ConnectToDB() == FALSE) return ;
			char szQuery[QUERY_MAX_LEN];
			sprintf(szQuery, 
                "DELETE FROM openMSYNC_User WHERE UserID = '%s'", strUserID);
			if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)  
				return;
			GetListCtrl().DeleteItem(nIndex);
		}
	}
}

void CRightListView::OnRclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
    int nIndex ;
    CString strValue;
    
    // TODO: Add your control notification handler code here
    switch(m_pMainFrame->m_pLeftTreeView->m_nSelectedMenu)
    {
    case MENU_USER_MGR:
    case MENU_CONNECTED_USER:	
    case MENU_LOG_MGR:		
        break;
    case MENU_USER_PROFILE:		// userID 한개가 선택된 상태
        MakeMenu(IDR_USERMENU, 2);
        break;
    case MENU_SYNC_SCRIPT_MGR:	// Application.Version 한개가 선택된 상태
        MakeMenu(IDR_APPMENU, 3);
        break;
    case APP_LEVEL:		// DSN 한개가 선택된 상태
        nIndex = GetListCtrl().GetNextItem(-1, LVNI_FOCUSED);
        strValue = GetListCtrl().GetItemText(nIndex, 0); 
        if( strValue != "") 	
            MakeMenu(IDR_DSNMENU, 2);
        break;
    case DSN_LEVEL:		// Table 한개가 선택된 상태
        MakeMenu(IDR_TABLEMENU, 2);
        break;
    case TABLE_LEVEL:	// Script 한개가 선택된 상태
        MakeMenu(IDR_SCRIPTMENU);
        break;
    }
    *pResult = 0;
}

void CRightListView::MakeMenu(int nID, int nPos)
{
	CMenu menu, *pSubMenu;
	if(!menu.LoadMenu(nID))	return;
	if(!(pSubMenu = menu.GetSubMenu(0))) return;
	if(nPos)
	{
		while(menu.GetSubMenu(0)->DeleteMenu(nPos, MF_BYPOSITION));
	}
	CPoint pos;
	GetCursorPos(&pos);
	//::SetForegroundWindow(m_nid.hWnd);
	pSubMenu->TrackPopupMenu(TPM_LEFTALIGN, pos.x, pos.y, this);
	menu.DestroyMenu();
}

void CRightListView::OnDeleteApp() 
{
	// TODO: Add your command handler code here
    int nIndex = GetListCtrl().GetNextItem(-1, LVNI_FOCUSED);
    // 큰 아이콘은 0에 이름이고 1에 VersionID	
    CString strAppVer = GetListCtrl().GetItemText(nIndex, 0); 
    CString strVerID = GetListCtrl().GetItemText(nIndex, 1); 
    
    if(strVerID != "")
    {
        if(AfxMessageBox("선택하신 어플리케이션 '" + strAppVer 
            + "' 항목을 삭제하시겠습니까?", MB_YESNO|MB_ICONSTOP)==IDYES)
        {
            CSQLDirect aSD1, aSD2, aSD3;
            
            if(aSD1.ConnectToDB() == FALSE || aSD2.ConnectToDB() == FALSE 
                || aSD3.ConnectToDB() == FALSE) 
            {
                return;
            }
            
            char szQuery[QUERY_MAX_LEN];
            // VersionID는 있으므로 해당 DBID 얻어서
            sprintf(szQuery,
                "SELECT DBID FROM openMSYNC_DSN WHERE VersionID = %s", strVerID);
            if(aSD1.ExecuteSQL(szQuery) != SQL_SUCCESS)
            {
                AfxMessageBox("삭제 중에 에러가 발생하였습니다");
                return;
            }
            // 매 DBID에 해당하는 Table, Script 삭제
            while(aSD1.FetchSQL() == SQL_SUCCESS)
            {
                CString strDBID = aSD1.GetColumnValue(1);
                if(m_pMainFrame->DeleteDSNTableScript(&aSD2, &aSD3, strDBID) == FALSE)
                {
                    aSD1.RollBack();
                    aSD2.RollBack();
                    aSD3.RollBack();
                    return;
                }
            }
            // application 삭제
            sprintf(szQuery, 
                "DELETE FROM openMSYNC_Version WHERE VersionID = %s", strVerID);
            if(aSD1.ExecuteSQL(szQuery) != SQL_SUCCESS)
            {
                AfxMessageBox("삭제 중에 에러가 발생하였습니다");
                aSD1.RollBack();
                aSD2.RollBack();
                aSD3.RollBack();
                return;
            }
            aSD1.Commit();
            aSD2.Commit();
            aSD3.Commit();
            GetListCtrl().DeleteItem(nIndex);	// list에서 삭제
            m_pMainFrame->m_pLeftTreeView->DeleteItemFromTree(strAppVer);	// tree에서 삭제
            DisplayApplications();	// 전체 application을 display
        }		
    }
}

void CRightListView::OnDeleteDsn() 
{
	// TODO: Add your command handler code here
	int nIndex = GetListCtrl().GetNextItem(-1, LVNI_FOCUSED);
	CString strDSN = GetListCtrl().GetItemText(nIndex, 0);
	CString strDBID = GetListCtrl().GetItemText(nIndex, 1); // 큰 아이콘은 0에 이름이고 1에 DBID	
	
	if(strDBID != "")
	{
		if(AfxMessageBox("선택하신 DSN '" + strDSN 
            + "' 항목을 삭제하시겠습니까?", MB_YESNO|MB_ICONSTOP)==IDYES)
		{
			CSQLDirect aSD1, aSD2;

			if(aSD1.ConnectToDB() == FALSE || aSD2.ConnectToDB() == FALSE) return;

			if(m_pMainFrame->DeleteDSNTableScript(&aSD1, &aSD2, strDBID) == FALSE) 
			{
				aSD1.RollBack();
				aSD2.RollBack();
				return;
			}
			aSD1.Commit();
			aSD2.Commit();
			GetListCtrl().DeleteItem(nIndex);	// list에서 삭제하고
			m_pMainFrame->m_pLeftTreeView->DeleteItemFromTree(strDSN);	// tree에서 삭제

            // 새로 선택된 DSN의 내용을 display
			HTREEITEM hItem;
			hItem = 
                m_pMainFrame->m_pLeftTreeView->GetTreeCtrl().GetSelectedItem();	
			DisplayDSNs(hItem, 
                m_pMainFrame->m_pLeftTreeView->GetTreeCtrl().GetItemText(hItem));
		}		
	}	
}

void CRightListView::OnDeleteTable() 
{
	// TODO: Add your command handler code here
	int nIndex = GetListCtrl().GetNextItem(-1, LVNI_FOCUSED);
    // 큰 아이콘은 0에 이름이고 1에 TableID	
	CString strTable = GetListCtrl().GetItemText(nIndex, 0);
	CString strTableID = GetListCtrl().GetItemText(nIndex, 1); 
	
	if(strTableID != "")
	{
		if(AfxMessageBox("선택하신 테이블 '" + strTable 
            + "' 항목을 삭제하시겠습니까?", MB_YESNO|MB_ICONSTOP)==IDYES)
		{
			CSQLDirect aSD;

			if(aSD.ConnectToDB() == FALSE) return;
			if(m_pMainFrame->DeleteTableScript(&aSD, strTableID) == FALSE)
			{
				aSD.RollBack();
				return;
			}
			aSD.Commit();
			GetListCtrl().DeleteItem(nIndex);  //list에서 삭제
			m_pMainFrame->m_pLeftTreeView->DeleteItemFromTree(strTable);// tree에서 삭제
            // 새로 선택된 table의 내용을 display
			HTREEITEM hItem = 
                m_pMainFrame->m_pLeftTreeView->GetTreeCtrl().GetSelectedItem();	
			DisplayTables(hItem, 
                m_pMainFrame->m_pLeftTreeView->GetTreeCtrl().GetItemText(hItem));
		}		
	}		
}

void CRightListView::OnDeleteScript() 
{
	// TODO: Add your command handler code here
	int nIndex = GetListCtrl().GetNextItem(-1, LVNI_FOCUSED);
    // REPORT 스타일이기 때문에 0번째에 ScriptID	
	CString strScriptID = GetListCtrl().GetItemText(nIndex, 0); 
	CString strEvent = GetListCtrl().GetItemText(nIndex, 1);
	CString strEventName = GetListCtrl().GetItemText(nIndex, 2);
	
	if(strScriptID != "")
	{
		if(AfxMessageBox("선택하신 스크립트 '" + strEventName 
            + "' 항목을 삭제하시겠습니까?", MB_YESNO|MB_ICONSTOP)==IDYES)
		{
			CSQLDirect aSD;
	
			if(aSD.ConnectToDB() == FALSE) return ;

			char szQuery[QUERY_MAX_LEN];
			               //
			if(m_pMainFrame->m_nDBType == SYBASE)
            {
                sprintf(szQuery, "DELETE FROM openMSYNC_Script "
                    "WHERE TableID = %s AND Event = upper('%s')", 
                    strScriptID, strEvent);
            }
			else
            {
                sprintf(szQuery, "DELETE FROM openMSYNC_Script "
                    "WHERE TableID = %s AND Event = '%s'", 
                    strScriptID, strEvent);
            }
			
			if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS) {
				AfxMessageBox("삭제 중에 에러가 발생하였습니다");
				return;
			}
			aSD.Commit();

            // list에서 삭제.  tree에는 없기 때문에 아무 일 안한다.
			GetListCtrl().DeleteItem(nIndex);	
		}		
	}		
}


void CRightListView::OnModifyApp() 
{
	// TODO: Add your command handler code here
	CApplicationDlg pAppDlg(m_pMainFrame);
	int nIndex = GetListCtrl().GetNextItem(-1, LVNI_FOCUSED);
	CString strAppVer = GetListCtrl().GetItemText(nIndex, 0); // Application.Version
	
	if(strAppVer == "") return;
	GetApplicationVersion(strAppVer, &pAppDlg.m_strAppName, &pAppDlg.m_strVersion);
	
	pAppDlg.SetValue(nIndex, GetListCtrl().GetItemText(nIndex, 1), 
        strAppVer, MODIFY_RIGHT); // VersionID, App.Ver
	pAppDlg.DoModal();
}

void CRightListView::OnModifyDsn() 
{
	// TODO: Add your command handler code here
	CModifyDSNDlg pDSNDlg(m_pMainFrame);
	int nIndex = GetListCtrl().GetNextItem(-1, LVNI_FOCUSED);
	CString strDBID = GetListCtrl().GetItemText(nIndex, 1); // DBID

	if(strDBID == "") return;

	CSQLDirect aSD;
	char szQuery[QUERY_MAX_LEN];

	if(aSD.ConnectToDB() == FALSE)  return;
	sprintf(szQuery, "SELECT SvrDSN, DSNUserID, DSNPwd, CliDSN "
        "FROM openMSYNC_DSN WHERE DBID = %s", strDBID);
	if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)
	{
		AfxMessageBox(GetListCtrl().GetItemText(nIndex, 0)
            +"을 수정하는 데에 실패하였습니다");
		return;
	}
	while(aSD.FetchSQL() == SQL_SUCCESS)
	{
		pDSNDlg.m_strDSNName = aSD.GetColumnValue(1);
		pDSNDlg.m_strDSNUid = aSD.GetColumnValue(2);
		pDSNDlg.m_strDSNPwd = aSD.GetColumnValue(3);
		pDSNDlg.m_strCliDSN = aSD.GetColumnValue(4);
	}
	pDSNDlg.SetValue(nIndex, 
        GetListCtrl().GetItemText(nIndex, 0), strDBID, MODIFY_RIGHT);
	pDSNDlg.DoModal();	
}

void CRightListView::OnModifyTable() 
{
	// TODO: Add your command handler code here
	CModifyTableDlg pTableDlg(m_pMainFrame);
	int nIndex = GetListCtrl().GetNextItem(-1, LVNI_FOCUSED);
	CString strTableID = GetListCtrl().GetItemText(nIndex, 1); // TableID

	if(strTableID == "") return;

	CSQLDirect aSD;

	char szQuery[QUERY_MAX_LEN];
	if(aSD.ConnectToDB() == FALSE)  return;
	sprintf(szQuery, "SELECT CliTableName FROM openMSYNC_Table "
                     "WHERE TableID = %s", strTableID);
	if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)
	{
		AfxMessageBox(GetListCtrl().GetItemText(nIndex, 0)
            +"을 수정하는 데에 실패하였습니다");
		return;
	}
	while(aSD.FetchSQL() == SQL_SUCCESS)
	{
		pTableDlg.m_strCliTabName = aSD.GetColumnValue(1);
	}
	pTableDlg.SetValue(GetListCtrl().GetItemText(nIndex, 0), strTableID);
	pTableDlg.DoModal();		
}

void CRightListView::OnModifyScript() 
{
	// TODO: Add your command handler code here
	int nIndex = GetListCtrl().GetNextItem(-1, LVNI_FOCUSED);
	CString strTableID = GetListCtrl().GetItemText(nIndex, 0); // TableID
	CString strEvent = GetListCtrl().GetItemText(nIndex, 1); // Event
	if(strTableID == "" || strEvent == "") return;

	CModifyScriptDlg pScriptDlg(m_pMainFrame);

	CSQLDirect aSD;

	char szQuery[QUERY_MAX_LEN];
	if(aSD.ConnectToDB() == FALSE)  return;
	
	if(m_pMainFrame->m_nDBType == SYBASE)
    {    
		sprintf(szQuery, "SELECT Script FROM openMSYNC_Script "
            "WHERE TableID = %s AND Event = upper('%s')", strTableID, strEvent);
    }
	else
    {
        sprintf(szQuery, "SELECT Script FROM openMSYNC_Script "
            "WHERE TableID = %s AND Event = '%s'", strTableID, strEvent);
    }
	
	if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)
	{
		AfxMessageBox(GetListCtrl().GetItemText(nIndex, 2)
            +"의 script를 수정하는 데에 실패하였습니다");
		return;
	}
	while(aSD.FetchSQL() == SQL_SUCCESS)
	{
		pScriptDlg.m_strScript = aSD.GetColumnValue(1);		
	}
	pScriptDlg.SetValue(nIndex, strTableID, strEvent);
	pScriptDlg.DoModal();			
}

void CRightListView::AddStringToList(LPCSTR szText, int nImage)
{
	LVITEM listItem;
  int ListNum = GetListCtrl().GetItemCount();	
  
	listItem.mask = LVIF_TEXT | LVIF_IMAGE;
	listItem.iItem =ListNum;
	listItem.iSubItem = 0;
	listItem.pszText = (char *)szText;
	listItem.iImage = nImage;
	GetListCtrl().InsertItem(&listItem);

}

void CRightListView::OnCopyApp() 
{
	// TODO: Add your command handler code here
	CApplicationDlg pAppDlg(m_pMainFrame);
	int nIndex = GetListCtrl().GetNextItem(-1, LVNI_FOCUSED);
	CString strAppVer = GetListCtrl().GetItemText(nIndex, 0); // Application.Version
	CString strVersion = GetListCtrl().GetItemText(nIndex, 1);

	if(strAppVer != "") 
	{
		GetApplicationVersion(strAppVer, &pAppDlg.m_strAppName, &pAppDlg.m_strVersion);
		if(strVersion.GetLength() == 0)
		{
			AfxMessageBox(pAppDlg.m_strAppName+"의 버전 "+pAppDlg.m_strVersion
                +"에 대한 정보를 읽을 수 없습니다");			
			return;
		}
        // VersionID, App.Ver
		pAppDlg.SetValue(nIndex, strVersion, strAppVer, COPY_RIGHT); 
		pAppDlg.DoModal();
	}
}
