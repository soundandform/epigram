//  JdMarshall.hpp
//
//  Created by Steven Massey on 3/12/25.
//  Copyright © 2025 Massey Plugins. All rights reserved.

#ifndef JdMarshall_hpp
#define JdMarshall_hpp

# include "JdMessageQueue.hpp"


const u32 c_defaultMarshallSize = 256;

namespace std
{
	template <typename T> using shared 	= shared_ptr <T>;
	template <typename T> using weak 	= weak_ptr <T>;
}

template <u32 t_size>
struct JdMarshallOpaque
{
	typedef void (* invoke_t) (JdMarshallOpaque *);

	invoke_t		invoke;

	u8 				opaque		[t_size - sizeof (invoke_t)];
};


template <u32 t_size = c_defaultMarshallSize>
using JdMarshallQueueT = JdMessageQueue <JdMarshallOpaque <t_size>>;


namespace JdMarshallObj
{
	template <typename Obj, typename Cast, typename T, typename R, typename... Args>
	struct Shared
	{
		typedef void (* invoke_t) (Shared *);
		
		typedef R (Cast::* function_t) (Args...);
		
		invoke_t				invoke = & Call;
		
		std::shared	<Obj>		object;
		function_t				function;
		T						tuple;			// tuple's gotta go last
		
		static void Call (Shared * _this)
		{
			std::apply (std::bind_front (_this->function, static_cast <Cast *> (_this->object.get ())), _this->tuple);
			_this->~Shared ();
		}
	};

	template <typename RO, typename T, typename RF>
	struct Raw
	{
		typedef void (* invoke_t) (Raw *);
		
		invoke_t				invoke = & Call;
		
		RO *					object;
		RF						function;
		T						tuple;			// tuple's gotta go last
		
		static void Call (Raw * _this)
		{
			std::apply (std::bind_front (_this->function, _this->object), _this->tuple);
			_this->~Raw ();
		}
	};

}

template <typename> struct is_tuple: std::false_type {};
template <typename ...T> struct is_tuple<std::tuple<T...>>: std::true_type {};

template <typename T>
auto wrap_in_a_tuple_if_not_a_tuple(T * i_value) {
	if constexpr (is_tuple <T>::value)
		return * i_value;
	else
		return std::make_tuple (std::forward<T> (* i_value));
}


namespace JdMarshallObj
{

	template <typename Q, typename D, typename R, typename Tuple, typename... Args>
	static void Reply (Q & i_queue, std::shared <D> & i_object, R (D::* i_function)(Args...), Tuple && i_tuple)
	{																															using namespace std;
	//	typedef tuple <typename decay <Args>::type...>  tuple_t;
		typedef Tuple  tuple_t;		// don't mutate the return tuple.  keep a string a string so it can cast to a string_view, for example
		typedef JdMarshallObj::Shared <D, D, tuple_t, R, Args...> marshall_t;				d_jdAssert (sizeof (marshall_t) <= sizeof (typename Q::type));

		seq_t sequence;
		auto slot = i_queue.AcquireMessageSlot (sequence);
		
		new ((void *) slot) marshall_t { .object= i_object, .function= i_function, .tuple= i_tuple};
		
		i_queue.CommitMessage (sequence);
	}


	template <typename RQ, typename RO, typename RF, typename TO, typename T, typename R, typename... Args>
	struct SharedToSharedT
	{
		typedef void	(* invoke_t)			(SharedToSharedT *);
		typedef R 		(TO::* function_t) 	(Args...);
		
		invoke_t										invoke = & Call;
		
		std::shared	<TO>								object;
		function_t										function;
			
		RQ												queue;
		std::shared <RO>								expectant;
		RF												replyFunction;

		T												tuple;		// tuple's gotta go last
		
		static void Call (SharedToSharedT * _this)
		{
			R result = std::apply (std::bind_front (_this->function, _this->object.get ()), _this->tuple);
			
			Reply (* _this->queue, _this->expectant, _this->replyFunction, wrap_in_a_tuple_if_not_a_tuple (& result));
			
			_this->~SharedToSharedT ();
		}
	};

	template <typename RQ, typename RO, typename RF, typename T, typename R, typename... Args>
	struct StaticToSharedT
	{
		typedef void	(* invoke_t)			(StaticToSharedT *);
		typedef R 		(* function_t) 			(Args...);
		
		invoke_t										invoke = & Call;
		
