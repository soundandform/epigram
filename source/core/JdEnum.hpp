//
//  JdEnum.h
//  Jigidesign
//
//  Created by Steven Massey on 5/3/16.
//  Copyright © 2016 Jigidesign. All rights reserved.
//

#ifndef JdEnum_h
#define JdEnum_h

//#include "EpSerializationRaw.h"
#include "JdStdStringUtils.hpp"
#include "JdCoreL2.hpp"


namespace Jd
{
	template <typename E>
	inline string EnumToString (const E & i_enum, cstr_t i_enums)
	{
		auto strings = Jd::SplitString (i_enums, ",");
		
		size_t e = (size_t) i_enum;
		if (e < strings.size ())
			return strings [e];
		else
			return "<enum out of range>";
	}
}


//
//template <typename TYPE, typename ENUM, typename I>
//struct JdEnumT : JdSerialize::Unversioned <JdEnumT <TYPE,ENUM,I>, I>
//{
//	operator TYPE & ()
//	{
//		return m_value;
//	}
//	
//	const operator TYPE () const
//	{
//		return m_value;
//	}
//
//	JdEnumT & operator = (const TYPE & i_value) { m_value = i_value; return * this; }
//	JdEnumT & operator = (const ENUM & i_value) { m_value = i_value; return * this; }
//	JdEnumT (const TYPE & i_value) { m_value = i_value; }
//	JdEnumT () { }
//	
//	operator std::string () const { return Jd::ToString (* this); }
//	
//	static void OutputValueString (std::ostream & output, u32 i_index, cstr_t i_enums)
//	{
//		if (i_index < ENUM::__limit)
//		{
//			auto strings = Jd::SplitString (i_enums, ",");
//			
//			if (i_index < strings.size ())
//				output << strings [i_index];
//			else
//				output << "<error>";
//		}
//		else output << "<undefined>";
//	}
//	
//	d_jdSerialize (m_value);
//
//	protected:
//	union
//	{
//		ENUM	m_enum;				// to see human readable version in debugger
//		TYPE	m_value = 0;
//	};
//};
//
//

#define d_jdEnumStringify(VALUES...) #VALUES

#define d_jdEnum2Strings(LOWNAME, UPNAME, ...) inline \
	std::ostream & operator << (std::ostream & output, const UPNAME & i_enum) { \
	output << Jd::EnumToString (i_enum, d_jdEnumStringify (__VA_ARGS__)) << ":" << (size_t) i_enum; \
	return output; }


#define d_jdEnum2_(LOWNAME, UPNAME, ...) enum class UPNAME { __VA_ARGS__ }; typedef UPNAME LOWNAME; d_jdEnum2Strings (LOWNAME, UPNAME, __VA_ARGS__)

#define d_jdEnum2(NS, NSUP, NAME, ...) d_jdEnum2_(c_ ## NS ## NAME, E## NSUP ##NAME, null, __VA_ARGS__)

#define d_jdEnum_(NAME, ...) d_jdEnum2 (jd, Jd, NAME, __VA_ARGS__)

// template: #define d_scEnum(NAME, ...) d_jdEnum2 (sc, Sc, NAME, __VA_ARGS__)



/*

#define d_jdEnumStringify(VALUES...) #VALUES

#define d_jdEnum2Strings(LOWNAME, UPNAME, ...) inline \
	std::ostream & operator << (std::ostream & output, const UPNAME & i_enum) { \
	UPNAME::OutputValueString (output, i_enum, d_jdEnumStringify (__VA_ARGS__)); \
	return output; }

#define d_jdEnum2___(LOWNAME, ...) namespace LOWNAME { typedef enum { __VA_ARGS__, __limit } enum_t; }


// notes: (1) BASENAME just creates an empty enum (could be struct as well) to provide a name to pass to the serializer code.
//			e.g. "Enum::MyEnum"


#define d_jdEnum2_(TYPE, BASENAME, LOWNAME, UPNAME, ...) d_jdEnum2___(LOWNAME,__VA_ARGS__) namespace Enum { enum BASENAME{}; } using UPNAME = JdEnumT <TYPE, LOWNAME::enum_t, Enum::BASENAME>; d_jdEnum2Strings (LOWNAME, UPNAME, __VA_ARGS__)

#define d_jdEnum2(TYPE, NS, _NSUP_, NAME, ...) d_jdEnum2_(TYPE, _NSUP_##NAME, c_ ## NS ## NAME, E##_NSUP_##NAME, null, __VA_ARGS__)

// template: #define d_scEnum(NAME, ...) d_jdEnum2 (u8, sc, Sc, NAME, __VA_ARGS__)

#define d_jdEnum_(NAME, ...) d_jdEnum2 (u8, jd, Jd, NAME, __VA_ARGS__)
*/

#endif /* JdEnum_h */
