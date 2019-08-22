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
#include "asio.hpp"

class AsyncUdpClient: public std::enable_shared_from_this<AsyncUdpClient>
{
public:
    AsyncUdpClient(asio::io_context& io_context, asio::ip::udp::endpoint ep) :
            io_context_(io_context), end_point_(ep), socket_(io_context,
                    asio::ip::udp::endpoint(asio::ip::udp::v4(), 0))
    {
    }

    void SendMessage(const std::string msg)
    {
        io_context_.restart();
        auto p = shared_from_this();
        std::shared_ptr<std::string> str(new std::string(msg));
        socket_.async_send_to(asio::buffer(&(*str)[0], str->size()), end_point_,
                [p, str](const asio::error_code& ec,
                        std::size_t bytes_transferred)
                {
                    std::cout<<"sent: "<<*str<<"\n";
                });
    }
private:
    asio::io_context& io_context_;
    asio::ip::udp::endpoint end_point_;
    asio::ip::udp::socket socket_;
};

#endif /* ASYNC_UDP_CLIENT_ASYNC_UDP_CLIENT_H_ */
