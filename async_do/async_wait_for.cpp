/**
 * @file async_wait_for.cpp
 * @author frank
 * @brief 
 * @version 0.1
 * @date 2023-01-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <iostream>
#include <chrono>
#include <map>
#include <functional>
#include <memory>
#include "asio.hpp"

// 用map来用作优先队列，因为map本来就是排序的。
using QueueType = std::map<std::chrono::milliseconds, std::function<void()>>;

template<asio::completion_handler_for<void()> CompletionToken>
auto async_wait_for(std::chrono::milliseconds time,
    QueueType& queue,
    CompletionToken&& token)
{
    return asio::async_initiate<CompletionToken, void()>(
        [](auto completion_handler, std::chrono::milliseconds time, QueueType& queue) {
            std::function<void()> f = [handler = std::move(completion_handler)](){
                handler();
            };
            auto now = std::chrono::steady_clock::now();
            auto timeout = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch() + time;
            queue.insert(std::make_pair(timeout, f));
        },
        token, time, std::ref(queue)
    );
}

int main() {
    QueueType queue;
    using namespace std::chrono_literals;
    std::cout<<"wait for 1000ms\n";
    std::function<void()> f = [](){ std::cout<<"hello world\n"; };
    async_wait_for(1000ms, queue, f);
    asio::thread_pool pool;
    asio::post(pool, [&queue](){
        while(!queue.empty()) {
            auto now = std::chrono::steady_clock::now();
            auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch();
            if(queue.begin()->first <= now_ms) {
                queue.begin()->second();
                queue.erase(queue.begin());
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });
    pool.join();
}