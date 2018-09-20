/*	
 	Epilog tracing library

	Steven M Massey
	Epigram Software, LLC 
	(c) 2012


 	Usage:
 			epilog (LEVEL, "printf string: %s ...", object, ...)
	
			where LEVEL = realtime, tedious, detail, normal, warning, fatal OR special
 
 	Objective-C++:
 
			If logging from a C function or static member function use:				epilog_func (...)
 
 			The epilog macros rely on "this" or "self" to generate a "pretty" name of the class, requiring the special cases above.
 
 	Enabling/Disabling:
 			
 			Logging can be enabled & disabled.  This was needed for performace-oriented unit tests that were spewing massive amounts of logs.
 
 			epg_Enable ();
 			epg_Disable ();
 
	 // TODO
	 - switch to JdTypeId
	 - C++11 updgrade
 
 */

#ifndef Epilog_h

#ifndef d_disableEpilog
	#define d_disableEpilog 0
#endif

#include "JdFlatString.hpp"
#include "JdUtils.hpp"
#include "JdCore.hpp"

// some stupid header was defining a check macro
#ifdef check
#	undef check
#endif

//----------------------------------------------------------------------------------------------------------------------------------------------------
const uint8_t c_epilogClassification_realtime 	= 0x01;
// realtime events are completely disabled (macro'd into nothingness) in release builds

const uint8_t c_epilogClassification_tedious	= 0x01;
const uint8_t c_epilogClassification_detail 	= 0x02;
const uint8_t c_epilogClassification_normal 	= 0x07;

//----------------------------------------------------------------------------------------------------------------------------------------------------
const uint8_t c_epilogClassification_special 	= 0x80;
// a rare but interesting condition that we're designed to deal with. just taking notice for diagnostics.

//----------------------------------------------------------------------------------------------------------------------------------------------------
const uint8_t c_epilogClassification_warning 	= 0x0C;
const uint8_t c_epilogClassification_fatal 		= 0x0F;

#include "JdCoreL2.hpp"

#include <type_traits>
#define type_if typedef typename std::conditional
using namespace std;

const char c_epilogInsertToken = '@';

static void EpilogF (std::ostringstream & o_oss, cstr_t i_format)
{
	cstr_t format = i_format;
	
	while (* format)
	{
		if (* format == c_epilogInsertToken)
		{
			if (* (format + 1) == c_epilogInsertToken)
			{
				++format;
			}
			else
			{
				string msg = "invalid epilog: '";
				(msg += format) += "' is missing argument(s)";
				
				throw std::runtime_error (msg);
			}
		}
		
		o_oss << *format++;
	}
}

template <typename T, typename... Args>
void EpilogF (std::ostringstream & o_oss, cstr_t i_format, T value, Args... i_args)
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
				o_oss << value;
				EpilogF (o_oss, i_format + 1, i_args ...); // call even when *s == 0 to detect extra arguments
				return;
			}
		}
		
		o_oss << *i_format++;
	}
	
	throw std::logic_error ("extra arguments provided to EpilogF");
}

template <typename... t_args>
void EpilogPrint (char * o_string4k, cstr_t i_format, t_args... i_args)
{
	ostringstream oss;
	EpilogF (oss, i_format, i_args...);
	oss << std::ends;
	
	strncpy (o_string4k, oss.str().data(), 4095);
	o_string4k [4095] = 0;
}

inline
void EpilogPrint (char * o_string4k, cstr_t i_format)
{
	ostringstream oss;
	EpilogF (oss, i_format);
	oss << std::ends;
	
	strncpy (o_string4k, oss.str().data(), 4095);
	o_string4k [4095] = 0;
}



// This is a pointer to the printf or printf-like function that can generate the log string from the i_eventInfo arguments
typedef void (* EpEventFormatter) (char *o_string, void * i_eventInfo);

struct EpilogEvent
{
	EpEventFormatter	formatter;
	u32					size;
};


#ifdef d_epilogLibBuild
	#define d_epExtern __attribute__ ((visibility("default")))
    #define d_epImport
#else
#if _MSC_VER
#	define d_epExtern extern
#	define d_epImport 
#else
	#define d_epExtern extern
    #define d_epImport __attribute__((weak_import))
#endif

struct EpilogEmptyArg {};

template <typename T>
struct EpilogConvert
{
	template <typename Y>
	struct Fundamental
	{
		static Y & Convert (Y &i_value)
		{
			return i_value;
		}
		
		typedef Y cast_type;
	};
	
	template <typename Y>
	struct Object
	{
		static JdString256 Convert (Y &i_object)
		{
			return Jd::ToString (i_object);
		}

		typedef const char* cast_type;
	};
	
	struct EmptyArg
	{
		static void * Convert (EpilogEmptyArg &i_empty)
		{
			return 0;
		}
		
		typedef void * cast_type;
	};
	
	type_if <is_fundamental<T>::value,			Fundamental<T>, Object<T> >::type type_;
	type_if <is_pointer<T>::value,				Fundamental<T>, type_>::type type__;
	type_if <is_same <T,EpilogEmptyArg>::value, EmptyArg,		type__>::type type;
};

const i32 c_epilogStackSize = 3072;
const i32 c_epilogHeaderPad = 128;


template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
struct EpilogEvent6 : public EpilogEvent
{
	EpilogEvent6			(const char *i_format, const T1 &i_v1, const T2 &i_v2, const T3 &i_v3, const T4 &i_v4, const T5 &i_v5, const T6 &i_v6)
	:
	EpilogEvent				{ &FormatString, sizeof (*this) },
	m_printfFormat			(i_format),		/// FIX: this can just go into EpilogEvent
	m_v1					(i_v1),
	m_v2					(i_v2),
	m_v3					(i_v3),
	m_v4					(i_v4),
	m_v5					(i_v5),
	m_v6					(i_v6)
	{}
	
