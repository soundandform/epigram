//
//  JdMD5.cpp
//
//  Created by Steven Massey on 1/28/25.
//  Copyright © 2025 Steven Massey. All rights reserved.
//

# include "JdMD5.hpp"

std::ostream & operator << (std::ostream & stream, const JdMD5::MD5 & i_md5)
{
	stream << i_md5.GetString ();
	return stream;
}
