/*
 *  Epigram.hpp
 *
 *  Created by Steven Massey on 9/20/11.
 *  Copyright 2011-2015 Epigram Software LLC. All rights reserved.
 *
 */

#if 0
	// FIX: this is invalid. the Iterator temporary goes away, invalidating the element reference.
	auto & element = * msg.begin ();

	// FIX: frequent gotcha: key lookup is too explicit
	msg [0] // a signed index won't find a key of type <u32>; should look for <i32> first then try <u32> if i_index >= 0, i reckon?


/*
	Creation/Insertion
	------------------------------------------------------------------------------------------------------------------------------------------------------
	There's two insertion methods: argument & map style.  Argument-style uses an overloaded () operator in the form: ("key", value)  Argument-style can
	concatenate insertions since the operator returns a reference back to the same message object.  Map-style uses overloaded [] operators.  Examples follow:

	 
		Unnamed temporary:
			object-> MemberFunction (msg_ ("argumentX", valueOfX) ("argumentY", valueOfY));		// with helper msg_ macro
			object-> MemberFunctionB (Epigram("msg-what") ("argumentZ", valueOfZ));				// without helper macro + a message "what"


		Named stack object:
			Epigram msg; msg ("number", 4394) ("e", 2.718);

			object-> MemberFunctionC (msg);


		Named stack object, map-style: Doesn't insert duplicate entries.  Useful for more generic metadata records.

			Epigram metadata;
			metadata ["id"] = 4394;
			metadata ["path"] = "/Blah/blah/blah";
*/

	/* Extraction
	----------------------------------------------------------------------------------------------------------------------------------------------------*/

		float f = msg ["name of float"];				// f equals 0.0f if "name of float" nonexistent

		MyObject obj = msg ["name of an object"];		// MyObject must implement d_jdSerialize () or d_msgSerialize().  See EpSerialization.


	/* Soft Extraction
	----------------------------------------------------------------------------------------------------------------------------------------------------*/

		double d = 123.4;
		msg ["name of double"] >> d;					// doesn't overwrite d (d still equals 123.4) if "name of double" doesn't exists inside message


	/* Hard Extraction
	----------------------------------------------------------------------------------------------------------------------------------------------------*/

		// due to ambiguous overloading of the string class and loose casting of the epigramitem class, this is ISN'T possible:

		string t;
		t = msg ["name of string"];

		// so, use this syntax instead:

		string s;
		s << msg ["name of string"];

		// or:

		s = msg ["name of string"].To <String> ();

		// However,, if using an EpAttribute, you can use the equals syntax

		s = msg [a_testAttribute::someString];


	/* Extraction by index
	----------------------------------------------------------------------------------------------------------------------------------------------------*/

		if (msg.Index (i).Is <int> ())
		{
			int x = msg.Index (i);
		}


	/* Range-based Loop Iteration
	----------------------------------------------------------------------------------------------------------------------------------------------------*/

		for (auto & i : msg)
		{
			cout << i.GetKeyString () << " = " << i.GetValueTypeName () << endl;
		}


	/* Discovery
	----------------------------------------------------------------------------------------------------------------------------------------------------*/
	msg [index/name].IsSet()							// true if element is defined to something
	msg [index/name].IsNull()
	msg [index/name].GetValueTypeName ()  						// return c-string descriptor; returns the class name if an object
	msg [index/name].TypeId ()							// a char
	msg [index/name].Is < fundamental/class > ()		// classes must implement EpSerialization
	
	e.g.
 
	if (msg ["key"] == "value")
	{
	}

	if (msg ["name"].To <int> () == 999)
	{
	}

	if (msg [i].Is <JdUUID> ())
	{
 		JdUUID uuid = msg [i];
	}
 

	/* Sending EpMsgs
	------------------------------------------------------------------------------------------------------------------------------------------------------
 	Epigrams automatic convert to IEpigram's and in general I'm using a "const IEpigram *" (typedef: EpDelivery) to push messages across function
 	calls.  I figure this pure virtual interface method is less fragile as Epigrams are pushed across compilation/language boundaries.  More importantly
 	it allows for proxy implementations of the Epigram and for requesting asynchronous delivery (by the recipient) in certain situations.
 
 
	Argument-style vs. Map-style
	------------------------------------------------------------------------------------------------------------------------------------------------------
 	- duplicate-name prevention is implemented for map style insertion: 
 			msg ["name"] = value; 			// replaces existing "name" element

 	But not implemented for argument style insertion:
 			msg ("name", value);				// inserts duplicate "name" entry. Won't cause a crash.  First will be found for a search.
 
 	This isn't a shortcoming: argument-style is intended to be fast for function calls. Just don't do it, fool. Not too hard.
	 
 
 	Notes
	------------------------------------------------------------------------------------------------------------------------------------------------------
 	- no endian swapping checks
	- need to do overflow/underflow testing

	 
 	Implementation Details
	------------------------------------------------------------------------------------------------------------------------------------------------
	 
 
	fundamentals
	------------------------------------------------------------------------------------------------------------------------------------------------

	objects
	------------------------------------------------------------------------------------------------------------------------------------------------
	 
	POD structs
	----------------------------------
	
		[ 7+: sizeof-POD ] [ 8+: class-name-plus-null ] repeat: { pod-n [ 8+: pod-data ]}
	
	 
	 //----------------------------------------

	 
	 (  8+: value-count    )		extant if isArray flag is set
	
	 {  X:  value-payload  }

	 {  Y:  key-payload    }
	
	 {  8+: key-size       }
	 [  8:  key-type       ]
	 [  8:  value-type     ]
	 {  8+: element-size   }
	 
	 
	QFT:
	----------------------------------------------
	 - Fetch of objects is screwed up it seems. Casted fetch for objects shouldn't actually "cast" but instead check that they are the same.
	Unless a casting system is added.
	 
		MuiPoint p = m_["rect"];  // rect is MuiRect
	
	 - TODO: object registration at init.  (note: would require mutex lock at lookup-time)
	 
		e.g. Epigram::RegisterClass <MuiPoint> ();  This would allow for:
			- object recognition and casting. See above
			- and smarter Dumps ()
		
	 - TODO: single integer key type?  7s encoded
	 
			- compaction could be deferred until Delivery
	
			- info byte ?
				- 2: version
				- 1: needs compaction
				- 1: is-key unique
				- 1: is-key-sorted
				- has sequence?
	 
	
	 - TODO: sequence assertion full converage
	 
*/

#endif

#pragma once

#include <string>

#include <vector>
#include <list>
#include <deque>
#include <set>
#include <map>
#include <functional>

#include "JdTypeId.hpp"
#include "JdAssert.hpp"
#include "IEpigram.hpp"
#include "EpSerializationRaw.hpp"
#include "EpArgs.hpp"

#pragma warning (disable: 4200)  // warning about zero size m_data [] in allocator

#define type_def typedef typename
#define type_if typedef typename std::conditional


class EpBinary : public Jd::TypedT <c_jdTypeId::binary>
{
	public:
	EpBinary () {}
	
	EpBinary (voidptr_t i_data, size_t i_size)
	:
	m_data		(i_data),
	m_size		(i_size) { }
	
	EpBinary & operator = (const EpBinary & i_binary)
	{
		size_t size;
		auto data = i_binary.Get (size);
		Set (data, size);
		
		return *this;
	}
	
	EpBinary (const std::function <void (voidptr_t i_data, size_t i_size)> & i_handler)
	:
	m_handler	(i_handler) { }
	
	void Set (voidptr_t i_data, size_t i_size)
	{
		m_data = i_data;
		m_size = i_size;
		
		if (m_handler)
		{
			m_handler (i_data, i_size);
		}
	}
	
	operator bool () const
	{
		return m_size and m_data;
	}
	
	voidptr_t Get (size_t & o_size) const
	{
		o_size = m_size;
		return m_data;
	}
	
	voidptr_t	GetData () const { return m_data; }
	size_t		GetSize () const { return m_size; }

	protected:
	
	voidptr_t												m_data = nullptr;
	size_t													m_size = 0;
	std::function <void (voidptr_t i_data, size_t i_size)>	m_handler;
};


const u32 c_epigramStackSize = 512;

void DumpEpigram (EpDelivery i_epigram, u32 i_sequence);

template <typename K, typename V> class EpAttribute;


namespace c_epigram
{
	const char * const nullString = "<\004null\004>"; // some invisible chars for uniqueness
};


template <typename T> inline T		EpigramDefaultValue () { return T(); }
template <> inline const char *		EpigramDefaultValue () { return c_epigram::nullString; }
template <> inline std::string		EpigramDefaultValue () { return c_epigram::nullString; }


struct EpNoType { };


template <typename T, typename S>
struct EpigramCast
{
	struct Auto
	{
		static T Cast (const S & i_value) { return (T) i_value; }
	};

	struct Null
	{
		static T Cast (const S &) { return T (); }
	};
	
	struct FundamentalToEnum			{ static T Cast (const S & i_value) { return (T) ((size_t) i_value); } };

	struct FundamentalToFundamental
	{
		static T Cast (const S &i_from)
		{
			return i_from;
		}
	};

//	struct EnumToFundamental
//	{
//		static T Cast (const S &i_from)
//		{
//			return i_from;
//		}
//	};

	struct FundamentalToString
	{
		template <typename F>
		static std::string Cast (const F &i_from)
		{
			std::ostringstream oss;
			oss << i_from;
			return oss.str();
		}
		
		static std::string Cast (const bool &i_from)
		{
			return i_from ? "true" : "false";
		}
	};

	struct StringToFundamental
	{
		static T Cast (const std::string &i_from)
		{
			T value = EpigramDefaultValue <T> ();
			std::istringstream iss (i_from);
			iss >> value;
			return value;
		}
	};

	type_if <	std::is_class <T>::value,									Null,						Auto>::type				caster_a;
	type_if <	jd::is_cstring <T>::value,									Null,						caster_a>::type			caster_b;
	
	type_if <	std::is_same <S, std::string>::value and
						std::is_fundamental <T>::value,						StringToFundamental,		caster_b>::type			caster_c;
	
	type_if <	std::is_same <T, std::string>::value and
						std::is_fundamental <S>::value,						FundamentalToString,		caster_c>::type			caster_d;

	type_if <	std::is_same <T, S>::value,									Auto,						caster_d>::type			caster_e;

