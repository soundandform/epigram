//  JdAtomicBankSwitch.h
//  Jigidesign
//
//  Created by Steven Massey on 4/11/15.
//  Copyright (c) 2015 Jigidesign. All rights reserved.

#ifndef __Jigidesign__JdAtomicBankSwitch__
#define __Jigidesign__JdAtomicBankSwitch__

#include "JdNucleus.hpp"
#include <atomic>

/*
 This class implementes an atomic mutex-free single-consumer, single-producer 2-bank switcher.  It assumes that the reader doesn't need
 to access to every bank write. It just assures that the reader and writer are always looking at the opposing buffer. The Writer signals to the
 reader that a new bank is available. The reader is responsible for the switch. If the read doesn't happen, the writer just requires the same bank
 and writes new information on the next pass.
 
 	- the higher bit is a signal from the writer to the reader that the banks can be swapped
 		- the writer turns this on when it has finished writing (ReleaseWrite)
 		- the reader turns it off, and simultaneously switch the bank, when it acquires (GetReadBank)
 		- the writer will turn it off, if necessary, when reacquiring a bank (in other words, GetReadBank didn't happen in time)
 
 	- the lower bit is the writer bank.  the reader bank is the inverse.

*/

class JdAtomicSwitch
{
	public:
	// called by the write thread
	u32			GetWriteIndex 		();			// returns 0 or 1
	void		ReleaseWrite	 	();
	
	// called by the read thread
	u32			GetReadIndex 		();			// returns 0 or 1
	
	protected:
	std::atomic <u8>			m_state			{ 0 };
};

template <typename T>
struct JdAtomicBankSwitchT
{
	T &			ViewWriteBank 		()
	{
		return m_banks [2];
	}

	T &			AcquireWriteBank 	()			// was GetWriteBank
	{
		m_writeAcquiredBank = & m_banks [m_switch.GetWriteIndex ()];
		return m_banks [2];
	}

	void		ReleaseWriteBank 	()
	{
		* m_writeAcquiredBank = m_banks [2];
		m_switch.ReleaseWrite ();
	}
	
	T &			GetReadBank 		()
	{
		return m_banks [m_switch.GetReadIndex ()];
	}
	
	u32			SwitchReadBank		()
	{
		return m_switch.GetReadIndex ();
	}

//	protected:
	// bank [2] is the static producer bank, that's copied to [0] or [1]
	// when the write happens to send it to the consumer.
	T				m_banks 				[3];
	
	T *				m_writeAcquiredBank		= nullptr;	// points to either bank 0 or 1
	
	JdAtomicSwitch	m_switch;
};


#endif
