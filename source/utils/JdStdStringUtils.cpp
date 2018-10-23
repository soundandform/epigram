//
//  JdStdStringUtils.cpp
//  Jigidesign
//
//  Created by Steven Massey on 6/29/16.
//  Copyright © 2016 Jigidesign. All rights reserved.
//

#include "JdStdStringUtils.hpp"

#include <fstream>
using namespace std;

namespace Jd
{
	std::string TrimString (std::string i_string)
	{
		size_t p;
		
		while ((p = i_string.find (' ')) == 0)
		{
			i_string = i_string.substr (1);
		}
		
		while ((p = i_string.rfind (' ')) == (i_string.size() - 1))
		{
			i_string = i_string.substr (0, p);
		}
		
		return i_string;
	}
	
	std::vector <std::string> SplitString (std::string i_string, std::string i_delimiter)
	{
		std::vector <std::string> strings;
		
		std::string subString;
		
		while (true)
		{
			size_t pos = i_string.find (i_delimiter);
			if (pos == std::string::npos) break;
			
			subString = i_string.substr (0, pos);
			i_string = i_string.substr (pos + 1);
			
			if (subString.size())
			{
				strings.push_back (TrimString (subString));
			}
		}
		
		strings.push_back (TrimString (i_string));
		return strings;
	}
	
	
	std::string StripString (const std::string &i_string, cstr_t i_chars)
	{
		std::string s = i_string;
		
		for (int i = 0; i_chars [i]; ++i)
		{
			size_t pos;
			while ((pos = s.find (i_chars[i])) != std::string::npos)
				s.erase (pos, 1);
		}
		
		return s;
	}

	
	std::string ReadFileContentsToString (stringRef_t i_filename)
	{
		ifstream ifs (i_filename, ios::binary | ios::ate);
		
		if (ifs.good ())
		{
			ifstream::pos_type pos = ifs.tellg();
			
			std::string result (pos, 0);
			
			ifs.seekg (0, ios::beg);
			ifs.read (& result [0], pos);
			
			return result;
		}
		else return std::string ();
	}
}
