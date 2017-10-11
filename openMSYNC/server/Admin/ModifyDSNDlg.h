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

#if !defined(AFX_MODIFYDSNDLG_H__5B7AA82C_5269_4A4D_8DD4_1C19ABA58253__INCLUDED_)
#define AFX_MODIFYDSNDLG_H__5B7AA82C_5269_4A4D_8DD4_1C19ABA58253__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ModifyDSNDlg.h : header file
//
/////////////////////////////////////////////////////////////////////////////
// CModifyDSNDlg dialog

class CModifyDSNDlg : public CDialog
{
// Construction
public:
	void SetValue(int nIndex, CString strDSN, CString strDBID, int nAct);
	CModifyDSNDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CModifyDSNDlg)
	enum { IDD = IDD_MODIFY_DSNDLG };
	CComboBox	m_ctlDSNName;
	CString	m_strCliDSN;
	CString	m_strDSNName;
	CString	m_strDSNPwd;
	CString	m_strDSNUid;
	CString	m_strBeforeDSNName;
	CString m_strDBID;
	int	m_nIndex;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModifyDSNDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	int		m_nAction;
	// Generated message map functions
	//{{AFX_MSG(CModifyDSNDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MODIFYDSNDLG_H__5B7AA82C_5269_4A4D_8DD4_1C19ABA58253__INCLUDED_)
