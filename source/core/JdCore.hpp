//
//  JdCore.hpp
//  Jigidesign
//
//  Created by Steven Massey on 8/3/11.
//  Copyright 2011-2012 Epigram Software, LLC. All rights reserved.
//

#include "JdNucleus.hpp"

# if ! defined (_MSC_VER)
	# pragma GCC diagnostic ignored "-Wconversion"
#endif

#ifndef Jigidesign_JdCore_h
#define Jigidesign_JdCore_h

#	ifndef d_objC
#		if d_objCVersion
#			define d_objC_(OBJ,VERSION)		OBJ ## _v ## VERSION
#			define d_objC__(OBJ,VERSION)	d_objC_ (OBJ, VERSION)
#			define d_objC(OBJ)				d_objC__ (OBJ, d_objCVersion)
#		else
#			define d_objC(OBJ) OBJ
#		endif
#	endif

#include <iostream>
#include <sstream>
#include <limits>
#include <type_traits>

# ifndef d_epigramDisableMutex
#	include <mutex>
	typedef std::lock_guard <std::mutex> mutex_lock;
	typedef std::lock_guard <std::recursive_mutex> rmutex_lock;
# endif

#include "JdTypeTraits.hpp"

using namespace std;


namespace Jd
{
	u32		Pow2CeilLog2		(u32 i_value);
	u32		RoundUpToAPowerOf2 	(u32 i_value);
	u32		CeilLog2 			(u32 i_value);

	template <typename V>
	V AlignTo (u32 i_boundary, V i_value)
	{
		V aligner = i_boundary - 1;
		return (i_value + aligner) & ~aligner;
	}
	
	template <typename T>
	T Align64 (T i_value)
	{
		return (i_value + 7) & ~7;
	}
	
	inline u32 WordAlign (u32 i_word)
	{
		return (i_word + 3) & ~3;
	}
	
	inline size_t NativeAlign (size_t i_toAlign)
	{
#		if defined (__LP64__)
			return (i_toAlign + 7) & ~7;
#		else
			return (i_toAlign + 3) & ~3;
#		endif
	}
	
	template <typename T, u32 t_length>
	u32 SizeOfArray (const T (&) [t_length])
	{
		return t_length;
	}
	
	template <typename T, size_t t_length>
	std::vector <T>  ArrayToVector  (const T (& i_array) [t_length])
	{
		return { i_array, i_array + t_length };
	}

	struct OtherToString
	{
		template <typename T>
		static void Stringify (std::ostringstream & o_oss, const T & i_value)
		{
			o_oss << i_value;
		}
	};

	struct ContainerToString;

	template <typename T>
	std::string ToString (const T & i_value)
	{
		typedef typename std::conditional <jd::has_iterator <T>::value, ContainerToString, OtherToString>::type	 s;

		std::ostringstream oss;
		s::Stringify (oss, i_value);
		
		return oss.str ();
	}
	
	struct ContainerToString
	{
		template <typename T>
		static void Stringify (std::ostringstream & o_oss, const T & i_container)
		{
			size_t c = 0;
			for (auto & i : i_container)
			{
				if (c++) o_oss << ", ";
				o_oss << Jd::ToString (i);
			}
		}
	};
	
	const char c_epilogInsertToken = '@';
	
	struct SSPrintFHelper
	{
		template <typename T>
		static void Stream (std::ostringstream & o_oss, const T & i_value)
		{
			o_oss << i_value;
		}
		
		static void Stream (std::ostringstream & o_oss, const bool & i_value)
		{
			o_oss << (i_value ? "true" : "false");
		}
	};
	
	
	void SSPrintF (std::ostringstream & o_oss, cstr_t i_format);
	
	template <typename T, typename... Args>
	void SSPrintF (std::ostringstream & o_oss, cstr_t i_format, T i_value, Args... i_args)
	{
		while (* i_format)
		{
			if (* i_format == c_epilogInsertToken)
			{
				if (*(i_format + 1) == c_epilogInsertToken)
				{
					++i_format;
				}
				else
				{
					SSPrintFHelper::Stream (o_oss, i_value);
					SSPrintF (o_oss, i_format + 1, i_args ...); // call even when *s == 0 to detect extra arguments
					return;
				}
			}
			
			o_oss << *i_format++;
		}
		
		# if __cpp_exceptions
			throw std::logic_error ("extra arguments provided to SSPrintF");
		# endif
	}
	
