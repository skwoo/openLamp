// teladdrDlg.cpp : implementation file
//

#include "isql.h"
#include "db_api.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define USER "guest"
#define PASSWD "guest"

#define TEST_LOOP	100
#define NUM_INSERT	1000

//#define DBNAME "libtest"
char CREATE_DBNAME[64] = "\\skwoo\\db\\libtest";
char DBNAME[64]		   = "\\skwoo\\db\\libtest";
char DBNAME2[64]	   = "\\skwoo\\db2\\libtest";

int timediff(SYSTEMTIME *st, SYSTEMTIME *et);
void Start_Test(char* cfg_path, char* out_path, char* db_path);

void do_query(char *dbname, char *query);
void do_dml_query(char *dbname, char *query);

int do_dml_query2(iSQL *isql, char *query);
int do_dml_query2_nocommit(iSQL *isql, char *query);
int do_query2(iSQL *isql, char *query);
int do_query2_nocommit(iSQL *isql, char *query);
int do_query2_noprint(iSQL *isql, char *query);
int do_query2_msgdrafttable(iSQL *isql, char *query);

void test_thread();
void test_check_inserttime();
void test_checkDB2();
void test_bulkinsert();
void test_bulkinsert2();
void test_bulkinsert_segalloc();
void test_stephen();
void test_nologgingtable(int loop);
void test_checkpoint();
void test_RFID();
void test_not_avail_resource();
void test_dic_insertselect();
void test_dic();
void test_smallbuffercache();
void test_insert_fail();
void test_contact();
void test_zipcode(int loop);
void test_memoryalloc();
void test_aerodb();
void test_insert_delete_longfield_exit();
void test_insert_longfield_exit();
void test_insert_longfield_exit2();
void test_delete_longfield();
void test_insert_longfield2();
void test_decode();
void test_distinct();
void check_health();
void test_count_table();
void test_convert2();
void test_convert();
void test_groupby();
void test_countalltable();
void test_createdb();
void test_insert_longfield();
void test_err_query();
void test_double_connect();
void test_sysfields();
void test_db_path();
void test_dropdb_createdb();
void test_isql_connect();
void test_remove_db();
void test_lower_names();
void test_set_log_path();
void insert_test_api_null(int p);
void test();
void test_orderby();
int recreate_table(char *dbname);
int create_table(char *dbname);
void insert_test();
void insert_test_api();
void insert_test_api2();
void delete_test();
void select_test();
void insert_test2();
void test_shift();

bool execQuery( LPCTSTR pszFmt, ... );
bool updateRec( LPCTSTR pszKey, LPCTSTR pszValue );
bool insertRec( LPCTSTR pszKey, LPCTSTR pszValue );
bool deleteRec( LPCTSTR pszKey );
void commit();

bool	m_fConnected = true;
void *  m_pData;
char *  m_pszFolderPath;
int m_nLastErrorCode;

int teladdr_num = 0;

/////////////////////////////////////////////////////////////////////////////
// CTeladdrDlg message handlers

int testdb()
{
    int i;

    test_shift();

    test_thread();

    //test_RFID();

	//return 0;

    test_check_inserttime();

    //test_checkDB2();

    //test_bulkinsert_segalloc();

    test_bulkinsert();

    test_bulkinsert2();

    test_stephen();

#if 0
    for (i=0; i<10; i++)
    {
        test_nologgingtable(i);
        __SYSLOG("Done %d...\n", i);
    }

    test_checkpoint();

	test_not_avail_resource();
    
	test_dic_insertselect();

	test_dic();

	//Start_Test("\\test.cfg", "\\result", "\\db\\mbtest");

//    test_memoryalloc();
//    return TRUE;

	//test_countalltable();
//	return TRUE;

    //test_delete_longfield();

    //test_aerodb();

//    return TRUE;

	test_smallbuffercache();

	test_contact();
#endif

    Start_Test("\\test.cfg", "\\result", "\\db\\mbtest");

    for (i=0; i<=50; i++)
    {
    	test_zipcode(i);
        __SYSLOG("test_zipcode done... %d\n\n", i);
    }

	return TRUE;

    test_insert_longfield2();

    test_insert_delete_longfield_exit();
//    return FAIL;

    test_insert_longfield_exit();

    for (i=0; i<100; i++)
    {
        test_delete_longfield();
    
        test_insert_longfield_exit2();
    }

    test_insert_longfield();

	test_decode();

	test_insert_fail();

	test_distinct();

	test_count_table();

	check_health();

	test_convert2();

	test_convert();

	test_groupby();

	//return TRUE;

	test_createdb();

	test_err_query();

	test_isql_connect();

	test_dropdb_createdb();

	test_remove_db();

	test_sysfields();

	test_db_path();

//	test_double_connect();

	test_lower_names();

	test();

	test_orderby();

	return TRUE;
}

void test_shift()
{
    iSQL    isql;
    char dbname[] = "\\db\\nares";
    int p;

	if (iSQL_connect(&isql, NULL, dbname, USER, PASSWD) == NULL)
        return;

    for (p=0; p<50; p++)
    {
        __SYSLOG("%x: %x\n", p, (0x80>>((p)&0x7)));
    }

    iSQL_disconnect(&isql);
}

iSQL    isql_thread;
DWORD WINAPI ThreadProc( LPVOID lpParam ) ;

DWORD WINAPI ThreadProc( LPVOID lpParam ) 
{ 
    int ret;
    char dbname[] = "\\db\\nares";
    int i;
    char *query;

    iSQL_disconnect(&isql_thread);

    if (iSQL_connect(&isql_thread, NULL, dbname, USER, PASSWD) == NULL)
        return -1;

    query = (char *)malloc(2048);

    for (i=1; i<=50000; i++)
    {
	    sprintf(query, "INSERT INTO engdic VALUES('000000000%d', '%*d', %d)",
			    i, i % 1000, i, i);
	    ret = do_dml_query2_nocommit(&isql_thread, query);

		if (ret != 0)
        {
            iSQL_rollback(&isql_thread);
			break;
        }

        if (i % 200 == 0)
            iSQL_commit(&isql_thread);
	}

    iSQL_commit(&isql_thread);

    free(query);

    return 0; 
} 

void test_thread()
{
    DWORD dwThreadId;
    HANDLE hThread;
    int ret;
    char dbname[] = "\\db\\nares";
    DWORD state;
    int i, n, j;
    int wait_count;

#if 1
	//Drop_DB(dbname);
	if (createdb(dbname) < 0)
	{
		__SYSLOG("test_createdb: first createdb fail %s\n", dbname);
	}
#endif

	if (iSQL_connect(&isql_thread, NULL, dbname, USER, PASSWD) == NULL)
        return;

	ret = do_dml_query2(&isql_thread, 
			"create table engdic ("
                "word varchar(100), "
                "meaning varchar(2048),"
                "number int,"
                "primary key(word, meaning, number))");

    for (i=0; i<=10; i++)
    {
        if (i==0)
            goto selection;

        ret = do_query2(&isql_thread, "select count(*) from engdic;");
        
        ret = do_dml_query2(&isql_thread, 
			"truncate engdic");

        ret = do_query2(&isql_thread, "select count(*) from engdic;");

        hThread = CreateThread( 
                NULL,              // default security attributes
                0,                 // use default stack size  
                ThreadProc,        // thread function 
                NULL,             // argument to thread function 
                0,                 // use default creation flags 
                &dwThreadId);   // returns the thread identifier 

        //WaitForMultipleObjects(1, &hThread, TRUE, INFINITE);

        wait_count =  0;
        while ((state=WaitForSingleObject(hThread, 50)) == WAIT_TIMEOUT)
        {
            wait_count++;
        }

        if (state != WAIT_OBJECT_0)
            __SYSLOG("state %d, %d\n", state, GetLastError());

        CloseHandle(hThread);

selection:
        ret = do_query2(&isql_thread, "select count(*) from engdic;");

        {
            char *query;

            query = (char *)malloc(2048);

            for (j=1; j<=10; j++)
            {
                n = (rand() * rand()) / 50000 + 1;
                sprintf(query,
                    "select number, word, meaning from engdic "
                    "where word between '000000000%d' and '000000000%d' and "
                          "number between %d and %d "
                    "order by word, number desc",
                    n, n+10, n, n+5);
                ret = do_query2(&isql_thread, query);
                if (ret < 0)
                    __SYSLOG("rand select error %d\n", iSQL_errno(&isql_thread));

                sprintf(query,
                    "select number, word, min(word), max(word), "
                            "count(*), avg(number), "
                            "min(number), max(number) from engdic "
                    "where word between '000000000%d' and '000000000%d' and "
                          "number between %d and %d "
                    "group by number, word "
                    "order by word, number desc",
                    n, n+100, n, n+100);
                ret = do_query2(&isql_thread, query);
                if (ret < 0)
                    __SYSLOG("rand select error %d\n", iSQL_errno(&isql_thread));
            }

            free(query);
        }

        __SYSLOG("Done %d, %d waited\n", i, wait_count);
        __SYSLOG("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");
    }

    ret = do_query2(&isql_thread, "select count(*) from systables;");
    ret = do_query2(&isql_thread, "select count(*) from engdic;");

    iSQL_disconnect(&isql_thread);
}

void test_check_inserttime()
{
	iSQL		isql;
	int i, j;
	char query[2048];
	char dbname[] = "\\db\\nares";
	int ret;
	int gap;
	int count;
	SYSTEMTIME st, et;
	char s_word[100];
	char s_meaning[2048];
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			num_fields;
	int num;
	//unsigned long int *lengths;

#if 1
	Drop_DB(dbname);
	if (createdb(dbname) < 0)
	{
		__SYSLOG("test_createdb: first createdb fail %s\n", dbname);
	}
#endif

 	GetSystemTime(&st);

	if (iSQL_connect(&isql, NULL, dbname, USER, PASSWD) == NULL)
        return;

	GetSystemTime(&et);

	gap = timediff(&st, &et);
	
	__SYSLOG("[%d] connection time... %d\n", i, gap);

    sprintf(query, "select * from systables;");
	ret = do_query2(&isql, query);

	sprintf(query, "select count(*) from engdic");
	ret = do_query2_nocommit(&isql, query);

	ret = do_dml_query2(&isql, 
			"drop table engdic");

	ret = do_dml_query2(&isql, 
			"create table engdic (word varchar(100) primary key, meaning varchar(2048))");

	for (i=0,j=1; j<=5; j++)
    {
        GetSystemTime(&st);

	    for (;i<j*10000; i++)
	    {
		    sprintf(query, "INSERT INTO engdic VALUES('000000000%d', '%*d')",
			    i, i % 1000, i);
		    ret = do_dml_query2_nocommit(&isql, query);

		    if (ret != 0)
			    break;
	    }

        iSQL_commit(&isql);

	    GetSystemTime(&et);

	    gap = timediff(&st, &et);
	    
    	__SYSLOG("%d inserted... %d\n", i, gap);
    }

	sprintf(query, "select count(*) from engdic");
	ret = do_query2(&isql, query);

	iSQL_disconnect(&isql);
}

void test_checkDB2()
{
	iSQL		isql;
	int i, j;
	char query[2048];
	char dbname[] = "\\db\\DB2";
	int ret;
	int gap;
	int count;
	SYSTEMTIME st, et;
	char s_word[100];
	char s_meaning[2048];
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			num_fields;
	int num;
	//unsigned long int *lengths;

 	GetSystemTime(&st);

	if (iSQL_connect(&isql, NULL, dbname, USER, PASSWD) == NULL)
	{
		__SYSLOG("isql connect fail\n");
        return;
	}

	GetSystemTime(&et);

	gap = timediff(&st, &et);
	
	__SYSLOG("[%d] connection time... %d\n", i, gap);

	sprintf(query, "select * from systables");
	ret = do_query2(&isql, query);

	iSQL_disconnect(&isql);
}

FIELDDESC_T _msgdrafttable_8[] =
{
	{"field_8",   DT_INTEGER, 0, 0, 0,  0, 0, 0, FIXED_VARIABLE, 
		MDB_COL_NUMERIC, "\0"},
    {"field_8",   DT_INTEGER, 0, 0, 0,  0, 0, 0, FIXED_VARIABLE, 
	MDB_COL_NUMERIC, "\0"}
};

void test_bulkinsert_segalloc()
{
	iSQL		isql;
	int i, j;
	char query[2048];
	char dbname[] = "\\db\\ssp";
	int ret;
	int gap;
	int count;
	SYSTEMTIME st, et;
	char s_word[100];
	char s_meaning[2048];
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			num_fields;
	int num;
	//unsigned long int *lengths;
    int cursor;
    struct t_msgdrafttable {
        int     field_0;
        int     field_1;
        int     field_2;
        int     field_3;
        int     field_4;
        int     field_5;
        int     field_6;
        int     field_7;
        int     field_8;
        wchar_t field_9[43];
        wchar_t field_10[43];
        wchar_t field_11[43];
        int     field_12;
        int     field_13;
        int     field_14;
        int     field_15;
        int     field_16;
        long    dummy[DUMMY_FIELD_LEN(17)];
    } rec;
    int rec_len;

#if 0
    Drop_DB(dbname);

    if (createdb(dbname) < 0)
	{
		__SYSLOG("test_createdb: first createdb fail %s\n", dbname);
	}
#endif

 	GetSystemTime(&st);

	if (iSQL_connect(&isql, NULL, dbname, USER, PASSWD) == NULL)
	{
		__SYSLOG("isql connect fail\n");
        return;
	}

	GetSystemTime(&et);

    do_dml_query2(&isql, 
        "CREATE TABLE msginboxtable ("
            "field_0 INT NOT NULL,"
            "field_1 INT NOT NULL,"
            "field_2 INT NOT NULL,"
            "field_3 INT NOT NULL,"
            "field_4 INT NOT NULL,"
            "field_5 INT NOT NULL,"
            "field_6 INT NOT NULL,"
            "field_7 INT NOT NULL,"
            "field_8 INT NOT NULL,"
            "field_9 NVARCHAR(42) NOT NULL,"
            "field_10 NVARCHAR(42) NOT NULL,"
            "field_11 NVARCHAR(42) NOT NULL,"
            "field_12 INT NOT NULL,"
            "field_13 INT NOT NULL,"
            "field_14 INT NOT NULL,"
            "field_15 INT NOT NULL,"
            "field_16 INT NOT NULL);");
    do_dml_query2(&isql, 
        "CREATE UNIQUE INDEX msginboxtable ON msginboxtable(field_0);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msginboxtable_8 ON msginboxtable(field_8,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msginboxtable_10 ON msginboxtable(field_10,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msginboxtable_11 ON msginboxtable(field_11,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msginboxtable_4 ON msginboxtable(field_4,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msginboxtable_1 ON msginboxtable(field_1,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msginboxtable_12 ON msginboxtable(field_12,field_8);");

    do_dml_query2(&isql, 
        "CREATE TABLE msgdrafttable ("
        //"CREATE NOLOGGING TABLE msgdrafttable ("
            "field_0 INT NOT NULL,"
            "field_1 INT NOT NULL,"
            "field_2 INT NOT NULL,"
            "field_3 INT NOT NULL,"
            "field_4 INT NOT NULL,"
            "field_5 INT NOT NULL,"
            "field_6 INT NOT NULL,"
            "field_7 INT NOT NULL,"
            "field_8 INT NOT NULL,"
            "field_9 NVARCHAR(42) NOT NULL,"
            "field_10 NVARCHAR(42) NOT NULL,"
            "field_11 NVARCHAR(42) NOT NULL,"
            "field_12 INT NOT NULL,"
            "field_13 INT NOT NULL,"
            "field_14 INT NOT NULL,"
            "field_15 INT NOT NULL,"
            "field_16 INT NOT NULL);");
    do_dml_query2(&isql, 
        "CREATE UNIQUE INDEX msgdrafttable ON msgdrafttable(field_0);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgdrafttable_8 ON msgdrafttable(field_8,field_8);");
    
    do_dml_query2(&isql, 
        "CREATE  INDEX msgdrafttable_10 ON msgdrafttable(field_10,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgdrafttable_11 ON msgdrafttable(field_11,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgdrafttable_4 ON msgdrafttable(field_4,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgdrafttable_1 ON msgdrafttable(field_1,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgdrafttable_12 ON msgdrafttable(field_12,field_8);");

    do_dml_query2(&isql, 
        "CREATE TABLE msgsenttable ("
            "field_0 INT NOT NULL,"
            "field_1 INT NOT NULL,"
            "field_2 INT NOT NULL,"
            "field_3 INT NOT NULL,"
            "field_4 INT NOT NULL,"
            "field_5 INT NOT NULL,"
            "field_6 INT NOT NULL,"
            "field_7 INT NOT NULL,"
            "field_8 INT NOT NULL,"
            "field_9 NVARCHAR(42) NOT NULL,"
            "field_10 NVARCHAR(42) NOT NULL,"
            "field_11 NVARCHAR(42) NOT NULL,"
            "field_12 INT NOT NULL,"
            "field_13 INT NOT NULL,"
            "field_14 INT NOT NULL,"
            "field_15 INT NOT NULL,"
            "field_16 INT NOT NULL);");
    do_dml_query2(&isql, 
        "CREATE UNIQUE INDEX msgsenttable ON msgsenttable(field_0);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgsenttable_8 ON msgsenttable(field_8,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgsenttable_10 ON msgsenttable(field_10,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgsenttable_11 ON msgsenttable(field_11,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgsenttable_4 ON msgsenttable(field_4,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgsenttable_1 ON msgsenttable(field_1,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgsenttable_12 ON msgsenttable(field_12,field_8);");

    do_dml_query2(&isql, 
        "CREATE TABLE msgmailboxtable ("
            "field_0 INT NOT NULL,"
            "field_1 INT NOT NULL,"
            "field_2 INT NOT NULL,"
            "field_3 INT NOT NULL,"
            "field_4 INT NOT NULL,"
            "field_5 INT NOT NULL,"
            "field_6 INT NOT NULL,"
            "field_7 INT NOT NULL,"
            "field_8 INT NOT NULL,"
            "field_9 NVARCHAR(42) NOT NULL,"
            "field_10 NVARCHAR(42) NOT NULL,"
            "field_11 NVARCHAR(42) NOT NULL,"
            "field_12 INT NOT NULL,"
            "field_13 INT NOT NULL,"
            "field_14 INT NOT NULL,"
            "field_15 INT NOT NULL,"
            "field_16 INT NOT NULL);");
    do_dml_query2(&isql, 
        "CREATE UNIQUE INDEX msgmailboxtable ON msgmailboxtable(field_0);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgmailboxtable_8 ON msgmailboxtable(field_8,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgmailboxtable_10 ON msgmailboxtable(field_10,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgmailboxtable_11 ON msgmailboxtable(field_11,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgmailboxtable_4 ON msgmailboxtable(field_4,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgmailboxtable_1 ON msgmailboxtable(field_1,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgmailboxtable_12 ON msgmailboxtable(field_12,field_8);	");

	gap = timediff(&st, &et);
	
	__SYSLOG("[%d] connection time... %d\n", i, gap);

	sprintf(query, "select count(*) from msgdrafttable");
	ret = do_query2(&isql, query);

	GetSystemTime(&st);

    for (j=0; j<2; j++)
    {
        __SYSLOG("+++++++++++++++++++++++++ %d ++++++++++++++++++++++++++\n", j);
        ret = 0;
	    for (i=0; i<100; i++)
	    {
		    sprintf(query, "INSERT INTO msgdrafttable VALUES('%d','%d','%d','%d',"
                                "'%d','%d','%d','%d','%d',"
                                "n'%10d%0d',n'%10d%0d',n'%10d%0d',"
                                "'%d','%d','%d','%d','%d')",
			    i, i, i, i, 
                i, i, i, i, i, 
                i % 100, i %100, i % 100, i%100, i%100, i%100, 
                i, i, i, i, i);
		    ret = do_dml_query2_nocommit(&isql, query);

		    if (ret != 0)
			    break;
	    }

        iSQL_commit(&isql);

        __SYSLOG("inserted\n");
	    sprintf(query, "select count(*) from msgdrafttable");
	    ret = do_query2(&isql, query);

	    //sprintf(query, "select * from msgdrafttable");
	    //ret = do_query2_msgdrafttable(&isql, query);

	    for (i=0; i<500; i++)
	    {
		    sprintf(query, "DELETE FROM msgdrafttable WHERE field_0 = %d;",
			    i);
		    ret = do_dml_query2(&isql, query);

            if (ret != 0)
			    break;
	    }

        __SYSLOG("deleted\n");
	    sprintf(query, "select count(*) from msgdrafttable");
	    ret = do_query2(&isql, query);
    }

    GetSystemTime(&et);

  	gap = timediff(&st, &et);
	
	__SYSLOG("[%d] insert-delete time... %d\n", i, gap);

	iSQL_disconnect(&isql);
}

void test_bulkinsert()
{
	iSQL		isql;
	int i, j;
	char query[2048];
	char dbname[] = "\\db\\ssp";
	int ret;
	int gap;
	int count;
	SYSTEMTIME st, et;
	char s_word[100];
	char s_meaning[2048];
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			num_fields;
	int num;
	//unsigned long int *lengths;
    int cursor;
    struct t_msgdrafttable {
        int     field_0;
        int     field_1;
        int     field_2;
        int     field_3;
        int     field_4;
        int     field_5;
        int     field_6;
        int     field_7;
        int     field_8;
        wchar_t field_9[43];
        wchar_t field_10[43];
        wchar_t field_11[43];
        int     field_12;
        int     field_13;
        int     field_14;
        int     field_15;
        int     field_16;
        long    dummy[DUMMY_FIELD_LEN(17)];
    } rec;
    int rec_len;

#if 1
    Drop_DB(dbname);
#endif

    if (createdb(dbname) < 0)
	{
		__SYSLOG("test_createdb: first createdb fail %s\n", dbname);
	}

 	GetSystemTime(&st);

	if (iSQL_connect(&isql, NULL, dbname, USER, PASSWD) == NULL)
	{
		__SYSLOG("isql connect fail\n");
        return;
	}

	GetSystemTime(&et);

    do_dml_query2(&isql, 
        "CREATE TABLE msginboxtable ("
            "field_0 INT NOT NULL,"
            "field_1 INT NOT NULL,"
            "field_2 INT NOT NULL,"
            "field_3 INT NOT NULL,"
            "field_4 INT NOT NULL,"
            "field_5 INT NOT NULL,"
            "field_6 INT NOT NULL,"
            "field_7 INT NOT NULL,"
            "field_8 INT NOT NULL,"
            "field_9 NVARCHAR(42) NOT NULL,"
            "field_10 NVARCHAR(42) NOT NULL,"
            "field_11 NVARCHAR(42) NOT NULL,"
            "field_12 INT NOT NULL,"
            "field_13 INT NOT NULL,"
            "field_14 INT NOT NULL,"
            "field_15 INT NOT NULL,"
            "field_16 INT NOT NULL);");
    do_dml_query2(&isql, 
        "CREATE UNIQUE INDEX msginboxtable ON msginboxtable(field_0);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msginboxtable_8 ON msginboxtable(field_8,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msginboxtable_10 ON msginboxtable(field_10,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msginboxtable_11 ON msginboxtable(field_11,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msginboxtable_4 ON msginboxtable(field_4,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msginboxtable_1 ON msginboxtable(field_1,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msginboxtable_12 ON msginboxtable(field_12,field_8);");

    do_dml_query2(&isql, 
        "CREATE TABLE msgdrafttable ("
        //"CREATE NOLOGGING TABLE msgdrafttable ("
            "field_0 INT NOT NULL,"
            "field_1 INT NOT NULL,"
            "field_2 INT NOT NULL,"
            "field_3 INT NOT NULL,"
            "field_4 INT NOT NULL,"
            "field_5 INT NOT NULL,"
            "field_6 INT NOT NULL,"
            "field_7 INT NOT NULL,"
            "field_8 INT NOT NULL,"
            "field_9 NVARCHAR(42) NOT NULL,"
            "field_10 NVARCHAR(42) NOT NULL,"
            "field_11 NVARCHAR(42) NOT NULL,"
            "field_12 INT NOT NULL,"
            "field_13 INT NOT NULL,"
            "field_14 INT NOT NULL,"
            "field_15 INT NOT NULL,"
            "field_16 INT NOT NULL);");
    do_dml_query2(&isql, 
        "CREATE UNIQUE INDEX msgdrafttable ON msgdrafttable(field_0);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgdrafttable_8 ON msgdrafttable(field_8,field_8);");

    do_dml_query2(&isql, 
        "CREATE  INDEX msgdrafttable_10 ON msgdrafttable(field_10,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgdrafttable_11 ON msgdrafttable(field_11,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgdrafttable_4 ON msgdrafttable(field_4,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgdrafttable_1 ON msgdrafttable(field_1,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgdrafttable_12 ON msgdrafttable(field_12,field_8);");

    do_dml_query2(&isql, 
        "CREATE TABLE msgsenttable ("
            "field_0 INT NOT NULL,"
            "field_1 INT NOT NULL,"
            "field_2 INT NOT NULL,"
            "field_3 INT NOT NULL,"
            "field_4 INT NOT NULL,"
            "field_5 INT NOT NULL,"
            "field_6 INT NOT NULL,"
            "field_7 INT NOT NULL,"
            "field_8 INT NOT NULL,"
            "field_9 NVARCHAR(42) NOT NULL,"
            "field_10 NVARCHAR(42) NOT NULL,"
            "field_11 NVARCHAR(42) NOT NULL,"
            "field_12 INT NOT NULL,"
            "field_13 INT NOT NULL,"
            "field_14 INT NOT NULL,"
            "field_15 INT NOT NULL,"
            "field_16 INT NOT NULL);");
    do_dml_query2(&isql, 
        "CREATE UNIQUE INDEX msgsenttable ON msgsenttable(field_0);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgsenttable_8 ON msgsenttable(field_8,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgsenttable_10 ON msgsenttable(field_10,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgsenttable_11 ON msgsenttable(field_11,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgsenttable_4 ON msgsenttable(field_4,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgsenttable_1 ON msgsenttable(field_1,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgsenttable_12 ON msgsenttable(field_12,field_8);");

    do_dml_query2(&isql, 
        "CREATE TABLE msgmailboxtable ("
            "field_0 INT NOT NULL,"
            "field_1 INT NOT NULL,"
            "field_2 INT NOT NULL,"
            "field_3 INT NOT NULL,"
            "field_4 INT NOT NULL,"
            "field_5 INT NOT NULL,"
            "field_6 INT NOT NULL,"
            "field_7 INT NOT NULL,"
            "field_8 INT NOT NULL,"
            "field_9 NVARCHAR(42) NOT NULL,"
            "field_10 NVARCHAR(42) NOT NULL,"
            "field_11 NVARCHAR(42) NOT NULL,"
            "field_12 INT NOT NULL,"
            "field_13 INT NOT NULL,"
            "field_14 INT NOT NULL,"
            "field_15 INT NOT NULL,"
            "field_16 INT NOT NULL);");
    do_dml_query2(&isql, 
        "CREATE UNIQUE INDEX msgmailboxtable ON msgmailboxtable(field_0);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgmailboxtable_8 ON msgmailboxtable(field_8,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgmailboxtable_10 ON msgmailboxtable(field_10,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgmailboxtable_11 ON msgmailboxtable(field_11,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgmailboxtable_4 ON msgmailboxtable(field_4,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgmailboxtable_1 ON msgmailboxtable(field_1,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgmailboxtable_12 ON msgmailboxtable(field_12,field_8);	");

	gap = timediff(&st, &et);
	
	__SYSLOG("[%d] connection time... %d\n", i, gap);

	sprintf(query, "select count(*) from msgdrafttable");
	ret = do_query2(&isql, query);

	GetSystemTime(&st);

    for (j=0; j<2; j++)
    {
        __SYSLOG("+++++++++++++++++++++++++ %d ++++++++++++++++++++++++++\n", j);
        ret = 0;
	    for (i=0; i<500; i++)
	    {
		    sprintf(query, "INSERT INTO msgdrafttable VALUES('%d','%d','%d','%d',"
                                "'%d','%d','%d','%d','%d',"
                                "n'%10d%0d',n'%10d%0d',n'%10d%0d',"
                                "'%d','%d','%d','%d','%d')",
			    i, i, i, i, 
                i, i, i, i, i, 
                i % 100, i %100, i % 100, i%100, i%100, i%100, 
                i, i, i, i, i);
		    ret = do_dml_query2(&isql, query);

		    if (ret != 0)
			    break;
	    }

//        iSQL_commit(&isql);

        __SYSLOG("inserted\n");
        for (i=0; i<1; i++)
        {
	    sprintf(query, "select count(*) from msgdrafttable");
	    ret = do_query2(&isql, query);
        }

	    //sprintf(query, "select * from msgdrafttable");
	    //ret = do_query2_msgdrafttable(&isql, query);

#if 0
        ret = DB_Trans_Begin(isql.handle);
        if (ret < 0)
            break;

        ret = cursor = DB_Cursor_Open(isql.handle, "msgdrafttable", 
            "msgdrafttable_11", NULL, NULL, NULL, LK_EXCLUSIVE);
        if (ret < 0)
        {
            DB_Trans_Abort(isql.handle);
            break;
        }

        while (1)
        {
            ret = DB_Cursor_Read(isql.handle, cursor, (char*)&rec, &rec_len, 1);
            if (ret < 0)
            {
                break;
            }

            __SYSLOG("%d %s %s %s\n", rec.field_0, rec.field_9, rec.field_10,
                rec.field_11);
        }

        if (ret < 0)
        {
            DB_Trans_Abort(isql.handle);
            goto remove;
        }

        ret = DB_Cursor_Close(isql.handle, cursor);
        if (ret < 0)
        {
            DB_Trans_Abort(isql.handle);
            goto remove;
        }

        ret = DB_Trans_Commit(isql.handle);
        if (ret < 0)
        {
            DB_Trans_Abort(isql.handle);
            goto remove;
        }

remove:
#endif

	    for (i=0; i<500; i++)
	    {
#if 0
            ret = DB_Trans_Begin(isql.handle);
            if (ret < 0)
                break;

            ret = cursor = DB_Cursor_Open(isql.handle, "msgdrafttable", "msgdrafttable",
                NULL, NULL, NULL, LK_EXCLUSIVE);
            if (ret < 0)
            {
                DB_Trans_Abort(isql.handle);
                break;
            }

            ret = DB_Cursor_Seek(isql.handle, cursor, 0);
            if (ret < 0)
            {
                DB_Trans_Abort(isql.handle);
                break;
            }

            ret = DB_Cursor_Remove(isql.handle, cursor);
            if (ret < 0)
            {
                DB_Trans_Abort(isql.handle);
                break;
            }

            ret = DB_Cursor_Close(isql.handle, cursor);
            if (ret < 0)
            {
                DB_Trans_Abort(isql.handle);
                break;
            }

            ret = DB_Trans_Commit(isql.handle);
            if (ret < 0)
            {
                DB_Trans_Abort(isql.handle);
                break;
            }
#else
		    sprintf(query, "DELETE FROM msgdrafttable WHERE field_0 = %d;",
			    i);
		    ret = do_dml_query2(&isql, query);
#endif
		    if (ret != 0)
			    break;
	    }

        __SYSLOG("deleted\n");
	    sprintf(query, "select count(*) from msgdrafttable");
	    ret = do_query2(&isql, query);
    }

    GetSystemTime(&et);

  	gap = timediff(&st, &et);
	
	__SYSLOG("[%d] insert-delete time... %d\n", i, gap);

	iSQL_disconnect(&isql);
}

