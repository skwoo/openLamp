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

// LeftTreeView.cpp : implementation file
//

#include "stdafx.h"
#include "Admin.h"
#include "MainFrm.h"
#include "LeftTreeView.h"
#include "RightListView.h"
#include "SQLDirect.h"
#include "UserDlg.h"
#include "AddDSNDlg.h"
#include "WizardTableScript.h"
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
// CLeftTreeView

IMPLEMENT_DYNCREATE(CLeftTreeView, CTreeView)

CLeftTreeView::CLeftTreeView()
{
	n_TreeFolderClosed = 0;
	n_TreeFolderOpenSel = 1;
	n_TreeApp = 2;
	n_TreeDB = 3;
	n_TreeTable = 4;
	n_TreeFile = 5;
	n_TreeUpgrade = 6;
	m_bMakeTree = FALSE;
}

CLeftTreeView::~CLeftTreeView()
{
}


BEGIN_MESSAGE_MAP(CLeftTreeView, CTreeView)
	//{{AFX_MSG_MAP(CLeftTreeView)
	ON_NOTIFY_REFLECT(NM_CLICK, OnClick)
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY_REFLECT(NM_RCLICK, OnRclick)
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(ID_NEW_USER, OnNewUser)
	ON_COMMAND(ID_NEW_SYNCSCRIPT, OnNewSyncscript)
	ON_COMMAND(ID_ADD_TABLE, OnAddTable)
	ON_COMMAND(ID_MODIFY_APP, OnModifyApp)
	ON_COMMAND(ID_MODIFY_DSN, OnModifyDsn)
	ON_COMMAND(ID_MODIFY_TABLE, OnModifyTable)
	ON_COMMAND(ID_DELETE_APP, OnDeleteApp)
	ON_COMMAND(ID_DELETE_DSN, OnDeleteDsn)
	ON_COMMAND(ID_DELETE_TABLE, OnDeleteTable)
	ON_COMMAND(ID_ADD_DSN, OnAddDsn)
	ON_COMMAND(ID_ADD_SCRIPT, OnAddScript)
	ON_COMMAND(ID_COPY_APP, OnCopyApp)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLeftTreeView drawing

void CLeftTreeView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CLeftTreeView diagnostics

#ifdef _DEBUG
void CLeftTreeView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CLeftTreeView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLeftTreeView message handlers
BOOL CLeftTreeView::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	// 점선이나 하부 항목이 있는지의 여부에 따라 +,- 표시 할 수 있도록 window style 바꿈
	cs.style |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS; 

	return CTreeView::PreCreateWindow(cs);
}

void CLeftTreeView::OnInitialUpdate() 
{
	CTreeView::OnInitialUpdate();
	// TODO: Add your specialized code here and/or call the base class

	HTREEITEM       hTreeParent, hCurItem;	// Parent of recently added item

	m_pMainFrame = (CMainFrame *)AfxGetMainWnd();

	AddBitMap();	// 

	hTreeParent = AddItemToTree(TVI_ROOT, IDS_MENU1);
	hCurItem = AddItemToTree(hTreeParent, IDS_MENU1_SUB1);
	hCurItem = AddItemToTree(hTreeParent, IDS_MENU1_SUB2);
	m_hTreeScriptParent = AddItemToTree(TVI_ROOT, IDS_MENU2);
	hTreeParent = AddItemToTree(TVI_ROOT, IDS_MENU3);

	m_tableImage.Create(IDB_TABLE, 32, 1, RGB(255, 255, 255));
	m_tableImage.SetBkColor(GetSysColor(COLOR_WINDOW));

	m_DBImage.Create(IDB_DB, 32, 3, RGB(255, 255, 255));
	m_DBImage.SetBkColor(GetSysColor(COLOR_WINDOW));

	m_appImage.Create(IDB_APPLICATION, 32, 1, RGB(255, 255, 255));
	m_appImage.SetBkColor(GetSysColor(COLOR_WINDOW));

	m_folderImage.Create(IDB_FOLDER, 32, 1, RGB(255, 255, 255));
	m_folderImage.SetBkColor(GetSysColor(COLOR_WINDOW));

}

void CLeftTreeView::AddBitMap()
{

	CBitmap	bBitmap;

	m_ctlImage.Create(16, 16, ILC_COLOR, 2, 2);
	
	bBitmap.LoadMappedBitmap(IDB_CLOSED);
	m_ctlImage.Add(&bBitmap, (COLORREF)0x000000);
	bBitmap.DeleteObject();

	bBitmap.LoadMappedBitmap(IDB_OPEN_SEL);
	m_ctlImage.Add(&bBitmap, (COLORREF)0x000000);
	bBitmap.DeleteObject();

	bBitmap.LoadMappedBitmap(IDB_TREEAPP);
	m_ctlImage.Add(&bBitmap, (COLORREF)0x000000);
	bBitmap.DeleteObject();

	bBitmap.LoadMappedBitmap(IDB_TREEDB);
	m_ctlImage.Add(&bBitmap, (COLORREF)0x000000);
	bBitmap.DeleteObject();

	bBitmap.LoadMappedBitmap(IDB_DISPLAYTAB);
	m_ctlImage.Add(&bBitmap, (COLORREF)0x000000);
	bBitmap.DeleteObject();

	bBitmap.LoadMappedBitmap(IDB_TREEFILE);
	m_ctlImage.Add(&bBitmap, (COLORREF)0x000000);
	bBitmap.DeleteObject();

	bBitmap.LoadMappedBitmap(IDB_TREEUPGRADE);
	m_ctlImage.Add(&bBitmap, (COLORREF)0x000000);
	bBitmap.DeleteObject();

	GetTreeCtrl().SetImageList( &m_ctlImage , TVSIL_NORMAL );
}

