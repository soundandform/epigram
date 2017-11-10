//
//  JdUUID.cpp
//  Jigidesign
//
//  Created by Steven Massey on 9/3/15.
//  Copyright (c) 2015 Jigidesign. All rights reserved.
//

#include "JdUUID.hpp"

// JdUUID ------------------------------------------------------------------------------------------------------------

JdUUID::Generator JdUUID::m_boost;

JdUUID::JdUUID (bool)
{
	m_boost.Generate (uuid);
}


JdUUID::JdUUID ()
{
	memset (&uuid, 0x0, sizeof (uuid));
}

JdUUID::JdUUID (stringRef_t i_stringUuid)
{
	boost::uuids::string_generator gen;
	uuid = gen (i_stringUuid.c_str());
}


JdUUID::JdUUID (cstr_t i_cStringUuid)
{
	boost::uuids::string_generator gen;
	uuid = gen (i_cStringUuid);
}


#include <boost/detail/endian.hpp>

JdUUID::JdUUID (u32 a, u16 b, u16 c, u16 d, u64 e)
{
	#ifdef BOOST_LITTLE_ENDIAN
		Jd::ByteSwap (a);
		Jd::ByteSwap (b);
		Jd::ByteSwap (c);
		Jd::ByteSwap (d);
		e <<= 16; Jd::ByteSwap (e);
	#endif
	
	memcpy (&uuid.data [0], &a, sizeof (u32));
	memcpy (&uuid.data [4], &b, sizeof (u16));
	memcpy (&uuid.data [6], &c, sizeof (u16));
	memcpy (&uuid.data [8], &d, sizeof (u16));
	memcpy (&uuid.data [10], &e, 6);
}


JdUUID::JdUUID (u64 i_msw, u64 i_lsw)
{
	memcpy (uuid.data, &i_msw, sizeof (u64));
	memcpy (&uuid.data[8], &i_lsw, sizeof (u64));
}


JdUUID::JdUUID (const u8 * i_bytes)
{
	memcpy (&uuid, i_bytes, uuid.size());
}


JdUUID::JdUUID (const JdUUID & i_uuid)
{
	memcpy (&uuid, &i_uuid.uuid, uuid.size());
}



JdUUID& JdUUID::operator = (const JdUUID & i_uuid)
{
	memcpy (&uuid, &i_uuid.uuid, uuid.size());
	return *this;
}


void JdUUID::Nullify ()
{
	memset (&uuid, 0x0, uuid.size());
}

bool JdUUID::IsNull () const
{
	return uuid.is_nil();
}


bool JdUUID::IsSet () const
{
	return ! uuid.is_nil();
}

JdUUID::operator bool () const
{
	return IsSet();
}

size_t JdUUID::Hash () const
{
	return (size_t) GetLow();
	//		return boost::uuids::hash_value (uuid);
}


std::string JdUUID::ToString () const
{
	return to_string (uuid);
}

u64 JdUUID::GetHigh () const
{
	u64 msw;
	memcpy (&msw, & uuid.data [0], sizeof (u64));
	return msw;
}

u64 JdUUID::GetLow () const
{
	u64 lsw;
	memcpy (&lsw, & uuid.data [8], sizeof (u64));
	return lsw;
}

void * JdUUID::GetBytes () const
{
	return (void *) &uuid.data[0];
}


bool operator == (JdUUID const& lhs, JdUUID const& rhs)
{
	return (lhs.uuid == rhs.uuid);
}

bool operator != (JdUUID const& lhs, JdUUID const& rhs)
{
	return (lhs.uuid != rhs.uuid);
}


bool operator == (JdNewUUID const& lhs, JdUUID const& rhs)
{
	return (lhs.uuid == rhs.uuid);
}


bool operator == (JdUUID const& lhs, JdNewUUID const& rhs)
{
	return (lhs.uuid == rhs.uuid);
}


//bool operator == (bool lhs, JdUUID const& rhs)
//{
//	return (lhs == rhs.IsSet());
//}
//
//bool operator == (JdUUID const& lhs, bool rhs)
//{
//	return (lhs.IsSet() == rhs);
//}
//

bool operator < (JdUUID const& lhs, JdUUID const& rhs)
{
    return lhs.uuid < rhs.uuid;
};


#if DEBUG
	#ifndef d_jdShowFriendlyUuidNames
		#define d_jdShowFriendlyUuidNames 1
	#endif
#endif


namespace Jd
{
	std::string JdEncodeBitsToString (const u8 * i_bits, i32 i_numBits, bool i_lowercase)
	{
		std::string o_string;
		
		int numBytes = (i_numBits + 7) >> 3;
		int b = 0;
		
		int bits;
		
		char offset = 0;
		if (i_lowercase) offset  = 'a' - 'A';
		
		while (i_numBits > 0)
		{
			bits = ((int) *i_bits) << 8;
			
			if (numBytes > 1)
			{
				//cout << "appending extra byte\n";
				bits |= ((int) *(i_bits+1));
			}
			//cout << "WORD: " << std::hex << bits << endl;
			
			int shift = 11 - b;
			//cout << "shift: " << std::dec << shift << endl;
			
			bits >>= shift;
			bits &= 0x0000001F;
			
			//cout << std::hex << bits << ":" << std::dec << bits << endl;
			
			char c;
			if (bits < 8)           c = bits - 0  + 'A' + offset;
			else if (bits < 13)     c = bits - 8  + 'J' + offset;
			else if (bits < 24)     c = bits - 13 + 'P' + offset;
			else                    c = bits - 24 + '2';
			
			o_string += c;
			
			b += 5;
			i_numBits -= 5;
			
			if (b >= 8)
			{
				i_bits++;
				numBytes--;
				b -= 8;
			}
		}
		
		return o_string;
	}

	
	string IdToFriendlyString (JdId i_uuid)
	{
		string s;
		
		if (i_uuid.IsNull())
		{
			s = "0000";
		}
		else
		{
			u32 truncated = (u32) i_uuid.Hash() & 0x000FFFFF;
			s = Jd::JdEncodeBitsToString ((u8 *) &truncated, 20, true);
		}
		
		return s;
	}
}

std::ostream& operator << (std::ostream &output, JdId i_uuid)
{
#undef d_jdShowFriendlyUuidNames
#if d_jdShowFriendlyUuidNames
	string s = Jd::IdToFriendlyString (i_uuid);
	output << "(" << s << ")";
#else
	output << i_uuid.uuid;
#endif
	return output;
}

