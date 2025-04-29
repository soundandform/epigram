//
//  JdStopwatch.cpp
//  m3test
//
//  Created by Steven Massey on 10/22/18.
//  Copyright © 2018 Steven Massey. All rights reserved.
//

#include "JdStopwatch.hpp"

using namespace std;

JdStopwatch::JdStopwatch	(string_view const i_label, bool i_doDisplay)
:
m_label		(i_label),
m_display 	(i_doDisplay)
{
	m_timer.Restart ();
}


void JdStopwatch::Start ()
{
	m_timer.Restart ();
}

f64 JdStopwatch::End ()
{
	return m_timer.GetSeconds ();
}

void  JdStopwatch::Finish ()
{
	f64 time = m_timer.GetSeconds ();
	cstr_t unit = "secs";
	
	if (time < 1.)
	{
		time *= 1000.; unit = "ms";
		
		if (time < 1.)
		{
			time *= 1000.; unit = "µs";
			
			if (time < 1.)
			{
				time *= 1000.; unit = "ns";
			}
		}
	}
	
	printf ("%20s: %f %s\n", m_label.cString (), time, unit);
	
	m_display = false;
}


JdStopwatch::~ JdStopwatch ()
{
	if (m_display)
		Finish ();
}
	