	type_if <	std::is_pointer <T>::value and
						not std::is_pointer <S>::value,						Null,						caster_e>::type			caster_f;

	type_if <	std::is_enum <T>::value,									Null,						caster_f>::type			caster_x;
	type_if <	std::is_enum <T>::value and std::is_fundamental <S>::value,	FundamentalToEnum,			caster_f>::type			caster_g;


	typedef caster_x type;
};

template <u32 t_size>
class EpigramHybridAllocator
{
	template <u32 S> friend class EpigramHybridAllocator;
	
	public:
	struct BufferRange { u8 * start, * end; };
	

	u32			GetSequence			() const	{ return m_sequence; }
	
	u8 *		GetBuffer			() 			{ return m_start; }
	u8 * 		GetBufferEnd		() const	{ return m_end; }
	
	size_t 		GetCapacity			()			{ return m_size; }
	size_t		GetNumUsedBytes 	()			{ return (m_end - m_start); }
	size_t		GetNumFreeBytes		()			{ return m_size - (m_end - m_start); }
	
	bool		IsHeapAllocated	 	() const	{ return (m_start != m_data); }
	
	BufferRange GetBufferRange		() 			{ return { m_start, m_end }; }
	
	
	void Reset				()		// delete heap & return to stack storage
	{
		++m_sequence;
		
		if (IsHeapAllocated ())
			free (m_start);
		
		m_end = m_start = m_data;

		m_size = t_size;
	}

	void Clear				()
	{
		++m_sequence;
		m_end = m_start;
	}
	
	void EraseRange				(void * i_start, void * i_end)
	{
		++m_sequence;
		
		auto numBytesToMove = (u8 *) m_end - (u8 *) i_end;
		auto numBytesToRemove = (u8 *) i_end - (u8 *) i_start;
		
		memmove (i_start, i_end, numBytesToMove);
		m_end -= numBytesToRemove;
	}


	// TODO: use malloc/realloc
	void Grow					(size_t i_requiredBytes)
	{
		size_t newSize = CalculateGrowSize (i_requiredBytes);
		size_t oldSize = GetNumUsedBytes ();
		
		if (IsHeapAllocated ())
		{
			m_start = (u8 *) realloc (m_start, newSize);
		}
		else
		{
			m_start = (u8 *) malloc (newSize);
			memcpy (m_start, m_data, oldSize);
		}
		
		m_end = m_start + oldSize;
		m_size = newSize;
	}
	
	inline
	u8 * Allocate				(size_t i_requiredBytes)
	{
		if (i_requiredBytes > GetNumFreeBytes ())
			Grow (i_requiredBytes);
		
		++m_sequence;
		u8 * data = m_end;
		m_end += i_requiredBytes;
		
		return data;
	}
	
	
	~ EpigramHybridAllocator ()
	{
		if (IsHeapAllocated ())
			free (m_start);
	}
	
	EpigramHybridAllocator ()
	{
		d_jdAssert (m_start, "wtf?");
	}
	
	EpigramHybridAllocator (EpigramHybridAllocator &) = delete;
	EpigramHybridAllocator (EpigramHybridAllocator &&) = delete;
	EpigramHybridAllocator & operator= (EpigramHybridAllocator &) = delete;
	
	template <u32 S>
	void operator = (EpigramHybridAllocator <S> && i_allocator)
	{
//		cout << "moving\n";
		Reset ();
		
		auto buffer = i_allocator.GetBufferRange ();

		if (i_allocator.IsHeapAllocated ())
		{
			m_start = buffer.start;
			m_end = buffer.end;
			m_size = i_allocator.GetCapacity ();
			
			i_allocator.m_start = i_allocator.m_data;
		}
		else
		{
			size_t size = buffer.end - buffer.start;
			u8 * bytes = Allocate (size);
			memcpy (bytes, buffer.start, size);
		}
	}
	
	
	protected://-------------------------------------------------------
	size_t			CalculateGrowSize		(size_t i_extraRequiredBytes)
	{
		size_t newSize = std::max (((m_size * 207) >> 7) , m_size + i_extraRequiredBytes);
		newSize = (newSize + 15) & ~15;
		
		return newSize;
	}
	
	
	u8 *			m_start			= m_data;
	u8 *			m_end			= m_start;

	size_t			m_size			= t_size;
	u32				m_sequence		= 0;

	u8				m_data			[t_size];
};



template <typename allocator_t = EpigramHybridAllocator <c_epigramStackSize>, typename interface_t = IIEpigram>
class EpigramT : public interface_t
{
	public: template <typename AA, typename II> friend class EpigramT;
	
	private://----------------------------------------------------------------------------------------------------------------------
	
	typedef Jd::TypedT <c_jdTypeId::versionedObject> versionedObject_t;
	typedef Jd::TypedT <c_jdTypeId::object> unversionedObject_t;
	typedef Jd::TypedT <c_jdTypeId::binary> binary_t;
	typedef Jd::TypedT <c_jdTypeId::string> string_t;
	

	static const size_t c_payloadMinSize = 3; // key-size, value-type + key-type

	struct KeyValue
	{
		friend class EpigramT;
		
		protected:
		
		KeyValue			(EpigramT * i_epigram) : epigram (i_epigram) {}
		
		EpigramT *			epigram;

		u8 *				payload;	// valuePayload
		
		union
		{
			const u8 *		endPayload;
			const u8 *		keyPayload;
		};
		
		const u8 *			start;
		const u8 *			end;

		size_t				count					= 0;

		u32					sequence;
		
		u8					keyType					= c_jdTypeId::unknown;
		u8					valueType				= c_jdTypeId::unknown;
	};

	
	// Type Handler Helpers -------------------------------------------------------------------------------------------------------------------
	
	template <typename T>
	static const u8 * FetchHeaderClassName (const u8 * i_ptr, const u8 * i_end)
	{
		cstr_t className = Jd::ParseClassName <T> ();
		u32 classNameLength = * (className - 1) + 1 /*null*/;
		
		size_t i_availableBytes = i_end - i_ptr;
		
		if (i_availableBytes >= classNameLength)
		{
			for (u32 i = 0; i < classNameLength; ++i)
			{
				if (*className != *i_ptr)
					return nullptr;
				
				++className; ++i_ptr;
			}
			
			return i_ptr;
		}
		else return nullptr;
	}
	
	// Type Handlers ---------------------------------------------------------------------------------------------------------------------

	struct Payload
	{
		const u8 * start; const u8 * end;
		
		size_t GetNumBytes () { return end - start; }
	};
	typedef const Payload PayloadRef;
	
	template <typename T>
	struct FundamentalT
	{
		static constexpr u8	GetTypeId			()												{ return Jd::TypeId <T> (); }
		static void			StoreHeader			(allocator_t &)									{ }
		static const u8 *	FetchHeader			(u8 *, PayloadRef i_data)						{ return i_data.start; }
		
		
		static void			StoreItem			(allocator_t & i_allocator, const T & i_value)
		{
			u32 requiredSize = sizeof (T);
			u8 * data = i_allocator.Allocate (requiredSize);
			memcpy (data, & i_value, requiredSize);
		}
		
		static const u8 *	Fetch				(T & o_value, const u8 * i_iterator, PayloadRef i_payload, u8 unused = 0)
		{
			size_t availableBytes = i_payload.end - i_iterator;
			
			if (availableBytes >= sizeof (T))
			{
//				o_value = * ((T *) i_iterator);
				memcpy (& o_value, i_iterator, sizeof (T));
				return i_iterator + sizeof (T);
			}
			else
			{
				d_jdThrow ("epigram underrun");
				return nullptr;
			}
		}
		
		static bool			IsType				(u8 i_type, PayloadRef)
		{
			return (i_type == Jd::TypeId <T> ());
		}

		
		typedef	T compare_t;
		typedef T store_t;
	};

	
	template <typename T>
	struct BinaryT
	{
		static constexpr u8	GetTypeId			() { return c_jdTypeId::binary; }
		
		static void			StoreHeader			(allocator_t &)									{ }
		static const u8 *	FetchHeader			(u8 *, PayloadRef i_data)						{ return i_data.start; }
		
		static void			StoreItem			(allocator_t & i_allocator, const T & i_value)
		{
			size_t requiredSize;
			auto source = i_value.Get (requiredSize);
		
			Jd::_7bRE bs (requiredSize);
			u8 * dest = i_allocator.Allocate (requiredSize + bs.numBytes);
			bs.Copy (dest);
			
			memcpy (dest, source, requiredSize);
		}
		
		static const u8 *	Fetch				(T & o_value, const u8 * i_iterator, PayloadRef i_payload, u8 unused = 0)
		{
			size_t binSize = Jd::Decode7bRE <size_t> (i_iterator, i_payload.end);
			
//			cout << "diff: " << end - (i_payload + binSize) << endl;
			
			auto next = i_iterator + binSize;

			if (next <= i_payload.end)
			{
				o_value.Set ((void *) i_iterator, binSize);
				return next;
			}
			else return nullptr;
		}
		
		static bool			IsType				(u8 i_type, PayloadRef)
		{
			return (i_type == GetTypeId ());
		}
		
		
//		typedef	T compare_t;
//		typedef T store_t;
	};
	
	
	struct StringType
	{
		static bool			IsType				(u8 i_type, PayloadRef)
		{
			return (i_type == c_jdTypeId::string);
		}
		
		
		static constexpr u8	GetTypeId () { return Jd::TypeId <std::string> (); }
		
		template <typename T>
		static u32 GetItemCount (const T & i_value) { return 1; }
		

		static void StoreHeader (allocator_t &) {}
		static const u8 * FetchHeader (u8 *, PayloadRef i_data) { return i_data.start; }
	
		static void StoreItem (allocator_t & i_allocator, const std::string & i_string)
		{
			StoreString (i_allocator, i_string.c_str(), i_string.length ());
		}

		static void StoreItem (allocator_t & i_allocator, const char * const & i_string)
		{
			StoreString (i_allocator, i_string, strlen (i_string));
		}

		static void StoreString (allocator_t & i_allocator, const char * const i_string, size_t i_length)
		{
			++i_length;
			u8 * ptr = i_allocator.Allocate (i_length);
			memcpy (ptr, i_string, i_length);
		}
		
