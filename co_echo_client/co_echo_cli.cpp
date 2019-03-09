/*
 * co_echo_cli.cpp
 *
 *  Created on: Mar 9, 2019
 *      Author: frank
 */
#include <asio/experimental/co_spawn.hpp>
#include <asio/experimental/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <asio/write.hpp>
#include <iostream>

// clang++ -std=c++2a -DASIO_STAND_ALONE -fcoroutines-ts -stdlib=libc++ -I../asio/asio/include

using asio::ip::tcp;
using asio::experimental::co_spawn;
using asio::experimental::detached;
namespace this_coro = asio::experimental::this_coro;

template <typename T>
using awaitable = asio::experimental::awaitable<T, asio::io_context::executor_type>;

awaitable<void> Echo()
{
	auto token = co_await this_coro::token();
	auto executor = co_await this_coro::executor();
	tcp::socket socket(executor.context());
	co_await socket.async_connect({tcp::v4(), 55555}, token); // 异步执行连接服务器
	for(int i = 0; i < 100; ++i) // echo 100次
	{
		char buff[256];
		snprintf(buff, sizeof(buff), "hello %02d", i);
		size_t n = co_await socket.async_send(asio::buffer(buff, strlen(buff)), token); // 异步写数据
		//assert(n == strlen(buff));
		n = co_await socket.async_receive(asio::buffer(buff), token); // 异步读数据
		buff[n] = 0;
		std::cout<<"received from server: "<<buff<<"\n";
	}
}

int main()
{
	asio::io_context ioc(1);
	co_spawn(ioc, Echo, detached);
	ioc.run();
	return 0;
}



