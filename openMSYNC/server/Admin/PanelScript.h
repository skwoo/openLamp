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

#if !defined(AFX_PANELSCRIPT_H__9BD3ED93_9478_4A4D_A7D9_8B8F18419889__INCLUDED_)
#define AFX_PANELSCRIPT_H__9BD3ED93_9478_4A4D_A7D9_8B8F18419889__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PanelScript.h : header file
//
class CWizardTableScript;
class CMainFrame;
/////////////////////////////////////////////////////////////////////////////
// CPanelScript dialog

class CPanelScript : public CPropertyPage
{
	DECLARE_DYNCREATE(CPanelScript)

// Construction
public:
	void InitializeEvent();
	void DisableExistScript();
	CPanelScript();
	~CPanelScript();

// Dialog Data
	//{{AFX_DATA(CPanelScript)
	enum { IDD = IDD_PANEL_SCRIPTDLG };
	CTreeCtrl	m_ctlTabColumnTree;
	CString	m_strScript;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPanelScript)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPanelScript)
	afx_msg void OnEventD();
	afx_msg void OnEventF();
	afx_msg void OnEventI();
	afx_msg void OnEventR();
	afx_msg void OnEventU();
	afx_msg void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void DisableEvent(LPCSTR szEvent);
	CWizardTableScript* m_pWizard;
	CStringArray	m_listSelColumn;	
	CString			m_strTableName;
	CString			m_strEvent;
	CString			m_strClitableName;

	CMainFrame *m_pMainFrame;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PANELSCRIPT_H__9BD3ED93_9478_4A4D_A7D9_8B8F18419889__INCLUDED_)
