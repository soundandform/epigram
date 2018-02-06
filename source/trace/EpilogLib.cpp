/*
 *  EpilogLib.cpp
 *  Epigram
 *
 *  Created by Steven Massey on 5/13/11.
 *  Copyright 2011-2014 Epigram Software, LLC. All rights reserved.
 *
 */

#include <cstdlib>
#include <deque>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/functional/hash.hpp>
#include <unordered_map>

using boost::asio::ip::tcp;

#include "EpilogLib.hpp"
#include "IJdPlatform.h"
#include "JdUtils.hpp"


static CEpigramConnection * GetEpilog ()
{
    static CEpigramConnection s_epilog;
    return & s_epilog;
}


u32 epg_Init (cstr_t i_serviceNameOrLocation, u16 i_portNumber, bool i_logToFile, EpilogHandler i_handler)
{
	return GetEpilog()->Connect (i_serviceNameOrLocation, i_portNumber, i_logToFile, i_handler);
}


void epg_Quit ()
{
    GetEpilog ()->Quit ();
}


//void epg_SetHandler (EpilogHandler * i_handler)
//{
//	GetEpilog ()->SetHandler ();
//}
//

void epg_Disable ()
{
	GetEpilog()->Enable (false);
}


void epg_Enable ()
{
	GetEpilog()->Enable (true);
}


void epg_DefineCategory (const char * i_className, const char * i_category) { GetEpilog()->DefineCategory (i_className, i_category); }


void CEpigramConnection::DefineCategory (const char * i_className, const char * i_category)
{
	if (! m_enabled) return;

	bool sendMsg = false;
	
	if (true)
	{
		// these calls should all end happening at static init, thus in the main thread... but for good measure, a mutex:
		std::lock_guard <std::mutex> lock (m_mutex);
		
		const char * & category = m_categories [i_className];
		
		if (category == nullptr)
		{
//			cout << i_className << " -> " << i_category << endl;
			category = i_category;
			sendMsg = true;
		}
	}
	
	if (sendMsg)
	{
		EpilogQueuedMsg2k msg;
		
		msg.msgType = c_epilogMsgType::defineCategory;
		msg.locationId = (location_t) i_className;
		
		msg.msgSize = strlen (i_category) + 1;
		msg.event.size = msg.msgSize + sizeof (EpilogEvent);
		
		memcpy (msg.eventPayload, i_category, msg.msgSize);
		
		m_queue.QueueMessage (msg);
	}
}



uint32_t epg_RegisterLocation (void * i_location, uint32_t i_sessionId, const char *i_locationName, uint32_t i_lineNum, const char *i_name)
{
	return GetEpilog()->RegisterLocation (i_location, i_sessionId, i_locationName, i_lineNum, i_name);
}


uint32_t epg_RegisterVariable (void * i_location, uint32_t i_sessionId, const char * i_functionName, uint32_t i_lineNum, 
							   const char * i_variableName, uint8_t i_variableType)
{
	return GetEpilog()->RegisterVariable (i_location, i_sessionId, i_functionName, i_lineNum, i_variableName, i_variableType);
}


void epg_LogDeferred (uint8_t i_importance, const char * i_className, EpilogEvent * i_event)
{
	GetEpilog()->LogDeferred (i_importance, i_className, i_event);
}


//void epg_Table (const char * i_className, void * i_object, JdId  i_rowId, const char * i_columnName, EpilogEvent * i_event)
//{
//	GetEpilog()->LogTable (i_className, i_object, i_rowId, i_columnName, i_event);
//}



void epg_RegisterEventType (uint32_t i_eventId, EpEventFormatter i_eventFormatter)
{
	
}


void epg_LogEvent (uint32_t i_eventType, const void * i_eventStruct, uint32_t i_eventStructSize)
{
	
}


void epg_LogVariable (uint32_t i_locationId, const void *i_value, uint8_t i_size)
{
	GetEpilog()->SendVariable (i_locationId, i_value, i_size);
}


//void epg_RegisterTable (void * i_tableId, const char * i_name)
//{
//	GetEpilog()->RegisterTable (i_tableId, i_name);
//}
//
//void epg_DumpTable (void * i_tableId)
//{
//	GetEpilog()->DumpTable (i_tableId);
//}


