//
//  EpSerializationRaw.h
//  Jigidesign
//
//  Created by Steven Massey on 5/12/13.
//  Copyright (c) 2013 Jigidesign. All rights reserved.
//

#ifndef Jigidesign_EpSerializationRaw_h
#define Jigidesign_EpSerializationRaw_h

#include <iostream>
#include <assert.h>
#include <algorithm>
#include <type_traits>
#include <cstring>
#include <string.h>

#include "JdNucleus.hpp"
#include "JdUtils.hpp"
#include "JdTypeId.hpp"

#ifdef d_epigramUseCityHash
#	include "city.h"
#endif


namespace EpSerialization
{
	template <int t_nameLength>
	struct TemplateNameParser
	{
		TemplateNameParser (cstr_t i_prettyFunction)
		{
//		 	cout << i_prettyFunction << endl;
			// example: static const char *EpSerializable<Serialized2>::ClassName() [T = Serialized2]
			// NOTE: could also parse '[T = ...]'s

			char * dest = & name [1];
			const ptrdiff_t maxNameLength = t_nameLength - 2;
			strncpy (dest, "(Unparsable)", maxNameLength);
			
			const char * openBrace = strstr (i_prettyFunction, "<");
			
			if (openBrace)
			{
				cstr_t start = openBrace + 1; // move past '<'
				const char * comma = strstr (start, ",");
				
				if (comma)
				{
					--comma;
					
					size_t length = comma - start;
					if (length < maxNameLength)
					{
						strncpy (dest, start, length);
					}
				}
				
				
				/*
				cstr_t start = ++openBrace; // move past '<'
				size_t length = strlen (start);
				
				i32 scope = 1;
				for (size_t i = 0; i < length; ++i)
				{
					if (start [i] == '>')
						--scope;
					else if (start [i] == '<')
						++scope;
					
					if (scope == 0)
					{
						if (i <= maxNameLength)
						{
							strncpy (dest, start, i);
							dest [i] = 0;
						}
						
						break;
					}
				}
				*/
			}
			
			dest [maxNameLength] = 0;

			name [0] = strlen (dest);
		}
		
		const char * Name () const { return & name [1]; }
		
		protected:
		char name [t_nameLength];
	};
	
	template <int t_nameLength>
	struct ClassNameParser
	{
		ClassNameParser (const char *i_prettyFunction)
		{
			name [1] = 0;
			
			char *dest = &name[1];
			const size_t nameLength = t_nameLength - 2;

			const char *colons = strstr (i_prettyFunction, "::ClassName()");

			if (colons == 0)
			{
				strncpy (dest, "(Unparsable)", nameLength);
				dest [nameLength] = 0;
			}
			else
			{
				const char *ptr = --colons;
				while (ptr != i_prettyFunction)
				{
					char c = *ptr;

					// gotta do this 'cause LLVM started concatenating a "*" return value w/ the function name.  I suspect & would
					// have similar results.  How 'bout a space guys?
					
					if (c == ' ' || c == '*' || c == '&')
					{
						uint32_t length = (uint32_t) std::min (nameLength, (size_t) (colons-ptr));
						strncpy (dest, ptr+1, length);
						dest [length] = 0;
						break;
					}
					--ptr;
				}
			}

			name [0] = strlen (dest);
		}

		const char * Name () const { return & name [1]; }
		
		protected:
			char name [t_nameLength];
	};
	
	struct ClassNameHasher 
	{
		ClassNameHasher (const char *i_className)
		{
			#ifdef d_epigramUseCityHash
				m_hash = CityHash64 (i_className, *(i_className-1));
			#endif
		}
		
		uint64_t operator () () { return m_hash; }
		
		protected:
		uint64_t	 m_hash			= 0;
	};

} // namespace EpSerialization



/*	This function creates a hybrid c+pascal string (the length is encoded in the first byte while also being null-terminated.)
	The pascal-ness is hidden by returning a pointer advanced by one byte. */
 
#define d_serializeClassName													\
static const char * ClassName()													\
{																				\
	static EpSerialization::ClassNameParser <128> parser (__PRETTY_FUNCTION__);	\
	return parser.Name();														\
}																				\
static uint64_t ClassHash ()													\
{																				\
	static EpSerialization::ClassNameHasher hasher (ClassName()) ;				\
	return hasher ();															\
}



