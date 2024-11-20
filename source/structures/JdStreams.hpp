//  JdStreams.hpp
//
//  Created by Steven Massey on 11/15/24.

#ifndef JdStreams_hpp
#define JdStreams_hpp

# include "JdMessageQueue.hpp"
# include "STLHelpers.hpp"
# include <map>


using std::map, std::string;

const u32 c_packetSize = 1024-16; //1024;

struct JdStreams
{
	JdStreams (u32 i_sampleRate = 0)  : m_sampleRate (i_sampleRate) {}
	
	void SetSampleRate (u32 i_sampleRate) { m_sampleRate = i_sampleRate; }

	
	struct Packet
	{
		u32		sourceId;
		u16		isFinalPacket;
		u16		sampleCount;
		i64		timestamp;
		
		f64 * 	GetSamples ()
		{
			return (f64 *) (this + 1);
		}
	};
	
	// static/global for all streams
	struct Spring
	{
		Spring (u32 i_packetSizeInBytes = c_packetSize * sizeof (f64) + sizeof (Packet), u32 i_maxNumPackets = 1024)
		:
		m_packetSize	(i_packetSizeInBytes),
		m_store			(i_maxNumPackets),
		m_record		(i_maxNumPackets)
		{
			d_jdAssert (sizeof (Packet) == sizeof (u64) * 2);
		}
		
		~ Spring ()
		{
			// gonna assume that not every packet ends up being released when Lua code goes wonky
			// so we need a record of all generated packets
			Packet * dead;
			while (m_record.Pop (dead))
				delete dead;
		}
		
		Packet *		AcquirePacket		()
		{
			Packet * packet = nullptr;
			
			if (not m_store.Pop (packet))
			{
				if (m_numPackets++ < m_store.GetNumMessageCapacity ())
				{
//					jd::out ("packet @", m_numPackets.load ());
					packet = (Packet *) new u8 [m_packetSize];
					m_record.Push (packet);
				}
			}

			return packet;
		}

		void			ReleasePacket		(Packet * i_packet)
		{
			m_store.Push (i_packet);
		}

		u32								m_packetSize;
		JdMessageQueue <Packet *>		m_store;
		atomic <i64>					m_numPackets			{ 0 };

		JdMessageQueue <Packet *>		m_record;
	};
	
	static Spring				s_spring;
	
	
	struct Stream
	{
					~Stream			()
		{
			FlushStream ();
		}
		
		
		void		SetSampleRate			(u32 * i_sampleRate)
		{															//if (m_sampleRate) d_jdAssert (m_sampleRate == i_sampleRate);
			m_sampleRate = i_sampleRate;
		}
		
		void		FlushStream		()
		{
			if (m_writePacket)
				ReleasePacket (m_writePacket);
			if (m_readPacket)
				ReleasePacket (m_readPacket);

			Packet * packet;
			while (packet = TryGetPacket ())
			{
				ReleasePacket (packet);
			}
		}
		
		bool		IsReaderActive	()
		{
			bool active = m_readAccessCount;
			m_readAccessCount -= active;

//			jd::out ((u64) m_readAccessCount);

			return active;
		}
		
		void		Send	    	(i64 i_index, f64 i_value)
		{																				//	d_jdAssert (not isnan (i_value));
			if (m_writePacket)
			{
				i64 expectedIndex = m_writePacket->timestamp + m_writePacket->sampleCount;
				
				if (i_index != expectedIndex)
				{
					m_writePacket->isFinalPacket = true;
					m_queue.Push (m_writePacket);
					m_writePacket = nullptr;
				}
			}
		
			if (m_writePacket == nullptr)
			{
				m_writePacket = s_spring.AcquirePacket ();
				if (m_writePacket)
				{
					m_writePacket->timestamp = i_index;
					m_writePacket->sampleCount = 0;
				}
				else
				{
					++m_writeDropped;										jd::out ("stream write failed ******");
				}
			}
			
			if (m_writePacket)
			{
				i64 offset = i_index - m_writePacket->timestamp;
				auto ptr = (f64 *) (m_writePacket + 1);
				ptr [offset] = i_value;
				m_writePacket->sampleCount++;
				
				if (offset == m_packetSize - 1)
				{
					m_writePacket->isFinalPacket = false;
					m_queue.Push (m_writePacket);
					m_writePacket = nullptr;
				}
			}
		}

		
		void		Prime	    	(f64 * i_values, u32 i_numSamples)
		{																							d_jdAssert (m_writePacket == nullptr);
			Packet * packets [100];

			u32 numPackets = (i_numSamples + m_packetSize - 1) / m_packetSize;						d_jdAssert (numPackets <= 100);
			
			i64 timestamp = 0;
			for (u32 i = 0; i < numPackets; ++i)
			{
				auto packet = packets [i] = s_spring.AcquirePacket ();
				packet->timestamp = timestamp;
				timestamp += m_packetSize;
				
				u32 copySize = jd::min (i_numSamples, m_packetSize);
				
				packet->sampleCount = copySize;

				jd::memclr (packet->GetSamples (), m_packetSize);
				jd::memcpy (packet->GetSamples (), i_values, copySize);
				
				i_numSamples -= m_packetSize;
				i_values += m_packetSize;
			}
			
			for (u32 i = 0; i < numPackets; ++i)
				m_queue.Push (packets [i]);
		}

		
		void		EndSend	    	(i64 i_index)
		{
			if (m_writePacket == nullptr)
			{
				m_writePacket = s_spring.AcquirePacket ();
				if (m_writePacket)
					m_writePacket->timestamp = i_index;
			}
			
			if (m_writePacket)
			{
				auto ptr = (f64 *) (m_writePacket + 1);
				i64 offset = i_index - m_writePacket->timestamp;
				
				m_writePacket->sampleCount = offset;
				m_writePacket->isFinalPacket = true;
				
				while (offset < m_packetSize)
					ptr [offset++] = 0.;

				m_queue.Push (m_writePacket);
				m_writePacket = nullptr;
			}
		}

		
		f64			Receive			()
		{
			f64 value = 0.;
			
			if (m_readPacket == nullptr)
			{
//				d_jdStopwatch ("read");
				m_readPacket = (m_readActive) ? GetPacket () : TryGetPacket ();
				m_readIndex = 0;
				
//				if (not m_readActive and m_readPacket)
//				{
//					m_readActive = true;
//					jd::out ("start read");
//				}
			}

			if (m_readPacket)
			{
				auto ptr = (f64 *) (m_readPacket + 1);
				value = ptr [m_readIndex++];

				if (m_readIndex == m_packetSize)
				{
					if (m_readPacket->isFinalPacket)
					{
						m_readActive = false;
//						jd::out ("read end---------------");
					}
					ReleasePacket (m_readPacket);
					m_readPacket = nullptr;
				}
			}
			
			return value;
		}
		
