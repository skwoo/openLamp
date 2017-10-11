/* 
   This file is part of openML, mobile and embedded DBMS.

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

// QueryTestDlg.h : header file
//

#if !defined(AFX_QUERYTESTDLG_H__E9B40862_D106_4DB6_833B_65F6C1125DF6__INCLUDED_)
#define AFX_QUERYTESTDLG_H__E9B40862_D106_4DB6_833B_65F6C1125DF6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "QueryTest.h"
/////////////////////////////////////////////////////////////////////////////
// CQueryTestDlg dialog

class CQueryTestDlg : public CDialog
{
// Construction
public:
	int SelectProcess(char* _cquery);
	CQueryTestDlg(CWnd* pParent = NULL);	// standard constructor

	CArray<CString, CString&> m_FetchRow;	//select 쿼리 수행한 결과 데이터 저장변수
	int			m_nFetchSize;
	int			m_nField;
	int			m_nSel;
	iSQL_RES 	*res;
	iSQL_ROW	row;
// Dialog Data
	//{{AFX_DATA(CQueryTestDlg)
	enum { IDD = IDD_QUERYTEST_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQueryTestDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CQueryTestDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnButtonExit();
	afx_msg void OnButtonExe();
	afx_msg void OnButtonConnect();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QUERYTESTDLG_H__E9B40862_D106_4DB6_833B_65F6C1125DF6__INCLUDED_)
