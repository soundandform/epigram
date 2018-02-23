//
//  JdThreadPort.hpp
//  Jigidesign
//
//  Created by Steven Massey on 11/10/12.
//  Copyright (c) 2012 Jigidesign. All rights reserved.
//

/*
 
 A JdPortedThread handler must implement the following protocol.  I use the word "protocol" vs interface because it is template-based & does not rely on a pure-virtual
 base class stategy.
 
 --------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 	bool		Idle				();			// return true if the run-loop should continue calling idle or false to demand that at least one new message be processed.

 
 
 Usage of a thread port directly:
 --------------------------------------------------------------------------------------------------------------------------------------------------------------------------

 	class YourClass
	{
 		d_portFunc (FunctionA, int aint, float afloat)
		{
 			
		}
	};
 
 
	YourClass yc;
 
	yc.FunctionA ({ 1, 3.14 });
 
	or 
 
	yc.FunctionA ({ .aint= 1, .afloat= 3.14 });
 
*/



#ifndef __Jigidesign__JdThreadPort__
#define __Jigidesign__JdThreadPort__

#include <iostream>

#include "JdSemaphore.hpp"
#include "JdAssert.hpp"
#include "Epigram.hpp"
#include "JdTimer.hpp"

#include <boost/interprocess/detail/atomic.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/locks.hpp>

using namespace boost::interprocess::ipcdetail;


typedef u32 seq_t;

const int c_jdCacheLineBytes = 64;

template <typename T>
struct alignas (c_jdCacheLineBytes) JdCacheLinePadded
{
	JdCacheLinePadded () { d_jdAssert (sizeof (JdCacheLinePadded) == c_jdCacheLineBytes, "JdCacheLinePadded not aligned properly"); }

	T					value	= T ();
    volatile			u8 		_cacheLinePadding [c_jdCacheLineBytes - sizeof (T)];
};


class JdPortSequence : protected JdCacheLinePadded <seq_t>
{
	public:
					JdPortSequence		(seq_t i_initTo)
	{
		value = i_initTo;
		memset ((void *) & _cacheLinePadding [0], 0x0, Jd::SizeOfArray (_cacheLinePadding));
	}
	
	
	seq_t				Acquire				()
	{
		return atomic_inc32 (& value);
	}
	
	
	void				Update				(seq_t i_newSequenceNum)
	{
		while (true)
		{
			seq_t previousSeq = i_newSequenceNum - 1;
			seq_t previous = atomic_cas32 (&value, i_newSequenceNum, previousSeq);
			if (previous == previousSeq) return;
			
			sleep (0); // FIX: improve!
//			usleep (10);
		}
	}
	
	seq_t			Value				()
	{
		return atomic_read32 (& value);
	}
};


template <u32 t_maxSize>
class JdPortDataType
{
	typedef u32 length_t;
	
	public:
	JdPortDataType () {}
	
	template <typename T>
	struct ArgEntry
	{
		length_t		length;
		T				value;
	};
	
	template <typename T>
	JdPortDataType (const T &i_value)
	{
		d_jdAssert ((sizeof(T) + sizeof (length_t)) <= t_maxSize, "data type is too big for port; up the port datatype size");

		ArgEntry <T> *entry = reinterpret_cast <ArgEntry<T> *> (m_data);
		entry->length = sizeof (T) + sizeof (length_t);
		memcpy (&(entry->value), &i_value, sizeof (T));
	}

	template <typename A0, typename A1>
	JdPortDataType (const A0 &i_arg0, const A1 &i_arg1)
	{
		d_jdAssert (sizeof(A0) + sizeof (A1) + sizeof (length_t) <= t_maxSize, "data type is too big for port; up the port datatype size");

		ArgEntry <A0> *entry0 = reinterpret_cast <ArgEntry<A0> *> (m_data);
		entry0->length = sizeof (A0) + sizeof (length_t);
		memcpy (&(entry0->value), &i_arg0, sizeof (A0));

		ArgEntry <A1> *entry1 = reinterpret_cast <ArgEntry<A1> *> (m_data + entry0->length);
		entry1->length = sizeof (A1) + sizeof (length_t);
		memcpy (&(entry1->value), &i_arg1, sizeof (A1));
	}


	template <typename C>
	operator C () const
	{
		C r;
		const ArgEntry <C> *entry = reinterpret_cast <const ArgEntry<C> *> (m_data);
//		cout << sizeof (C) << endl;
		memcpy (&r, &(entry->value), sizeof (C));
		
		return r;
	}
	
		
	struct Argument
	{
		template <typename C>
		operator C () const
		{
			C r;
			
			#if DEBUG
				const u8 * sizePtr = m_argumentData - sizeof (length_t);
				length_t size = * ((length_t *) sizePtr) - sizeof (length_t);
				d_jdAssert (size == sizeof (C), "mismatched types");
			#endif
			
			memcpy (&r, m_argumentData, sizeof (C));
			return r;
		}
		
