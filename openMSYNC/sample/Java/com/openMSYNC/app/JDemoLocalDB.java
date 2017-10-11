  package com.openMSYNC.app;
 

import java.lang.String;
import com.openml.JEDB;


class DemoRecord
{
    int  ID;
    String username;
    String Company;
    String Name;
    String e_mail;
    String Company_phone;
    String Mobile_phone;
    String Address;
    String mTime;
    
    DemoRecord()
    {
    	 ID = 0;
    	 username = "";
		 Company = "";
		 Name = "";
		 e_mail = "";
		 Company_phone = "";
		 Mobile_phone = "";
		 Address = "";
		 mTime = "";
	}
}

class JDemoLocalDB
{
	static {
		System.loadLibrary("openml");
    	System.loadLibrary("jedb");
	}	

	class LDB_Result{
		int  _hstmt;
		int  _nRows;
		int  _nCurRow;
		int  _nFields;
		
		LDB_Result()
		{
			_hstmt   = -1;
			_nRows   = 0;
			_nCurRow = 0;
			_nFields = 0;			
		}
	}
	
	static int g_hconnid = -1;
	static LDB_Result g_Result = null;	
	/******************************************************************************************/	
	/******************************************************************************************/
	
	
	static final int QUERY_MAX = 9206;
    static final int MAX_HEADLINE = 80;
    static final int WCHAR_SIZE = 2;

   
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
                    case 10:
                        System.out.print("1 row exists. "
                                + "(upserted before)");
                        break;
                    case 11:
                        System.out.println("1 row inserted. "
                                + "(upserted before)");
                        break;
                    case 12:
                        System.out.println("1 row updated. "
                                + "(upserted before)");
                        break;
                }
                break;
				/*
            case JEDB.SQL_STMT_TOUCH:
                System.out.println(rowCnt + " row(s) touched.");
                break;
				*/
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
                if (rowCnt > 0)
                {
                    System.out.println("Admin Statement complete("
                            + rowCnt + " object(s) affected).");
                }
                else
                {
                    System.out.println("Admin Statement complete.");
                }
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

    
/******************************************************************************************/
    
    int create_sample_table()
    {
    	int hstmt;
    	int ret;    	
    	String  szContact_schema = "CREATE msync TABLE contact( " 
	            + "ID int, "
    			+ "username varchar(32),"
                + "Company varchar(255), "
                + "name varchar(255), "
                + "e_mail varchar(255), "
                + "Company_phone char(20), "
                + "Mobile_phone char(20), "
                + "Address varchar(255), "
                + "mTime DateTime,"
                + "primary key(ID, username) )";
    	
    	hstmt = JEDB.QueryCreate(g_hconnid, szContact_schema, 0);
    	if (hstmt < 0)
		{          
		    System.out.println("ERROR: QueryCreate() ..." + hstmt);          
		    return -1;
		}
    	
    	ret = JEDB.QueryExecute(g_hconnid, hstmt);
    	if (ret < 0)
		{          
		    System.out.println("ERROR: QueryExecute() ..." + ret);
		    JEDB.QueryDestroy(g_hconnid, hstmt);
		    return -1;
		}
    	
    	JEDB.Commit(g_hconnid);
    	JEDB.QueryDestroy(g_hconnid, hstmt);
    	
    	return 0;
    }
    
    
	
	int InitLocalDB(String dbname)
	{
		int ret;
		
		g_hconnid = JEDB.GetConnection();
		ret = JEDB.Connect(g_hconnid, dbname);
        if (ret < 0)
        {            
            ret = JEDB.CreateDB1(dbname);
            if( ret < 0 )
            {
            	System.out.println("ERROR: createdb() ..." + ret );
            	JEDB.ReleaseConnection(g_hconnid);
            	return -1;
            }
            
            ret = JEDB.Connect(g_hconnid, dbname);
            if (ret < 0)
            {                
                System.out.println("ERROR: Connect() ..." + ret);
                JEDB.ReleaseConnection(g_hconnid);               
                return -1;
            }
            
            ret = create_sample_table();
        }
        
        return 0;
	}
	
	void QuitLocalDB()
	{    
	    if( g_Result != null )
		{
			JEDB.QueryDestroy(g_hconnid, g_Result._hstmt);
			g_Result = null;
		}
		
		JEDB.Commit(g_hconnid);		
	}
	
	int GetMaxID(String userName)
	{
	    int			 nID = -999;
	    int hstmt;
	    int ret;
	   	   
	    hstmt = JEDB.QueryCreate(g_hconnid, "select max(id) from contact where username = '" + userName + "'" , 0);	    
	    if (hstmt < 0)
	    {
	    	System.out.println("");
	        System.out.println("ERROR: " + hstmt);
	        System.out.println("");	    	
	        return -1;
	    }
	
	    ret = JEDB.QueryExecute(g_hconnid, hstmt);
	    if (ret < 0)
	    {
	    	System.out.println("");
	        System.out.println("ERROR: " + ret);
	        System.out.println("");
	        
	        JEDB.QueryDestroy(g_hconnid, hstmt);
	        return -1;
	    }
	    else if( ret == 0 )
	    	nID = 0;
	    else
	    {	
	    	ret = JEDB.QueryGetNextRow(g_hconnid, hstmt);
	        if (ret < 0)
	        {	            
	        	System.out.println("");
		        System.out.println("ERROR: " + ret);
		        System.out.println("");
	            return (ret);
	        }
	        
	        String s = JEDB.QueryGetFieldData(g_hconnid, hstmt, 0);
	        if( s == null || s.isEmpty() )
	        	nID = -9999;
	        else
	        	nID = Integer.parseInt( s );
	        //nID = Integer.parseInt( JEDB.QueryGetFieldData(g_hconnid, hstmt, 0) );
	        nID++;
	    }
		
	    JEDB.Commit(g_hconnid);	    
	    JEDB.QueryDestroy(g_hconnid, hstmt);
	    
	    return nID;    
	}
	
	int Query2LocalDB(String query)
	{	    
	    int hstmt;
	    int ret;
	            
	    hstmt = JEDB.QueryCreate(g_hconnid, query, 0);
	    if (hstmt < 0)
	    {
	        System.out.println("");
	        System.out.println("ERROR: " + hstmt);
	        System.out.println("");
	        return -1;
	    }
	
	    ret = JEDB.QueryExecute(g_hconnid, hstmt);
	    if (ret < 0)
	    {
	        System.out.println("");
	        System.out.println("ERROR: " + ret);
	        System.out.println("");
	        JEDB.QueryDestroy(g_hconnid, hstmt);
	        return -1;
	    }
	
	    print_result(g_hconnid, hstmt, ret);
	
	    System.out.println("");
	
	    JEDB.Commit(g_hconnid);
	    
	    ret = JEDB.QueryDestroy(g_hconnid, hstmt);
	    if (ret < 0)
	    {
	        System.out.println("");
	        System.out.println("ERROR: " + ret);
	        System.out.println("");
	        return -1;
	    }
	    
	    return 0;
	}
	
	int Show_LocalDB() 
	{
		int    ret;
		int    step;
		int    hstmt;
		String QueryBody = "select id, username, company, name, e_mail, company_phone, mobile_phone, address, mTime from contact";
		String Query;

		String[] QryOpt = new String[]{"", "INSERT_RECORD", "UPDATE_RECORD", "DELETE_RECORD",  "SYNCED_RECORD"};

		
		for( step = 0; step < 5; step++ )
		{
			Query = QueryBody + " " + QryOpt[step];
			
			System.out.println(Query);
			
			hstmt = JEDB.QueryCreate(g_hconnid, Query, 0);
			if (hstmt < 0)
			{          
			    System.out.println("ERROR: QueryCreate() ..." + hstmt);          
			    return -1;
			}
			
			ret = JEDB.QueryExecute(g_hconnid, hstmt);
			if (ret < 0)
			{             
			    System.out.println("ERROR: QueryExecute() ..." + ret);          
			    JEDB.QueryDestroy(g_hconnid, hstmt);
			    return -1;
			}
			
			if( ret == 0 )
				continue;
			
			System.out.println("");
			if( step == 0 )
			{
				System.out.println("*************************************************************");
				System.out.println("*** LocalDB...");
			}
			else if( step == 1 )
	           System.out.println("*** LocalDB(INSERT)...");        
	       else if( step == 2 )
	           System.out.println("*** LocalDB(UPDATE)...");        
	       else if( step == 3 )
	           System.out.println("*** LocalDB(DELETE)...");        
	       else if( step == 4 )
	           System.out.println("*** LocalDB(SYNCED)...");      
	       	
			ret = print_result(g_hconnid, hstmt, ret);
			   
			JEDB.Commit(g_hconnid);
			ret = JEDB.QueryDestroy(g_hconnid, hstmt);
			if (ret < 0)
			{
			     System.out.println("ERROR: QueryDestroy() ..." + ret);         
			     return -1;
			}			
		}
		
		return 0;		
	}	
	
	int SelectLocalDB(String Query)
	{	
		g_Result = null; /* clear */
		g_Result = new LDB_Result();		
		
		g_Result._hstmt = JEDB.QueryCreate(g_hconnid, Query, 0);
		if (g_Result._hstmt < 0)
		{          
		    System.out.println("ERROR: QueryCreate() ..." + g_Result._hstmt);   
		    g_Result = null;
		    return -1;
		}
		
		g_Result._nRows = JEDB.QueryExecute(g_hconnid, g_Result._hstmt);
		if (g_Result._nRows < 0)
		{             
		    System.out.println("ERROR: QueryExecute() ..." + g_Result._nRows);          
		    JEDB.QueryDestroy(g_hconnid, g_Result._hstmt);
		    g_Result = null;
		    return -1;
		}
		
		g_Result._nFields = JEDB.QueryGetFieldCount(g_hconnid, g_Result._hstmt);
		if( g_Result._nRows == 0 )
			JEDB.QueryDestroy(g_hconnid, g_Result._hstmt);	
		
	    return g_Result._nRows;
	}
	
	int FetchLocalDB(DemoRecord  Record)	
	{
		int ret, j;
		String fieldname;
	  
		if( g_Result._nCurRow >= g_Result._nRows )
			return 0;	/* No more record */	
				
		ret = JEDB.QueryGetNextRow(g_hconnid, g_Result._hstmt);
        if (ret < 0)
        {
            System.out.println("Error: " + ret);
            return (ret);
        }

        for(j = 0; j < g_Result._nFields; j++)
        {
        	fieldname = JEDB.QueryGetFieldName(g_hconnid, g_Result._hstmt, j);        	
                        
            
            fieldname = fieldname.toLowerCase();
            
            if( fieldname.equalsIgnoreCase("id") )
	        { /* Not null field! */
            	Record.ID = Integer.parseInt( JEDB.QueryGetFieldData(g_hconnid, g_Result._hstmt, j) );            	    
	        }
            else if( fieldname.equalsIgnoreCase("username") )
	        { /* Not null field! */
            	Record.username =  JEDB.QueryGetFieldData(g_hconnid, g_Result._hstmt, j);            	    
	        }
	        else if( fieldname.equalsIgnoreCase("Company") )
	        {
	        	if (JEDB.QueryIsFieldNull(g_hconnid, g_Result._hstmt, j) == 1)
	        		Record.Company = "";	        	
	            else
	            	Record.Company = JEDB.QueryGetFieldData(g_hconnid, g_Result._hstmt, j);
	        }
	        else if( fieldname.equalsIgnoreCase("name") )
	        {
	        	if (JEDB.QueryIsFieldNull(g_hconnid, g_Result._hstmt, j) == 1)
	        		Record.Name = "";	        	
	            else
	            	Record.Name = JEDB.QueryGetFieldData(g_hconnid, g_Result._hstmt, j);	            	
	        }
	        else if( fieldname.equalsIgnoreCase("e_mail") )
	        {
	        	if (JEDB.QueryIsFieldNull(g_hconnid, g_Result._hstmt, j) == 1)
	        		Record.e_mail = "";	        	
	            else
	            	Record.e_mail = JEDB.QueryGetFieldData(g_hconnid, g_Result._hstmt, j);	            
	        }
	        else if( fieldname.equalsIgnoreCase( "Company_phone") )
	        {
	        	if (JEDB.QueryIsFieldNull(g_hconnid, g_Result._hstmt, j) == 1)
	        		Record.Company_phone = "";	        	
	            else
	            	Record.Company_phone = JEDB.QueryGetFieldData(g_hconnid, g_Result._hstmt, j);	            
	        }
	        else if( fieldname.equalsIgnoreCase("Mobile_phone") )
	        {
	        	if (JEDB.QueryIsFieldNull(g_hconnid, g_Result._hstmt, j) == 1)
	        		Record.Mobile_phone = "";	        	
	            else
	            	Record.Mobile_phone = JEDB.QueryGetFieldData(g_hconnid, g_Result._hstmt, j);	            
	        }
	        else if( fieldname.equalsIgnoreCase("Address") )
	        {
	        	if (JEDB.QueryIsFieldNull(g_hconnid, g_Result._hstmt, j) == 1)
	        		Record.Address = "";	        	
	            else
	            	Record.Address = JEDB.QueryGetFieldData(g_hconnid, g_Result._hstmt, j);	            
	        }
	        else if( fieldname.equalsIgnoreCase("mTime") )
	        {
	        	Record.mTime = JEDB.QueryGetFieldData(g_hconnid, g_Result._hstmt, j);	        	
	        }
	        else 
	        {
	            return -2; /* unknown field */
	        }
        }
        
        return 1; /* continue */       
	}

	
	void QuitSelectLocalDB()
	{    
		JEDB.QueryDestroy(g_hconnid, g_Result._hstmt);
		JEDB.Commit(g_hconnid);	   
		
		g_Result = null;
	}

	
} /* class JDemoLocalDB */



