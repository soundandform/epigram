//
//  JdCoreL2.h
//  Jigidesign
//
//  Created by Steven Massey on 9/4/15.
//  Copyright (c) 2015 Jigidesign. All rights reserved.
//

#ifndef __Jigidesign__JdCoreL2__
#define __Jigidesign__JdCoreL2__

#include <sstream>

#include "JdNucleus.hpp"


namespace Jd
{
	i32 HashCString31 (cstr_t i_string);
	u64 HashString64 (const std::string & i_string);

	
	template <class X>
	X* SingletonHelper (bool i_create)
	{
		static X * lonely = new X;
		static i64 usageCount = 0;
		
		if (i_create)
		{
			++usageCount;
		}
		else
		{
			if (--usageCount <= 0)
			{
				delete lonely;
				lonely = nullptr;
				usageCount = 0;
			}
		}
		
		return lonely;
	}
	
	
	template <class X>
	X* AcquireSingleton () // X * should be subsequently released with the ReleaseSingleton () call, if you want the singleton to be eventually shutdown
	{
		return SingletonHelper <X> (true);
	}
	
	
	template <class X>
	void ReleaseSingleton (X * i_singleton)
	{
		if (i_singleton)
			SingletonHelper <X> (false);
	}

	
	template <typename X>
	struct Singleton
	{
		Singleton ()
		{
			m_lonely = Jd::AcquireSingleton <X> ();
		}
		
		~Singleton ()
		{
			Jd::ReleaseSingleton (m_lonely);
		}
		
		X * operator -> () const
		{
			return m_lonely;
		}
		
		operator X * () const
		{
			return m_lonely;
		}
		
		X * m_lonely;
	};
}


#endif /* defined(__Jigidesign__JdCoreL2__) */
