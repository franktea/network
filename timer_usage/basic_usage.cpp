/*
 * basic_usage.cpp
 *
 *  Created on: Nov 1, 2018
 *      Author: frank
 */
#include <functional>
#include <iostream>
#include "asio.hpp"

int main()
{
    asio::io_context ioc;
    asio::steady_timer timer1(ioc, std::chrono::seconds(1));
    auto f = [](const std::error_code& ec)
    {
        if(!ec) // asio::error::operation_aborted
            std::cout<<"timer1 timeout\n";
        };
    timer1.async_wait(f);

    asio::steady_timer timer2(ioc);
    timer2.expires_after(std::chrono::milliseconds(100));
    timer2.async_wait([&timer1](const std::error_code&)
    {
        timer1.cancel();
        std::cout<<"timer2 timeout\n";
    });
    ioc.run();
    return 0;
}

