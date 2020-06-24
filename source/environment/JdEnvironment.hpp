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
	
	JdEnvironment				(bool i_enableTimers = false)
	:
	m_server					(m_libraries),
	m_timersEnabled				(i_enableTimers)
	{
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
					try
					{
//						jd_broadcaster.Bind (& m_server); // This is the notification delivery module
						jd_timers.Bind (& m_server);
					}
					
					catch (...)
					{
						
					}
					
//					auto timers = JdPlatform::Get <IJdPlatform::Notification> ();
//					m_timer = timers->ScheduleTimer (10, this, 0);
				}
				
				auto scheduler = m_server.GetScheduler ();
				
				if (m_timersEnabled)
				{
					auto driver = jd_timers.Cast <IJdTimerDriver> ();
					
					scheduler->AddTimerThread (driver);
				}
				
//				scheduler->AddThreads (1);
				
				scheduler->Start ();
				
				m_initialized = true;
			}
		}
		
		return result;
	}
	
	
	void Teardown ()
	{
		JdResult result;
		
		if (m_timersEnabled)
		{
			jd_lock (jd_timers)
			{
				jd_timers->StopAllTimers ();
			}
		}
		
		jd_timers.Release ();
		
		result = m_server.GetScheduler()->Teardown ();
		
		m_server.Teardown ();
		
#if FIX
		jd_broadcaster.ForceRelease ();	// teardown broadcaster last
#endif
		
		m_server.Teardown (true);
		
		m_initialized = false;
	}
	
	
	void  			RunMainThread			(f64 i_maxDurationInSeconds);
	void  			RunMainLoop				(f64 i_durationInSeconds);

	
	operator IJdModuleServer		()
	{
		return & m_server;
	}
	
	//	IJdModuleServer					GetServer					() { return & m_server; }
	
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























#if 0
class JdEnvironmentX
{
	public:
									JdEnvironmentX			(Epigram i_configuration = Epigram ())
	:
	m_configuration					(i_configuration)
	{
		m_server = JdServerX::Get ();
		
		m_setupResult = m_server->Setup ();
		
		if (m_server)
		{
//			m_mainQueue = m_server->m_scheduler.GetMainThreadQueue ();
		}
		
		jd_notifications = m_server->AcquireModule ("JdNotifications");

		if (not jd_notifications)
			d_jdThrow ("couldn't connect to JdNotifications");
		
		bool runTimers = true;
		
		if (runTimers)
		{
			auto timerQueue = m_server->AcquireQueue ("timers");

			jd_timers = m_server->AcquireModule ("JdTimers");

			m_timerThread->Connect (jd_timers.GetDirectInterface (), timerQueue);

			if (not jd_timers)
				d_jdThrow ("couldn't connect to JdTimers");

			jd_timers.BindToQueue (timerQueue);
			jd_notifications.BindToQueue (timerQueue);

			m_timerThread.Start ();
		}
	}
	
	virtual							~ JdEnvironmentX			()
	{
		Jd::EnforceMainThread ();
		
		m_mainQueue->Run (10.);		// FIX: need a Flush () method
		
		epilog (normal, "~JdEnvironment -----------------------------------------------------");

#if 0
		jd_notifications.send <JdE::StopAllNotifications> ();
		jd_timers.send <JdE::StopAllTimers> ();
#endif
		m_timerThread.Stop ();

		jd_notifications.Release ();
		jd_timers.Release ();

		if (m_server)
			m_server->Teardown ();
		
		JdServerX::Reset ();	// for Unit tests
	}
	
	void							Run						()
	{
		m_mainQueue->Run (.01);
	}
	
	
	IJdEnvironmentServer 			GetServer				()		{ return m_server; }
	
	IJdEnvironmentServer			operator ->				()		{ return m_server; }
	
	
//	u32								TriggerTimers			()
//	{
////		return jd_timers.call <JdE::RunTimers> ();
//		return jd_timers->RunTimers ();
//	}

	
	struct TimerDriver : public IJdThread
	{
		void						Connect				(IJdTimers i_timers, IJdServerQueue i_timerQueue)
		{
			m_timers = i_timers;
			m_timerQueue = i_timerQueue;
		}
	
		virtual JdResult                Run             (EpigramRef i_args, IJdThread::ThreadInfo & i_info)
		{
//			JdScheduler::SetCurrentThreadQueue (m_timerQueue);
			
			u32 sleepIntervalInMicroseconds = 1000;
			
			u32 wakes = 0;
			while (i_info.IsAlive ())
			{
				m_timerQueue->WaitRun (sleepIntervalInMicroseconds * 1e-6);
				++ wakes;
				
				sleepIntervalInMicroseconds = m_timers->RunTimers ();
			}

			m_timerQueue->Run (.1);
			
			return c_jdNoErr;
		}
		
		virtual JdResult                Break           ()
		{
			m_timerQueue->Exit (); // not sure this matters
			return c_jdNoErr;
		}

		//		virtual JdResult                Finalize        (const JdResult &i_runResult) { return i_runResult; }
		//		virtual JdResult                Teardown        (const JdResult &i_runResult) { return i_runResult; }
		
		f64							m_sleepInterval				= .25;
		IJdServerQueue				m_timerQueue				= nullptr;

		IJdTimers					m_timers					= nullptr;
	};
	
	
	protected: //--------------------------------------------------------------------------------------------
	Epigram							m_configuration;
	
	JdServerX *						m_server;
	JdSchedulerXQueue *				m_mainQueue					= nullptr;
	
	JdModuleXT <IJdTimers>			jd_timers;
	JdModule						jd_notifications;
	
//	JdModule						jd_moduleManager;
//	JdModule						jd_factory;
	
	JdResult						m_setupResult;
	
	JdThreadT <TimerDriver>			m_timerThread				{ "timers" };
};
#endif

#endif /* JdEnvironment_h */
