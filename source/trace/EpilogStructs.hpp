//
//  EpilogStructs.hpp
//  Epilog
//
//  Created by Steven Massey on 8/21/14.
//
//

#ifndef Epilog_EpilogStructs_h
#define Epilog_EpilogStructs_h

#include <inttypes.h>

typedef uint64_t location_t;

struct EpilogQueuedMsgHeader
{
	location_t		locationId;
	
	u8				msgType;
	u8				level;
	
	u16				msgSize			= 0;
	
	u32				timestamp;
	
	union
	{
		location_t	threadId;
		location_t	objectId;
	};
};

struct EpilogMsg : public EpilogQueuedMsgHeader
{
	EpilogMsg ()
	{
	}
	
	EpilogMsg (const EpilogMsg & i_msg)
	{
		memcpy (this, & i_msg, sizeof (EpilogQueuedMsgHeader));
		memcpy (payload, i_msg.payload, i_msg.msgSize);
	}

//	EpilogMsg & operator = (const EpilogMsg & i_msg)
//	{
//		memcpy (this, & i_msg, sizeof (EpilogQueuedMsgHeader));
//		memcpy (payload, i_msg.payload, i_msg.msgSize);
//
//		return *this;
//	}

	
	static const size_t c_payloadSize = 512 - sizeof (EpilogQueuedMsgHeader);
	
	u8				payload 		[c_payloadSize];

//	EpilogMsg (EpilogMsg && i_msg)
//	{
//		memcpy (this, & i_msg, sizeof (EpilogQueuedMsgHeader));
//		memcpy (payload, i_msg.payload, i_msg.msgSize);
//	}

	

};


enum
{
	c_epilogMsg_quit =						'q',
	e_epigramMsgType_register =				'r',
	e_epigramMsgType_registerVariable =		'R',
	e_epigramMsgType_log =					'L',
	e_epigramMsgType_deferredLog = 			'D',
	e_epigramMsgType_class =		 		'C',
	
	e_epigramMsgType_newPanel =				'P',
	e_epigramMsgType_panelLog =				'p',
	
	e_epigramMsgType_var =					'v',
	e_epigramMsgType_registerTable =		'T',
	e_epigramMsgType_tableColumn =			'c',
	e_epigramMsgType_tableEntry =			'e'
};

namespace c_epilogMsgType
{
	enum
	{
		defineCategory =		'G'
	};
};




#endif
