//
//  JdAssert.hpp
//  Jigidesign
//
//  Created by Steven Massey on 6/29/16.
//  Copyright © 2016 Jigidesign. All rights reserved.
//

#ifndef JdAssert_hpp
#define JdAssert_hpp

# if __APPLE__
#	include <signal.h>
#	include <unistd.h>
# endif

#include "JdConfig.hpp"
#include "JdResult.hpp"


class JdException : public std::exception, public JdResult
{
	public:
	
	JdException (cstr_t i_message, cstr_t i_location = 0, u32 i_line = 0);

	virtual const char * what() const throw ();
};


template <typename... args_t>
void JdAssert (bool i_shouldBeTrue, cstr_t i_truthfulOrLyingStatement, cstr_t i_filePath, u32 i_lineNum, cstr_t i_format, args_t... i_args)
{
	if (! i_shouldBeTrue)
	{
		std::ostringstream oss;
		// FIX: this should probably be wrapped in a JdException. Upper level code that catches the exception will log, display, or cerr the .what()
		if (strcmp (i_truthfulOrLyingStatement, "0") == 0)
			oss << "\n| throw: ";
		else
			oss << "\n| '" << i_truthfulOrLyingStatement << "' assertion failed. ";
		
		if (i_format)
		{
			Jd::SSPrintF (oss, i_format, i_args...);
		}
		
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
}


template <typename... args_t>
JdResult JdResert (bool i_shouldBeTrue, cstr_t i_truthfulOrLyingStatement, cstr_t i_filePath, u32 i_lineNum, cstr_t i_format, args_t... i_args)
{
	JdResult result;
	
	if (! i_shouldBeTrue)
	{
		if (JdConfig::IsDebug ())
		{
			std::ostringstream oss;
			// FIX: this should probably be wrapped in a JdException. Upper level code that catches the exception will log, display, or cerr the .what()
			if (strcmp (i_truthfulOrLyingStatement, "0") == 0)
				oss << "\n| throw: ";
			else
				oss << "\n| '" << i_truthfulOrLyingStatement << "' assertion failed. ";
			
			if (i_format)
			{
				Jd::SSPrintF (oss, i_format, i_args...);
			}
			
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
			std::ostringstream oss;
			if (i_format) Jd::SSPrintF (oss, i_format, i_args...);
			
			result = JdResult (oss.str().c_str(), i_filePath, i_lineNum);
		}
	}
	
	return result;
}



#define d_jdPermAssert(TRUTH, ...) JdAssert (TRUTH, #TRUTH, __FILE__, __LINE__, __VA_ARGS__)

#if DEBUG
	#define d_jdAssert(TRUTH, ...) JdAssert (TRUTH, #TRUTH, __FILE__, __LINE__, __VA_ARGS__)
#else
	#define d_jdAssert(...)
#endif

#define d_jdResert(TRUTH, ...) JdResert (TRUTH, #TRUTH, __FILE__, __LINE__, __VA_ARGS__)

#define d_jdThrow(...) JdAssert (false, "throw", __FILE__, __LINE__, __VA_ARGS__)
#define d_jdFix() d_jdThrow ("fix")


#endif /* JdAssert_hpp */