void test_bulkinsert2()
{
	iSQL		isql;
	int i, j;
	char query[2048];
	char dbname[] = "\\db\\ssp";
	int ret;
	int gap;
	int count;
	SYSTEMTIME st, et;
	char s_word[100];
	char s_meaning[2048];
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			num_fields;
	int num;
	//unsigned long int *lengths;
    int cursor;
    struct t_msgdrafttable {
        int     field_0;
        int     field_1;
        int     field_2;
        int     field_3;
        int     field_4;
        int     field_5;
        int     field_6;
        int     field_7;
        int     field_8;
        wchar_t field_9[43];
        wchar_t field_10[43];
        wchar_t field_11[43];
        int     field_12;
        int     field_13;
        int     field_14;
        int     field_15;
        int     field_16;
        long    dummy[DUMMY_FIELD_LEN(17)];
    } rec;
    int rec_len;

#if 0
    Drop_DB(dbname);
#endif

    if (createdb(dbname) < 0)
	{
		__SYSLOG("test_createdb: first createdb fail %s\n", dbname);
	}

 	GetSystemTime(&st);

	if (iSQL_connect(&isql, NULL, dbname, USER, PASSWD) == NULL)
	{
		__SYSLOG("isql connect fail\n");
        return;
	}

	GetSystemTime(&et);

    do_dml_query2(&isql, 
        "CREATE TABLE msginboxtable ("
            "field_0 INT NOT NULL,"
            "field_1 INT NOT NULL,"
            "field_2 INT NOT NULL,"
            "field_3 INT NOT NULL,"
            "field_4 INT NOT NULL,"
            "field_5 INT NOT NULL,"
            "field_6 INT NOT NULL,"
            "field_7 INT NOT NULL,"
            "field_8 INT NOT NULL,"
            "field_9 NVARCHAR(42) NOT NULL,"
            "field_10 NVARCHAR(42) NOT NULL,"
            "field_11 NVARCHAR(42) NOT NULL,"
            "field_12 INT NOT NULL,"
            "field_13 INT NOT NULL,"
            "field_14 INT NOT NULL,"
            "field_15 INT NOT NULL,"
            "field_16 INT NOT NULL);");
    do_dml_query2(&isql, 
        "CREATE UNIQUE INDEX msginboxtable ON msginboxtable(field_0);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msginboxtable_8 ON msginboxtable(field_8,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msginboxtable_10 ON msginboxtable(field_10,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msginboxtable_11 ON msginboxtable(field_11,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msginboxtable_4 ON msginboxtable(field_4,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msginboxtable_1 ON msginboxtable(field_1,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msginboxtable_12 ON msginboxtable(field_12,field_8);");

    do_dml_query2(&isql, 
        "CREATE TABLE msgdrafttable ("
        //"CREATE NOLOGGING TABLE msgdrafttable ("
            "field_0 INT NOT NULL,"
            "field_1 INT NOT NULL,"
            "field_2 INT NOT NULL,"
            "field_3 INT NOT NULL,"
            "field_4 INT NOT NULL,"
            "field_5 INT NOT NULL,"
            "field_6 INT NOT NULL,"
            "field_7 INT NOT NULL,"
            "field_8 INT NOT NULL,"
            "field_9 NVARCHAR(42) NOT NULL,"
            "field_10 NVARCHAR(42) NOT NULL,"
            "field_11 NVARCHAR(42) NOT NULL,"
            "field_12 INT NOT NULL,"
            "field_13 INT NOT NULL,"
            "field_14 INT NOT NULL,"
            "field_15 INT NOT NULL,"
            "field_16 INT NOT NULL);");
    do_dml_query2(&isql, 
        "CREATE UNIQUE INDEX msgdrafttable ON msgdrafttable(field_0);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgdrafttable_8 ON msgdrafttable(field_8,field_8);");

    do_dml_query2(&isql, 
        "CREATE  INDEX msgdrafttable_10 ON msgdrafttable(field_10,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgdrafttable_11 ON msgdrafttable(field_11,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgdrafttable_4 ON msgdrafttable(field_4,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgdrafttable_1 ON msgdrafttable(field_1,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgdrafttable_12 ON msgdrafttable(field_12,field_8);");

    do_dml_query2(&isql, 
        "CREATE TABLE msgsenttable ("
            "field_0 INT NOT NULL,"
            "field_1 INT NOT NULL,"
            "field_2 INT NOT NULL,"
            "field_3 INT NOT NULL,"
            "field_4 INT NOT NULL,"
            "field_5 INT NOT NULL,"
            "field_6 INT NOT NULL,"
            "field_7 INT NOT NULL,"
            "field_8 INT NOT NULL,"
            "field_9 NVARCHAR(42) NOT NULL,"
            "field_10 NVARCHAR(42) NOT NULL,"
            "field_11 NVARCHAR(42) NOT NULL,"
            "field_12 INT NOT NULL,"
            "field_13 INT NOT NULL,"
            "field_14 INT NOT NULL,"
            "field_15 INT NOT NULL,"
            "field_16 INT NOT NULL);");
    do_dml_query2(&isql, 
        "CREATE UNIQUE INDEX msgsenttable ON msgsenttable(field_0);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgsenttable_8 ON msgsenttable(field_8,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgsenttable_10 ON msgsenttable(field_10,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgsenttable_11 ON msgsenttable(field_11,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgsenttable_4 ON msgsenttable(field_4,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgsenttable_1 ON msgsenttable(field_1,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgsenttable_12 ON msgsenttable(field_12,field_8);");

    do_dml_query2(&isql, 
        "CREATE TABLE msgmailboxtable ("
            "field_0 INT NOT NULL,"
            "field_1 INT NOT NULL,"
            "field_2 INT NOT NULL,"
            "field_3 INT NOT NULL,"
            "field_4 INT NOT NULL,"
            "field_5 INT NOT NULL,"
            "field_6 INT NOT NULL,"
            "field_7 INT NOT NULL,"
            "field_8 INT NOT NULL,"
            "field_9 NVARCHAR(42) NOT NULL,"
            "field_10 NVARCHAR(42) NOT NULL,"
            "field_11 NVARCHAR(42) NOT NULL,"
            "field_12 INT NOT NULL,"
            "field_13 INT NOT NULL,"
            "field_14 INT NOT NULL,"
            "field_15 INT NOT NULL,"
            "field_16 INT NOT NULL);");
    do_dml_query2(&isql, 
        "CREATE UNIQUE INDEX msgmailboxtable ON msgmailboxtable(field_0);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgmailboxtable_8 ON msgmailboxtable(field_8,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgmailboxtable_10 ON msgmailboxtable(field_10,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgmailboxtable_11 ON msgmailboxtable(field_11,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgmailboxtable_4 ON msgmailboxtable(field_4,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgmailboxtable_1 ON msgmailboxtable(field_1,field_8);");
    do_dml_query2(&isql, 
        "CREATE  INDEX msgmailboxtable_12 ON msgmailboxtable(field_12,field_8);	");

	gap = timediff(&st, &et);
	
	__SYSLOG("[%d] connection time... %d\n", i, gap);

	sprintf(query, "select count(*) from msgdrafttable");
	ret = do_query2(&isql, query);

	GetSystemTime(&st);

    for (j=0; j<2; j++)
    {
        __SYSLOG("+++++++++++++++++++++++++ %d ++++++++++++++++++++++++++\n", j);
        ret = 0;
	    for (i=0; i<600; i++)
	    {
		    sprintf(query, "INSERT INTO msgdrafttable VALUES('%d','%d','%d','%d',"
                                "'%d','%d','%d','%d','%d',"
                                "n'%10d%0d',n'%10d%0d',n'%10d%0d',"
                                "'%d','%d','%d','%d','%d')",
			    i, i, i, i, 
                i, i, i, i, i, 
                i % 100, i %100, i % 100, i%100, i%100, i%100, 
                i, i, i, i, i);
		    ret = do_dml_query2_nocommit(&isql, query);

		    if (ret != 0)
			    break;
	    }

        iSQL_commit(&isql);

        __SYSLOG("inserted\n");
	    sprintf(query, "select count(*) from msgdrafttable");
	    ret = do_query2(&isql, query);

	    for (i=0; i<600; i++)
	    {
		    sprintf(query, "INSERT INTO msgmailboxtable VALUES('%d','%d','%d','%d',"
                                "'%d','%d','%d','%d','%d',"
                                "n'%10d%0d',n'%10d%0d',n'%10d%0d',"
                                "'%d','%d','%d','%d','%d')",
			    i, i, i, i, 
                i, i, i, i, i, 
                i % 100, i %100, i % 100, i%100, i%100, i%100, 
                i, i, i, i, i);
		    ret = do_dml_query2_nocommit(&isql, query);

		    if (ret != 0)
			    break;
	    }

        iSQL_commit(&isql);

        __SYSLOG("inserted\n");
	    sprintf(query, "select count(*) from msgmailboxtable");
	    ret = do_query2(&isql, query);

	    //sprintf(query, "select * from msgdrafttable");
	    //ret = do_query2_msgdrafttable(&isql, query);

#if 0
        ret = DB_Trans_Begin(isql.handle);
        if (ret < 0)
            break;

        ret = cursor = DB_Cursor_Open(isql.handle, "msgdrafttable", 
            "msgdrafttable_11", NULL, NULL, NULL, LK_EXCLUSIVE);
        if (ret < 0)
        {
            DB_Trans_Abort(isql.handle);
            break;
        }

        while (1)
        {
            ret = DB_Cursor_Read(isql.handle, cursor, (char*)&rec, &rec_len, 1);
            if (ret < 0)
            {
                break;
            }

            __SYSLOG("%d %s %s %s\n", rec.field_0, rec.field_9, rec.field_10,
                rec.field_11);
        }

        if (ret < 0)
        {
            DB_Trans_Abort(isql.handle);
            goto remove;
        }

        ret = DB_Cursor_Close(isql.handle, cursor);
        if (ret < 0)
        {
            DB_Trans_Abort(isql.handle);
            goto remove;
        }

        ret = DB_Trans_Commit(isql.handle);
        if (ret < 0)
        {
            DB_Trans_Abort(isql.handle);
            goto remove;
        }

remove:
#endif

	    for (i=0; i<500; i++)
	    {
#if 0
            ret = DB_Trans_Begin(isql.handle);
            if (ret < 0)
                break;

            ret = cursor = DB_Cursor_Open(isql.handle, "msgdrafttable", "msgdrafttable",
                NULL, NULL, NULL, LK_EXCLUSIVE);
            if (ret < 0)
            {
                DB_Trans_Abort(isql.handle);
                break;
            }

            ret = DB_Cursor_Seek(isql.handle, cursor, 0);
            if (ret < 0)
            {
                DB_Trans_Abort(isql.handle);
                break;
            }

            ret = DB_Cursor_Remove(isql.handle, cursor);
            if (ret < 0)
            {
                DB_Trans_Abort(isql.handle);
                break;
            }

            ret = DB_Cursor_Close(isql.handle, cursor);
            if (ret < 0)
            {
                DB_Trans_Abort(isql.handle);
                break;
            }

            ret = DB_Trans_Commit(isql.handle);
            if (ret < 0)
            {
                DB_Trans_Abort(isql.handle);
                break;
            }
#else
		    sprintf(query, "DELETE FROM msgdrafttable WHERE field_0 = %d;",
			    i);
		    ret = do_dml_query2(&isql, query);
#endif
		    if (ret != 0)
			    break;
	    }

        __SYSLOG("deleted\n");
	    sprintf(query, "select count(*) from msgdrafttable");
	    ret = do_query2(&isql, query);

        for (i=0; i<500; i++)
	    {
		    sprintf(query, "DELETE FROM msgmailboxtable WHERE field_0 = %d;",
			    i);
		    ret = do_dml_query2(&isql, query);
		    if (ret != 0)
			    break;
	    }

        __SYSLOG("deleted\n");
	    sprintf(query, "select count(*) from msgmailboxtable");
	    ret = do_query2(&isql, query);
    }

    GetSystemTime(&et);

  	gap = timediff(&st, &et);
	
	__SYSLOG("[%d] insert-delete time... %d\n", i, gap);

	iSQL_disconnect(&isql);
}

void test_stephen()
{
	iSQL		isql;
	int i, j;
	char query[2048];
	char dbname[] = "\\db\\dic";
	int ret;
	int gap;
	int count;
	SYSTEMTIME st, et;
	char s_word[100];
	char s_meaning[2048];
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			num_fields;
	int num;
	//unsigned long int *lengths;

    Drop_DB(dbname);
	if (createdb(dbname) < 0)
	{
		__SYSLOG("test_createdb: first createdb fail %s\n", dbname);
	}

 	GetSystemTime(&st);

	if (iSQL_connect(&isql, NULL, dbname, USER, PASSWD) == NULL)
	{
		__SYSLOG("isql connect fail\n");
        return;
	}

	GetSystemTime(&et);

	gap = timediff(&st, &et);
	
	__SYSLOG("[%d] connection time... %d\n", i, gap);

	sprintf(query, "select * from systables");
	ret = do_query2(&isql, query);

	iSQL_disconnect(&isql);
}

void test_nologgingtable(int loop)
{
	iSQL		isql;
	int i, j;
	char query[2048];
	char dbname[] = "\\db\\nares";
	int ret;
	int gap;
	int count;
	SYSTEMTIME st, et;
	char s_word[100];
	char s_meaning[2048];
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			num_fields;
	int num;
	//unsigned long int *lengths;

    if (loop == 0)
    {
	    Drop_DB(dbname);
	    if (createdb(dbname) < 0)
	    {
		    __SYSLOG("test_createdb: first createdb fail %s\n", dbname);
	    }
    }

 	GetSystemTime(&st);

	if (iSQL_connect(&isql, NULL, dbname, USER, PASSWD) == NULL)
	{
		__SYSLOG("isql connect fail\n");
        return;
	}

	GetSystemTime(&et);

	gap = timediff(&st, &et);
	
	__SYSLOG("[%d] connection time... %d\n", i, gap);

	sprintf(query, "select count(*) from engdic");
	ret = do_query2(&isql, query);

    if (loop == 0)
    {
	    ret = do_dml_query2(&isql, 
			    "drop table engdic");

	    ret = do_dml_query2(&isql, 
			    //"create nologging table engdic "
                "create table engdic "
                "(word char(100) primary key, meaning varchar(2048))");
    }

    ret = do_dml_query2(&isql, 
			"truncate engdic ");

	GetSystemTime(&st);

	for (i=0; i<10009; i++)
	{
		sprintf(query, "INSERT INTO engdic VALUES('000000000%d', '%*d')",
			i, i % 1000, i);
		ret = do_dml_query2_nocommit(&isql, query);

		if (ret != 0)
			break;

        if (i == 1000)
        {
            //ret = DB_Checkpoint(isql.handle);
            //iSQL_commit(&isql);
            //ret = DB_Checkpoint(isql.handle);
        }
	}

	GetSystemTime(&et);

	gap = timediff(&st, &et);
	
	__SYSLOG("%d inserted... %d\n", i, gap);

	sprintf(query, "select count(*) from engdic");
	ret = do_query2_nocommit(&isql, query);

    if (loop % 2 == 0)
        iSQL_commit(&isql);
    else
        iSQL_rollback(&isql);

	sprintf(query, "select count(*) from engdic");
	ret = do_query2(&isql, query);

	iSQL_disconnect(&isql);
}

void test_checkpoint()
{
	iSQL		isql;
	int i, j;
	char query[2048];
	char dbname[] = "\\db\\nares";
	int ret;
	int gap;
	int count;
	SYSTEMTIME st, et;
	char s_word[100];
	char s_meaning[2048];
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			num_fields;
	int num;
	//unsigned long int *lengths;

#if 1
	Drop_DB(dbname);
	if (createdb(dbname) < 0)
	{
		__SYSLOG("test_createdb: first createdb fail %s\n", dbname);
	}
#endif

 	GetSystemTime(&st);

	if (iSQL_connect(&isql, NULL, dbname, USER, PASSWD) == NULL)
	{
        int count = 0;
        while (count < 100)
        {
       	    Drop_DB(dbname);
            createdb(dbname);
       	    if (iSQL_connect(&isql, NULL, dbname, USER, PASSWD) == NULL)
            {
		        __SYSLOG("isql connect fail\n");
		        continue;
            }
            break;
        }
	}

	GetSystemTime(&et);

	gap = timediff(&st, &et);
	
	__SYSLOG("[%d] connection time... %d\n", i, gap);

	sprintf(query, "select count(*) from engdic");
	ret = do_query2_nocommit(&isql, query);

#if 1
	ret = do_dml_query2_nocommit(&isql, 
			"drop table engdic");

	ret = do_dml_query2_nocommit(&isql, 
			"create table engdic (word char(100) primary key, meaning varchar(2048))");

	GetSystemTime(&st);

	for (i=0; i<10009; i++)
	{
		sprintf(query, "INSERT INTO engdic VALUES('000000000%d', '%*d')",
			i, i % 1000, i);
		ret = do_dml_query2_nocommit(&isql, query);

		if (ret != 0)
			break;

        if (i == 1000)
        {
            ret = iSQL_checkpoint(&isql);
            iSQL_commit(&isql);
            ret = iSQL_checkpoint(&isql);
        }
	}

	GetSystemTime(&et);

	gap = timediff(&st, &et);
	
	__SYSLOG("%d inserted... %d\n", i, gap);
#endif

	sprintf(query, "select count(*) from engdic");
	ret = do_query2_nocommit(&isql, query);

#if 1
	GetSystemTime(&st);

	count = 0;
	for (i=0; i<10009; i++)
	{
		sprintf(s_word, "000000000%d", i);
		sprintf(s_meaning, "%*d", i % 1000, i);

		sprintf(query, "select * from engdic where word = '%s'", s_word);
		if (iSQL_query(&isql, query))
		{
			ret = iSQL_errno(&isql);

			__SYSLOG("Error: %s\n", query);
			__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));

			//iSQL_rollback(&isql);

			break;
		}

		//res = iSQL_use_result(&isql);
		res = iSQL_store_result(&isql);
		if (res == NULL)
		{
			ret = iSQL_errno(&isql);
			__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
			//iSQL_rollback(&isql);

			break;
		}

		//iSQL_commit(&isql);

		num = iSQL_num_rows(res);

		num_fields = iSQL_num_fields(res);
		for (j = 0; (row = iSQL_fetch_row(res)) != NULL; j++)
		{
			if (strcmp(s_word, row[0]) != 0)
			{
				__SYSLOG("word diff: %s %s\n", s_word, row[0]);
				break;
			}

			if (strcmp(s_meaning, row[1]) != 0)
			{
				__SYSLOG("word diff: %s %s\n", s_meaning, row[1]);
				break;
			}

			count++;
		}

		iSQL_free_result(res);
	}

	GetSystemTime(&et);

	gap = timediff(&st, &et);
	
	__SYSLOG("%d selected... %d\n", count, gap);
#endif

	iSQL_disconnect(&isql);
}

void test_RFID()
{
	iSQL		isql;
	int i, j;
	char query[1024];
	char dbname[] = "\\db\\RFID";
	int ret;
	int gap;
	int count;
	SYSTEMTIME st, et;
	char s_word[100];
	char s_meaning[2048];
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			num_fields;
	int num;
	//unsigned long int *lengths;

	if (iSQL_connect(&isql, NULL, dbname, USER, PASSWD) == NULL)
	{
		__SYSLOG("isql connect fail\n");
		return;
	}

	do_query2(&isql, "select fscl_name,accts_name, org_name, fscl_cd, accts_cd, "
                     "org_cd,  goffc_cd "
                     "from TBCZ120 "
                     "where userid = 'LSGOU'");

    do_query2(&isql, "desc TBCZ120");

    do_query2(&isql, "desc tbca040");

	do_query2(&isql, "desc tbca220");

//	do_query2(&isql, "select ledg_amt, real_qty from tbca220");

    do_query2(&isql, "select count(*) from tbca220");

    do_query2(&isql, "select  valbls_surv_guag_name, valbls_surv_guag, sum(real_qty), "
	                    "acqu_date, life ,count(indv_ident_cd) "
                     "from tbca220 "
                     "where goffc_cd = '03014287' "
                        " and oper_dept_cd ='0067' and tag_targ_yn ='N' "
                     "group by valbls_surv_guag_name, "
                        "valbls_surv_guag, acqu_date ,life");

    do_query2(&isql, "select  valbls_surv_guag_name, valbls_surv_guag, "
	                    "acqu_date, life, sum(real_qty) ,count(indv_ident_cd) "
                     "from tbca220 "
                     "where goffc_cd = '03014287' "
                        " and oper_dept_cd ='0067' and tag_targ_yn ='N' "
                     "group by valbls_surv_guag_name, "
                        "valbls_surv_guag, acqu_date ,life");

    do_query2(&isql, "select  valbls_surv_guag, valbls_surv_guag_name, sum(real_qty), "
	                    "acqu_date, life ,count(indv_ident_cd) "
                     "from tbca220 "
                     "where goffc_cd = '03014287' "
                        " and oper_dept_cd ='0067' and tag_targ_yn ='N' "
                     "group by valbls_surv_guag_name, "
                        "valbls_surv_guag, acqu_date ,life");

    do_query2(&isql, "select  count(indv_ident_cd), "
	                     "valbls_surv_guag_name, "
	                     "valbls_surv_guag, "
	                     "sum(real_qty), "
	                     "acqu_date, "
	                     "life "
                    "from tbca220 "
                    "where goffc_cd = '03014287' "
                    "and oper_dept_cd ='0088' "
                    "and tag_targ_yn ='Y' "
                    "group by valbls_surv_guag_name,valbls_surv_guag, acqu_date ,life ");

	//sprintf(query, "select real_qty from tbca220 where real_qty = 0;");
	//sprintf(query, "select real_qty from tbca220 limit 10;");

	//while (1)
	//	do_query2(&isql, query);

	iSQL_disconnect(&isql);
}

void test_not_avail_resource()
{
	iSQL		isql;
	int i, j;
	char query[2048];
	char dbname[] = "\\db\\nares";
	int ret;
	int gap;
	int count;
	SYSTEMTIME st, et;
	char s_word[100];
	char s_meaning[2048];
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			num_fields;
	int num;
	//unsigned long int *lengths;

#if 1
	Drop_DB(dbname);
	if (createdb(dbname) < 0)
	{
		__SYSLOG("test_createdb: first createdb fail %s\n", dbname);
	}
#endif

    //i=0;
    i = 9990;
    for (; i< 10000; i++)
    {
	    GetSystemTime(&st);

	    if (iSQL_connect(&isql, NULL, dbname, USER, PASSWD) == NULL)
	    {
		    __SYSLOG("isql connect fail\n");
		    return;
	    }

	    {
		    int on = 1;
		    iSQL_options(&isql, OPT_AUTOCOMMIT, &on, sizeof(on));
	    }

	    GetSystemTime(&et);

	    gap = timediff(&st, &et);
	    
	    __SYSLOG("[%d] connection time... %d\n", i, gap);

        iSQL_disconnect(&isql);
    }

	if (iSQL_connect(&isql, NULL, dbname, USER, PASSWD) == NULL)
	{
		__SYSLOG("isql connect fail\n");
		return;
	}

	sprintf(query, "select count(*) from engdic");
	ret = do_query2_nocommit(&isql, query);

#if 1
	ret = do_dml_query2_nocommit(&isql, 
			"drop table engdic");

	ret = do_dml_query2_nocommit(&isql, 
			"create table engdic (word char(100) primary key, meaning varchar(2048))");

	GetSystemTime(&st);

	for (i=0; i<10009; i++)
	{
		sprintf(query, "INSERT INTO engdic VALUES('000000000%d', '%*d')",
			i, i % 1000, i);
		//ret = do_dml_query2_nocommit(&isql, query);
        ret = do_dml_query2(&isql, query);
		if (ret != 0)
			break;

//		sprintf(query, "select count(*) from engdic");
//		ret = do_query2_nocommit(&isql, query);
	
//		if (i % 100 == 0)
//			__SYSLOG("%d insert done...\n", i);

        if (i > 981)
        {
       		//if (i % 10 == 0)
		    	__SYSLOG("%d insert done...\n", i);
        }
	}

	GetSystemTime(&et);

	gap = timediff(&st, &et);
	
	__SYSLOG("%d inserted... %d\n", i, gap);
#endif

	sprintf(query, "select count(*) from engdic");
	ret = do_query2_nocommit(&isql, query);

#if 0
	GetSystemTime(&st);

	count = 0;
	for (i=0; i<10009; i++)
	{
		sprintf(s_word, "000000000%d", i);
		sprintf(s_meaning, "%*d", i % 1000, i);

		sprintf(query, "select * from engdic where word = '%s'", s_word);
		if (iSQL_query(&isql, query))
		{
			ret = iSQL_errno(&isql);

			__SYSLOG("Error: %s\n", query);
			__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));

			//iSQL_rollback(&isql);

			break;
		}

		//res = iSQL_use_result(&isql);
		res = iSQL_store_result(&isql);
		if (res == NULL)
		{
			ret = iSQL_errno(&isql);
			__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
			//iSQL_rollback(&isql);

			break;
		}

		//iSQL_commit(&isql);

		num = iSQL_num_rows(res);

		num_fields = iSQL_num_fields(res);
		for (j = 0; (row = iSQL_fetch_row(res)) != NULL; j++)
		{
			if (strcmp(s_word, row[0]) != 0)
			{
				__SYSLOG("word diff: %s %s\n", s_word, row[0]);
				break;
			}

			if (strcmp(s_meaning, row[1]) != 0)
			{
				__SYSLOG("word diff: %s %s\n", s_meaning, row[1]);
				break;
			}

			count++;
		}

		iSQL_free_result(res);
	}

	GetSystemTime(&et);

	gap = timediff(&st, &et);
	
	__SYSLOG("%d selected... %d\n", count, gap);
#endif

	iSQL_disconnect(&isql);
}

int timediff(SYSTEMTIME *st, SYSTEMTIME *et)
{
	int gap;
	gap = et->wHour * 3600000 + et->wMinute * 60000 + et->wSecond * 1000 + et->wMilliseconds;
	gap -= (st->wHour * 3600000 + st->wMinute * 60000 + st->wSecond * 1000 + st->wMilliseconds);
	return gap;
}

void test_dic_insertselect()
{
	iSQL		isql;
	int i, j;
	char query[2048];
	char dbname[] = "\\db\\dic";
	int ret;
	int gap;
	int count;
	SYSTEMTIME st, et;
	char s_word[100];
	char s_meaning[2048];
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			num_fields;
	int num;
	//unsigned long int *lengths;

#if 1
	Drop_DB(dbname);
	if (createdb(dbname) < 0)
	{
		__SYSLOG("test_createdb: first createdb fail %s\n", dbname);
	}
#endif

	GetSystemTime(&st);

	if (iSQL_connect(&isql, NULL, dbname, USER, PASSWD) == NULL)
	{
		__SYSLOG("isql connect fail\n");
		return;
	}

	GetSystemTime(&et);

	gap = timediff(&st, &et);
	
	__SYSLOG("connection time... %d\n", gap);

	sprintf(query, "select count(*) from engdic");
	ret = do_query2(&isql, query);

#if 1
	ret = do_dml_query2(&isql, 
			"drop table engdic");

	ret = do_dml_query2(&isql, 
			"create table engdic (word char(100) primary key, meaning varchar(2048))");
#endif

#if 1
	GetSystemTime(&st);

	for (i=0; i<10009; i++)
//  	for (i=0; i<100; i++)
	{
		sprintf(query, "INSERT INTO engdic VALUES('000000000%d', '%*d')",
			i, i % 1000, i);
		ret = do_dml_query2(&isql, query);
		if (ret != 0)
			break;

		if (i % 1000 == 0)
			__SYSLOG("%d insert done...\n", i);
	}

	GetSystemTime(&et);

	gap = timediff(&st, &et);
	
	__SYSLOG("%d inserted... %d\n", i, gap);
#endif

	sprintf(query, "select count(*) from engdic");
	ret = do_query2(&isql, query);

#if 1
	GetSystemTime(&st);

	count = 0;
	for (i=0; i<10009; i++)
//	for (i=0; i<100; i++)
	{
		sprintf(s_word, "000000000%d", i);
		sprintf(s_meaning, "%*d", i % 1000, i);

		sprintf(query, "select * from engdic where word = '%s'", s_word);
		if (iSQL_query(&isql, query))
		{
			ret = iSQL_errno(&isql);

			__SYSLOG("Error: %s\n", query);
			__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));

			iSQL_rollback(&isql);

			break;
		}

		//res = iSQL_use_result(&isql);
		res = iSQL_store_result(&isql);
		if (res == NULL)
		{
			ret = iSQL_errno(&isql);
			__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
			iSQL_rollback(&isql);

			break;
		}

		iSQL_commit(&isql);

		num = iSQL_num_rows(res);

		num_fields = iSQL_num_fields(res);
		for (j = 0; (row = iSQL_fetch_row(res)) != NULL; j++)
		{
			if (strcmp(s_word, row[0]) != 0)
			{
				__SYSLOG("word diff: %s %s\n", s_word, row[0]);
				break;
			}

			if (strcmp(s_meaning, row[1]) != 0)
			{
				__SYSLOG("word diff: %s %s\n", s_meaning, row[1]);
				break;
			}

			count++;
		}

		iSQL_free_result(res);
	}

	GetSystemTime(&et);

	gap = timediff(&st, &et);
	
	__SYSLOG("%d selected... %d\n", count, gap);
#endif

	iSQL_disconnect(&isql);
}

void test_dic()
{
	iSQL		isql;
	int i;
	char query[2048];
	char dbname[] = "\\db\\dic";
	int ret;
	int gap;
	SYSTEMTIME st, et;
#if 1
	Drop_DB(dbname);
	if (createdb(dbname) < 0)
	{
		__SYSLOG("test_createdb: first createdb fail %s\n", dbname);
	}
#endif
	if (iSQL_connect(&isql, NULL, dbname, USER, PASSWD) == NULL)
	{
		__SYSLOG("isql connect fail\n");
		return;
	}

	ret = do_dml_query2(&isql, 
			"create table mxrecord (db_idx  int primary key,mx_type tinyint ,\
				mx_media tinyint default 0, path  varchar(100), name varchar(44))");

	do_dml_query2(&isql, 
		"insert into mxrecord(db_idx,path,name) values(1,'Media/Videos','xxx');");
	do_dml_query2(&isql, 
		"insert into mxrecord(db_idx,path,name) values(2,'Media/Video','xxx');");

	do_query2(&isql, "select * from mxrecord;");

	do_query2(&isql, "select * from mxrecord where path = 'Media/Videos';");

	do_query2(&isql, "select * from mxrecord where path = 'Media/Video';");

	sprintf(query, "select count(*) from engdic");
	ret = do_query2(&isql, query);

	ret = do_dml_query2(&isql, 
			"drop table engdic");

	ret = do_dml_query2(&isql, 
			"create table engdic (word char(100) primary key, meaning varchar(2048))");

	GetSystemTime(&st);

//	for (i=0; i<100; i++)
    for (i=0; i<10000; i++)
	{

		sprintf(query, "INSERT INTO engdic VALUES('%d', '%*d')",
			i, i % 1000, i);
		ret = do_dml_query2(&isql, query);
		if (ret != 0)
			break;

		if (i % 1000 == 0)
			__SYSLOG("%d insert done...\n", i);
	}

	GetSystemTime(&et);

	gap = et.wHour * 3600 + et.wMinute * 60 + et.wSecond;
	gap -= (st.wHour * 3600 + st.wMinute * 60 + st.wSecond);
	
	__SYSLOG("%d inserted... %d\n", i, gap);

	sprintf(query, "select count(*) from engdic");
	ret = do_query2(&isql, query);
	
	iSQL_disconnect(&isql);
}