		static const u8 * Fetch (std::string & o_value, const u8 * i_iterator, PayloadRef i_payload, u8 unused = 0)
		{
			// FIX!! buffer overun. check for missing null-termination
			
			o_value = (char *) i_iterator;
			i_iterator += o_value.size() + 1;
			
			return i_iterator;
		}
		
		static const u8 * Fetch (const char * &o_value, const u8 * i_iterator, PayloadRef i_payload, u8 unused = 0)
		{
			// FIX!!: buffer overun. check for missing null-termination
			
			o_value = (char *) i_iterator;
			i_iterator += strlen (o_value) + 1;
			return i_iterator;
		}

		struct StringCompare
		{
			StringCompare () {}
			StringCompare (cstr_t i_value) : value (i_value) {}
			
			cstr_t	value;
			
			bool operator == (cstr_t i_string)				{ return (strcmp (i_string, value) == 0); }
			bool operator == (const std::string & i_string)		{ return (i_string == value); }
		};
		
		static const u8 * Fetch (StringCompare & o_value, const u8 * i_iterator, PayloadRef i_payload, u8 unused = 0)
		{
			// FIX!! buffer overun! check for missing null-termination
			
			o_value.value = (cstr_t) i_iterator;
			i_iterator += strlen (o_value.value) + 1;

			return i_iterator;
		}

		typedef StringCompare		compare_t;
		typedef std::string			store_t;
	};
	
	
	template <typename CastFrom>
	struct EpigramType
	{
		static bool			IsType				(u8 i_type, PayloadRef)
		{
			return (i_type == c_jdTypeId::epigram);
		}
		
		static void StoreHeader (allocator_t &) { }
		
//		template <typename T>
		static void StoreItem (allocator_t & i_allocator, const CastFrom & i_value)
		{
			auto epigram = i_value.GetPayload ();

			Jd::_7bRE es (epigram.size);
			
			u8 * ptr = i_allocator.Allocate (epigram.size + es.numBytes);
			es.Copy (ptr);

			memcpy (ptr, epigram.data, epigram.size);
		}
		
		static const u8 * FetchHeader (u8 *, PayloadRef i_data) { return i_data.start; }
		
		template <typename E>
		static const u8 * Fetch (E & o_epigram, const u8 * i_iterator, PayloadRef i_payload, u8)
		{
			size_t epigramSize = Jd::Decode7bRE <size_t> (i_iterator, i_payload.end);

			auto next = i_iterator + epigramSize;
			
			if (epigramSize and next <= i_payload.end)
			{
				o_epigram.Load (i_iterator, epigramSize);
			}
			
			return next;
		}
	};


	
	template <typename T>
	struct EnumT
	{
		static void StoreHeader (allocator_t & i_allocator)
		{
			cstr_t className = Jd::ParseClassName <T> ();
			u32 classNameLength = (u32) * (className-1) + 1;
			u8 * ptr = i_allocator.Allocate (classNameLength);
			memcpy (ptr, className, classNameLength);
		}
		
		static const u8 *	FetchHeader			(u8 *, PayloadRef i_data)
		{
			return FetchHeaderClassName<T> (i_data.start, i_data.end);
		}

		
		static void StoreItem (allocator_t & i_allocator, const T & i_enum)
		{
			u64 value = (u64) i_enum;
			
			Jd::_7bRE e (value);
			
			u8 * ptr = i_allocator.Allocate (e.numBytes);
			e.Copy (ptr);
		}
		
		static const u8 *	Fetch				(T & o_value, const u8 * i_iterator, PayloadRef i_payload, u8 unused = 0)
		{
			size_t enumeration = Jd::Decode7bRE <size_t> (i_iterator, i_payload.end);

			o_value =  (T) enumeration;
			
			return i_iterator;
		}

		
		static bool			IsType				(u8 i_type, PayloadRef i_data)
		{
			if (i_type == Jd::TypeId <T> ())
			{
				return (FetchHeader (i_data) != nullptr);
			}
			else return false;
		}
	};
	
	template <typename T>
	struct ObjectT
	{
		typedef T store_t;
		
		// [ class-name-with-null ] (opt: version-# ) { reapeat: [ object-data ] [ object-size ] }
		
		static void StoreHeader (allocator_t & i_allocator)
		{
			cstr_t className = T::ClassName ();
			size_t classNameLength = (u32) * (className-1) + 1 /* null */;	// secret count
			
			size_t headerLength = classNameLength + ((T::ClassVersion () > 0) ? 1 : 0);
			
			u8 * ptr = i_allocator.Allocate (headerLength);
			
			memcpy (ptr, className, classNameLength);
			ptr += classNameLength;
			
			if (T::ClassVersion ())
				* ptr = T::ClassVersion ();
		}
		
		static void StoreItem (allocator_t & i_allocator, const T & i_object)
		{
			size_t payloadSize = const_cast <T*> (&i_object)->Serialize (i_allocator);
			
			Jd::_7bRE os (payloadSize);
			u8 * ptr = i_allocator.Allocate (os.numBytes);
			os.CopyFlipped (ptr);
		}
		
		
		static const u8 * FetchHeader (u8 * o_objectVersion, PayloadRef i_data)
		{
			const u8 * ptr = i_data.start;
			
			cstr_t className = T::ClassName();
			u32 classNameLength = *(className - 1) + 1 /*null*/; // get hidden name length
			
			for (u32 i = 0; i < classNameLength; ++i)
			{
				if (*className != *ptr)
					return nullptr;
				++className; ++ptr;
			}
			
			if (T::ClassVersion ())
			{
				if (ptr < i_data.end)
				{
					u8 objectVersion = *ptr++;
					if (objectVersion > T::ClassVersion ())
					{
						d_jdThrow ("object version mismatch. object: @ class: @", (u32) * o_objectVersion, (u32) T::ClassVersion ());
						return nullptr;		// this should maybe throw a VersionMismatch ()?
					}
					else * o_objectVersion = objectVersion;
				}
				else
				{
					d_jdThrow ("object version mismatch/underrun. object: @ class: @", (u32) * o_objectVersion, (u32) T::ClassVersion ());
					return nullptr;
				}
			}

			// since the object size is at the end, gotta run from the bottom to top to find the object. not awesome, but fine fore me now.
			// an alternative would be to make all fetch code non-static. so that FetchHeader could build a deque of object sizes that's popped in Fetch.
			
			if (ptr < i_data.end)
			{
				ptr = i_data.end - 1; // start at last object
			}
				
			return ptr;
		}
		
		// objects fetch in reverse
		static const u8 * Fetch (T & o_object, const u8 * i_iterator, PayloadRef i_payload, u8 i_objectVersion)
		{
			auto end = i_payload.start-1;
			
			size_t objectSize = Jd::ReverseDecode7bRE <size_t> (i_iterator, end);
			i_iterator -= objectSize;
			
			if (objectSize)
				o_object.Deserialize ((voidptr_t) (i_iterator + 1), objectSize, i_objectVersion);
			
			return i_iterator;
		}
		
		static bool			IsType				(u8 i_type, PayloadRef i_data)
		{
			if (i_type == Jd::TypeId <T> ())
			{
				u8 objVersion;
				return (FetchHeader (&objVersion, i_data) != nullptr);
			}
			else return false;
		}
	};

	
	template <typename T>
	struct IntrinsicPodT
	{
		struct SizeOfNothing	{ static constexpr size_t sizeOfT () { return 0; }};	// cause c++ sizeof on an empty struct still equals 1
		struct SizeOfSomething	{ static constexpr size_t sizeOfT () { return sizeof (T); }};
		
		type_if <std::is_empty <T>::value,	SizeOfNothing,		 SizeOfSomething>::type			sizer;

		static constexpr u8	GetTypeId () { return Jd::TypeId <T> (); }
		
		static void StoreHeader (allocator_t & i_allocator) {}
		
		
		static void StoreItem (allocator_t & i_allocator, const T & i_object)
		{
			size_t payloadSize = sizer::sizeOfT ();
			auto ptr = i_allocator.Allocate (payloadSize);
			new (ptr) T (i_object);
		}
		
		static const u8 * FetchHeader (u8 * /* no objectVersion */, PayloadRef i_data)
		{
			return i_data.start;
		}
		
		static bool			IsType				(u8 i_type, PayloadRef i_data)
		{
			return (i_type == Jd::TypeId <T> ());
		}
		
		static const u8 * Fetch (T & o_object, const u8 * i_iterator, PayloadRef i_payload, u8 i_objectVersion = 0)
		{
			auto next = i_iterator + sizer::sizeOfT ();
		
			if (next <= i_payload.end)
			{
				auto object = (const T *) i_iterator;
				memcpy (&o_object, object, sizer::sizeOfT ());
				return next;
			}
			else
			{
				d_jdThrow ("object size overrun/mismatch");
				return nullptr;
			}
		}
		
		typedef T			compare_t;
		typedef T			store_t;
	};
	
	
	template <typename T>
	struct PodT
	{
		// [ 8+: class-name-with-null ] [ 7r: sizeof (pod) ]
		
		static constexpr u8	GetTypeId () { return c_jdTypeId::pod;  } // c_jdTypeId::pod;

		static void StoreHeader (allocator_t & i_allocator)
		{
			cstr_t className = Jd::ParseClassName <T> ();
			u32 classNameLength = (u32) * (className-1) + 1;	// secret count
			
			size_t podSize = sizeof (T);
			Jd::_7bRE ps (podSize);

			u32 headerLength = classNameLength + ps.numBytes;
			u8 * ptr = i_allocator.Allocate (headerLength);
			
			memcpy (ptr, className, classNameLength);
			ptr += classNameLength;
			
			ps.Copy (ptr);
		}
		
		static void StoreItem (allocator_t & i_allocator, const T & i_object)
		{
			size_t payloadSize = sizeof (T);
			auto ptr = i_allocator.Allocate (payloadSize);
			new (ptr) T (i_object);
		}
		
		
		static const u8 * FetchHeader (u8 * /* no objectVersion */, PayloadRef i_data)
		{
			auto ptr = FetchHeader (i_data);
			if (not ptr)
				d_jdThrow ("epigram: class extraction mismatch");
			
			return ptr;
		}

		static const u8 * FetchHeader (PayloadRef i_data)
		{
			const u8 * ptr = (u8 *) i_data.start;
			
			ptr = FetchHeaderClassName <T> (ptr, i_data.end);
			
			if (ptr)
			{
				size_t podSize = Jd::Decode7bRE <size_t> (ptr, i_data.end);
				if (podSize != sizeof (T))
					ptr = nullptr;
			}

			return ptr;
		}


