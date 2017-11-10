//
//  JdConfig.cpp
//  Jigidesign
//
//  Created by Steven Massey on 6/29/16.
//  Copyright © 2016 Jigidesign. All rights reserved.
//

#include "JdConfig.hpp"

JdConfig::JdConfig ()
{
	#ifdef Epilog_h
//		epilog (normal, "build: @", (IsDebug() ? "debug" : "release"));
	#endif
}


const bool JdConfig::IsPlatform (EJdPlatform i_platform)
{
	EJdPlatform platform;
	
	#if __APPLE__
		platform = c_jdPlatform::macOS;
	#endif
	
	return (i_platform == platform);
}

JdConfig JdConfig::config = JdConfig ();


