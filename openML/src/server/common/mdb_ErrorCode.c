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

#include "mdb_config.h"
#include "dbport.h"
#include "ErrorCode.h"

#ifndef DO_NOT_USE_ERROR_DESC
__DECL_PREFIX const struct Error_Desc_T _error_desc[] = {
    {DB_SUCCESS, /*   0 */ "No Error"},
    {NOTSUPPORTED, /*    -2 */ "Not Supported"},
    {DB_E_INVALIDPARAM, /*    -3 */ "Invalid Parameter"},
    //{DB_E_UNKNOWNERROR,       /*    -4 */       "" },
    {DB_DISCONNECTED, /*     5 */ ""},
    {DB_E_DISCONNECTED, /*    -5 */ ""},
    {DB_LOGINED, /*     7 */ ""},
    {DB_E_TERMINATED, /*    -8 */ ""},
    {DB_E_DBEXISTED, /*    -9 */ "DB File/Dir Existed"},
    {DB_E_DISKFULL, /*    -11 */ "Operation Fail by Disk Full"},
    {DB_E_INVALIDDBPATH, /*    -18 */ "Invalid DB path"},

    {DB_E_DBPATHMAKE, /*    -26 */ "DB Path Make Error"},

/* comm stub error code */

    {CS_E_INVALIDCONNID, /*  -50 */ "Invalid Connection ID"},
    {CS_E_ALREADYDISCONNECTED, /*    -51 */ "Already Disconnected"},
    {CS_E_NOMOREHANDLE, /*    -54 */ "No more connection ID"},
    {CS_E_OUTOFMEMORY, /*    -60 */ "Out of memory"},
    {CS_E_NOTINITAILIZED, /*    -62 */ "-62"},

/* db object */

    {DB_E_KEYFIELDNAME, /*    -100 */ "Reserved Field Name Used"},
    {DB_E_DUPFIELDNAME, /*    -101 */ "Field Name Duplicated"},
    {DB_E_DUPTABLENAME, /*    -102 */ "Table Exists"},
    {DB_E_UNKNOWNTABLE, /*    -103 */ "Unknown Table/View"},
    {DB_E_DUPUNIQUEKEY, /*    -104 */ "Unique Key Duplicated"},
    {DB_E_DUPDISTRECORD,        /*    -105 */
            "Distinct Restriction Violated"},
    {DB_W_ITEMMORETHANKEY, /*     108 */ "108"},
    {DB_E_TOOBIGRECORD, /*    -109 */ "Record Too Big"},
    {DB_E_UNKNOWNTYPE, /*    -110 */ "Unknown Data Type"},
    {DB_E_DUPINDEXNAME, /*    -111 */ "Index Name Aleady Existed"},
    {DB_E_EXCEEDNUMINDEXES,     /*    -112 */
            "Exceed the limit number of indexes per table"},
    {DB_E_NOMORERECORDS, /*    -113 */ "No More Records"},
    {DB_E_DBNAMENOTPATH, /*    -114 */ "DB name not contained path"},
    {DB_E_OUTOFTHRMEMORY, /*    -115 */ "-115"},
    {DB_E_INSERTTHREAD, /*    -116 */ "-116"},
    {DB_E_UNKNOWNINDEX, /*    -119 */ "Unknown Index"},
    {DB_E_OUTOFMEMORY, /*    -120 */ "Out of Memory"},
    {DB_E_CURSORKEYDESC,        /*    -121 */
            "Key description error for cursor open"},
    {DB_E_PIDPOINTER, /*    -124 */ "Invalid RID."},
    {DB_E_OUTOFDBMEMORY, /*    -125 */ "Out of DB (System) Memory"},
    {DB_E_NUMIDXFIEDLS, /*    -126 */
            "Exceed the Max No. of indexed fields"},
    {DB_E_ALREADYCACHED, /*  -128 */ "Object already cached"},
    {DB_E_NOTCACHED, /*  -129 */ "Object not cached"},

    {DB_E_DBFILECREATE, /*    -160 */
            "DB File creation fail"},
    {DB_E_DBFILEOPEN,   /*    -161 */
            "DB File open fail"},
    {DB_E_DBFILEWRITE,  /*    -162 */
            "DB File write fail"},
    {DB_E_DBFILEREAD,   /*    -163 */
            "DB File read fail"},
    {DB_E_DBFILESEEK,   /*    -164 */
            "DB File lseek fail"},
    /* Check basic tables */
    {DB_E_SYSTABLECONT, /*    -167 */
            "system table not exists"},
    {DB_E_LIMITSIZE,    /* -174 */
            "Exceed the Limit of max database size."},

/* db object locking... */

    {DB_E_DEADLOCKABORT, /*    -200 */ "Aborted by Deadlock Resolving"},
    {DB_E_INVALIDLOCKMODE, /*    -204 */ "Invalid Lock Mode"},
    {DB_E_OUTOFLOCKMEMORY, /*    -205 */ "Out of Memory for Locking"},
    {DB_E_LOCKTIMEOUT, /*  -206 */ "Lock Timeout"},
    {DB_E_DEADLOCKOPER, /*  -208 */
            "Operation Aborted due to Deadlock"},

    {DB_E_PAGELINKLOOP, /*  -262 */
            "Page Link has Loop."},

    {DB_E_INVALIDLIKESYNTAX,    /*  -292 */
            "Missing or illegal character following the escape character."},

/* transaction */

    {DB_E_NOTTRANSBEGIN, /*    -400 */ "Transaction Not Began"},
    {DB_E_TRANSINTERRUPT,       /*    -401 */
            "Transaction has been interrupted"},

/* cursor */

    {DB_E_NOMORECURSOR, /*    -500 */
            "Exceed the maximum number of cursores"},
    {DB_E_INVALIDCURSORID,      /*  -501 */
            "Invalid cursor id"},

/* action */

    {DB_E_NOTPERMITTED, /*    -601 */ "Not Permitted Action"},


    {DB_E_SEQUENCE_INVALID_NAME,        /* -900 */
            "sequence's name is INVALID."},
    {DB_E_SEQUENCE_INVALID_INCNUM,      /* -901 */
            "Invalid increment number. "},
    {DB_E_SEQUENCE_INVALID_MINNUM_MAXNUM,       /* -902 */
            "Minimum number must be less than maximum number."},
    {DB_E_SEQUENCE_INVALID_STARTNUM_MAXNUM,     /* -903 */
            "Start number cannot be more than maximum number."},
    {DB_E_SEQUENCE_INVALID_STARTNUM_MINNUM,     /* -904 */
            "Start number cannot be less than minimum number."},
    {DB_E_SEQUENCE_INVALID_INCNUM_RANGE,        /* -905 */
            "Increase must be less than maximum number minus minimum number."},
    {DB_E_SEQUENCE_ALREADY_EXIST,       /* -910 */
            "the same name's sequence exist."},
    {DB_E_SEQUENCE_EXCESS_LIMIT,        /* -911 */
            "Exceed the maximum or minimum value."},
    {DB_E_SEQUENCE_NOT_EXIST,   /* -912 */
            "Sequence does not exist."},
    {DB_E_SEQUENCE_NOT_INIT,    /* -913 */
            "Sequence is not yet defined."},

    {DB_E_INVALID_RID,  /* -921 */
            "Invalid RID."},
    {DB_E_LIMITRID_SINGLETABLE, /* -922 */
            "LIMIT @rid can be used with single table."},

    {DB_E_INVALID_VARIABLE_TYPE,        /* -923 */
            "Variable Types's fixed-part must be less than 255 bytes."},
    {DB_E_INDEX_HAS_BYTE_OR_VARBYTE,    /* -924 */
            "Index can not have BYTE or VARBYTE type."},

    {DB_E_LIMITRID_NOTALLOWED,  /* -926 */
            "LIMIT @rid can not be used with in predicate."},

    {DB_E_NOTFOUND_PRIMARYKEY,  /* -928 */
            "The Primary key is not found."},

    {DB_E_RECORDLIMIT_EXCEEDED, /* -935 */
            "Record limit exceeded."},

/* sql error code */

    {SQL_E_ERROR,       /* -1001 */
            "Internal error. contact with administrator"},
    {SQL_E_NOTIMPLEMENTED,      /* -1002 */
            "Optional feature is not implemented yet "},
    {SQL_E_INVALIDARGUMENT,     /* -1003 */
            "Invalid arguments "},

    {SQL_E_NOTCONNECTED,        /* -1100 */
            "Connection is not established "},
    {SQL_E_NOTLOGON,    /* -1101 */
            "Not login "},
    {SQL_E_NOTAVAILABLEHANDLE,  /* -1102 */
            "Not available resource "},
    {SQL_E_EXCEEDMAXSTATEMENT,  /* -1104 */
            "The number of maximum allowable statements currently opened in handle "},
    {SQL_E_STATEMENTNOTFOUND,   /* -1105 */
            "The statement handle is not found "},

    {SQL_E_INVALIDSYNTAX,       /* -1203 */
            "Query syntax error "},
    {SQL_E_MULTIPRIMARYKEY,     /* -1204 */
            "Table can have only one PRIMARY KEY "},
    {SQL_E_INVALIDKEYCOLUMN,    /* -1205 */
            "Key column not existed in table "},
    {SQL_E_NOTGROUPBYEXPRESSION,        /* -1206 */
            "Not GROUP BY expression "},
    {SQL_E_NOTUNIQUETABLEALIAS, /* -1207 */
            "Not unique table/alias "},
    {SQL_E_INVALIDDEFAULTVALUE, /* -1208 */
            "Invalid default value "},
    {SQL_E_NOTALLOWEDNULLVALUE, /* -1209 */
            "Cannot be NULL "},
    {SQL_E_DUPLICATECOLUMNNAME, /* -1210 */
            "Duplicate column name "},
    {SQL_E_INCONSISTENTTYPE,    /* -1211 */
            "Inconsistent datatypes "},
    {SQL_E_INVALIDEXPRESSION,   /* -1212 */
            "Invalid expression "},
    {SQL_E_NESTEDAGGREGATION,   /* -1213 */
            "Not support nested group function "},
    {SQL_E_NOTSINGLETONSUBQ,    /* -1214 */
            "Multiple rows in singleton select"},
    {SQL_E_INVALIDFIXEDPART,    /* -1215 */
            "fixed_part out of range"},

    {SQL_E_STACKUNDERFLOW,      /* -1301 */
            "Internal structure(#2) manipulation error"},
    {SQL_E_OUTOFMEMORY, /* -1303 */
            "Not enough memory available"},
    {SQL_E_INVALIDCOMMAND,      /* -1304 */
            "Unknown command"},

    {SQL_E_INVALIDTABLE,        /* -1401 */
            "Table or view does not exist"},
    {SQL_E_INVALIDCOLUMN,       /* -1402 */
            "Invalid column name"},
    {SQL_E_AMBIGUOUSCOLUMN,     /* -1403 */
            "Column ambiguously defined"},
    {SQL_E_INVALIDTYPE, /* -1404 */
            "You have an expression that is composed of invalid types"},
    {SQL_E_NOTMATCHCOLUMNCOUNT, /* -1405 */
            "Column count doesn't match value count "},
    {SQL_E_INVALIDINDEX,        /* -1406 */
            "You have an invalid index "},
    {SQL_E_ALREADYEXISTTABLE,   /* -1407 */
            "Object name is already used by an existing object "},
    {SQL_E_NOTALLOWED,  /* -1408 */
            "Insufficient privileges "},
    {SQL_E_TOOLARGEVALUE,       /* -1409 */
            "Specified value too large for column "},
    {SQL_E_NOTALLOWEDNULLKEY,   /* -1410 */
            "Any part of a PRIMARY KEY must not be NULL "},
    {SQL_E_DUPLICATEINDEXNAME,  /* -1411 */
            "Object name already used by an existing object "},
    {SQL_E_DUPLICATEINDEX,      /* -1413 */
            "Such column list already indexed "},
    {SQL_E_NOTNUMBER,   /* -1414 */
            "Invalid number "},
    {SQL_E_ALLCOLUMNDROP,       /* -1415 */
            "Cannot drop all columns in a table"},
    {SQL_E_DISTINCTORDERBY,     /* -1416 */
            "ORDER BY items must appear in the select list if SELECT DISTINCT is specified."},
    {SQL_E_TOOLONGVIEW, /* -1418 */
            "View definition is too long"},
    {SQL_E_INVALIDVIEW, /* -1419 */
            "View does not exist"},
    {SQL_E_INVALIDBASETABLE,    /* -1420 */
            "Table does not exist"},
    {SQL_E_SETNOTNULL,  /* -1421 */
            "Cannot set NOT NULL. null value found"},
    {SQL_E_SETPRIMARYKEY,       /* -1422 */
            "Cannot set PRIMARY KEY. duplicate value found"},
    {SQL_E_SETTYPE_PRECSCALE,   /* -1423 */
            "Column to be altered must be empty to decrease precision or scale"},
    {SQL_E_SETTYPE_COMPATIBILITY,       /* -1424 */
            "Column to be altered must be empty for type conversion"},
    {SQL_E_INVALIDSEEK, /* -1425 */
            "Invalid seek the resultset"},
    {SQL_E_INVALID_OPERATION_VIEW,
            "Invalid operation on view"},       /* -1426 */
    /* type check */
    {SQL_E_EXCEED_ORDERBY_FILEDS,       /* -1433 */
            "Exceed the Max No. of orderby fields"},
    {SQL_E_EXCEED_GROUPBY_FILEDS,       /* -1434 */
            "Exceed the Max No. of groupby fields"},
    {SQL_E_DUPIDXFIXEDNUMTABLE, /* -1435  */
            "Such column list already indexed due to fixed-number table."},
    {SQL_E_INVALIDITEMINDEX,    /* -1436  */
            "Invalid selection item index."},
    /* autoincrement */
    {SQL_E_NOTALLOW_AUTO,       /* -1450  */
            "This type is not allowed in AUTOINCREMENT."},

    {SQL_E_RESTRICT_WHERE_CLAUSE,       /* -1602 */
            "Can't use binary types in where clause"},
    {SQL_E_RESTRICT_GROUPBY_CLAUSE,     /* -1603 */
            "Can't use binary types in groupby clause"},
    {SQL_E_RESTRICT_HAVING_CLAUSE,      /* -1604 */
            "Can't use binary types in having clause"},
    {SQL_E_RESTRICT_ORDERBY_CLAUSE,     /* -1605 */
            "Can't use binary types in orderby clause"},
    {SQL_E_RESTRICT_KEY_OR_FIELD,       /* -1606 */
            "Can't binary types in key "},
    {SQL_E_INVALID_AGGREGATION_TYPE,    /* -1607 */
            "Can't use binary types in aggregation "},
    {SQL_E_INVALID_BYTE_FILE_PATH,      /* -1608 */
            "Invalid File Path"},
    {SQL_E_MISMATCH_COLLATION,  /* -1700 */
            "Mismatch collation type"},
    {DB_E_MISMATCH_COLLATION,   /* -1700 */
            "Mismatch collation type"},

    {SQL_E_INVALID_NOLOGGING_COLUMN,    /* -1710 */
            "Invalid NOLOGGING column."},
    {SQL_E_NOLOGGING_COLUMN_INDEX,      /* -1711 */
            "Cannot create index on NOLOGGING column."},

    {SQL_E_INVALID_MSYNCTABLE,  /* -1900 */
            "Invalid mSync table."},
    {SQL_E_NOTALLOW_MSYNC_RID,  /* -1901 */
            "Rid in where or limit clause can not be used with "
                "SELECT DELETE_RECORD."},

    {SQL_E_TOOMANYJOINTABLE,    /* -3201 */
            "Too many join tables "},
    {SQL_E_TOOMANYDISTINCT,     /* -3202 */
            "Too many distinct expressions "},

    {SQL_E_NULLPLANSTRING,      /* -3203 */
            "Plan string is null"},

    {SQL_E_NOTPREPARED, /* -5001 */
            "No previous preparing job done"},
    {SQL_E_ILLEGALCALLSEQUENCE, /* -5002 */
            "Illegal function call sequence"},
    {SQL_E_NOPARAMETERS,        /* -5003 */
            "No parameters exists to bind"},
    {SQL_E_NOTBINDRESULT,       /* -5004 */
            "SQL_E_NOTBINDRESULT"},
    {SQL_E_INVALIDFETCHCALL,    /* -5005 */
            "SQL_E_INVALIDFETCHCALL"},

    {E_INVALID_RECLEN,  /* -5006 */
            ""},
    {E_MAXCLIENTS_EXEEDED,      /* -5007 */
            ""},
    {E_MAXLEN_EXEEDED,  /* -5008 */
            "type's length is too large."},

    {SQL_E_CANNOTBINDING,       /* -5009 */
            "Can not Binding"},

    {SQL_E_RESULTAPINOTCALLED,  /* -5010 */
            "Result API not called"},

    /* CHECK_MAX_TABLE_NAME_LENGTH */
    {SQL_E_MAX_TABLE_NAME_LEN_EXEEDED,  /* -9000 */
            "TABLENAME IS MORE THAN 31 BYTES"},
    /* CHECK_MAX_FIELD_LENGTH */
    {SQL_E_MAX_FIELD_NAME_LEN_EXEEDED,  /* -9001 */
            "FIELDNAME IS MORE THAN 31 BYTES"},
    /* CHECK_MAX_INDEX_LENGTH */
    {SQL_E_MAX_INDEX_NAME_LEN_EXEEDED,  /* -9002 */
            "INDEXNAME IS MORE THAN 31 BYTES"},
    /* CHECK_MAX_ALIAS_LENGTH */
    {SQL_E_MAX_ALIAS_NAME_LEN_EXEEDED,  /* -9003 */
            "INDEXNAME IS MORE THAN 31 BYTES"},
    {E_LASTERROR, "Unknown Error"}

};

__DECL_PREFIX const int num_error_desc =
        sizeof(_error_desc) / sizeof(struct Error_Desc_T);
#endif


/*****************************************************************************/
//! function_name 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX const char *
DB_strerror(int errorno)
{
#ifdef DO_NOT_USE_ERROR_DESC
    static char error_desc[] = "No Error Description";

    return error_desc;
#else
    int i;
    static char error_line[] = "Error Line";

    if (errno <= E_LINE_BASE)
        return error_line;

    for (i = 0; i < num_error_desc - 1; i++)
    {
        if (_error_desc[i].error_no == errorno)
            return _error_desc[i].error_desc;
    }

    return _error_desc[i].error_desc;
#endif
}
