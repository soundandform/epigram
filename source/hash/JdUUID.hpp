//
//  JdUUID.hpp
//  Jigidesign
//
//  Created by Steven Massey on 9/3/15.
//  Copyright (c) 2015 Jigidesign. All rights reserved.
//

#ifndef __Jigidesign__JdUUID__
#define __Jigidesign__JdUUID__

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "EpSerializationRaw.hpp"
#include "JdResult.hpp"

#if __APPLE__
	#include <objc/runtime.h>
	#include <CoreFoundation/CoreFoundation.h>
#endif


class JdUUID : public Jd::TypedT <c_jdTypeId::uuid>
{
	public:
//	static atomic <u32> s_numGenerated;
	
	boost::uuids::uuid uuid;
	
				JdUUID ();
				JdUUID (stringRef_t i_stringUuid);
				JdUUID (cstr_t i_cstringUuid);
				JdUUID (u32 a, u16 b, u16 c, u16 d, u64 e);
				JdUUID (u64 i_msw, u64 i_lsw);
	explicit	JdUUID (const u8 * i_bytes);
				JdUUID (const JdUUID & i_uuid);

    #if __APPLE__
				JdUUID (id);	// convert from a JdUid
    #endif
	
	protected:
	JdUUID (bool);	// DEPRECATED: Use JdNewUUID

	public:
	
	JdUUID & operator = (const JdUUID & i_uuid);
	
	static JdUUID Generate()
	{
		return JdUUID (true);
	}
	
	void			Nullify ();
	
	bool			IsNull () const;
	bool			IsSet () const;
	
	operator bool	() const;
	
	size_t			Hash () const;
	std::string		ToString () const;
	
	u64				GetHigh () const;
	u64				GetLow () const;
	
	void *			GetBytes () const;
	
	d_jdSerialize (uuid)

	protected:
	struct Generator
	{
		void Generate (boost::uuids::uuid & o_uuid)
		{
			lock_guard <mutex> lock (m_lock);
			o_uuid = m_uuid ();
		}
		
		mutex m_lock;
		boost::uuids::random_generator m_uuid;
	};
	
	static Generator m_boost;
	
	private:
		
	operator i32 ()	const { return 0; }	// the operator bool overload would normally satisfy these casts
	operator u32 ()	const { return 0; }	// so, avoid casting to a JdPort (u64), etc. (particually in a mismatched argument list) let's kill these.
	operator i64 ()	const { return 0; }
	operator u64 ()	const { return 0; }
};


class JdNewUUID : public JdUUID
{
	public:
	JdNewUUID (): JdUUID (true) { }
};

typedef const JdUUID&	JdId;

std::ostream& operator << (std::ostream &output, const JdUUID & i_uuid);
std::istream& operator >> (std::istream &input, JdUUID &o_uuid);

bool operator != (JdUUID const& lhs, JdUUID const& rhs);
bool operator == (JdUUID const& lhs, JdUUID const& rhs);

bool operator == (JdUUID const& lhs, JdNewUUID const& rhs);
bool operator == (JdNewUUID const& lhs, JdUUID const& rhs);

bool operator < (JdUUID const& lhs, JdUUID const& rhs);
	
const JdUUID c_jdNullId;

d_jdResultConst (Uuid, isExtant, "UUID is extant");


namespace std
{
	template <> struct hash <JdUUID>
	{
		size_t operator () (const JdUUID & i_uuid) const
		{
			return i_uuid.Hash();
		}
	};
};


namespace Jd
{
	string JdEncodeBitsToString (const u8 * i_bits, i32 i_numBits, bool i_lowercase);
	string IdToFriendlyString (JdId i_uuid);
}


#endif /* defined(__Jigidesign__JdUUID__) */
