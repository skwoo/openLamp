/* 
   This file is part of openML, mobile and embedded DBMS.

   Copyright (C) 2012 Inervit Co., Ltd.
   support@inervit.com

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Less General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Less General Public License for more details.

   You should have received a copy of the GNU Less General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

package com.openml;

public class JEDB
{
    public static final int SQL_DATA_NONE        = 0;
    public static final int SQL_DATA_TINYINT     = 2;
    public static final int SQL_DATA_SMALLINT    = 3;
    public static final int SQL_DATA_INT         = 4;
    public static final int SQL_DATA_BIGINT      = 6;
    public static final int SQL_DATA_FLOAT       = 7;
    public static final int SQL_DATA_DOUBLE      = 8;
    public static final int SQL_DATA_DECIMAL     = 9;
    public static final int SQL_DATA_CHAR        = 10;
    public static final int SQL_DATA_NCHAR       = 11;
    public static final int SQL_DATA_VARCHAR     = 12;
    public static final int SQL_DATA_NVARCHAR    = 13;
    public static final int SQL_DATA_BYTE        = 14;
    public static final int SQL_DATA_VARBYTE     = 15;
    public static final int SQL_DATA_TIMESTAMP   = 18;
    public static final int SQL_DATA_DATETIME    = 19;
    public static final int SQL_DATA_DATE        = 20;
    public static final int SQL_DATA_TIME        = 21;
    public static final int SQL_DATA_OID         = 23;

    public static final int SQL_STMT_NONE        = 0;
    public static final int SQL_STMT_SELECT      = 1;
    public static final int SQL_STMT_INSERT      = 2;
    public static final int SQL_STMT_UPDATE      = 3;
    public static final int SQL_STMT_DELETE      = 4;
    public static final int SQL_STMT_UPSERT      = 5;
    public static final int SQL_STMT_DESCRIBE    = 7;
    public static final int SQL_STMT_CREATE      = 8;
    public static final int SQL_STMT_RENAME      = 9;
    public static final int SQL_STMT_ALTER       = 10;
    public static final int SQL_STMT_TRUNCATE    = 11;
    public static final int SQL_STMT_PRIVILEGE   = 12;
    public static final int SQL_STMT_ADMIN       = 13;
    public static final int SQL_STMT_DROP        = 14;
    public static final int SQL_STMT_COMMIT      = 15;
    public static final int SQL_STMT_ROLLBACK    = 16;
    public static final int SQL_STMT_SET         = 17;


    public native static int CreateDB1 (String dbname);

    public native static int CreateDB2 (String dbname, int dbseg_num);

    public native static int CreateDB3 (String dbname, int dbseg_num,
            int idxseg_num, int tmpseg_num);

    public native static int DestoryDB (String dbname);

    public native static int GetConnection ();

    public native static int Connect (int connid, String dbname);

    public native static void Disconnect (int connid);

    public native static void ReleaseConnection (int connid);

    public native static void Commit (int connid);

    public native static void CommitFlush (int connid);

    public native static void Rollback (int connid);

    public native static void RollbackFlush (int connid);

    public native static int QueryCreate (int connid, String query, int utf16);

    public native static int QueryDestroy (int connid, int stmt);

    public native static int QueryBindParam (int connid, int stmt,
            int param_idx, int is_null, int type, String buffer, int length);

    public native static int QueryExecute (int connid, int stmt);

    public native static int QueryClearRow (int connid, int stmt);

    public native static int QueryGetNextRow (int connid, int stmt);

    public native static int QueryGetFieldCount (int connid, int stmt);

    public native static String QueryGetFieldData (int connid, int stmt,
            int field_idx);

    public native static int QueryIsFieldNull (int connid, int stmt,
            int field_idx);

    public native static String QueryGetFieldName (int connid, int stmt,
            int field_idx);

    public native static int QueryGetFieldType (int connid, int stmt,
            int field_idx);

    public native static int QueryGetFieldLength (int connid, int stmt,
            int field_idx);

    public native static int QueryGetQueryType (int connid, int stmt);
}
