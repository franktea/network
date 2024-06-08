#include <iostream>
#include <memory>
#include <utility>
#include <vector>
#include <ctime>
#include <string_view>
#include "asio.hpp"

using asio::ip::tcp;

asio::awaitable<void> GetTime(asio::io_context& io_context, int port) {
    tcp::socket socket(io_context);

    co_await socket.async_connect(tcp::endpoint(tcp::v4(), port), asio::use_awaitable);

    char buff[4096];
    size_t len = co_await socket.async_read_some(asio::buffer(buff), asio::use_awaitable);
    std::cout<< "time is " << std::string_view(buff, len) << "\n";
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        std::cout << "usage: " << argv[0] << " port\n";
        return -1;
    }

    asio::io_context io_context;

    asio::co_spawn(io_context, GetTime(io_context, std::atoi(argv[1])), asio::detached);

    io_context.run();
}
