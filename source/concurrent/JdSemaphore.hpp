/*
 *  JdSemaphore.hpp
 *  Jigidesign
 *
 *  Created by Steven Massey on 11/22/11.
 *  Copyright 2011 Steven M. Massey. All rights reserved.
 *
 */
#pragma once

#include "JdNucleus.hpp"
#include <thread>

#include <condition_variable>
#include <mutex>    

class JdSemaphore
{
	public:
    explicit JdSemaphore (u64 i_initialCount = 0)
	:
	m_count				(i_initialCount)
    { }
	
	
	void LockMutex ()
	{
		m_mutex.lock ();
	}
	
	void UnlockMutex ()
	{
		m_mutex.unlock ();
	}
	
    u64 GetCount () const //for debugging/testing only
    {
        return m_count;
    }
	
    void Signal (u32 i_count = 1)
    {
		std::unique_lock <std::mutex> lock (m_mutex);

		m_count += i_count;
		
        while (i_count--)
            m_condition.notify_one ();
    }

	
    void Wait ()
    {
		std::unique_lock <std::mutex> lock (m_mutex);

		while (m_count == 0)
			m_condition.wait (lock);
		
        --m_count;
    }

	
	bool Acquire ()
	{
		std::unique_lock <std::mutex> lock (m_mutex);
		
		if (m_count)
		{
			--m_count;
			return true;
		}
		else return false;
	}

	bool AcquireAndRemainLocked ()
	{
		m_mutex.lock ();
		
		if (m_count)
		{
			--m_count;
			return true;
		}
		else
		{
			m_mutex.unlock ();
			return false;
		}
	}

	
	void WaitAndRemainLocked ()
	{
		std::unique_lock <std::mutex> lock (m_mutex);

		while (m_count == 0)
			m_condition.wait (lock);
		
		--m_count;
		
		lock.release ();
	}

	
	void WaitAll ()
	{
		std::unique_lock <std::mutex> lock (m_mutex);

		while (m_count == 0)
			m_condition.wait (lock);
		
		m_count -= m_count;
	}

	
	bool TimedWait (u32 i_microseconds)	// returns true if semaphore acquired
	{
		auto waitTime = std::chrono::microseconds (i_microseconds);
		
		std::unique_lock <std::mutex> lock (m_mutex);

		while (m_count == 0)
		{
			if (m_condition.wait_for (lock, waitTime) == std::cv_status::timeout)
				return false;
		}
		
		if (m_count)
		{
			--m_count;
			return true;
		}
		else return false;
	}

	
	bool TimedWaitAndRemainLocked (u32 i_microseconds)						// returns true if semaphore acquired
	{
		auto waitTime = std::chrono::microseconds (i_microseconds);
		
		std::unique_lock <std::mutex> lock (m_mutex);

		while (m_count == 0)
		{
			if (m_condition.wait_for (lock, waitTime) == std::cv_status::timeout)
				return false;
		}
		
		if (m_count)
		{
			--m_count;
			
			lock.release ();
			
			return true;
		}
		else return false;
	}
	
	protected:

	u64								m_count;
    std::mutex						m_mutex;
    std::condition_variable			m_condition;
};
