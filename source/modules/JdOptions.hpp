//
//  JdOptions.hpp
//
//  Created by Steven Massey on 7/7/20.
//  Copyright © 2020 Steven Massey. All rights reserved.
//

#ifndef JdOptions_hpp
#define JdOptions_hpp

#include "JdInterface.h"

d_jdModuleInterface (IJdOptions, 1.0)
{
	d_jdMethodDecl		(void,		Set,			string key;	string value)
//	d_jdMethodDecl		(void,		Set,			string key;	u64 value)

	d_jdMethodDecl		(string,	GetString,		string key)
//	d_jdMethodDecl		(i64,		GetInt,			string key)
};



d_jdLinkDef (Options)


#endif /* JdOptions_hpp */
