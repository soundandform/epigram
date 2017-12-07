//
//  JdTypeId.hpp
//
//  Created by Steven Massey on 9/20/14.
//
//

#ifndef JdTypeId_h
#define JdTypeId_h

#include <type_traits>
#include "IEpigram.hpp"
#include "JdUtils.hpp"

#include <unordered_map>

#define type_def typedef typename
#define type_if typedef typename std::conditional


namespace c_jdTypeId
{
	// note: bit 5 is unused.
	
	const u8	isPointer			=	1	<< 6,
				isArray				=	1	<< 7,
	
				voidNull			=	0,
				nullVoid			=	voidNull,
				null				=	voidNull,
	
				i8					=	1,
				i16					=	2,
				i32					=	3,
				i64					=	4,
	
				u8					=	5,
				u16					=	6,
				u32					=	7,
				u64					=	8,

//				i128				=	9,
//				u128				=	10,

				boolean				=	11,
				enumeration			=	12,

				f32					=	13,
				f64					=	14,

				string				=	15,
				string16			=	16,
				string32			=	17,
	
				binary				=	19,
	
				epigram				=	20,

				pod					=	21,
				object				=	22,
				versionedObject		=	23,
	
				signature			=	24,
	
				// 25
				// 26
	
				hash				=	27,
				uuid				=	28,
	
				function			=	29,
				method				=	function,
	
				any					=	30,
				none				=	31, // 'none' means unset. ('default' epigram item uses this key-type,  void/null means "set to null")
	
				unknown				=	0xff,
	
				typeMask			=	0x1f | isPointer;		// 5 LS bits + isPointer
}



struct TypeIdsMap
{
	static TypeIdsMap & Get ()
	{
		static TypeIdsMap m;
		return m;
	}
	
