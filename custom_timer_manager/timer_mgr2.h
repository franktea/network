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

namespace asio
{

template <>
struct wait_traits<asio::chrono::steady_clock>
{
    using Clock = asio::chrono::steady_clock;

    static typename Clock::duration to_wait_duration(
        const typename Clock::duration &d)
    {
        return d;
    }

    static typename Clock::duration to_wait_duration(
        const typename Clock::time_point &t)
    {
        typename Clock::time_point now = Clock::now();
        if (now + (Clock::duration::max)() < t)
            return (Clock::duration::max)();
        if (now + (Clock::duration::min)() > t)
            return (Clock::duration::min)();
        return t - now;
    }
};

} // namespace asio
#endif /* TIMER_PERFORMANCE_TIMER_MGR2_H_ */