//void epg_DefineTableColumn (void * i_tableId, uint32_t i_columIndex, const char * i_name)
//{
//	GetEpilog()->DefineTableColumn (i_tableId, i_columIndex, i_name);
//}
//
//void epg_SetTableElement (void * i_tableId, uint64_t i_rowId [2], uint32_t i_columnIndex, const char * i_contents)
//{
//	GetEpilog()->SetTableElement (i_tableId, i_rowId, i_columnIndex, i_contents);
//}
//
//void epg_DeleteTableRow (void * i_tableId, uint64_t i_rowId [2])
//{
//	GetEpilog()->DeleteTableRow (i_tableId, i_rowId);
//}

void epg_NewPanel (const char * i_panel, ...)
{
	va_list args;
	va_start(args, i_panel);
	
	Epigram msg; msg ("panel", i_panel);
	const char *columnName;
	std::vector <std::string> columns;
	
	while (true)
	{
		columnName = va_arg (args, const char *);
		if (! columnName) break;
		
		columns.push_back (columnName);
//		cout << "col: " << columnName << endl;
	}
	
	msg ("columns", columns);
	va_end(args);
	
	GetEpilog()->NewPanel (msg);
}


void epg_LogToPanel (const char * i_panel, const uint8_t * i_uuid, const char * i_info)
{
	GetEpilog()->LogToPanel (i_panel, i_uuid, i_info);
}




CEpigramConnection::CEpigramConnection ()
	:
	m_queue							(2048),
	m_sessionId						(0)
{
	m_messagesSent = 0;
	
	m_senders [1-1] = &CEpigramConnection::SendVariable8;
	m_senders [2-1] = &CEpigramConnection::SendVariable16;
	m_senders [3-1] = &CEpigramConnection::DummySend;
	m_senders [4-1] = &CEpigramConnection::SendVariable32;
	m_senders [5-1] = &CEpigramConnection::DummySend;
	m_senders [6-1] = &CEpigramConnection::DummySend;
	m_senders [7-1] = &CEpigramConnection::DummySend;
	m_senders [8-1] = &CEpigramConnection::SendVariable64;
}


u32 CEpigramConnection::Connect (const char * i_serviceNameOrLocation, uint16_t i_portNumber, bool i_logToFile, EpilogHandler i_handler)
{
    m_maxMsgSize = 0;
    
	int previous = m_clientCount;
	++m_clientCount;
	
	if (previous == 0)
	{
		m_enabled = true;
		
        if (i_logToFile)
        {
            m_logFileEnabled = true;
            
            auto fs = JdPlatform::Get <IJdPlatform::FileSystem> ();

            string path = fs->GetSandboxPath (c_jdFileSystemLocation::appData) + JdNewUUID ().ToString () + ".epilog";
            
            m_logFile = fopen (path.c_str(), "w");
        }
        
		CEpigramRunner runner (i_serviceNameOrLocation, i_portNumber, m_queue, m_queueMessages, m_logFile, i_handler, this);
		m_thread = boost::thread (runner);
	}
	
	return ++m_sessionId;
}


bool CEpigramConnection::Quit ()
{
	if (--m_clientCount == 0)
	{
	//	if (m_queueMessages)
		{
			EpilogQueuedMsg2k msg;
			msg.event.size = 0;
			msg.msgType = c_epilogMsg_quit;
			m_queue.QueueMessage (msg);
		}

		m_thread.join();
		
        if (m_logFile) fclose (m_logFile);
        
//		cout << "msgs: " << m_messagesSent << " ";
        return true;
	}
    else return false;
}


//void CEpigramConnection::SetHandler (EpilogHandler * i_handler)
//{
//	
//}
//

void CEpigramConnection::SendVariable (uint32_t i_locationId, const void *i_value, uint8_t i_size)
{
	if (m_queueMessages)
	{
		EpilogQueuedMsgHeader header = { i_locationId, e_epigramMsgType_var };
		
		i_size = 0x07 & (i_size - 1);
		
		sendFunc_t &memPtr = m_senders[i_size];
		((*this).* (memPtr)) (header, i_value);
	}
}

void CEpigramConnection::SendVariable8 (const EpilogQueuedMsgHeader &i_header, const void *i_value)
{
	QueueMessage (i_header, msg_ ("var", *((uint8_t*) i_value)));
}


void CEpigramConnection::SendVariable16 (const EpilogQueuedMsgHeader &i_header, const void *i_value)
{
	QueueMessage (i_header, msg_ ("var", *((uint16_t*) i_value)));
}


void CEpigramConnection::SendVariable32 (const EpilogQueuedMsgHeader &i_header, const void *i_value)
{
	QueueMessage (i_header, msg_ ("var", *((uint32_t*) i_value)));
}


