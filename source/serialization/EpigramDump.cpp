//
//  EpigramDump.cpp
//  Jigidesign
//
//  Created by Steven Massey on 3/26/17.
//  Copyright © 2017 Jigidesign. All rights reserved.
//

#include "EpigramDump.hpp"
#include "JdUUID.hpp"

#include <iomanip>
#include <regex>

void DumpEpigram (EpDelivery i_epigram, u32 i_sequence)
{
	EpigramDumper d (i_epigram);
	d.Dump (i_sequence);
	d.Decompose ();
}

void EpigramDumper::PrintArgs () const
{
	m_argsMode = true;
	Decompose (m_start, m_end, 0);
	m_argsMode = false;
}

EpigramDumper::EpigramDumper (EpDelivery i_epigram, std::ostream & i_stream)
:
out (i_stream)
{
	auto payload = i_epigram->GetPayload ();
	
	m_end = m_start = (u8*) payload.bytes;
	m_end += payload.size;
}

EpigramDumper::EpigramDumper (const u8 * i_payload, u32 i_size, std::ostream & i_stream)
:
out (i_stream)
{
	m_end = m_start = const_cast <u8*> (i_payload);
	m_end += i_size;
}


void EpigramDumper::Dump (u32 i_sequence) const
{
	Dump (m_start, m_end, 0, i_sequence);
}

void EpigramDumper::Decompose (u32 i_depth) const
{
	Decompose (m_start, m_end, i_depth);
}


void EpigramDumper::InsetDump (u32 i_depth) const
{
	for (u32 i = 0; i < i_depth; ++i)
		out << "    ";
}

void EpigramDumper::DumpCode () const
{
	DumpCode (m_start, m_end);
}


void EpigramDumper::DumpCode (const u8 * i_start, const u8 * i_end) const
{
	const u8 * ptr = i_start;
	
	i32 bytes = (int32_t) (i_end - i_start);
	printf ("message length: %d bytes\n", bytes);
	int i = 0;
	
	if (bytes == 0) return;
	
	cout << "const u8 c_epigram [] = {\n";
	
	while (ptr < i_end)
	{
		printf ("0x%02x", *ptr);
		
		bool endLine = (++i == 16);
		
		if (++ptr == i_end)
		{
			endLine = true;
		}
		else cout << ", ";
		
		if (endLine)
		{
			printf ("\n");
			i = 0;
		}
		
	}
	
	cout << "};\n";
}



void EpigramDumper::Dump (const u8 * i_start, const u8 * i_end, u32 i_depth, u32 i_sequence) const
{
	i32 bytes = (int32_t) (i_end - i_start);
	printf ("epigram length: %d bytes  sequence: %d\n", bytes, i_sequence);
	if (bytes == 0)
		return;
	
	const u8 * ptr = i_start;
	const u8 * ptr2 = i_start;
	
	u32 i = 0;
	
	if (i_depth > 0) out << "\n";
	InsetDump (i_depth); printf ("-----------+===========+-----------+===========   ----====----====\n");
	InsetDump (i_depth);
	while (ptr < i_end)
	{
		printf ("%02x ", *ptr);
		
		bool printAscii = (++i == 16);
		
		if (++ptr == i_end)
		{
			printAscii = true;
		}
		
		if (printAscii)
		{
			while (++i <= 16) printf ("   ");
			
			printf ("  ");
			while (ptr2 != ptr)
			{
				if (*ptr2 == ' ')						printf ("⋅");
				else if (*ptr2 >= 32 && *ptr2 <= 127)	printf ("%c", *ptr2);
				else if (*ptr2 == 0)					printf ("▫");
				else									printf ("▪");
				
				++ptr2;
			}
			printf ("\n");
			InsetDump (i_depth);
			
			i = 0;
		}
	}
	
	printf ("-----------+===========+-----------+===========   ----====----====\n");
//	if (i_depth == 0)
//		out << "\n";
}


