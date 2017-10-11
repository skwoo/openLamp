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

#ifndef ERRORCODE_H
#define ERRORCODE_H

#include "mdb_config.h"

#define E_LINE_BASE         -90000000

/* error < 0, warning > 0 */

/* General */
#define DB_PACK_ENOUGH             11

#define DB_EXIST                   10
#define DB_SUCCESS                  0
#define DB_NOERROR                  0
#define DB_FAIL                    (E_LINE_BASE - __LINE__)
#define DB_E_ERROR                 -1
#define DB_E_NOTSUPPORTED          -2
#define NOTSUPPORTED               DB_E_NOTSUPPORTED
#define DB_E_INVALIDPARAM          -3
//#define DB_E_UNKNOWNERROR        -4
#define DB_DISCONNECTED             5
#define DB_E_DISCONNECTED          -5
#define DB_LOGINED                  7
#define DB_E_TERMINATED            -8
#define DB_E_DBEXISTED             -9
#define DB_E_DBDIREXISTED          -9
#define DB_E_DISKFULL             -11
#define DB_E_TOOSMALLRECBUF       -15
#define DB_E_DBNAMENOTSPECIFIED   -16
#define DB_E_ENVIRONVAR           -17
#define DB_E_INVALIDDBPATH        -18
#define DB_E_DBMEMINIT            -19
#define DB_E_INVALIDLONGDBPATH    -20
#define DB_E_VERNOTCOMPATIBLE     -21
#define DB_E_LIMITDBSIZE          -23

#define DB_E_ENDIANNOTCOMPATIBLE  -25

#define DB_E_DBPATHMAKE           -26
#define DB_E_ALREADYCONNECTED     -27

/* comm stub error code */
#define CS_E_INVALIDCONNID        -50
#define CS_E_ALREADYDISCONNECTED  -51
#define CS_E_NOMOREHANDLE         -54
#define CS_E_OUTOFMEMORY          -60
#define CS_E_NOTINITAILIZED       -62

/* db object */
#define DB_E_KEYFIELDNAME        -100   /* reserved field name violation */
#define DB_E_DUPFIELDNAME        -101   /* duplicated field name */
#define DB_E_DUPTABLENAME        -102   /* duplicated table name */
#define DB_E_UNKNOWNTABLE        -103   /* table not existed */
#define DB_E_UNKNOWNVIEW         -103   /* view not existed */
#define DB_E_DUPUNIQUEKEY        -104   /* the same unique key record existed */
#define DB_E_DUPDISTRECORD       -105   /* duplicated record at distinction */
#define DB_W_ITEMMORETHANKEY      108
#define DB_E_TOOBIGRECORD        -109   /* record size limit over */
#define DB_E_UNKNOWNTYPE         -110   /* unknown data type */
#define DB_E_DUPINDEXNAME        -111   /* duplicated index name */
#define DB_E_EXCEEDNUMINDEXES    -112   /* exceed # of indexes per table */
#define DB_E_NOMORERECORDS       -113   /* no more records */
#define DB_E_DBNAMENOTPATH       -114   /* wince: db name is not path */
#define DB_E_OUTOFTHRMEMORY      -115   /* short of thread allocation memory */
#define DB_E_INSERTTHREAD        -116
#define DB_E_UNKNOWNINDEX        -119   /* unknown index */
#define DB_E_OUTOFMEMORY         -120   /* out of memmory */
#define DB_E_CURSORKEYDESC       -121   /* key description error
                                           for cursor open */
#define DB_E_INITMUTEX           -122   /* initialization error of mutex */
#define DB_E_INITCOND            -123   /* initialization error of cv */
#define DB_E_PIDPOINTER          -124
#define DB_E_OIDPOINTER          DB_E_PIDPOINTER
#define DB_E_OUTOFDBMEMORY       -125   /* out of DB (system) memmory */
#define DB_E_NUMIDXFIEDLS        -126   /* exceed the no. of indexed fields. */
#define DB_E_FIELDINFO           -127   /* error on field info. */
#define DB_E_ALREADYCACHED       -128   /* object already cached */
#define DB_E_NOTCACHED           -129   /* object not cached */
#define DB_E_VERIFYNEWSLOT       -130   /* allocated slot verification fail */
#define DB_E_NOTNULLFIELD        -131   /* null in not null field */
#define DB_E_TTREEPOSITION       -132   /* last position */

