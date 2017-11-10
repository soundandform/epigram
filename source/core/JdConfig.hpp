//
//  JdConfig.hpp
//  Jigidesign
//
//  Created by Steven Massey on 6/29/16.
//  Copyright © 2016 Jigidesign. All rights reserved.
//

#ifndef JdConfig_hpp
#define JdConfig_hpp

#include "JdEnum.hpp"

d_jdEnum_(Platform, macOS, linux)

struct JdConfig
{
	JdConfig ();
	
	static const bool IsUnitTest () { return config.m_isUnitTest; }
	
	static constexpr bool IsDebug () { return m_isDebug; }
	
	static const bool IsPublicBuild ()
	{
		#if d_jdIsPublicBuild
			return true;
		#else
			return false;
		#endif
	}
	
	static const bool IsPlatform (EJdPlatform i_type);
	
	static void			EnableUnitTest ()
	{
		config.m_isUnitTest = true;
	}
	
	protected://------------------------------------------------------------------------------
	
	static JdConfig config;
	
	bool				m_isUnitTest		= false;
	static const bool	m_isDebug =
#	if DEBUG
		true;
#	else
		false;
#	endif
};




#endif /* JdConfig_hpp */
