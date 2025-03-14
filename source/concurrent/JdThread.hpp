//
//  JdThread.hpp
//  Jigidesign
//
//  Created b1y Steven Massey on 11/10/12.
//  Copyright (c) 2012 Jigidesign. All rights reserved.
//

#ifndef __Jigidesign__JdThread__
#define __Jigidesign__JdThread__

#include "Epilog.hpp"
#include "Epigram.hpp"
#include "JdSemaphore.hpp"
#include "JdEnum.hpp"
#include <thread>
#include <pthread.h>

//#define d_epilogLibBuild 0

/*------------------------------------------------------------------------------------------------------------------------------------------------

 How to use:
 
 1. Derive from IJdThread
 2A. Implement Run ().  Use IsAlive () to test whether the thread loop should continue executing.  Or:
 2B. Implement Break () if necessary to signal your Run () loop that it should return.
 3. Optionally, implement Terminated () for any post-Run () cleanup
 
 IsAlive () is the default "signaling" method, but does no good if the thread run loop blocks on a semaphore.  So, alternatively,
 	a. use a timed semaphore 
 	b. send the proper escape signal/message to Run() in the Break() override.
 
------------------------------------------------------------------------------------------------------------------------------------------------*/

d_jdEnum_ (ThreadState, pending, initializing, running, quiting, quit, zombie);


/*
 	TODO: maybe refactor like the Fiber classes
 
 	Create IJdThread.

 	Then, to implement a thread:
		1) implement PortHandler protocol, with macro helpers
 		2) implement IJdThread while also inheriting from PortHandler.  Fill in port PortHandler methods.
 
 	And, instatiate like so:
 	JdThread <MyThread, PortHandlers> thread (128, "name");
 	thread.Start();
 	
	 JdThread <MyThread, PortHandlers>::producer_t producer = thread.CreateProducer ();
	producer.SetSomething (8838);
 
 	thread.Stop();
 
 */


//FIX: make m_state atomic with CAS : not necessary..?

const u8 c_jdThread_defaultPriority = 0;



struct IJdThread
{
    struct Info
    {
		virtual ~					Info			() {}
        virtual bool                IsAlive         () = 0;
		virtual cstr_t				GetName			() const = 0;
    };
    
    virtual ~                       IJdThread       () { };
    virtual JdResult                Setup           () 												{ return c_jdNoErr; }		// Setup happens in creator thread (which is probably main)
    virtual JdResult                Run             (EpigramRef i_args, IJdThread::Info & i_info)	= 0;
    virtual void	                Break           () 												{ }
	virtual JdResult                Finalize        (const JdResult &i_runResult) 					{ return i_runResult; }		// Finalize happens in-thread
    virtual JdResult                Teardown        (const JdResult &i_runResult)					{ return i_runResult; }		// Teardown happens in creator thread (which is probably main)
};


