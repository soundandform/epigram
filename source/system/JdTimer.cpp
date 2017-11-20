/*
 *  Time.cpp
 *
 *  Created by steven massey on 8/20/09.
 *  Copyright 2009 Epigram Software LLC. All rights reserved.
 *
 */

#include <iostream>
#include <sys/time.h>
#include "JdTimer.hpp"

using namespace std;


JdTimer::JdTimer ()
{
	m_timer.start ();
}


void JdTimer::Restart()
{
	m_timer = boost::timer::cpu_timer ();
}


u64 JdTimer::GetSeconds ()
{
	return m_timer.elapsed ().wall / 1000000000;
}


u64 JdTimer::GetMilliseconds ()
{
	return m_timer.elapsed ().wall / 1000000;
}


u64 JdTimer::GetMicroseconds ()
{
	return m_timer.elapsed ().wall / 1000;
}

u64 JdTimer::GetNanoseconds ()
{
	return m_timer.elapsed ().wall;
}


f64 JdTimer::GetTimeInSeconds ()
{
	return f64 (m_timer.elapsed ().wall) * 1e-9;
}


namespace Jd
{
	static JdTimer s_jdTimer;
	
	u64 GetMicroseconds ()
	{
		return s_jdTimer.GetMicroseconds ();
	}
	
} // end-namespace Jd