		static bool			IsType				(u8 i_type, PayloadRef i_data)
		{
			if (i_type == Jd::TypeId <T> ())
			{
				return (FetchHeader (i_data) != nullptr);
			}
			else return false;
		}

		
		static const u8 * Fetch (T & o_object, const u8 * i_iterator, PayloadRef i_payload, u8 i_objectVersion = 0)
		{
			auto next = i_iterator + sizeof (T);
			
			if (next <= i_payload.end)
			{
				auto object = (const T *) i_iterator;
				memcpy (&o_object, object, sizeof (T));
				return next;
			}
			else
			{
				d_jdThrow ("object size overrun/mismatch");
				return nullptr;
			}
		}

		typedef T			compare_t;
		typedef T			store_t;
	};

	
	template <typename T, typename P>
	struct ObjPointerT
	{
		static void StoreHeader (allocator_t &i_allocator)
		{
			cstr_t className = Jd::ParseClassName <T> ();
			u32 classNameLength = (u32) * (className-1);	// secret count
			
			u32 headerLength = classNameLength + 1 /* null  */;
			
			u8 * ptr = i_allocator.Allocate (headerLength);
			memcpy (ptr, className, headerLength);
		}
		
		static void StoreItem (allocator_t & i_allocator, const P & i_value)
		{
			u32 requiredSize = sizeof (P);
			u8 * data = i_allocator.Allocate (requiredSize);
			memcpy (data, & i_value, requiredSize);
		}
		
		static const u8 * FetchHeader (u8 * o_objectVersion, PayloadRef i_data)
		{
			const u8 * ptr = i_data.start;
			
			cstr_t className = Jd::ParseClassName <T>();
			u32 classNameLength = * (className - 1) + 1 /*null*/; // get hidden name length
			
			for (u32 i = 0; i < classNameLength; ++i)
			{
				if (*className != *ptr)
					return nullptr;
				++className; ++ptr;
			}
			
			ptr = i_data.start + classNameLength;
			return ptr;
		}
		
		static const u8 * Fetch (P &o_value, const u8 * i_iterator, PayloadRef i_payload, u8)
		{
			auto next = i_iterator + sizeof (P);
			
			if (next <= i_payload.end)
			{
				memcpy (&o_value, i_iterator, sizeof (P));
				return next;
			}
			else
			{
				d_jdThrow ("epigram underrun");
				return nullptr;
			}
		}
	};

	
	//------------------------------------------------------------------------------------------------------------------------------------------------------
	
	template <typename R>
	struct TypeT
	{
		protected:
		type_def std::remove_pointer <R>::type						Rnp;
		
		type_if <std::is_class <R>::value,							PodT <R>,						FundamentalT <R>>::type			storerA;
		
		type_if <std::is_pointer <R>::value 
					and std::is_class <Rnp>::value,					ObjPointerT <Rnp, R>,			storerA>::type					storerP;
		
		type_if <std::is_same <std::string, R>::value,				StringType,						storerP>::type					storerB;
		type_if <jd::is_cstring <R>::value,							StringType,						storerB>::type					storerC;
		
		type_if <std::is_base_of <IIEpigram, R>::value,				EpigramType <R>,				storerC>::type					storerD;

		type_if <std::is_base_of <Jd::Typed, R>::value,				IntrinsicPodT <R>,				storerD>::type					storerE;

		type_if <std::is_base_of <versionedObject_t, R>::value,		ObjectT <R>,					storerE>::type					storerF;
		type_if <std::is_base_of <unversionedObject_t, R>::value,	ObjectT <R>,					storerF>::type					storerG;
//		type_if <std::is_base_of <unversionedObject_t, R>::value,		UnversionedObjectT <R>,			storerF>::type					storerG;
		type_if <std::is_base_of <string_t, R>::value,				StringType,						storerG>::type					storerH;

		type_if <std::is_enum <R>::value,							EnumT <R>,						storerH>::type					storerI;
		type_if <std::is_base_of <binary_t, R>::value,				BinaryT <R>,					storerI>::type					storerJ;

		type_if <jd::is_mappish <R>::value,							EpigramType <EpigramT>,			storerJ>::type					storerX;
		
		public: typedef storerX	type;
	};
	

	template <typename C, typename T>
	struct ContainerStorer
	{
		static size_t GetItemCount (const C & i_container) { return i_container.size (); }
		static u8 GetTypeId () { return Jd::TypeId <R> (); }
		
		static u8 Store (allocator_t & i_allocator, const C & i_value, size_t i_count)
		{
			Jd::_7bRE count (i_count);
			u8 * ptr = i_allocator.Allocate (count.numBytes);
			count.Copy (ptr);

			storer_t::StoreHeader (i_allocator);
			
			for (auto & i : i_value)
				storer_t::StoreItem (i_allocator, i);
	
			return c_jdTypeId::isArray;
		}
		
		type_def C::value_type R;

		type_if <std::is_same <C, T>::value,		R,		T>::type					store_t;
		
		type_def TypeT <store_t>::type storer_t;
	};
	
	
	
	template <typename T>
	struct ItemStorer
	{
		type_def TypeT <T>::type	storer_t;
		
		static u8		GetTypeId		()					{ return Jd::TypeId <T> (); }
		static size_t	GetItemCount	(const T &)			{ return 1; }
		
		static u8		Store			(allocator_t & i_allocator, const T & i_value, size_t /* i_count */)
		{
			storer_t::StoreHeader	(i_allocator);
			storer_t::StoreItem		(i_allocator, i_value);
			
			return 0; // not an array
		}
	};

	
	template <typename T>
	struct MapStorer
	{
		type_def TypeT <T>::type	storer_t;		// maps map to an Epigram; maybe could just be explicit here.
		
		static u8		GetTypeId		()					{ return c_jdTypeId::epigram; }
		static size_t	GetItemCount	(const T &)			{ return 1; }
		
		static u8		Store			(allocator_t & i_allocator, const T & i_value, size_t /* i_count */)
		{
			storer_t::StoreHeader	(i_allocator);
			storer_t::StoreItem		(i_allocator, i_value);
			
			return 0; // not an array
		}
	};
	
	template <typename R>
	struct ArrayStorer
	{
		type_def TypeT <R>::type storer_t;
		
		static u8 GetTypeId () { return Jd::TypeId <R> (); }
		
		template <typename T, size_t t_length>
		static size_t GetItemCount (const T (&) [t_length])
		{
			return t_length;
		}
		
		template <typename T>
		static u8 Store (allocator_t & i_allocator, const T & i_value, size_t i_count)
		{
			Jd::_7bRE count (i_count);
			u8 * ptr = i_allocator.Allocate (count.numBytes);
			count.Copy (ptr);
			
			storer_t::StoreHeader		(i_allocator);
			
			for (u32 i = 0; i < i_count; ++i)
				storer_t::StoreItem	(i_allocator, i_value [i]);
			
			return c_jdTypeId::isArray;
		}
	};
	
	
	
	template <typename T, typename IT = T> // IT = inner type.  allows the store to cast container contents to pre-defined attribute types (or simply enforce & fail)
	class StorerType
	{
		type_def std::remove_extent <T>::type R;
		
		type_if <jd::has_iterator	<T>::value,		ContainerStorer <T, IT>,	ItemStorer <T>	>::type		A;
		type_if <std::is_array		<T>::value,		ArrayStorer <R>,			A				>::type		B;
		type_if <jd::is_cstring		<T>::value,		ItemStorer <cstr_t>,		B				>::type		C;
		type_if <jd::is_mappish		<T>::value,		MapStorer <T>,				C				>::type		D;
		
		public:
		
		typedef D type;
	};
	
	
	
	/// Fetchers -------------------------------------------------------------------------------------------------------------------------------
	
	template <typename T>
	struct CastingFetcher
	{
		template <typename S>
		static const u8 * FetchAndCast (T & o_value, const u8 * i_iterator, PayloadRef i_payload, u8 i_objectVersion)
		{
			type_def TypeT <S>::type fetcher_t;
			type_def EpigramCast <T, S>::type caster_t;
			
			S value;
			const u8 * next = fetcher_t::Fetch (value, i_iterator, i_payload, i_objectVersion);
			o_value = caster_t::Cast (value);
			
			return next;
		}
		
		static const u8 * FetchNoCast (T & o_value, const u8 * i_iterator, PayloadRef i_payload, u8 i_objectVersion)
		{
			type_def TypeT <T>::type fetcher_t;
			return fetcher_t::Fetch (o_value, i_iterator, i_payload, i_objectVersion);
		}
		
		static const u8 * NullFetch (T & o_value, const u8 * i_iterator, PayloadRef i_payload, u8)
		{
			o_value = EpigramDefaultValue <T> ();
			return nullptr;
		}
		
		typedef const u8* (* fetchAndCast_t) (T&, const u8*, PayloadRef, u8);

		struct Caster
		{
			Caster ()
			{
				for (u32 i = 0; i < Jd::SizeOfArray (casters); ++i)
					casters [i] = NullFetch;
				
				casters [c_jdTypeId::f32] = FetchAndCast <f32>;
				casters [c_jdTypeId::f64] = FetchAndCast <f64>;
				casters [c_jdTypeId::i8] = FetchAndCast <i8>;
				casters [c_jdTypeId::u8] = FetchAndCast <u8>;
				casters [c_jdTypeId::i16] = FetchAndCast <i16>;
				casters [c_jdTypeId::u16] = FetchAndCast <u16>;
				casters [c_jdTypeId::i32] = FetchAndCast <i32>;
				casters [c_jdTypeId::u32] = FetchAndCast <u32>;
				casters [c_jdTypeId::i64] = FetchAndCast <i64>;
				casters [c_jdTypeId::u64] = FetchAndCast <u64>;
//				casters [c_jdTypeId::enumeration] = FetchAndCast ... 
				casters [c_jdTypeId::boolean] = FetchAndCast <bool>;
				casters [c_jdTypeId::string] = FetchAndCast <std::string>;
			}
			
