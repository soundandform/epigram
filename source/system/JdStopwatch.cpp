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
	m_timer.Restart ();
}

JdStopwatch::JdStopwatch	(stringRef_t i_label, bool i_doDisplay)
:
m_label		(i_label),
m_display 	(i_doDisplay)
{
	m_timer.Restart ();
}


//JdStopwatch::operator bool () const
//{
//	auto self = const_cast <JdStopwatch *> (this);
//	self->m_timer.start ();
//	return true;
//}

void JdStopwatch::Start ()
{
	m_timer.Restart ();
}

f64 JdStopwatch::End ()
{
	return m_timer.GetSeconds ();
//	m_timer.stop ();
//	f64 elapsed = m_timer.elapsed ().wall;
//	elapsed /= 1e9;
//	return elapsed;
}

void  JdStopwatch::Finish ()
{
	f64 secs = m_timer.GetSeconds ();
	
	cout << setw (20) << m_label << ": " << secs << "s" << endl;
	
	m_display = false;
}


JdStopwatch::~ JdStopwatch ()
{
	if (m_display)
		Finish ();
}
	
