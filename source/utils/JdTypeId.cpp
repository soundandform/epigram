//
//  JdTypeId.cpp
//
//  Created by Steven Massey on 9/20/14.
//
//

#include "JdTypeId.hpp"



TypeIdsMap::TypeIdsMap ()
{
	/*
	 not placed: string16 string32
	 ---- are the pointer equivalents
	 
	 A |                 a | any                  0  \void
	 B | binary          b  \bool                 1  /----
	 C  \i8              c  /----                 2 |
	 D  /--              d  \ default/none/auto   3 |
	 E | epigram		 e  / -----------------   4 |
	 F  \f64             f  \f32                  5 |
	 G  /---             g  /---                  6 |
	 H |                 h | hash-64              7 |
	 I  \i64             i  \i32                  8  \u8
	 J  /---             j  /---                  9  /--
	 K  \u16             k  \i16
	 L  /---             l  /---
	 M |                 m | method/function
	 N | enum            n | signature
	 O  \versioned       o  \object
	 P  /object          p  /------
	 Q |                 q |
	 R |	             r |
	 S  \struct          s  \string
	 T  /pod             t  /------
	 U  \u64             u  \u32
	 V  /---             v  /---
	 W |                 w |
	 X |                 x | uuid
	 Y |                 y |
	 Z |                 z |
	 
	 */
	
	chars [c_jdTypeId::object]			= 'o';
	chars [c_jdTypeId::versionedObject] = 'O';
	
	chars [c_jdTypeId::pod]				= 'S';
	
	chars [c_jdTypeId::enumeration]		= 'N';
	chars [c_jdTypeId::signature]		= 'n';
	chars [c_jdTypeId::epigram]			= 'E';
	chars [c_jdTypeId::binary]			= 'B';
	chars [c_jdTypeId::uuid]			= 'x';
	chars [c_jdTypeId::none]			= 'd';
	chars [c_jdTypeId::any]				= 'a';
	
	
	chars [c_jdTypeId::boolean]			= 'b';
	chars [c_jdTypeId::f64]				= 'F';
	chars [c_jdTypeId::f32]				= 'f';
	
	chars [c_jdTypeId::string] =		's';
	
	chars [c_jdTypeId::i8] =			'C';
	chars [c_jdTypeId::u8] =			'8';
	
	chars [c_jdTypeId::i16] =			'k';
	chars [c_jdTypeId::u16] =			'K';
	
	chars [c_jdTypeId::i32] =			'i';
	chars [c_jdTypeId::u32] =			'u';
	
	chars [c_jdTypeId::i64] =			'I';
	chars [c_jdTypeId::u64] =			'U';
	
	chars [c_jdTypeId::hash] =			'h';
	
	chars [c_jdTypeId::function] =		'm';
	
	chars [c_jdTypeId::voidNull] =		'0';
	
	sizes [c_jdTypeId::f64]				= sizeof (f64);
	sizes [c_jdTypeId::f32]				= sizeof (f32);
	sizes [c_jdTypeId::boolean]			= sizeof (bool);
	sizes [c_jdTypeId::i8]				= sizeof (i8);
	sizes [c_jdTypeId::u8]				= sizeof (u8);
	sizes [c_jdTypeId::i16]				= sizeof (i16);
	sizes [c_jdTypeId::u16]				= sizeof (u16);
	sizes [c_jdTypeId::i32]				= sizeof (i32);
	sizes [c_jdTypeId::u32]				= sizeof (u32);
	sizes [c_jdTypeId::i64]				= sizeof (i64);
	sizes [c_jdTypeId::u64]				= sizeof (u64);
	
	names ['O'] = "vob";
	names ['P'] = "vob*";
	
	names ['o'] = "obj";
	names ['p'] = "obj*";
	
	names ['S'] = "pod";
	names ['T'] = "pod*";
	
	names ['x'] = "uuid";  //	TODO: ptr to enum just not allowed. dumb.
	names ['N'] = "enum";  //	TODO: ptr to enum just not allowed. dumb.
	names ['n'] = "sig";
	names ['B'] = "bin";  //	TODO: ptr to binary not allowed
	//		names ['a'] = "#32";
	names ['h'] = "#64";
	names ['a'] = "any";

	names ['d'] = "def";
	names ['e'] = "def*";

	names ['m'] = "fun";
	
	names ['E'] = "epg";
	
	names ['b'] = "bool";
	names ['c'] = "bol*";
	
	names ['F'] = "f64";
	names ['G'] = "f64*";
	
	names ['f'] = "f32";
	names ['g'] = "f32*";
	
	names ['s'] = "str";
	names ['t'] = "str*";
	
	names ['C'] = "i8";
	names ['D'] = "i8*";
	names ['8'] = "u8";
	names ['9'] = "u8*";
	
	names ['k'] = "i16";
	names ['l'] = "i16*";
	names ['K'] = "u16";
	names ['L'] = "u16*";
	
	names ['i'] = "i32";
	names ['j'] = "i32*";
	names ['u'] = "u32";
	names ['v'] = "u32*";
	
	names ['I'] = "i64";
	names ['J'] = "i64*";
	names ['U'] = "u64";
	names ['V'] = "u64*";
	
	names ['0'] = "void";
	names ['1'] = "ptr";	// void pointer
	
	//
	longNames ['B'] = "binary";
	
	longNames ['O'] = "v.object";
	longNames ['P'] = "v.object *";
	
	longNames ['o'] = "object";
	longNames ['p'] = "object *";
	
	//		longNames ['S'] = "pod";
	longNames ['T'] = "pod *";
	
	longNames ['d'] = "default";
	longNames ['e'] = "default *";
	
	longNames ['E'] = "epigram";
	
	longNames ['m'] = "function";
	longNames ['n'] = "signature";
	
	longNames ['b'] = "bool";
	longNames ['c'] = "bool *";
	
	longNames ['F'] = "f64";
	longNames ['G'] = "f64 *";
	
	longNames ['f'] = "f32";
	longNames ['g'] = "f32 *";
	
	longNames ['s'] = "string";
	longNames ['t'] = "cstr_t"; // this right?
	
	longNames ['C'] = "i8";
	longNames ['D'] = "i8 *";
	longNames ['8'] = "u8";
	longNames ['9'] = "u8 *";
	
	longNames ['k'] = "i16";
	longNames ['l'] = "i16 *";
	longNames ['K'] = "u16";
	longNames ['L'] = "u16 *";
	
	longNames ['i'] = "i32";
	longNames ['j'] = "i32 *";
	longNames ['u'] = "u32";
	longNames ['v'] = "u32 *";
	
	longNames ['I'] = "i64";
	longNames ['J'] = "i64 *";
	longNames ['U'] = "u64";
	longNames ['V'] = "u64 *";
	
	longNames ['1'] = "void *";	// void pointer
	
	for (u32 i = 0; i < 32; ++i)
	{
		char c = chars [i];
		if (c)
		{
			ThrowIf (not names [c], "no name for JdTypeId");
			
			namesToIds [names [c]] = i;
			
			if (longNames [c])
				namesToIds [longNames [c]] = i;

			ThrowIf (ids [c], "type char collision");
			ids [c] = i;

			bool noPointer = false;
			for (u8 pf : pointerForbidden)
			{
				if (i == pf)
				{
					noPointer = true;
					break;
				}
			}
			
			if (not noPointer)
			{
				ThrowIf (ids [c+1], "type char collision");
				ids [c+1] = i | c_jdTypeId::isPointer;
			}
		}
	}
}



namespace Jd
{
	bool HasPointerTypeChar (u8 i_typeChar)
	{
		const u8 c_typeChars [] = { 'C', 'F', 'I', 'K', 'U', 'b', 'd', 'f', 'i', 'k', 'o', 's', 'u', 'v', '0', '8' };
		
		for (u8 c : c_typeChars)
		{
			if (c == i_typeChar)
				return true;
		}
		
		return false;
	}

	char TypeIdToChar (u8 i_typeId)
	{
		u16 typeId = i_typeId;
		
		char c = '?';
		if (typeId == c_jdTypeId::unknown)
			return c;
		
		bool isPointer = i_typeId & c_jdTypeId::isPointer;

		typeId &= c_jdTypeId::typeMask;
		typeId &= ~c_jdTypeId::isPointer;
		
		if (typeId <= c_jdTypeId::none)
		{
			c = TypeIdsMap::Get().chars [typeId];
			
			if (c == 0)
				c = '?';
			else if (isPointer)
			{
				if (HasPointerTypeChar (c))
					++c;
				else
					abort (); //"unrepresentable type char"
			}
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
