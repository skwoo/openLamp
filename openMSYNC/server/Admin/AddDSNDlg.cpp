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

// AddDSNDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Admin.h"
#include "AddDSNDlg.h"
#include "MainFrm.h"
#include "LeftTreeView.h"
#include "RightListView.h"
#include "SQLDirect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddDSNDlg dialog


CAddDSNDlg::CAddDSNDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddDSNDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddDSNDlg)
	m_strAppName = _T("");
	m_strCliDSN = _T("");
	m_strDSNName = _T("");
	m_strDSNPwd = _T("");
	m_strDSNUid = _T("");
	m_strVersion = _T("");
	//}}AFX_DATA_INIT

    m_bNewApp = FALSE;
    m_nAction = NO_ACTION;
}


void CAddDSNDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddDSNDlg)
	DDX_Control(pDX, IDC_DSNUID, m_ctlDSNUid);
	DDX_Control(pDX, IDC_DSNPWD, m_ctlDSNPwd);
	DDX_Control(pDX, IDC_CLIDSN, m_ctlCliDSN);
	DDX_Control(pDX, IDC_APPVER, m_ctlVersion);
	DDX_Control(pDX, IDC_APPNAME, m_ctlAppName);
	DDX_Control(pDX, IDC_DSNNAME, m_ctlDSNName);
	DDX_Text(pDX, IDC_APPNAME, m_strAppName);
	DDV_MaxChars(pDX, m_strAppName, 30);
	DDX_Text(pDX, IDC_CLIDSN, m_strCliDSN);
	DDV_MaxChars(pDX, m_strCliDSN, 30);
	DDX_CBString(pDX, IDC_DSNNAME, m_strDSNName);
	DDV_MaxChars(pDX, m_strDSNName, 30);
	DDX_Text(pDX, IDC_DSNPWD, m_strDSNPwd);
	DDV_MaxChars(pDX, m_strDSNPwd, 30);
	DDX_Text(pDX, IDC_DSNUID, m_strDSNUid);
	DDV_MaxChars(pDX, m_strDSNUid, 30);
	DDX_Text(pDX, IDC_APPVER, m_strVersion);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddDSNDlg, CDialog)
	//{{AFX_MSG_MAP(CAddDSNDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CAddDSNDlg::Check_Data()
{
    if( m_bNewApp )
    {
        if(m_strAppName == "")
        {
            AfxMessageBox("Application 이름을 입력하여 주십시오.");
            m_ctlAppName.SetFocus();            
        }
        else if(m_strVersion == "")
        {
            AfxMessageBox("Version을 입력하여 주십시오.");
            m_ctlVersion.SetFocus();            
        }
        else if(m_strVersion.Find(".")>0 
               || atoi(m_strVersion.GetBuffer(0)) < 1 
               || atoi(m_strVersion.GetBuffer(0)) > 50  )
        {
            AfxMessageBox("Version의 값은 1부터 50 사이의 정수 값입니다.");
            m_ctlVersion.SetFocus();            
        }
        else if(m_strDSNName == "" || m_strDSNUid == "" 
                || m_strDSNPwd == "" || m_strCliDSN == "")
        {   
            if(AfxMessageBox("입력하신 어플리케이션 '" + m_strAppName 
                + "' 의 script 설정은 나중에 하시겠습니까?", 
                MB_YESNO|MB_ICONSTOP)==IDYES)
            {
                return TRUE;
            }
            
            m_ctlDSNName.SetFocus();                		    
        }		
        else
        {
            return TRUE;
        }
    }
    else
    {
        if(m_strDSNName == "")
        {
            AfxMessageBox("DSN을 선택하여 주십시오.");
            m_ctlDSNName.SetFocus();		    
        }
        else if(m_strDSNUid == "")
        {
            AfxMessageBox("DSN의 사용자 ID를 입력하여 주십시오.");
            m_ctlDSNUid.SetFocus();		    
        }
        else if(m_strDSNPwd == "")
        {
            AfxMessageBox("DSN의 사용자 패스워드를 입력하여 주십시오.");
            m_ctlDSNPwd.SetFocus();		    
        }
        else if(m_strCliDSN == "")
        {
            AfxMessageBox("단말쪽에서 인식할 DSN 값을 입력하여 주십시오.");
            m_ctlCliDSN.SetFocus();		    
        }
        else
            return TRUE;
    }
    
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CAddDSNDlg message handlers
void CAddDSNDlg::OnOK() 
{
    // TODO: Add extra validation here
    UpdateData(TRUE);
    if( !Check_Data() )
        return;
    
    CSQLDirect aSD;
    BOOL isDSNInserted;
    char szQuery[QUERY_MAX_LEN];
    CString	  strVersionID = "", strDBID = "", strAppVer;
    
    isDSNInserted = FALSE;
    if(aSD.ConnectToDB() == FALSE) 
        goto ret_error1;
    
    if( m_bNewApp )
    {   // openMSYNC_Version의 MAX VersionID 구하기
        sprintf(szQuery, "SELECT max(VersionID)+1 FROM openMSYNC_Version");
        if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS) 
            goto ret_error2;
        if(aSD.FetchSQL() != SQL_SUCCESS)  
            goto ret_error2;
        strVersionID = aSD.GetColumnValue(1);
        if(strVersionID.GetLength() == 0) strVersionID = "1";
        
        // openMSYNC_Version에 insert
        sprintf(szQuery, "INSERT INTO openMSYNC_Version VALUES(%s, '%s', %s)",
            strVersionID, m_strAppName, m_strVersion);
        if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)  
            goto ret_error2;
        
    }
    else
    {
        sprintf(szQuery, "SELECT VersionID FROM openMSYNC_Version "
                         "WHERE Application = '%s' AND Version = %s",
                m_strAppName, m_strVersion);
        if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS) 
            goto ret_error2;
        if(aSD.FetchSQL() != SQL_SUCCESS)  
            goto ret_error2;
        strVersionID = aSD.GetColumnValue(1);        
    }
    
    if( !m_bNewApp || 
        (m_strDSNName != "" && m_strDSNUid != "" 
         && m_strDSNPwd != "" && m_strCliDSN != "") )
    {
        // openMSYNC_DSN의 MAX DBID 구하기
        sprintf(szQuery, "SELECT Max(DBID)+1 FROM openMSYNC_DSN");
        if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)
        {			
            goto ret_error2;;
        }
        if(aSD.FetchSQL() != SQL_SUCCESS)
        {		
            goto ret_error2;
        }
        strDBID = aSD.GetColumnValue(1);
        if(strDBID.GetLength() == 0) strDBID = "1";
        // openMSYNC_DSN에 insert
        sprintf(szQuery, "INSERT INTO openMSYNC_DSN VALUES(%s, %s, '%s', '%s', '%s', '%s')",
                strDBID, strVersionID, m_strDSNName, 
                m_strDSNUid, m_strDSNPwd, m_strCliDSN);
        if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)
        {			
            goto ret_error2;
        }
        isDSNInserted = TRUE;
    }
    
    aSD.Commit();
    
    if( m_bNewApp )
    {
        // 왼쪽 TreeView에 추가
        strAppVer.Format("%s.%s", m_strAppName, m_strVersion); // Application.Version
        // Application.Version 추가
        HTREEITEM	hDSNParent, hTreeScriptParent;
        
        hTreeScriptParent = m_pMainFrame->m_pLeftTreeView->GetTreeScriptParent();
        hDSNParent = m_pMainFrame->m_pLeftTreeView->AddItemToTree( 
            hTreeScriptParent, strAppVer, m_pMainFrame->m_pLeftTreeView->n_TreeApp);
        
        if(isDSNInserted)
        {
            m_pMainFrame->m_pLeftTreeView->AddItemToTree(hDSNParent, 
                m_strDSNName, m_pMainFrame->m_pLeftTreeView->n_TreeDB);
        }
        
        if(m_nAction == ADD_LIST)  // 아니면 NO_ACTION
        {
            // 오른쪽 ListView에 추가
            char *szColValue[2];
            szColValue[0] = new char[strAppVer.GetLength()+1];
            strcpy(szColValue[0], strAppVer.GetBuffer(0));		// strAppVer
            szColValue[1] = new char[strVersionID.GetLength()+1];
            strcpy(szColValue[1], strVersionID.GetBuffer(0));	// VersionID
            m_pMainFrame->m_pRightListView->AddStringToList(szColValue);
            for(int i=0 ; i<2 ; i++)			
                delete szColValue[i];
        }	    
    }
    else
    {
        // 왼쪽 Tree에 추가
        HTREEITEM	hDSNParent;
        hDSNParent = 
            m_pMainFrame->m_pLeftTreeView->GetTreeCtrl().GetSelectedItem();
        m_pMainFrame->m_pLeftTreeView->AddItemToTree(hDSNParent, m_strDSNName, 
                                    m_pMainFrame->m_pLeftTreeView->n_TreeDB);
        
       	// 오른쪽 ListView에 추가
        char *szColValue[2];
        szColValue[0] = new char[m_strDSNName.GetLength()+1];
        strcpy(szColValue[0], m_strDSNName.GetBuffer(0));	// DSNName
        szColValue[1] = new char[strDBID.GetLength()+1];
        strcpy(szColValue[1], strDBID.GetBuffer(0));		// DBID
        m_pMainFrame->m_pRightListView->AddStringToList(szColValue);
        
        for(int i=0 ; i<2 ; i++)			
            delete szColValue[i];
    }
    
    UpdateData(FALSE);	
    CDialog::OnOK();
    return;
    
