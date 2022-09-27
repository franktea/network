/*
 * async_udp_client.h
 *
 *  Created on: Sep 24, 2018
 *      Author: frank
 */

#ifndef ASYNC_UDP_CLIENT_ASYNC_UDP_CLIENT_H_
#define ASYNC_UDP_CLIENT_ASYNC_UDP_CLIENT_H_

#include <string>
#include <iostream>
#include <memory>
//#include "asio.hpp"
#include "awaitable_default.h"

class AsyncUdpClient
{
public:
    AsyncUdpClient(asio::io_context& io_context, asio::ip::udp::endpoint ep)
        : io_context_(io_context)
        , end_point_(ep)
        , socket_(io_context_
                , asio::ip::udp::endpoint(asio::ip::udp::v4()
                , 0))
    {
    }

    awaitable<void> SendMessage(const std::string msg)
    {
        co_await socket_.async_send_to(asio::buffer(&msg[0], msg.size()), end_point_);
        std::cout<<"sent: "<<msg<<"\n";
    }
private:
    asio::io_context& io_context_;
    asio::ip::udp::endpoint end_point_;
    udp_socket socket_;
};

#endif /* ASYNC_UDP_CLIENT_ASYNC_UDP_CLIENT_H_ */
