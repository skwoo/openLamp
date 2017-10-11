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

package com.openml.app;

import com.openml.JEDB;
import java.util.*;

public class Jsql
{
    static
    {
        System.loadLibrary("jedb");
    }

    static final int QUERY_MAX = 9206;
    static final int MAX_HEADLINE = 80;
    static final int WCHAR_SIZE = 2;

    static long start_time = 0;
    static long end_time = 0;

    static int print_resultset(int connid, int hstmt, int rowCnt)
    {
        int i, j, fieldCnt, ret;
        int querytype;

        querytype = JEDB.QueryGetQueryType(connid, hstmt);
        if (rowCnt == 0)
        {
            if (querytype == JEDB.SQL_STMT_SELECT)
            {
                System.out.println("no rows selected");
            }
        }
        else
        {
            int fieldtype;
            int length;
            String exp;
            String fieldname;

            fieldCnt = JEDB.QueryGetFieldCount(connid, hstmt);
            for(i = 0; i < fieldCnt; i++)
            {
                fieldtype = JEDB.QueryGetFieldType(connid, hstmt, i);
                length = JEDB.QueryGetFieldLength(connid, hstmt, i);
                if (fieldtype == JEDB.SQL_DATA_NCHAR
                        || fieldtype == JEDB.SQL_DATA_NVARCHAR)
                {
                    length *= WCHAR_SIZE;
                }
                length = length < MAX_HEADLINE ? length : MAX_HEADLINE;
                if (fieldtype < JEDB.SQL_DATA_CHAR)
                {
                    exp = "%" + length + "s";
                }
                else
                {
                    exp = "%-" + length + "s";
                }
                fieldname = JEDB.QueryGetFieldName(connid, hstmt, i);
                if (fieldname.length() > length)
                {
                    fieldname = fieldname.substring(0, length);
                }
                System.out.printf(exp, fieldname);
                if (i+1 < fieldCnt)
                {
                    System.out.print(" ");
                }
            }
            System.out.println("");

            for(i = 0; i < fieldCnt; i++)
            {
                fieldtype = JEDB.QueryGetFieldType(connid, hstmt, i);
                length = JEDB.QueryGetFieldLength(connid, hstmt, i);
                if (fieldtype == JEDB.SQL_DATA_NCHAR
                        || fieldtype == JEDB.SQL_DATA_NVARCHAR)
                {
                    length *= WCHAR_SIZE;
                }
                length = length < MAX_HEADLINE ? length : MAX_HEADLINE;

                for (j = 0; j < length; j++)
                {
                    System.out.print("-");
                }
                if (i+1 < fieldCnt)
                {
                    System.out.print(" ");
                }
            }
            System.out.println("");

            for(i = 0; i < rowCnt; i++)
            {
                ret = JEDB.QueryGetNextRow(connid, hstmt);
                if (ret < 0)
                {
                    System.out.println("Error: " + ret);
                    return (ret);
                }

                for(j = 0; j < fieldCnt; j++)
                {
                    fieldtype =
                        JEDB.QueryGetFieldType(connid, hstmt, j);
                    length = JEDB.QueryGetFieldLength(connid, hstmt, j);
                    if (fieldtype == JEDB.SQL_DATA_NCHAR
                            || fieldtype == JEDB.SQL_DATA_NVARCHAR)
                    {
                        length *= WCHAR_SIZE;
                    }
                    length = length < MAX_HEADLINE ? length : MAX_HEADLINE;
                    if (fieldtype < JEDB.SQL_DATA_CHAR)
                    {
                        exp = "%" + length + "s";
                    }
                    else
                    {
                        exp = "%-" + length + "s";
                    }
                    if (JEDB.QueryIsFieldNull(connid, hstmt, j) == 1)
                    {
                        System.out.printf(exp, "");
                    }
                    else
                    {
                        System.out.printf(exp,
                                JEDB.QueryGetFieldData(connid, hstmt,
                                    j));
                    }
                    if (j+1 < fieldCnt)
                    {
                        System.out.print(" ");
                    }

                }
                System.out.println("");
            }

            if ((querytype == JEDB.SQL_STMT_SELECT && rowCnt > 1) ||
                    (querytype == JEDB.SQL_STMT_DESCRIBE && fieldCnt > 1))
            {
                System.out.println("");
                System.out.println(rowCnt + " rows selected");
            }

            JEDB.QueryClearRow(connid, hstmt);
        }

        return (0);
    }

