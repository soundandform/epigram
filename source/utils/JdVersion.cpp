//
//  JdVersion.cpp
//
//  Created by Steven Massey on 11/10/12.
//  Copyright (c) 2012 Jigidesign. All rights reserved.
//

#include <stdio.h>
#include "JdVersion.hpp"

bool operator == (JdVersionRef i_versionA, JdVersionRef i_versionB)
{
	if (i_versionA.major != i_versionB.major) return false;
	
	if (i_versionA.minor != c_jdVersionNumDontCare && i_versionB.minor != c_jdVersionNumDontCare)
	{
		if (i_versionA.minor != i_versionB.minor) return false;
	}
	
	if (i_versionA.revision != c_jdRevisionNumDontCare && i_versionB.revision != c_jdRevisionNumDontCare)
	{
		if (i_versionA.revision != i_versionB.revision) return false;
	}
	return true;
}


bool operator >= (JdVersionRef i_versionA, JdVersionRef i_versionB)
{
	if (i_versionA.major < i_versionB.major && i_versionB.major != c_jdVersionNumDontCare) return false;
	
	if (i_versionA.minor != c_jdVersionNumDontCare && i_versionB.minor != c_jdVersionNumDontCare)
	{
		if (i_versionA.minor < i_versionB.minor) return false;
	}
	
	if (i_versionA.revision != c_jdRevisionNumDontCare && i_versionB.revision != c_jdRevisionNumDontCare)
	{
		if (i_versionA.revision < i_versionB.revision) return false;
	}
	
	return true;
}


bool operator <= (JdVersionRef i_versionA, JdVersionRef i_versionB)
{
	if (i_versionA.major > i_versionB.major) return false;
	
	if (i_versionA.minor != c_jdVersionNumDontCare && i_versionB.minor != c_jdVersionNumDontCare)
	{
		if (i_versionA.minor > i_versionB.minor) return false;
	}
	
	if (i_versionA.revision != c_jdRevisionNumDontCare && i_versionB.revision != c_jdRevisionNumDontCare)
	{
		if (i_versionA.revision > i_versionB.revision) return false;
	}
	
	return true;
}


bool operator < (JdVersionRef i_versionA, JdVersionRef i_versionB)
{
	if (i_versionA.major >= i_versionB.major) return false;
	
	if (i_versionA.minor != c_jdVersionNumDontCare && i_versionB.minor != c_jdVersionNumDontCare)
	{
		if (i_versionA.minor >= i_versionB.minor) return false;
	}
	
	if (i_versionA.revision != c_jdRevisionNumDontCare && i_versionB.revision != c_jdRevisionNumDontCare)
	{
		if (i_versionA.revision >= i_versionB.revision) return false;
	}
	
	return true;
}


std::ostream& operator << (std::ostream &output, JdVersionRef i_version)
{
	if (i_version.major == c_jdVersionNumDontCare) output << "x";
	else output << i_version.major;
	
	output << ".";
	
	if (i_version.minor == c_jdVersionNumDontCare) output << "x";
	else output << i_version.minor;
	
	output << ".";
	
	if (i_version.revision == c_jdRevisionNumDontCare) output << "x";
	else output << i_version.revision;
	
	return output;
}
