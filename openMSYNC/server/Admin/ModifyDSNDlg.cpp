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

// ModifyDSNDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Admin.h"
#include "ModifyDSNDlg.h"
#include "MainFrm.h"
#include "RightListView.h"
#include "LeftTreeView.h"
#include "SQLDirect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CModifyDSNDlg dialog


CModifyDSNDlg::CModifyDSNDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CModifyDSNDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CModifyDSNDlg)
	m_strCliDSN = _T("");
	m_strDSNName = _T("");
	m_strDSNPwd = _T("");
	m_strDSNUid = _T("");
	//}}AFX_DATA_INIT
}


void CModifyDSNDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModifyDSNDlg)
	DDX_Control(pDX, IDC_DSNNAME, m_ctlDSNName);
	DDX_Text(pDX, IDC_CLIDSN, m_strCliDSN);
	DDV_MaxChars(pDX, m_strCliDSN, 30);
	DDX_CBString(pDX, IDC_DSNNAME, m_strDSNName);
	DDV_MaxChars(pDX, m_strDSNName, 30);
	DDX_Text(pDX, IDC_DSNPWD, m_strDSNPwd);
	DDV_MaxChars(pDX, m_strDSNPwd, 30);
	DDX_Text(pDX, IDC_DSNUID, m_strDSNUid);
	DDV_MaxChars(pDX, m_strDSNUid, 30);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CModifyDSNDlg, CDialog)
	//{{AFX_MSG_MAP(CModifyDSNDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModifyDSNDlg message handlers

BOOL CModifyDSNDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CMainFrame *m_pMainFrame =(CMainFrame *)AfxGetMainWnd();
	// TODO: Add extra initialization here
	m_pMainFrame->ShowSystemDSN(&m_ctlDSNName);	
	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CModifyDSNDlg::SetValue(int nIndex, CString strDSN, CString strDBID, int nAct)
{
	m_nIndex = nIndex;
	m_strBeforeDSNName = strDSN;
	m_strDBID = strDBID;
	m_nAction = nAct;
}

void CModifyDSNDlg::OnOK() 
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
	sprintf(szQuery, "UPDATE openMSYNC_DSN SET SvrDSN = '%s', "
        "DSNUserID = '%s', DSNPwd = '%s', CliDSN = '%s' WHERE DBID = %s", 
		m_strDSNName, m_strDSNUid, m_strDSNPwd, m_strCliDSN, m_strDBID);
	if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)
	{
		AfxMessageBox("저장에 실패하였습니다");
		return;
	}	
	if(m_nAction == MODIFY_RIGHT)
	{
		// DSNName, DBID 니까 0번째
		pMainFrame->m_pRightListView->ModifyColValue(m_nIndex, 0, m_strDSNName);
		pMainFrame->m_pLeftTreeView->ChangeItemText(m_strBeforeDSNName, m_strDSNName);
	}
	else
		pMainFrame->m_pLeftTreeView->ChangeItemText(m_strDSNName);

	CDialog::OnOK();
}