    static int print_result(int connid, int hstmt, int rowCnt)
    {
        int querytype;
        int ret = 0;

        System.out.println("");
        querytype = JEDB.QueryGetQueryType(connid, hstmt);

        switch(querytype)
        {
            case JEDB.SQL_STMT_SELECT:
                ret = print_resultset(connid, hstmt, rowCnt);
                break;
            case JEDB.SQL_STMT_INSERT:
                System.out.println(rowCnt + " row(s) created.");
                break;
            case JEDB.SQL_STMT_UPSERT:
                switch(rowCnt)
                {
                    case 0:
                        System.out.println("1 row exists.");
                        break;
                    case 1:
                        System.out.println("1 row inserted.");
                        break;
                    case 2:
                        System.out.println("1 row updated.");
                        break;
                }
                break;
            case JEDB.SQL_STMT_UPDATE:
                System.out.println(rowCnt + " row(s) updated.");
                break;
            case JEDB.SQL_STMT_DELETE:
                System.out.println(rowCnt + " row(s) deleted.");
                break;
            case JEDB.SQL_STMT_CREATE:
                System.out.println("Object created.");
                break;
            case JEDB.SQL_STMT_DROP:
                System.out.println("Object dropped.");
                break;
            case JEDB.SQL_STMT_RENAME:
                System.out.println("Object renamed.");
                break;
            case JEDB.SQL_STMT_ALTER:
                System.out.println("Object altered.");
                break;
            case JEDB.SQL_STMT_COMMIT:
                System.out.println("Commit complete.");
                break;
            case JEDB.SQL_STMT_ROLLBACK:
                System.out.println("Rollback complete.");
                break;
            case JEDB.SQL_STMT_DESCRIBE:
                print_resultset(connid, hstmt, rowCnt);
                break;
            case JEDB.SQL_STMT_SET:
                System.out.println("Set complete.");
                break;
            case JEDB.SQL_STMT_TRUNCATE:
                System.out.println("Truncate complete.");
                break;
            case JEDB.SQL_STMT_ADMIN:
                System.out.println("Admin Statement complete.");
                break;
            default:
                System.out.println("Inappropriate input");
                if (querytype == JEDB.SQL_STMT_NONE)
                {
                    System.out.println("(no query).");
                }
                else
                {
                    System.out.println("(" + querytype + ").");
                }
                break;
        }

        return (ret);
    }

    static class Query_str {
        String str;
        String next_query = null;
        int is_exception = 0;
    }

    static int is_procedure_in_query(char ch, int step)
    {
        switch(step)
        {
            case 0:
                if (ch == '\'') step = 1;
                else if (ch == '"') step = 2;
                break;
            case 1:
                if (ch == '\'') step = 0;
                break;
            case 2:
                if (ch == '"') step = 0;
                break;
            default:
                step = 0;
                break;
        }

        return (step);
    }

    static void display_timeinterval()
    {
        long run_time = end_time - start_time;
        long sec = run_time / 1000;
        System.out.print("Elapsed time: ");
        System.out.printf("%02d:%02d:%02d.%03d\n",
                sec/3600, (sec/60)%60, sec%60, run_time%1000);
        System.out.println("");
    }

    static void start_time()
    {
        start_time = System.currentTimeMillis(); 
    }

    static void end_time()
    {
        end_time = System.currentTimeMillis(); 

        display_timeinterval();
    }

    static int find_command(String str)
    {
        if (str.toLowerCase().equals("exit")
                || str.toLowerCase().equals("quit"))
        {
            return (-1);
        }
        else if (str.toLowerCase().equals("ctrl-c"))
        {
            System.out.println("Program is terminated~~!");
            System.exit(0);
        }
        else if (str.toLowerCase().equals("help"))
        {
            System.out.println("");
            System.out.println("List of all commands:");
            System.out.println("exit           : Exit Jsql.");
            System.out.println("quit           : Exit Jsql.");
            System.out.println("ctrl-c         : Terminate Jsql.");
            System.out.println("help           : Display this help.");
            System.out.println("start-time(st) : Set time.");
            System.out.println("end-time(et)   : End time.");
            System.out.println("");

            return (0);
        }
        else if (str.toLowerCase().equals("start-time")
                || str.toLowerCase().equals("st"))
        {
            start_time(); 
            return (0);
        }
        else if (str.toLowerCase().equals("end-time")
                || str.toLowerCase().equals("et"))
        {
            end_time(); 
            return (0);
        }

        return (1);
    }

