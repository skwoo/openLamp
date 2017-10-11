package com.SyncDemo;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

import com.SyncDemo.*; 

public class JSyncDemoEditActivity extends Activity {
	
	int mPos = 0;
	int mID = -1;
	String muserID = "";
	EditText med_name;
	EditText med_co;
	EditText med_ph1;
	EditText med_ph2;
	EditText med_email;
	EditText med_addr;
	
	@Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.edit);
        
        med_name = (EditText) this.findViewById(R.id.ed_name);
        med_co = (EditText) this.findViewById(R.id.ed_company);
        med_ph1 = (EditText) this.findViewById(R.id.ed_phnum1);
        med_ph2 = (EditText) this.findViewById(R.id.ed_phnum2);
        med_email = (EditText) this.findViewById(R.id.ed_email);
        med_addr = (EditText) this.findViewById(R.id.ed_addr);
        
        mPos = getIntent().getIntExtra("pos", 0);
        if( mPos < 0 )
        { // Add new person
        	mID = JSyncDemoActivity.mNextID;       
        	 muserID = PrefSetting.mUser;
        }
        else
        { // show detail & update
        	String str;
            int ret;    	
            DemoRecord OldRec = new DemoRecord();
        	MyListItem  m;
        	
            m = (MyListItem)getIntent().getSerializableExtra("ItemObj");            
            //Log.v("MyMsg", "ID=" +  m.ID + "Name=" + m.Name + "Num=" + m.ph_num);
            	    
    	    mID = m.ID;
    	    muserID = m.UserID;
    	    str = String.format("select company, name, e_mail, company_phone, mobile_phone, address" +
    	                 " from contact where id = %d AND username = '%s'", mID, muserID);
    	
    	    ret = JSyncDemoActivity.mldb.SelectLocalDB(str);
    	    if( ret < 0 )
    	    {
    	    	JSyncDemoActivity.ShowMyMessage(this, JSyncDemoActivity.mldb.g_errMessage);
    	    	finish();    	    	
    	    }
    	    else if( ret == 0 )
    	    {
    	    	JSyncDemoActivity.mldb.QuitSelectLocalDB();
    	    	JSyncDemoActivity.ShowMyMessage(this, "대상 레코드가 없습니다.");
    	    	finish();
    	    }
    	    else
    	    {    	    
    	    	ret = JSyncDemoActivity.mldb.FetchLocalDB(OldRec);
    	    	JSyncDemoActivity.mldb.QuitSelectLocalDB();
	    	    if( ret < 0 )
	    	    {	
	    	    	JSyncDemoActivity.ShowMyMessage(this, JSyncDemoActivity.mldb.g_errMessage);
	    	    	finish();
	    	    }	    
	    	    	    	    
	    	    med_name.setText( OldRec.Name );
	    	    med_co.setText( OldRec.Company );
	    	    med_ph1.setText( OldRec.Mobile_phone );
	    	    med_ph2.setText( OldRec.Company_phone );
	    	    med_email.setText( OldRec.e_mail );
	    	    med_addr.setText( OldRec.Address );
    	    }
        }        
	}       
	
	@Override
    public void onDestroy()
	{
		super.onDestroy();		
		this.setResult(RESULT_CANCELED, null);
	}
		
	public void EditClickHandler(View target)
	{		
    	switch(target.getId())
    	{
    	case R.id.bt_ok:
    		if( save_record() < 0 )
    		{
    			JSyncDemoActivity.ShowMyMessage(this, JSyncDemoActivity.mldb.g_errMessage);
    			break;
    		}  
    		
    		if( mPos < 0 )
    	    { // Add new person
    			JSyncDemoActivity.mNextID++;
    	    }
    		
    		Intent intent = new Intent(this, JSyncDemoActivity.class);
    		MyListItem m = new MyListItem( mID, muserID,
    				                       JSyncDemoActivity.checkShowData(med_name.getText().toString().trim(), med_co.getText().toString().trim()), 
    				                       JSyncDemoActivity.checkShowData(med_ph1.getText().toString().trim(), med_ph2.getText().toString().trim())
    				                       );
    		intent.putExtra("ItemObj", m);
    		
    		Bundle extras = new Bundle();
        	extras.putInt("pos", mPos);
    		intent.putExtras(extras);        	    		
    		this.setResult(RESULT_OK, intent);
    		finish();
    		break;
    		
    	case R.id.bt_cancel:    
    		// this.setResult(RESULT_CANCELED, null);
    		finish();
    		break;
    	}
	}	
	
	int save_record()
	{
		String str;
		
		if( mPos < 0 )
		{
			str = String.format("insert into contact(id, username, company, name, e_mail, company_phone," +
	                   "mobile_phone, address, mtime) " +
	                   "values(%d, '%s','%s', '%s', '%s', '%s', '%s', '%s', now())",
	                   mID, muserID,
	                   med_co.getText().toString(),
	                   med_name.getText().toString(),
	                   med_email.getText().toString(),
	                   med_ph2.getText().toString(),
	                   med_ph1.getText().toString(),
	                   med_addr.getText().toString() );	            
			
			int ret = JSyncDemoActivity.mldb.Query2LocalDB(str, false);
			if( ret < 0 )
				 JSyncDemoActivity.mldb.CommitLocalDB(false);
			
			str = String.format("update contactID set maxid = %d where username =  '%s'", 
					mID, muserID);
			
			return JSyncDemoActivity.mldb.Query2LocalDB(str, true);						
		}
		else
		{
			 str = String.format("update contact set company = '%s', name = '%s', e_mail = '%s'," +
	                 "company_phone = '%s', mobile_phone = '%s', address = '%s', " +
	                 "mtime = now() where id = %d AND username = '%s' ",	                 	                 
	                 med_co.getText().toString(),
	                 med_name.getText().toString(),
	                 med_email.getText().toString(),
	                 med_ph2.getText().toString(),
	                 med_ph1.getText().toString(),
	                 med_addr.getText().toString(),
	                 mID,
	                 muserID);
			 
			 return JSyncDemoActivity.mldb.Query2LocalDB(str, true);
		}		
	}
	
}


