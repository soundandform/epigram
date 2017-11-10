//
//  JdResult.h
//  Jigidesign
//
//  Created by Steven Massey on 9/3/15.
//  Copyright (c) 2015 Jigidesign. All rights reserved.
//

#ifndef __Jigidesign__JdResult__
#define __Jigidesign__JdResult__

#include "EpSerializationRaw.hpp"
#include "JdFlatString.hpp"
#include "JdCore.hpp"
#include "JdCoreL2.hpp"


struct JdResultConst
{
	cstr_t		message;
	cstr_t		location;
	u32			lineNum;
};

struct JdErrorConst
{
	cstr_t		message;
	cstr_t		location;
	u32			lineNum;
};


template <typename T1>
struct JdReturn
{
	tuple <T1> values;
};

template <typename T1>
tuple <T1> jd_return (T1 && i_return1) { return tuple <T1> (i_return1); }


struct JdR {};

template <typename t_location = cstr_t, typename R1 = JdR>
class JdResultT : public JdSerialize::Versioned <JdResultT <R1>, /* version: */ 1, /* epigram name: */ JdR>
{
	template <typename, typename> friend class JdResultT;

	public:
							JdResultT				() {}
							~JdResultT				() {}


	template <typename T1>
							JdResultT				(const tuple <T1> & i_return)
	{
		m_return = tuple <R1> (get <0> (i_return));
	}
	
							JdResultT				(const R1 & i_return)
							:
							m_return				(i_return)
							{ }

	
							JdResultT <JdR>			(const JdResultT <JdR> & i_return)
							:
							m_location				(i_return.m_location),
//							m_info					(i_return.m_info),
							m_resultCode			(i_return.m_resultCode),
							m_message				(i_return.m_message)
							{ }
	

							JdResultT				(const JdResultT &i_result, t_location i_location, u32 i_lineNum, u32 i_columnNum = 0)
							:
							m_location				(i_location),
							m_lineNum				(i_lineNum)
							{
								m_resultCode = i_result.m_resultCode;
								m_message = i_result.m_message;
							}


							JdResultT				(cstr_t i_message, t_location i_location, u32 i_lineNum, u32 i_columnNum, bool i_isFailure)
							:
							m_message				(i_message),
							m_resultCode			(i_isFailure ? -1 : 0),
							m_location				(i_location),
							m_lineNum				(i_lineNum),
							m_columnNum				(i_columnNum)
							{ }
	
							
							JdResultT				(i32 i_result, t_location i_location = 0, u32 i_lineNum = 0, u32 i_columnNum = 0)
							:
							m_resultCode			(i_result),
							m_location				(i_location),
							m_lineNum				(i_lineNum),
							m_columnNum				(i_columnNum)
							{ }

	
							JdResultT				(cstr_t i_message, bool i_isError, bool) // use this to define constants (d_jdResult)
							:
							m_message				(i_message)
	{
		i32 hash = Jd::HashCString31 (i_message);
		hash |= (0x80000000 * (i32) i_isError);
		m_resultCode = (i32) hash;
	}
	
	template <typename L, typename R>
	JdResultT &				operator =				(const JdResultT <L,R> & i_other)
	{
		if ((voidptr_t) & i_other != this)
		{
			m_resultCode	= i_other.m_resultCode;
//			m_info		 	= i_other.m_info;
			m_lineNum		= i_other.m_lineNum;
			m_columnNum		= i_other.m_columnNum;
			m_location 		= t_location ();
			m_message		= i_other.m_message;
			m_return 		= R1 ();
		}
		return * this;
	}
	
	// anything that is not an error is positive; negative if it is an error; zero meaning "okay" state

	bool					operator ==				(i32 i_result) const
	{
		return i_result == m_resultCode;
	}
	
	bool					operator !=				(i32 i_result) const
	{
		return i_result != m_resultCode;
	}
	
	
	JdResultT &				operator |=				(const JdResultT &i_result)
	{
		if (m_resultCode == 0)
		{
			m_resultCode = i_result.m_resultCode;
//			m_info = i_result.m_info;
			m_location = i_result.m_location;
			m_message = i_result.m_message;
		}
		
		return *this;
	}

	void					SetReturn 				(const R1 & i_return)
	{
		m_return = i_return;
	}

	R1						GetReturn 				() const
	{
		return get <0> (m_return);
	}

	
	// FIX: Get these
	i32						Code					() const
	{
		return m_resultCode;
	}
	
	string					Message					() const
	{
		return m_message;
	}
	
