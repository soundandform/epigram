//  JdMarshall.hpp
//
//  Created by Steven Massey on 3/12/25.
//  Copyright © 2025 Massey Plugins. All rights reserved.

#ifndef JdMarshall_hpp
#define JdMarshall_hpp

# include "JdMessageQueue.hpp"


const u32 c_defaultMarshallSize = 128;

namespace std
{
	template <typename T> using shared = shared_ptr <T>;
}

template <u32 t_size>
struct JdMarshalled
{
	typedef void (* invoke_t) (JdMarshalled *);

	invoke_t		invoke;

	u8 				opaque		[t_size - sizeof (invoke_t)];
};


template <u32 t_size = c_defaultMarshallSize>
using JdMarshallQueueT = JdMessageQueue <JdMarshalled <t_size>>;



namespace JdMarshall
{
	template <typename Obj, typename T, typename R, typename... Args>
	struct Shared
	{
		typedef void (* invoke_t) (Shared *);
		
		typedef R (Obj::* function_t) (Args...);
		
		invoke_t				invoke = & Call;
		
		std::shared	<Obj>		object;
		function_t				function;
		T						tuple;			// tuple's gotta go last
		
		static void Call (Shared * _this)
		{
			std::apply (std::bind_front (_this->function, _this->object.get ()), _this->tuple);
			_this->~Shared ();
		}
	};

}


template <typename Q, typename D, typename R, typename... Args, typename ... Ins>
static void JdMarshallReply (Q & i_queue, std::shared <D> & i_object, R (D::* i_function)(Args...), Ins && ... i_args)
{																															using namespace std;
	typedef tuple <typename decay <Args>::type...>  tuple_t;
	typedef JdMarshall::Shared <D, tuple_t, R, Args...> marshall_t;				d_jdAssert (sizeof (marshall_t) <= sizeof (typename Q::type));

	seq_t sequence;
	auto slot = i_queue.AcquireMessageSlot (sequence);
	
	new ((void *) slot) marshall_t { .object= i_object, .function= i_function, .tuple= { i_args... }};
	
	i_queue.CommitMessage (sequence);
}



namespace JdMarshall
{
	template <typename RQ, typename ReplyObj, typename Obj, typename T, typename R, typename... Args>
	struct SharedWithReply
	{
		typedef void (* invoke_t) (SharedWithReply *);
		
		typedef R 		(Obj::* function_t) 	(Args...);
		typedef void 	(ReplyObj::* reply_t) 	(R);

		
		invoke_t										invoke = & Call;
		
		std::shared	<Obj>								object;
		function_t										function;
			
		RQ												queue;
		std::shared <ReplyObj>							expectant;
		reply_t											replyFunction;

		T												tuple;		// tuple's gotta go last
		
		static void Call (SharedWithReply * _this)
		{
			R result = std::apply (std::bind_front (_this->function, _this->object.get ()), _this->tuple);
			
			JdMarshallReply (* _this->queue, _this->expectant, _this->replyFunction, result);
			
			_this->~SharedWithReply ();
		}
	};

	
	template <typename Obj, typename T, typename R, typename... Args>
	struct Unsafe
	{
		typedef void (* invoke_t) (void *);

		typedef R (Obj::* function_t) (Args...);

		invoke_t				invoke = & Call;
		
		Obj *					object;
		function_t				function;
		T						tuple;	// tuple's gotta go last
		
		static void Call (void * i_marshall)
		{
			auto _this = (Unsafe *) i_marshall;
			std::apply (std::bind_front (_this->function, _this->object), _this->tuple);
			_this->~Unsafe ();
		}
	};
}



