/*
 * timer_mgr2.h
 *
 *  Created on: Jul 19, 2018
 *      Author: frank
 */

#ifndef TIMER_PERFORMANCE_TIMER_MGR2_H_
#define TIMER_PERFORMANCE_TIMER_MGR2_H_

//#include "asio/detail/config.hpp"
#include <cstddef>
#include <map>
#include <iostream>
#include <ctime>
#include <chrono>
#include "asio/detail/cstdint.hpp"
#include "asio/detail/date_time_fwd.hpp"
//#include "asio/detail/limits.hpp"
#include "asio/detail/op_queue.hpp"
#include "asio/detail/timer_queue_base.hpp"
//#include "asio/detail/wait_op.hpp"
#include "asio/error.hpp"

//#include "asio/detail/push_options.hpp"


namespace asio {
namespace detail {

/**
 * demonstraits how to specialize a timer queue.
 * use std::map to manage all timers.
 */
template <>
class timer_queue<detail::chrono_time_traits<asio::chrono::steady_clock, asio::wait_traits<asio::chrono::steady_clock>>>
  : public timer_queue_base
{
public:
	using Time_Traits = detail::chrono_time_traits<asio::chrono::steady_clock, asio::wait_traits<asio::chrono::steady_clock>>;

	// The time type.
	typedef typename Time_Traits::time_type time_type;

	// The duration type.
	typedef typename Time_Traits::duration_type duration_type;

	// Per-timer data.
	class per_timer_data
	{
	public:
		per_timer_data()
		{
		}

	private:
		friend class timer_queue;

		// The operations waiting on the timer.
		op_queue<wait_op> op_queue_;

		time_type time;
	};

	timer_queue()
	{
		std::cout<<"======>custom timer queue constructed.\n";
	}

	  // Whether there are no timers in the queue.
	virtual bool empty() const override
	{
		return timers_.empty();
	}

	// Get the time to wait until the next timer.
	virtual long wait_duration_msec(long max_duration) const override
	{
		if(timers_.empty())
			return max_duration;

	    return this->to_msec(
	        Time_Traits::to_posix_duration(
	          Time_Traits::subtract(timers_.begin()->first, Time_Traits::now())),
	        max_duration);
	}

	// Get the time to wait until the next timer.
	virtual long wait_duration_usec(long max_duration) const override
	{
		if(timers_.empty())
			return max_duration;

	    return this->to_usec(
	        Time_Traits::to_posix_duration(
	          Time_Traits::subtract(timers_.begin()->first, Time_Traits::now())),
	        max_duration);
	}

	// Dequeue all ready timers.
	virtual void get_ready_timers(op_queue<operation>& ops) override
	{
		if(timers_.empty())
			return;

		const time_type now = Time_Traits::now();
		do
		{
			auto it = timers_.begin();
			if(Time_Traits::less_than(now, it->first))
			{
				break;
			}

			ops.push(it->second->op_queue_);

			timers_.erase(it);
		} while(! timers_.empty());
	}

	// Dequeue all timers.
	virtual void get_all_timers(op_queue<operation>& ops) override
	{
		while(! timers_.empty())
		{
			auto it = timers_.begin();
			ops.push(it->second->op_queue_);
			timers_.erase(it);
		}
	}

	// Add a new timer to the queue. Returns true if this is the timer that is
	// earliest in the queue, in which case the reactor's event demultiplexing
	// function call may need to be interrupted and restarted.
	bool enqueue_timer(const time_type& time, per_timer_data& timer,
			wait_op* op)
	{
		auto it = timers_.find(time);
		if(it == timers_.end())
		{
			timer.time = time;
			timers_.insert(std::make_pair(time, &timer));
			timer.op_queue_.push(op);
		}
		else
		{
			it->second->op_queue_.push(op);
		}

		std::cout<<"insert a timer which will fire after "<<std::chrono::duration<double>(time - std::chrono::steady_clock::now()).count() <<" seconds\n";

		return time == timers_.begin()->first;
	}

	// Cancel and dequeue operations for the given timer.
	std::size_t cancel_timer(per_timer_data& timer, op_queue<operation>& ops,
			std::size_t max_cancelled =
					(std::numeric_limits<std::size_t>::max)())
	{
		std::size_t num_cancelled = 0;
		while (wait_op* op =
				(num_cancelled != max_cancelled) ? timer.op_queue_.front() : 0)
		{
			op->ec_ = asio::error::operation_aborted;
			timer.op_queue_.pop();
			ops.push(op);
			++num_cancelled;
		}

		if (timer.op_queue_.empty())
		{
			timers_.erase(timer.time);
		}

		return num_cancelled;
	}

	// Move operations from one timer to another, empty timer.
	void move_timer(per_timer_data& target, per_timer_data& source)
	{
		//TODO: this method is for move constructor, just implement as needed
	}
private:
	// Helper function to convert a duration into milliseconds.
	template<typename Duration>
	long to_msec(const Duration& d, long max_duration) const
	{
		if (d.ticks() <= 0)
			return 0;
		int64_t msec = d.total_milliseconds();
		if (msec == 0)
			return 1;
		if (msec > max_duration)
			return max_duration;
		return static_cast<long>(msec);
	}

	// Helper function to convert a duration into microseconds.
	template<typename Duration>
	long to_usec(const Duration& d, long max_duration) const
	{
		if (d.ticks() <= 0)
			return 0;
		int64_t usec = d.total_microseconds();
		if (usec == 0)
			return 1;
		if (usec > max_duration)
			return max_duration;
		return static_cast<long>(usec);
	}

	std::map<time_type, per_timer_data*> timers_;
};

} //end namespace detail
} //end namespace asio



#endif /* TIMER_PERFORMANCE_TIMER_MGR2_H_ */
