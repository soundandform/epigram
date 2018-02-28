#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.hpp"
#include "JdTypeId.hpp"
#include "JdStopwatch.hpp"
#include "JdTimer.hpp"
#include "JdAssert.hpp"


test_suite (Timers)
{
	doctest ("basic")
	{
		f64 seconds = Jd::MeasureTime ([] { sleep (1); }); 			expect (seconds > 1.) expect (seconds < 1.01)
		u64 us = Jd::GetMicroseconds ();							expect (us > 1000000);
	}
}


doctest ("TypeId")
{
	char typeChar = Jd::TypeIdChar <string> ();					expect (typeChar == 's')
	u8 typeId = Jd::TypeId <string> ();							expect (typeId == c_jdTypeId::string)
}

doctest ("JdAssert")
{
	string msg = "ok";

	try { d_jdThrow ("failure"); }
	catch (JdResult & r) { msg = "failure"; }					expect (msg == "failure")
}

#include "Epigram.hpp"

doctest ("Epigram")
{
	Epigram e;
	
	e ["value"] = 1234.567;
	f64 v = e ["value"];										expect (v == 1234.567)

	e.dump ();
}


#include "JdFiber.hpp"

class MyFiber : public IIJdFiber
{
	public:
	virtual i64					RunFiber			()
	{
		cout << "FIBER: " << fibers ()->GetName () << endl;
		fibers()->YieldTo (m_myFriend);
		cout << "EXIT : " << fibers ()->GetName () << endl;

		return 0;
	}
	
	void 						SetFriend			(IJdFiber i_friend)
	{
		m_myFriend = i_friend;
	}
	
	~MyFiber ()
	{
		cout << "fiber dead\n";
	}
	
	IJdFiber	m_myFriend = nullptr;
};


class PersistentFiber : public IIJdFiber
{
	public:
	virtual i64					RunFiber			()
	{
		while (true)
		{
			cout << value << " i'm still here: " << fibers ()->GetName () << endl;
			value += 1.234;
			
			fibers()->Yield ();
		}
		return 0;
	}
	
	~PersistentFiber () { cout << fibers()->GetName () << " dead\n"; }
	
	f64 value = 0;
};


doctest ("JdFiberV2")
{
	for (u32 i = 0; i < 3; ++i)
	{
		JdResult result;
		JdFibers fibers;
		if (1)
		{
			auto fiber1 = fibers.CreateFiber <MyFiber> (32768, & result, "fiber1");
			auto fiber2 = fibers.CreateFiber <MyFiber> (32768, & result, "fiber2");
			
			fiber1->SetFriend (fiber2);

			if (not result)
			{
				fiber1->Run (result);		cout << result << endl;

				auto r = fiber2->Run ();
			}
			
			result = fibers.ReleaseFiber (fiber1, true);	cout << result << endl; 		expect (result == c_jdNoErr)
			result = fibers.ReleaseFiber (fiber2);			cout << result << endl;			expect (result == c_jdNoErr)
			
			cout << "\n\n";
		}
		
		auto pf = fibers.CreateFiber <PersistentFiber> (8192, & result, "pam");
		
//		for (u32 i = 0; i < 1000; ++i)
//			pf->Run ();
		
		pf->Run ();
		pf->Run ();
		pf->Run ();
		pf->Run ();
		pf->Run ();
		pf->Run ();

		cout << "\n\n\n";
		pf->Terminate ();
		
		result = fibers.ReleaseFiber (pf);	expect (result == c_jdNoErr)
	}
}








