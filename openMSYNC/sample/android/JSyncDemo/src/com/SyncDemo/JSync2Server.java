package com.SyncDemo;

import java.io.UnsupportedEncodingException;

import com.openMSYNC.mSyncClientJNI;

public class JSync2Server  {	
    static
    {   
		//System.loadLibrary("mSyncClient");
		System.loadLibrary("mSyncClientJNI");		    	
    }
    
    final static boolean USE_BYTEARRAY = true;
  
    final static String g_appName = "demoApp";
    final static int    g_Version = 1;    
    final static String g_clientDSN = "demoApp_clientDSN";	
   	final static String g_serverTable = "contact_place";
    final static String g_Log_PATH = "/mnt/sdcard/Android/data/com.SyncDemo/";
    final static int RET_NEED_UPGRADE = 1;
    
	static String g_errMessage = "";

	int DownloadDelete_contact(String Data)
	{
		int i, nErrSum;		
		String check_fld = mSyncClientJNI.FIELD_DEL + ".";
	    String[] recArray;
	    String[] colArray;
	    String QBuf;
	   
	    recArray = Data.split("" + mSyncClientJNI.RECORD_DEL);
	    nErrSum = 0;
	    for( i = 0; i < recArray.length; i++ )
	    {
	    	recArray[i] += check_fld;	    	
	    	colArray = recArray[i].split( "" +  mSyncClientJNI.FIELD_DEL ) ;
	    	if( colArray.length == 3 )	    	
	    	{
	    		QBuf = String.format("delete from contact where id = %s AND username ='%s'", colArray[0], colArray[1] );
	    		nErrSum += JSyncDemoActivity.mldb.Query2LocalDB(QBuf, true);
	    	}
	    	else
	    		nErrSum++;	    		
	    }	    
	    
		return nErrSum;
	}
	
