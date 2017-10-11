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

// PanelTable.cpp : implementation file
//

#include "stdafx.h"
#include "Admin.h"
#include "PanelTable.h"
#include "SQLDirect.h"
#include "WizardTableScript.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPanelTable property page

IMPLEMENT_DYNCREATE(CPanelTable, CPropertyPage)

CPanelTable::CPanelTable() : CPropertyPage(CPanelTable::IDD)
{
	//{{AFX_DATA_INIT(CPanelTable)
	m_bAllColumns = FALSE;
	m_strCliTableName = _T("");
	//}}AFX_DATA_INIT
	m_nSelTableIndex = -1;
	m_nTableCount = -1;
	m_nColumnCount = -1;
}

CPanelTable::~CPanelTable()
{
}

void CPanelTable::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPanelTable)
	DDX_Control(pDX, IDC_ALLCOLUMNS, m_ctlAllColumns);
	DDX_Control(pDX, IDC_TABLELIST, m_ctlTableList);
	DDX_Control(pDX, IDC_COLUMNLIST, m_ctlColumnList);
	DDX_Check(pDX, IDC_ALLCOLUMNS, m_bAllColumns);
	DDX_Text(pDX, IDC_CLITABLENAME, m_strCliTableName);
	DDV_MaxChars(pDX, m_strCliTableName, 30);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPanelTable, CPropertyPage)
	//{{AFX_MSG_MAP(CPanelTable)
	ON_LBN_SELCHANGE(IDC_COLUMNLIST, OnSelchangeColumnlist)
	ON_LBN_SELCHANGE(IDC_TABLELIST, OnSelchangeTablelist)
	ON_BN_CLICKED(IDC_ALLCOLUMNS, OnAllcolumns)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPanelTable message handlers

BOOL CPanelTable::OnSetActive() 
{
	// TODO: Add your specialized code here and/or call the base class
	CPropertySheet* pSheet = static_cast<CPropertySheet*>( GetParent() );
    pSheet->SetWizardButtons ( PSWIZB_NEXT );	
	return CPropertyPage::OnSetActive();
}

BOOL CPanelTable::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	// TODO: Add extra initialization here

	m_pWizard = (CWizardTableScript *)GetParent();
	m_ctlTableList.ResetContent();

	((CStatic *)GetDlgItem(IDC_APPVERNAME))->SetWindowText(
        m_pWizard->m_strApp+"."+m_pWizard->m_strVer);
	((CStatic *)GetDlgItem(IDC_DSNNAME))->SetWindowText(
        m_pWizard->m_strDSNName);

	CSQLDirect aSD;
	CString strTable;

	if(aSD.ConnectToDB(m_pWizard->m_strDSNName, 
        m_pWizard->m_strLoginID, m_pWizard->m_strPWD) == FALSE)
	{
		return FALSE;
	}	
	
	m_nTableCount = aSD.OpenAllTables(m_pWizard->m_strLoginID);
	
	for(int i=0 ; i<m_nTableCount ; i++)
	{
		strTable = aSD.GetTableName(i);
		m_ctlTableList.AddString(strTable);
		if(m_pWizard->m_nState == ID_ADD_SCRIPT 
            && strTable == m_pWizard->m_strTableName)
		{
			m_ctlTableList.SetCheck(i, 1);
			m_nSelTableIndex = i;
			m_strTableName = strTable;
			m_strCliTableName = GetCliTableName(m_strTableName);
			DisplayColumnList(m_strTableName);
		}
	}
	
	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPanelTable::OnSelchangeColumnlist() 
{
	// TODO: Add your control notification handler code here
    UpdateData(TRUE);
	int idx = m_ctlColumnList.GetCurSel();
	if(m_ctlColumnList.GetCheck(idx))
		m_ctlColumnList.SetCheck(idx, 1);
	else
		m_ctlColumnList.SetCheck(idx, 0);
	UpdateData(FALSE);
	m_ctlAllColumns.SetCheck(0);
}

