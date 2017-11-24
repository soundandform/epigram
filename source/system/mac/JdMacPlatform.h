//
//  File.h
//  Jigidesign
//
//  Created by Steven Massey on 5/7/13.
//  Copyright (c) 2013 Jigidesign. All rights reserved.
//

#ifndef __Jigidesign__JdMacPlatform__
#define __Jigidesign__JdMacPlatform__

#include "IJdPlatform.h"
#include "JdInternal.h"

#include <string>

#import <Foundation/Foundation.h>

@interface d_objC (JdTimerBridgeCallbackWrapper) : NSObject
{
	@public
	
	IJdCallback * m_callback;
}
@end


@interface d_objC (JdTimerBridge) : NSObject
{
	NSMutableArray * m_timers;
}
@end


namespace JdMacPlatform
{
	struct FileSystem : public IJdPlatform::FileSystem
	{
									FileSystem				();
		virtual std::string			GetSandboxPath			(EJdFileSystemLocation i_persistence);
		virtual u64					GetAvailableBytesInPath	(const std::string & i_path);
		virtual u64					GetUsedBytesInPath		(const std::string & i_path);
		virtual std::string			ResolveToFullPath		(const std::string & i_path);
		virtual i32					RemoveFile				(const std::string & i_path);
	};
	
	struct Notification : public IJdPlatform::Notification
	{
//		Notification				() {}
		
		virtual JdTimerRef			ScheduleTimer				(f64 i_periodInSeconds, IJdCallback * i_callback, void * i_refData);
		virtual void				DestroyTimer				(const JdTimerRef &i_timerRef);
		
		virtual void				PostNotification			(cstr_t i_name, JdId i_zone, EpDelivery i_message);
		
		protected:
		d_objC (JdTimerBridge) *	m_timerBridge				= nullptr;
	};
}




#endif /* defined(__Jigidesign__File__) */
