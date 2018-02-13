//
//  JdThread.cpp
//  Jigidesign
//
//  Created by Steven Massey on 11/10/12.
//  Copyright (c) 2012 Jigidesign. All rights reserved.
//

#include "JdThread.hpp"

//d_epilogCategory (threads, JdThread)

namespace Jd
{
	void EnforceMainThread ()
	{
		#if __APPLE__
			d_jdAssert (pthread_main_np (), "not in main thread");
		#else
			// FIX
		#endif
	}
}