void CPanelTable::OnSelchangeTablelist() 
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);

	int idx = m_ctlTableList.GetCurSel();
	if(idx < 0)
		return;
	// 한개만 선택해야하기때문에 기존에 선택된거는 uncheck
	if(m_nSelTableIndex > -1 && m_nSelTableIndex != idx)
		m_ctlTableList.SetCheck(m_nSelTableIndex, 0);

	m_ctlTableList.SetCheck(idx, 1);

	m_ctlTableList.GetText(idx, m_strTableName);
	m_nSelTableIndex = idx;
	m_strCliTableName = GetCliTableName(m_strTableName);
	DisplayColumnList(m_strTableName);
	UpdateData(FALSE);
	m_ctlAllColumns.SetCheck(1);
}

void CPanelTable::DisplayColumnList(CString strTable)
{
	CSQLDirect aSD;
	if(aSD.ConnectToDB(m_pWizard->m_strDSNName,
        m_pWizard->m_strLoginID, m_pWizard->m_strPWD) == FALSE)
    {
        return;
    }
	m_ctlColumnList.ResetContent();

	m_nColumnCount = aSD.OpenAllColumns(m_pWizard->m_strLoginID, strTable);
	
	for(int i=0 ; i<m_nColumnCount ; i++)
	{
		m_ctlColumnList.AddString(aSD.GetColumnName(i));
		m_ctlColumnList.SetCheck(i, 1);
	}	
	int a = m_ctlAllColumns.GetCheck();
}

void CPanelTable::OnAllcolumns() 
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	for(int i=0 ; i<m_nColumnCount ; i++)
	{
        // AllColumns를 check하면 Set, uncheck면 unset
		m_ctlColumnList.SetCheck(i, m_bAllColumns);	
	}	
}

LRESULT CPanelTable::OnWizardNext() 
{
	// TODO: Add your specialized code here and/or call the base class

	if(GetCheckStatus(m_nTableCount, &m_ctlTableList) == 0)
	{
		AfxMessageBox("테이블을 선택하세요");		
	}
	else if(GetCheckStatus(m_nColumnCount, &m_ctlColumnList) == 0)
	{
		AfxMessageBox("한개 이상의 필드를 선택하세요.");		
	}
	else 
	{
		m_pWizard->m_chkListSelColumn = &m_ctlColumnList;
		m_pWizard->m_strTableName = m_strTableName;
		m_pWizard->m_strCliTableName = m_strCliTableName;
		return CPropertyPage::OnWizardNext();
	}
	CPropertySheet* pSheet = static_cast<CPropertySheet*>( GetParent() );
    pSheet->SetActivePage(0);
	return CPropertyPage::OnSetActive();
}

int CPanelTable::GetCheckStatus(int nCount, CCheckListBox *ctlList)
{
	int nChkCount = 0;
	for(int i=0 ; i<nCount ; i++)
		if(ctlList->GetCheck(i)==1)
			nChkCount++;

	return nChkCount;
}

CString CPanelTable::GetCliTableName(CString strTable)
{
	char szQuery[QUERY_MAX_LEN];
	CSQLDirect aSD;
	CString strCliTable = "";

	if(aSD.ConnectToDB() == FALSE) return "";

	sprintf(szQuery, "SELECT CliTableName FROM openMSYNC_Table t, "
        "openMSYNC_DSN d, openMSYNC_version v WHERE t.DBID = d.DBID "
        "AND d.VersionID = v.VersionID AND v.Application = '%s' "
        "AND v.Version = %s AND d.SvrDSN = '%s' AND t.SvrTableName = '%s'", 
		m_pWizard->m_strApp, m_pWizard->m_strVer,
        m_pWizard->m_strDSNName, strTable);

	if(aSD.ExecuteSQL(szQuery) == SQL_SUCCESS)
	{
		while(aSD.FetchSQL() == SQL_SUCCESS)
		{
			strCliTable = aSD.GetColumnValue(1);
		}
	}
	if(strCliTable.GetLength() == 0)
		return strTable;

	return strCliTable;
}
