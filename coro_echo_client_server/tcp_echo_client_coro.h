/*
 * tcp_echo_client_coro.h
 *
 *  Created on: Sep 30, 2022
 *      Author: frank
 */

#ifndef CORO_ECHO_CLIENT_SERVER_TCP_ECHO_CLIENT_CORO_H_
#define CORO_ECHO_CLIENT_SERVER_TCP_ECHO_CLIENT_CORO_H_

#include <iostream>
#include "awaitable_default.h"
#include <string>

// 发送一个字符串，返回接受到的字符串
awaitable<std::string> EchoClient(tcp_socket& socket, std::string send_string) {
    char buffer[4096];
    co_await asio::async_write(socket, asio::buffer(send_string));
    size_t len = co_await socket.async_read_some(asio::buffer(buffer));
    co_return std::string(buffer, len);
}

#endif /* CORO_ECHO_CLIENT_SERVER_TCP_ECHO_CLIENT_CORO_H_ */
