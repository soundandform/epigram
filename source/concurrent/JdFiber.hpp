//
//  JdFiber.hpp
//
//  Created by Steven Massey on 5/30/12.
//  Copyright (c) 2012 Steven Massey. All rights reserved.
//

#ifndef __JdFiber_hpp__

#include <boost/context/all.hpp>
#include <list>

#include "JdAssert.hpp"


d_jdForwardInterface (IJdFiber);

d_jdInterface (IJdFiberControl)
{
	virtual string				GetName				() = 0;
	virtual bool				IsRunnable			() = 0;
	virtual JdResult			Run					() = 0;
	virtual void				Yield				(IJdFiber i_toFiber = nullptr) = 0;
	virtual void				YieldTo				(IJdFiber i_toFiber) = 0;
	virtual JdResult			Terminate			() = 0;
};

class JdFibers;

d_jdInterface (IJdFiber)
{
	public: friend JdFibers;

	virtual						~ IIJdFiber 		() {}
	
	JdResult					Run					()
	{
		return m_controller->Run ();
	}

	JdResult					Terminate			()
	{
		return m_controller->Terminate ();
	}

	bool						IsRunnable			()
	{
		return m_controller->IsRunnable ();
	}

	protected:
	
	virtual i64					RunFiber			() = 0;

	
	IJdFiberControl 			fibers				()
	{
		return m_controller;
	}
	
	private:
	
	IJdFiberControl				m_controller		= nullptr;
};


enum EJdFiberStatus
{
	e_jdFiber_invalid,
	
	e_jdFiber_running,
	e_jdFiber_finished,
	e_jdFiber_terminated,
	e_jdFiber_aborted,

	// runnable states:
	e_jdFiber_initialized,
	e_jdFiber_paused,
	e_jdFiber_exited,
	e_jdFiber_terminating
};


namespace c_jdFiber
{
	auto const 	invalid 			= e_jdFiber_invalid,
				initialized 		= e_jdFiber_initialized,
				paused				= e_jdFiber_paused,
				running				= e_jdFiber_running,
				terminating			= e_jdFiber_terminating,
				exited				= e_jdFiber_exited,
				finished			= e_jdFiber_finished,
				terminated			= e_jdFiber_terminated,
				aborted				= e_jdFiber_aborted;
};

class JdFiberTerminate {};

namespace boost_ctx = boost::context::detail;


class JdFibers
{
	public:
								JdFibers				()
								:
								m_homeFiber				(this, nullptr, nullptr, "home")
	{
	}
	
	virtual 					~ JdFibers				()
	{
		for (auto i : m_stacks)
			free (i.stack);
	}
	
	template <typename T>
	T *							CreateFiber				(size_t i_stackSize, JdResult * o_result, stringRef_t i_fiberName = "")
	{
		d_jdAssert (i_stackSize >= 8192, "stack is too small"); // 8192 might even be too small. termination throw seems to cause a stack overflow @ 4kB
		
		i_stackSize = (i_stackSize + 4095) & ~4095;
		
		// align objects to 16 bytes
		size_t controllerSize = sizeof (FiberController);
		controllerSize = (controllerSize + 15) & ~15;
		
		size_t sizeOfT = sizeof (T);
		sizeOfT = (sizeOfT + 15) &  ~15;
		
		size_t objectsSize = controllerSize + sizeOfT;
		d_jdAssert (i_stackSize > objectsSize, "fiber stack not large enough");
		
		auto stack = CreateStack (i_stackSize);
//		auto stack = (u8 *) malloc (i_stackSize);
		
//		cout << "stack: " << (voidptr_t) stack << endl;

		// create IJdFiber
		auto ptr = stack;
		ptr += i_stackSize;
		ptr -= sizeOfT;
		auto fiber = new (ptr) T;

		// create FiberController
		ptr -= controllerSize;
		auto fc = new (ptr) FiberController (this, stack, fiber, i_fiberName);
		fc->m_stackSize = i_stackSize;
		
		stack += i_stackSize;
		stack -= objectsSize;
		
//		cout << "stacksize: " << stackSize << endl;

		fc->m_context = boost_ctx::make_fcontext (stack, i_stackSize - objectsSize, FiberRunner);
		
//		cout << "free: " << (u8 *) fc->m_context - stack << endl;
		
		if (fc->m_context)
			fc->m_state = e_jdFiber_initialized;
		
		return fiber;
	}
	
	
	JdResult					ReleaseFiber			(IJdFiber i_fiber, bool i_forceQuit = false)
	{
		auto controller = static_cast <FiberController *> (i_fiber->m_controller);
		d_jdAssert (controller != m_activeFiber, "can't release the active fiber");

		if (controller->m_state == c_jdFiber::paused)
		{
			if (i_forceQuit)
			{
			}
			else return d_jdError2 ("can't release active fiber without i_forceQuit");
		}
		
		if (controller->m_state == c_jdFiber::exited)
		{
			 // get fiber out of restart loop
			if (AtHome ())
				controller->Run ();
			else
				m_activeFiber->Yield (i_fiber);		// TODO: test this scenario of releasing a fiber from another fiber
		}
		
		auto stack = controller->m_stack;
		m_stacks.push_back ({ stack, controller->m_stackSize });

		i_fiber->~IIJdFiber ();
		controller->~FiberController ();
		
//		cout << "free: " << (voidptr_t) stack << endl;
//		free (stack);
		
		return c_jdNoErr;
	}
	
//	void						MoveFiber				(IJdFiber i_fiber)
//	{
//
//	}
	
	protected:
	
