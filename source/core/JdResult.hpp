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
	std::tuple <T1> values;
};

template <typename T1>
std::tuple <T1> jd_return (T1 && i_return1) { return std::tuple <T1> (i_return1); }


struct JdR {};

struct JdFileLocation
{
	typedef cstr_t type;
	
	void		SetLocation		(voidptr_t i_location)		{ m_location = (cstr_t) i_location; }
	cstr_t		GetLocation		() const					{ return m_location; }

	void		SetLineNum		(u32 i_line)				{ m_lineNum = i_line; }
	u32			GetLineNum		() const					{ return m_lineNum; }
	
	cstr_t		m_location				= nullptr;
	u32			m_lineNum				= 0;
};


template <typename R1, typename t_locationInfo = JdFileLocation>
class JdResultT : public t_locationInfo, public JdSerialize::Versioned <JdResultT <R1, t_locationInfo>, /* version: */ 1, /* epigram name: */ JdR>
{
	typedef typename t_locationInfo::type location_t;
	
	template <typename, typename> friend class JdResultT;

	public:
							JdResultT				() {}
//							~JdResultT				() {}


	template <typename T1>
							JdResultT				(const std::tuple <T1> & i_return)
	{
		m_return = std::tuple <R1> (std::get <0> (i_return));
	}
	
							JdResultT				(const R1 & i_return)
							:
							m_return				(i_return)
							{ }


							JdResultT 				(const JdResultT & i_result)
							:
							t_locationInfo			(i_result),
							m_resultCode			(i_result.m_resultCode),
							m_message				(i_result.m_message),
							m_return				(i_result.m_return)
							{ }

	
							template <typename R, typename L>
							JdResultT 				(const JdResultT <R,L> & i_result)
							:
							t_locationInfo			(i_result),
							m_resultCode			(i_result.m_resultCode),
							m_message				(i_result.m_message)
							{ }
	

							JdResultT				(const JdResultT &i_result, location_t i_location, u32 i_lineNum)
							{
								t_locationInfo::SetLocation (i_location);
								t_locationInfo::SetLineNum (i_lineNum);
								
								m_resultCode = i_result.m_resultCode;
								m_message = i_result.m_message;
							}

							JdResultT				(std::string_view i_message)
							:
							m_message				(i_message),
							m_resultCode			(-1)
							{ }

	
							JdResultT				(i32 i_resultCode)
							:
							m_resultCode			(i_resultCode)
							{ }

	
							JdResultT				(i32 i_resultCode, std::string_view i_message, location_t i_location = 0, u32 i_lineNum = 0)
							:
							m_resultCode			(i_resultCode),
							m_message				(i_message)
							{
								t_locationInfo::SetLocation (i_location);
								t_locationInfo::SetLineNum (i_lineNum);
							}

	
							JdResultT				(std::string_view i_message, location_t i_location, u32 i_lineNum = 0)
							:
							m_message				(i_message),
							m_resultCode			(-1)
							{
								t_locationInfo::SetLocation (i_location);
								t_locationInfo::SetLineNum (i_lineNum);
							}

	
							JdResultT				(std::string_view i_message, bool i_isError, bool) // use this to define constants (d_jdResult)
							:
							m_message				(i_message)
	{
		i32 hash = Jd::HashCString31 (i_message.data ());
		hash |= (0x80000000 * (i32) i_isError);
		m_resultCode = (i32) hash;
	}

	JdResultT &				operator =				(std::string_view i_message)
	{
		if (i_message.data ())
		{
			m_message = i_message;
			m_resultCode = -100;
		}
		
		return * this;
	}

	JdResultT &				operator =				(const JdResultT <R1, t_locationInfo> & i_other)
	{
		if ((voidptr_t) & i_other != this)
		{
			* (t_locationInfo *) this = i_other;

			m_resultCode		= i_other.m_resultCode;
			m_message			= i_other.m_message;
			m_return 			= i_other.m_return;
		}
		return * this;
	}

