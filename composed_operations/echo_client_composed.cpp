#include <iostream>
#include <asio.hpp>
#include <string>
#include <string_view>
#include <chrono>
#include <asio/experimental/co_composed.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include "../coro_echo_client_server/tcp_echo_server_coro.h"

using asio::async_connect;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
using asio::deferred;
using asio::experimental::co_composed;
using asio::ip::tcp;
namespace this_coro = asio::this_coro;

using namespace asio::experimental::awaitable_operators;
using namespace std::chrono_literals;

// 短连接，发送几条消息以后断开
template<typename CompletionToken>
auto async_echo(asio::io_context& ioc, 
    const std::string message,
    const std::string server,
    const std::string port,
    CompletionToken&& token)
{
    return asio::async_initiate<CompletionToken, void(std::error_code)> (
        co_composed<void(std::error_code)>(
            [message, server, port](auto state, asio::io_context& ioc) -> void {
                try {
                    state.throw_if_cancelled(true);
                    state.reset_cancellation_state(asio::enable_terminal_cancellation());

                    // initialize a socket
                    tcp::socket s(ioc);

                    // resolve host and port
                    tcp::resolver resolver(ioc);
                    tcp::resolver::query q{server, port};
                    tcp::resolver::iterator ep = co_await resolver.async_resolve(q, asio::deferred);

                    // connect to the server
                    co_await s.async_connect(*ep, asio::deferred);

                    // send message
                    co_await asio::async_write(s, asio::buffer(message), asio::deferred);

                    // receive message from server
                    std::array<char, 1024> buff;
                    const auto bytes = co_await s.async_receive(asio::buffer(buff), asio::deferred);
                    std::cout<<"client received "<<bytes<<" bytes: "<<std::string_view(&buff[0], bytes)<<"\n";

                    co_return state.complete(std::error_code{});

                } catch (const std::error_code& e) {
                    co_return {e};
                }
            }),
        token, std::ref(ioc));
}

int main() {
    try {
        asio::io_context io(1);

        co_spawn(io, [&io]()->asio::awaitable<void> {
                // 先启动server
                tcp::endpoint ep{tcp::v4(), 1111};
                co_await Listen(ep);
            }, asio::detached);

        co_spawn(io,
            [&io]()->asio::awaitable<void> {
                // 先sleep一下，等待server启动成功。
                asio::steady_timer timer(io);
                timer.expires_after(100ms);
                co_await timer.async_wait(asio::use_awaitable);

                // 启动几个client
                co_await ( async_echo(io, "c++20 coro message 1", "127.0.0.1", "1111", use_awaitable)
                 && async_echo(io, "c++20 coro message 2", "127.0.0.1", "1111", use_awaitable) );

                // 消息收完了，退出io_context
                io.post([&io](){ io.stop(); });
            }, asio::detached);
        
        io.run();
    } catch(std::exception& e) {
        std::cout<<"echo exception: "<<e.what()<<"\n";
    }
}
