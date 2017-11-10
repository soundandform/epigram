//
//  test.epigram.cpp
//  epitest
//
//  Created by Steven Massey on 11/9/17.
//  Copyright © 2017 Steven Massey. All rights reserved.
//

#include "doctest.hpp"
#include "Epigram.hpp"

#include "JdStopwatch.hpp"
#include "JdTimer.hpp"

test_suite (Timers)
{
	doctest (Basic)
	{
		f64 seconds = Jd::MeasureTime ([] { sleep (1); }); 			expect (seconds > 1.) expect (seconds < 1.01)
		u64 us = Jd::GetMicroseconds ();							expect (us > 1000000);
	}
}

test_suite (Epigram)
{
	doctest (ItBuilds)
	{
		Epigram e;
		
		e ["string"] = 1234.678;
		f64 value = e ["string"];									expect (value == 1234.678)
	}
	
	doctest (Pointers)
	{
		Epigram e;
		
		void * ptr = &e;
		u32 v = 293784;
		u32 * u32ptr = &v;
		
		e ("u32 *", u32ptr);
		e ("class*", &e);
		e ("ptr", ptr);
		
		Epigram *ep = e ["class*"];									expect (ep == &e);
		void * r = e ["ptr"];										expect (r == &e);
		u32 * ru = e ["u32 *"];										expect (ru == &v);
	}
	
}