		function_t										function;
			
		RQ												queue;
		std::shared <RO>								expectant;
		RF												replyFunction;

		T												tuple;		// tuple's gotta go last
		
		static void Call (StaticToSharedT * _this)
		{
			R result = std::apply (_this->function, _this->tuple);
			Reply (* _this->queue, _this->expectant, _this->replyFunction, wrap_in_a_tuple_if_not_a_tuple (& result));
			_this->~StaticToSharedT ();
		}
	};



	template <typename RQ, typename RO, typename RF, typename T, typename R, typename... Args>
	struct StaticToRawT
	{
		typedef void	(* invoke_t)			(StaticToRawT *);
		typedef R 		(* function_t) 			(Args...);
		
		invoke_t										invoke = & Call;
		
		function_t										function;
			
		RQ												queue;
		RO *											expectant;
		RF												replyFunction;

		T												tuple;		// tuple's gotta go last
		
		
		template <typename Tuple>
		static void Reply (RQ & i_queue, RO * i_object, RF i_replyFunction, Tuple && i_tuple)
		{
			typedef JdMarshallObj::Raw <RO, Tuple, RF> marshall_t;								 d_jdAssert (sizeof (marshall_t) <= i_queue->getMessageSize ());

			seq_t sequence; auto slot = i_queue->AcquireMessageSlot (sequence);
			
			new (slot) marshall_t { .object= i_object, .function= i_replyFunction, .tuple= i_tuple };
			
			i_queue->CommitMessage (sequence);
		}
		
		static void Call (StaticToRawT * _this)
		{
			R result = std::apply (_this->function, _this->tuple);

			Reply (_this->queue, _this->expectant, _this->replyFunction, wrap_in_a_tuple_if_not_a_tuple (& result));
			
			_this->~StaticToRawT ();
		}
	};
	
	template <typename Obj, typename T, typename R, typename Fun, typename... Args>
	struct Unsafe
	{
		typedef void (* invoke_t) (void *);

		invoke_t				invoke;
		Obj *					object;			// unused for Static case
		Fun						function;
		T						tuple;			// tuple's gotta go last
		
		static void Call (void * i_marshall)
		{
			auto _this = (Unsafe *) i_marshall;
			std::apply (std::bind_front (_this->function, _this->object), _this->tuple);
			_this->~Unsafe ();
		}
		
		static void CallStatic (void * i_marshall)
		{
			auto _this = (Unsafe *) i_marshall;
			std::apply (_this->function, _this->tuple);
			_this->~Unsafe ();
		}
	};
}


// TOQUE: should the marshalled tuple be the decayed args of the target function (cast at start) or the input args (cast in thread)


struct Marshall
{
	// RQ: ReplyQueue
	// RO: ReplyObject
	// RA: ReplyArgs
	// TO: TargetObject
	
