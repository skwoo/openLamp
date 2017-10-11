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

// MSyncIPBar.h: interface for the CMyToolBar class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MYTOOLBAR_H__D04C13B6_98CD_42E2_B9B5_7D27834FA7D6__INCLUDED_)
#define AFX_MYTOOLBAR_H__D04C13B6_98CD_42E2_B9B5_7D27834FA7D6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
//#include "MyEdit.h"

class CMSyncIPBar : public CToolBar
{
public:
	CString GetServerIP();
	void MakeEdit(int pos, int toolid, CSize size);
	CMSyncIPBar();
	virtual ~CMSyncIPBar();
	//CMyEdit m_Edit;
//	CEdit m_Edit;
	CStatic m_Edit;
};

#endif // !defined(AFX_MYTOOLBAR_H__D04C13B6_98CD_42E2_B9B5_7D27834FA7D6__INCLUDED_)