template <typename allocator_t>
class EpSerializer
{
	public:							EpSerializer		(allocator_t & i_allocator)
									:
									m_allocator			(i_allocator)
									{ }


	template <typename T>
	EpSerializer &					operator ()			(const T& i_value)
	{
		//if (! boost::is_pod <T> ()) cout << "EpSerializerOut warning: " << typeid(T).name() << " isn't P.O.D.\n";
		
		m_numBytes += sizeof (T);
		u8 * ptr = m_allocator.Allocate (sizeof (T));
		memcpy (ptr, (void *) &i_value, sizeof (T));
		
//		cout << "serial: " << (void *) ptr << endl;
		
		return *this;
	}
	
	template <typename T, typename... Args>
	EpSerializer &					operator ()			(const T& i_value, const Args &... i_args)
	{
		operator () (i_value);
		operator () (i_args...);
		
		return * this;
	}
	
	
	template <typename T>
	EpSerializer &					Compact				(const T& i_value)
	{
		Jd::_7bRE value (i_value);

		u8 * ptr = m_allocator.Allocate (value.numBytes);
		memcpy (ptr, (void *) value.bytes, value.numBytes);
		m_numBytes += value.numBytes;
		
//		cout << "compact: " << value.numBytes << endl;
		//		cout << "serial: " << (void *) ptr << endl;
		
		return *this;
	}
	
	template <typename T, typename ... Args>
	EpSerializer &					Compact				(const T& i_value, const Args & ... i_args)
	{
		Compact (i_value);
		Compact (i_args...);
		
		return *this;
	}
	
	

	EpSerializer &					CString				(cstr_t i_string, u32 i_maxStringLength)
	{
		size_t numBytes = strlen (i_string) + 1;
	
		m_numBytes += numBytes;
		u8 * ptr = m_allocator.Allocate (numBytes);
		memcpy (ptr, i_string, numBytes);
		
		return * this;
	}

	void							Store				(voidptr_t i_data, u32 i_numBytes)
	{
		u8 * ptr = m_allocator.Allocate (i_numBytes);
		m_numBytes += i_numBytes;
		memcpy (ptr, (void *) i_data, i_numBytes);
	}
	
	
	u32								NumBytes ()		{ return m_numBytes; }
	
	protected://-------------------------------------------------------------------------------------------------------------------
	allocator_t &					m_allocator;
	u32								m_numBytes		= 0;
};


class EpDeserializer
{
	public:						EpDeserializer			(voidptr_t i_ptr, size_t i_bytes, u8 /* i_version */)
	:
	m_ptr						((const u8 *) i_ptr),
	m_numBytes					(i_bytes)
	{ }
		
	template <typename T>
	EpDeserializer &			operator ()			(T& o_value)
	{
		//if (! boost::is_pod <T> ()) cout << "EpSerializerIn warning: " << typeid(T).name() << " isn't P.O.D.\n";
//		cout << "de-serial: " << (void *) m_ptr << endl;;
		
		if (m_numBytes >= sizeof (T))
		{
			memcpy ((void *) &o_value, m_ptr, sizeof (T));
			m_ptr += sizeof (T);
			m_numBytes -= sizeof (T);
		}
		else
		{
			m_numBytes = 0; // something wrong or this is just an older record with less entries; either way, abort subsequent in's.
			m_underrun = true;
		}

		return *this;
	}

	
	template <typename T, typename ... Args>
	EpDeserializer &			operator ()			(T& o_value, const Args & ... i_args)
	{
		operator () (o_value);
		operator () (i_args...);
		
		return *this;
	}

	
	template <typename T>
	EpDeserializer &				DeCompact			(T & o_value)
	{
		if (m_numBytes >= 1)
		{
			auto end = m_ptr + m_numBytes;
			o_value = Jd::Decode7bRE <T> (m_ptr, end);
			m_numBytes = end - m_ptr;
		}
		else m_underrun = true;
		
		return *this;
	}
	
	
	template <typename T, typename ... Args>
	EpDeserializer &				/*De*/Compact		(T&& o_value, Args && ... i_args)
	{
		DeCompact (std::forward <T> (o_value));
		DeCompact (i_args...);
		
		return *this;
	}