	template <typename RQ, typename RO, typename Q, typename TO, typename R, typename... RA, typename... Args, typename ... Ins>
	static void  SharedToShared   (RQ & i_replyQueue, std::shared <RO> & i_expectantObj, void (RO::* i_replyFunction) (RA...),
								   Q & i_queue, std::shared <TO> & i_object, R (TO::* i_function)(Args...), Ins && ... i_args)
	{																				using namespace std;
		typedef tuple <typename decay <Args>::type...>  tuple_t;
		
		typedef void (RO::* replyFunction_t) (RA...);
		
		typedef JdMarshallObj::SharedToSharedT <RQ, RO, replyFunction_t, TO, tuple_t, R, Args...> marshall_t;				d_jdAssert (sizeof (marshall_t) <= i_queue.getMessageSize ());
	
		seq_t sequence;
		auto slot = i_queue.AcquireMessageSlot (sequence);
		
		new (slot) marshall_t { .object= i_object, .function= i_function,
										 .queue = i_replyQueue, .expectant= i_expectantObj, .replyFunction = i_replyFunction, .tuple= { i_args... }};
		
		i_queue.CommitMessage (sequence);
	}

	
	template <typename RQ, typename RO, typename Q, typename R, typename... RA, typename... Args, typename ... Ins>
	static void  StaticToShared   (RQ & i_replyQueue, std::shared <RO> & i_expectantObj, void (RO::* i_replyFunction) (RA...),
								   Q & i_queue, R (* i_staticFunction)(Args...), Ins && ... i_args)
	{																				using namespace std;
		typedef tuple <typename decay <Args>::type...>  tuple_t;
		
		typedef void (RO::* replyFunction_t) (RA...);
		
		typedef JdMarshallObj::StaticToSharedT <RQ, RO, replyFunction_t, tuple_t, R, Args...> marshall_t;				d_jdAssert (sizeof (marshall_t) <= i_queue.getMessageSize ());

		seq_t sequence; auto slot = i_queue.AcquireMessageSlot (sequence);

		new (slot) marshall_t { .function= i_staticFunction,
										 .queue = i_replyQueue, .expectant= i_expectantObj, .replyFunction = i_replyFunction, .tuple= { i_args... }};
		
		i_queue.CommitMessage (sequence);
	}

	
	template <typename RQ, typename RO, typename Q, typename R, typename... RA, typename... Args, typename ... Ins>
	static void  StaticToRaw	  (RQ & i_replyQueue, RO * i_expectantObj, void (RO::* i_replyFunction) (RA...),
								   Q & i_queue, R (* i_staticFunction)(Args...), Ins && ... i_args)
	{																				using namespace std;
		typedef tuple <typename decay <Args>::type...>  tuple_t;
		
		typedef void (RO::* replyFunction_t) (RA...);
		
		typedef JdMarshallObj::StaticToRawT <RQ, RO, replyFunction_t, tuple_t, R, Args...> marshall_t;				d_jdAssert (sizeof (marshall_t) <= i_queue.getMessageSize ());

		seq_t sequence; auto slot = i_queue.AcquireMessageSlot (sequence);

		new ((void *) slot) marshall_t { .function= i_staticFunction,
										 .queue = i_replyQueue, .expectant= i_expectantObj, .replyFunction = i_replyFunction, .tuple= { i_args... }};
		
		i_queue.CommitMessage (sequence);
	}
	
	
	template <typename Q, typename TO, typename TOCast, typename R, typename... Args, typename ... Ins>
	static void  Shared  (Q & i_queue, std::shared <TO> & i_object, R (TOCast::* i_function)(Args...), Ins && ... i_args)
	{																			using namespace std;
		typedef tuple <typename decay <Args>::type...>  tuple_t;
		typedef JdMarshallObj::Shared <TO, TOCast, tuple_t, R, Args...> marshall_t;										 d_jdAssert (sizeof (marshall_t) <= i_queue.getMessageSize ());
	
		seq_t sequence;
		auto slot = i_queue.AcquireMessageSlot (sequence);
		
		new (slot) marshall_t { .object= i_object, .function= i_function, .tuple= { i_args... }};
		
		i_queue.CommitMessage (sequence);
	}
	
	
	template <typename Q, typename TO, typename R, typename... Args, typename ... Ins>
	static void  Raw  (Q & i_queue, TO * i_object, R (TO::* i_function)(Args...), Ins && ... i_args)
	{																											using namespace std;
		typedef R (TO::* function_t) (Args...);
		typedef tuple <typename decay <Args>::type...>  tuple_t;
		typedef JdMarshallObj::Unsafe <TO, tuple_t, R, function_t, Args...> marshall_t;							d_jdAssert (sizeof (marshall_t) <= i_queue.getMessageSize ());
	
		seq_t sequence; auto slot = i_queue.AcquireMessageSlot (sequence);
		
		new (slot) marshall_t { .invoke = & marshall_t::Call, .object= i_object, .function= i_function, .tuple= { i_args... }};
		
		i_queue.CommitMessage (sequence);
	}

	template <typename Q, typename R, typename... Args, typename ... Ins>
	static void  Static  (Q & i_queue, R (* i_function) (Args...), Ins && ... i_args)
	{																														using namespace std;
		typedef R (* function_t) (Args...);
		typedef tuple <typename decay <Args>::type...>  tuple_t;
		typedef JdMarshallObj::Unsafe <void, tuple_t, R, function_t, Args...> marshall_t;									d_jdAssert (sizeof (marshall_t) <= i_queue.getMessageSize ());
	
		seq_t sequence; auto slot = i_queue.AcquireMessageSlot (sequence);
		
		new (slot) marshall_t { .invoke = & marshall_t::CallStatic, .function= i_function, .tuple= { i_args... }};
		
		i_queue.CommitMessage (sequence);
	}

	
	// Shared --------------------------------------------------------------------------------------------------------------------------------------------
	template <u32 t_size, typename D, typename R, typename... Args, typename ... Ins>
	static void call (JdMarshallQueueT <t_size> & i_queue, std::shared <D> & i_dest, R (D::* i_function)(Args...), Ins && ... i_args)
	{
		Shared (i_queue, i_dest, i_function, i_args...);
	}
	
	
	// Raw -----------------------------------------------------------------------------------------------------------------------------------------------
	