struct Marshaller
{
	template <typename Q, typename D, typename R, typename... Args, typename ... Ins>
	static void MarshallShared (Q & i_queue, std::shared <D> & i_object, R (D::* i_function)(Args...), Ins && ... i_args)
	{																			using namespace std;
		typedef tuple <typename decay <Args>::type...>  tuple_t;
		typedef JdMarshall::Shared <D, tuple_t, R, Args...> marshall_t;				d_jdAssert (sizeof (marshall_t) <= sizeof (typename Q::type));		jd::out (sizeof (marshall_t));
	
		seq_t sequence;
		auto slot = i_queue->AcquireMessageSlot (sequence);
		
		new ((void *) slot) marshall_t { .object= i_object, .function= i_function, .tuple= { i_args... }};
		
		i_queue->CommitMessage (sequence);
	}

	
	template <typename RQ, typename RD, typename Q, typename D, typename R, typename... Args, typename ... Ins>
	static void  SharedWithReply  (RQ & i_replyQueue, std::shared <RD> & i_expectantObj, void (RD::* i_replyFunction) (R), Q & i_queue, std::shared <D> & i_object, R (D::* i_function)(Args...), Ins && ... i_args)
	{																				using namespace std;
		typedef tuple <typename decay <Args>::type...>  tuple_t;
		typedef JdMarshall::SharedWithReply <RQ, RD, D, tuple_t, R, Args...> marshall_t;				d_jdAssert (sizeof (marshall_t) <= c_defaultMarshallSize);		jd::out (sizeof (marshall_t));
	
		seq_t sequence;
		auto slot = i_queue->AcquireMessageSlot (sequence);
		
		new ((void *) slot) marshall_t { .object= i_object, .function= i_function, 
										 .queue = i_replyQueue, .expectant= i_expectantObj, .replyFunction = i_replyFunction, .tuple= { i_args... }};
		
		i_queue->CommitMessage (sequence);
	}

	
	template <typename Q, typename D, typename R, typename... Args, typename ... Ins>
	static void MarshallRaw (Q & i_queue, D * i_object, R (D::* i_function)(Args...), Ins && ... i_args)
	{																			using namespace std;
		typedef tuple <typename decay <Args>::type...>  tuple_t;
		typedef JdMarshall::Unsafe <D, tuple_t, R, Args...> marshall_t;				d_jdAssert (sizeof (marshall_t) <= sizeof (typename Q::type));		jd::out (sizeof (marshall_t));
	
		seq_t sequence;
		auto slot = i_queue.AcquireMessageSlot (sequence);
		
		new ((void *) slot) marshall_t { .object= i_object, .function= i_function, .tuple= { i_args... }};
		
		i_queue.CommitMessage (sequence);
	}

	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	template <u32 t_size, typename D, typename R, typename... Args, typename ... Ins>
	static void call (JdMarshallQueueT <t_size> & i_queue, std::shared <D> & i_dest, R (D::* i_function)(Args...), Ins && ... i_args)
	{
		MarshallShared (i_queue, i_dest, i_function, i_args...);
	}
	
