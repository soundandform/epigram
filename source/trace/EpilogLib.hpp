#ifndef EpilogLib_h

#include <inttypes.h>

#include "Epigram.hpp"
#include "Epilog.hpp"
#include "EpilogStructs.hpp"

#include "JdThreadPort.hpp"
#include "JdTimer.hpp"

//#include "EpilogTable.h"

//#include <thread>


template <u32 t_msgSize>
struct EpilogQueuedMsg : EpilogQueuedMsgHeader
{
	EpilogQueuedMsg & operator = (const EpilogQueuedMsg &i_msg)
	{
		EpilogQueuedMsgHeader & header = *this;
		header = i_msg;
		
		const u32 c_maxMsgSize = sizeof (this) - sizeof (EpilogQueuedMsgHeader);
		
		if (i_msg.event.size <= c_maxMsgSize)
			memcpy (&event, &i_msg.event, i_msg.event.size);
		
//		cout << "size: " << i_msg.event.size + sizeof (EpilogQueuedMsgHeader) << endl;
		
		return * this;
	}
	
	EpilogEvent			event;
	u8					eventPayload [t_msgSize - sizeof (EpilogQueuedMsgHeader) - sizeof (EpilogEvent)];
};

typedef EpilogQueuedMsg <2048> EpilogQueuedMsg2k;


//struct EpilogQueueTableHeader : EpilogQueuedMsgHeader
//{
//	JdUUID 		rowId;
//	location_t	column;
//};


struct IEpilogCore
{
//	virtual void					DumpTables () = 0;
};



typedef JdMessageQueue2 <EpilogQueuedMsg2k> queue_t;


class CEpigramRunner
{
	public:
	CEpigramRunner					(const char *i_ipAddress, uint16_t i_portNumber, queue_t &i_queue, bool &i_isAlive, FILE *i_logFile, EpilogHandler i_handler, IEpilogCore * i_core)
	:
	m_core							(i_core),
	m_threadIsAlive					(i_isAlive),
	m_ipAddress						(i_ipAddress),
	m_portNumber					(i_portNumber),
	m_msgQueue						(i_queue),
	m_dataSent						(0),
    m_logFile						(i_logFile),
	m_handler						(i_handler)
	{
	//	d_jdAssert (sizeof (EpilogQueuedMsgHeader) == 32, "Epilog message header is the wrong size");
	}
	
	~ CEpigramRunner					()
	{
	}
	
	void operator () ();
	
	void							PrintLogToStdout (const EpilogQueuedMsgHeader &i_header, cstr_t i_logString)
	{
		// FIX: implement me better
		puts (i_logString);
	}
	
	void							SendToHandler (const EpilogQueuedMsgHeader &i_header, cstr_t i_logString)
	{
		if (m_handler)
		m_handler (i_logString);
	}
	
	
protected:
	IEpilogCore *					m_core;
	bool &							m_threadIsAlive;
	JdString128						m_ipAddress;
	uint16_t						m_portNumber;
	queue_t &						m_msgQueue;
	uint32_t						m_dataSent;
    FILE *							m_logFile;
	EpilogHandler					m_handler;
	
	JdTimer							m_timer;
	i64								m_lastTableDump = -1;
};


#include <boost/thread.hpp>
struct EpilogEvent;

#include "JdThreadPort.hpp"

#if 0
class CEpilogRunner : public JdThreadPort2 <CEpilogRunner>
{
	public:
	CEpilogRunner () : JdThreadPort2 (16384 /* msgs */) {}
	
//	d_jdPortFunc (
};
#endif


class CEpigramConnection : public IEpilogCore
{
	typedef void (CEpigramConnection::*sendFunc_t) (const EpilogQueuedMsgHeader &, const void *);
	
public:
	CEpigramConnection						();
	
	uint32_t	Connect						(const char *i_serviceNameOrLocation, uint16_t i_portNumber, bool i_logToFile, EpilogHandler i_handler);
	bool /* didQuit */		Quit			();
    
//    void    LogToFile                   (bool i_logFileEnabled) { m_logFileEnabled = i_logFileEnabled; }

//	void		SetHandler					(EpilogHandler * i_handler);
	
	void		Enable						(bool i_enabled) { m_enabled = i_enabled; }
	
	void		DefineCategory				(const char * i_className, const char * i_category);

	uint32_t	RegisterLocation			(void * i_locationId, uint32_t i_sessionId, const char *i_locationName, uint32_t i_lineNum, const char *i_name);
	uint32_t	RegisterVariable			(void * i_locationId, uint32_t i_sessionId, const char * i_functionName, uint32_t i_lineNum,
											 const char * i_variableName, uint8_t i_variableType);
	
	void		SendVariable				(uint32_t i_locationId, const void *i_value, uint8_t i_size);
	
//	void		RegisterTable				(void * i_tableId, const char * i_name);
//	void		DumpTable					(void * i_tableId);

//	void		DefineTableColumn			(void * i_tableId, uint32_t i_columnIndex, const char * i_name);
//	void		SetTableElement				(void * i_tableId, uint64_t i_rowId [2], uint32_t i_columnIndex, const char * i_contents);
//	void		DeleteTableRow				(void * i_tableId, uint64_t i_rowId [2]);

	void		NewPanel					(const Epigram &i_msg);//const char * i_panel);
 	void		LogToPanel					(const char * i_panel, const uint8_t * i_uuid, const char * i_info);

	void		LogDeferred					(uint8_t i_importance, const char * i_className, EpilogEvent * i_event);
//	void		LogTable					(const char * i_className, void * i_object, JdId  i_rowId, const char * i_columnName, EpilogEvent * i_event);
	
//	virtual void		DumpTables ();
	
protected:
	void		SendVariable8			(const EpilogQueuedMsgHeader &i_header, const void *i_value);
	void		SendVariable16			(const EpilogQueuedMsgHeader &i_header, const void *i_value);
	void		SendVariable32			(const EpilogQueuedMsgHeader &i_header, const void *i_value);
	void		SendVariable64			(const EpilogQueuedMsgHeader &i_header, const void *i_value);
	void		DummySend				(const EpilogQueuedMsgHeader &i_header, const void *i_value) { }
	
	void		QueueMessage			(const EpilogQueuedMsgHeader &i_header, IEpigramIn i_msg);
	void		QueueMessage			(const void *i_location, uint32_t i_messageType, IEpigramIn i_msg);
	
//	uint32_t	HashLocationId			(const void *i_location);
	
	sendFunc_t							m_senders [8];
	
	u32									m_messagesSent;			// erase me eventually
	std::atomic <u32>					m_sequence { 0 };
	
	queue_t								m_queue;
	
	boost::thread						m_thread;
	bool								m_queueMessages		= true;
	bool								m_enabled			= false;
    bool                                m_logFileEnabled	= false;
    FILE *                              m_logFile			= nullptr;
	
	std::atomic <u32>					m_clientCount { 0 };
	
	uint32_t							m_sessionId;
	JdTimer								m_timer;
    u32                                 m_maxMsgSize;
	
//	unordered_map
//	<void *, EpilogTableInternal *>		m_tables;
	
	std::mutex							m_mutex;
	unordered_map
	<const char *, const char *>		m_categories;
	
//	JdThreadT <CEpilogRunner>			m_runner { "epilog2", 0 /* priority */ };
};

#define EpilogLib_h
#endif
