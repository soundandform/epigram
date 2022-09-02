//
//  EpAttribute.cpp
//  Tape
//
//  Created by Steven Massey on 6/23/20.
//  Copyright © 2020 Massey Plugins Inc. All rights reserved.
//
#include <string_view>

#include "EpAttribute.hpp"

using namespace std;

u8 EpAttributes::GetTypeForName_ (const string & i_attributeName)
{
	return m_attributes [i_attributeName].type;
}

string EpAttributes::GetHashAttributeName_ (u64 i_hash)
{
	for (auto & i : m_attributes)
	{
		if (i.second.hash == i_hash)
			return i.first;
	}
	
	return "<unregistered>";
}

void EpAttributes::RegisterAttribute_ (cstr_t i_name, u8 i_typeId, stringRef_t)
{
	RegisterAttribute_ (i_name, i_typeId, (u64) 0);
}

void EpAttributes::RegisterAttribute_ (cstr_t i_name, u8 i_typeId, u64 i_hash)
{
	u8 type = m_attributes [i_name].type;
	
	if (type)
	{
		d_jdAssert (type == i_typeId, "attribute type collision");		// this should never happen, cause there'd be a class name collission first.
	}
	
	//		cout << "register: " << std::hex << i_hash << "  " << i_name << endl;
	
	m_attributes [i_name] = { i_hash, i_typeId };
}

void EpAttributes::Dump_ ()
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

//----------------------------------------------------------------------------------------------------------------------------------------

EpHash32::EpHash32 ()
{
}

//	EpHash32 (cstr_t i_name) : m_key (CityHash32 (i_name, strlen (i_name))) {}
EpHash32::EpHash32 (cstr_t i_name)
:
m_key ((u32) hash <string_view> () (string_view (i_name, strlen (i_name))))
{
}
	

EpHash64::EpHash64 (cstr_t i_name) : m_key (std::hash <std::string_view> () (std::string_view (i_name, strlen (i_name)))) {}
