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

#include "stdafx.h"
//#include "Admin.h"
#include "TabItem.h"
#include <afxpriv.h>        // Needed for WM_SIZEPARENT

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CTabItem
//

CTabItem::CTabItem(CWnd *pWnd, LPCTSTR szLabel, int imageId, CWnd *pParent)
{
	m_pWnd = pWnd;
	m_nMinX = m_nMaxX = 0;
	m_szLabel = szLabel;
	m_imageId = imageId;
	RECT rect;
	::ZeroMemory(&rect, sizeof(RECT));
	m_Caption.Create(szLabel, 
        WS_CHILD|SS_CENTER|SS_CENTERIMAGE|WS_VISIBLE, rect, pParent);
}

CTabItem::~CTabItem()
{
	m_Caption.DestroyWindow();
}

// Set rectangle for tab caption
void CTabItem::SetRect(CRect& rect)
{
	m_Caption.MoveWindow(&rect);
}

// Set font for tab caption
void CTabItem::SetFont(CFont *pFont)
{
	m_Caption.SetFont(pFont, FALSE);
}

// Get tab caption text
CString CTabItem::GetText(void)
{
	CString str;
	m_Caption.GetWindowText(str);
	return str;
}
void CTabItem::SetText(LPCTSTR szLabel)
{
	m_Caption.SetWindowText(szLabel);
}