ret_error2:
    aSD.RollBack();
    
ret_error1:    
    AfxMessageBox("DB 처리 작업에서 오류가 발생했습니다.");
}


BOOL CAddDSNDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();
    
    // TODO: Add extra initialization here
    m_pMainFrame =(CMainFrame *)AfxGetMainWnd();
    m_pMainFrame->ShowSystemDSN(&m_ctlDSNName);	
    
    if( m_bNewApp )
    {        
        ((CStatic *)GetDlgItem(IDC_APPINFOADD))->ShowWindow(SW_HIDE);
        ((CStatic *)GetDlgItem(IDC_APPINFONEW))->ShowWindow(SW_SHOW);
        ((CStatic *)GetDlgItem(IDOK))->ShowWindow(SW_SHOW);
        ((CStatic *)GetDlgItem(IDCANCEL))->ShowWindow(SW_SHOW);
        
        m_ctlAppName.SetFocus();
        UpdateData(FALSE);
    }
    else
    {        
        // applicationName은 disable하고 title 이름 바꾸기
        m_ctlAppName.SetReadOnly();
        m_ctlVersion.SetReadOnly();
        ((CStatic *)GetDlgItem(IDC_APPINFOADD))->ShowWindow(SW_SHOW);
        ((CStatic *)GetDlgItem(IDC_APPINFONEW))->ShowWindow(SW_HIDE);
        ((CStatic *)GetDlgItem(IDOK))->ShowWindow(SW_SHOW);
        ((CStatic *)GetDlgItem(IDCANCEL))->ShowWindow(SW_SHOW);
        
        m_ctlDSNName.SetFocus();	
        UpdateData(FALSE);
    }
    
    
    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
