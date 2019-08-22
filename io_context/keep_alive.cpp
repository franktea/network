/*
 * keep_alive.cpp
 *  prevent io_context::run from returning
 *  Created on: Nov 1, 2018
 *      Author: frank
 */
#include <iostream>
#include "asio.hpp"

// 防止io_context退出
int main()
{
    asio::io_context ioc;

    // 方法1，创建一个executor_work_guard对象，注意生命周期
    //auto wg = asio::make_work_guard(ioc);

    // 方法2，用一个定时器，等待最长时间
    //asio::steady_timer t(ioc);
    //t.expires_after(asio::steady_timer::clock_type::duration::max());
    //t.async_wait([](const std::error_code&){});

    // 方法3，等待信号
    asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](const std::error_code&, int signal)
    {
        std::cout<<"signal "<<signal<<"\n";
        ioc.stop();
    });

    ioc.run();
    return 0;
}

