//
//  JdTypeId.hpp
//
//  Created by Steven Massey on 9/20/14.
//
//

#ifndef JdTypeId_h
#define JdTypeId_h

#include <type_traits>
#include <unordered_map>

#include "IEpigram.hpp"
#include "JdUtils.hpp"


#define type_def typedef typename
#define type_if typedef typename std::conditional


namespace c_jdTypeId
{
	/*
	 
	0 0 0 1 1 1 1 1 =	c_jdType::unknown
	 
	o o o o o o o o
	7 6 5 4 3 2 1 0
	| | | | | | | |
	 \ \ \_____________ unused: can be used for extended custom types.
	  \ \______________ isPointer
	   \_______________ isArray
	 
	 */
	 
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
//				string16			=	16,
//				string32			=	17,
	
				// unused			=	18,
	
				binary				=	19,
	
				epigram				=	20,
				dictionary			=	epigram,

				pod					=	21,
				object				=	22,
				versionedObject		=	23,
	
				signature			=	24,
	
				// unused			=	25,

				hash				=	26,
				uuid				=	27,
	
				function			=	28,
				method				=	function,
	
				any					=	29,

				none				=	30, // 'none' means unset. ('default' epigram item uses this key-type,  void/null means "set to null")
				auto_			 	= 	30,
	
				unknown				=	31, // 0x1f
	
				typeMask			=	0x1f | isPointer;		// 5 LS bits + isPointer		// FIX: hmm, there really shouldn't be a type mask now
}



struct TypeIdsMap
{
	static const TypeIdsMap & Get ()
	{
		static const TypeIdsMap m;
		return m;
	}
	
	TypeIdsMap ();
	
	char		chars		[32] = {};
	
	static constexpr
	u8			sizes		[32] =
	{
		[c_jdTypeId::f64]				= sizeof (f64),
		[c_jdTypeId::f32]				= sizeof (f32),
		[c_jdTypeId::boolean]			= sizeof (bool),
		[c_jdTypeId::i8]				= sizeof (i8),
		[c_jdTypeId::u8]				= sizeof (u8),
		[c_jdTypeId::i16]				= sizeof (i16),
		[c_jdTypeId::u16]				= sizeof (u16),
		[c_jdTypeId::i32]				= sizeof (i32),
		[c_jdTypeId::u32]				= sizeof (u32),
		[c_jdTypeId::i64]				= sizeof (i64),
		[c_jdTypeId::u64]				= sizeof (u64)
	};
	
	u8			ids			[128] = {};
	cstr_t		names		[128] = {};
	cstr_t		longNames	[128] = {};
	
	std::unordered_map <std::string, u8>		namesToIds;
	
	u8 pointerForbidden [11] = {
		c_jdTypeId::uuid, c_jdTypeId::signature, c_jdTypeId::epigram, c_jdTypeId::enumeration, c_jdTypeId::hash, c_jdTypeId::binary,
		c_jdTypeId::method, c_jdTypeId::uuid, c_jdTypeId::hash, c_jdTypeId::any, c_jdTypeId::none };
	
	void ThrowIf (bool i_test, cstr_t i_string)
	{
		if (i_test)
		{
#			if __cpp_exceptions
				throw (std::string (i_string));
#			else
				abort ();
#			endif
		}
	}
};

namespace jd
{
	template <typename T>
	struct is_cstring : public std::integral_constant
	<bool, std::is_same <char *, typename std::decay <T>::type>::value or std::is_same<const char *, typename std::decay< T >::type>::value> {};

	template <typename T> struct is_vector : std::false_type {};
	template <typename... Ts> struct is_vector <std::vector	<Ts...>> : std::true_type {};
}



namespace Jd
{
	char TypeIdToChar (u8 i_typeId);
	u8 TypeCharToId (char i_typeChar);
	
	bool IsPointer (u8 i_typeId);
	
	constexpr bool IsFloatingPointType (u8 i_typeId)		{ return (i_typeId == c_jdTypeId::f64 or i_typeId == c_jdTypeId::f32); }
	
	constexpr bool IsIntegerType (u8 i_typeId)				{ return (i_typeId >= c_jdTypeId::i8 and i_typeId <= c_jdTypeId::u64); }

	constexpr bool IsSignedIntegerType (u8 i_typeId)		{ return (i_typeId >= c_jdTypeId::i8 and i_typeId <= c_jdTypeId::i64); }

	constexpr bool IsUnsignedIntegerType (u8 i_typeId)		{ return (i_typeId >= c_jdTypeId::u8 and i_typeId <= c_jdTypeId::u64); }

