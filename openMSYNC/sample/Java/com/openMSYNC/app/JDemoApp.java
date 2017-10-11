package com.openMSYNC.app;

import java.io.UnsupportedEncodingException;
import java.util.Scanner;

import com.openMSYNC.mSyncClientJNI;


class GlobalVar 
{		 
	public final static String _clientDSN = "demoApp_clientDSN";	
	public final static String _serverTable = "contact_place";
	public final static int _sync_mode = mSyncClientJNI.ALL_SYNC;
	public final static int _timeout = 1000;
    	
	static class SyncSrv_Info
	{
		String szSrvAddr;
		int    nPort;    
		String appName;
		int    version;
		
		SyncSrv_Info()
		{
			szSrvAddr = "192.168.4.9"; 
			nPort = 7788;
			appName = "demoApp";
			version = 1;
		}
	}
	static SyncSrv_Info _SrvInfo;

	static class virtualClient_Info
	{
		String db_name; /* local db_name */
		String userid;  /* mSync server account */
		String passwd;
		
		public virtualClient_Info(String db, String user, String pwd)
		{
			db_name = db;
			userid= user;
			passwd = pwd;			
		}
	}			
	virtualClient_Info  _ClientInfo[];	
	
	public GlobalVar()
	{		
		_SrvInfo = new SyncSrv_Info();
		
		_ClientInfo = new virtualClient_Info[2];		
		_ClientInfo[0] = new virtualClient_Info("/mSyncClientDB/contactdb1", "sync_user1", "passwd" );
		_ClientInfo[1] = new virtualClient_Info("/mSyncClientDB/contactdb2", "sync_user2", "passwd" );		
	}	
}


public class JDemoApp  {	
    static
    {    		
		//System.loadLibrary("openml");
		//System.loadLibrary("jedb");
		//System.loadLibrary("mSyncClient");
		System.loadLibrary("mSyncClientJNI");		    	
    } 	
    
    final static boolean USE_BYTEARRAY = true;
    	
	static GlobalVar g_var = new GlobalVar();
	static JDemoLocalDB ldb = new JDemoLocalDB();
	static int g_vClientIdx = -1;

	static int DownloadDelete_contact(String Data)
	{
		int i, nErrSum;		
		String check_fld = mSyncClientJNI.FIELD_DEL + ".";
	    String[] recArray;
	    String[] colArray;
	    String Qbuf;
	   
	    recArray = Data.split("" + mSyncClientJNI.RECORD_DEL);
	    nErrSum = 0;
	    for( i = 0; i < recArray.length; i++ )
	    {
	    	recArray[i] += check_fld;	    	
	    	colArray = recArray[i].split( "" +  mSyncClientJNI.FIELD_DEL ) ;
	    	if( colArray.length == 3 )	    	
	    	{
	    		Qbuf = String.format("delete from contact where id = %s AND username = '%s' ", colArray[0],colArray[1]);
	    		nErrSum += ldb.Query2LocalDB(Qbuf);
	    	}
	    	else
	    		nErrSum++;	    		
	    }	    
	    
		return nErrSum;
	}
	
	static int DownloadSelect_contact(String Data)
	{			
	    int  i, ret;	   
	    String check_fld = mSyncClientJNI.FIELD_DEL + ".";
	    String[] recArray;
	    String[] colArray;
	    String   str;
	    
	    recArray = Data.split("" + mSyncClientJNI.RECORD_DEL);
	    for( i = 0; i < recArray.length; i++ )
	    {	    	
	    	recArray[i] += check_fld;
	    	colArray = recArray[i].split( "" +  mSyncClientJNI.FIELD_DEL ) ;	   
	    	if( colArray.length == 9 )
	    	{
	    		str = String.format("UPSERT into contact(id, username, company, name, e_mail, company_phone, mobile_phone, address, mtime) " +
	    				"values(%s, '%s', '%s', '%s', '%s', '%s', '%s', '%s', now()) SYNCED_RECORD",
	    				colArray[0],colArray[1], colArray[2], colArray[3], colArray[4], colArray[5], colArray[6] , colArray[7]);
	    		
	    		ret = ldb.Query2LocalDB(str);
	    		if( ret < 0 )
		        	 return -1;
	    	}
	    	else
	    	{
	    		return -2;
	    	}	         			
	    }	    	    
	    
		return 0;		
	}
		
