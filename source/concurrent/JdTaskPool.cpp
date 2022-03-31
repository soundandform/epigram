//
//  JdTaskPool.cpp
//
//  Created by Steven Massey on 3/4/22.
//  Copyright © 2022 Steven Massey. All rights reserved.
//

#include "JdTaskPool.hpp"



namespace jd
{
	JdTaskPool & task_pool ()
	{
		static JdTaskPool pool;
		return pool;
	}
	
	//	template <typename T>
	//	T task (const std::function <T (void)> & i_function)
	//	{
	//		T t {};
	//
	//		auto & pool = task_pool ();
	//
	//		return t;
	//	}
	
	JdTaskBatch batch ()
	{
		auto & pool = task_pool ();
		return pool.CreateBatch ();
	}
	
	void task (const std::function <void (void)> & i_function)
	{
		auto & pool = task_pool ();
		
		pool.m_globalBatch.Add ();
		pool.m_queue.Push ({ & pool.m_globalBatch, i_function });
	}
}
