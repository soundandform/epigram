//
//  JdEnvironment.cpp
//  Jigidesign
//
//  Created by Steven Massey on 7/5/12.
//  Copyright © 2012-2020 Steven M Massey. All rights reserved.
//

#include "JdEnvironment.hpp"


// Module Libraries ---------------------------------------------------------------------------
//#include "JdSql.h"

#if d_JdLinkBasic
//#	include "JdTimers.hpp"
//#	include "JdTimerManager.h"
//#	include "JdNotifications.hpp"
//#	include "JdBroadcaster.h"
#endif


d_jdLinkDef (Sql)
d_jdLinkDef (Timers)
d_jdLinkDef (TimerDriver)
d_jdLinkDef (Notifications)
d_jdLinkDef (Broadcaster)


void JdLinkBasic::Setup ()
{
	//#	if d_JdLinkBasic
	//		JdLink::Timers				();
	//		JdLink::TimerDriver			();
	//
	//		JdLink::Notifications		();
	//		JdLink::Broadcaster			();
	//#	endif
	
	JdLink::Sql					();
}


void JdLinkSimple::Setup ()
{
	JdLink::Sql ();
}


void  JdEnvironment::RunMainThread  (f64 i_maxDurationInSeconds)
{
	JdTimerThread * mainThread = m_server.GetScheduler ()->GetMainThread ();

	mainThread->Run (i_maxDurationInSeconds * 1e6);
}


void  JdEnvironment::RunMainLoop  (f64 i_durationInSeconds)
{
	JdTimerThread * mainThread = m_server.GetScheduler ()->GetMainThread ();
	
	mainThread->RunLoop (i_durationInSeconds * 1e6);
}


JdResult  JdEnvironment::Teardown  ()
{
	JdResult result;
	
	Jd::EnforceMainThread ();
	
	if (m_timersEnabled)
	{
		jd_lock (jd_timers)
			jd_timers->StopAllTimers ({});
		
		m_timersEnabled = false;
	}
	
	jd_timers.Release ();
	
	result = m_server.GetScheduler()->Teardown ();
	
	m_server.Teardown ();
	
#if FIX
	jd_broadcaster.ForceRelease ();	// teardown broadcaster last
#endif
	
	m_server.Teardown (true);
	
	m_initialized = false;
	
	return result;
}
