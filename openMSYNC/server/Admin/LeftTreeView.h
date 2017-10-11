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

#if !defined(AFX_LEFTTREEVIEW_H__3EC905C0_478A_41DC_B05A_2E1120A4B0C1__INCLUDED_)
#define AFX_LEFTTREEVIEW_H__3EC905C0_478A_41DC_B05A_2E1120A4B0C1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LeftTreeView.h : header file
//

class CMainFrame;
/////////////////////////////////////////////////////////////////////////////
// CLeftTreeView view

class CLeftTreeView : public CTreeView
{
protected:
	CLeftTreeView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CLeftTreeView)

// Attributes
public:
	int	n_TreeFolderClosed;
	int	n_TreeFolderOpenSel;
	int	n_TreeApp;
	int	n_TreeDB;
	int	n_TreeTable;
	int n_TreeFile;
	int n_TreeUpgrade;
private:
	HTREEITEM       m_hTreeScriptParent;
	CImageList		m_ctlImage;
	CImageList		m_DBImage, m_folderImage, m_tableImage, m_appImage;
	BOOL			m_bMakeTree;
	CMainFrame	*	m_pMainFrame;
	CList<CString,CString&> m_strMenu;
protected:
//	NOTIFYICONDATA m_nid;

//	CPoint		m_LastRClickPoint;
//	HTREEITEM	m_LastSelectedItem;

// Operations
public:
	void MakeDSNTableTree(CSQLDirect *aDSNSD, CSQLDirect *aTabSD, CString strVID, HTREEITEM hDSNParent);
	void DisplayAfterDelete();
	void ChangeItemText(CString strAfterSel);
	void InitialTree();
	HTREEITEM GetTreeScriptParent();
	void MakeMenu(int nID);
	void ChangeItemText(CString strBeforeSel, CString strAfterSel);
	void DeleteItemFromTree(CString strSel);
	int m_nSelectedMenu;
	void InitializeSyncScriptTree();
	HTREEITEM AddItemToTree(HTREEITEM hTreeParent, LPCSTR cTemp, int nIcon);
	BOOL MakeSyncScriptTree();
	int GetTreeLevel( HTREEITEM hCurItem);
	void ChangeListCtrlStyle(int nType);
	int GetMenuIndex(CString strSel);
	HTREEITEM AddItemToTree(HTREEITEM hTreeParent, UINT nID);
private:
	void AddBitMap();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLeftTreeView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CLeftTreeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CLeftTreeView)
	afx_msg void OnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnNewUser();
	afx_msg void OnNewSyncscript();
	afx_msg void OnAddTable();
	afx_msg void OnModifyApp();
	afx_msg void OnModifyDsn();
	afx_msg void OnModifyTable();
	afx_msg void OnDeleteApp();
	afx_msg void OnDeleteDsn();
	afx_msg void OnDeleteTable();
	afx_msg void OnAddDsn();
	afx_msg void OnAddScript();	
	afx_msg void OnCopyApp();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LEFTTREEVIEW_H__3EC905C0_478A_41DC_B05A_2E1120A4B0C1__INCLUDED_)
