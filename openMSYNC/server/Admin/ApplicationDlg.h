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

#if !defined(AFX_APPLICATIONDLG_H__E499C515_0E7A_46B5_AEDD_515A38C7A551__INCLUDED_)
#define AFX_APPLICATIONDLG_H__E499C515_0E7A_46B5_AEDD_515A38C7A551__INCLUDED_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ApplicationDlg.h : header file
//
/////////////////////////////////////////////////////////////////////////////
// CApplicationDlg dialog
class CSQLDirect;

class CApplicationDlg : public CDialog
{
// Construction
public:
	void SetValue(int nIndex, CString strVerID, CString strAppVer, int nAct);
	CApplicationDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CApplicationDlg)
	enum { IDD = IDD_APPDLG };
	CEdit	m_ctlVersion;
	CEdit	m_ctlAppName;
	CString	m_strAppName;
	CString m_strAppVerID;
	CString m_strBeforeAppVer;
	int	m_nIndex;
	CString	m_strVersion;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CApplicationDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	int		m_nAction;
	// Generated message map functions
	//{{AFX_MSG(CApplicationDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	BOOL InsertTableScript(CSQLDirect *aSD1, CSQLDirect *aSD2, CString strDBID, CString strSvrTableName, CString strCliTableName, CString strTableID);
	BOOL InsertDSNTableScript(CSQLDirect *aSD1, CSQLDirect *aSD2, CSQLDirect *aSD3, CString strNewVersionID, CString strDSNName, CString strDSNUid, CString strDSNPwd, CString strCliDSN, CString strDBID);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


#endif // !defined(AFX_APPLICATIONDLG_H__E499C515_0E7A_46B5_AEDD_515A38C7A551__INCLUDED_)
