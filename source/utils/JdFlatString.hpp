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
	JdFlatString				()
	{
		m_cstring[0] = 0;
	}

	JdFlatString					(const JdFlatString & i_string)
	{
		strncpy (m_cstring, i_string.m_cstring, t_length);
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

	JdFlatString					(const std::string &i_sstring)
	{
		strncpy (m_cstring, i_sstring.c_str(), t_length);
		m_cstring [t_length-1] = 0;
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


	JdFlatString& operator +=	(const std::string &i_string)
	{
		strncat (m_cstring, i_string.c_str(), t_length - strlen (m_cstring));
		m_cstring [t_length-1] = 0;
		return *this;
	}


	bool		operator <		(const JdFlatString &i_string)
	{
		return (strcmp (m_cstring, i_string.m_cstring) < 0);
	}
	
	bool		operator ==		(const JdFlatString &i_string)
	{
		return (strcmp (m_cstring, i_string.m_cstring) == 0);
	}

	bool		operator !=		(const JdFlatString &i_string)
	{
		return (strcmp (m_cstring, i_string.m_cstring) != 0);
	}


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

	
	const char *					CString					() const { return m_cstring; }
	char *							CString					() { return m_cstring; }
	
	size_t							Length					() const
	{	
		return strlen (m_cstring);
	}
	
	u32								Capacity				() const { return t_length - 1; }
	
	operator const char * () const
	{
		return m_cstring;
	}
	
	
	operator std::string () const
	{
		return m_cstring;
	}
		
		
	protected:
	char							m_cstring				[t_length];
};


template <int t_length>
std::ostream & operator << (std::ostream &output, const JdFlatString <t_length> & i_string)
{
	output << i_string.CString();
	return output;
}

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