	static void FormatString (char *o_string, void * i_object)
	{
		auto object = (EpilogEvent6 <T1, T2, T3, T4, T5, T6> *) i_object;
		
		typedef typename EpilogConvert <T1>::type T1Converter;
		typedef typename EpilogConvert <T2>::type T2Converter;
		typedef typename EpilogConvert <T3>::type T3Converter;
		typedef typename EpilogConvert <T4>::type T4Converter;
		typedef typename EpilogConvert <T5>::type T5Converter;
		typedef typename EpilogConvert <T6>::type T6Converter;
		
		typedef typename T1Converter::cast_type T1Cast;
		typedef typename T2Converter::cast_type T2Cast;
		typedef typename T3Converter::cast_type T3Cast;
		typedef typename T4Converter::cast_type T4Cast;
		typedef typename T5Converter::cast_type T5Cast;
		typedef typename T5Converter::cast_type T6Cast;
		
		EpilogPrint (o_string, object->m_printfFormat,
					 (T1Cast) (T1Converter::Convert (object->m_v1)),
					 (T2Cast) (T2Converter::Convert (object->m_v2)),
					 (T3Cast) (T3Converter::Convert (object->m_v3)),
					 (T4Cast) (T4Converter::Convert (object->m_v4)),
					 (T5Cast) (T5Converter::Convert (object->m_v5)),
					 (T6Cast) (T6Converter::Convert (object->m_v6))
					 );
	}
	
	const char * 	m_printfFormat;
	T1				m_v1;
	T2				m_v2;
	T3				m_v3;
	T4				m_v4;
	T5				m_v5;
	T5				m_v6;
};




template <typename T1, typename T2, typename T3, typename T4, typename T5>
struct EpilogEvent5 : public EpilogEvent
{
	EpilogEvent5			(const char *i_format, const T1 &i_v1, const T2 &i_v2, const T3 &i_v3, const T4 &i_v4, const T5 &i_v5)
	:
	EpilogEvent				{ &FormatString, sizeof (*this) },
	m_printfFormat			(i_format),		/// FIX: this can just go into EpilogEvent
	m_v1					(i_v1),
	m_v2					(i_v2),
	m_v3					(i_v3),
	m_v4					(i_v4),
	m_v5					(i_v5)
	{
	}
	
	static void FormatString (char *o_string, void * i_object)
	{
		auto object = (EpilogEvent5 <T1, T2, T3, T4, T5> *) i_object;
		
		typedef typename EpilogConvert <T1>::type T1Converter;
		typedef typename EpilogConvert <T2>::type T2Converter;
		typedef typename EpilogConvert <T3>::type T3Converter;
		typedef typename EpilogConvert <T4>::type T4Converter;
		typedef typename EpilogConvert <T5>::type T5Converter;
		
		typedef typename T1Converter::cast_type T1Cast;
		typedef typename T2Converter::cast_type T2Cast;
		typedef typename T3Converter::cast_type T3Cast;
		typedef typename T4Converter::cast_type T4Cast;
		typedef typename T5Converter::cast_type T5Cast;
		
		EpilogPrint (o_string, object->m_printfFormat,
					 (T1Cast) (T1Converter::Convert (object->m_v1)),
					 (T2Cast) (T2Converter::Convert (object->m_v2)),
					 (T3Cast) (T3Converter::Convert (object->m_v3)),
					 (T4Cast) (T4Converter::Convert (object->m_v4)),
					 (T5Cast) (T5Converter::Convert (object->m_v5))
					 );
	}
	
	const char * 	m_printfFormat;
	T1				m_v1;
	T2				m_v2;
	T3				m_v3;
	T4				m_v4;
	T5				m_v5;
};

template <typename T1, typename T2, typename T3, typename T4>
struct EpilogEvent4 : public EpilogEvent
{
	EpilogEvent4			(const char *i_format, const T1 &i_v1, const T2 &i_v2, const T3 &i_v3, const T4 &i_v4)
	:
	EpilogEvent				{ &FormatString, sizeof (*this) },
	m_printfFormat			(i_format),
	m_v1					(i_v1),
	m_v2					(i_v2),
	m_v3					(i_v3),
	m_v4					(i_v4)
	{
	}

	static void FormatString (char *o_string, void * i_object)
	{
		EpilogEvent4 <T1, T2, T3, T4> * object = (EpilogEvent4 <T1, T2, T3, T4> *) i_object;

		typedef typename EpilogConvert <T1>::type T1Converter;
		typedef typename EpilogConvert <T2>::type T2Converter;
		typedef typename EpilogConvert <T3>::type T3Converter;
		typedef typename EpilogConvert <T4>::type T4Converter;

		typedef typename T1Converter::cast_type T1Cast;
		typedef typename T2Converter::cast_type T2Cast;
		typedef typename T3Converter::cast_type T3Cast;
		typedef typename T4Converter::cast_type T4Cast;

		EpilogPrint (o_string, object->m_printfFormat, 	(T1Cast) (T1Converter::Convert (object->m_v1)),
														(T2Cast) (T2Converter::Convert (object->m_v2)),
														(T3Cast) (T3Converter::Convert (object->m_v3)),
														(T4Cast) (T4Converter::Convert (object->m_v4)));
	}
	
