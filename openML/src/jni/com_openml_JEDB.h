/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_openml_JEDB */

#ifndef _Included_com_openml_JEDB
#define _Included_com_openml_JEDB
#ifdef __cplusplus
extern "C" {
#endif
#undef com_openml_JEDB_SQL_DATA_NONE
#define com_openml_JEDB_SQL_DATA_NONE 0L
#undef com_openml_JEDB_SQL_DATA_TINYINT
#define com_openml_JEDB_SQL_DATA_TINYINT 2L
#undef com_openml_JEDB_SQL_DATA_SMALLINT
#define com_openml_JEDB_SQL_DATA_SMALLINT 3L
#undef com_openml_JEDB_SQL_DATA_INT
#define com_openml_JEDB_SQL_DATA_INT 4L
#undef com_openml_JEDB_SQL_DATA_BIGINT
#define com_openml_JEDB_SQL_DATA_BIGINT 6L
#undef com_openml_JEDB_SQL_DATA_FLOAT
#define com_openml_JEDB_SQL_DATA_FLOAT 7L
#undef com_openml_JEDB_SQL_DATA_DOUBLE
#define com_openml_JEDB_SQL_DATA_DOUBLE 8L
#undef com_openml_JEDB_SQL_DATA_DECIMAL
#define com_openml_JEDB_SQL_DATA_DECIMAL 9L
#undef com_openml_JEDB_SQL_DATA_CHAR
#define com_openml_JEDB_SQL_DATA_CHAR 10L
#undef com_openml_JEDB_SQL_DATA_NCHAR
#define com_openml_JEDB_SQL_DATA_NCHAR 11L
#undef com_openml_JEDB_SQL_DATA_VARCHAR
#define com_openml_JEDB_SQL_DATA_VARCHAR 12L
#undef com_openml_JEDB_SQL_DATA_NVARCHAR
#define com_openml_JEDB_SQL_DATA_NVARCHAR 13L
#undef com_openml_JEDB_SQL_DATA_BYTE
#define com_openml_JEDB_SQL_DATA_BYTE 14L
#undef com_openml_JEDB_SQL_DATA_VARBYTE
#define com_openml_JEDB_SQL_DATA_VARBYTE 15L
#undef com_openml_JEDB_SQL_DATA_TIMESTAMP
#define com_openml_JEDB_SQL_DATA_TIMESTAMP 18L
#undef com_openml_JEDB_SQL_DATA_DATETIME
#define com_openml_JEDB_SQL_DATA_DATETIME 19L
#undef com_openml_JEDB_SQL_DATA_DATE
#define com_openml_JEDB_SQL_DATA_DATE 20L
#undef com_openml_JEDB_SQL_DATA_TIME
#define com_openml_JEDB_SQL_DATA_TIME 21L
#undef com_openml_JEDB_SQL_DATA_OID
#define com_openml_JEDB_SQL_DATA_OID 23L
#undef com_openml_JEDB_SQL_STMT_NONE
#define com_openml_JEDB_SQL_STMT_NONE 0L
#undef com_openml_JEDB_SQL_STMT_SELECT
#define com_openml_JEDB_SQL_STMT_SELECT 1L
#undef com_openml_JEDB_SQL_STMT_INSERT
#define com_openml_JEDB_SQL_STMT_INSERT 2L
#undef com_openml_JEDB_SQL_STMT_UPDATE
#define com_openml_JEDB_SQL_STMT_UPDATE 3L
#undef com_openml_JEDB_SQL_STMT_DELETE
#define com_openml_JEDB_SQL_STMT_DELETE 4L
#undef com_openml_JEDB_SQL_STMT_UPSERT
#define com_openml_JEDB_SQL_STMT_UPSERT 5L
#undef com_openml_JEDB_SQL_STMT_DESCRIBE
#define com_openml_JEDB_SQL_STMT_DESCRIBE 7L
#undef com_openml_JEDB_SQL_STMT_CREATE
#define com_openml_JEDB_SQL_STMT_CREATE 8L
#undef com_openml_JEDB_SQL_STMT_RENAME
#define com_openml_JEDB_SQL_STMT_RENAME 9L
#undef com_openml_JEDB_SQL_STMT_ALTER
#define com_openml_JEDB_SQL_STMT_ALTER 10L
#undef com_openml_JEDB_SQL_STMT_TRUNCATE
#define com_openml_JEDB_SQL_STMT_TRUNCATE 11L
#undef com_openml_JEDB_SQL_STMT_ADMIN
#define com_openml_JEDB_SQL_STMT_ADMIN 12L
#undef com_openml_JEDB_SQL_STMT_DROP
#define com_openml_JEDB_SQL_STMT_DROP 13L
#undef com_openml_JEDB_SQL_STMT_COMMIT
#define com_openml_JEDB_SQL_STMT_COMMIT 14L
#undef com_openml_JEDB_SQL_STMT_ROLLBACK
#define com_openml_JEDB_SQL_STMT_ROLLBACK 15L
#undef com_openml_JEDB_SQL_STMT_SET
#define com_openml_JEDB_SQL_STMT_SET 16L
/*
 * Class:     com_openml_JEDB
 * Method:    CreateDB1
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_openml_JEDB_CreateDB1
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_openml_JEDB
 * Method:    CreateDB2
 * Signature: (Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_openml_JEDB_CreateDB2
  (JNIEnv *, jclass, jstring, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    CreateDB3
 * Signature: (Ljava/lang/String;III)I
 */
JNIEXPORT jint JNICALL Java_com_openml_JEDB_CreateDB3
  (JNIEnv *, jclass, jstring, jint, jint, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    DestoryDB
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_openml_JEDB_DestoryDB
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_openml_JEDB
 * Method:    GetConnection
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_openml_JEDB_GetConnection
  (JNIEnv *, jclass);

/*
 * Class:     com_openml_JEDB
 * Method:    Connect
 * Signature: (ILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_openml_JEDB_Connect
  (JNIEnv *, jclass, jint, jstring);

/*
 * Class:     com_openml_JEDB
 * Method:    Disconnect
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_openml_JEDB_Disconnect
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    ReleaseConnection
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_openml_JEDB_ReleaseConnection
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    Commit
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_openml_JEDB_Commit
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    CommitFlush
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_openml_JEDB_CommitFlush
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    Rollback
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_openml_JEDB_Rollback
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    RollbackFlush
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_openml_JEDB_RollbackFlush
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    QueryCreate
 * Signature: (ILjava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_openml_JEDB_QueryCreate
  (JNIEnv *, jclass, jint, jstring, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    QueryDestroy
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_openml_JEDB_QueryDestroy
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    QueryBindParam
 * Signature: (IIIIILjava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_openml_JEDB_QueryBindParam
  (JNIEnv *, jclass, jint, jint, jint, jint, jint, jstring, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    QueryExecute
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_openml_JEDB_QueryExecute
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    QueryClearRow
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_openml_JEDB_QueryClearRow
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    QueryGetNextRow
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_openml_JEDB_QueryGetNextRow
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    QueryGetFieldCount
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_openml_JEDB_QueryGetFieldCount
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    QueryGetFieldData
 * Signature: (III)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_openml_JEDB_QueryGetFieldData
  (JNIEnv *, jclass, jint, jint, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    QueryIsFieldNull
 * Signature: (III)I
 */
JNIEXPORT jint JNICALL Java_com_openml_JEDB_QueryIsFieldNull
  (JNIEnv *, jclass, jint, jint, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    QueryGetFieldName
 * Signature: (III)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_openml_JEDB_QueryGetFieldName
  (JNIEnv *, jclass, jint, jint, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    QueryGetFieldType
 * Signature: (III)I
 */
JNIEXPORT jint JNICALL Java_com_openml_JEDB_QueryGetFieldType
  (JNIEnv *, jclass, jint, jint, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    QueryGetFieldLength
 * Signature: (III)I
 */
JNIEXPORT jint JNICALL Java_com_openml_JEDB_QueryGetFieldLength
  (JNIEnv *, jclass, jint, jint, jint);

/*
 * Class:     com_openml_JEDB
 * Method:    QueryGetQueryType
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_openml_JEDB_QueryGetQueryType
  (JNIEnv *, jclass, jint, jint);

#ifdef __cplusplus
}
#endif
#endif
