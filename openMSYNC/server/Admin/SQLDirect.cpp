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
#include "SQLDirect.h"

CSQLDirect::CSQLDirect()
{
	// 메인프레임 포인트 얻어서 m_pMainTreeView를 가져온다
	m_pMainFrame=(CMainFrame *)AfxGetMainWnd(); 
	m_hEnv = NULL;
	m_hDBC = NULL;
	m_hStmt = NULL;
	m_bCommitted = FALSE;
	m_bOpened = FALSE;
}

CSQLDirect::~CSQLDirect()
{
	if(m_bCommitted == FALSE)	Commit();

	if(m_hStmt)	
		SQLFreeHandle(SQL_HANDLE_STMT, m_hStmt);

	if(m_hDBC){
		if(m_bOpened) SQLDisconnect(m_hDBC);
		SQLFreeHandle(SQL_HANDLE_DBC, m_hDBC);		
	}
	if(m_hEnv)		SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
}

BOOL CSQLDirect::ConnectToDB()
{
	LPCSTR svDSN = m_pMainFrame->m_strDSNName;
	LPCSTR svUID = m_pMainFrame->m_strLoginID ;
	LPCSTR svPwd = m_pMainFrame->m_strPWD;

	if(m_pMainFrame->CheckDBSetting() == FALSE) return FALSE;
	
	int retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_hEnv);
	if (retcode != SQL_SUCCESS) {
		DisplayError();
		return FALSE;  
	}
	// Set the ODBC version environment attribute 
	retcode = SQLSetEnvAttr(m_hEnv, 
                            SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); 
	if (retcode != SQL_SUCCESS){
		DisplayError();
		return FALSE;
	}
	// Allocate connection handle ==> hdbc 
	retcode = SQLAllocHandle(SQL_HANDLE_DBC, m_hEnv, &m_hDBC); 
	if (retcode != SQL_SUCCESS) {
		DisplayError();
		return FALSE;  
	}
	
	// Set login timeout to 5 seconds. 
	SQLSetConnectAttr(m_hDBC, SQL_ATTR_LOGIN_TIMEOUT, (void*)5, 0);
	SQLSetConnectAttr(m_hDBC, SQL_ATTR_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_OFF, 0);
	// Connect to data source 
	retcode = SQLConnect(m_hDBC, (SQLCHAR *)svDSN, SQL_NTS,
			            (SQLCHAR *)svUID, SQL_NTS,
					    (SQLCHAR *)svPwd, SQL_NTS);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
		DisplayError();
		return FALSE; // error returned 
	}
	m_bOpened = TRUE;
	return TRUE;
}

BOOL CSQLDirect::ConnectToDB(LPCSTR svDSN, LPCSTR svUID, LPCSTR svPwd)
{

	if(strlen(svDSN) == 0 || strlen(svUID) == 0 || strlen(svPwd) == 0 )
	{
		AfxMessageBox("Application의 DSN 설정이 잘못되었습니다.");
		return FALSE;
	}
	
	int retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_hEnv);
	if (retcode != SQL_SUCCESS) {
		DisplayError();
		return FALSE;  
	}
	// Set the ODBC version environment attribute 
	retcode = SQLSetEnvAttr(m_hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); 
	if (retcode != SQL_SUCCESS){
		DisplayError();
		return FALSE;
	}
	// Allocate connection handle ==> hdbc 
	retcode = SQLAllocHandle(SQL_HANDLE_DBC, m_hEnv, &m_hDBC); 
	if (retcode != SQL_SUCCESS) {
		DisplayError();
		return FALSE;  
	}
	
	// Set login timeout to 5 seconds. 
	SQLSetConnectAttr(m_hDBC, SQL_ATTR_LOGIN_TIMEOUT, (void*)5, 0);
	SQLSetConnectAttr(m_hDBC, SQL_ATTR_AUTOCOMMIT, (void*)SQL_AUTOCOMMIT_OFF, 0);
	// Connect to data source 
	retcode = SQLConnect(m_hDBC, (SQLCHAR *)svDSN, SQL_NTS,
			            (SQLCHAR *)svUID, SQL_NTS,
					    (SQLCHAR *)svPwd, SQL_NTS);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO){
		DisplayError();
		return FALSE; // error returned 
	}
	m_bOpened = TRUE;
	return TRUE;
}

void CSQLDirect::DisplayError()
{
	_TUCHAR* pSqlState=new _TUCHAR[SQL_MAX_MESSAGE_LENGTH-1];
	_TUCHAR* m_psvErrorMsg = new _TUCHAR[256];
	SDWORD NativeError;
	SWORD svErrorMsg;
	CString svError;

	SQLError(m_hEnv, m_hDBC, m_hStmt, pSqlState, &NativeError, m_psvErrorMsg,
		SQL_MAX_MESSAGE_LENGTH-1,&svErrorMsg );

	svError =m_psvErrorMsg;
	AfxMessageBox(svError);

	delete m_psvErrorMsg;
	delete pSqlState;
}


