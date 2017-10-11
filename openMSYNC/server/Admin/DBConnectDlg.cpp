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

// DBConnectDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Admin.h"
#include "DBConnectDlg.h"
#include "odbcinst.h"	// SQLCreateDataSource()
#include "MainFrm.h"
#include "LeftTreeView.h"
#include "SQLDirect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDBConnectDlg dialog


CDBConnectDlg::CDBConnectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDBConnectDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDBConnectDlg)
	m_strDSNName = _T("");
	m_strPWD = _T("");
	m_strLoginID = _T("");
	//}}AFX_DATA_INIT
}


void CDBConnectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDBConnectDlg)
	DDX_Control(pDX, IDC_DSNNAME, m_ctrlDSNName);
	DDX_CBString(pDX, IDC_DSNNAME, m_strDSNName);
	DDX_Text(pDX, IDC_PWD, m_strPWD);
	DDX_Text(pDX, IDC_LOGINID, m_strLoginID);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDBConnectDlg, CDialog)
	//{{AFX_MSG_MAP(CDBConnectDlg)
	ON_BN_CLICKED(IDC_CREATE, OnCreateDsn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDBConnectDlg message handlers
void CDBConnectDlg::OnOK()
{
	UpdateData(TRUE);

	if(m_strDSNName == "")
	{
		AfxMessageBox("Data Source Name을 입력해 주세요");
		CWnd *pWnd = GetDlgItem(IDC_DSNNAME);
		pWnd->SetFocus();
		return ;
	}
	WriteProfileString("INERVIT_CONFIG", "openMSync_DSNName", m_strDSNName);
	WriteProfileString("INERVIT_CONFIG", "openMSync_UID", m_strLoginID);
	WriteProfileString("INERVIT_CONFIG", "openMSync_PWD", m_strPWD);
	m_pMainFrame->GetDBServerType(m_strDSNName);
	m_pMainFrame->m_strDSNName = m_strDSNName;
	m_pMainFrame->m_strLoginID = m_strLoginID;
	m_pMainFrame->m_strPWD = m_strPWD;
	m_pMainFrame->m_pLeftTreeView->InitialTree();
	
	CDialog::OnOK();
}
BOOL CDBConnectDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	m_pMainFrame=(CMainFrame *)AfxGetMainWnd();

	UpdateData(TRUE);
	char cCfg[101];
	int  nSize = 100;

	GetProfileString("INERVIT_CONFIG", "openMSync_DSNName", "", cCfg, nSize);
	m_strDSNName = cCfg;
	GetProfileString("INERVIT_CONFIG", "openMSync_UID", "", cCfg, nSize);
	m_strLoginID = cCfg;
	GetProfileString("INERVIT_CONFIG", "openMSync_PWD", "", cCfg, nSize);
	m_strPWD = cCfg;
	/* system DSN 내용 보여주는 부분 */
 	m_pMainFrame->ShowSystemDSN(&m_ctrlDSNName);

	UpdateData(FALSE);
	return TRUE;
}
void CDBConnectDlg::OnCancel(){
	CDialog::OnCancel();
}
void CDBConnectDlg::OnCreateDsn() 
{
	// TODO: Add your control notification handler code here
	HWND hWnd = this->m_hWnd;

	if (SQLCreateDataSource(hWnd, NULL)==TRUE)
		MessageBox("DSN이 성공적으로 생성되었습니다.");
}
