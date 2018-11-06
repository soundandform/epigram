//
//  JdList.hpp
//  Jigidesign
//
//  Created by Steven Massey on 4/20/17.
//  Copyright © 2017 Jigidesign. All rights reserved.
//

#ifndef JdList_h
#define JdList_h

#include "JdNucleus.hpp"

#if 1
template <typename T>
struct JdListElementT
{
	template <typename> friend class JdList1T;
	protected:

	T * next			= nullptr;
	T * previous		= nullptr;
};


template <typename T>
struct JdList1T
{
	typedef T * element_t;

					JdList1T			()
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

#endif



template <typename T>
struct JdListT
{
	struct Header
	{
		u64			_refCount		= 0;
		Header * 	_next			= nullptr;
		Header *	_previous		= nullptr;
	};
	
	struct Item : Header, T
	{
		Item		()
		:
		T () 		{}
		
		Item		(const T & i_item)
		:
		T			(i_item)
		{ }
	};
	
	template <typename IT>
	struct IterT
	{
		IterT							(const IterT & i_other)
		:
		m_item (i_other.m_item)			{ }

		
		IterT							()
		:
		m_item (nullptr)				{ }
		
		IterT							(IT * i_item)
		:
		m_item (i_item)					{ }
		
		
		T &			operator	*		() const
		{
			return * const_cast <Item *> (static_cast <const Item *> (m_item));
		}

		T * 		operator 	->		() const
		{
			return static_cast <Item *> (m_item);
		}
		
		bool		isValid				() const
		{
			return m_item;
		}
		
		void		invalidate			()
		{
			m_item = nullptr;
		}
		
		bool		operator ==			(const IterT & i_iter) const
		{
			return m_item == i_iter.m_item;
		}
		
		bool		operator !=			(const IterT & i_iter) const
		{
			return m_item != i_iter.m_item;
		}
		
		bool		operator <			(const IterT & i_iter) const
		{
			auto i = i_iter.m_item;

			if (m_item and i)
			{
				while (i)
				{
					if (i == m_item)
						return false;

					i = i->_next;
				}
				
				i = m_item->_next;
				while (i)
				{
					if (i == i_iter.m_item)
						return true;

					i = i->_next;
				}
				
				return false;
			}
			
			return false;
		}
		
		IterT & 	operator 	++		()				{ m_item = m_item->_next; return * this; }
		IterT & 	operator 	--		()				{ m_item = m_item->_previous; return *this; }
		
		IterT 	 	operator 	+		(size_t i_offset) const
		{
			IterT i (* this);
			while (i_offset--)
				++i;
			return i;
		}

		
		IterT 	 	operator 	-		(size_t i_offset) const
		{
			IterT i (* this);
			while (i_offset--)
				--i;
			return i;
		}

		
		IterT 		insert	 			(const T & i_item) const
		{
			auto i = new Item (i_item);
//			cout << "insert: " << i << " " << Jd::ParseClassName <T> () <<  endl;
			
			auto previous = m_item->_previous;
			i->_previous = previous;
			i->_next = m_item;
			
			m_item->_previous = i;
			previous->_next = i;
			
			return i;
		}
		
		void		erase				()
		{
			//			d_jdAssert (m_item->_refCount != -1, "attempting to erase begin/end");
			
			auto next = m_item->_next;
			
			m_item->_previous->_next = next;
			next->_previous = m_item->_previous;
			
//			cout << "delete: " << m_item << endl;
			delete m_item;
			
			m_item = next;
		}
		
		mutable IT *		m_item		= nullptr;
	};
	
	typedef IterT <Header> 					iterator;
	typedef IterT <const Header> 			constIterator;
	
	template <typename I>
	static iterator 	getIterator		(I * i_item)
	{
		return static_cast <Item *> (i_item);
	}
	
	iterator 			begin			()			{ return m_begin._next; }
	iterator			end				()			{ return & m_end; }

	constIterator 		begin			() const	{ return m_begin._next; }
	constIterator		end				() const	{ return & m_end; }

	constIterator 		cbegin			() const	{ return m_begin._next; }
	constIterator		cend			() const	{ return & m_end; }

	
	iterator 			pushBack 		(const T & i_item)
	{
		return end ().insert (i_item);
	}
	
	iterator 			pushFront 		(const T & i_item)
	{
		return begin ().insert (i_item);
	}
	
	void			clear			()
	{
		auto i = begin ();
		while (i != end ())
			i.erase ();
	}
	
	bool			isEmpty			() const
	{
		return m_begin._next == & m_end;
	}
	
	size_t			size			() const
	{
		size_t size = 0;
		auto i = cbegin ();
		while (i != cend ())
		{
			++i;
			++size;
		}

		return size;
	}
	
	JdListT		()
	{
		m_begin._next 	= & m_end;
		m_end._previous = & m_begin;
		
		m_begin._refCount = m_end._refCount = -1;
	}
	
	~JdListT		()
	{
		clear ();
	}
	
	Header		m_begin;
	Header 		m_end;
};




#endif /* JdList_h */
