/**
 * @file async_do.cpp
 * @author frank
 * @brief 
 * @version 0.1
 * @date 2023-01-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <iostream>
#include <string>
#include "asio.hpp"

using namespace asio;

// 通过模拟通过一个key异步计算一个value，来演示如何实现async_xxx。
// 通过key计算value的过程被放在一个thread_pool中，避免阻塞io_context
template<asio::completion_token_for<void(std::string)> CompletionToken>
auto async_query_value(asio::thread_pool& pool, asio::io_context& ioc, std::string key, CompletionToken&& token)
{
    return asio::async_initiate<CompletionToken, void(std::string)>(
        [&](auto completion_handler, std::string const key) {
            auto io_eq = asio::prefer(ioc.get_executor(), asio::execution::outstanding_work.tracked);
            asio::post(pool, [key, io_eq = std::move(io_eq), completion_handler = std::move(completion_handler)]() mutable {
                std::string value = std::string("value for ") + key;
                asio::post(io_eq, [value = std::move(value), completion_handler = std::move(completion_handler)]() mutable {
                    completion_handler(value);
                });
            });
        },
        token, key
    );
}

// 测试用coroutine的方式来调用async_xxx
asio::awaitable<void> test_coroutine(asio::thread_pool& pool, asio::io_context& ioc)
{
    std::string key = "key001";
    std::string value = co_await async_query_value(pool, ioc, key, asio::use_awaitable);
    std::cout<<"got value: "<<value<<"\n";
}

int main() {
    asio::thread_pool pool;
    asio::io_context ioc;
    asio::co_spawn(ioc, test_coroutine(pool, ioc), asio::detached);
    ioc.run();
    pool.join();
}
