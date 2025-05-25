//
//  STL Help.hpp
//  Speaker
//
//  Created by Steven Massey on 3/18/22.
//  Copyright © 2022 Massey Plugins LLC. All rights reserved.
//

#ifndef StlUtils_hpp
#define StlUtils_hpp

#include "JdCore.hpp"

# ifndef type_def
#	define type_def 	typedef typename
# endif

# ifndef type_if
#	define type_if 		typedef typename std::conditional
# endif


namespace jd
{
	
template <typename C>
bool  erase_front  (C & io_container, size_t i_count = 1)
{
	if (io_container.size () >= i_count)
	{
		io_container.erase (io_container.begin (), io_container.begin () + i_count);
		return true;
	}
	else return false;
}

template <typename C>
bool  erase_back  (C & io_container, size_t i_count = 1)
{
	if (io_container.size () >= i_count)
	{
		io_container.erase (io_container.end () - i_count, io_container.end ());
		return true;
	}
	else return false;
}


template <typename C>
typename C::value_type  pop_back  (C & io_container)
{
	auto v = io_container.back ();
	io_container.pop_back ();
	return v;
}


template <typename C>
void  prepend  (C & i_container, const C & i_prepend)
{
	i_container.insert (i_container.begin (), i_prepend.begin (), i_prepend.end ());
}

template <typename CA, typename CB>
void  append  (CA & i_container, const CB & i_append)
{
	i_container.insert (i_container.end (), i_append.begin (), i_append.end ());
}


template <typename C>
void  reverse  (C & i_container)
{
	std::reverse (i_container.begin (), i_container.end ());
}


template <typename C>
typename C::value_type &  at  (C & i_container, size_t i_index)
{
	auto i = i_container.begin ();
	std::advance (i, i_index);
	return *i;
}



template <typename C>
C  reversed  (C & i_container)
{
	auto c = i_container;
	std::reverse (c.begin (), c.end ());
	return c;
}



	template <typename T, typename C>
	auto to (const C & i_container)
	{
		// TODO: metaswitch container type
		//		type_if <std::is_vector <C>::value,					vector <T>,			NullType>::type		typeA;

		
		std::vector <T> container;
		
		container.insert (container.end (), i_container.begin (), i_container.end ());

//		for (auto & v : i_container)
//			container.push_back (v);
		
		return container;
	}

	
	template <typename T>
	void memcpy (T * i_dest, const T * i_src, size_t i_count)
	{
		std::memcpy (i_dest, i_src, sizeof (T) * i_count);
	}

	template <typename T>
	void memcpy (void * i_dest, voidptr_t i_src, size_t i_count)
	{
		std::memcpy (i_dest, i_src, sizeof (T) * i_count);
	}

	template <typename T>
	void memclr (T * i_dest, size_t i_count)
	{
		std::memset (i_dest, 0x0, sizeof (T) * i_count);
	}

	template <typename T>
	T memread (voidptr_t i_ptr)
	{
		T value;
		std::memcpy (& value, i_ptr, sizeof (T));
		return value;
	}

	
template <typename Container, typename Compare>
void sort (Container & i_container, const Compare & i_compare)
{
	std::sort (i_container.begin (), i_container.end (), i_compare);
}



template <typename T>
bool update (T & io_value, T const & i_newValue)
{
	if (io_value != i_newValue)
	{
		io_value = i_newValue;
		return true;
	}
	else return false;
}


}


template <typename T>
std::vector <T> & operator += (std::vector <T> & i_lhs, const std::vector <T> & i_rhs)
{
	jd::append (i_lhs, i_rhs);
	return i_lhs;
}

#endif /* StlUtils_hpp */
