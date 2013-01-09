/*
 * =====================================================================================
 *
 *       Filename:  threadpool.h
 *
 *    Description:  Generic thread pool
 *
 *        Version:  1.0
 *        Created:  01/09/2013 12:00:30 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ayush Dubey (), dubey@cs.cornell.edu
 *   Organization:  Cornell University
 *
 * =====================================================================================
 */

#ifndef __THREADPOOL__
#define __THREADPOOL__

//C++
#include <vector>
#include <deque>
#include <thread>

//po6
#include <po6/threads/mutex.h>
#include <po6/threads/cond.h>

//Weaver
#include "../graph.h"
#include "../../message/message.h"

namespace db
{
namespace thread
{
	class pool;
	void thread_loop (pool *tpool);

	class unstarted_thread
	{
		public:
			unstarted_thread (
				void (*f) (db::graph*, std::shared_ptr<message::message>),
				db::graph *g,
				std::shared_ptr<message::message> m);

		public:
			void (*func) (db::graph*, std::shared_ptr<message::message>);
			db::graph *G;
			std::shared_ptr<message::message> msg;
	};

	inline
	unstarted_thread :: unstarted_thread ( 
			void (*f) (db::graph*, std::shared_ptr<message::message>),
			db::graph *g,
			std::shared_ptr<message::message> m)
		: func (f)
		, G (g)
		, msg (m)
	{
	}

	class pool
	{
		public:
			pool (int n_threads);

		public:
			int num_threads;
			std::deque<unstarted_thread> queue;
			std::vector<std::thread> threads;
			po6::threads::mutex queue_mutex;
			po6::threads::cond empty_queue_cond;
			void thread_loop1 ();
		
		public:
			void add_request (unstarted_thread t);
	};

	inline
	pool :: pool (int n_threads)
		: num_threads (n_threads)
		, empty_queue_cond (&queue_mutex)
	{
		int i;
		std::unique_ptr<std::thread> t;
		for (i = 0; i < num_threads; i++)
		{
			t.reset (new std::thread (thread_loop, this));
			t->detach();
		}
	}

	void
	pool :: thread_loop1 ()
	{
		unstarted_thread thr (NULL, NULL, NULL);
		while (true)
		{
			queue_mutex.lock();
			while (queue.empty())
			{
				empty_queue_cond.wait();
			}
			thr = queue.front();
			queue.pop_front();
			if (!queue.empty())
			{
				empty_queue_cond.signal();
			}
			queue_mutex.unlock();
			(*thr.func) (thr.G, thr.msg);
		}
	}

	inline void
	pool :: add_request (unstarted_thread t)
	{
		queue_mutex.lock();
		if (queue.empty())
		{
			empty_queue_cond.signal();
		}
		queue.push_back (t);
		queue_mutex.unlock();
	}

	void
	thread_loop (pool *tpool)
	{
		unstarted_thread thr (NULL, NULL, NULL);
		while (true)
		{
			tpool->queue_mutex.lock();
			while (tpool->queue.empty())
			{
				tpool->empty_queue_cond.wait();
			}
			thr = tpool->queue.front();
			tpool->queue.pop_front();
			if (!tpool->queue.empty())
			{
				tpool->empty_queue_cond.signal();
			}
			tpool->queue_mutex.unlock();
			(*thr.func) (thr.G, thr.msg);
		}
	}
} //namespace thread
} //namespace db

#endif //__THREADPOOL__
