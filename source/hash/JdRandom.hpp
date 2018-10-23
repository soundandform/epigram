//
//  JdRandom.hpp
//  Jigidesign
//
//  Created by Steven Massey on 5/13/16.
//  Copyright © 2016 Jigidesign. All rights reserved.
//

#ifndef JdRandom_hpp
#define JdRandom_hpp

#include "JdNucleus.hpp"
#include <random>
#include <mutex>

class JdRandom
{
	public:
	
	template <typename T>
	static T Get ()
	{
		return s_64.Generate ();
	}
	

	template <typename T>
	static T GetPositive ()
	{
		return s_64.Generate () & 0x7fffffffffffffff;
	}

	protected:
	struct Generator64
	{
		u64 Generate ()
		{
			std::lock_guard <std::mutex> lock (m_lock);
			
			return dis (gen);
		}
		
		std::random_device rd;
		std::mt19937_64 gen { rd() };
		
		std::uniform_int_distribution <u64> dis;

		std::mutex m_lock;
	};
	
	static Generator64 s_64;
};


#endif /* JdRandom_hpp */
