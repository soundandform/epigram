//
//  JdMessageQueue
//
//  Copyright (c) 2012-2018 Steven Massey. All rights reserved.
//
#ifndef JdMessageQueue_hpp
#define JdMessageQueue_hpp

# include <iostream>
# include <atomic>
# include <deque>

using std::atomic, std::mutex, std::condition_variable, std::lock_guard, std::unique_lock, std::deque;

#include "JdAssert.hpp"
#include "JdSemaphore.hpp"

typedef u64 seq_t;

const int c_jdCacheLineBytes = 64;

// I really don't know if this cache sharing prevention acheives anything...
template <typename T>
struct /* alignas (c_jdCacheLineBytes) */ JdCacheLinePadded
{
	JdCacheLinePadded () {	d_jdAssert (sizeof (JdCacheLinePadded) == c_jdCacheLineBytes, "JdCacheLinePadded not aligned properly"); }

	T					value	{ 0 };
    volatile			u8 		_cacheLinePadding [c_jdCacheLineBytes - sizeof (T)];
};


class JdPortSequence : protected JdCacheLinePadded <atomic <seq_t>>
{
	public:
					JdPortSequence		(seq_t i_initTo)
	{
		value = i_initTo;
		memset ((void *) & _cacheLinePadding [0], 0x0, Jd::SizeOfArray (_cacheLinePadding));
	}
	
	
	seq_t				Acquire				() 							{ return value++; }
	void				Acquire				(seq_t i_value)				{ value += i_value; }
	
	void				Update				(seq_t i_newSequenceNum)
	{
		while (true)
		{
			seq_t previousSeq = i_newSequenceNum - 1;
			if (value.compare_exchange_weak (previousSeq, i_newSequenceNum))
				return;
			
			std::this_thread::yield ();  // TODO: could improve?
		}
	}
	
	seq_t				Value				() 		{ return value; }
						operator seq_t		()		{ return value; }
};


const u32 c_jdThreadPort_startIndex = 0xffffffffffffff71;	// to prove these mechanisms are integer wrap-around safe


template <typename MessageRecord>
struct JdThreadPortPathway
{
	JdThreadPortPathway	() { }
	
//	JdThreadPortPathway	(u32 i_pathwaySizeInNumMessages)
//	{
//		u32 roundSize = Jd::Pow2CeilLog2 (i_pathwaySizeInNumMessages);
//		m_queue.Resize (roundSize);
//		m_sequenceMask = --roundSize;
//	}
	
	void SetPathwaySize (u32 i_pathwaySizeInNumMessages)
	{
		u32 roundSize = Jd::Pow2CeilLog2 (i_pathwaySizeInNumMessages);
		m_queue.resize (roundSize);
		m_sequenceMask = --roundSize;
	}
	
	/*	- writers atomically increment the 'insert' sequence and then spin/sleep-loop check it against the 'claim' sequence to ensure
		the consumer has cleared enough space in the queue.
		- once the writer has copied the record into the queue, it advances the 'commit' to match 'insert', signaling the consumer a new record
		is available.
		- the claim sequence is incremented by the consumer once it has finished with the record
	 */
	
	u32	get_pathway_num_bytes ()
	{
		return m_queue.capacity () * sizeof (MessageRecord);		// note: capacity () not size (). this is a diagnostic API
	}
	
	
	JdPortSequence					insertSequence		{ c_jdThreadPort_startIndex }; // producer index
	JdPortSequence					commitSequence		{ c_jdThreadPort_startIndex }; // producer "finished-with" index
	JdPortSequence					claimSequence		{ c_jdThreadPort_startIndex }; // consumer index  -- this doesn't need to be an atomic in single consumer case; mutex protected in Pop/PopWait.
	
	seq_t							m_sequenceMask;
	std::vector <MessageRecord>		m_queue;
};


d_jdTrueFalse (MessagePort, waitForMessages, doNotWaitForMessages)


template <typename T>
class JdMessageQueue
{
	// JdMessageQueue is at it's core a multiple writer - single reader safe message queue.
	// The Pop () and PopWait () functions add a mutex so that it can be multi-consumer safe/

	public:
	
	JdMessageQueue								(u32 i_numMessagesInQueue = 512)
	{
		m_pathway.SetPathwaySize (i_numMessagesInQueue);
	}
	
//	~ JdMessageQueue () {}
	
	u32  GetNumMessageCapacity  ()
	{
		return m_pathway.m_queue.size ();
	}
	
