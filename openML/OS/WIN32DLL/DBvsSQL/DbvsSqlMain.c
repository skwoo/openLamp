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

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <direct.h>
#include <ctype.h>

#include "dbport.h"
#include "db_api.h"
#include "isql.h"


typedef struct
{
    iSQL _isql;
    iSQL_STMT *_pstmt;
    iSQL_BIND *_pbind;
} S_DB_SQL;

enum
{
    ATTR_NULL = 0,
    ATTR_NSTRING = 1,
    ATTR_STRING = 2,
    ATTR_ETC = 3,
    ATTR_DUMMY = INT_MAX        /* to guarantee sizeof(enum) == 4 */
} enAttr;

S_DB_SQL sISQL;
char *gl_pDBPath = 0x00;
char *gl_pDBTable = 0x00;
char *gl_pFileName = 0x00;
int gl_hFile = -1;

int
ShowUsage(char *pExe)
{
    printf("\n-------------------------------------------");
    printf("\nUsage: %s", pExe);
    printf("\n  to Import: %s -Import <file_name> <db_path_name> <table_name>",
            pExe);
    printf("\n  to export: %s -Export <file_name> <db_path_name> <table_name>",
            pExe);
    printf("\n-------------------------------------------\n");
    return 0;
}


int
CheckParameter(int argc, char *argv[])
{
    if (argc != 5)
        return -1;

    gl_pFileName = argv[2];
    gl_pDBPath = argv[3];
    gl_pDBTable = argv[4];

    if ((sc_strcasecmp(argv[1], "-E") == 0)
            || (sc_strcasecmp(argv[1], "-Export") == 0))
    {
        return 0;
    }
    else if ((sc_strcasecmp(argv[1], "-I") == 0)
            || (sc_strcasecmp(argv[1], "-Import") == 0))
    {
        return 1;
    }
    else
        return -1;
}


int
__iSql_ErrMessage(int nErrCode)
{
    if (sISQL._pstmt)
        printf("iSQL Error %s:%d \n", iSQL_stmt_error(sISQL._pstmt),
                iSQL_stmt_errno(sISQL._pstmt));
    else
        printf("iSQL Error %s:%d \n", iSQL_error(&(sISQL._isql)),
                iSQL_errno(&(sISQL._isql)));

    return nErrCode;
}

int
__DumpQuery(iSQL_RES ** pRes, int *pnRowCnt, int *pnFieldCnt, char *pStrQuery)
{
    iSQL_BIND *param = 0x00;

    sISQL._pstmt = 0x00;
    sISQL._pbind = 0x00;

    // 1. 준비 작업
    sISQL._pstmt =
            iSQL_prepare(&(sISQL._isql), pStrQuery, sc_strlen(pStrQuery));
    if (sISQL._pstmt == 0x00)
    {
        return __iSql_ErrMessage(-1);
    }

    if (iSQL_describe(sISQL._pstmt, &param, &(sISQL._pbind)) < 0)
    {
        return __iSql_ErrMessage(-1);
    }

    *pRes = iSQL_prepare_result(sISQL._pstmt);
    if (*pRes == 0x00)
    {
        return __iSql_ErrMessage(-1);
    }
    // 2. 퀘리 수행 
    if (iSQL_execute(sISQL._pstmt) < 0)
    {
        return __iSql_ErrMessage(-1);
    }

    if (iSQL_stmt_store_result(sISQL._pstmt) < 0)
    {
        return __iSql_ErrMessage(-1);
    }

    if (pnRowCnt && pnFieldCnt)
    {
        *pnRowCnt = iSQL_stmt_affected_rows(sISQL._pstmt);
        *pnFieldCnt = iSQL_num_fields(*pRes);
    }

    return 0;
}

