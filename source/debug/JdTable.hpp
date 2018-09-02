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
	
		// or
		JdTable table ({ "column1,10", ".:column2,20", ":column3,5", "L:column4,9"});
	#endif
	
	struct Column
	{
		size_t				width		= 2;
		char				fill		= ' ';
		bool				alignLeft	= false;
		
		vector <string>		rows;
	};
	
	~ JdTable ()
	{
		Print ();
	}
	
	
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
			
			auto comma = name.find (",");
			if (comma != string::npos)
			{
				string number = name.substr (comma + 1);
				
				column.width = stoll (number);
				name = name.substr (0, comma);
			}

			column.rows.push_back (name);
		}
	}
	
	template <typename T, typename... Args>
	void PrintRow (T && i_column, Args... i_columns)
	{
		size_t padding = 4;
		size_t width = CalculateWidth (padding);
		string div = GetDivider (width);

		if (not m_printed)
		{
			cout << div;
			OutputRow (0, padding);
			cout << div;
			m_printed = true;
		}

		for (auto & c : m_columns)
			c.rows.clear ();

		AddRow (i_column, i_columns...);
		OutputRow (0, padding);
	}
	
	void End ()
	{
		size_t padding = 4;
		size_t width = CalculateWidth (padding);
		cout << GetDivider (width);
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
	
	protected:
	
	string GetDivider (size_t i_width)
	{
		string div (i_width + 4, '-');
		div = "+" + div + "+";
		div += "\n";
		
		return div;
	}
	
	size_t CalculateWidth (size_t i_padding)
	{
		size_t padding = i_padding;
		size_t totalWidth = -padding;
		for (auto & c : m_columns)
		{
			size_t & w = c.width;
			
			for (auto & r : c.rows)
				w = max (r.size (), w);
			
			totalWidth += (c.width + padding);
		}
		
		return totalWidth;
	}
	
	
	void Print ()
	{
		if (not m_printed)
		{
			size_t padding = 4;
			size_t totalWidth = CalculateWidth (padding);
			string div = GetDivider (totalWidth);
			
			cout << div;
			for (size_t r = 0; r < m_numRows + 1; ++r)
			{
				OutputRow (r, padding);
				if (r == 0) cout << div;
			}
			
			cout << div << endl;
			
			m_printed = true;
		}
	}
	
	
	void OutputRow (size_t i_row, size_t i_padding)
	{
		auto r = i_row;
		
		string pad (i_padding, ' ');
		string headerPad = pad;

		cout << "|  ";
		for (auto & c : m_columns)
		{
			d_jdAssert (r < c.rows.size (), "row out of bounds");
			
			if (r != 0) cout << setfill (c.fill);
			cout << setw (c.width) << (c.alignLeft or r == 0 ? left : right) << c.rows [r];
			cout << setfill (' ');
			if (& c != & m_columns.back ())
				cout << (r == 0 ? headerPad : pad);
		}
		cout << "  |\n";
	}
	
	
	vector <Column>			 	m_columns;
	bool						m_printed			= false;
	size_t 						m_numRows			= 0;
	
};


#endif /* JdTable_h */