		Argument (const u8 *i_argumentData)
		:
		m_argumentData (i_argumentData)
		{
		}
		
		protected:
		const u8 * m_argumentData;
	};
	
	
	Argument GetArg (u32 i_index) const
	{
		const u8 *data = m_data;
		while (i_index)
		{
			length_t *size = (length_t *) data;
			data += *size;
			
			--i_index;
		}
		
		return Argument (data + sizeof (length_t));
	}
	
	private:
	u8		m_data [t_maxSize];
};


typedef JdPortDataType <128> tp_arg_t;

struct JdEmptyArgs {};

typedef JdEmptyArgs emptyArgs();


#define d_jdThreadPortMethod(NAME,ARG) void NAME (const tp_arg_t &i_args) { _##NAME (i_args); } void _##NAME (ARG)

#define d_jdThreadPortMethod2(NAME,ARG0,ARG1) \
		void NAME (const tp_arg_t &i_args) { _##NAME (i_args.GetArg(0), i_args.GetArg(1)); } \
		void _##NAME (ARG0, ARG1)


#define d_jdPortMethodMacroChooser(...) d_get6thArg (__VA_ARGS__, fail, fail, fail, d_jdThreadPortMethod2, d_jdThreadPortMethod, fail, )

#define d_jdPortMethod(...) d_jdPortMethodMacroChooser(__VA_ARGS__)(__VA_ARGS__)


#define d_jdThreadPortMethodWithReply(NAME,ARG) tp_arg_t NAME (const tp_arg_t &i_args) { return _##NAME (i_args); } tp_arg_t _##NAME (ARG)

// 64-bit atomics seem a tad faster... should switch.

const u32 c_jdThreadPort_startIndex = 0xffffff71;	// to prove these mechanisms are integer wrap-around safe


template <typename MessageRecord>
struct JdThreadPortPathway
{
	JdThreadPortPathway	() { }
	
	JdThreadPortPathway	(u32 i_pathwaySizeInNumMessages)
	{
		u32 roundSize = Jd::Pow2CeilLog2 (i_pathwaySizeInNumMessages);
		m_queue.Resize (roundSize);
		m_sequenceMask = --roundSize;
	}
	
	void SetPathwaySize (u32 i_pathwaySizeInNumMessages)
	{
		u32 roundSize = Jd::Pow2CeilLog2 (i_pathwaySizeInNumMessages);
		m_queue.resize (roundSize);
		m_sequenceMask = --roundSize;
//		cout << "size: " << roundSize * sizeof (MessageRecord) << endl;
	}
	
	/*	- writers atomically increment the 'insert' sequence and then spin/sleep-loop check it against the 'claim' sequence to ensure
		the consumer has cleared enough space in the queue.
		- once the writer has copied the record into the queue, it advances the 'commit' to match 'insert', signaling the consumer a new record
		is available.
		- the claim sequence is incremented by the consumer once it has finished with the record
	 
	 */
	
	u32	GetPathwayNumBytes ()
	{
		return m_queue.capacity () * sizeof (MessageRecord);
	}
	
	
	JdPortSequence					insertSequence	{ c_jdThreadPort_startIndex }; // producer index
	JdPortSequence					commitSequence	{ c_jdThreadPort_startIndex }; // producer "finished-with" index
	JdPortSequence					claimSequence	{ c_jdThreadPort_startIndex }; // consumer index
	
	seq_t							m_sequenceMask;
	vector <MessageRecord>			m_queue;
};


// FIX: kill me and switch to JdMessagePort
template <typename HandlerT, typename ArgT = tp_arg_t>
class JdThreadPort
{
	typedef JdThreadPort <HandlerT, ArgT> ThisT ;

	typedef void (HandlerT::* handler_t) (const ArgT & i_arg);
	typedef tp_arg_t (HandlerT::* replying_handler_t) (const ArgT & i_arg);
	
	public:
	JdThreadPort			(u32 i_sizeOfBufferInNumMessages, HandlerT *i_handler)
	:
	m_handler				(i_handler),
	m_signal				(0)
	{
		m_pathway.SetPathwaySize (i_sizeOfBufferInNumMessages);
	}
	
	
	void SetHandler (HandlerT *i_handler)
	{
		m_handler = i_handler;
	}
	
	protected:
	
	class JdThreadPortSender
	{
		typedef ThisT Port;

		public:
		JdThreadPortSender				()
		:
		m_port							(0)
		{
		}
		
		
		JdThreadPortSender				(Port *i_port)		// make private
		:
		m_port							(i_port)
		{
		}
		
		void Call (handler_t i_method, const ArgT &i_args0 = JdEmptyArgs())
		{
			m_port->SendMethod (i_args0, i_method);
		}

		template <typename A0, typename A1>
		void Call (handler_t i_method, const A0 &i_arg0, const A1 &i_arg1)
		{
			m_port->SendMethod (ArgT (i_arg0, i_arg1), i_method);
		}

		template <typename A0, typename A1, typename A2>
		void Call (handler_t i_method, const A0 &i_arg0, const A1 &i_arg1, const A2 &i_arg2)
		{
			m_port->SendMethod (ArgT (i_arg0, i_arg1, i_arg2), i_method); // FIX: implement 3 arguments
		}

		private:
		Port					*m_port;
	};
	
	public:

	typedef JdThreadPortSender sender_t;
	
	sender_t				CreateSender	()
	{
		return JdThreadPortSender (this);
	}
    
    void Call (handler_t i_method, const ArgT &i_args0 = JdEmptyArgs())
    {
        SendMethod (i_args0, i_method);
    }

	
	struct JdThreadPortRecord;
	struct IJdThreadPortReply
	{
		virtual void SendReply (const tp_arg_t &i_reponse, JdThreadPortRecord *i_record) = 0;
	};

	
	struct JdThreadPortRecord
	{
		handler_t				handler;
		replying_handler_t		replyingHandler;
		ArgT						arguments;
		
		handler_t				replyHandler;
		IJdThreadPortReply*		replyTo;
	};
	

	template <typename ReplyPortHandler>
	class ExpectantSender : public IJdThreadPortReply
	{
		typedef ThisT Port;
		typedef void (ReplyPortHandler::* reply_t) (const ArgT & i_arg);
		
		public:
		ExpectantSender					()		// make private
		:
		m_port							(0),
		m_replyPort						(0)
		{
		}

		ExpectantSender					(Port &i_port, JdThreadPort <ReplyPortHandler> &i_replyPort)		// make private
		:
		m_port							(&i_port),
		m_replyPort						(&i_replyPort)
		{
		}
		
		// ExpectantSender must remain persistent until the reponse is recieved!
		virtual void SendReply (const tp_arg_t &i_reponse, JdThreadPortRecord *i_record)
		{
			m_replyPort->SendMethod (i_reponse, reinterpret_cast<reply_t> (i_record->replyHandler));
		}
		
		void Call (handler_t i_method, const ArgT &i_args = JdEmptyArgs())
		{
			m_port->SendMethod (i_args, i_method);
		}
		
		void CallWithResponse (replying_handler_t i_method, reply_t i_respondToMethod, const ArgT &i_args)
		{
			m_port-> template SendMethodWithResponse <reply_t> (i_args, i_method, this, i_respondToMethod);
		}
		
		private:
	//	JdPortSequence						&m_sequence;
		Port									*m_port;
		JdThreadPort <ReplyPortHandler>		*m_replyPort;
	};

//	typedef JdThreadPortExpectantSender <ThisT>  expectant_sender_t;
	
	template <typename R>
	ExpectantSender <R> CreateSenderWithReplyPort (JdThreadPort <R> &i_port)
	{
		ExpectantSender <R> sender (*this, i_port);
		return sender;
	}
	
	
	void HandleMessages (bool i_waitForMessages = false, i32 i_maxNumMessagesToProcess = 0x7fffffff)
	{
		u32 available = WaitForProducers (i_waitForMessages, i_maxNumMessagesToProcess);

		while (available)
		{
			u32 sequence = m_pathway.claimSequence.Value();
			
			JdThreadPortRecord &record = m_pathway.m_queue [sequence & m_pathway.m_sequenceMask];
			
			if (! record.handler)
			{
				if (!record.replyingHandler) return; // "die" message; Break() condition
				
				tp_arg_t reply = ((m_handler) ->* (record.replyingHandler)) (record.arguments);
				
				record.replyTo->SendReply (reply, &record);
			}
			else ((m_handler) ->* (record.handler)) (record.arguments);
			
			m_pathway.claimSequence.Acquire ();
			
			--available;
		}
	}

	protected://---------------------------------------------------------------------------------------------------------------------------------------------

	public:
	void SendMethod (const ArgT &i_value, handler_t i_handler)
	{
		u32 sequence = m_pathway.insertSequence.Acquire();
		JdThreadPortRecord &record = m_pathway.m_queue [sequence & m_pathway.m_sequenceMask];
		
		const u32 maxSequenceOffset = m_pathway.m_queue.size ();
		
		while (true)
		{
			u32 offset = sequence - m_pathway.claimSequence.Value ();		 // getting an offset here first, instead of doing a one-to-one comparison, solves the issue of wrap-around
			
			if (offset < maxSequenceOffset)
				break;
			
			this_thread::yield ();
//			sleep (0);		// NOTE: for higher-performance throughput (nothing really ever needed for Jigi probably.) but this could spin, then backoff to a sleep.
		}
		
		record.handler = i_handler;
		record.replyingHandler = 0;
		record.arguments = i_value;
				
		m_pathway.commitSequence.Update (sequence+1);	// update returns when all previous sequence numbers have been updated
		SignalConsumer ();
		// is this thread safe? I think so.  One thread could signal before the other, out of sequence, but it's guaranteed
		// that the lower-sequence commit is ready to go 'cause of the Update() synchronization enforced just before that.
	}
	

	template <typename ReplyPortHandler>
	void SendMethodWithResponse (const ArgT &i_value, replying_handler_t i_handler, IJdThreadPortReply *i_replyTo, ReplyPortHandler i_replyHandler)
	{
		u32 sequence = m_pathway.insertSequence.Acquire();
		JdThreadPortRecord &record = m_pathway.m_queue [sequence & m_pathway.m_sequenceMask];
		
		const u32 maxSequenceOffset = m_pathway.m_queue.Size();
		
		while (true)
		{
			u32 offset = sequence - m_pathway.claimSequence.Value ();		 // getting an offset here first, instead of doing a one-to-one comparison, solves the issue of wrap-around
			
			if (offset < maxSequenceOffset)
				break;
			
			this_thread::yield ();
//			sleep (0);		// NOTE: for higher-performance throughput (nothing really ever needed for Jigi probably.) but this could spin, then backoff to a sleep.
		}
		
		record.handler = 0;
		record.replyingHandler = i_handler;
		record.arguments = i_value;
		record.replyHandler = reinterpret_cast <handler_t> (i_replyHandler);
		record.replyTo = i_replyTo;
		
		
		m_pathway.commitSequence.Update (sequence+1);	// update returns when all previous sequence numbers have been updated
		SignalConsumer ();
		// is this thread safe? I think so.  One thread could signal before the other, out of sequence, but it's guaranteed
		// that the lower-sequence commit is ready to go 'cause of the Update() synchronization enforced just before that.
	}
	
	
	inline void SignalConsumer ()
	{
		i32 previous = (i32) atomic_inc32 ((u32 *) &m_signalCount.value);
		if (previous < 0)
		{
			m_signal.Signal ();
		}
		m_handler->SignalMessage ();
	}
	
	
	inline u32 WaitForProducers (bool i_wait, i32 i_maxMessagesToGrab)
	{
		i32 available = atomic_read32 ((u32*) &m_signalCount.value);	// available always monotonically increases from this perspective as the consumer
		
		if (available == 0)
		{
			if (! i_wait) return 0;
			else available = 1; // try to grab at least one, probably triggering a wait below
		}
		
		available = std::min (available, i_maxMessagesToGrab);
		
		i32 previous = (i32) atomic_add32 ((u32 *) &m_signalCount.value, -available);
		
		if (previous < available)
		{
			m_signal.Wait ();
		}
		
		return available;
	}
	

	JdCacheLinePadded <i32>						m_signalCount;
	
	JdThreadPortPathway <JdThreadPortRecord>	m_pathway;

	JdSemaphore									m_signal;
	
	HandlerT *									m_handler;
};


template <u32 t_argsSize = 64>
struct JdArgsWrapper
{
    JdArgsWrapper () {}
	
	template <typename t_arg>
	JdArgsWrapper & operator = (const t_arg &i_arg)
	{
		d_jdAssert (sizeof (t_arg) <= t_argsSize, "arg size overflows thread port capacity");
		
//		cout << "assign at: " << (void *) m_data << endl;
		
		new (m_data) t_arg (i_arg);

		return * this;
	}
	
    template <typename t_arg>
    operator t_arg & ()
    {
        t_arg * ptr = (t_arg *) m_data;
		
//		cout << "read from: " << (void *) m_data << endl;

        return * ptr;
    }
	
	template <typename t_arg>
	void Delete ()
	{
		t_arg * a =  (t_arg *) m_data;
		a->~t_arg ();
	}
	
    u8      m_data [t_argsSize];
};


#define d_epCapture3(ARG0, ARG1, ARG2) struct Capture3 { ARG0,_0; ARG1,_1; ARG2,_2; struct Layout { decltype (_0) a; decltype (_1) b; decltype (_2) c; };};
#define d_epCapture1(ARG0) struct Capture1 { ARG0,_0; struct Layout { decltype (_0) a; };};

#define d_jdPort1(NAME, ARG0) \
		struct NAME##Args { ARG0; }; \
		void NAME (const NAME##Args &i_args = NAME##Args()) \
        { \
            MarshallMethod <NAME##Args> (& port_t :: _##NAME, i_args); \
        } \
        void _##NAME (args_t &i_args) \
        { \
			d_epCapture1 (ARG0); auto * captured = (Capture1::Layout *) &i_args; \
            __##NAME (captured->a); \
        } \
        inline void __##NAME (ARG0)


#define d_jdPort6(NAME, ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)		struct NAME##Args { ARG0; ARG1; ARG2; ARG3; ARG4; ARG5; }; d_jdXPort (NAME)
#define d_jdPort5(NAME, ARG0, ARG1, ARG2, ARG3, ARG4)			struct NAME##Args { ARG0; ARG1; ARG2; ARG3; ARG4; }; d_jdXPort (NAME)
#define d_jdPort4(NAME, ARG0, ARG1, ARG2, ARG3)					struct NAME##Args { ARG0; ARG1; ARG2; ARG3 }; d_jdXPort (NAME)
#define d_jdPort3(NAME, ARG0, ARG1, ARG2)						struct NAME##Args { ARG0; ARG1; ARG2; }; d_jdXPort (NAME)
#define d_jdPort2(NAME, ARG0, ARG1)								struct NAME##Args { ARG0; ARG1; }; d_jdXPort (NAME)
//#define d_jdPort1(NAME, ARG0)									; d_jdXPort (NAME)
#define d_jdPort0(NAME)											struct NAME##Args { }; d_jdXPort (NAME)

#define _jdPortChooser(...) d_get7thArg (__VA_ARGS__, d_jdPort6, d_jdPort5, d_jdPort4, d_jdPort3, d_jdPort2, d_jdPort1, d_jdPort0 )

#define d_jdPort(...) _jdPortChooser(__VA_ARGS__)(__VA_ARGS__)



#define d_jdMsgPort(NAME) \
		template <typename... Args> \
		void NAME (Args... i_args) { MarshallMethod (& port_t :: _##NAME, Epigram128 (i_args...)); } \
		void _##NAME (args_t & i_args) { __##NAME (i_args); i_args.Delete <Epigram128> (); } \
        void __##NAME (Epigram128 & i_)

#define d_jdMsgReplyPort(NAME) \
		template <typename... Args> Epigram NAME (Args... i_args) { return MarshallMethod (& port_t :: _##NAME, Epigram128 (i_args...)); } \
		Epigram _##NAME (args_t & i_args) { Epigram reply = __##NAME (i_args); i_args.Delete <Epigram128> (); return reply; } \
        Epigram __##NAME (Epigram128 & i_)

#define d_jdMsgResultPort(NAME) \
		template <typename... Args> JdResult NAME (Args... i_args) { return MarshallMethod (& port_t :: _##NAME, Epigram128 (i_args...)); } \
		JdResult _##NAME (args_t & i_args) { JdResult result = __##NAME (i_args); i_args.Delete <Epigram128> (); return result; } \
        JdResult __##NAME (Epigram128 & i_)


//#define i_(VAR) VAR = i_[#VAR]

//-----------------------------------------------------------------------------------------------------------------------------------------------------

#define d_jdXPortFunc(NAME) \
		void NAME (const NAME##Args &i_args = NAME##Args()) \
        { \
            MarshallMethod <NAME##Args> (& port_t :: _##NAME, i_args); \
        } \
        void _##NAME (args_t & i_args) \
        { \
            __##NAME (i_args); \
			i_args.Delete <NAME##Args> (); \
        } \
        void __##NAME (const NAME##Args &i_)


#define d_jdPortFunc6(NAME, ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)		struct NAME##Args { ARG0; ARG1; ARG2; ARG3; ARG4; ARG5; }; d_jdXPortFunc (NAME)
#define d_jdPortFunc5(NAME, ARG0, ARG1, ARG2, ARG3, ARG4)			struct NAME##Args { ARG0; ARG1; ARG2; ARG3; ARG4; }; d_jdXPortFunc (NAME)
#define d_jdPortFunc4(NAME, ARG0, ARG1, ARG2, ARG3)					struct NAME##Args { ARG0; ARG1; ARG2; ARG3; }; d_jdXPortFunc (NAME)
#define d_jdPortFunc3(NAME, ARG0, ARG1, ARG2)						struct NAME##Args { ARG0; ARG1; ARG2; }; d_jdXPortFunc (NAME)
#define d_jdPortFunc2(NAME, ARG0, ARG1)								struct NAME##Args { ARG0; ARG1; }; d_jdXPortFunc (NAME)
#define d_jdPortFunc1(NAME, ARG0)									struct NAME##Args { ARG0; }; d_jdXPortFunc (NAME)
#define d_jdPortFunc0(NAME)											struct NAME##Args { }; d_jdXPortFunc (NAME)

#define d_jdPortFuncChooser(...) d_get7thArg (__VA_ARGS__, d_jdPortFunc6, d_jdPortFunc5, d_jdPortFunc4, d_jdPortFunc3, d_jdPortFunc2, d_jdPortFunc1, d_jdPortFunc0 )

#define d_jdPortFunc(...) d_jdPortFuncChooser(__VA_ARGS__)(__VA_ARGS__)

// ***********************************************************************************************************************************
// ** Latest *************************************************************************************************************************
// ***********************************************************************************************************************************


d_jdTrueFalse (MessagePort, waitForMessages, doNotWaitForMessages)

// JdMessageQueue2 (and all prior works) *aren't* multiple consumer safe nor easily compatible because of the disconnected atomic signal variable
// and the sporadically triggered semaphore.  one thread might be expecting the signal but a different thread takes it.

template <typename t_record>
class JdMessageQueue2
{
	public:
	
	JdMessageQueue2								(u32 i_numMessagesInQueue = 512)
	{
		m_pathway.SetPathwaySize (i_numMessagesInQueue);
	}
	
	u32 GetQueueSizeInBytes ()
	{
		return m_pathway.GetPathwayNumBytes ();
	}
	
	
	void QueueMessage (const t_record & i_message)
	{
		u32 sequence;
		auto & slot = *AcquireMessageSlot (sequence);
		slot = i_message;
		CommitMessage (sequence);
	}
	
	inline t_record * AcquireMessageSlot (u32 & o_sequence)
	{
		o_sequence = m_pathway.insertSequence.Acquire();
		
		u32 slotIndex = o_sequence & m_pathway.m_sequenceMask;
		t_record * record = & m_pathway.m_queue [slotIndex];
		
		const u32 maxSequenceOffset = m_pathway.m_queue.size ();

		u32 tries = 0;
		
		while (true)
		{
			
			// getting an offset here first, instead of doing a one-to-one comparison, solves the issue of wrap-around
			u32 offset = o_sequence - m_pathway.claimSequence.Value ();
			
			if (offset < maxSequenceOffset)
				break;
			
			this_thread::yield ();
			// sleep (0);
			// NOTE: for higher-performance throughput (nothing really ever needed for Jigi probably.)
			// but this could spin, then backoff to a sleep/yield
			// dunnno the diff between sleep (0) & yield... they may be equivalent
			
//			cout << ++tries << endl;
			
			if (++tries > 100000000)
				d_jdThrow ("Deadlock. Or, more likely, you've blown out the queue. Each sequence (lock) can push up to 512 transactions (calls).");
		}
		
		return record;
	}
	
	
	inline
	void CommitMessage (u32 i_sequence)
	{
		m_pathway.commitSequence.Update (i_sequence + 1);	// update returns when all previous sequence numbers have been updated
		
		i32 previous = (i32) atomic_inc32 ((u32 *) &m_signalCount.value);
		if (previous < 0)
		{
			m_signal.Signal ();
		}
	}
	
	inline u32 ClaimMessages () // don't wait; grab all available
	{
		i32 available = atomic_read32 ((u32*) &m_signalCount.value);	// available always monotonically increases from this perspective as the consumer

		if (available)
		{
//			i32 previous = (i32)
			atomic_add32 ((u32 *) &m_signalCount.value, -available);
		}
		
		return available;
	}
	
	inline u32 WaitForMessages (bool i_wait = true, i32 i_maxMessagesToGrab = std::numeric_limits <i32>::max ())
	{
		d_jdAssert (m_acquiredPending == 0, "implement");
		
		i32 available = atomic_read32 ((u32*) &m_signalCount.value);	// available always monotonically increases from this perspective as the consumer
		
		if (available == 0)
		{
			if (not i_wait)
				return 0;
			else
				available = 1; // try to grab at least one, probably triggering a wait below
		}
		
		available = std::min (available, i_maxMessagesToGrab);
		
		i32 previous = (i32) atomic_add32 ((u32 *) &m_signalCount.value, -available);
		
		if (previous < available)
		{
			m_signal.Wait ();
		}
		
		return available;
	}

	inline u32 TimedWaitForMessages (u32 i_timeoutInMicroseconds, i32 i_maxMessagesToGrab = std::numeric_limits <i32>::max ())
	{
		i32 available;
		
		if (m_acquiredPending == 0) // no previous timeout occurred...
		{
			available = atomic_read32 ((u32 *) &m_signalCount.value);	// available always monotonically increases from this perspective as the consumer
			
			if (available == 0)
			{
				if (i_timeoutInMicroseconds == 0)
					return 0;
				else
					available = 1; // try to grab at least one, probably triggering a wait below
			}
			
			available = std::min (available, i_maxMessagesToGrab);
			
			i32 previous = (i32) atomic_add32 ((u32 *) &m_signalCount.value, -available);
			
			if (previous < available)
			{
				if (not m_signal.TimedWait (i_timeoutInMicroseconds))
				{
					m_acquiredPending = available;
					available = 0;
				}
			}
		}
		else
		{
			if (m_signal.TimedWait (i_timeoutInMicroseconds))
			{
				available = m_acquiredPending;
				m_acquiredPending = 0;
			}
			else
				available = 0;
		}
		
		return available;
	}
	
	inline t_record * ViewMessage (u32 i_index = 0)
	{
		u32 sequence = m_pathway.claimSequence.Value() + i_index;
		sequence &= m_pathway.m_sequenceMask;
		
		return & m_pathway.m_queue [sequence];
	}
	
	inline void ReleaseMessage ()
	{
		m_pathway.claimSequence.Acquire ();
	}
	
	inline void ReleaseMessages (u32 i_numMessages)
	{
		while (i_numMessages--)
			m_pathway.claimSequence.Acquire ();
	}
	
	bool GetMessage (t_record & o_message)
	{
		i32 available = atomic_read32 ((u32*) &m_signalCount.value);
		
		if (available)
		{
			available = 1;
			atomic_add32 ((u32 *) &m_signalCount.value, -available);
			o_message = * ViewMessage ();
			ReleaseMessage ();

			return true;
		}
		else return false;
	}

	protected:
	
	JdCacheLinePadded <i32>						m_signalCount;
	JdSemaphore									m_signal			{ 0 };
	u32											m_acquiredPending	= 0;
	
	JdThreadPortPathway <t_record>				m_pathway;
};



// This is a multiple-producer/single-consumer message queue... I think.
template <typename t_handler, u32 t_argsSize = 64>
class JdMessagePortT
{
    /*
     
     Usage: Inherit from "public JdMessagePortT <...DerivedClass...>" e.g.,
     
            public MyPortedClass : public JdMessagePortT <MyPortedClass>
            {
            };
     
     */
    
    protected:
	
    typedef JdArgsWrapper <t_argsSize> args_t;

    typedef t_handler port_t; // used in macros
	
	typedef JdResult (t_handler::* resultHandler_t)			(args_t & i_arg);
	typedef Epigram (t_handler::* replyingHandler_t)		(args_t & i_arg);
	typedef void (t_handler::* handler_t)					(args_t & i_arg);
	
	public:
	
	JdMessagePortT          (u32 i_queueSize = 512)
	:
	m_queue					(i_queueSize)
	{
        m_handler = static_cast <t_handler *> (this);
	}
	
    void SendBreak ()
    {
        struct {} dummy;
        MarshallMethod ((handler_t) nullptr, dummy);
    }
	
    public:
	
	struct IJdThreadPortReply
	{
		virtual void SendReply (EpigramRef i_reply) {}
		virtual void SendReply (const JdResult & i_result) {}
	};
	
	template <typename T>
	struct JdThreadPortReply : public IJdThreadPortReply
	{
		virtual void SendReply (const T & i_reply)
		{
			m_reply = i_reply;
			m_signal.Signal ();
		}
		
		T WaitForReply ()
		{
			m_signal.Wait ();
			return m_reply;
		}

		JdSemaphore					m_signal;
		T							m_reply;
	};
	
	
	struct JdThreadPortRecord
	{
		args_t                      arguments;
		
		union
		{
			handler_t                   handler;
			replyingHandler_t			replyingHandler;
		};
		
		resultHandler_t				resultingHandler;
		IJdThreadPortReply *		replyTo;
	};
	
	
	bool HandleMessages (bool i_waitForMessages = false, i32 i_maxNumMessagesToProcess = 0x7fffffff)
	{
		u32 available = m_queue.WaitForMessages (i_waitForMessages, i_maxNumMessagesToProcess);
		
		return ProcessMessages (available);
	}
	
	bool HandleMessages (u32 i_timeoutInMilliseconds, i32 i_maxNumMessagesToProcess = 0x7fffffff)
	{
		u32 available = m_queue.TimedWaitForMessages (i_timeoutInMilliseconds, i_maxNumMessagesToProcess);
		
		return ProcessMessages (available);
	}
	
	bool ProcessMessages (u32 i_available)
	{
		while (i_available)
		{
			JdThreadPortRecord * r = m_queue.ViewMessage ();
			
			if (r->handler)
			{
				if (r->replyTo)
				{
					Epigram reply = (m_handler ->* r->replyingHandler) (r->arguments);
					r->replyTo->SendReply (reply);
				}
				else
					(m_handler ->* r->handler) (r->arguments);
			}
			else
			{
				if (r->replyTo && r->resultingHandler)
				{
					JdResult result = (m_handler ->* r->resultingHandler) (r->arguments);
					r->replyTo->SendReply (result);
				}
				else return true;  // a Break() message
			}
			
			m_queue.ReleaseMessage ();
			
			--i_available;
		}
		
		return false;
	}

	
	protected://--------------------------------------------------------------------------------------------------------------------------------------
    template <typename t_args>
	void MarshallMethod (handler_t i_function, const t_args &i_args)
	{
		u32 sequence;
		JdThreadPortRecord * record = m_queue.AcquireMessageSlot (sequence);

		record->handler = i_function;
		record->replyTo = nullptr;
		record->arguments = i_args;
		
		m_queue.CommitMessage (sequence);
	}
	
    
	template <typename t_args>
	Epigram MarshallMethod (replyingHandler_t i_handler, const t_args &i_args)
	{
		u32 sequence;
		JdThreadPortRecord * record = m_queue.AcquireMessageSlot (sequence);

		JdThreadPortReply <Epigram> replyHandler;
		
		record->handler = nullptr;
		record->replyingHandler = i_handler;
		record->arguments = i_args;
		record->replyTo = & replyHandler;
		
		m_queue.CommitMessage (sequence);
		
		return replyHandler.WaitForReply ();
	}

	template <typename t_args>
	JdResult MarshallMethod (resultHandler_t i_handler, const t_args &i_args)
	{
		u32 sequence;
		JdThreadPortRecord * record = m_queue.AcquireMessageSlot (sequence);
		
		JdThreadPortReply <JdResult> replyHandler;
		
		record->handler = nullptr;
		record->resultingHandler = i_handler;
		record->arguments = i_args;
		record->replyTo = & replyHandler;
		
		m_queue.CommitMessage (sequence);
		
		return replyHandler.WaitForReply ();
	}
	
	
	//------------------------------------------------------------------------------------------
	JdMessageQueue2 <JdThreadPortRecord>		m_queue;
	t_handler *									m_handler;
};


#include "JdThread.hpp"

template <typename Handler>
class JdPortedThread : public JdThread, public JdThreadPort <Handler>
{
	typedef JdThreadPort <Handler> Port;
	typedef typename JdThreadPort <Handler>::JdThreadPortRecord Record;
	
	public:
	JdPortedThread					(cstr_t i_threadName, u32 i_portSizeInNumMessages, u8 i_priority = c_jdThread_defaultPriority)
	:
	JdThread						(i_threadName, i_priority),
	Port							(i_portSizeInNumMessages, 0),
	m_handler						(new Handler)
	{
		Port::SetHandler (m_handler);
	}

	virtual ~JdPortedThread		()
	{
		delete m_handler;
	}

	Handler *			Get		()
	{
		d_jdAssert (GetState() == c_jdThreadState::pending || GetState() == c_jdThreadState::quit, "shouldn't be tapping the class directly when the thread is active");
		return m_handler;
	}
	
	protected:		
	
	virtual JdResult		Run 		()
	{
		bool waitOnNewMessages = false;
		while (IsAlive ())
		{
			Port::HandleMessages (waitOnNewMessages, 11);	// FIX: this 11 needs to be configurable and possibly dynamic
														// or based on a time threshold (process messages until X milliseconds exceeded)
			waitOnNewMessages = m_handler->Idle ();
		}
		
		Port::HandleMessages (false, 0x7fffffff); // finish off anything in the queue.
		
		return c_jdNoErr;
	}
	
	virtual void			Break ()
	{
		Port::SendMethod ("die", 0);
	}
	
	Handler *			m_handler;
};



template <typename t_thread, u32 t_argSize = 64>
struct JdThreadPort2 : public JdMessagePortT <t_thread, t_argSize>, public IJdThread
{
    JdThreadPort2 (u32 i_queueSize = 512)
    :
    JdMessagePortT <t_thread, t_argSize> (i_queueSize)
    {
//        idles = 0;
    }
    
//    ~JdThreadPort2 ()
//    {
////        cout << "idles: " << idles << endl;
//    }
	
	
	// QUE: add IThreadInfo NeedsIdle () ?  Or IdlePeriod ()
    virtual JdResult                Run             (const Epigram & i_args, IThreadInfo & i_info)
    {
		bool waitOnNewMessages = false;
        
        i32 numMessages = 512;   // FIX: dynamic
        
		while (true)
		{
			if (this->HandleMessages (waitOnNewMessages, numMessages))
                break; // got SendBreak() message

            u64 ms = m_timer.GetMilliseconds ();
            if (ms >= m_nextIdle)
            {
                waitOnNewMessages = Idle ();
                m_nextIdle = ms + 1;
            }
		}
		
        return c_jdNoErr;
    }
    
	virtual JdResult			Break ()
	{
        this->SendBreak ();
        return c_jdNoErr;
	}
    
	virtual bool Idle ()
	{
//        ++idles;
		return true; // yes, wait on new messages
	}
	
	virtual void SignalMessage () { }
    
    protected:
    u32                 idles;
    JdTimer             m_timer;
    u64                 m_nextIdle			= 0;
};


#endif /* defined(__Jigidesign__JdThreadPort__) */
