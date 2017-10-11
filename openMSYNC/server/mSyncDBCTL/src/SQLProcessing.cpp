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

#include	<windows.h>
#include	<stdio.h>

#include	"Sync.h"
#include	"ODBC.h"
#include    <mbstring.h>


int	GetSyncTime(ODBC_PARAMETER *pOdbcParam, 
                char *ptr, int dataLength, int mode);

unsigned int  gl_MaxUserCount;

/*****************************************************************************/
//! delete_space
/*****************************************************************************/
/*! \breif  버퍼의 뒤에 나오는 space 값들을 삭제
 ************************************
 * \param	src(in)	: 문자열 
 * \param	len(in)	: 문자열의 길이 
 ************************************
 * \return  void
 ************************************
 * \note	버퍼의 뒤에 나오는 space 값들을 삭제한다. 
 *****************************************************************************/
void delete_space(char *src, int len)
{
    int	i = 0;
    
    if (src == NULL)	return;		
    if (strlen(src) > 0)
    {
        for (i = len-1 ; i > 0 && src[i] == ' ' ; i--);
            src[i+1] = '\0';
    }
}

/*****************************************************************************/
//! SetMaxUserCount
/*****************************************************************************/
/*! \breif  최대 사용자 수를 변경 시키는 함수  \n
 ************************************
 * \param	max : 변경하고자 하는 최대 사용자 수 
 ************************************
 * \return	void
 ************************************
 * \note	MaxUserCount의 값을 max로 변경 시킨다. \n
 *			Auth.ini에 지정해 놓은 최대 사용자 수 값을 setting 한다.
 *****************************************************************************/
void SetMaxUserCount(int max)
{
	gl_MaxUserCount = max;
}

/*****************************************************************************/
//! UpdateConnectFlag
/*****************************************************************************/
/*! \breif update connectionFlag, DBSyncTime  in openMSYNC_User table 
 ************************************
 * \param	pOdbcParam(in)	: User's DB Connection 정보가 들어 있는 필드 
 * \param	flag(in)		: User의 로그인 상태
 ************************************
 * \return  int : 0, -1
 ************************************
 * \note	Client가 connect, disconnect 될 때에 불려지는 함수로\n
 *			openMSYNC_User테이블의 ConnectionFlag 값을 CONNECT_STATE,\n
 *			DISCONNECT_ERROR_STATE, DISCONNECT_STATE 값으로 셋팅해준다. 
 *			정상적인 disconnect 상태인 경우 할당했던 메모리들도 반환한다.
 *****************************************************************************/
int	UpdateConnectFlag(ODBC_PARAMETER *pOdbcParam, int flag)
{
    SQLRETURN	returncode;
    SQLCHAR		Statement[QUERY_MAX_LEN + 1];
    int			ret = 0;
    
    if (AllocStmtHandle(pOdbcParam, SYSTEM) < 0)
        return (-1); 
    if (flag == CONNECT_STATE)
    {
        sprintf((char *)Statement, 
            "update openMSYNC_User set ConnectionFlag='%d' where UserID='%s'",
            flag, pOdbcParam->uid);
    }
    else if (flag == DISCONNECT_ERROR_STATE)
    {    
        sprintf((char *)Statement, 
            "update openMSYNC_User set ConnectionFlag='%d' where UserID='%s'",
            flag, pOdbcParam->uid);
    }
    else
    { // DISCONNECT_STATE인 경우 Last Sync time도 update
        SQLCHAR		buffer1[100];
        SQLCHAR		buffer2[100];
        
        memset(buffer1, 0x00, sizeof(buffer1));	
        memset(buffer2, 0x00, sizeof(buffer2));
        
        sprintf((char *)Statement, 
            "update openMSYNC_User set ConnectionFlag='%d' ", flag);

        if(pOdbcParam->syncFlag==ALL_SYNC || pOdbcParam->syncFlag==MOD_SYNC)
        {  
            if(pOdbcParam->mode & DBSYNC)			
            {
                sprintf((char *)buffer2, 
                        ", DBSyncTime='%s'", pOdbcParam->connectTime);
            }
        }
        
        sprintf((char *)Statement, "%s %s %s where UserID='%s'", 
            Statement, buffer1, buffer2, pOdbcParam->uid);
    }
    
    returncode = SQLExecDirect(pOdbcParam->hstmt4System, Statement, SQL_NTS);
    if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
    {
        DisplayError(pOdbcParam->hstmt4System, 
                     pOdbcParam->uid, pOdbcParam->order, returncode);
        ret = -1;
    }
    FreeStmtHandle(pOdbcParam, SYSTEM);
    
    if(flag != CONNECT_STATE)
    {
        if(pOdbcParam->connectTime) 
        {
            free(pOdbcParam->connectTime);
            pOdbcParam->connectTime = NULL;
        }
        if(pOdbcParam->lastSyncTime)	
        {
            free(pOdbcParam->lastSyncTime);
            pOdbcParam->lastSyncTime = NULL;
        }
    }
    return (ret);
}

/*****************************************************************************/
//! CheckCurrentUserCount 
/*****************************************************************************/
/*! \breif 현재 connection user의 개수 체크하여 인증 가능 여부 
 ************************************
 * \param pOdbcParam(in)	: DB 관련 정보
 ************************************
 * \return  int : \n
 *			0 < error \n
 *			0 >= success \n
 *			EXCESS_USER 초과시, 0, -1
 ************************************
 * \note	openMSYNC_User 테이블에 등록되어 있는 사용자 수를 MaxUser 값과 \n
 *			비교하여 현재 등록되어 있는 사용자가 그 값을 초과하면 \n
 *			EXCESS_USER를, 아닐 경우 0을 return 한다. \n
 *			알고리즘\n
 *			1. connected user의 개수를 읽어 오는 SQL 문장 실행 \n
 *			2. column Binding\n
 *****************************************************************************/
int	 CheckCurrentUserCount(ODBC_PARAMETER *pOdbcParam)
{
    SQLUINTEGER		DCount;
    SQLINTEGER		DCountLenOrInd;		
    SQLRETURN		returncode;
    SQLCHAR			Statement[QUERY_MAX_LEN + 1];
    int				ret = 0;
    
    if (AllocStmtHandle(pOdbcParam, SYSTEM) < 0)
        return (-1);
    
    // 1. connected user의 개수를 읽어 오는 SQL 문장 실행    
    sprintf((char *)Statement,"select count(*) from openMSYNC_User");
    returncode = SQLExecDirect(pOdbcParam->hstmt4System, Statement, SQL_NTS);
    if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
    {
        DisplayError(pOdbcParam->hstmt4System, 
                     pOdbcParam->uid, pOdbcParam->order, returncode);
        ret = -1;
        goto CheckCount_END;
    }
    // 2. column Binding
    SQLBindCol(pOdbcParam->hstmt4System, 
               1, SQL_C_ULONG, &DCount, 0, &DCountLenOrInd);
    
    // 3. SQL Fetch
    returncode = SQLFetch(pOdbcParam->hstmt4System);
    // SQL_ERROR면 return -1
    if( returncode != SQL_SUCCESS && 
        returncode != SQL_SUCCESS_WITH_INFO && 
        returncode != SQL_NO_DATA )
    {
        DisplayError(pOdbcParam->hstmt4System, 
                     pOdbcParam->uid, pOdbcParam->order, returncode);
        ret = -1;
        goto CheckCount_END;
    }	

    // 4. count 비교    
    if (DCount > gl_MaxUserCount)
    {
        ErrLog("SYSTEM [%d];Allowed maximun users = %d;\n", 
                pOdbcParam->order, gl_MaxUserCount);
        ret = EXCESS_USER;
    }

CheckCount_END:
    FreeStmtHandle(pOdbcParam, SYSTEM);
    return (ret);
}

/*****************************************************************************/
//! CheckUserInfo 
/*****************************************************************************/  
/*! \breif  LD_USER table의 password와 비교하여 사용자 인증 
 ************************************
 * \param	pOdbcParam(in)	: DB 관련 정보(uid, passwd)
 * \param	passwd(in)		: user's passwd 
 ************************************
 * \return  return -1, 0, NO_USER, INVALID_USER
 ************************************
 * \note	Connect한 client의 userID가 openMSYNC_User 테이블에 등록되어 있는지\n 
 *			확인하고 등록된 사용자인 경우 password가 입력받은 값과 같은지\n
 *			인증하는 작업을 수행한다. 등록되어 있지 않은 사용자인 경우\n
 *			NO_USER를 return 하고 password 값이 잘못된 경우 INVALID_USER를\n
 *			return 하며 정상적인 경우 0을 return 해준다.
 *****************************************************************************/
