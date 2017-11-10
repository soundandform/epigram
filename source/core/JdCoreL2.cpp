//
//  JdCoreL2.cpp
//  Jigidesign
//
//  Created by Steven Massey on 9/4/15.
//  Copyright (c) 2015 Jigidesign. All rights reserved.
//

#include "JdCoreL2.hpp"
#include <city.h>


namespace Jd
{
    i32 HashCString31 (const char *i_string)
    {
        u64 hash = CityHash64 (i_string, strlen (i_string)) & 0x000000007FFFFFFF;
        return (i32) hash;
    }
	
	u64 HashString64 (const string &i_string)
	{
		return CityHash64 (i_string.c_str(), i_string.size());
	}


}; // end-namespace Jd