	static int Upload_contact(char flag, String Data)
	{	
		String  tmp, flagStr;
		String  DataStr;
	    int   i, nRows;
	    DemoRecord  Record;
	    	    
	    if(flag == mSyncClientJNI.INSERT_FLAG )
	    {   // Upload Insert
	    	tmp = "select id, username, company, name, e_mail, company_phone, mobile_phone, address, mtime from contact INSERT_RECORD";
	    }
		else if(flag == mSyncClientJNI.UPDATE_FLAG )
	    {   // Upload Update
			tmp = "select id, username, company, name, e_mail, company_phone, mobile_phone, address, mtime from contact UPDATE_RECORD";
	    }
		else// if( flag == mSyncClientJNI.DELETE_FLAG )
	    {   // Upload Delete
			tmp = "select id, username from contact  DELETE_RECORD";		
		}

	    nRows = ldb.SelectLocalDB(tmp);
	    if( nRows < 0  )
	    {       
	    	ldb.QuitSelectLocalDB();
	        return -1;
	    }
	    else if( nRows == 0  )
	    {
	    	ldb.QuitSelectLocalDB();
	    	return 0;
	    }
	    
	    flagStr = "" + flag;
	    DataStr = "";
	    for( i = 0; i < nRows; i++ )
	    {
	    	Record = new DemoRecord();
	        if( ldb.FetchLocalDB(Record) <= 0 )
	        {     
	        	ldb.QuitSelectLocalDB();
	        	return -2;	            
	        }

			//필드 구분자    mSyncClientJNI.FIELD_DEL
			//레코드 구분자  mSyncClientJNI.RECORD_DEL
				 
	        tmp = "";
	        if( flag != mSyncClientJNI.UPDATE_FLAG )
	       	{
	        	tmp += Record.ID;
	        	tmp += mSyncClientJNI.FIELD_DEL;
	        	tmp += Record.username;
	        	tmp += mSyncClientJNI.FIELD_DEL;
	       	}
        	        
	        if( flag != mSyncClientJNI.DELETE_FLAG )
	        {
	        	tmp += Record.Company + mSyncClientJNI.FIELD_DEL;
	         	tmp += Record.Name + mSyncClientJNI.FIELD_DEL;
	         	tmp += Record.e_mail + mSyncClientJNI.FIELD_DEL;
	         	tmp += Record.Company_phone + mSyncClientJNI.FIELD_DEL;
	         	tmp += Record.Mobile_phone + mSyncClientJNI.FIELD_DEL;
	         	tmp += Record.Address + mSyncClientJNI.FIELD_DEL;
	            //tmp += Record.mTime + mSyncClientJNI.FIELD_DEL;
	        }

	        if( flag == mSyncClientJNI.UPDATE_FLAG )
	        {
	        	tmp += Record.ID;
	        	tmp += mSyncClientJNI.FIELD_DEL;
	        	tmp += Record.username;
	        	tmp += mSyncClientJNI.FIELD_DEL;
	        }	             
	        
	        tmp = tmp.substring(0, tmp.length()-1);	        
	        tmp += mSyncClientJNI.RECORD_DEL;
	        
			if ( (DataStr.length() + tmp.length()) > mSyncClientJNI.HUGE_STRING_LENGTH )
			{
				if (mSyncClientJNI.mSync_SendUploadData(flagStr, DataStr) < 0)
				{
					ldb.QuitSelectLocalDB();
					return -3;	                
				}
				
				DataStr = "";				
			}
			
			DataStr += tmp;			
	    }

	    ldb.QuitSelectLocalDB();
	    if ( DataStr.length() > 0)
		{
	    	if( USE_BYTEARRAY )
	    	{
		    	byte[] bData = null;
		    	
		    	try {
					bData =  DataStr.getBytes("KSC5601");
				} catch (UnsupportedEncodingException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
		    	if (mSyncClientJNI.mSync_SendUploadData2(flagStr, bData) < 0)			
				{		
					return -4;
				}
	    	}
	    	else
	    	{
				if (mSyncClientJNI.mSync_SendUploadData( flagStr, DataStr) < 0)
				{
					return -4;
				}
	    	}
		}	    
		return 0;
	}
	
	static int SYNC_TABLE(String ClientDSN, String ServerTable, int nSyncMode )
	{		
	    int		nRet;	   
	    
	    String 	Data = "";
	    
	    /* DSN name 전송 */
	    if(mSyncClientJNI.mSync_SendDSN(ClientDSN) < 0)
	    {
	    	mSyncClientJNI.mSync_ClientLog("SendDSN ==> " +  mSyncClientJNI.mSync_GetSyncError() + "\n");
	        return -1;
	    }	
	    mSyncClientJNI.mSync_ClientLog(ClientDSN + " sync processing\n");
	    
	    /* Table name 전송 */
	    if((nRet = mSyncClientJNI.mSync_SendTableName(nSyncMode, ServerTable)) < 0) 
	    {
	    	mSyncClientJNI.mSync_ClientLog("SendTableName ==> " + ServerTable +"\n");
	        return -1;
	    }
	    
	    /* Uploading */	    
	    // insert upload				
	    if (Upload_contact(mSyncClientJNI.INSERT_FLAG, Data) < 0) 
	    {
	    	mSyncClientJNI.mSync_ClientLog("Insert MakeMessage error\n");
	        return -1;
	    }
	    
	    // update upload    
	    if (Upload_contact(mSyncClientJNI.UPDATE_FLAG, Data) < 0) 
	    {
	    	mSyncClientJNI.mSync_ClientLog("Update MakeMessage error : addr\n");
	        return -1;
	    }
	    
	    // delete upload    
	    if (Upload_contact(mSyncClientJNI.DELETE_FLAG, Data) < 0) 
	    {
	    	mSyncClientJNI.mSync_ClientLog("Delete MakeMessage error : addr\n");
	        return -1;
	    }
	    
	    // Upload 완료
	    // download parameter 생성
	    // download parameter 2개 이상일 경우 FIELD_DEL로 구분하여 string을 생성한다.
	    if (mSyncClientJNI.mSync_UploadOK("") < 0)
	    {
	    	mSyncClientJNI.mSync_ClientLog("Upload ERROR\n");
	    	mSyncClientJNI.mSync_ClientLog("mSync_UploadOK ==> " + mSyncClientJNI.mSync_GetSyncError() + "\n");
	        return  -1;		
	    }
	        
	    /* Downloading */	   
	    // Download_Select script 수행
	    if( USE_BYTEARRAY )
	    {
	    	byte[] bData = null;
		    
		    bData = mSyncClientJNI.mSync_ReceiveDownloadData2();	    
		    while( bData != null )
		    {
		    	try {
					Data = new String(bData, "EUC-KR");
				} catch (UnsupportedEncodingException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
		    	if (DownloadSelect_contact(Data) < 0)
			        return -1;
			    
			    Data = "";	    	
			    bData = mSyncClientJNI.mSync_ReceiveDownloadData2();
		    }	
	    	
	    }
	    else
	    {
		    while ( (Data = mSyncClientJNI.mSync_ReceiveDownloadData()) != null ) 
		    {
		        if (DownloadSelect_contact(Data) < 0)
		            return -1;
		        
		        Data = "";
		    }
	    }
	    
	    if (nRet < 0)
	    {
	    	mSyncClientJNI.mSync_ClientLog("ERROR : " + mSyncClientJNI.mSync_GetSyncError() + "\n");
	        return -1;
	    }	
	    else /* if(nRet == 0) */
	    {
	    	mSyncClientJNI.mSync_ClientLog("DownLoad Success ----------\n");
	    }
	    
	    // Download_Delete script 수행    
	    if( USE_BYTEARRAY )
	    {
	    	byte[] bData = null;
	    	
	    	bData = mSyncClientJNI.mSync_ReceiveDownloadData2();	    
		    while( bData != null )
		    {
		    	try {
					Data = new String(bData, "EUC-KR");					
				} catch (UnsupportedEncodingException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
		    	if (DownloadDelete_contact(Data) < 0)
			        return -1;
			    
			    Data = "";	    	
			    bData = mSyncClientJNI.mSync_ReceiveDownloadData2();
		    }	    	
	    }
	    else
	    {
		    while ((Data = mSyncClientJNI.mSync_ReceiveDownloadData()) != null) 
		    {
		        if (DownloadDelete_contact(Data) < 0)
		            return -1;
		        
		       Data = "";
		    }
	    }

	    if( ldb.Query2LocalDB("alter table contact msync flush") < 0 )
	    {
	       ;
	    }
	
	    if (nRet < 0)
	    {
	    	mSyncClientJNI.mSync_ClientLog("ERROR : " + mSyncClientJNI.mSync_GetSyncError() + "\n" );
	        return -1;
	    }	
	    else /* if(nRet == 0) */
	    {
	    	mSyncClientJNI.mSync_ClientLog("DownLoad Success ----------\n");
	    }
		return 0;		
	}
	
			
	static int DoSyncDB(String user, String pwd)
	{		
		int nRet;
		
		mSyncClientJNI.mSync_InitializeClientSession("==open_mSync.log", GlobalVar._timeout );
		
		if (mSyncClientJNI.mSync_ConnectToSyncServer( GlobalVar._SrvInfo.szSrvAddr, GlobalVar._SrvInfo.nPort ) < 0)	
		{		
			mSyncClientJNI.mSync_ClientLog("[ERROR] Connect To Sync Server : \n");
			System.out.println("서버 연결에 실해했습니다.");	        
	        return -1;
		}
	   
	    /* 사용자 인증 */
		nRet =mSyncClientJNI.mSync_AuthRequest(user, pwd, 
				GlobalVar._SrvInfo.appName, GlobalVar._SrvInfo.version );
		if(nRet < 0)
		{	
			mSyncClientJNI.mSync_ClientLog("AuthRequest Error\n");
			nRet = -1;		
	    }
		else if(nRet == mSyncClientJNI.NACK_FLAG)
		{
			mSyncClientJNI.mSync_ClientLog("Auth Fail\n");
			nRet = -1;		
		}
	    else if(nRet == mSyncClientJNI.UPGRADE_FLAG)
	    {
	    	mSyncClientJNI.mSync_ClientLog("Need Upgrade...\n");	        
	        mSyncClientJNI.mSync_Disconnect(mSyncClientJNI.NORMAL);

	        return 1;            
	    }        
	    else if(nRet != mSyncClientJNI.ACK_FLAG)
	    {
	    	mSyncClientJNI.mSync_ClientLog("Auth Fail... returned " + nRet + "\n");
	        nRet = -1;
	    }
	    else
	    {   
	    	int sync_mode; 
			
	    	sync_mode  = mSyncClientJNI.ALL_SYNC;  /* 전체 동기화 */
	        // sync_mode = mSyncClientJNI.MOD_SYNC; /* 부분 동기화 */ 	

	        nRet = SYNC_TABLE(GlobalVar._clientDSN,  GlobalVar._serverTable, sync_mode);	    
	        if( nRet < 0)
		    {
	        	mSyncClientJNI.mSync_ClientLog("동기화 작업 실패\n");			                
		    }
		    else
	        {    
		    	mSyncClientJNI.mSync_ClientLog("동기화 작업 성공\n");            
	            nRet = 0;
	        }                
	    }

	    if( nRet < 0)
	    {
	        if( mSyncClientJNI.mSync_Disconnect(mSyncClientJNI.ABNORMAL) < 0 )	
	        	mSyncClientJNI.mSync_ClientLog("mSync_Disconnect ==> " + mSyncClientJNI.mSync_GetSyncError() + "\n");
	        
	        return nRet;
	    }
	
	    if( mSyncClientJNI.mSync_Disconnect(mSyncClientJNI.NORMAL) < 0 )	
        {
        	mSyncClientJNI.mSync_ClientLog("mSync_Disconnect ==> " + mSyncClientJNI.mSync_GetSyncError() + "\n");
            return -2;
        }
    	return 0;
	}
	
	static boolean NumericCheck(String str)
	{
		int  i;
	    char c;

	    if(str.equals(""))
	    	return false;
	    	    
	    for( i = 0 ; i < str.length() ; i++)
	    {
	        c = str.charAt(i);
	        if(c < '0' || c > '9')
	        {
	            return false;
	        }
	    }
	    
	    return true;
	}
	
	static int insert2LocalDB(Scanner scanner)
	{
		String    str;
		DemoRecord sRec;
	
        System.out.println("");
        System.out.println("Enter new contact....");
        
        while( true )
        {    
		    System.out.printf("%-7s: ", "ID");
	        str = scanner.nextLine();        
	        str.trim();
	        
	        if( NumericCheck(str) )
	        	break;
	        
	        System.out.println("Invalid ID, try again...");	        	        
        }
        
        sRec = new DemoRecord();
        sRec.ID = Integer.parseInt(str);
        
        sRec.username = g_var._ClientInfo[g_vClientIdx].userid;
      
        System.out.printf("%-7s: ", "Company");
        sRec.Company = scanner.nextLine();        
        sRec.Company.trim();	    

	    System.out.printf("%-7s: ", "Name");
	    sRec.Name = scanner.nextLine();        
        sRec.Name.trim();
     
	    System.out.printf("%-7s: ", "E-Mail");
	    sRec.e_mail = scanner.nextLine();        
        sRec.e_mail.trim();
        
	    System.out.printf("%-7s: ", "Phone");
	    sRec.Company_phone = scanner.nextLine();        
        sRec.Company_phone.trim();
        
	    System.out.printf("%-7s: ", "Mobile");
	    sRec.Mobile_phone = scanner.nextLine();        
        sRec.Mobile_phone.trim();
        
	    System.out.printf("%-7s: ", "Address");
	    sRec.Address = scanner.nextLine();        
        sRec.Address.trim();
                    
        str = String.format("insert into contact(id, username, company, name, e_mail, company_phone," +
	                   "mobile_phone, address, mtime) " +
	                   "values(%d, '%s', '%s', '%s', '%s', '%s', '%s', '%s', now())",
	                   sRec.ID, sRec.username, sRec.Company, sRec.Name, sRec.e_mail,
	                   sRec.Company_phone, sRec.Mobile_phone, sRec.Address);
	
        return ldb.Query2LocalDB(str);		
	}
	
	static int update2LocalDB(Scanner scanner, int update_id)	
	{			
	    String str;
	    DemoRecord NewRec, OldRec;
	    int ret;
	
	    str = String.format("select company, name, e_mail, company_phone, mobile_phone, " +
	                 "address, mtime from contact where id = %d AND username = '%s'", 
	                 update_id, g_var._ClientInfo[g_vClientIdx].userid);
	
	    ret = ldb.SelectLocalDB(str);
	    if( ret < 0 )
	    	return ret;
	    else if( ret == 0 )
	    {
	    	ldb.QuitSelectLocalDB();
	    	return 0;
	    }
	    
	    NewRec = new DemoRecord();
	    OldRec = new DemoRecord();
	    
	    ret = ldb.FetchLocalDB(OldRec); 
	    if( ret < 0 )
	    {
	    	ldb.QuitSelectLocalDB();
	    	return ret;	    	
	    }	    
	    ldb.QuitSelectLocalDB();
	    
	    NewRec.ID = update_id;
	    NewRec.username = g_var._ClientInfo[g_vClientIdx].userid;

	    System.out.printf("%-7s: %d\n", "ID", update_id);
	    System.out.printf("----------------------------------------------------\n");
	    System.out.printf("%-7s: %s ", "Company", OldRec.Company);
	    System.out.printf("%-7s: %s ", "Name", OldRec.Name );
	    System.out.printf("%-7s: %s ", "E-Mail", OldRec.e_mail);
	    System.out.printf("%-7s: %s ", "Phone", OldRec.Company_phone);
	    System.out.printf("%-7s: %s ", "Mobile", OldRec.Mobile_phone);
	    System.out.printf("%-7s: %s ", "Address", OldRec.Address);
	    System.out.printf("\n----------------------------------------------------\n"); 
	    
	    System.out.printf("\nEnter update contact....");
	    System.out.printf("%-7s: ", "Company");	    
	    NewRec.Company = scanner.nextLine();
	    NewRec.Company.trim();			    
	    System.out.printf("%-7s: ", "Name");
	    NewRec.Name = scanner.nextLine();
	    NewRec.Name.trim();
	    System.out.printf("%-7s: ", "E-Mail");
	    NewRec.e_mail = scanner.nextLine();
	    NewRec.e_mail.trim();	    
	    System.out.printf("%-7s: ", "Phone");
	    NewRec.Company_phone = scanner.nextLine();
	    NewRec.Company_phone.trim();
	    System.out.printf("%-7s: ", "Mobile");
	    NewRec.Mobile_phone = scanner.nextLine();
	    NewRec.Mobile_phone.trim();
	    System.out.printf("%-7s: ", "Address");
	    NewRec.Address = scanner.nextLine();
	    NewRec.Address.trim();	   
	 
	    str = String.format("update contact set company = '%s', name = '%s', e_mail = '%s'," +
	                 "company_phone = '%s', mobile_phone = '%s', address = '%s', " +
	                 "mtime = now() where id = %d AND username = '%s'",
	                 NewRec.Company, NewRec.Name, NewRec.e_mail, NewRec.Company_phone,
	                 NewRec.Mobile_phone, NewRec.Address, NewRec.ID, NewRec.username);
	    
	    return ldb.Query2LocalDB(str);	 
	}
	
	static int delete2LocalDB(int delete_id)
	{
	    String str;

	    str = String.format("delete from contact where id = %d AND username = '%'", delete_id, g_var._ClientInfo[g_vClientIdx].userid);
	    return ldb.Query2LocalDB(str);
	}
	
	static void  DoLocal(Scanner scanner)
	{
		String str, substr;
		int id, ret;
		
		while( true )
		{
			System.out.println("\n************************************************************************");
			System.out.println("Enter action code (i, ux, dx, l, q)...");
			System.out.println("usage i / u3 / d2 / l / q / s");
	
			str = scanner.nextLine();
			str.trim();
			System.out.println(str);	
						
			if( str.equalsIgnoreCase("i") )
			{				
				ret = insert2LocalDB(scanner);
				if( ret < 0 )			
					System.out.println("ERROR: insert2LocalDB()... " + ret);
			}
			else if( str.equalsIgnoreCase("l") )
			{				
				ldb.Show_LocalDB();
			}
			else if( str.equalsIgnoreCase("q") )
			{
				return ;
			}
			else if( str.equalsIgnoreCase("s") )
			{
				ret = DoSyncDB(g_var._ClientInfo[g_vClientIdx].userid, g_var._ClientInfo[g_vClientIdx].passwd);
		        if( ret == 1 )
		        {  /* need upgrade */		        	
		            return;
		        }
		        else if( ret < 0 )
		        {		        	
		            return;        	
		        }				
			}
			else if( str.length() > 1 )
			{	
				System.out.println("length" + str.length());
				
				substr = str.substring(1);
				if( NumericCheck(substr) )
				{ 
					id = Integer.parseInt( substr );				
					if( str.substring(0, 1).compareToIgnoreCase("u") == 0 )
					{					
						 ret = update2LocalDB(scanner, id);
						 if( ret < 0 )			
								System.out.println("ERROR: update2LocalDB()... " + ret);
					}
					else if( str.substring(0, 1).compareToIgnoreCase("d") == 0 )
					{
						 ret = delete2LocalDB(id);										
						 if( ret < 0 )			
								System.out.println("ERROR: delete2LocalDB()... " + ret);
					}
				}
			}			
		}		
	}     
		
	public static void main(String[] args)
    {        
        int ret;
     
        if (args.length < 1)
        {
        	g_vClientIdx = 0;
        }
		else if (args.length == 1)		
		{
			g_vClientIdx = Integer.parseInt(args[1]);			
		}
        
        if( g_vClientIdx != 0 && g_vClientIdx != 1 )
		{
			System.out.println("Usage: JDemoApp client_No(0 or 1)");
			return;
		}	
          
        System.out.println("");
        System.out.println("JDemoApp for openML & openMSYNC");
        System.out.println("Copyright (C) 2012-2015 " +
                "INERVIT Co., Ltd. All rights reserved.");
        System.out.println("");
        System.out.println("===================================================");
        System.out.println(" DB = " + g_var._ClientInfo[g_vClientIdx].db_name );
        System.out.println("USER= " + g_var._ClientInfo[g_vClientIdx].userid);
        
        ret = ldb.InitLocalDB(g_var._ClientInfo[g_vClientIdx].db_name);
        if( ret < 0 )
        	return ;
                
        ret = ldb.Show_LocalDB();
        if( ret < 0 )
        {
        	ldb.QuitLocalDB();
        	return;
        }
        
        ret = DoSyncDB(g_var._ClientInfo[g_vClientIdx].userid, g_var._ClientInfo[g_vClientIdx].passwd);
        if( ret == 1 )
        {  /* need upgrade */
        	ldb.QuitLocalDB();
            return;
        }
        else if( ret < 0 )
        {
        	ldb.QuitLocalDB();
            return;        	
        }
        
        ret = ldb.Show_LocalDB();
        if( ret < 0 )
        {
        	ldb.QuitLocalDB();
        	return;
        }    
        
        Scanner scanner = new Scanner(System.in);			
        DoLocal(scanner);
        scanner.close();
        
        ldb.Show_LocalDB();       
        
        ldb.QuitLocalDB();          
    }
}
