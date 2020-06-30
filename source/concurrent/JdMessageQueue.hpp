//
//  JdMessageQueue
//
//  Copyright (c) 2012-2018 Steven Massey. All rights reserved.
//
#ifndef JdMessageQueue_hpp
#define JdMessageQueue_hpp

#include <iostream>

#include "JdAssert.hpp"
#include "JdSemaphore.hpp"

typedef u64 seq_t;

const int c_jdCacheLineBytes = 64;

// I really don't know if this cache sharing prevention acheives anything...
template <typename T>
struct alignas (c_jdCacheLineBytes) JdCacheLinePadded
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
	
	
	seq_t				Acquire				()
	{
		return value++; //atomic_inc32 (& value);
	}
	
	
	void				Update				(seq_t i_newSequenceNum)
	{
		while (true)
		{
			seq_t previousSeq = i_newSequenceNum - 1;
			if (value.compare_exchange_strong (previousSeq, i_newSequenceNum))
				return;
			
			sleep (0); // FIX: improve
		}
	}
	
	seq_t				Value				()
	{
		return value;
	}
	
	operator seq_t							()
	{
		return value;
	}
};


const u32 c_jdThreadPort_startIndex = 0xffffffffffffff71;	// to prove these mechanisms are integer wrap-around safe


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


d_jdTrueFalse (MessagePort, waitForMessages, doNotWaitForMessages)

// JdMessageQueue isn't multiple consumer safe nor easily compatible because of the disconnected atomic signal variable
// and the sporadically triggered semaphore.  one thread might be expecting the signal but a different thread takes it. Two multiple consumer
// strategies: i think just adding a comsumer mutex would do the trick and/or mutiple channels of MessageQueues

template <typename T>
class JdMessageQueue // (v2)
{
	public:
	
	JdMessageQueue								(u32 i_numMessagesInQueue = 512)
	{
		m_pathway.SetPathwaySize (i_numMessagesInQueue);
	}
	
	~ JdMessageQueue ()
	{
		cout << "numSleeps: " << m_numSleeps << endl;
	}
	
	u32 GetQueueSizeInBytes ()
	{
		return m_pathway.GetPathwayNumBytes ();
	}
	
	
	// producer --------------------------------------------------------------------------------------------------------------------------------
	// 1. AcquireMessageSlot ()
	// 2. copy the message to the pointer
	// 3. CommitMessage ()
	//
	// alternatively, use QueueMessage () which packages these three steps into one call
	
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
	
	void Push (const T & i_message)
	{
		seq_t sequence;
		auto & slot = * AcquireMessageSlot (sequence);
		slot = i_message;
		CommitMessage (sequence);
	}
	
	
	inline T * AcquireMessageSlot (seq_t & o_sequence)
	{
		o_sequence = m_pathway.insertSequence.Acquire();
		
		seq_t slotIndex = o_sequence & m_pathway.m_sequenceMask;
		T * record = & m_pathway.m_queue [slotIndex];
		
		const seq_t maxSequenceOffset = m_pathway.m_queue.size ();

		u32 tries = 0;
		
		while (true)
		{
			// getting an offset here first, instead of doing a one-to-one comparison, solves the issue of wrap-around
			seq_t offset = o_sequence - m_pathway.claimSequence;
			
			if (offset < maxSequenceOffset)
				break;

			// TODO: condition variable this?
			this_thread::yield ();
			++m_numSleeps;
			
			if (++tries > 100000000)
				d_jdThrow ("Deadlock. Or, more likely, you've blown out the queue. Each sequence (lock) can push up to 512 transactions (calls).");
		}
		
		return record;
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
	
	inline u32 ClaimAvailableMessages () // don't wait; grab all available
	{
		i64 available = m_signalCount.value;	// available always monotonically increases from this perspective as the consumer

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
			else
				available = 0;
		}
		
		return available;
	}
	
	inline T * ViewMessage (seq_t i_index = 0)
	{
		seq_t sequence = m_pathway.claimSequence + i_index;
		sequence &= m_pathway.m_sequenceMask;
		
		return & m_pathway.m_queue [sequence];
	}
	
	inline void ReleaseMessage ()
	{
		m_pathway.claimSequence.Acquire ();
	}
	
	inline void ReleaseMessages (seq_t i_numMessages)
	{
		m_pathway.claimSequence.Acquire (i_numMessages);
	}
	
	
	// TODO: validate that m_consumerLock makes these multiple-consumer safe. Seems reasonable on the surface.
	bool Pop (T & o_message)
	// no wait. claims and releases one message if it's available
	{
		lock_guard <mutex> lock (m_consumerLock);
		
		i64 available = m_signalCount.value;
		
		if (available)
		{
			m_signalCount.value--;
			o_message = * ViewMessage ();
			ReleaseMessage ();

			return true;
		}
		else return false;
	}

	
	inline void PopWait (T & o_message)
	{
		lock_guard <mutex> lock (m_consumerLock);
		
		WaitForMessages (1);
		o_message = * ViewMessage ();
		ReleaseMessage ();
	}

	
	protected:
	
	mutex										m_consumerLock;
	JdCacheLinePadded <atomic <i64>>			m_signalCount;
	JdSemaphore									m_signal			{ 0 };
	i64											m_acquiredPending	= 0;

	u64											m_numSleeps			= 0;

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
		vector <T>		m_elements;
	};
	
//	deque <T>									m_queue;
	
	queue										m_queue;
	
	mutex										m_mutex;
	condition_variable							m_queueNotEmpty;
	condition_variable							m_queueNotFull;
	u32											m_maxNumElements		= numeric_limits <u32>::max ();
};


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
			size_t available = min (required, (size_t) i_count);
			
			memcpy (& m_inPacket.values [m_inPending], i_values, sizeof (T) * available);
			
			i_count -= available;
			i_values += available;
			m_inPending += available;
			
			if (m_inPending == t_packetSize)
			{
				m_queue.QueueMessage (m_inPacket);
				m_inPending = 0;
			}
		}
		
		while (i_count >= t_packetSize)
		{
			auto packet = (Packet *) i_values;
			m_queue.QueueMessage (* packet);
			
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
			size_t required = min ((size_t) i_required, m_outPending);

			auto i = t_packetSize - m_outPending;
			memcpy (o_values, & m_outPacket.values [i], required * sizeof (T));
			
			m_outPending -= required;
			o_values += required;
			i_required -= required;
		}
		
		while (i_required >= t_packetSize)
		{
			auto packet = (Packet *) o_values;
			m_queue.WaitForMessage (* packet);
			
			o_values += t_packetSize;
			i_required -= t_packetSize;
		}
		
		if (i_required)
		{
			m_queue.WaitForMessage (m_outPacket);
			
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




#endif /* JdMessageQueue_hpp */
