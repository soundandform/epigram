//
//  JdAssert.hpp
//  Jigidesign
//
//  Created by Steven Massey on 6/29/16.
//  Copyright © 2016 Jigidesign. All rights reserved.
//

#ifndef JdAssert_hpp
#define JdAssert_hpp


#include "JdConfig.hpp"
#include "JdResult.hpp"


class JdException : public std::exception, public JdResult
{
	public:
	
	JdException (cstr_t i_message, cstr_t i_location = 0, u32 i_line = 0);

	virtual const char * what() const throw ();
};


void JdAssert (bool i_debugBuild, cstr_t i_truthfulOrLyingStatement, cstr_t i_filePath, u32 i_lineNum, stringRef_t i_message);

template <typename... args_t>
void JdAssert (bool i_debugBuild, bool i_shouldBeTrue, cstr_t i_truthfulOrLyingStatement, cstr_t i_filePath, u32 i_lineNum, cstr_t i_format, args_t... i_args)
{
	if (not i_shouldBeTrue)
	{
		std::string message;

		if (i_format)
			message = Jd::SPrintF (i_format, i_args...);
		
		JdAssert (i_debugBuild, i_truthfulOrLyingStatement, i_filePath, i_lineNum, message);
	}
}


JdResult JdResert (cstr_t i_truthfulOrLyingStatement, cstr_t i_filePath, u32 i_lineNum, stringRef_t i_message);


template <typename... args_t>
JdResult JdResert (bool i_shouldBeTrue, cstr_t i_truthfulOrLyingStatement, cstr_t i_filePath, u32 i_lineNum, cstr_t i_format, args_t... i_args)
{
	JdResult result;
	
	if (not i_shouldBeTrue)
	{
		std::string message;
		
		if (i_format)
			message = Jd::SPrintF (i_format, i_args...);
		
//		result = JdResert (i_truthfulOrLyingStatement, i_filePath, i_lineNum, message);
	}
	
	return result;
}



#define d_jdShipAssert(TRUTH, ...) JdAssert (false, TRUTH, #TRUTH, __FILE__, __LINE__, "" __VA_ARGS__)

#if DEBUG
	#define d_jdAssert(TRUTH, ...) JdAssert (DEBUG, TRUTH, #TRUTH, __FILE__, __LINE__, "" __VA_ARGS__)
#else
	#define d_jdAssert(...)
#endif


#define d_jdResert(TRUTH, ...) JdResert (TRUTH, #TRUTH, __FILE__, __LINE__, __VA_ARGS__)

#define d_jdThrow(...) JdAssert (false, false, "throw", __FILE__, __LINE__, __VA_ARGS__)
#define d_jdFix(...) d_jdThrow ("fix: " __VA_ARGS__)


#endif /* JdAssert_hpp */