// folder image로 구성할 경우
HTREEITEM CLeftTreeView::AddItemToTree(HTREEITEM hTreeParent, UINT nID)
{
	HTREEITEM		hCurItem;
	TV_INSERTSTRUCT tvstruct;				// Info for inserting Tree items

	CString strTemp;
	char cTemp[MAX_PATH];

	strTemp.LoadString(nID);
	strcpy(cTemp,strTemp);

	tvstruct.hParent = hTreeParent;
	tvstruct.hInsertAfter = TVI_LAST;
	tvstruct.item.iImage = n_TreeFolderClosed;
	tvstruct.item.iSelectedImage = n_TreeFolderOpenSel;	
	tvstruct.item.pszText = cTemp;
	tvstruct.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT;
	hCurItem = GetTreeCtrl().InsertItem(&tvstruct);
	m_strMenu.AddTail(strTemp);
	return hCurItem;
}

// DB나 table, file의 image로 구성할 경우
HTREEITEM CLeftTreeView::AddItemToTree(HTREEITEM hTreeParent, 
                                       LPCSTR cTemp, int nIcon)
{
	HTREEITEM		hCurItem;
	TV_INSERTSTRUCT tvstruct;				// Info for inserting Tree items

	tvstruct.hParent = hTreeParent;
	tvstruct.hInsertAfter = TVI_LAST;
	tvstruct.item.iImage = nIcon;
	tvstruct.item.iSelectedImage = nIcon;	
	tvstruct.item.pszText = (char *)cTemp;
	tvstruct.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT;
	hCurItem = GetTreeCtrl().InsertItem(&tvstruct);
	return hCurItem;
}

#define LOG_TIMER_INTERVAL  (3000)  /* 3 sec */
#define LOG_TIMER_ID        (1010)
static BOOL gl_bLogTime = FALSE;

void CLeftTreeView::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
    if( nIDEvent == LOG_TIMER_ID )
    {
        KillTimer(LOG_TIMER_ID);
        gl_bLogTime = FALSE;
        m_pMainFrame->m_pRightListView->DisplayLogs();        
        SetTimer(LOG_TIMER_ID, LOG_TIMER_INTERVAL, NULL); 
        gl_bLogTime = TRUE;
    }
	
	CTreeView::OnTimer(nIDEvent);
}

void CLeftTreeView::OnClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
    // TODO: Add your control notification handler code here
    HTREEITEM	hItem;
    
    hItem = GetTreeCtrl().GetSelectedItem();
    CString  strSel = GetTreeCtrl().GetItemText(hItem);
    
    m_nSelectedMenu = GetMenuIndex(strSel);
    m_pMainFrame->ChangeTitleText(strSel);	
    
    if( gl_bLogTime )
    {
        KillTimer(LOG_TIMER_ID);
        gl_bLogTime = FALSE;
    }

    switch(m_nSelectedMenu)
    {
    case MENU_USER_MGR:
        ChangeListCtrlStyle(FOLDER_STYLE);
        m_pMainFrame->m_pRightListView->DisplayUserMgr();
        break;
    case MENU_USER_PROFILE:
        ChangeListCtrlStyle(REPORT_STYLE);
        m_pMainFrame->m_pRightListView->DisplayAllUsers();
        break;
    case MENU_CONNECTED_USER:
        ChangeListCtrlStyle(REPORT_STYLE);
        m_pMainFrame->m_pRightListView->DisplayConnectedUsers();
        break;
    case MENU_SYNC_SCRIPT_MGR:
        ChangeListCtrlStyle(APP_STYLE);
        if(m_bMakeTree == FALSE)
        {
            if(MakeSyncScriptTree() == FALSE) return;
            m_bMakeTree = TRUE;
        }
        
        m_pMainFrame->m_pRightListView->DisplayApplications();
        break;
    case MENU_LOG_MGR:
        ChangeListCtrlStyle(REPORT_STYLE);
        m_pMainFrame->m_pRightListView->DisplayLogs();        
        SetTimer(LOG_TIMER_ID, LOG_TIMER_INTERVAL, NULL);
        gl_bLogTime = TRUE;
        break;
    default:	// syncScript Manager의 sub menu
        m_nSelectedMenu = GetTreeLevel(hItem);
        if(m_nSelectedMenu == APP_LEVEL){
            ChangeListCtrlStyle(DSN_STYLE);
            // APPLICATION:VERSION
            m_pMainFrame->m_pRightListView->DisplayDSNs(hItem, strSel);	
        }
        else if (m_nSelectedMenu == DSN_LEVEL){
            ChangeListCtrlStyle(TABLE_STYLE);
            // DSN
            m_pMainFrame->m_pRightListView->DisplayTables(hItem, strSel);
        }			
        else { // table
            ChangeListCtrlStyle(REPORT_STYLE);
            // TABLE
            m_pMainFrame->m_pRightListView->DisplayScripts(hItem, strSel);	
        }
        
        break;
    }
    
    *pResult = 0;
}

