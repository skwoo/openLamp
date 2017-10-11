//
// mSyncDemoLocal.c

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "msyncDemo.h"
#include "isql.h"

typedef struct
{
    iSQL_RES *res;
    iSQL_ROW row;
    iSQL_STMT *stmt;
    iSQL_BIND *bind_res;
    int nRows;
    int nCurRow;
    int nFields;
} SLocalDB_Result;

iSQL g_iSQL;
SLocalDB_Result g_SResult;


static int
create_sample_table()
{
    int nRet;

#ifdef USE_MSYNC_TABLE
    static char szContact_schema[] = "CREATE msync TABLE contact( "
            "ID int, "
            "username varchar(32),"
            "Company varchar(255), "
            "name varchar(255), "
            "e_mail varchar(255), "
            "Company_phone char(20), "
            "Mobile_phone char(20), "
            "Address varchar(255), "
            "mTime DateTime, " "primary key(ID, username) )";
#else
    static char szContact_schema[] = "CREATE TABLE contact( "
            "ID int, "
            "username varchar(32),"
            "Company varchar(255), "
            "name varchar(255), "
            "e_mail varchar(255), "
            "Company_phone char(20), "
            "Mobile_phone char(20), "
            "Address varchar(255), "
            "mTime DateTime, " "dFlag tinyint, " "primary key(ID, username) )";
#endif


    nRet = iSQL_query(&g_iSQL, szContact_schema);
    if (nRet < 0)
    {
        printf("ERROR: iSQL_query() ..%s\n", iSQL_error(&g_iSQL));
        iSQL_rollback(&g_iSQL);
    }
    else
        iSQL_commit(&g_iSQL);

    return nRet;
}

void
QuitLocalDB()
{
    iSQL_disconnect(&g_iSQL);

    if (g_SResult.res)
    {
        iSQL_free_result(g_SResult.res);
        g_SResult.res = NULL;
    }
}

int
InitLocalDB(char *dbname)
{
    int nRet;

    memset(&g_SResult, 0x00, sizeof(SLocalDB_Result));

    // 单捞磐海捞胶 立加.    
    if (iSQL_connect(&g_iSQL, "127.0.0.1", dbname, "id", "pwd") == NULL)
    {
        // 单捞磐海捞胶 积己.
        nRet = createdb(dbname);
        if (nRet < 0)
        {
            printf("ERROR: createdb()\n");
            return -1;
        }

        if (iSQL_connect(&g_iSQL, "127.0.0.1", dbname, "id", "pwd") == NULL)
        {
            printf("ERROR: iSQL_connect() ..%s\n", iSQL_error(&g_iSQL));
            return -2;
        }

        // 抗力 抛捞喉 积己.
        nRet = create_sample_table();
        if (nRet < 0)
        {
            iSQL_disconnect(&g_iSQL);
            return -3;
        }
    }

    return 0;
}

void
CommitLocalDB(int isSuccess)
{
    if (isSuccess)
        iSQL_commit(&g_iSQL);
    else
        iSQL_rollback(&g_iSQL);
}

