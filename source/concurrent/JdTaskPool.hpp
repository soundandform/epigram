//
//  JdTaskPool.hpp
//
//  Created by Steven Massey on 3/4/22.
//  Copyright © 2022 Steven Massey. All rights reserved.
//

#ifndef JdTaskPool_hpp
#define JdTaskPool_hpp


#include "JdThread.hpp"
#include "JdMessageQueue.hpp"

using std::vector;

namespace jd
{
	typedef voidptr_t task_batch_t;
}


struct JdTask
{
	struct Batch
	{
		void  Add  ()
		{
			unique_lock <mutex> lock (m_mutex);
			++m_count;
		}
		
		void  Remove ()
		{
			unique_lock <mutex> lock (m_mutex);
			
			if (--m_count == 0)
				m_condition.notify_one ();
		}
		
		void  WaitForCompletion ()
		{
			unique_lock <mutex> lock (m_mutex);
			
			while (m_count)
				m_condition.wait (lock);
		}
		
		u64								m_count			= 0;
		std::mutex						m_mutex;
		std::condition_variable			m_condition;
	};
	
	Batch *									batch		= nullptr;
	std::function <void (void)>				func;
};


struct JdTaskThread : IJdThread
{
	JdResult                Run             (EpigramRef i_args, IJdThread::Info & i_info) override
	{
		JdResult result;
		
		JdTask task;
		
		while (1)
		{
			m_queue->PopWait (task);
			
			if (not task.batch)
				break;
			
			task.func ();
			task.batch->Remove ();
			++m_numTasksRan;
		}
		
		return result;
	}
	
	JdResult                Teardown        (const JdResult &i_runResult) override
	{
//		jd::out ("thread: @; tasks: @", this, m_tasksRun);
		return i_runResult;
	}
	

	JdMessageQueue <JdTask> *			m_queue			= nullptr;
	u64									m_numTasksRan		= 0;
};


struct JdTaskPool
{
	JdTaskPool ()
	{
		m_globalBatch.m_count = 1;		// so, it never signals the condition
		
//		jd::out ("task pool");
		
		Start ();
	}
	
	virtual ~ JdTaskPool ()
	{
		Stop ();
	}
	
	
	void  Stop  ()
	{
		std::lock_guard <std::mutex> lock (m_mutex);
		
		if (m_threads.size ())
		{
//			jd::out ("stopping task pool");
			
			for (u32 i = 0; i < m_threads.size (); ++i)
			{
				m_queue.Push (JdTask {});
			}
			
			for (auto t : m_threads)
			{
				t->Stop ();
				delete t;
			}
			
			m_threads.clear ();
		}
		
		for (auto batch : m_freeBatches)
			delete batch;
		
		m_freeBatches.clear ();
	}
	
	
	void  Start  ()
	{
		std::lock_guard <std::mutex> lock (m_mutex);
		
		if (m_threads.size () == 0)
		{
			auto numThreads = std::thread::hardware_concurrency ();
			if (numThreads == 0)
				numThreads = 8;
			
			while (numThreads--)
			{
				auto thread = new JdThreadT <JdTaskThread> (Jd::SPrintF ("jd::task @", numThreads + 1));
				m_threads.push_back (thread);
				thread->Get ()->m_queue = & m_queue;
				thread->Start ();
			}
		}
	}
	
	jd::task_batch_t  	CreateBatch  ()
	{
		JdTask::Batch * batch = nullptr;
		
		std::lock_guard <std::mutex> lock (m_mutex);
		
		if (m_freeBatches.size ())
		{
			batch = m_freeBatches.back ();
			m_freeBatches.pop_back ();
		}
		else batch = new JdTask::Batch;
		
//		jd::out ("get batch: @", batch);
		
		return batch;
	}
	
	
	void				FinishBatch  (jd::task_batch_t i_batchId)
	{
		auto batch = (JdTask::Batch *) i_batchId;
		batch->WaitForCompletion ();
		
		std::lock_guard <std::mutex> lock (m_mutex);
		m_freeBatches.push_back (batch);
	}
	
	mutex									m_mutex;
	
	vector <JdThreadT <JdTaskThread> *>		m_threads;
	JdMessageQueue <JdTask>					m_queue					{ 4096 };
	
	vector <JdTask::Batch *>				m_freeBatches;
	
	JdTask::Batch							m_globalBatch;
};

namespace jd
{
	JdTaskPool & task_pool 	();
}


struct JdTaskBatch
{
	JdTaskBatch (bool i_useTaskPool = true)
	{
		if (i_useTaskPool)
		{
			auto & pool = jd::task_pool ();
			m_batch = pool.CreateBatch ();
		}
	}
	
	JdTaskBatch (jd::task_batch_t i_batch)
	:
	m_batch (i_batch) {}
	
	JdTaskBatch (JdTaskBatch && i_other)
	{
		m_batch = i_other.m_batch;
		i_other.m_batch = 0;
	}
	
	~ JdTaskBatch ()
	{
		finish ();
	}
	
	void finish ()
	{
		if (m_batch)
		{
			auto & pool = jd::task_pool ();
			pool.FinishBatch (m_batch);
			m_batch = 0;
		}
	}
	
	void operator () (const std::function <void (void)> & i_function)
	{
		if (m_batch)
		{
			auto & pool = jd::task_pool ();
			
			auto batch = (JdTask::Batch *) m_batch;
			batch->Add ();
			pool.m_queue.Push ({ batch, i_function });
		}
		else i_function ();
	}

	
	operator jd::task_batch_t () const
	{
		return m_batch;
	}
	
	jd::task_batch_t 	m_batch		= 0;
};


namespace jd
{
	JdTaskBatch 	batch 	();
	void			task	(const std::function <void (void)> & i_function);
}



#endif /* JdTaskPool_hpp */
