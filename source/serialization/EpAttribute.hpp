/*
 *  EpAttribute.hpp
 *  Jigidesign
 *
 *  Created by Steven Massey on 3/5/12.
 *  Copyright 2012 Jigidesign. All rights reserved.
 *
 */

#pragma once

#include "Epigram.hpp"
#include "JdAssert.hpp"
#include "JdTypeId.hpp"
#include "EpArgs.hpp"

#include <map>

class EpAttributes
{
	public:
	
	static u8 GetTypeForName (const string & i_attributeName)
	{
		return Static().GetTypeForName_ (i_attributeName);
	}

//	static string GetTypeStringForName (const string & i_attributeName)
//	{
//		return Static().GetTypeStringForName_ (i_attributeName);
//	}
//
	template <typename T, typename H>
	static void RegisterAttribute (cstr_t i_name, H i_hash)
	{
		Static ().RegisterAttribute_ (i_name, Jd::TypeId <T> (), i_hash);
	}

	static string GetHashAttributeName (u64 i_hash)
	{
		return Static ().GetHashAttributeName_ (i_hash);
	}
	
	static EpAttributes & Static ()
	{
		static EpAttributes attributes;
		return attributes;
	}
	
	static void Dump ()
	{
		Static ().Dump_ ();
	}
	
	protected://-------------------------------------------------------------------------
	u8 GetTypeForName_ (const string & i_attributeName)
	{
		return m_attributes [i_attributeName].type;
	}
	
	string GetHashAttributeName_ (u64 i_hash)
	{
		for (auto & i : m_attributes)
		{
			if (i.second.hash == i_hash)
				return i.first;
		}
		
		return "<unregistered>";
	}

	void RegisterAttribute_ (cstr_t i_name, u8 i_typeId, stringRef_t)
	{
		RegisterAttribute_ (i_name, i_typeId, (u64) 0);
	}
	
	void RegisterAttribute_ (cstr_t i_name, u8 i_typeId, u64 i_hash)
	{
		u8 type = m_attributes [i_name].type;
		
		if (type)
		{
			d_jdAssert (type == i_typeId, "attribute type collision");		// this should never happen, cause there'd be a class name collission first.
		}
		
//		cout << "register: " << std::hex << i_hash << "  " << i_name << endl;
		
		m_attributes [i_name] = { i_hash, i_typeId };
	}
	
	void Dump_ ()
	{
		for (auto i : m_attributes)
		{
			cout << i.second.type << ": ";
			cout << i.first;
			
			if (i.second.hash)
				cout << " (hash: 0x" << std::hex << i.second.hash << std::dec << ")";
			
			cout << endl;
		}
	}
	
	struct Info
	{
//		string	typeName;
		u64		hash;
		u8		type;
	};
	
	map <string, Info>		m_attributes;
};



class EpStringKey : public Jd::TypedT <c_jdTypeId::string>
{
	public:
	
	EpStringKey (cstr_t i_name) : m_key (i_name) {}
	EpStringKey () {}
	
//	EpHashString (const EpHashString &i_value)
//	:
//	m_key (i_value.m_key)
//	{
//	}
	
	operator cstr_t () const { return m_key; }
	
//	bool operator == (const EpHashString & i_hash)
//	{
//		return (strcmp (m_hash, i_hash.m_hash) == 0);
//	}

	cstr_t		m_key;
};


class EpHash32 : public Jd::TypedT <c_jdTypeId::hash>
{
	public:
	
	EpHash32 (cstr_t i_name) : m_key (CityHash32 (i_name, strlen (i_name))) {}
	EpHash32 () {}
//	EpHash32 (const EpHash32 & i_value) : m_key (i_value.m_key) {}
	
	operator u32 () { return m_key; }
	
	bool operator == (const EpHash32 & i_hash) { return m_key == i_hash.m_key; }
	
	u32 m_key;
};

class EpHash64 : public Jd::TypedT <c_jdTypeId::hash>
{
	public:
	
	EpHash64 (cstr_t i_name) : m_key (CityHash64 (i_name, strlen (i_name))) {}
	EpHash64 () {}
//	EpHash64 (const EpHash64 & i_value) : m_key (i_value.m_key) {}

	operator u64 () { return m_key; }
	
	// the epigram key search uses this for lookup matching:
	bool operator == (const EpHash64 & i_hash) { return m_key == i_hash.m_key; }
	
	u64 m_key;
};


// it's kinda amazing i got this to doubly function for both epigram insertion and key-lookup.
// this must remain a memberless class so that EpHashXX functions as an IntrinsicPodT inside the epigram
template <typename H, typename T>
class EpAttribute : public H
{
	public:
	
	EpAttribute (cstr_t i_name) : H (i_name)
	{
		EpAttributes::RegisterAttribute <T> (i_name, this->m_key);
	}
	
	EpAttribute () {}

	EpArg <H, T> operator = (const T & i_value) const
	{
		return EpArg <H, T> (*this, i_value);
	}

	EpArg <H, T> operator = (T && i_value) //const
	{
		return EpArg <H, T> (*this, forward <T> (i_value));
	}
};

template <typename T> using EpStringAttribute = EpAttribute <EpStringKey, T>;
template <typename T> using EpHash32Attribute = EpAttribute <EpHash32, T>;
template <typename T> using EpHash64Attribute = EpAttribute <EpHash64, T>;



#define d_attribute(TYPE, NAME)								static const EpStringAttribute <TYPE> a_##NAME (#NAME);
#define d_cattribute(TYPE, CATEGORY, NAME)					namespace a_##CATEGORY { static const EpStringAttribute <TYPE> NAME (#NAME); }
#define d_epAttribute(TYPE, CATEGORY, NAME)					namespace a_##CATEGORY { static const EpStringAttribute <TYPE> NAME (#NAME); }
#define d_epAttributeWithDefault(TYPE, CATEGORY, NAME, DEFAULT)		namespace a_##CATEGORY { static const EpStringAttribute <TYPE> NAME (#NAME, DEFAULT); }

#define d_jdAttribute(TYPE, CATEGORY, NAME)					namespace a_jd##CATEGORY { static const EpStringAttribute <TYPE> NAME (#NAME); }

#define d_epHashAttribute(TYPE, CATEGORY, NAME)				namespace a_##CATEGORY { static const EpHash32Attribute <TYPE> NAME (#CATEGORY ":" #NAME); }

#define d_attributeWithDefault(TYPE, NAME, DEFAULT)         static const EpStringAttribute <TYPE> a_##NAME (#NAME, DEFAULT);

#define d_attribute_enum(ENUM,NAME)     d_attribute (u32, NAME)

#define d_attributeWithDefault_i32(NAME, DEFAULT)   d_attributeWithDefault(i32, NAME, DEFAULT)