#define MEMSIZE (128 * 1024)
void test_memoryalloc()
{
    char *ptr;
#if 0
	char *prev, *ptr0;
    int i;

    prev = NULL;
    for (i=0; i < 8*10; i++)
    {
again:
        ptr = (char *)malloc(MEMSIZE);
        if (ptr == NULL)
            break;
        memset(ptr, 0, MEMSIZE);

        if (i % 2 == 0)
        { 
            free(ptr);
            ptr = (char *)malloc(MEMSIZE);
            if (ptr == NULL)
                break;
            memset(ptr, 0, MEMSIZE);
        }

        if (prev)
            *(unsigned long *)ptr = (unsigned long)prev;

        prev = ptr;
    }
    __SYSLOG("malloc: %d\n", i);

    while (ptr)
    {
        prev = (char *)(*(unsigned long *)ptr);
        free(ptr);
        ptr = prev;
        i--;
    }
    __SYSLOG("free: %d\n", i);
#endif

#if 1
    HANDLE hFile, hMap;

    hFile = CreateFileForMapping( 
                L"\\temp\\a", 
                GENERIC_READ | GENERIC_WRITE, 
                FILE_SHARE_READ | FILE_SHARE_WRITE, 
                NULL, 
                CREATE_ALWAYS, 
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS , 
                NULL);
    if (hFile == NULL)
    {
        __SYSLOG("Error CreateFileForMapping %d\n", GetLastError());
        return;
    }

    hMap = CreateFileMapping(
                hFile, 
                NULL, 
                PAGE_READWRITE, 
                0, 
                32 * 1024 * 1024, /* 32 MB */
                NULL);
    if (hMap == NULL)
    {
        __SYSLOG("Error CreateFileMapping %d\n", GetLastError());
        return;
    }
    
    ptr = (char *)MapViewOfFile( 
                hMap, 
                FILE_MAP_ALL_ACCESS, 
                0, 
                0, 
                32 * 1024 * 1024);
    if (ptr == NULL)
        return;

    memset(ptr, 0, 32 * 1024 * 1024);

#if 0
    for (i=0; ; i++)
    {
        ptr = (char *)MapViewOfFile( 
                hMap, 
                FILE_MAP_ALL_ACCESS, 
                0, 
                i * MEMSIZE, 
                MEMSIZE);
        if (ptr == NULL)
            break;
        memset(ptr, 0, MEMSIZE);
        if (i == 0)
            ptr0 = ptr;
        __SYSLOG("memory mapping: %d\n", i);
    }
    __SYSLOG("memory mapping: %d\n", i);
#endif
#endif
}

void test_aerodb()
{
    do_query("\\db\\demo", "select * from systables;");
    do_query("\\db\\AeroDB", "select * from systables;");
    do_query("\\db\\AeroDB", "select * from sysindexes;");
    do_query("\\db\\AeroDB", "select count(*) from mos_cls");
    do_query("\\db\\AeroDB", "select count(*) from mos_wkmbr");
    do_query("\\db\\AeroDB", "select count(*) from mos_stdproc_sum");
    do_query("\\db\\AeroDB", "select count(*) from mos_kna1");
    do_query("\\db\\AeroDB", "select count(*) from mos_prdt_sub");
    do_query("\\db\\AeroDB", "select count(*) from mos_notice");
    do_query("\\db\\AeroDB", "select count(*) from mos_bank_acct");
    do_query("\\db\\AeroDB", "select count(*) from mos_wkmbr_feeh");
    do_query("\\db\\AeroDB", "select count(*) from mos_mbr_mng");
    do_query("\\db\\AeroDB", "select count(*) from mos_wkmbr_care");
    do_query("\\db\\AeroDB", "select count(*) from mos_stru");
    do_query("\\db\\AeroDB", "select count(*) from mos_stru_r");
    do_query("\\db\\AeroDB", "select count(*) from mos_stdproc");
    do_query("\\db\\AeroDB", "select count(*) from mos_wkmbr_prc");
    do_query("\\db\\AeroDB", "select count(*) from mos_rcpts");
    do_query("\\db\\AeroDB", "select count(*) from mos_rcpt_cms");
    do_query("\\db\\AeroDB", "select count(*) from mos_rcpt_stdb");
    do_query("\\db\\AeroDB", "select count(*) from mos_pro_std_dtl");
    do_query("\\db\\AeroDB", "select count(*) from mos_prdt_proc");
    do_query("\\db\\AeroDB", "select count(*) from mos_pro_std_mo");
    do_query("\\db\\AeroDB", "select count(*) from mos_prdt_set");
    do_query("\\db\\AeroDB", "select count(*) from mos_pro_stat");
}

void test_smallbuffercache()
{
	iSQL		isql;
	int i, j;
	char query[1024];
	char dbname[] = "\\db\\zipcode";
	int ret;
#if 1
	Drop_DB(dbname);
	if (createdb(dbname) < 0)
	{
		__SYSLOG("test_createdb: first createdb fail %s\n", dbname);
	}
#endif
	if (iSQL_connect(&isql, NULL, dbname, "aladdin", "aladdin") == NULL)
	{
		__SYSLOG("isql connect fail\n");
		return;
	}

#if 1
	ret = do_dml_query2(&isql, 
			"create table zipcode1 (zipcode char(7) NOT NULL, si varchar(10) NOT NULL, gu varchar(16) NOT NULL, dong varchar(30) NOT NULL, ri varchar(46), etc varchar(40), etc_start varchar(10), etc_end varchar(10), primary key(zipcode, dong, ri, etc, etc_start, etc_end))\0\0");
	ret = do_dml_query2(&isql, 
			"create index siIdx1 on ZipCode1(si);\0\0");
	ret = do_dml_query2(&isql, 
			"create index guIdx1 on ZipCode1(gu, si, dong);\0\0");
	ret = do_dml_query2(&isql, 
			"create index dongIdx1 on ZipCode1(dong);\0\0");
	ret = do_dml_query2(&isql, 
			"create index riIdx1 on ZipCode1(ri);\0\0");
	ret = do_dml_query2(&isql, 
			"create index etcIdx1 on ZipCode1(etc);\0\0");

	ret = do_dml_query2(&isql, 
			"create table zipcode2 (zipcode char(7) NOT NULL, si varchar(10) NOT NULL, gu varchar(16) NOT NULL, dong varchar(30) NOT NULL, ri varchar(46), etc varchar(40), etc_start varchar(10), etc_end varchar(10), primary key(zipcode, dong, ri, etc, etc_start, etc_end))\0\0");
	ret = do_dml_query2(&isql, 
			"create index siIdx2 on ZipCode2(si);\0\0");
	ret = do_dml_query2(&isql, 
			"create index guIdx2 on ZipCode2(gu, si, dong);\0\0");
	ret = do_dml_query2(&isql, 
			"create index dongIdx2 on ZipCode2(dong);\0\0");
	ret = do_dml_query2(&isql, 
			"create index riIdx2 on ZipCode2(ri);\0\0");
	ret = do_dml_query2(&isql, 
			"create index etcIdx2 on ZipCode2(etc);\0\0");

	ret = do_dml_query2(&isql, 
			"create table zipcode3 (zipcode char(7) NOT NULL, si varchar(10) NOT NULL, gu varchar(16) NOT NULL, dong varchar(30) NOT NULL, ri varchar(46), etc varchar(40), etc_start varchar(10), etc_end varchar(10), primary key(zipcode, dong, ri, etc, etc_start, etc_end))\0\0");
	ret = do_dml_query2(&isql, 
			"create index siIdx3 on ZipCode3(si);\0\0");
	ret = do_dml_query2(&isql, 
			"create index guIdx3 on ZipCode3(gu, si, dong);\0\0");
	ret = do_dml_query2(&isql, 
			"create index dongIdx3 on ZipCode3(dong);\0\0");
	ret = do_dml_query2(&isql, 
			"create index riIdx3 on ZipCode3(ri);\0\0");
	ret = do_dml_query2(&isql, 
			"create index etcIdx3 on ZipCode3(etc);\0\0");

	ret = do_dml_query2(&isql, 
			"create table zipcode4 (zipcode char(7) NOT NULL, si varchar(10) NOT NULL, gu varchar(16) NOT NULL, dong varchar(30) NOT NULL, ri varchar(46), etc varchar(40), etc_start varchar(10), etc_end varchar(10), primary key(zipcode, dong, ri, etc, etc_start, etc_end))\0\0");
	ret = do_dml_query2(&isql, 
			"create index siIdx4 on ZipCode4(si);\0\0");
	ret = do_dml_query2(&isql, 
			"create index guIdx4 on ZipCode4(gu, si, dong);\0\0");
	ret = do_dml_query2(&isql, 
			"create index dongIdx4 on ZipCode4(dong);\0\0");
	ret = do_dml_query2(&isql, 
			"create index riIdx4 on ZipCode4(ri);\0\0");
	ret = do_dml_query2(&isql, 
			"create index etcIdx4 on ZipCode4(etc);\0\0");

	ret = do_dml_query2(&isql, 
			"create table zipcode5 (zipcode char(7) NOT NULL, si varchar(10) NOT NULL, gu varchar(16) NOT NULL, dong varchar(30) NOT NULL, ri varchar(46), etc varchar(40), etc_start varchar(10), etc_end varchar(10), primary key(zipcode, dong, ri, etc, etc_start, etc_end))\0\0");
	ret = do_dml_query2(&isql, 
			"create index siIdx5 on ZipCode5(si);\0\0");
	ret = do_dml_query2(&isql, 
			"create index guIdx5 on ZipCode5(gu, si, dong);\0\0");
	ret = do_dml_query2(&isql, 
			"create index dongIdx5 on ZipCode5(dong);\0\0");
	ret = do_dml_query2(&isql, 
			"create index riIdx5 on ZipCode5(ri);\0\0");
	ret = do_dml_query2(&isql, 
			"create index etcIdx5 on ZipCode5(etc);\0\0");


	for (j=1; j<=5; j++)
	{
		for (i=0; i<10000; i++)
		{

			sprintf(query, "INSERT INTO ZipCode%d (zipcode, si, gu, dong, ri, etc, etc_start, etc_end) \
				VALUES ('219-%.3d', '강원', '고성군', '토성면', '용촌리', '%.10d', '', '')",
				j, i % 100, i+1);
			ret = do_dml_query2(&isql, query);
			if (ret != 0)
				break;

			if (ret != 0)
				break;

			if (i % 1000 == 0)
				__SYSLOG("%d %d insert done...\n", j, i);
		}
	}

	__SYSLOG("%d inserted...\n", i);
#endif

	for (j=1; j<=5; j++)
	{
		sprintf(query, "select count(*) from ZipCode%d", j);
		ret = do_query2(&isql, query);
	}

	for (j=1; j<=5; j++)
	{
		sprintf(query, "delete from ZipCode%d", j);
		ret = do_dml_query2(&isql, query);
	}

	for (j=1; j<=5; j++)
	{
		sprintf(query, "select count(*) from ZipCode%d", j);
		ret = do_query2(&isql, query);
	}

	iSQL_disconnect(&isql);
}

void test_zipcode(int loop)
{
	iSQL		isql;
	int i;
	char query[1024];
	char dbname[] = "\\db\\zipcode";
	int ret;
    int num = 100000;

    if (loop == 0)
    {
	    Drop_DB(dbname);
	    if (createdb(dbname) < 0)
	    {
		    __SYSLOG("test_createdb: first createdb fail %s\n", dbname);
	    }
    }

	if (iSQL_connect(&isql, NULL, dbname, "aladdin", "aladdin") == NULL)
	{
		__SYSLOG("isql connect fail\n");
		return;
	}

	ret = do_dml_query2(&isql, 
			"create table zipcode (zipcode char(7) NOT NULL, si varchar(10) NOT NULL, "
            "gu varchar(16) NOT NULL, dong varchar(30) NOT NULL, ri varchar(46), "
            "etc varchar(40), etc_start varchar(10), etc_end varchar(10), "
            "primary key(zipcode, dong, ri, etc, etc_start, etc_end))\0\0");
 
	ret = do_dml_query2(&isql, 
			"create table zipcode (zipcode char(7) NOT NULL, si varchar(10) NOT NULL, "
            "gu varchar(16) NOT NULL, dong varchar(30) NOT NULL, ri varchar(46), "
            "etc varchar(40), etc_start varchar(10), etc_end varchar(10), "
            "primary key(zipcode, dong, ri, etc, etc_start, etc_end))\0\0");
	
	ret = do_dml_query2(&isql, 
			"create index siIdx on ZipCode(si);\0\0");

	ret = do_dml_query2(&isql, 
			"create index guIdx on ZipCode(gu, si, dong);\0\0");

	ret = do_dml_query2(&isql, 
			"create index dongIdx on ZipCode(dong);\0\0");

	ret = do_dml_query2(&isql, 
			"create table zipcode (zipcode char(7) NOT NULL, si varchar(10) NOT NULL, "
            "gu varchar(16) NOT NULL, dong varchar(30) NOT NULL, ri varchar(46), "
            "etc varchar(40), etc_start varchar(10), etc_end varchar(10), "
            "primary key(zipcode, dong, ri, etc, etc_start, etc_end))\0\0");

#if 0
	ret = do_query2(&isql, 
			"select zipcode, si, gu, dong, ri, etc, etc_start, etc_end from zipcode;");
#endif

	ret = do_query2(&isql, 
			"select count(*) from zipcode where zipcode = '000-123';");

	ret = do_query2(&isql, 
			"select count(tableid) from systables where tablename = 'pre_zipcode';\0\0");

	ret = do_query2(&isql, 
			"select count(*) from systables where tablename = 'pre_zipcode';\0\0");

	ret = do_dml_query2(&isql, 
			"create table pre_zipcode (si varchar(20), gu varchar(30), "
            "primary key(si, gu));\0\0");

	ret = do_dml_query2(&isql, 
			"create table engdic (word varchar(64) NOT NULL, "
            "pronunciation varchar(64) , content varchar(1024))");
	
	ret = do_dml_query2(&isql, 
			"create index wordIdx on engdic(word)");
	
	ret = do_query2(&isql, 
			"select * from engdic where word = 'a-frame'");

	for (i=0; i<num; i++)
	{
		sprintf(query, "INSERT INTO ZipCode (zipcode, si, gu, dong, ri, etc, "
            "etc_start, etc_end) "
			"VALUES ('219-%.3d', '강원', '고성군', '토성면', '용촌리', '%.10d', '', '')",
			i % 100, i+1);
		ret = do_dml_query2(&isql, query);
		if (ret != 0)
			break;

		if (i % 1000 == 0)
			__SYSLOG("%d insert done...\n", i);
	}

	__SYSLOG("%d inserted...\n", i);

	ret = do_query2(&isql, 
			"select count(*) from ZipCode");

    for (i=0; i<num; i++)
    {
        sprintf(query, "UPDATE ZipCode SET si = '경기' "
                        "where zipcode = '219-%.3d' and "
                        "si = '강원' and gu = '고성군' and dong = '토성면' and "
                        "ri = '용촌리' and etc = '%.10d'",
            i % 100, i+1);
        ret = do_dml_query2(&isql, query);
		if (ret != 0)
			break;

        if (i % 1000 == 0)
			__SYSLOG("%d update done...\n", i);
    }

   	__SYSLOG("%d updated...\n", i);

    ret = do_query2(&isql, 
		"select count(*) from ZipCode where si = '경기';");

    for (i=0; i<num; i++)
    {
        sprintf(query, "DELETE FROM ZipCode where zipcode = '219-%.3d'",
            i % 100);
        sprintf(query, "DELETE FROM ZipCode where zipcode = '219-%.3d' and "
                "si = '경기' and gu = '고성군' and dong = '토성면' and "
                "ri = '용촌리' and etc = '%.10d'",
            i % 100, i+1);
        ret = do_dml_query2(&isql, query);
		if (ret != 0)
			break;

        if (i % 1000 == 0)
			__SYSLOG("%d delete done...\n", i);
    }

    __SYSLOG("%d deleted...\n", i);

	ret = do_query2(&isql, 
			"select count(*) from ZipCode");

	iSQL_disconnect(&isql);
}

void test_contact()
{
	iSQL		isql;
	char query[1024];
	char dbname[] = "\\db\\contact";
	int ret;
	int i;

	Drop_DB(dbname);
	//if (createdb(dbname) < 0)
	if (createdb2(dbname, 8) < 0)
	{
		__SYSLOG("test_createdb: first createdb fail %s\n", dbname);
	}

	if (iSQL_connect(&isql, NULL, dbname, "aladdin", "aladdin") == NULL)
	{
		__SYSLOG("isql connect fail\n");
		return;
	}

#if 1
	ret = do_dml_query2(&isql, 
			"create table contact (\
          id smallint primary key,\
   first_name varchar(60),\
   last_name varchar(60),\
   ph_num0 varchar(40), \
   ph_type0 tinyint default 2, \
   ph_vcheck0 tinyint default 0, \
   ph_num1 varchar(40), \
   ph_type1 tinyint default 2, \
   ph_vcheck1 tinyint default 0, \
   ph_num2 varchar(40), \
   ph_type2 tinyint default 2, \
   ph_vcheck2 tinyint default 0, \
   ph_num3 varchar(40), \
   ph_type3 tinyint default 2, \
   ph_vcheck3 tinyint default 0, \
   ph_num4 varchar(40), \
   ph_type4 tinyint default 2, \
   ph_vcheck4 tinyint default 0, \
   ph_defaultindex tinyint default 0, \
   email varchar(300),\
   homepage varchar(300),\
   memo varchar(300),\
   ringtone varchar(80) default NULL,\
   msgtone varchar(80) default NULL,\
   image varchar(80) default NULL,\
   devtype tinyint default 1)");
#else
	ret = do_dml_query2(&isql, 
			"create table contact (\
          id smallint primary key,\
   first_name varchar(60),\
   last_name varchar(60),\
   ph_num0 varchar(40), \
   ph_type0 tinyint, \
   ph_vcheck0 tinyint, \
   ph_num1 varchar(40), \
   ph_type1 tinyint, \
   ph_vcheck1 tinyint, \
   ph_num2 varchar(40), \
   ph_type2 tinyint, \
   ph_vcheck2 tinyint, \
   ph_num3 varchar(40), \
   ph_type3 tinyint, \
   ph_vcheck3 tinyint, \
   ph_num4 varchar(40), \
   ph_type4 tinyint, \
   ph_vcheck4 tinyint, \
   ph_defaultindex tinyint, \
   email varchar(300),\
   homepage varchar(300),\
   memo varchar(300),\
   ringtone varchar(80),\
   msgtone varchar(80),\
   image varchar(80),\
   devtype tinyint)");
#endif

	ret = do_query2(&isql, 
			"select count(*) from contact");

	for (i=0; i<2000; i++)
	{
		sprintf(query, "insert into contact(id, first_name, last_name, ph_num0) \
							values(%d, '%d', '%d', '%d')", i, i, i, i);
		ret = do_dml_query2(&isql, query);
		if (ret < 0)
			break;
	}

	ret = do_query2(&isql, 
			"select * from contact");

	ret = do_query2(&isql, 
			"select count(*) from contact");

	iSQL_disconnect(&isql);
}

void test_decode()
{
	iSQL		isql;
	char dbname[] = "\\db\\testdb0";
	int ret;

	Drop_DB(dbname);
	if (createdb(dbname) < 0)
	{
		__SYSLOG("test_createdb: first createdb fail testdb0\n");
	}

	iSQL_connect(&isql, NULL, dbname, "aladdin", "aladdin");

	ret = do_dml_query2(&isql, 
			"create table de(a int, b char(10))");

	ret = do_dml_query2(&isql, "insert into de values(1, 'aa')");
	ret = do_dml_query2(&isql, "insert into de values(1, 'bb')");
	ret = do_dml_query2(&isql, "insert into de values(1, 'cc')");
	ret = do_dml_query2(&isql, "insert into de values(1, 'dd')");
 
	ret = do_query2(&isql, 
			"select decode(b, 'aa','kk','bb','ll','cc','mm','ZZ') from de");
	ret = do_query2(&isql, 
			"select decode(b, 'aa','kk','bb','ll','cc','mm','ZZ'), \
			        decode(b, 'aa','kk','bb','ll','cc','mm','ZZ') from de");

	iSQL_disconnect(&isql);
}


void test_insert_fail()
{
	iSQL		isql;
	char query[1024];
	char dbname[] = "\\db\\testdb0";
	int ret;

	Drop_DB(dbname);
	if (createdb(dbname) < 0)
	{
		__SYSLOG("test_createdb: first createdb fail testdb0\n");
	}

	iSQL_connect(&isql, NULL, dbname, "aladdin", "aladdin");

	m_pData = &isql;

	sprintf(query, "%s", 
		"create table tb_test (id char(31) primary key, value varchar(20))");

	if (iSQL_query(&isql, query))
	{
		__SYSLOG("ERROR[%d]: %s\n", iSQL_errno(&isql), query);

		iSQL_rollback(&isql);

		__SYSLOG("table \'tb_test\' 만들기 실패...");

		return;
	}
	else
	{
		iSQL_commit(&isql);
	}

/*	ret = do_query2(&isql, "select * from testin where id = '안영회의'");
	ret = do_dml_query2(&isql, "insert into testin values('안영회의', '1')");

	ret = do_query2(&isql, "select * from testin where id = 'UML'");
	ret = do_dml_query2(&isql, "insert into testin values('UML', '2')");

	ret = do_query2(&isql, "select * from testin where id = '강좌'");
	ret = do_dml_query2(&isql, "insert into testin values('강좌', '3')");

	ret = do_query2(&isql, "select * from testin where id = '[안영회의'");
	ret = do_dml_query2(&isql, "insert into testin values('[안영회의', '4')");

	ret = do_query2(&isql, "select * from testin where id = 'UML'");
	ret = do_dml_query2(&isql, "delete from testin where id = 'UML'");

	ret = do_query2(&isql, "select * from testin where id = '강좌1]'");
	ret = do_dml_query2(&isql, "insert into testin values('강좌1]', '6')");

	ret = do_query2(&isql, "select * from testin where id = '-'");
	ret = do_dml_query2(&isql, "insert into testin values('-', '7')");

	ret = do_query2(&isql, "select * from testin where id = '모델링과'");
	ret = do_dml_query2(&isql, "insert into testin values('모델링과', '8')");

	ret = do_query2(&isql, "select * from testin where id = '모델링'");
	ret = do_dml_query2(&isql, "insert into testin values('모델링', '9')");

	ret = do_query2(&isql, "select * from testin where id = '언어(1)'");
	ret = do_dml_query2(&isql, "insert into testin values('언어(1)', '10')");

	ret = do_query2(&isql, "select * from testin where id = '1'");
	ret = do_dml_query2(&isql, "insert into testin values('1', '11')");

	ret = do_query2(&isql, "select * from testin where id = '모델링(Modeling)이란?'");
	ret = do_dml_query2(&isql, "insert into testin values('모델링(Modeling)이란?', '12')");

	ret = do_query2(&isql, "select * from testin where id = '1'");
	ret = do_dml_query2(&isql, "update testin set value = '13' where id = '1'");

	ret = do_query2(&isql, "select * from testin where id = '모델링의'");
	ret = do_dml_query2(&isql, "insert into testin values('모델링의', '14')");

	ret = do_query2(&isql, "select * from testin where id = '특성과'");
	ret = do_dml_query2(&isql, "insert into testin values('특성과', '15')");

	ret = do_query2(&isql, "select * from testin where id = '다양한'");
	ret = do_dml_query2(&isql, "insert into testin values('다양한', '16')");

	ret = do_query2(&isql, "select * from testin where id = '계층의'");
	ret = do_dml_query2(&isql, "insert into testin values('계층의', '17')");

	ret = do_query2(&isql, "select * from testin where id = '언어들'");
	ret = do_dml_query2(&isql, "insert into testin values('언어들', '18')");

	ret = do_query2(&isql, "select * from testin where id = '2'");
	ret = do_dml_query2(&isql, "insert into testin values('2', '19')");

	ret = do_query2(&isql, "select * from testin where id = '[안영회의'");
	ret = do_query2(&isql, "select * from testin where id = '[안영회의'");

	ret = do_query2(&isql, "select * from testin where id = 'UML'");
	ret = do_dml_query2(&isql, "insert into testin values('UML', '21')");
	ret = do_dml_query2(&isql, "delete from testin where id = 'UML'");
	ret = do_dml_query2(&isql, "insert into testin values('UML', '21')");*/

	ret = insertRec(L"안영회의", L"1");
	ret = insertRec(L"UML", L"2");
	ret = insertRec(L"강좌", L"3");
	ret = insertRec(L"[안영회의", L"4");
	ret = deleteRec(L"UML");
	ret = insertRec(L"UML", L"6");
	ret = deleteRec(L"UML");
	ret = insertRec(L"UML", L"8");

	do_query2(&isql, "select * from tb_test");

	iSQL_disconnect(&isql);
}

void test_distinct()
{
	iSQL		isql;
	iSQL_RES	*res;
	iSQL_ROW	row;
	int i;
	char query[1024];
	char dbname[] = "\\db\\mData_BASS";

	iSQL_connect(&isql, NULL, dbname, "aladdin", "aladdin");
/*
	sprintf(query, "%s", 
		"CREATE TABLE DBST_DONGHO ( \
DS_DONG VARCHAR(5), \
DS_CUNG VARCHAR(3), \
DS_HO VARCHAR(4), \
PRIMARY KEY(DS_DONG,DS_CUNG,DS_HO))");

	if (iSQL_query(&isql, query))
	{
		__SYSLOG("ERROR: %s\n", query);

		iSQL_rollback(&isql);

		__SYSLOG("table \'teladdr\' 만들기 실패...");

		return;
	}
*/
	sprintf(query, "SELECT DISTINCT DS_DONG FROM DBST_DONGHO ORDER BY DS_DONG");

	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}

	res = iSQL_use_result(&isql);
	if (res == NULL)
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
	else
	{
		int num, num_fields;
		iSQL_LENGTH lengths;

		num = iSQL_num_rows(res);

		num_fields = iSQL_num_fields(res);
		for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
		{
			lengths = iSQL_fetch_lengths(res);

			__SYSLOG("<%d> ", i);

			for (int j=0; j<num_fields; j++)
				__SYSLOG("%s,", row[j]);
			__SYSLOG("\n");
		}

		__SYSLOG("count = %d\n", i);

		iSQL_free_result(res);
	}

	iSQL_commit(&isql);

	res = iSQL_list_fields(&isql, "DBST_DONGHO", NULL);
	if (res == NULL)
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
	else
	{
		int num, num_fields;
		iSQL_FIELD* field;

		num = iSQL_num_rows(res);

		num_fields = iSQL_num_fields(res);

		field = iSQL_fetch_fields(res);

		for (i = 0; i < num_fields != NULL; i++)
		{
			 __SYSLOG("<%d> %s\n", i, field[i].name);
		}
		__SYSLOG("\n");

		__SYSLOG("count = %d\n", i);

		iSQL_free_result(res);
	}

	iSQL_disconnect(&isql);
}

void test_count_table()
{
	iSQL		isql;
	int i;
	char query[1024];
	char dbname[] = "\\db\\testdb0";

	Drop_DB(dbname);
	if (createdb(dbname) < 0)
	{
		__SYSLOG("test_createdb: first createdb fail testdb0\n");
	}

	iSQL_connect(&isql, NULL, dbname, "aladdin", "aladdin");

	sprintf(query, "%s", 
		"create table teladdr (number int primary key, name char(20), tel char(20), \
		addr varchar(50), memo char(50))");

	if (iSQL_query(&isql, query))
	{
		__SYSLOG("ERROR: %s\n", query);

		iSQL_rollback(&isql);

		__SYSLOG("table \'teladdr\' 만들기 실패...");

		return;
	}
	else
	{
		iSQL_commit(&isql);
	}
	iSQL_commit(&isql);

	iSQL_disconnect(&isql);

	do_query(dbname, 
		"select count(*) from teladdr");

	iSQL_connect(&isql, NULL, dbname, "aladdin", "aladdin");

	for (i=0; i < 2000; i++)
	{
		sprintf(query, "insert into teladdr values('%d', 'name%d', 'tel%d', 'addr%d', \
			'memo%d')",
			i, i, i, i, i);

		if (iSQL_query(&isql, query))
		{
			__SYSLOG("ERROR: %s\n", iSQL_error(&isql));
			__SYSLOG("query: %s\n", query);
			iSQL_rollback(&isql);
			return;
		}
	}

	iSQL_commit(&isql);

	iSQL_disconnect(&isql);

	do_query(dbname, 
		"select count(*) from teladdr");

	do_dml_query(dbname, 
		"delete from teladdr where number < 1000");

	do_query(dbname, 
		"select count(*) from teladdr");

	do_query(dbname, 
		"select count(number) from teladdr");
}

void check_health()
{
	char dbname[] = "\\db\\health_src";

	do_query(dbname, "desc hlwtnman07");

	do_query("\\db\\health_src", 
		"select count(*) from hlwtnman07 where supptr_sid is not null");

	do_query("\\db\\health_src", 
		"select * from hlwtnman07 where supptr_sid is not null");

	do_query("\\db\\health_src", 
		"select count(*) from hlwtnman07 where supptr_sid is null");

	do_query("\\db\\health_src", 
		"select count(*) from hlwtnman07 where supptr_sid_sno is not null");

	do_query("\\db\\health_src", 
		"select count(*) from hlwtnman07 where bsin_yy is not null");

	do_query("\\db\\health_src", 
		"select count(*) from hlwtnman07 where chg_chasu is not null");

	do_query("\\db\\health_src", 
		"select count(*) from hlwtnman07 where cnsl_notm is not null");
	do_query(dbname, "select count(*) from hlwtnman07");

	do_query(dbname, "select * from hlwtnman07");
}

