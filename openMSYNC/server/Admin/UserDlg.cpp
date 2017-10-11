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

// UserDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Admin.h"
#include "UserDlg.h"
#include "MainFrm.h"
#include "RightListView.h"
#include "SQLDirect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUserDlg dialog


CUserDlg::CUserDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUserDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CUserDlg)
	m_strCPwd = _T("");
	m_strName = _T("");
	m_strPwd = _T("");
	m_strUserID = _T("");
	//}}AFX_DATA_INIT
}


void CUserDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUserDlg)
	DDX_Control(pDX, IDC_PWD, m_ctlPwd);
	DDX_Control(pDX, IDC_INIT, m_ctrlSyncTime);
	DDX_Control(pDX, IDC_USERID, m_ctlUserID);
	DDX_Text(pDX, IDC_CPWD, m_strCPwd);
	DDV_MaxChars(pDX, m_strCPwd, 20);
	DDX_Text(pDX, IDC_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 20);
	DDX_Text(pDX, IDC_PWD, m_strPwd);
	DDV_MaxChars(pDX, m_strPwd, 20);
	DDX_Text(pDX, IDC_USERID, m_strUserID);
	DDV_MaxChars(pDX, m_strUserID, 20);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUserDlg, CDialog)
	//{{AFX_MSG_MAP(CUserDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUserDlg message handlers

void CUserDlg::OnOK() 
{
	// TODO: Add extra validation here
	
	UpdateData(TRUE);

	if(m_strUserID == "")
	{
		AfxMessageBox("사용자 ID를 입력하여 주십시오.");
		m_ctlUserID.SetFocus();
		return;
	}
	if(m_strPwd == "")
	{
		AfxMessageBox("패스워드를 입력하여 주십시오.");
		m_ctlPwd.SetFocus();
		return;
	}
	if(m_strPwd != m_strCPwd)
	{
		AfxMessageBox("패스워드가 일치하지 않습니다.");
		m_ctlPwd.SetFocus();
		return;
	}
	
	CSQLDirect aSD;
	char szQuery[QUERY_MAX_LEN];
	BOOL dbSync;

    dbSync =  FALSE;

	if(aSD.ConnectToDB() == FALSE) return ;

	if(m_nState == ID_NEW_USER)
	{	
		if(m_pMainFrame->m_nDBType == MYSQL || m_pMainFrame->m_nDBType == CUBRID)
			sprintf(szQuery, "INSERT INTO openMSYNC_User "
            "VALUES('%s', '%s', '%s', '0', '1970-01-01 10:00:00' )",
            m_strUserID, m_strName, m_strPwd);
		else
			sprintf(szQuery, "INSERT INTO openMSYNC_User "
                "VALUES('%s', '%s', '%s', '0', '1900-01-01')",
				m_strUserID, m_strName, m_strPwd);

		if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)  
            return;
	}
	else
	{
		if((((CButton *)GetDlgItem(IDC_DBSYNC))->GetState()&0x0003))
			dbSync = TRUE;

		CString strQuery1, strQuery2;
		strQuery1.Format("UPDATE openMSYNC_User Set UserName = '%s', UserPwd = '%s' ", 
        m_strName, m_strPwd);

		if(m_pMainFrame->m_nDBType == MYSQL || m_pMainFrame->m_nDBType == CUBRID)
        {
			if(dbSync)
				strQuery1 = strQuery1 + ", DBSyncTime = '1970-01-01 10:00:00' ";
		}
		else{
			if(dbSync)
				strQuery1 = strQuery1 + ", DBSyncTime = '1900-01-01' ";
		}
	
		sprintf(szQuery, 
            "%s WHERE UserID = '%s'", strQuery1.GetBuffer(0), m_strUserID);
		if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)  return;
	}	
	if (m_nAction == ADD_LIST)	// User Profile을 선택한 상태에서 New한 경우
	{
		char *szColValue[3];
		CString date;
		if(m_pMainFrame->m_nDBType == MYSQL || m_pMainFrame->m_nDBType == CUBRID)
		{
			date = "1970-01-01 10:00:00.000";
		}
		else
		{
			date = "1900-01-01 00:00:00.000";
		}

		szColValue[0]=new char[m_strUserID.GetLength()+1];
		szColValue[1]=new char[m_strName.GetLength()+1];
		szColValue[2]=new char[date.GetLength()+1];		
		strcpy(szColValue[0], m_strUserID.GetBuffer(0));
		strcpy(szColValue[1], m_strName.GetBuffer(0));
		strcpy(szColValue[2], date.GetBuffer(0));		
		m_pMainFrame->m_pRightListView->AddStringToList(szColValue);
		delete szColValue[0];
		delete szColValue[1];
		delete szColValue[2];		
	}
	else if(m_nAction == MODIFY_LIST)	//Modify
	{
		// UserID, UserName, dbSyncDate에서 UserID는 변경 불가니까 1,2,3 값 수정
		int row = m_pMainFrame->m_pRightListView->FindIndex(m_strUserID.GetBuffer(0));
		m_pMainFrame->m_pRightListView->ModifyColValue(row, 1, m_strName);

        if(dbSync)
        {
            if(m_pMainFrame->m_nDBType == MYSQL 
                || m_pMainFrame->m_nDBType == CUBRID )
            {
                m_pMainFrame->m_pRightListView->ModifyColValue(
                    row, 2, "1970-01-01 10:00:00.000");
            }
            else
            {
                m_pMainFrame->m_pRightListView->ModifyColValue(
                    row, 2, "1900-01-01 00:00:00.000");
            }
        }
	}

	UpdateData(FALSE);

	CDialog::OnOK();
}

BOOL CUserDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// 메인프레임 포인트 얻어서 m_pMainTreeView를 가져온다
	m_pMainFrame = (CMainFrame *)AfxGetMainWnd(); 

	if (m_nState == ID_MODIFY_USER)
	{
		// UserID edit는 disable..
		// check box는 enable하고 title 이름 바꾸기
		m_ctlUserID.SetReadOnly();
		((CButton *)GetDlgItem(IDC_INIT))->ShowWindow(SW_SHOW);
		((CButton *)GetDlgItem(IDC_DBSYNC))->ShowWindow(SW_SHOW);
		((CStatic *)GetDlgItem(IDC_USERINFO))->SetWindowText(" User 정보 수정 ");

		CSQLDirect	aSD;
		if(aSD.ConnectToDB() == FALSE) return FALSE;

		char szQuery[QUERY_MAX_LEN];
		sprintf(szQuery, 
            "SELECT UserPwd, UserName FROM openMSYNC_User WHERE UserID = '%s'",
            m_strUserID);

		if(aSD.ExecuteSQL(szQuery) == SQL_SUCCESS)
		{
			while(aSD.FetchSQL() == SQL_SUCCESS)
			{
				m_strPwd = aSD.GetColumnValue(1);
				m_strName = aSD.GetColumnValue(2);
			}
				
		}
		UpdateData(FALSE);
	}
	else
	{
		((CButton *)GetDlgItem(IDC_INIT))->ShowWindow(SW_HIDE);
		((CButton *)GetDlgItem(IDC_DBSYNC))->ShowWindow(SW_HIDE);
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CUserDlg::SetState(int nID, int nAct)
{
	m_nState = nID;
	m_nAction = nAct;
}