void CEpigramConnection::SendVariable64 (const EpilogQueuedMsgHeader &i_header, const void *i_value)
{
	QueueMessage (i_header, msg_ ("var", *((uint64_t*) i_value)));
}


uint32_t CEpigramConnection::RegisterLocation (void *i_location, uint32_t i_sessionId, const char *i_functionName, uint32_t i_lineNum, const char *i_name)
{
	size_t hash = 0;
	boost::hash_combine(hash, i_functionName);
	boost::hash_combine(hash, i_lineNum);
	boost::hash_combine(hash, i_name);
	
	uint32_t h = hash; h ^= (hash >> 32);

	if (m_queueMessages)
	{
		EpilogQueuedMsgHeader header = { h, e_epigramMsgType_register };

		Epigram msg;
		msg ("loc", i_functionName) ("num", i_lineNum);
		if (i_name) msg ("nme", i_name);
		QueueMessage (header, msg);
	}
	
	return h;
}


uint32_t CEpigramConnection::RegisterVariable (void *i_location, uint32_t i_sessionId, const char * i_functionName, uint32_t i_lineNum, 
												const char * i_variableName, uint8_t i_variableType)
{
	size_t hash = 0;
	boost::hash_combine(hash, i_functionName);
	boost::hash_combine(hash, i_lineNum);
	boost::hash_combine(hash, i_variableName);
	
	uint32_t h = hash; h ^= (hash >> 32);
	
	if (m_queueMessages)
	{
		EpilogQueuedMsgHeader header = { h, e_epigramMsgType_registerVariable };
		
//		cout << "reg: " << i_variableType << endl;
		QueueMessage (header, msg_ ("loc", i_functionName) ("num", i_lineNum) ("var", i_variableName) ("typ", i_variableType));
	}
	return h;
}

#if 0
void CEpigramConnection::RegisterTable (void * i_tableId, const char * i_name)
{
	std::lock_guard <std::mutex> lock (m_mutex);

	if (m_tables [i_tableId] == nullptr)
		m_tables [i_tableId] = new EpilogTableInternal (i_name);

//	QueueMessage (i_location, e_epigramMsgType_registerTable, msg_ ("tbl", i_name) ("key", i_key));
}


void CEpigramConnection::DumpTable (void * i_tableId)
{
	std::lock_guard <std::mutex> lock (m_mutex);
	
	auto table = m_tables [i_tableId];
	if (table)
	{
		cout << table->ToString() << endl;
	}
	
	//	QueueMessage (i_location, e_epigramMsgType_registerTable, msg_ ("tbl", i_name) ("key", i_key));
}


void CEpigramConnection::DefineTableColumn (void * i_tableId, uint32_t i_columnIndex, const char * i_name)
{
	std::lock_guard <std::mutex> lock (m_mutex);

	EpilogTableInternal * table = m_tables [i_tableId];
	if (table)
	{
		table->SetElement (JdUUID (), i_columnIndex, i_name);
	}
}


void CEpigramConnection::SetTableElement (void * i_tableId, uint64_t i_rowId [2], uint32_t i_columnIndex, const char * i_contents)
{
	std::lock_guard <std::mutex> lock (m_mutex);

	EpilogTableInternal * table = m_tables [i_tableId];

	if (table)
	{
		table->SetElement (JdUUID (i_rowId [1], i_rowId[0]), i_columnIndex, i_contents);
	}
}


void CEpigramConnection::DeleteTableRow (void * i_tableId, uint64_t i_rowId [2])
{
	std::lock_guard <std::mutex> lock (m_mutex);
	
	EpilogTableInternal * table = m_tables [i_tableId];
	
	if (table)
	{
		table->DeleteRow (JdUUID (i_rowId [1], i_rowId[0]));
	}
}


void CEpigramConnection::DumpTables ()
{
	std::lock_guard <std::mutex> lock (m_mutex);
	
	for (auto t : m_tables)
	{
		EpilogTableInternal * table = t.second;
		
		cout << table->ToString () << "\n" << endl;
	}
}
#endif


void CEpigramConnection::NewPanel (const Epigram &i_msg)//const char *i_panel)
{
	QueueMessage (0, e_epigramMsgType_newPanel, i_msg);
}