	u32 get_queue_size_in_bytes ()	// this is the amount of memory actually allocated
	{
		return m_pathway.get_pathway_num_bytes ();
	}
	
	
	// producer --------------------------------------------------------------------------------------------------------------------------------
	// 1. AcquireMessageSlot ()
	// 2. copy the message to the pointer
	// 3. CommitMessage ()
	//
	// alternatively, use Push () which packages these three steps into one call
	
	void Push (const T * i_messages, u32 i_count)
	{
		seq_t sequence;
		while (i_count--)
		{
			auto slot = AcquireMessageSlot (sequence);
			*slot = * i_messages++;
			CommitMessage (sequence);
		}
	}

	template <typename P>
	void Push (P const & i_message)
	{
		seq_t sequence;
		auto & slot = * AcquireMessageSlot (sequence);
		slot = i_message;
		CommitMessage (sequence);
	}

	
	bool Push (const T & i_message, u32 i_waitMilliseconds)
	{
		seq_t sequence;
		auto slot = AcquireMessageSlot (sequence, i_waitMilliseconds);
		
		if (slot)
		{
			* slot = i_message;
			CommitMessage (sequence);
			return true;
		}
		else
		{
			lock_guard <mutex> lock (m_consumerLock);
			m_failedSequences.push_back (sequence);
			return false;
		}
	}
	
	
	inline
	void CommitMessage (seq_t i_sequence)
	{
		m_pathway.commitSequence.Update (i_sequence + 1);	// update returns when all previous sequence numbers have been updated
		
		i64 previous = m_signalCount.value++;

		if (previous < 0)
			m_signal.Signal ();
	}
	
	//-----------------------------------------------------------------------------------------------------------------------------------------------------
	// consumer is similar to producer
	// 1. ClaimMessages () returns a count of available messages
	// 2. Use ViewMessage () to peak at the messages
	// 3. ReleaseMessage/s () when done with them.
	// alternatively,
	
	inline u32 ClaimMessages (u32 i_maxMessagesToClaim = std::numeric_limits <u32>::max ())
	{
		i64 available = m_signalCount.value;	// available always monotonically increases from this perspective as the consumer

		available = std::min (available, (i64) i_maxMessagesToClaim);
		
		m_signalCount.value -= available;
		
		return available;
	}
	
	
	inline u32 WaitForMessages (i32 i_maxMessagesToGrab)
	{
		i64 available = m_signalCount.value;	// available always monotonically increases from this perspective as the consumer
		
		available = std::max (available, (i64) 1); // wait for at least one
		available = std::min (available, (i64) i_maxMessagesToGrab);
		
		i64 previous = m_signalCount.value.fetch_sub (available);
		
		if (previous < available)
			m_signal.Wait ();
		
		return available;
	}
	
	
	inline u32 WaitForMessages (bool i_wait = true, i32 i_maxMessagesToGrab = std::numeric_limits <i32>::max ())
	{
		d_jdAssert (m_acquiredPending == 0, "implement");
		
		i64 available = m_signalCount.value;	// available always monotonically increases from this perspective as the consumer
		
		if (available == 0)
		{
			if (not i_wait)
				return 0;
			else
				available = 1; // try to grab at least one, probably triggering a wait below
		}
		
		available = std::min (available, (i64) i_maxMessagesToGrab);
		
		i64 previous = m_signalCount.value.fetch_sub (available);
		
		if (previous < available)
			m_signal.Wait ();
		
		return available;
	}

	
	inline u32 TimedWaitForMessages (u32 i_timeoutInMicroseconds, i32 i_maxMessagesToGrab = std::numeric_limits <i32>::max ())
	{
		i64 available;
		
		if (m_acquiredPending == 0) // no previous timeout occurred...
		{
			available = m_signalCount.value; // available always monotonically increases from this perspective as the consumer
			
			if (available == 0)
			{
				if (i_timeoutInMicroseconds == 0)
					return 0;
				else
					available = 1; // try to grab at least one, probably triggering a wait below
			}
			
			available = std::min (available, (i64) i_maxMessagesToGrab);
			
			i64 previous = m_signalCount.value.fetch_sub (available);
			
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
			else available = 0;
		}
		
		return available;
	}

	inline T & DerefMessage (seq_t i_index = 0)
	{
		seq_t sequence = m_pathway.claimSequence + i_index;
		sequence &= m_pathway.m_sequenceMask;
		
		return m_pathway.m_queue [sequence];
	}
	
