//
//  JdFlatString.cpp
//  Tape
//
//  Created by Steven Massey on 6/23/20.
//  Copyright © 2020 Massey Plugins Inc. All rights reserved.
//

#include "JdFlatString.hpp"



#if __OBJC__

//#	import <Foundation/NSString.h>

	JdString256ObjC::JdString256ObjC (NSString * i_nsString)
	{
		CFStringGetCString ((__bridge CFStringRef) i_nsString, m_cstring, 256, kCFStringEncodingMacRoman);
	}
};

#endif

