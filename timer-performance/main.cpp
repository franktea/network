/*
 * add tons of timer to see the performance of timers in asio.
 frank Jun 5, 2018
 */

#include <iostream>
#include <functional>
#include "asio.hpp"

using namespace std;
using namespace asio;

static int total = 0;

void timeout(const asio::error_code& ec)
{
    ++total;
}

int main()
{
    asio::io_context ioc(1);
    for (int i = 0; i < 1000000; ++i)
    {
        auto t = std::make_shared<asio::steady_timer>(ioc,
                asio::chrono::milliseconds(1));
        t->async_wait(timeout);
    }
    ioc.run();
    cout << total << "\n";
    return 0;
}
