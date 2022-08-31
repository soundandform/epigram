/*
 *  JdCore.cpp
 *  Jigidesign
 *
 *  Created by Steven Massey on 8/11/11.
 *  Copyright 2011 Jigidesign. All rights reserved.
 *
 */
 
#include "JdCore.hpp"
#include "Epigram.hpp"
#include "IEpigram.hpp"
#include "JdResult.hpp"


JdPreconditions JdPreconditions::precoditions = JdPreconditions ();

JdPreconditions::JdPreconditions ()
{
//	d_jdAssert (sizeof (JdResult) == 512,			"JdResult is misaligned");

	d_jdAssert (sizeof (Epigram) <= 1024,			"Epigram is missized");
	d_jdAssert (sizeof (Epigram128) == 128,			"Epigram is missized");
	
	d_jdAssert (sizeof (JdString256) == 256,		"JdFlatString whacked");
	d_jdAssert (sizeof (JdString32) == 32,			"JdFlatString whacked");
}

namespace jd
{
	std::ostream & operator << (std::ostream & stream, const fixed & i_dummy)			{ return (stream << std::fixed); }

	void _defaultOutHandler (cstr_t i_cstr)
	{
		std::cout << i_cstr << std::endl;
		std::cout.flush ();
	}

	outHandler_t outHandler (const outHandler_t i_outHandler)
	{
		static outHandler_t s_outHandler = _defaultOutHandler;
		
		if (i_outHandler)
			s_outHandler = i_outHandler;
		
		return s_outHandler;
	}

}

namespace Jd
{
	std::ostream & operator << (std::ostream & stream, const FormatDefault & i_dummy)	{ return stream; }

	u32		Pow2CeilLog2 (u32 i_value)
	{
		u32 v = i_value, s = i_value;
		while (s >>= 1)
		{
			v |= s;
		}
		v++;
		
		if (v == (i_value << 1)) v = i_value;
		
		return v;
	}
	
	u32		RoundUpToAPowerOf2 (u32 i_value)
	{
		return Pow2CeilLog2 (i_value);
	}
	
	u32		CeilLog2 (u32 i_value)
	{
		u32 shift = 0;
		if (i_value)
		{
			--i_value;
			while (i_value) { i_value >>= 1; ++shift; }
		}
		return shift;
	}
	
	
	template <>
	std::string ToString (const bool &i_value)
	{
		return i_value ? "true" : "false";
	}
	

	void SSPrintF (std::ostringstream & o_oss, cstr_t i_format)
	{
		cstr_t format = i_format;
		
		while (* format)
		{
			if (* format == c_epilogInsertToken)
			{
				if (* (format + 1) == c_epilogInsertToken)
				{
					++format;
				}
				else
				{
					std::string msg = "invalid ssprintf: '";
					(msg += format) += "' is missing argument(s)";
					
					# if __cpp_exceptions
						throw std::runtime_error (msg);
					# else
						abort ();
					# endif
				}
			}
			
			o_oss << *format++;
		}
	}
	
	std::string SPrintF (cstr_t i_format)
	{
		return i_format;
	}
}