#define DB_E_LOGMGRINIT                    -153
#define DB_E_CONTCATINIT                   -158
#define DB_E_IDXCATINIT                    -159

#define DB_E_DBFILECREATE                  -160
#define DB_E_DBFILEOPEN                    -161
#define DB_E_DBFILEWRITE                   -162
#define DB_E_DBFILEREAD                    -163
#define DB_E_DBFILESEEK                    -164
#define DB_E_DBFILECLOSE                   -165

#define DB_E_INVLAIDCONTOID                -166
#define DB_E_SYSTABLECONT                  -167
#define DB_E_SYSFIELDCONT                  -168
#define DB_E_SYSINDEXCONT                  -169
#define DB_E_SYSINDEXFIELDCONT             -170

#define DB_E_FIELDNUM                      -171
#define DB_E_INDEXNUM                      -172

#define DB_E_LIMITSIZE                     -174

#define DB_E_IDXSEGALLOCFAILL              -175

#define DB_E_TABLESCHEMA                   -183
#define DB_E_INDEXSCHEMA                   -184

#define DB_E_ANCHORFILEOPEN                -186
#define DB_E_ANCHORFILEWRITE               -187

#define DB_E_LOGBUFFERCHECK1               -188
#define DB_E_LOGBUFFERCHECK2               -189
#define DB_E_INVALIDLOGDATA                -190
#define DB_E_READLOGRECORD                 -191
#define DB_E_LOGFILECREATEPTR              -192
#define DB_E_INVALIDLOGDATASIZE            -193

#define DB_E_LOGFILEOPEN                   -194
#define DB_E_LOGFILEWRITE                  -195
#define DB_E_LOGFILECREATE                 -196
#define DB_E_LOGFILESEEK                   -197
#define DB_E_LOGFILEREAD                   -199

/* continue numbering at -210 */

/* db object locking... */
#define DB_E_DEADLOCKABORT                 -200
#define DB_E_INVALIDLOCKMODE               -204
#define DB_E_OUTOFLOCKMEMORY               -205
#define DB_E_LOCKTIMEOUT                   -206 /* lock timeout */
#define DB_E_DEADLOCKOPER                  -208 /* DEADLOCK operation */

#define DB_E_DBFILEOID                     -209

#define DB_E_MAKELOGPATH                   -211
#define DB_E_INVALIDREADLSN                -212

#define DB_E_REBALANCEINSERT               -213

#define DB_E_INVALIDLOGTYPE                -215

#define DB_E_SEGALLOCFAIL                  -217
#define DB_E_SEGALLOCLOGFAIL               -218

#define DB_E_MEMMGRINIT                    -220

#define DB_E_LASTCHKPTLSN                  -221
#define DB_E_UNDOLOGRECORD                 -222
#define DB_E_FREESLOT                      -223

#define DB_E_DBFILESIZE                    -227

#define DB_E_NUMSEGMENT                    -228

#define DB_E_INVALIDSEGNUM                 -230
#define DB_E_DBFILESYNC                    -231

#define DB_E_DBFILEFILLUP                  -235

#define DB_E_INVALIDPAGE                   -261
#define DB_E_PAGELINKLOOP                  -262
#define DB_E_INDEXNODELOOP                 -263

#define DB_E_UPDATEFIELDCOUNT              -264

#define DB_E_CRASHLOGRECORD                -272
#define DB_E_CRASHLOGANCHOR                -273

#define DB_E_OPENFILE                      -274
#define DB_E_WRITEFILE                     -275
#define DB_E_READFILE                      -276
#define DB_E_INVALIDFILE                   -277

#define DB_E_LOGCHECKSUM                   -278

#define DB_E_INVALIDLIKESYNTAX             -292

#define DB_E_REMOVEDRECORD                 -293

#define DB_E_LOGOFFSETMISMATCH             -302

#define DB_E_FIXEDDBSIZE                   -350

/* transaction */
#define DB_E_NOTTRANSBEGIN                 -400
#define DB_E_TRANSINTERRUPT                -401
#define DB_E_UNKNOWNTRANSID                -402
#define DB_E_NOTCOMMITTED                  -403
#define DB_E_NOMORETRANS                   -404

/* thread */
#define DB_E_CREATETHREAD                  -450

