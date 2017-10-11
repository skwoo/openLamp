/* 
   This file is part of openML, mobile and embedded DBMS.

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

// QueryTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "QueryTestDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern iSQL isql;
// API 

// 리스트 컨트롤 핸들
HWND m_hResultList = NULL;
// 리스트 컨트롤 Create
HWND Create_ResultList(HWND hWnd, int nIndex, int w, char **_cfield) 
{
	HWND hwndLV;
	LVCOLUMN lvc;
	int i = 0;
	TCHAR _tFieldName[32];
	if(m_hResultList)
	{
		ListView_DeleteAllItems(m_hResultList);
		while(1)
		{
			if(!ListView_DeleteColumn(m_hResultList, 0))
				break;
		}
		m_hResultList = NULL;
	}
	hwndLV = GetDlgItem(hWnd, IDC_LIST_RESULT);
		
	// Add columns.
	if( hwndLV )
	{
		lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT | LVCF_SUBITEM;
		lvc.fmt = LVCFMT_LEFT;
		for(i = 0; i < nIndex; i++)
		{
			wsprintf(_tFieldName, "%s", _cfield[i]);
			lvc.pszText = _tFieldName;
			lvc.cx = w;
			lvc.iSubItem = i;
			ListView_InsertColumn( hwndLV, i, (LPARAM)&lvc);
		}
	}

	ListView_SetExtendedListViewStyle ( hwndLV, LVS_EX_FULLROWSELECT );
//	HDC hdc = ::GetDC(hwndLV);
//	::SetBkColor(hdc, 0x00FF0000);
	return hwndLV;
}

// 리스트 컨트롤 데이터 생성 
void Build_ResultList(int i)
{
	ListView_SetItemCount(m_hResultList, i );
	InvalidateRect(m_hResultList, NULL, TRUE);

	if(i)
	{
		ListView_EnsureVisible(m_hResultList, 0, FALSE );
		ListView_SetItemState(m_hResultList, 0, 
							   LVIS_SELECTED | LVIS_FOCUSED, 
							   LVIS_SELECTED | LVIS_FOCUSED); 
	}

}
// API~
/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQueryTestDlg dialog

CQueryTestDlg::CQueryTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CQueryTestDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CQueryTestDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CQueryTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CQueryTestDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CQueryTestDlg, CDialog)
	//{{AFX_MSG_MAP(CQueryTestDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_EXIT, OnButtonExit)
	ON_BN_CLICKED(IDC_BUTTON_EXE, OnButtonExe)
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, OnButtonConnect)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQueryTestDlg message handlers

BOOL CQueryTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	CString	strtmp;
	strtmp.Format("%s", DBNAME);
	SetDlgItemText(IDC_EDIT_PATH, strtmp);

	char	dbfilename[256];
	int		result;
	char*	_cfield[1];
	//_cfield = (char*)malloc(1);
	//memset(*_cfield, 0x00, 1);

    {
        char* path;

        path = (char*)malloc(256);

        //memcpy(path, (LPSTR)(LPCSTR)strDBPathA, strDBPathA.GetLength()+1);

        sprintf(path, "C:\\OZApplication Viewer\\forcs\\work\\19FA76C58FE646da85A9AB168F7A698C\\Q_20060407173552DTBD");

        int i = createdb(path);

        free(path);

    }

	_cfield[0] = (char*)malloc(FIELD_NAME_LENG);
	sprintf(_cfield[0], "초기화\0");
	
	sprintf(dbfilename, "%s", DBNAME);
	m_hResultList = Create_ResultList(this->GetSafeHwnd(), 1, 400, _cfield);
	// 데이터베이스 생성.
	int			nlen		= 0;
	result = createdb(dbfilename);		
	if (result != DB_E_DBDIREXISTED && result != DB_SUCCESS)
	{		
		strtmp.Format("데이터베이스 초기화 실패");
		m_FetchRow.Add(strtmp);
	}
	else
	{
		if (result != DB_E_DBDIREXISTED)
		{
			strtmp.Format("데이터베이스 생성");
			m_FetchRow.Add(strtmp);
		}
		iSQL_connect(&isql, NULL, dbfilename, "id", "pwd");
		strtmp.Format("데이터베이스 초기화 성공");
		m_FetchRow.Add(strtmp);
		strtmp.Format("데이터베이스에 연결 되었습니다.");
		m_FetchRow.Add(strtmp);
	}
	m_nField = 1;		
	m_nFetchSize = m_FetchRow.GetSize();
	Build_ResultList(m_nFetchSize / m_nField);
	free(_cfield[0]);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CQueryTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CQueryTestDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CQueryTestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CQueryTestDlg::OnButtonExit() 
{
	// TODO: Add your control notification handler code here
	iSQL_disconnect(&isql);
	CWnd::DestroyWindow();
	PostQuitMessage(0);
}

void CQueryTestDlg::OnButtonExe() 
{
	// TODO: Add your control notification handler code here
	CString strQuery;
	char* _cquery;
//	_cquery = (char*)malloc(QUERY_LENGTH);
//	memset(_cquery, 0x00, QUERY_LENGTH);

	m_FetchRow.RemoveAll();
	m_FetchRow.FreeExtra();
	GetDlgItemText(IDC_EDIT_QUERY, strQuery);
	strQuery.TrimLeft();
	strQuery.TrimRight();
	_cquery = (char*)(LPCTSTR)strQuery;
	CString strTmp;
	strTmp.Format("%s", strQuery.Mid(0, 6));
	strTmp.MakeLower();
	if(strTmp == "select" || strTmp.Mid(0, 4) == "desc")
	{
		BeginWaitCursor();
		SelectProcess(_cquery);
		EndWaitCursor();
	}
	else
	{
		long a, b;
		a = GetTickCount();
		if(iSQL_query(&isql, _cquery) != 0)
		{
			// 에러가 발생하면 rollback한다.
			strQuery.Format("%s(%d)", iSQL_error(&isql), iSQL_errno(&isql));
			AfxMessageBox(strQuery, MB_OK);
			__SYSLOG("%s(%d)\n", iSQL_error(&isql), iSQL_errno(&isql));
			__SYSLOG("%s\n", strQuery);
			iSQL_rollback(&isql);
			strQuery.Format("쿼리실패");
			m_FetchRow.Add(strQuery);
			strQuery.Format("%s(%d)", iSQL_error(&isql), iSQL_errno(&isql));
			m_FetchRow.Add(strQuery);
		}
		else
		{
			iSQL_commit(&isql);
			strQuery.Format("쿼리성공");
			m_FetchRow.Add(strQuery);
		}
		b = GetTickCount();
		strQuery.Format("Commit Query Time : %ld", b - a);
		SetDlgItemText(IDC_STATIC_TIME, strQuery);
		m_nField = 1;		

		char*	_cfield[1];
		_cfield[0] = (char*)malloc(FIELD_NAME_LENG);
		sprintf(_cfield[0], "Commit\0");
		m_hResultList = Create_ResultList(this->GetSafeHwnd(), 1, 400, _cfield);
		free(_cfield[0]);
	}
	m_nFetchSize = m_FetchRow.GetSize();
	Build_ResultList(m_nFetchSize / m_nField);

}

int CQueryTestDlg::SelectProcess(char *_cquery)
{
	int			i;
	iSQL_RES 	*res;
	iSQL_ROW	row;
	CString		temp;
	long	a, b;
	a = GetTickCount();
	iSQL_query(&isql, _cquery);
	if(iSQL_errno(&isql) != 0)
	{
		// 에러가 발생하면 rollback한다.
		__SYSLOG("%s(%d)\n", iSQL_error(&isql), iSQL_errno(&isql));
		__SYSLOG("%s\n", _cquery);
		iSQL_rollback(&isql);
		temp.Format("ERROR");
		m_FetchRow.Add(temp);
		temp.Format("%s(%d)", iSQL_error(&isql), iSQL_errno(&isql));
		m_FetchRow.Add(temp);
		m_nField = 1;
		temp.Empty();
		temp.FreeExtra();
		
		char*	_cfield[1];
		_cfield[0] = (char*)malloc(FIELD_NAME_LENG);
		sprintf(_cfield[0], "ERROR");
		m_hResultList = Create_ResultList(this->GetSafeHwnd(), m_nField, 100, _cfield);	
		free(_cfield[0]);
		return SQL_ERROR;
	}
	else
	{
		// select 문을 실행했을때 수행한다.
		// 실행결과물(select결과물)을 m_DlgHID 순서대로 add한다.
		//res = iSQL_use_result(&isql);
		res = iSQL_store_result(&isql);

		if (res != NULL)
		{
			m_nField = iSQL_num_fields(res);

			while (TRUE)
			{
				// 데이터가 없으면 break해서 while문을 종료한다.
				if ((row = iSQL_fetch_row(res)) == SQL_NO_DATA)
				{
					if(iSQL_eof(res) == 0)
						return SQL_ERROR;
					else
						break;
				}
				
                __SYSLOG("insert into a values(");
				for(i = 0; i < m_nField; i++)
				{
					if (row[i] == NULL || row[i] == "")
						temp.Format("");
					else
						temp.Format("%s", row[i]);
                    if (i == 0)
                        __SYSLOG("'%s'", row[i]);
                    else
                        __SYSLOG(",'%s'", row[i]);
					m_FetchRow.Add(temp);
				}
                __SYSLOG("\n", row[i]);
			}
		}
		else
		{
			m_nField = 1;
			temp.Format("ERROR");
			m_FetchRow.Add(temp);
			temp.Empty();
			temp.FreeExtra();
			return SQL_ERROR;
		}
	}
	b = GetTickCount();

	char**	_cfield = (char **)malloc(m_nField * sizeof(char *));
	iSQL_FIELD* isql_field;
	isql_field = iSQL_fetch_fields(res);
	for(i = 0; i < m_nField; i++)
	{
		_cfield[i] = (char*)malloc(FIELD_NAME_LENG);
		sprintf(_cfield[i], "%s", isql_field[i].name);
	}
	m_hResultList = Create_ResultList(this->GetSafeHwnd(), m_nField, 100, _cfield);

	temp.Format("Select Time : %ld ms", b - a);
	SetDlgItemText(IDC_STATIC_TIME, temp);
	iSQL_free_result(res);
	iSQL_commit(&isql);
	temp.Empty();
	temp.FreeExtra();
	for(i = 0; i < m_nField; i++)
		free(_cfield[i]);
    free(_cfield);
	return SQL_OK;
}

BOOL CQueryTestDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	// TODO: Add your specialized code here and/or call the base class
	LV_DISPINFO *pLvdi = (LV_DISPINFO *)lParam;
	NM_LISTVIEW *pNm = (NM_LISTVIEW *)lParam;
	NMDATETIMECHANGE *lpChange = (NMDATETIMECHANGE *)lParam;
	switch( wParam )
	{
		case IDC_LIST_RESULT:
			switch(pLvdi->hdr.code)
			{
				case LVN_GETDISPINFO:
					pLvdi = (NMLVDISPINFO *)lParam;

					if(pLvdi->item.mask & LVIF_IMAGE) 
						pLvdi->item.iImage = 0;

					if(pLvdi->item.mask & LVIF_STATE) 
						pLvdi->item.state = 0;

					if(pLvdi->item.mask & LVIF_PARAM) 
						pLvdi->item.lParam = pLvdi->item.iItem;
					// ListCtrl에 데이터 Set
					if(pLvdi->item.mask & LVIF_TEXT &&
						m_nFetchSize > (pLvdi->item.iItem * m_nField + pLvdi->item.iSubItem))
						sprintf(pLvdi->item.pszText, "%s", m_FetchRow.GetAt(pLvdi->item.iItem * m_nField + pLvdi->item.iSubItem));
					break;
				case NM_CLICK:
					m_nSel = pNm->iItem;
					break;
			}
			break;
	}
	return CDialog::OnNotify(wParam, lParam, pResult);
}

BOOL CQueryTestDlg::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	if(pMsg->message == WM_KEYUP)
	{
		switch(pMsg->wParam)
		{
		case VK_F5:
			OnButtonExe();
			break;
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}

void CQueryTestDlg::OnButtonConnect() 
{
	// TODO: Add your control notification handler code here
	iSQL_disconnect(&isql);

	CString	strtmp;
	char*	dbfilename;
	dbfilename = (char*)malloc(256);
	memset(dbfilename, 0x00, 256);
	int		result;
	char*	_cfield[1];
	_cfield[0] = (char*)malloc(FIELD_NAME_LENG);
	sprintf(_cfield[0], "초기화\0");
	GetDlgItemText(IDC_EDIT_PATH, strtmp);
	//dbfilename = (char*)(LPCTSTR)strtmp;
    strncpy(dbfilename, strtmp, 256);
	m_hResultList = Create_ResultList(this->GetSafeHwnd(), 1, 400, _cfield);
	// 데이터베이스 생성.
	int			nlen		= 0;
	result = createdb(dbfilename);		
	if (result != DB_E_DBDIREXISTED && result != DB_SUCCESS)
	{		
		strtmp.Format("데이터베이스 초기화 실패");
		m_FetchRow.Add(strtmp);
	}
	else
	{
		if (result != DB_E_DBDIREXISTED)
		{
			strtmp.Format("데이터베이스 생성");
			m_FetchRow.Add(strtmp);
		}
		iSQL_connect(&isql, NULL, dbfilename, "id", "pwd");
		strtmp.Format("데이터베이스 초기화 성공");
		m_FetchRow.Add(strtmp);
		strtmp.Format("데이터베이스에 연결 되었습니다.");
		m_FetchRow.Add(strtmp);
	}
	m_nField = 1;		
	m_nFetchSize = m_FetchRow.GetSize();		
	Build_ResultList(m_nFetchSize / m_nField);
	free(_cfield[0]);
    free(dbfilename);
}
