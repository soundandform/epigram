//
//  JdThread.cpp
//  Jigidesign
//
//  Created by Steven Massey on 11/10/12.
//  Copyright (c) 2012 Jigidesign. All rights reserved.
//

#include "JdThread.hpp"

//d_epilogCategory (threads, JdThread)

//std::thread::id  s_mainThreadId = std::this_thread::get_id ();


namespace Jd
{

bool  IsMainThread  ()
{
	return std::this_thread::get_id () == s_mainThreadId;
}

	void EnforceMainThread ()
	{
		d_jdAssert (IsMainThread (), "not in main thread");

		#if __APPLE__
//			d_jdAssert (pthread_main_np (), "not in main thread");
		#else
			// FIX
		#endif
	}
}
