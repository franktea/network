/*
 * tcp_echo_client_server.cpp
 *
 *  Created on: Sep 30, 2022
 *      Author: frank
 */

#include <iostream>
#include <string>
#include "tcp_echo_client_coro.h"
#include "tcp_echo_server_coro.h"

// client和server都要使用，为了简化传参定义成全局变量先
tcp::endpoint ep{tcp::v4(), 10001};

// 希望client和server在echo一定次数以后双双退出，
// client退出很容易，但是server是一只在监听的，
// 这时候只需要使用asio中对协程重载的运算符||，
// 当客户端退出时，server也会退出。
awaitable<void> Echo() {
    auto client = []() -> awaitable<void> {
        tcp_socket socket(co_await this_coro::executor);
        co_await socket.async_connect(ep);
        for(int i = 0; i < 10; ++i) {
            std::string s = std::string("this is message ") + std::to_string(i);
            std::string ret = co_await EchoClient(socket, s);
            std::cout<<"client got: "<<ret<<"\n";
        }
    };

    // 其实将Listen放在前面也并不能100%保证server先启动，但是经过测试都ok，这样写并不严谨，
    // 但是这不是这个例子演示的重点。
    co_await (Listen(ep) || client());
}

int main() {
    asio::io_context io_context(1);
    asio::co_spawn(io_context, Echo(), detached);
    io_context.run();
}


