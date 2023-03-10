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
#		ifdef d_epigramUseCityHash
			u64 hash = CityHash64 (i_string, strlen (i_string));
#		else
			u64 hash = std::hash <std::string_view> () (i_string);
#		endif
		
		hash ^= (hash >> 32);
		hash &= 0x000000007FFFFFFF;
		
        return (i32) hash;
    }
	
	u64 HashString64 (const std::string &i_string)
	{
#		ifdef d_epigramUseCityHash
			return CityHash64 (i_string.c_str(), i_string.size());
#		else
			return std::hash <std::string> () (i_string);
#		endif
	}

	u64 HashString64 (cstr_t i_string)
	{
#		ifdef d_epigramUseCityHash
			return CityHash64 (i_string, strlen (i_string));
#		else
			return std::hash <std::string_view> () (i_string);
#		endif
	}

}; // end-namespace Jd
