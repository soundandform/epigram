//
//  JdResourceCache.hpp
//  Jigidesign
//
//  Created by Steven Massey on 4/11/17.
//  Copyright © 2017 Jigidesign. All rights reserved.
//

#ifndef JdResourceCache_hpp
#define JdResourceCache_hpp

#include "JdList.hpp"
#include <list>

template <typename T>
struct JdCacheHandlerT
{
	void FlushCacheItem (T * & i_resource)
	{
		delete i_resource;
		i_resource = nullptr;
	}
	
//	void ReviveCacheItem (T * i_resource
};


template <typename T, typename t_handler = JdCacheHandlerT <T>>
class JdCacheT : protected JdCacheHandlerT <T>
{
	protected:
	
	struct ResourceRecord : JdListElementT <ResourceRecord>
	{
		ResourceRecord (T * i_ptr, size_t i_resourceSize)
		:
		resource		(i_ptr),
		resourceSize	(i_resourceSize)
		{ }
		
		ResourceRecord () {}
		
		JdCacheT *			cacheOwner			= nullptr;
		T *					resource			= nullptr;
		size_t				resourceSize		= 0;
		i32					acquireCount		= 0;
	};
	
	public:

	typedef T *					pointer_t;
	typedef pointer_t *			handle_t;
	typedef voidptr_t			ref_t;
	
	
								JdCacheT					(t_handler * i_cacheHandler = nullptr)
								:
								m_handler					(i_cacheHandler)
	{
		if (m_handler == nullptr)
			m_handler = this;
	}
	
	void						SetHandler					(t_handler * i_handler)
	{
		m_handler = i_handler;
	}

								~JdCacheT					()
	{
		FlushResources (0);
		
		auto i = m_freeRecords.begin ();

		while (i != m_freeRecords.end ())
		{
			auto hr = i;
			i = m_freeRecords.erase (i);
			delete hr;
		}
	}
	
	pointer_t					TryToAcquire				(ref_t i_cacheId)
	{
		auto hr = (ResourceRecord *) i_cacheId;

		if (hr->acquireCount >= 0 and hr->cacheOwner == this)
		{
			if (hr->acquireCount == 0)
				m_queue.erase (hr);
		
			++hr->acquireCount;

			return hr->resource;
		}
		else
		{
		}
		
		return nullptr;
	}

	pointer_t					Dereference					(ref_t i_cacheId)
	{
		auto hr = (ResourceRecord *) i_cacheId;
		return hr->resource;
	}

	bool						Has							(ref_t i_cacheId)
	{
		auto hr = (ResourceRecord *) i_cacheId;
		return (hr->cacheOwner == this);
	}
	
	void						Resize						(ref_t i_cacheId, size_t i_newSize)
	{
		auto hr = (ResourceRecord *) i_cacheId;
		m_totalResourceSize -= hr->resourceSize;
		m_totalResourceSize += i_newSize;
		hr->resourceSize = i_newSize;
	}
	
	void						Release						(ref_t i_cacheId)
	{
		auto hr = (ResourceRecord *) i_cacheId;
		
		if (hr->resource)
			--hr->acquireCount;
		
		d_jdAssert (hr->acquireCount >= 0, "acquire/release mismatch");

		if (hr->acquireCount == 0)
			m_queue.push_back (hr);   // move to MRU position
	}


	void						Insert						(ref_t i_cacheId, size_t i_resourceSize)
	{
		auto hr = (ResourceRecord *) i_cacheId;
		
		d_jdAssert (hr->acquireCount == -1, "cache record out of whack");

		hr->acquireCount = 0;
		hr->cacheOwner = this;
		hr->resourceSize = i_resourceSize;
		m_totalResourceSize += i_resourceSize;
		
		m_queue.insert (m_queue.end (), hr);
	}
	
	ref_t						Insert						(const pointer_t i_pointer, size_t i_resourceSize)
	{
		ResourceRecord * hr;
		
		m_totalResourceSize += i_resourceSize;
		
		if (m_freeRecords.size ())
		{
			hr = m_freeRecords.pop_front ();
		}
		else hr = new ResourceRecord ();
		
		m_queue.insert (m_queue.end (), hr);

		hr->resource = i_pointer;
		hr->resourceSize = i_resourceSize;
		hr->acquireCount = 0;
		hr->cacheOwner = this;
		
		return hr;
	}


	pointer_t					Remove						(ref_t i_cacheId)
	{
		auto hr = (ResourceRecord *) i_cacheId;
		
		if (hr->cacheOwner == this and hr->acquireCount == 0)
		{
			Flush (hr);
			
			return hr->resource;
//			return true;
		}
		else return nullptr;
	}

	
	void						Erase						(ref_t i_cacheId)
	{
		auto hr = (ResourceRecord *) i_cacheId;

		if (hr->cacheOwner == this)
		{
			Flush (hr);
			m_freeRecords.push_back (hr);
		}
	}
	
	size_t						GetCacheSize				()
	{
		return m_totalResourceSize;
	}
	
