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

// TODO: std'ize
#include <boost/interprocess/detail/atomic.hpp>
using namespace boost::interprocess::ipcdetail;

typedef u32 seq_t;

const int c_jdCacheLineBytes = 64;

// I really don't know if this cache sharing prevention acheives anything...
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

// JdMessageQueue2 (and all prior works) *aren't* multiple consumer safe nor easily compatible because of the disconnected atomic signal variable
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
	
	u32 GetQueueSizeInBytes ()
	{
		return m_pathway.GetPathwayNumBytes ();
	}
	
	void QueueMessages (const T * i_messages, u32 i_count)
	{
		u32 sequence;
		while (i_count--)
		{
			auto slot = AcquireMessageSlot (sequence);
			*slot = *i_messages++;
			CommitMessage (sequence);
		}
	}
	
	void QueueMessage (const T & i_message)
	{
		u32 sequence;
		auto & slot = *AcquireMessageSlot (sequence);
		slot = i_message;
		CommitMessage (sequence);
	}
	
	inline T * AcquireMessageSlot (u32 & o_sequence)
	{
		o_sequence = m_pathway.insertSequence.Acquire();
		
		u32 slotIndex = o_sequence & m_pathway.m_sequenceMask;
		T * record = & m_pathway.m_queue [slotIndex];
		
		const u32 maxSequenceOffset = m_pathway.m_queue.size ();

		u32 tries = 0;
		
		while (true)
		{
			// getting an offset here first, instead of doing a one-to-one comparison, solves the issue of wrap-around
			u32 offset = o_sequence - m_pathway.claimSequence.Value ();
			
			if (offset < maxSequenceOffset)
				break;
			
			this_thread::yield ();
			
			if (++tries > 100000000)
				d_jdThrow ("Deadlock. Or, more likely, you've blown out the queue. Each sequence (lock) can push up to 512 transactions (calls).");
		}
		
		return record;
	}
	
	
	inline
	void CommitMessage (u32 i_sequence)
	{
		m_pathway.commitSequence.Update (i_sequence + 1);	// update returns when all previous sequence numbers have been updated
		
		i32 previous = (i32) atomic_inc32 ((u32 *) & m_signalCount.value);
		if (previous < 0)
			m_signal.Signal ();
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
	
	
	inline u32 WaitForMessages (i32 i_maxMessagesToGrab)
	{
		i32 available = atomic_read32 ((u32 *) & m_signalCount.value);	// available always monotonically increases from this perspective as the consumer
		
		available = std::max (available, (i32) 1); // wait for at least one
		available = std::min (available, i_maxMessagesToGrab);
		
		i32 previous = (i32) atomic_add32 ((u32 *) & m_signalCount.value, -available);
		
		if (previous < available)
			m_signal.Wait ();
		
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
			m_signal.Wait ();
		
		return available;
	}

	
	inline u32 TimedWaitForMessages (u32 i_timeoutInMicroseconds, i32 i_maxMessagesToGrab = std::numeric_limits <i32>::max ())
	{
		i32 available;
		
		if (m_acquiredPending == 0) // no previous timeout occurred...
		{
			available = atomic_read32 ((u32 *) & m_signalCount.value);	// available always monotonically increases from this perspective as the consumer
			
			if (available == 0)
			{
				if (i_timeoutInMicroseconds == 0)
					return 0;
				else
					available = 1; // try to grab at least one, probably triggering a wait below
			}
			
			available = std::min (available, i_maxMessagesToGrab);
			
			i32 previous = (i32) atomic_add32 ((u32 *) & m_signalCount.value, -available);
			
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
	
	inline T * ViewMessage (u32 i_index = 0)
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
	
	bool GetMessage (T & o_message)
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

	inline void WaitForMessage (T & o_message)
	{
		WaitForMessages (1);
		o_message = * ViewMessage ();
		ReleaseMessage ();
	}

	
	protected:
	
	JdCacheLinePadded <i32>						m_signalCount;
	JdSemaphore									m_signal			{ 0 };
	u32											m_acquiredPending	= 0;
	
	JdThreadPortPathway <T>						m_pathway;
};


template <typename T>
struct JdMpMcQueueT
{
	void SetMaxNumElements (u32 i_numElements)
	{
		m_maxNumElements = i_numElements;
	}
	
	void Push (T && i_value)
	{
		unique_lock <mutex> lock (m_mutex);
		
		while (m_queue.size () >= m_maxNumElements)
			m_queueNotFull.wait (lock);

		m_queue.push_front (i_value);
	}
	
	T Pop ()
	{
		unique_lock <mutex> lock (m_mutex);
		
		while (m_queue.empty ())
			m_queueNotEmpty.wait (lock);
		
		T back = m_queue.back ();
		m_queue.pop_back ();
		
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

			o_value = m_queue.back ();
			m_queue.pop_back ();
		}
		
		if (notifyProducer)
			m_queueNotFull.notify_one ();

		return true;
	}
	
	deque <T>									m_queue;
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
