//
//  EpArgs.hpp
//  Jigidesign
//
//  Created by Steven Massey on 3/26/17.
//  Copyright © 2017 Jigidesign. All rights reserved.
//

#ifndef EpArgs_h
#define EpArgs_h

#include "JdAssert.hpp"


template <typename A, typename T>
struct EpArg
{
	EpArg		(const A & i_name, const T & i_value)
	:
	name		(i_name),
	value		(i_value)
	{}
	
	template <typename X>
	void operator , (X &&) = delete;   // you need to brace intitializers around Epigram arguments: { ... } not paren (...)
	
	const A &	name;
	const T &	value;
};


template <typename A>
class EpArg_
{
	public:
	
	template <typename T>
	EpArg <A, T> operator = (T && i_value) const
	{
		return EpArg <A, T> (m_arg, std::forward <T> (i_value));
	}
	
	
	EpArg_ (const A & i_arg)
	:
	m_arg		(i_arg)
	{}
	
	protected:
	A		m_arg;
};


inline const EpArg_ <cstr_t> operator"" _ (const char *i_name, size_t i_length)
{
	return EpArg_ <cstr_t> (i_name);
}

inline const EpArg_<u8> operator"" _ (unsigned long long int i_index)
{
	d_jdAssert (i_index < 256, "indexed argument out of range");
	
	return EpArg_ <u8> ((u8) i_index);
}


#endif /* EpArgs_h */
