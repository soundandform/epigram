//
//  M3Token.hpp
//  libgestalt
//
//  Created by Steven Massey on 11/4/18.
//  Copyright © 2018 Steven Massey. All rights reserved.
//

#ifndef M3Token_h
#define M3Token_h

#include "JdNucleus.hpp"

struct M3Token
{
	u32 Code () const 	{ return m_code; }
	
	u32 m_code			= 0;
};


#endif /* M3Token_h */