/* cursor */
#define DB_E_NOMORECURSOR                  -500
#define DB_E_INVALIDCURSORID               -501
#define DB_E_CURSORNOTATTACHED             -502
#define DB_E_CURSOROFFSET                  -504 /* dbi_Cursor_Seek fail */
#define DB_E_CURSORNODE                    -505 /* dbi_Cursor_Offset fail */

/* action */
#define DB_E_NOTPERMITTED                  -601

/* Container */
#define DB_E_INVALIDCOLLECTIDX             -899

/* sequence */
#define DB_E_SEQUENCE_INVALID_NAME         -900
#define DB_E_SEQUENCE_INVALID_INCNUM       -901

#define DB_E_SEQUENCE_INVALID_MINNUM_MAXNUM     -902
#define DB_E_SEQUENCE_INVALID_STARTNUM_MAXNUM   -903
#define DB_E_SEQUENCE_INVALID_STARTNUM_MINNUM   -904
#define DB_E_SEQUENCE_INVALID_INCNUM_RANGE -905
#define DB_E_SEQUENCE_ALREADY_EXIST        -910
#define DB_E_SEQUENCE_EXCESS_LIMIT         -911
#define DB_E_SEQUENCE_NOT_EXIST            -912
#define DB_E_SEQUENCE_NOT_INIT             -913

#define DB_E_INVALID_RID                   -921
#define DB_E_LIMITRID_SINGLETABLE          -922

#define DB_E_INVALID_VARIABLE_TYPE         -923
#define DB_E_INDEX_HAS_BYTE_OR_VARBYTE     -924

#define DB_E_LIMITRID_NOTALLOWED           -926

/* UPSERT error codes */
#define DB_E_NOTFOUND_PRIMARYKEY           -928

#define DB_E_RECORDLIMIT_EXCEEDED          -935

#define DB_E_INDEXCHANGED                  -991

/*************************************************************************
 *  Error Value Format
 *
 *  -xyab   sign(-) : error indicator
 *          x       : phase number
 *          y       : sub-phase number
 *          ab      : error value
 *************************************************************************/

/****************************/
/* ODBC standard definition */
/****************************/
#ifndef WIN32
#ifndef SQL_SUCCESS
#define SQL_SUCCESS                           0
#endif
#ifndef SQL_ERROR
#define SQL_ERROR                            -1
#endif
#endif
/****************************/

#define SQL_E_SUCCESS                         1
#define SQL_E_NOERROR                         0
#define SQL_E_ERROR                       -1001
#define SQL_E_NOTIMPLEMENTED              -1002
#define SQL_E_INVALIDARGUMENT             -1003

#define SQL_E_NOTCONNECTED                -1100
#define SQL_E_NOTLOGON                    -1101
#define SQL_E_NOTAVAILABLEHANDLE          -1102
#define SQL_E_EXCEEDMAXSTATEMENT          -1104
#define SQL_E_STATEMENTNOTFOUND           -1105

#define SQL_E_INVALIDSYNTAX               -1203
#define SQL_E_MULTIPRIMARYKEY             -1204
#define SQL_E_INVALIDKEYCOLUMN            -1205
#define SQL_E_NOTGROUPBYEXPRESSION        -1206
#define SQL_E_NOTUNIQUETABLEALIAS         -1207
#define SQL_E_INVALIDDEFAULTVALUE         -1208
#define SQL_E_NOTALLOWEDNULLVALUE         -1209
#define SQL_E_DUPLICATECOLUMNNAME         -1210
#define SQL_E_INCONSISTENTTYPE            -1211
#define SQL_E_INVALIDEXPRESSION           -1212
#define SQL_E_NESTEDAGGREGATION           -1213
#define SQL_E_NOTSINGLETONSUBQ            -1214
#define SQL_E_INVALIDFIXEDPART            -1215
#define SQL_E_INVALIDDIRTYCOUNT           -1216

#define SQL_E_STACKUNDERFLOW              -1301
#define SQL_E_OUTOFMEMORY                 -1303
#define SQL_E_INVALIDCOMMAND              -1304

