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

// PanelScript.cpp : implementation file
//

#include "stdafx.h"
#include "Admin.h"
#include "PanelScript.h"
#include "WizardTableScript.h"
#include "SQLDirect.h"
#include "MainFrm.h"
#include "RightListView.h"
#include "LeftTreeView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPanelScript property page

IMPLEMENT_DYNCREATE(CPanelScript, CPropertyPage)

CPanelScript::CPanelScript() : CPropertyPage(CPanelScript::IDD)
{
	//{{AFX_DATA_INIT(CPanelScript)
	m_strScript = _T("");
	//}}AFX_DATA_INIT
}

CPanelScript::~CPanelScript()
{
}

void CPanelScript::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPanelScript)
	DDX_Control(pDX, IDC_TABCOLUMNTREE, m_ctlTabColumnTree);
	DDX_Text(pDX, IDC_SCRIPT, m_strScript);
	DDV_MaxChars(pDX, m_strScript, 4000);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPanelScript, CPropertyPage)
	//{{AFX_MSG_MAP(CPanelScript)
	ON_BN_CLICKED(IDC_EVENT_D, OnEventD)
	ON_BN_CLICKED(IDC_EVENT_F, OnEventF)
	ON_BN_CLICKED(IDC_EVENT_I, OnEventI)
	ON_BN_CLICKED(IDC_EVENT_R, OnEventR)
	ON_BN_CLICKED(IDC_EVENT_U, OnEventU)
	ON_BN_CLICKED(IDOK, OnOK)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPanelScript message handlers

BOOL CPanelScript::OnSetActive() 
{
	// TODO: Add your specialized code here and/or call the base class
	CPropertySheet* pSheet = static_cast<CPropertySheet*>( GetParent() );
    pSheet->SetWizardButtons ( PSWIZB_BACK | PSWIZB_FINISH );	

	m_pWizard = (CWizardTableScript *)GetParent();
	
	if((const unsigned int)m_pWizard->m_chkListSelColumn == 0xcccccccc)
		return FALSE;
	/*~2005. 11. 10. LEE. BEOM. HEE. */
	m_strTableName = m_pWizard->m_strTableName;

	((CStatic *)GetDlgItem(IDC_APPVERNAME))->SetWindowText(
        m_pWizard->m_strApp+"."+m_pWizard->m_strVer);
	((CStatic *)GetDlgItem(IDC_DSNNAME))->SetWindowText(
        m_pWizard->m_strDSNName);
	
	int nCount = m_pWizard->m_chkListSelColumn->GetCount();
	CString strColumn;
	
	m_listSelColumn.RemoveAll();
	m_strScript = _T("");
	m_ctlTabColumnTree.DeleteAllItems();

	HTREEITEM  hItem = m_ctlTabColumnTree.InsertItem(m_strTableName);

	for(int i=0 ; i<nCount ; i++)
	{
		if(m_pWizard->m_chkListSelColumn->GetCheck(i)==1)
		{
			m_pWizard->m_chkListSelColumn->GetText(i, strColumn);
			m_listSelColumn.Add(strColumn);
			m_ctlTabColumnTree.InsertItem(strColumn, hItem);
		}
	}
	DisableExistScript();
	return CPropertyPage::OnSetActive();
}

BOOL CPanelScript::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	m_pMainFrame=(CMainFrame *)AfxGetMainWnd(); 
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPanelScript::OnEventD() 
{
	// TODO: Add your control notification handler code here
	CString strCol,strI;
	
	int nCount = m_listSelColumn.GetSize();	
	for(int i=0; i<nCount; i++)
	{
#ifdef WINDOWS
		strCol = strCol + m_listSelColumn.GetAt(i) + " = ?";
#else
		strI.Format("%d", i);
		strCol = strCol + m_listSelColumn.GetAt(i) + " = :a" + strI;
#endif
		if(i != nCount-1)
		{
			strCol = strCol + " AND ";
		}
	}
	m_strScript.Format("DELETE FROM %s WHERE %s",m_strTableName, strCol);
	m_strEvent = "D";
	UpdateData(FALSE);
}

void CPanelScript::OnEventF() 
{
	// TODO: Add your control notification handler code here
	CString strCol;

	int nCount = m_listSelColumn.GetSize();	
	for(int i=0; i<nCount; i++)
	{
		strCol = strCol + m_listSelColumn.GetAt(i);
		if(i != nCount-1)
		{
			strCol = strCol + ",";
		}
	}
	m_strScript.Format("SELECT %s FROM %s", strCol, m_strTableName);
	m_strEvent = "F";
	UpdateData(FALSE);	
}