	template <typename T, typename... Args>
	std::string SPrintF (cstr_t i_format, T i_value, Args... i_args)
	{
		std::ostringstream oss;
		SSPrintF (oss, i_format, i_value, i_args...);
		return oss.str ();
	}
	
	string SPrintF (cstr_t i_format);
}


namespace jd
{
	template <typename T, typename... Args>
	std::string sprintf (cstr_t i_format, T i_value, Args... i_args)
	{
		std::ostringstream oss;
		Jd::SSPrintF (oss, i_format, i_value, i_args...);
		return oss.str ();
	}

#	ifndef d_epigramDisableMutex
		template <u32 lockid_t>
		std::mutex & global_lock ()
		{
			static std::mutex mutex;
			return mutex;
		}

#		define d_jdGlobalLock(ID) 	mutex_lock lock (global_lock <ID> ());
#	else
#		define d_jdGlobalLock(ID)
#	endif

	inline void out (cstr_t i_format)
	{
		d_jdGlobalLock ('cout');
		cout << i_format << "\r" << endl;
	}

	template <typename T, typename... Args>
	void out (cstr_t i_format, T i_value, Args... i_args)
	{
		d_jdGlobalLock ('cout');
		cout << Jd::SPrintF (i_format, i_value, i_args...) << "\r" << endl;
	}

	template <typename T>
	void out (const T & i_value)
	{
		d_jdGlobalLock ('cout');
		cout << i_value << "\r" << endl;
	}

	
	template <typename A, typename B>
	auto min (const A & a, const B & b)
	{
		typedef decltype (a*b) fused;
		return ::min ((fused) a, (fused) b);
	}

	template <typename A, typename B>
	auto max (const A & a, const B & b)
	{
		typedef decltype (a*b) fused;
		return ::max ((fused) a, (fused) b);
	}
}

#define debug(OUTPUT) std::cout << OUTPUT << endl

#define d_jdBitConst(CAT,VAR,BIT) namespace c_jd##CAT { const u32 VAR = (1 << BIT); }

#define d_trueFalse(TRUE,FALSE) const bool TRUE = true; const bool FALSE = false;

#define d_jdTrueFalse(NAMESPACE,TRUE,FALSE) namespace c_jd##NAMESPACE { const bool TRUE = true, FALSE = false; }

#include "JdFlatString.hpp"

class JdFormatter
{
	public:
	JdFormatter (bool i_maxPrecision = true)
	{
		// FIX: kill this. was just using this for sqlite: which should switch to "bind" method input
		if (i_maxPrecision)
			oss.precision (numeric_limits <f64>::max_digits10);
	}

		template <typename T>
		JdFormatter& operator << (const T & i_value)
		{
			Reset();
			oss << i_value;
			return *this;
		}
		
		template <typename T>
		JdFormatter& operator , (const T & i_value)
		{
			oss << i_value;
			return *this;
		}

		template <typename T>
		JdFormatter& operator += (const T & i_value)
		{
			oss << i_value;
			return *this;
		}

/*
		template <typename T>
		JdFormatter& operator * (const T & i_value)
		{
//			cout << "plus: " << i_value << endl;
			oss << i_value;
			return *this;
		}
*/	
		cstr_t CString () const
		{
			formatted = oss.str();
			return formatted.c_str();
		}
	
		operator std::string () const
		{
			return oss.str();
		}
		
		operator cstr_t () const 
		{
			return CString();
		}
		
	/*
		const char* operator () () const
		{
			return CString();
		}
	*/	
		void Reset ()
		{
			oss.clear();
			oss.str("");
			formatted.clear();
		}
	
		size_t		GetLength () const
		{
			return oss.tellp();
		}
		
	public:
		mutable std::ostringstream	oss;
		mutable std::string			formatted;
};

std::ostream& operator << (std::ostream &output, const JdFormatter &i_string);



//
//namespace std
//{
//	template <>
//    struct hash <JdString64>
//    {
//        size_t operator () (const JdString64 & i_string) const
//        {
//			return CityHash64 (i_string, i_string.Length ());
//        }
//    };
//};


template <typename T>
struct JdEnum
{
	
	operator T & ()
	{
		return m_value;
	}
	
	operator T () const
	{
		return m_value;
	}

	/*
	template <typename C>
	const operator C () const
	{
		return static_cast <C> (m_value);
	}
	 */
	