#define SQL_E_INVALIDTABLE                -1401 /* SQL */
#define SQL_E_INVALIDCOLUMN               -1402
#define SQL_E_AMBIGUOUSCOLUMN             -1403
#define SQL_E_INVALIDTYPE                 -1404
#define SQL_E_NOTMATCHCOLUMNCOUNT         -1405
#define SQL_E_INVALIDINDEX                -1406
#define SQL_E_ALREADYEXISTTABLE           -1407
#define SQL_E_NOTALLOWED                  -1408
#define SQL_E_TOOLARGEVALUE               -1409
#define SQL_E_NOTALLOWEDNULLKEY           -1410
#define SQL_E_DUPLICATEINDEXNAME          -1411
#define SQL_E_DUPLICATEINDEX              -1413
#define SQL_E_NOTNUMBER                   -1414
#define SQL_E_ALLCOLUMNDROP               -1415
#define SQL_E_DISTINCTORDERBY             -1416
#define SQL_E_TOOLONGVIEW                 -1418
#define SQL_E_INVALIDVIEW                 -1419
#define SQL_E_INVALIDBASETABLE            -1420
#define SQL_E_SETNOTNULL                  -1421
#define SQL_E_SETPRIMARYKEY               -1422
#define SQL_E_SETTYPE_PRECSCALE           -1423
#define SQL_E_SETTYPE_COMPATIBILITY       -1424

#define SQL_E_INVALIDSEEK                 -1425

/* To insert, delete or update a record or to create a index on a view
   is not valid. */
#define SQL_E_INVALID_OPERATION_VIEW      -1426

#define SQL_E_EXCEED_ORDERBY_FILEDS       -1433
#define SQL_E_EXCEED_GROUPBY_FILEDS       -1434
/* duplicated idx of fixed-number table's */
#define SQL_E_DUPIDXFIXEDNUMTABLE         -1435

#define SQL_E_INVALIDITEMINDEX            -1436

#define SQL_E_NOTALLOW_AUTO               -1450 /* autoincrement */

#define SQL_E_RESTRICT_WHERE_CLAUSE       -1602
#define SQL_E_RESTRICT_GROUPBY_CLAUSE     -1603
#define SQL_E_RESTRICT_HAVING_CLAUSE      -1604
#define SQL_E_RESTRICT_ORDERBY_CLAUSE     -1605
#define SQL_E_RESTRICT_KEY_OR_FIELD       -1606
#define SQL_E_INVALID_AGGREGATION_TYPE    -1607
#define SQL_E_INVALID_BYTE_FILE_PATH      -1608

#define SQL_E_MISMATCH_COLLATION          -1700
#define DB_E_MISMATCH_COLLATION           -1701

#define SQL_E_INVALID_NOLOGGING_COLUMN    -1710
#define SQL_E_NOLOGGING_COLUMN_INDEX      -1711

#define SQL_E_INVALID_MSYNCTABLE          -1900
#define SQL_E_NOTALLOW_MSYNC_RID          -1901

#define SQL_E_TOOMANYJOINTABLE            -3201
#define SQL_E_TOOMANYDISTINCT             -3202
#define SQL_E_NULLPLANSTRING              -3203

#define SQL_E_NOTPREPARED                 -5001
#define SQL_E_ILLEGALCALLSEQUENCE         -5002
#define SQL_E_NOPARAMETERS                -5003
#define SQL_E_NOTBINDRESULT               -5004
#define SQL_E_INVALIDFETCHCALL            -5005
#define     E_INVALID_RECLEN              -5006
#define     E_MAXCLIENTS_EXEEDED          -5007
#define     E_MAXLEN_EXEEDED              -5008

#define SQL_E_CANNOTBINDING               -5009

#define SQL_E_RESULTAPINOTCALLED          -5010

#define SQL_MEM_E_OUTOFMEMORY             -8000
#define SQL_MEM_E_FULL_CHUNK_ARRAY        -8002
#define SQL_MEM_E_INVALIDARGUMENT         -8003

 /* CHECK_MAX_TABLE_NAME_LENGTH */
#define SQL_E_MAX_TABLE_NAME_LEN_EXEEDED  -9000
 /* CHECK_MAX_FIELD_LENGTH */
#define SQL_E_MAX_FIELD_NAME_LEN_EXEEDED  -9001
 /* CHECK_MAX_INDEX_LENGTH */
#define SQL_E_MAX_INDEX_NAME_LEN_EXEEDED  -9002
 /* CHECK_MAX_ALIAS_LENGTH */
#define SQL_E_MAX_ALIAS_NAME_LEN_EXEEDED  -9003

#define     E_LASTERROR                  -10000

struct Error_Desc_T
{
    int error_no;
    const char error_desc[128];
};

extern __DECL_PREFIX const struct Error_Desc_T _error_desc[];

__DECL_PREFIX const char *DB_strerror(int errorno);

#endif