void test_convert2()
{
	iSQL		isql;
	SYSTEMTIME st, et;
	int gap;
	char query[2048];
	char dbname[] = "\\db\\SMILKSALE";

	do_query(dbname,"select count(*) from smilkdb_ch");

	do_query(dbname,"select count( * ) from smilkdb_ch where agent_code is not null");

	do_dml_query(dbname, "drop table chTmp");

	do_dml_query(dbname,
		"create table chTmp ( \
			agent_code		char(5),\
			client_code		char(5),\
			ch_date          varchar(9),\
			ch_gd_code       varchar(11), \
			ch_out_many			int, \
			ch_in_many			int, \
			ch_cus_many			int, \
			ch_agtot_many		int, \
			ch_diff_many			int, \
			ch_ex_many			int, \
			ch_self_many			int, \
			ch_clack_many		int, \
			ch_service_many		int, \
			ch_gd_name		varchar(37), \
			pdaflag			char(1))");

	do_dml_query(dbname, 
		"insert into chTmp(agent_code, client_code, ch_date, \
				ch_gd_code, ch_out_many, ch_in_many, \
				ch_cus_many, ch_agtot_many, ch_diff_many, \
				ch_ex_many, ch_self_many, ch_clack_many, \
				ch_service_many, ch_gd_name, pdaflag) \
			select	agent_code, client_code, ch_date,\
				ch_gd_code, ch_out_many, ch_in_many,\
				ch_cus_many, ch_agtot_many, ch_diff_many,\
				ch_ex_many, ch_self_many, ch_clack_many,\
				ch_service_many, ch_gd_name, pdaflag \
			from smilkdb_ch");

	do_query(dbname, "select count(*) from chtmp where ch_date='20040322'");

	do_query(dbname, "select * from smilkdb_ch where ch_date='20040322'");

	do_query(dbname, "select * from chtmp where ch_date='20040322'");

//	do_query(dbname, "select cs_gd_name from smilkdb_cs");

//	do_query(dbname, "select min(cs_gd_name) from smilkdb_cs");
/*
	do_dml_query(dbname, "create table vv (a int, b char(10))");
	do_dml_query(dbname, "insert into vv values(1, '9876543210')");
	do_dml_query(dbname, "insert into vv values(2, '76543210')");
	do_dml_query(dbname, "insert into vv values(3, '543210')");*/
	do_query(dbname, "select a, min(b) from vv group by a");
	do_query(dbname, "select a, max(b) from vv group by a");

do_query(dbname, "select cs_gd_code, min(cs_gd_name) \
from		smilkdb_cs \
group by cs_gd_code \
order by cs_gd_code");

	do_query(dbname, "select cs_gd_code, min(cs_gd_name) \
from		smilkdb_cs \
where	cs_fromdate <= '20040317' \
and		cs_todate	>= '20040317' \
and		client_code = '8002' \
and		agent_code	= '9999' \
group by cs_gd_code \
order by cs_gd_code");

	do_query(dbname, "desc smilkdb_gd");

	__SYSLOG("--------------------------------------------------------------------\n");

	GetSystemTime(&st);

	if (iSQL_connect(&isql, NULL, dbname, "aladdin", "aladdin") == NULL)
	{
		__SYSLOG("iSQL_connect(): %s\n", iSQL_error(&isql));
		return;
	}

	sprintf(query, "create table gdTmp (gd_code varchar(11), \
                    gd_name varchar(37), \
                    gd_cnvmany float, \
                    gd_taxkind varchar(2), \
                    gd_fullbarcode varchar(27), \
                    gd_size varchar(41), \
                    gd_order_gubn char(3), \
                    gd_cntmany int, \
                    gd_cost float, \
                    primary key(gd_code))");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}
	iSQL_commit(&isql);

	sprintf(query, "insert into gdTmp(gd_code, gd_name, gd_cnvmany, \
                gd_taxkind, gd_fullbarcode, gd_size, \
                gd_order_gubn, gd_cntmany, gd_cost) \
        select  gd_code, gd_name, gd_cnvmany, \
                gd_taxkind, gd_fullbarcode, gd_size, \
                gd_order_gubn, gd_cntmany, gd_cost \
        from smilkdb_gd");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}
	iSQL_commit(&isql);

	sprintf(query, "drop table smilkdb_gd");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}
	iSQL_commit(&isql);

	sprintf(query, "create table SMILKDB_GD (gd_code varchar(11), \
                         gd_name varchar(37), \
                         gd_cnvmany float, \
                         gd_taxkind varchar(2), \
                         gd_fullbarcode varchar(27), \
                         gd_size varchar(41), \
                         gd_order_gubn char(3), \
                         gd_cntmany int, \
                         gd_cost float, primary key(gd_code))");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}
	iSQL_commit(&isql);

	sprintf(query, "insert into smilkdb_gd(gd_code, gd_name, gd_cnvmany, \
                       gd_taxkind, gd_fullbarcode, gd_size, \
                       gd_order_gubn, gd_cntmany, gd_cost) \
            select gd_code, gd_name, gd_cnvmany, \
                   gd_taxkind, gd_fullbarcode, gd_size, \
                   gd_order_gubn, gd_cntmany, gd_cost \
            from gdTmp");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}
	iSQL_commit(&isql);

	sprintf(query, "drop table gdTmp");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}
	iSQL_commit(&isql);

	iSQL_disconnect(&isql);

	GetSystemTime(&et);

	gap = (et.wMinute - st.wMinute);
	gap *= 60;
	gap += (et.wSecond - st.wSecond);

	__SYSLOG(">>>>> %d\n", gap);
	__SYSLOG("--------------------------------------------------------------------\n");
}

void test_convert()
{
	iSQL		isql;
	SYSTEMTIME st, et;
	int gap;
	char query[2048];
	char dbname[] = "\\db\\SMILKSALE";

	do_query(dbname,"select count(*) from smilkdb_ch");

	do_query(dbname,"select count( * ) from smilkdb_ch where agent_code is not null");

	do_dml_query(dbname, "drop table chTmp");

	do_dml_query(dbname,
		"create table chTmp ( \
			agent_code		char(5),\
			client_code		char(5),\
			ch_date          varchar(9),\
			ch_gd_code       varchar(11), \
			ch_out_many			int, \
			ch_in_many			int, \
			ch_cus_many			int, \
			ch_agtot_many		int, \
			ch_diff_many			int, \
			ch_ex_many			int, \
			ch_self_many			int, \
			ch_clack_many		int, \
			ch_service_many		int, \
			ch_gd_name		varchar(37), \
			pdaflag			char(1))");

	do_dml_query(dbname, 
		"insert into chTmp(agent_code, client_code, ch_date, \
				ch_gd_code, ch_out_many, ch_in_many, \
				ch_cus_many, ch_agtot_many, ch_diff_many, \
				ch_ex_many, ch_self_many, ch_clack_many, \
				ch_service_many, ch_gd_name, pdaflag) \
			select	agent_code, client_code, ch_date,\
				ch_gd_code, ch_out_many, ch_in_many,\
				ch_cus_many, ch_agtot_many, ch_diff_many,\
				ch_ex_many, ch_self_many, ch_clack_many,\
				ch_service_many, ch_gd_name, pdaflag \
			from smilkdb_ch");

	do_query(dbname, "select * from smilkdb_ch where ch_date='20040322'");

	do_query(dbname, "select * from chtmp where ch_date='20040322'");

//	do_query(dbname, "select cs_gd_name from smilkdb_cs");

//	do_query(dbname, "select min(cs_gd_name) from smilkdb_cs");
/*
	do_dml_query(dbname, "create table vv (a int, b char(10))");
	do_dml_query(dbname, "insert into vv values(1, '9876543210')");
	do_dml_query(dbname, "insert into vv values(2, '76543210')");
	do_dml_query(dbname, "insert into vv values(3, '543210')");*/
	do_query(dbname, "select a, min(b) from vv group by a");
	do_query(dbname, "select a, max(b) from vv group by a");

do_query(dbname, "select cs_gd_code, min(cs_gd_name) \
from		smilkdb_cs \
group by cs_gd_code \
order by cs_gd_code");

	do_query(dbname, "select cs_gd_code, min(cs_gd_name) \
from		smilkdb_cs \
where	cs_fromdate <= '20040317' \
and		cs_todate	>= '20040317' \
and		client_code = '8002' \
and		agent_code	= '9999' \
group by cs_gd_code \
order by cs_gd_code");

	do_query(dbname, "desc smilkdb_gd");

	__SYSLOG("--------------------------------------------------------------------\n");

	GetSystemTime(&st);

	if (iSQL_connect(&isql, NULL, dbname, "aladdin", "aladdin") == NULL)
	{
		__SYSLOG("iSQL_connect(): %s\n", iSQL_error(&isql));
		return;
	}

	sprintf(query, "create table gdTmp (gd_code varchar(11), \
                    gd_name varchar(37), \
                    gd_cnvmany float, \
                    gd_taxkind varchar(2), \
                    gd_fullbarcode varchar(27), \
                    gd_size varchar(41), \
                    gd_order_gubn char(3), \
                    gd_cntmany int, \
                    gd_cost float, \
                    primary key(gd_code))");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}
	iSQL_commit(&isql);

	sprintf(query, "insert into gdTmp(gd_code, gd_name, gd_cnvmany, \
                gd_taxkind, gd_fullbarcode, gd_size, \
                gd_order_gubn, gd_cntmany, gd_cost) \
        select  gd_code, gd_name, gd_cnvmany, \
                gd_taxkind, gd_fullbarcode, gd_size, \
                gd_order_gubn, gd_cntmany, gd_cost \
        from smilkdb_gd");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}
	iSQL_commit(&isql);

	sprintf(query, "drop table smilkdb_gd");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}
	iSQL_commit(&isql);

	sprintf(query, "create table SMILKDB_GD (gd_code varchar(11), \
                         gd_name varchar(37), \
                         gd_cnvmany float, \
                         gd_taxkind varchar(2), \
                         gd_fullbarcode varchar(27), \
                         gd_size varchar(41), \
                         gd_order_gubn char(3), \
                         gd_cntmany int, \
                         gd_cost float, primary key(gd_code))");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}
	iSQL_commit(&isql);

	sprintf(query, "insert into smilkdb_gd(gd_code, gd_name, gd_cnvmany, \
                       gd_taxkind, gd_fullbarcode, gd_size, \
                       gd_order_gubn, gd_cntmany, gd_cost) \
            select gd_code, gd_name, gd_cnvmany, \
                   gd_taxkind, gd_fullbarcode, gd_size, \
                   gd_order_gubn, gd_cntmany, gd_cost \
            from gdTmp");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}
	iSQL_commit(&isql);

	sprintf(query, "drop table gdTmp");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}
	iSQL_commit(&isql);

	iSQL_disconnect(&isql);

	GetSystemTime(&et);

	gap = (et.wMinute - st.wMinute);
	gap *= 60;
	gap += (et.wSecond - st.wSecond);

	__SYSLOG(">>>>> %d\n", gap);
	__SYSLOG("--------------------------------------------------------------------\n");
}

/*
void test_groupby()
{
	do_query("\\db\\SMILKSALE",
		"select	ot_gd_code, sum(ot_many)\
			from	smilkdb_ot\
			where	agent_code = '94642' and\
					client_code = '8002' and \
					ot_date = '20040227' \
			group by ot_gd_code");
}*/
void test_groupby()
{
	iSQL		isql;
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			i;
	int			num_fields;
	int num;
	unsigned long int *lengths;
	DWORD st, et;
	char query[2048];
	int loop;
	int af_rows;

	if (iSQL_connect(&isql, NULL, "\\db\\SMILKSALE", "aladdin", "aladdin") == NULL)
	{
		__SYSLOG("iSQL_connect(): %s\n", iSQL_error(&isql));
		return;
	}
/*
	sprintf(query, "select	ot_gd_code, sum(ot_many), sum(ot_remany	), sum(ot_exmany) \
			from	smilkdb_ot\
			where	agent_code = '94642' and\
					client_code = '8002' and \
					ot_date = '20040227' \
			group by ot_gd_code \
			order by ot_gd_code");

	sprintf(query, "delete	from smilkdb_ot \
		where	agent_code='85027' \
		and		client_code='1011' \
		and		ot_ag_code='3074'	\
		and		ot_date='20040304' \
		and		ot_num=0");
*/
	sprintf(query, "select agent_code, client_code, ot_ag_code, ot_date, ot_num from smilkdb_ot \
		where	agent_code='85027' \
		and		client_code='1011' \
		and		ot_ag_code='3074'	\
		and		ot_date='20040304' \
		and		ot_num=0");
	for (loop=0; loop < 100; loop++)
	{
		st = GetTickCount();

		if (iSQL_query(&isql, query))
		{
			__SYSLOG("Error: %s\n", query);
			__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));

			iSQL_disconnect(&isql);

			return;
		}
#if 1
		res = iSQL_use_result(&isql);
		if (res == NULL)
			__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
		else
		{
			num = iSQL_num_rows(res);

			num_fields = iSQL_num_fields(res);
			for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
			{
				lengths = iSQL_fetch_lengths(res);

				for (int j=0; j<num_fields; j++)
					__SYSLOG("%s,", row[j]);
				__SYSLOG("\n");

			}

			__SYSLOG("count = %d\n", i);

			iSQL_free_result(res);
		}
#endif
		af_rows = iSQL_affected_rows(&isql);
		__SYSLOG("affected rows: %d\n", af_rows);
		iSQL_commit(&isql);

		et = GetTickCount();

		__SYSLOG(">>>>> %d: %u\n", loop, (et-st)/60);
	}

	iSQL_disconnect(&isql);
}

void test_countalltable()
{
/*	do_query("\\db\\SMILKSALE",
		"select	A.agent_code, \
				A.proc_ymd,\
				A.goods_code,\
				A.pdaflag\
			from	smilkdb_stock A\
			where	A.agent_code = '93117'\
			and	A.proc_ymd = '20040108'\
			and	A.pdaflag in ('I', 'U')");

	do_query("\\db\\SMILKSALE",
		"select	B.gd_code \
			from	smilkdb_gd B");
*/
	do_query("\\db\\SMILKSALE",
		"select	A.agent_code,\
			A.goods_code,\
			B.gd_code,\
			A.proc_ymd,\
			A.con_ea_cnt,\
			A.con_box_cnt,\
			B.gd_cntmany,\
			A.pdaflag\
			from	smilkdb_stock A, smilkdb_gd B\
			where	A.goods_code = B.gd_code\
			and	A.agent_code = '93117'\
			and	A.proc_ymd = '20040108'\
			and	A.pdaflag in ('I', 'U')");

	do_query("\\db\\SMILKSALE",
		"select	A.agent_code,\
			A.goods_code,\
			B.gd_code,\
			A.proc_ymd,\
			A.con_ea_cnt,\
			A.con_box_cnt,\
			B.gd_cntmany,\
			A.pdaflag\
			from	smilkdb_stock A, smilkdb_gd B\
			where	A.agent_code = '93117'\
			and	A.proc_ymd = '20040108'\
			and	A.pdaflag in ('I', 'U')\
			and A.goods_code = B.gd_code");

#if 0
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
		"select * from systables");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
		"select count(*) from smilkdb_alert");

	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc system_info");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_alert");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilk_oder_client");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilk_oder_expect");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_cs");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_gd");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_bs");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_ag");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_ai");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_ot");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_ch");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_ib");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_uc");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_cus");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_od");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_sync");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_stock");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_gd_user");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_customer");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_drink_change");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_cus_uc");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_cus_ib");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_common");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_area");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
			"desc smilkdb_month");

	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
		"select * from smilkdb_alert");
	do_dml_query("\\Program Files\\aladdin\\db\\SMILKSALE",
		"delete from smilkdb_alert where ymd_to < '20040102'");
	do_query("\\Program Files\\aladdin\\db\\SMILKSALE",
		"select * from smilkdb_alert");
#endif
	return;
}

void test_createdb()
{
	Drop_DB("testdb0");
	if (createdb("testdb0") < 0)
	{
		__SYSLOG("test_createdb: first createdb fail testdb0\n");
	}

	Drop_DB("db\\testdb0");
	if (createdb("db\\testdb0") < 0)
	{
		__SYSLOG("test_createdb: first createdb fail db\\testdb0\n");
	}

	Drop_DB("\\db\\testdb1");
	if (createdb("\\db\\testdb1") < 0)
	{
		__SYSLOG("test_db_path: first createdb fail\n");
		return;
	}

	return;
}

void test_err_query()
{
	iSQL		isql;
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			i;
	int			num_fields;
	int num;
	unsigned long int *lengths;
	SYSTEMTIME st, et;
	int gap;
	char *query, *query1, *query2;

	Drop_DB(DBNAME);

	if (createdb(DBNAME) < 0)
	{
		__SYSLOG("test_db_path: first createdb fail\n");
		return;
	}

	query1 = (char *)malloc(4096);
	query2 = (char *)malloc(4096);
	sprintf(query1, "%s", "create table hlwtnsrc05 (trgt_sid VARCHAR(14) NOT NULL,\
trgt_sid_sno VARCHAR(4) NOT NULL,bsin_yy CHAR(4) NOT NULL,\
chg_chasu CHAR(2) NOT NULL,trgt_nm VARCHAR(61),mg_umd_code CHAR(7) NOT NULL,\
trgt_telno CHAR(12),rcvr_gbn_code VARCHAR(2),incm_lv CHAR(3),\
fam_cmpnt_code VARCHAR(2),house_gbn_code VARCHAR(2) NOT NULL,\
famco_num CHAR(2),wel_tel_yn CHAR(1) NOT NULL,wad_fare_rae_yn CHAR(1) NOT NULL,\
trs_ev_yn CHAR(1) NOT NULL,vtv_cost_rae_cstmr_no VARCHAR(20),\
srch_rt_code VARCHAR(2),slfspt_nd_gigan_code VARCHAR(2),\
prot_dcs_opi_code VARCHAR(2),srch_stdt CHAR(8),srch_enddt CHAR(8),\
wrt_ymd CHAR(8),chg_ymd CHAR(8),chg_why VARCHAR(251),\
bs_life_prot_yn CHAR(1) NOT NULL,baby_yn CHAR(1) NOT NULL,");
	sprintf(query2, "%s%s", query1,"so_headf_yn CHAR(1) NOT NULL,mns_home_yn CHAR(1) NOT NULL,\
fns_home_yn CHAR(1) NOT NULL,obsp_wel_yn CHAR(1) NOT NULL,\
oldm_wel_yn CHAR(1) NOT NULL,hj_db_yn CHAR(1) NOT NULL,\
prot_cost_prfdoc_yn CHAR(1) NOT NULL,emp_wage_confrm_doc_yn CHAR(1) NOT NULL,\
leas_veh_ctr_doc_yn CHAR(1) NOT NULL,jd_doc_yn CHAR(1) NOT NULL,\
jh_prfdoc_yn CHAR(1) NOT NULL,js_prfdoc_yn CHAR(1) NOT NULL,\
bm_confrm_doc_yn CHAR(1) NOT NULL,dprt_entrnc_real_prfdoc_yn CHAR(1) NOT NULL,\
bc_prf_rell_pps_yn CHAR(1) NOT NULL,med_insur_pymnt_rcpt_yn CHAR(1) NOT NULL,\
free_leas_confrm_doc_yn CHAR(1) NOT NULL,etc_prsnt_pps_yn CHAR(1) NOT NULL,\
etc_prsnt_pps_ctn VARCHAR(51),incm_inq_yn CHAR(1) NOT NULL,\
mnty_org_inq_yn CHAR(1) NOT NULL,land_lg_inq_yn CHAR(1) NOT NULL,\
bdng_lg_inq_yn CHAR(1) NOT NULL,njwb_inq_yn CHAR(1) NOT NULL,\
cars_inq_yn CHAR(1) NOT NULL,deduc_pps_yn CHAR(1) NOT NULL,");
	sprintf(query1, "%s%s", query2, "supp_duter_yn CHAR(1) NOT NULL,trgt_mg_no VARCHAR(17) NOT NULL,\
wel_trgt_take_no VARCHAR(13),rsdc_kind_code VARCHAR(3),mon_tax_amt INT,\
rsdc_etc_ctn VARCHAR(51),ert_state_code VARCHAR(2),ert_state_etc VARCHAR(51),\
rsdc_alw_sort_code VARCHAR(2),rent_gigan_from CHAR(8),rent_gigan_to CHAR(8),\
ming_facility_bhj_yn CHAR(1) NOT NULL,wrk_caper_yn CHAR(1) NOT NULL,\
upr_cls_yn CHAR(1) NOT NULL,spec_case_gbn_code VARCHAR(3),\
spec_case_recp_yn CHAR(1) NOT NULL,spec_case_alw_str_day CHAR(8),\
spec_case_alw_end_day CHAR(8),rcr_id CHAR(8),rcd_dthr CHAR(14),demd_no CHAR(18),\
mnty_info_of_cst_doc_yn CHAR(1) NOT NULL,cntct_telno CHAR(12),incm_apr_amt BIGINT,\
means_cnv_amt BIGINT,status CHAR(1),chasu_increase_yn CHAR(1),agree_request_yn CHAR(1))");

	do_dml_query(DBNAME, query1);

	do_dml_query(DBNAME, "CREATE TABLE KFATNPATCL\
(\
   UPSO_SNO                      VARCHAR(40)    NOT NULL,\
   ATCL_PRD_SNO                  VARCHAR(40)    NOT NULL,\
   PRDT_NM                       VARCHAR(60)    NULL,\
   PRDT_KIND_NM                  VARCHAR(200)   NULL,\
   MAIN_RM                       VARCHAR(2000)  NULL,\
   YUTONGGIGAN                   VARCHAR(250)   NULL,\
   CHG_WHY    VARCHAR(250)   NULL,\
   SUNGSANG                      VARCHAR(250)   NULL,\
   SRV_USE                       VARCHAR(250)   NULL,\
   POJANG_METHOD                 VARCHAR(200)   NULL,\
   POJANG_UNT                    VARCHAR(60)    NULL,\
   ETC                           VARCHAR(100)   NULL,\
   ATCL_PRD_CN                   VARCHAR(80)    NULL,\
   primary key(UPSO_SNO,ATCL_PRD_SNO)\
)\
");

	do_dml_query(DBNAME, "insert into kfatnpatcl values('3040000106199100001', '000004', '바바리안크림먼치킨도너츠','','1) 이스트레이즈드도넛믹스(47.20%) - 소맥분79.65, 황산제일철 0.01, 니코탄산 0.01, 비타민B1질산염0.01, 비타민B2 0.01, 엽산 0.01, 포도당 8.20, 대두유 5.85, 정제염 1.50, 피로인산나트륨 0.80, 유청 1.00, 탈지대두분 0.50, 모노&디글리세리드 0.50, 구연산 0.20, 스테이릴젖산나트륨 0.30, 레시틴 0.20, 탈지분유 0.20, 셀룰로스검 0.20, 구아검 0.20, 천연색소(아나토, 심황색소) 0.20, 카제인나트륨 0.10, 알파아밀라아제 0.10, 바닐라향 0.10, 아라비아검 0.05, 산탄검 0.05, 카라기난 0.05\
\
2) 쇼트닝(15.52%) - 정제가공유지(대두유) 99.98,D-토코페롤 0.02 \
\
3) 던킨도넛슈가(12.11%) - 포도당 71.78, 옥수수전분 20.00, 쇼트닝 5.00, 설탕 2.10, 대두유 1.10, 바닐라향 0.02\
\
4)던킨바바리안필링(23.28%) - 정제수 56.74, 설탕시럽 25.42, 옥수수시럽 6.45, 식물성경화유(면실유,대두유) 4.44, 변성전분(히드록시프로필인산전분) 3.00, 정제염 0.38, 한천 0.30, 이산화티타늄0.0392, 식용색소황색4호&식용색소황색5호 1.00, 글루코노델타락톤 0.45, 바닐라크림향 0.26, 카리기난 0.75, 로커스빈검 0.77, 소르빈산칼륨 0.0008\
\
5)생호모(1.89%) - 생효모 100.00','2일','', '백색의도넛슈가를묻힌원구형의도넛에바바리안크림주입', '간식 및 식사대용','8, 80, 160, 960','401','','')");
//	query1 = (char *)malloc(2048);
//	sprintf(query1, "update hlwtnsrc16 set status='U', rcr_id='THINKM02', rcd_dthr='20031022135808', why_etc='ㅇㅇㅇㅇ' where trgt_sid='2603142067914' and trgt_sid_sno='000' and chg_chasu='05' and prot_why_code='113'e");
//	query2 = (char *)malloc(2048);
//	sprintf(query2, "update hlwtnsrc05 set BSIN_YY = '2003', CHG_CHASU = '03', TRGT_NM = '정영열',MG_UMD_CODE = '3100047', TRGT_TELNO = '    978 6021', RCVR_GBN_CODE = '2',INCM_LV = '888', FAM_CMPNT_CODE = '3', HOUSE_GBN_CODE = '5', FAMCO_NUM = '02',WEL_TEL_YN = 'N', WAD_FARE_RAE_YN = 'Y', TRS_EV_YN = 'Y',VTV_COST_RAE_CSTMR_NO = '153410493106302', SRCH_RT_CODE = '5',SLFSPT_ND_GIGAN_CODE = '', PROT_DCS_OPI_CODE = '1', SRCH_STDT = '20030701',SRCH_ENDDT = '20030731', WRT_YMD = '20030908', CHG_YMD = '', CHG_WHY = '',BS_LIFE_PROT_YN = 'N', BABY_YN = 'N', SO_HEADF_YN = 'N', MNS_HOME_YN = 'N',FNS_HOME_YN = 'N', OBSP_WEL_YN = 'N', OLDM_WEL_YN = 'N', HJ_DB_YN = 'N',PROT_COST_PRFDOC_YN = 'N', EMP_WAGE_CONFRM_DOC_YN = 'N', LEAS_VEH_CTR_DOC_YN = 'N',JD_DOC_YN = 'N', JH_PRFDOC_YN = 'N', JS_PRFDOC_YN = 'N', BM_CONFRM_DOC_YN = 'N',DPRT_ENTRNC_REAL_PRFDOC_YN = 'N', BC_PRF_RELL_PPS_YN = 'N',MED_INSUR_PYMNT_RCPT_YN = 'N', FREE_LEAS_CONFRM_DOC_YN = 'N', ETC_PRSNT_PPS_YN = 'N',ETC_PRSNT_PPS_CTN = '', INCM_INQ_YN = 'N', MNTY_ORG_INQ_YN = 'N', LAND_LG_INQ_YN = 'N',BDNG_LG_INQ_YN = 'N', NJWB_INQ_YN = 'N', CARS_INQ_YN = 'N', DEDUC_PPS_YN = 'N',SUPP_DUTER_YN = 'N', TRGT_MG_NO = '3100000200009344', WEL_TRGT_TAKE_NO = '3100000',RSDC_KIND_CODE = '24', MON_TAX_AMT = 0, RSDC_ETC_CTN = '', ERT_STATE_CODE = '0',ERT_STATE_ETC = '', RSDC_ALW_SORT_CODE = '1', RENT_GIGAN_FROM = '', RENT_GIGAN_TO = '',MING_FACILITY_BHJ_YN = 'N', WRK_CAPER_YN = 'N', UPR_CLS_YN = 'N',SPEC_CASE_GBN_CODE = '', SPEC_CASE_RECP_YN = 'N', SPEC_CASE_ALW_STR_DAY = '',SPEC_CASE_ALW_END_DAY = '', RCR_ID = 'SINSUNOK', RCD_DTHR = '20031021164525',DEMD_NO = '', MNTY_INFO_OF_CST_DOC_YN = 'N', CNTCT_TELNO = ' 01198890866',INCM_APR_AMT = 350000, MEANS_CNV_AMT = 0, STATUS = 'U'where trgt_sid = '5212201478814' and trgt_sid_sno = '000'");

//	create_table(DBNAME);

	sprintf(query1, "update hlwtnsrc05 set BSIN_YY = '2003', CHG_CHASU = '03', TRGT_NM = '정영열',MG_UMD_CODE = '3100047', TRGT_TELNO = '    978 6021', RCVR_GBN_CODE = '2',INCM_LV = '888', FAM_CMPNT_CODE = '3', HOUSE_GBN_CODE = '5', FAMCO_NUM = '02',WEL_TEL_YN = 'N', WAD_FARE_RAE_YN = 'Y', TRS_EV_YN = 'Y',VTV_COST_RAE_CSTMR_NO = '153410493106302', SRCH_RT_CODE = '5',SLFSPT_ND_GIGAN_CODE = '', PROT_DCS_OPI_CODE = '1', SRCH_STDT = '20030701',SRCH_ENDDT = '20030731', WRT_YMD = '20030908', CHG_YMD = '', CHG_WHY = '',BS_LIFE_PROT_YN = 'N', BABY_YN = 'N', SO_HEADF_YN = 'N', MNS_HOME_YN = 'N',FNS_HOME_YN = 'N', OBSP_WEL_YN = 'N', OLDM_WEL_YN = 'N', HJ_DB_YN = 'N',PROT_COST_PRFDOC_YN = 'N', EMP_WAGE_CONFRM_DOC_YN = 'N', LEAS_VEH_CTR_DOC_YN = 'N',JD_DOC_YN = 'N', JH_PRFDOC_YN = 'N', JS_PRFDOC_YN = 'N', BM_CONFRM_DOC_YN = 'N',DPRT_ENTRNC_REAL_PRFDOC_YN = 'N', BC_PRF_RELL_PPS_YN = 'N',MED_INSUR_PYMNT_RCPT_YN = 'N', FREE_LEAS_CONFRM_DOC_YN = 'N', ETC_PRSNT_PPS_YN = 'N',ETC_PRSNT_PPS_CTN = '', INCM_INQ_YN = 'N', MNTY_ORG_INQ_YN = 'N', LAND_LG_INQ_YN = 'N',BDNG_LG_INQ_YN = 'N', NJWB_INQ_YN = 'N', CARS_INQ_YN = 'N', DEDUC_PPS_YN = 'N',SUPP_DUTER_YN = 'N', TRGT_MG_NO = '3100000200009344', WEL_TRGT_TAKE_NO = '3100000',RSDC_KIND_CODE = '24', MON_TAX_AMT = 0, RSDC_ETC_CTN = '', ERT_STATE_CODE = '0',ERT_STATE_ETC = '', RSDC_ALW_SORT_CODE = '1', RENT_GIGAN_FROM = '', RENT_GIGAN_TO = '',MING_FACILITY_BHJ_YN = 'N', WRK_CAPER_YN = 'N', UPR_CLS_YN = 'N',SPEC_CASE_GBN_CODE = '', SPEC_CASE_RECP_YN = 'N', SPEC_CASE_ALW_STR_DAY = '',SPEC_CASE_ALW_END_DAY = '', RCR_ID = 'SINSUNOK', RCD_DTHR = '20031021164525',DEMD_NO = '', MNTY_INFO_OF_CST_DOC_YN = 'N', CNTCT_TELNO = ' 01198890866',INCM_APR_AMT = 350000, MEANS_CNV_AMT = 0, STATUS = 'U'where trgt_sid = '5212201478814' and trgt_sid_sno = '000'");
//	sprintf(query1, "insert into teladdr(number,bb) values(1,1)");
//	sprintf(query2, "select crit_amt1 from hlwtnply21 where crit_gbn_code='10' and bsin_yy='2003' and sf_team_gbn_code='1'");
	sprintf(query2, "select a.tableid, count(*) from systables a, sysfields b where a.tableid=b.tableid group by a.tableid order by a.tableid desc");

	GetSystemTime(&st);

	if (iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin") == NULL)
	{
		__SYSLOG("iSQL_connect(): %s\n", iSQL_error(&isql));
		free(query1); free(query2);
		return;
	}

	for (i=0; i<=100; i++)
	{
//		iSQL_disconnect(&isql);

		if (i % 2 == 0)
			query = query1;
		else
			query = query2;

		if (iSQL_query(&isql, query))
		{
			__SYSLOG("Error: %s\n", query);
			__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));

			iSQL_rollback(&isql);
		}
		else
		{
			if((res = iSQL_use_result(&isql)) == NULL)
			{
				// select가 아닌 다른 Query(Delete, Insert, Update)인 경우 통과
				if(iSQL_errno(&isql) == -1102)
				{
					iSQL_commit(&isql);		// Data에 변형을 가했으므로 Commit을 한다.
					continue;
				}

				iSQL_rollback(&isql);
				continue;
			}

			num = iSQL_num_rows(res);
			num_fields = iSQL_num_fields(res);
			for (int j = 0; (row = iSQL_fetch_row(res)) != NULL; j++)
			{
				lengths = iSQL_fetch_lengths(res);

				for (int k=0; k<num_fields; k++)
					__SYSLOG("%s,", row[k]);
				__SYSLOG("\n");
			}

			__SYSLOG("count = %d\n", j);

			iSQL_free_result(res);

			iSQL_commit(&isql);
		}
	}

	iSQL_disconnect(&isql);

	GetSystemTime(&et);

	gap = (et.wMinute - st.wMinute);
	gap *= 60;
	gap += (et.wSecond - st.wSecond);

	__SYSLOG(">>>>> %d\n", gap);
	__SYSLOG("--------------------------------------------------------------------\n");

	free(query1); free(query2);

	return;
}

