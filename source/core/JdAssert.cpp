//
//  JdAssert.cpp
//  Jigidesign
//
//  Created by Steven Massey on 6/29/16.
//  Copyright © 2016 Jigidesign. All rights reserved.
//

# if __APPLE__
#	include <signal.h>
#	include <unistd.h>
# endif

#include "JdAssert.hpp"
using namespace std;


JdException::JdException (cstr_t i_message, cstr_t i_location, u32 i_line)
:
JdResult (i_message, i_location, i_line)
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


void JdAssert (cstr_t i_truthfulOrLyingStatement, cstr_t i_filePath, u32 i_lineNum, stringRef_t i_message)
{
	std::ostringstream oss;
	// FIX: this should probably be wrapped in a JdException. Upper level code that catches the exception will log, display, or cerr the .what()
	if (strcmp (i_truthfulOrLyingStatement, "0") == 0)
		oss << "\n| throw: ";
	else
		oss << "\n| '" << i_truthfulOrLyingStatement << "' assertion failed. ";
	
	oss << i_message;
	
	cstr_t file = strstr (i_filePath, "../");
	if (! file) file = i_filePath;
	else file += 2;
	
	if (file)
	{
		oss << "\n+" << string (strlen (file) + 7, '-');
		oss << "\n| " << file << ":" << i_lineNum;
	}
	
	//epilog_(fatal, "%s", epistr (oss.str()));
	std::cout << oss.str() << std::endl << std::endl;
	
	// Break into the debugger
	if (not JdConfig::IsUnitTest () and JdConfig::IsDebug ())
	{
		#if __APPLE__
			usleep (50000);		// give epilog 50ms to flush.
		#endif
//			raise (SIGTRAP);
	}
	
	# if __cpp_exceptions
		throw JdException (oss.str().c_str(), i_filePath, i_lineNum);
	# else
		abort ();
	# endif
}


JdResult JdResert (cstr_t i_truthfulOrLyingStatement, cstr_t i_filePath, u32 i_lineNum, stringRef_t i_message)
{
	JdResult result;
	
	if (JdConfig::IsDebug ())
	{
		std::ostringstream oss;
		// FIX: this should probably be wrapped in a JdException. Upper level code that catches the exception will log, display, or cerr the .what()
		if (strcmp (i_truthfulOrLyingStatement, "0") == 0)
			oss << "\n| throw: ";
		else
			oss << "\n| '" << i_truthfulOrLyingStatement << "' assertion failed. ";
		
		oss << i_message;
		
		cstr_t file = strstr (i_filePath, "../");
		if (! file) file = i_filePath;
		else file += 2;
		
		if (file) oss << "\n| " << file << ":" << i_lineNum;
		
		//epilog_(fatal, "%s", epistr (oss.str()));
		std::cout << oss.str() << std::endl << std::endl;
		
		// Break into the debugger
		if (not JdConfig::IsUnitTest () and JdConfig::IsDebug ())
		{
			#if __APPLE__
				usleep (50000);		// give epilog 50ms to flush.
			#endif
//				raise (SIGTRAP);
		}
		
		# if __cpp_exceptions
			throw JdException (oss.str().c_str(), i_filePath, i_lineNum);
		# else
			abort ();
		# endif
	}
	else
	{
		result = JdResult (i_message.c_str(), i_filePath, i_lineNum);
	}
	
	return result;
}

