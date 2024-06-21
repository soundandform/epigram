/*
 *  Time.h
 *
 *  Created by steven massey on 8/20/09.
 *  Copyright 2009 Epigram Software LLC. All rights reserved.
 *
 */

#ifndef JDTIME_HPP
#define JDTIME_HPP

#include <boost/timer/timer.hpp>
#include "JdNucleus.hpp"


class JdTimer
{
	public:
				JdTimer				();

	void		Restart				();
	
	f64			GetSeconds			();
	
	u64			GetMilliseconds		();
	u64			GetMicroseconds		();
	u64			GetNanoseconds		();

	protected:
	boost::timer::cpu_timer			m_timer;
};



namespace Jd
{
	u64 GetMicroseconds ();
	u64 GetMilliseconds ();
	u64 GetNanoseconds ();
};

#endif
