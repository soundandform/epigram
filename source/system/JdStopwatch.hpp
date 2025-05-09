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
	
	JdStopwatch	()
	{
		m_timer.Restart ();
	}
	
//	JdStopwatch	(std::string_view const i_label, bool i_doDisplay = true);
	JdStopwatch	(std::string_view const i_label, f64 i_thresholdForDisplayInSecs = 0.);

//	operator bool () const;
	
	void 	Start ();
	f64 	End ();
	
	void	Finish ();
	
	~ JdStopwatch ();
	
	protected:
	
	JdString256					m_label;
    bool            			m_display		= true;
	JdTimer						m_timer;
	f64							m_threshold		= 0.;
};




namespace Jd
{
	// These return seconds
	
	inline f64 MeasureTime (const std::function <void()> &i_toBeMeasured)
	{
		JdStopwatch stopwatch;
		
		stopwatch.Start ();
		i_toBeMeasured ();
		return stopwatch.End ();
	}

	inline f64 MeasureTime (u64 i_numLoops, const std::function <void (u64)> &i_toBeMeasured)
	{
		JdStopwatch stopwatch;
		
		stopwatch.Start ();
		i_toBeMeasured (i_numLoops);
		return stopwatch.End () / f64 (i_numLoops);
	}
	
	
	inline f64 MeasureTime (u64 i_numLoops, const std::function <void ()> &i_toBeMeasured)
	{
		JdStopwatch stopwatch;
		
		stopwatch.Start ();
		for (u32 i = 0; i < i_numLoops; ++i)
			i_toBeMeasured ();
		return stopwatch.End () / f64 (i_numLoops);
	}
};



#define d_jdMeasureTime(LABEL) if (const JdStopwatch cycles = JdStopwatch (LABEL, true))

# define d_jdStopwatchVar2(VAR,LINE) VAR ## LINE
# define d_jdStopwatchVar(VAR,LINE) d_jdStopwatchVar2(VAR,LINE)
# define d_jdStopwatch(...) JdStopwatch d_jdStopwatchVar(stopwatch_, __LINE__) (__VA_ARGS__);

//#define d_jdMeasureTimeIf(COND, LABEL) if (const JdAbsoluteNanos cycles = JdAbsoluteNanos (COND, LABEL))

//#define JdMeasureCycles(LABEL) if (const JdCycles cycles = JdCycles (LABEL))


#endif
