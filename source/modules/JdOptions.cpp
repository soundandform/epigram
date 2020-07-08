//
//  JdOptions.cpp
//
//  Created by Steven Massey on 7/7/20.
//  Copyright © 2020 Steven Massey. All rights reserved.
//

#include "JdOptions.hpp"
#include "IJdSql.h"

#include "JdDbTable.hpp"

d_jdModule (JdOptions, IJdOptions)
{
	JdResult			Setup		(IJdServer i_server) override
	{
		JdResult result = j_sql.Singleton (i_server);
		
		if (not result)
		{
			string sql = Jd::SPrintF ("CREATE TABLE IF NOT EXISTS @ (rowid INTEGER PRIMARY KEY, k UNIQUE, v);", GetTableName ());
			
			jd_lock (j_sql)
			{
				result = j_sql->Prepare (sql.c_str());
				if (not result)
				{
					result = QuietSqlResult (j_sql->Step ());
				}
			}
		}
		
		return result;
	}
	
	template <typename K, typename V>
	JdResult		Set			(const K & i_key, const V & i_value)
	{
		JdResult result;
		
		JdDatabase::QueryGenerator <1024> keyGenerator;
		string key = keyGenerator.EncodeColumns ({"k"_= i_key}, ",", false);
		
		JdDatabase::QueryGenerator <1024> valueGenerator;
		string value = valueGenerator.EncodeColumns ({"v"_= i_value}, ",", false);
		
		string sql = Jd::SPrintF ("INSERT INTO @ (k,v) VALUES (?,?);", GetTableName ());		// cout << sql << endl;
		
		result = j_sql->Prepare (sql.c_str ());
		
		if (not result)
		{
			i32 index = keyGenerator.BindBlobs (j_sql);
			valueGenerator.BindBlobs (j_sql, index);
			
			result = QuietSqlResult (j_sql->Step ());
			
			if (result)
			{
				sql = Jd::SPrintF ("UPDATE @ SET @ WHERE @;", GetTableName (), value, key);		// cout << sql << endl;
				
				result = j_sql->Prepare (sql.c_str ());
				
				index = valueGenerator.BindBlobs (j_sql);
				keyGenerator.BindBlobs (j_sql, index);
				
				result = QuietSqlResult (j_sql->Step ());
			}
		}
		
		return result;
	}
	
	template <typename K, typename V>
	JdResult		Get			(const K & i_key, V & i_value)
	{
		JdResult result;
		
		JdDatabase::QueryGenerator <1024> keyGenerator;
		string key = keyGenerator.EncodeColumns ({"k"_= i_key}, ",", false);
		
		string sql = Jd::SPrintF ("SELECT (v) FROM @ WHERE @", GetTableName (), key);	//	cout << sql << endl;
		
		result = j_sql->Prepare (sql.c_str ());
		
		if (not result)
		{
			keyGenerator.BindBlobs (j_sql);

			result = j_sql->Step ();
			
			if (result == SQLITE_ROW)
			{
				Epigram e = JdDatabase::EncodeRow (j_sql, 0, false);
//				e.dump ();
				e ["v"] >> i_value;
			}
		}
		
		return result;
	}
	
	
	d_jdMethod		(void,		Set,			string key;	string value)
	{
		Set (i_.key, i_.value);
	}
	
	
	d_jdMethod		(string,	GetString,		string key)
	{
		string value;
		Get (i_.key, value);
		
		return value;
	}
	
	
	cstr_t			GetTableName 	()
	{
		return "app_options";
	}

	IJdSql				j_sql;
};









//--------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "JdLibrary.h"

d_jdLibrary (Options)
{
	void Definition ()
	{
		Module	<JdOptions>		(c_jdCreationPolicy::singleton);
	}
};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------

