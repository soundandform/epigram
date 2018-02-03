//
//  JdTable.hpp
//  Jigidesign
//
//  Created by Steven Massey on 2/2/18.
//  Copyright © 2018 Jigidesign. All rights reserved.
//

#ifndef JdTable_h
#define JdTable_h

#include "JdStdStringUtils.hpp"

struct JdTable
{
	#if 0
		// usage:
		JdTable table ({ "column1", ".:column2", ":column3", "L:column4"}); 	// fill column2 whitespace with '.' and  align column4 to the left
		table.AddRow (w, x, y, z); 												// Jd::ToString () is applied on each element
		table.Print (); 														// optional.  ~JdTable() prints by default.
	#endif
	
	struct Column
	{
		size_t				minWidth	= 2;
		char				fill		= ' ';
		bool				alignLeft	= false;
		
		size_t				width		= 0;
		
		vector <string>		rows;
	};
	
	JdTable (const initializer_list <string> & i_columnNames)
	{
		for (auto & n : i_columnNames)
		{
			m_columns.push_back (Column ());
			auto & column = m_columns.back ();
			
			string name = n;
			auto colon = name.find (":");
			if (colon != string::npos)
			{
				for (size_t i = 0; i < colon; ++i)
				{
					if (tolower (name [i]) == 'l')
						column.alignLeft = true;
					else
						column.fill = name [i];
				}
				
				name = name.substr (colon + 1);
			}
			
			column.rows.push_back (name);
		}
	}
	
	template <typename T, typename... Args>
	void AddRow (T && i_column, Args... i_columns)
	{
		string row [] = { Jd::ToString <T> (i_column), Jd::ToString <Args> (i_columns)... };
		
		d_jdAssert (Jd::SizeOfArray (row) == m_columns.size (), "mismatched column count");
		
		size_t i = 0;
		for (auto & element : row)
		{
			d_jdAssert (i < m_columns.size (), "column overflow");
			auto & column = m_columns [i];
			column.rows.push_back (element);
			
			++i;
		}
		
		++m_numRows;
	}
	
	void Print ()
	{
		size_t padding = 4;
		size_t totalWidth = -padding;
		for (auto & c : m_columns)
		{
			size_t w = c.minWidth;
			
			for (auto & r : c.rows)
				w = max (r.size (), w);
			
			c.width = w;
			
			totalWidth += (c.width + padding);
		}
		
		string div (totalWidth + 2, '-');
		string end = "~" + div + "~";
		div += "--";
		end += "\n"; div += "\n";
		
		string pad (padding, ' ');
		
		for (size_t r = 0; r < m_numRows + 1; ++r)
		{
			if (r == 0) cout << div;
			
			cout << "  ";
			for (auto & c : m_columns)
			{
				d_jdAssert (r < c.rows.size (), "row out of bounds");
				
				if (r != 0) cout << setfill (c.fill);
				cout << setw (c.width) << (c.alignLeft or r == 0 ? left : right) << c.rows [r];
				cout << setfill (' ') << pad;
			}
			cout << endl;
			
			if (r == 0) cout << div;
		}
		
		cout << end << endl;
		
		m_printed = true;
	}
	
	
	~ JdTable ()
	{
		if (not m_printed) Print ();
	}
	
	vector <Column>			 	m_columns;
	bool						m_printed			= false;
	size_t 						m_numRows			= 0;
	
};


#endif /* JdTable_h */
