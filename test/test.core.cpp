#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.hpp"
#include "Epigram.hpp"



#include "JdTypeId.hpp"

test_("TypeId")
{
	char typeChar = Jd::TypeIdChar <string> ();					expect (typeChar == 's')
	u8 typeId = Jd::TypeId <string> ();							expect (typeId == c_jdTypeId::string)
}

#include "JdAssert.hpp"

test_ ("JdAssert")
{
	string msg = "ok";

	try { d_jdThrow ("failure"); }
	catch (JdResult & r) { msg = "failure"; }					expect (msg == "failure")
}

#include "Epigram.hpp"

test_ ("Epigram")
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


test_ ("JdFiberV2")
{
	for (u32 i = 0; i < 3; ++i)
	{
		JdResult result;
		JdFibers fibers;
		if (1)
		{
			auto fiber = fibers.CreateFiber <MyFiber> (32768, & result, "fiber1");
			auto fiber2 = fibers.CreateFiber <MyFiber> (32768, & result, "fiber2");
			
			fiber->SetFriend (fiber2);

			if (not result)
			{
				result = fiber->Run ();		cout << result << endl;

	//			d_jdAssert (fibers.AtHome (), "fiber state wrong");

				result = fiber2->Run ();
	//			d_jdAssert (fibers.AtHome (), "fiber state wrong");
			}
			
			fibers.ReleaseFiber (fiber);
			fibers.ReleaseFiber (fiber2);
			
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
		
		fibers.ReleaseFiber (pf);
		
		cout << "bye\n";
	}
}