void test_insert_longfield()
{
	iSQL		isql;
	int			i;
	SYSTEMTIME st, et;
	int gap;
	char *query1;

	query1 = (char *)malloc(4096);

	Drop_DB(DBNAME);

	if (createdb(DBNAME) < 0)
	{
		__SYSLOG("test_db_path: first createdb fail\n");
		free(query1);
		return;
	}

		do_dml_query(DBNAME, "CREATE TABLE KFATNPATCL\
	(\
	   UPSO_SNO                      VARCHAR(40)    NOT NULL,\
	   ATCL_PRD_SNO                  VARCHAR(40)    NOT NULL,\
	   PRDT_NM                       VARCHAR(60)    NULL,\
	   PRDT_KIND_NM                  VARCHAR(200)   NULL,\
	   MAIN_RM                       VARCHAR(2000)  NULL,\
	   YUTONGGIGAN                   VARCHAR(250)   NULL,\
	   CHG_WHY    VARCHAR(250)   NULL,\
	   SUNGSANG                      VARCHAR(250)   NULL,\
	   SRV_USE                       VARCHAR(250)   NULL,\
	   POJANG_METHOD                 VARCHAR(200)   NULL,\
	   POJANG_UNT                    VARCHAR(60)    NULL,\
	   ETC                           VARCHAR(100)   NULL,\
	   ATCL_PRD_CN                   VARCHAR(80)    NULL,\
	   primary key(UPSO_SNO,ATCL_PRD_SNO)\
	)\
	");

	GetSystemTime(&st);

	if (iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin") == NULL)
	{
		__SYSLOG("iSQL_connect(): %s\n", iSQL_error(&isql));
		free(query1);
		return;
	}

	for (i=0; i<=100; i++)
	{
		sprintf(query1, "insert into kfatnpatcl values('3040000106199100001', '%d', '바바리안크림먼치킨도너츠','','1) 이스트레이즈드도넛믹스(47.20%) - 소맥분79.65, 황산제일철 0.01, 니코탄산 0.01, 비타민B1질산염0.01, 비타민B2 0.01, 엽산 0.01, 포도당 8.20, 대두유 5.85, 정제염 1.50, 피로인산나트륨 0.80, 유청 1.00, 탈지대두분 0.50, 모노&디글리세리드 0.50, 구연산 0.20, 스테이릴젖산나트륨 0.30, 레시틴 0.20, 탈지분유 0.20, 셀룰로스검 0.20, 구아검 0.20, 천연색소(아나토, 심황색소) 0.20, 카제인나트륨 0.10, 알파아밀라아제 0.10, 바닐라향 0.10, 아라비아검 0.05, 산탄검 0.05, 카라기난 0.05\
	\
	2) 쇼트닝(15.52%) - 정제가공유지(대두유) 99.98,D-토코페롤 0.02 \
	\
	3) 던킨도넛슈가(12.11%) - 포도당 71.78, 옥수수전분 20.00, 쇼트닝 5.00, 설탕 2.10, 대두유 1.10, 바닐라향 0.02\
	\
	4)던킨바바리안필링(23.28%) - 정제수 56.74, 설탕시럽 25.42, 옥수수시럽 6.45, 식물성경화유(면실유,대두유) 4.44, 변성전분(히드록시프로필인산전분) 3.00, 정제염 0.38, 한천 0.30, 이산화티타늄0.0392, 식용색소황색4호&식용색소황색5호 1.00, 글루코노델타락톤 0.45, 바닐라크림향 0.26, 카리기난 0.75, 로커스빈검 0.77, 소르빈산칼륨 0.0008\
	\
	5)생호모(1.89%) - 생효모 100.00','2일','', '백색의도넛슈가를묻힌원구형의도넛에바바리안크림주입', '간식 및 식사대용','8, 80, 160, 960','401','','')",
		i);

		if (iSQL_query(&isql, query1))
		{
			__SYSLOG("Error: %s\n", query1);
			__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));
			iSQL_rollback(&isql);
			break;
		}
		else
		{
			iSQL_commit(&isql);
		}
	}

	iSQL_disconnect(&isql);

	GetSystemTime(&et);

	gap = (et.wMinute - st.wMinute);
	gap *= 60;
	gap += (et.wSecond - st.wSecond);

	__SYSLOG(">>>>> %d\n", gap);
	__SYSLOG("\tst:%d:%d:%d - et:%d:%d:%d\n",
			st.wHour, st.wMinute, st.wSecond,
			et.wHour, et.wMinute, et.wSecond);
	__SYSLOG("--------------------------------------------------------------------\n");

	free(query1);

	return;
}

void test_insert_longfield2()
{
	iSQL		isql;
	int			i;
	SYSTEMTIME st, et;
	int gap;
	char *query1;

	query1 = (char *)malloc(4096);

#if 1
	Drop_DB(DBNAME);

	if (createdb(DBNAME) < 0)
	{
		__SYSLOG("test_db_path: first createdb fail\n");
		free(query1);
		return;
	}

		do_dml_query(DBNAME, "CREATE TABLE KFATNPATCL\
	(\
	   UPSO_SNO                      VARCHAR(40)    NOT NULL,\
	   ATCL_PRD_SNO                  VARCHAR(40)    NOT NULL,\
	   PRDT_NM                       VARCHAR(60)    NULL,\
	   PRDT_KIND_NM                  VARCHAR(200)   NULL,\
	   MAIN_RM                       VARCHAR(2000)  NULL,\
	   YUTONGGIGAN                   VARCHAR(250)   NULL,\
	   CHG_WHY    VARCHAR(250)   NULL,\
	   SUNGSANG                      VARCHAR(250)   NULL,\
	   SRV_USE                       VARCHAR(250)   NULL,\
	   POJANG_METHOD                 VARCHAR(200)   NULL,\
	   POJANG_UNT                    VARCHAR(60)    NULL,\
	   ETC                           VARCHAR(100)   NULL,\
	   ATCL_PRD_CN                   VARCHAR(80)    NULL,\
	   primary key(UPSO_SNO,ATCL_PRD_SNO)\
	)\
	");

		do_dml_query(DBNAME, "CREATE TABLE KFATNPATCL2\
	(\
	   UPSO_SNO                      VARCHAR(40)    NOT NULL,\
	   ATCL_PRD_SNO                  VARCHAR(40)    NOT NULL,\
	   PRDT_NM                       VARCHAR(60)    NULL,\
	   PRDT_KIND_NM                  VARCHAR(200)   NULL,\
	   MAIN_RM                       VARCHAR(2000)  NULL,\
	   YUTONGGIGAN                   VARCHAR(250)   NULL,\
	   CHG_WHY    VARCHAR(250)   NULL,\
	   SUNGSANG                      VARCHAR(250)   NULL,\
	   SRV_USE                       VARCHAR(250)   NULL,\
	   POJANG_METHOD                 VARCHAR(200)   NULL,\
	   POJANG_UNT                    VARCHAR(60)    NULL,\
	   ETC                           VARCHAR(100)   NULL,\
	   ATCL_PRD_CN                   VARCHAR(80)    NULL,\
	   primary key(UPSO_SNO,ATCL_PRD_SNO)\
	)\
	");

		do_dml_query(DBNAME, "CREATE TABLE KFATNPATCL3\
	(\
	   UPSO_SNO                      VARCHAR(40)    NOT NULL,\
	   ATCL_PRD_SNO                  VARCHAR(40)    NOT NULL,\
	   PRDT_NM                       VARCHAR(60)    NULL,\
	   PRDT_KIND_NM                  VARCHAR(200)   NULL,\
	   MAIN_RM                       VARCHAR(2000)  NULL,\
	   YUTONGGIGAN                   VARCHAR(250)   NULL,\
	   CHG_WHY    VARCHAR(250)   NULL,\
	   SUNGSANG                      VARCHAR(250)   NULL,\
	   SRV_USE                       VARCHAR(250)   NULL,\
	   POJANG_METHOD                 VARCHAR(200)   NULL,\
	   POJANG_UNT                    VARCHAR(60)    NULL,\
	   ETC                           VARCHAR(100)   NULL,\
	   ATCL_PRD_CN                   VARCHAR(80)    NULL,\
	   primary key(UPSO_SNO,ATCL_PRD_SNO)\
	)\
	");
#endif

	GetSystemTime(&st);

	if (iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin") == NULL)
	{
		__SYSLOG("iSQL_connect(): %s\n", iSQL_error(&isql));
		free(query1);
		return;
	}

    do_query2(&isql, "select count(*) from KFATNPATCL");
    do_query2(&isql, "select count(*) from KFATNPATCL2");
    do_query2(&isql, "select count(*) from KFATNPATCL3");
    do_query2(&isql, "select count(*) from KFATNPATCL where UPSO_SNO = '3040000106199100001'");
    do_query2(&isql, "select count(*) from KFATNPATCL2 where UPSO_SNO = '3040000106199100001'");
    do_query2(&isql, "select count(*) from KFATNPATCL3 where UPSO_SNO = '3040000106199100001'");
    iSQL_commit(&isql);

    do_dml_query2(&isql, "delete from KFATNPATCL");
    do_dml_query2(&isql, "delete from KFATNPATCL2");
    do_dml_query2(&isql, "delete from KFATNPATCL3");

	for (i=0; i<=1000; i++)
	{
		sprintf(query1, "insert into kfatnpatcl values('3040000106199100001', '%d', '바바리안크림먼치킨도너츠','','1) 이스트레이즈드도넛믹스(47.20%) - 소맥분79.65, 황산제일철 0.01, 니코탄산 0.01, 비타민B1질산염0.01, 비타민B2 0.01, 엽산 0.01, 포도당 8.20, 대두유 5.85, 정제염 1.50, 피로인산나트륨 0.80, 유청 1.00, 탈지대두분 0.50, 모노&디글리세리드 0.50, 구연산 0.20, 스테이릴젖산나트륨 0.30, 레시틴 0.20, 탈지분유 0.20, 셀룰로스검 0.20, 구아검 0.20, 천연색소(아나토, 심황색소) 0.20, 카제인나트륨 0.10, 알파아밀라아제 0.10, 바닐라향 0.10, 아라비아검 0.05, 산탄검 0.05, 카라기난 0.05\
	\
	2) 쇼트닝(15.52%) - 정제가공유지(대두유) 99.98,D-토코페롤 0.02 \
	\
	3) 던킨도넛슈가(12.11%) - 포도당 71.78, 옥수수전분 20.00, 쇼트닝 5.00, 설탕 2.10, 대두유 1.10, 바닐라향 0.02\
	\
	4)던킨바바리안필링(23.28%) - 정제수 56.74, 설탕시럽 25.42, 옥수수시럽 6.45, 식물성경화유(면실유,대두유) 4.44, 변성전분(히드록시프로필인산전분) 3.00, 정제염 0.38, 한천 0.30, 이산화티타늄0.0392, 식용색소황색4호&식용색소황색5호 1.00, 글루코노델타락톤 0.45, 바닐라크림향 0.26, 카리기난 0.75, 로커스빈검 0.77, 소르빈산칼륨 0.0008\
	\
	5)생호모(1.89%) - 생효모 100.00','2일','', '백색의도넛슈가를묻힌원구형의도넛에바바리안크림주입', '간식 및 식사대용','8, 80, 160, 960','401','','')",
		i);

		if (iSQL_query(&isql, query1))
		{
			__SYSLOG("Error: %s\n", query1);
			__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));
			iSQL_rollback(&isql);
			break;
		}
		else
		{
			iSQL_commit(&isql);
		}

		sprintf(query1, "insert into kfatnpatcl2 values('3040000106199100001', '%d', '바바리안크림먼치킨도너츠','','1) 이스트레이즈드도넛믹스(47.20%) - 소맥분79.65, 황산제일철 0.01, 니코탄산 0.01, 비타민B1질산염0.01, 비타민B2 0.01, 엽산 0.01, 포도당 8.20, 대두유 5.85, 정제염 1.50, 피로인산나트륨 0.80, 유청 1.00, 탈지대두분 0.50, 모노&디글리세리드 0.50, 구연산 0.20, 스테이릴젖산나트륨 0.30, 레시틴 0.20, 탈지분유 0.20, 셀룰로스검 0.20, 구아검 0.20, 천연색소(아나토, 심황색소) 0.20, 카제인나트륨 0.10, 알파아밀라아제 0.10, 바닐라향 0.10, 아라비아검 0.05, 산탄검 0.05, 카라기난 0.05\
	\
	2) 쇼트닝(15.52%) - 정제가공유지(대두유) 99.98,D-토코페롤 0.02 \
	\
	3) 던킨도넛슈가(12.11%) - 포도당 71.78, 옥수수전분 20.00, 쇼트닝 5.00, 설탕 2.10, 대두유 1.10, 바닐라향 0.02\
	\
	4)던킨바바리안필링(23.28%) - 정제수 56.74, 설탕시럽 25.42, 옥수수시럽 6.45, 식물성경화유(면실유,대두유) 4.44, 변성전분(히드록시프로필인산전분) 3.00, 정제염 0.38, 한천 0.30, 이산화티타늄0.0392, 식용색소황색4호&식용색소황색5호 1.00, 글루코노델타락톤 0.45, 바닐라크림향 0.26, 카리기난 0.75, 로커스빈검 0.77, 소르빈산칼륨 0.0008\
	\
	5)생호모(1.89%) - 생효모 100.00','2일','', '백색의도넛슈가를묻힌원구형의도넛에바바리안크림주입', '간식 및 식사대용','8, 80, 160, 960','401','','')",
		i);

		if (iSQL_query(&isql, query1))
		{
			__SYSLOG("Error: %s\n", query1);
			__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));
			iSQL_rollback(&isql);
			break;
		}
		else
		{
			iSQL_commit(&isql);
		}

		sprintf(query1, "insert into kfatnpatcl3 values('3040000106199100001', '%d', '바바리안크림먼치킨도너츠','','1) 이스트레이즈드도넛믹스(47.20%) - 소맥분79.65, 황산제일철 0.01, 니코탄산 0.01, 비타민B1질산염0.01, 비타민B2 0.01, 엽산 0.01, 포도당 8.20, 대두유 5.85, 정제염 1.50, 피로인산나트륨 0.80, 유청 1.00, 탈지대두분 0.50, 모노&디글리세리드 0.50, 구연산 0.20, 스테이릴젖산나트륨 0.30, 레시틴 0.20, 탈지분유 0.20, 셀룰로스검 0.20, 구아검 0.20, 천연색소(아나토, 심황색소) 0.20, 카제인나트륨 0.10, 알파아밀라아제 0.10, 바닐라향 0.10, 아라비아검 0.05, 산탄검 0.05, 카라기난 0.05\
	\
	2) 쇼트닝(15.52%) - 정제가공유지(대두유) 99.98,D-토코페롤 0.02 \
	\
	3) 던킨도넛슈가(12.11%) - 포도당 71.78, 옥수수전분 20.00, 쇼트닝 5.00, 설탕 2.10, 대두유 1.10, 바닐라향 0.02\
	\
	4)던킨바바리안필링(23.28%) - 정제수 56.74, 설탕시럽 25.42, 옥수수시럽 6.45, 식물성경화유(면실유,대두유) 4.44, 변성전분(히드록시프로필인산전분) 3.00, 정제염 0.38, 한천 0.30, 이산화티타늄0.0392, 식용색소황색4호&식용색소황색5호 1.00, 글루코노델타락톤 0.45, 바닐라크림향 0.26, 카리기난 0.75, 로커스빈검 0.77, 소르빈산칼륨 0.0008\
	\
	5)생호모(1.89%) - 생효모 100.00','2일','', '백색의도넛슈가를묻힌원구형의도넛에바바리안크림주입', '간식 및 식사대용','8, 80, 160, 960','401','','')",
		i);

		if (iSQL_query(&isql, query1))
		{
			__SYSLOG("Error: %s\n", query1);
			__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));
			iSQL_rollback(&isql);
			break;
		}
		else
		{
			iSQL_commit(&isql);
		}
	}

    do_query2(&isql, "select count(*) from KFATNPATCL");
    iSQL_commit(&isql);

    do_query2(&isql, "select count(*) from KFATNPATCL2");
    iSQL_commit(&isql);

    do_query2(&isql, "select count(*) from KFATNPATCL3");
    iSQL_commit(&isql);

    do_query2(&isql, 
			"select count(*) from KFATNPATCL \
            where UPSO_SNO='3040000106199100001' and ATCL_PRD_SNO < '150'");
    iSQL_commit(&isql);

    do_query2(&isql, 
			"select count(*) from KFATNPATCL2 \
            where UPSO_SNO='3040000106199100001' and ATCL_PRD_SNO < '150'");
    iSQL_commit(&isql);

    do_query2(&isql, 
			"select count(*) from KFATNPATCL3 \
            where UPSO_SNO='3040000106199100001' and ATCL_PRD_SNO < '150'");
    iSQL_commit(&isql);

	iSQL_disconnect(&isql);

	GetSystemTime(&et);

	gap = (et.wMinute - st.wMinute);
	gap *= 60;
	gap += (et.wSecond - st.wSecond);

	__SYSLOG(">>>>> %d\n", gap);
	__SYSLOG("\tst:%d:%d:%d - et:%d:%d:%d\n",
			st.wHour, st.wMinute, st.wSecond,
			et.wHour, et.wMinute, et.wSecond);
	__SYSLOG("--------------------------------------------------------------------\n");

	free(query1);

	return;
}

void test_insert_delete_longfield_exit()
{
	iSQL		isql;
	int			i;
	SYSTEMTIME st, et;
	int gap;
	char *query1;
    int loop;

	query1 = (char *)malloc(4096);

#if 1
    Drop_DB(DBNAME);

	if (createdb(DBNAME) < 0)
	{
		__SYSLOG("test_db_path: first createdb fail\n");
	}
#endif

#if 0
    if (DB_NeedVerification(DBNAME))
    {
        AfxMessageBox(L"Need Verification");
    }
#endif

	if (iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin") == NULL)
    //if (iSQL_connect(&isql, NULL, "skwoo\\db\\libtest", "aladdin", "aladdin") == NULL)
	{
		__SYSLOG("iSQL_connect(): %s\n", iSQL_error(&isql));
		free(query1);
		return;
	}

    GetSystemTime(&st);

    for (loop = 0; loop < 1000; loop++)
    {

        do_query2(&isql, 
			    "select count(*) from KFATNPATCL");
        iSQL_commit(&isql);

        do_query2(&isql, 
			    "select count(*) from KFATNPATCL \
                where UPSO_SNO='3040000106199100001' and ATCL_PRD_SNO < '150'");
        iSQL_commit(&isql);

        do_query2(&isql, 
			    "select count(*) from KFATNPATCL \
                where UPSO_SNO='3040000106199100001' and ATCL_PRD_SNO > '900'");
        iSQL_commit(&isql);

        do_query2(&isql, 
			    "select UPSO_SNO, ATCL_PRD_SNO from KFATNPATCL \
                where UPSO_SNO='3040000106199100001' and ATCL_PRD_SNO < '150'");
        iSQL_commit(&isql);

        do_dml_query2(&isql, "drop table KFATNPATCL");
        do_query2(&isql, 
			    "select count(*) from KFATNPATCL");
        iSQL_commit(&isql);
	do_dml_query2(&isql, "CREATE TABLE KFATNPATCL\
	(\
	   UPSO_SNO                      VARCHAR(40)    NOT NULL,\
	   ATCL_PRD_SNO                  VARCHAR(40)    NOT NULL,\
	   PRDT_NM                       VARCHAR(60)    NULL,\
	   PRDT_KIND_NM                  VARCHAR(200)   NULL,\
	   MAIN_RM                       VARCHAR(2000)  NULL,\
	   YUTONGGIGAN                   VARCHAR(250)   NULL,\
	   CHG_WHY    VARCHAR(250)   NULL,\
	   SUNGSANG                      VARCHAR(250)   NULL,\
	   SRV_USE                       VARCHAR(250)   NULL,\
	   POJANG_METHOD                 VARCHAR(200)   NULL,\
	   POJANG_UNT                    VARCHAR(60)    NULL,\
	   ETC                           VARCHAR(100)   NULL,\
	   ATCL_PRD_CN                   VARCHAR(80)    NULL,\
	   primary key(UPSO_SNO,ATCL_PRD_SNO)\
	)\
	");
	    for (i=0; i<=1000; i++)
        //for (i=100; i>=0; i--)
	    {
		    sprintf(query1, "insert into kfatnpatcl values('3040000106199100001', '%d', '바바리안크림먼치킨도너츠','','1) 이스트레이즈드도넛믹스(47.20%) - 소맥분79.65, 황산제일철 0.01, 니코탄산 0.01, 비타민B1질산염0.01, 비타민B2 0.01, 엽산 0.01, 포도당 8.20, 대두유 5.85, 정제염 1.50, 피로인산나트륨 0.80, 유청 1.00, 탈지대두분 0.50, 모노&디글리세리드 0.50, 구연산 0.20, 스테이릴젖산나트륨 0.30, 레시틴 0.20, 탈지분유 0.20, 셀룰로스검 0.20, 구아검 0.20, 천연색소(아나토, 심황색소) 0.20, 카제인나트륨 0.10, 알파아밀라아제 0.10, 바닐라향 0.10, 아라비아검 0.05, 산탄검 0.05, 카라기난 0.05\
	    \
	    2) 쇼트닝(15.52%) - 정제가공유지(대두유) 99.98,D-토코페롤 0.02 \
	    \
	    3) 던킨도넛슈가(12.11%) - 포도당 71.78, 옥수수전분 20.00, 쇼트닝 5.00, 설탕 2.10, 대두유 1.10, 바닐라향 0.02\
	    \
	    4)던킨바바리안필링(23.28%) - 정제수 56.74, 설탕시럽 25.42, 옥수수시럽 6.45, 식물성경화유(면실유,대두유) 4.44, 변성전분(히드록시프로필인산전분) 3.00, 정제염 0.38, 한천 0.30, 이산화티타늄0.0392, 식용색소황색4호&식용색소황색5호 1.00, 글루코노델타락톤 0.45, 바닐라크림향 0.26, 카리기난 0.75, 로커스빈검 0.77, 소르빈산칼륨 0.0008\
	    \
	    5)생호모(1.89%) - 생효모 100.00','2일','', '백색의도넛슈가를묻힌원구형의도넛에바바리안크림주입', '간식 및 식사대용','8, 80, 160, 960','401','','')",
		    i);

		    if (iSQL_query(&isql, query1))
		    {
			    __SYSLOG("Error: %s\n", query1);
			    __SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));
			    iSQL_rollback(&isql);
			    break;
		    }
		    else
		    {
			    iSQL_commit(&isql);
		    }

    //        if (i % 100 == 0)
    //            DB_FlushBuffer();
	    }

        do_query2(&isql, 
			    "select count(*) from KFATNPATCL");
        iSQL_commit(&isql);

        do_dml_query2(&isql, "delete from KFATNPATCL");
        iSQL_commit(&isql);

        do_query2(&isql, 
			    "select count(*) from KFATNPATCL \
                where UPSO_SNO='3040000106199100001' and ATCL_PRD_SNO < 150");
        iSQL_commit(&isql);

        do_query2(&isql, 
			    "select count(*) from KFATNPATCL \
                where UPSO_SNO='3040000106199100001' and ATCL_PRD_SNO > 900");
        iSQL_commit(&isql);
    } /* for-loop */

    iSQL_disconnect(&isql);

	GetSystemTime(&et);

	gap = (et.wMinute - st.wMinute);
	gap *= 60;
	gap += (et.wSecond - st.wSecond);

	__SYSLOG(">>>>> %d\n", gap);
	__SYSLOG("\tst:%d:%d:%d - et:%d:%d:%d\n",
			st.wHour, st.wMinute, st.wSecond,
			et.wHour, et.wMinute, et.wSecond);
	__SYSLOG("--------------------------------------------------------------------\n");

	free(query1);

	return;
}

/* disconnect 없이 exit */
void test_insert_longfield_exit()
{
	iSQL		isql;
	int			i;
	SYSTEMTIME st, et;
	int gap;
	char *query1;

	query1 = (char *)malloc(4096);
#if 0
    Drop_DB(DBNAME);

	if (createdb(DBNAME, query1) < 0)
	{
		__SYSLOG("test_db_path: %d first createdb fail\n", i);
	}
#endif

#if 0
    if (DB_NeedVerification(DBNAME))
    {
        AfxMessageBox(L"Need Verification");
    }
#endif

	if (iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin") == NULL)
    //if (iSQL_connect(&isql, NULL, "skwoo\\db\\libtest", "aladdin", "aladdin") == NULL)
	{
		__SYSLOG("iSQL_connect(): %s\n", iSQL_error(&isql));
		free(query1);
		return;
	}
#if 0
	do_dml_query2(&isql, "CREATE TABLE KFATNPATCL\
	(\
	   UPSO_SNO                      VARCHAR(40)    NOT NULL,\
	   ATCL_PRD_SNO                  VARCHAR(40)    NOT NULL,\
	   PRDT_NM                       VARCHAR(60)    NULL,\
	   PRDT_KIND_NM                  VARCHAR(200)   NULL,\
	   MAIN_RM                       VARCHAR(2000)  NULL,\
	   YUTONGGIGAN                   VARCHAR(250)   NULL,\
	   CHG_WHY    VARCHAR(250)   NULL,\
	   SUNGSANG                      VARCHAR(250)   NULL,\
	   SRV_USE                       VARCHAR(250)   NULL,\
	   POJANG_METHOD                 VARCHAR(200)   NULL,\
	   POJANG_UNT                    VARCHAR(60)    NULL,\
	   ETC                           VARCHAR(100)   NULL,\
	   ATCL_PRD_CN                   VARCHAR(80)    NULL,\
	   primary key(UPSO_SNO,ATCL_PRD_SNO)\
	)\
	");
#endif
	GetSystemTime(&st);

	for (i=0; i<=10; i++)
	{
		sprintf(query1, "insert into kfatnpatcl values('3040000106199100001', '%d', '바바리안크림먼치킨도너츠','','1) 이스트레이즈드도넛믹스(47.20%) - 소맥분79.65, 황산제일철 0.01, 니코탄산 0.01, 비타민B1질산염0.01, 비타민B2 0.01, 엽산 0.01, 포도당 8.20, 대두유 5.85, 정제염 1.50, 피로인산나트륨 0.80, 유청 1.00, 탈지대두분 0.50, 모노&디글리세리드 0.50, 구연산 0.20, 스테이릴젖산나트륨 0.30, 레시틴 0.20, 탈지분유 0.20, 셀룰로스검 0.20, 구아검 0.20, 천연색소(아나토, 심황색소) 0.20, 카제인나트륨 0.10, 알파아밀라아제 0.10, 바닐라향 0.10, 아라비아검 0.05, 산탄검 0.05, 카라기난 0.05\
	\
	2) 쇼트닝(15.52%) - 정제가공유지(대두유) 99.98,D-토코페롤 0.02 \
	\
	3) 던킨도넛슈가(12.11%) - 포도당 71.78, 옥수수전분 20.00, 쇼트닝 5.00, 설탕 2.10, 대두유 1.10, 바닐라향 0.02\
	\
	4)던킨바바리안필링(23.28%) - 정제수 56.74, 설탕시럽 25.42, 옥수수시럽 6.45, 식물성경화유(면실유,대두유) 4.44, 변성전분(히드록시프로필인산전분) 3.00, 정제염 0.38, 한천 0.30, 이산화티타늄0.0392, 식용색소황색4호&식용색소황색5호 1.00, 글루코노델타락톤 0.45, 바닐라크림향 0.26, 카리기난 0.75, 로커스빈검 0.77, 소르빈산칼륨 0.0008\
	\
	5)생호모(1.89%) - 생효모 100.00','2일','', '백색의도넛슈가를묻힌원구형의도넛에바바리안크림주입', '간식 및 식사대용','8, 80, 160, 960','401','','')",
		i);

		if (iSQL_query(&isql, query1))
		{
			__SYSLOG("Error: %s\n", query1);
			__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));
			iSQL_rollback(&isql);
			break;
		}
		else
		{
			iSQL_commit(&isql);
		}

//        if (i % 100 == 0)
//            DB_FlushBuffer();
	}

    do_query2(&isql, 
			"select count(*) from KFATNPATCL \
             where UPSO_SNO='3040000106199100001' and ATCL_PRD_SNO < 1000");
    iSQL_commit(&isql);
    iSQL_flush_buffer();

    do_query2(&isql, 
			"select count(*) from KFATNPATCL \
             where UPSO_SNO='3040000106199100001' and ATCL_PRD_SNO < 3000");
    iSQL_commit(&isql);
    iSQL_flush_buffer();

    do_query2(&isql, 
			"select count(*) from KFATNPATCL \
             where UPSO_SNO='3040000106199100001' and ATCL_PRD_SNO < 5000");
    iSQL_commit(&isql);
    iSQL_flush_buffer();

    do_query2(&isql, 
			"select count(*) from KFATNPATCL \
             where UPSO_SNO='3040000106199100001' and ATCL_PRD_SNO < 7000");
    iSQL_commit(&isql);
    iSQL_flush_buffer();

    do_query2(&isql, 
			"select count(*) from KFATNPATCL \
             where UPSO_SNO='3040000106199100001' and ATCL_PRD_SNO < 10000");
    iSQL_commit(&isql);
    iSQL_flush_buffer();

    iSQL_disconnect(&isql);

	GetSystemTime(&et);

	gap = (et.wMinute - st.wMinute);
	gap *= 60;
	gap += (et.wSecond - st.wSecond);

	__SYSLOG(">>>>> %d\n", gap);
	__SYSLOG("\tst:%d:%d:%d - et:%d:%d:%d\n",
			st.wHour, st.wMinute, st.wSecond,
			et.wHour, et.wMinute, et.wSecond);
	__SYSLOG("--------------------------------------------------------------------\n");

	free(query1);

	return;
}

