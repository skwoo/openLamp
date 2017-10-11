package com.SyncDemo;

import java.io.Serializable;
import java.util.ArrayList;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;

import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.Toast;

class PrefSetting   {	
	static String mIP = "127.0.0.1";
	static int    mPort = 7979;
	static int    mTimeout = 3000;
	static String mUser = "";
	static String mPWD  = "";
	static boolean mIsFullSync = false;
	
	private static SharedPreferences m_Pref = null;
	
	static boolean mbChecked = false;
	
		
	PrefSetting(Context context)
	{			
		m_Pref = PreferenceManager.getDefaultSharedPreferences(context);
		//m_Pref = getSharedPreferences("com.SyncDemo.SyncDemoPref",Activity.MODE_PRIVATE);		
	}
		
	static int ReadConfig()
	{			
		mIP = m_Pref.getString("IP", "127.0.0.1");		
		mPort = m_Pref.getInt("PORT", 7979); 
		mTimeout = m_Pref.getInt("TIMEOUT", 3000); 
		mUser = m_Pref.getString("USER", "");
		mPWD = m_Pref.getString("PWD", "");						
		mIsFullSync = m_Pref.getBoolean("SYNCMODE", false);
		return 0;
	}
	
	static int WriteConfig()
	{
		SharedPreferences.Editor editor = m_Pref.edit();
		
        editor.putString("IP", mIP); 
        editor.putInt("PORT", mPort); 
        editor.putInt("TIMEOUT", mTimeout); 
        editor.putString("USER", mUser);
        editor.putString("PWD", mPWD);	
        editor.putBoolean("SYNCMODE", mIsFullSync);
		        
        editor.commit();
		return 0;
	}
	
	static void CheckConfigValue()
	{
		if( mIP.isEmpty() )
		{
			mbChecked = false;
			return;
		}
		if( JSyncDemoConfigActivity.CheckIPAddr( mIP, true) == false )
		{
			mbChecked = false;
			return;
		}
		
		if( mPort <= 0 /* || mPort > 9999 */ )
		{
			mbChecked = false;
			return;
		}
		
		if( mTimeout <= 0 )
		{
			mbChecked = false;
			return;
		}
		
		if( mUser.isEmpty() || mPWD.isEmpty() ) 
		{
			mbChecked = false;
			return;
		}
		
		mbChecked = true;
	}		
}


class MyListItem implements Serializable {
	int    ID;
	String UserID;
	String Name;
	String ph_num;	
	
	MyListItem(int id, String userid,  String name, String  num)
	{
		ID = id;
		UserID = userid;
		Name = name;
		ph_num = num;	
	}
}

class MyListArray {
	ArrayList<MyListItem> mItemArray;
	ArrayList<String>     mListArray = null;	
    ArrayAdapter<String>  mListAdapter = null;    
	    
	MyListArray()
	{	
	   mItemArray = new ArrayList<MyListItem>();
	   mListArray = new ArrayList<String>();	
	   mListAdapter = null;
	}
	
	int GetItem(int index, int id, String userid, String name, String  num)
	{
		if( index < mItemArray.size() && index >= 0 )
		{		
			MyListItem m = mItemArray.get(index);
			
			id = m.ID;
			userid = m.UserID;
			name = m.Name;
			num = m.ph_num;		
			return 0;		
		}
		
		return -1;		
	}
	
	int AddItem_Bulk(int id, String userid, String name, String  num)
	{
		MyListItem m = new MyListItem(id, userid, name, num);
		mItemArray.add(m);	
		if( userid.compareTo(PrefSetting.mUser) == 0 )
			mListArray.add( "**"  + name + "  [" + num + "]");
		else
			mListArray.add( name + "  [" + num + "]");
			
		return 0;		
	}
	
	int AddItem(MyListItem m)
	{
		mItemArray.add(m);		
		if( m.UserID.compareTo(PrefSetting.mUser) == 0 )
			mListArray.add( "**" + m.Name + "  [" + m.ph_num + "]");
		else
			mListArray.add( m.Name + "  [" + m.ph_num + "]");			
		mListAdapter.notifyDataSetChanged(); 
		return 0;				
	}
	
