//
//  JdUtils.hpp
//  Jigidesign
//
//  Created by Steven Massey on 9/3/15.
//  Copyright (c) 2015 Jigidesign. All rights reserved.
//

#ifndef __Jigidesign__JdUtils__
#define __Jigidesign__JdUtils__

#include "JdNucleus.hpp"

#include <string.h>
#include <vector>

namespace Jd
{
	template <typename T>
	void ByteSwap (T & io_value)
	{
		u8 * a = (u8 *) &io_value;
		u8 * b = (u8 *) &io_value + sizeof (T);
		
		for (u32 i = 0; i < (sizeof (T) >> 1); ++i)
			std::swap (*a++, *--b);
	}

  	template <typename T>
	cstr_t ParseObjectName (T*)
	{
		static cstr_t namePtr = nullptr;
		if (namePtr == nullptr)
		{
			const int c_maxNameLength = 127;
			static char storedName [c_maxNameLength + 1];	// name [0] stores length

			u8 & nameLength = * ((u8 *) storedName);
			char * name = & (storedName [1]);
			name [0] = 0;
			
			cstr_t end = 0;
			
#if __clang__
            cstr_t start = strstr (__PRETTY_FUNCTION__, "[T = ");
            if (start)
            {
                start += 5;
                end = strstr (start, "]");
            }
#elif __GNUC__
            const char *start = strstr (__PRETTY_FUNCTION__, "(");
            if (start)
            {
                ++start;
                end = strstr (start, " *");
            }
            
            if (! end)
            {
                start = strstr (__PRETTY_FUNCTION__, "with T = ");
                if (start)
                {
                    start += 9;
                    end = strstr (start, "]");
                }
            }
#elif _MSC_VER
			cstr_t start = strstr(__FUNCSIG__, "[T = ");
			if (start)
			{
				start += 5;
				end = strstr(start, "]");
			}
#endif
			
			if (end)
			{
				i32 length = (i32) (end - start);
				strncpy (name, start, std::min (length, c_maxNameLength));
				name [c_maxNameLength-1] = 0;
				nameLength = length;
			}
			
			namePtr = name;
		}
		
		return namePtr;
	}
	
	template <typename T>
	cstr_t ParseObjectName (T & i_object)
	{
		return ParseObjectName (& i_object);
	}
	
	template <typename T>
	cstr_t ParseClassName ()
	{
		T * o = nullptr;
		static cstr_t name = ParseObjectName (o);
		return name;
	}
	
	template <typename T>
	u32 Encode7bRE (u8 * o_bytes, T i_value)
	{
		const T mask = 0x7f;
		const T flag = 0x80;
		
		u32 byteCount = 1;
		
		T lsb = i_value & mask;
		i_value >>= 7;
		
		while (i_value)
		{
			lsb |= flag;
			
			*o_bytes++ = (u8) lsb;
			++byteCount;
			
			lsb = i_value & mask;
			i_value >>= 7;
		}
		
		*o_bytes++ = (u8) lsb;
		
		return byteCount;
	}


	
	template <typename T>
	T Decode7bRE (const u8 * & io_bytes, const u8 * i_end)
	{
		const T mask = 0x7f;
		const T flag = 0x80;
		
		u32 shift = 0;
		
		T bits = *io_bytes++;
		T value = bits & mask;
		
		while (bits & flag and io_bytes < i_end)
		{
			shift += 7;
			bits = *io_bytes++;
			T shifted = (bits & mask) << shift;
			value |= shifted;
		}
		
		return value;
	}

	template <typename T>
	T ReverseDecode7bRE (const u8 * & io_bytes, const u8 * i_end)
	// precondition: io_bytes >= i_end
	{
		const T mask = 0x7f;
		const T flag = 0x80;
		
		u32 shift = 0;
		
		T bits = *io_bytes--;
		T value = bits & mask;
		
		while (bits & flag and io_bytes > i_end)
		{
			shift += 7;
			bits = *io_bytes--;
			T shifted = (bits & mask) << shift;
			value |= shifted;
		}
		
		return value;
	}

	
	struct _7bRE
	{
		template <typename T>
		_7bRE (const T & i_value)
		{
			numBytes = Encode7bRE <T> (bytes, i_value);
		}
		
		std::vector <u8> Get ()
		{
			std::vector <u8> v;
			
			u32 i = 0;
			while (i < numBytes)
				v.push_back (bytes [i++]);
			
			return v;
		}
		
		void Copy (u8 * & i_dest)
		{
			u32 i = 0;
			while (i < numBytes)
			{
				*i_dest++ = bytes [i++];
			}
		}
		
		size_t CopyFlipped (u8 * & i_dest)
		{
			u32 i = numBytes;
			while (i--)
			{
				*i_dest++ = bytes [i];
			}
			
			return numBytes;
		}
		
		u32		numBytes;
		u8		bytes		[20];
	};
}


struct JdByteSize
{
	JdByteSize (size_t i_size)
	:
	m_size (i_size)
	{
	}
	
	operator size_t () const
	{
		return m_size;
	}
	
	protected:
	size_t m_size;
};

std::ostream& operator<< (std::ostream &output, const JdByteSize &i_size);


#endif /* defined(__Jigidesign__JdUtils__) */
