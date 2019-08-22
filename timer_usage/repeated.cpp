/*
 * example.cpp
 *
 *  Created on: Nov 1, 2018
 *      Author: frank
 */
#include <functional>
#include <iostream>
#include "asio.hpp"

using namespace std::placeholders;

void print(const asio::error_code& ec, asio::steady_timer* t)
{
    std::cout << "time out...\n";
    t->expires_at(t->expiry() + std::chrono::seconds(1));
    t->async_wait(std::bind(print, _1, t)); // add again
}

int main()
{
    asio::io_context ioc;
    asio::steady_timer t(ioc, std::chrono::seconds(1));
    t.async_wait(std::bind(print, _1, &t));
    ioc.run();
    return 0;
}

