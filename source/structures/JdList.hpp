//
//  JdList.hpp
//  Jigidesign
//
//  Created by Steven Massey on 4/20/17.
//  Copyright © 2017 Jigidesign. All rights reserved.
//

#ifndef JdList_h
#define JdList_h




template <typename T>
struct JdListElementT
{
	template <typename> friend class JdListT;
	protected:
	
	T * next			= nullptr;
	T * previous		= nullptr;
};

template <typename T>
struct JdListT
{
	typedef T * element_t;
	
					JdListT			()
	{
		m_start.next = & m_end;
		m_end.previous = & m_start;
	}


	size_t			size			() const
	{
		return m_size;
	}
	
	element_t		begin			()
	{
		return m_start.next;
	}

	element_t		end				()
	{
		return & m_end;
	}
	

	element_t		insert			(element_t i_before, element_t i_element)
	{
		if (i_before != & m_start)
		{
			auto previous = i_before->previous;
			i_before->previous = i_element;
			previous->next = i_element;
			
			i_element->previous = previous;
			i_element->next = i_before;
			
			++m_size;
		}
		
		return i_element;
	}

	void			push_back		(element_t i_element)
	{
		insert (end (), i_element);
	}

	element_t		pop_front		()
	{
		if (m_size)
		{
			element_t i = begin ();
			erase (i);
			return i;
		}
		else return nullptr;
	}
	
	element_t		next			(element_t i_element)
	{
		return i_element->next;
	}

	element_t		previous		(element_t i_element)
	{
		return i_element->previous;
	}

	
	element_t		erase			(element_t i_element)
	{
		d_jdAssert (i_element, "nul element");
		d_jdAssert (i_element != & m_start, "deleting null element");
		d_jdAssert (i_element != & m_end, "deleting end element");
		
		auto next = i_element->next;
		
		if (i_element->previous)
			i_element->previous->next = next;
		if (next)
			next->previous = i_element->previous;
		
		i_element->next = nullptr;
		i_element->previous = nullptr;
		
		--m_size;
		
		return next;
	}

//	element_t		delete			(element_t i_element)
//	{
//		auto next = remove (i_element);
//		delete i_element;
//		return next;
//	}
	
	protected:
	
	T				m_start,
					m_end;
	
	size_t			m_size			= 0;
};



template <typename T>
struct JdList2T
{
	struct ElementT : T
	{
		ElementT (const T & i_element) : T (i_element) {}
		ElementT () : T () {}

		friend class JdList2T;
		
		private:
		ElementT * previous			= nullptr;
		ElementT * next				= nullptr;
		
		#if DEBUG
			JdList2T * linkedTo			= nullptr;
		#endif
	};

	struct Iterator
	{
		typedef const Iterator & IteratorRef;
		friend class JdList2T;
		friend class Iterator;

		bool operator == (IteratorRef i_iterator) const
		{
			return m_at == i_iterator.m_at;
		}

		bool operator != (IteratorRef i_iterator) const
		{
			return m_at != i_iterator.m_at;
		}

		bool operator < (IteratorRef i_iterator) const
		{
			d_jdAssert (m_list == i_iterator.m_list, "iterators are in different lists");

			if (m_at == i_iterator.m_at)
			{
				return false;
			}
			else
			{
				auto i = m_at;

				while (true)
				{
					i = i->next;
					if (i == nullptr)
						return false;
					if (i == i_iterator.m_at)
						return true;
				}
			}
		}

		bool operator > (IteratorRef i_iterator) const
		{
			return (*this != i_iterator and not operator < (i_iterator));
		}
		
		Iterator & operator ++ ()
		{
			d_jdAssert (m_at != & m_list->m_end, "list iterator overrun");
			m_at = m_at->next;
			
			return *this;
		}

		Iterator & operator -- ()
		{
			d_jdAssert (m_at->previous != & m_list->m_start, "list iterator overrun");
			m_at = m_at->previous;

			return *this;
		}
		
		
		Iterator operator + (i32 i_offset) const
		{
			return operator + ((off_t) i_offset);
		}
		
		Iterator operator - (i32 i_offset) const
		{
			return operator + ((off_t) -i_offset);
		}
		