	size_t						FlushResources				(size_t i_resourceSizeLimit)
	{
		if (m_totalResourceSize > i_resourceSizeLimit)
		{
			auto i = m_queue.begin ();
			
			while (i != m_queue.end ())
			{
				if (i->acquireCount == 0 and i->resourceSize > 0)
				{
					i = Flush (i);
					
					// FIX: insert into freeRecords?

					if (m_totalResourceSize < i_resourceSizeLimit)
						break;
				}
				else i = m_queue.next (i);
			}
		}
		
		return m_totalResourceSize;
	}
	

	ref_t						FlushResource				(size_t i_resourceSizeLimit)
	{
		ref_t ref = nullptr;
		
		if (m_totalResourceSize > i_resourceSizeLimit)
		{
			auto i = m_queue.begin ();
			
			while (i != m_queue.end ())
			{
				if (i->acquireCount == 0 and i->resourceSize > 0)
				{
					ref = i;
					Flush (i);
					
					break;
				}
				else i = m_queue.next (i);
			}
		}
		
		return ref;
	}

	protected:
	
	ResourceRecord * /* next */		Flush					(ref_t i_cacheId)
	{
		auto hr = (ResourceRecord *) i_cacheId;
		
		if (hr->acquireCount == 0)
		{
			--hr->acquireCount;	// -1 = flushed
			m_totalResourceSize -= hr->resourceSize;
		
			if (m_handler)
				m_handler->FlushCacheItem (hr->resource);
			
			auto next = m_queue.erase (hr);
			
			return next;
		}
		else return nullptr;
	}

	protected:
	t_handler *										m_handler				= nullptr;
	
	JdList1T <ResourceRecord>						m_queue;
	JdList1T <ResourceRecord>						m_freeRecords;
	
	size_t											m_totalResourceSize		= 0;
};


class JdMemoryPool
{
public:
					~ JdMemoryPool			()
	{
//		cout << "freeing: " << m_memory.size () << endl;
		for (auto i : m_memory)
		{
			u8 * memory = (u8 *) i;
			delete [] memory;
		}
	}
	
	size_t			GetPoolSize				()
	{
		return m_totalAllocatedBytes;
	}

	size_t			GetFreePoolSize			()
	{
		return m_totalFreeBytes;
	}

	
	size_t			SizeOf					(voidptr_t i_memory)
	{
		size_t * sizePtr = (size_t *) i_memory;
		--sizePtr;
		return * sizePtr;
	}
	
	template <typename T>
	T *				Allocate				(size_t i_numBytes)
	{
		void * memory = FindAllocation (i_numBytes);
		
		if (not memory)
		{
			auto numBytes = i_numBytes + sizeof (size_t); // size header
			
			// 4k aligned size
			const size_t c_alignment = 4096-1;
			
			numBytes = (numBytes + c_alignment) & ~c_alignment;
			
			auto bytes = new u8 [numBytes];
//			cout << "alloc: " << i_numBytes << " -> " << numBytes << endl;
			
			m_totalAllocatedBytes += numBytes;
			
			// set hidden size info
			size_t * size = (size_t *) bytes;
			* size = numBytes - sizeof (size_t);
			memory = bytes + sizeof (size_t);
		}
		
		return (T *) memory;
	}
	
	void			Free				(voidptr_t i_memory)
	{
		u8 * allocation = (u8 *) i_memory - sizeof (size_t);
		size_t size = GetSizeOfAllocation (allocation);

		m_totalFreeBytes += size;

		if (m_memory.size () == 0)
		{
			m_memory.push_front (allocation);
		}
		else
		{
			// insert, low-high sorted
			for (auto i = m_memory.begin (); i != m_memory.end (); ++i)
			{
				size_t s = GetSizeOfAllocation (*i);
				
				if (size <= s)
				{
					m_memory.insert (i, allocation);
					return;
				}
			}
			
			m_memory.push_back (allocation);
		}
	}
	
	
	protected:
	
	void *			FindAllocation			(size_t i_numBytes)
	{
		void * memory = nullptr;

		size_t upperLimit = (i_numBytes * 36) >> 5;
		
//		cout << "psearch " << JdByteSize (i_numBytes) << ": ";
		
		for (auto i = m_memory.begin (), e = m_memory.end (); i != e; ++i)
		{
			u8 * allocation = *i;
		
			size_t size = GetSizeOfAllocation (allocation);

//			cout << " " << JdByteSize (size);
			

			if (size > upperLimit)
				break;
			
			if (size >= i_numBytes)
			{
				m_memory.erase (i);
				memory = allocation + sizeof (size_t);
				
				m_totalFreeBytes -= size;
				
				break;
			}
		}
		
//		cout << endl;
		
		return memory;
	}
	
	size_t			GetSizeOfAllocation		(void * i_memory)
	{
		size_t * sizePtr = (size_t *) i_memory;
		return * sizePtr;
	}
	
	
	list <u8 *>		m_memory;
	size_t			m_totalAllocatedBytes		= 0;
	size_t			m_totalFreeBytes			= 0;
	size_t			m_totalUsedBytes			= 0;
};



#endif /* JdResourceCache_hpp */
