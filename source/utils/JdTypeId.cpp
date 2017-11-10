//
//  JdTypeId.cpp
//
//  Created by Steven Massey on 9/20/14.
//
//

#include "JdTypeId.hpp"



namespace Jd
{
	char TypeIdToChar (u8 i_typeId)
	{
		char c = '?';
		if (i_typeId == c_jdTypeId::unknown)
			return c;
		
		u8 isPointer = i_typeId & c_jdTypeId::isPointer;

		i_typeId &= c_jdTypeId::typeMask;
		i_typeId &= ~c_jdTypeId::isPointer;
		
		if (i_typeId <= c_jdTypeId::none)
		{
			c = TypeIdsMap::Get().chars [i_typeId];
			
			if (c == 0)
				c = '?';
			else if (isPointer)
				++c;
		}
		
		return c;
	}
	
	u8 TypeCharToId (char i_typeChar)
	{
		u8 typeId = TypeIdsMap::Get().ids [i_typeChar];
		return typeId;
	}
	
	// TODO: just make these string
	cstr_t TypeIdToName (u8 i_typeId)
	{
		if (i_typeId == c_jdTypeId::unknown)
		{
			return "unknown";
		}
		else
		{
			char typeChar = TypeIdToChar (i_typeId);
			cstr_t name = TypeIdsMap::Get().names [typeChar];
			return name;
		}
	}
	
	
	cstr_t TypeIdToFullName (u8 i_typeId)
	{
		auto & m = TypeIdsMap::Get();
		
		char typeChar = TypeIdToChar (i_typeId);
		cstr_t name = m.longNames [typeChar];
		
		if (name == nullptr)
			name = m.names [typeChar];
		
		return name;
	}

	bool IsFloatingPointType (u8 i_typeId)
	{
		return (i_typeId == c_jdTypeId::f64 or i_typeId == c_jdTypeId::f32);
	}

	
	bool IsIntegerType (u8 i_typeId)
	{
		return (i_typeId >= c_jdTypeId::i8 and i_typeId <= c_jdTypeId::u64);
	}

	bool IsSignedIntegerType (u8 i_typeId)
	{
		return (i_typeId >= c_jdTypeId::i8 and i_typeId <= c_jdTypeId::i64);
	}

	bool IsUnsignedIntegerType (u8 i_typeId)
	{
		return (i_typeId >= c_jdTypeId::u8 and i_typeId <= c_jdTypeId::u64);
	}

	bool IsTypeSet (u8 i_typeId)
	{
		return (i_typeId < c_jdTypeId::unknown);
			
	}

	size_t TypeIdToSize (u8 i_typeId)
	{
		auto & map = TypeIdsMap::Get ();
		return map.sizes [i_typeId];
	}

	
	u8 TypeNameToId (stringRef_t i_typeName)
	{
		auto & map = TypeIdsMap::Get ();
		
		auto i = map.namesToIds. find (i_typeName);
		
		if (i != map.namesToIds.end ())
			return i->second;
		else
			return c_jdTypeId::unknown;
	}

}