	template <u32 t_size, typename D, typename R, typename... Args, typename ... Ins>
	static void call (JdMarshallQueueT <t_size> & i_queue, D * i_raw, R (D::* i_function)(Args...), Ins && ... i_args)
	{
		Raw (i_queue, i_raw, i_function, i_args...);
	}

	template <u32 t_size, typename R, typename... Args, typename ... Ins>
	static void call (JdMarshallQueueT <t_size> & i_queue, R (* i_function)(Args...), Ins && ... i_args)
	{
		Static (i_queue, i_function, i_args...);
	}

	
	//----------------------------------------------------------------------------------------------------------------------------------------------------


	template <u32 t_sizeReply, u32 t_size, typename RO, typename DO, typename R, typename... Args, typename ... Ins>
	static void callWithReply (std::shared <JdMarshallQueueT <t_sizeReply>> & i_replyQueue, std::shared <RO> & i_expectant, void (RO::* i_replyFunction) (R),
						JdMarshallQueueT <t_size> & i_queue, std::shared <DO> & i_dest, R (DO::* i_function)(Args...), Ins && ... i_args)
	{
		Marshall::SharedToShared  (i_replyQueue, i_expectant, i_replyFunction,
									   i_queue, i_dest, i_function, i_args...);
	}


	template <u32 t_sizeReply, u32 t_sizeCall, typename RO,  typename R, typename... RA, typename... Args, typename ... Ins>
	static void callWithReply (std::shared <JdMarshallQueueT <t_sizeReply>> & i_replyQueue, std::shared <RO> & i_expectant, void (RO::* i_replyFunction) (RA...),
							   JdMarshallQueueT <t_sizeCall> & i_queue, R (* i_staticFunction)(Args...), Ins && ... i_args)
	{
		Marshall::StaticToShared  (i_replyQueue, i_expectant, i_replyFunction,
									i_queue, i_staticFunction, i_args...);
	}

	
	template <u32 t_sizeReply, u32 t_sizeCall, typename RO,  typename R, typename... RA, typename... Args, typename ... Ins>
	static void callWithReply (std::shared <JdMarshallQueueT <t_sizeReply>> & i_replyQueue, RO * i_rawExpectant, void (RO::* i_replyFunction) (RA...),
							   JdMarshallQueueT <t_sizeCall> & i_queue, R (* i_staticFunction)(Args...), Ins && ... i_args)
	{
		Marshall::StaticToRaw  (i_replyQueue, i_rawExpectant, i_replyFunction,
									i_queue, i_staticFunction, i_args...);
	}

	
	template <u32 t_size>
	static u32  ProcessQueue  (JdMarshallQueueT <t_size> & i_queue, u32 const i_numMessages = std::numeric_limits <u32>::max ())
	{
		u32 numMessages = i_queue.ClaimMessages (i_numMessages);
		
		for (u32 i = 0; i < numMessages; ++i)
		{
			auto marshall = i_queue.ViewMessage (i);
			marshall->invoke (marshall);
		}
		
		i_queue.ReleaseMessages (numMessages);
		
		return numMessages;
	}
};


template <typename Obj, typename Cast, u32 t_size = c_defaultMarshallSize>
struct JdObjMarshallerT
{
	JdObjMarshallerT  (JdObjMarshallerT && i_other)
	:
	m_queue		(i_other.m_queue),
	m_dest		(i_other.m_dest)
	{ }

	
	
	JdObjMarshallerT  (JdMarshallQueueT <t_size> * i_queue, std::shared <Obj> && i_destination)
	:
	m_queue		(i_queue),
	m_dest		(i_destination)
	{ }
	
	template <typename D, typename R, typename... Args, typename ... Ins>
	void operator () (R (D::* i_function)(Args...), Ins && ... i_args)
	{
		Marshall::Shared (* m_queue, m_dest, i_function, i_args...);
	}

	JdMarshallQueueT <t_size> *		m_queue			= nullptr;
	std::shared <Obj>				m_dest;
};


using namespace std;

