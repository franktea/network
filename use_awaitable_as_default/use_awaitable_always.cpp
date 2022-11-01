// example of how to use asio::use_awaitable as default.
// 2023-06-07

#include <iostream>
#include <asio.hpp>
#include "awaitable_default.h"

awaitable<void> Echo(tcp_socket stream) {
    try {
        for(;;) {
            char buff[512];
            size_t n = co_await stream.async_read_some(buffer(buff)); // 不需要显式的use_awaitable了
            co_await async_write(stream, buffer(buff, n));
        }
    } catch (std::exception& ex) {
        std::cout<<"exception: "<<ex.what()<<"\n";
    }
}

awaitable<void> Listen(asio::io_context& ioc, asio::ip::tcp::endpoint ep) {
    tcp_acceptor acceptor(ioc, ep);
    for(;;) {
        try {
            tcp_socket socket(ioc);
            co_await acceptor.async_accept(socket);
            co_spawn(ioc, Echo(std::move(socket)), asio::detached);
        } catch (std::exception& ex) {
            std::cout<<"listen exception: "<<ex.what()<<"\n";
        }
    }
}

int main() {
    asio::io_context ioc(1);
    asio::ip::tcp::endpoint ep {asio::ip::address::from_string("127.0.0.1"), 5555};
    co_spawn(ioc, Listen(ioc, ep), asio::detached);
    ioc.run();
}