	// these all could be static but a warning here about static operator () is in C++Whatever
	template <u32 t_size, typename D, typename R, typename... Args, typename ... Ins>
	void operator () (JdMarshallQueueT <t_size> & i_queue, std::shared <D> & i_dest, R (D::* i_function)(Args...), Ins && ... i_args)
	{
		MarshallShared (i_queue, i_dest, i_function, i_args...);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
	template <u32 t_size, typename D, typename R, typename... Args, typename ... Ins>
	void operator () (JdMarshallQueueT <t_size> & i_queue, D * i_raw, R (D::* i_function)(Args...), Ins && ... i_args)
	{
		MarshallRaw (i_queue, i_raw, i_function, i_args...);
	}

	template <u32 t_size, typename D, typename R, typename... Args, typename ... Ins>
	static void call (JdMarshallQueueT <t_size> & i_queue, D * i_raw, R (D::* i_function)(Args...), Ins && ... i_args)
	{
		MarshallRaw (i_queue, i_raw, i_function, i_args...);
	}

	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------


	template <u32 t_sizeReply, u32 t_size, typename RO, typename DO, typename R, typename... Args, typename ... Ins>
	void withReply (JdMarshallQueueT <t_sizeReply> & i_replyQueue, std::shared <RO> & i_expectant, void (RO::* i_replyFunction) (R),
					JdMarshallQueueT <t_size> & i_queue, std::shared <DO> & i_dest, R (DO::* i_function)(Args...), Ins && ... i_args)
	{
		 Marshaller::SharedWithReply  (i_replyQueue, i_expectant, i_replyFunction,
									   i_queue, i_dest, i_function, i_args...);
	}

	
	template <u32 t_size>
	static void  ProcessQueue  (JdMarshallQueueT <t_size> & i_queue, u32 const i_numMessages = std::numeric_limits <u32>::max ())
	{
		u32 numMessages = i_queue.ClaimMessages (i_numMessages);
		
		for (u32 i = 0; i < numMessages; ++i)
		{
			auto marshall = i_queue.ViewMessage (i);
			marshall->invoke (marshall);
		}
		
		i_queue.ReleaseMessages (numMessages);
	}
};


/*
template <typename Obj, u32 t_size = c_defaultMarshallSize>
struct JdObjMarshallerT
{
	JdObjMarshallerT  (JdMarshallQueueT <t_size> * i_queue, std::shared <Obj> & i_destination)
	:
	m_dest		(i_destination),
	m_queue		(i_queue)
	{ }

	JdObjMarshallerT  (JdMarshallQueueT <t_size> * i_queue)
	:
	m_queue				(i_queue)
	{ }

	JdObjMarshallerT  ()
	{ }
	
	template <typename R, typename... Args, typename ... Ins>
	void operator () (R (Obj::* i_function)(Args...), Ins && ... i_args)
	{
		MarshallShared (m_dest, i_function, i_args...);
	}

	
	void  ProcessQueue  (u32 const i_numMessages = std::numeric_limits <u32>::max ())
	{
		ProcessQueue (m_queue, i_numMessages);
	}


	std::shared <Obj>				m_dest;
	JdMarshallQueueT <t_size> *		m_queue			= nullptr;
};

 */

using namespace std;

# include "JdThread.hpp"


template <u32 t_marshallSize = c_defaultMarshallSize, u32 t_replyMarshallSize = c_defaultMarshallSize>
struct JdTasks
{
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
				Marshaller::ProcessQueue (* m_taskQueue);

				++m_numTasksRan;
			}
			
			return result;
		}
		
		void					Quit			() { m_quit = true; }

		JdResult                Break           () override
		{
			Marshaller::call (* m_taskQueue, this, & Thread::Quit);
			
			return c_jdNoErr;
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

	template <typename RO, typename DO, typename R, typename... Args, typename ... Ins>
	void getReply (std::shared <RO> & i_expectant, void (RO::* i_replyFunction) (R), /* <-- */ std::shared <DO> & i_dest, R (DO::* i_function)(Args...), Ins && ... i_args)
	{
		Marshaller::SharedWithReply  (m_replyQueue, i_expectant, i_replyFunction,
									 m_taskQueue, i_dest, i_function, i_args...);
	}


//	template <typename RO,  typename R>
//	void wtf (std::shared <RO> & i_expectant, void (RO::* i_replyFunction) (R))
//	{
//	}

	template <typename RO>
	void wtf (std::shared_ptr <RO> & i_expectant)
	{
	}


	//----------------------------------------------------------------------------------------------------------------
	void  ProcessReplies  (u32 const i_numMessages = std::numeric_limits <u32>::max ())
	{
		Marshaller::ProcessQueue (* m_replyQueue, i_numMessages);
	}
	
	//----------------------------------------------------------------------------------------------------------------
	
	JdThreadT <Thread>			m_thread					{ "JdTask" };

	std::shared <JdMarshallQueueT <t_marshallSize>>			m_taskQueue;
	std::shared <JdMarshallQueueT <t_replyMarshallSize>>	m_replyQueue;
};




#endif /* JdMarshall_hpp */