			fetchAndCast_t		casters		[32] = {};
		};
		
		
		inline const u8 * FetchHeader (u8 i_type, PayloadRef i_payload)
		{
			static Caster c;
			
			type_def TypeT <T>::type fetcher_t;
			
			if (i_type == Jd::TypeId <T> ())
			{
				m_payload = fetcher_t::FetchHeader (& m_objectVersion, i_payload);
				m_fetchAndCaster = FetchNoCast;
				return m_payload;
//				return (m_payload < i_payload.end);
			}
			else if (i_type != c_jdTypeId::unknown)
			{
				m_payload = i_payload.start;
				m_fetchAndCaster = c.casters [i_type];
				return m_payload;
			}
			else return nullptr;
		}

		
//		bool FetchHeader (const EpigramKV * i_data)
//		{
//			return FetchHeader (i_data->valueType, { i_data->payload, i_data->endPayload });
//		}

		
		bool FetchHeaderNoCast (u8 i_type, PayloadRef i_payload)		// used by PointerFetcher
		{
			type_def TypeT <T>::type fetcher_t;
			
			m_payload = i_payload.start;
			
			if (i_type == Jd::TypeId <T> ())
			{
				m_payload = fetcher_t::FetchHeader (& m_objectVersion, { m_payload, i_payload.end });
				m_fetchAndCaster = FetchNoCast;
				return true;
			}
			else return false; //m_fetchAndCaster = NullFetch;
		}
		
		inline bool Fetch (T & o_value, PayloadRef i_payload)
		{
			m_payload = m_fetchAndCaster (o_value, m_payload, i_payload, m_objectVersion);
			
			return (m_payload >= i_payload.start and m_payload < i_payload.end);
		}

//		bool Fetch (T & o_value, const EpigramKV * i_data)
//		{
//			return Fetch (o_value, { i_data->payload, i_data->endPayload });
//		}
		
		type_def TypeT <T>::type							fetcher_t;

		
		fetchAndCast_t										m_fetchAndCaster;
		const u8 *											m_payload;
		u8													m_objectVersion;
	};

	
	struct RawFetcher
	{
		struct RawValue
		{
			Payload			data;
			
			cstr_t			className;
			u8				version;
		};


		static RawValue Fetch (u8 i_typeId, PayloadRef i_data, size_t i_index)
		{
			RawValue raw = {};
			
			if (i_typeId != c_jdTypeId::unknown)
			{
				bool isObject = (i_typeId == c_jdTypeId::pod or i_typeId == c_jdTypeId::object or i_typeId == c_jdTypeId::versionedObject);
				bool isEnum = i_typeId == c_jdTypeId::enumeration;
				bool canExtract = (isObject or i_typeId == c_jdTypeId::epigram or isEnum);
				d_jdAssert (canExtract, "only extracts objects & epigrams");
				
				if (canExtract)
				{
					size_t objSize = 0;

					const u8 * ptr = (u8 *) i_data.start;
					
					if (isObject or isEnum)
					{
						auto className = ptr;
						
						while (ptr < i_data.end)
						{
							if (*ptr == 0)
							{
								raw.className = (cstr_t) className;
								++ptr;
								break;
							}
							
							++ptr;
						}
					}
					else raw.className = "Epigram";
						
					if (raw.className)
					{
						if (i_typeId == c_jdTypeId::pod)
						{
							objSize = Jd::Decode7bRE <size_t> (ptr, i_data.end);
						}
						else if (i_typeId == c_jdTypeId::versionedObject)
						{
							raw.version = *ptr++;
						}
						
						if (i_typeId == c_jdTypeId::epigram)
						{
							while (ptr < i_data.end)
							{
								objSize = Jd::Decode7bRE <size_t> (ptr, i_data.end);
								
								if (i_index == 0)
								{
									raw.data.start = ptr;
									raw.data.end = ptr + objSize;
									
									break;
								}
								
								ptr += objSize;
								--i_index;
							}
							
						}
						else if (i_typeId == c_jdTypeId::enumeration)
						{
							while (ptr < i_data.end)
							{
								auto start = ptr;
								Jd::Decode7bRE <size_t> (ptr, i_data.end);
								
								if (i_index == 0)
								{
									raw.data.start = start;
									raw.data.end = ptr;
									break;
								}
								
								--i_index;
							}
						}
						else if (i_typeId == c_jdTypeId::pod)
						{
							size_t offset = objSize * i_index;
							
							ptr += offset;
							
							if (ptr + objSize <= i_data.end)
							{
								raw.data.start = ptr;
								raw.data.end = ptr + objSize;
							}
						}
						else
						{
							auto end = ptr - 1;
							ptr = i_data.end - 1;

							while (ptr > end)
							{
								objSize = Jd::ReverseDecode7bRE <size_t> (ptr, end);
								ptr -= objSize;
								
								if (i_index == 0)
								{
									auto * start = ptr + 1;
									if (start > end)
									{
										raw.data.start = start;
										raw.data.end = start + objSize;
									}
									
									break;
								}
								
								--i_index;
							}
						}
					}
				}
			}
			
			return raw;
		}
	};

	
	template <typename T>
	struct ItemFetcher
	{
		typedef CastingFetcher <T> fetcher_t;
		
		static void Fetch (T & o_value, const KeyValue * i_data)
		{
			fetcher_t fetcher;
			
			Payload pl { i_data->payload, i_data->endPayload };
			
			if (fetcher.FetchHeader (i_data->valueType, pl))
				fetcher.Fetch (o_value, pl);
		}
	};

	template <typename T>
	struct KeyFetcher
	{
		typedef CastingFetcher <T> fetcher_t;
		
		static void Fetch (T & o_value, const KeyValue * i_data)
		{
			fetcher_t fetcher;
			
			Payload pl { i_data->keyPayload, i_data->end };			// FIX: this is overruning actual keypayload!
			
			if (fetcher.FetchHeader (i_data->keyType, pl))
				fetcher.Fetch (o_value, pl);
		}
	};
	
	template <typename T>
	struct PointerFetcher
	{
		typedef CastingFetcher <T> fetcher_t;
		
		static void Fetch (T & o_value, const KeyValue * i_data)
		{
			fetcher_t fetcher;

			Payload pl { i_data->payload, i_data->endPayload };
			
			if (fetcher.FetchHeaderNoCast (i_data->valueType, pl))
				fetcher.Fetch (o_value, pl);
		}
	};
	
	
	template <typename T>
	struct ContainerFetcher
	{
		type_def T::value_type R;
		typedef CastingFetcher <R> fetcher_t;


		static void FetchReverse (fetcher_t * i_fetcher, T & o_container, PayloadRef i_payload, size_t i_count)
		{
			auto i = o_container.rbegin ();
			
			while (i_count--)
			{
				R value;
				i_fetcher->Fetch (value, i_payload);
				
				* i = value;
				++i;
			}
		}

		
		static void FetchForward (fetcher_t * i_fetcher, T & o_container, PayloadRef i_payload, size_t i_count)
		{
			auto i = o_container.begin ();
			
			while (i_count--)
			{
				R value;
				i_fetcher->Fetch (value, i_payload);
				
				* i = value;
				++i;
			}
		}
		
		
		static void Fetch (T & o_container, const KeyValue * i_data)
		{
			type_if <std::is_base_of <versionedObject_t, R>::value,			std::true_type,						std::false_type>::type			t1;
			type_if <std::is_base_of <unversionedObject_t, R>::value,		std::true_type,						t1>::type					t2;
			
			fetcher_t fetcher;
			
			// TODO: only works with containers that implement: resize, begin & rbegin.
			o_container.resize (i_data->count);
			
			Payload pl { i_data->payload, i_data->endPayload };
			
			pl.start = fetcher.FetchHeader (i_data->valueType, pl);
			
			if (pl.start) // adjust start past header (class info)
			{
				if (t2::value)
					FetchReverse (& fetcher, o_container, pl, i_data->count);
				else
					FetchForward (& fetcher, o_container, pl, i_data->count);
			}
		}
	};

	
	public:
	
	template <typename K>
	struct KVBase : public KeyValue
	{
		typedef K KEY;
		type_def TypeT <K>::type key_t;

		
		KVBase											(EpigramT * i_epigram, const K & i_key)
		:
		KeyValue										(i_epigram),
		key												(i_key)
		{ }

		const K &				key;
	};
	
	
	template <typename K>
	struct KVAny : public KVBase <K>
	{
		KVAny											(EpigramT * i_epigram, const K & i_key)
		:
		KVBase <K>	(i_epigram, i_key)
		{ }
		
		template <typename T>
		operator				T 						() const
		{
			T value = EpigramDefaultValue <T> ();
			CastedFetch (value);
			return value;
		}

		// TODO: implement. this would be useful		e1 ["whatever"] = e2 [key]
//		KVAny & 				operator =				(const KVAny & i_kv)
//		{
//			jd::out ("KVAny::op=");
//
//			return * this;
//		}

		protected:
		
		template <typename KS, typename T>
		void					Set						(const T & i_value)
		{
			this->epigram->In ((KS) this->key, i_value);
		}

		
		template <typename T>
		void CastedFetch								(T & o_value) const
		{
			type_if <jd::has_iterator <T>::value,		ContainerFetcher <T>,	ItemFetcher <T>>::type		fetcher_a;
			type_if <std::is_pointer <T>::value,		PointerFetcher <T>,		fetcher_a>::type			fetcher_t;
			
			fetcher_t::Fetch (o_value, this);
		}
	};

	
	
	template <typename K, typename V>
	struct KVSet : public KVBase <K>
	{
		KVSet											(EpigramT * i_epigram, const K & i_key)
		:
		KVBase <K>	(i_epigram, i_key)
		{ }

		operator				V 						() const
		{
			V value = EpigramDefaultValue <V> ();
			CastedFetch (value);						// FIX: don't cast?
			return value;
		}

		V						Value					() const
		{
			V value = EpigramDefaultValue <V> ();
			CastedFetch (value);						// FIX: don't cast?
			return value;
		}
		
		protected: //--------------------------------------------------------------------------------
		
		template <typename KS>
		void					Set						(const V & i_value)
		{
			this->epigram->In ((KS) this->key, i_value);
		}

		template <typename KS, typename T>
		void					Set						(const T & i_value)
		{
			this->epigram->template In <KS, T, V> ((KS) this->key, i_value);
		}
		
		
		template <typename T>
		void CastedFetch								(T & o_value) const
		{
			type_if <jd::has_iterator <T>::value,		ContainerFetcher <T>,	ItemFetcher <T>>::type		fetcher_a;
			type_if <std::is_pointer <T>::value,		PointerFetcher <T>,		fetcher_a>::type			fetcher_t;
			
			fetcher_t::Fetch (o_value, this);
		}
	};
	