	inline T * ViewMessage (seq_t i_index = 0)
	{
		return & DerefMessage (i_index);
	}
	
	inline void ReleaseMessage ()
	{
		DerefMessage () = T ();				// reset slot (release shared pointers, strings, whatever)
		
		m_pathway.claimSequence.Acquire ();

		m_condition.notify_all ();
	}
	
	inline void ReleaseMessages (seq_t i_numMessages)
	{
		// FIX: * ViewMessage () = T ();
		m_pathway.claimSequence.Acquire (i_numMessages);
		
		m_condition.notify_all ();
	}
	
	
	//--- Pop () and PopWait () are locked w/ a mutex for multi-consumer support ----------------------------------------------------------------
	
	bool Pop (T & o_message)
	// no wait. claims and releases one message if it's available
	{
		lock_guard <mutex> lock (m_consumerLock);

		if (m_failedSequences.size ()) FlushQueueOverflows ();

		i64 available = m_signalCount.value;
		
		if (available)
		{
			m_signalCount.value--;
			o_message = DerefMessage ();
			ReleaseMessage ();

			return true;
		}
		else return false;
	}

	
	inline void PopWait (T & o_message)
	{
		lock_guard <mutex> lock (m_consumerLock);
		
		if (m_failedSequences.size ()) FlushQueueOverflows ();
		
		WaitForMessages (1);
		o_message = DerefMessage ();
		ReleaseMessage ();
	}

	
	inline bool PopWait (T & o_message, u32 i_microsecondTimeout)
	{
		lock_guard <mutex> lock (m_consumerLock);
		
		if (m_failedSequences.size ()) FlushQueueOverflows ();
		
		if (TimedWaitForMessages (i_microsecondTimeout, 1))
		{
			o_message = DerefMessage ();
			ReleaseMessage ();
			return true;
		}
		else return false;
	}
	
	
	i64 debug_get_num_messages_in_queue ()
	{
		return m_signalCount.value;
	}
	
	
	protected:
	
	
	inline T * AcquireMessageSlot (seq_t & o_sequence, u32 i_waitMilliseconds = std::numeric_limits <u32>::max ())
	{
		o_sequence = m_pathway.insertSequence.Acquire ();
		
		seq_t slotIndex = o_sequence & m_pathway.m_sequenceMask;
		T * record = & m_pathway.m_queue [slotIndex];
		
		const seq_t maxSequenceOffset = m_pathway.m_queue.size ();

		bool tried = false;
		
		while (true)
		{
			// getting an offset here first, instead of doing a one-to-one comparison, solves the issue of wrap-around
			seq_t offset = o_sequence - m_pathway.claimSequence;
			
			if (offset < maxSequenceOffset)
				break;

			std::unique_lock <std::mutex> lock (m_conditionLock);
			m_condition.wait_for (lock, std::chrono::milliseconds (i_waitMilliseconds >> 1));	// div/2 because we loop twice
			
			if (tried)
			{
				record = nullptr;
				break;
			}
			else tried = true;
		}
		
		return record;
	}


	
	void  FlushQueueOverflows ()
	{
		// m_consumerLock must be locked
		while (m_pathway.claimSequence == m_failedSequences.front ())
		{
			CommitMessage (m_pathway.claimSequence);
			ReleaseMessage ();
			m_failedSequences.pop_front ();
		}
	}

	
	protected:
	
	mutex										m_consumerLock;		// makes Pop methods multi-consumer safe
	
	JdCacheLinePadded <atomic <i64>>			m_signalCount;
	JdSemaphore									m_signal			{ 0 };
	i64											m_acquiredPending	= 0;

	deque <seq_t>								m_failedSequences;
	
	mutex										m_conditionLock;
	condition_variable							m_condition;

	JdThreadPortPathway <T>						m_pathway;
};


template <typename T>
struct JdMpMcQueueT
{
	void SetMaxNumElements (u32 i_numElements)
	{
		m_maxNumElements = Jd::RoundUpToAPowerOf2 (i_numElements);
		m_queue.resize (m_maxNumElements);
	}
	
	void Push (T && i_value)
	{
		unique_lock <mutex> lock (m_mutex);
		
		while (m_queue.size () == m_maxNumElements)
			m_queueNotFull.wait (lock);

		m_queue.push_front (i_value);
		
		if (m_queue.size () == 1)
			m_queueNotEmpty.notify_one ();
	}
	
