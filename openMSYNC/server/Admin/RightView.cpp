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

// RightView.cpp : implementation file
//

#include "stdafx.h"
#include "Admin.h"
#include "RightView.h"
#include "TabItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define TABWND_HEIGHT     30    // Height of the gray border between the toolbar 
                                // and the client area
#define TAB_HEIGHT        24    // Height on the normal tab
#define TABSEL_HEIGHT     24    // Height of the selected tab
#define TAB_SPACE         10    // Add to tab caption text width
#define TAB_DEPL          4     // Distance between the tabs and the client area
#define TAB_MAXLEN        200
/////////////////////////////////////////////////////////////////////////////
// CRightView

IMPLEMENT_DYNCREATE(CRightView, CView)

CRightView::CRightView()
{
	m_nSelectedTab = -1;
	m_pTabItem = NULL;
	::ZeroMemory(&m_ImageRect, sizeof(RECT));

	m_BrushBlack.CreateSolidBrush(RGB(0,0,0));
	m_BrushLGray.CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
	m_PenBlack.CreatePen(PS_SOLID, 1, (COLORREF)0);
	m_PenLGray.CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_BTNFACE));
	m_PenWhite.CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_BTNHIGHLIGHT));
	m_PenWhite2.CreatePen(PS_SOLID, 2, ::GetSysColor(COLOR_BTNHIGHLIGHT));
	m_PenDGray.CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_BTNSHADOW));
	m_PenDGray2.CreatePen(PS_SOLID, 2, ::GetSysColor(COLOR_BTNSHADOW));
}

CRightView::~CRightView()
{
	m_BrushBlack.DeleteObject();
	m_BrushLGray.DeleteObject();
	m_PenBlack.DeleteObject();
	m_PenLGray.DeleteObject();
	m_PenWhite.DeleteObject();
	m_PenWhite2.DeleteObject();
	m_PenDGray.DeleteObject();
	m_PenDGray2.DeleteObject();
}


BEGIN_MESSAGE_MAP(CRightView, CView)
	//{{AFX_MSG_MAP(CRightView)
		ON_WM_PAINT()
		ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CRightView::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CView::PreCreateWindow(cs);
}
/////////////////////////////////////////////////////////////////////////////
// CRightView drawing

void CRightView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CRightView diagnostics

#ifdef _DEBUG
void CRightView::AssertValid() const
{
	CView::AssertValid();
}

void CRightView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CRightView message handlers

void CRightView::SetTabImage()
{
	IMAGEINFO info;
	m_ImageList.Create(IDB_DISPLAYTAB, 16, 1, RGB(255,0,0));
	m_ImageList.SetBkColor(::GetSysColor(COLOR_BTNFACE));
	m_ImageList.GetImageInfo(0, &info);
	m_ImageRect = info.rcImage;
}

CWnd * CRightView::AddView(LPCTSTR szTitle, CRuntimeClass *pRuntimeClass, 
                           CCreateContext *pContext, int imageId)
{
	UINT nID = AFX_IDW_PANE_FIRST;

    CView* pView = NULL;
    TRY
	{
	    pView = (CView *)pRuntimeClass->CreateObject();
	    if (pView == NULL)
		AfxThrowMemoryException();
	}
    CATCH_ALL(e)
	{
	    TRACE0("Out of memory creating a tab window view\n");
	    return NULL;
	}
    END_CATCH_ALL
	
    if (!pView->Create(NULL, NULL, WS_CHILD|WS_VISIBLE, 
                       CRect(0,0,0,0), this, nID, pContext))
	{
	    TRACE0("Warning: couldn't create the tab window view \n");
	    return NULL;
	}

	m_pTabItem = new CTabItem(pView, szTitle, imageId, this);

	return (CWnd *)pView;
}