bool EpigramDumper::Decompose (const u8 * i_start, const u8 * i_end, u32 i_depth) const
{
	bool ok = true;
	
	const u32 width = 52;
	string e;
	
	if (not m_argsMode)
	{
		out << "\n";
		InsetDump (i_depth);
		out << "----+-----+" << setfill('-') << setw(width+2) << e << "+------------+-----------------------------------------------------------------\n";
	}
	
	out << setfill(' ');
	
	const u8 * ptr = i_end-1;
	const u8 * flippedEnd = i_start-1;
	
	u32 index = 0;
	while (ptr > flippedEnd)
	{
		if (m_argsMode && ptr != i_start) out << ", ";
		
		const u8 * decode = ptr;
		
		auto payloadSize = Jd::ReverseDecode7bRE <size_t> (decode, flippedEnd) + 3; // c_payloadMinSize
		
		auto next = decode - payloadSize;
		
		u8 valType = *decode--;
		u8 keyType = *decode--;
		
		u8 isMap = keyType & c_jdTypeId::isArray; keyType &= c_jdTypeId::typeMask;
		u8 isArray = valType & c_jdTypeId::isArray; valType &= c_jdTypeId::typeMask;
		
		auto endKey = decode;
		auto keySize = Jd::ReverseDecode7bRE <size_t> (decode, flippedEnd);
		
		auto key = decode - keySize + 1;
		auto value = next + 1;
		
		//		cout << "isArray: " << (int) (bool) isArray << endl;
		
		if (not m_argsMode)
		{
			InsetDump (i_depth);
			out << setw (3) << right << index << " | ";
		}
		
		// hack to change output style for the key
		auto argsMode = m_argsMode;
		m_argsMode = true;
		
		using namespace Jd;
		
		out << setw (3) << Jd::TypeIdToName (keyType) << " | ";
		
		size_t keyCount = 1;
		
		string dump;
		switch (keyType)
		{
			case TypeId <string> ():		dump = ParseStringPayload (key, endKey);	break;
			case TypeId <u8> ():			dump = ParsePayload <u8,u16> (key, endKey);	break;
			case TypeId <i32> ():			dump = ParsePayload <i32> (key, endKey);	break;
			case TypeId <u32> ():			dump = ParsePayload <u32> (key, endKey);	break;
			case TypeId <i64> ():			dump = ParsePayload <i64> (key, endKey);	break;
			case TypeId <u64> ():			dump = ParsePayload <u64> (key, endKey);	break;
			case TypeId <f32> ():			dump = ParsePayload <f32> (key, endKey);	break;
			case c_jdTypeId::hash:			dump = ParseHashPayload <u64> (key, endKey, &keyCount);	break;
			case c_jdTypeId::none:			/* dump = ParseHashPayload <u64> (key, endKey);	*/ break;
			case c_jdTypeId::pod:			dump = ParsePODPayload (key, endKey, &keyCount); break;

			default: d_jdThrow ("unimplemented epigram key dumper: @", Jd::TypeIdToName (keyType));	break;
				
		};
		
		dump = std::regex_replace (dump, std::regex ("'"), "");
		
		out << setw (width) << setfill ('.') << left << dump << "" << setfill (' ');
		
		m_argsMode = argsMode;
		
		size_t arrayCount = 1;
		if (isArray)
			arrayCount = Jd::Decode7bRE <size_t> (value, key);
		
		size_t count = 0;
		
		if (valType & c_jdTypeId::isPointer)
		{
			valType &= c_jdTypeId::typeMask;
			valType &= ~c_jdTypeId::isPointer;	// disable pointer bit

			switch (valType)
			{
				case c_jdTypeId::versionedObject:
				case c_jdTypeId::object:
				case c_jdTypeId::pod:
												dump = ParseObjectPointerPayload (value, key, & count); break;
					
				default:						dump = ParsePointerPayload (value, key, & count); break;
			}
		}
		else
		{
			switch (valType)
			{
				case TypeId <f32> ():			dump = ParsePayload <f32> (value, key, &count);	break;
				case TypeId <f64> ():			dump = ParsePayload <f64> (value, key, &count);	break;
					
				case TypeId <i8> ():			dump = ParsePayload <i8,i16> (value, key, &count);	break;
				case TypeId <i16> ():			dump = ParsePayload <i16> (value, key, &count);	break;
				case TypeId <i32> ():			dump = ParsePayload <i32> (value, key, &count);	break;
				case TypeId <i64> ():			dump = ParsePayload <i64> (value, key, &count);	break;
					
				case TypeId <u8> ():			dump = ParsePayload <u8,u16> (value, key, &count);	break;
				case TypeId <u16> ():			dump = ParsePayload <u16> (value, key, &count);	break;
				case TypeId <u32> ():			dump = ParsePayload <u32> (value, key, &count);	break;
				case TypeId <u64> ():			dump = ParsePayload <u64> (value, key, &count);	break;
					
				case TypeId <bool> ():			dump = ParsePayload <bool> (value, key, &count);break;
					
				case TypeId <string> ():		dump = ParseStringPayload (value, key, &count);	break;
					
				case c_jdTypeId::pod:				dump = ParsePODPayload (value, key, &count); break;
				case c_jdTypeId::object:			dump = ParseObjectPayload (value, key, false, &count); break;
				case c_jdTypeId::versionedObject:	dump = ParseObjectPayload (value, key, true, &count); break;
					
				case c_jdTypeId::uuid:			dump = ParseIntrinsicPayload <JdUUID> (value, key, &count); break;
					
				case c_jdTypeId::epigram:		dump = ParseEpigramPayload (value, key, &count, i_depth); break;
					
				case c_jdTypeId::binary:		dump = ParseBinaryPayload (value, key, &count); break;
					
				case c_jdTypeId::enumeration:	dump = ParseEnumPayload (value, key, &count); break;

//				case c_jdTypeId::none:			dump = ParseEnumPayload (value, key, &count); break;

				default: d_jdThrow ("unimplemented epigram value dumper: '@'", Jd::TypeIdToChar (valType));	break;
			}
		}
		
		d_jdAssert (count == arrayCount, "fetcher didn't match count");
		
		if (not m_argsMode)
		{
			out << " | " << setw (4) << right << count << "  " << left << setw (4) << Jd::TypeIdToName (valType) << " | ";
			out << dump;
		}
		
		++index;
		
		ptr = next;
		
		if (not m_argsMode)
			out << "\n";
	}
	
	if (not m_argsMode)
	{
		InsetDump (i_depth);
		out << "----+-----+" << setfill('-') << setw(width+2) << e << "+------------+-----------------------------------------------------------------";
		out << setfill(' ');
		if (i_depth == 0) out << "\n\n";
	}
	
	return ok;
}


