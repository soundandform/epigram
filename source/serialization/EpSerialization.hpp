/*
 How to use:
---------------------------------------------------------------------------------------------------------------------------------------------------------------
 for simple objects using POD members that probably won't be evolving, add to the class definition:

		d_rawSerialize (m_member1, m_member2, m_member3)
 
 The serialize mechanism simply raw memcpy's the variable into the serialized chunk.  It is possible to add new/additional members in new versions of a class. 
 The original ordering just must remain persistant. So new members must be appended to the end of the list. Older versions will simply not parse beyond their 
 required number of bytes -- all is well.  Trying to read older chunks into a newer object will simply abort once the serialized bytes runs out, returning back
 to the serializer extraction process.
 
 To have more control over the in/out process, see the d_serializeIn () and d_serializeOut(version_of_this_class) macros.  To move beyond version 1.x, the
 version number can be specified in the out macro and inspected in the in macro.  Epigram will not attempt a deserialization if the stored object is
 greater than the instantiated object, but will if stored <= instantiated.  Obviously, the SerializeIn must handle older formats by if-switching on the 'version'
 argument.
 
 For more complex objects that have variable content or are likely to evolve, requiring less fragility in the de/serialization process:
 
 		d_msgSerialize (member1, member2, member3)
 
 The msg version expects all member variables to start with m_. The variables are stored in the message using name provided, e.g. 
 		
	msg ["member1"] = m_member1;
 
 Variables are then extracted by name in deserialization process.  So, attention must be paid to renaming variables.  They should be renamed if they represent
 a newer, incompatible entity.
 
*/

#ifndef EpSerialization_h
#define EpSerialization_h

#include "EpSerializationRaw.hpp"
#include "Epigram.hpp"

template <typename allocator_t>
class EpMsgSerializerOut : public Epigram
{
	public:
								EpMsgSerializerOut		(EpSerializer <allocator_t> &i_serializer)
								:
								m_serializer			(i_serializer)
	{
		
	}
								~EpMsgSerializerOut ()
	{
		u32 numBytes = m_allocator.GetNumUsedBytes();
		
		d_jdAssert (numBytes, "empty serialization");
		m_serializer.Store (m_allocator.GetBuffer(), numBytes);
	}
	
	protected:
	EpSerializer <allocator_t>						& m_serializer;
};




//class EpMsgSerializerIn
//{
//	public:
//	EpMsgSerializerIn	(EpSerializerIn &i_serializer)
//	:
//	m_archive			(i_serializer.m_ptr, i_serializer.m_numBytes)
//	{
//	}
//	
//	template <typename T>
//	EpMsgSerializerIn&		operator () (cstr_t i_name, T &o_value)
//	{
//		m_archive [i_name] >> o_value;
//		return *this;
//	}
//	
//	protected:
//	ConstEpigram			m_archive;
//};

/*
#define d_rawSerialize(...) 	public: d_serializeClassName \
								static u8 ClassVersion () { return 0; } \
								void Deserialize (void * i_ptr, u32 i_size, u8 i_version) \
									{ EpSerializerIn in (i_ptr, i_size, i_version); Serializer (in); } \
								template <typename A> u32 Serialize (A &o_allocator) const	\
									{ EpSerializerOut <A> out (o_allocator); Serializer (out); return out.NumBytes (); } \
								template <typename T> void Serializer (T &io) const					{ io (__VA_ARGS__); }
*/


// FIX:!!

#define d_serializeWithMsg() 	public: d_serializeClassName \
								static u8 ClassVersion () { return 1; } \
								void Deserialize (voidptr_t i_ptr, u32 i_size, u8 i_version) \
								{ } \
								template <typename A> u32 Serialize (A &o_allocator) const	\
								{ return 0; } \


//#define d_msgSerialize1Args(_arg0)					d_serializeWithMsg() { io (#_arg0, m_##_arg0) ; }
//#define d_msgSerialize2Args(_arg0,_arg1)			d_serializeWithMsg() { io (#_arg0, m_##_arg0) (#_arg1, m_##_arg1); }
//#define d_msgSerialize3Args(_arg0,_arg1,_arg2)		d_serializeWithMsg() { io (#_arg0, m_##_arg0) (#_arg1, m_##_arg1) (#_arg2, m_##_arg2); }

#define d_msgSerialize1Args(_arg0)					d_serializeWithMsg()
#define d_msgSerialize2Args(_arg0,_arg1)			d_serializeWithMsg()
#define d_msgSerialize3Args(_arg0,_arg1,_arg2)		d_serializeWithMsg()

#define d_msgSerialize4Args(...)
#define d_msgSerialize5Args(...)
#define d_msgSerialize6Args(ARG0,...)

#define d_get8thArg(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ...) ARG8
#define d_get7thArg(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ...) ARG7
#define d_get6thArg(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ...)	ARG6

#define d_msgSerializeChooser(...) d_get6thArg(__VA_ARGS__, d_msgSerialize6Args, d_msgSerialize5Args, d_msgSerialize4Args, d_msgSerialize3Args, d_msgSerialize2Args, d_msgSerialize1Args, )

#define d_msgSerialize(...) d_msgSerializeChooser(__VA_ARGS__)(__VA_ARGS__)


#endif

