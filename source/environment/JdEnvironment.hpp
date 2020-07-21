//
//  JdEnvironment.h
//  Jigidesign
//
//  Created by Steven Massey on 8/24/16.
//  Copyright © 2016 Jigidesign. All rights reserved.
//

#ifndef JdEnvironment_h
#define JdEnvironment_h


//#include "JdLibraryManager.h"
#include "JdServer.h"
#include "JdLibraryFactory.h"

#include "IJdBroadcaster.h"
#include "IJdTimers.h"


class JdEnvironment
{
	public:
	
	JdEnvironment				(bool i_enableTimers = false, bool i_deferSetup = false)
	:
	m_server					(m_libraries),
	m_timersEnabled				(i_enableTimers)
	{
		if (not i_deferSetup)
			Setup ();
	}
	
	virtual ~ JdEnvironment		()
	{
		Teardown ();
	}
	
	
	void DisableModules (cstr_t i_disabledModules)
	{
		if (i_disabledModules)
			m_disabledModules = Jd::SplitString (i_disabledModules, ",");
	}
	
	
	JdResult Setup (EpigramRef i_configuration = Epigram ())
	{
		JdResult result;
		
		if (not m_initialized)
		{
			result = m_server.Setup ();
			
			if (not result)
			{
				m_libraries.ParseLibraries (& JdLibraryFactory::Get ());
				
				if (i_configuration)
					m_server.SetConfiguration (i_configuration);
				
				for (auto moduleName : m_disabledModules)
				{
					//epilog_(normal,  "disable: %s", epistr (moduleName));
					
					m_libraries.DisableModule (moduleName);
				}
				
				if (m_timersEnabled)
				{
					result = jd_timers.Bind (& m_server);

					try
					{
//						jd_broadcaster.Bind (& m_server); // This is the notification delivery module
					}
					
					catch (...)
					{
						
					}
					
//					auto timers = JdPlatform::Get <IJdPlatform::Notification> ();
//					m_timer = timers->ScheduleTimer (10, this, 0);
				}
				
				if (not result)
				{
					auto scheduler = m_server.GetScheduler ();
					
					if (m_timersEnabled)
					{
						auto driver = jd_timers.Cast <IJdTimerDriver> ();
						
						scheduler->AddTimerThread (driver);
					}
					else scheduler->AddThreads (1);
					
					scheduler->Start ();
				}
				
				m_initialized = true;
			}
		}
		
		return result;
	}
	
	JdResult 		Teardown				();
	
	void  			RunMainThread			(f64 i_maxDurationInSeconds);
	void  			RunMainLoop				(f64 i_durationInSeconds);

	
	operator IJdModuleServer		()
	{
		return & m_server;
	}
	
	
	JdServer &						GetServer					() { return m_server; }
	
	protected ://------------------------------------------------------------------------------------------------------
	
	vector <string>					m_disabledModules;
	
	JdLibrarian						m_libraries;
	
	JdServer						m_server;
	
	IJdBroadcaster					jd_broadcaster;
	IJdTimers						jd_timers;
	
	bool							m_timersEnabled				= false;
	bool							m_initialized				= false;
};


struct JdLinkBasic		{ static void Setup (); };
struct JdLinkSimple		{ static void Setup (); };

#endif /* JdEnvironment_h */