int
Dump2File()
{
    int nRet, nRowCnt, nFieldCnt;
    int i, j, isAttr;
    unsigned long int len, buf_len;
    char szBuf[256];
    iSQL_RES *pres = 0x00;

    sc_sprintf(szBuf, "select * from %s", gl_pDBTable);

    nRet = __DumpQuery(&pres, &nRowCnt, &nFieldCnt, szBuf);
    if (nRet < 0)
    {
        if (pres)
            iSQL_free_result(pres);

        if (sISQL._pstmt)
            iSQL_stmt_close(sISQL._pstmt);

        sISQL._pstmt = 0x00;
        sISQL._pbind = 0x00;
        return nRet;
    }

    printf("[%d] row selected\n", nRowCnt);

    sc_write(gl_hFile, &nFieldCnt, sizeof(int));
    sc_write(gl_hFile, &nRowCnt, sizeof(int));

    for (i = 0; i < nRowCnt; ++i)
    {
        if (iSQL_fetch(sISQL._pstmt) < 0)
        {
            nRet = __iSql_ErrMessage(-1);
            goto ISQL_ERROR;
        }

        //
        for (j = 0; j < nFieldCnt; ++j)
        {
            if (*(sISQL._pbind[j].is_null) == 1)
            {
                isAttr = ATTR_NULL;
                if (sc_write(gl_hFile, &isAttr, sizeof(int)) != sizeof(int))
                {
                    printf("sc_write(1) \n");
                    goto ISQL_ERROR;
                }
            }
            else
            {
                if (sISQL._pbind[j].buffer_type == DT_NCHAR
                        || sISQL._pbind[j].buffer_type == DT_NVARCHAR)
                    isAttr = ATTR_NSTRING;
                else if (sISQL._pbind[j].buffer_type == DT_CHAR
                        || sISQL._pbind[j].buffer_type == DT_VARCHAR)
                    isAttr = ATTR_STRING;
                else
                    isAttr = ATTR_ETC;

                len = *(sISQL._pbind[j].length);
                buf_len = sISQL._pbind[j].buffer_length;

                if (sc_write(gl_hFile, &isAttr, sizeof(int)) != sizeof(int))
                {
                    printf("sc_write(2) \n");
                    goto ISQL_ERROR;
                }
                //
                if (sc_write(gl_hFile, &len,
                                sizeof(unsigned long int)) !=
                        sizeof(unsigned long int))
                {
                    printf("sc_write(3) \n");
                    goto ISQL_ERROR;
                }

                if (sc_write(gl_hFile, &buf_len,
                                sizeof(unsigned long int)) !=
                        sizeof(unsigned long int))
                {
                    printf("sc_write(4) \n");
                    goto ISQL_ERROR;
                }

                if (isAttr == ATTR_NSTRING)
                    len = len * sizeof(DB_WCHAR);

                if (len > 0)
                {
                    if (sc_write(gl_hFile, sISQL._pbind[j].buffer,
                                    len) != (int) len)
                    {
                        printf("sc_write(5) \n");
                        goto ISQL_ERROR;
                    }
                }
            }
        }
    }

    if (pres)
        iSQL_free_result(pres);

    if (sISQL._pstmt)
        iSQL_stmt_close(sISQL._pstmt);

    //
    sISQL._pstmt = 0x00;
    sISQL._pbind = 0x00;

    return 0;

  ISQL_ERROR:
    if (pres)
        iSQL_free_result(pres);

    if (sISQL._pstmt)
        iSQL_stmt_close(sISQL._pstmt);

    //
    sISQL._pstmt = 0x00;
    sISQL._pbind = 0x00;

    return -1;
}

