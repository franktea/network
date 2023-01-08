/**
 * @file thread_switch.cpp
 * @author frank
 * @brief 
 * @version 0.1
 * @date 2023-01-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <iostream>
#include <thread>
#include "asio.hpp"

inline void cout2(const auto& msg)
{
    static std::mutex mutex;
    std::lock_guard lg(mutex);
    std::cout<<"in thread "<<std::this_thread::get_id()<<": "<<msg<<"\n";
}

asio::awaitable<void> main_coro(asio::executor exec_main,
    asio::executor exec_work)
{
    auto switch_to_work = asio::bind_executor(asio::make_strand(exec_work), asio::use_awaitable);
    auto switch_to_main = asio::bind_executor(asio::make_strand(exec_main), asio::use_awaitable);

    cout2("now in main thread");
    co_await asio::post(switch_to_work);
    cout2("in work thread");
    co_await asio::post(switch_to_main);
    cout2("in main thread");
    co_await asio::post(switch_to_work);
    cout2("in work thread");
    co_await asio::post(switch_to_work);
    cout2("in work thread");
    co_await asio::post(switch_to_main);
    cout2("in main thread");
}

int main()
{
    asio::io_context io_main, io_work;
    auto work_guard = asio::make_work_guard(io_main);

    std::thread work_thread([&io_main]{
        cout2("work thread start running");
        io_main.run();
        cout2("work thread stopped");
    });

    asio::co_spawn(io_work, main_coro(io_work.get_executor(), io_main.get_executor()), asio::detached);

    cout2("main thread run start");
    io_work.run();
    cout2("main thread stopped");

    work_guard.reset();
    work_thread.join();
}