    static int read_querystr(Query_str query, Scanner scanner)
    {
        String buffer;
        int     ret = 0;
        int     i = 0;
        int     first_pos = -1;
        int     step = 0;
        char    ch;
        int     is_end_mark = 0;

        if (query.next_query != null)
        {
            query.str = query.next_query;
        }
        else
        {
            try
            {
                while(true)
                {
                    if (first_pos == -1)
                    {
                        System.out.print("Jsql> ");
                    }
                    else
                    {
                        System.out.print("   -> ");
                    }

                    buffer = scanner.nextLine();

                    if (first_pos == -1)
                    {
                        if (buffer.length() == 0)
                        {
                            continue;
                        }

                        ret = find_command(buffer);
                        if (ret < 1)
                        {
                            return (ret);
                        }

                        query.str = buffer;
                    }
                    else
                    {
                        query.str += "\n" + buffer;
                    }

                    if (query.str.length() > QUERY_MAX)
                    {
                        return (-2);
                    }

                    is_end_mark = 0;

                    for (i = 0; i < buffer.length(); ++i)
                    {
                        ch = buffer.charAt(i);
                        if (first_pos == -1)
                        {
                            if (Character.isWhitespace(ch) == false)
                            {
                                first_pos = i;
                            }
                        }

                        step = is_procedure_in_query(ch, step);
                    };

                    if (first_pos > -1 && step == 0
                            && buffer.lastIndexOf(";") > -1)
                    {
                        for (i = buffer.length()-1; i >= 0; --i)
                        {
                            ch = buffer.charAt(i);
                            if (Character.isWhitespace(ch) == true)
                            {
                                continue;
                            }

                            if (ch == ';')
                            {
                                is_end_mark = 1;
                            }
                            break;
                        }

                        if (is_end_mark == 1)
                        {
                            break;
                        }
                    }
                }
            }
            catch(Exception ee)
            {
                query.is_exception = 1;
                if (query.next_query == null)
                {
                    return (-1);
                }
            }
        }

        first_pos = -1;
        for (i=0; i < query.str.length(); ++i)
        {
            ch = query.str.charAt(i);
            step = is_procedure_in_query(ch, step);

            if (ch == ';')
            {
                if (step == 0)
                {
                    break;
                }
            }

            if (first_pos == -1)
            {
                if (Character.isWhitespace(ch) == false)
                {
                    first_pos = i;
                }
            }
        }

        if (i < query.str.length())
        {
            query.next_query = query.str.substring(i+1, query.str.length());
            query.str = query.str.substring(first_pos, i);
        }
        else
        {
            query.next_query = null;
            if (query.is_exception == 1)
            {
                return (-1);
            }
        }

        if (first_pos == - 1)
        {
            ret = 0;
        }
        else
        {
            ret = query.str.length();
        }

        return (ret);
    }

    static int set_option(int connid)
    {
        int ret;
        int hstmt;

        hstmt = JEDB.QueryCreate(connid, "SET AUTOCOMMIT ON;", 0);
        if (hstmt < 0)
        {
            return (hstmt);
        }

        ret = JEDB.QueryExecute(connid, hstmt);
        if (ret < 0)
        {
            return (ret);
        }

        ret = JEDB.QueryDestroy(connid, hstmt);
        if (ret < 0)
        {
            return (ret);
        }

        return (0);
    }

    public static void main(String[] args)
    {
        int connid;
        int hstmt;
        int ret;
        String dbname;
        Query_str query = new Query_str();

        if (args.length != 1)
        {
            System.out.println("Usage: Jsql database");
            return;
        }

        System.out.println("");
        System.out.println("Jsql for openML Ver 0.1.0.0");
        System.out.println("Copyright (C) 2012 " +
                "INERVIT Co., Ltd. All rights reserved.");
        System.out.println("");

        dbname = args[0];

        connid = JEDB.GetConnection();

        Scanner scanner = new Scanner(System.in);

        ret = JEDB.Connect(connid, dbname);
        if (ret < 0)
        {
            System.out.println("");
            System.out.println("Error: " + ret);
            System.out.println("");
            System.out.println("");
            System.out.println("Do you want to connect another database?");
            System.out.println("No? Just forget it, type Ctrl+C.");
            System.out.print("Let me know your new database name: ");

            dbname = scanner.nextLine();
            if (dbname.length() < 2)
            {
                scanner.close();
                JEDB.ReleaseConnection(connid);
                return;
            }

            JEDB.CreateDB1(dbname);

            ret = JEDB.Connect(connid, dbname);
            if (ret < 0)
            {
                System.out.println("");
                System.out.println("Error: " + ret);
                scanner.close();
                JEDB.ReleaseConnection(connid);
                return;
            }
        }

        ret = set_option(connid);
        if (ret < 0)
        {
            System.out.println("Error: " + ret);
            JEDB.Disconnect(connid);
            scanner.close();
            JEDB.ReleaseConnection(connid);
            return;
        }

        while(true)
        {
            ret = read_querystr(query, scanner);

            if (ret == 0)
            {
                continue;
            }
            else if (ret == -1)
            {
                System.out.println("Bye~~!");
                break;
            }
            else if (ret == -2)
            {
                System.out.println("");
                System.out.println("ERROR: A query size is too large.");
                System.out.println("");
                continue;
            }

            System.out.println(query.str);

            hstmt = JEDB.QueryCreate(connid, query.str, 0);
            if (hstmt < 0)
            {
                System.out.println("");
                System.out.println("ERROR: " + hstmt);
                System.out.println("");
                continue;
            }

            ret = JEDB.QueryExecute(connid, hstmt);
            if (ret < 0)
            {
                System.out.println("");
                System.out.println("ERROR: " + ret);
                System.out.println("");
                JEDB.QueryDestroy(connid, hstmt);
                continue;
            }

            print_result(connid, hstmt, ret);

            System.out.println("");

            ret = JEDB.QueryDestroy(connid, hstmt);
            if (ret < 0)
            {
                System.out.println("");
                System.out.println("ERROR: " + ret);
                System.out.println("");
                continue;
            }
        }

        JEDB.Disconnect(connid);
        scanner.close();
        JEDB.ReleaseConnection(connid);
    }
}
