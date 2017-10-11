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

// ModifyTableDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Admin.h"
#include "ModifyTableDlg.h"
#include "MainFrm.h"
#include "RightListView.h"
#include "SQLDirect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CModifyTableDlg dialog


CModifyTableDlg::CModifyTableDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CModifyTableDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CModifyTableDlg)
	m_strCliTabName = _T("");
	m_strSvrTabName = _T("");
	//}}AFX_DATA_INIT
}


void CModifyTableDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModifyTableDlg)
	DDX_Control(pDX, IDC_SVRTABNAME, m_ctlSvrTabName);
	DDX_Text(pDX, IDC_CLITABNAME, m_strCliTabName);
	DDV_MaxChars(pDX, m_strCliTabName, 30);
	DDX_Text(pDX, IDC_SVRTABNAME, m_strSvrTabName);
	DDV_MaxChars(pDX, m_strSvrTabName, 30);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CModifyTableDlg, CDialog)
	//{{AFX_MSG_MAP(CModifyTableDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModifyTableDlg message handlers

BOOL CModifyTableDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_ctlSvrTabName.SetReadOnly();	

	UpdateData(FALSE);
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CModifyTableDlg::SetValue(CString strTableName, CString strTableID)
{
	m_strSvrTabName = strTableName;
	m_strTableID = strTableID;
}

void CModifyTableDlg::OnOK() 
{
	// TODO: Add extra validation here
	UpdateData(TRUE);

	CMainFrame *pMainFrame = (CMainFrame *)AfxGetMainWnd(); 
	CSQLDirect aSD;

	if(aSD.ConnectToDB() == FALSE)
	{
		AfxMessageBox("저장에 실패하였습니다");
		return;
	}

	char szQuery[QUERY_MAX_LEN];
	sprintf(szQuery, 
        "UPDATE openMSYNC_Table SET CliTableName = '%s' WHERE TableID = %s",
		m_strCliTabName, m_strTableID);
	if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)
	{
		AfxMessageBox("저장에 실패하였습니다");
		return;
	}	
	CDialog::OnOK();
}