	protected:
	T	m_value = 0;
};


#define d_baseEnum(TYPE, CATEGORY, ...) struct EJd##CATEGORY : public JdEnum <TYPE>, public JdSerialize::Unversioned <EJd##CATEGORY> { \
				enum { null = 0, __VA_ARGS__}; \
				EJd##CATEGORY () { } \
				d_jdSerialize(m_value); \
				template <typename C> EJd##CATEGORY (const C & i_value) { m_value = i_value; } \
				operator std::string () const { return Jd::ToString (*this); } \
				}; typedef EJd##CATEGORY c_jd##CATEGORY;


#define d_enumStrings(CATEGORY, ...) inline std::ostream & operator << (std::ostream & output, const EJd##CATEGORY & i_enum) { \
										u32 i = i_enum; \
										cstr_t c_names [] = { "null", __VA_ARGS__ }; \
										output << ((i < Jd::SizeOfArray (c_names)) ? c_names [i] : "undefined"); \
										return output; }

#define d_enumStr(E) #E

#define d_enum1Args(CAT)						enum needs a category base and at least one enum

#define d_enum2Args(TYPE,CAT)					enum needs a category base and at least one enum

#define d_enum3Args(TYPE,CAT,E0)				d_baseEnum		(TYPE, CAT, E0) \
												d_enumStrings	(CAT, d_enumStr(E0))

#define d_enum4Args(TYPE,CAT,E0,E1)				d_baseEnum		(TYPE, CAT, E0, E1) \
												d_enumStrings	(CAT, d_enumStr(E0), d_enumStr(E1))
	
#define d_enum5Args(TYPE,CAT,E0,E1,E2)			d_baseEnum		(TYPE, CAT, E0, E1, E2) \
												d_enumStrings	(CAT, d_enumStr(E0), d_enumStr(E1), d_enumStr(E2))

#define d_enum6Args(TYPE,CAT,E0,E1,E2,E3)		d_baseEnum		(TYPE, CAT, E0, E1, E2, E3) \
												d_enumStrings	(CAT, d_enumStr(E0), d_enumStr(E1), d_enumStr(E2), d_enumStr(E3))
	
#define d_enum7Args(TYPE,CAT,E0,E1,E2,E3,E4)	d_baseEnum		(TYPE, CAT, E0, E1, E2, E3, E4) \
												d_enumStrings	(CAT, d_enumStr(E0), d_enumStr(E1), d_enumStr(E2), d_enumStr(E3), d_enumStr(E4))

#define d_enum8Args(TYPE,CAT,E0,E1,E2,E3,E4,E5)	d_baseEnum		(TYPE, CAT, E0, E1, E2, E3, E4, E5) \
												d_enumStrings	(CAT, d_enumStr(E0), d_enumStr(E1), d_enumStr(E2), d_enumStr(E3), d_enumStr(E4), d_enumStr(E5))


	
#define d_enumMacroChooser(...)	d_get8thArg(__VA_ARGS__, d_enum8Args, d_enum7Args, d_enum6Args, d_enum5Args, d_enum4Args, d_enum3Args, d_enum2Args, d_enum1Args, )
#define d_jdEnumT(...)			d_enumMacroChooser(__VA_ARGS__)(__VA_ARGS__)

#define d_jdEnum(...)			d_jdEnumT (u32, __VA_ARGS__)


#define d_jdConst(TYPE,NAMESPACE,NAME_VALUE) namespace c_jd##NAMESPACE { const TYPE NAME_VALUE; }



// d_jdInterface (IMyInterface) helps create an interface (struct) named "IIMyInterface" and typedefs a ptr to that interface as "IMyInterface"
// This eliminates 10,000 superfluous asterisks in the code


struct IIJdInterface { virtual ~IIJdInterface () {} };

#define d_jdForwardInterface(IFACE) struct I##IFACE; typedef I##IFACE * IFACE;
#define d_jdInterface(IFACE) d_jdForwardInterface (IFACE) struct I##IFACE : virtual IIJdInterface


struct JdPreconditions
{
	static JdPreconditions precoditions;
	
	JdPreconditions ();
};

#define type_def typedef typename
#define type_if typedef typename std::conditional

# if DEBUG
#	define d_jdIfDebug(STUFF) STUFF
# else
#	define d_jdIfDebug(STUFF) {}
# endif

#endif