BOOL CRightView::UpdateFrame(CFrameWnd *pFrame, CWnd *pWnd)
{
    if (pWnd->IsKindOf(RUNTIME_CLASS(CView))) 
    {
        // New tab item is a view
        pFrame->SetActiveView((CView*)pWnd);
        return TRUE;
    }
    else if (pWnd->IsKindOf(RUNTIME_CLASS(CRightView))) 
    {
        pFrame->SetActiveView((CView*)pWnd);
        return TRUE;
    }
    else if (pWnd->IsKindOf(RUNTIME_CLASS(CSplitterWnd))) 
    {
        CSplitterWnd *pSplitter = (CSplitterWnd*)pWnd;
        CWnd *pView = pSplitter->GetActivePane();
        if (pView == NULL) 
        {
            CWnd *pTmpView;
            for (int x = 0; x < pSplitter->GetRowCount(); x ++) 
            {
                for (int y = 0; y < pSplitter->GetColumnCount(); y ++) 
                {
                    pTmpView = pSplitter->GetPane(x,y);
                    if (pTmpView->IsWindowEnabled()) 
                    {
                        if (UpdateFrame(pFrame, pTmpView)) return TRUE;
                    }
                }
            }
        }
        if (pView == NULL) pView = pSplitter->GetPane(0,0);
        pFrame->SetActiveView((CView*)pView);
    }
    
	return FALSE;
}

void CRightView::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	CRect client;

	GetClientRect(&client);
	drawClient(&dc, client);

	int x = 2, nIndex = 0;
	if(m_pTabItem){
		m_pTabItem->m_nMinX = x;
		x += drawTabTop(&dc, x, client);
		m_pTabItem->m_nMaxX = x;
	}

}
// Draw edge arround client area
void CRightView::drawClient(CDC *pDc, CRect &rect)
{
	CWnd *pParent = GetParent();

	if (pParent->IsKindOf(RUNTIME_CLASS(CFrameWnd))) {
		pDc->DrawEdge(&rect, EDGE_ETCHED, BF_TOP);
	}
	pDc->Draw3dRect(0,TABWND_HEIGHT, rect.right, rect.bottom-TABWND_HEIGHT, 
        ::GetSysColor(COLOR_BTNSHADOW), ::GetSysColor(COLOR_BTNHIGHLIGHT));
	pDc->Draw3dRect(1,TABWND_HEIGHT+1, rect.right-2, rect.bottom-TABWND_HEIGHT-2, 
        0, ::GetSysColor(COLOR_3DLIGHT));

}

