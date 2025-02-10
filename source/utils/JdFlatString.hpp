//
//  JdFlatString.hpp
//  Jigidesign
//
//  Created by Steven Massey on 9/4/15.
//  Copyright (c) 2015 Jigidesign. All rights reserved.
//

#ifndef Jigidesign_JdFlatString_h
#define Jigidesign_JdFlatString_h

#if __OBJC__
	#include <CoreFoundation/CoreFoundation.h>
#endif

#include "JdNucleus.hpp"
#include <cstring>

template <int t_length>
class JdFlatString
{
	public:
	JdFlatString				()	{}

	JdFlatString					(const JdFlatString & i_string)
	{
		strncpy (m_cstring, i_string.m_cstring, t_length);
		m_cstring [t_length-1] = 0;
	}

	JdFlatString					(stringRef_t i_stdString)
	{
		strncpy (m_cstring, i_stdString.c_str (), t_length);
		m_cstring [t_length-1] = 0;
	}

	JdFlatString					(cstr_t i_cstring)
	{
		if (i_cstring)
		{
			strncpy (m_cstring, i_cstring, t_length);
			m_cstring [t_length-1] = 0;
		}
	}
	
//	#if __OBJC__
//	JdFlatString					(CFStringRef i_cfString)
//	{
//		CFStringGetCString (i_cfString, m_cstring, t_length, kCFStringEncodingMacRoman);
//	}
//	#endif

	
	JdFlatString					(cstr_t i_start, cstr_t i_end)
	{
		Set (i_start, i_end);
	}

	JdFlatString& operator =		(const JdFlatString & i_string)
	{
		strncpy (m_cstring, i_string.m_cstring, t_length);
		m_cstring [t_length-1] = 0;
		return *this;
	}
	
	JdFlatString& operator =		(const char * i_cstring)
	{
		if (i_cstring)
		{
			strncpy (m_cstring, i_cstring, t_length);
			m_cstring [t_length-1] = 0;
		}
		else m_cstring [0] = 0;
		
		return *this;
	}
	
	JdFlatString& operator =		(const std::string &i_stdString)
	{
		strncpy (m_cstring, i_stdString.c_str(), t_length);
		m_cstring [t_length-1] = 0;
		return *this;
	}
	
	JdFlatString& operator +=	(const char *i_cstring)
	{
		strncat (m_cstring, i_cstring, t_length - strlen (m_cstring));
		m_cstring [t_length-1] = 0;
		return *this;
	}


	JdFlatString& operator +=	(const std::string & i_string)
	{
		strncat (m_cstring, i_string.c_str(), t_length - strlen (m_cstring));
		m_cstring [t_length-1] = 0;
		return *this;
	}


	bool		operator <		(const JdFlatString & i_string)
	{
		return (strcmp (m_cstring, i_string.m_cstring) < 0);
	}
	
	bool		operator ==		(cstr_t i_cstring)					{ return (strcmp (m_cstring, i_cstring) == 0); }
	bool		operator !=		(cstr_t i_cstring)					{ return (strcmp (m_cstring, i_cstring) != 0); }
	
	bool		operator ==		(const JdFlatString & i_string)		{ return (strcmp (m_cstring, i_string.m_cstring) == 0); }
	bool		operator !=		(const JdFlatString & i_string)		{ return (strcmp (m_cstring, i_string.m_cstring) != 0); }

	void							Set						(const uint8_t * i_bytes, uint32_t i_numBytes)
	{
		uint32_t length = std::min (i_numBytes, (uint32_t) t_length-1);
		memcpy (m_cstring, i_bytes, length);
		m_cstring [length] = 0;
	}
	
	char *							Set						(cstr_t i_start, cstr_t i_end)
	{
		size_t length = i_end - i_start;
		if (i_start and i_end and length > 0)
		{
			strncpy (m_cstring, i_start, std::min (length, (size_t) t_length));
			m_cstring [t_length-1] = 0;
		}
		
		m_cstring [length] = 0;
		
		return m_cstring;
	}

	
	//	char *							CString					() { return m_cstring; }
	const char *					CString					() const 	{ return m_cstring; }
									operator const char * 	() const 	{ return m_cstring; }
									operator std::string 	() const 	{ return m_cstring; }
	
	size_t							Length					() const	{ return strlen (m_cstring); }
	size_t							Size					() const	{ return Length (); }
	
	u32								Capacity				() const 	{ return t_length - 1; }
	
		
	protected:
	char							m_cstring				[t_length]	= { 0 };			// always null terminated
};



template <int t_length>
std::ostream & operator << (std::ostream &output, const JdFlatString <t_length> & i_string)
{
	output << i_string.CString ();
	return output;
}

// --------------------------------------------------------------------------------------------------------
// needed for map & unordered_map key compatibility
template <int t_length>
bool operator < (const JdFlatString <t_length> & i_lhs, const JdFlatString <t_length> & i_rhs)
{
	return std::strcmp (i_lhs, i_rhs) < 0;
}

template <int t_length>
bool operator == (const JdFlatString <t_length> & i_lhs, const JdFlatString <t_length> & i_rhs)
{
	return std::strcmp (i_lhs, i_rhs) == 0;
}

namespace std
{
	template <int L>
	struct hash <JdFlatString <L>>
	{
		size_t operator () (const JdFlatString <L> & i_flatString) const
		{
			return  std::hash <std::string_view> () (i_flatString.CString ());
		}
	};
}
// --------------------------------------------------------------------------------------------------------


typedef JdFlatString <256>	JdString256;
typedef JdFlatString <128>	JdString128;
typedef JdFlatString <64>	JdString64;
typedef JdFlatString <32>	JdString32;


#if __OBJC__

#	import <Foundation/NSString.h>

	struct JdString256ObjC : public JdString256
	{
		JdString256ObjC (NSString * i_nsString);
	};

#endif


#endif