int CLeftTreeView::GetMenuIndex(CString strSel)
{
	POSITION pos = m_strMenu.Find(strSel);

	for(int i=0 ; i<m_strMenu.GetCount() ; i++){
		if(pos == m_strMenu.FindIndex(i))
			return i;
	}
	return (-1);
}

void CLeftTreeView::OnLButtonDown(UINT nFlags, CPoint point) 
{
    // TODO: Add your message handler code here and/or call default    
    CTreeCtrl& tree = GetTreeCtrl();	
    UINT uFlags;	
    HTREEITEM hItem = tree.HitTest(point, &uFlags);	
    if ((hItem != NULL) && (TVHT_ONITEM & uFlags))	
    {		
        tree.Select(hItem, TVGN_CARET);		        
    }	
    CTreeView::OnLButtonDown(nFlags, point);
}

void CLeftTreeView::ChangeListCtrlStyle(int nType)
{
    if(nType == REPORT_STYLE)
    {
        CListCtrl& theCtrl = m_pMainFrame->m_pRightListView->GetListCtrl();
        theCtrl.ModifyStyle(LVS_TYPEMASK, LVS_REPORT);
    }
    else 
    {
        // 리스트뷰의 스타일을 큰 아이콘 리스트로 바꾼다.
        CListCtrl& theCtrl = m_pMainFrame->m_pRightListView->GetListCtrl();
        theCtrl.ModifyStyle(LVS_TYPEMASK, LVS_ICON);
        
        if(nType == FOLDER_STYLE)
            m_pMainFrame->m_pRightListView->GetListCtrl().SetImageList (&m_folderImage,TVSIL_NORMAL);
        else if(nType == APP_STYLE)
            m_pMainFrame->m_pRightListView->GetListCtrl().SetImageList (&m_appImage,TVSIL_NORMAL);
        else if(nType == DSN_STYLE)
            m_pMainFrame->m_pRightListView->GetListCtrl().SetImageList (&m_DBImage,TVSIL_NORMAL);
        else if(nType == TABLE_STYLE)
            m_pMainFrame->m_pRightListView->GetListCtrl().SetImageList (&m_tableImage,TVSIL_NORMAL);        
    }
}
/* Root의 level이 0*/
int CLeftTreeView::GetTreeLevel(HTREEITEM hCurItem)
{
    int nLevel = 0;
    
    if (hCurItem == NULL) return (-1);
    HTREEITEM hParent = GetTreeCtrl().GetNextItem(hCurItem, TVGN_PARENT);
    while(hParent)
    {
        nLevel++;
        hParent = GetTreeCtrl().GetNextItem(hParent, TVGN_PARENT);
    }
    return nLevel+MENU_SYNC_SCRIPT_MGR*10;
}
void CLeftTreeView::MakeDSNTableTree(CSQLDirect *aDSNSD, CSQLDirect *aTabSD, 
                                     CString strVID, HTREEITEM hDSNParent)
{
    HTREEITEM hScriptParent, hItem;
    CString strDBID;
    char szQuery[QUERY_MAX_LEN];
    
    sprintf(szQuery, "SELECT SvrDSN, DBID FROM openMSYNC_DSN "
        "WHERE VersionID = %s ORDER BY SvrDSN", strVID);
    if (aDSNSD->ExecuteSQL(szQuery) == SQL_SUCCESS)
    {
        while (aDSNSD->FetchSQL() == SQL_SUCCESS)		// application fetch
        {
            // DSN tree 추가
            hScriptParent = AddItemToTree(hDSNParent, 
                aDSNSD->GetColumnValue(1), n_TreeDB);
            // DBID로 Table 검색
            strDBID = aDSNSD->GetColumnValue(2);
            sprintf(szQuery, "SELECT SvrTableName FROM openMSYNC_Table "
                "WHERE DBID = %s ORDER BY SvrTableName", strDBID);
            if (aTabSD->ExecuteSQL(szQuery) == SQL_SUCCESS)
            {
                while (aTabSD->FetchSQL() == SQL_SUCCESS)// application fetch
                {
                    hItem = AddItemToTree(hScriptParent, 
                        aTabSD->GetColumnValue(1), n_TreeTable);
                }
            }
        }
    }
}