template <typename t_thread>
class JdThreadT
{
    static void Runner (JdThreadT * i_owner, IJdThread * i_implementation, u8 i_priority)
    {
#		if __APPLE__
			pthread_setname_np (i_owner->m_threadName.c_str ());
#		else
			pthread_setname_np (pthread_self (), i_owner->m_threadName.c_str ());
#		endif
				
		// set thread priority
		if (i_priority != c_jdThread_defaultPriority)
		{
			i32 policy;
//			policy = SCHED_OTHER;
//			policy = SCHED_RR;
			policy = SCHED_FIFO;
			
			// TODO: read this. https://developer.apple.com/library/archive/documentation/Darwin/Conceptual/KernelProgramming/scheduler/scheduler.html#//apple_ref/doc/uid/TP30000905-CH211-TPXREF109
			
			// these don't seem to be correct. returns max of 47. can set to 127 without failure
//			i_priority = min ((u8) sched_get_priority_max (policy), i_priority);
//			i_priority = max ((u8) sched_get_priority_min (policy), i_priority);

			sched_param sp = {};
			sp.sched_priority = i_priority;

			if (pthread_setschedparam (pthread_self (), policy, &sp))
			{
				#ifndef d_epilogLibBuild
					epilog_func (detail, "set priority of @ failed for '@'", (u32) sp.sched_priority, i_owner->m_threadName);
				#endif
			}
			else
			{
				#ifndef d_epilogLibBuild
					epilog_func (detail, "'@' set to priority: @", i_owner->m_threadName, (u32) i_priority);
				#endif
			}
		}

		if (false)
		{
//			cout << "min: " << sched_get_priority_min (SCHED_OTHER) << " max: " << sched_get_priority_max (SCHED_OTHER) << endl;

			struct sched_param sp;
			int policy;
			pthread_getschedparam (pthread_self (), &policy, & sp);
			
			std::cout << "policy: " << policy << " priority: " << sp.sched_priority << std::endl;
		}
		
//		i_owner->m_state = c_jdThreadState::running;
		
		struct ThreadInfo : IJdThread::Info
		{
			ThreadInfo (EJdThreadState &i_state, cstr_t i_name)
			: m_state (i_state), m_name (i_name)												{ }
			
			virtual bool				IsAlive ()
			{
				// TODO: could sleep here for paused threads
				
				return m_state == c_jdThreadState::running;
			}
			
			virtual cstr_t				GetName () const
			{
				return m_name;
			}

			EJdThreadState &			m_state;
			cstr_t						m_name;
		}
		info (i_owner->m_state, i_owner->m_threadName.c_str ());
		
		JdResult runResult;

		i_owner->m_threadReady.Signal();

		runResult = i_implementation->Run (i_owner->m_args, info);
		runResult = i_implementation->Finalize (runResult);
		i_owner->Exited (runResult);
    }
	
    public:
    
    JdThreadT                   (stringRef_t i_threadName = "unnamed", u8 i_priority = c_jdThread_defaultPriority /* 0-63 */)
    :
	m_priority					(i_priority),
	m_threadName				(i_threadName)
	{
        m_implementation = new t_thread;
	}
	
    void                    SetPriority     (u8 i_priority)
    {
		if (i_priority > 0 && i_priority < 100)
			m_priority = i_priority;
    }
	
	virtual 				~JdThreadT ()
	{
		Stop ();
		d_jdAssert (m_state == c_jdThreadState::pending || m_state == c_jdThreadState::quit, "thread wasn't quit");
        
        delete m_implementation;
	}
	
	JdResult				Reset		()
	{
		JdResult result;
		
		if (m_state != c_jdThreadState::pending)
		{
			result = Stop ();
			
			if (not result)
			{
				if (m_state == c_jdThreadState::quit)
				{
					delete m_implementation;
					
					m_implementation = new t_thread;
					m_state = c_jdThreadState::pending;
					m_exited = false;
					m_runResult = c_jdNoErr;
				}
				else result = d_jdError ("can't reset a thread not quit");
			}
		}
		
		return result;
	}
	
	JdResult				Start		(EpigramRef i_args = Epigram ())
	{
        d_jdAssert (m_implementation, "thread impl is null");
        
		m_args = i_args;
		
        JdResult result = m_implementation->Setup ();
        if (result) return result;
        
		if (m_state == c_jdThreadState::pending)
		{
//			m_state = c_jdThreadState::initializing;
			
			#ifndef d_epilogLibBuild
				epilog (detail, "start: '@'", m_threadName);
			#endif
			
			m_state = c_jdThreadState::running;
			
			m_stdThread = std::thread (Runner, this, m_implementation, m_priority);
			m_threadReady.Wait ();	// NOTE: don't remember why i needed this, exactly. i guess it doesn't hurt
		}
		else
		{
			d_jdThrow ("Attempting to start thread '@' which is already in state: @", m_threadName, Jd::ToString (m_state));
		}
		
		return c_jdNoErr;
	}

    t_thread *				operator -> ()
    {
        return static_cast <t_thread *> (m_implementation);
    }

	
	t_thread *				Get ()
    {
        return static_cast <t_thread *> (m_implementation);
    }
	
	bool					IsRunning ()
	{
		return (m_state == c_jdThreadState::running and not m_exited);
	}

	bool					DidExit ()
	{
		if (m_exited)
		{
			
		}
		
		return m_exited;
	}
	
	
	JdResult				CancelAndRestart	()
	{
		JdResult result;
		
		m_state = c_jdThreadState::zombie;
		pthread_cancel (m_stdThread.native_handle ());

		result = m_implementation->Teardown (m_runResult);
		delete m_implementation;
		m_implementation = nullptr;
		
		if (not result)
		{
			m_state = c_jdThreadState::pending;
			auto args = m_args;
			
			Start (args);
		}
		
		return result;
	}
	