	const char * 	m_printfFormat;
	T1				m_v1;
	T2				m_v2;
	T3				m_v3;
	T4				m_v4;
};

template <typename T1, typename T2, typename T3>
struct EpilogEvent3 : public EpilogEvent
{
	EpilogEvent3			(const char *i_format, const T1 &i_v1, const T2 &i_v2, const T3 &i_v3)
	:
	EpilogEvent				{ &FormatString, sizeof (*this) },
	m_printfFormat			(i_format),
	m_v1					(i_v1),
	m_v2					(i_v2),
	m_v3					(i_v3)
	{
	}
	
	static void FormatString (char *o_string, void * i_object)
	{
		EpilogEvent3 <T1, T2, T3> * object = (EpilogEvent3 <T1, T2, T3> *) i_object;
		
		typedef typename EpilogConvert <T1>::type T1Converter;
		typedef typename EpilogConvert <T2>::type T2Converter;
		typedef typename EpilogConvert <T3>::type T3Converter;
		
		typedef typename T1Converter::cast_type T1Cast;
		typedef typename T2Converter::cast_type T2Cast;
		typedef typename T3Converter::cast_type T3Cast;
		
		EpilogPrint (o_string, object->m_printfFormat, 	(T1Cast) (T1Converter::Convert (object->m_v1)),
					 (T2Cast) (T2Converter::Convert (object->m_v2)),
					 (T3Cast) (T3Converter::Convert (object->m_v3)));
	}
	
	const char * 	m_printfFormat;
	T1				m_v1;
	T2				m_v2;
	T3				m_v3;
};

template <typename T1, typename T2>
struct EpilogEvent2 : public EpilogEvent
{
	EpilogEvent2			(const char *i_format, const T1 &i_v1, const T2 &i_v2)
	:
	EpilogEvent				{ &FormatString, sizeof (*this) },
	m_printfFormat			(i_format),
	m_v1					(i_v1),
	m_v2					(i_v2)
	{
	}
	
	static void FormatString (char *o_string, void * i_object)
	{
		EpilogEvent2 <T1, T2> * object = (EpilogEvent2 <T1, T2> *) i_object;
		
		typedef typename EpilogConvert <T1>::type T1Converter;
		typedef typename EpilogConvert <T2>::type T2Converter;
		
		typedef typename T1Converter::cast_type T1Cast;
		typedef typename T2Converter::cast_type T2Cast;
		
		EpilogPrint (o_string, object->m_printfFormat,
					 (T1Cast) (T1Converter::Convert (object->m_v1)),
					 (T2Cast) (T2Converter::Convert (object->m_v2)));
	}
	
	const char * 	m_printfFormat;
	T1				m_v1;
	T2				m_v2;
};



template <typename T1>
struct EpilogEvent1 : public EpilogEvent
{
	EpilogEvent1			(const char *i_format, const T1 &i_v1)
	:
	EpilogEvent				{ &FormatString, sizeof (*this) },
	m_printfFormat			(i_format),
	m_v1					(i_v1)
	{
	}
	
	static void FormatString (char *o_string, void * i_object)
	{
		EpilogEvent1 <T1> * object = (EpilogEvent1 <T1> *) i_object;
		
		typedef typename EpilogConvert <T1>::type T1Converter;
		
		typedef typename T1Converter::cast_type T1Cast;

		EpilogPrint (o_string, object->m_printfFormat,
					 (T1Cast) (T1Converter::Convert (object->m_v1)));
	}
	
	const char * 	m_printfFormat;
	T1				m_v1;
};


struct EpilogEvent0 : public EpilogEvent
{
	EpilogEvent0			(const char *i_format)
	:
	EpilogEvent				{ &FormatString, sizeof (*this) },
	m_printfFormat			(i_format)
	{
	}
	
	static void FormatString (char *o_string, void * i_object)
	{
		EpilogEvent0 * object = (EpilogEvent0 *) i_object;
		
		EpilogPrint (o_string, object->m_printfFormat);
	}
	
	const char * 	m_printfFormat;
};



template <typename T1, typename T2, typename T3, typename T4>
struct EpilogEvent_ : public EpilogEvent
{
	EpilogEvent_				(const char *i_format, const T1 &i_v1, const T2 &i_v2, const T3 &i_v3, const T4 &i_v4)
	:
	m_printfFormat			(i_format),
	m_v1					(i_v1),
	m_v2					(i_v2),
	m_v3					(i_v3),
	m_v4					(i_v4)
	{
		formatter = &FormatString;
		size = sizeof (*this);
	}
	
	static void FormatString (char *o_string, void *i_object)
	{
		EpilogEvent_ <T1, T2, T3, T4> *object = (EpilogEvent_ <T1, T2, T3, T4> *) i_object;

		typedef typename EpilogConvert <T1>::type T1Converter;
		typedef typename EpilogConvert <T2>::type T2Converter;
		typedef typename EpilogConvert <T3>::type T3Converter;
		typedef typename EpilogConvert <T4>::type T4Converter;

		typedef typename T1Converter::cast_type T1Cast;
		typedef typename T2Converter::cast_type T2Cast;
		typedef typename T3Converter::cast_type T3Cast;
		typedef typename T4Converter::cast_type T4Cast;

		sprintf (o_string, object->m_printfFormat, 	(T1Cast) (T1Converter::Convert (object->m_v1)),
													(T2Cast) (T2Converter::Convert (object->m_v2)),
													(T3Cast) (T3Converter::Convert (object->m_v3)),
													(T4Cast) (T4Converter::Convert (object->m_v4)));
	}
	