BOOL CLeftTreeView::MakeSyncScriptTree()
{
    CSQLDirect aAppSD, aDSNSD, aTabSD;
    HTREEITEM	hDSNParent;
    
    if(aAppSD.ConnectToDB() == FALSE || aDSNSD.ConnectToDB() == FALSE ||
        aTabSD.ConnectToDB() == FALSE ) 
        return FALSE;
    
    //m_hTreeScriptParent의 child를 찾아서 삭제한 뒤에
    // 아래를 수행하도록 한다.
    
    InitializeSyncScriptTree();
    CString strApp, strVID, strDBID;
    char cTemp[MAX_PATH];
    
    if (aAppSD.ExecuteSQL("SELECT Application, Version, VersionID "
        "FROM openMSYNC_Version ORDER BY Application, Version") == SQL_SUCCESS)
    {
        while (aAppSD.FetchSQL() == SQL_SUCCESS)		// application fetch
        {
            // Application.Version tree 추가
            strApp = aAppSD.GetColumnValue(1);	
            sprintf(cTemp, "%s.%s", strApp, aAppSD.GetColumnValue(2));
            hDSNParent = AddItemToTree(m_hTreeScriptParent, cTemp, n_TreeApp);
            
            // VersionID로 DSN 검색
            strVID = aAppSD.GetColumnValue(3);			
            MakeDSNTableTree(&aDSNSD, &aTabSD, strVID, hDSNParent);         
        }
    }
    InvalidateRect(&CRect(0, 0, 32000, 30)); 
    return TRUE;
}

void CLeftTreeView::InitializeSyncScriptTree()
{
	HTREEITEM hNextItem;
	HTREEITEM hChildItem = GetTreeCtrl().GetChildItem(m_hTreeScriptParent);		

	while (hChildItem != NULL)
	{
		hNextItem = GetTreeCtrl().GetNextItem(hChildItem, TVGN_NEXT);
		GetTreeCtrl().DeleteItem(hChildItem);
		hChildItem = hNextItem;
	}
}

void CLeftTreeView::DeleteItemFromTree(CString strSel)
{
	HTREEITEM	hItem, hChildItem;

	hItem = GetTreeCtrl().GetSelectedItem();
	hChildItem = GetTreeCtrl().GetChildItem(hItem);
	while(hChildItem != NULL)
	{
		if(GetTreeCtrl().GetItemText(hChildItem) == strSel)
		{
			GetTreeCtrl().DeleteItem(hChildItem);
			break;
		}
		hChildItem = GetTreeCtrl().GetNextItem(hChildItem, TVGN_NEXT);
	}
}

void CLeftTreeView::ChangeItemText(CString strBeforeSel, CString strAfterSel)
{
	HTREEITEM	hItem, hChildItem;

	hItem = GetTreeCtrl().GetSelectedItem();
	hChildItem = GetTreeCtrl().GetChildItem(hItem);
	while(hChildItem != NULL)
	{
		if(GetTreeCtrl().GetItemText(hChildItem) == strBeforeSel)
		{
			GetTreeCtrl().SetItemText(hChildItem, strAfterSel);
			break;
		}
		hChildItem = GetTreeCtrl().GetNextItem(hChildItem, TVGN_NEXT);
	}

}

void CLeftTreeView::ChangeItemText(CString strAfterSel)
{
	HTREEITEM	hItem;

	hItem = GetTreeCtrl().GetSelectedItem();
	GetTreeCtrl().SetItemText(hItem, strAfterSel);
}

void CLeftTreeView::OnRclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
    OnClick(pNMHDR,pResult);
    // TODO: Add your control notification handler code here
    switch(m_nSelectedMenu)
    {
    case MENU_USER_MGR:
        if(m_pMainFrame->CheckDBSetting())
            MakeMenu(IDR_USERMENU);
        break;
    case MENU_CONNECTED_USER:	
    case MENU_LOG_MGR:		
    case MENU_USER_PROFILE:		
        break;
    case MENU_SYNC_SCRIPT_MGR:// SyncScript Manager이 선택된 상태
        if(m_bMakeTree)		  // Tree가 구성되지 않은 경우는 DB가 셋팅되지 않은 상태 
            MakeMenu(IDR_SYNCSCRIPTMENU);
        break;
    case APP_LEVEL:		// Application이 한개 선택된 상태
        MakeMenu(IDR_APPMENU);
        break;
    case DSN_LEVEL:		// DSN 한개가 선택된 상태
        MakeMenu(IDR_DSNMENU);
        break;
    case TABLE_LEVEL:	// Table 한개가 선택된 상태
        MakeMenu(IDR_TABLEMENU);
        break;
    }
    *pResult = 0;
}
void CLeftTreeView::MakeMenu(int nID)
{
    CMenu menu, *pSubMenu;
    if(!menu.LoadMenu(nID))	return;
    if(!(pSubMenu = menu.GetSubMenu(0))) return;
    if(nID==IDR_USERMENU)
    {
        menu.GetSubMenu(0)->DeleteMenu(0, MF_BYPOSITION);	// Modify User
        menu.GetSubMenu(0)->DeleteMenu(0, MF_BYPOSITION);	// Delete User
        menu.GetSubMenu(0)->DeleteMenu(0, MF_BYPOSITION);	// seperator
    }
    CPoint pos;
    GetCursorPos(&pos);
    //::SetForegroundWindow(m_nid.hWnd);
    pSubMenu->TrackPopupMenu(TPM_LEFTALIGN, pos.x, pos.y, this);
    menu.DestroyMenu();
}

