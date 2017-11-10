/*
 *  Time.h
 *
 *  Created by steven massey on 8/20/09.
 *  Copyright 2009 Epigram Software LLC. All rights reserved.
 *
 */

#ifndef EPTIME_H
#define EPTIME_H

#include <boost/timer/timer.hpp>
#include "JdNucleus.hpp"

class JdTimer
{
	
	public:
				JdTimer				();

	void		Restart				();
	
	u64			GetSeconds			();
	u64			GetMilliseconds		();
	u64			GetMicroseconds		();
	u64			GetNanoseconds		();

	f64			GetTimeInSeconds	();
	
	protected:
	boost::timer::cpu_timer			m_timer;
};


static JdTimer s_jdTimer;


namespace Jd
{
//	u64 GetTimestamp ();
	u64 GetMicroseconds ();
};

#endif
