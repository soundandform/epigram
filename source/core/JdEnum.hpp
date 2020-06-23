//
//  JdEnum.h
//  Jigidesign
//
//  Created by Steven Massey on 5/3/16.
//  Copyright © 2016 Jigidesign. All rights reserved.
//

#ifndef JdEnum_h
#define JdEnum_h

#include "JdStdStringUtils.hpp"
#include "JdCoreL2.hpp"

#include <ostream>


namespace Jd
{
	template <typename E>
	inline std::string EnumToString (const E & i_enum, cstr_t i_enums)
	{
		auto strings = Jd::SplitString (i_enums, ",");
		
		size_t e = (size_t) i_enum;
		if (e < strings.size ())
			return strings [e];
		else
			return "<enum out of range>";
	}
}



#if _MSC_VER
#	define d_jdEnumStringify(...) #__VA_ARGS__
#else
#	define d_jdEnumStringify(VALUES...) #VALUES
#endif

#define d_jdEnum2Strings(LOWNAME, UPNAME, ...) inline \
	std::ostream & operator << (std::ostream & output, const UPNAME & i_enum) { \
	output << Jd::EnumToString (i_enum, d_jdEnumStringify (__VA_ARGS__)) << ":" << (size_t) i_enum; \
	return output; }


#define d_jdEnum2_(LOWNAME, UPNAME, ...) enum class UPNAME { __VA_ARGS__ }; typedef UPNAME LOWNAME; d_jdEnum2Strings (LOWNAME, UPNAME, __VA_ARGS__)

#define d_jdEnum2(NS, NSUP, NAME, ...) d_jdEnum2_(c_ ## NS ## NAME, E## NSUP ##NAME, null, __VA_ARGS__)

#define d_jdEnum_(NAME, ...) d_jdEnum2 (jd, Jd, NAME, __VA_ARGS__)

// template: #define d_scEnum(NAME, ...) d_jdEnum2 (sc, Sc, NAME, __VA_ARGS__)


#endif /* JdEnum_h */
