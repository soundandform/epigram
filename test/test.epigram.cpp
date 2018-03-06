//
//  test.epigram.cpp
//  epitest
//
//  Created by Steven Massey on 11/9/17.
//  Copyright © 2017 Steven Massey. All rights reserved.
//

#include "doctest.hpp"
#include "Epigram.hpp"

struct ObjectKey
{
	bool operator == (const ObjectKey & i_other)
	{
		return true;
	}
};

doctest ("epigram.usage")
{
	// An Epigram is a fast, compact key-value container. It was designed for message passing or as a simple data store.
	
	// An epigram is initialized with 512 bytes of storage from the stack. This makes startup and creation fast.
	Epigram e;														expect (e.GetCapacity () == 512)
	
	// keys can be strings, objects, integers, floats
	e ["string key"] = 9999;

	ObjectKey key;
	e [key] = 0x1234;
	
	e.dump ();
}



doctest ("epigram.builds")
{
	Epigram e;
	
	e ["string"] = 1234.678;
	f64 value = e ["string"];									expect (value == 1234.678)
}


doctest ("epigram.pointers")
{
	Epigram e;
	
	void * ptr = &e;
	u32 v = 293784;
	u32 * u32ptr = &v;
	
	e ("u32 *", u32ptr);
	e ("class*", &e);
	e ("ptr", ptr);
	
	Epigram *ep = e ["class*"];									expect (ep == &e)
	void * r = e ["ptr"];										expect (r == &e)
	u32 * ru = e ["u32 *"];										expect (ru == &v)
}


doctest ("epigram.empty")
{
	bool hi = jd::has_iterator <vector <string>>::value;		expect (hi)
	
	Epigram empty;
	
	vector <string> dontDie = empty ["not here"];       		expect (dontDie.size() == 0)
	string null = empty ["or here"];                    		expect (null == c_epigram::nullString)
}


doctest ("epigram.count")
{
	Epigram e;
	e ["key"] = "value";
	
//	e.dump ();
	vector <string> values = e ["key"];							expect (values.size () == 1)
	if (values.size ())											expect (values [0] == "value")
}



doctest ("epigram.strings")
{
	Epigram msg;
	
	std::string stringA = "this is string a";
	msg ("stringA", stringA);
	
	const char * stringB = "this is string b";
	msg ("stringB", stringB);
	
	const char *fetchedA = msg ["stringA"];						expect (string (fetchedA) == "this is string a");
	std::string fetchedB = msg ["stringB"];						expect (fetchedB == "this is string b");
}


doctest ("epigram.delivery")
{
	struct
	{
		void SendMessage (EpigramRef i_msg)
		{
//			i_msg.dump ();
			i_msg ["value"] >> value;
			i_msg ["other"] >> other;
		}
		
		i32		value;
		string 	other;
	}
	r;
	
	r.SendMessage ({ "value"_= 123, "other"_= "string" });			expect (r.value == 123)
																	expect (r.other == "string")
}




