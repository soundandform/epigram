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
#include "JdNucleus.hpp"

namespace Jd
{
	std::string					TrimString					(std::string i_string);
	std::string					StripString					(const std::string &i_string, cstr_t i_chars);
	std::vector <std::string>	SplitString					(std::string i_string, std::string i_delimiter);		// TODO: i think there's a boost replacement
	
	std::string					ReadFileContentsToString	(stringRef_t i_filename);
}

#endif /* JdStdStringUtils_hpp */
