package com.SyncDemo;
 

import java.lang.String;

import android.app.ActivityManager;
import android.content.Context;

import com.openml.JEDB;
import com.SyncDemo.*; 

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

class JSync2Local
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

	final static String g_DB_PATH = "/mnt/sdcard/Android/data/com.SyncDemo/";
	static int g_hconnid = -1;
	static LDB_Result g_Result = null;
	
	static String g_errMessage = "";	
    
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
    	
    	String  szidTbl_schema = "CREATE TABLE contactID( " 
	            + "maxid int NOT NULL, "
    			+ "username varchar(32)   )";    	
    	
    	hstmt = JEDB.QueryCreate(g_hconnid, szContact_schema, 0);
    	if (hstmt < 0)
		{   
    		g_errMessage = "ERROR: QueryCreate() ..." + hstmt;
		    return -1;
		}
    	
    	ret = JEDB.QueryExecute(g_hconnid, hstmt);
    	JEDB.QueryDestroy(g_hconnid, hstmt);
    	if (ret < 0)
		{   
		    g_errMessage = "ERROR: QueryExecute() ..." + ret;
		    JEDB.Rollback(g_hconnid);
		    return -1;
		}
    	    	
    	hstmt = JEDB.QueryCreate(g_hconnid, szidTbl_schema, 0);
    	if (hstmt < 0)
		{
    		g_errMessage = "ERROR: QueryCreate() ..." + hstmt;
    		JEDB.Rollback(g_hconnid);
		    return -1;
		}
    	
    	ret = JEDB.QueryExecute(g_hconnid, hstmt);
    	JEDB.QueryDestroy(g_hconnid, hstmt);
    	if (ret < 0)
		{          
		    g_errMessage = "ERROR: QueryExecute() ..." + ret;
		    JEDB.Rollback(g_hconnid);
		    return -1;
		}
    	
    	JEDB.Commit(g_hconnid);
    	
    	String tmp;
    	tmp = String.format("insert into contactID(maxid, username) values( 0, '%s')", PrefSetting.mUser);
    	return  Query2LocalDB(tmp, true);
    }
	
    int ReMakeDB(String dbname)
    {
    	QuitLocalDB();    	
    	JEDB.DestoryDB( g_DB_PATH + dbname );
    	
    	return InitLocalDB(dbname);    	
    }
    
	int InitLocalDB(String dbname)
	{
		int ret;
		
		g_errMessage = "";
		g_hconnid = JEDB.GetConnection();
		ret = JEDB.Connect(g_hconnid, g_DB_PATH + dbname);
        if (ret < 0)
        {   
            ret = JEDB.CreateDB1(g_DB_PATH + dbname);
            if( ret < 0 )
            {            	
            	//System.out.println("ERROR: createdb() ..." + ret );
            	g_errMessage = "ERROR: createdb() ..." + ret;
            	JEDB.ReleaseConnection(g_hconnid);
            	return -1;
            }
            
            ret = JEDB.Connect(g_hconnid, g_DB_PATH + dbname);
            if (ret < 0)
            {                
                //System.out.println("ERROR: Connect() ..." + ret);
                g_errMessage = "ERROR: Connect() ..." + ret;
                JEDB.ReleaseConnection(g_hconnid);               
                return -1;
            }
            
            return  create_sample_table();
        }
        
        return 0;
	}
	
	void QuitLocalDB()
	{
		if( g_hconnid >= 0)
		{
			JEDB.Disconnect(g_hconnid);	       
			JEDB.ReleaseConnection(g_hconnid);			
			g_hconnid = -1;
		}
	    
	    g_Result = null;
	}
	
	private int GetSubMaxID(String UserName)
	{
		int			 nID = -999;
	    int hstmt;
	    int ret;
	    String QBuf;
	    
	    QBuf = String.format("select max(id) from contact where username = '%s'", UserName);    
	    
	    hstmt = JEDB.QueryCreate(g_hconnid, QBuf, 0);	    
	    if (hstmt < 0)
	    {
	    	g_errMessage = "ERROR: QueryCreate() ..." + hstmt;
	        return -1;
	    }
	
	    ret = JEDB.QueryExecute(g_hconnid, hstmt);
	    if (ret < 0)
	    {
	        g_errMessage = "ERROR: QueryExecute() ..." + ret;
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
	        	g_errMessage = "ERROR: QueryGetNextRow() ..." + ret;
	            return (ret);
	        }
	        
	        String s = JEDB.QueryGetFieldData(g_hconnid, hstmt, 0);
	        if( s == null || s.isEmpty() )
	        	nID = 0;
	        else
	        {
	        	nID = Integer.parseInt( s );	        	
	        }
	    }
		
	    JEDB.Commit(g_hconnid);	    
	    JEDB.QueryDestroy(g_hconnid, hstmt);
	    
	    return nID;  		
	}
	
	
	int ResetMaxID()
	{
		int submaxid, nid;
				
		submaxid = GetSubMaxID(PrefSetting.mUser);
		nid = GetNextID();
		
		if( submaxid >= nid )
		{
			String tmp;
			
			nid = submaxid+1;			
        	tmp = String.format("update contactID set maxid = %d where username =  '%s'", 
        			nid, PrefSetting.mUser);	   
        	
        	submaxid = Query2LocalDB(tmp, true);   
        	
        	nid++; // next id
		}
		return nid;		
	}
	
	int GetNextID()
	{
	    int	nID = -999;
	    int subID;   
	    int hstmt;
	    int ret;
	    String QBuf;
	    String tmp;
	            
	    g_errMessage = "";
	  	     	    
	    QBuf = String.format("select maxid from contactID where username = '%s'", PrefSetting.mUser);
	    hstmt = JEDB.QueryCreate(g_hconnid, QBuf, 0);	    
	    if (hstmt < 0)
	    {
	    	g_errMessage = "ERROR: QueryCreate() ..." + hstmt;
	        return -1;
	    }
	
	    ret = JEDB.QueryExecute(g_hconnid, hstmt);
	    if (ret < 0)
	    {
	        g_errMessage = "ERROR: QueryExecute() ..." + ret;
	        JEDB.QueryDestroy(g_hconnid, hstmt);
	        return -1;
	    }
	    else if( ret == 0 )
	    {
	    	g_errMessage = "ERROR: No selected .. " + PrefSetting.mUser;
	    	JEDB.Commit(g_hconnid);	    
		    JEDB.QueryDestroy(g_hconnid, hstmt);
	    	return -1;
	    }	    	
	    else
	    {	
	    	ret = JEDB.QueryGetNextRow(g_hconnid, hstmt);
	        if (ret < 0)
	        {	            
	        	g_errMessage = "ERROR: QueryGetNextRow() ..." + ret;
	            return (ret);
	        }
	        
	        String s = JEDB.QueryGetFieldData(g_hconnid, hstmt, 0);
	        if( s == null || s.isEmpty() )
	        {
	        	g_errMessage = "ERROR: maxid is empty!!";
	        	nID = -1;	        	
	        }
	        else
	        {
	        	nID = Integer.parseInt( s );
	        	nID++;  
	        }
	    }
		
	    JEDB.Commit(g_hconnid);	    
	    JEDB.QueryDestroy(g_hconnid, hstmt);
	    
	    return nID;    
	}
			
	int Query2LocalDB(String query, boolean beCommit)
	{	    
	    int hstmt;
	    int ret;
	    int nRows;
	            
	    g_errMessage = "";
	    
	    hstmt = JEDB.QueryCreate(g_hconnid, query, 0);
	    if (hstmt < 0)
	    {
	    	g_errMessage = "ERROR: QueryCreate() ..." + hstmt;
	    	if( beCommit )
	        	JEDB.Rollback(g_hconnid);
	        return -1;
	    }
	
	    nRows = JEDB.QueryExecute(g_hconnid, hstmt);
	    if (nRows < 0)
	    {
	        g_errMessage = "ERROR: QueryExecute() ..." + nRows;
	        JEDB.QueryDestroy(g_hconnid, hstmt);
	        if( beCommit )
	        	JEDB.Rollback(g_hconnid);			        	
	        return -1;
	    }
	    
	    /*
        switch( JEDB.QueryGetQueryType(g_hconnid, hstmt) )
        {
            case JEDB.SQL_STMT_INSERT:                            
            case JEDB.SQL_STMT_UPDATE:                
            case JEDB.SQL_STMT_DELETE:            	
                break;
            case JEDB.SQL_STMT_UPSERT:
                switch(nRows)
                {
                    case 0: // "1 row exists." 
                        break;
                    case 1: // "1 row inserted." 
                        break;
                    case 2: // "1 row updated." 
                        break;                    
                }
                break;            	 
        }
       */
	    
	    JEDB.QueryDestroy(g_hconnid, hstmt);
	    
	    if( beCommit )
	    	JEDB.Commit(g_hconnid);
	    
	    return nRows;
	}
	
	void CommitLocalDB(boolean isCommit)
	{
		if( isCommit )
			JEDB.Commit(g_hconnid);
		else
			JEDB.Rollback(g_hconnid);		
	}
		
	int SelectLocalDB(String Query)
	{	
		g_errMessage =  "";
		g_Result = null; /* clear */
		g_Result = new LDB_Result();		
		
		g_Result._hstmt = JEDB.QueryCreate(g_hconnid, Query, 0);
		if (g_Result._hstmt < 0)
		{          
		    //System.out.println("ERROR: QueryCreate() ..." + g_Result._hstmt);   
		    g_errMessage = "ERROR: QueryCreate() ..." + g_Result._hstmt;
		    g_Result = null;
		    return -1;
		}
		
		g_Result._nRows = JEDB.QueryExecute(g_hconnid, g_Result._hstmt);
		if (g_Result._nRows < 0)
		{             
		    //System.out.println("ERROR: QueryExecute() ..." + g_Result._nRows);   
		    g_errMessage = "ERROR: QueryExecute() ..." + g_Result._nRows;
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
            //System.out.println("Error: " + ret);
        	g_errMessage = "ERROR: QueryGetNextRow() ..." + ret;
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
	        {
	        	if (JEDB.QueryIsFieldNull(g_hconnid, g_Result._hstmt, j) == 1)
	        		Record.username = "";	        	
	            else
	            	Record.username = JEDB.QueryGetFieldData(g_hconnid, g_Result._hstmt, j);
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
		if( g_Result != null )
		{
			JEDB.QueryDestroy(g_hconnid, g_Result._hstmt);
			g_Result = null;
		}
		
		JEDB.Commit(g_hconnid);		
	}

	
} /* class JDemoLocalDB */



