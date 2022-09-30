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

// 希望client和server在echo一定次数以后双双退出，
// client退出很容易，但是server是一只在监听的，
// 这时候只需要使用asio中对协程重载的运算符||，
// 当客户端退出时，server也会退出。
awaitable<void> EchoForTimes(asio::io_context& io_context, tcp::endpoint ep) {
	auto client = [&io_context, ep]() -> awaitable<void> {
		tcp_socket socket(io_context);
		co_await ConnectTo(socket, ep);
		for(int i = 0; i < 10; ++i) {
			std::string s = std::string("this is message ") + std::to_string(i);
			std::string ret = co_await EchoClient(socket, s);
			std::cout<<"recved: "<<ret<<"\n";
		}
	};

	co_await (client() || Listen(io_context, ep));
}

int main() {
	asio::io_context io_context(1);
	tcp::endpoint ep{tcp::v4(), 10001};

	asio::co_spawn(io_context, EchoForTimes(io_context, ep), detached);

	io_context.run();
}


