//
//  IEpigram.hpp
//  Jigidesign
//
//  Created by Steven Massey on 4/28/13.
//  Copyright (c) 2013 Jigidesign. All rights reserved.
//

#ifndef Jigidesign_IEpigram_h
#define Jigidesign_IEpigram_h


struct IIEpigramIn // : Jd::TypedT <c_jdTypeId::epigram>
{
	struct Payload
	{
		union
		{
			const unsigned char *	bytes;
			const void *			data;
		};
		
		unsigned long				size;
	};
	
//	virtual const unsigned char *	/* payload */		GetPayload			(unsigned int * o_size) const = 0;			// FIX: size_t
	virtual Payload										GetPayload			() const = 0;
};


struct IIEpigramOut : IIEpigramIn
{
	virtual void										Deliver					(const IIEpigramIn * i_epigram) = 0;
	virtual void										Deliver					(const void * i_data, unsigned long i_size) = 0; // FIX: size_t
};

typedef IIEpigramOut IIEpigram;

typedef IIEpigramOut * IEpigramOut;
typedef const IIEpigramIn * IEpigramIn;

typedef IIEpigram * IEpigram;

typedef const IIEpigramIn * EpDelivery;
typedef IIEpigram * EpReceiver;


#endif