	template <typename I>
	struct KVT : public I
	{
		public: //-----------------------------------------------------------------------------------------------------------------------------
		
		
		KVT				(EpigramT * i_epigram, const typename I::KEY & i_key)
		:
		I	(i_epigram, i_key)	{ }
		
		
		type_def TypeT <typename I::KEY>::type key_t;

		template <typename T>
		KVT &			operator =				(const T & i_value)
		{
			if (this->IsSet ())
				this->epigram->EraseItem (this);
			
			this->template Set <typename key_t::store_t> (i_value);
			
			return * this;
		}
			
		template <typename T>
		T						To								() const
		{
			T value = EpigramDefaultValue <T> ();
			this->CastedFetch (value);
			
			return value;
		}

		template <typename T>
		T						As								() const
		{
			return To <T> ();
		}
		
		template <typename T>
		T						as								() const { return To <T> (); }
		
		// Raw gets the direct pointer to the element inside the epigram.  This is primary for referencing elements during
		// SQL binding.  Obviously these pointers are fragile and only persistent as long as the Epigrm remains untouched.
		template <typename T>
		T *						unsafePointer					() const
		{
			if (Is <T> ())
			{
				return (T *) this->payload;
			}
			else return nullptr;
		}

		std::pair <voidptr_t, voidptr_t>  unsafePayload				() const
		{
			return { this->payload, this->endPayload };
		}

		size_t					Count					() const					{ return this->count; }

		bool					IsSet					() const					{ return this->valueType != c_jdTypeId::unknown; }
		bool					isSet					() const					{ return IsSet (); }
		bool					IsNull					() const					{ return not IsSet (); }
		
		u8						GetKeyTypeId			() const					{ return this->keyType; }
		u8						GetValueTypeId			() const					{ return this->valueType; }
		
		bool					IsObject						() const
		{
			return		this->valueType == c_jdTypeId::pod
					or	this->valueType == c_jdTypeId::object
					or	this->valueType == c_jdTypeId::versionedObject;
		}

		bool					HasKeyType						(u8 i_keyTypeId) const
		{
			return (this->keyType == i_keyTypeId);
		}

		template <typename T>
		T						GetKey							() const
		{
			T key = EpigramDefaultValue <T> ();
			KeyFetcher <T>::Fetch (key, this);
			
			return key;
		}

		
		bool					IsType							(u8 i_valueTypeId) const
		{
			return (this->valueType == i_valueTypeId);
		}
		
		char					GetValueTypeChar				() const
		{
			return Jd::TypeIdToChar (this->valueType);
		}
		
		cstr_t					GetClassName					() const
		{
			cstr_t name = c_epigram::nullString;
			
			if (IsObject () or this->valueType == c_jdTypeId::enumeration)
			{
				return (cstr_t) this->payload;
			}
			
			return name;
		}
		
		std::string				GetValueTypeName				() const
		{
			std::string name = Jd::TypeIdToName (this->valueType);
			if (this->count > 1) name += "[]";
			
			return name;
		}
		
		std::string				GetKeyTypeName					() const
		{
			return Jd::TypeIdToFullName (this->keyType);
		}
		
		
		// GetKey <string> () can be used which will then cast other types to a string
		// GetKeyString () is a fast alternative which points to the null terminated string in the epigram structure
		cstr_t					GetKeyString					() const
		{
			cstr_t key = c_epigram::nullString;
			
			if (this->keyType == c_jdTypeId::string)
				key = (cstr_t) this->keyPayload;
			
			return key;
		}
		
		template <typename T>
		void							operator >>			(T &o_value) const
		{
			if (this->IsSet ()) this->CastedFetch (o_value);
		}


		template <typename T>
		bool operator ==			(const T &i_value) const
		{
			type_def TypeT <T>::type value_t;
			
			if (this->Is <T> ())
			{
				typename value_t::store_t ev;
//				this->template CastedFetch (ev);		// FIX: this should probably be an uncasted fetch!
				this-> CastedFetch(ev);		// FIX: this should probably be an uncasted fetch!

				return (ev == i_value);
			}
			else return false;
		}

		
		bool					Erase					()
		{
			if (this->IsSet ())
			{
				this->epigram->EraseItem (this);
				return true;
			}
			else return false;
		}
		
		
		template <typename T>
		bool						Is						() const
		{
			type_def TypeT <T>::type compare_t;
			return compare_t::IsType (this->valueType, { this->payload, this->endPayload });
		}

		template <typename T> bool	is						() const { return Is <T> (); }
		
		typedef typename RawFetcher::RawValue rawobj_t;
		
		rawobj_t GetRawObject (size_t i_index = 0)
		{
			return RawFetcher::Fetch (this->valueType, { this->payload, this->endPayload }, i_index);
		}
		

		protected://--------------------------------------------------------------------------------------------------------------------------
		
		friend class EpigramT;
		
		template <typename II, typename T> friend void operator << (T & o_value, const KVT <II> & i_kv);
	};
	
	
	typedef KVT <KVAny <EpNoType>> EpigramElement;

	public:
	
	// constructors --------------------------------------------------------------------------------------
	
	EpigramT () {}

	EpigramT (cstr_t i_what)
	{
		this->operator () (i_what);
	}

	EpigramT (const std::string & i_what)
	{
		this->operator () (i_what);
	}

	
	EpigramT			(voidptr_t i_data, size_t i_numBytes)
	{
		Load ((const u8 *) i_data, i_numBytes);
	}

	EpigramT			(IEpigramIn i_deliveredMsg)
	{
		this->operator = (i_deliveredMsg);
	}
	
	template <typename A>
	EpigramT			(const EpigramT <A> & i_epigram)
	{
		this->operator = (i_epigram);
	}


	EpigramT			(const EpigramT & i_epigram)
	{
		this->operator = (i_epigram);
	}
	
	template <typename K, typename V, typename Compare, typename Allocator>
	EpigramT 			(std::map <K, V, Compare, Allocator> const & i_map)
	{
		this->operator = (i_map);
	}

	
	// Movers ----------------------------------------------------------------------------------------------------------------

	EpigramT			(EpigramT && i_epigram)
	{
		m_allocator = std::move (i_epigram.m_allocator);
	}

	EpigramT&			operator= (EpigramT && i_epigram)
	{
		m_allocator = std::move (i_epigram.m_allocator);
		return * this;
	}

	template <typename A>
	EpigramT			(EpigramT <A> && i_epigram)
	{
		m_allocator = std::move (i_epigram.m_allocator);
	}

	template <typename A>
	EpigramT&			operator= (EpigramT <A> && i_epigram)
	{
		m_allocator = std::move (i_epigram.m_allocator);
		return * this;
	}

	//----------------------------------------------------------------------------------------------------------------------
	template <typename... _Args>
	EpigramT			(const EpigramT & i_epigram1, _Args ... i_args)
	{
		this->operator = (i_epigram1);
		this->Args (i_args...);
	}

	
	//----------------------------------------------------------------------------------------------------------------------

	template <typename A, typename T>
	EpigramT			(const EpArg <A,T> & i_arg)
	{
		this->In (i_arg.name, i_arg.value);
	}

	template <typename A, typename T, typename... _Args>
	EpigramT			(const EpArg <A,T> & i_arg, _Args ... i_args)
	{
		this->In (i_arg.name, i_arg.value);
		this->Args (i_args...);
	}


	template <typename A, typename T, typename... _Args>
	void Args (const EpArg <A,T> & i_arg)
	{
		this->In (i_arg.name, i_arg.value);
	}
	

	template <typename A, typename T, typename... _Args>
	void Args (const EpArg <A,T> & i_arg, _Args ... i_args)
	{
		this->In (i_arg.name, i_arg.value);
		this->Args (i_args...);
	}
	
	void Args (const EpigramT & i_epigram)
	{
		Merge (i_epigram);
	}
	
	template <typename... _Args>
	void Args (const EpigramT & i_epigram, _Args ... i_args)
	{
		this->Merge (i_epigram);
		this->Args (i_args...);
	}
	
	
	EpigramT &					operator =			(IEpigramIn i_msg)
	{
		if (i_msg)
		{
			auto payload = i_msg->GetPayload ();
			Load (payload.bytes, payload.size);
		}
		else this->Clear ();
		
		return *this;
	}

	template <typename A>
	EpigramT &					operator =			(const EpigramT <A> &i_msg)
	{
		auto payload = i_msg.GetPayload ();
		Load (payload.bytes, payload.size);

		return * this;
	}


	// I don't understand how this is not under the umbrella of the templated version above...
	// but seems this is needed. Likewise with constructor.
	EpigramT &					operator =			(const EpigramT &i_msg)
	{
		auto payload = i_msg.GetPayload ();
		Load (payload.bytes, payload.size);
		
		return * this;
	}
	
	
	template <typename K, typename V, typename Compare, typename Allocator>
	EpigramT &					operator =			(std::map <K, V, Compare, Allocator> const & i_map)
	{
		Clear ();
		
		return operator << (i_map);
	}

	template <typename K, typename V, typename Compare, typename Allocator>
	EpigramT &					operator <<			(std::map <K, V, Compare, Allocator> const & i_map)
	{
		for (auto & i : i_map)
			(* this) [i.first] = i.second;
		
		return * this;
	}

	
	size_t						Count               () const
	{
		size_t count = 0;
		
		auto d = m_allocator.GetBufferRange ();
		
		const u8 * ptr = d.end-1;
		const u8 * end = d.start;
		
		while (ptr >= end)
		{
			auto payloadSize = Jd::ReverseDecode7bRE <size_t> (ptr, end) + c_payloadMinSize;
			ptr -= payloadSize;
			
			++count;
		}
		
		return count;
	}
	
	bool						HasElements			() const
	{
		return not IsEmpty ();
	}

	bool						hasElements			() const { return not IsEmpty (); }