string EpigramDumper::ParseEpigramPayload (const u8 * i_payload, const u8 * i_end, size_t * o_count, u32 i_depth) const
{
	ostringstream out;
	
	size_t count = 0;
	while (i_payload < i_end)
	{
		size_t epigramSize = Jd::Decode7bRE <size_t> (i_payload, i_end);
		
		++count;
		
		if (epigramSize)
		{
			EpigramDumper dumper (i_payload, epigramSize, out);
			
			if (m_argsMode)
			{
				out << "{";
				dumper.PrintArgs ();
				out << "}";
			}
			else
				dumper.Decompose (i_depth + 1);
			
		}
		
		i_payload += epigramSize;
	}
	
	if (o_count) *o_count = count;
	
	
	return out.str ();
}


string EpigramDumper::ParseObjectPointerPayload (const u8 * i_payload, const u8 * i_end, size_t * o_count) const
{
	ostringstream out;

	string className = (char *) i_payload;
	i_payload += (className.size() + 1);
	out << "<" << className << "> ";
	char temp [32];

	size_t count = 0;
	while (i_payload < i_end)
	{
		voidptr_t ptr;
		memcpy (&ptr, i_payload, sizeof (voidptr_t));
		
		if (count > 0) out << ", ";
		
		sprintf (temp, "0x%llX", (u64) ptr);
		out << temp;
		
		i_payload += sizeof (voidptr_t);
		++count;
	}
	
	if (o_count) *o_count = count;

	return out.str ();
}


string EpigramDumper::ParsePODPayload (const u8 * i_payload, const u8 * i_end, size_t * o_count) const
{
	ostringstream out;
	
	auto ptr = i_payload;
	
	string className = (char *) ptr;
	
	ptr += className.size () + 1;
	
	out << "<" << className << ">";

	size_t podSize = Jd::Decode7bRE <size_t> (ptr, i_end);
	
	size_t totalSize = i_end - ptr;
	
	if (o_count)
		* o_count = totalSize / podSize;
	
	out << " (" << std::dec << totalSize << " bytes)";
	
	return out.str ();
}



