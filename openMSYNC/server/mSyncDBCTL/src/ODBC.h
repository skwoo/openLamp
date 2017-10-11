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


#ifndef ODBC_H
#define ODBC_H

typedef struct _odbc_parameter {
    SQLHENV     henv4Processing;
    SQLHENV		henv4System;
    SQLHDBC     hdbc4Processing;
    SQLHDBC		hdbc4System;
    SQLHSTMT    hstmt4Processing;
    SQLHSTMT	hstmt4System;
    int			hflag4Processing;	
    int			hflag4System;
    SQLPOINTER* ParamArray ;		
    SQLINTEGER* ParamLenArray;	
    SQLINTEGER* ParamLenOrIndArray;
    SQLPOINTER* ColumnArray;
    SQLINTEGER* ColumnLenArray;		
    SQLINTEGER* ColumnLenOrIndArray;
    
    short		UserDBOpened;			
    short		NumCols;		
    short		NumParams;	
    int			NumRows;
    int			VersionID;
    int			newVersionID;		
    int			DBID;					
    short		syncFlag;				
    char		tableName[TABLE_NAME_LEN+1];
    char		event;
    char		uid[USER_ID_LEN+1];
    SQLCHAR*	dbuid;		
    SQLCHAR*	dbpasswd;
    SQLCHAR*	dbdsn;	
	short		mode;

    int		    dbType;		
    char*		lastSyncTime;

    char*		connectTime;		
	short		Authenticated;
	int			order;	
} ODBC_PARAMETER;

int	AllocStmtHandle(ODBC_PARAMETER *pOdbcParam, int flag);
void FreeStmtHandle(ODBC_PARAMETER *pOdbcParam, int flag);
void FreeHandles(int flag, ODBC_PARAMETER *pOdbcParam);

int OpenSystemDB(ODBC_PARAMETER *pOdbcParam);
int	OpenUserDB(ODBC_PARAMETER *pOdbcParam, 
               SQLCHAR *dsn, SQLCHAR *dsnuid, SQLCHAR *dsnpasswd);


#endif
