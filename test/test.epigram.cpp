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


doctest ("epigram.iterate")
{
	Epigram e;
	e ["key"] = "value";
	e [0] = "other1";
	e ["key2"] = "value23";
	
	for (auto & kv : e)
	{
		cout << kv.GetKeyString () << endl;
	}
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





template <typename ... types_t>
struct JdThreadedObjPool
{
	// reuses objects to avoid (but not eliminate) allocating in realtime threads
	// also manages lifetimes so that "garbage collection" can happen in main or bookkeeping thread
	
	template <typename ... T>
	vector <size_t> GetHashCodes  ()
	{
		return { typeid (T).hash_code ()... };
	}

	static constexpr size_t SharedPtrSize ()	// always 16 on clang
	{
		size_t size = 0;
		auto s = { sizeof (shared_ptr <types_t>)... };
		
		for (auto i : s)
			size = std::max (i, size);
		
		return size;
	}

	
	void  Setup  ()
	{
		SharedPtrSize ();
		
		auto codes = GetHashCodes <types_t...> ();
		u32 tableSize = codes.size ();
		
		set <u32> collisions;
		
		retry: collisions.clear ();
		
		for (auto hc : codes)
		{
			auto m = hc % tableSize;

			if (collisions.count (m))
			{
				++tableSize;
				goto retry;
			}
			
			collisions.insert (m);
		}
		
		jd::out ("tablesize: @", tableSize);
		
		m_objList.resize (tableSize);
		

		for (auto hc : codes)
		{
			auto m = hc % m_objList.size ();
			
			m_objList [m] = { new mutex, hc };
		}
	}


	JdThreadedObjPool ()
	{
		jd::out ("obj pool");
		
		Setup ();
	}
	
	
	template <typename T, typename ... Args>
	shared_ptr <T>  New  (Args && ... i_args)
	{
		shared_ptr <T> obj;
		
		size_t hashCode = typeid (T).hash_code ();
		u32 index = hashCode % m_objList.size ();
		
		auto & objs = m_objList [index];		 		d_jdAssert (hashCode == objs.hashCode, "object type not added to JdThreadedObjPool <Types>");
		{
			mutex_lock l (* objs.lock);
			
			if (objs.sharedPtrs.empty ())
			{
				
			}
		}
		
		if (not obj)
		{
			obj = make_shared <T> (forward <Args> (i_args)...);
		}
		
		return obj;
	}
	
	
	struct PooledObjs
	{
		mutex *					lock								= nullptr;		// todo delete
		size_t					hashCode							= 0;
	
		struct SharedOpaque
		{
			u8 						sharedPtr		[SharedPtrSize ()]	= {};
		};
		
		list <SharedOpaque>		free;
	};

	
	vector <PooledObjs>					m_objList;
};
	
	

struct TypeA
{
	
};


struct TypeB
{
	TypeB (stringRef_t i_name) {}
};

struct TypeC
{
	
};

struct TypeD
{
	
};


doctest ("epigram.objpool")
{
	JdThreadedObjPool <TypeA, TypeB, TypeC> pool;
	
	
	pool.New <TypeB> ("type B");
	
	
}