void CLeftTreeView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CTreeCtrl& tree = GetTreeCtrl();	
	UINT uFlags;	
	HTREEITEM hItem = tree.HitTest(point, &uFlags);	
	if ((hItem != NULL) && (TVHT_ONITEM & uFlags))	
	{		
		tree.Select(hItem, TVGN_CARET);		
	}	

	CTreeView::OnRButtonDown(nFlags, point);
}


void CLeftTreeView::OnNewUser() 
{
	// TODO: Add your command handler code here
	CUserDlg	pUserDlg(this);
	pUserDlg.SetState(ID_NEW_USER, NO_ACTION);
	pUserDlg.DoModal();	
}

void CLeftTreeView::OnNewSyncscript() 
{
	// TODO: Add your command handler code here
    CAddDSNDlg dlg;

    dlg.m_bNewApp = TRUE;
    dlg.m_nAction = ADD_LIST;
    dlg.DoModal();
    delete dlg;
}

HTREEITEM CLeftTreeView::GetTreeScriptParent()
{
	return m_hTreeScriptParent;
}

void CLeftTreeView::InitialTree()
{
	m_bMakeTree = FALSE;
}

void CLeftTreeView::OnModifyApp() 
{
    // TODO: Add your command handler code here
    HTREEITEM	hItem;
    CApplicationDlg pAppDlg(m_pMainFrame);
    
    hItem = GetTreeCtrl().GetSelectedItem();
    CString  strAppVer = GetTreeCtrl().GetItemText(hItem);
    
    if(strAppVer == "") return;
    m_pMainFrame->m_pRightListView->GetApplicationVersion(strAppVer, 
                                &pAppDlg.m_strAppName, &pAppDlg.m_strVersion);
    
    CSQLDirect aSD;
    char szQuery[QUERY_MAX_LEN];
    
    if(aSD.ConnectToDB() == FALSE)  return;
    
    sprintf(szQuery, "SELECT VersionID FROM openMSYNC_Version "
                     "WHERE Application = '%s' AND Version = %s",
                     pAppDlg.m_strAppName, pAppDlg.m_strVersion);
    if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)
    {
        AfxMessageBox(strAppVer+"을 수정하는 데에 실패하였습니다");
        return;
    }
    
    CString strVerID;
    while(aSD.FetchSQL() == SQL_SUCCESS)
    {
        strVerID = aSD.GetColumnValue(1);
    }	
    pAppDlg.SetValue(-1, strVerID, strAppVer, MODIFY_LEFT);// versionID, App.Ver
    pAppDlg.DoModal();
}

void CLeftTreeView::OnModifyDsn() 
{
    // TODO: Add your command handler code here	
    HTREEITEM	hItem;
    CModifyDSNDlg pDSNDlg(m_pMainFrame);
    
    hItem = GetTreeCtrl().GetSelectedItem();
    CString  strDSN = GetTreeCtrl().GetItemText(hItem);
    CString  strApp, strVer, strDBID;
    
    if(strDSN == "") return;
    
    m_pMainFrame->m_pRightListView->GetApplicationVersion(hItem, &strApp, &strVer);
    
    CSQLDirect aSD;
    char szQuery[QUERY_MAX_LEN];
    
    if(aSD.ConnectToDB() == FALSE)  return;
    
    sprintf(szQuery, "SELECT DBID, DSNUserID, DSNPwd, CliDSN "
        "FROM openMSYNC_Version v, openMSYNC_DSN d "
        "WHERE v.VersionID = d.VersionID AND v.Application = '%s' "
        "AND v.Version = %s AND d.SvrDSN = '%s'",
        strApp, strVer, strDSN);
    if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)
    {
        AfxMessageBox(strDSN+"을 수정하는 데에 실패하였습니다");
        return;
    }
    while(aSD.FetchSQL() == SQL_SUCCESS)
    {
        strDBID = aSD.GetColumnValue(1);
        pDSNDlg.m_strDSNName = strDSN;
        pDSNDlg.m_strDSNUid = aSD.GetColumnValue(2);
        pDSNDlg.m_strDSNPwd = aSD.GetColumnValue(3);
        pDSNDlg.m_strCliDSN = aSD.GetColumnValue(4);
    }
    pDSNDlg.SetValue(-1, strDSN, strDBID, MODIFY_LEFT);
    pDSNDlg.DoModal();	
}

