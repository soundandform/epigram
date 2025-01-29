//
//  JdMD5.hpp
//
//  Created by Steven Massey on 7/16/13.
//  Copyright (c) 2013 Steven Massey. All rights reserved.
//

#ifndef __UPF__MpMD5__
#define __UPF__MpMD5__

#include "JdNucleus.hpp"

#include <string>
#include <iostream>

#include "MD5.h"

class JdMD5
{
    public:						JdMD5               ()
    {
        MD5Init (& m_md5);
    }
   
    
    void                        Add                 (std::istream &i_stream)
    {
        while (i_stream)
        {
            char buffer [4096];
            i_stream.read (buffer, sizeof (buffer));
            Add ((u8*) buffer, (u32) i_stream.gcount());
        }
    }
    
    void                        Add                 (const u8 * i_bytes, u32 i_numBytes)
    {
        m_totalBytes += i_numBytes;
        MD5Update (&m_md5, (unsigned char *) i_bytes, i_numBytes);
    }
    
    
    u32                         NumBytesHashed      ()
    {
        return m_totalBytes;
    }
    
    
                                operator std::string ()
    {
        return GetString();
    }
	
	struct MD5
	{
		u8 digest [16] = {};
		
		auto operator <=> (const MD5 & i_other) const
		{
			return memcmp (digest, i_other.digest, sizeof (digest));
		}
		
		bool operator == (const MD5 & i_other) const
		{
			return memcmp (digest, i_other.digest, sizeof (digest)) == 0;
		}
		
		void Clear () { std::fill (std::begin (digest), std::end (digest), 0); }
		
		std::string  GetString  () const
		{
			std::string string;
			
			char temp [8];

			for (int i = 0; i < 16; i++)
			{
				snprintf (temp, 8, "%02x", digest[i]);
				string += temp;
			}


			return string;
		}
		
		
	};
	
	MD5						Get						()
	{
		MD5Final (& m_md5);
		m_finalized = true;
		
		MD5 md5; memcpy (md5.digest, m_md5.digest, 16);
		
		return md5;
	}
    
    std::string                 GetString            ()
    {
        if (! m_finalized)
        {
            MD5Final (& m_md5);
            m_finalized = true;
        }
        
        char temp [256];
        MD5String (&m_md5, temp);
        return temp;
    }
    
    protected:
    bool                        m_finalized			= false;
    MD5_CTX                     m_md5;
    u32                         m_totalBytes		= 0;
};


std::ostream & operator << (std::ostream & stream, const JdMD5::MD5 & i_md5);

#endif