int
Import()
{
    char szQuery[512];
    char szBuf[8192];
    int nRet, nRowCnt, nFieldCnt;
    int i, j, isAttr;
    unsigned long int len, buf_len;
    iSQL_BIND *param = NULL;
    iSQL_BIND *res = NULL;

    sISQL._pstmt = 0x00;
    sISQL._pbind = 0x00;

    sc_read(gl_hFile, &nFieldCnt, sizeof(int));
    sc_read(gl_hFile, &nRowCnt, sizeof(int));

    sc_sprintf(szQuery, "insert into %s values (", gl_pDBTable);

    nRet = sc_strlen(szQuery);

    for (i = 0; i < nFieldCnt; i++)
    {
        if (i == (nFieldCnt - 1))
            sc_strcpy(szQuery + nRet, "?)");
        else
            sc_strcpy(szQuery + nRet, "?, ");

        nRet = sc_strlen(szQuery);
    }

    sISQL._pstmt = iSQL_prepare(&(sISQL._isql), szQuery, strlen(szQuery));
    if (sISQL._pstmt == NULL)
    {
        __iSql_ErrMessage(-1);
        if (sISQL._pstmt)
            iSQL_stmt_close(sISQL._pstmt);

        return -1;
    }

    if (iSQL_describe(sISQL._pstmt, &param, &res) < 0)
    {
        __iSql_ErrMessage(-1);
        if (sISQL._pstmt)
            iSQL_stmt_close(sISQL._pstmt);

        iSQL_rollback(&(sISQL._isql));
        return -1;
    }

    for (i = 0; i < nRowCnt; ++i)
    {
        for (j = 0; j < nFieldCnt; j++)
        {
            if (sc_read(gl_hFile, &isAttr, sizeof(int)) != sizeof(int))
            {
                printf("sc_read(1) \n");

                if (sISQL._pstmt)
                    iSQL_stmt_close(sISQL._pstmt);

                iSQL_rollback(&(sISQL._isql));
                return -1;
            }

            if (isAttr == ATTR_NULL)
            {
                sc_memset(param[j].buffer, 0x00, param[j].buffer_length);

                *(param[j].is_null) = 1;
                param[j].length = 0;
            }
            else
            {
                if (sc_read(gl_hFile, &len,
                                sizeof(unsigned long int)) !=
                        sizeof(unsigned long int))
                {
                    printf("sc_read(2) \n");
                    if (sISQL._pstmt)
                        iSQL_stmt_close(sISQL._pstmt);

                    iSQL_rollback(&(sISQL._isql));
                    return -1;
                }

                if (sc_read(gl_hFile, &buf_len,
                                sizeof(unsigned long int)) !=
                        sizeof(unsigned long int))
                {
                    printf("sc_read(3) \n");
                    if (sISQL._pstmt)
                        iSQL_stmt_close(sISQL._pstmt);

                    iSQL_rollback(&(sISQL._isql));
                    return -1;
                }


                if (param[j].buffer_length != buf_len)
                {
                    printf("miss match buffer_length..... \n");

                    if (sISQL._pstmt)
                        iSQL_stmt_close(sISQL._pstmt);

                    iSQL_rollback(&(sISQL._isql));
                    return -1;
                }

                if (isAttr == ATTR_NSTRING)
                    len = len * sizeof(DB_WCHAR);

                if (len > 0)
                {
                    if (sc_read(gl_hFile, szBuf, len) != (int) len)
                    {
                        printf("sc_read(4) \n");
                        if (sISQL._pstmt)
                            iSQL_stmt_close(sISQL._pstmt);

                        iSQL_rollback(&(sISQL._isql));
                        return -1;
                    }

                    if (isAttr == ATTR_ETC)
                    {
                        sc_memcpy(param[j].buffer, szBuf, len);
                        *(param[j].length) = len;
                    }
                    else if (isAttr == ATTR_STRING)
                    {
                        sc_memcpy(param[j].buffer, szBuf, len);
                        param[j].buffer[len] = 0x00;
                        *(param[j].length) = len;
                    }
                    else        //if( isAttr == ATTR_NSTRING )
                    {
                        sc_memcpy(param[j].buffer, szBuf, len);
                        param[j].buffer[len] = 0x00;
                        param[j].buffer[len + 1] = 0x00;
                        *(param[j].length) = len;
                    }
                }
                else
                {
                    param[j].buffer[0] = 0x00;
                    if (isAttr == ATTR_NSTRING)
                        param[j].buffer[1] = 0x00;
                }

                *(param[j].is_null) = 0;
            }
        }

        if (iSQL_execute(sISQL._pstmt) < 0)
        {
            __iSql_ErrMessage(-1);
            if (sISQL._pstmt)
                iSQL_stmt_close(sISQL._pstmt);

            iSQL_rollback(&(sISQL._isql));
            return -1;
        }

        if ((i % 10) == 0)
        {
            iSQL_commit(&(sISQL._isql));
            printf("[%d] record committed.\n", (i + 1));
        }
    }

    if (sISQL._pstmt)
        iSQL_stmt_close(sISQL._pstmt);

    iSQL_commit(&(sISQL._isql));
    printf("[%d] record committed.\n", i);

    //
    sISQL._pstmt = 0x00;
    sISQL._pbind = 0x00;

    return 0;
}


int
main(int argc, char *argv[])
{
    int nRet;

    sISQL._pbind = 0x00;
    sISQL._pstmt = 0x00;

    if ((nRet = CheckParameter(argc, argv)) < 0)
        return ShowUsage(argv[0]);

    printf("\niSQL_connect...[%s]\n", gl_pDBPath);
    if (iSQL_connect(&(sISQL._isql), "127.0.0.1", gl_pDBPath, "mmdb",
                    "mmdb") == 0x00)
    {
        return __iSql_ErrMessage(0);
    }

    if (nRet == 0)
    {       // dump       
        gl_hFile =
                sc_open(gl_pFileName, O_RDWR | O_CREAT | O_TRUNC | O_BINARY,
                OPEN_MODE);
        if (gl_hFile < 0)
        {
            printf("Failure: create %s file...\n", gl_pFileName);
            iSQL_disconnect(&(sISQL._isql));
            return 0;
        }

        nRet = Dump2File();
    }
    else
    {       // insert
        gl_hFile =
                sc_open(gl_pFileName, O_RDONLY | O_EXCL | O_BINARY, OPEN_MODE);
        if (gl_hFile < 0)
        {
            printf("Failure: open %s file...\n", gl_pFileName);
            iSQL_disconnect(&(sISQL._isql));
            return 0;
        }

        nRet = Import();
    }

    sc_close(gl_hFile);
    iSQL_disconnect(&(sISQL._isql));

    if (nRet < 0)
        printf("Error!!!\n");
    else
        printf("Success!!!\n");

    return 0;
}
