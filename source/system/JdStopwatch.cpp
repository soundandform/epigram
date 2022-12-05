//
//  JdStopwatch.cpp
//  m3test
//
//  Created by Steven Massey on 10/22/18.
//  Copyright © 2018 Steven Massey. All rights reserved.
//

#include "JdStopwatch.hpp"

using namespace std;

JdStopwatch::JdStopwatch	(cstr_t i_label, bool i_doDisplay)
:
m_label		(i_label),
m_display 	(i_doDisplay)
{
	Start ();
}

JdStopwatch::JdStopwatch	(stringRef_t i_label, bool i_doDisplay)
:
m_label		(i_label),
m_display 	(i_doDisplay)
{
	Start ();
}


JdStopwatch::operator bool () const
{
	auto self = const_cast <JdStopwatch *> (this);
	self->m_timer.start ();
	return true;
}

void JdStopwatch::Start ()
{
	m_timer.start ();
}

f64 JdStopwatch::End ()
{
	m_timer.stop ();
	f64 elapsed = m_timer.elapsed ().wall;
	elapsed /= 1e9;
	return elapsed;
}

void  JdStopwatch::Finish ()
{
	m_timer.stop ();
	
	string t = m_timer.format (boost::timer::default_places, "%w"); // don't seem to get user/system time on mac
	cout << setw (14) << m_label << ": " << t << "s" << endl;
	
	m_display = false;
}


JdStopwatch::~ JdStopwatch ()
{
	if (m_display)
		Finish ();
}
	
