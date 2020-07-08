//
//  JdOptions.hpp
//
//  Created by Steven Massey on 7/7/20.
//  Copyright © 2020 Steven Massey. All rights reserved.
//

#ifndef JdOptions_hpp
#define JdOptions_hpp

#include "IJdModule.hpp"

d_jdModuleInterface (IJdOptions, 1.0)
{
	d_jdMethodDecl		(void,		Set,			string key;	string value)
//	d_jdMethodDecl		(void,		Set,			string key;	u64 value)

	d_jdMethodDecl		(string,	Get,			string key;	string type)
	d_jdMethodDecl1		(i64,		Get,			string key;	i64 type)

	d_jdMethodDecl		(string,	GetString,		string key)
//	d_jdMethodDecl		(i64,		GetInt,			string key)
};

d_jdLinkDef (Options)



template <typename T>
struct JdOptionT
{
	JdOptionT (IJdOptions & i_options, cstr_t i_name)
	:
	j_options		(i_options),
	m_key 			(i_name)
	{ }
	
	void 		Set				(const T & i_value)
	{
		jd_lock (j_options) j_options->Set ({ m_key, i_value });
	}

	
	void		operator =		(const T & i_value)
	{
		Set (i_value);
	}
	
	T	 		Get				()
	{
		T value;
		jd_lock (j_options)
			value = j_options->Get ({ m_key, value });
		return value;
	}
	
	operator T					()
	{
		return Get ();
	}

	protected:

	IJdOptions &		j_options;
	cstr_t				m_key			= nullptr;
};




#endif /* JdOptions_hpp */
