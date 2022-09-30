/*
 * tcp_echo_server_coro.h
 *
 *  Created on: Sep 30, 2022
 *      Author: frank
 */

#ifndef CORO_ECHO_CLIENT_SERVER_TCP_ECHO_SERVER_CORO_H_
#define CORO_ECHO_CLIENT_SERVER_TCP_ECHO_SERVER_CORO_H_

#include <string>
#include "awaitable_default.h"

awaitable<void> Echo(tcp_socket socket) {
    char buffer[4096];
    for(;;) {
        size_t n = co_await socket.async_read_some(asio::buffer(buffer));
        co_await asio::async_write(socket, asio::buffer(buffer, n));
    }
}

awaitable<void> Listen(asio::io_context& io_context, tcp::endpoint server_addr) {
    tcp_acceptor acceptor(io_context, server_addr);
    for(;;) {
        tcp::socket socket = co_await acceptor.async_accept();
        asio::co_spawn(io_context, Echo(std::move(socket)), detached);
    }
}

#endif /* CORO_ECHO_CLIENT_SERVER_TCP_ECHO_SERVER_CORO_H_ */
