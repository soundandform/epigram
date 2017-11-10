//
//  JdAssert.cpp
//  Jigidesign
//
//  Created by Steven Massey on 6/29/16.
//  Copyright © 2016 Jigidesign. All rights reserved.
//

#include "JdAssert.hpp"

JdException::JdException (cstr_t i_message, cstr_t i_location, u32 i_line)
:
JdResult (i_message, i_location, i_line, 0 /* column */, true)
{
	if (i_location)
	{
		m_message += " @ ";
		const char *file = strstr (i_location, "../");
		if (file) m_message += (file + 2);
		else m_message += i_location;
		m_message += ":";
		m_message += Jd::ToString (i_line);
	}
	
	//		cout << "JdException: " << m_message << endl;
}


const char * JdException::what() const throw ()
{
	return m_message;
}