	T Pop ()
	{
		unique_lock <mutex> lock (m_mutex);
		
		while (m_queue.empty ())
			m_queueNotEmpty.wait (lock);

//		T back = m_queue.back ();
		T back = m_queue.pop_back ();
		
		if (m_queue.size () == m_maxNumElements - 1)
			m_queueNotFull.notify_one ();

		return back;
	}
	
	
	bool Pop (T & o_value, u32 i_microsecondsWait)
	{
		bool notifyProducer;
		
		{
			unique_lock <mutex> lock (m_mutex);
			
			while (m_queue.empty ())
			{
				auto waitTime = std::chrono::microseconds (i_microsecondsWait);
				
				if (m_queueNotEmpty.wait_for (lock, waitTime) == std::cv_status::timeout)
					return false;
			}
			
			notifyProducer = (m_queue.size () == m_maxNumElements);

			o_value = m_queue.pop_back ();
//			m_queue.pop_back ();
		}
		
		if (notifyProducer)
			m_queueNotFull.notify_one ();

		return true;
	}
	
//	template <typename TT>
	struct queue
	{
		void resize (size_t i_size)
		{
			m_elements.resize (i_size);
			m_mask = i_size - 1;
			
		}
		
		size_t size () const
		{
			return m_frontIndex - m_backIndex;
		}
		
		bool empty () const
		{
			return size () == 0;
		}
		
		T pop_back ()
		{
			return m_elements [m_backIndex++ & m_mask];
		}

		
		void push_front (T & value)
		{
			m_elements [m_frontIndex++ & m_mask] = value;
		}

		
		size_t			m_frontIndex;
		size_t			m_backIndex;
		size_t			m_mask;
		std::vector <T>	m_elements;
	};
	
//	deque <T>									m_queue;
	
	queue										m_queue;
	
	mutex										m_mutex;
	condition_variable							m_queueNotEmpty;
	condition_variable							m_queueNotFull;
	u32											m_maxNumElements		= std::numeric_limits <u32>::max ();
};



#if KILL
// Hmm... does packetizing has more overhead than just pushing single values through the already efficient MessageQueue?
template <typename T, u32 t_packetSize = 32>
struct JdThreadStream
{
	struct Packet
	{
		T 	values  	[t_packetSize];
	};
	
	JdThreadStream ()
	{
	}
	
	void	Insert			(u32 i_count, const T * i_values)
	{
		if (m_inPending)
		{
			size_t required = t_packetSize - m_inPending;
			size_t available = std::min (required, (size_t) i_count);
			
			memcpy (& m_inPacket.values [m_inPending], i_values, sizeof (T) * available);
			
			i_count -= available;
			i_values += available;
			m_inPending += available;
			
			if (m_inPending == t_packetSize)
			{
				m_queue.Push (m_inPacket);
				m_inPending = 0;
			}
		}
		
		while (i_count >= t_packetSize)
		{
			auto packet = (Packet *) i_values;
			m_queue.Push (* packet);
			
			i_count -= t_packetSize;
			i_values += t_packetSize;
		}
		
		if (i_count)
		{
			memcpy (& m_inPacket.values [0], i_values, sizeof (T) * i_count);
			m_inPending = i_count;
		}
	}
	
	
	bool	TryFetch		(T * o_values, u32 i_required)
	{
	}
	
	
	void	Fetch			(T * o_values, u32 i_required)
	{
		if (m_outPending)
		{
			size_t required = std::min ((size_t) i_required, m_outPending);

			auto i = t_packetSize - m_outPending;
			memcpy (o_values, & m_outPacket.values [i], required * sizeof (T));
			
			m_outPending -= required;
			o_values += required;
			i_required -= required;
		}
		
		while (i_required >= t_packetSize)
		{
			auto packet = (Packet *) o_values;
			m_queue.PopWait (* packet);
			
			o_values += t_packetSize;
			i_required -= t_packetSize;
		}
		
		if (i_required)
		{
			m_queue.PopWait (m_outPacket);
			
			memcpy (o_values, & m_outPacket.values [0], i_required * sizeof (T));
			m_outPending = t_packetSize - i_required;
		}
	}
	
	
//	void	SetSize			(u32 i_size)
//	{
//	}
	
	protected:
	
	JdMessageQueue <Packet>		m_queue;

	Packet						m_inPacket;
	size_t						m_inPending						= 0;

	Packet						m_outPacket;
	size_t						m_outPending					= 0;
	
};
#endif



#endif /* JdMessageQueue_hpp */
