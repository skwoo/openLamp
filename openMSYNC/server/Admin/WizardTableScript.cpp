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

// WizardTableScript.cpp : implementation file
//

#include "stdafx.h"
#include "Admin.h"
#include "WizardTableScript.h"
#include "PanelTable.h"
#include "PanelScript.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWizardTableScript

IMPLEMENT_DYNAMIC(CWizardTableScript, CPropertySheet)

CWizardTableScript::CWizardTableScript(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CWizardTableScript::CWizardTableScript(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

CWizardTableScript::~CWizardTableScript()
{
}


BEGIN_MESSAGE_MAP(CWizardTableScript, CPropertySheet)
	//{{AFX_MSG_MAP(CWizardTableScript)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizardTableScript message handlers

int CWizardTableScript::DoModal() 
{
	// TODO: Add your specialized code here and/or call the base class
	AddPage ( &m_panelTable );
	m_panelTable.m_psp.dwFlags &= ~PSP_HASHELP;
	
	AddPage ( &m_panelScript );
	m_panelScript.m_psp.dwFlags &= ~PSP_HASHELP;
	
    m_psh.dwFlags &= ~PSH_HASHELP;
    m_psh.dwFlags |= PSH_WIZARD;	

	return CPropertySheet::DoModal();
}

void CWizardTableScript::SetValue(int nState, CString strApp, CString strVer, 
                                  CString strDSN, CString strUid, CString strPwd,
                                  CString strTable)
{
	m_nState = nState;
	m_strApp = strApp;
	m_strVer = strVer;
	m_strDSNName = strDSN;
	m_strLoginID = strUid;
	m_strPWD = strPwd;
	m_strTableName = strTable;
}
