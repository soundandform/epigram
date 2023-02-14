//
//  EpilogCliServer.cpp
//  EpilogCliServer
//
//  Copyright (c) 2012 Epigram. All rights reserved.
//

#include "EpilogCliServer.hpp"
#include "EpilogStructs.hpp"

#include "Epigram.hpp"

#include <iomanip>
#include <unordered_map>

#include "Epilog.hpp"

using namespace std;

void EpilogCliServer::SetColor (EEpilogColorIndex i_color)
{
	
	if		(i_color == e_epilogColor_blue) cout << "\e[34m";
	else if	(i_color == e_epilogColor_green) cout << "\e[32m";
	else if	(i_color == e_epilogColor_red) cout << "\e[31m";
	else if	(i_color == e_epilogColor_cyan) cout << "\e[36m";
	else if	(i_color == e_epilogColor_purple) cout << "\e[35m";
	else if	(i_color == e_epilogColor_yellow) cout << "\e[33m";
}

void EpilogCliServer::ResetColor ()
{
	cout << "\e[0m";
}


void EpilogCliServer::Run ()
{
	unordered_map <u64, string> classes;
	
	int msgCount = 0;
	
	while (true)
	{
		u32 numMessages = m_queue->TimedWaitForMessages (1000000.);

		while (numMessages--)
		{
			EpilogQueuedMsgHeader * packet = m_queue->ViewMessage ();
			auto data = (u8 *) packet;
			
			++msgCount;
			
			if (packet->msgSize == 0)
			{
				cout << "\n\n\n\n\n\n\n ** new session **\n\n";
				m_threads.clear();
				m_threadIndex = 1;
			}
			
			if (packet->msgType == e_epigramMsgType_log)
			{
				char txt [4096];
				int stringLength = packet->msgSize;
				strncpy (txt, (char *) data + sizeof (EpilogQueuedMsgHeader), stringLength);
				txt [stringLength] = 0;
				
				double milliseconds = double (packet->timestamp) * 0.001;
				
//                    string obj = classes [packet->locationId];
				
//                    int c = m_map [obj];
				EpilogCategory * categoryPtr = m_classIdToCategory [packet->locationId];
				
				EpilogCategory tmpCat;
				
				if (! categoryPtr)
				{
					categoryPtr = &tmpCat;
					categoryPtr->name = classes [packet->locationId];
					categoryPtr->filter = false;
					categoryPtr->level = 0; // no category specific filtering for an unknown category.
					categoryPtr->color = e_epilogColor_black;
				}
				
				EpilogCategory & category = * categoryPtr;
				
				u8 level = packet->level & 0x0F;
				u8 special = packet->level & 0xF0;
				
				bool isWarningOrFatal = level >= c_epilogClassification_warning;
				
				if (isWarningOrFatal || (! category.filter && (level >= category.level || special))) // show special cases always for now...
				{
					cout << setw (8) << right << fixed << setprecision(3) << milliseconds << " │ ";

					SetColor (category.color);
					
					std::string categoryName = category.name;
					
					u32 threadIndex = m_threads [packet->threadId];
					
					if (threadIndex == 0)
					{
						threadIndex = m_threads [packet->threadId] = m_threadIndex++;
					}

					u32 categoryWidth = 20;
					if (threadIndex-1 == 0) categoryWidth += 2;
					
					if (categoryName.length() > categoryWidth)
					{
						categoryName = categoryName.substr (0,categoryWidth-1) + "~";
					}
					
					cout << setw (categoryWidth) << left << categoryName;
					
					if (threadIndex-1)
						cout << " " << std::hex << std::setw(1) << threadIndex-1 << std::dec;
					
					ResetColor ();
					
					cout << " │ ";
					
					if (isWarningOrFatal) cout << "\e[1m";
					
					SetColor (category.color);
					cout << txt;
					
					ResetColor ();

					cout << endl;
				}
			}
			else if (packet->msgType == e_epigramMsgType_class)
			{
				ConstEpigram msg (data + sizeof (EpilogQueuedMsgHeader), packet->msgSize);

				std::string className = msg ["cls"];
				
				if (className == c_epigram::nullString)
					className = "";
				
				classes [packet->locationId] = className;
				
//					cout << "register: " << className << endl;
			}
			else if (packet->msgType == c_epilogMsgType::defineCategory)
			{
				char * ptr = (char *) data + sizeof (EpilogQueuedMsgHeader);
				string categoryName = ptr;
				
				EpilogCategory * c = m_categoryMap [categoryName];
				
				if (! c)
				{
					c = new EpilogCategory;
					
					c->name = ptr;
					c->filter = false;
					c->level = 0; // no category specific filtering for an unknown category.
					c->color = e_epilogColor_black;
					
					m_categoryMap [categoryName] = c;
				}
				
				m_classIdToCategory [packet->locationId] = c;
				
//					cout << "cat: " << categoryName << " for: " << classes [packet->locationId] << endl;
			}
			
			m_queue->ReleaseMessage ();
		}
	}
}