	bool						IsEmpty				() const
	{
		return (m_allocator.GetCapacity () == m_allocator.GetNumFreeBytes ());
	}
	
	
	template <typename T>
	bool						operator ==			(const T &i_value) const
	{
		auto def = Default ();
		
		if (def.template Is <T> ())
		{
			return (def == i_value);
		}
		else return false;
	}

	
	size_t						GetCapacity			() const	{ return m_allocator.GetCapacity (); }
	size_t						GetSize				() const	{ return m_allocator.GetNumUsedBytes (); }
	size_t						size				() const	{ return GetSize (); }
	u32							GetSequence			() const	{ return m_allocator.GetSequence (); }		// starts at 0
	void						Reset				()			{ m_allocator.Reset (); }
	void						Clear				()			{ m_allocator.Clear (); }
	
	void						Load		(const std::string & i_string)
	{
		Load ((const u8 *) i_string.data (), i_string.size ());
	}
	
	void						Load				(const u8 * i_bytes, size_t i_numBytes)
	{
		m_allocator.Clear ();
		u8 * data = m_allocator.Allocate (i_numBytes);
		
		memcpy (data, i_bytes, i_numBytes);
	}

	

	EpigramT &					operator +=			(const KeyValue & i_item)
	{
		if (i_item.valueType != c_jdTypeId::unknown)
		{
			size_t size = i_item.end - i_item.payload;

			// unnecessary data copy here:
			Merge (EpigramT (i_item.payload, size));
		}
		
		return * this;
	}
	
	EpigramT &					operator +=			(const EpigramT & i_message)
	{
		Merge (i_message);
		return * this;
	}
	
	
	void						Compact				()
	{
		// remove duplicate keys
	}

	
	void                        Merge				(const EpigramT & i_merging)
	{
		if (not i_merging.IsEmpty ())
		{
			// find existing/duplicates and erase
			for (auto & i : i_merging)
			{
				u8 keyType = i.GetKeyTypeId ();

				if (keyType == c_jdTypeId::string)
				{
					cstr_t key = i.GetKeyString ();
					Erase (key);
				}
				else d_jdThrow ("fix");
					
				// options: examine raw key. fastest, but only perfectly works for fundamentals and simple POD. probably good enough. having an epigram as the key is kinda fringe usage
				// create FetchAndCompare <f32>... etc. and run through a list.
			}
		}
		
		auto pl = i_merging.GetPayload ();
		
		u8 * dest = m_allocator.Allocate (pl.size);
		memcpy (dest, pl.data, pl.size);
	}

	template <typename... Args>
	void                        Merge				(const EpigramT & i_merging, Args... i_args)
	{
		Merge (i_merging);
		Merge (i_args...);
	}


	template <typename K>
	bool						Erase				(const K & i_key)
	{
		auto kv = FindItem <KVAny <K>> (i_key);
		
		if (kv.IsSet ())
		{
			EraseItem (& kv);
			return true;
		}
		else return false;
	}

	template <typename K, typename... Args>
	void                        Erase				(const K & i_key, Args... i_args)
	{
		Erase (i_key);
		Erase (i_args...);
	}


	operator const IIEpigram * () const 			{ return this; }
	operator IIEpigram * ()							{ return this; }
	
	
	explicit operator std::string () const
	{
		return ToString ();
	}

	std::string			ToString  () const
	{
		auto payload = GetPayload ();
		return std::string ((const char *) payload.bytes, payload.size);
	}

	std::string			toString  () const  { return ToString (); }

	
	IIEpigramIn::Payload GetPayload () const
	{
		return IIEpigramIn::Payload { m_allocator.GetBuffer (), m_allocator.GetNumUsedBytes () };
	}
	
	
	u8 const *			data				() { return m_allocator.GetBuffer (); }
	size_t				size				() { return m_allocator.GetNumUsedBytes (); }

	
	void				Deliver				(IEpigramIn i_epigram)
	{
		auto payload = i_epigram->GetPayload ();

		if (payload.data)
			Load (payload.bytes, payload.size);
	}
	
	void				Deliver				(const void * i_data, size_t i_size)
	{
		Load ((const u8 *) i_data, i_size);
	}

	
	public:

	template <typename K, typename V>
	EpigramT &					InArray				(const K & i_key, const V & i_value, size_t i_arraySize)			// Array
	{
		// FIX: need to refactor to AddElement to GetItemCount() vs. pushing i_arraySize.  For now:
		typedef typename std::remove_pointer <V>::type B;
		
		std::vector <B> vec (i_value, i_value + i_arraySize);
		
		return this->In (i_key, vec);
	}

	template <typename K, typename V>
	EpigramT &					operator ()				(const K & i_key, const V & i_value)			// AddElement InsertElement PushElement ?
	{
		return In (i_key, i_value);
	}
	
	
	template <typename K>
	EpigramT &					AddRawObject			(const K & i_key, u8 i_objectTypeId, cstr_t i_objectName, u8 i_version, voidptr_t i_bytes, size_t i_size)
	{
		type_def StorerType <K>::type key;
		//---------------------------------------------------------------------------------------------
		d_jdAssert (key::GetItemCount (i_key) == 1, "yo, only a singled-value key allowed");
		
		size_t startBytes = m_allocator.GetNumUsedBytes ();
		
		u8 isArrayFlag = 0;
		
		size_t classNameLength = strlen (i_objectName) + 1;
		size_t headerLength = classNameLength;
		if (i_version) ++headerLength;
				
		u8 * ptr = m_allocator.Allocate (headerLength);
		
		memcpy (ptr, i_objectName, classNameLength);
		ptr += classNameLength;

		if (i_version) *ptr++ = i_version;
		
		Jd::_7bRE objSize (i_size);
		
		if (i_objectTypeId == c_jdTypeId::enumeration)
			objSize.numBytes = 0;

		ptr = m_allocator.Allocate (objSize.numBytes + i_size);

		if (i_objectTypeId == c_jdTypeId::pod)
			objSize.Copy (ptr);
		
		memcpy (ptr, i_bytes, i_size);

		if (i_objectTypeId != c_jdTypeId::pod and i_objectTypeId != c_jdTypeId::enumeration)
		{
			ptr += i_size;
			objSize.CopyFlipped (ptr);
		}
		
		size_t valueEnd = m_allocator.GetNumUsedBytes ();
		
		//---------------------------------------------------------------------------------
		
		key::Store (m_allocator, i_key, 1);
		
		size_t usedBytes = m_allocator.GetNumUsedBytes ();
		size_t keySize = usedBytes - valueEnd;
		
		Jd::_7bRE ks (keySize);
		size_t payloadSize = usedBytes - startBytes + ks.numBytes - 1; // ks is at least one
		
		Jd::_7bRE ps (payloadSize);
		
		size_t allocationSize = ks.numBytes + ps.numBytes + 2;
		
		ptr = m_allocator.Allocate (allocationSize);
		
		ks.CopyFlipped (ptr);
		
		u8 isMapFlag = 0 << 5;
		
		*ptr++ = key::GetTypeId () | isMapFlag;
		*ptr++ = i_objectTypeId | isArrayFlag;
		
		ps.CopyFlipped (ptr);
		
		return *this;
	}
	
	protected:
	
	template <typename K, typename V, typename IV = V>
	EpigramT &					In						(const K & i_key, const V & i_value)			// PushElement
	{
		type_def StorerType <K>::type key;
		type_def StorerType <V>::type value;
		//---------------------------------------------------------------------------------------------
		d_jdAssert (key::GetItemCount (i_key) == 1, "yo, only a singled-value key allowed");

		size_t startBytes = m_allocator.GetNumUsedBytes ();
		
		size_t count = value::GetItemCount (i_value);
		u8 isArrayFlag = value::Store (m_allocator, i_value, count);

		size_t valueEnd = m_allocator.GetNumUsedBytes ();
		
		key::Store (m_allocator, i_key, 1);

		size_t usedBytes = m_allocator.GetNumUsedBytes ();
		
		size_t keySize = usedBytes - valueEnd;

		Jd::_7bRE ks (keySize);
		
		size_t payloadSize = usedBytes - startBytes + ks.numBytes - 1; // ks is at least one

		Jd::_7bRE ps (payloadSize);

		size_t allocationSize = ks.numBytes + ps.numBytes + 2;
		
		u8 * ptr = m_allocator.Allocate (allocationSize);
		
		ks.CopyFlipped (ptr);

		u8 isMapFlag = 0 << 5;
		
		*ptr++ = key::GetTypeId () | isMapFlag;
		*ptr++ = value::GetTypeId () | isArrayFlag;

//		cout << std::hex << (int) ps.bytes [0] << endl;
		
		ps.CopyFlipped (ptr);	// payload size doesn't include self or 2 type-info bytes
		
		return *this;
	}

	
	struct EpDefault : Jd::TypedT <c_jdTypeId::none>
	{
		bool operator == (const EpDefault &) { return true; }
	};
	
	public:	

	// default (unnamed) item ------------------------------------------------------------------------------------------------------------
	template <typename T>
	EpigramT &							operator ()			(const T& i_value)
	{
		return operator () (EpDefault (), i_value);
	}

	KVT <KVAny <EpDefault>> 		operator ()			() const
	{
		return operator [] (EpDefault ());
	}	

	KVT <KVAny <EpDefault>> 		Default				() const
	{
		return operator [] (EpDefault ());
	}

	
	KVT <KVAny <EpDefault>> 		operator ()			() 
	{
		return operator [] (EpDefault ());
	}


	class Iterator
	{
		friend class EpigramT;
		
		public:

		EpigramElement & operator * () const
		{
			m_kv.valueType = c_jdTypeId::unknown;
			
			if (m_index >= 0)
			{
				const u8 * ptr = m_elements [m_index];
				const u8 * end = m_allocator.GetBuffer () - 1;
			
				const u8 * decode = ptr;
				
				auto payloadSize = Jd::ReverseDecode7bRE <size_t> (decode, end) + c_payloadMinSize;
				auto next = decode - payloadSize;
				
				u8 valueType = *decode--;
				u8 keyType = *decode--;
				
				keyType &= c_jdTypeId::typeMask;
				
				bool isArray = valueType & c_jdTypeId::isArray;
				valueType &= c_jdTypeId::typeMask;
				
				auto keySize = Jd::ReverseDecode7bRE <size_t> (decode, end);
				
				auto key = decode - keySize + 1;
				
				++next;
				
				if (isArray)
					m_kv.count = Jd::Decode7bRE <size_t> (next, key);
				else
					m_kv.count = 1;
				
				m_kv.sequence = m_allocator.GetSequence ();
				m_kv.keyType = keyType;
				m_kv.valueType = valueType;
				m_kv.payload = const_cast <u8 *> (next);
				m_kv.endPayload = key;
				m_kv.end = ptr + 1;
			}

			return m_kv;
		}
		
