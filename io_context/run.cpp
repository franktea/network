/*
 * run.cpp
 *  测试调用多次run
 *  Created on: Nov 13, 2018
 *      Author: frank
 */
#include <iostream>
#include "asio.hpp"

int main()
{
    asio::io_context ioc;

    asio::steady_timer t(ioc);
    t.expires_after(std::chrono::milliseconds(10));
    t.async_wait([](const std::error_code&){ std::cout<<"timeout 1\n"; });
    ioc.run();

    std::cout<<"here\n";

    ioc.post([](){ std::cout<<"this is a callback\n";});
    asio::steady_timer t2(ioc);
    t2.expires_after(std::chrono::milliseconds(100));
    t2.async_wait([](const std::error_code&){ std::cout<<"timeout 2\n"; });
    //ioc.restart(); // 必须先调用restart再调用run或者poll，才会执行
    //ioc.run();
    //ioc.poll_one();

    return 0;
}



