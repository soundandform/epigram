//
//  JdEnvironment.cpp
//  Jigidesign
//
//  Created by Steven Massey on 7/5/12.
//  Copyright © 2012-2020 Steven M Massey. All rights reserved.
//

#include "JdEnvironment.hpp"


// Module Libraries ---------------------------------------------------------------------------

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
	
	jd_lock (jd_timers)
		jd_timers->StopAllTimers ({});
	
	m_server.ReleaseSingletons ();

	result = m_server.GetScheduler()->Teardown ();

	// disable the ModuleDiener, so timer release doesn't try to send txn
	m_server.Shutdown ();

	// timers has to be released after stopping thread, so timer thread doesn't
	// try to drive the timers
	jd_timers.Release ();

	// Teardown will delete JdModuleDiener and JdTimers on the main thread
	result |= m_server.Teardown ();

	m_initialized = false;
	
	return result;
}