int
SyncInsertLocalDB(S_Recoed * pRecord, int bCommit)
{
    char query[8192];

#ifdef USE_MSYNC_TABLE
    sprintf(query,
            "UPSERT into contact(id, username, company, name, e_mail, company_phone,"
            "mobile_phone, address, mtime) "
            "values(%d, '%s', '%s', '%s', '%s', '%s', '%s', '%s', now()) SYNCED_RECORD",
            pRecord->ID, pRecord->username, pRecord->Company, pRecord->Name,
            pRecord->e_mail, pRecord->Company_phone, pRecord->Mobile_phone,
            pRecord->Address);

#else
    sprintf(query,
            "insert into contact(id, username, company, name, e_mail, company_phone, "
            "mobile_phone, address, mtime, dflag) "
            "values(%d, '%s', '%s', '%s', '%s', '%s', '%s', '%s', now(), %d)",
            pRecord->ID, pRecord->username, pRecord->Company, pRecord->Name,
            pRecord->e_mail, pRecord->Company_phone,
            pRecord->Mobile_phone, pRecord->Address, pRecord->dflag);
#endif

    if (iSQL_query(&g_iSQL, query) != 0)
    {
#ifndef USE_MSYNC_TABLE
        if (iSQL_errno(&g_iSQL) == DB_E_DUPUNIQUEKEY)
        {
            sprintf(query, "update contact set company = '%s', name = '%s', "
                    "e_mail = '%s', company_phone = '%s', mobile_phone = '%s', "
                    "address = '%s', mtime = now(), dflag = %d "
                    "where id = %d AND username = '%s'",
                    pRecord->Company, pRecord->Name, pRecord->e_mail,
                    pRecord->Company_phone, pRecord->Mobile_phone,
                    pRecord->Address, pRecord->dflag, pRecord->ID,
                    pRecord->username);
            if (iSQL_query(&g_iSQL, query) != 0)
            {
                printf("%s\n", query);
                printf("%s(%d)", iSQL_error(&g_iSQL), iSQL_errno(&g_iSQL));
                if (bCommit)
                    iSQL_rollback(&g_iSQL);

                return -1;
            }
        }
        else
#endif
        {
            printf("%s\n", query);
            printf("%s(%d)", iSQL_error(&g_iSQL), iSQL_errno(&g_iSQL));
            if (bCommit)
                iSQL_rollback(&g_iSQL);

            return -1;
        }
    }

    if (bCommit)
        iSQL_commit(&g_iSQL);

    return 0;
}

int
GetMaxIDLocalDB(char *puser)
{
    iSQL_RES *res;
    iSQL_ROW row;
    int nID = -999;
    char buf[256];

    sprintf(buf, "select max(id) from contact where username = '%s' ", puser);

    if (iSQL_query(&g_iSQL, buf) != 0)
    {
        printf("%s(%d)\n", iSQL_error(&g_iSQL), iSQL_errno(&g_iSQL));
        iSQL_rollback(&g_iSQL);
    }
    else
    {
        res = iSQL_store_result(&g_iSQL);
        row = iSQL_fetch_row(res);
        nID = atoi(row[0]);

        iSQL_free_result(res);
        iSQL_commit(&g_iSQL);
    }

    return nID;
}

int
QueryLocalDB(char *pQuery)
{
    int nRet;

    nRet = iSQL_query(&g_iSQL, pQuery);
    if (nRet < 0)
    {
        printf("ERROR: iSQL_query(%s) ..%s\n", pQuery, iSQL_error(&g_iSQL));
        iSQL_rollback(&g_iSQL);
    }
    else
        iSQL_commit(&g_iSQL);

    return nRet;
}

int
SelectLocalDB(char *pQuery)
{
    iSQL_BIND *bind_param;

    g_SResult.nRows = 0;
    g_SResult.nCurRow = 0;
    if (!(g_SResult.stmt = iSQL_prepare(&g_iSQL, pQuery, strlen(pQuery))))
        goto error_pos;

    if (iSQL_describe(g_SResult.stmt, &bind_param, &(g_SResult.bind_res)) < 0)
        goto error_pos;

    if (!(g_SResult.res = iSQL_prepare_result(g_SResult.stmt)))
        goto error_pos;

    if (iSQL_execute(g_SResult.stmt))
        goto error_pos;

    if (iSQL_stmt_store_result(g_SResult.stmt))
        goto error_pos;

    g_SResult.nRows = iSQL_stmt_affected_rows(g_SResult.stmt);
    g_SResult.nFields = iSQL_num_result_fields(g_SResult.stmt);

    return g_SResult.nRows;

  error_pos:
    printf("%s(%d)\n", iSQL_error(&g_iSQL), iSQL_errno(&g_iSQL));

    if (g_SResult.res)
    {
        iSQL_free_result(g_SResult.res);
        g_SResult.res = NULL;
    }

    return -1;
}

