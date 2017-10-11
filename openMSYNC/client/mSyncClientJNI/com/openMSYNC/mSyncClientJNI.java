/* 
    This file is part of openMSync client library, DB synchronization software.

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

package com.openMSYNC;

public class mSyncClientJNI
{
	public static final char UPDATE_FLAG  = 'U'; // Update
	public static final char DELETE_FLAG  = 'D'; // Delete
	public static final char INSERT_FLAG  = 'I'; // Insert
	public static final char DOWNLOAD_FLAG        = 'F'; //Fetch
	public static final char DOWNLOAD_DELETE_FLAG = 'R'; // Remove
	public static final char END_FLAG	  = 'E'; // End
	public static final char OK_FLAG      = 'K'; // OK
	public static final char TABLE_FLAG   = 'M'; // more tables
	public static final char APPLICATION_FLAG = 'S';	// DBSYNC 
	public static final char ACK_FLAG	  = 'A'; // ACK
	public static final char NACK_FLAG	  = 'N'; // Nack
	public static final char XACK_FLAG	  = 'X'; // Excess
	public static final char UPGRADE_FLAG =  'V';
	public static final char END_OF_TRANSACTION = 'T'; // EOT
	public static final char NO_SCRIPT_FLAG = 'B';  /* No Script */ 
	
	public static final int ALL_SYNC = 1;
	public static final int MOD_SYNC = 2;
	
	public static final int DBSYNC  = (1);
		
	public static final char FIELD_DEL = 0x08;
	public static final char RECORD_DEL = 0x7F;
	
	public static final int NORMAL  = 1;
	public static final int ABNORMAL = 2;
	
	public static final int HUGE_STRING_LENGTH	= 8156;
	 
	    
    // Native functions	
	public native static 
		void mSync_InitializeClientSession(String filename, int timeout);
	public native static 
		int  mSync_ConnectToSyncServer(String serverAddr, int port);
	public native static 
		int  mSync_AuthRequest(String id, String password, String application, int version);
	public native static 
		int  mSync_SendDSN(String dsn);
	public native static 
		int  mSync_SendTableName(int syncmode, String tablename);
	public native static 
		int  mSync_SendUploadData(String flag, String data);
	public native static 
		int  mSync_UploadOK(String parameter);
	public native 
		static String mSync_ReceiveDownloadData();
	public native static 
		int  mSync_Disconnect(int mode);
	public native static 
		void mSync_ClientLog(String msg);
	public native static 
		String mSync_GetSyncError();	

	public native static 
		int  mSync_SendUploadData2(String flag, byte[] data);

	public native 
		static byte[] mSync_ReceiveDownloadData2();
}
