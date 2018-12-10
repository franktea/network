/*
 * custom_timer_mgr.cpp
 *
 *  Created on: Jul 19, 2018
 *      Author: frank
 */
#include <iostream>
#include <chrono>
#include "asio.hpp"
#include "timer_mgr2.h"

int main()
{
	asio::io_context io;
	asio::steady_timer t(io, std::chrono::seconds(10));
	t.async_wait([](const asio::error_code&) { std::cout<<"fire after 10 sec.\n"; } );
	asio::steady_timer t2(io, std::chrono::seconds(5));
	t2.async_wait([](const asio::error_code&) { std::cout<<"fire after 5 sec.\n"; } );
	io.run();
	return 0;
}