void CEpigramConnection::LogToPanel (const char * i_panel, const uint8_t * i_uuid, const char * i_info)
{
//	cout << "panel: " << i_panel << " " << i_info << endl;
	
	/*
	EpilogQueuedMsgHeader header;
	header.locationId = 0;	// FIX: make into panelid
	header.msgType = e_epigramMsgType_panelLog;
	header.timestamp = 0;
	header.msgSize =
	*/
	
	QueueMessage (i_panel, e_epigramMsgType_panelLog, msg_("panel", i_panel)("info", i_info));
}


void CEpigramConnection::LogDeferred (uint8_t i_importance, const char * i_className, EpilogEvent * i_event)
{
	if (! m_enabled) return;
	if (i_event->size > sizeof (EpilogQueuedMsg2k::eventPayload) + sizeof (EpilogEvent))
	{
		cout << "epilog event overflow!";
		return;
	}
	
	// i_event points to the event (allocated on the call stack) with some extra padding so that an EpilogQueuedMsgHeader can be inserted before it.
	
	EpilogQueuedMsgHeader * msgPtr = (EpilogQueuedMsgHeader *) i_event - 1;
	EpilogQueuedMsg2k & msg = * static_cast <EpilogQueuedMsg2k *> (msgPtr);

	msg.locationId = (location_t) i_className;
	msg.msgType = e_epigramMsgType_deferredLog;
	msg.level = i_importance;
	msg.timestamp = m_timer.GetMilliseconds ();
//	msg.msgSize = i_event->size;						// FIX: this isn't really used...
	msg.threadId = (location_t) pthread_self ();
	
	m_queue.QueueMessage (msg);		// this copies the entire msg/event to the queue, so it can be formatted later
}


#if 0
void CEpigramConnection::LogTable (const char * i_className, void * i_object, JdId  i_rowId, const char * i_columnName, EpilogEvent * i_event)
{
	EpilogQueueTableHeader header;
	
	header.locationId = (location_t) i_className;
	header.msgType = e_epigramMsgType_tableEntry;
	header.level = c_epilogClassification_normal;
	header.timestamp = m_timer.GetMilliseconds();
	header.msgSize = i_event->size;

	header.objectId = (location_t) i_object;
	
	header.column = (location_t) i_columnName;
	header.rowId = i_rowId;
	
	
//	m_msgQueue.Insert (&header, sizeof (EpilogQueueTableHeader), i_event, header.msgSize);
//	++m_messagesSent;
}
#endif


void CEpigramConnection::QueueMessage (const void *i_location, uint32_t i_msgType, IEpigramIn i_msg)
{
	auto msgData = i_msg->GetPayload ();
		
	EpilogQueuedMsgHeader header;
	
//	header.locationId = HashLocationId (i_location);
	header.msgType = i_msgType;
	header.timestamp = m_timer.GetMilliseconds();
	header.msgSize = msgData.size;
	
//	m_msgQueue.Insert (&header, sizeof (EpilogQueuedMsgHeader), msgData, msgSize);
//	++m_messagesSent;
}


void CEpigramConnection::QueueMessage (const EpilogQueuedMsgHeader &i_header, IEpigramIn i_msg)
{
	auto msg = i_msg->GetPayload ();
	
	EpilogQueuedMsgHeader header = i_header;
	
	header.timestamp = m_timer.GetMilliseconds();
	header.msgSize = msg.size;

//	m_msgQueue.Insert (&header, sizeof (EpilogQueuedMsgHeader), msg.data, msg.size);
//
//	++m_messagesSent;
}