		u32  		GetPacketSize	() const
		{
			return m_packetSize;
		}
		
		u32			GetSampleRate	() const
		{																d_jdAssert (m_sampleRate);
			return * m_sampleRate;
		}
		
		u32			GetIdealQueueStuffingSamples  ()
		{
			f64 primeTime = .1; // 100 ms = three 30 frames-per-sec callbacks
			
			u32 numSamples = primeTime * GetSampleRate ();
			
			u32 numPackets = (numSamples + GetPacketSize () - 1) / GetPacketSize ();
			numSamples = numPackets * GetPacketSize ();
			
			return numSamples;
		}
		
		u32			GetQueueStuffNumSamples  ()
		{
			i32 idealNumPacketsStuffed = GetIdealQueueStuffingSamples () / GetPacketSize ();
			
			i32 numPacketsInQueue = m_queue.debug_get_num_messages_in_queue ();
			
			i32 additionalPackets = idealNumPacketsStuffed - numPacketsInQueue;
			
			if (additionalPackets < 0)
				additionalPackets = 0;
			
			return additionalPackets * GetPacketSize ();
		}
		
		//--------------------------------------------------------------
		
		Packet *	GetPacket		()
		{
			m_readAccessCount = 10;

			Packet * packet = nullptr;
			m_queue.PopWait (packet);
			return packet;
		}

		
		Packet *	TryGetPacket	()
		{
			m_readAccessCount = 10;
			
			Packet * packet = nullptr;
			m_queue.Pop (packet);

			return packet;
		}
		
		void		ReleasePacket	(Packet * i_packet)
		{
//			jd::out ("release packet");
			s_spring.ReleasePacket (i_packet);
		}
		
		u32	*							m_sampleRate		= nullptr;
			
		Packet *						m_writePacket		= nullptr;
		u64								m_writeDropped		= 0;
		
		Packet *						m_readPacket		= nullptr;
		i64								m_readIndex			= 0;
		bool							m_readActive		= false;

		JdMessageQueue <Packet *> 		m_queue;
		u32								m_packetSize 		= c_packetSize;			// HACK: hardcode
		
		atomic <i32>					m_readAccessCount	{ 1 };
	};
	
	
	Stream *		AcquireStream		(cstr_t i_identifier)
	{
		lock_guard <mutex> lock (m_mutex);
		
		auto & stream = m_streams [i_identifier];
		
		stream.SetSampleRate  (& m_sampleRate);
		
		return & stream;
	}
	
	
	void			diagnostics			()
	{
		lock_guard <mutex> lock (m_mutex);

		i64 created = s_spring.m_record.debug_get_num_messages_in_queue (); //s_spring.m_numPackets.load ();
		i64 queued = s_spring.m_store.debug_get_num_messages_in_queue ();
		i64 running = created - queued;
		
		printf ("spring | malloc'd packets: %llu (of %d); avail: %lld; inuse: %lld\n",
					created, s_spring.m_store.GetNumMessageCapacity (), queued, running);
		
		for (auto & i : m_streams)
		{
			auto & s = i.second;
			printf ("%*s: %d; dropped: %lld\n", 40, i.first.c_str (), s.m_packetSize, s.m_writeDropped);
		}
	}
	
	u32								m_sampleRate;
	map <string, Stream>			m_streams;
	mutex							m_mutex;
};





#endif /* JdStreams_hpp */