string EpigramDumper::ParseBinaryPayload (const u8 * i_payload, const u8 * i_end, size_t * o_count) const
{
	ostringstream out;
	
	size_t totalSize = i_end - i_payload;
	
	out << " (";
	
	size_t count = 0;
	
	while (i_payload < i_end)
	{
		size_t binSize = Jd::Decode7bRE <size_t> (i_payload, i_end);
		
		i_payload += binSize;
		
		if (i_payload <= i_end)
		{
			if (count) out << ", ";
			out << binSize;
			
			++count;
		}
	}
	
	if (o_count) (* o_count) = count;
	
	out << " bytes)";
	
	out << " total: " << std::dec << totalSize << " bytes";
	
	return out.str ();
}

string EpigramDumper::ParseEnumPayload (const u8 * i_payload, const u8 * i_end, size_t * o_count) const
{
	ostringstream out;
	
	auto ptr = i_payload;
	
	string className = (char *) ptr;
	
	ptr += className.size () + 1;
	
	out << "<" << className << ">";
	
	size_t totalSize = i_end - ptr;
	
	while (ptr < i_end)
	{
		size_t e = Jd::Decode7bRE <size_t> (ptr, i_end);
		
		if (o_count)
			(* o_count)++;
		
		out << " " << e;
	}
	
	out << " (" << std::dec << totalSize << " bytes)";
	
	return out.str ();
}




string EpigramDumper::ParseObjectPayload (const u8 * i_payload, const u8 * i_end, bool i_versioned, size_t * o_count) const
{
	ostringstream out;
	u8 version;
	string className;
 
	className = (char *) i_payload;
	i_payload += (className.size() + 1);

	if (i_versioned)
	{
		version = * i_payload;
		++i_payload;
		
		out << "<" << className << ":" << (u32) version << ">";
	}
	else out << "<" << className << ">";
	
	
	vector <size_t> objectSizes;
	
	auto next = i_end;
	auto ptr = next - 1;
	auto end = i_payload;
	size_t count = 0;
	
	if (ptr > end)
	{
		size_t objectSize = 0;
		while (ptr > end)
		{
			next = ptr + 1;
			objectSize = Jd::ReverseDecode7bRE <size_t> (ptr, end);
			ptr -= objectSize;
			
			objectSizes.push_back (objectSize);
			
			++count;
		}
		
		size_t i = 0;
		size_t numDump = min ((size_t) 6, objectSizes.size ());
		out << " (";
		bool first = true;
		for (auto s : objectSizes)
		{
			if (not first) out << ", ";
			out << s;
		 	first = false;
			
			if (i > numDump)
			{
				out << "...";
				break;
			}
			++i;
		}
		out << " bytes) ";
	}

	if (o_count) *o_count = count;
	

	out << std::dec << (i_end - i_payload) << " byte payload";
	
	return out.str ();
}



string EpigramDumper::ParsePointerPayload (const u8 * i_payload, const u8 * i_end, size_t * o_count) const
{
	ostringstream out;
	
	size_t count = 0;
	
	size_t available = i_end - i_payload;
	
	while (available >= sizeof (void *))
	{
		void * ptr;
		memcpy (& ptr, i_payload, sizeof (void *));
		
		if (count)
			out << ", ";
		
		out << "0x" << ptr;
		
		i_payload += sizeof (void *);
		--available;
		++count;
	}
	
	if (o_count) *o_count = count;
	
	return out.str ();
}

string EpigramDumper::ParseStringPayload (const u8 * i_payload, const u8 * i_end, size_t * o_count) const
{
	string out;
	
	size_t count = 0;
	while (i_payload < i_end)
	{
		string s = (cstr_t) i_payload;
		i_payload += (s.size() + 1);
		
		s = "'" + s + "'";
		
		if (count)
			out += ", ";
		
		out += s;
		++count;
	}
	if (o_count) * o_count = count;
	
	return out;
}

const u8 * EpigramDumper::DumpStringPayload (size_t i_count, const u8 * i_payload) const
{
	const u32 c_maxElements = 4;
	//		u32 count = std::min (i_count, (u32) c_maxElements-1);
	
	for (u32 i = 0; i < i_count; ++i)
	{
		string s = (cstr_t) i_payload;
		i_payload += (s.size() + 1);
		
		if (i == i_count-1)
		{
			if (i_count > c_maxElements)
				out << "... ";
			
			out << "'" << s << "'";
		}
		else if (i < c_maxElements-1)
		{
			out << "'" <<  s << "', ";
		}
	}
	
	if (not m_argsMode)
		out << " ]";
	
	return i_payload;
}