# include "JdThread.hpp"


template <u32 t_marshallSize = c_defaultMarshallSize, u32 t_replyMarshallSize = c_defaultMarshallSize>
struct JdTasks
{
	JdTasks (u32 i_numThreads = 1, string_view i_threadName = "unnamed")
	:
	m_thread (i_threadName.data ()) 
	{
	}
	
	
	~ JdTasks ()
	{
		m_thread.Stop ();
		ProcessReplies ();
	}
	
	struct Thread : IJdThread
	{
		JdResult		Run						(EpigramRef i_args, IJdThread::Info & i_info) override
		{
			JdResult result;
			
			while (not m_quit)
			{
				Marshall::ProcessQueue (* m_taskQueue);
				++m_numTasksRan;
			}
			
			return result;
		}
		
		void					Quit			() 
		{
			m_quit = true;
		}

		void	                Break           () override
		{
			Marshall::call (* m_taskQueue, this, & Thread::Quit);
		}

		shared <JdMarshallQueueT <t_marshallSize>>			m_taskQueue;
		shared <JdMarshallQueueT <t_replyMarshallSize>>		m_replyQueue;
		
		u64													m_numTasksRan		= 0;
		bool												m_quit				= false;
	};
	
	//----------------------------------------------------------------------------------------------------------------

	void  Start  ()
	{
		if (not m_taskQueue)	m_taskQueue 	= make_shared <JdMarshallQueueT <t_marshallSize>> ();
		if (not m_replyQueue)	m_replyQueue	= make_shared <JdMarshallQueueT <t_replyMarshallSize>> ();
		
		m_thread->m_taskQueue	= m_taskQueue;
		m_thread->m_replyQueue	= m_replyQueue;

		m_thread.Start ();
	}

	
	template <typename R, typename... Args, typename ... Ins>
	void call (R (* i_function)(Args...), Ins && ... i_args)
	{
		Marshall::Static (* m_taskQueue, i_function, i_args...);
	}

	
	template <typename WRO, typename RO, typename...  Replies>
	struct ReplyInfo
	{
		template <typename DO, typename R, typename... Args, typename ... Ins>
		void call (std::shared <DO> & i_dest, R (DO::* i_function)(Args...), Ins && ... i_args)
		{
			Marshall::SharedToShared  (tasks.m_replyQueue, expectant, replyFunction,
										  * tasks.m_taskQueue, i_dest, i_function, i_args...);
		}

		template <typename R, typename... Args, typename ... Ins>
		void call (R (* i_staticFunction)(Args...), Ins && ... i_args)
		{
			Marshall::StaticToRaw  (tasks.m_replyQueue, expectant, replyFunction,
									* tasks.m_taskQueue, i_staticFunction, i_args...);
		}

		
		typedef void (RO::* replyFunction_t) (Replies...);

		JdTasks	&				tasks;
		WRO						expectant;
		replyFunction_t			replyFunction;
	};


	// Shared
	template <typename RO, typename... Args>
	ReplyInfo <std::shared <RO>, RO, Args...>  replyTo  (std::shared <RO> & i_expectant, void (RO::* i_replyFunction) (Args...))
	{
		return { * this, i_expectant, i_replyFunction };
	}

	// Weak
	template <typename RO, typename... Args>
	ReplyInfo <std::shared <RO>, RO, Args...>  replyTo  (std::weak <RO> & i_expectant, void (RO::* i_replyFunction) (Args...))
	{
		return { * this, i_expectant.lock (), i_replyFunction };
	}

	// Raw
	template <typename RO, typename... Args>
	ReplyInfo <RO *, RO, Args...>  replyTo  (RO * i_expectant, void (RO::* i_replyFunction) (Args...))
	{
		return { * this, i_expectant, i_replyFunction };
	}


	//----------------------------------------------------------------------------------------------------------------
	void  ProcessReplies  (u32 const i_numMessages = std::numeric_limits <u32>::max ())
	{
		if (m_replyQueue)
			Marshall::ProcessQueue (* m_replyQueue, i_numMessages);
	}
	
	//----------------------------------------------------------------------------------------------------------------
	
	JdThreadT <Thread>			m_thread;

	std::shared <JdMarshallQueueT <t_marshallSize>>			m_taskQueue;
	std::shared <JdMarshallQueueT <t_replyMarshallSize>>	m_replyQueue;
};




#endif /* JdMarshall_hpp */
