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

#if !defined(AFX_WIZARDTABLESCRIPT_H__FC56DE57_58E3_4E02_BA31_59D6EBC3920C__INCLUDED_)
#define AFX_WIZARDTABLESCRIPT_H__FC56DE57_58E3_4E02_BA31_59D6EBC3920C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WizardTableScript.h : header file
//
#include "PanelTable.h"
#include "PanelScript.h"

/////////////////////////////////////////////////////////////////////////////
// CWizardTableScript

class CWizardTableScript : public CPropertySheet
{
	DECLARE_DYNAMIC(CWizardTableScript)

// Construction
public:
	CWizardTableScript(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CWizardTableScript(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:
	CPanelTable m_panelTable;
	CPanelScript m_panelScript;
	int		m_nState;
	CString	m_strApp;
	CString	m_strVer;
	CString	m_strDSNName;
	CString	m_strPWD;
	CString	m_strLoginID;
	CString	m_strTableName;
	CString m_strCliTableName;
	CCheckListBox *m_chkListSelColumn;
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWizardTableScript)
	public:
	virtual int DoModal();
	//}}AFX_VIRTUAL

// Implementation
public:
	void SetValue(int nState, CString strApp, CString strVer, CString strDSN, CString strUid, CString strPwd, CString strTable="");
	virtual ~CWizardTableScript();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWizardTableScript)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIZARDTABLESCRIPT_H__FC56DE57_58E3_4E02_BA31_59D6EBC3920C__INCLUDED_)
