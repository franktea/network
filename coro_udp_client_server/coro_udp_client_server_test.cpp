/*
 * async_udp_client_test.cpp
 *
 *  Created on: Sep 24, 2018
 *      Author: frank
 */
#include <chrono>
#include "udp_client_coro.h"
#include "udp_server_coro.h"

int main()
{
    asio::io_context io_context;
    // server监听的地址
    asio::ip::udp::endpoint ep_server(asio::ip::address::from_string("127.0.0.1"), 9099);

    const int times = 100000; // 收发次数

    // 一个协程，client 发送N次数据
    auto client = std::make_shared<AsyncUdpClient>(io_context, ep_server);    
    auto sending = [client]() -> awaitable<void> {
        for(int i = 0; i < times; ++i) {
            co_await client->SendMessage(std::string("this is message ") + std::to_string(i));
        }
    };
    co_spawn(io_context, sending(), detached);

    // 另一个协程，server 接收N次数据
    auto server = std::make_shared<AsyncUdpServer>(io_context, ep_server);
    auto recving = [server]() -> awaitable<void> {
        for(int i = 0; i < times; ++i) {
            co_await server->RecvMessage();
        }
    };
    co_spawn(io_context, recving(), detached);

    io_context.run();

    return 0;
}