int
FetchLocalDB(S_Recoed * pRecord)
{
    int i;
    iSQL_BIND *bind_res;
    iSQL_TIME time_val;
    iSQL_FIELD *fields;

    if (g_SResult.nCurRow >= g_SResult.nRows)
        return 0;       /* No more result */

    if (iSQL_fetch(g_SResult.stmt) < 0)
    {
        iSQL_free_result(g_SResult.res);
        g_SResult.res = NULL;
        return -1;
    }

    memset(pRecord, 0x00, sizeof(S_Recoed));
    fields = g_SResult.stmt->fields;

    bind_res = g_SResult.bind_res;
    for (i = 0; i < g_SResult.nFields; i++)
    {
        if (stricmp(fields[i].name, "id") == 0)
        {
            pRecord->ID = *(int *) bind_res[i].buffer;
        }
        else if (stricmp(fields[i].name, "username") == 0)
        {
            strcpy(pRecord->username, bind_res[i].buffer);
        }
        else if (stricmp(fields[i].name, "Company") == 0)
        {
            strcpy(pRecord->Company, bind_res[i].buffer);
        }
        else if (stricmp(fields[i].name, "name") == 0)
        {
            strcpy(pRecord->Name, bind_res[i].buffer);
        }
        else if (stricmp(fields[i].name, "e_mail") == 0)
        {
            strcpy(pRecord->e_mail, bind_res[i].buffer);
        }
        else if (stricmp(fields[i].name, "Company_phone") == 0)
        {
            strcpy(pRecord->Company_phone, bind_res[i].buffer);
        }
        else if (stricmp(fields[i].name, "Mobile_phone") == 0)
        {
            strcpy(pRecord->Mobile_phone, bind_res[i].buffer);
        }
        else if (stricmp(fields[i].name, "Address") == 0)
        {
            strcpy(pRecord->Address, bind_res[i].buffer);
        }
        else if (stricmp(fields[i].name, "mTime") == 0)
        {
            memcpy(&time_val, bind_res[i].buffer, bind_res[i].buffer_length);
            time_val.fraction = 0;
            sprintf(pRecord->mTime, "%04d-%02d-%02d %02d:%02d:%02d",
                    time_val.year, time_val.month, time_val.day,
                    time_val.hour, time_val.minute, time_val.second);
        }
#ifndef USE_MSYNC_TABLE
        else if (stricmp(fields[i].name, "dFlag") == 0)
        {
            pRecord->dflag = *(int *) bind_res[i].buffer;
        }
#endif
        else
        {
            return -2;
        }
    }

    return 1;
}

void
QuitSelectLocalDB()
{
    if (g_SResult.res)
    {
        iSQL_free_result(g_SResult.res);
        g_SResult.res = NULL;
    }

    iSQL_commit(&g_iSQL);
}