int	CheckUserInfo(ODBC_PARAMETER *pOdbcParam, SQLCHAR *passwd)
{
    SQLCHAR			DPasswd[PASSWD_LEN + 1];
    SQLINTEGER		DPasswdLenOrInd, DDBSyncTimeOrInd;
    SQLRETURN		returncode;
    SQLCHAR			Statement[QUERY_MAX_LEN + 1];
    char			szBuffer[FIELD_LEN+1];      // field name buffer
    SQLSMALLINT		SQLDataType, DecimalDigits, Nullable, szBufferLen;
    SQLUINTEGER		ColumnSize;
    int		ret = 0;
    
    if (AllocStmtHandle(pOdbcParam, SYSTEM) < 0)	
        return (-1);
    
    // 1. userID의 password와 DBSyncTime 읽어오는 SQL 문장 실행
    sprintf((char *)Statement,
        "select UserPwd, DBSyncTime  from openMSYNC_User where UserID='%s'", 
        pOdbcParam->uid);
        
    returncode = SQLExecDirect(pOdbcParam->hstmt4System, Statement, SQL_NTS);
    if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
    {
        DisplayError(pOdbcParam->hstmt4System, 
                     pOdbcParam->uid, pOdbcParam->order, returncode);
        ret = -1;
        goto CheckUser_END;
    }
    
    // 2. column Binding
    // password binding
    SQLBindCol(pOdbcParam->hstmt4System, 
               1, SQL_C_CHAR, DPasswd , PASSWD_LEN + 1, &DPasswdLenOrInd);
    SQLDescribeCol(pOdbcParam->hstmt4System, 
                   2, (unsigned char *)szBuffer, sizeof(szBuffer), &szBufferLen,
                   &SQLDataType, &ColumnSize, &DecimalDigits, &Nullable);
    
    pOdbcParam->lastSyncTime = NULL;
    pOdbcParam->lastSyncTime = (char *)calloc(ColumnSize+4, sizeof(char));    
    if(pOdbcParam->lastSyncTime==NULL)
    {
        ErrLog("SYSTEM [%d];Memory Allocation Error : size = %dbytes;\n",
               pOdbcParam->order, ColumnSize+4);
        ret = -1;
        goto CheckUser_END;
    }

    SQLBindCol(pOdbcParam->hstmt4System, 
               2, SQL_C_CHAR, pOdbcParam->lastSyncTime, 
               (ColumnSize+1)*sizeof(char), &DDBSyncTimeOrInd);
    
    // 3. SQL Fetch
    returncode = SQLFetch(pOdbcParam->hstmt4System);
    // SQL_ERROR면 return -1
    if( returncode != SQL_SUCCESS && 
        returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
    {
        DisplayError(pOdbcParam->hstmt4System,
            pOdbcParam->uid, pOdbcParam->order, returncode);
        ret = -1;
        goto CheckUser_END;
    }	
    SQLCloseCursor(pOdbcParam->hstmt4System);
    
    // openMSYNC_User table에 없는 user
    if (returncode == SQL_NO_DATA) 
    {
        ret = NO_USER;
        goto CheckUser_END;
    }
    delete_space((char *)DPasswd, DPasswdLenOrInd);	
    
    // 4. password 비교
    if (strcmp((char *)passwd, (char *)DPasswd))
        ret = INVALID_USER;		
    
CheckUser_END:
    FreeStmtHandle(pOdbcParam, SYSTEM);
    return ret;
}

/*****************************************************************************/
//! GetVersionInfo 
 /*****************************************************************************/
/*! \breif  openMSYNC_Version table에서 version or VersionID를 fetch\n
 *			2003. 2. 18
 ************************************
 * \param	pOdbcParam(in)	: DB 관련 정보
 * \param	Statement(in)	: versionID나 version 값을 가져오는 query문
 * \param	newVersion(out)	: versionID나 version
 ************************************
 * \return  int \n
 *			0 < error \n
 *			0 >= sucess \n
 *			return : 0, -1, SQL_NO_DATA
 ************************************
 * \note	미리 설정해 놓은 Statement 문장을 수행하여 versionID나 version\n
 *			값을 검색하여 newVersion 변수에 저장해준다. 해당 version 정보가 
 *			없는 경우 SQL_NO_DATA를 return 해준다.\n
 *			CheckVersion() 함수에서 세번 호출된다.
 *			내부 알고리즘\n
 *			openMSYNC_Version table에서 version or versionID를 fetch
 *****************************************************************************/
int GetVersionInfo(ODBC_PARAMETER *pOdbcParam, 
                   SQLCHAR *Statement, SQLUINTEGER *newVersion)
{
    SQLRETURN		returncode;
    SQLINTEGER		newVersionLenOrInd;	
    
    // 1. 해당 statement 실행
    returncode = SQLExecDirect(pOdbcParam->hstmt4System, Statement, SQL_NTS);
    if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
    {
        DisplayError(pOdbcParam->hstmt4System, 
            pOdbcParam->uid, pOdbcParam->order, returncode);
        return (-1);
    }
    // 2. column Binding
    SQLBindCol(pOdbcParam->hstmt4System, 
        1, SQL_C_ULONG, newVersion , 0, &newVersionLenOrInd);	
    
    // 3. SQL Fetch
    returncode = SQLFetch(pOdbcParam->hstmt4System) ;	
    // SQL_ERROR면 return -1
    if(returncode != SQL_SUCCESS && 
        returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
    {
        DisplayError(pOdbcParam->hstmt4System, 
            pOdbcParam->uid, pOdbcParam->order, returncode);
        
        return (-1);
    }	
    
    SQLCloseCursor(pOdbcParam->hstmt4System);
    // record가 없는 경우 max()는 NULL이지 SQL_NO_DATA가 return 되지 않는다.
    if (returncode == SQL_NO_DATA || newVersionLenOrInd == SQL_NULL_DATA)
    { 
        return (SQL_NO_DATA);
    }	
    return 0;
}

/*****************************************************************************/
//! CheckVersion 
 /*****************************************************************************/
/*! \breif	버젼 검사
 ************************************
 * \param	pOdbcParam(in,out)	: pOdbcParam->versionID(out)
 * \param	application(in)		: 
 * \param	version(in)			 :
 ************************************
 * \return	<0, on error
			>0, on versionID를 찾은 경우 new version ID
			=0, upgrade할 version이 없는 경우
 ************************************
 * \note	Parameter로 받은 application, version에 대한 검사를 하는 함수로\n
 *			먼저 versionID를 검색하고 해당 version 보다 높은 버전이 등록되어\n
 *			있는지 확인하여 높은 버전이 있으면 그 버전의 versionID를 검색하여\n
 *			pOdbcParam->newVersionID에 저장해준다. \n
 *			Upgrade할 version이 없는 경우 0이 return 되고 upgrade할 version이\n
 *			등록되어 있으면 그 version을 return하므로 >0값이 된다.
 *			- application, version의 versionID를 구하고 현재 application의 보다\n
 *			  높은 version이 등록되어 있는지 여부 확인
 *****************************************************************************/
int	CheckVersion(ODBC_PARAMETER *pOdbcParam, SQLCHAR *application, int version)
{
    SQLRETURN		returncode;
    SQLUINTEGER		newVersion;
    SQLCHAR			Statement[QUERY_MAX_LEN + 1];
    int				ret = 0;
    
    if (AllocStmtHandle(pOdbcParam, SYSTEM) < 0)
        return (-1);
    
    // 1. versionID 구하는 SQL 문장 실행
    sprintf((char *)Statement,
            "select VersionID from openMSYNC_Version "
            "where Application = '%s' and Version = %d", 
            application, version);
    if((returncode = GetVersionInfo(pOdbcParam, Statement, &newVersion))<0 ||
        returncode == SQL_NO_DATA)
    {
        if(returncode == SQL_NO_DATA)
            ErrLog("SYSTEM [%d];Application '%s'의 버전 '%d'에" 
                   "대한 정보를 찾을 수 없습니다. ;\n", 
                   pOdbcParam->order, application, version);
        ret = -1;
        goto CheckVersion_END;
    }	
    pOdbcParam->VersionID = newVersion;
    
    // 2. 보다 높은 version이 있는지 구하는 SQL 문장 실행
    sprintf((char *)Statement,
            "select max(Version) from openMSYNC_Version"
            " where Application = '%s' and Version > %d", 
            application, version);
    if((returncode = GetVersionInfo(pOdbcParam, Statement, &newVersion)) <0)
    {
        ret = -1;
        goto CheckVersion_END;
    }	
    // 없으면 0을 return
    if(returncode == SQL_NO_DATA)		goto CheckVersion_END;
    
    // 3. 보다 높은 version의 versionID 구하는 SQL 문장 실행
    sprintf((char *)Statement,
            "select VersionID from openMSYNC_Version"
            "where application = '%s' and version = %d", 
            application, newVersion);
    ret = newVersion;
    if((returncode = GetVersionInfo(pOdbcParam, Statement, &newVersion)) <0 ||
       returncode == SQL_NO_DATA)
    {
        if(returncode == SQL_NO_DATA)
            ErrLog("SYSTEM [%d];업그레이드를 위한 Application"
                   "'%s'의 버전 '%d'에 대한 정보를 찾을 수 없습니다.;\n", 
                   pOdbcParam->order, application, newVersion);
        ret = -1;
        goto CheckVersion_END;
    }	

    // 4. 새로운 version을 return하고 versionID를 저장
    pOdbcParam->newVersionID = newVersion;
    
CheckVersion_END:
    FreeStmtHandle(pOdbcParam, SYSTEM);
    return (ret);
}

/*****************************************************************************/
//! GetDBConnectTime 
 /*****************************************************************************/
/*! \breif	2003.2.18 fileTime과 dbConnectionTime을 모두 지원\n
 *			DBSYNC인 경우 pOdbcParam->connectTime, FILESYNC인 경우 pOdbcParam->fileTime
 ************************************
 * \param	pOdbcParam(out)	: pOdbcParam->versionID
 * \param	hstmt(in)		: 
 * \param	flag(in)		:
 * \param	connectTime(in)	:
 ************************************
 * \return	<0, on error \n
 *			>0, on versionID를 찾은 경우 new version ID \n
 *			=0, upgrade할 version이 없는 경우 \n
 ************************************
 * \note	현재 DB의 system time을 구하는 함수로 각 데이터베이스에 맞는 \n
 *			문장을 실행하여 현재 시간을 구한다. 시간을 저장할 버퍼에\n
 *			메모리를 할당한 뒤 '9999-99-99'로 초기화해주고 \n
 *			여러 DB(동일 데이터베이스 시스템의 여러DSN)를 접속해서 동기화를\n
 *			하는 경우 각 DB의 system time이 동일하지 않을 수도 있으므로 각\n
 *			DB의 system time 중에 가장 작은 값을 유지할 수 있도록 새로 구한\n
 *			값을 무조건 저장하는 것이 아니라 비교하여 오래된 값으로 저장한다.\n
 *			이는 이 값이 동기화 과정이 끝난 뒤에 이전 동기화 시간으로\n
 *			저장되는데 이후 동기화 요청 시, 변경된 데이터만 동기화 하기\n
 *			위한 검색 조건에 쓰이기 때문이다.
 *****************************************************************************/
int	GetDBConnectTime(ODBC_PARAMETER *pOdbcParam, SQLHSTMT *hstmt, int flag, char **connectTime)
{
    SQLINTEGER		DConnectTimeOrInd;		
    SQLRETURN		returncode;
    SQLCHAR			Statement[QUERY_MAX_LEN + 1];
    char			szBuffer[FIELD_LEN+1];      // field name buffer
    SQLSMALLINT		SQLDataType, DecimalDigits, Nullable, szBufferLen;
    SQLUINTEGER		ColumnSize;
    char			*date = NULL;
    int				ret = 0;
    
    if (AllocStmtHandle(pOdbcParam, flag) < 0) 
        return (-1);
    
    // 1. 현재 시간을 읽어오는 SQL 문장 실행
    if(pOdbcParam->dbType == DBSRV_TYPE_ORACLE)		
        sprintf((char *)Statement,"select SYSDATE from dual");
    else if(pOdbcParam->dbType == DBSRV_TYPE_MSSQL || 
            pOdbcParam->dbType == DBSRV_TYPE_SYBASE )
        sprintf((char *)Statement,"select getdate()");		
    else if(pOdbcParam->dbType == DBSRV_TYPE_MYSQL)
        sprintf((char *)Statement,"select sysdate()");		
    else if(pOdbcParam->dbType == DBSRV_TYPE_ACCESS)
        sprintf((char *)Statement,"select Now()");	
    else if(pOdbcParam->dbType == DBSRV_TYPE_CUBRID)
        sprintf((char *)Statement,"select SYSDATE");	
    else if(pOdbcParam->dbType == DBSRV_TYPE_DB2)
        sprintf((char *)Statement,"SELECT CURRENT DATE FROM SYSIBM.SYSDUMMY1");
    
    returncode = SQLExecDirect(*hstmt, Statement, SQL_NTS);
    if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
    {
        DisplayError(*hstmt, pOdbcParam->uid, pOdbcParam->order, returncode);
        ret = -1;
        goto GetDBTime_END2;
    }

    // 2. column Binding
    SQLDescribeCol(*hstmt, 
                   1, (unsigned char *)szBuffer, sizeof(szBuffer), &szBufferLen,
                   &SQLDataType, &ColumnSize, &DecimalDigits, &Nullable);
    if(*connectTime == NULL)
    {
        *connectTime = (char *)calloc(ColumnSize+1, sizeof(char));		
        if (*connectTime==NULL)
        {
            ErrLog("SYSTEM [%d];Memory Allocation Error : size = %dbytes;\n", 
                pOdbcParam->order, ColumnSize+1);
            ret = -1;
            goto GetDBTime_END2;
        }
        sprintf(*connectTime, "9999-99-99");
    }
    
    date = (char *)calloc(ColumnSize+1, sizeof(char));
    if (date==NULL)
    {
        ErrLog("SYSTEM [%d];Memory Allocation Error : size = %dbytes;\n", 
            pOdbcParam->order, ColumnSize+1);
        ret = -1;
        goto GetDBTime_END2;
    }	
    SQLBindCol(*hstmt, 
        1, SQL_C_CHAR, date, (ColumnSize+1)*sizeof(char), &DConnectTimeOrInd);
    // 3. SQL Fetch
    returncode = SQLFetch(*hstmt) ;	
    // SQL_ERROR면 return -1
    if( returncode != SQL_SUCCESS && 
        returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
    {
        DisplayError(*hstmt, pOdbcParam->uid, pOdbcParam->order, returncode);
        
        ret = -1;
        goto GetDBTime_END1;
    }	
    if(strcmp(*connectTime, date)>0)
        strcpy(*connectTime, date);
    
GetDBTime_END1:
    free(date);	
GetDBTime_END2:
    FreeStmtHandle(pOdbcParam, flag);
    return (ret);
}

/*****************************************************************************/
//! GetStatement 
 /*****************************************************************************/
/*! \breif	해당 event의 statement를 구하는 함수
 ************************************
 * \param	pOdbcParam(in)	: pOdbcParam->(tableName, versionID, event)
 * \param	Statement(out)	: 해당 이벤트의 statement
 ************************************
 * \return	<0, on error
 *			>0, on versionID를 찾은 경우 new version ID
 *			=0, upgrade할 version이 없는 경우
 *			statement가 없는 경우 SQL_NO_DATA를 return해준다.
 ************************************
 * \note	해당 event의 statement를 구하는 함수로 statement가 없는 경우\n
 *			SQL_NO_DATA를 return해준다.
 *****************************************************************************/
int	GetStatement(ODBC_PARAMETER *pOdbcParam, SQLCHAR *Statement)
{
    SQLCHAR		Query[QUERY_MAX_LEN + 1];
    char		buff[QUERY_MAX_LEN + 1];
    SQLRETURN	returncode;
    int			RowCount=0;
    SDWORD		StmtLen;
    int			ret = 0;
    
    if (AllocStmtHandle(pOdbcParam, SYSTEM) < 0)	
        return	(-1);
    
    sprintf((char *)Query, "SELECT Script ");
    strcat((char *)Query, "FROM openMSYNC_Script S, openMSYNC_Table T ");
    strcat((char *)Query, "WHERE T.TableID = S.TableID ");
    if( pOdbcParam->dbType == DBSRV_TYPE_ORACLE || 
        pOdbcParam->dbType == DBSRV_TYPE_DB2 )
    {   // upper case로 바꾸되 기존 값은 그대로 유지
        char *tableUp;
        tableUp = _strupr(_strdup(pOdbcParam->tableName));
        sprintf((char *)buff, "AND T.CliTableName ='%s' ", tableUp);
        free(tableUp);
    }
    else
    {
        sprintf((char *)buff, 
            "AND T.CliTableName ='%s' ", pOdbcParam->tableName);
    }    
    strcat((char *)Query, buff);
    
    sprintf((char *)buff, "AND T.DBID = %d ", pOdbcParam->DBID);
    strcat((char *)Query, buff);
    
    if(pOdbcParam->dbType == DBSRV_TYPE_SYBASE)
        sprintf((char *)buff, "AND S.Event = upper('%c')", pOdbcParam->event);
    else
        sprintf((char *)buff, "AND S.Event = '%c'", pOdbcParam->event);

    strcat((char *)Query, buff);
    
    // 쿼리문 소문자로             //
    returncode = SQLExecDirect(pOdbcParam->hstmt4System, Query, SQL_NTS);		
    if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
    {
        DisplayError(pOdbcParam->hstmt4System,
                     pOdbcParam->uid, pOdbcParam->order, returncode);
        ret = -1;
        goto GetStmt_END;
    }
    SQLBindCol(pOdbcParam->hstmt4System, 
               1, SQL_C_CHAR, Statement, SCRIPT_LEN+1, &StmtLen);
    
    returncode = SQLFetch(pOdbcParam->hstmt4System) ;	
    if (returncode != SQL_SUCCESS && 
        returncode != SQL_SUCCESS_WITH_INFO && returncode!=SQL_NO_DATA)
    {
        DisplayError(pOdbcParam->hstmt4System,
                     pOdbcParam->uid, pOdbcParam->order, returncode);
        
        ret = -1;
        goto GetStmt_END;
    }
    /* 해당 script가 없는 경우 */
    if (returncode == SQL_NO_DATA) 
        ret = SQL_NO_DATA;
    
GetStmt_END:
    FreeStmtHandle(pOdbcParam, SYSTEM);
    return	(ret);	
}

/*****************************************************************************/
//! BindColumns 
/*****************************************************************************/
/*! \breif	해당 event의 statement를 구하는 함수
 ************************************
 * \param	pOdbcParam(in)	: pOdbcParam->(tableName, versionID, event)
 * \param	Statement(out)	: 해당 이벤트의 statement
 ************************************
 * \return	<0, on error
 *			>0, on versionID를 찾은 경우 new version ID
 *			=0, upgrade할 version이 없는 경우
 ************************************
 * \note	Select a, b, c from table; 문장으로 데이터를 fetch해올 때\n
 *			a, b, c 필드의 값(column)을 저장할 변수가 필요하다. \n
 *			Fetch 실행에 앞서 스키마 정보를 이용하여 이 필드의 type을 \n
 *			구해서(SQLDescribeCol()) 적절한 크기의 메모리를 할당한 뒤에\n
 *			그 column에 bind 시켜준다(SQLBindCol()).
 *****************************************************************************/
int	BindColumns(SQLHSTMT hstmt, ODBC_PARAMETER *pOdbcParam)
{
    char			szBuffer[FIELD_LEN+1];      // field name buffer
    SQLSMALLINT		i, SQLDataType, DecimalDigits, Nullable, szBufferLen;
    SQLUINTEGER		ColumnSize;
    SQLRETURN		returncode;
    
    pOdbcParam->ColumnArray = 
        (SQLPOINTER *) malloc(pOdbcParam->NumCols * sizeof(SQLPOINTER));
    pOdbcParam->ColumnLenArray = 
        (SQLINTEGER *) malloc(pOdbcParam->NumCols * sizeof(SQLINTEGER));
    pOdbcParam->ColumnLenOrIndArray = 
        (SQLINTEGER *) malloc(pOdbcParam->NumCols * sizeof(SQLINTEGER));
    
    for (i = 0 ; i < pOdbcParam->NumCols; i++) 
        pOdbcParam->ColumnArray[i] = NULL;
    
    for (i = 0 ; i < pOdbcParam->NumCols; i++) 
    {
        returncode = SQLDescribeCol(hstmt, 
                                    i + 1, 
                                    (unsigned char *)szBuffer, 
                                    sizeof(szBuffer), 
                                    &szBufferLen, 
                                    &SQLDataType, 
                                    &ColumnSize, 
                                    &DecimalDigits, 
                                    &Nullable);
        
        if (returncode != SQL_SUCCESS && 
            returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
        {
            DisplayError(hstmt, pOdbcParam->uid, pOdbcParam->order, returncode);
            return	-1;
        }
        
        // TODO: 
        //   임시로 varchar, char 타입의 필드의 binding 버퍼의 크기로 
        //   TEXT_TYPE_LEN 으로 사용, 적절한 크기로 수정 필요 함.
        if (SQLDataType == SQL_LONGVARCHAR || SQLDataType == SQL_WLONGVARCHAR)
            ColumnSize = TEXT_TYPE_LEN;        
        else if (SQLDataType == SQL_TYPE_TIMESTAMP)
            ColumnSize = TIMESTAMP_TYPE_LEN;

        // TODO:
        //   ColumnSize와 DataType으로부터 적절한 char buffer의 크기를 구한다.
        /*
        if( SQLDataType == SQL_WCHAR || SQLDataType == SQL_WVARCHAR )
        {
            pOdbcParam->ColumnLenArray[i] = (ColumnSize+1)*sizeof(SQLWCHAR);
        }
        else
        {
            ;
        }
        */
        pOdbcParam->ColumnLenArray[i] = ColumnSize*2+4;
        
        pOdbcParam->ColumnArray[i] = 
            (SQLPOINTER *)calloc(pOdbcParam->ColumnLenArray[i], sizeof(char));
        
        if(pOdbcParam->ColumnArray[i]==NULL)
        {
            ErrLog("%d_%s;Memory Allocation Error : size = %dbytes;\n", 
                   pOdbcParam->order, pOdbcParam->uid, ColumnSize+4);
            return	-1;
        }
        
        // SQL 실행 후 결과셋을 받아올 변수를 준비하여 결과값과 binding
        // TODO:
        //   Stored Procedure 사용시 4번째 인자인 결과값을 저장할 버퍼의
        //   크키 설정에 에러가 발생 수정 필요.
        returncode = SQLBindCol(hstmt, 
                                i + 1, 
                                SQL_C_CHAR, 
                                pOdbcParam->ColumnArray[i], 
                                pOdbcParam->ColumnLenArray[i], 
                                &(pOdbcParam->ColumnLenOrIndArray[i]));
                
        if(returncode != SQL_SUCCESS && 
           returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
        {
            DisplayError(hstmt, pOdbcParam->uid, pOdbcParam->order, returncode);
            return	-1;
        }
    }
    
    return	0;
}

/*****************************************************************************/
//! BindParameters 
/*****************************************************************************/
/*! \breif	해당 event의 statement를 구하는 함수
 ************************************
 * \param	pOdbcParam(in)	: pOdbcParam->(tableName, versionID, event)
 * \param	Statement(out)	: 해당 이벤트의 statement
 ************************************
 * \return	<0, on error
 *			>0, on versionID를 찾은 경우 new version ID
 *			=0, upgrade할 version이 없는 경우
 ************************************
 * \note	Select a from table where b=? and c=? 문장이나\n
 *			insert into table(b, c) values(?, ?) 문장을 실행할 때 b, c에 \n
 *			해당하는 값들을 parameter라 한다. Column과 마찬가지로 이 값을 \n
 *			저장해 둔 변수가 필요하며 sql 문장을 prepare 하고 반복 실행할 \n
 *			때에 이 변수 값만 바꿔 줄 수 있도록 bind 시켜둘 필요가 있다. 
 *			BindColumns()와 마찬가지로 SQLDescribeParam() 함수를 이용하여\n
 *			파라미터의 적절한 크기를 구해 메모리를 할당하고 SQLBindParameter() 
 *			함수를 이용하여 bind 시켜준다.
 *****************************************************************************/
int	BindParameters(SQLHSTMT hstmt, ODBC_PARAMETER *pOdbcParam)
{
    SQLSMALLINT	i;	
    SQLSMALLINT	SQLDataType, DecimalDigits, Nullable;
    SQLUINTEGER	ParamSize;
    SQLRETURN	returncode;
    
    pOdbcParam->ParamArray = 
        (SQLPOINTER *) malloc(pOdbcParam->NumParams * sizeof(SQLPOINTER));
    pOdbcParam->ParamLenArray = 
        (SQLINTEGER *) malloc(pOdbcParam->NumParams * sizeof(SQLINTEGER));
    pOdbcParam->ParamLenOrIndArray = 
        (SQLINTEGER *) malloc(pOdbcParam->NumParams * sizeof(SQLINTEGER));
    
    for(i=0 ; i<pOdbcParam->NumParams; i++) 
        pOdbcParam->ParamArray[i] = NULL;
    
    for (i = 0; i < pOdbcParam->NumParams; i++) 
    {
        // Describe the parameter.        
        returncode = SQLDescribeParam(hstmt, 
                                      i + 1, 
                                      &SQLDataType, 
                                      &ParamSize, 
                                      &DecimalDigits, 
                                      &Nullable);
        
        if (returncode != SQL_SUCCESS && 
            returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
        {
            DisplayError(hstmt, pOdbcParam->uid, pOdbcParam->order, returncode);
            return	-1;
        }
        
        if (SQLDataType == SQL_LONGVARCHAR || SQLDataType == SQL_WLONGVARCHAR)
            ParamSize = TEXT_TYPE_LEN;
        else if (SQLDataType == SQL_TYPE_TIMESTAMP)
        {            
            // 서버 데이터의 type이 DATE인 경우
            // SQLDescribeParam후에 조건절(where절)의 경우 VARCHAR로 설정되지만 
            // 나머지의 경우 SQL_TYPE_TIMESTAMP으로 설정됨으로 해서 
            // script에 DATE type인 경우 to_date API를 이용해서 처리해야 함.
            if(pOdbcParam->dbType == DBSRV_TYPE_ORACLE)
            {
                ParamSize = 999;
                SQLDataType = SQL_VARCHAR;
            }
            else
                ParamSize = TIMESTAMP_TYPE_LEN;
        }
		else if (pOdbcParam->dbType == DBSRV_TYPE_SYBASE)
		{
			if (SQLDataType == SQL_NTS)
			{
				ParamSize = 255;
			}
		}
                
        if( SQLDataType == SQL_WCHAR || SQLDataType == SQL_WVARCHAR )
        {
            pOdbcParam->ParamArray[i] = 
                (SQLPOINTER)calloc(ParamSize+1, sizeof(SQLWCHAR));        
            pOdbcParam->ParamLenArray[i] = ((ParamSize + 1)*sizeof(SQLWCHAR));
        }
        else
        {
            pOdbcParam->ParamArray[i] = 
                (SQLPOINTER)calloc(ParamSize+1, sizeof(char));
            pOdbcParam->ParamLenArray[i] = ParamSize + 1;
        }
                
        if (pOdbcParam->ParamArray[i]==NULL)
        {	
            ErrLog("%d_%s;Memory Allocation Error : size = %dbytes;\n", 
                   pOdbcParam->order, pOdbcParam->uid, ParamSize+1);			
            return	-1;
        }

        pOdbcParam->ParamLenOrIndArray[i] = SQL_NTS;

        // Bind the memory to the parameter.         
        returncode = SQLBindParameter(hstmt, 
                                        i + 1, 
                                        SQL_PARAM_INPUT, 
                                        SQL_C_CHAR, 
                                        SQLDataType, 
                                        ParamSize,	
                                        DecimalDigits, 
                                        pOdbcParam->ParamArray[i], 
                                        pOdbcParam->ParamLenArray[i], 
                                        &(pOdbcParam->ParamLenOrIndArray[i]));
        
        if (returncode != SQL_SUCCESS && 
            returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
        {
            DisplayError(hstmt, pOdbcParam->uid, pOdbcParam->order, returncode);
            return	-1;
        }
    }
    
    return	0;

}

/*****************************************************************************/
//! PrepareStmt
/*****************************************************************************/
/*! \breif	SQL 문장 실행하기 전에 prepare를 처리해주는 함수
 ************************************
 * \param	hstmt(in)		:	
 * \param	pOdbcParam(out)	: pOdbcParam->(Column, Param) Array binding
 * \param	Statement(in)	:
 ************************************
 * \return	<0, on error
 *			>0, on versionID를 찾은 경우 new version ID
 ************************************
 * \note	SQL 문장을 실행하기에 앞서 prepare 해주는 함수로 parameter나 \n
 *			column이 있는 경우 그에 대한 binding 처리까지 해준다.
 *****************************************************************************/
int	PrepareStmt(SQLHSTMT hstmt, ODBC_PARAMETER *pOdbcParam, SQLCHAR *Statement)
{
    SQLRETURN	returncode;
    
    returncode = SQLPrepare(hstmt, Statement, SQL_NTS);
    if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
    {
        DisplayError(hstmt, pOdbcParam->uid, pOdbcParam->order, returncode);
        return	-1;
    }
    
    SQLNumParams(hstmt, &(pOdbcParam->NumParams));
    if (pOdbcParam->NumParams > 0)
    {
        if(BindParameters(hstmt, pOdbcParam) < 0)	
            return	-1;
    }
        
    // mySQL인 경우 select 외에 BindColumn 하면 auto_increment가 있을 경우 
    // 하나의 row가 생긴다. 따라서 download 시에만 BindColumn이 필요하므로
    // select 문일 경우에만 호출하도 한다.
    // stored procedure를 수행하기 위해서 {를 추가
    if(!strnicmp((char *)Statement, "select", 6) || 
       !strncmp((char *)Statement, "{", 1)          )
    {
        SQLNumResultCols(hstmt, &(pOdbcParam->NumCols));
        if (pOdbcParam->NumCols > 0) 
        {
            if (BindColumns(hstmt, pOdbcParam) < 0)	
                return -1;
        }
    }
    else 
        pOdbcParam->NumCols = 0;

    return 0;
}

/*****************************************************************************/
//! GetParameters
/*****************************************************************************/
/*! \breif	delimeter에 맞춰 field를 parsing하고 pOdbcParam->ParamArray[]에 넣어준다.
 ************************************
 * \param	pOdbcParam(out)	: pOdbcParam->(Column, Param) Array binding
 * \param	ptr(in)			: 하나의 문장
 * \param	dataLength(in)	:	
 * \param	gnIdx(in)		:
 ************************************
 * \return	<0, on error
 *			>0, on versionID를 찾은 경우 new version ID
 ************************************
 * \note	Client로부터 upload 받은 data stream으로부터 FIELD_DEL과 \n
 *			RECORD_DEL을 이용하여 각 필드 값을 parsing 해주는 함수로 parsing\n
 *			한 각 필드 값들은 BindParameters()을 통해 binding 된 버퍼인 \n
 *			pOdbcParam->ParamArray[]에 저장된다.
 *****************************************************************************/
int	
GetParameters(ODBC_PARAMETER *pOdbcParam, char *ptr, int dataLength, int *gnIdx)
{
    int i;
    int Idx = *gnIdx, s_Idx = *gnIdx;
    char		field[HUGE_STRING_LENGTH+1];
    int	length;
    
    for(i=0 ; i<pOdbcParam->NumParams && Idx < dataLength; )
    {
        while(ptr[Idx]!=FIELD_DEL && ptr[Idx]!=RECORD_DEL && Idx<dataLength)
            Idx++;

        memset(field, 0x00, HUGE_STRING_LENGTH+1);
        length = Idx-s_Idx;
        strncpy(field, ptr+s_Idx, length);
        field[length] = 0x00;
        if((int)strlen(field)>(pOdbcParam->ParamLenArray[i]-1)) 
        {
            ErrLog("%s;The size of data in DB is %d. "
                   "But the size of data[%s] from %s is %d;\n", 
                    pOdbcParam->uid, pOdbcParam->ParamLenArray[i]-1, 
                    field, pOdbcParam->uid, strlen(field));
            return (-1);
        }
        strcpy((char *)pOdbcParam->ParamArray[i++], field);
        if(ptr[Idx] == RECORD_DEL) 
            break;
        Idx++;		// delimeter skip
        s_Idx = Idx;		
    }

    if(pOdbcParam->event == DOWNLOAD_FLAG ||
       pOdbcParam->event == DOWNLOAD_DELETE_FLAG)
    {
        if (i == pOdbcParam->NumParams-1)
        {
            if(pOdbcParam->syncFlag == ALL_SYNC )
            {
                if(pOdbcParam->dbType == DBSRV_TYPE_MYSQL || pOdbcParam->dbType == DBSRV_TYPE_CUBRID)
                {
                    strcpy((char *)pOdbcParam->ParamArray[i++], "1970-01-01");
                }
                else
                {
                    strcpy((char *)pOdbcParam->ParamArray[i++], "1900-01-01");
                }
            }
            else
            {
                strcpy((char *)pOdbcParam->ParamArray[i++], 
                       pOdbcParam->lastSyncTime);
            }
        }
    }
    else  if(ptr[Idx] != RECORD_DEL)
    {
        ErrLog("%d_%s;The format of data is wrong."
               " There is no Record Delimeter.;\n",
               pOdbcParam->order, pOdbcParam->uid);
        return (-1);
    }
    
    if(i!=pOdbcParam->NumParams)
    {
        ErrLog("%d_%s;[SCRIPT ERROR] Client부터 받은 parameter의 개수가"
            " script에 정의된 parameter의 개수와 맞지 않습니다.;\n", 
            pOdbcParam->order, pOdbcParam->uid);
        return (-1);
    }

#ifdef DEBUG
    for(i=0 ; i<pOdbcParam->NumParams ; i++)
        ErrLog("%s;%s/(%d);\n",pOdbcParam->uid,pOdbcParam->ParamArray[i], 
               strlen((char *)pOdbcParam->ParamArray[i]));
#endif

    // break로 빠져나오면 RECORD_DEL을 pointing 하므로 다음 index 값은 1 증가
    *gnIdx = Idx+1;  
    return (0);
}

/*****************************************************************************/
//! FreeArrays
/*****************************************************************************/
/*! \breif	BindParameters()와 BindColumns()을 통해 할당한 메모리들을 해제
 ************************************
 * \param	pOdbcParam(out)	: pOdbcParam->(Column, Param) Array binding
 ************************************
 * \return	void
  ************************************
 * \note	BindParameters()와 BindColumns()을 통해 할당한 메모리들을 해제해준다.
 *****************************************************************************/
void FreeArrays(ODBC_PARAMETER *pOdbcParam)
{
    int	i;
    if (pOdbcParam->NumParams)
    {
        for (i = 0; i < pOdbcParam->NumParams; i++)
        {
            if(pOdbcParam->ParamArray[i])	
                free(pOdbcParam->ParamArray[i]);
        }
        free(pOdbcParam->ParamArray);
        free(pOdbcParam->ParamLenArray);
        free(pOdbcParam->ParamLenOrIndArray);
    }
    if (pOdbcParam->NumCols)
    {
        for (i = 0; i < pOdbcParam->NumCols; i++)	
        {
            if(pOdbcParam->ColumnArray[i])
                free(pOdbcParam->ColumnArray[i]);
        }
        free(pOdbcParam->ColumnArray);
        free(pOdbcParam->ColumnLenArray);
        free(pOdbcParam->ColumnLenOrIndArray);
    }
}

/*****************************************************************************/
//! ReplaceParameterToData
/*****************************************************************************/
/*! \breif	delimeter에 맞춰 field를 parsing 하고 ? 부분에 넣어 statement를 바꿔준다.
 ************************************
 * \param	pOdbcParam(in)		: 
 * \param	Statement(in)		: 하나의 문장
 * \param	newStatement(out)	:	
 * \param	ptr(out)				:	
 * \param	dataLength(in)		:
 * \param	gnIdx(in)			:
 ************************************
 * \return	<0, on error
 *			>0, on versionID를 찾은 경우 new version ID
 ************************************
 * \note	데이터베이스가 ACCESS인 경우 SQLDescribeParam() 함수를 ODBC에서 \n
 *			지원하지 않기 때문에 '?'에 해당하는 부분에 직접 값을 assign 해서\n
 *			실행해주어야 한다. 이럴 때 호출되는 함수로 data stream을 \n
 *			parsing 하여 각 필드 값을 구해 문장에 직접 대입해서 새로운 \n
 *			statement로 바꿔준다. 
 *****************************************************************************/
int 
ReplaceParameterToData(ODBC_PARAMETER *pOdbcParam, 
                       SQLCHAR *Statement, SQLCHAR *newStatement, 
                       char *ptr, int dataLength, int *gnIdx)
{	
    int Idx = *gnIdx, s_Idx = *gnIdx;
    char field[HUGE_STRING_LENGTH+1];
    char tmpStatement[HUGE_STRING_LENGTH+1];
    int	length;
    char *sptr;
    
    strcpy(tmpStatement, (char *)Statement);
    sptr = strtok(tmpStatement, "?");
    sprintf((char *)newStatement, "%s", sptr);
    
    for( ; sptr && Idx < dataLength; )
    {
        while(ptr[Idx]!=FIELD_DEL && ptr[Idx]!=RECORD_DEL && Idx<dataLength)
            Idx++;

        memset(field, 0x00, HUGE_STRING_LENGTH+1);
        length = Idx-s_Idx;
        strncpy(field, ptr+s_Idx, length);
        field[length] = 0x00;
        sptr = strtok(NULL, "?");
        sprintf((char *)newStatement, "%s%s", newStatement, field);
        if(sptr) 
            strcat((char *)newStatement, sptr);        
        
        if(ptr[Idx] == RECORD_DEL) 
            break;
        Idx++;		// delimeter skip
        s_Idx = Idx;
    }	

    if(pOdbcParam->event == DOWNLOAD_FLAG || 
       pOdbcParam->event == DOWNLOAD_DELETE_FLAG)
    {
        sptr = strtok(NULL, "?");
        if (sptr) 
        {
            if(pOdbcParam->syncFlag == ALL_SYNC)
            {
                if(pOdbcParam->dbType == DBSRV_TYPE_CUBRID)
                {
                    sprintf((char *)newStatement, 
                            "%s1970-01-01%s", newStatement, sptr);
                }
                else
                {
                    sprintf((char *)newStatement, 
                            "%s1900-01-01%s", newStatement, sptr);
                }
            }
            else
            {
                sprintf((char *)newStatement, 
                        "%s%s%s", newStatement, pOdbcParam->lastSyncTime, sptr);
            }

            sptr = strtok(NULL, "?");
        }
    }

    // break로 빠져나오면 RECORD_DEL을 pointing 하므로 다음 index 값은 1 증가
    *gnIdx = Idx+1;  
    return (0);
}

/*****************************************************************************/
//! ProcessStmt
/*****************************************************************************/
/*! \breif	statement를 제대로 꾸며주는 부분
 ************************************
 * \param	dataBuffer(in)		: 
 * \param	dataLength(in)		: 하나의 문장
 * \param	pOdbcParam(out)		:	
 * \param	Statement(om)		:	
 ************************************
 * \return	<0, on error
 *			>0, on versionID를 찾은 경우 new version ID
 ************************************
 * \note	각 이벤트에 해당하는 statement를 구한 뒤 statement handle을 하나\n
 *			할당 받아 SQL 문장을 prepare 하고 parameter와 columns을 binding\n
 *			한 뒤 실행해준다.
 *			데이터베이스가 ACCESS인 경우는 SQLPrepare()로 문장을 prepare \n
 *			해주고 parameter 의 정보를 ODBC 함수를 통해 얻어올 수 없으므로 \n
 *			ReplaceParameterToData()함수를 이용하여 '?' 값이 적절한 필드 \n
 *			값으로 치환된 문장을 얻어 SQLExecDirect ()로 실행해준다. 
 *			그 외 데이터베이스에 대해서는 PrepareStmt() 함수를 이용하여 \n
 *			parameter나 column들을 binding 해주고 GetParameters() 함수를 \n
 *			이용하여 binding 된 버퍼에 적절한 값들을 assign 한 뒤 \n
 *			SQLExecute() 함수로 실행해준다. 단, SQL 문장이 stored procedure인\n 
 *			경우는 한번 alloc한 statement handle을 계속 이용할 수 없으므로 \n
 *			매번 free하고 다시 alloc, prepare, execute를 하도록 한다.
 *****************************************************************************/
int	ProcessStmt(char *dataBuffer, int dataLength, ODBC_PARAMETER *pOdbcParam, SQLCHAR *Statement)
{
    SQLRETURN	returncode;
    int		record_count =0;
    int		gnIdx = 0;
    int		ret = 0;
    
    if (AllocStmtHandle(pOdbcParam, PROCESSING) < 0) 
        return (-1);
        
    if(pOdbcParam->dbType == DBSRV_TYPE_ACCESS || pOdbcParam->dbType == DBSRV_TYPE_CUBRID)
    {    
        SQLCHAR newStatement[HUGE_STRING_LENGTH+1];
        
        returncode = 
            SQLPrepare(pOdbcParam->hstmt4Processing, Statement, SQL_NTS);
        if (returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
        {
            DisplayError(pOdbcParam->hstmt4Processing, 
                         pOdbcParam->uid, pOdbcParam->order, returncode);
            return	-1;
        }
        SQLNumParams(pOdbcParam->hstmt4Processing, &(pOdbcParam->NumParams));
        pOdbcParam->NumCols = 0;

        // SQLDescribeParam() 함수를 Access & Sybase에서는 지원하지 않으므로
        // parameter에 해당하는 ?를 전달 받은 데이터로 채워서 
        // statement를 만든 후 SQLExecDirect()함수로 실행해주도록 한다.
        while(gnIdx < dataLength)
        {		
            if(ReplaceParameterToData(pOdbcParam, Statement, newStatement, 
                                      dataBuffer, dataLength, &gnIdx) < 0  )
            {
                ret = -1;
                pOdbcParam->NumParams = 0;
                goto ProcessStmt_END;
            }			
            
            // 쿼리문 소문자로               
            returncode = SQLExecDirect(pOdbcParam->hstmt4Processing, 
                                       newStatement, SQL_NTS);
            
            if(returncode!=SQL_SUCCESS && 
               returncode!=SQL_SUCCESS_WITH_INFO && returncode!=SQL_NO_DATA)
            {
                DisplayError(pOdbcParam->hstmt4Processing, 
                             pOdbcParam->uid, pOdbcParam->order, returncode);
                ret = -1;
                pOdbcParam->NumParams = 0;
                goto ProcessStmt_END;
            }		
            record_count++;
        }
        pOdbcParam->NumParams = 0;
    }
    else
    {        
        if (PrepareStmt(pOdbcParam->hstmt4Processing, pOdbcParam, Statement)<0)
        {
            ret = -1;
            goto ProcessStmt_END;
        }
        
        while (gnIdx < dataLength)
        {		
            if(pOdbcParam->NumParams) 
            {
                if(GetParameters(pOdbcParam, dataBuffer, dataLength, &gnIdx)<0)
                {
                    ret = -1;
                    goto ProcessStmt_END;
                }
            }

            returncode = SQLExecute(pOdbcParam->hstmt4Processing);
            if(returncode != SQL_SUCCESS && 
               returncode != SQL_SUCCESS_WITH_INFO && 
               returncode != SQL_NO_DATA)
            {
                DisplayError(pOdbcParam->hstmt4Processing, 
                             pOdbcParam->uid, pOdbcParam->order, returncode);
                ret = -1;
                goto ProcessStmt_END;
            }		

            record_count++;

            // upload script에 parameter가 없는 경우 한번 실행은 하고 종료
            if(!pOdbcParam->NumParams)
                break; 

           // Upload가 stored procedure인 경우 
           // SQLPrepare, SQLExecute를 계속 할 수 없으므로 Free 해주고 
           // 다시 Alloc, Prepare, Execute를 반복한다 
            if(pOdbcParam->dbType == DBSRV_TYPE_MSSQL)
            {		
                if(!strncmp((char *)Statement, "{", 1))
                { 
                    FreeStmtHandle(pOdbcParam, PROCESSING);
                    FreeArrays(pOdbcParam);	
                    if (AllocStmtHandle(pOdbcParam, PROCESSING) < 0) 
                        return (-1);
                    if (PrepareStmt(pOdbcParam->hstmt4Processing, pOdbcParam, Statement) < 0) 
                    {
                        ret = -1;
                        goto ProcessStmt_END;
                    }
                }
            }
        } 
    }

    if(pOdbcParam->event==UPDATE_FLAG)
        ErrLog("%d_%s;Upload_Update : %d rows affected;\n", 
               pOdbcParam->order, pOdbcParam->uid, record_count);
    else if(pOdbcParam->event==DELETE_FLAG)
        ErrLog("%d_%s;Upload_Delete : %d rows affected;\n", 
               pOdbcParam->order, pOdbcParam->uid, record_count);
    else if(pOdbcParam->event==INSERT_FLAG)
        ErrLog("%d_%s;Upload_Insert : %d rows affected;\n", 
               pOdbcParam->order, pOdbcParam->uid, record_count);	
    
ProcessStmt_END:
    FreeStmtHandle(pOdbcParam, PROCESSING);
    FreeArrays(pOdbcParam);
    return	(ret);
}

/*****************************************************************************/
//! FetchResult
/*****************************************************************************/
/*! \breif	recordBuffer와, recordBuffer로 query의 실행 결과를 적어준다.
 ************************************
 * \param	pOdbcParam(in)		: DB와 연관성 있는 데이터
 * \param	recordBuffer(out)	: 버퍼
 * \param	recordBuffer(out)	: 버퍼 
 ************************************
 * \return	<0, on error
 *			>0, on versionID를 찾은 경우 new version ID
 *			-1(error), 1(find), SQL_NO_DATA (no_file)
 ************************************
 * \note	PrepareStmt() 함수로 prepare, execute 한 cursor로부터 데이터를\n
 *			하나씩 fetch하여 pOdbcParam->ColumnArray[]에 들어 있는 각 필드 \n
 *			값들을 recordBuffer에 써준다. 각 필드를 쓸 때에는 FIELD_DEL을 \n
 *			이용하고 recordBuffer를 client에 보낼 sendBuffer에 쓸 때에는 \n
 *			RECORD_DEL을 구분자로 이용한다. 
 *			SendBuffer가 HUGE_STRING_LENGTH (8192 byte) 값보다 크면 1을 \n
 *			return 하여 socket을 통해 client에 데이터를 전달할 수 있도록 한다. 
 *			더 이상 보낼 데이터가 없을 시에 2를 return 해준다.
 *****************************************************************************/
int	FetchResult(ODBC_PARAMETER *pOdbcParam, char *sendBuffer, char *recordBuffer)
{
    SQLRETURN	returncode;
    int			nByte=0;
    int			i;
    
    if (pOdbcParam->NumRows > 0)
    {  // 이전에 fetch한 데이타를 카피한다. 첫번째 call일 때는 skip
        sprintf(sendBuffer, "%s", recordBuffer);
        nByte = strlen(recordBuffer);	
    }
    memset(recordBuffer, '\0', HUGE_STRING_LENGTH+1);	
    // download data fetch 
    while((returncode = SQLFetch(pOdbcParam->hstmt4Processing))!=SQL_NO_DATA &&
          (returncode == SQL_SUCCESS) ) 
    {	
        for (i = 0; i < pOdbcParam->NumCols; i++) 
        {
            delete_space((char *)pOdbcParam->ColumnArray[i], 
                         pOdbcParam->ColumnLenOrIndArray[i]);
            strcat(recordBuffer, (char *)pOdbcParam->ColumnArray[i]);
            sprintf(recordBuffer, "%s%c", recordBuffer, FIELD_DEL);            
            memset(pOdbcParam->ColumnArray[i], 
                   0x00, pOdbcParam->ColumnLenArray[i]);
        }
        
        recordBuffer[strlen(recordBuffer)-1] = RECORD_DEL;
        pOdbcParam->NumRows++;
        
        if (nByte+strlen(recordBuffer) > HUGE_STRING_LENGTH)	
            break;
        nByte += strlen(recordBuffer);
        
        strcat(sendBuffer, recordBuffer);
        memset(recordBuffer, '\0', HUGE_STRING_LENGTH+1);
    }
    
    if (returncode != SQL_SUCCESS && 
        returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
    {
        DisplayError(pOdbcParam->hstmt4System, 
                     pOdbcParam->uid, pOdbcParam->order, returncode);        
        return -1;
    }

    if (returncode == SQL_NO_DATA && nByte == 0)	
        return	2;
    else
        return (1);
}

/*****************************************************************************/
//! GetSyncTime
/*****************************************************************************/
/*! \breif	달된 parameter가 있는 경우 GetParameters()를 호출하여 값을 구하고 없는\n
 *			경우는 syncTime만 assign
 ************************************
 * \param	pOdbcParam(out)	:	
 * \param	ptr(in)			: parameter 값 or NO_PARAM)
 * \param	dataLength(in)	:
 * \param	mode(in)		:
 ************************************
 * \return	<0, on error
 *			>0, on versionID를 찾은 경우 new version ID
 ************************************
 * \note	Download의 script를 prepare 할 때 호출되는 함수로 select 문장에\n
 *			조건 절이 없는 경우 pOdbcParam->NumParams 값이 0이므로 바로 \n
 *			return 되고 조건 절은 있으나 client로부터 전달 받은 parameter가 \n
 *			없는 경우(NO_PARAM이 전달됨), parameter의 개수가 한 개라면 \n
 *			자동으로 sync time을 넣어 주는 작업을 한다. 
 *			만약 parameter의 개수가 한 개가 아닌데 client로부터 전달 받은 \n
 *			parameter가 없는 경우는 error 처리한다.
 *			또한, 전달 받은 parameter가 있는 경우는 GetParameters() 함수를 \n
 *			이용하여 parameter를 binding 시켜준다.
 *****************************************************************************/
int	 
GetSyncTime(ODBC_PARAMETER *pOdbcParam, char *ptr, int dataLength, int mode)
{
    int gnIdx=0;
    
    if (pOdbcParam->NumParams == 0)		
        return 0;  // SyncTime 없이 전체 sync 하는 걸로 디자인 한 경우

    if (!strcmp(ptr, "NO_PARAM"))
    {  // 전달 받은 parameter가 없는 경우
        if (pOdbcParam->NumParams == 1)
        { // 하나면 syncTime을 넣어 준다.
            if(mode == PROCESSING)
            {
                if(pOdbcParam->syncFlag == ALL_SYNC)	
                {
                    if(pOdbcParam->dbType == DBSRV_TYPE_MYSQL || pOdbcParam->dbType == DBSRV_TYPE_CUBRID)
                        strcpy((char *)pOdbcParam->ParamArray[0], "1970-01-01");
                    else
                        strcpy((char *)pOdbcParam->ParamArray[0], "1900-01-01");
                }
                else
                {
                    strcpy((char *)pOdbcParam->ParamArray[0], 
                            pOdbcParam->lastSyncTime);
                }
            }
        }
        else
        { // script의 parmaeter의 개수는 한개가 아닌데 parameter로
          // 받은 값이 없으므로 default인 syncTime만 넣어 줘서는 안된다.
            ErrLog("%d_%s;[SCRIPT ERROR] PDA로부터 받은 parameter의 개수가" 
                   " script에 정의된 parameter의 개수와 맞지 않습니다.;\n", 
                   pOdbcParam->order, pOdbcParam->uid);
            return	(-1);	
        }
    }
    else
    {   // 전달 받은 parameter가 있는 경우
        if(GetParameters(pOdbcParam, ptr, dataLength, &gnIdx)<0) 
            return -1;
    }
    
    return (0);
}

/*****************************************************************************/
//! PrepareDownload
/*****************************************************************************/
/*! \breif	Download시 호출되는 함수
 ************************************
 * \param	dataBuffer(out)	: parameter 값 or NO_PARAM)
 * \param	pOdbcParam(out)	:	
 * \param	dataLength(in)	:
 * \param	Statement(in)		:
 ************************************
 * \return	<0, on error
 *			>0, on versionID를 찾은 경우 new version ID
 ************************************
 * \note	DownloadProcessingMain() 함수에서 불러지는 함수로 ACCESS\n
 *			데이터베이스인 경우 조건 절이 있으면 '?' 값을 적절한 parameter로\n
 *			치환시키고(ReplaceParameterToData()), 전달 받은 parameter가 없는\n
 *			경우는 sync time으로 치환해준다. 
 *			다른 데이터베이스도 PrepareStmt()와 GetSyncTime()으로 parameter를\n
 *			적절하게 binding한 뒤에 SQLExecute()로 실행해준다.
 *****************************************************************************/
int	
PrepareDownload(char *dataBuffer, 
                ODBC_PARAMETER *pOdbcParam, int dataLength, SQLCHAR *Statement)
{
    SQLRETURN	returncode;
    
    pOdbcParam->NumRows = 0;
        
    if(pOdbcParam->dbType == DBSRV_TYPE_ACCESS || 
       pOdbcParam->dbType == DBSRV_TYPE_CUBRID   )
    {        
        int gnIdx =0;
        SQLCHAR newStatement[HUGE_STRING_LENGTH+1];
        char *sptr;
        
        pOdbcParam->NumCols = 0;
        char tmpStatement[HUGE_STRING_LENGTH+1];
        
        if(!strcmp(dataBuffer,"NO_PARAM"))
        {
            // Statement : select * from employee where updateDate>=CDate('?')
            strcpy(tmpStatement, (char *)Statement);
            // select ..... where updatedate>=CDate( 까지의 부분 or ?가 없는 경우 전체 문장
            sptr = strtok(tmpStatement, "?");  
            sprintf((char *)newStatement, "%s", sptr);
            sptr = strtok(NULL, "?");	// ') 부분
            if(sptr)
            {	// ') 가 있는 경우
                if(pOdbcParam->syncFlag == ALL_SYNC)
                {
                    if(pOdbcParam->dbType == DBSRV_TYPE_CUBRID)
                    {
                        sprintf((char *)newStatement, 
                                "%s1970-01-01%s", newStatement, sptr);
                    }
                    else
                    {
                        sprintf((char *)newStatement, 
                                "%s1900-01-01%s", newStatement, sptr);
                    }
                }
                else
                {
                    sprintf((char *)newStatement, "%s%s%s", 
                            newStatement, pOdbcParam->lastSyncTime, sptr);
                }

                sptr = strtok(NULL, "?");  // NULL이어야 정상
                if(sptr)
                {
                    ErrLog("%d_%s;[SCRIPT ERROR] PDA로부터 받은 parameter의"
                            " 개수가 script에 정의된 parameter의 "
                            "개수와 맞지 않습니다.;\n", 
                            pOdbcParam->order, pOdbcParam->uid);
                    return	(-1);
                }
            }
            else if( sptr == NULL )
            {
                if(pOdbcParam->dbType == DBSRV_TYPE_CUBRID)
                {
                    if(pOdbcParam->syncFlag == ALL_SYNC)
                    {
                        sprintf((char *)newStatement, 
                                "%s'1970-01-01'", newStatement);
                    }
                }
            }            
        }
        else 
        {
            if(ReplaceParameterToData(pOdbcParam, Statement, newStatement, 
                                      dataBuffer, dataLength, &gnIdx) < 0 )
            {
                return (-1);	
            }
        }

        if (PrepareStmt(pOdbcParam->hstmt4Processing, 
                        pOdbcParam, newStatement) < 0)
        {
            return	(-1);
        }
    }
    else 
    {
        if (PrepareStmt(pOdbcParam->hstmt4Processing, pOdbcParam, Statement)<0)
            return	(-1);
        if (GetSyncTime(pOdbcParam, dataBuffer, dataLength, PROCESSING) < 0)
            return	(-1);
    }

    returncode = SQLExecute(pOdbcParam->hstmt4Processing);
    if(returncode != SQL_SUCCESS && 
       returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
    {
        DisplayError(pOdbcParam->hstmt4Processing, 
                     pOdbcParam->uid, pOdbcParam->order, returncode);
        return (-1);			
    }
    
    return 0;
}

/*****************************************************************************/
//! GetDSN
/*****************************************************************************/
/*! \breif	cliDSN과 pOdbcParam->VersionID로 정보를 구한다.
 ************************************
 * \param	pOdbcParam(in,out)	: in(VersionID, cliDSN), out(DBID)
 * \param	dsn(out)			:
 * \param	dsn_uid(out)		:
 * \param	dsn_passwd(out)		:
 ************************************
 * \return	<0, on error
 *			>0, on versionID를 찾은 경우 new version ID
 ************************************
 * \note	mSync_SetDSN()으로 입력받은 dsn의 정보를 얻는 함수로 server의 \n
 *			DSN, DB에 접속할 사용자 id, password 값을 얻어온다.
 *****************************************************************************/
int	GetDSN(ODBC_PARAMETER *pOdbcParam, 
           char *cliDSN, SQLCHAR *dsn, SQLCHAR *dsn_uid, SQLCHAR *dsn_passwd)
{
    SQLCHAR		Statement[QUERY_MAX_LEN + 1];
    SQLRETURN	returncode;
    SQLINTEGER	dsnLenOrInd, dbidLenOrInd, dsnuidLenOrInd, dsnpasswdLenOrInd;
    int ret = 0;
    
    if(AllocStmtHandle(pOdbcParam, SYSTEM)<0) 
        return (-1);
    
    // SQL 문장 실행
    sprintf((char *)Statement,
            "select DBID, SvrDSN, DSNUserID, DSNPwd from openMSYNC_DSN "
            "where VersionID = %d and CliDSN = '%s'",
            pOdbcParam->VersionID, cliDSN);	
    
    // 쿼리문 소문자로 
    returncode = SQLExecDirect(pOdbcParam->hstmt4System, Statement, SQL_NTS);    
    if(returncode != SQL_SUCCESS && returncode != SQL_SUCCESS_WITH_INFO)
    {
        DisplayError(pOdbcParam->hstmt4System, 
                     pOdbcParam->uid, pOdbcParam->order, returncode);
        ret = -1;
        goto GetDSN_END;
    }

    SQLBindCol(pOdbcParam->hstmt4System, 
               1, SQL_C_ULONG, &(pOdbcParam->DBID), 0, &dbidLenOrInd);
    SQLBindCol(pOdbcParam->hstmt4System, 
               2, SQL_C_CHAR, dsn, DSN_LEN+1, &dsnLenOrInd);
    SQLBindCol(pOdbcParam->hstmt4System,
               3, SQL_C_CHAR, dsn_uid, DSN_UID_LEN+1, &dsnuidLenOrInd);
    SQLBindCol(pOdbcParam->hstmt4System, 
               4, SQL_C_CHAR, dsn_passwd, DSN_PASSWD_LEN+1, &dsnpasswdLenOrInd);
    
    // SQL Fetch
    returncode = SQLFetch(pOdbcParam->hstmt4System);	

    // SQL_ERROR면 return -1
    if(returncode != SQL_SUCCESS && 
       returncode != SQL_SUCCESS_WITH_INFO && returncode != SQL_NO_DATA)
    {
        DisplayError(pOdbcParam->hstmt4System, 
                     pOdbcParam->uid, pOdbcParam->order, returncode);        
        ret = -1;
        goto GetDSN_END;
    }	
    // openMSYNC_Version table에 없는 application or version
    if(returncode == SQL_NO_DATA) 
    {
        ErrLog("%d_%s;There is no DSN of '%s' which is defined in client;\n", 
               pOdbcParam->order, pOdbcParam->uid, cliDSN);
        ret = -1;
        goto GetDSN_END;
    }
    
    delete_space((char *)dsn, dsnLenOrInd);
    delete_space((char *)dsn_uid, dsnuidLenOrInd);
    delete_space((char *)dsn_passwd, dsnpasswdLenOrInd);
    
GetDSN_END:
    FreeStmtHandle(pOdbcParam, SYSTEM);
    return (ret);
}

