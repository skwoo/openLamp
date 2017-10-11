package com.SyncDemo;

import java.util.ArrayList;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.util.SparseBooleanArray;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ListView;
import android.widget.AdapterView.OnItemClickListener;


import  com.SyncDemo.JSyncDemoActivity;

public class JSyncDemoDeleteActivity  extends Activity implements OnItemClickListener {
	Button            mbt_ok;	
	Button            mbt_allreset;
	ListView          mlv_List;
	//ArrayList<String> m_ListArr;	
	ArrayAdapter<String> m_Adapter;	
	boolean mb_isRemoved = false;
		
	@Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);                
        setContentView(R.layout.delete);
        
        mbt_allreset = (Button) this.findViewById(R.id.bt_del_all);
        mbt_ok = (Button)this.findViewById(R.id.bt_del_ok);   
        mbt_ok.setEnabled(false);
        mb_isRemoved = false;
        
        mlv_List = (ListView)this.findViewById(R.id.lv_del);        
        //m_ListArr = new ArrayList<String>();
        //m_Adapter = new ArrayAdapter<String>(this, android.R.layout.simple_list_item_multiple_choice, m_ListArr);        
        m_Adapter = new ArrayAdapter<String>(this, android.R.layout.simple_list_item_multiple_choice, JSyncDemoActivity.mMyList.mListArray);
        
        mlv_List.setAdapter(m_Adapter);
        mlv_List.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE);        
        mlv_List.setOnItemClickListener(this);             
        
        /*
        if( fillResultRecord() < 0 )
        {
        	JSyncDemoActivity.ShowMyMessage(this, JSyncDemoActivity.mldb.g_errMessage);
        	finish();
        }
        */        
        m_Adapter.notifyDataSetChanged();        
	}
	
	@Override
    public void onDestroy()
	{
		super.onDestroy();
		
		if( mb_isRemoved )
			this.setResult(RESULT_OK, null);
		else
			this.setResult(RESULT_CANCELED, null);
	}
	
	public void onItemClick(AdapterView<?> parent, View v, int position, long id)
    {	
		if( mlv_List.isItemChecked(position) )
		{
			if( mlv_List.getCheckedItemCount() == 1 )
			{			
				mbt_ok.setEnabled(true);
				mbt_allreset.setText(R.string.bt_reset);
			}			
		}
		else 
		{
			if( mlv_List.getCheckedItemCount() == 0 )
			{		
				mbt_ok.setEnabled(false);
				mbt_allreset.setText(R.string.bt_all);
			}			
		}    	  	 	
    }
    
	
	public void EditClickHandler(View target)
	{		
    	switch(target.getId())
    	{
    	case R.id.bt_del_ok:
    		int ret;
    		
    		if( mlv_List.getCheckedItemCount() == JSyncDemoActivity.mMyList.mListAdapter.getCount() )
    		{
    			ret = delete_record( -1, "" );    			
    			
    			JSyncDemoActivity.mMyList.mItemArray.clear();
    			JSyncDemoActivity.mMyList.mListArray.clear();
    			JSyncDemoActivity.mMyList.mListAdapter.clear();
    			mb_isRemoved = true;    
    		}
    		else
    		{
	    		int correction_val = 0;
	    		int index;
	    		//int length = m_ListArr.size();
	    		int length = JSyncDemoActivity.mMyList.mListArray.size();    		
	    		SparseBooleanArray checkedItems = mlv_List.getCheckedItemPositions();
	    		    		    		
	    		for(int i = 0; i < length; i++)
	    		{
	    			if (checkedItems.valueAt(i))
	    			{   
	    				// call remove record();
	    				index = checkedItems.keyAt(i) - correction_val;
	    				MyListItem m = JSyncDemoActivity.mMyList.mItemArray.get(index);
	    				ret = delete_record( m.ID, m.UserID );
	    				//m_ListArr.remove(checkedItems.keyAt(i) - correction_val);
	    				
	    				JSyncDemoActivity.mMyList.RemoveItem(index);
	    				//JSyncDemoActivity.mMyList.mListArray.remove(checkedItems.keyAt(i) - correction_val);
	    				correction_val++;    
	    				mb_isRemoved = true;    			
	    			}
	    		}  
    		}
    		
    		m_Adapter.notifyDataSetChanged();    		
    		mlv_List.clearChoices();
    		
    		if( mlv_List.getCount() == 0  )
    		{
    			this.setResult(RESULT_OK, null);
    			finish();
    		}   		
    		
    		break;
    		
    	case R.id.bt_del_cancel:
    		finish();
    		break;
    		
    	case R.id.bt_del_all:
    		
    		if( mbt_allreset.getText().toString().compareTo( this.getString(R.string.bt_all) ) == 0 )
    		{
    			int i;
    			for( i = 0; i < mlv_List.getCount() ; i++ )
    			{
    				mlv_List.setItemChecked(i, true);   				
    			}    			
    			
    			mbt_ok.setEnabled(true);
				mbt_allreset.setText(R.string.bt_reset);
    		}
    		else
    		{
    			int i;
    			for( i = 0; i < mlv_List.getCount() ; i++ )
    			{
    				mlv_List.setItemChecked(i, false);   				
    			}        			   			
    			
    			mbt_ok.setEnabled(false);
    			mbt_allreset.setText(R.string.bt_all);   		
    		}  		
    		break;
    	}
	}	
			
	int delete_record(int idval, String user )
	{
		String str;
		
		if( idval < 0 )
			str = String.format("delete from contact");
		else
			str = String.format("delete from contact where id = %d AND username = '%s'", idval, user);	 
				
		return JSyncDemoActivity.mldb.Query2LocalDB(str, true);		
	}

}
