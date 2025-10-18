/*
 * tcp_echo_server_coro.h
 *
 *  Created on: Sep 30, 2022
 *      Author: frank
 */

#ifndef CORO_ECHO_CLIENT_SERVER_TCP_ECHO_SERVER_CORO_H_
#define CORO_ECHO_CLIENT_SERVER_TCP_ECHO_SERVER_CORO_H_

#include <string_view>
#include <iostream>
#include "awaitable_default.h"

awaitable<void> Echo(tcp_socket socket) {
    char buffer[4096];
    for(;;) {
        size_t n = co_await socket.async_read_some(asio::buffer(buffer));
        std::cout<<"server got: "<<std::string_view(buffer, n)<<"\n";
        co_await asio::async_write(socket, asio::buffer(buffer, n));
    }
}

awaitable<void> Listen(tcp::endpoint server_addr) {
    auto executor = co_await this_coro::executor;
    tcp_acceptor acceptor(executor, server_addr);
    for(;;) {
        tcp::socket socket = co_await acceptor.async_accept();
        asio::co_spawn(executor, Echo(std::move(socket)), detached);
    }
}

#endif /* CORO_ECHO_CLIENT_SERVER_TCP_ECHO_SERVER_CORO_H_ */