	struct FiberController : IIJdFiberControl
	{
								FiberController			(JdFibers * i_home, void * i_stack, IJdFiber i_implementation, stringRef_t i_name)
								:
								m_home					(i_home),
								m_stack					(i_stack),
								m_implementation		(i_implementation),
								m_name					(i_name)
		{
			if (i_implementation)
				i_implementation->m_controller = this;
		}
		
		virtual string			GetName					()
		{
			return m_name;
		}

		virtual bool			IsRunnable				()
		{
			return (m_state == c_jdFiber::initialized or m_state == c_jdFiber::paused);
		}

		virtual JdResult		Run						()
		{
			d_jdAssert (m_home->AtHome (), "fiber can only be run from home state");
			
			if (m_state >= c_jdFiber::initialized)
			{
				boost_ctx::transfer_t from = boost_ctx::jump_fcontext (m_context, this);
				m_home->ComingHome (from.fctx);
				
				return m_result;
			}
			else return d_jdError2 ("invalid fiber state");
		}

		virtual void			YieldTo					(IJdFiber i_yieldTo)
		{
			Yield (i_yieldTo);
		}
		
		virtual void			Yield					(IJdFiber i_yieldTo)
		{
//			u8 stack; cout << "freestack: " << & stack - (u8*) m_stack << endl;
			
			d_jdAssert (m_state == c_jdFiber::running, "fiber can't yield; aint't runin");
		
			if (i_yieldTo)
			{
				auto fc = static_cast <FiberController *> (i_yieldTo->m_controller);
				boost_ctx::transfer_t from = boost_ctx::jump_fcontext (fc->m_context, fc);
				m_home->MakeTransfer (this, from.fctx);
			}
			else
			{
				boost_ctx::transfer_t from = boost_ctx::jump_fcontext (m_transferredFrom->m_context, nullptr);
				m_home->MakeTransfer (this, from.fctx);
			}
			
			if (m_state == c_jdFiber::terminating)
				throw JdFiberTerminate ();
		}
		
		virtual JdResult		Terminate				()
		{
			if (m_state == c_jdFiber::running)
			{
				throw JdFiberTerminate ();
			}
			else
			{
				m_state = c_jdFiber::terminating;
				m_home->Terminate (this);
			}
			
			return c_jdNoErr;
		}
		

		void *					m_stack					= nullptr;
		size_t					m_stackSize				= 0;
		JdFibers *				m_home					= nullptr;
		boost_ctx::fcontext_t	m_context				= nullptr;
		IJdFiber				m_implementation		= nullptr;
		string					m_name;
		EJdFiberStatus			m_state 				= e_jdFiber_invalid;
		FiberController *		m_transferredFrom		= nullptr;
		JdResult				m_result;
	};
	
	
	static void FiberRunner (boost_ctx::transfer_t i)
	{
		auto f = reinterpret_cast <FiberController *> (i.data);
		f->m_home->MakeTransfer (f, i.fctx);
		
		try
		{
			while (f->m_state == c_jdFiber::running)
			{
				i64 result = f->m_implementation->RunFiber ();

				if (result == 0)
					f->m_state = c_jdFiber::exited;

				f->m_home->ReturnHome (f);
			}
		}
		catch (JdFiberTerminate & _exit)
		{
//			cout << "terminated\n";
			f->m_state = c_jdFiber::terminated;
		}
		catch (...)
		{
			f->m_state = c_jdFiber::aborted;
		}

		if (f->m_state == c_jdFiber::exited)
			f->m_state = c_jdFiber::finished;

//		cout << "really exited\n";
		
		f->m_home->ReturnHome (f);
	}

	void 					ComingHome					(boost_ctx::fcontext_t i_previousContext)
	{
		MakeTransfer (& m_homeFiber, i_previousContext);
	}

	void 					ReturnHome					(FiberController * i_fromFiber)
	{
//		cout << "RETURN HOME\n";
		
		auto homeContext = m_homeFiber.m_context;
		if (homeContext)
		{
			boost_ctx::transfer_t from = boost_ctx::jump_fcontext (homeContext, nullptr);
			MakeTransfer (i_fromFiber, from.fctx);
		}
	}

	bool					AtHome					() const
	{
		return m_activeFiber == & m_homeFiber;
	}
	
	
	void					MakeTransfer			(FiberController * i_newFiber, boost_ctx::fcontext_t i_previousContext)
	{
		m_activeFiber->m_context = i_previousContext;

//		cout << m_activeFiber->GetName () << " (" << m_activeFiber->m_context << ")" << " --> " << i_newFiber->GetName () << " (" << i_newFiber->m_context << ")" << endl;

		i_newFiber->m_transferredFrom = m_activeFiber;

		if (i_newFiber->m_state != c_jdFiber::terminating)
			i_newFiber->m_state = c_jdFiber::running;
		
		if (m_activeFiber->m_state == c_jdFiber::running)
			m_activeFiber->m_state = c_jdFiber::paused;
		
		m_activeFiber = i_newFiber;
	}
	
	
	void					Terminate				(FiberController * i_terminatingFiber)
	{
		if (AtHome ())
			i_terminatingFiber->Run ();
		else
			m_activeFiber->Yield (i_terminatingFiber->m_implementation);
	}
	
	u8 *					CreateStack				(size_t i_stackSize)
	{
		auto i = m_stacks.begin (), e = m_stacks.end ();
		while (i != e)
		{
			if (i->size == i_stackSize)
			{
				auto stack = i->stack;
				m_stacks.erase (i);
				return (u8 *) stack;
			}
			++i;
		}
		
		return (u8 *) malloc (i_stackSize);
	}
	
	FiberController					m_homeFiber;
	FiberController *				m_activeFiber			= & m_homeFiber;

	struct StackRecord
	{
		void *		stack;
		size_t		size;
	};
	
	list <StackRecord>				m_stacks;
};

#define __JdFiber_hpp__
#endif