/* isAllshow  1: show all, 0: not deleted */
int
Show_LocalDB(int isAllshow)
{
    iSQL_BIND *bind_res;
    char query[512];
    int i, j;
    iSQL_TIME time_val;

    g_SResult.res = NULL;
#ifdef USE_MSYNC_TABLE
    sprintf(query,
            "select id, username, company, name, e_mail, company_phone, "
            "mobile_phone, address, mTime from contact");

    isAllshow = (isAllshow != 0);
  deleted_rec:
#else
    if (isAllshow)
        sprintf(query,
                "select id, username, company, name, e_mail, company_phone, "
                "mobile_phone, address, mTime, dflag from contact");
    else
        sprintf(query,
                "select id, username, company, name, e_mail, company_phone, "
                "mobile_phone, address, mTime, dflag from contact "
                "where dflag <> %d", DFLAG_DELETED);
#endif

    if (SelectLocalDB(query) < 0)
        return -1;

#ifdef USE_MSYNC_TABLE
    if (isAllshow == -1)
        printf("\n*** LocalDB(INSERT)...\n");
    else if (isAllshow == -2)
        printf("\n*** LocalDB(UPDATE)...\n");
    else if (isAllshow == -3)
        printf("\n*** LocalDB(DELETE)...\n");
    else if (isAllshow == -4)
        printf("\n*** LocalDB(SYNCED)...\n");
    else
#endif
    {
        printf("\n*************************************************************");
        printf("\n*** LocalDB...");
        printf("\n*************************************************************\n");
    }

    bind_res = g_SResult.bind_res;
    for (i = 0; i < g_SResult.nRows; i++)
    {
        if (iSQL_fetch(g_SResult.stmt) < 0)
            goto error_pos;

        for (j = 0; j < g_SResult.nFields; j++)
        {
            switch (j)
            {
            case 0:    // id                    
                printf("%d", *(int *) bind_res[j].buffer);
                break;
            case 1:    // username
                printf("  |  %s", bind_res[j].buffer);
                break;
            case 2:    // company
                printf("  |  %s", bind_res[j].buffer);
                break;
            case 3:    // name
                printf("  |  %s", bind_res[j].buffer);
                break;
            case 4:    // e_mail
                printf("  |  %s", bind_res[j].buffer);
                break;
            case 5:    // company_phone
                printf("  |  %s", bind_res[j].buffer);
                break;
            case 6:    // mobile_phone
                printf("  |  %s", bind_res[j].buffer);
                break;
            case 7:    // address
                printf("  |  %s", bind_res[j].buffer);
                break;
            case 8:    // mTime
                memcpy(&time_val, bind_res[j].buffer,
                        bind_res[j].buffer_length);
                time_val.fraction = 0;
                printf("  |  %04d-%02d-%02d %02d:%02d:%02d",
                        time_val.year, time_val.month, time_val.day,
                        time_val.hour, time_val.minute, time_val.second);
                break;
#ifndef USE_MSYNC_TABLE
            case 9:    // dflag
                printf("  |  %d", *(int *) bind_res[j].buffer);
                break;
#endif
            }
        }
        printf("\n");
    }

    if (g_SResult.res)
    {
        iSQL_free_result(g_SResult.res);
        g_SResult.res = NULL;
    }
    iSQL_commit(&g_iSQL);
#ifdef USE_MSYNC_TABLE
    if (isAllshow == 1)
    {
        sprintf(query,
                "select id, username, company, name, e_mail, company_phone, "
                "mobile_phone, address, mTime from contact  INSERT_RECORD");

        isAllshow = -1;
        goto deleted_rec;
    }
    else if (isAllshow == -1)
    {
        sprintf(query,
                "select id, username, company, name, e_mail, company_phone, "
                "mobile_phone, address, mTime from contact  UPDATE_RECORD");

        isAllshow = -2;
        goto deleted_rec;
    }
    else if (isAllshow == -2)
    {
        sprintf(query,
                "select id, username, company, name, e_mail, company_phone, "
                "mobile_phone, address, mTime from contact  DELETE_RECORD");

        isAllshow = -3;
        goto deleted_rec;
    }
    else if (isAllshow == -3)
    {
        sprintf(query,
                "select id, username, company, name, e_mail, company_phone, "
                "mobile_phone, address, mTime from contact  SYNCED_RECORD");

        isAllshow = -4;
        goto deleted_rec;
    }
#endif

    return 0;

  error_pos:
    printf("%s(%d)\n", iSQL_error(&g_iSQL), iSQL_errno(&g_iSQL));
    if (g_SResult.res)
    {
        iSQL_free_result(g_SResult.res);
        g_SResult.res = NULL;
    }

    iSQL_rollback(&g_iSQL);
    return -1;
}
