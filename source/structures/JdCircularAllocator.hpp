//
//  JdCircularAllocator.hpp
//  Tape
//
//  Created by Steven Massey on 6/10/20.
//  Copyright © 2020 Massey Plugins Inc. All rights reserved.
//

#ifndef JdCircularAllocator_hpp
#define JdCircularAllocator_hpp

#include "JdCore.cpp"
#include <condition_variable>
#include <mutex>

/*
 	This is a thread-safe fixed-size circular buffer allocator.  It's used in the JdModule system for message-passing and assumes that
 	all allocations are short-lived and allocated and deallocated in a roughly first-out, first-back-in manner.
 */

class JdCircularAllocator
{
	public:					JdCircularAllocator					(u32 i_maxAllocationSize, u32 i_bufferSize = 0)
							:
							m_maxAllocationSize					(i_maxAllocationSize)
	{
		if (i_bufferSize)
			Resize (i_bufferSize);														d_jdAssert (i_maxAllocationSize, "must be >=1");
	}
	
	
	void					Resize								(u32 i_numBytes, u32 i_maxAllocationSize = 0 /* 0= don't modify */)
	{
		std::unique_lock <std::mutex> lock (m_mutex);
		
		while (m_claimedIndex != m_releaseIndex)				// can only resize if nothing allocated
			m_condition.wait (lock);
		
		if (i_maxAllocationSize)
			m_maxAllocationSize = i_maxAllocationSize;

		m_bufferSize = Jd::RoundUpToAPowerOf2 (i_numBytes);
		m_bytes.resize (m_bufferSize + m_maxAllocationSize);	// pad the buffer so that m_maxAllocationSize bytes can always be safelyused when m_claimedIndex is near the end of the buffer
		m_data = m_bytes.data ();								// in worst case situation, size word is last word within the nominal buffer size and the overflow is equal to the size of the alloc.
		
		m_indexMask = m_bufferSize - 1;
	}
	
	template <typename T>
	T *							Allocate						(u32 i_numBytes = sizeof (T))
	{
		T * allocated = nullptr;
		
		u32 numBytes = Jd::Align64 (i_numBytes);						d_jdAssert (numBytes <= m_maxAllocationSize, "allocation too large");

		if (numBytes <= m_maxAllocationSize)
		{
			numBytes += sizeof (u64);			// size word
			
			std::unique_lock <std::mutex> lock (m_mutex);
			
			while (GetNumAvailableBytesUnlocked () < numBytes)
			{
				m_hasOverflowed = true;
				m_condition.wait (lock);
			}

			u32 index = m_claimedIndex & m_indexMask;
			m_claimedIndex += numBytes;
			
			u64 * data = (u64 *) & m_data [index];
			* data = numBytes | 1;	// 1 = allocated signal bit
			
			allocated = (T *) ++data;
		}
		
		return allocated;
	}
	
	void					Release								(void * i_data)
	{
		u32 numBytesToRelease = 0;

		m_mutex.lock ();

		u64 * word = (u64 *) i_data;
		(* --word) &= ~1;	// erase allocation bit flag
		
		u32 index = (u8 *) word - m_data;

		if (index == m_releaseIndex)
		{
			// if the release index is pointing to this chunk, then free it by advancing the release index.
			// also, free any further contiguous chunks that have been deallocated prior.
			do
			{
				u64 numBytes = * word;
				numBytesToRelease += numBytes;
				m_releaseIndex += numBytes;
				
				if (m_releaseIndex == m_claimedIndex)
					break;
				
				word = (u64 *) & m_data [m_releaseIndex & m_indexMask];
			}
			while (not (* word & 1)); // check allocated bit flag
		}

		m_mutex.unlock ();

		if (numBytesToRelease)
			m_condition.notify_all ();
	}
	
	u32						GetNumAvailableBytes				()
	{
		std::unique_lock <std::mutex> lock (m_mutex);
		
		return GetNumAvailableBytesUnlocked ();
	}
	
	bool					HasOverflowed						()
	{
		std::unique_lock <std::mutex> lock (m_mutex);
		
		bool hasOverflowed = m_hasOverflowed;
		m_hasOverflowed = false;
		
		return hasOverflowed;
	}
	
	protected:

	u32						GetNumAvailableBytesUnlocked		()
	{
		return m_bufferSize - (m_claimedIndex - m_releaseIndex);
	}

	mutex					m_mutex;
	condition_variable		m_condition;

	vector <u8>				m_bytes;
	u8 *					m_data								= nullptr;

	u32						m_maxAllocationSize					= 0;
	u32						m_bufferSize						= 0;

	u32						m_claimedIndex						= 0;
	u32						m_releaseIndex						= 0;
	
	u32						m_indexMask							= 0;
	
	bool					m_hasOverflowed						= false;
};


#endif /* JdCircularAllocator_hpp */