void test_insert_longfield_exit2()
{
	iSQL		isql;
	int			i;
	SYSTEMTIME st, et;
	int gap;
	char *query1;

	query1 = (char *)malloc(4096);

	//Drop_DB(DBNAME);

	if (createdb(DBNAME) < 0)
	{
		__SYSLOG("test_db_path: first createdb fail\n");
	}

	if (iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin") == NULL)
	{
		__SYSLOG("iSQL_connect(): %s\n", iSQL_error(&isql));
		free(query1);
		return;
	}

	do_dml_query2(&isql, "CREATE TABLE KFATNPATCL\
	(\
	   UPSO_SNO                      VARCHAR(40)    NOT NULL,\
	   ATCL_PRD_SNO                  VARCHAR(40)    NOT NULL,\
	   PRDT_NM                       VARCHAR(60)    NULL,\
	   PRDT_KIND_NM                  VARCHAR(200)   NULL,\
	   MAIN_RM                       VARCHAR(2000)  NULL,\
	   YUTONGGIGAN                   VARCHAR(250)   NULL,\
	   CHG_WHY    VARCHAR(250)   NULL,\
	   SUNGSANG                      VARCHAR(250)   NULL,\
	   SRV_USE                       VARCHAR(250)   NULL,\
	   POJANG_METHOD                 VARCHAR(200)   NULL,\
	   POJANG_UNT                    VARCHAR(60)    NULL,\
	   ETC                           VARCHAR(100)   NULL,\
	   ATCL_PRD_CN                   VARCHAR(80)    NULL,\
	   primary key(UPSO_SNO,ATCL_PRD_SNO)\
	)\
	");

	GetSystemTime(&st);

	for (i=0; i<=10000; i++)
	{
		sprintf(query1, "insert into kfatnpatcl values('3040000106199100001', '%d', '바바리안크림먼치킨도너츠','','1) 이스트레이즈드도넛믹스(47.20%) - 소맥분79.65, 황산제일철 0.01, 니코탄산 0.01, 비타민B1질산염0.01, 비타민B2 0.01, 엽산 0.01, 포도당 8.20, 대두유 5.85, 정제염 1.50, 피로인산나트륨 0.80, 유청 1.00, 탈지대두분 0.50, 모노&디글리세리드 0.50, 구연산 0.20, 스테이릴젖산나트륨 0.30, 레시틴 0.20, 탈지분유 0.20, 셀룰로스검 0.20, 구아검 0.20, 천연색소(아나토, 심황색소) 0.20, 카제인나트륨 0.10, 알파아밀라아제 0.10, 바닐라향 0.10, 아라비아검 0.05, 산탄검 0.05, 카라기난 0.05\
	\
	2) 쇼트닝(15.52%) - 정제가공유지(대두유) 99.98,D-토코페롤 0.02 \
	\
	3) 던킨도넛슈가(12.11%) - 포도당 71.78, 옥수수전분 20.00, 쇼트닝 5.00, 설탕 2.10, 대두유 1.10, 바닐라향 0.02\
	\
	4)던킨바바리안필링(23.28%) - 정제수 56.74, 설탕시럽 25.42, 옥수수시럽 6.45, 식물성경화유(면실유,대두유) 4.44, 변성전분(히드록시프로필인산전분) 3.00, 정제염 0.38, 한천 0.30, 이산화티타늄0.0392, 식용색소황색4호&식용색소황색5호 1.00, 글루코노델타락톤 0.45, 바닐라크림향 0.26, 카리기난 0.75, 로커스빈검 0.77, 소르빈산칼륨 0.0008\
	\
	5)생호모(1.89%) - 생효모 100.00','2일','', '백색의도넛슈가를묻힌원구형의도넛에바바리안크림주입', '간식 및 식사대용','8, 80, 160, 960','401','','')",
		i);

		if (iSQL_query(&isql, query1))
		{
			__SYSLOG("Error: %s\n", query1);
			__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));
			iSQL_rollback(&isql);
			break;
		}
		else
		{
			iSQL_commit(&isql);
		}
	}

	iSQL_disconnect(&isql);

	GetSystemTime(&et);

	gap = (et.wMinute - st.wMinute);
	gap *= 60;
	gap += (et.wSecond - st.wSecond);

	__SYSLOG(">>>>> %d\n", gap);
	__SYSLOG("\tst:%d:%d:%d - et:%d:%d:%d\n",
			st.wHour, st.wMinute, st.wSecond,
			et.wHour, et.wMinute, et.wSecond);
	__SYSLOG("--------------------------------------------------------------------\n");

	free(query1);

	return;
}

void test_delete_longfield()
{
    do_query(DBNAME, "select count(*) from KFATNPATCL");
	do_dml_query(DBNAME, "delete from KFATNPATCL");
    do_query(DBNAME, "select count(*) from KFATNPATCL");

	return;
}

void test_double_connect()
{
	iSQL		isql;
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			i;
	char		query[512];
	int			num_fields;
	int num;
	unsigned long int *lengths;

	Drop_DB(DBNAME);

	if (createdb(DBNAME) < 0)
	{
		__SYSLOG("test_db_path: first createdb fail\n");
		return;
	}

	/* table 생성 */
	if (create_table(DBNAME) < 0)
	{
		__SYSLOG("test_db_path: first create table fail\n");
		return;
	}

	if (iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin") == NULL)
	{
		__SYSLOG("test_double_connect: connection fail\n");
		return;
	}

	sprintf(query, "select * from sysfields");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}

	if (iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin") == NULL)
	{
		__SYSLOG("test_double_connect: 2nd connection fail\n");
		return;
	}

	res = iSQL_use_result(&isql);
	if (res == NULL)
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
	else
	{
		if (iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin") == NULL)
		{
			__SYSLOG("test_double_connect: 2nd connection fail\n");
			return;
		}
		__SYSLOG("sysfields test!!!\n");
		num = iSQL_num_rows(res);
		__SYSLOG("iSQL_num_rows(): %d\n", num);

		num_fields = iSQL_num_fields(res);
		for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
		{
			lengths = iSQL_fetch_lengths(res);

			for (int j=0; j<num_fields; j++)
				__SYSLOG("%s,", row[j]);
			__SYSLOG("\n");
		}

		__SYSLOG("count = %d\n", i);

		iSQL_free_result(res);
	}

	iSQL_commit(&isql);

	if (iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin") == NULL)
	{
		__SYSLOG("test_double_connect: 2nd connection fail\n");
		return;
	}

	iSQL_disconnect(&isql);
}

void test_sysfields()
{
	iSQL		isql;
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			i;
	char		query[512];
	int			num_fields;
	int num;
	unsigned long int *lengths;

	Drop_DB(DBNAME);

	if (createdb(DBNAME) < 0)
	{
		__SYSLOG("test_db_path: first createdb fail\n");
		return;
	}

	/* table 생성 */
	if (create_table(DBNAME) < 0)
	{
		__SYSLOG("test_db_path: first create table fail\n");
		return;
	}

	iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin");

	sprintf(query, "select * from sysfields");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}

	res = iSQL_use_result(&isql);
	if (res == NULL)
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
	else
	{
		__SYSLOG("sysfields test!!!\n");
		num = iSQL_num_rows(res);
		__SYSLOG("iSQL_num_rows(): %d\n", num);

		num_fields = iSQL_num_fields(res);
		for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
		{
			lengths = iSQL_fetch_lengths(res);

			for (int j=0; j<num_fields; j++)
				__SYSLOG("%s,", row[j]);
			__SYSLOG("\n");
		}

		__SYSLOG("count = %d\n", i);

		iSQL_free_result(res);
	}

	iSQL_commit(&isql);

	iSQL_disconnect(&isql);
}

void do_query(char *dbname, char *query)
{
	iSQL		isql;
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			i;
	int			num_fields;
	int num;
	unsigned long int *lengths;
	SYSTEMTIME st, et;
	int gap;

	__SYSLOG("--------------------------------------------------------------------\n");

	GetSystemTime(&st);

	if (iSQL_connect(&isql, NULL, dbname, "aladdin", "aladdin") == NULL)
	{
		__SYSLOG("iSQL_connect(): %s\n", iSQL_error(&isql));
		return;
	}

	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}

	__SYSLOG("%s\n", query);
	res = iSQL_use_result(&isql);
	if (res == NULL)
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
	else
	{
		num = iSQL_num_rows(res);

		num_fields = iSQL_num_fields(res);
		for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
		{
			lengths = iSQL_fetch_lengths(res);

			__SYSLOG("<%d> ", i);

			for (int j=0; j<num_fields; j++)
				__SYSLOG("%s,", row[j]);
			__SYSLOG("\n");
		}

		__SYSLOG("count = %d\n", i);

		iSQL_free_result(res);
	}

	iSQL_commit(&isql);

    iSQL_flush_buffer();

	iSQL_disconnect(&isql);

	GetSystemTime(&et);

	gap = (et.wMinute - st.wMinute);
	gap *= 60;
	gap += (et.wSecond - st.wSecond);

	__SYSLOG(">>>>> %d\n", gap);
	__SYSLOG("--------------------------------------------------------------------\n");
}


int do_query2_msgdrafttable(iSQL *isql, char *query)
{
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			i;
	int			num_fields;
	int num;
	unsigned long int *lengths;
	int ret;
    iSQL_FIELD* field;

	if (iSQL_query(isql, query))
	{
		ret = iSQL_errno(isql);

		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(isql));

		iSQL_rollback(isql);

		return ret;
	}

	//res = iSQL_use_result(isql);
	res = iSQL_store_result(isql);
	if (res == NULL)
	{
		ret = iSQL_errno(isql);
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(isql));
		iSQL_rollback(isql);

		return ret;
	}
	else
	{
   		num_fields = iSQL_num_fields(res);

        for (i = 0; i < num_fields != NULL; i++)
		{
             field = iSQL_fetch_field(res);
			 __SYSLOG("<%d> %s\n", i, field->name);
		}
		__SYSLOG("\n");

        field = iSQL_fetch_fields(res);
		for (i = 0; i < num_fields != NULL; i++)
		{
			 __SYSLOG("<%d> %s\n", i, field[i].name);
		}
		__SYSLOG("\n");

		num = iSQL_num_rows(res);

		for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
		{
			lengths = iSQL_fetch_lengths(res);

			__SYSLOG("<%d> ", i);

            if ((unsigned long)row[9] % 2 ||
                (unsigned long)row[10] % 2 ||
                (unsigned long)row[11] % 2)
                __SYSLOG("NVARCHAR not aligned\n");

			for (int j=0; j<num_fields; j++)
				__SYSLOG("%s,", row[j]);
			__SYSLOG("\n");
		}

		__SYSLOG("count = %d\n", i);

		iSQL_free_result(res);
	}

	iSQL_commit(isql);

	return i;
}

int do_query2(iSQL *isql, char *query)
{
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			i;
	int			num_fields;
	int num;
	unsigned long int *lengths;
	int ret;
    iSQL_FIELD* field;

	if (iSQL_query(isql, query))
	{
		ret = iSQL_errno(isql);

		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(isql));

		iSQL_rollback(isql);

		return ret;
	}

	//res = iSQL_use_result(isql);
	res = iSQL_store_result(isql);
	if (res == NULL)
	{
		ret = iSQL_errno(isql);
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(isql));
		iSQL_rollback(isql);

		return ret;
	}
	else
	{
   		num_fields = iSQL_num_fields(res);
#if 0
        for (i = 0; i < num_fields != NULL; i++)
		{
             field = iSQL_fetch_field(res);
			 __SYSLOG("<%d> %s\n", i, field->name);
		}
		__SYSLOG("\n");
#endif
        field = iSQL_fetch_fields(res);
		for (i = 0; i < num_fields != NULL; i++)
		{
			 __SYSLOG("<%d> %s\n", i, field[i].name);
		}
		__SYSLOG("\n");

		num = iSQL_num_rows(res);

		for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
		{
			lengths = iSQL_fetch_lengths(res);

			__SYSLOG("<%d> ", i);

			for (int j=0; j<num_fields; j++)
				__SYSLOG("%s,", row[j]);
			__SYSLOG("\n");
		}

		__SYSLOG("count = %d\n", i);

		iSQL_free_result(res);
	}

	iSQL_commit(isql);

	return i;
}

int do_query2_nocommit(iSQL *isql, char *query)
{
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			i;
	int			num_fields;
	int num;
	unsigned long int *lengths;
	int ret;

	if (iSQL_query(isql, query))
	{
		ret = iSQL_errno(isql);

		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(isql));

		//iSQL_rollback(isql);

		return ret;
	}

	//res = iSQL_use_result(isql);
	res = iSQL_store_result(isql);
	if (res == NULL)
	{
		ret = iSQL_errno(isql);
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(isql));
		//iSQL_rollback(isql);

		return ret;
	}
	else
	{
		num = iSQL_num_rows(res);

		num_fields = iSQL_num_fields(res);
		for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
		{
			lengths = iSQL_fetch_lengths(res);

			__SYSLOG("<%d> ", i);

			for (int j=0; j<num_fields; j++)
				__SYSLOG("%s,", row[j]);
			__SYSLOG("\n");
		}

		__SYSLOG("count = %d\n", i);

		iSQL_free_result(res);
	}

	//iSQL_commit(isql);

	return i;
}

int do_query2_noprint(iSQL *isql, char *query)
{
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			i;
	int			num_fields;
	int num;
	//unsigned long int *lengths;
	int ret;

	if (iSQL_query(isql, query))
	{
		ret = iSQL_errno(isql);

		iSQL_rollback(isql);

		return ret;
	}

	//res = iSQL_use_result(isql);
	res = iSQL_store_result(isql);
	if (res == NULL)
	{
		ret = iSQL_errno(isql);

		iSQL_rollback(isql);

		return ret;
	}
	else
	{
		num = iSQL_num_rows(res);

		num_fields = iSQL_num_fields(res);
		for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
		{
			;
		}

		iSQL_free_result(res);
	}

	iSQL_commit(isql);

	return i;
}

void do_dml_query(char *dbname, char *query)
{
	iSQL		isql;
	SYSTEMTIME st, et;
	int gap;

	__SYSLOG("--------------------------------------------------------------------\n");

	GetSystemTime(&st);

	if (iSQL_connect(&isql, NULL, dbname, "aladdin", "aladdin") == NULL)
	{
		__SYSLOG("iSQL_connect(): %s\n", iSQL_error(&isql));
		return;
	}

	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_query(): %s\n", iSQL_error(&isql));
		iSQL_rollback(&isql);
	}
	else
	{
		__SYSLOG("affected rows: %d\n", iSQL_affected_rows(&isql));
		iSQL_commit(&isql);
	}

	iSQL_disconnect(&isql);

	GetSystemTime(&et);

	gap = (et.wMinute - st.wMinute);
	gap *= 60;
	gap += (et.wSecond - st.wSecond);

	__SYSLOG(">>>>> %d\n", gap);
	__SYSLOG("--------------------------------------------------------------------\n");
}

int do_dml_query2(iSQL *isql, char *query)
{
	int ret = 0;

	if (iSQL_query(isql, query))
	{
		ret = iSQL_errno(isql);
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_query(): %s\n", iSQL_error(isql));
		iSQL_rollback(isql);
	}
	else
	{
		//__SYSLOG("affected rows: %d\n", iSQL_affected_rows(isql));
		iSQL_commit(isql);
	}

	return ret;
}

int do_dml_query2_nocommit(iSQL *isql, char *query)
{
	int ret = 0;

	if (iSQL_query(isql, query))
	{
		ret = iSQL_errno(isql);
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_query(): %s\n", iSQL_error(isql));
		//iSQL_rollback(isql);
	}
	else
	{
		//__SYSLOG("affected rows: %d\n", iSQL_affected_rows(isql));
		//iSQL_commit(isql);
	}

	return ret;
}

void test_lower_names()
{
	iSQL		isql;
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			i;
	char		query[512];
	int			num_fields;
	int num;
	unsigned long int *lengths;

	Drop_DB(DBNAME);

	if (createdb(DBNAME) < 0)
	{
		__SYSLOG("test_db_path: first createdb fail\n");
		return;
	}

	/* table 생성 */
	if (create_table(DBNAME) < 0)
	{
		__SYSLOG("test_db_path: first create table fail\n");
		return;
	}

	iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin");

	__SYSLOG(">>> systables\n");
	sprintf(query, "select * from systables");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}

	res = iSQL_use_result(&isql);
	if (res == NULL)
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
	else
	{
		__SYSLOG("sysfields test!!!\n");
		num = iSQL_num_rows(res);
		__SYSLOG("iSQL_num_rows(): %d\n", num);

		num_fields = iSQL_num_fields(res);
		for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
		{
			lengths = iSQL_fetch_lengths(res);

			for (int j=0; j<num_fields; j++)
				__SYSLOG("%s,", row[j]);
			__SYSLOG("\n");
		}

		__SYSLOG("count = %d\n", i);

		iSQL_free_result(res);
	}

	iSQL_commit(&isql);

	__SYSLOG(">>> sysfields\n");
	sprintf(query, "select * from sysfields");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}

	res = iSQL_use_result(&isql);
	if (res == NULL)
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
	else
	{
		__SYSLOG("sysfields test!!!\n");
		num = iSQL_num_rows(res);
		__SYSLOG("iSQL_num_rows(): %d\n", num);

		num_fields = iSQL_num_fields(res);
		for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
		{
			lengths = iSQL_fetch_lengths(res);

			for (int j=0; j<num_fields; j++)
				__SYSLOG("%s,", row[j]);
			__SYSLOG("\n");
		}

		__SYSLOG("count = %d\n", i);

		iSQL_free_result(res);
	}

	iSQL_commit(&isql);

	__SYSLOG(">>> sysindexes\n");
	sprintf(query, "select * from sysindexes");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}

	res = iSQL_use_result(&isql);
	if (res == NULL)
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
	else
	{
		__SYSLOG("sysfields test!!!\n");
		num = iSQL_num_rows(res);
		__SYSLOG("iSQL_num_rows(): %d\n", num);

		num_fields = iSQL_num_fields(res);
		for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
		{
			lengths = iSQL_fetch_lengths(res);

			for (int j=0; j<num_fields; j++)
				__SYSLOG("%s,", row[j]);
			__SYSLOG("\n");
		}

		__SYSLOG("count = %d\n", i);

		iSQL_free_result(res);
	}

	iSQL_commit(&isql);

	iSQL_disconnect(&isql);
}

void test_isql_connect()
{
	iSQL		isql;
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			i, j;
	char		query[512];
	int			num_fields;
	int num;
	unsigned long int *lengths;

	Drop_DB(DBNAME);

	if (createdb(DBNAME) < 0)
	{
		__SYSLOG("test_db_path: first createdb fail\n");
		return;
	}

	for (j=0; j < 100; j++)
	{
		if (iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin") == NULL)
			return;

		__SYSLOG(">>> systables\n");
		sprintf(query, "select * from systables");
		if (iSQL_query(&isql, query))
		{
			__SYSLOG("Error: %s\n", query);
			__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));

			iSQL_disconnect(&isql);

			return;
		}

		res = iSQL_use_result(&isql);
		if (res == NULL)
			__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
		else
		{
			__SYSLOG("sysfields test!!!\n");
			num = iSQL_num_rows(res);
			__SYSLOG("iSQL_num_rows(): %d\n", num);

			num_fields = iSQL_num_fields(res);
			for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
			{
				lengths = iSQL_fetch_lengths(res);

				for (int j=0; j<num_fields; j++)
					__SYSLOG("%s,", row[j]);
				__SYSLOG("\n");
			}

			__SYSLOG("count = %d\n", i);

			iSQL_free_result(res);
		}

		iSQL_commit(&isql);

		iSQL_disconnect(&isql);
	}

	return;
}

void test_db_path()
{
	/* 처음 새로 만들기 */
	Drop_DB(DBNAME);

	if (createdb(DBNAME) < 0)
	{
		__SYSLOG("test_db_path: first createdb fail\n");
		return;
	}

	/* table 생성 */
	if (create_table(DBNAME) < 0)
	{
		__SYSLOG("test_db_path: first create table fail\n");
		return;
	}

	/* 다른 DB 만들기 */
	if (Drop_DB(DBNAME2) == FALSE)
	{
		__SYSLOG("test_db_path: Drop DB fail DBNAME2\n");
		return;
	}

	/* 새로 db 만들기 */
	if (createdb(DBNAME2) < 0)
	{
		__SYSLOG("test_db_path: second createdb fail\n");
		return;
	}

	/* table 생성되면 OK, 안되면 FAIL */
	if (create_table(DBNAME2) < 0)
	{
		__SYSLOG(">>>>> test_db_path: create table in db2 FAIL\n");
	}
	else
		__SYSLOG(">>>>> test_db_path: create table in db2 OK\n");

	/* table 생성되면 OK, 안되면 FAIL */
	if (create_table(DBNAME) < 0)
	{
		__SYSLOG(">>>>> test_db_path: create table fail in db OK\n");
	}
	else
		__SYSLOG(">>>>> test_db_path: create table success in db FAIL\n");

	return;
}

void test_dropdb_createdb()
{
	int i = 0;

	Drop_DB(DBNAME);

	if (createdb(DBNAME) < 0)
	{
		__SYSLOG("test_dropdb_createdb: createdb fail\n");
		return;
	}

	for (i=0; i<10; i++)
	{
		if (Drop_DB(DBNAME) == FALSE)
		{
			__SYSLOG("test_dropdb_createdb: %d Drop DB fail\n", i);
			return;
		}

		if (createdb(DBNAME) < 0)
		{
			__SYSLOG("test_remove_db: %d first createdb fail\n", i);
			return;
		}
	}

	return;
}

void test_remove_db()
{
	int i;

	for (i=0; i<10; i++)
	{
		/* 처음 새로 만들기 */
		Drop_DB(DBNAME);

		if (createdb(DBNAME) < 0)
		{
			__SYSLOG("test_remove_db: %d first createdb fail\n", i);
			return;
		}

		/* db 지우기 */
		if (Drop_DB(DBNAME) == FALSE)
		{
			__SYSLOG("test_remove_db: %d second Drop DB fail\n", i);
			return;
		}

		if (createdb(DBNAME) < 0)
		{
			__SYSLOG("test_remove_db: %d first createdb fail\n", i);
			return;
		}

		/* table 생성 */
		if (create_table(DBNAME) < 0)
		{
			__SYSLOG("test_remove_db: %d first create table fail\n", i);
			return;
		}

		/* db 지우기 */
		if (Drop_DB(DBNAME) == FALSE)
		{
			__SYSLOG("test_remove_db: %d second Drop DB fail\n", i);
			return;
		}

		/* 새로 db 만들기 */
		if (createdb(DBNAME) < 0)
		{
			__SYSLOG("test_remove_db: %d second createdb fail\n", i);
			return;
		}

		/* table 생성되면 OK, 안되면 FAIL */
		if (create_table(DBNAME) < 0)
		{
			__SYSLOG(">>>>> test_remove_db: FAIL\n");
		}
		else
			__SYSLOG(">>>>> test_remove_db: OK\n");

		/* db 지우기 */
		if (Drop_DB(DBNAME) == FALSE)
		{
			__SYSLOG("test_remove_db: %d 3rd Drop DB fail\n", i);
			return;
		}

		/* 새로 db 만들기 */
		if (createdb(DBNAME) < 0)
		{
			__SYSLOG("test_remove_db: %d 3rd createdb fail\n", i);
			return;
		}

		/* table 생성되면 OK, 안되면 FAIL */
		if (create_table(DBNAME) < 0)
		{
			__SYSLOG(">>>>> test_remove_db: FAIL\n");
		}
		else
			__SYSLOG(">>>>> test_remove_db: OK\n");
	}

	return;
}

void test_orderby()
{
	iSQL		isql;
	iSQL_RES	*res;
	iSQL_ROW	row;
	int			i;
	char		query[512];
	int			num_fields;
	unsigned long int *lengths;
	int count = 0;
	SYSTEMTIME st, et;
	int gap;

	GetSystemTime(&st);

	iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin");

	sprintf(query, "select * from sysfields order by fieldid");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}
	res = iSQL_use_result(&isql);
	if (res == NULL)
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
	else
	{
		num_fields = iSQL_num_fields(res);
		count = 0;
		for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
		{
			lengths = iSQL_fetch_lengths(res);

			for (int j=0; j<num_fields; j++)
				__SYSLOG("%s,", row[j]);
			__SYSLOG("\n");
			count++;
		}

		__SYSLOG("count = %d\n", i);

		iSQL_free_result(res);
	}

	iSQL_commit(&isql);

	GetSystemTime(&et);

	gap = (et.wMinute - st.wMinute);
	gap *= 60;
	gap += (et.wSecond - st.wSecond);

	__SYSLOG("test orderby 1: %d\n", gap);

	sprintf(query, "select a.fieldname, a.position, a.offset \
		from sysfields a, sysfields b \
		where a.fieldid = b.fieldid \
		order by a.offset desc");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}
	res = iSQL_use_result(&isql);
	if (res == NULL)
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
	else
	{
		num_fields = iSQL_num_fields(res);
		count = 0;
		for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
		{
			lengths = iSQL_fetch_lengths(res);

			for (int j=0; j<num_fields; j++)
				__SYSLOG("%s,", row[j]);
			__SYSLOG("\n");
			count++;
		}

		__SYSLOG("count = %d\n", i);

		iSQL_free_result(res);
	}

	iSQL_commit(&isql);

	GetSystemTime(&et);

	gap = (et.wMinute - st.wMinute);
	gap *= 60;
	gap += (et.wSecond - st.wSecond);

	__SYSLOG("test orderby 2: %d\n", gap);

	GetSystemTime(&st);

	sprintf(query, "select a.number, a.name, a.dt from teladdr a, teladdr b \
		where a.number = b.number and a.number %% 100 = 0 \
		order by a.bb desc");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}
	res = iSQL_use_result(&isql);
	if (res == NULL)
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
	else
	{
		num_fields = iSQL_num_fields(res);
		count = 0;
		for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
		{
			lengths = iSQL_fetch_lengths(res);

			for (int j=0; j<num_fields; j++)
				__SYSLOG("%s,", row[j]);
			__SYSLOG("\n");
			count++;
		}

		__SYSLOG("count = %d\n", i);

		iSQL_free_result(res);
	}

	iSQL_commit(&isql);

	GetSystemTime(&et);

	gap = (et.wMinute - st.wMinute);
	gap *= 60;
	gap += (et.wSecond - st.wSecond);

	__SYSLOG("test orderby 3: %d\n", gap);

	GetSystemTime(&st);

	sprintf(query, "select count(*) from teladdr a, teladdr b \
		where a.number = b.number and a.number %% 100 = 0");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}
	res = iSQL_use_result(&isql);
	if (res == NULL)
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
	else
	{
		num_fields = iSQL_num_fields(res);
		count = 0;
		for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
		{
			lengths = iSQL_fetch_lengths(res);

			for (int j=0; j<num_fields; j++)
				__SYSLOG("%s,", row[j]);
			__SYSLOG("\n");
			count++;
		}

		__SYSLOG("count = %d\n", i);

		iSQL_free_result(res);
	}

	iSQL_commit(&isql);

	GetSystemTime(&et);

	gap = (et.wMinute - st.wMinute);
	gap *= 60;
	gap += (et.wSecond - st.wSecond);

	__SYSLOG("test orderby 4: %d\n", gap);

	iSQL_disconnect(&isql);
}

void test()
{
	iSQL		isql;
	iSQL_RES	*res;
	iSQL_ROW	row;
	char		query[512];
	int			num_fields, fExist;

	Drop_DB(DBNAME);

	if (createdb(DBNAME) < 0)
	{
		__SYSLOG("test: createdb fail\n");
		return;
	}

	recreate_table(DBNAME);

	iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin");

	sprintf(query, "select * from systables where tablename='teladdr'");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("select query %s Error\n", query);
		return;
	}

	res = iSQL_use_result(&isql);
	if (res == NULL)
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
	else
	{
		num_fields = iSQL_num_fields(res);

		if (iSQL_fetch_row(res) == NULL)
			fExist = FALSE;
		else
			fExist = TRUE;

		iSQL_eof(res);

		iSQL_free_result(res);
	}

	iSQL_commit(&isql);

	if (fExist == FALSE) // table 없음. 추가!
	{ // API로 된 것을 추후에 SQL로 변경해야함.
		sprintf(query, "%s", "create table teladdr (number int primary key, name char(20), tel char(20), addr varchar(50), memo char(50))");

		if (iSQL_query(&isql, query))
		{
			__SYSLOG("ERROR: %s\n", query);

			iSQL_rollback(&isql);

			__SYSLOG("table \'teladdr\' 만들기 실패...");

			return;
		}
		else
		{
			iSQL_commit(&isql);
		}

		sprintf(query, "%s", "create index ix_teladdr on teladdr(name)");

		if (iSQL_query(&isql, query))
		{
			__SYSLOG("ERROR: %s\n", query);

			iSQL_rollback(&isql);

			__SYSLOG("index \'ix_teladdr\' 만들기 실패...");

			return;
		}
		else
		{
			iSQL_commit(&isql);
		}

		teladdr_num = 0;
	}
	else
	{
		__SYSLOG("Relation teladdr already existed!\n");

		// 제일 큰 번호 찾기

		sprintf(query, "select max(number) from teladdr");
		if (iSQL_query(&isql, query))
		{
			__SYSLOG("Error: %s\n", query);

			__SYSLOG("오류발생 1");

			teladdr_num = 0;

			iSQL_disconnect(&isql);

			return;
		}

		res = iSQL_use_result(&isql);
		if (res == NULL)
			__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
		else
		{
			row = iSQL_fetch_row(res);
			if (row != NULL)
				teladdr_num = atoi(row[0]) + 1;
			else
				teladdr_num = 0;
/*
			num_fields = iSQL_num_fields(res);
			for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
			{
				lengths = iSQL_fetch_lengths(res);

				num = atoi(row[0]);
				if (num > teladdr_num)
					teladdr_num = num;

//				__SYSLOG("num = %d, teladdr_num=%d\n", num, teladdr_num);
			}

			__SYSLOG("i = %d\n", i);
*/

			__SYSLOG("teladdr_num=%d\n", teladdr_num);

			iSQL_free_result(res);
		}

		iSQL_commit(&isql);
	}

	iSQL_disconnect(&isql);

	{
		SYSTEMTIME st, et;
		int gap;

		recreate_table(DBNAME);

		GetSystemTime(&st);

		insert_test();

		GetSystemTime(&et);

		gap = (et.wMinute - st.wMinute);
		gap *= 60;
		gap += (et.wSecond - st.wSecond);

		__SYSLOG("insert sql time %d\n", gap);
		__SYSLOG("\t%d:%d - %d:%d\n", st.wMinute, st.wSecond,
				et.wMinute, et.wSecond);

#if 0
		GetSystemTime(&st);

		select_test();

		GetSystemTime(&et);

		gap = (et.wMinute - st.wMinute);
		gap *= 60;
		gap += (et.wSecond - st.wSecond);

		__SYSLOG("select time %d\n", gap);
		__SYSLOG("\t%d:%d - %d:%d\n", st.wMinute, st.wSecond,
				et.wMinute, et.wSecond);
#endif

#if 0
		delete_test();
#endif
	}

	return;  // return TRUE  unless you set the focus to a control
}