	const char * 	m_printfFormat;
	T1				m_v1;
	T2				m_v2;
	T3				m_v3;
	T4				m_v4;
};

#if __OBJC__
	#import <Foundation/NSString.h>
	#define d_epilogCaster EpilogCasterObjC
#else
	#define d_epilogCaster EpilogCaster
#endif

template <typename T>
#if __OBJC__
struct EpilogCasterObjC
#else
struct EpilogCaster
#endif
{
	type_if <is_same <T, string>::value, JdString256, T>::type			A;
	type_if <is_same <T, cstr_t>::value, JdString256, A>::type			B;
	
	#if __OBJC__
		type_if <is_same <T, CFStringRef>::value, JdString256, B>::type		C;
		type_if <is_same <T, NSString *>::value, JdString256ObjC, C>::type		D;
		typedef D cast_t;
	#else
		typedef B cast_t;
	#endif
	
	static inline
	cast_t Cast (const T &i_value)
	{
//		cout << typeid (T).name () << " ---> " << typeid (cast_t).name () << endl;
		return i_value;
	}
	
};


inline
EpilogEvent * CreateEpilogEvent_ (char * i_memory, const char *i_format)
{
	EpilogEvent * event = new (i_memory + c_epilogHeaderPad) EpilogEvent_ <EpilogEmptyArg, EpilogEmptyArg, EpilogEmptyArg, EpilogEmptyArg>
	(i_format, EpilogEmptyArg (), EpilogEmptyArg (), EpilogEmptyArg (), EpilogEmptyArg ());
	
	return event;
}

template <typename T1>
EpilogEvent * CreateEpilogEvent_ (char * i_memory, const char *i_format, const T1 &i_v1)
{
	typedef d_epilogCaster <T1> C1;
	typedef typename C1::cast_t TC1;
	
	EpilogEvent * event = new (i_memory + c_epilogHeaderPad) EpilogEvent_ <TC1, EpilogEmptyArg, EpilogEmptyArg, EpilogEmptyArg>
	(i_format, C1::Cast (i_v1), EpilogEmptyArg (), EpilogEmptyArg (), EpilogEmptyArg ());
	
	return event;
}

template <typename T1, typename T2>
EpilogEvent * CreateEpilogEvent_ (char * i_memory, const char *i_format, const T1 &i_v1, const T2 &i_v2)
{
	typedef d_epilogCaster <T1> C1; typedef typename C1::cast_t TC1;
	typedef d_epilogCaster <T2> C2; typedef typename C2::cast_t TC2;
	
	EpilogEvent * event = new (i_memory + c_epilogHeaderPad) EpilogEvent_ <TC1, TC2, EpilogEmptyArg, EpilogEmptyArg>
	(i_format, C1::Cast (i_v1), C2::Cast (i_v2), EpilogEmptyArg (), EpilogEmptyArg ());
	
	return event;
}

template <typename T1, typename T2, typename T3>
EpilogEvent * CreateEpilogEvent_ (char * i_memory, const char *i_format, const T1 &i_v1, const T2 &i_v2, const T3 &i_v3)
{
	typedef d_epilogCaster <T1> C1; typedef typename C1::cast_t TC1;
	typedef d_epilogCaster <T2> C2; typedef typename C2::cast_t TC2;
	typedef d_epilogCaster <T3> C3; typedef typename C3::cast_t TC3;
	
	EpilogEvent * event = new (i_memory + c_epilogHeaderPad) EpilogEvent_ <TC1, TC2, TC3, EpilogEmptyArg>
	(i_format, C1::Cast (i_v1), C2::Cast (i_v2), C3::Cast (i_v3), EpilogEmptyArg ());
	
	return event;
}

