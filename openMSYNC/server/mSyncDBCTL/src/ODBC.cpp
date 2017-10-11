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

#include <windows.h>
#include <stdio.h>
#include "Sync.h"
#include "ODBC.h"

/*****************************************************************************/
//! ConnectToDB
/*****************************************************************************/
/*! \breif  DB에 connect 하는 부분
 ************************************
 * \param	Mode(in)	: ALL_HANDLES 인지 아닌지를 체크
 * \param	henv(out)	: ODBC environment handle
 * \param	hdbc(out)	: ODBC connection handle
 * \param	hstmt(out)	: ODBC statement handle(이 함수에서는 안쓰이는듯) (?)
 * \param	hflag(out)	: handle들의 상태(환경 변수가 설정여부, 연결여부 등등)
 * \param	dsn(in)		: data source	
 * \param	uid(in)		: db user's id
 * \param	passwd(in)	: db user's passwd
 ************************************
 * \return	int :  \n
 *			0 < return : Error \n
 *			0 = retrun : Success
 ************************************
 * \note	ODBC로 DB에 connect를 담당하는 함수로 environment handle, \n
 *			connection handle을 할당 받고 transaction auto commit을 off로 \n
 *			setting 한 뒤 DB에 connect(SQLConnect())한 결과를 return 한다. 
 *****************************************************************************/
int ConnectToDB(int Mode, SQLHENV *henv, SQLHDBC *hdbc, 
	    		SQLHSTMT *hstmt, int *hflag,
		    	SQLCHAR *dsn, SQLCHAR *uid, SQLCHAR *passwd)
{	
    SQLRETURN   retcode;
    // Allocate environment handle ==> henv 
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, henv);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
        return -1;  // error returned 
    
    *hflag = hENV;
    // Set the ODBC version environment attribute 
    retcode = 
        SQLSetEnvAttr(*henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
        return -1; // error returned 
    
    // Allocate connection handle ==> hdbc 
    retcode = SQLAllocHandle(SQL_HANDLE_DBC, *henv, hdbc); 
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) 
        return -1; // error returned 
    *hflag = *hflag | hDBC;
    
    // Set login timeout to 5 seconds. 
    retcode = SQLSetConnectAttr(*hdbc, SQL_ATTR_LOGIN_TIMEOUT, (void *)10, 0);
    
    if(Mode == ALL_HANDLES)
    {
        retcode = SQLSetConnectAttr(*hdbc, SQL_ATTR_AUTOCOMMIT, 
                                    (void *)SQL_AUTOCOMMIT_OFF, 0);
    }
    
    retcode = SQLSetConnectAttr(*hdbc, SQL_ATTR_CONNECTION_POOLING, 
                                (void *)SQL_CP_OFF, 0);    
    
    // Connect to data source 
    retcode = SQLConnect(*hdbc, dsn, SQL_NTS, uid, SQL_NTS, passwd, SQL_NTS);
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
    {
        SQLCHAR			SQLSTATE[6], Msg[256];
        SQLINTEGER		NativeError;
        SQLSMALLINT		MsgLen;
        
        Msg[0] = '\0';
        SQLSTATE[0] = '\0';
        SQLGetDiagRec(SQL_HANDLE_DBC, *hdbc, 1, SQLSTATE, &NativeError, 
            Msg, sizeof(Msg), &MsgLen);
        
        if (Msg[strlen((char *)Msg)-1]=='\n')
        {        
            Msg[strlen((char *)Msg)-1] = '\0';
        }
        
        ErrLog("SYSTEM;DBERROR[%d:%s]:%s;\n", retcode, SQLSTATE, Msg);	
        return -1; // error returned 
    }
    *hflag = *hflag | hCONNECT;

    return 0;	
}

/*****************************************************************************/
//! FreeHandle
/*****************************************************************************/
/*! \breif  DB와의 연결을 해제
 ************************************
 * \param	henv(in)	: ODBC environment handle
 * wparam	hdbc(in)	: ODBC connection handle
 * \param	hstmt(in)	: ?
 * \param	hflag(out)	: DB connection의 상태를 나타내는 플래그(ConnectToDB 참고)
 ************************************
 * \return	void
 ************************************
 * \note	Connect가 되었는지 확인하고 disconnect(SQLDisconnect()), \n
 *			connection handle, environment handle이 할당되었으면 해제한다.
 *****************************************************************************/
void FreeHandle(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt, int hflag)
{
    int ret;
    
    if((hflag&hCONNECT) == hCONNECT)
    {
        ret = SQLDisconnect(hdbc);    
        if(ret<0) ErrLog("SYSTEM;Disconnect error : %d;\n", ret);
    }
    
    if((hflag&hDBC) == hDBC)
    {
        ret = SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    }
    
    if((hflag&hENV) == hENV)
    {
        ret = SQLFreeHandle(SQL_HANDLE_ENV, henv);
    }
}

/*****************************************************************************/
//! FreeHandles
/*****************************************************************************/
/*! \breif  DB와의 연결을 해제
 ************************************
 * \param	flag(in)		: DB connection의 상태
 * \param	pOdbcParam(in)	: UserDB의 커넥션이 정의되어 있는 구조체
 ************************************
 * \return	void 
 ************************************
 * \note	UserDB가 open되어 있으면 userDB를 위한 각 handle들을 \n
 *			FreeHandle() 함수를 이용하여 해제하고 system DB를 위한 \n
 *			handle도 FreeHandle() 함수를 이용하여 해제한다
 *****************************************************************************/