void CPanelScript::OnEventI() 
{
	// TODO: Add your control notification handler code here
	CString strCol,strVal, strI;

	int nCount = m_listSelColumn.GetSize();	
	for(int i=0; i<nCount; i++)
	{
		strCol = strCol + m_listSelColumn.GetAt(i);
#ifdef WINDOWS
		strVal = strVal + "?";
#else
		strI.Format("%d", i);
		strVal = strVal + ":a" + strI;
#endif
		if(i != nCount-1)
		{
			strCol = strCol + ",";
			strVal = strVal + ",";
		}
	}
	m_strScript.Format("INSERT INTO %s (%s) VALUES (%s)", 
        m_strTableName, strCol, strVal);
	m_strEvent = "I";
	UpdateData(FALSE);	
}

void CPanelScript::OnEventR() 
{
	// TODO: Add your control notification handler code here
	CString strCol;

	int nCount = m_listSelColumn.GetSize();	
	for(int i=0; i<nCount; i++)
	{
		strCol = strCol + m_listSelColumn.GetAt(i);
		if(i != nCount-1)
		{
			strCol = strCol + ",";
		}
	}
	m_strScript.Format("SELECT %s FROM %s", strCol, m_strTableName);
	m_strEvent = "R";
	UpdateData(FALSE);		
}

void CPanelScript::OnEventU() 
{
	// TODO: Add your control notification handler code here
	CString strCol, strI;
	
	int nCount = m_listSelColumn.GetSize();	
	for(int i=0; i<nCount; i++)
	{
#ifdef WINDOWS
		strCol = strCol + m_listSelColumn.GetAt(i) + " = ?";
#else
		strI.Format("%d", i);
		strCol = strCol + m_listSelColumn.GetAt(i) + " = :a" + strI;
#endif
		if(i != nCount-1)
		{
			strCol = strCol + " , ";
		}
	}
	m_strScript.Format("UPDATE %s SET %s WHERE ",m_strTableName, strCol);	
	m_strEvent = "U";
	UpdateData(FALSE);	
}

void CPanelScript::DisableExistScript()
{
	CSQLDirect aSD;
	char szQuery[QUERY_MAX_LEN];
	CString strApp, strVer;

	if(aSD.ConnectToDB() == FALSE) return;

	InitializeEvent();

	sprintf(szQuery, "SELECT Event FROM openMSYNC_Script s, openMSYNC_Table t, "
        "openMSYNC_DSN d, openMSYNC_Version v WHERE s.TableID = t.TableID "
        "AND t.DBID = d.DBID AND d.VersionID = v.VersionID "
        "AND v.Application = '%s' and v.Version = %s AND d.SvrDSN = '%s' "
        "AND t.SvrTableName = '%s'", 
		m_pWizard->m_strApp, m_pWizard->m_strVer, 
        m_pWizard->m_strDSNName, m_strTableName);
	if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)
	{
		AfxMessageBox("기존의 script 정보를 읽는 데에 실패하였습니다");
		return;
	}
	while(aSD.FetchSQL() == SQL_SUCCESS)
	{
		DisableEvent(aSD.GetColumnValue(1));
	}
}