template <typename T1, typename T2, typename T3, typename T4>
EpilogEvent * CreateEpilogEvent_ (char * i_memory, const char *i_format, const T1 &i_v1, const T2 &i_v2, const T3 &i_v3, const T4 &i_v4)
{
	typedef d_epilogCaster <T1> C1; typedef typename C1::cast_t TC1;
	typedef d_epilogCaster <T2> C2; typedef typename C2::cast_t TC2;
	typedef d_epilogCaster <T3> C3; typedef typename C3::cast_t TC3;
	typedef d_epilogCaster <T4> C4; typedef typename C4::cast_t TC4;
	
	EpilogEvent * event = new (i_memory + c_epilogHeaderPad) EpilogEvent_ <TC1, TC2, TC3, TC4>
	(i_format, C1::Cast (i_v1), C2::Cast (i_v2), C3::Cast (i_v3), C4::Cast (i_v4));
	
	return event;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------
inline
EpilogEvent *
CreateEpilogEvent (char * i_memory, const char *i_format)
{
	EpilogEvent * event = new (i_memory + c_epilogHeaderPad) EpilogEvent0 (i_format);
	
	return event;
}

template <typename T1>
EpilogEvent *
CreateEpilogEvent (char * i_memory, const char *i_format, const T1 &i_v1)
{
	typedef d_epilogCaster <T1> C1;
	typedef typename C1::cast_t TC1;

	EpilogEvent * event = new (i_memory + c_epilogHeaderPad) EpilogEvent1 <TC1>
							(i_format, C1::Cast (i_v1));
	
	return event;
}

template <typename T1, typename T2>
EpilogEvent * CreateEpilogEvent (char * i_memory, const char *i_format, const T1 &i_v1, const T2 &i_v2)
{
	typedef d_epilogCaster <T1> C1; typedef typename C1::cast_t TC1;
	typedef d_epilogCaster <T2> C2; typedef typename C2::cast_t TC2;

	EpilogEvent * event = new (i_memory + c_epilogHeaderPad) EpilogEvent2 <TC1, TC2>
	(i_format, C1::Cast (i_v1), C2::Cast (i_v2));
	
	return event;
}

template <typename T1, typename T2, typename T3>
EpilogEvent * CreateEpilogEvent (char * i_memory, const char *i_format, const T1 &i_v1, const T2 &i_v2, const T3 &i_v3)
{
	typedef d_epilogCaster <T1> C1; typedef typename C1::cast_t TC1;
	typedef d_epilogCaster <T2> C2; typedef typename C2::cast_t TC2;
	typedef d_epilogCaster <T3> C3; typedef typename C3::cast_t TC3;
	
	EpilogEvent * event = new (i_memory + c_epilogHeaderPad) EpilogEvent3 <TC1, TC2, TC3>
	(i_format, C1::Cast (i_v1), C2::Cast (i_v2), C3::Cast (i_v3));
	
	return event;
}

template <typename T1, typename T2, typename T3, typename T4>
EpilogEvent * CreateEpilogEvent (char * i_memory, const char *i_format, const T1 &i_v1, const T2 &i_v2, const T3 &i_v3, const T4 &i_v4)
{
	typedef d_epilogCaster <T1> C1; typedef typename C1::cast_t TC1;
	typedef d_epilogCaster <T2> C2; typedef typename C2::cast_t TC2;
	typedef d_epilogCaster <T3> C3; typedef typename C3::cast_t TC3;
	typedef d_epilogCaster <T4> C4; typedef typename C4::cast_t TC4;
	
	EpilogEvent * event = new (i_memory + c_epilogHeaderPad) EpilogEvent4 <TC1, TC2, TC3, TC4>
	(i_format, C1::Cast (i_v1), C2::Cast (i_v2), C3::Cast (i_v3), C4::Cast (i_v4));
	
	return event;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
EpilogEvent * CreateEpilogEvent (char * i_memory, const char *i_format, const T1 &i_v1, const T2 &i_v2, const T3 &i_v3, const T4 &i_v4, const T5 &i_v5)
{
	typedef d_epilogCaster <T1> C1; typedef typename C1::cast_t TC1;
	typedef d_epilogCaster <T2> C2; typedef typename C2::cast_t TC2;
	typedef d_epilogCaster <T3> C3; typedef typename C3::cast_t TC3;
	typedef d_epilogCaster <T4> C4; typedef typename C4::cast_t TC4;
	typedef d_epilogCaster <T5> C5; typedef typename C5::cast_t TC5;
	
	EpilogEvent * event = new (i_memory + c_epilogHeaderPad) EpilogEvent5 <TC1, TC2, TC3, TC4, TC5>
	(i_format, C1::Cast (i_v1), C2::Cast (i_v2), C3::Cast (i_v3), C4::Cast (i_v4), C5::Cast (i_v5));
	
	return event;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
EpilogEvent * CreateEpilogEvent (char * i_memory, const char *i_format, const T1 &i_v1, const T2 &i_v2, const T3 &i_v3, const T4 &i_v4, const T5 &i_v5, const T6 &i_v6)
{
	typedef d_epilogCaster <T1> C1; typedef typename C1::cast_t TC1;
	typedef d_epilogCaster <T2> C2; typedef typename C2::cast_t TC2;
	typedef d_epilogCaster <T3> C3; typedef typename C3::cast_t TC3;
	typedef d_epilogCaster <T4> C4; typedef typename C4::cast_t TC4;
	typedef d_epilogCaster <T5> C5; typedef typename C5::cast_t TC5;
	typedef d_epilogCaster <T6> C6; typedef typename C6::cast_t TC6;
	
	EpilogEvent * event = new (i_memory + c_epilogHeaderPad) EpilogEvent6 <TC1, TC2, TC3, TC4, TC5, TC6>
	(i_format, C1::Cast (i_v1), C2::Cast (i_v2), C3::Cast (i_v3), C4::Cast (i_v4), C5::Cast (i_v5), C6::Cast (i_v6));

	return event;
}



//	#define M_EPEXPORT
#endif

#include <inttypes.h>


typedef void (* EpilogHandler) (const char * const i_logString);


#ifdef __cplusplus
extern "C" {
#endif

	d_epExtern uint32_t		epg_Init				(const char *			i_serviceNameOrIpAddress, uint16_t i_portNumber, bool i_logToFile, EpilogHandler i_handler) d_epImport;
	d_epExtern void			epg_Quit				() d_epImport;
	
//	d_epExtern void			epg_SetHandler			(EpilogHandler *		i_handler);
	
	d_epExtern void			epg_Disable				() d_epImport;
	d_epExtern void			epg_Enable				() d_epImport;
	
	d_epExtern void			epg_DefineCategory		(const char *			i_className,
													 const char *			i_category);
	
	d_epExtern uint32_t		epg_RegisterLocation	(void *					i_location,
													 uint32_t				i_sessionId, 
													 const char *			i_functionName,
													 uint32_t				i_lineNum,
													 const char *			i_name);	// can be NULL

	d_epExtern uint32_t		epg_RegisterVariable	(void *					i_location,
													 uint32_t				i_sessionId, 
													 const char *			i_functionName, 
													 uint32_t				i_lineNum, 
													 const char *			i_variableName,
													 uint8_t				i_variableType);

	d_epExtern void			epg_Log					(uint32_t				i_locationId,	const char *i_cstring);
	
	d_epExtern void			epg_LogDeferred			(uint8_t				i_importance,
													 const char *			i_className,
													 EpilogEvent *			i_event) d_epImport;

//	d_epExtern void			epg_Table				(const char *			i_className,
//													 void *					i_object,
//													 JdId					i_rowId,
//													 const char *			i_columnName,
//													 EpilogEvent *			i_event);

	
	d_epExtern void			epg_RegisterEventType	(uint32_t				i_eventId,
													EpEventFormatter		i_eventFormatter);
	
	d_epExtern void			epg_LogEvent			(uint32_t				i_eventType,
													 const void *			i_eventInfo,
													 uint32_t				i_eventInfoSize);
	
	d_epExtern void			epg_LogVariable			(uint32_t				i_locationId,
													 const void *			i_value,
													 uint8_t				i_size);
	
	d_epExtern void			epg_RegisterTable		(void *					i_tableId,
													 const char *			i_name);

	d_epExtern void			epg_DumpTable			(void *					i_tableId);

	d_epExtern void			epg_DefineTableColumn	(void *					i_tableId,
													 uint32_t				i_columnIndex,
													 const char *			i_name);
													 
	d_epExtern void			epg_SetTableElement		(void *					i_tableId,
													 uint64_t				i_rowId [2],
													 uint32_t				i_columnIndex,
													 const char *			i_contents);

	d_epExtern void			epg_DeleteTableRow		(void *					i_tableId,
													 uint64_t				i_rowId [2]);

	d_epExtern void			epg_NewPanel			(const char *			i_panel, ...);

	d_epExtern void			epg_LogToPanel			(const char *			i_panel,
													 const uint8_t *		i_uuid,
													 const char *			i_info);


#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
#include <iostream>
#include <sstream>


namespace Epilog
{

class Table
{
/*
	protected: class Row;
	public:
				Table						(const char *i_name, const uint8_t i_shortcut)
				:
				m_columnIndex				(0)
		{
			epg_RegisterTable (this, i_name, i_shortcut);
		}
		
		Table &					operator () (const char *i_columnName, int i_width)
		{
			if (m_columnIndex != -1)
				epg_DefineTableColumn (m_columnIndex++, i_columnName, i_width);
			return *this;
		}
		
		template <typename T>
		Row &					operator () (const T &i_rowId)
		{
			m_columnIndex = -1;

			std::ostringstream oss; oss << i_rowId;
			row.SetId (oss.str());
			return row;
		}

	protected:
		class Row
		{
			friend class Table;
			public:
				template <typename T>
				Row &			operator () (const T &i_value)
				{
					std::ostringstream oss; oss << i_value;
					epg_SetTableElement (m_rowId.c_str(), m_columnIndex++, oss.str().c_str());
				
					return *this;
				}
				
				
			protected:
				void			SetId (const std::string &i_rowId)
				{ 
					m_rowId = i_rowId;
					m_columnIndex = 0;
				}
				
				std::string		m_rowId;
				uint32_t		m_columnIndex;
		};
		

		Row						row;
		
		int32_t					m_columnIndex;
 */
};


#define epTable(NAME, CHARCODE) static Epilog::Table NAME (#NAME, CHARCODE); NAME

#define epilog_table(ROWID) Epilog::Table (Jd::ParseObjectName (this))
			
inline uint32_t SessionId (uint32_t i_sessionId = 0)
{
	static uint32_t s_sessionId = 0;
	if (s_sessionId == 0) s_sessionId = i_sessionId;
	return s_sessionId;
}

	
class Formatter : public std::ostringstream
{
	public:
		template <typename T>
		Formatter& operator , (const T &i_value)
		{
			*this << i_value;
			return *this;
		}
};

			
class PanelFormatter : public std::ostringstream
{
	public:
	template <typename T>
	PanelFormatter& operator , (const T &i_value)
	{
		*this << i_value;
		*this << "|";
		return *this;
	}

	operator const char * ()
	{
		return str().c_str();
	}
};
	

#if !d_disableEpilog
class Session
{
	public:
	Session () { }
	
	static Session & Get (bool i_acquire = true)
	{
		static Epilog::Session session;
		
		if (! i_acquire)
			session.~Session ();

		return session;
	}
	
	Session (const char * i_serviceNameOrIpAddress, uint16_t i_portNumber, bool i_logToFile)
	{
		if (epg_Init) SessionId (epg_Init (i_serviceNameOrIpAddress, i_portNumber, i_logToFile, nullptr));
	}

	void LogToFile ()
	{
		if (epg_Init) SessionId (epg_Init (nullptr, 0, true, nullptr));
	}
	
	void ConnectTo (const char *i_serviceNameOrIpAddress, uint16_t i_portNumber, EpilogHandler i_handler)
	{
		if (epg_Init) SessionId (epg_Init (i_serviceNameOrIpAddress, i_portNumber, false, i_handler));
	}
		
	~Session ()
	{
		if (epg_Quit) epg_Quit();
	}
	
//	static void SetLogHandler (EpilogHandler * i_handler)
//	{
//		Session::Get().SetHandler (i_handler);
//	}

	static void Start (int argc, char * argv [], EpilogHandler i_handler = nullptr)
	{
		bool ipAddressFound = false;
		for (int i = 1; i < argc; ++i)
		{
			string arg = argv [i];
			
			if (arg.substr (0,9) == "--epilog=")
			{
				string ip = arg.substr (9);
				
				size_t pos = ip.find(":");
				
				string port = "9999";
				if (pos != string::npos)
				{
					port = ip.substr (pos+1);
					ip = ip.substr (0, pos);
				}
				
				//			cout << "ip:" << ip << " port: " << port << endl;
				Session::Get ().ConnectTo (ip.c_str(), (u16) stoi (port), i_handler);
				ipAddressFound = true;
			}
		}
		
		if (! ipAddressFound)
		{
			#if DEBUG
				Session::Get ().LogToFile ();
			#else
				if (epg_Init) SessionId (epg_Init (nullptr, 0, false, i_handler));
			#endif
		}
	}
};
#endif
	
struct Mute
{
	Mute () { epg_Disable (); }
	~Mute () { epg_Enable (); }
};
	
    
template <typename T> const uint8_t TypeId () { return '?'; }

template <> inline const uint8_t TypeId	<int> ()			{ return 'i'; }
template <> inline const uint8_t TypeId	<unsigned int> ()	{ return 'I'; }

template <> inline const uint8_t TypeId	<int64_t> ()		{ return 'l'; }
template <> inline const uint8_t TypeId	<uint64_t> ()		{ return 'L'; }

template <> inline const uint8_t TypeId	<float> ()			{ return 'f'; }
template <> inline const uint8_t TypeId	<double> ()			{ return 'd'; }

template <> inline const uint8_t TypeId	<bool> ()			{ return 'b'; }

    
class TraceClient
{
	public:
		TraceClient (const char * i_locationName, uint32_t i_lineNum, const char *i_name = 0)
		{
			m_locationId = epg_RegisterLocation (this, SessionId (), i_locationName, i_lineNum, i_name);
		}
	
		template <typename T>
		TraceClient (const char * i_locationName, uint32_t i_lineNum, const char *i_name, const T &i_variable)
		{
			m_locationId = epg_RegisterVariable (this, SessionId (), i_locationName, i_lineNum, i_name, TypeId<T>());
		}

		TraceClient (const char * i_locationName, uint32_t i_lineNum, const char *i_name, const char *i_variable)
		{
			m_locationId = epg_RegisterLocation (this, SessionId (), i_locationName, i_lineNum, 0);
		}

		template <typename T>
		TraceClient& operator () (const T &i_variable)
		{
			epg_LogVariable (m_locationId, &i_variable, sizeof (T));
			return *this;
		}

		TraceClient& operator () (const char *i_cstring)
		{
			epg_Log (m_locationId, i_cstring);
			return *this;
		}

		TraceClient& operator () (const Formatter &i_formatted)
		{
			epg_Log (m_locationId, i_formatted.str().c_str());
			return *this;
		}
		
	protected:
		uint32_t				m_locationId; // kill this? And, register location based on "this"
};
	
	
	template <typename T>
	T * SetCategory (T * i_ptr, cstr_t i_category)
	{
		static bool set = false;
		if (! set)
		{
			cstr_t className = Jd::ParseObjectName (i_ptr);
			epg_DefineCategory (className, i_category);
			set = true;
		}
		
		return nullptr;
	};
	
} // namespace Epilog



#if d_disableEpilog
#	define epilogger(NAME,PORT)
#else
#	if DEBUG
#		define epilogger(NAME,PORT) static Epilog::Session s_epigramConnection (#NAME,PORT, false);
#	else
#		define epilogger(NAME,PORT) static Epilog::Session s_epigramConnection (#NAME,PORT, true);
#	endif
#endif

// Macro awesomeness follows:

#define d_epResult1Args(ARG1)									epv(ARG1)
#define d_epResult2Args(...)									epf(__VA_ARGS__)
#define d_epResult3Args(...)									epf(__VA_ARGS__) 
#define d_epResult4Args(...)									epf(__VA_ARGS__)
#define d_epResult5Args(...)									epf(__VA_ARGS__)
#define d_epResult6Args(...)									epf(__VA_ARGS__)


#define d_epigramGet10thArg(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ...)	ARG10

#define d_epigramGet7thArg(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ...)	ARG7
#define d_epigramMacroChooser(...)							d_epigramGet7thArg(__VA_ARGS__, d_epResult6Args, d_epResult5Args, d_epResult4Args, d_epResult3Args, d_epResult2Args, d_epResult1Args )
#define epo(...)											d_epigramMacroChooser(__VA_ARGS__)(__VA_ARGS__)

#ifdef d_epilogObject
#undef d_epilogObject
#endif

#ifdef d_epilogObjC // __OBJC__
	#define d_epilogObject self
#else
	#define d_epilogObject this
#endif

// Different strategy? This avoids a copy
// memory = epg_NewEvent (); CreateEpilogEvent (memory, ...); epg_PushEvent (memory) /

#include <iomanip>

static void LogNow (uint8_t i_importance, const char * i_className, EpilogEvent * i_event)
{
	char str [4096];
	i_event->formatter (str, i_event);
	cout << std::setw (20) << i_className << "| " << str << endl;
}

#if 1
#define d_epilog(a_classification, ...) if (epg_LogDeferred) \
	{	\
		char stack [c_epilogStackSize]; \
		epg_LogDeferred (c_epilogClassification_##a_classification, Jd::ParseObjectName (d_epilogObject), CreateEpilogEvent (stack, __VA_ARGS__)); \
	}
#else
#define d_epilog(a_classification, ...) if (epg_LogDeferred) \
	{	\
		char stack [c_epilogStackSize]; \
		LogNow (c_epilogClassification_##a_classification, Jd::ParseObjectName (d_epilogObject), CreateEpilogEvent (stack, __VA_ARGS__)); \
	}
#endif

//#if 0
//#	define d_epilog_(a_classification, ...) if (epg_LogDeferred) { char stack [c_epilogStackSize]; epg_LogDeferred (c_epilogClassification_##a_classification, Jd::ParseObjectName (d_epilogObject), CreateEpilogEvent_ (stack, __VA_ARGS__)); }
//#else
//# 	define d_epilog_(a_classification, ...) if (true)fdsfsd { char stack [c_epilogStackSize]; fdsfsd LogNow (c_epilogClassification_##a_classification, Jd::ParseObjectName (d_epilogObject), CreateEpilogEvent_ (stack, __VA_ARGS__)); }
//#endif

#define d_epilog_normal(...)	d_epilog (normal, __VA_ARGS__)
#define d_epilog_fatal(...)		d_epilog (fatal, __VA_ARGS__)
#define d_epilog_detail(...)	d_epilog (detail, __VA_ARGS__)
#define d_epilog_tedious(...)	d_epilog (tedious, __VA_ARGS__)
#define d_epilog_warning(...)	d_epilog (warning, __VA_ARGS__)
#define d_epilog_special(...)	d_epilog (special, __VA_ARGS__)

#define d_epilog__normal(...)	d_epilog_ (normal, __VA_ARGS__)
#define d_epilog__fatal(...)	d_epilog_ (fatal, __VA_ARGS__)
#define d_epilog__detail(...)	d_epilog_ (detail, __VA_ARGS__)
#define d_epilog__tedious(...)	d_epilog_ (tedious, __VA_ARGS__)
#define d_epilog__warning(...)	d_epilog_ (warning, __VA_ARGS__)
#define d_epilog__special(...)	d_epilog_ (special, __VA_ARGS__)

#if DEBUG
//	#define epilog_ifdebug(...) epilog(__VA_ARGS__)
	#define d_epilog_realtime(...)	d_epilog (realtime, __VA_ARGS__)
	#define d_epilog__realtime(...)	d_epilog_(realtime, __VA_ARGS__)
#else
//	#define epilog_ifdebug(...)
	#define d_epilog_realtime(...)	{}
	#define d_epilog__realtime(...)	{}
#endif

#define epilog(a_classification, ...) d_epilog_##a_classification (__VA_ARGS__)
#define epilog_(a_classification, ...) d_epilog__##a_classification (__VA_ARGS__)

#if d_disableEpilog
	#undef epilog
	#undef epilog_
	#define epilog(...) {}
	#define epilog_(...) {}
	#define epilog_func(...) {}
#else
	#define epilog(a_classification, ...) d_epilog_##a_classification (__VA_ARGS__)
	#define epilog_(a_classification, ...) d_epilog__##a_classification (__VA_ARGS__)
	#define epilog_func(a_classification, ...) if (epg_LogDeferred) { char stack [c_epilogStackSize]; epg_LogDeferred (c_epilogClassification_##a_classification, 0, CreateEpilogEvent (stack, __VA_ARGS__)); }
#endif


#define epi_panel(panel_name, ...)							epg_NewPanel (#panel_name, __VA_ARGS__);
#define epi_logpanel(panel_name, uuid, ...)					epg_LogToPanel (#panel_name, uuid, ((Epilog::PanelFormatter(), __VA_ARGS__)));

#endif // if cpp

namespace Epilog
{
	template <typename T>
	struct ClassCategory
	{
		ClassCategory (cstr_t i_category)
		{
			static bool set = false;
			if (! set)
			{
				T * ptr = nullptr;
				cstr_t className = Jd::ParseObjectName (ptr);
				epg_DefineCategory (className, i_category);
				set = true;
			}
		}
	};
}

#define _epilogCat2(CATEGORY,CLASS) static Epilog::ClassCategory <CLASS> epilogCategory_##CLASS (#CATEGORY);
#define _epilogCat3(CATEGORY,CLASS1,CLASS2) _epilogCat2 (CATEGORY,CLASS1) _epilogCat2 (CATEGORY,CLASS2)
#define _epilogCat4(CATEGORY,CLASS1,...) _epilogCat2 (CATEGORY,CLASS1) _epilogCat3 (CATEGORY,__VA_ARGS__)
#define _epilogCat5(CATEGORY,CLASS1,...) _epilogCat2 (CATEGORY,CLASS1) _epilogCat4 (CATEGORY,__VA_ARGS__)
#define _epilogCat6(CATEGORY,CLASS1,...) _epilogCat2 (CATEGORY,CLASS1) _epilogCat5 (CATEGORY,__VA_ARGS__)
#define _epilogCat7(CATEGORY,CLASS1,...) _epilogCat2 (CATEGORY,CLASS1) _epilogCat6 (CATEGORY,__VA_ARGS__)
#define _epilogCat8(CATEGORY,CLASS1,...) _epilogCat2 (CATEGORY,CLASS1) _epilogCat7 (CATEGORY,__VA_ARGS__)
#define _epilogCat9(CATEGORY,CLASS1,...) _epilogCat2 (CATEGORY,CLASS1) _epilogCat8 (CATEGORY,__VA_ARGS__)

#define d_epilogCatChooser(...) d_epigramGet10thArg(__VA_ARGS__, _epilogCat9, _epilogCat8, _epilogCat7, _epilogCat6, _epilogCat5, _epilogCat4, _epilogCat3, _epilogCat2, _epilogCat1)


# if d_disableEpilog
#	define d_epilogCategory(...)
# else
#	define d_epilogCategory(...) d_epilogCatChooser(__VA_ARGS__)(__VA_ARGS__)
# endif


#undef type_if

#define Epilog_h
#endif

