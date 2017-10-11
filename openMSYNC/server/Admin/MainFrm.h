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

// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__D289274A_DCD0_4DBD_8BF4_E224BB1437EB__INCLUDED_)
#define AFX_MAINFRM_H__D289274A_DCD0_4DBD_8BF4_E224BB1437EB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MODIFY_LEFT   0
#define MODIFY_RIGHT  1
#define COPY_LEFT 2
#define COPY_RIGHT 3

#define REPORT_STYLE 0
#define FOLDER_STYLE 1
#define APP_STYLE 2
#define DSN_STYLE 3
#define TABLE_STYLE 4

#define MENU_USER_MGR		0
#define MENU_USER_PROFILE	1
#define MENU_CONNECTED_USER	2
#define MENU_SYNC_SCRIPT_MGR	3
#define MENU_LOG_MGR	4

#define APP_LEVEL 31
#define DSN_LEVEL 32
#define TABLE_LEVEL 33

#define ORACLE 1
#define MSSQL  2
#define MYSQL  3
#define ACCESS 4
#define SYBASE 5
#define CUBRID 6
#define DB2    7

class CRightListView;
class CRightView;
class CLeftTreeView;
class CSQLDirect;
#include "MSyncIPBar.h"

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:
	BOOL			m_bConnectFlag;
	CSplitterWnd	m_wndSplitter;
	CLeftTreeView*	m_pLeftTreeView;
	CRightListView*	m_pRightListView;
	CRightView*		m_pRightView;


	CString	m_strDSNName;
	CString	m_strPWD;
	CString	m_strLoginID;
	int		m_nDBType;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
public:
	void GetDBServerType(CString strDSNName);
	int GetFileCount(CString strItem);
	BOOL DeleteTableScript(CSQLDirect *aSD, CString tableID);
	BOOL DeleteDSNTableScript(CSQLDirect *aSD1, CSQLDirect *aSD2, CString strDBID);
	BOOL CheckDBSetting();
	void ShowSystemDSN(CComboBox *pWnd);
	void ChangeTitleText(CString strTitle);
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
//	CToolBar    m_wndToolBar;    // 2003.8.20 toolbar에 mSync IP 보이게
	CMSyncIPBar m_wndToolBar;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnMenuCreatedsn();
	afx_msg void OnMenuNewSyncScript();
	afx_msg void OnMenuDBConnect();
	afx_msg void OnMenuDBDisConnect();
	afx_msg void OnMenuNewUser();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__D289274A_DCD0_4DBD_8BF4_E224BB1437EB__INCLUDED_)
