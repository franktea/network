/*
 * async_udp_client_test.cpp
 *
 *  Created on: Sep 24, 2018
 *      Author: frank
 */
#include <chrono>
#include "async_udp_client.h"

int main()
{
    asio::io_context io_context;
    asio::ip::udp::endpoint ep(asio::ip::address::from_string("127.0.0.1"),
            9099);
    auto client = std::make_shared<AsyncUdpClient>(io_context, ep);
    for (int i = 0; i < 100000; ++i)
    {
        client->SendMessage(
                std::string("this is message ") + std::to_string(i));
    }

    for (int i = 0; i < 10000000; ++i)
    {
        io_context.restart();
        io_context.run_for(std::chrono::microseconds(100));
    }
    return 0;
}

