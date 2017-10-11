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

package com.openml.android;

import com.openml.JEDB;
import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;
import android.widget.EditText;
import android.widget.Button;

public class Jsql extends Activity implements OnClickListener {

    static {
    	 System.loadLibrary("openml");
        System.loadLibrary("jedb");        
    }
    
    static int connid = -1;
    static String path = "/mnt/sdcard/Android/data/com.openml.android.Jsql/";
    static String dbname;
    private Button[] mButton = new Button[5];
    static final int QUERY_MAX = 9206;
    static final int MAX_HEADLINE = 80;
    static final int WCHAR_SIZE = 2;

    @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            setContentView(R.layout.activity_jsql);

            mButton[0] = (Button) findViewById(R.id.createButton);
            mButton[0].setOnClickListener(this);
            mButton[1] = (Button) findViewById(R.id.dropButton);
            mButton[1].setOnClickListener(this);
            mButton[2] = (Button) findViewById(R.id.connectButton);
            mButton[2].setOnClickListener(this);
            mButton[3] = (Button) findViewById(R.id.disconnectButton);
            mButton[3].setOnClickListener(this);
            mButton[4] = (Button) findViewById(R.id.queryButton);
            mButton[4].setOnClickListener(this);

            if (connid > -1)
            {
                EditText  et_dbname =
                    (EditText) findViewById(R.id.editText_dbname);
                EditText  et_query =
                    (EditText) findViewById(R.id.editText_query);

                et_dbname.setEnabled(false);
                mButton[0].setEnabled(false);
                mButton[0].setFocusable(false);
                mButton[1].setEnabled(false);
                mButton[1].setFocusable(false);
                mButton[2].setEnabled(false);
                mButton[2].setFocusable(false);
                mButton[3].setEnabled(true);
                mButton[3].setFocusable(true);
                et_query.setEnabled(true);
                mButton[4].setEnabled(true);
                mButton[4].setFocusable(true);
            }
        }


    @Override
        public boolean onCreateOptionsMenu(Menu menu) {
            getMenuInflater().inflate(R.menu.activity_jsql, menu);
            return true;
        }


    public void onClick(View v)    {
        int ret = 0;

        EditText  et_dbname = (EditText) findViewById(R.id.editText_dbname);
        EditText  et_query = (EditText) findViewById(R.id.editText_query);
        TextView  tv_status = (TextView) findViewById(R.id.textView_status);


        tv_status.setText("");


        if (et_dbname.getText().length() < 1)
        {
            tv_status.setText("Error : Invalid DB name");
            return;
        }

        switch(v.getId())
        {
            case R.id.createButton:
                ret = dbCreate(path + et_dbname.getText().toString());
                break;
            case R.id.dropButton:
                ret = dbDrop(path + et_dbname.getText().toString());
                break;
            case R.id.connectButton:
                ret = dbConnect(path + et_dbname.getText().toString());
                break;
            case R.id.disconnectButton:
                ret = dbDisconnect();
                break;
            case R.id.queryButton:
                if (et_query.getText().length() < 1)
                {
                    tv_status.setText("Error : Invalid query string");
                    return;
                }
                ret = dbQuery(et_query.getText().toString());
                break;
            default:
                return;
        }

        if (ret < 0)
        {
            tv_status.setText("Error : " + ret);
            return;
        }

        switch(v.getId())
        {
            case R.id.createButton:
                tv_status.setText("DB Created.");
                break;
            case R.id.dropButton:
                tv_status.setText("DB Dropped");
                break;
            case R.id.connectButton:
                et_dbname.setEnabled(false);
                mButton[0].setEnabled(false);
                mButton[0].setFocusable(false);
                mButton[1].setEnabled(false);
                mButton[1].setFocusable(false);
                mButton[2].setEnabled(false);
                mButton[2].setFocusable(false);
                mButton[3].setEnabled(true);
                mButton[3].setFocusable(true);
                et_query.setEnabled(true);
                mButton[4].setEnabled(true);
                mButton[4].setFocusable(true);
                tv_status.setText("DB Connected.");
                break;
            case R.id.disconnectButton:
                TableLayout tl = (TableLayout)findViewById(R.id.result_field);
                tl.removeAllViews();
                tl = (TableLayout)findViewById(R.id.result_list);
                tl.removeAllViews();
                et_dbname.setEnabled(true);
                mButton[0].setEnabled(true);
                mButton[0].setFocusable(true);
                mButton[1].setEnabled(true);
                mButton[1].setFocusable(true);
                mButton[2].setEnabled(true);
                mButton[2].setFocusable(true);
                mButton[3].setEnabled(false);
                mButton[3].setFocusable(false);
                et_query.setText("");
                et_query.setEnabled(false);
                mButton[4].setEnabled(false);
                mButton[4].setFocusable(false);
                tv_status.setText("DB Disconnected.");
                break;
            case R.id.queryButton:
                break;
        }
    }

    public int dbCreate(String dbname)
    {
        int ret;

        ret = JEDB.CreateDB1(dbname);

        return ret;
    }

    public int dbDrop(String dbname)
    {
        int ret;

        ret = JEDB.DestoryDB(dbname);

        return ret;
    }

    public int dbConnect(String dbname)
    {
        int ret;
        int hstmt;

        connid = JEDB.GetConnection();
        if (connid < 0)
        {
            return connid;
        }

        ret = JEDB.Connect(connid, dbname);
        if (ret < 0)
        {
            return ret;
        }

        hstmt = JEDB.QueryCreate(connid, "SET AUTOCOMMIT ON;", 0);
        if (hstmt < 0)
        {
            return (hstmt);
        }

        ret = JEDB.QueryExecute(connid, hstmt);
        if (ret < 0)
        {
            JEDB.QueryDestroy(connid, hstmt);
            return (ret);
        }

        ret = JEDB.QueryDestroy(connid, hstmt);
        if (ret < 0)
        {
            return (ret);
        }

        ret = connid;

        return ret;
    }

    public int dbDisconnect()
    {
        JEDB.Disconnect(connid);
        JEDB.ReleaseConnection(connid);
        connid = -1;

        return 0;
    }

    public int dbQuery(String query)
    {
        int ret;
        int hstmt;

        hstmt = JEDB.QueryCreate(connid, query, 0);
        if (hstmt < 0)
        {
            return hstmt;
        }

        ret = JEDB.QueryExecute(connid, hstmt);
        if (ret < 0)
        {
            JEDB.QueryDestroy(connid, hstmt);
            return ret;
        }

        ret = print_result(hstmt, ret);
        if (ret < 0)
        {
            JEDB.QueryDestroy(connid, hstmt);
            return ret;
        }

        ret = JEDB.QueryDestroy(connid, hstmt);
        if (ret < 0)
        {
            return ret;
        }

        return ret;
    }

    public int print_result(int hstmt, int rowCnt)
    {
        int ret = 0;
        TextView  tv_status = (TextView) findViewById(R.id.textView_status);
        int querytype = JEDB.QueryGetQueryType(connid, hstmt);

        switch(querytype)
        {
            case JEDB.SQL_STMT_SELECT:
                ret = print_resultset(hstmt, rowCnt, querytype);
                break;
            case JEDB.SQL_STMT_INSERT:
                tv_status.setText(rowCnt + " row(s) created.");
                break;
            case JEDB.SQL_STMT_UPSERT:
                switch(rowCnt)
                {
                    case 0:
                        tv_status.setText("1 row exists.");
                        break;
                    case 1:
                        tv_status.setText("1 row inserted.");
                        break;
                    case 2:
                        tv_status.setText("1 row updated.");
                        break;
                }
                break;
            case JEDB.SQL_STMT_UPDATE:
                tv_status.setText(rowCnt + " row(s) updated.");
                break;
            case JEDB.SQL_STMT_DELETE:
                tv_status.setText(rowCnt + " row(s) deleted.");
                break;
            case JEDB.SQL_STMT_CREATE:
                tv_status.setText("Object created.");
                break;
            case JEDB.SQL_STMT_DROP:
                tv_status.setText("Object dropped.");
                break;
            case JEDB.SQL_STMT_RENAME:
                tv_status.setText("Object renamed.");
                break;
            case JEDB.SQL_STMT_ALTER:
                tv_status.setText("Object altered.");
                break;
            case JEDB.SQL_STMT_COMMIT:
                tv_status.setText("Commit complete.");
                break;
            case JEDB.SQL_STMT_ROLLBACK:
                tv_status.setText("Rollback complete.");
                break;
            case JEDB.SQL_STMT_DESCRIBE:
                ret = print_resultset(hstmt, rowCnt, querytype);
                break;
            case JEDB.SQL_STMT_SET:
                tv_status.setText("Set complete.");
                break;
            case JEDB.SQL_STMT_TRUNCATE:
                tv_status.setText("Truncate complete.");
                break;
            case JEDB.SQL_STMT_ADMIN:
                tv_status.setText("Admin Statement complete.");
                break;
            default:
                if (querytype == JEDB.SQL_STMT_NONE)
                {
                    tv_status.setText("Inappropriate input : (no query).");
                }
                else
                {
                    tv_status.setText("Inappropriate input : ("
                            + querytype + ").");
                }
                break;
        }

        return ret;
    }

    public int print_resultset(int hstmt, int rowCnt, int querytype)
    {
        int ret = 0;
        TextView  tv_status = (TextView) findViewById(R.id.textView_status);
        if (rowCnt == 0)
        {
            if (querytype == JEDB.SQL_STMT_SELECT)
            {
                tv_status.setText("no rows selected.");
            }
        }
        else
        {
            int fieldtype;
            int fieldCnt;
            int length, i, j;
            String fieldname;

            TableLayout tl = (TableLayout)findViewById(R.id.result_field);
            tl.removeAllViewsInLayout();

            fieldCnt = JEDB.QueryGetFieldCount(connid, hstmt);

            TableRow tr = new TableRow(this);
            tr.setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT,
                        LayoutParams.WRAP_CONTENT));
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

                fieldname = JEDB.QueryGetFieldName(connid, hstmt, i);
                if (fieldname.length() > length)
                {
                    fieldname = fieldname.substring(0, length);
                }

                TextView b = new TextView(this);
                b.setTextSize(11);
                b.setWidth(11*length+10);
                b.setText(fieldname);
                b.setPadding(5, 5, 5, 5);
                b.setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT,
                            LayoutParams.WRAP_CONTENT));
                tr.addView(b,
                        new TableRow.LayoutParams(LayoutParams.WRAP_CONTENT,
                            LayoutParams.WRAP_CONTENT));    			
            }
            tl.addView(tr,
                    new TableLayout.LayoutParams(LayoutParams.MATCH_PARENT,
                        LayoutParams.WRAP_CONTENT));

            tl = (TableLayout)findViewById(R.id.result_list);

            tl.removeAllViewsInLayout();

            for(i = 0; i < rowCnt; i++)
            {
                ret = JEDB.QueryGetNextRow(connid, hstmt);
                if (ret < 0)
                {
                    return ret;
                }

                tr = new TableRow(this);
                tr.setLayoutParams(
                        new TableRow.LayoutParams(LayoutParams.WRAP_CONTENT,
                            LayoutParams.WRAP_CONTENT));

                for(j = 0; j < fieldCnt; j++)
                {
                    fieldtype = JEDB.QueryGetFieldType(connid, hstmt, j);
                    length = JEDB.QueryGetFieldLength(connid, hstmt, j);
                    if (fieldtype == JEDB.SQL_DATA_NCHAR
                            || fieldtype == JEDB.SQL_DATA_NVARCHAR)
                    {
                        length *= WCHAR_SIZE;
                    }
                    length = length < MAX_HEADLINE ? length : MAX_HEADLINE;

                    TextView b = new TextView(this);
                    b.setTextSize(11);
                    b.setWidth(11*length+10);
                    b.setText(JEDB.QueryGetFieldData(connid, hstmt, j));
                    b.setPadding(5, 5, 5, 5);
                    b.setLayoutParams(
                            new LayoutParams(LayoutParams.WRAP_CONTENT,
                                LayoutParams.WRAP_CONTENT));
                    tr.addView(b,
                            new TableRow.LayoutParams(LayoutParams.WRAP_CONTENT,
                                LayoutParams.WRAP_CONTENT));
                }
                tl.addView(tr,
                        new TableLayout.LayoutParams(LayoutParams.MATCH_PARENT,
                            LayoutParams.WRAP_CONTENT));

            }

            if (querytype == JEDB.SQL_STMT_SELECT ||
                    querytype == JEDB.SQL_STMT_DESCRIBE)
            {
                tv_status.setText(rowCnt + " rows selected.");
            }

            JEDB.QueryClearRow(connid, hstmt);
        }

        return ret;
    }
}

