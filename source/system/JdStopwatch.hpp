//
//  JdStopwatch.hpp
//  Jigidesign
//
//  Created by Steven Massey on 10/18/12.
//  Copyright (c) 2012 Jigidesign. All rights reserved.
//

#ifndef Jigidesign_JdCycles_h
#define Jigidesign_JdCycles_h

#include <iomanip>
#include <iostream>

#include <boost/timer/timer.hpp>
#include "JdNucleus.hpp"

class JdStopwatch
{
	public:
	
	JdStopwatch	(cstr_t i_label, bool i_display = false)
	:
	m_label		(i_label),
	m_display 	(i_display)
	{ }
	
	operator bool () const
    {
		auto self = const_cast <JdStopwatch *> (this);
		self->m_timer.start ();
        return true;
    }
	
	void Start ()
	{
		m_timer.start ();
	}
	
	f64 End ()
	{
		m_timer.stop ();
		f64 elapsed = m_timer.elapsed ().wall;
		elapsed /= 1e9;
		return elapsed;
	}
	
	~ JdStopwatch ()
	{
		m_timer.stop ();

		if (m_display)
		{
			string t = m_timer.format (boost::timer::default_places, "%w"); // don't seem to get user/system time on mac
			cout << setw (14) << m_label << ": " << t << "s" << endl;
		}
	}
	
	protected:
	
	cstr_t		 				m_label;
    bool            			m_display;
	boost::timer::cpu_timer		m_timer;
};
//
//
//class JdTimeFormatter
//{
//	public:
//	JdTimeFormatter (f64 i_seconds) : m_seconds (i_seconds)
//	{
//	}
//	
//	string			ToString () const
//	{
//		ostringstream oss;
//		
//		cstr_t postfix [4] = { "nsec", "usec", "msec", "secs" };
//		
//		for (u32 i = 0; i < 4; ++i)
//		{
//			f64 multiplier = pow (10., 9 - i * 3);
//			
//			f64 value = multiplier * m_seconds;
//			if (i == 3 || value < 1000.)
//			{
//				if (i < 3) oss <<setprecision (4);
//				oss << value << " " << postfix [i];
//				break;
//			}
//		}
//		
//		return oss.str ();
//	}
//		
//	protected:
//	f64 m_seconds;
//};
//
//inline
//std::ostream & operator << (std::ostream &output, const JdTimeFormatter &i_time)
//{
//	output << i_time.ToString ();
//	return output;
//}



namespace Jd
{
	// These return seconds
	
	inline f64 MeasureTime (const std::function <void()> &i_toBeMeasured)
	{
		JdStopwatch stopwatch (nullptr);
		
		stopwatch.Start ();
		i_toBeMeasured ();
		return stopwatch.End ();
	}

	inline f64 MeasureTime (u64 i_numLoops, const std::function <void (u32)> &i_toBeMeasured)
	{
		JdStopwatch stopwatch (nullptr);
		
		stopwatch.Start ();
		i_toBeMeasured (i_numLoops);
		return stopwatch.End () / f64 (i_numLoops);
	}
	
	
	inline f64 MeasureTime (u64 i_numLoops, const std::function <void ()> &i_toBeMeasured)
	{
		JdStopwatch stopwatch ( nullptr);
		
		stopwatch.Start ();
		for (u32 i = 0; i < i_numLoops; ++i)
			i_toBeMeasured ();
		return stopwatch.End () / f64 (i_numLoops);
	}
};


#define d_jdMeasureTime(LABEL) if (const JdStopwatch cycles = JdStopwatch (LABEL, true))
#define d_jdStopwatch(LABEL) if (const JdStopwatch cycles = JdStopwatch (LABEL, true))
//#define d_jdMeasureTimeIf(COND, LABEL) if (const JdAbsoluteNanos cycles = JdAbsoluteNanos (COND, LABEL))

//#define JdMeasureCycles(LABEL) if (const JdCycles cycles = JdCycles (LABEL))


#endif
