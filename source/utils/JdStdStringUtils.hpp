//
//  JdStdStringUtils.hpp
//  Jigidesign
//
//  Created by Steven Massey on 6/29/16.
//  Copyright © 2016 Jigidesign. All rights reserved.
//

#ifndef JdStdStringUtils_hpp
#define JdStdStringUtils_hpp

#include <string>
#include <vector>
#include <fstream>
#include "JdNucleus.hpp"

namespace Jd
{
	std::string					TrimString						(std::string i_string);
	std::string					StripString						(const std::string &i_string, cstr_t i_chars);
	std::vector <std::string>	SplitString						(std::string i_string, std::string i_delimiter);		// TODO: i think there's a boost replacement
	
	std::string					ReadFileContentsToString		(stringRef_t i_filename);
	
	// FIX: only works for string/vector
	template <typename C> i32	WriteContainerToFile			(stringRef_t i_filename, C & i_container)
	{
		using namespace std;
		ofstream ofs (i_filename, ios::binary);
		
		size_t valueSize = sizeof (typename C::value_type);
		
		ofs.write ((char *) i_container.data (), i_container.size () * valueSize);
		
		return - (not ofs.good ());		// return -1 if not good
	}
}

#endif /* JdStdStringUtils_hpp */
