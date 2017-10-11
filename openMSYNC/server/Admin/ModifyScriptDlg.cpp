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

// ModifyScriptDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Admin.h"
#include "ModifyScriptDlg.h"
#include "MainFrm.h"
#include "RightListView.h"
#include "SQLDirect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CModifyScriptDlg dialog


CModifyScriptDlg::CModifyScriptDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CModifyScriptDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CModifyScriptDlg)
	m_strScript = _T("");
	//}}AFX_DATA_INIT
}


void CModifyScriptDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModifyScriptDlg)
	DDX_Text(pDX, IDC_SCRIPT, m_strScript);
	DDV_MaxChars(pDX, m_strScript, 4000);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CModifyScriptDlg, CDialog)
	//{{AFX_MSG_MAP(CModifyScriptDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModifyScriptDlg message handlers
void CModifyScriptDlg::SetValue(int nIndex, CString strTableID, CString strEvent)
{
	m_nIndex = nIndex;
	m_strTableID = strTableID;
	m_strEvent = strEvent;
}

void CModifyScriptDlg::OnOK() 
{
	// TODO: Add extra validation here
	UpdateData(TRUE);

	CMainFrame *pMainFrame = (CMainFrame *)AfxGetMainWnd(); 
	CSQLDirect aSD;
	CString strScript;

	if(aSD.ConnectToDB() == FALSE)
	{
		AfxMessageBox("저장에 실패하였습니다");
		return;
	}
	m_strScript.TrimLeft(_T("\r\n "));
	strScript = m_strScript;  

	strScript.Replace("'", "''");

	char szQuery[QUERY_MAX_LEN];
	
	if(pMainFrame->m_nDBType == SYBASE)
		sprintf(szQuery, "UPDATE openMSYNC_Script SET Script = '%s' "
            "WHERE TableID = %s AND Event = upper('%s')", 
			strScript, m_strTableID, m_strEvent);
	else
		sprintf(szQuery, "UPDATE openMSYNC_Script SET Script = '%s' "
            "WHERE TableID = %s AND Event = '%s'", 
			strScript, m_strTableID, m_strEvent);
	
	if(aSD.ExecuteSQL(szQuery) != SQL_SUCCESS)
	{
		AfxMessageBox("저장에 실패하였습니다");
		return;
	}		
	// TableID, event(U,I,D), event(Upload_Update...), script니까 
    // 4번째 값이므로 index는 3
	pMainFrame->m_pRightListView->ModifyColValue(m_nIndex, 3, m_strScript);	
	CDialog::OnOK();
}
