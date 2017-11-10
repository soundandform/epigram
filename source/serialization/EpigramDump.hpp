//
//  EpigramDump.hpp
//  Jigidesign
//
//  Created by Steven Massey on 3/26/17.
//  Copyright © 2017 Jigidesign. All rights reserved.
//

#ifndef EpigramDump_hpp
#define EpigramDump_hpp

#include <iomanip>
#include "Epigram.hpp"
#include "EpAttribute.hpp"

//---------------------------------------------------------------------------------------------------------------------------------------------

struct EpigramDumper
{
	//	EpigramDumper					(Epigram & i_epigram, std::ostream & i_stream);
	EpigramDumper						(EpDelivery i_epigram, std::ostream & i_stream = cout);
	EpigramDumper						(const u8 * i_payload, u32 i_size, std::ostream & i_stream = cout);
	
	void	DumpCode					() const;
	void	Dump						(u32 i_sequence) const;
	void	Decompose					(u32 i_depth = 0) const;
	
	void	PrintArgs					() const;
	
	protected:
	
	void	DumpCode					(const u8 * i_start, const u8 * i_end) const;
	void	Dump						(const u8 * i_start, const u8 * i_end, u32 i_depth = 0, u32 i_sequence = 0) const;
	bool	Decompose					(const u8 * i_start, const u8 * i_end, u32 i_depth = 0) const;
	
	void	InsetDump					(u32 i_depth) const;

	const u8 *	DumpStringPayload		(size_t i_count, const u8 * i_payload) const;
	
	string	ParseStringPayload			(const u8 * i_payload, const u8 * i_end, size_t * o_count = nullptr) const;
	string	ParseObjectPayload			(const u8 * i_payload, const u8 * i_end, bool i_versioned, size_t * o_count = nullptr) const;
	string	ParsePODPayload				(const u8 * i_payload, const u8 * i_end, size_t * o_count = nullptr) const;
	string	ParseEpigramPayload			(const u8 * i_payload, const u8 * i_end, size_t * o_count, u32 i_depth) const;
	string	ParseBinaryPayload			(const u8 * i_payload, const u8 * i_end, size_t * o_count = nullptr) const;
	string	ParseEnumPayload			(const u8 * i_payload, const u8 * i_end, size_t * o_count = nullptr) const;
	string	ParseObjectPointerPayload	(const u8 * i_payload, const u8 * i_end, size_t * o_count) const;

	string	ParsePointerPayload			(const u8 * i_payload, const u8 * i_end, size_t * o_count = nullptr) const;
	
	template <typename T>
	string	ParseIntrinsicPayload		(const u8 * i_payload, const u8 * i_end, size_t * o_count) const
	{
		ostringstream out;
		
		size_t totalSize = i_end - i_payload;
		
		size_t count = totalSize / sizeof (T);
		if (o_count) *o_count = count;

		for (u32 i = 0; i < count; ++i)
		{
			T t;
			memcpy (&t, i_payload, sizeof (T));
			
			if (i > 0)
				out << ", ";
			
			out << "{" << t << "}";
		}
		
		out << " (" << std::dec << totalSize << " bytes)";
		
		return out.str ();
	}
	
	template <typename T, typename C = T> // C is a cast so that output for u8's is an integer and not a ascii char.
	string ParsePayload (const u8 * i_payload, const u8 * i_end, size_t * o_count = nullptr) const
	{
		ostringstream out;
		
		size_t i_count = (i_end - i_payload) / sizeof (T);
		if (o_count)
			*o_count = i_count;
		
		size_t c_maxElements = m_argsMode ? 20 : 6;
		
		size_t count = std::min (i_count, c_maxElements-1);
		
		if (i_count > 1) out << "{ ";
		
		T * payload = (T *) i_payload;
		for (u32 i = 0; i < count; ++i)
		{
			T value = * (payload++);
			
			out << (C) value;
			if (i != count-1) out << ", ";
		}
		
		if (i_count >= c_maxElements)
		{
			if (i_count > c_maxElements)	out << ", ... ";
			else							out << ", ";
			
			T * cast = (T *) (i_payload + (i_count - 1) * sizeof (T));
			out << (C) * cast;
		}
		
		//		if (not m_argsMode) out << " ]";
		if (i_count > 1) out << " }";
		
		return out.str();
	}

	template <typename T, typename C = T> // C is a cast so that output for u8's is an integer and not a ascii char.
	string ParseHashPayload (const u8 * i_payload, const u8 * i_end, size_t * io_count) const
	{
		ostringstream out;
		
		size_t i_count =  1;
		
		if (io_count)
			i_count = *io_count;

		size_t hashSize = (i_end - i_payload) / i_count;
		
		if (hashSize == 4)
		{
			u32 value = * ((u32 *) i_payload++);
			out << EpAttributes::GetHashAttributeName (value);
			out << " (0x" << std::hex << setfill ('0') << setw (sizeof(u32) * 2) << value << ")";
		}
		else if (hashSize == 4)
		{
			u64 value = * ((u64 *) i_payload++);
			out << EpAttributes::GetHashAttributeName (value);
			out << " (0x" << std::hex << setfill ('0') << setw (sizeof(u32) * 2) << value << ")";
		}
		
//		size_t c_maxElements = m_argsMode ? 20 : 6;
//		
//		size_t count = std::min (i_count, c_maxElements-1);
//		
//		if (i_count > 1) out << "{ ";
//		
//		T * payload = (T *) i_payload;
//		for (u32 i = 0; i < count; ++i)
//		{
//			T value = * (payload++);
//			
//			out << EpAttributes::GetHashAttributeName (value);
//			out << " (0x" << std::hex << setfill ('0') << setw (sizeof(C) * 2) << (C) value << ")";
//			
//			if (i != count-1) out << ", ";
//		}
//		
//		if (i_count >= c_maxElements)
//		{
//			if (i_count > c_maxElements)	out << ", ... ";
//			else							out << ", ";
//			
//			T * cast = (T *) (i_payload + (i_count - 1) * sizeof (T));
//			
//			C value = * cast;
//			
//			out << EpAttributes::GetHashAttributeName (value);
//			out << " (0x" << std::hex << setfill ('0') << setw (sizeof(C) * 2) << value << ")";
//		}
//		
//		//		if (not m_argsMode) out << " ]";
//		if (i_count > 1) out << " }";
		
		return out.str();
	}

	
	u8 * m_start, * m_end;
	
	std::ostream &				out;
	mutable bool				m_argsMode			= false;
};

#endif /* EpigramDump_hpp */