int recreate_table(char *dbname)
{
	iSQL		isql;
	char query[256];

	iSQL_connect(&isql, NULL, dbname, "aladdin", "aladdin");

	sprintf(query, "drop table teladdr");

	if (iSQL_query(&isql, query))
		;
	else
		iSQL_commit(&isql);

	sprintf(query, "%s",
		"create table teladdr (number int primary key, name char(20), tel char(20), \
		addr char(50), memo varchar(50), dt datetime, bb bigint)");

	if (iSQL_query(&isql, query))
	{
		__SYSLOG("ERROR: %s\n", iSQL_error(&isql));
		iSQL_rollback(&isql);
		iSQL_disconnect(&isql);

		return -1;
	}
	iSQL_commit(&isql);

	sprintf(query, "%s", "create index ix_teladdr on teladdr(name)");

	if (iSQL_query(&isql, query))
	{
		__SYSLOG("ERROR: %s\n", query);
		__SYSLOG("ERROR: %s\n", iSQL_error(&isql));
		iSQL_rollback(&isql);
		iSQL_disconnect(&isql);

		return -1;
	}
	iSQL_commit(&isql);

	iSQL_disconnect(&isql);

	teladdr_num = 0;

	return 0;
}

int create_table(char *dbname)
{
	iSQL		isql;
	char query[256];

	if (iSQL_connect(&isql, NULL, dbname, "aladdin", "aladdin") == NULL)
	{
		__SYSLOG("%d %s\n", iSQL_errno(&isql), iSQL_error(&isql));
		return -1;
	}

	sprintf(query, "%s",
		"CREATE TABLE TELADDR (NUMBER INT PRIMARY KEY, NAME CHAR(20), TEL CHAR(20), \
		ADDR CHAR(50), MEMO VARCHAR(50), DT DATETIME, BB BIGINT)");

	if (iSQL_query(&isql, query))
	{
		__SYSLOG("ERROR: %s\n", iSQL_error(&isql));
		iSQL_rollback(&isql);
		iSQL_disconnect(&isql);

		return -1;
	}
	iSQL_commit(&isql);

	sprintf(query, "%s", "CREATE INDEX IX_TELADDR ON TELADDR(NAME)");

	if (iSQL_query(&isql, query))
	{
		__SYSLOG("ERROR: %s\n", query);
		__SYSLOG("ERROR: %s\n", iSQL_error(&isql));
		iSQL_rollback(&isql);
		iSQL_disconnect(&isql);

		return -1;
	}
	iSQL_commit(&isql);

	iSQL_disconnect(&isql);

	teladdr_num = 0;

	return 0;
}

void insert_test()
{
	iSQL		isql;
	int i;
	char query[256];

	iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin");

	for (i=0; i < NUM_INSERT; i++)
	{
		sprintf(query, "insert into teladdr values('%d', 'name%d', 'tel%d', 'addr%d', 'memo%d', sysdate, '%d')",
			teladdr_num, teladdr_num, teladdr_num, teladdr_num, teladdr_num, teladdr_num);

		if (iSQL_query(&isql, query))
		{
			__SYSLOG("ERROR: %s\n", iSQL_error(&isql));
			__SYSLOG("query: %s\n", query);
			iSQL_rollback(&isql);
			return;
		}

		//iSQL_commit(&isql);

		teladdr_num++;

		if (!(i % 5000))
		{
			iSQL_commit(&isql);
			__SYSLOG("%d inserted\n", i+1);
		}
	}

	iSQL_disconnect(&isql);
}

typedef struct teladdr_struct
{
	int number;
	char name[20];
	char tel[20];
	char addr[50];
	char memo[50];
	char dt[32];
	char bb[50];
	char dummy[20]; /* 16 + (필드 수/8 +1) */
} teladdr_t;

void insert_test2()
{
	iSQL		isql;
	int i;
	char query[256];

	iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin");

	for (i=0; i < NUM_INSERT; i++)
	{
		sprintf(query, "insert into teladdr values(%d, 'name%d', 'tel%d', 'addr%d', 'memo%d')",
			teladdr_num, teladdr_num, teladdr_num, teladdr_num, teladdr_num);

		if (iSQL_query(&isql, query))
		{
			__SYSLOG("ERROR: %s\n", iSQL_error(&isql));
			iSQL_rollback(&isql);
			return;
		}

		//iSQL_commit(&isql);

		teladdr_num++;

		if (!(i % 5000))
		{
			iSQL_commit(&isql);
			__SYSLOG("%d inserted\n", i+1);
		}
	}

	iSQL_disconnect(&isql);
}

void select_test()
{
	iSQL		isql;
	iSQL_RES	*res;
	char		query[512];
	int num;

	iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin");

	// 제일 큰 번호 찾기

	sprintf(query, "select * from teladdr where number > 20");
	if (iSQL_query(&isql, query))
	{
		__SYSLOG("Error: %s\n", query);
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));

		iSQL_disconnect(&isql);

		return;
	}

//	res = iSQL_use_result(&isql);
	res = iSQL_store_result(&isql);

	if (res == NULL)
		__SYSLOG("iSQL_use_result(): %s\n", iSQL_error(&isql));
	else
	{
		num = iSQL_num_rows(res);
		__SYSLOG("iSQL_num_rows(): %d\n", num);
/*
		num_fields = iSQL_num_fields(res);
		for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
		{
			lengths = iSQL_fetch_lengths(res);

			num = atoi(row[0]);

//			__SYSLOG("num = %d\n", num);
		}

		__SYSLOG("count = %d\n", i);
*/
		iSQL_free_result(res);
	}

	iSQL_commit(&isql);

	iSQL_disconnect(&isql);
}

void delete_test()
{
	iSQL		isql;
	int i;
	char query[256];

	iSQL_connect(&isql, NULL, DBNAME, "aladdin", "aladdin");

	for (i=0; i < 100; i++)
	{
		sprintf(query, "delete from teladdr where name = 'name%d'", i);

		if (iSQL_query(&isql, query))
		{
			__SYSLOG("ERROR: %s\n", iSQL_error(&isql));
			iSQL_rollback(&isql);
			break;
		}

		iSQL_commit(&isql);

		if (!(i % 10))
		{
			__SYSLOG("%d deleted\n", i+1);
		}
	}

	iSQL_disconnect(&isql);
}

bool execQuery( LPCTSTR pszFmt, ... )
{
	if( !m_fConnected ) {
		return false;
	}

	wchar_t szBuffer[ 1024 ];
	char szQuery[ 1024 + 1 ];
	int i;
	bool ret;

	va_list start;
	va_start( start, pszFmt );
	i = vswprintf( szBuffer, pszFmt, start );
	va_end( start );
	wcstombs( szQuery, szBuffer, sizeof(szQuery) - 1 );
	__SYSLOG("query:%s\n", szQuery);
	m_nLastErrorCode = iSQL_query((iSQL *)m_pData, szQuery);
	ret = (m_nLastErrorCode == 0) ? true : false;
	return ret;
}

bool updateRec( LPCTSTR pszKey, LPCTSTR pszValue )
{
	bool fOk = execQuery( L"UPDATE tb_test set value='%s' WHERE id='%s'", pszValue, pszKey );
	if( fOk ) {
		commit();
	}
	return fOk;
}

bool insertRec( LPCTSTR pszKey, LPCTSTR pszValue )
{
	bool fOk = execQuery( L"INSERT INTO tb_test VALUES ('%s', '%s')", pszKey, pszValue );
	if( fOk ) {
		commit();
	}
	return fOk;
}

bool deleteRec( LPCTSTR pszKey )
{
	bool fResult = execQuery( L"DELETE from tb_test WHERE id='%s'", pszKey );
	if( fResult ) {
		commit();
	}
	return fResult;
}

void commit()
{
	if( m_fConnected )
		iSQL_commit( (iSQL *)m_pData );
}

#include "mobile_test.h"
#include "mobile_define.h"

#define  RECORDSIZE 4000

iSQL	isql;
iSQL_RES	*res;
iSQL_ROW	row;
BOOL    bFileOpen = FALSE;

FILE* SIMUL_LOG_Init(char* result_file)
{
	FILE *syslog_fp = NULL;

	if(bFileOpen == FALSE)
	{
		syslog_fp = fopen(result_file, "w");

		if(syslog_fp == NULL)
			return NULL;
		bFileOpen = TRUE;
		return syslog_fp;
	}
	syslog_fp = fopen(result_file, "a+");
	if(syslog_fp == NULL)
		return NULL;
	return syslog_fp;
}

void SIMUL_LOG(char* result_file, char *format, ...)
{
	va_list ap;
	FILE *syslog_fp;

	syslog_fp = SIMUL_LOG_Init(result_file);
	if (syslog_fp == NULL)
		return;

	va_start(ap, format);

	vfprintf(syslog_fp, format, ap);

	va_end(ap);

	if (strlen(result_file) != 0)
		fclose(syslog_fp);
}

static int MOCHA_LOG_Init(void);
__DECL_PREFIX void MOCHA_LOG(char *format, ...);

void Start_Test(char* cfg_path, char* out_path, char* db_path)
{	
	int nSize;
	nSize = FileSize(cfg_path);
	if(nSize > 0)
		SetCFGnStart(cfg_path, out_path, db_path, nSize);
}

// File Size return
int FileSize(char* path)
{
	FILE* fp_Size;
	int nSize = 0;
	if((fp_Size = fopen(path, "rb")) == NULL)
	{
		return -1;
	}
	/*
	while (E_fgetc(fp_Size) != EOF)
	{
		nSize = nSize + 1;
	}
	*/
	fseek(fp_Size, 0, SEEK_END); 
	nSize = ftell(fp_Size);
	fclose(fp_Size);
	return nSize;
}

// Test.cfg read
int ReadTestCFG(char* cfg_path, char *param, int nSize)
{
	FILE* fp_Read;
	int  nCount = 0;
	char c;
	
	if((fp_Read = fopen(cfg_path, "rb" )) == NULL)
	{
		return -1;
	}
	while((c = fgetc(fp_Read)) != EOF)
	{
		param[nCount] = c;
		nCount = nCount + 1;
	}
	fclose(fp_Read);
	return nCount;
}

// aslite.cfg set
int SetasliteCFG(char* bufSize)
{
	long  nSize;
	int   i;
	char  c;
	FILE* fp_Set;
	
	if((fp_Set = fopen(E_aslitePath, "r+b" )) == NULL)
	{
		return -1;
	}
	fseek(fp_Set, 0, SEEK_END); 
	fseek(fp_Set, -1, SEEK_CUR);
	while(c = fgetc(fp_Set) != '=')
	{ 
		fseek(fp_Set, -2, SEEK_CUR); 
	}
	fseek(fp_Set, -1, SEEK_CUR); 
	fseek(fp_Set, 1, SEEK_CUR); 
	nSize = ftell(fp_Set);
	for(i = 0; i < 6; i = i + 1)
	{
		fputc(bufSize[i], fp_Set);
	}
	fclose(fp_Set);
	return 1;
}

// config set
int SetCFGnStart(char* cfg_path, char* out_path, char* db_path, int nsize)
{
	char* cfgBuffer;
	char tmp[6];
	char* result_file;
	int ncase;
	int nbufsize;
	int niteration;
	int idx = 0;
	int i, j, nStart = 0;
	long start, end;
	bool bfirst = true;

	cfgBuffer = (char*)malloc(nsize + 1);
	memset(cfgBuffer, 0x00, nsize + 1);
	
	if(nsize > 0)
	{
		ReadTestCFG(cfg_path, cfgBuffer, nsize);
	}
	else
	{
		free(cfgBuffer);
		cfgBuffer = NULL;
		return -1;
	}

	for(i = 0; i < nsize;)
	{
		if(cfgBuffer[i] == ',' && bfirst)
		{
			bfirst = false;
			memset(tmp, 0x00, 6);
			strncpy(tmp, cfgBuffer + nStart, i - nStart);
			ncase = atoi(tmp);
			nStart = i + 1;
		}
		else if(cfgBuffer[i] == ',')
		{
			bfirst = true;
			memset(tmp, 0x00, 6);
			strncpy(tmp, cfgBuffer + nStart, i - nStart);
			nbufsize = atoi(tmp);
			nStart = i + 1;
			SetasliteCFG(tmp);
		}
		else if(cfgBuffer[i] == ';')
		{
			idx = idx + 1;		
			memset(tmp, 0x00, 6);
			strncpy(tmp, cfgBuffer + nStart, i - nStart);
			niteration = atoi(tmp);
			//nsize = nsize - i;
			nStart = i + 1;
			if(ncase == 1)
			{
				result_file = (char*)malloc(strlen(out_path) + 7);
				sprintf(result_file, "%s_%d.dat", out_path, idx);
				
				/***********************----/----/----/----/----/----/----/----/----/*/
				SIMUL_LOG(result_file, "\n**************************************************\n");
				SIMUL_LOG(result_file, "TEST CASE 1\n");
				SIMUL_LOG(result_file, "BUFFER SIZE %d MB\n", nbufsize);
				SIMUL_LOG(result_file, "ITERATION   %d\n", niteration);
				SIMUL_LOG(result_file, "**************************************************\n");
				//DB Create
#if 1
				if(CreateDBMS(result_file, db_path) < 0)
				{
					free(result_file);
					result_file = NULL;
					return -1;
				}
#endif
				//DB Connect 
				start = GetTickCount();
				if (iSQL_connect(&isql, "127.0.0.1", db_path, "mmdb", "mmdb") == NULL)
				{
					SIMUL_LOG(result_file, "%s(%d) : %s\n", __FILE__, __LINE__, iSQL_error(&isql));
					SIMUL_LOG(result_file, "접속 실패\n");
					free(result_file);
					result_file = NULL;
					return -1;
				}
				end = GetTickCount();
				/***********************----/----/----/----/----/----/----/----/----/*/
				SIMUL_LOG(result_file, "DB Connect time                             : %ld\n", end - start);
				//Create Table
#if 1
				if(CreateTable(result_file) < 0)
				{
					free(result_file);
					result_file = NULL;
					return -1;
				}

				//Create index
				if(CreateIndex(result_file) < 0)
				{
					free(result_file);
					result_file = NULL;
					return -1;
				}
#endif
				// 반복구간 : insert, select, update, delete
				for(j = 0; j < niteration; j = j + 1)
				{
					SIMUL_LOG(result_file, "\n\n**************************************************\n");
					SIMUL_LOG(result_file, "COUNT   %d\n", j + 1);
					SIMUL_LOG(result_file, "**************************************************\n\n");

					if(CASE_ONE_TestStart(j, result_file, db_path, nbufsize) < 0)
						break;
				}
#if 1
				//Drop Index
				if(DropIndex(result_file) < 0)
				{
					free(result_file);
					result_file = NULL;
					return -1;
				}

				//Drop Table
				if(DropTable(result_file) < 0)
				{
					free(result_file);
					result_file = NULL;
					return -1;
				}

				//Drop DB
				if(DropDBMS(result_file, db_path) < 0)
				{
					free(result_file);
					result_file = NULL;
					return -1;
				}
#endif
				iSQL_disconnect(&isql);
				free(result_file);
				result_file = NULL;
			}
			else if(ncase == 2)
			{
			}
			else if(ncase == 3)
			{
			}
		}
		i = i + 1;
	}
	free(result_file);
	result_file = NULL;
	free(cfgBuffer);
	cfgBuffer = NULL;
	return 1;
}

int CASE_ONE_TestStart(int iterration, char* result_file, char* db_path, int nbufsize)
{
#if 1
	//Insert Record

	if(InsertRecord(result_file) < 0)
	{
		return -1;
	}

	if(SelectSingleTableWithPK(result_file) < 0)
	{
		return -1;
	}

	if(SelectSingleTableWithIndex(result_file) < 0)
	{
		return -1;
	}

	if(SelectSingleTableWithoutIndex(result_file) < 0)
	{
		return -1;
	}

	if(SelectSingleTableLikeWithPK(result_file) < 0)
	{
		return -1;
	}
#endif
	if(SelectJoin2WithIndex(result_file) < 0)
	{
		return -1;
	}

	if(SelectJoin2LiktWithIndex(result_file) < 0)
	{
		return -1;
	}

	if(SelectJoin3WithIndex(result_file) < 0)
	{
		return -1;
	}

	if(SelectJoin3LiktWithIndex(result_file) < 0)
	{
		return -1;
	}

	if(UpdateRecord(result_file) < 0)
	{
		return -1;
	}

	if(DeleteRecord(result_file) < 0)
	{
		return -1;
	}

	return 1;
}

// DBMS create
int CreateDBMS(char* result_file, char* db_path)
{
	long start, end;
	Drop_DB(db_path);
	start = GetTickCount();
	if (createdb(db_path) < 0 ) {
		SIMUL_LOG(result_file, "ERROR : %s DB 삭제 실패\n", db_path);
		return -1;
	}
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "DB Create Time                              : %ld\n", end - start);
	
	return 1;
}

// DBMS drop
int DropDBMS(char* result_file, char* db_path)
{
	long start, end;
	start = GetTickCount();
	if (Drop_DB(db_path) < 0 ) {
		SIMUL_LOG(result_file, "ERROR : %s DB 생성 실패\n", db_path);
		return -1;
	}
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "DB Drop Time                                : %ld\n", end - start);
	return 1;
}

// Table create
int CreateTable(char* result_file)
{
	long start, end;
	char* query;
	query = (char*)malloc(QUERY_SIZE);

	// without PK
	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "create table a(a_int int, ");
	sprintf(query, "%s a_smallint	smallint, ", query);
	sprintf(query, "%s a_tinyint 	tinyint,  ", query);
	sprintf(query, "%s a_bigint 	bigint,  ", query);
	sprintf(query, "%s a_float 		float,  ", query);
	sprintf(query, "%s a_double 	double,  ", query);
	sprintf(query, "%s a_real 		real, ", query);
	sprintf(query, "%s a_decimal 	decimal(10, 4), ", query);
	sprintf(query, "%s a_numeric	numeric(10, 4), ", query);
	sprintf(query, "%s a_char		char, ", query);
	sprintf(query, "%s a_char10		char(10), ", query);
	sprintf(query, "%s a_varchar20	varchar(20), ", query);
	sprintf(query, "%s a_datetime	datetime, ", query);
	sprintf(query, "%s a_timestamp	timestamp) ", query);

	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : a table create %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "without PK Table Create Time                : %ld\n", end - start);

	// with PK
	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "create table b(b_int int, ");
	sprintf(query, "%s b_smallint	smallint, ", query);
	sprintf(query, "%s b_tinyint 	tinyint,  ", query);
	sprintf(query, "%s b_bigint 	bigint,  ", query);
	sprintf(query, "%s b_float 		float,  ", query);
	sprintf(query, "%s b_double 	double,  ", query);
	sprintf(query, "%s b_real 		real, ", query);
	sprintf(query, "%s b_decimal 	decimal(10, 4), ", query);
	sprintf(query, "%s b_numeric	numeric(10, 4), ", query);
	sprintf(query, "%s b_char		char, ", query);
	sprintf(query, "%s b_char10		char(10), ", query);
	sprintf(query, "%s b_varchar20	varchar(20), ", query);
	sprintf(query, "%s b_datetime	datetime, ", query);
	sprintf(query, "%s b_timestamp	timestamp, ", query);
	sprintf(query, "%s primary key(b_int)) ", query);

	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : b table create %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "create table c(c_int int, ");
	sprintf(query, "%s c_smallint	smallint, ", query);
	sprintf(query, "%s c_tinyint 	tinyint,  ", query);
	sprintf(query, "%s c_bigint 	bigint,  ", query);
	sprintf(query, "%s c_float 		float,  ", query);
	sprintf(query, "%s c_double 	double,  ", query);
	sprintf(query, "%s c_real 		real, ", query);
	sprintf(query, "%s c_decimal 	decimal(10, 4), ", query);
	sprintf(query, "%s c_numeric	numeric(10, 4), ", query);
	sprintf(query, "%s c_char		char, ", query);
	sprintf(query, "%s c_char10		char(10), ", query);
	sprintf(query, "%s c_varchar20	varchar(20), ", query);
	sprintf(query, "%s c_datetime	datetime, ", query);
	sprintf(query, "%s c_timestamp	timestamp, ", query);
	sprintf(query, "%s primary key(c_int)) ", query);
	
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : c table create %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "with PK Table Create Time                   : %ld\n", end - start);

	
	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "create table d(d_int int, ");
	sprintf(query, "%s d_smallint	smallint, ", query);
	sprintf(query, "%s d_tinyint 	tinyint,  ", query);
	sprintf(query, "%s d_bigint 	bigint,  ", query);
	sprintf(query, "%s d_float 		float,  ", query);
	sprintf(query, "%s d_double 	double,  ", query);
	sprintf(query, "%s d_real 		real, ", query);
	sprintf(query, "%s d_decimal 	decimal(10, 4), ", query);
	sprintf(query, "%s d_numeric	numeric(10, 4), ", query);
	sprintf(query, "%s d_char		char, ", query);
	sprintf(query, "%s d_char10		char(10), ", query);
	sprintf(query, "%s d_varchar20	varchar(20), ", query);
	sprintf(query, "%s d_datetime	datetime, ", query);
	sprintf(query, "%s d_timestamp	timestamp, ", query);
	sprintf(query, "%s primary key(d_int)) ", query);
	
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : d table create %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "create table e(e_int int, ");
	sprintf(query, "%s e_smallint	smallint, ", query);
	sprintf(query, "%s e_tinyint 	tinyint,  ", query);
	sprintf(query, "%s e_bigint 	bigint,  ", query);
	sprintf(query, "%s e_float 		float,  ", query);
	sprintf(query, "%s e_double 	double,  ", query);
	sprintf(query, "%s e_real 		real, ", query);
	sprintf(query, "%s e_decimal 	decimal(10, 4), ", query);
	sprintf(query, "%s e_numeric	numeric(10, 4), ", query);
	sprintf(query, "%s e_char		char, ", query);
	sprintf(query, "%s e_char10		char(10), ", query);
	sprintf(query, "%s e_varchar20	varchar(20), ", query);
	sprintf(query, "%s e_datetime	datetime, ", query);
	sprintf(query, "%s e_timestamp	timestamp, ", query);
	sprintf(query, "%s primary key(e_int)) ", query);
	
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : e table create %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "create table f(f_int int, ");
	sprintf(query, "%s f_smallint	smallint, ", query);
	sprintf(query, "%s f_tinyint 	tinyint,  ", query);
	sprintf(query, "%s f_bigint 	bigint,  ", query);
	sprintf(query, "%s f_float 		float,  ", query);
	sprintf(query, "%s f_double 	double,  ", query);
	sprintf(query, "%s f_real 		real, ", query);
	sprintf(query, "%s f_decimal 	decimal(10, 4), ", query);
	sprintf(query, "%s f_numeric	numeric(10, 4), ", query);
	sprintf(query, "%s f_char		char, ", query);
	sprintf(query, "%s f_char10		char(10), ", query);
	sprintf(query, "%s f_varchar20	varchar(20), ", query);
	sprintf(query, "%s f_datetime	datetime, ", query);
	sprintf(query, "%s f_timestamp	timestamp, ", query);
	sprintf(query, "%s primary key(f_int)) ", query);
	
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : f table create %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);

	free(query);
	query = NULL;
	return 1;
}

// Table drop
int DropTable(char* result_file)
{
	long start, end;
	char* query;
	query = (char*)malloc(QUERY_SIZE);

	sprintf(query, "drop table a");
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : a table drop %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "without PK Table drop Time                  : %ld\n", end - start);

	sprintf(query, "drop table b");
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : b table drop %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);

	sprintf(query, "drop table c");
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : c table drop %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "with PK Table drop Time                     : %ld\n", end - start);

	sprintf(query, "drop table d");
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : d table drop %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);

	sprintf(query, "drop table e");
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : e table drop %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);

	sprintf(query, "drop table f");
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : f table drop %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);

	free(query);
	query = NULL;
	return 1;
}

// Index create
int CreateIndex(char* result_file)
{
	long start, end;
	char* query;
	query = (char*)malloc(QUERY_SIZE);

	sprintf(query, "create index idx_a1 on a(a_int)");
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : a table index create %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "without PK Table single index create Time   : %ld\n", end - start);
	
	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "create index idx_a2 on a(a_bigint, a_int, a_datetime)");
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : a table index create %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "without PK Table multi(3) index create Time : %ld\n", end - start);


	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "create index idx_b1 on b(b_varchar20)");
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : b table index create %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "with PK Table single index create Time      : %ld\n", end - start);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "create index idx_b2 on b(b_bigint, b_int, b_datetime)");
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : b table index create %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "with PK Table multi(3) index create Time    : %ld\n", end - start);

	sprintf(query, "create index idx_a3 on a(a_varchar20)");
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "create index idx_c1 on c(c_varchar20)");
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : c table index create %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "create index idx_d1 on d(d_varchar20)");
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : d table index create %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "create index idx_e1 on e(e_varchar20)");
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : e table index create %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "create index idx_f1 on f(f_varchar20)");
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : f table index create %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "create index idx_c2 on c(c_tinyint)");
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : c table index create %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "create index idx_d2 on d(d_tinyint)");
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : d table index create %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "create index idx_e2 on e(e_tinyint)");
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : e table index create %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "create index idx_f2 on f(f_tinyint)");
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : f table index create %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);
	free(query);
	query = NULL;
	return 1;
}

// Index drop
int DropIndex(char* result_file)
{
	long start, end;
	char* query;
	query = (char*)malloc(QUERY_SIZE);

	sprintf(query, "drop index idx_a1");
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "without PK Table single index drop Time     : %ld\n", end - start);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "drop index idx_a2");
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query,  iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "without PK Table multi(3) index drop Time   : %ld\n", end - start);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "drop index idx_b1");
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : b table index drop %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "with PK Table single index drop Time        : %ld\n", end - start);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "drop index idx_b2");
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : b table index drop %s \n", iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "with PK Table multi(3) index drop Time      : %ld\n", end - start);

	free(query);
	query = NULL;
	return 1;
}

// 데이터 insert, withCommit에 따라 commit의 간격 조절
int InsertRecord(char* result_file)
{
	long start, end;
	int  i;
	char* query;
	query = (char*)malloc(QUERY_SIZE);
	
	start = GetTickCount();
	for(i = 1; i <= RECORDSIZE; i = i + 1)
	{
		sprintf(query, "insert into a values(%d, %d, %d, %d, %d.0, %d.0, %d.0, %d.%d, %d.%d, 'c', '%02d0', '%02d0', SYSDATE, SYSDATE)", i*13, i%10, i%10, i*13, i*13, i*13, i*13, i*13, i*13, i*13, i*13, i%10, i%10);
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		iSQL_commit(&isql);
	}
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "%d record insert with   1 commit        : %ld\n", RECORDSIZE, end - start);

	start = GetTickCount();
	for(i = 1; i <= RECORDSIZE; i = i + 1)
	{
		if (i*13 == 8021)
			printf("here\n");

		sprintf(query, "insert into b values(%d, %d, %d, %d, %d.0, %d.0, %d.0, %d.%d, %d.%d, 'c', '%02d0', '%02d0', SYSDATE, SYSDATE)", i*13, i%10, i%10, i*13, i*13, i*13, i*13, i*13, i*13, i*13, i*13, i%10, i%10);
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : %s\n(%s) \n", query, iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		if(i % 10 == 0)
			iSQL_commit(&isql);
	}
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "%d record insert with  10 commit        : %ld\n", RECORDSIZE, end - start);

	start = GetTickCount();
	for(i = 1; i <= RECORDSIZE; i = i + 1)
	{
		sprintf(query, "insert into c values(%d, %d, %d, %d, %d.0, %d.0, %d.0, %d.%d, %d.%d, 'c', '%02d0', '%02d0', SYSDATE, SYSDATE)", i*31, i%10, i%10, i*31, i*31, i*31, i*31, i*31, i*31, i*31, i*31, i%10, i%10);
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		if(i % 100 == 0)
			iSQL_commit(&isql);
	}
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "%d record insert with 100 commit        : %ld\n", RECORDSIZE, end - start);

	start = GetTickCount();
	for(i = 1; i <= RECORDSIZE; i = i + 1)
	{
		sprintf(query, "insert into d values(%d, %d, %d, %d, %d.0, %d.0, %d.0, %d.%d, %d.%d, 'c', '%02d0', '%02d0', SYSDATE, SYSDATE)", i*31, i%10, i%10, i*31, i*31, i*31, i*31, i*31, i*31, i*31, i*31, i%10, i%10);
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		if(i % 500 == 0)
			iSQL_commit(&isql);
	}
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "%d record insert with 500 commit        : %ld\n", RECORDSIZE, end - start);

	for(i = 1; i <= RECORDSIZE; i = i + 1)
	{
		sprintf(query, "insert into e values(%d, %d, %d, %d, %d.0, %d.0, %d.0, %d.%d, %d.%d, 'c', '%02d0', '%02d0', SYSDATE, SYSDATE)", i*51, i%10, i%10, i*51, i*51, i*51, i*51, i*51, i*51, i*51, i*51, i%10, i%10);
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		if(i % 500 == 0)
			iSQL_commit(&isql);
	}

	for(i = 1; i <= RECORDSIZE; i = i + 1)
	{
		sprintf(query, "insert into f values(%d, %d, %d, %d, %d.0, %d.0, %d.0, %d.%d, %d.%d, 'c', '%02d0', '%02d0', SYSDATE, SYSDATE)", i*51, i%10, i%10, i*51, i*51, i*51, i*51, i*51, i*51, i*51, i*51, i%10, i%10);
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		if(i % 500 == 0)
			iSQL_commit(&isql);
	}

	free(query);
	query = NULL;
	return 1;
}

int UpdateRecord(char* result_file)
{
	long start, end;
	int  i;
	char* query;
	query = (char*)malloc(QUERY_SIZE);
	
	start = GetTickCount();
	for(i = 1; i <= 100; i = i + 1)
	{
		sprintf(query, "update b set b_smallint=10, b_tinyint=1, b_bigint = 100, b_float=0.0, ");
		sprintf(query, "%s b_double=0.0, b_real=0.0, b_decimal=0.0, b_numeric=0.0, b_char='z', b_char10='zzzzzzzzzz', ", query);
		sprintf(query, "%s b_varchar20='zzzzz', b_datetime=SYSDATE, b_timestamp=SYSDATE where b_int=%d",query, i*13*100);

		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : b table update %s \n", iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		iSQL_commit(&isql);
	}
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "100 record all field update with  1 commit  : %ld\n", end - start);

	start = GetTickCount();
	for(i = 1; i <= 100; i = i + 1)
	{
		sprintf(query, "update c set c_smallint=10, c_tinyint=1, c_bigint = 100, c_float=0.0, ");
		sprintf(query, "%s c_double=0.0, c_real=0.0, c_decimal=0.0, c_numeric=0.0, c_char='z', c_char10='zzzzzzzzzz', ", query);
		sprintf(query, "%s c_varchar20='zzzzz', c_datetime=SYSDATE, c_timestamp=SYSDATE where c_int=%d",query, i*31*100);
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : c table update %s \n", iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		if(i % 10 == 0)
			iSQL_commit(&isql);
	}
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "100 record all field update with 10 commit  : %ld\n", end - start);

	free(query);
	query = NULL;
	return 1;
}

