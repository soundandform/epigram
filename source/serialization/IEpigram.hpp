//
//  IEpigram.hpp
//  Jigidesign
//
//  Created by Steven Massey on 4/28/13.
//  Copyright (c) 2013 Jigidesign. All rights reserved.
//

#ifndef Jigidesign_IEpigram_h
#define Jigidesign_IEpigram_h

#include <stddef.h>

struct IIEpigramIn // : Jd::TypedT <c_jdTypeId::epigram>
{
	struct Payload
	{
		union
		{
			const unsigned char *	bytes;
			const char *			chars;
			const void *			data;
		};
		
		size_t						size;
	};
	
//	virtual const unsigned char *	/* payload */		GetPayload			(unsigned int * o_size) const = 0;
	virtual Payload										GetPayload			() const = 0;
};


struct IIEpigramOut : IIEpigramIn
{
	virtual void										Deliver					(const IIEpigramIn * i_epigram) = 0;
	virtual void										Deliver					(const void * i_data, size_t i_size) = 0;
};

typedef IIEpigramOut IIEpigram;

typedef IIEpigramOut * IEpigramOut;
typedef const IIEpigramIn * IEpigramIn;

typedef IIEpigram * IEpigram;

typedef const IIEpigramIn * EpDelivery;
typedef IIEpigram * EpReceiver;


#endif
