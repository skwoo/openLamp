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

#if !defined(AFX_ADDDSNDLG_H__5F6C7F1B_B858_4F3A_9B64_2371DD71E4D0__INCLUDED_)
#define AFX_ADDDSNDLG_H__5F6C7F1B_B858_4F3A_9B64_2371DD71E4D0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddDSNDlg.h : header file
//

class CMainFrame;

/////////////////////////////////////////////////////////////////////////////
// CAddDSNDlg dialog

class CAddDSNDlg : public CDialog
{
// Construction
public:
	CAddDSNDlg(CWnd* pParent = NULL);   // standard constructor
	CMainFrame *m_pMainFrame ;

// Dialog Data
	//{{AFX_DATA(CAddDSNDlg)
	enum { IDD = IDD_PANEL_APPDSNDLG };
	CEdit	m_ctlDSNUid;
	CEdit	m_ctlDSNPwd;
	CEdit	m_ctlCliDSN;
	CEdit	m_ctlVersion;
	CEdit	m_ctlAppName;
	CComboBox	m_ctlDSNName;
	CString	m_strAppName;
	CString	m_strCliDSN;
	CString	m_strDSNName;
	CString	m_strDSNPwd;
	CString	m_strDSNUid;
	CString	m_strVersion;
	//}}AFX_DATA

     BOOL m_bNewApp;
     int  m_nAction;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddDSNDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL



// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddDSNDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

    BOOL Check_Data();

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDDSNDLG_H__5F6C7F1B_B858_4F3A_9B64_2371DD71E4D0__INCLUDED_)
