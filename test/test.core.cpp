#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.hpp"


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

//#include "JdFiber.hpp"
//
//namespace ctx = boost::context::detail;
////namespace ctx = boost::context;
//
//ctx::fcontext_t f1, f2;
//
//ctx::fcontext_t home;
//
//void Fiber (ctx::transfer_t i)
//{
//	home = i.fctx;
//
//	cout << "f1: " << i.fctx << endl;
//
//	cout << "hi I'm a fiber?\n";
//	cout << i.data << endl;
//
//	ctx::jump_fcontext (f2, nullptr);
//
//	cout << "uhg\n";
//}
//
//void Fiber2 (ctx::transfer_t i)
//{
//	cout << "f2: " << i.fctx << endl;
//
////	cout << i.data << endl;
//
//	ctx::jump_fcontext (home, nullptr);
//
//	cout << "2 is dead\n";
//}
//
//test ("boost fcontext")
//{
//	cout << sizeof (ctx::fcontext_t) << endl;
//
//	auto stack = (u8 *) malloc (32768) + 32768;
//	auto stack2 = (u8 *) malloc (32768) + 32768;
//
//	cout << "stack: " << (voidptr_t) stack << ", " << (voidptr_t) stack2 << endl;
//
//	f1 = ctx::make_fcontext (stack, 32768, Fiber);
//	f2 = ctx::make_fcontext (stack2, 32768, Fiber2);
//
//	cout << (u8*) stack - (u8*) f1  << endl;
//
//	cout << f1 << ", " << f2 << endl;
//
//	u32 hi;
//
//	cout << & hi << endl;
//
//	ctx::transfer_t t = ctx::jump_fcontext (f1, (void *) 6666);
//
//	cout << "t.fctx: " << t.fctx << endl;
//
//	cout << "returned\n";
//
////	ctx::jump_fcontext (f1, (void *) 6666);
////	ctx::jump_fcontext (f1, (void *) 6666);
//}

#include "JdFiber.hpp"

class MyFiber : public IIJdFiber
{
	public:
	virtual i64					RunFiber			()
	{
		cout << "FIBER: " << fibers ()->GetName () << endl;
		
		fibers()->Yield (m_myFriend);

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
		if (0)
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