void CLeftTreeView::OnModifyTable() 
{
	// TODO: Add your command handler code here
	HTREEITEM	hItem;
	CModifyTableDlg pTableDlg(m_pMainFrame);

	hItem = GetTreeCtrl().GetSelectedItem();
	CString  strTable = GetTreeCtrl().GetItemText(hItem);
	CString  strApp, strVer, strDSN, strTableID;

	if(strTable == "") return;
	
	m_pMainFrame->m_pRightListView->GetApplicationVersionDSN(hItem, &strApp, &strVer, &strDSN);

	CSQLDirect aSD;
	char szQuery[QUERY_MAX_LEN];

	if(aSD.ConnectToDB() == FALSE)  return;
	
	sprintf(szQuery, "SELECT TableID, CliTableName "
        "FROM openMSYNC_Table t, openMSYNC_DSN d, openMSYNC_Version v "
        "WHERE t.DBID = d.DBID AND d.VersionID = v.VersionID AND "
        "v.Application = '%s' and v.Version = %s AND d.SvrDSN = '%s' AND "
        "t.SvrTableName = '%s'", strApp, strVer, strDSN, strTable);
	if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)
	{
		AfxMessageBox(strTable+"을 수정하는 데에 실패하였습니다");
		return;
	}
	while(aSD.FetchSQL() == SQL_SUCCESS)
	{
		strTableID = aSD.GetColumnValue(1);
		pTableDlg.m_strCliTabName = aSD.GetColumnValue(2);
	}
	pTableDlg.SetValue(strTable, strTableID);
	pTableDlg.DoModal();			
}


void CLeftTreeView::OnCopyApp() 
{
	// TODO: Add your command handler code here
	HTREEITEM	hItem;
	hItem = GetTreeCtrl().GetSelectedItem();
	CString  strAppVer = GetTreeCtrl().GetItemText(hItem);
	CApplicationDlg pAppDlg(m_pMainFrame);
	
	if(strAppVer != ""){
		/* application, version의 VersionID 구하기 */
		m_pMainFrame->m_pRightListView->GetApplicationVersion(strAppVer, 
            &pAppDlg.m_strAppName, &pAppDlg.m_strVersion);

		CSQLDirect aSD;
		char szQuery[QUERY_MAX_LEN];

		if(aSD.ConnectToDB() == FALSE)  return;

		sprintf(szQuery, "SELECT VersionID FROM openMSYNC_Version "
            "WHERE Application = '%s' AND Version = %s",
			pAppDlg.m_strAppName, pAppDlg.m_strVersion);
		if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)
		{
			AfxMessageBox(strAppVer+"의 script를 복사하는 데에 실패하였습니다");
			return;
		}

		CString strVerID = "";
		if(aSD.FetchSQL() == SQL_SUCCESS)
		{
			strVerID = aSD.GetColumnValue(1);
		}
		
		if(strVerID.GetLength() == 0)
		{
			AfxMessageBox(pAppDlg.m_strAppName+"의 버전 "+pAppDlg.m_strVersion
                +"에 대한 정보를 읽을 수 없습니다");
			return;
		}
		pAppDlg.SetValue(-1, strVerID, strAppVer, COPY_LEFT); // versionID, App.Ver
		pAppDlg.DoModal();
	}
}

void CLeftTreeView::OnDeleteApp() 
{
    // TODO: Add your command handler code here
    HTREEITEM	hItem;
    hItem = GetTreeCtrl().GetSelectedItem();
    CString  strAppVer = GetTreeCtrl().GetItemText(hItem);
    
    if(strAppVer != "")
    {
        if(AfxMessageBox("선택하신 어플리케이션 '" + strAppVer 
            + "' 항목을 삭제하시겠습니까?", MB_YESNO|MB_ICONSTOP)==IDYES)
        {
            CSQLDirect aSD1, aSD2, aSD3;
            CString strApp, strVer, strVerID;
            char szQuery[QUERY_MAX_LEN];
            
            if(aSD1.ConnectToDB() == FALSE || aSD2.ConnectToDB() == FALSE 
                || aSD3.ConnectToDB() == FALSE)
            {
                return;
            }
            
            m_pMainFrame->m_pRightListView->GetApplicationVersion(strAppVer, 
                &strApp, &strVer);
            // VersionID 얻어서
            sprintf(szQuery, "SELECT VersionID FROM openMSYNC_Version "
                "WHERE Application = '%s' AND Version = %s", 
                strApp, strVer);
            if(aSD1.ExecuteSQL(szQuery) != SQL_SUCCESS)
            {
                AfxMessageBox("삭제 중에 에러가 발생하였습니다");
                return;
            }
            while(aSD1.FetchSQL() == SQL_SUCCESS)
            {
                strVerID = aSD1.GetColumnValue(1);
            }
            // 해당 DBID 얻기
            sprintf(szQuery, "SELECT DBID FROM openMSYNC_DSN "
                "WHERE VersionID = %s", strVerID);
            if(aSD1.ExecuteSQL(szQuery) != SQL_SUCCESS)
            {
                AfxMessageBox("삭제 중에 에러가 발생하였습니다");
                return;
            }
            // 해당 DBID와 관련된 Table, script 삭제
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
            sprintf(szQuery, "DELETE FROM openMSYNC_Version "
                "WHERE VersionID = %s", strVerID);
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
            GetTreeCtrl().DeleteItem(hItem);	// tree에서 삭제
            DisplayAfterDelete();
        }		
    }	
}

