//
//  JdFiber.hpp
//
//  Created by Steven Massey on 5/30/12.
//  Copyright (c) 2012 Steven Massey. All rights reserved.
//

#ifndef __JdFiber_hpp__

#include <boost/context/all.hpp>
#include <list>
#include <iomanip>

#include "JdAssert.hpp"
#include "JdTable.hpp"


d_jdForwardInterface (IJdFiber);


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
	e_jdFiber_exiting,
	e_jdFiber_terminating
};


d_jdInterface (IJdFiberControl)
{
	virtual string				GetName				() = 0;
	virtual void				SetName				(stringRef_t i_name) = 0;
	virtual bool				IsRunnable			() = 0;
	virtual EJdFiberStatus		Run					(JdResult & o_result) = 0;
	virtual EJdFiberStatus		Run					() = 0;
	virtual void				Yield				(IJdFiber i_toFiber = nullptr) = 0;		// Yield (nullptr) switches to calling fiber
	virtual void				YieldTo				(IJdFiber i_toFiber) = 0;				// YieldTo (nullptr) switches to home
	virtual JdResult			Terminate			() = 0;
};

class JdFibers;

d_jdInterface (IJdFiber)
{
	public: friend JdFibers;

	virtual						~ IIJdFiber 		() {}
	
	size_t						Run					(JdResult & o_result)
	{
		return m_controller->Run (o_result);
	}

	size_t						Run					()
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

	void						Yield				()
	{
		m_controller->Yield ();
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


namespace c_jdFiber
{
	auto const 	invalid 			= e_jdFiber_invalid,
				initialized 		= e_jdFiber_initialized,
				paused				= e_jdFiber_paused,
				running				= e_jdFiber_running,
				terminating			= e_jdFiber_terminating,
				exiting				= e_jdFiber_exiting,
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
		
		d_jdAssert (m_fibers.size () == 0, "unreleased fibers");
	}
	
	template <typename T>
	T *							CreateFiber				(size_t i_stackSize, JdResult * o_result = nullptr, stringRef_t i_fiberName = "")
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
		
		m_fibers.push_back (fiber);
		
		return fiber;
	}
	
	template <typename T = IIJdFiber>
	T *							GetActiveFiber			()
	{
		return static_cast <T *> (m_activeFiber->m_implementation);
	}

	IJdFiberControl				GetActiveController 	()
	{
		return m_activeFiber;
	}

	size_t						GetNumFibers			() const
	{
		return m_fibers.size ();
	}
	
	template <typename T>
	T *							GetFiber				(size_t i_index)
	{
		return static_cast <T *> (m_fibers [i_index]);
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
		
		if (controller->m_state == c_jdFiber::exiting)
		{
			 // get fiber out of restart loop
			if (AtHome ())
				controller->Run ();
			else
				m_activeFiber->Yield (i_fiber);		// TODO: test this scenario of releasing a fiber from another fiber
		}

		auto i = find (m_fibers.begin (), m_fibers.end (), controller->m_implementation);
		m_fibers.erase (i);

		auto stack = controller->m_stack;
		m_stacks.push_back ({ stack, controller->m_stackSize });
		
		i_fiber->~IIJdFiber ();
		controller->~FiberController ();
		
		return c_jdNoErr;
	}
	
//	void						MoveFiber				(IJdFiber i_fiber)
//	{
//
//	}
	

	
	void						dump					()
	{
		if (m_fibers.size ())
		{
			JdTable t ({ "fiber", "L:name", "stack", "max-used", "runs", "L:state" });

			cstr_t states [] = { "invalid", "running", "finished", "terminated", "aborted", "inited", "paused", "exiting", "terminating" };
			
			for (auto i : m_fibers)
			{
				auto f = static_cast <FiberController *> (i->m_controller);

				t.AddRow (f, f->m_name, JdByteSize (f->m_stackSize), JdByteSize (measure_stack_height (f)), f->m_runs, states [f->m_state]);
			}
		}
	}
	
	protected:
	
	struct FiberController : IIJdFiberControl
	{
								FiberController			(JdFibers * i_home, void * i_stack, IJdFiber i_implementation, stringRef_t i_name)
								:
								m_stack					(i_stack),
								m_home					(i_home),
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

		virtual void			SetName					(stringRef_t i_name)
		{
			m_name = i_name;
		}

		virtual bool			IsRunnable				()
		{
			return (m_state == c_jdFiber::initialized or m_state == c_jdFiber::paused);
		}

		virtual EJdFiberStatus	Run						(JdResult & o_result)
		{
			d_jdAssert (m_home->AtHome (), "fiber can only be run from home state");
			
			if (m_state >= c_jdFiber::initialized)
			{
//				++m_runs;
				boost_ctx::transfer_t from = boost_ctx::jump_fcontext (m_context, this);
				m_home->ComingHome (from.fctx);
				
				o_result = m_result;
			}
			else o_result = d_jdError2 ("invalid fiber state");
			
			return m_state;
		}

		virtual EJdFiberStatus	Run						()
		{
			if (m_state >= c_jdFiber::initialized)
			{
//				++m_runs;
				boost_ctx::transfer_t from = boost_ctx::jump_fcontext (m_context, this);
				m_home->ComingHome (from.fctx);
			}

			return m_state;
		}

		virtual void			YieldTo					(IJdFiber i_yieldTo)
		{
			if (i_yieldTo)
				Yield (i_yieldTo);
			else
			{
				++m_runs;
				m_home->ReturnHome (this);
			}
		}
		
		virtual void			Yield					(IJdFiber i_yieldTo)
		{
//			u8 stack; cout << "freestack: " << & stack - (u8*) m_stack << endl;
			
			++m_runs;

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
			
//			++m_runs;
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
		size_t					m_runs					= 0;
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
//			while (f->m_state == c_jdFiber::running)
			{
				i64 result = f->m_implementation->RunFiber ();

				if (result == 0)
					f->m_state = c_jdFiber::exiting;

//				f->m_home->ReturnHome (f);
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

		if (f->m_state == c_jdFiber::exiting)
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
		
		auto stack = (u8 *) malloc (i_stackSize);
#		if DEBUG
			memset (stack, 0x0, i_stackSize);
#		endif
		
		return stack;
	}
	
	size_t						measure_stack_height	(FiberController * i_fiberController)
	{
		size_t numWords = i_fiberController->m_stackSize / sizeof (size_t);
		auto words = (size_t *) i_fiberController->m_stack;
		
		size_t t = 0;
		while (t < numWords)
		{
			if (words [t])
				break;
			++t;
		}
		
		numWords -= t;
		return numWords * sizeof (size_t);
	}
	
	FiberController					m_homeFiber;
	FiberController *				m_activeFiber			= & m_homeFiber;

	struct StackRecord
	{
		void *		stack;
		size_t		size;
	};
	
	list <StackRecord>				m_stacks;
	vector <IJdFiber>				m_fibers;
	
};

#define __JdFiber_hpp__
#endif