	JdResult				Stop 				(u32 i_waitToExitMicroseconds = 5'000'000)
	{
		JdResult result;
		
        if (m_state != c_jdThreadState::running)
			return d_jdError1 ("Thread not running.");
		
        m_state = c_jdThreadState::quiting;

		auto p = static_cast <IJdThread *> (m_implementation);
		p->Break ();

		u32 sleepTime = i_waitToExitMicroseconds / 1000;
		
        u32 breakAttempts = 0;
        while (not DidExit () and breakAttempts++ < 1000)
        {
//			if (not result)
//				break;
			
			std::this_thread::sleep_for (std::chrono::microseconds (sleepTime));
        }
		
		if (not DidExit ())
		{
			m_state = c_jdThreadState::zombie;
			pthread_cancel (m_stdThread.native_handle ());

			result = m_implementation->Teardown (m_runResult);
		}
		else //if (not result)
		{
			m_stdThread.join ();   // FIX: could use do_try_join_until () then report zombie if fail
			m_state = c_jdThreadState::quit;
			
			result = m_implementation->Teardown (m_runResult);
		}
//		else m_state = c_jdThreadState::zombie;
		
		return result;
	}
	
    private: // internals ---------------------------------------------------------------------------------------
	void					Exited (const JdResult &i_runResult)
	{
		m_runResult = i_runResult;
		m_exited = true;
		
		#ifndef d_epilogLibBuild
			epilog (detail, "'@' exited", m_threadName);
		#endif
	}
	

    t_thread *          m_implementation		= nullptr;
	std::thread			m_stdThread;
	u32					m_priority;
	
	EJdThreadState		m_state					= c_jdThreadState::pending;
	JdResult			m_runResult;
	bool				m_exited				= false;
	
	std::string			m_threadName;
	JdSemaphore			m_threadReady;
	Epigram				m_args;
};


template <typename t_thread>
class JdThreadPool
{
	public:
	
	virtual						~ JdThreadPool				()
	{
		for (auto thread : m_threads)
		{
			thread->Stop ();
			delete thread;
		}
	}
	
	JdThreadT <t_thread> *		CreateThread				(stringRef_t i_name = "")
	{
		GroomDeadThreads ();
		
		auto thread = new JdThreadT <t_thread> (i_name.c_str ());
	
		m_threads.insert (thread);
		
		return thread;
	}
	
	JdResult					StopAllThreads				()
	{
		JdResult result;
		
		for (auto thread : m_threads)
		{
			result |= thread->Stop ();
		}
		
		return result;
	}
	
	protected:
	
	JdResult					GroomDeadThreads			()
	{
		for (auto i = m_threads.begin(), e = m_threads.end (); i != e;)
		{
			auto thread = *i;
			
			if (thread->DidExit ())
			{
				#ifndef d_epilogLibBuild
					epilog (normal, "purging dead thread");
				#endif
				thread->Stop ();
				delete thread;
				i = m_threads.erase (i);
			}
			else ++i;
		}
		
		return c_jdNoErr;
	}
	
	std::set <JdThreadT <t_thread> *>		m_threads;
};


class JdSingleThreadEnforcer
{
	public:

	void						Enforce						()
	{
		if (m_id == std::thread::id	())
		{
			m_id = std::this_thread::get_id ();
		}
		else
		{
			if (m_id != std::this_thread::get_id())
			{
				d_jdThrow ("can't call this code from multiple threads. thank you, come again.");
			}
		}
	}
	
	bool						IsEnforcedThread			()
	{
		d_jdAssert (m_id != std::thread::id	(), "Enforce () should be called at least once prior to IsEnforcedThread ()");
		
		return  (m_id == std::this_thread::get_id());
	}

	protected:
	std::thread::id				m_id;
};

static const std::thread::id 	s_mainThreadId = std::this_thread::get_id ();

namespace Jd
{
	void			EnforceMainThread		();
	bool			IsMainThread			();
}


#endif /* defined(__Jigidesign__JdThread__) */