	template <typename L, typename R>
	JdResultT &				operator =				(const JdResultT <L,R> & i_other)
	{
		if ((voidptr_t) & i_other != this)
		{
			m_resultCode		= i_other.m_resultCode;
			
//			* (t_locationInfo *) this = i_other;
			
//			m_lineNum			= i_other.m_lineNum;
//			m_columnNum			= i_other.m_columnNum;
//			this->m_location	= t_locationInfo ();
			
			m_message			= i_other.m_message;
			std::get <0> (m_return)	= R1 ();
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
			this->m_location = i_result.m_location;
			m_message = i_result.m_message;
		}
		
		return *this;
	}

	void					SetReturn 				(const R1 & i_return)
	{
		std::get <0> (m_return) = i_return;
	}

	R1						GetReturn 				() const
	{
		return std::get <0> (m_return);
	}

	const R1 &					Return 					() const
	{
		return std::get <0> (m_return);
	}

	
	// FIX: Get these
	i32						Code					() const
	{
		return m_resultCode;
	}

	void					SetResultCode			(i32 i_resultCode)
	{
		m_resultCode = i_resultCode;
	}
	
	i32						GetCode					() const
	{
		return m_resultCode;
	}
	
//	void					Clear					()
//	{
//		m_resultCode = 0;
//		m_location = t_location ();
//		m_columnNum = 0;
//		m_lineNum = 0;
//	}
	
	void					SetMessage				(stringRef_t i_message)
	{
		m_message = i_message;
	}
	
	std::string				GetMessage				() const
	{
		return m_message;
	}
	
	bool					IsError () const
	{
		return (m_resultCode < 0);
	}

	bool					IsInfo () const
	{
		return (m_resultCode > 0);
	}
	
	
	operator bool			() const		{ return m_resultCode != 0; }


	static bool				PauseOnError (i8 i_trueFalseOrTest = -1)
	{
		static bool pauseOnError = false;
		
		if (i_trueFalseOrTest == 1)			pauseOnError = true;
		else if (i_trueFalseOrTest == 0)	pauseOnError = false;
		
		return pauseOnError;
	}
	
	protected:

	i32							m_resultCode			= 0;
	JdFlatString <112>			m_message;
	
	std::tuple <R1>				m_return;
	
	public:

	template <typename ST> void Serializer (ST &o) const
	{
		o (m_resultCode, this->m_location).CString (m_message.CString (), m_message.Capacity ());		/// FIX: line/column
	}

	template <typename ST> void Serializer (ST &i)
	{
		i (m_resultCode, this->m_location).CString (m_message.CString (), m_message.Capacity ());		/// FIX: line/column
	}
};

typedef JdResultT <JdR, JdFileLocation> 			JdResult;
typedef const JdResult & 							JdResultRef;

template <typename L, typename R1>
std::ostream & operator << (std::ostream &output, const JdResultT <L, R1> & i_result)
{
	using namespace std;

	char code [16];
	
	i32 resultCode = i_result.Code();
	if (resultCode == 0)
		sprintf (code, "ok");
	else
		sprintf (code, "%d", i_result.Code ());
	
	string msg = i_result.GetMessage ();
	
	output << msg;
//	if  (msg.size ()) output << " ";
	
	if (resultCode != -1 or msg.size() == 0) output << " (" << code << ")";
	
	if (i_result.GetLocation ())
	{
		string location = Jd::ToString (i_result.GetLocation ());
		
		auto pos = location.find ("..");
		if (pos != string::npos)
			location = location.substr (pos + 2);
		
		output << " <" << location << ":" << i_result.GetLineNum () << ">";
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
	
#define d_jdResult(...)											JdResult ((JdFormatter() << __VA_ARGS__), __FILE__, __LINE__)
#define d_jdError1(...)											JdResult ((JdFormatter() << __VA_ARGS__), __FILE__, __LINE__)
#define d_jdError(...)											JdResult (Jd::SPrintF(__VA_ARGS__).c_str(), __FILE__, __LINE__)
#define d_jdErr(...)											d_jdError (__VA_ARGS__)

#endif /* defined(__Jigidesign__JdResult__) */
