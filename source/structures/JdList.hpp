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



template <typename T>
struct JdListT
{
	struct Header
	{
//		Element (const T & i_element) : value (i_element) {}
//		ElementT () : T () {}

		friend class JdListT;
		
		private:
		Header *	previous			= nullptr;
		Header * 	next				= nullptr;
		
		#if DEBUG
			JdListT * linkedTo			= nullptr;
		#endif
	};
	
//	struct Element
//	{
//		Header 	header;
//		T		object;
//	};

	struct Iterator
	{
		typedef const Iterator & IteratorRef;
		friend class JdListT;
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
		
		Header * operator -> ()			{ return m_at; }
		Header * operator -> () const		{ return m_at; }

		private://--------------------------------------------------------------------------------
		Iterator	(Header * i_start, JdListT * i_list)
		:
		m_at		(i_start),
		m_list		(i_list)
		{
			cout << "m_at: " << m_at << endl;
		}
		
		Header * operator () () { return m_at; }
		
		Header *		m_at;
		JdListT * 		m_list;
	};

	typedef Iterator iterator_t;
	typedef Header * header_t;
	
	JdListT							()
	{
		m_start.next = & m_end;
		m_end.previous = & m_start;
		
		#if DEBUG
			m_start.linkedTo = this;
			m_end.linkedTo = this;
		#endif
	}
	
	~ JdListT						()
	{
		auto i = begin ();
		while (i != end ())
		{
			cout << "DELETE: " << this << "\n";
			auto e = i.m_at;
			++i; // inc. first before blown away
//			DeleteElement (e);
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

	
	iterator_t					append			(const T & i_element)
	{
		auto element = NewElement (i_element);
		_insert (end (), element);
		return { element, this };
	}
	
	
	iterator_t /* next */		insert			(iterator_t i_before, const T & i_element)
	{
		auto element = NewElement (i_element);
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
	
	header_t			NewElement				(const T & i_element)
	{
		size_t s = sizeof (Header) + sizeof (T);
		
		auto header = (header_t) new u8 [s];
		new (header) Header;
		new (header+1) T (i_element);
		return header;
	}

	void				DeleteElement			(header_t i_header)
	{
		T * element = (T *) (i_header + 1);
		element->~T();
		
		auto data = (u8 *) i_header;
		delete [] data;
	}

	void				_insert					(iterator_t i_before, header_t i_header)
	{
		#if DEBUG
			d_jdAssert (i_header->linkedTo == nullptr, "list element belongs to another list");
			i_header->linkedTo = this;
		#endif
		
		if (i_before() != & m_start)
		{
			auto previous = i_before->previous;
			i_before->previous = i_header;
			previous->next = i_header;
			
			i_header->previous = previous;
			i_header->next = i_before ();
			
			++m_size;
		}
	}
	
	Header 			m_start,
					m_end;
	
	size_t			m_size			= 0;
};




template <typename T>
struct GraphT : T
{
	typedef vector <GraphT *>						nodes_t;
	typedef typename nodes_t::iterator				iterator_t;
	typedef typename nodes_t::reverse_iterator		riterator_t;

	GraphT () {}
	
	~GraphT ()
	{
		if (m_up.size () == 0)
		{
			auto down = m_down;
			for (auto d : down)
				detach (*d);
		}
	}

	GraphT (const T & i_element)
	{
		T * base = this;
		*base = i_element;
	}

	GraphT (GraphT && i_other)
	:
	m_up	(move (i_other.m_up)),
	m_down 	(move (i_other.m_down)),
	T		(move ((T &&)i_other)) 			{}

	
	GraphT (GraphT &) = delete;
	
	size_t				numChildren			() const
	{
		return m_down.size ();
	}

	size_t				numNodes			() const
	{
		size_t s = m_down.size ();
		for (auto n : m_down)
			s += n->numNodes ();
		
		return s;
	}

	
	GraphT &			operator []			(size_t i_index)
	{
		d_jdAssert (i_index < m_down.size (), "out of bounds");
		return * m_down [i_index];
	}
	
	vector <GraphT *> &	children			() { return m_down; }
	
	
	iterator_t			begin				() { return m_down.begin (); }
	iterator_t			end					() { return m_down.end (); }

	riterator_t			rbegin				() { return m_down.rbegin (); }
	riterator_t			rend				() { return m_down.rend (); }


	bool 				find 				(const function <bool (T & i_node)> & i_visitor)
	{
		auto done = i_visitor (* this);
		
		if (not done)
		{
			for (auto & n : m_down)
			{
				done = n->find (i_visitor);
				if (done) break;
			}
		}
		
		return done;
	}
	

	void 				visit 				(const function <void (T & i_node)> & i_visitor)
	{
		i_visitor (* this);
		
		for (auto & n : m_down)
			n->visit (i_visitor);
	}

	void 				visitBottomUp		(const function <void (T & i_node)> & i_visitor)
	{
		for (auto & n : m_down)
			n->visitBottomUp (i_visitor);

		i_visitor (* this);
	}

	void				prepend				(GraphT & i_node)
	{
		d_jdAssert (i_node.m_up.count (this) == 0, "node already linked");
		i_node.AttachTo (this);
		m_down.push_front (& i_node);
	}
	
	
	GraphT &			prepend				(const T & i_element)
	{
		GraphT * node = new GraphT (i_element);
		node->m_up.insert (this);
		m_down.push_front (node);
		return * node;
	}

	void				append				(GraphT & i_node)
	{
		d_jdAssert (i_node.m_up.count (this) == 0, "node already linked");
		i_node.AttachTo (this);
		m_down.push_back (& i_node);
	}
	
	
	GraphT &			append				(const T & i_element)
	{
		GraphT * node = new GraphT (i_element);
		node->m_up.insert (this);
		m_down.push_back (node);
		return * node;
	}

	void				clone				(GraphT & i_other)
	{
		T * e = this;
		* e = i_other;
		CloneR (*this, i_other.children ());
	}

	
	GraphT				clone				()
	{
		GraphT base;
		T * element = & base;
		*element = *this;
		
		CloneR (base, m_down);
		
		return base;
	}
	
	void				CloneR				(GraphT & i_to, nodes_t & i_from)
	{
		for (auto i : i_from)
		{
			T * element = i;
			auto & copy = i_to.append (* element);
			CloneR (copy, i->children ());
		}
	}

	// deletes the node if its ref count = 0
	void				detach				(GraphT & i_node)
	{
		GraphT * node = & i_node;										d_jdAssert (i_node.m_up.count (this), "doesn't reference node");

		i_node.m_up.erase (this);
		auto i = std::find (m_down.begin (), m_down.end (), node);			d_jdAssert (i != m_down.end (), "node not found");
		m_down.erase (i);

		i_node.Release ();
	}
	
	void				dump				()
	{
		DumpR (this);
		cout << endl;
	}
	
	protected://--------------------------------------------------------------------------------

	void				DumpR				(GraphT * i_node)
	{
		T & obj = * i_node;
		cout << "[" << obj << " " << i_node;
		
		if (i_node->numChildren ())
		{
			cout << " ";
			for (auto i : i_node->children ())
				DumpR (i);
		}
		
		cout << "]";
	}

	
	void				AttachTo			(GraphT * i_parent)
	{
		m_up.insert (i_parent);
		Release ();
	}
	
	void				Release				()
	{
		m_up.erase (this);
		if (m_up.size () == 0)
			delete this;
	}
	
	friend class GraphT;
	
	set <GraphT *>					m_up;
	vector <GraphT *>				m_down;
};





#endif /* JdList_h */
