#pragma once
/*
 * udp_server_coro.h
 *
 *  Created on: 2022-09-27
 *      Author: frank
 */

#include <string_view>
#include <iostream>
#include <memory>
//#include "asio.hpp"
#include "awaitable_default.h"

class AsyncUdpServer
{
public:
    AsyncUdpServer(io_context& ioc, udp::endpoint ep)
        : io_context_(ioc)
        , socket_(io_context_, ep) {}
    
    awaitable<void> RecvMessage()
    {
        char buff[4096];
        udp::endpoint remote_addr;
        size_t len = co_await socket_.async_receive_from(asio::buffer(buff), remote_addr);
        std::cout<<"received: "<<std::string_view(buff, len)<<"\n";
    }
private:
    io_context& io_context_;
    udp_socket socket_;
};