int DeleteRecord(char* result_file)
{
	long start, end;
	int  i;
	char* query;
	query = (char*)malloc(QUERY_SIZE);
	
	start = GetTickCount();
	for(i = 1; i <= 100; i = i + 1)
	{
		sprintf(query, "delete from b where b_int=%d", i*13*100);
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : b table delete %s \n", iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		iSQL_commit(&isql);
	}
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "100 record all field delete with  1 commit  : %ld\n", end - start);

	start = GetTickCount();
	for(i = 1; i <= 100; i = i + 1)
	{
		sprintf(query, "delete from c where c_int=%d", i*31*100);
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : c table delete %s \n", iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		if(i % 10 == 0)
			iSQL_commit(&isql);
	}
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "100 record all field delete with 10 commit  : %ld\n", end - start);

	start = GetTickCount();
	sprintf(query, "delete from d");
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "all record field delete                     : %ld\n", end - start);

	sprintf(query, "delete from a");
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);
	
	sprintf(query, "delete from b");
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);

	sprintf(query, "delete from c");
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);

	sprintf(query, "delete from e");
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);

	sprintf(query, "delete from f");
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	iSQL_commit(&isql);
	free(query);
	query = NULL;
	return 1;
}

// 싱글 테이블에서 PK를 가지고 select
int SelectSingleTableWithPK(char* result_file)
{
	long start, end;
	int  i, j, k;
	int	 num_fields;
	char* query;
	query = (char*)malloc(QUERY_SIZE);
	
	for(i = 1; i <= 10; i = i + 1)
	{
		switch(i % 5)
		{
		case 0:
			sprintf(query, "select * from b where b_int=%d", i*13*1000);
			break;
		case 1:
			sprintf(query, "select * from c where c_int=%d", i*31*1000);
			break;
		case 2:
			sprintf(query, "select * from d where d_int=%d", i*31*1000);
			break;
		case 3:
			sprintf(query, "select * from e where e_int=%d", i*51*1000);
			break;
		case 4:
			sprintf(query, "select * from f where f_int=%d", i*51*1000);
			break;
		}
		start = GetTickCount();
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : c table select %s \n", iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		res = iSQL_store_result(&isql);
		if (res == NULL)
		{
			SIMUL_LOG(result_file, "ERROR : failed to store result 1\n");
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}

		num_fields = iSQL_num_fields(res);
		for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
		{
			for (j = 0; j < num_fields; ++j)
				;
		}

		if (!iSQL_eof(res))
			SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

		iSQL_free_result(res);
		iSQL_commit(&isql);
		end = GetTickCount();
		/***********************----/----/----/----/----/----/----/----/----/*/
		SIMUL_LOG(result_file, "select   1 record of single talbe with PK   : %ld\n", end - start);
	}
	free(query);
	query = NULL;
	return 1;
}

// 싱글 테이블에서 Index를 가지고  select
int SelectSingleTableWithIndex(char* result_file)
{
	long start, end;
	int  i, j, k;
	int	 num_fields;
	char* query;
	query = (char*)malloc(QUERY_SIZE);
	
	for(i = 0; i < 10; i = i + 1)
	{
		switch(i % 5)
		{
		case 0:
			sprintf(query, "select * from b where b_tinyint = 5 limit %d, %d", i * 100, 100);
			break;
		case 1:
			sprintf(query, "select * from c where c_tinyint = 5 limit %d, %d", i * 100, 100);
			break;
		case 2:
			sprintf(query, "select * from d where d_tinyint = 5 limit %d, %d", i * 100, 100);
			break;
		case 3:
			sprintf(query, "select * from e where e_tinyint = 5 limit %d, %d", i * 100, 100);
			break;
		case 4:
			sprintf(query, "select * from f where f_tinyint = 5 limit %d, %d", i * 100, 100);
			break;
		}		
		start = GetTickCount();		
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : %s(%s)\n", query, iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		res = iSQL_store_result(&isql);
		if (res == NULL)
		{
			SIMUL_LOG(result_file, "ERROR : failed to store result 2\n");
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}

		num_fields = iSQL_num_fields(res);
		for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
		{
			for (j = 0; j < num_fields; ++j)
				;
		}

		if (!iSQL_eof(res))
			SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

		iSQL_free_result(res);
		iSQL_commit(&isql);
		end = GetTickCount();
		/***********************----/----/----/----/----/----/----/----/----/*/
		SIMUL_LOG(result_file, "select %-3d record of single talbe with index: %ld\n", k, end - start);
	}
	free(query);
	query = NULL;
	return 1;
}

// 싱글 테이블에서 Index 없이  select
int SelectSingleTableWithoutIndex(char* result_file)
{
	long start, end;
	int  i, j, k;
	int	 num_fields;
	char* query;
	query = (char*)malloc(QUERY_SIZE);
	
	for(i = 0; i < 10; i = i + 1)
	{
		switch(i % 5)
		{
		case 0:
			sprintf(query, "select * from b where b_smallint = 5 limit %d, %d", i * 100, 100);
			break;
		case 1:
			sprintf(query, "select * from c where c_smallint = 5 limit %d, %d", i * 100, 100);
			break;
		case 2:
			sprintf(query, "select * from d where d_smallint = 5 limit %d, %d", i * 100, 100);
			break;
		case 3:
			sprintf(query, "select * from e where e_smallint = 5 limit %d, %d", i * 100, 100);
			break;
		case 4:
			sprintf(query, "select * from f where f_smallint = 5 limit %d, %d", i * 100, 100);
			break;
		}	
		start = GetTickCount();
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : c table select %s \n", iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		res = iSQL_store_result(&isql);
		if (res == NULL)
		{
			SIMUL_LOG(result_file, "ERROR : failed to store result 3\n");
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}

		num_fields = iSQL_num_fields(res);
		for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
		{
			for (j = 0; j < num_fields; ++j)
				;
		}

		if (!iSQL_eof(res))
			SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

		iSQL_free_result(res);
		iSQL_commit(&isql);
		end = GetTickCount();
		/***********************----/----/----/----/----/----/----/----/----/*/
		SIMUL_LOG(result_file, "select %-3d record of single talbe without index: %ld\n", k, end - start);
	}
	free(query);
	query = NULL;
	return 1;
}

// 싱글 테이블에서 INDEX를 가지고  Like select
int SelectSingleTableLikeWithPK(char* result_file)
{
	long start, end;
	int  j, k, i;
	int	 num_fields;
	char* query;
	query = (char*)malloc(QUERY_SIZE);
	
	for(i = 0; i < 10; i = i + 1)
	{
		memset(query, 0x00, QUERY_SIZE);
		switch(i % 5)
		{
		case 0:
			sprintf(query, "select * from b where b_varchar20 like '05%%' limit %d, %d", i * 100, 100);
			break;
		case 1:
			sprintf(query, "select * from c where c_varchar20 like '05%%' limit %d, %d", i * 100, 100);
			break;
		case 2:
			sprintf(query, "select * from d where d_varchar20 like '05%%' limit %d, %d", i * 100, 100);
			break;
		case 3:
			sprintf(query, "select * from e where e_varchar20 like '05%%' limit %d, %d", i * 100, 100);
			break;
		case 4:
			sprintf(query, "select * from f where f_varchar20 like '05%%' limit %d, %d", i * 100, 100);
			break;
		}		
		start = GetTickCount();
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		res = iSQL_store_result(&isql);
		if (res == NULL)
		{
			SIMUL_LOG(result_file, "ERROR : failed to store result 4\n");
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}

		num_fields = iSQL_num_fields(res);
		for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
		{
			for (j = 0; j < num_fields; ++j)
				;
		}

		if (!iSQL_eof(res))
			SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

		iSQL_free_result(res);
		iSQL_commit(&isql);
		end = GetTickCount();
		/***********************----/----/----/----/----/----/----/----/----/*/
		SIMUL_LOG(result_file, "select %-3d record of single talbe via right like with Index : %ld\n", k, end - start);
	}


	for(i = 0; i < 10; i = i + 1)
	{
		memset(query, 0x00, QUERY_SIZE);
		switch(i % 5)
		{
		case 0:
			sprintf(query, "select * from b where b_varchar20 like '%%50' limit %d, %d", i * 100, 100);
			break;
		case 1:
			sprintf(query, "select * from c where c_varchar20 like '%%50' limit %d, %d", i * 100, 100);
			break;
		case 2:
			sprintf(query, "select * from d where d_varchar20 like '%%50' limit %d, %d", i * 100, 100);
			break;
		case 3:
			sprintf(query, "select * from e where e_varchar20 like '%%50' limit %d, %d", i * 100, 100);
			break;
		case 4:
			sprintf(query, "select * from f where f_varchar20 like '%%50' limit %d, %d", i * 100, 100);
			break;
		}		
		start = GetTickCount();
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		res = iSQL_store_result(&isql);
		if (res == NULL)
		{
			SIMUL_LOG(result_file, "ERROR : failed to store result 4\n");
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}

		num_fields = iSQL_num_fields(res);
		for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
		{
			for (j = 0; j < num_fields; ++j)
				;
		}

		if (!iSQL_eof(res))
			SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

		iSQL_free_result(res);
		iSQL_commit(&isql);
		end = GetTickCount();
		/***********************----/----/----/----/----/----/----/----/----/*/
		SIMUL_LOG(result_file, "select %-3d record of single talbe via left like with Index : %ld\n", k, end - start);
	}
	
	for(i = 0; i < 10; i = i + 1)
	{
		memset(query, 0x00, QUERY_SIZE);
		switch(i % 5)
		{
		case 0:
			sprintf(query, "select * from b where b_varchar20 like '%%5%%' limit %d, %d", i * 100, 100);
			break;
		case 1:
			sprintf(query, "select * from c where c_varchar20 like '%%5%%' limit %d, %d", i * 100, 100);
			break;
		case 2:
			sprintf(query, "select * from d where d_varchar20 like '%%5%%' limit %d, %d", i * 100, 100);
			break;
		case 3:
			sprintf(query, "select * from e where e_varchar20 like '%%5%%' limit %d, %d", i * 100, 100);
			break;
		case 4:
			sprintf(query, "select * from f where f_varchar20 like '%%5%%' limit %d, %d", i * 100, 100);
			break;
		}		
		start = GetTickCount();
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		res = iSQL_store_result(&isql);
		if (res == NULL)
		{
			SIMUL_LOG(result_file, "ERROR : failed to store result 4\n");
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}

		num_fields = iSQL_num_fields(res);
		for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
		{
			for (j = 0; j < num_fields; ++j)
				;
		}

		if (!iSQL_eof(res))
			SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

		iSQL_free_result(res);
		iSQL_commit(&isql);
		end = GetTickCount();
		/***********************----/----/----/----/----/----/----/----/----/*/
		SIMUL_LOG(result_file, "select %-3d record of single talbe via both like with Index : %ld\n", k, end - start);
	}



		for(i = 0; i < 10; i = i + 1)
	{
		memset(query, 0x00, QUERY_SIZE);
		switch(i % 5)
		{
		case 0:
			sprintf(query, "select * from b where b_char10 like '05%%' limit %d, %d", i * 100, 100);
			break;
		case 1:
			sprintf(query, "select * from c where c_char10 like '05%%' limit %d, %d", i * 100, 100);
			break;
		case 2:
			sprintf(query, "select * from d where d_char10 like '05%%' limit %d, %d", i * 100, 100);
			break;
		case 3:
			sprintf(query, "select * from e where e_char10 like '05%%' limit %d, %d", i * 100, 100);
			break;
		case 4:
			sprintf(query, "select * from f where f_char10 like '05%%' limit %d, %d", i * 100, 100);
			break;
		}		
		start = GetTickCount();
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		res = iSQL_store_result(&isql);
		if (res == NULL)
		{
			SIMUL_LOG(result_file, "ERROR : failed to store result 4\n");
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}

		num_fields = iSQL_num_fields(res);
		for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
		{
			for (j = 0; j < num_fields; ++j)
				;
		}

		if (!iSQL_eof(res))
			SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

		iSQL_free_result(res);
		iSQL_commit(&isql);
		end = GetTickCount();
		/***********************----/----/----/----/----/----/----/----/----/*/
		SIMUL_LOG(result_file, "select %-3d record of single talbe via right like without Index : %ld\n", k, end - start);
	}


	for(i = 0; i < 10; i = i + 1)
	{
		memset(query, 0x00, QUERY_SIZE);
		switch(i % 5)
		{
		case 0:
			sprintf(query, "select * from b where b_char10 like '%%50' limit %d, %d", i * 100, 100);
			break;
		case 1:
			sprintf(query, "select * from c where c_char10 like '%%50' limit %d, %d", i * 100, 100);
			break;
		case 2:
			sprintf(query, "select * from d where d_char10 like '%%50' limit %d, %d", i * 100, 100);
			break;
		case 3:
			sprintf(query, "select * from e where e_char10 like '%%50' limit %d, %d", i * 100, 100);
			break;
		case 4:
			sprintf(query, "select * from f where f_char10 like '%%50' limit %d, %d", i * 100, 100);
			break;
		}		
		start = GetTickCount();
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		res = iSQL_store_result(&isql);
		if (res == NULL)
		{
			SIMUL_LOG(result_file, "ERROR : failed to store result 4\n");
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}

		num_fields = iSQL_num_fields(res);
		for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
		{
			for (j = 0; j < num_fields; ++j)
				;
		}

		if (!iSQL_eof(res))
			SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

		iSQL_free_result(res);
		iSQL_commit(&isql);
		end = GetTickCount();
		/***********************----/----/----/----/----/----/----/----/----/*/
		SIMUL_LOG(result_file, "select %-3d record of single talbe via left like without Index : %ld\n", k, end - start);
	}
	
	for(i = 0; i < 10; i = i + 1)
	{
		memset(query, 0x00, QUERY_SIZE);
		switch(i % 5)
		{
		case 0:
			sprintf(query, "select * from b where b_char10 like '%%5%%' limit %d, %d", i * 100, 100);
			break;
		case 1:
			sprintf(query, "select * from c where c_char10 like '%%5%%' limit %d, %d", i * 100, 100);
			break;
		case 2:
			sprintf(query, "select * from d where d_char10 like '%%5%%' limit %d, %d", i * 100, 100);
			break;
		case 3:
			sprintf(query, "select * from e where e_char10 like '%%5%%' limit %d, %d", i * 100, 100);
			break;
		case 4:
			sprintf(query, "select * from f where f_char10 like '%%5%%' limit %d, %d", i * 100, 100);
			break;
		}		
		start = GetTickCount();
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		res = iSQL_store_result(&isql);
		if (res == NULL)
		{
			SIMUL_LOG(result_file, "ERROR : failed to store result 4\n");
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}

		num_fields = iSQL_num_fields(res);
		for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
		{
			for (j = 0; j < num_fields; ++j)
				;
		}

		if (!iSQL_eof(res))
			SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

		iSQL_free_result(res);
		iSQL_commit(&isql);
		end = GetTickCount();
		/***********************----/----/----/----/----/----/----/----/----/*/
		SIMUL_LOG(result_file, "select %-3d record of single talbe via both like without Index : %ld\n", k, end - start);
	}

	free(query);
	query = NULL;
	return 1;
}

// 2개의 테이블을 조인하여 select 
int SelectJoin2WithIndex(char* result_file)
{
	long start, end;
	int  j, k;
	int	 num_fields;
	char* query;
	query = (char*)malloc(QUERY_SIZE);
	
	start = GetTickCount();
	sprintf(query, "select btable.b_int, ctable.c_bigint, btable.b_datetime, ctable.c_timestamp \
		from b btable, c ctable where btable.b_int = ctable.c_int and btable.b_int \
		between 4030 and 8060");
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	res = iSQL_store_result(&isql);
	if (res == NULL)
	{
		SIMUL_LOG(result_file, "ERROR : failed to store result 10\n");
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}

	num_fields = iSQL_num_fields(res);
	for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
	{
		for (j = 0; j < num_fields; ++j)
			;
	}

	if (!iSQL_eof(res))
		SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

	iSQL_free_result(res);
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "select %-3d record of 2 talbe join with index : %ld\n", k, end - start);


	start = GetTickCount();
	sprintf(query, "select atable.a_int, dtable.d_bigint, atable.a_datetime, dtable.d_timestamp from a atable, d dtable ");
	sprintf(query, "%s where atable.a_float = dtable.d_float and atable.a_int between 4030 and 8060", query);
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	res = iSQL_store_result(&isql);
	if (res == NULL)
	{
		SIMUL_LOG(result_file, "ERROR : failed to store result 11\n");
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}

	num_fields = iSQL_num_fields(res);
	for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
	{
		for (j = 0; j < num_fields; ++j)
			;
	}

	if (!iSQL_eof(res))
		SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

	iSQL_free_result(res);
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "select %-3d record of 2 talbe join without index : %ld\n", k, end - start);

	free(query);
	query = NULL;
	return 1;
}

// 2개의 테이블을 조인하여 Like select
int SelectJoin2LiktWithIndex(char* result_file)
{
	long start, end;
	int  j, k;
	int	 num_fields;
	char* query;
	query = (char*)malloc(QUERY_SIZE);
	
	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "select ctable.c_int, etable.e_bigint, ctable.c_datetime, etable.e_timestamp from c ctable, e etable ");
	sprintf(query, "%s where ctable.c_int = etable.e_int and ctable.c_varchar20 like '05%%'", query);
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	res = iSQL_store_result(&isql);
	if (res == NULL)
	{
		SIMUL_LOG(result_file, "ERROR : failed to store result 12\n");
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}

	num_fields = iSQL_num_fields(res);
	for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
	{
		for (j = 0; j < num_fields; ++j)
			;
	}

	if (!iSQL_eof(res))
		SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

	iSQL_free_result(res);
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "select %-3d record of 2 talbe join and right like with index : %ld\n", k, end - start);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "select dtable.d_int, ftable.f_bigint, dtable.d_datetime, ftable.f_timestamp from d dtable, f ftable ");
	sprintf(query, "%s where dtable.d_int = ftable.f_int and dtable.d_varchar20 like '%%50'", query);
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	res = iSQL_store_result(&isql);
	if (res == NULL)
	{
		SIMUL_LOG(result_file, "ERROR : failed to store result 13\n");
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}

	num_fields = iSQL_num_fields(res);
	for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
	{
		for (j = 0; j < num_fields; ++j)
			;
	}

	if (!iSQL_eof(res))
		SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

	iSQL_free_result(res);
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "select %-3d record of 2 talbe join and left like with index : %ld\n", k, end - start);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "select ctable.c_int, ftable.f_bigint, ctable.c_datetime, ftable.f_timestamp from c ctable, f ftable ");
	sprintf(query, "%s where ctable.c_int = ftable.f_int and ctable.c_varchar20 like '%%5%%'", query);
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	res = iSQL_store_result(&isql);
	if (res == NULL)
	{
		SIMUL_LOG(result_file, "ERROR : failed to store result 14\n");
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}

	num_fields = iSQL_num_fields(res);
	for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
	{
		for (j = 0; j < num_fields; ++j)
			;
	}

	if (!iSQL_eof(res))
		SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

	iSQL_free_result(res);
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "select %-3d record of 2 talbe join and both like with index : %ld\n", k, end - start);

	/*********************************************************************/
	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "select ctable.c_int, etable.e_bigint, ctable.c_datetime, etable.e_timestamp from c ctable, e etable ");
	sprintf(query, "%s where ctable.c_int = etable.e_int and ctable.c_char10 like '05%%'", query);
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	res = iSQL_store_result(&isql);
	if (res == NULL)
	{
		SIMUL_LOG(result_file, "ERROR : failed to store result 12\n");
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}

	num_fields = iSQL_num_fields(res);
	for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
	{
		for (j = 0; j < num_fields; ++j)
			;
	}

	if (!iSQL_eof(res))
		SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

	iSQL_free_result(res);
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "select %-3d record of 2 talbe join and right like without index : %ld\n", k, end - start);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "select dtable.d_int, ftable.f_bigint, dtable.d_datetime, ftable.f_timestamp from d dtable, f ftable ");
	sprintf(query, "%s where dtable.d_int = ftable.f_int and dtable.d_char10 like '%%50'", query);
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	res = iSQL_store_result(&isql);
	if (res == NULL)
	{
		SIMUL_LOG(result_file, "ERROR : failed to store result 13\n");
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}

	num_fields = iSQL_num_fields(res);
	for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
	{
		for (j = 0; j < num_fields; ++j)
			;
	}

	if (!iSQL_eof(res))
		SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

	iSQL_free_result(res);
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "select %-3d record of 2 talbe join and left like without index : %ld\n", k, end - start);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "select ctable.c_int, ftable.f_bigint, ctable.c_datetime, ftable.f_timestamp from c ctable, f ftable ");
	sprintf(query, "%s where ctable.c_int = ftable.f_int and ctable.c_char10 like '%%5%%'", query);
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	res = iSQL_store_result(&isql);
	if (res == NULL)
	{
		SIMUL_LOG(result_file, "ERROR : failed to store result 14\n");
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}

	num_fields = iSQL_num_fields(res);
	for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
	{
		for (j = 0; j < num_fields; ++j)
			;
	}

	if (!iSQL_eof(res))
		SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

	iSQL_free_result(res);
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "select %-3d record of 2 talbe join and both like without index : %ld\n", k, end - start);

	free(query);
	query = NULL;
	return 1;
}

int SelectJoin3WithIndex(char* result_file)
{
	long start, end;
	int  j, k;
	int	 num_fields;
	char* query;
	query = (char*)malloc(QUERY_SIZE);
	
	start = GetTickCount();
	sprintf(query, "select atable.a_int, ctable.c_bigint, atable.a_datetime, etable.e_timestamp ");
	sprintf(query, "%s from a atable, c ctable, e etable ", query);
	sprintf(query, "%s where atable.a_int = ctable.c_int and atable.a_int = etable.e_int and ctable.c_int = etable.e_int", query);
	sprintf(query, "%s and atable.a_int = 20553", query);
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	res = iSQL_store_result(&isql);
	if (res == NULL)
	{
		SIMUL_LOG(result_file, "ERROR : failed to store result 10\n");
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}

	num_fields = iSQL_num_fields(res);
	for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
	{
		for (j = 0; j < num_fields; ++j)
			;
	}

	if (!iSQL_eof(res))
		SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

	iSQL_free_result(res);
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "select %-3d record of 3 talbe join with index : %ld\n", k, end - start);


	start = GetTickCount();
	sprintf(query, "select btable.b_int, dtable.d_bigint, ftable.f_datetime, btable.b_timestamp ");
	sprintf(query, "%s from b btable, d dtable, f ftable ", query);
	sprintf(query, "%s where btable.b_int = dtable.d_int and btable.b_int = ftable.f_int and dtable.d_int = ftable.f_int", query);
	sprintf(query, "%s and btable.b_int = 20553", query);
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	res = iSQL_store_result(&isql);
	if (res == NULL)
	{
		SIMUL_LOG(result_file, "ERROR : failed to store result 11\n");
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}

	num_fields = iSQL_num_fields(res);
	for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
	{
		for (j = 0; j < num_fields; ++j)
			;
	}

	if (!iSQL_eof(res))
		SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

	iSQL_free_result(res);
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "select %-3d record of 3 talbe join without index : %ld\n", k, end - start);

	free(query);
	query = NULL;
	return 1;
}

// 3개의 테이블을 조인하여 Like select
int SelectJoin3LiktWithIndex(char* result_file)
{
	long start, end;
	int  j, k;
	int	 num_fields;
	char* query;
	query = (char*)malloc(QUERY_SIZE);
	
	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "select ctable.c_int, etable.e_bigint, ctable.c_datetime, etable.e_timestamp ");
	sprintf(query, "%s from a atable, c ctable, e etable ", query);
	sprintf(query, "%s where atable.a_int = ctable.c_int and atable.a_int = etable.e_int and ctable.c_int = etable.e_int and atable.a_varchar20 like '05%%'", query);
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	res = iSQL_store_result(&isql);
	if (res == NULL)
	{
		SIMUL_LOG(result_file, "ERROR : failed to store result 12\n");
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}

	num_fields = iSQL_num_fields(res);
	for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
	{
		for (j = 0; j < num_fields; ++j)
			;
	}

	if (!iSQL_eof(res))
		SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

	iSQL_free_result(res);
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "select %-3d record of 3 talbe join and right like with index : %ld\n", k, end - start);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "select dtable.d_int, ftable.f_bigint, dtable.d_datetime, ftable.f_timestamp ");
	sprintf(query, "%s from b btable, d dtable, f ftable ", query);
	sprintf(query, "%s where btable.b_int = dtable.d_int and btable.b_int = ftable.f_int and dtable.d_int = ftable.f_int and btable.b_varchar20 like '%%50'", query);
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	res = iSQL_store_result(&isql);
	if (res == NULL)
	{
		SIMUL_LOG(result_file, "ERROR : failed to store result 13\n");
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}

	num_fields = iSQL_num_fields(res);
	for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
	{
		for (j = 0; j < num_fields; ++j)
			;
	}

	if (!iSQL_eof(res))
		SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

	iSQL_free_result(res);
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "select %-3d record of 3 talbe join and left like with index : %ld\n", k, end - start);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "select ctable.c_int, etable.e_bigint, ctable.c_datetime, etable.e_timestamp ");
	sprintf(query, "%s from a atable, c ctable, e etable ", query);
	sprintf(query, "%s where atable.a_int = ctable.c_int and atable.a_int = etable.e_int and ctable.c_int = etable.e_int and atable.a_varchar20 like '%%5%%'", query);
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	res = iSQL_store_result(&isql);
	if (res == NULL)
	{
		SIMUL_LOG(result_file, "ERROR : failed to store result 14\n");
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}

	num_fields = iSQL_num_fields(res);
	for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
	{
		for (j = 0; j < num_fields; ++j)
			;
	}

	if (!iSQL_eof(res))
		SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

	iSQL_free_result(res);
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "select %-3d record of 3 talbe join and both like with index : %ld\n", k, end - start);

	/*********************************************************************/
	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "select ctable.c_int, etable.e_bigint, ctable.c_datetime, etable.e_timestamp ");
	sprintf(query, "%s from a atable, c ctable, e etable ", query);
	sprintf(query, "%s where atable.a_int = ctable.c_int and atable.a_int = etable.e_int and ctable.c_int = etable.e_int and atable.a_char10 like '05%%'", query);
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	res = iSQL_store_result(&isql);
	if (res == NULL)
	{
		SIMUL_LOG(result_file, "ERROR : failed to store result 12\n");
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}

	num_fields = iSQL_num_fields(res);
	for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
	{
		for (j = 0; j < num_fields; ++j)
			;
	}

	if (!iSQL_eof(res))
		SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

	iSQL_free_result(res);
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "select %-3d record of 3 talbe join and right like without index : %ld\n", k, end - start);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "select dtable.d_int, ftable.f_bigint, dtable.d_datetime, ftable.f_timestamp ");
	sprintf(query, "%s from b btable, d dtable, f ftable ", query);
	sprintf(query, "%s where btable.b_int = dtable.d_int and btable.b_int = ftable.f_int and dtable.d_int = ftable.f_int and btable.b_char10 like '%%50'", query);
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	res = iSQL_store_result(&isql);
	if (res == NULL)
	{
		SIMUL_LOG(result_file, "ERROR : failed to store result 13\n");
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}

	num_fields = iSQL_num_fields(res);
	for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
	{
		for (j = 0; j < num_fields; ++j)
			;
	}

	if (!iSQL_eof(res))
		SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

	iSQL_free_result(res);
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "select %-3d record of 3 talbe join and left like without index : %ld\n", k, end - start);

	memset(query, 0x00, QUERY_SIZE);
	sprintf(query, "select ctable.c_int, etable.e_bigint, ctable.c_datetime, etable.e_timestamp ");
	sprintf(query, "%s from a atable, c ctable, e etable ", query);
	sprintf(query, "%s where atable.a_int = ctable.c_int and atable.a_int = etable.e_int and ctable.c_int = etable.e_int and atable.a_char10 like '%%5%%'", query);
	start = GetTickCount();
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	res = iSQL_store_result(&isql);
	if (res == NULL)
	{
		SIMUL_LOG(result_file, "ERROR : failed to store result 14\n");
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}

	num_fields = iSQL_num_fields(res);
	for (k = 0; (row = iSQL_fetch_row(res)) != NULL; ++k)
	{
		for (j = 0; j < num_fields; ++j)
			;
	}

	if (!iSQL_eof(res))
		SIMUL_LOG(result_file, "the last row was not read : %s\n", iSQL_error(&isql));

	iSQL_free_result(res);
	iSQL_commit(&isql);
	end = GetTickCount();
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "select %-3d record of 3 talbe join and both like without index : %ld\n", k, end - start);

	free(query);
	query = NULL;
	return 1;
}

int CASE_THREE_TestStart(char* out_path)
{
	int i = 0;
	int nCount = 0;
	char nrow[10];
	char* result_file;
	char* query;
	query = (char*)malloc(QUERY_SIZE);

	result_file = (char*)malloc(strlen(out_path) + 11);
	sprintf(result_file, "%s_Three.dat", out_path);

	sprintf(query, "select count(*) from a");
	if (iSQL_query(&isql, query) < 0)
	{
		SIMUL_LOG(result_file, "ERROR : %s(%s) \n", query, iSQL_error(&isql));
		iSQL_disconnect(&isql);
		free(query);
		query = NULL;
		return -1;
	}
	res = iSQL_store_result(&isql);
	if (res != NULL)
		if((row = iSQL_fetch_row(res)) != SQL_NO_DATA)
		{
			sprintf(nrow, "%s", row[0]);
			nCount = atoi(nrow);
		}
	iSQL_free_result(res);
	iSQL_commit(&isql);
	if(nCount < 1)
		return FALSE;
	/***********************----/----/----/----/----/----/----/----/----/*/
	SIMUL_LOG(result_file, "all record is                :%d\n", nCount);

	while(1)
	{
		sprintf(query, "insert into a values(%d, %d, %d, %d, %d.0, %d.0, %d.0, %d.%d, %d.%d, 'c', '%02d0', '%02d0', SYSDATE, SYSDATE)", i, i%10, i%10, i, i, i, i, i, i, i, i, i%10, i%10);
		if (iSQL_query(&isql, query) < 0)
		{
			SIMUL_LOG(result_file, "ERROR : a table insert %s \n", iSQL_error(&isql));
			iSQL_disconnect(&isql);
			free(query);
			query = NULL;
			return -1;
		}
		if(++i % 100)
			iSQL_commit(&isql);
	}

	free(query);
	query = NULL;
	return 1;
}