	int AddItem(int id, String userid, String name, String  num)
	{
		MyListItem m = new MyListItem(id, userid, name, num);
		mItemArray.add(m);	
		if( m.UserID.compareTo(PrefSetting.mUser) == 0 )
			mListArray.add( "**" + m.Name + "  [" + m.ph_num + "]");
		else
			mListArray.add( m.Name + "  [" + m.ph_num + "]");			
		mListAdapter.notifyDataSetChanged(); 
		return 0;		
	}
	
	int RemoveItem(int index)
	{
		if( index < 0 )
		{
			mItemArray.clear();
			mListArray.clear();
		}
		else
		{
			mItemArray.remove(index);
			mListArray.remove(index);
		}		
		mListAdapter.notifyDataSetChanged();
		return 0;
	}
	
	int ChangeItem(int index, MyListItem m)
	{	
		mItemArray.set(index, m);
		if( m.UserID.compareTo(PrefSetting.mUser) == 0 )
			mListArray.set(index, "**" + m.Name + "  [" + m.ph_num + "]");
		else
			mListArray.set(index,  m.Name + "  [" + m.ph_num + "]" );		
		mListAdapter.notifyDataSetChanged();		
		return 0;
	}
	
	int ChangeItem(int index, int id,  String userid, String name, String  num)
	{
		MyListItem m = new MyListItem(id, userid, name, num);
		mItemArray.set(index, m);
		if( m.UserID.compareTo(PrefSetting.mUser) == 0 )
			mListArray.set(index, "**" + m.Name + "  [" + m.ph_num + "]");
		else
			mListArray.set(index,  m.Name + "  [" + m.ph_num + "]" );		
		mListAdapter.notifyDataSetChanged();		
		return 0;
	}
}

public class JSyncDemoActivity extends Activity implements OnItemClickListener  {		
	
	static
    {    		
		//System.loadLibrary("openML");
		//System.loadLibrary("jedb");
		/*
		System.loadLibrary("mSyncClient");
		System.loadLibrary("mSyncClientJNI");
		*/
    } 	
	

	ListView mlv_List;
	Button mbt_del;
	
	private static final int EDIT_ACTIVITY = 0;
	private static final int DELETE_ACTIVITY = 1;	
	private static final int CONFIGURE_ACTIVITY = 2;
	
	static PrefSetting mMyConf = null;		
	static MyListArray mMyList = new MyListArray();	
	static JSync2Local mldb = new JSync2Local();
	static JSync2Server mSyncSrv = new JSync2Server();
	static int mNextID = -1;
	
	static int mInit = 0;
	static void ShowMyMessage(Context cxt, String msg)
	{
		AlertDialog.Builder alert = new AlertDialog.Builder( cxt );		
		alert.setPositiveButton("확인", new DialogInterface.OnClickListener() {
		    //@Override
		    public void onClick(DialogInterface dialog, int which) {
		    dialog.dismiss();     //닫기
		    }
		});
		alert.setTitle("Alert");
		alert.setMessage(msg);
		alert.show();
	}
	
