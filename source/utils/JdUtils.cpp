//
//  JdUtils.cpp
//  Jigidesign
//
//  Created by Steven Massey on 9/3/15.
//  Copyright (c) 2015 Jigidesign. All rights reserved.
//

#include "JdUtils.hpp"
#include <iomanip>


JdByteSize::JdByteSize (size_t i_size)
:
m_size (i_size)
{
}

JdByteSize::operator size_t () const
{
	return m_size;
}


std::ostream& operator<< (std::ostream &output, const JdByteSize &i_size)
{
	u64 size = i_size;
	if (size < 1024) output << std::setprecision(4) << size << " bytes";
	else if (size < 1048576) output << std::setprecision(4) << double (size) / 1024 << " kB";
	else if (size < 1073741824) output << std::setprecision(5) << double (size) / 1048576 << " MB";
	else output << std::setprecision(3) << double (size) / 1073741824 << " GB";
	
	return output;
}

