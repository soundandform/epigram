/*
 *  JdVersion.hpp
 *  Jigidesign
 *
 *  Created by Steven Massey on 2/19/12.
 *  Copyright 2012 Jigidesign. All rights reserved.
 *
 */


#pragma once

#include "JdCore.hpp"


const uint16_t c_jdVersionNumDontCare = 0xffff;
const uint32_t c_jdRevisionNumDontCare = 0xffffffff;

struct JdVersion
{
	u16		major;
	u16		minor;
	u32		revision;
	
	JdVersion (uint64_t i_compactVersion)
	{
		major = (u16) (i_compactVersion >> 48);
		minor = (u16) ((i_compactVersion >> 32) & 0x000000000000FFFF);
		revision = (u32) (i_compactVersion & 0x00000000FFFFFFFF);
	}
	
	JdVersion (const char *i_versionString)
	:
	major (0), minor (c_jdVersionNumDontCare), revision (c_jdRevisionNumDontCare)
	{
		std::string temp = i_versionString;
		
		size_t found;
		while ((found = temp.find(".")) != std::string::npos)
			temp [found] = ' ';
		
		while ((found = temp.find("x")) != std::string::npos)
			temp.replace(found, 1, "-1");
		
		int32_t a = c_jdVersionNumDontCare, b = c_jdVersionNumDontCare, c = c_jdRevisionNumDontCare;
		
		std::istringstream iss (temp);
		iss >> a >> b >> c;
		major = a & 0x0000ffff;
		minor = b & 0x0000ffff;
		revision = c;
	}
	
	JdVersion (const char *i_versionString, uint32_t i_revision)
	:
	major (0), minor (c_jdVersionNumDontCare), revision (i_revision)
	{
		std::string temp = i_versionString;
		
		size_t found;
		while ((found = temp.find(".")) != std::string::npos)
			temp [found] = ' ';
		
		while ((found = temp.find("x")) != std::string::npos)
			temp.replace(found, 1, "-1");
		
		int32_t a = c_jdVersionNumDontCare, b = c_jdVersionNumDontCare;
		
		std::istringstream iss (temp);
		iss >> a >> b;
		major = a & 0x0000ffff;
		minor = b & 0x0000ffff;
	}
	
	JdVersion (uint16_t i_major, uint16_t i_minor)
	: major (i_major), minor (i_minor), revision (c_jdRevisionNumDontCare)
	{
	}
	
	JdVersion () : major (c_jdVersionNumDontCare), minor (c_jdVersionNumDontCare), revision (c_jdRevisionNumDontCare)
	{
	}
	
	bool IsSet () const
	{
		return (major != c_jdVersionNumDontCare || minor != c_jdVersionNumDontCare || revision != c_jdRevisionNumDontCare);
	}
	
	operator u64 ()
	{
		return ((u64) major << 48) | ((u64) minor << 32) | ((u64) revision);
	}
};

bool operator == (const JdVersion &i_versionA, const JdVersion &i_versionB);
bool operator >= (const JdVersion &i_versionA, const JdVersion &i_versionB);
bool operator <= (const JdVersion &i_versionA, const JdVersion &i_versionB);
bool operator  < (const JdVersion &i_versionA, const JdVersion &i_versionB);

std::ostream & operator<< (std::ostream & output, const JdVersion & i_version);



#include "Epilog.hpp"

