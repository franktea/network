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

int main() {
	asio::io_context io_context;
	tcp::endpoint ep{tcp::v4(), 10001};

	asio::co_spawn(io_context, Listen(io_context, ep), detached);

	asio::co_spawn(io_context, [&io_context, ep]() -> awaitable<void> {
		tcp_socket socket(io_context);
		co_await ConnectTo(socket, ep);
		for(int i = 0; i < 10; ++i) {
			std::string s = std::string("this is message ") + std::to_string(i);
			std::string ret = co_await EchoClient(socket, s);
			std::cout<<"recved: "<<ret<<"\n";
		}
	}, detached);

	io_context.run();
}


