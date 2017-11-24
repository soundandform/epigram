//
//
//  Created by Steven Massey on 9/13/12.
//  Copyright (c) 2012 Jiggydesign. All rights reserved.
//

#ifndef Playset_ObjCCriticalSection_h
#define Playset_ObjCCriticalSection_h

#include "IJdPlatform.h"
#include "JdInternal.h"

#include <string>

#import <Foundation/Foundation.h>

@interface JdTimerBridgeCallbackWrapper : NSObject
{
@public
	
	IJdCallback * m_callback;
}
@end


@interface JdTimerBridge : NSObject
{
	NSMutableArray * m_timers;
}


@end


namespace JdiOSPlatform
{
	struct FileSystem : public IJdPlatform::FileSystem
	{
									FileSystem				();
		
		virtual std::string			GetSandboxPath			(EJdFileSystemLocation i_location);
		virtual u64					GetAvailableBytesInPath	(const std::string & i_path);
		virtual u64					GetUsedBytesInPath		(const std::string & i_path);
		virtual std::string			ResolveToFullPath		(const std::string & i_path);
		virtual i32 				RemoveFile				(const std::string & i_path);
	};

	struct Notification : public IJdPlatform::Notification
	{
									Notification				()
		:
		m_timerBridge 				(0)
		{
			
		}
		
		virtual JdTimerRef			ScheduleTimer			(int i_periodInMilliseconds, IJdCallback * i_callback, void * i_refData);
		virtual void				DestroyTimer			(const JdTimerRef &i_timerRef);
		
		virtual void				PostNotification		(cstr_t i_name, JdId i_zone, EpDelivery i_message);

		protected:
		JdTimerBridge *				m_timerBridge;
	};
}


#endif


