//
//  JdGraph
//  Jigidesign
//
//  Created by Steven Massey on 4/20/17.
//  Copyright © 2017 Jigidesign. All rights reserved.
//

#ifndef JdGraph_hpp
#define JdGraph_hpp

#include <vector>
#include "JdNucleus.hpp"

template <typename T>
struct GraphT : T
{
	static GraphT *		castToGraphNode 			(T * i_node)
	{
		return static_cast <GraphT *> (i_node);
	}
	
	
	typedef vector <GraphT *>						nodes_t;
	typedef typename nodes_t::iterator				iterator_t;
	typedef typename nodes_t::reverse_iterator		riterator_t;

	GraphT () {}
	
	~GraphT ()
	{
		if (m_up.size () == 0)
			clear ();
	}
	
	void 	clear ()
	{
		auto down = m_down;
		for (auto d : down)
			detach (*d);
		
		d_jdAssert (m_down.size () == 0, "right");
		
		T * _this = this;
		*_this = T ();
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

//	GraphT (const GraphT & i_other) = delete;
	
	GraphT (const GraphT & i_other)
	:
	m_up	(i_other.m_up),
	m_down 	(i_other.m_down),
	T		((T &) i_other) 			{}


	GraphT & operator = (const GraphT & i_other)
	{
		m_up = i_other.m_up;
		m_down = i_other.m_down;
		* (T *) this = i_other;

		return *this;
	}

	
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
	
	void 				visit 				(const function <void (T * i_parent, T & i_node)> & i_visitor)
	{
		VisitWithParent (nullptr, i_visitor);
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
		GraphT * node = & i_node;											d_jdAssert (i_node.m_up.count (this), "doesn't reference node");

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
	
//	protected://--------------------------------------------------------------------------------

	void 				VisitWithParent		(T * i_parent, const function <void (T * i_parent, T & i_node)> & i_visitor)
	{
		i_visitor (i_parent, * this);
		
		for (auto & n : m_down)
			n->VisitWithParent (this, i_visitor);
	}

	
	void				DumpR				(GraphT * i_node, size_t i_depth = 0)
	{
		T & obj = * i_node;
		
		cout << "\n";
		auto d = i_depth;
		while (d--)
			cout << "  ";

		cout << "[" << obj << " " << i_node;
		
		++i_depth;
		
		if (i_node->numChildren ())
		{
			for (auto i : i_node->children ())
				DumpR (i, i_depth);
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


#endif
