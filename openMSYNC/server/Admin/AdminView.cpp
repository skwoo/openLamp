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

// AdminView.cpp : implementation of the CAdminView class
//

#include "stdafx.h"
#include "Admin.h"

#include "AdminDoc.h"
#include "AdminView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAdminView

IMPLEMENT_DYNCREATE(CAdminView, CView)

BEGIN_MESSAGE_MAP(CAdminView, CView)
	//{{AFX_MSG_MAP(CAdminView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAdminView construction/destruction

CAdminView::CAdminView()
{
	// TODO: add construction code here

}

CAdminView::~CAdminView()
{
}

BOOL CAdminView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CAdminView drawing

void CAdminView::OnDraw(CDC* pDC)
{
	CAdminDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CAdminView diagnostics

#ifdef _DEBUG
void CAdminView::AssertValid() const
{
	CView::AssertValid();
}

void CAdminView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CAdminDoc* CAdminView::GetDocument() // non-debug version is inline
{
	return (CAdminDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CAdminView message handlers
