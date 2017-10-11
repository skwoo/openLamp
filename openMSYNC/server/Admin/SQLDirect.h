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

#ifndef __SQLDIRECT
#define __SQLDIRECT
#include <sqlext.h>
#include <sql.h>
#include "MainFrm.h"

class CSQLDirect {
public:
	BOOL ConnectToDB(LPCSTR svDSN, LPCSTR svUID, LPCSTR svPwd);
	CString GetColumnName(int nIndex);
	CString GetTableName(int nIndex);
	int OpenAllColumns(CString strLoginID, CString strTable);
	int OpenAllTables(CString strLoginID);
	BOOL RollBack();
	BOOL Commit();
	CString GetColumnValue(int nCol);
	int FetchSQL(void);
	int ExecuteSQL(LPCSTR svSQL);
	BOOL ConnectToDB();
	CSQLDirect();
	~CSQLDirect();
protected:
	CMainFrame*		m_pMainFrame;
private:
	void DisplayError( void );
	SQLHENV		m_hEnv;
	SQLHDBC		m_hDBC;
	SQLHSTMT	m_hStmt;
	CPtrArray	m_ColArray;
	BOOL		m_bCommitted;
	BOOL		m_bOpened;
	int	m_nTableCount;
	CStringArray	m_lstTableName;
	int	m_nColumnCount;
	CStringArray	m_lstColumnName;

};
#endif