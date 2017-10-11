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

#if !defined(AFX_PANELTABLE_H__25621CF5_6B80_4347_BF1B_6F07F93183C4__INCLUDED_)
#define AFX_PANELTABLE_H__25621CF5_6B80_4347_BF1B_6F07F93183C4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PanelTable.h : header file
//
class CWizardTableScript;

/////////////////////////////////////////////////////////////////////////////
// CPanelTable dialog

class CPanelTable : public CPropertyPage
{
	DECLARE_DYNCREATE(CPanelTable)

// Construction
public:
	CString GetCliTableName(CString strTable);
	int GetCheckStatus(int nCount, CCheckListBox *ctlList);
	void DisplayColumnList(CString strTable);
	CPanelTable();
	~CPanelTable();

// Dialog Data
	//{{AFX_DATA(CPanelTable)
	enum { IDD = IDD_PANEL_TABLEDLG };
	CButton	m_ctlAllColumns;
	CCheckListBox	m_ctlTableList;
	CCheckListBox	m_ctlColumnList;
	BOOL	m_bAllColumns;
	CString	m_strCliTableName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPanelTable)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPanelTable)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeColumnlist();
	afx_msg void OnSelchangeTablelist();
	afx_msg void OnAllcolumns();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	int m_nSelTableIndex;
	CWizardTableScript* m_pWizard;
	int m_nTableCount;
	int m_nColumnCount;
	CString m_strTableName;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PANELTABLE_H__25621CF5_6B80_4347_BF1B_6F07F93183C4__INCLUDED_)