void FreeHandles(int flag, ODBC_PARAMETER *pOdbcParam)
{
    int ret=0;	
    
    if(pOdbcParam->UserDBOpened)
    {
        ret = SQLEndTran(SQL_HANDLE_DBC, 
                         pOdbcParam->hdbc4Processing, SQL_COMMIT);
        if(ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
        {
            DisplayError(pOdbcParam->hdbc4Processing, 
                         pOdbcParam->uid, pOdbcParam->order, ret);
        }	
        
        FreeHandle(pOdbcParam->henv4Processing, pOdbcParam->hdbc4Processing, 
                   pOdbcParam->hstmt4Processing, pOdbcParam->hflag4Processing);
    }
    
    if(flag == ALL_HANDLES) 
    {
        FreeHandle(pOdbcParam->henv4System, pOdbcParam->hdbc4System, 
                   pOdbcParam->hstmt4System, pOdbcParam->hflag4System);
    }	
}

/*****************************************************************************/
//! AllocStmtHandle
/*****************************************************************************/
/*! \breif  SQL 문장을 처리하기 위해서 필요한 statement handle을 할당하는 함수
 ************************************
 * \param	pOdbcParam(out) : statement handle을 할당 받기 위한 구조체
 * \param	flag(in)		: 플래그
 ************************************
 * \return	int :  \n
 *			0 < return : Error \n
 *			0 = retrun : Success
 ************************************
 * \note	SQL 문장을 처리하기 위해서 필요한 statement handle을 할당하는 함수
 *****************************************************************************/
int	AllocStmtHandle(ODBC_PARAMETER *pOdbcParam, int flag)
{
    SQLRETURN returncode;
    if(flag==SYSTEM)
    {   // Allocate statement handle ==> hstmt
        returncode = SQLAllocHandle(SQL_HANDLE_STMT, 
            pOdbcParam->hdbc4System, &(pOdbcParam->hstmt4System)); 
        if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO) 
            return -1 ; // error returned 
    }
    else
    {	// Allocate statement handle ==> hstmt
        returncode = SQLAllocHandle(SQL_HANDLE_STMT, 
            pOdbcParam->hdbc4Processing, &(pOdbcParam->hstmt4Processing)); 
        if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO) 
            return -1 ; // error returned 
    }
    return 0;
}


/*****************************************************************************/
//! FreeStmtHandle
/*****************************************************************************/
/*! \breif  Statement handle을 해제하는 함수
 ************************************
 * \param	pOdbcParam(in) : statement handle을 해제 시키는 구조체
 * \param	flag(in)		: 플래그
  ************************************
 * \return	int :  \n
 *			0 < return : Error \n
 *			0 > retrun : Success
 ************************************
 * \note	Statement handle을 해제하는 함수
 *****************************************************************************/
void FreeStmtHandle(ODBC_PARAMETER *pOdbcParam, int flag)
{
	if(flag==SYSTEM)
		SQLFreeHandle(SQL_HANDLE_STMT, pOdbcParam->hstmt4System);
	else
		SQLFreeHandle(SQL_HANDLE_STMT, pOdbcParam->hstmt4Processing);
}

/*****************************************************************************/
//! OpenSystemDB
/*****************************************************************************/
/*! \breif  System DB에 connection을 맺음
 ************************************
 * \param	pOdbcParam(in) : statement handle을 해제 시키는 구조체
 * \param	flag(in)		: 플래그
 ************************************
 * \return	int :  \n
 *			0 < return : Error \n
 *			0 = retrun : Success
 ************************************
 * \note	ConnectToDB() 함수를 이용하여 적절한 파라미터를 전달해서\n
 *			System DB에 connection을 맺는다.
 *****************************************************************************/
int OpenSystemDB(ODBC_PARAMETER *pOdbcParam)
{	
    // Connect to System DB 로 연결 
    if(ConnectToDB(SYSTEM, &(pOdbcParam->henv4System), 
                   &(pOdbcParam->hdbc4System), &(pOdbcParam->hstmt4System), 
                   &(pOdbcParam->hflag4System), pOdbcParam->dbdsn, 
                   pOdbcParam->dbuid, pOdbcParam->dbpasswd) < 0)
    {
        ErrLog("SYSTEM [%d];Connect to System DB[%s] : FAIL...;\n", 
                pOdbcParam->order, pOdbcParam->dbdsn);
        return (-1);
    }
    return (0);
}

/*****************************************************************************/
//!  OpenUserDB 
/*****************************************************************************/
/*! \breif  User DB에 connection을 맺는 함수
 ************************************
 * \param	pOdbcParam(out)	: 
 * \param	dsn(in)			: data source	
 * \param	dsnuid(in)		: data source user's id
 * \param	dsnpasswd(in)	: data source user's passwd
 ************************************
 * \return	int :  \n
 *			0 < return : Error \n
 *			0 > retrun : Success
 ************************************
 * \note	ConnectToDB() 함수를 이용하여 적절한 파라미터를 전달해서 \n
 *			User DB에 connection을 맺는다.
 *****************************************************************************/
int	OpenUserDB(ODBC_PARAMETER *pOdbcParam, 
               SQLCHAR *dsn, SQLCHAR *dsnuid, SQLCHAR *dsnpasswd)
{
	
	if(ConnectToDB(ALL_HANDLES, &(pOdbcParam->henv4Processing), 
                   &(pOdbcParam->hdbc4Processing), 
                   &(pOdbcParam->hstmt4Processing), 
                   &(pOdbcParam->hflag4Processing), dsn, dsnuid, dsnpasswd)<0)
    {
		ErrLog("%d_%s;Connect to User DB[%s] : FAIL...;\n", 
            pOdbcParam->order, pOdbcParam->uid, dsn);
		return (-1);
	}	
	return 0;
}