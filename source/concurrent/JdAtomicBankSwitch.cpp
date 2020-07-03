//
//  JdAtomicBankSwitch.cpp
//  Jigidesign
//
//  Created by Steven Massey on 4/11/15.
//  Copyright (c) 2015 Jigidesign. All rights reserved.
//

#include "JdAtomicBankSwitch.hpp"


u32  JdAtomicSwitch::GetWriteIndex  ()		// returns 0 or 1
{
	while (true)
	{
		u8 state = m_state;
		
		u8 switchSignaled = state & 0x10;
		u8 writeBank = state & 0x01;
		
		if (switchSignaled == 0x00)
			return writeBank;
		
		// attempt to unsignal the reader switch (change 1X -> 0X)
		m_state.compare_exchange_weak (state, writeBank);
	}
}

void  JdAtomicSwitch::ReleaseWrite  ()
{
	// signal the reader to switch banks
	m_state |= 0x10;
}


u32  JdAtomicSwitch::GetReadIndex  ()	// returns 0 or 1
{
	u8 state = m_state;
	u8 switchSignaled = state & 0x10;
	u8 writeBank = state & 0x01;

	if (switchSignaled)
	{
		u8 switchedBank = writeBank ^ 0x01;
		m_state.compare_exchange_weak (state, switchedBank); // attempt to turn off the signal and switch bank
	}
	
	writeBank = m_state & 0x01;
	
	return writeBank ^ 0x01; // flip to opposite of writer
}