		const Iterator & operator++ ()
		{
			if (m_index >= 0)
				--m_index;
			
			return * this;
		}

//		bool operator == (const Iterator & i_other) const
//		{
//			return m_ptr == i_other.m_ptr;
//		}

		bool operator != (const Iterator & i_other) const
		{
			return m_index != i_other.m_index;
		}
		
		protected:
		Iterator	(EpigramT * i_epigram, allocator_t & i_allocator, const u8 * i_pointer)
		:
		m_kv		(i_epigram, EpNoType ()),
		m_ptr		(i_pointer),
		m_allocator	(i_allocator)
		{
			const u8 * end = m_allocator.GetBuffer () -1;
			
			// element sizes are decoded and pointers are pushed to a vector so that the iterator can traverse forward
			while (i_pointer > end)
			{
				m_elements.push_back (i_pointer);
				
				auto payloadSize = Jd::ReverseDecode7bRE <size_t> (i_pointer, end) + c_payloadMinSize;
				i_pointer -= payloadSize;
			}

			m_index = m_elements.size () - 1;
		}

		
		private://--------------------------------------------------------------------------------------------
		
		mutable EpigramElement			m_kv;
		const u8 *						m_ptr;
		allocator_t &					m_allocator;
		
		std::vector <const u8 *>		m_elements;
		ptrdiff_t						m_index;
	};
	
	
	Iterator begin () const
	{
		return Iterator (const_cast <EpigramT *> (this), m_allocator, m_allocator.GetBufferEnd () -1);
	}
	
	Iterator end () const
	{
		return Iterator (const_cast <EpigramT *> (this), m_allocator, m_allocator.GetBuffer () - 1);
	}
	
	//-------------------------------------------------------------------------------------------------------------------------------------------------
	EpigramElement						Index				(size_t i_index) const
	{
		return const_cast <EpigramT *> (this)->_Index (i_index);
	}
	
	EpigramElement						Index				(size_t i_index) // const
	{
		return _Index (i_index);
	}
		
	EpigramElement						_Index				(size_t i_index)
	{

		EpigramElement element (this, EpNoType ());
		
//		size_t count = Count ();
		
		auto d = m_allocator.GetBufferRange ();
		
		const u8 * ptr = d.end-1;
		const u8 * end = d.start-1;
		
		while (ptr > end)
		{
			auto elementEnd = ptr + 1;
			auto payloadSize = Jd::ReverseDecode7bRE <size_t> (ptr, end) + c_payloadMinSize;
			auto next = ptr - payloadSize;
			
			if (i_index == 0)
			{
				u8 valueType = *ptr--;
				u8 keyType = *ptr--;
				
				bool isArray = valueType & c_jdTypeId::isArray;
				valueType &= c_jdTypeId::typeMask;
				
				auto keySize = Jd::ReverseDecode7bRE <size_t> (ptr, end);
				auto key = ptr - keySize + 1;
				
				++next;
				element.start = next;
				
				if (isArray)
					element.count = Jd::Decode7bRE <size_t> (next, key);
				else
					element.count = 1;
				
				element.sequence = m_allocator.GetSequence ();
				element.keyType = keyType;
				element.valueType = valueType;
				element.payload = const_cast <u8 *> (next);
				element.endPayload = key;
				
//				cout << "ps: " << element.endPayload - element.payload << endl;
				
				element.end = elementEnd;
				
				break;
			}
			
			--i_index;
			ptr = next;
		}
		
		return element;
	}
	

	template <typename K>
	KVT <KVAny <K>>				operator []				(const K & i_key) const
	{
		auto _this = const_cast <EpigramT *> (this);
		
		return _this->template FindItem <KVAny <K>> (i_key);
	}

	
	template <typename K>
	KVT <KVAny <K>>				operator []				(const K & i_key)
	{
		return FindItem <KVAny <K>> (i_key);
	}


	template <typename K, typename V>
	KVT <KVSet <K, V>>			operator []				(const EpAttribute <K,V> & i_attribute) //const
	{
		return FindItem <KVSet <K, V>> ((K &) i_attribute);
	}
	
	
	template <typename K>
	bool								Has						(const K & i_key)
	{
		return operator [] (i_key).IsSet ();
	}


	void Dump () const { DumpEpigram (this, m_allocator.GetSequence ()); }
	void dump () const { DumpEpigram (this, m_allocator.GetSequence ()); }

	protected: //--------------------------------------------------------------------------------------------------------------------------------

	template <typename I, typename K>
	KVT <I>						FindItem					(const K & i_key)
	{
		type_def TypeT <K>::type key_t;
		//-------------------------------------------------------------------------------------------------------
		
		KVT <I> element (this, i_key);
		
		auto d = m_allocator.GetBufferRange ();

		const u8 * ptr = d.end-1;
		const u8 * end = d.start-1;
		
		while (ptr > end)
		{
			const u8 * decode = ptr;
			
			auto payloadSize = Jd::ReverseDecode7bRE <size_t> (decode, end) + c_payloadMinSize;
			auto next = decode - payloadSize;
			
			u8 valueType = *decode--;
			u8 keyType = *decode--;
			
			keyType &= c_jdTypeId::typeMask;
			
			if (keyType == key_t::GetTypeId ())
			{
				bool isArray = valueType & c_jdTypeId::isArray;
				valueType &= c_jdTypeId::typeMask;
				
				auto keySize = Jd::ReverseDecode7bRE <size_t> (decode, end);
				
				auto key = decode - keySize + 1;

				typename key_t::compare_t itemKey;
				key_t::Fetch (itemKey, key, { key, key + keySize });
				
				if (itemKey == i_key)
				{
					++next;
					element.start = next;

					if (isArray)
						element.count = Jd::Decode7bRE <size_t> (next, key);
					else
						element.count = 1;

					element.sequence = m_allocator.GetSequence ();
					element.keyType = keyType;
					element.valueType = valueType;
					element.payload = const_cast <u8 *> (next);
					element.endPayload = key;
					
					element.end = ptr + 1;

					break;
				}
			}
			
			ptr = next;
		}
		
		return element;
	}


	void						EraseItem				(KeyValue * i_item)
	{
		d_jdAssert (i_item->sequence == m_allocator.GetSequence (), "epigram has been touched");
		
		// alternative: set key to c_jdTypeId::null, and set needs compaction flag and compact on copy/move/deliver
		
		if (i_item->sequence == m_allocator.GetSequence ())
			m_allocator.EraseRange ((void *) i_item->start, (void *) i_item->end);
	}

	friend struct EpigramDumper;
	

	//public:
//	template <typename T>
//	friend std::ostream & operator << (std::ostream & output, const EpigramItemT <T> &i_epigramItem);

	template <typename D>
	friend void operator << (D & o_value, const EpigramElement &i_epigramItem);
	
	mutable allocator_t				m_allocator;
};

//
//template <typename T>
//std::ostream & operator << (std::ostream & output, const EpigramT<>::EpigramItemT <T> &i_epigramItem)
//{
//	T value = (T) i_epigramItem;
//	output << value;
//	return output;
//}


template <typename I, typename T>
void operator << (T & o_value, const EpigramT<>::KVT <I> & i_kv)
{
//	i_kv.template CastedFetch (o_value);
	i_kv. CastedFetch(o_value);
}


class EpigramNoAllocator
{
	struct BufferRange { u8 * start, * end; };
	
	public:
	u32 GetSequence				() const											{ return 0; }
	u8 * GetBuffer				()													{ return m_start; }
	u8 * GetBufferEnd			() const											{ return m_end; }
	BufferRange GetBufferRange	()													{ return { m_start, m_end }; }
	void Reset					()													{ m_end = m_start; }
	void Clear					()													{ m_end = m_start; }
	void EraseRange				(void * i_start, void * i_end)						{}
	u32 AllocateRange			(u32 i_requiredBytes)								{ d_jdThrow ("can't call Allocate() on a const epigram"); return 0; }
	void SetRange				(u32 i_position, void * i_data, u32 i_numBytes)		{ }
	u8 * Allocate				(size_t i_requiredBytes)							{ d_jdThrow ("can't call Allocate() on a const epigram"); return nullptr; }
	size_t GetCapacity			()													{ return 0; }
	size_t GetNumUsedBytes		()													{ return (m_end - m_start); }
	size_t GetNumFreeBytes		()													{ return 0; }
	bool IsHeapAllocated		() const											{ return true; }
	void SetData				(const void * i_data, size_t i_size)
	{
		m_start = const_cast <u8 *> ((u8 *) i_data);
		m_end = m_start + i_size;
	}
	
	protected:
	u8 *			m_start;
	u8 *			m_end;
};


class ConstEpigram : public EpigramT <EpigramNoAllocator>
{
	public:
	
	ConstEpigram (EpDelivery i_delivered)
	{
		if (i_delivered)
		{
			auto payload = i_delivered->GetPayload ();
			
			m_allocator.SetData (payload.data, payload.size);
		}
	}
	
	ConstEpigram (const u8 * i_data, u32 i_size)
	{
		m_allocator.SetData (i_data, i_size);
	}
};

template <u32 t_size> using SizedEpigramT = EpigramT <EpigramHybridAllocator <t_size>>;

const size_t _baseEpigramSize = sizeof (SizedEpigramT <0>);

typedef SizedEpigramT <c_epigramStackSize> Epigram;

typedef SizedEpigramT <256-_baseEpigramSize> Epigram256;
typedef SizedEpigramT <128-_baseEpigramSize> Epigram128;
typedef SizedEpigramT <0> Epigram0;

typedef SizedEpigramT <0> epigram;
typedef const epigram & epigramRef_t;

typedef const Epigram & EpigramRef;
typedef const Epigram256 & Epigram256Ref;

#define msg_ Epigram()



//
//template <typename A>
//std::ostream & operator << (std::ostream & output, const EpigramT <A> & i_epigram)
//{
//	ostringstream oss;
//	
//	EpigramDumper d (i_epigram, oss);
//	
//	d.PrintArgs ();
//	
//	output << "\nJdModule::PrintArgs (" << oss.str() << ")\n";
//	
////	output << "()";
//	return output;
//}


//#undef type_def
//#undef type_if



