//
//  JdCoreL2.cpp
//  Jigidesign
//
//  Created by Steven Massey on 9/4/15.
//  Copyright (c) 2015 Jigidesign. All rights reserved.
//

#include "JdCoreL2.hpp"
#include <cstring>

# ifdef d_epigramUseCityHash
# 	include <city.h>
# endif

namespace Jd
{
    i32 HashCString31 (cstr_t i_string)
    {
		u64 hash = std::hash <std::string> () (i_string);
		hash &= 0x000000007FFFFFFF;
        return (i32) hash;
    }
	
	u64 HashString64 (const std::string &i_string)
	{
		//		return CityHash64 (i_string.c_str(), i_string.size());
		return std::hash <std::string> () (i_string);
	}


}; // end-namespace Jd
