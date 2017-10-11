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

// TabView.h : header file
//

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif
#ifndef __AFXTEMPL_H__
#include <afxtempl.h>
#endif

#define WM_TTABWND_SELCHANGED    WM_USER + 63
/////////////////////////////////////////////////////////////////////////////
// CTabItem Object
//

class CTabItem : public CObject
{
public:
	CWnd*	m_pWnd;             // Window (view) associated with this tab

private:
	CString m_szLabel;
	int		m_imageId;
	CStatic	m_Caption;			// Caption of this tab
	int		m_nMinX;			// Client left X coordinate
	int		m_nMaxX;			// Client right X coordinate

public:
	void SetText(LPCTSTR szLabel);
	CTabItem(CWnd *pWnd, LPCTSTR szLabel, int imageId, CWnd *pParent);
	virtual ~CTabItem();
  
	void SetRect(CRect& rect);
	void SetFont(CFont *pFont);

	CString GetText(void);
	friend class CRightView;

};