void CPanelScript::OnOK() 
{
	// TODO: Add your control notification handler code here
	CSQLDirect aSD;
	char szQuery[QUERY_MAX_LEN];
	CString strTableID = "";
	BOOL	bTableInsert = FALSE;
	CString strScript;

	UpdateData(TRUE);
	if(aSD.ConnectToDB() == FALSE) return;

	sprintf(szQuery, "SELECT TableID FROM openMSYNC_Table t, openMSYNC_DSN d, "
        "openMSYNC_Version v WHERE t.DBID = d.DBID AND d.VersionID = v.VersionID "
        "AND v.Application = '%s' and v.Version = %s AND d.SvrDSN = '%s' "
        "AND t.SvrTableName = '%s'", 
		m_pWizard->m_strApp, m_pWizard->m_strVer, 
        m_pWizard->m_strDSNName, m_strTableName);
	if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)
	{
		AfxMessageBox("script 정보를 저장하는 데에 실패하였습니다");
		return;
	}
	while(aSD.FetchSQL() == SQL_SUCCESS)
	{
		strTableID = aSD.GetColumnValue(1);
	}
    if(strTableID.GetLength() == 0)	// table의 script를 처음 작성하는 경우
	{
		bTableInsert = TRUE;
		// openMSYNC_Table의 MAX TableID 구하기
		sprintf(szQuery, "SELECT max(TableID)+1 FROM openMSYNC_Table");
		if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS) return;
		if(aSD.FetchSQL() != SQL_SUCCESS)  return;
		strTableID = aSD.GetColumnValue(1);
		if(strTableID.GetLength() == 0) strTableID = "1";

		sprintf(szQuery, "SELECT DBID FROM openMSYNC_DSN d, openMSYNC_Version v "
            "WHERE d.VersionID = v.VersionID AND v.Application = '%s' "
            "and v.Version = %s AND d.SvrDSN = '%s'", 
            m_pWizard->m_strApp, m_pWizard->m_strVer, m_pWizard->m_strDSNName);
		if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS) return;
		if(aSD.FetchSQL() != SQL_SUCCESS)  return;
		CString strDBID = aSD.GetColumnValue(1);
		if(strDBID.GetLength() == 0) strDBID = "1";
	
		// openMSYNC_Table에 insert
		sprintf(szQuery, "INSERT INTO openMSYNC_Table VALUES(%s, %s, '%s', '%s')",
				strTableID, strDBID, m_strTableName, m_pWizard->m_strCliTableName);
		if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)  return;	
		
	}
	m_strScript.TrimLeft(_T("\r\n "));
	strScript = m_strScript;
	strScript.Replace("'", "''");  

	if(m_pMainFrame->m_nDBType == SYBASE || m_pMainFrame->m_nDBType == MSSQL)
		sprintf(szQuery, 
            "INSERT INTO openMSYNC_Script VALUES(%s, upper('%s'), '%s')",
			strTableID, m_strEvent, strScript);
	else
		sprintf(szQuery, "INSERT INTO openMSYNC_Script VALUES(%s, '%s', '%s')",
			strTableID, m_strEvent, strScript);
	
	
	if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)
	{
		AfxMessageBox("script 정보를 저장하는 데에 실패하였습니다");
		return;
	}
	DisableEvent(m_strEvent);
	aSD.Commit();
	if(bTableInsert && m_pWizard->m_nState == ID_ADD_TABLE)
	{
		HTREEITEM hDSNParent = 
            m_pMainFrame->m_pLeftTreeView->GetTreeCtrl().GetSelectedItem();
		m_pMainFrame->m_pLeftTreeView->AddItemToTree(
            hDSNParent, m_strTableName, m_pMainFrame->m_pLeftTreeView->n_TreeTable);

		// 오른쪽 ListView에 추가
		char *szColValue[2];

		szColValue[0] = new char[m_strTableName.GetLength()+1];
		strcpy(szColValue[0], m_strTableName.GetBuffer(0));	// strAppVer
		szColValue[1] = new char[strTableID.GetLength()+1];
		strcpy(szColValue[1], strTableID.GetBuffer(0));		// VersionID

		m_pMainFrame->m_pRightListView->AddStringToList(szColValue);
		for(int i=0 ; i<2 ; i++)			
			delete szColValue[i];
	}
	else if(m_pWizard->m_nState == ID_ADD_SCRIPT)
	{
		// 오른쪽 ListView에 추가
		char *szColValue[4];
		CString  strEvent;

		szColValue[0] = new char[strTableID.GetLength()+1];
		strcpy(szColValue[0], strTableID.GetBuffer(0));		// TableID
		szColValue[1] = new char[m_strEvent.GetLength()+1];
		strcpy(szColValue[1], m_strEvent.GetBuffer(0));		// Event : 'U','I', 'D', ..

		strEvent = m_pMainFrame->m_pRightListView->ChangeEventName(m_strEvent);
		szColValue[2] = new char[strEvent.GetLength()+1];
		strcpy(szColValue[2], strEvent.GetBuffer(0));		// str_Event : "Upload_Insert"...
		szColValue[3] = new char[m_strScript.GetLength()+1];
		strcpy(szColValue[3], m_strScript.GetBuffer(0));	// script

		m_pMainFrame->m_pRightListView->AddStringToList(szColValue);
		for(int i=0 ; i<4 ; i++)			
			delete szColValue[i];
	}

	UpdateData(FALSE);
}

void CPanelScript::DisableEvent(LPCSTR szEvent)
{
	switch(szEvent[0])
	{
		case 'U':
			GetDlgItem(IDC_EVENT_U)->EnableWindow(FALSE);
			break;
		case 'I':
			GetDlgItem(IDC_EVENT_I)->EnableWindow(FALSE);
			break;
		case 'D':
			GetDlgItem(IDC_EVENT_D)->EnableWindow(FALSE);
			break;
		case 'F':
			GetDlgItem(IDC_EVENT_F)->EnableWindow(FALSE);
			break;
		case 'R':
			GetDlgItem(IDC_EVENT_R)->EnableWindow(FALSE);
			break;
	}

}

void CPanelScript::InitializeEvent()
{
	CButton *pButton;

	pButton = (CButton *)GetDlgItem(IDC_EVENT_U);
	pButton->EnableWindow(TRUE);
	pButton->SetCheck(0);
	pButton = (CButton *)GetDlgItem(IDC_EVENT_I);
	pButton->EnableWindow(TRUE);
	pButton->SetCheck(0);
	pButton = (CButton *)GetDlgItem(IDC_EVENT_D);
	pButton->EnableWindow(TRUE);
	pButton->SetCheck(0);	
	pButton = (CButton *)GetDlgItem(IDC_EVENT_F);
	pButton->EnableWindow(TRUE);
	pButton->SetCheck(0);
	pButton = (CButton *)GetDlgItem(IDC_EVENT_R);
	pButton->EnableWindow(TRUE);
	pButton->SetCheck(0);
}

