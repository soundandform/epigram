/*
 *  Time.h
 *
 *  Created by steven massey on 8/20/09.
 *  Copyright 2009 Epigram Software LLC. All rights reserved.
 *
 */

#ifndef JDTIME_HPP
#define JDTIME_HPP

#include "JdNucleus.hpp"

# if d_jdTimerUsesBoost
#   include <boost/timer/timer.hpp>
# else
#   include <chrono>
# endif

class JdTimer
{
	public:     JdTimer				();

	void		Restart				();
	
	f64			GetSeconds			();
	
	u64			GetMilliseconds		();
	u64			GetMicroseconds		();
	u64			GetNanoseconds		();

	protected:

#   if d_jdTimerUsesBoost
        boost::timer::cpu_timer                 m_timer;
#   else
        std::chrono::steady_clock::time_point   m_start;
#   endif
};



namespace Jd
{
	u64 GetMicroseconds ();
	u64 GetMilliseconds ();
	u64 GetNanoseconds ();
};

#endif