int CSQLDirect::ExecuteSQL(LPCSTR svSQL)
{
	int retcode;

	if(m_hStmt)	{
		SQLFreeHandle(SQL_HANDLE_STMT, m_hStmt);
	}
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, m_hDBC, &m_hStmt); 
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
	{
		DisplayError();
		return retcode ; // error returned 	
	}
	
	retcode = SQLExecDirect(m_hStmt, (SQLCHAR *)svSQL, SQL_NTS);
	
	if (retcode != SQL_SUCCESS 
        && retcode != SQL_SUCCESS_WITH_INFO && retcode != SQL_NO_DATA)
    {
		DisplayError();
		retcode = SQL_ERROR;
	}
	else	retcode = SQL_SUCCESS;

	return retcode;
}

int CSQLDirect::FetchSQL()
{
	int retcode = SQLFetch(m_hStmt);
	if(retcode != SQL_SUCCESS && retcode != SQL_NO_DATA) 
		DisplayError();

	return retcode;
}
CString CSQLDirect::GetColumnValue(int nCol)
{
	CString  strValue;
	UCHAR svData[8192];
	SDWORD cbDataLen;
	memset(svData, 0x00, sizeof(svData));
	SQLGetData(m_hStmt, nCol, SQL_C_CHAR, &svData, 8192, &cbDataLen);
	strValue = svData;
	strValue.TrimRight();

	return strValue;	
}

BOOL CSQLDirect::Commit()
{
	int ret = SQLEndTran(SQL_HANDLE_DBC, m_hDBC, SQL_COMMIT);
	m_bCommitted = TRUE;
	if(ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
		return TRUE;
	return FALSE;
}

BOOL CSQLDirect::RollBack()
{
	int ret = SQLEndTran(SQL_HANDLE_DBC, m_hDBC, SQL_ROLLBACK);
	m_bCommitted = TRUE;
	if(ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
		return TRUE;
	return FALSE;
}

int CSQLDirect::OpenAllTables(CString strLoginID)
{
	CString strTable;
	
	int retcode = SQLAllocHandle(SQL_HANDLE_STMT, m_hDBC, &m_hStmt); 
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
	{
		DisplayError();
		return -1; // error returned 	
	}	
	if(m_pMainFrame->m_nDBType == ACCESS || m_pMainFrame->m_nDBType == MYSQL){
	    retcode = SQLTables(m_hStmt, NULL,SQL_NTS, NULL, SQL_NTS, NULL,SQL_NTS, NULL,SQL_NTS) ;
	}
	else
    {
		char czOwner[100];
		if(m_pMainFrame->m_nDBType == ORACLE || m_pMainFrame->m_nDBType == DB2)
        {    
            // oracle은 owner도 upstring이어야 한다.
			char *czOwnerUp;	
			czOwnerUp = _strupr(_strdup(strLoginID));
			sprintf(czOwner, "%s", czOwnerUp);
			free(czOwnerUp);
		}
		else
			sprintf(czOwner, "%s", strLoginID);

		// login ID가 sa면 소유자는 dbo로 검색해야 한다.
		if(!strcmp(czOwner, "sa"))	
            strcpy(czOwner, "dbo");


		retcode = SQLTables(m_hStmt, NULL,SQL_NTS, 
#ifdef ALL_TABLE
                            NULL, SQL_NTS, 
#else
                            (unsigned char*)czOwner, strlen(czOwner),
#endif
                            NULL,SQL_NTS, NULL,SQL_NTS);	
	}

    if(retcode == SQL_SUCCESS)	
    {
        m_nTableCount = 0;
        m_lstTableName.RemoveAll();
        while(FetchSQL() == SQL_SUCCESS)
        {
            strTable = GetColumnValue(3);
            if(strTable.GetLength !=0)
            {
                m_lstTableName.Add(strTable);
                m_nTableCount++;
            }
        }
    }
    else
    {
        DisplayError();
        return -1;
    }
    return m_nTableCount;
}

CString CSQLDirect::GetTableName(int nIndex)
{
	return m_lstTableName.GetAt(nIndex);
}

int CSQLDirect::OpenAllColumns(CString strLoginID, CString strTable)
{
	char czTable[100];
	SQLRETURN		returncode;
	unsigned char 	Statement[4096 + 1];
	char			szBuffer[8192+1];      // field name buffer
	SQLSMALLINT		SQLDataType, DecimalDigits, Nullable, szBufferLen;
	SQLUINTEGER		ColumnSize;
	int		i;
	short		nCol;

	int retcode = SQLAllocHandle(SQL_HANDLE_STMT, m_hDBC, &m_hStmt); 
	if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
	{
		DisplayError();
		return -1; // error returned 	
	}	
	sprintf(czTable, "%s", strTable);

	sprintf((char *)Statement, "select * from %s", czTable);
	
	returncode = SQLPrepare(m_hStmt, Statement, SQL_NTS);		
	if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
	{
		DisplayError();
		return -1;
	}
	
	SQLNumResultCols(m_hStmt, &nCol);

	if (nCol < 1)
	{
		DisplayError();
		return -1;
	}

	m_nColumnCount = 0;
	m_lstColumnName.RemoveAll();
	for (i = 1 ; i <= nCol; ++i)
	{
		SQLDescribeCol(m_hStmt, i, (unsigned char *)szBuffer, sizeof(szBuffer), 
			&szBufferLen, &SQLDataType, &ColumnSize, &DecimalDigits, &Nullable);
		
		m_lstColumnName.Add(szBuffer);
		m_nColumnCount++;
	}

	return m_nColumnCount;
}

CString CSQLDirect::GetColumnName(int nIndex)
{
	return m_lstColumnName.GetAt(nIndex);
}

