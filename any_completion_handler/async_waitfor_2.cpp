/**
 * @file async_waitfor_2.cpp
 *  implement async wait with any_completion_handler
 * @author frankt could.net@gmail.com
 * @brief 
 * @version 0.1
 * @date 2023-05-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <chrono>
#include <memory>
#include <iostream>
#include "asio.hpp"
#include "asio/any_completion_handler.hpp"
#include "asio/consign.hpp"

using namespace std::literals::chrono_literals;

void async_wait_for_impl(asio::any_completion_handler<void()> handler,
    asio::any_io_executor ex, std::chrono::milliseconds time)
{
    auto timer = std::make_shared<asio::steady_timer>(ex, time);
    //timer->async_wait(asio::consign(std::move(handler), timer));
    // timer.async_wait的handler必须是void(error_code)，不能没有error_code参数，所以上面的代码编不过
    // 下面的lambda不加mutable也编不过
    timer->async_wait([timer, h=std::move(handler)](asio::error_code) mutable { h(); });
}

template<typename Token>
auto async_wait_for(asio::any_io_executor ex, std::chrono::milliseconds time, Token&& token)
{
    return asio::async_initiate<Token, void()>(
        async_wait_for_impl, 
        token,
        ex,
        time
    );
}

asio::awaitable<void> async_sleep(std::chrono::milliseconds time)
{
    std::cout<<"begin sleep\n";
    co_await async_wait_for(co_await asio::this_coro::executor, time, asio::use_awaitable);
    std::cout<<"wake up after "<<time.count()<<" ms\n";
}

int main()
{
    asio::thread_pool pool;
    auto ex = pool.get_executor();
    asio::co_spawn(ex, async_sleep(1000ms), asio::detached);
    pool.join();
}
