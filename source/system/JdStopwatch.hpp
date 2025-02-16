//
//  JdStopwatch.hpp
//
//  Created by Steven Massey on 10/18/12.
//  Copyright (c) 2012 Jigidesign. All rights reserved.
//

#ifndef JdStopwatch_h
#define JdStopwatch_h

#include <iomanip>
#include <iostream>
# include <functional>

# include "JdNucleus.hpp"
# include "JdTimer.hpp"
# include "JdFlatString.hpp"

class JdStopwatch
{
	public:
	
	JdStopwatch	(std::string_view const i_label, bool i_doDisplay = true);

//	operator bool () const;
	
	void 	Start ();
	f64 	End ();
	
	void	Finish ();
	
	~ JdStopwatch ();
	
	protected:
	
	JdString256					m_label;
    bool            			m_display;
	JdTimer						m_timer;
};


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
#define d_jdStopwatch(LAYBUL) JdStopwatch _ (LAYBUL);
//#define d_jdMeasureTimeIf(COND, LABEL) if (const JdAbsoluteNanos cycles = JdAbsoluteNanos (COND, LABEL))

//#define JdMeasureCycles(LABEL) if (const JdCycles cycles = JdCycles (LABEL))


#endif