	constexpr bool IsNumberType (u8 i_typeId)				{ return IsFloatingPointType (i_typeId) or IsIntegerType (i_typeId); }
	
	bool IsTypeSet (u8 i_typeId);

	constexpr size_t TypeIdToSize (u8 i_typeId)
	{
		return TypeIdsMap::sizes [i_typeId];
	}

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
#if __APPLE__
		template <> inline constexpr const u8 TypeId2T <long> ()				{ return c_jdTypeId::i64; }	// what?
		template <> inline constexpr const u8 TypeId2T <unsigned long> ()		{ return c_jdTypeId::u64; }
		template <> inline constexpr const u8 TypeId2T <int64_t>()				{ return c_jdTypeId::i64; }
		template <> inline constexpr const u8 TypeId2T <uint64_t>()				{ return c_jdTypeId::u64; }
#else
		template <> inline constexpr const u8 TypeId2T <long long> ()			{ return c_jdTypeId::i64; }	
		template <> inline constexpr const u8 TypeId2T <unsigned long long> ()	{ return c_jdTypeId::u64; }	
#endif
		template <> inline constexpr const u8 TypeId2T <uint32_t> ()			{ return c_jdTypeId::u32; }
		template <> inline constexpr const u8 TypeId2T <std::string> ()			{ return c_jdTypeId::string; }
		template <> inline constexpr const u8 TypeId2T <void> ()				{ return c_jdTypeId::voidNull; }
		

		template <typename T>
		struct EpigramTypeIdT
		{
//			friend const u8 TypeId <T> ();
//			friend char TypeIdChar <T> ();
			
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
			
			type_if <std::is_class <T>::value,					EpigramStructTypeId,			EpigramFundamentalTypeId>::type		typeIdA;
			
			type_if <std::is_pointer <T>::value,				EpigramPointerTypeId,			typeIdA>::type						typeIdX;
			type_if <std::is_pointer <T>::value and
					 std::is_class <Tnp>::value,				EpigramObjPointerTypeId,		typeIdX>::type						typeIdY;
			
			type_if <jd::is_cstring <T>::value,					EpigramStringTypeId,			typeIdY>::type						typeIdZ;
			
			type_if <std::is_base_of <IIEpigram, T>::value,		EpigramEpigramTypeId,			typeIdZ>::type						typeIdB;
			
			type_if <std::is_base_of <std::string, T>::value,	EpigramStringTypeId,			typeIdB>::type						typeIdE;

			type_if <std::is_enum <T>::value,					EpigramEnumTypeId,				typeIdE>::type						typeIdF;

			type_if <std::is_same <Jd::NoType, T>::value,		NoTypeId,						typeIdF>::type						typeIdG;

			type_if <std::is_same <std::string_view, T>::value,	EpigramStringTypeId,			typeIdG>::type						typeIdH;

			type_if <std::is_base_of <Typed, T>::value,			T,								typeIdH>::type						type;
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
					JdTypeId		(u8 i_typeId);
					JdTypeId		();
	
					operator u8		() const
	{
		return m_typeId;
	}
	
	u8				Id				() const;
	bool			operator !=		(const JdTypeId & i_rhs);
	
	template <typename T>
	void			Set				() { m_typeId = Jd::TypeId <T> (); }

	template <typename T>
	void			Set				(const T &) { m_typeId = Jd::TypeId <T> (); }
	
	template <typename T>
	bool			Is				() const { return Jd::TypeId <T> () == m_typeId; }
	
	bool			Is				(u8 i_typeId) const;
	bool 			IsArray			() const;
	bool			IsAny			() const;
	bool			IsFunction		() const;
	bool 			IsObject		() const;

	bool 			IsIntegerType			() const 	{ return Jd::IsIntegerType (m_typeId); }
	bool 			IsFloatingPointType		() const 	{ return Jd::IsFloatingPointType (m_typeId); }
	bool 			IsSignedIntegerType		() const 	{ return Jd::IsSignedIntegerType (m_typeId); }
	bool 			IsUnsignedIntegerType	() const 	{ return Jd::IsUnsignedIntegerType (m_typeId); }
	
	static bool		IsObject				(u8 i_typeId);

	std::string		GetTypeName				() const;
	std::string		GetLongTypeName			() const;
	char			GetTypeChar				() const;
	
	protected:
	
	u8				m_typeId		= c_jdTypeId::unknown;
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

			# if __cpp_exceptions
				default: throw ("finish implementation"); break;
			# endif
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


//#undef type_def
//#undef type_if


#endif /* defined(__Epilog__JdTypeId__) */
