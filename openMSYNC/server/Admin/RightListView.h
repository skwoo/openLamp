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

#if !defined(AFX_RIGHTLISTVIEW_H__CD26F6E6_D2A4_4874_B239_209D5E221851__INCLUDED_)
#define AFX_RIGHTLISTVIEW_H__CD26F6E6_D2A4_4874_B239_209D5E221851__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RightListView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRightListView view
class CMainFrame;
class CRightListView : public CListView
{
protected:
	CRightListView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CRightListView)

// Attributes
public:
	CImageList		m_ImageList;
	int				m_nColSize;
	BOOL m_bFileExist;
protected:
//	NOTIFYICONDATA m_nid;
	CMainFrame*		m_pMainFrame;

// Operations
private:
	FILE * OpenFiles();
	void Fetch_N_MakeList(char *szQuery);
public:
	void AddStringToList(LPCSTR szText, int nImage);
	CString ChangeEventName(CString strEvent);
	void GetApplicationVersionDSN(HTREEITEM hItem, CString *strApplication, CString *strVersion, CString *strDSN);
	void GetApplicationVersion(HTREEITEM hItem, CString *strApplication, CString *strVersion);
	void GetApplicationVersion(CString strSel, CString *strApplication, CString *strVersion);

	void MakeMenu(int nID, int nPos = 0);
	void ModifyColValue(int row, int column, LPCSTR lpszStr);
	int FindIndex(char *cColName);
	void InitializeColumn(int nColSize, char *szColName[], int width[]);

	void DisplayUserMgr();	
	void DisplayAllUsers();
	void DisplayLogs();
	void DisplayApplications();
	void DisplayDSNs(HTREEITEM hItem, CString strSel);
	void DisplayTables(HTREEITEM hItem, CString strSel);
	void DisplayScripts(HTREEITEM hItem, CString strSel);
	void DisplayConnectedUsers();
	BOOL AddStringToList(char *cColName[]);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRightListView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CRightListView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CRightListView)
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnModifyUser();
	afx_msg void OnDeleteUser();
	afx_msg void OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteApp();
	afx_msg void OnDeleteDsn();
	afx_msg void OnDeleteScript();
	afx_msg void OnDeleteTable();
	afx_msg void OnModifyApp();
	afx_msg void OnModifyDsn();
	afx_msg void OnModifyScript();
	afx_msg void OnModifyTable();
	afx_msg void OnCopyApp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RIGHTLISTVIEW_H__CD26F6E6_D2A4_4874_B239_209D5E221851__INCLUDED_)