void CLeftTreeView::OnDeleteDsn() 
{
	// TODO: Add your command handler code here

	HTREEITEM	hItem;
	hItem = GetTreeCtrl().GetSelectedItem();
	CString  strDSN = GetTreeCtrl().GetItemText(hItem);
	CString  strApp, strVer, strDBID;

	if(strDSN != "")
	{
		if(AfxMessageBox("선택하신 DSN '" + strDSN 
            + "' 항목을 삭제하시겠습니까?", MB_YESNO|MB_ICONSTOP)==IDYES)
		{
			CSQLDirect aSD1, aSD2, aSD3;
			char szQuery[QUERY_MAX_LEN];

			if(aSD1.ConnectToDB() == FALSE || aSD2.ConnectToDB() == FALSE 
                || aSD3.ConnectToDB() == FALSE) 
            {
                return;
            }

			m_pMainFrame->m_pRightListView->GetApplicationVersion(hItem, &strApp, &strVer);

			sprintf(szQuery, "SELECT DBID FROM openMSYNC_Version v, openMSYNC_DSN d "
                "WHERE v.VersionID = d.VersionID AND v.Application = '%s' "
                "AND v.Version = %s AND d.SvrDSN = '%s'",
				strApp, strVer, strDSN);
			if(aSD1.ExecuteSQL(szQuery) != SQL_SUCCESS)
			{
				AfxMessageBox(strDSN+"을 수정하는 데에 실패하였습니다");
				return;
			}
			while(aSD1.FetchSQL() == SQL_SUCCESS)
			{
				strDBID = aSD1.GetColumnValue(1);
			}
			if(m_pMainFrame->DeleteDSNTableScript(&aSD2, &aSD3, strDBID) == FALSE) 
			{
				aSD1.RollBack();
				aSD2.RollBack();
				aSD3.RollBack();
				return;
			}
			aSD1.Commit();
			aSD2.Commit();
			aSD3.Commit();
			GetTreeCtrl().DeleteItem(hItem);	// tree에서 삭제
			DisplayAfterDelete();
		}		
	}		
}

void CLeftTreeView::OnDeleteTable() 
{
	// TODO: Add your command handler code here
	HTREEITEM	hItem;
	hItem = GetTreeCtrl().GetSelectedItem();
	CString  strTable = GetTreeCtrl().GetItemText(hItem);
	CString  strApp, strVer, strDSN, strTableID;

	if(strTable != "")
	{
		if(AfxMessageBox("선택하신 테이블 '" + strTable 
            + "' 항목을 삭제하시겠습니까?", MB_YESNO|MB_ICONSTOP)==IDYES)
		{
			CSQLDirect aSD1, aSD2;
			char szQuery[QUERY_MAX_LEN];

			if(aSD1.ConnectToDB() == FALSE || aSD2.ConnectToDB() == FALSE)
                return;

			m_pMainFrame->m_pRightListView->GetApplicationVersionDSN(hItem, 
                &strApp, &strVer, &strDSN);
	
			sprintf(szQuery, "SELECT TableID FROM openMSYNC_Table t, "
                "openMSYNC_DSN d, openMSYNC_Version v WHERE t.DBID = d.DBID "
                "AND d.VersionID = v.VersionID AND v.Application = '%s' "
                "and v.Version = %s AND d.SvrDSN = '%s' AND t.SvrTableName = '%s'", 
				strApp, strVer, strDSN, strTable);
			if(aSD1.ExecuteSQL(szQuery) != SQL_SUCCESS)
			{
				AfxMessageBox(strTable+"을 수정하는 데에 실패하였습니다");
				return;
			}
			while(aSD1.FetchSQL() == SQL_SUCCESS)
			{
				strTableID = aSD1.GetColumnValue(1);
			}

			if(m_pMainFrame->DeleteTableScript(&aSD2, strTableID) == FALSE)
			{
				aSD1.RollBack();
				aSD2.RollBack();
				return;
			}			
			aSD1.Commit();
			aSD2.Commit();
			GetTreeCtrl().DeleteItem(hItem);	// tree에서 삭제
			DisplayAfterDelete();
		}		
	}		
}

void CLeftTreeView::DisplayAfterDelete()
{
	HTREEITEM  hItem;

	hItem = GetTreeCtrl().GetSelectedItem();
	CString  strSel = GetTreeCtrl().GetItemText(hItem);
	
	m_nSelectedMenu = GetMenuIndex(strSel);
	m_pMainFrame->ChangeTitleText(strSel);	

	switch(m_nSelectedMenu){
		case MENU_SYNC_SCRIPT_MGR:
			ChangeListCtrlStyle(APP_STYLE);
			m_pMainFrame->m_pRightListView->DisplayApplications();
			break;
		default:	// syncScript Manager의 sub menu
			m_nSelectedMenu = GetTreeLevel(hItem);
			if(m_nSelectedMenu == APP_LEVEL){
				ChangeListCtrlStyle(DSN_STYLE);
                // APPLICATION:VERSION
				m_pMainFrame->m_pRightListView->DisplayDSNs(hItem, strSel);	
			}
			else if (m_nSelectedMenu == DSN_LEVEL){
				ChangeListCtrlStyle(TABLE_STYLE);
                // DSN
				m_pMainFrame->m_pRightListView->DisplayTables(hItem, strSel);	
			}
			else { // table
				ChangeListCtrlStyle(REPORT_STYLE);
                // TABLE
				m_pMainFrame->m_pRightListView->DisplayScripts(hItem, strSel);	
			}

			break;
	}
}

