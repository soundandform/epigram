/*
 *  Time.cpp
 *
 *  Created by steven massey on 8/20/09.
 *  Copyright 2009 Epigram Software LLC. All rights reserved.
 *
 */

#include <iostream>
#include "JdTimer.hpp"

using namespace std;

JdTimer::JdTimer ()
{
#   if d_jdTimerUsesBoost
        m_timer.start ();
#   else
        m_start = chrono::high_resolution_clock::now ();
#   endif
}


void JdTimer::Restart()
{
#   if d_jdTimerUsesBoost
        m_timer = boost::timer::cpu_timer ();
#   else
        m_start = chrono::high_resolution_clock::now ();
#   endif
}



f64 JdTimer::GetSeconds ()
{
	return GetNanoseconds () * 1e-9;
}


u64 JdTimer::GetMilliseconds ()
{
#   if d_jdTimerUsesBoost
        return m_timer.elapsed ().wall / 1000000;
#   else
        auto diff = chrono::high_resolution_clock::now () - m_start;
        return chrono::duration_cast <chrono::milliseconds> (diff).count ();
#   endif
}


u64 JdTimer::GetMicroseconds ()
{
#   if d_jdTimerUsesBoost
        return m_timer.elapsed ().wall / 1000;
#   else
        auto diff = chrono::high_resolution_clock::now () - m_start;
        return chrono::duration_cast <chrono::microseconds> (diff).count ();
#   endif
}

u64 JdTimer::GetNanoseconds ()
{
#   if d_jdTimerUsesBoost
        return m_timer.elapsed ().wall;
#   else
        auto diff = chrono::high_resolution_clock::now () - m_start;
        return chrono::duration_cast <chrono::nanoseconds> (diff).count ();
#   endif
}



namespace Jd
{
	static JdTimer s_jdTimer;
	
	u64 GetMicroseconds ()
	{
		return s_jdTimer.GetMicroseconds ();
	}
	
	u64 GetMilliseconds()
	{
		return s_jdTimer.GetMilliseconds ();
	}

	u64  GetNanoseconds  ()
	{
		return s_jdTimer.GetNanoseconds ();
	}

} // end-namespace Jd
