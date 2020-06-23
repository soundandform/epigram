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
	
	bool IsPointer (u8 i_typeId)
	{
		return (i_typeId & c_jdTypeId::isPointer);
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
		return (i_typeId != c_jdTypeId::unknown);
			
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




JdTypeId::JdTypeId		(u8 i_typeId) : m_typeId (i_typeId) { }
JdTypeId::JdTypeId		() {}
	
u8 JdTypeId::Id				() const { return m_typeId; }

bool JdTypeId::operator !=		(const JdTypeId & i_rhs)
{
	return m_typeId != i_rhs.m_typeId;
}

bool JdTypeId::Is				(u8 i_typeId) const
{
	return (m_typeId == i_typeId);
}

bool JdTypeId::IsArray			() const
{
	return (m_typeId & c_jdTypeId::isArray);
}

bool JdTypeId::IsAny			() const
{
	return (m_typeId == c_jdTypeId::any);
}

bool JdTypeId::IsFunction		() const
{
	return (m_typeId == c_jdTypeId::function);
}

bool JdTypeId::IsObject		() const
{
	return IsObject (m_typeId);
}

//static
bool  JdTypeId::IsObject		(u8 i_typeId)
{
	return (i_typeId == c_jdTypeId::versionedObject or i_typeId == c_jdTypeId::object);
}

std::string		 JdTypeId::GetTypeName		() const 	{ return Jd::TypeIdToName (m_typeId); }
std::string		 JdTypeId::GetLongTypeName	() const 	{ return Jd::TypeIdToFullName (m_typeId); }
char			 JdTypeId::GetTypeChar		() const	{ return Jd::TypeIdToChar (m_typeId); }