	t_location				GetLocation				() const
	{
		return m_location;
	}
	
	u32						GetLineNum				() const
	{
		return m_lineNum;
	}

	void					SetLineNum				(u32 i_lineNum)
	{
		m_lineNum = i_lineNum;
	}

	u32						GetColumnNum			() const
	{
		return m_columnNum;
	}
	
	void					SetColumnNum			(u32 i_lineNum)
	{
		m_columnNum = i_lineNum;
	}

	void					SetLocationInfo			(t_location i_location)
	{
		m_location = i_location;
	}
	
	bool					IsError () const
	{
		return (m_resultCode < 0);
	}

	bool					IsInfo () const
	{
		return (m_resultCode > 0);
	}
	
//	bool					IsEpMsg () const
//	{
//		return (GetType() == 'm');
//	}
	
	operator bool			() const		{ return m_resultCode != 0; }


	static bool				PauseOnError (i8 i_trueFalseOrTest = -1)
	{
		static bool pauseOnError = false;
		
		if (i_trueFalseOrTest == 1)			pauseOnError = true;
		else if (i_trueFalseOrTest == 0)	pauseOnError = false;
		
		return pauseOnError;
	}
	
	protected:
//		u8						GetType () const
//		{
//			u32 type = (m_info >> 16) & 0x000000FF;
//			return (u8) type;
//		}

	
	i32							m_resultCode			= 0;
//	u32							m_info					= 0;						// [ 8: reserved | 8: kind | 16: lineNum ]
	
	u32							m_columnNum				: 10;
	u32							m_lineNum				: 22;
	
	t_location					m_location				= t_location ();
	JdFlatString <112>			m_message;
	
	tuple <R1>					m_return;
	
	public:

	template <typename ST> void Serializer (ST &o) const
	{
		o (m_resultCode, m_location).CString (m_message.CString (), m_message.Capacity ());		/// FIX: line/column
	}

	template <typename ST> void Serializer (ST &i)
	{
		i (m_resultCode, m_location).CString (m_message.CString (), m_message.Capacity ());		/// FIX: line/column
	}
};

typedef JdResultT <> JdResult;

template <typename L, typename R1>
std::ostream & operator << (std::ostream &output, const JdResultT <L, R1> & i_result)
{
	char code [16];
	
	i32 resultCode = i_result.Code();
	if (resultCode == 0)
		sprintf (code, "ok");
	else
		sprintf (code, "%d", i_result.Code ());
	
	string msg = i_result.Message ();
	
	output << msg;
//	if  (msg.size ()) output << " ";
	
	if (resultCode != -1 or msg.size() == 0) output << " (" << code << ")";
	
	if (i_result.GetLocation ())
	{
		string location = Jd::ToString (i_result.GetLocation ());
		
		auto pos = location.find ("..");
		if (pos != string::npos)
			location = location.substr (pos + 2);
		
		output << "<" << location << ":" << i_result.GetLineNum () << ">";
	}
	
	return output;
}

template <typename R1A, typename R1B>
bool operator == (const JdResultT <R1A> &i_a, const JdResultT <R1B> &i_b)
{
	if (i_a.Code() == i_b.Code() /*&& i_a.msg == i_b.msg*/) return true;
	else return false;
}


template <typename R1A, typename R1B>
bool operator != (const JdResultT <R1A>  &i_a, const JdResultT <R1B> &i_b)
{
	if (i_a.Code() != i_b.Code() /*&& i_a.msg == i_b.msg*/) return true;
	else return false;
}


#define d_jdResultConst(CLASS,NAME,DESC)	namespace c_jd##CLASS { const JdResult NAME (DESC,false,true); }
#define d_jdErrorConst(CLASS,NAME,DESC)		namespace c_jd##CLASS { const JdResult NAME (DESC,true,true); }

#define d_jdReturn(...) return jd_return (__VA_ARGS__)

const JdResult c_jdNoErr;

#define d_get8thArg(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ...) ARG8
#define d_get7thArg(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ...) ARG7
#define d_get6thArg(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ...)	ARG6
	
#define d_jdResult(...)											JdResult ((JdFormatter() << __VA_ARGS__), __FILE__, __LINE__, 0, false)
#define d_jdError(...)											JdResult ((JdFormatter() << __VA_ARGS__), __FILE__, __LINE__, 0, true)
#define d_jdError2(...)											JdResult (Jd::SPrintF(__VA_ARGS__).c_str(), __FILE__, __LINE__, 0, true)

#endif /* defined(__Jigidesign__JdResult__) */
