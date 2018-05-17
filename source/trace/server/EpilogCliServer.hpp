//
//  EpilogCliServer.h
//  EpilogCliServer
//
//  Copyright (c) 2012 Epigram. All rights reserved.
//

#ifndef __EpilogCliServer__
#define __EpilogCliServer__

#include "JdCore.hpp"
#include <map>
#include <unordered_map>
#include "EpilogStructs.hpp"
#include "JdThreadPort.hpp"

typedef JdMessageQueue <EpilogMsg> EpilogMsgQueue;



const int c_epilogMaxTcpLength = 32768;


enum EEpilogColorIndex
{
	e_epilogColor_black = 0,
	e_epilogColor_red,
	e_epilogColor_green,
	e_epilogColor_blue,
	e_epilogColor_cyan,
	e_epilogColor_purple,
	e_epilogColor_yellow
};

struct EpilogCategory
{
	string				name;
	bool				filter;
	uint8_t				level;
	EEpilogColorIndex	color;
};


class EpilogCliServer
{
	public:
	
	EpilogCliServer				(EpilogMsgQueue *i_queue,
								 bool i_writeLogFile,
								 vector <EpilogCategory> &i_categories
//								 map <string, EpilogCategory *> &i_map
								)
	:
	m_queue						(i_queue),
	m_writeLogFile				(i_writeLogFile),
	m_file						(0)
	{
		for (auto c : i_categories)
		{
			EpilogCategory * category = new EpilogCategory (c);
			
			m_categoryMap [category->name] = category;
//			*category = c;
		}
		
//		m_categories = i_categories;
//		m_categoryMap = i_map;
	}
	
	void									Run 					();
	
	protected:
	void									SetColor				(EEpilogColorIndex i_color);
	void									ResetColor				();
	
	
	EpilogMsgQueue *						m_queue;
			
	bool									m_writeLogFile;
	
	FILE *									m_file;
	
	vector <string>							m_filter;
	
	map <string, EpilogCategory *>			m_categoryMap;			// category name to category obj
	
	unordered_map <u64, EpilogCategory *>	m_classIdToCategory;
    
    map <u64, u32>							m_threads;
	u32										m_threadIndex = 1;
};



#endif /* defined(__EpilogCliServer__) */