	EpDeserializer &				CString				(char * o_string, u32 i_maxStringLength)
	{
		size_t numBytes = Strnlen ((cstr_t) m_ptr, m_numBytes);		// strnlen wasn't avail on arm gcc for some reason
		
		numBytes = std::min (numBytes, (size_t) i_maxStringLength);
		
		memcpy (o_string, m_ptr, numBytes);
		o_string [numBytes] = 0;
		
		++numBytes;
		m_numBytes -= numBytes;
		m_ptr += numBytes;

		return * this;
	}
	
	size_t							Strnlen				(cstr_t i_cstring, size_t i_maxBytes)
	{
		size_t s = 0;
		while (i_maxBytes--)
		{
			if (i_cstring [s] == 0)
				break;
			++s;
		}
		
		return s;
	}


//		template <typename T>
//		EpSerializerIn &			operator ()			(const T * i_value, uint32_t i_count)
//		{
//			uint32_t bytes = sizeof (T) * i_count;
//			if (m_numBytes >= bytes)
//			{
//				memcpy ((void *) &i_value, m_ptr, bytes);
//				m_ptr += bytes;
//				m_numBytes -= (uint32_t) bytes;
//			}
//			else
//			{
//				m_numBytes = 0; // something wrong; abort subsequent
//				m_underrun = true;
//			}
//			
//			return *this;
//		}
	
	bool						Underran ()		{ return m_underrun; }
	size_t						NumBytes ()		{ return m_numBytes; }
	
	friend class EpMsgSerializerIn;
	
	protected:
	const u8 *				m_ptr;
	size_t					m_numBytes;
	bool					m_underrun			= false;
};


//----------------------------------------------------------------------------------------------------------------------------------------------------



namespace JdSerialize
{
	template <typename T, typename NAME = T>
	class Base
	{
		public:
		
		static const char *		ClassName ()
		{
			return Jd::ParseClassName <NAME> ();
		}
		
		static u64				ClassHash ()
		{
			static EpSerialization::ClassNameHasher hasher (ClassName ());
			return hasher ();
		}
		
		void Deserialize (voidptr_t i_data, size_t i_numBytes, u8 i_version)
		{
			EpDeserializer deserializer (i_data, i_numBytes, i_version);
			
			auto _this = static_cast <T *> (this);
			_this->Serializer (deserializer);
		}
		
		template <typename A> size_t Serialize (A &o_allocator) const
		{
			EpSerializer <A> serializer (o_allocator);
			
			auto _this = const_cast <T *> (static_cast <const T *> (this));
			_this->Serializer (serializer);
			
			return serializer.NumBytes ();
		}
		
		#if 0 // Implement this. Can use d_jdSerialize (...) helper macro
				template <typename T> void Serializer (T &io) const { io (m_memberX, m_memberY, m_memberZ); }
		#endif
	};
	
//	class VersionedT {}; // This is just used as type trait flag
	
	template <typename T, u8 t_version = 1, typename NAME = T>
	class Versioned : public Base <T,NAME>, public Jd::TypedT <c_jdTypeId::versionedObject>
	{
		public: static u8 ClassVersion () { return t_version; };
	};
	
//	class UnversionedT {}; // This is just used as type trait flag
	
	template <typename T, typename NAME = T>
	class Unversioned : public Base <T, NAME>, public Jd::TypedT <c_jdTypeId::object>
	{
		public: static u8 ClassVersion () { return 0; };
	};
}


#define d_jdSerialize(...) public: template <class JDSERIALIZET> void Serializer (JDSERIALIZET &io) { io (__VA_ARGS__); }
#define d_jdCompactSerialize(...) public: template <typename T> void Serializer (T &io) { io.Compact (__VA_ARGS__); }




//#define d_rawSerialize(...) 	public: \
//								void Deserialize (voidptr_t i_ptr, u32 i_size, u8 i_version) \
//									{ EpSerializerIn in (i_ptr, i_size, i_version); Serializer (in); } \
//								template <typename A> u32 Serialize (A &o_allocator) const	\
//									{ EpSerializerOut <A> out (o_allocator); Serializer (out); return out.NumBytes (); } \
//								template <typename T> void Serializer (T &io) const					{ io (__VA_ARGS__); }



#endif
