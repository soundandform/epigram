//
//  IJdPlatform.h
//  Jigidesign
//
//  Created by Steven Massey on 3/25/13.
//  Copyright (c) 2013 Jigidesign. All rights reserved.
//

#ifndef Jigidesign_IJdPlatform_h
#define Jigidesign_IJdPlatform_h

#include "IEpigram.hpp"
#include "JdCore.hpp"
#include "JdUUID.hpp"
#include "JdEnum.hpp"

/*
 
 All functionality needing platform specific functionality (that isn't handle by Boost) should be put into these pure virtual interfaces, organized under
 the IJdPlatform namespace umbrella.
 
 A platform can then be supported by filing out the implementations and specializing the JdPlatform::Get template.  (See JdiOSPlatform.h for example.)
 
 To use one of the interfaces:

 */

#if 0
	 	auto filesystem = JdPlatform::Get <IJdPlatform::FileSystem> ();
 
	 	filesystem->GetSandboxPath (c_jdFileSystemLocation::temporary);
#endif


struct IJdCallback
{
	virtual bool /* repeat */		CallbackFired			(void *i_refData) = 0;
};


struct JdTimerRef
{
	void *	ref;
};


// for iOS
//----------------------------------------------------------------------------
d_jdEnum_(FileSystemLocation,
		  
		  temporary,		//	 /tmp/
		  appCache,			//   [home]/Library/Cache on iOS
		  appData,			//   [home]/Library/AppData/				-- created ourselves
		  appPreferences,
		  userDocuments);	//   [home]/Documents						-- potentionally exposed to user in iTunes

namespace IJdPlatform {

	
	
struct FileSystem //--------------------------------------------------------------------------------------------------------------------------------------
{
	virtual std::string			GetSandboxPath			(EJdFileSystemLocation i_location) = 0;
	virtual u64					GetAvailableBytesInPath	(const std::string & i_path) = 0;
	virtual u64					GetUsedBytesInPath		(const std::string & i_path) = 0;
	virtual std::string			ResolveToFullPath		(const std::string & i_path) = 0;
	virtual i32					RemoveFile				(const std::string & i_path) = 0;
};

struct Notification //------------------------------------------------------------------------------------------------------------------------------------
{
	virtual JdTimerRef			ScheduleTimer				(f64 i_periodInSeconds, IJdCallback * i_callback, void * i_refData) = 0;
	virtual void				DestroyTimer				(const JdTimerRef &i_timerRef) = 0;

//	"1DE70882-2676-40A8-A002-6A2B03F77429.progress"
	virtual void				PostNotification			(cstr_t i_name, JdId i_zone = JdUUID (), EpDelivery i_message = nullptr) = 0;
//	virtual void *				GetNotifier					(c_str i_name, JdId i_zone) = 0;
	
//	virtual void				PingApplication			() = 0;
};

}; // end-namespace IJdPlatform


namespace JdPlatform
{
	template <typename service_t>
	service_t * 					Get						();
}

#endif