void CLeftTreeView::OnAddDsn() 
{
	// TODO: Add your command handler code here
	HTREEITEM	hItem;
	CAddDSNDlg pAddDSNDlg(m_pMainFrame);
	
	hItem = GetTreeCtrl().GetSelectedItem();
	CString  strAppVer = GetTreeCtrl().GetItemText(hItem);
	
	if(strAppVer == "") return;
	m_pMainFrame->m_pRightListView->GetApplicationVersion(strAppVer, 
		&pAddDSNDlg.m_strAppName, &pAddDSNDlg.m_strVersion);	
	pAddDSNDlg.DoModal();	
}

void CLeftTreeView::OnAddTable() 
{
	// TODO: Add your command handler code here
	CWizardTableScript  pWizardTableScript("");
	HTREEITEM	hItem;

	// 선택한 것은 DSN
	hItem = GetTreeCtrl().GetSelectedItem();
	CString strDSN = GetTreeCtrl().GetItemText(hItem);

	CString strApp, strVer, strUid, strPwd;
	m_pMainFrame->m_pRightListView->GetApplicationVersion(hItem, &strApp, &strVer);

	// DSNUid, DSNPwd 구하기
	CSQLDirect aSD1, aSD2;
	char szQuery[QUERY_MAX_LEN];

	if(aSD1.ConnectToDB() == FALSE)  return;

	sprintf(szQuery, "SELECT DSNUserID, DSNPwd FROM openMSYNC_Version v, openMSYNC_DSN d "
        "WHERE v.VersionID = d.VersionID AND v.Application = '%s' "
        "AND v.Version = %s AND d.SvrDSN = '%s'",
		strApp, strVer, strDSN);
	if(aSD1.ExecuteSQL(szQuery) != SQL_SUCCESS)
	{
		AfxMessageBox(strDSN+"에 Table을 추가하는 데에 실패하였습니다");
		return;
	}
	while(aSD1.FetchSQL() == SQL_SUCCESS)
	{
		strUid = aSD1.GetColumnValue(1);
		strPwd = aSD1.GetColumnValue(2);
	}
	pWizardTableScript.SetValue(ID_ADD_TABLE, 
        strApp, strVer, strDSN , strUid, strPwd);

	if(aSD2.ConnectToDB(strDSN, strUid, strPwd) == FALSE)
	{
		AfxMessageBox(strDSN
            +"의 정보가 잘못되어 Table을 추가하는 데에 실패하였습니다");
		return;
	}

	pWizardTableScript.DoModal();
	delete pWizardTableScript;
}

void CLeftTreeView::OnAddScript() 
{
	// TODO: Add your command handler code here
	CWizardTableScript  pWizardTableScript("");
	HTREEITEM	hItem;

	// 선택한 것은 Table
	hItem = GetTreeCtrl().GetSelectedItem();
	CString strTable = GetTreeCtrl().GetItemText(hItem);

	// parent로부터 Application.Version 정보 구함
	
	CString strApp, strVer, strDSN, strUid, strPwd;
	m_pMainFrame->m_pRightListView->GetApplicationVersionDSN(hItem, 
        &strApp, &strVer, &strDSN);

	// DSNUid, DSNPwd 구하기
	CSQLDirect aSD1, aSD2;
	char szQuery[QUERY_MAX_LEN];

	if(aSD1.ConnectToDB() == FALSE)  return;

	sprintf(szQuery, "SELECT DSNUserID, DSNPwd FROM openMSYNC_Version v, "
        "openMSYNC_DSN d WHERE v.VersionID = d.VersionID "
        "AND v.Application = '%s' AND v.Version = %s AND d.SvrDSN = '%s'",
		strApp, strVer, strDSN);
	if(aSD1.ExecuteSQL(szQuery) != SQL_SUCCESS)
	{
		AfxMessageBox(strTable+"에 script를 추가하는 데에 실패하였습니다");
		return;
	}
	while(aSD1.FetchSQL() == SQL_SUCCESS)
	{
		strUid = aSD1.GetColumnValue(1);
		strPwd = aSD1.GetColumnValue(2);
	}
	pWizardTableScript.SetValue(ID_ADD_SCRIPT, 
        strApp, strVer, strDSN , strUid, strPwd, strTable);

	if(aSD2.ConnectToDB(strDSN, strUid, strPwd) == FALSE)
	{
		AfxMessageBox(strTable
            +"의 정보가 잘못되어 script를 추가하는 데에 실패하였습니다");
		return;
	}

	pWizardTableScript.DoModal();
	delete pWizardTableScript;	
}