void CEpigramRunner::operator () ()
{
	unordered_map <void *, bool> locations;
	
	pthread_setname_np ("Epilog");
		
	// drop the priority of this thread
	struct sched_param sp = {};
	sp.sched_priority = 0;
	
    if (pthread_setschedparam (pthread_self(), SCHED_RR, &sp) == -1)
        cout << "epilog: thread priority change failed." << endl;
	
	boost::asio::io_service io_service;
	tcp::socket socket (io_service);
	
	bool tcpIpConnection = false;

	string address = m_ipAddress;
    if (address != "" && address != "console")
    {
        try {
            
//            cout << endl << "epilog trying to connect to " << m_ipAddress << ":" << m_portNumber << "\n" << endl;

            char portString [64]; sprintf (portString, "%d", m_portNumber);
            
            tcp::resolver resolver (io_service);
            tcp::resolver::query query (m_ipAddress, portString);
            tcp::resolver::iterator iterator = resolver.resolve (query);
            
            boost::asio::connect (socket, iterator);
            
            tcpIpConnection = true;
        }
        
        catch (const std::exception &e)
        {
            cout << "\n" << e.what() << endl;
            cout << "Connection failure: Make sure both devices are on the same Wi-Fi network. Other possible causes of failure: The server is not running, there is a mismatched IP address or port number between client and server, or possibly the port number is clogged: try changing the port number in your client & then recompile.  Use the --port command line option to change the server port number.\n" << endl;
        }
    }
	
	const u32 maxNumMessages = 1; // more doesn't seem to matter...
	char buffer [1028 + 4096 * maxNumMessages]; // EpilogPrint expects 4k string, so need header+4k
	
	bool quit = false;
	
	try
	{
	
	while (not quit)
	{
		u32 numMessages = m_msgQueue.WaitForMessages ();
		
		while (numMessages)
		{
			u32 numMessagesToProcess = std::min (numMessages, maxNumMessages);
			numMessages -= numMessagesToProcess;
			char * ptr = buffer;
			
			for (u32 i = 0; i < numMessagesToProcess; ++i)
			{
				EpilogQueuedMsg2k & msg = * m_msgQueue.ViewMessage ();
				
				if (msg.msgType == c_epilogMsg_quit)
				{
//					cout << "epilog exit." << endl;
					quit = true;
					break;
				}
				
//				if (msg.msgSize)
				{
					
					if (msg.locationId)
					{
						bool & location = locations [(void *) msg.locationId];
						
						if (! location)
						{
							location = true;
							cstr_t className = (cstr_t) msg.locationId;
							
							if (tcpIpConnection)
							{
								Epigram epMsg;			// FIX: Hmm, Epigram in-place
								epMsg ("cls", className);
								
								auto payload = epMsg.GetPayload ();
								
								EpilogQueuedMsgHeader newHeader = { msg.locationId, e_epigramMsgType_class, 0, (uint16_t) payload.size, 0 };
								
								size_t size = boost::asio::write (socket, boost::asio::buffer (&newHeader, sizeof (EpilogQueuedMsgHeader)));
								m_dataSent += size;
								
								size = boost::asio::write (socket, boost::asio::buffer (payload.data, payload.size));
								m_dataSent += size;
							}
						}
					}
					
					
					if (msg.msgType == e_epigramMsgType_deferredLog) //|| header->msgType == e_epigramMsgType_tableEntry)
					{
						char * logString = ptr + sizeof (EpilogQueuedMsgHeader);
						msg.event.formatter (logString, & msg.event);
						u32 stringLen = strlen (logString);

						if (tcpIpConnection)
						{
							EpilogQueuedMsgHeader & header = * ((EpilogQueuedMsgHeader *) ptr);

							header.locationId = msg.locationId;
							header.msgType = e_epigramMsgType_log;
							header.level = msg.level;
							header.msgSize = (uint16_t) stringLen;
							header.timestamp = msg.timestamp;
							header.threadId = msg.threadId;
							
							ptr += (sizeof (EpilogQueuedMsgHeader) + stringLen);
						}
						else
						{
							if (! m_handler) PrintLogToStdout (msg, logString);
						}
						
						if (m_logFile)
						{
							logString [stringLen] = '\n';
							logString [stringLen + 1] = 0;
							fwrite (logString, 1, stringLen + 1, m_logFile);
						}
						
//						if (m_handler)
							SendToHandler (msg, logString);
					}
					else if (msg.msgType == c_epilogMsgType::defineCategory)
					{
						if (tcpIpConnection)
						{
							EpilogQueuedMsgHeader & header = * ((EpilogQueuedMsgHeader *) ptr);
							
							header.locationId = msg.locationId;
							header.msgType = c_epilogMsgType::defineCategory;
//							header.level = msg.level;
							header.msgSize = msg.msgSize;
//							header.timestamp = msg.timestamp;
//							header.threadId = msg.threadId;
							
							char * category = ptr + sizeof (EpilogQueuedMsgHeader);
							memcpy (category, msg.eventPayload, msg.msgSize);
							
							ptr += (sizeof (EpilogQueuedMsgHeader) + msg.msgSize);
						}
					}
				}
				
				m_msgQueue.ReleaseMessage ();
			} // numToProces
			
			if (tcpIpConnection)
			{
				size_t size = boost::asio::write (socket, boost::asio::buffer (buffer, ptr - buffer));
				m_dataSent += size;
			}
			
		} // numMessages
	} // ! quit
	}
	
	catch (...)
	{
		// socket died probably
	}
	
    socket.close();
    io_service.stop();
}