	void JSyncDemo_Init()
	{
		int ret;
		
		if( mInit != 0 )
		{
			if( mMyList.mListAdapter.getCount() <= 0 )
				((Button)this.findViewById(R.id.bt_delete)).setEnabled(false);				
			
			return;
		}
		
        ret = mldb.InitLocalDB( this.getString(R.string.app_name) );
        if( ret < 0 )
        {
    	   ShowMyMessage(this, JSync2Local.g_errMessage);
    	   finish();
    	   return;
        }  
            	
    	((Button)this.findViewById(R.id.bt_new)).setEnabled(true) ;    	
    	((Button)this.findViewById(R.id.bt_sync)).setEnabled(true);
    	
    	mMyList.mItemArray.clear();
		mMyList.mListArray.clear();
		mMyList.mListAdapter.clear();
                      
        ret = FillList();        
        mMyList.mListAdapter.notifyDataSetChanged();        
        if( ret < 0 )
        {
    	   ShowMyMessage(this, JSync2Local.g_errMessage);    	 
    	   return;
        }    
                
        if( mMyList.mListAdapter.getCount() <= 0 )
        	mbt_del.setEnabled(false);        	
        else
        	mbt_del.setEnabled(true);
        
        mNextID = mldb.GetNextID();
        if( mNextID < 0 )
        {
        	ShowMyMessage(this, JSync2Local.g_errMessage);
        	return;
        }  
        
        mInit = 1;
	}
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);        
                     
        mMyConf = new  PrefSetting(this);	
        mMyConf.ReadConfig();        
        mMyConf.CheckConfigValue();
        
        mInit = 0;
              
        mbt_del =  (Button)this.findViewById(R.id.bt_delete);
        mlv_List = (ListView)this.findViewById(R.id.lv_list );        
        //mMyList.mListAdapter = new ArrayAdapter<MyListItem>(this, android.R.layout.simple_list_item_1, mMyList.mItemArray );
        mMyList.mListAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1, mMyList.mListArray );        
        //mMyList.mListAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_list_item_checked, mMyList.mListArray );
 
        mlv_List.setAdapter(mMyList.mListAdapter);
                        
        mlv_List.setOnItemClickListener(this);
        mlv_List.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE);
        
        if( mMyConf.mbChecked == false )
        {	
        	((Button)this.findViewById(R.id.bt_new)).setEnabled(false); 
        	((Button)this.findViewById(R.id.bt_sync)).setEnabled(false);
        	((Button)this.findViewById(R.id.bt_delete)).setEnabled(false);        	
        }
        else
        {	
        	JSyncDemo_Init();	        
        }
    }    
    
    @Override
    public void onDestroy()
   	{
   		super.onDestroy();
   		
    	mldb.QuitLocalDB();       	
    }
    
    public void onItemClick(AdapterView<?> parent, View v, int position, long id)
    {	
    	MyListItem m = mMyList.mItemArray.get(position);
    	Bundle extras = new Bundle();    	
    	extras.putInt("pos", position);    	
    	Intent intent = new Intent(this, JSyncDemoEditActivity.class);         
    	intent.putExtras(extras);
    	intent.putExtra("ItemObj", m);
    	startActivityForResult(intent, EDIT_ACTIVITY);    	
    }
    
    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent intent)
    {
    	super.onActivityResult(requestCode, resultCode, intent);
    	 
    	switch(requestCode)
    	{
    	case EDIT_ACTIVITY: 
    		if(resultCode == RESULT_OK)
    		{ 
    			int nPos = intent.getIntExtra("pos", 0);
    			
    			MyListItem  m;
                m = (MyListItem)intent.getSerializableExtra("ItemObj");        
                //Log.v("MyMsg", "pos=" + nPos + "id=" + m.ID );
                
                if( nPos < 0 )
                {
                	 mMyList.AddItem( m );                	 
                	 mbt_del.setEnabled(true);                	 
                }
                else
                {
                	 mMyList.ChangeItem(nPos, m);
                }
    		}
    		break;
    		
    	case DELETE_ACTIVITY:
    		if(resultCode == RESULT_OK)
    		{      			
    			mMyList.mListAdapter.notifyDataSetChanged();
    			if( mMyList.mListAdapter.getCount() <= 0 )
    	        	mbt_del.setEnabled(false);        	
    	        else
    	        	mbt_del.setEnabled(true);
    			
    			mMyList.mListAdapter.notifyDataSetChanged();
    		}
    		break;
    		
    	case CONFIGURE_ACTIVITY:
    		if(resultCode == RESULT_OK)
    		{
    			JSyncDemo_Init();
    		}
    		break;
    	}
    }
    
    public void MainClickHandler(View target)
    {
    	Intent intent;
    	
    	switch(target.getId())
    	{
    	case R.id.bt_new:
    		Bundle extras = new Bundle();
        	extras.putInt("pos", -1);        	
        	intent = new Intent(this, JSyncDemoEditActivity.class);         
        	intent.putExtras(extras);        	
        	startActivityForResult(intent, EDIT_ACTIVITY);        	
    		break;
    		
    	case R.id.bt_sync:
    		DoSync();
    		break;
    		
    	case R.id.bt_delete:
    		//if( mMyList.mListArray.size() > 0  )
    		if( mMyList.mListAdapter.getCount() > 0 )    		
    		{
	    		intent = new Intent(this,  JSyncDemoDeleteActivity.class);    		
	        	startActivityForResult(intent, DELETE_ACTIVITY);    		  		
    		}
    		break;
    		
    	case R.id.bt_configure:
    		intent = new Intent(this, JSyncDemoConfigActivity.class);         
        	startActivityForResult(intent, CONFIGURE_ACTIVITY);        	
    		//startActivity(intent);
    		break;    		
    	}    	
    }    
    
    static String checkShowData( String s1, String s2 )
    {
    	if( !s1.isEmpty() )
    		return s1;
    	
    	if( !s2.isEmpty() )
    		return s2;
    	
    	return "";    	
    }
    
     
    int FillList()
    {    	   
    	int   i, nRows;
    	DemoRecord  Record;    	
    	String Query = "select id, username, name, Company,  company_phone, mobile_phone from contact ";
    				
		nRows = mldb.SelectLocalDB(Query);
	    if( nRows < 0  )
	    {       
	    	mldb.QuitSelectLocalDB();		    	
	        return -1;
	    }
	    else if( nRows > 0  )
	    {		    	   	
		    Record = new DemoRecord();
		    
		    for( i = 0; i < nRows; i++ )
		    {		    
		        if( mldb.FetchLocalDB(Record) <= 0 )
		        {     
		        	mldb.QuitSelectLocalDB();
		        	return -2;	            
		        }   
		    		    	        
		        mMyList.AddItem_Bulk(Record.ID, Record.username,
		        		             checkShowData(Record.Name, Record.Company),
		        		             checkShowData(Record.Mobile_phone, Record.Company_phone) );
		    }
	    }
            
    	mldb.QuitSelectLocalDB();
		
        return 0;
    }
    
    int DoSync()
    {
    	int ret;     		
		ret = mSyncSrv.DoSyncDB( mMyConf.mUser, mMyConf.mPWD );   
		if( ret < 0 )
		{
			ShowMyMessage(this, JSync2Server.g_errMessage);			
		}
		else if( ret == JSync2Server.RET_NEED_UPGRADE )
		{
			 // need upgrade version 
			ShowMyMessage(this, "UPGRADE가 필요합니다.");
			finish();
		}
		else
		{
			Toast toast = Toast.makeText(JSyncDemoActivity.this, "동기화 작업을 완료했습니다.", Toast.LENGTH_LONG);
			toast.setGravity( Gravity.CENTER, 0 , 0 );
			toast.show();
				
			mMyList.mItemArray.clear();
			mMyList.mListArray.clear();
			mMyList.mListAdapter.clear();
		
			ret = FillList();        
	        mMyList.mListAdapter.notifyDataSetChanged();        
	        if( ret < 0 )
	        {
	    	   ShowMyMessage(this, JSync2Local.g_errMessage);   	 
	        }
	        
	        ret = mldb.ResetMaxID();
	        if( ret < 0 )
	        {
	        	ShowMyMessage(this, JSync2Local.g_errMessage);
	        }
	        
	        mNextID = mldb.GetNextID();
	        if( mNextID < 0 )
	        {
	        	ShowMyMessage(this, JSync2Local.g_errMessage);	        	
	        }			
		}
		
		mMyList.mListAdapter.notifyDataSetChanged();
		if( mMyList.mListAdapter.getCount() <= 0 )
        	mbt_del.setEnabled(false);        	
        else
        	mbt_del.setEnabled(true);
		
		return ret;    	
    }
}
