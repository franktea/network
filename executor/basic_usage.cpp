/*
 * basic_usage.cpp
 *
 *  Created on: Aug 22, 2019
 *      Author: frank
 */
#include <iostream>
#include <chrono>
#include "asio.hpp"

int main()
{
    // 用法1：直接post
    //asio::post([](){asio::system_executor(), std::cout<<"hello world\n"; });
    asio::post([](){ std::cout<<"hello world\n"; });

    // 用法2：post到thread pool中去
    asio::thread_pool pool;
    asio::post(pool.get_executor(), [](){ std::cout<<"hello from pool\n"; });
    pool.join();

    // 用法3：使用future
    std::future<int> f = asio::post(
            asio::use_future([](){
                return 99;
            }));
    std::cout<<"future get "<<f.get()<<"\n";

    return 0;
}


