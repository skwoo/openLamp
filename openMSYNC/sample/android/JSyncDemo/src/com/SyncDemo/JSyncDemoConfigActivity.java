package com.SyncDemo;

import com.openml.JEDB;

import android.app.Activity;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.Gravity;
import android.view.View;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Toast;

public class JSyncDemoConfigActivity extends Activity {
	EditText  med_ip;
	EditText  med_port;
	EditText  med_timeout;
	EditText  med_user;
	EditText  med_pwd;
	CheckBox  mchk_isfull;
	
	@Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.configure);               
                
        med_ip      = (EditText) this.findViewById(R.id.ed_ip);
        med_port    = (EditText) this.findViewById(R.id.ed_port);
        med_timeout = (EditText) this.findViewById(R.id.ed_timeout);
        med_user    = (EditText) this.findViewById(R.id.ed_user);
        med_pwd     = (EditText) this.findViewById(R.id.ed_pwd);
        mchk_isfull = (CheckBox) this.findViewById(R.id.chk_fullsync);
      
        med_ip.setText(PrefSetting.mIP);
        med_port.setText( Integer.toString(PrefSetting.mPort) );
        med_timeout.setText( Integer.toString(PrefSetting.mTimeout) );
        med_user.setText(PrefSetting.mUser);
        med_pwd.setText(PrefSetting.mPWD);                
        mchk_isfull.setChecked(PrefSetting.mIsFullSync);
        
        med_ip.setNextFocusDownId(R.id.ed_port);
        med_port.setNextFocusDownId(R.id.ed_timeout);
        med_timeout.setNextFocusDownId(R.id.ed_user);
        med_user.setNextFocusDownId(R.id.ed_pwd);
        med_pwd.setNextFocusDownId(R.id.chk_fullsync);

        med_ip.addTextChangedListener(textInputWatcher);     
        
        med_ip.setOnFocusChangeListener( new View.OnFocusChangeListener() {			
			public void onFocusChange(View v, boolean hasFocus) {
				// TODO Auto-generated method stub
				if( hasFocus ) return;				
				if( CheckIPAddr( med_ip.getText().toString().trim() , true) == false )
				{
					med_ip.requestFocus();
					Show_CheckMessage( "IP 주소 형식이 올바르지 않습니다");	
				}
			}
		} );
        
        med_port.setOnFocusChangeListener( new View.OnFocusChangeListener() {			
			public void onFocusChange(View v, boolean hasFocus) {
				// TODO Auto-generated method stub
				if( hasFocus ) return;
				if( med_port.getText().toString().isEmpty() )
					med_port.requestFocus();
			}
		} );
        
        med_timeout.setOnFocusChangeListener( new View.OnFocusChangeListener() {			
			public void onFocusChange(View v, boolean hasFocus) {
				// TODO Auto-generated method stub
				if( hasFocus ) return;		
				if( med_timeout.getText().toString().isEmpty() )
					med_timeout.requestFocus();
			}
		} ); 
        
        med_user.setOnFocusChangeListener( new View.OnFocusChangeListener() {			
			public void onFocusChange(View v, boolean hasFocus) {
				// TODO Auto-generated method stub
				if( hasFocus ) return;		
				if( med_user.getText().toString().isEmpty() )
					med_user.requestFocus();
			}
		} ); 
        
        med_pwd.setOnFocusChangeListener( new View.OnFocusChangeListener() {			
			public void onFocusChange(View v, boolean hasFocus) {
				// TODO Auto-generated method stub
				if( hasFocus ) return;			
				if( med_pwd.getText().toString().isEmpty() )
					med_pwd.requestFocus();
			}
		} ); 
        
	}	
		
	TextWatcher textInputWatcher = new TextWatcher() {          
  
        public void onTextChanged(CharSequence s, int start, int before, int count) {
            if( CheckIPAddr( med_ip.getText().toString().trim(), false) == false )
			{				
				Show_CheckMessage( "IP 주소 형식이 올바르지 않습니다");	
			}
        }  
 
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {                     
        }  
 
        public void afterTextChanged(Editable s) { 			
		}		
    } ;   
        
	
	public void ConfClickHandler(View target)
	{		
    	switch(target.getId())
    	{
    	case R.id.bt_conf_create:
    		int ret;    		
    		ret = JSyncDemoActivity.mldb.ReMakeDB( this.getString(R.string.app_name) );    		  
    		if( ret < 0 )
	        {
    		   JSyncDemoActivity.ShowMyMessage(this, JSync2Local.g_errMessage);
    		   this.setResult(RESULT_CANCELED, null);
	    	   finish();
	    	   return;
	        }    		
    		JSyncDemoActivity.ShowMyMessage(this, "DB를 새로 생성하여습니다.");
    		
    		JSyncDemoActivity.mMyList.mItemArray.clear();
			JSyncDemoActivity.mMyList.mListArray.clear();
			JSyncDemoActivity.mMyList.mListAdapter.clear();
			JSyncDemoActivity.mMyList.mListAdapter.notifyDataSetChanged();
    		
    		JSyncDemoActivity.mNextID = JSyncDemoActivity.mldb.GetNextID();    		 
    		if( ret < 0 )
    	    {
    			JSyncDemoActivity.ShowMyMessage(this, JSync2Local.g_errMessage);
    	       	return;
    	    }    	    
    		break;
    	
    	 case R.id.bt_conf_ok:    		
    		 PrefSetting.mIP = med_ip.getText().toString() ;    		     		 
    		 PrefSetting.mPort = Integer.parseInt( med_port.getText().toString() );    		 
    		 PrefSetting.mPort = Integer.parseInt( med_port.getText().toString() );    		 
    		 PrefSetting.mTimeout = Integer.parseInt( med_timeout.getText().toString() );    		 
    		 PrefSetting.mUser = med_user.getText().toString();    		 
    		 PrefSetting.mPWD = med_pwd.getText().toString();    		
    		 PrefSetting.mIsFullSync = mchk_isfull.isChecked();    		 
    		 PrefSetting.CheckConfigValue();    		 
    		 PrefSetting.WriteConfig();
    		 this.setResult(RESULT_OK, null);
    		 finish();
    		 break;
    		
    	case R.id.bt_conf_cancel:    
    		this.setResult(RESULT_OK, null);
    		finish();
    		break;
    	}
	}	
	
	static public boolean CheckIPAddr(String ipStr, boolean isStrict)	{		
		
		byte [] bstr = ipStr.getBytes();
		int len = bstr.length;
		
		if( len > 15/* 255.255.255.255 */ )
			return false; 
		else if( isStrict )
		{
			if( len < 7 /* 1.2.3.4 */)
				return false;
			if( bstr[bstr.length -1] == '.' )
				return false;
		}
		
		if( bstr[0] == '.' )
			return false;			
		
		int i;
		int dot_cnt = 0;
				
		len = 0;		
		while( len < bstr.length )
		{
			for( i = 0; i < 3; i++ )
			{
				if( len >= bstr.length )
					break;
					
				if( bstr[len] < '0' || bstr[len] > '9' )
					break;				
				
				len++;
			}
				
			if( len >= bstr.length )
				break;
			
			if( bstr[len] != '.'  )
				return false;
			
			dot_cnt++;
			len++;						
		}
				
		if( isStrict )
		{
			 if( dot_cnt != 3 )
				 return false;
		}
		else if( dot_cnt > 3 )
			return false;
				 
		return true;	
	}
	
	private void Show_CheckMessage(String msg)	{							
		Toast toast = Toast.makeText( JSyncDemoConfigActivity.this, msg, Toast.LENGTH_LONG);
		toast.setGravity( Gravity.CENTER, 0 , 0 );
		toast.show();		
	}
	
}