	TypeIdsMap ()
	{
	/*
		not placed: string16 string32
	
	
		A |                 a | any                  0  \void
		B | binary          b  \bool                 1  /----
		C  \i8              c  /----                 2 |
		D  /--              d | default/none         3 |
		E |	epigram			e | enum                 4 |
		F  \f64             f  \f32                  5 |
		G  /---             g  /---                  6 |
		H |                 h | hash-64              7 |
		I  \i64             i  \i32                  8  \u8
		J  /---             j  /---                  9  /--
		K  \u16             k  \i16
		L  /---             l  /---
		M |                 m | method/function
		N |                 n |
		O  \versioned       o  \object
		P  /object          p  /------
		Q |                 q |
		R |	                r |
		S  \struct          s  \string
		T  /pod---          t  /------
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
		
		chars [c_jdTypeId::enumeration]		= 'e';
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
		names ['e'] = "enum";  //	TODO: ptr to enum just not allowed. dumb.
		names ['B'] = "bin";  //	TODO: ptr to binary not allowed
//		names ['a'] = "#32";  //	TODO: ptr to binary not allowed
		names ['h'] = "#64";  //	TODO: ptr to binary not allowed
		names ['a'] = "any";  //	TODO: ptr to binary not allowed
		names ['d'] = "def";  //	TODO: ptr to binary not allowed
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
		
		longNames ['d'] = "default";  //	TODO: ptr to binary not allowed
		
		longNames ['E'] = "epigram";
		
		longNames ['b'] = "bool ";
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
				if (not names [c])
					throw (string ("no name for JdTypeId"));
				
				namesToIds [names [c]] = i;
				
				if (longNames [c])
					namesToIds [longNames [c]] = i;
				
				ids [c] = i;
				
//				std::cout << c << " -> " << i << endl;
				
				bool noPointer = false;
				for (u32 j = 0; j < 7; ++j)
				{
					if (pointerForbidden [j] == i)
					{
						noPointer = true;
						break;
					}
				}
				
				if (not noPointer)
					ids [c+1] = i | c_jdTypeId::isPointer;
			}
		}
		
	}
	
	char	chars		[32] = {};
	u8		sizes		[32] = {};
	u8		ids			[128] = {};
	cstr_t	names		[128] = {};
	cstr_t	longNames	[128] = {};
	
	unordered_map <string, u8>		namesToIds;
	
	u8 pointerForbidden [7] = {
		c_jdTypeId::uuid, c_jdTypeId::enumeration, c_jdTypeId::binary, c_jdTypeId::method, c_jdTypeId::uuid, c_jdTypeId::hash, c_jdTypeId::none };
			 
};

template <typename T>
struct is_cstring : public std::integral_constant
<bool, std::is_same <char *, typename std::decay <T>::type>::value or std::is_same<const char *, typename std::decay< T >::type>::value> {};


namespace Jd
{
	char TypeIdToChar (u8 i_typeId);
	u8 TypeCharToId (char i_typeChar);
	
	bool IsIntegerType (u8 i_typeId);
	bool IsFloatingPointType (u8 i_typeId);
	bool IsSignedIntegerType (u8 i_typeId);
	bool IsUnsignedIntegerType (u8 i_typeId);
	
	bool IsTypeSet (u8 i_typeId);

	size_t TypeIdToSize (u8 i_typeId);

	struct Typed {};
	struct NoType {};

	template <u32 t_type>
	struct TypedT : public Typed
	{
		static constexpr u8 Id () { return t_type; }
	};
	
	
	// forward:
	template <typename T> constexpr const u8 TypeId ();
	template <typename T> char TypeIdChar ();

	
	namespace Private
	{		
		using namespace std;
		
		
		template <typename T> constexpr const u8 TypeId2T ()					{ return c_jdTypeId::unknown; }
		
		template <> inline constexpr const u8 TypeId2T <bool> ()				{ return c_jdTypeId::boolean; }
		template <> inline constexpr const u8 TypeId2T <double> ()				{ return c_jdTypeId::f64; }
		template <> inline constexpr const u8 TypeId2T <float> ()				{ return c_jdTypeId::f32; }
		template <> inline constexpr const u8 TypeId2T <char> ()				{ return 's'; }	/// FIX
		template <> inline constexpr const u8 TypeId2T <const char *> ()		{ return c_jdTypeId::string; }
		template <> inline constexpr const u8 TypeId2T <int8_t> ()				{ return c_jdTypeId::i8; }
		template <> inline constexpr const u8 TypeId2T <uint8_t> ()				{ return c_jdTypeId::u8; }
		template <> inline constexpr const u8 TypeId2T <int16_t> ()				{ return c_jdTypeId::i16; }
		template <> inline constexpr const u8 TypeId2T <uint16_t> ()			{ return c_jdTypeId::u16; }
		template <> inline constexpr const u8 TypeId2T <int32_t> ()				{ return c_jdTypeId::i32; }
		template <> inline constexpr const u8 TypeId2T <long> ()				{ return c_jdTypeId::i64; }	// what?
		template <> inline constexpr const u8 TypeId2T <unsigned long> ()		{ return c_jdTypeId::u64; }
		template <> inline constexpr const u8 TypeId2T <uint32_t> ()			{ return c_jdTypeId::u32; }
		template <> inline constexpr const u8 TypeId2T <int64_t> ()				{ return c_jdTypeId::i64; }
		template <> inline constexpr const u8 TypeId2T <uint64_t> ()			{ return c_jdTypeId::u64; }
		template <> inline constexpr const u8 TypeId2T <std::string> ()			{ return c_jdTypeId::string; }
		template <> inline constexpr const u8 TypeId2T <void> ()				{ return c_jdTypeId::voidNull; }
		

		template <typename T>
		class EpigramTypeIdT
		{
			friend constexpr const u8 TypeId <T> ();
			friend char TypeIdChar <T> ();
			
			struct EpigramFundamentalTypeId
			{
				// TODO: should probably template this so that the id returned comes from the sizeof(T) for all ints. 
				
				static constexpr u8 Id ()
				{
					return TypeId2T <T> ();
				}
			};
			
			struct EpigramPointerTypeId
			{
				typedef typename std::remove_pointer <T>::type type;
				
				static constexpr u8 Id ()
				{
					return TypeId2T <type> () | c_jdTypeId::isPointer;
				}
			};

			struct EpigramObjPointerTypeId
			{
				static constexpr u8 Id () { return c_jdTypeId::object | c_jdTypeId::isPointer; }		// FIX: is this right? versioned/unversioned? Need Both?
			};

			struct EpigramEnumTypeId
			{
				static constexpr u8 Id () { return c_jdTypeId::enumeration; }
			};

			struct EpigramEpigramTypeId
			{
				static constexpr u8 Id () { return c_jdTypeId::epigram; }
			};

			struct EpigramStringTypeId
			{
				static constexpr u8 Id () { return c_jdTypeId::string; }
			};


			struct EpigramStructTypeId
			{
				static constexpr u8 Id () { return c_jdTypeId::pod; }
			};

			struct NoTypeId
			{
				static constexpr u8 Id () { return c_jdTypeId::none; }
			};


			type_def std::remove_pointer <T>::type		Tnp;
			
			type_if <std::is_class <T>::value,			EpigramStructTypeId,			EpigramFundamentalTypeId>::type		typeIdA;
			
			type_if <std::is_pointer <T>::value,		EpigramPointerTypeId,			typeIdA>::type						typeIdX;
			type_if <std::is_pointer <T>::value and
					 is_class <Tnp>::value,				EpigramObjPointerTypeId,		typeIdX>::type						typeIdY;
			
			type_if <is_cstring <T>::value,				EpigramStringTypeId,			typeIdY>::type						typeIdZ;
			
			type_if <is_base_of <IIEpigram, T>::value,	EpigramEpigramTypeId,			typeIdZ>::type						typeIdB;
			
			type_if <is_base_of <string, T>::value,		EpigramStringTypeId,			typeIdB>::type						typeIdE;

			type_if <is_enum <T>::value,				EpigramEnumTypeId,				typeIdE>::type						typeIdF;

			type_if <is_same <Jd::NoType, T>::value,	NoTypeId,						typeIdF>::type						typeIdG;

			type_if <is_base_of <Typed, T>::value,		T,								typeIdG>::type						type;
		};
	}

	// Public functions ------------------------------------------------------------------------------------------------------------------------------------------------
	
	// u8 type ids
	template <typename T>
	constexpr const u8 TypeId ()
	{
		type_def Private::EpigramTypeIdT <T>::type tid;
		
		return tid::Id ();
	}

	template <typename T>
	char TypeIdChar ()
	{
		type_def Private::EpigramTypeIdT <T>::type tid;
		
		return TypeIdToChar (tid::Id ());
	}

	template <typename T>
	u8 GetTypeId (const T &)
	{
		return TypeId <T> ();
	}

	template <typename T>
	char GetTypeIdChar (const T &)
	{
		return TypeIdToChar (TypeId <T> ());
	}

	
	// string type names
	cstr_t TypeIdToName (u8 i_typeId);
	cstr_t TypeIdToFullName (u8 i_typeId);
	
	template <typename T>
	cstr_t GetTypeName (const T &)
	{
		return TypeIdToName (GetTypeId <T> ());
	}
	
	
	template <typename T>
	cstr_t TypeName ()
	{
		return TypeIdToName (TypeId <T> ());
	}
	
	
	u8 TypeNameToId (stringRef_t i_typeName);
}

class JdTypeId
{
	public:
				JdTypeId		(u8 i_typeId) : m_typeId (i_typeId) { }
				JdTypeId		() {}
	
	template <typename T>
	void		Set				() { m_typeId = Jd::TypeId <T> (); }

	template <typename T>
	void		Set				(const T &) { m_typeId = Jd::TypeId <T> (); }
	
	template <typename T>
	bool		Is				() { return Jd::TypeId <T> () == m_typeId; }
	
				operator u8		() { return m_typeId; }
	cstr_t		GetTypeName		() { return Jd::TypeIdToName (m_typeId); }
	char		GetTypeChar		() { return Jd::TypeIdToChar (m_typeId); }
	
	
	protected:
	u8			m_typeId		= c_jdTypeId::unknown;
};



template <typename D>
class JdCasterT
{
	#if 0
	
		//usage:
		 JdCasterT </*to:*/ f64> cast (/*from: */ c_jdTypeId::i64);
	
		 i64 from = 1234;
		 auto to = cast (& from);

	#endif
	
	typedef D (* CastFunc)	(voidptr_t);

	template <typename S>
	static D Cast (voidptr_t i_valuePtr)
	{
		return * (reinterpret_cast <const S *> (i_valuePtr));
	}
	
	CastFunc			m_caster				= nullptr;

	public:

	JdCasterT (u8 i_srcTypeId)
	{
		switch (i_srcTypeId)
		{
			case c_jdTypeId::f64: m_caster = & Cast <f64>; break;
			case c_jdTypeId::i64: m_caster = & Cast <i64>; break;
			case c_jdTypeId::u64: m_caster = & Cast <u64>; break;

			default: throw ("finish implementation"); break;
		}
	}
	
	D operator () (voidptr_t i_valuePtr)
	{
		return (* m_caster) (i_valuePtr);
	}
};


struct JdCast
{
	template <typename F, typename T>
	T Cast () const
	{
		return * reinterpret_cast <const F *> (m_value);
	}
	
	
	JdCast (voidptr_t i_value, u8 i_typeId)
	:
	m_value (i_value),
	m_typeId (i_typeId) {}
	
	template <typename T>
	operator T () const
	{
		switch (m_typeId)
		{
			case c_jdTypeId::f64: return Cast <f64, T> ();
			case c_jdTypeId::i64: return Cast <i64, T> ();
			case c_jdTypeId::u64: return Cast <u64, T> ();
		}
		
		return T ();
	}
	
	voidptr_t			m_value;
	u8					m_typeId;
};


#undef type_def
#undef type_if


#endif /* defined(__Epilog__JdTypeId__) */
