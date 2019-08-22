/*
 * co_echo_cli.cpp
 *
 *  Created on: Mar 9, 2019
 *      Author: frank
 */
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <asio/write.hpp>
#include <iostream>

// clang++ -std=c++2a -DASIO_STAND_ALONE -fcoroutines-ts -stdlib=libc++ -I../asio/asio/include

using asio::ip::tcp;
using asio::co_spawn;
using asio::detached;
using asio::awaitable;
using asio::use_awaitable;
namespace this_coro = asio::this_coro;

awaitable<void> Echo()
{
    auto executor = co_await
    this_coro::executor;
    tcp::socket socket(executor);
    co_await socket
    .async_connect(
            {   tcp::v4(), 55555}, use_awaitable); // 异步执行连接服务器
    for (int i = 0; i < 100; ++i) // echo 100次
    {
        char buff[256];
        snprintf(buff, sizeof(buff), "hello %02d", i);
        size_t n = co_await
        socket.async_send(asio::buffer(buff, strlen(buff)), use_awaitable); // 异步写数据
        //assert(n == strlen(buff));
        n = co_await
        socket.async_receive(asio::buffer(buff), use_awaitable); // 异步读数据
        buff[n] = 0;
        std::cout << "received from server: " << buff << "\n";
    }
}

int main()
{
    asio::io_context ioc(1);
    co_spawn(ioc, Echo, detached);
    ioc.run();
    return 0;
}