	int DownloadSelect_contact(String Data)
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
	    				colArray[0],colArray[1], colArray[2], colArray[3], colArray[4], colArray[5], colArray[6], colArray[7]);
	    		
	    		ret = JSyncDemoActivity.mldb.Query2LocalDB(str, true);
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
		
	int Upload_contact(char flag, String Data)
	{	
		String  tmp, flagStr;
		String  DataStr;
	    int   i, nRows;
	    DemoRecord  Record;
	    	    
	    if(flag == mSyncClientJNI.INSERT_FLAG )
	    {   // Upload Insert
	    	tmp = "select id, username, company, name, e_mail, company_phone, mobile_phone, address from contact INSERT_RECORD";
	    }
		else if(flag == mSyncClientJNI.UPDATE_FLAG )
	    {   // Upload Update
			tmp = "select id, username, company, name, e_mail, company_phone, mobile_phone, address from contact UPDATE_RECORD";
	    }
		else// if( flag == mSyncClientJNI.DELETE_FLAG )
	    {   // Upload Delete
			tmp = "select id, username from contact  DELETE_RECORD";		
		}

	    nRows = JSyncDemoActivity.mldb.SelectLocalDB(tmp);
	    if( nRows < 0  )
	    {       
	    	JSyncDemoActivity.mldb.QuitSelectLocalDB();
	        return -1;
	    }
	    else if( nRows == 0  )
	    {
	    	JSyncDemoActivity.mldb.QuitSelectLocalDB();
	    	return 0;
	    }
	    
	    flagStr = "" + flag;
	    DataStr = "";
	    for( i = 0; i < nRows; i++ )
	    {
	    	Record = new DemoRecord();
	        if( JSyncDemoActivity.mldb.FetchLocalDB(Record) <= 0 )
	        {     
	        	JSyncDemoActivity.mldb.QuitSelectLocalDB();
	        	return -2;	            
	        }

			// 필드 구분지   mSyncClientJNI.FIELD_DEL
			// 레코드 구분지 mSyncClientJNI.RECORD_DEL
				 
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
					JSyncDemoActivity.mldb.QuitSelectLocalDB();
					return -3;	                
				}
				
				DataStr = "";				
			}
			
			DataStr += tmp;			
	    }

	    JSyncDemoActivity.mldb.QuitSelectLocalDB();
	    
	    if ( DataStr.length() > 0)
		{	  
	    	//////////////////////////////
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
            //////////////////////////////	    	
		}
	    
		return 0;
	}
	
	int SYNC_TABLE(String ClientDSN, String ServerTable, int nSyncMode )
	{		
	    int		nRet;	   
	    
	    String 	Data = "";
	    
	    /* DSN name 전송  */
	    if(mSyncClientJNI.mSync_SendDSN(ClientDSN) < 0)
	    {
	    	mSyncClientJNI.mSync_ClientLog("SendDSN ==> " +  mSyncClientJNI.mSync_GetSyncError() + "\n");
	    	g_errMessage = "SendDSN ==> " +  mSyncClientJNI.mSync_GetSyncError();	
	        return -1;
	    }	
	    mSyncClientJNI.mSync_ClientLog(ClientDSN + " sync processing\n");
	    
	    /* Table name 전송 */
	    if((nRet = mSyncClientJNI.mSync_SendTableName(nSyncMode, ServerTable)) < 0) 
	    {
	    	mSyncClientJNI.mSync_ClientLog("SendTableName ==> " + ServerTable +"\n");
	    	g_errMessage = "SendTableName ==> " + ServerTable;
	        return -1;
	    }
	    
	    /* Uploading */	    
	    // insert upload				
	    if (Upload_contact(mSyncClientJNI.INSERT_FLAG, Data) < 0) 
	    {
	    	mSyncClientJNI.mSync_ClientLog("Insert MakeMessage error\n");
	    	g_errMessage = "Insert MakeMessage error";
	        return -1;
	    }
	    
	    // update upload    
	    if (Upload_contact(mSyncClientJNI.UPDATE_FLAG, Data) < 0) 
	    {
	    	mSyncClientJNI.mSync_ClientLog("Update MakeMessage error : addr\n");
	    	g_errMessage = "Update MakeMessage error";
	        return -1;
	    }
	    
	    // delete upload    
	    if (Upload_contact(mSyncClientJNI.DELETE_FLAG, Data) < 0) 
	    {
	    	mSyncClientJNI.mSync_ClientLog("Delete MakeMessage error : addr\n");
	    	g_errMessage = "Delete MakeMessage error";
	        return -1;
	    }
	    	    
	    // Upload 완료
	    // download parameter 생성
	    // download parameter 2개 이상일 경우 FIELD_DEL로 구분하여 string을 생성한다
	    if (mSyncClientJNI.mSync_UploadOK("") < 0)
	    {
	    	mSyncClientJNI.mSync_ClientLog("Upload ERROR\n");
	    	mSyncClientJNI.mSync_ClientLog("mSync_UploadOK ==> " + mSyncClientJNI.mSync_GetSyncError() + "\n");
	    	g_errMessage = "mSync_UploadOK ==> " + mSyncClientJNI.mSync_GetSyncError();
	        return  -1;		
	    }
	    	    
	    if( JSyncDemoActivity.mldb.Query2LocalDB("alter table contact msync flush", true) < 0 )
	    {
	       ;
	    }
	    	        
	    /* Downloading */	   
	    // Download_Select script 수행
//////////////////////////////
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
////////////////////////////	    
	    
	    if (nRet < 0)
	    {
	    	mSyncClientJNI.mSync_ClientLog("ERROR : " + mSyncClientJNI.mSync_GetSyncError() + "\n");
	    	g_errMessage = "ERROR : " + mSyncClientJNI.mSync_GetSyncError();
	        return -1;
	    }	
	    else /* if(nRet == 0) */
	    {
	    	mSyncClientJNI.mSync_ClientLog("DownLoad Success ----------\n");
	    }
	    
	    // Download_Delete script 수행
//////////////////////////////
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
//////////////////////////////	    
	    
		
	    if( JSyncDemoActivity.mldb.Query2LocalDB("alter table contact msync flush", true) < 0 )
	    {
	       ;
	    }
	
	    if (nRet < 0)
	    {
	    	mSyncClientJNI.mSync_ClientLog("ERROR : " + mSyncClientJNI.mSync_GetSyncError() + "\n" );
	    	g_errMessage = "ERROR : " + mSyncClientJNI.mSync_GetSyncError();
	        return -1;
	    }	
	    else /* if(nRet == 0) */
	    {
	    	mSyncClientJNI.mSync_ClientLog("DownLoad Success ----------\n");
	    }
		return 0;		
	}
	
			
	public int DoSyncDB(String user, String pwd)
	{		
		int nRet;
		
		g_errMessage = "";
		mSyncClientJNI.mSync_InitializeClientSession(g_Log_PATH + "==open_mSync.log", PrefSetting.mTimeout );
		
		if (mSyncClientJNI.mSync_ConnectToSyncServer( PrefSetting.mIP, PrefSetting.mPort) < 0)	
		{		
			mSyncClientJNI.mSync_ClientLog("[ERROR] Connect To Sync Server : \n");
			g_errMessage = "서버 연결에 실패했습니다.";	        
	        return -1;
		}
	   
	    /* 사용자인증 */
		nRet =mSyncClientJNI.mSync_AuthRequest(user, pwd, g_appName, g_Version);
		if(nRet < 0)
		{	
			mSyncClientJNI.mSync_ClientLog("AuthRequest Error\n");
			g_errMessage = "AuthRequest Error";
			nRet = -1;		
	    }
		else if(nRet == mSyncClientJNI.NACK_FLAG)
		{
			mSyncClientJNI.mSync_ClientLog("Auth Fail\n");
			g_errMessage = "Auth Fail";
			nRet = -1;		
		}
	    else if(nRet == mSyncClientJNI.UPGRADE_FLAG)
	    {
	    	mSyncClientJNI.mSync_ClientLog("Need Upgrade...\n");	        
	        mSyncClientJNI.mSync_Disconnect(mSyncClientJNI.NORMAL);

	        return RET_NEED_UPGRADE; // need upgrade version           
	    }        
	    else if(nRet != mSyncClientJNI.ACK_FLAG)
	    {
	    	mSyncClientJNI.mSync_ClientLog("Auth Fail... returned " + nRet + "\n");
	    	g_errMessage = "Auth Fail... returned " + nRet ;
	        nRet = -1;
	    }
	    else
	    {   
	    	if( PrefSetting.mIsFullSync )
	    		nRet = SYNC_TABLE(g_clientDSN,  g_serverTable, mSyncClientJNI.ALL_SYNC);// 전체동기화
	    	else
	    		nRet = SYNC_TABLE(g_clientDSN,  g_serverTable, mSyncClientJNI.MOD_SYNC);// 부분동기화
	    		    
	        if( nRet < 0)
		    {
	        	mSyncClientJNI.mSync_ClientLog("동기화 작업에 실패\n");		
	        	g_errMessage += "\n 동기화 작업에 실패하였습니다." ;
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
	        {
	        	mSyncClientJNI.mSync_ClientLog("mSync_Disconnect ==> " + mSyncClientJNI.mSync_GetSyncError() + "\n");
	        	if( !g_errMessage.isEmpty() )
	        		g_errMessage += "\n";
	        	g_errMessage += "mSync_Disconnect ==> " + mSyncClientJNI.mSync_GetSyncError();
	        }
	        
	        return nRet;
	    }
	
	    if( mSyncClientJNI.mSync_Disconnect(mSyncClientJNI.NORMAL) < 0 )	
        {
        	mSyncClientJNI.mSync_ClientLog("mSync_Disconnect ==> " + mSyncClientJNI.mSync_GetSyncError() + "\n");
        	g_errMessage = "mSync_Disconnect ==> " + mSyncClientJNI.mSync_GetSyncError();
            return -2;
        }
	    		
		return 0;
	}	
}