		Iterator operator + (off_t i_offset) const
		{
			Iterator i = *this;
			
			if (i_offset > 0)
			{
				while (i_offset--)
					++i;
			}
			else if (i_offset < 0)
			{
				while (i_offset++)
					--i;
			}
			
			return i;
		}

		Iterator operator - (off_t i_offset) const
		{
			return operator + (-i_offset);
		}
		
		operator bool () const
		{
			return m_at;
		}
		
		void invalidate ()
		{
			m_at = nullptr;
		}

		T & operator * ()
		{
			return *m_at;
		}
		
		ElementT * operator -> ()			{ return m_at; }
		ElementT * operator -> () const		{ return m_at; }

		private://--------------------------------------------------------------------------------
		Iterator	(ElementT * i_start, JdList2T * i_list)
		:
		m_at		(i_start),
		m_list		(i_list)
		{}
		
		ElementT * operator () () { return m_at; }
		
		ElementT * m_at;
		JdList2T * m_list;
	};

	typedef Iterator iterator_t;
	typedef ElementT * element_t;
	
	JdList2T						()
	{
		m_start.next = & m_end;
		m_end.previous = & m_start;
		
		#if DEBUG
			m_start.linkedTo = this;
			m_end.linkedTo = this;
		#endif
	}
	
	~ JdList2T						()
	{
		auto i = begin ();
		while (i != end ())
		{
			auto e = i.m_at;
			++i;
			delete e;
		}
	}
	
	size_t			size			() const
	{
		return m_size;
	}

	iterator_t		begin			()
	{
		return { m_start.next, this };
	}
	
	iterator_t		end				()
	{
		return { & m_end, this };
	}
	
	T &							front			()
	{
		d_jdAssert (m_size, "no elements in list");
		
		return * m_start.next;
	}

	iterator_t					push_back		(const T & i_element)
	{
		auto element = new ElementT (i_element);
		
		_insert (end (), element);
		
		return { element, this };
	}
	
	
	iterator_t /* next */		insert			(iterator_t i_before, const T & i_element)
	{
		auto element = new ElementT (i_element);
		
		_insert (end (), element);
		
		return { next, this };
	}


	iterator_t		unlink_front		()
	{
		if (m_size)
		{
			iterator_t i = begin ();
			unlink (i);
			return i;
		}
		else return nullptr;
	}

	iterator_t		unlink_back			()
	{
		if (m_size)
		{
			iterator_t i = previous (end ());
			unlink (i);
			return i;
		}
		else return nullptr;
	}

	
	iterator_t /* next */		erase			(iterator_t i_iterator)			// deletes the element,
	{
		auto next = i_iterator->next;

		unlink (i_iterator);
		delete i_iterator();
		
		return { next, this };
	}

	void						unlink			(iterator_t i_iterator)			// removes the element from this list
	{
		d_jdAssert (i_iterator, "null element");
		d_jdAssert (i_iterator->linkedTo == this, "list element belongs to another list");
		d_jdAssert (i_iterator() != & m_start, "deleting null element");
		d_jdAssert (i_iterator() != & m_end, "deleting end element");
		
		auto next = i_iterator->next;			d_jdAssert (next, "misconnected list element");
		auto previous = i_iterator->previous;	d_jdAssert (previous, "misconnected list element");
		
		previous->next = next;
		next->previous = previous;

		#if DEBUG
		i_iterator->linkedTo = nullptr;
		#endif
		--m_size;
	}

	
	protected:
	
	void				_insert			(iterator_t i_before, element_t i_element)
	{
		#if DEBUG
			d_jdAssert (i_element->linkedTo == nullptr, "list element belongs to another list");
			i_element->linkedTo = this;
		#endif
		
		if (i_before() != & m_start)
		{
			auto previous = i_before->previous;
			i_before->previous = i_element;
			previous->next = i_element;
			
			i_element->previous = previous;
			i_element->next = i_before ();
			
			++m_size;
		}
	}
	


	ElementT		m_start,
					m_end;
	
	size_t			m_size			= 0;
};


#endif /* JdList_h */