// Draws a selected tab and returns its height
int CRightView::drawTabTop(CDC *pDC, int x, CRect &client)
{
	m_pTabItem->SetFont(&m_SelectFont);
	CFont *pOldFont = pDC->SelectObject(&m_SelectFont);

	CString str = m_pTabItem->GetText();
	CSize textSize = pDC->GetTextExtent(str);
	textSize.cx += 6;
	CSize tabSize = textSize;
	tabSize.cx += m_ImageRect.Width();
	if (tabSize.cx > TAB_MAXLEN) {
		tabSize.cx = TAB_MAXLEN;
		textSize.cx = TAB_MAXLEN - m_ImageRect.Width();
	}
	pDC->SelectObject(pOldFont);

	int y = TABWND_HEIGHT - TABSEL_HEIGHT - TAB_DEPL;

	// black border, no bottom line
	pDC->SelectObject(&m_PenBlack);
	pDC->MoveTo(x,y+TABSEL_HEIGHT-1);
	pDC->LineTo(x,y);
	pDC->LineTo(x+tabSize.cx+TAB_SPACE-1, y);
	pDC->LineTo(x+tabSize.cx+TAB_SPACE-1, y+TABSEL_HEIGHT);

	// left and upper border in white, double line
	pDC->SelectObject(&m_PenWhite2);
	pDC->MoveTo(x+2,y+TABSEL_HEIGHT-1);
	pDC->LineTo(x+2,y+2);
	pDC->LineTo(x+tabSize.cx+TAB_SPACE-4, y+2);

	// right border, dark gray, double line
	pDC->SelectObject(&m_PenDGray2);
	pDC->MoveTo(x+tabSize.cx+TAB_SPACE-2, y+2);
	pDC->LineTo(x+tabSize.cx+TAB_SPACE-2, y+TABSEL_HEIGHT-1);

	// clean up
	pDC->SelectObject(&m_PenLGray);
	pDC->MoveTo(x-1, y+TABSEL_HEIGHT);
	pDC->LineTo(x+tabSize.cx+TAB_SPACE, y+TABSEL_HEIGHT);
	pDC->MoveTo(x-1, y+TABSEL_HEIGHT+1);
	pDC->LineTo(x+tabSize.cx+TAB_SPACE, y+TABSEL_HEIGHT+1);

	// a black line to far left and right
	pDC->SelectObject(&m_PenBlack);
	pDC->MoveTo(0, y+TABSEL_HEIGHT-1);
	pDC->LineTo(x, y+TABSEL_HEIGHT-1);
	pDC->MoveTo(x+tabSize.cx+TAB_SPACE+1, y+TABSEL_HEIGHT-1);
	pDC->LineTo(client.right, y+TABSEL_HEIGHT-1);

	// and a white double line
	pDC->SelectObject(&m_PenWhite2);
	if (x!=0) {
		pDC->MoveTo(0, y+TABSEL_HEIGHT+1);
		pDC->LineTo(x, y+TABSEL_HEIGHT+1);
	}
	pDC->MoveTo(x+tabSize.cx+TAB_SPACE, y+TABSEL_HEIGHT+1);
	pDC->LineTo(client.right, y+TABSEL_HEIGHT+1);

	// gray inside
	pDC->FillSolidRect(x+3, y+3,tabSize.cx+TAB_SPACE-6, 
        TABSEL_HEIGHT, ::GetSysColor(COLOR_BTNFACE));

	// draw image
	if (m_pTabItem->m_imageId >= 0) 
	{
		CPoint p0(x+TAB_SPACE/2, y+(TAB_HEIGHT-m_ImageRect.Height())/2+1);
		m_ImageList.Draw(pDC, m_pTabItem->m_imageId, p0, ILD_TRANSPARENT);
	}

	// draw title label
	CRect rect(CPoint(x+TAB_SPACE/2+m_ImageRect.Width(), 
                      y+(TAB_HEIGHT-textSize.cy)/2+1), textSize);
	m_pTabItem->SetRect(rect);

	return tabSize.cx+TAB_SPACE;
}
// Erase area where the tabs are displayed
BOOL CRightView::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	CRect rect;
	GetClientRect(&rect);
	pDC->FillSolidRect(&CRect(0, 0, rect.right, TABWND_HEIGHT), 
                       ::GetSysColor(COLOR_BTNFACE));
	return TRUE;
}
// Handle resize
void CRightView::OnSize(UINT nType, int cx, int cy) 
{

	CView::OnSize(nType, cx, cy);
	CRect rect;
	CWnd *pParent = (CWnd*)GetParent();

	pParent->GetClientRect(&rect); 

	CSplitterWnd *splitter = (CSplitterWnd*)pParent;

	int row, col;
	splitter->IsChildPane(this, row, col);
	splitter->RecalcLayout();
	if(m_pTabItem){
		CWnd *pWnd;
		pWnd = m_pTabItem->m_pWnd; 
		if (cx == -1 && cy == -1) // 683, 952
			pWnd->MoveWindow(1, TABWND_HEIGHT+1, rect.Width()-2, rect.Height()-TABWND_HEIGHT-2);
		else	// 679, 723
			pWnd->MoveWindow(1, TABWND_HEIGHT+1, cx, cy-TABWND_HEIGHT-2);
	}
}


void CRightView::SetTabLabel(int nIndex, LPCTSTR szLabel)
{
	m_pTabItem->SetText(szLabel);
	InvalidateRect(&CRect(0, 0, 32000, TABWND_HEIGHT));
}
