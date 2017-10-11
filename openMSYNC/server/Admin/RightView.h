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

#if !defined(AFX_RIGHTVIEW_H__C03DF6BB_69C9_4212_9244_A2B556AA2C74__INCLUDED_)
#define AFX_RIGHTVIEW_H__C03DF6BB_69C9_4212_9244_A2B556AA2C74__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RightView.h : header file
//
class CTabItem;

/////////////////////////////////////////////////////////////////////////////
// CRightView view
#define WM_TTABWND_SELCHANGED    WM_USER + 63
class CRightView : public CView
{
protected:
	CRightView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CRightView)

	int		drawTabTop(CDC *pDC, int x, CRect& client);
	void	drawClient(CDC *pDc, CRect& rect);	
	BOOL	UpdateFrame(CFrameWnd *pFrame, CWnd *pWnd);

// Attributes
public:

// Operations
public:
	void SetTabLabel(int nIndex, LPCTSTR szLabel);
	void SetTabImage();
	CWnd * AddView(LPCTSTR szTitle, CRuntimeClass *pRuntimeClass, CCreateContext *pContext, int imageId/*=-1*/);
protected:
	int		m_nSelectedTab;
	CFont	m_SelectFont;
	CFont	m_Font;
	CBrush	m_BrushBlack;
	CBrush	m_BrushLGray;
	CPen	m_PenWhite;
	CPen	m_PenWhite2;
	CPen	m_PenBlack;
	CPen	m_PenLGray;
	CPen	m_PenDGray;
	CPen	m_PenDGray2;	

	CImageList	m_ImageList;
	CRect	m_ImageRect;
	CTabItem * m_pTabItem;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRightView)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CRightView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:

	//{{AFX_MSG(CRightView)	
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RIGHTVIEW_H__C03DF6BB_69C9_4212_9244_A2B556AA2C74__INCLUDED_)
