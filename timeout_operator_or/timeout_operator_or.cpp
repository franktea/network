/**
 * @file timeout_operator_or.cpp
 * @author frankt (could.net@gmail.com)
 * @brief  from: https://stackoverflow.com/questions/76073157/using-co-simultaneously-in-a-c20-environment-await-async-read-and-async/76076816#76076816
 * @version 0.1
 * @date 2023-06-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <iostream>
#include <set>
#include <string_view>
#include "asio.hpp"
#include "asio/experimental/as_tuple.hpp"
#include "asio/experimental/awaitable_operators.hpp"

using asio::experimental::as_tuple;
using namespace asio::experimental::awaitable_operators;
using namespace std::literals::chrono_literals;

// helper to reduced frequncy logging
void once_in(size_t n, auto&& action)
{
    static std::atomic_size_t counter_ = 0;
    if((++counter_ % n) == 0)
    {
        if constexpr(std::is_invocable_v<decltype(action), size_t>)
        {
            std::move(action)(counter_);
        } else {
            std::move(action)();
        }
    }
}

asio::awaitable<void> timeout(auto duration)
{
    asio::steady_timer timer(co_await asio::this_coro::executor);
    timer.expires_after(duration);
    co_await timer.async_wait(as_tuple(asio::use_awaitable));
}

asio::awaitable<void> server_session(asio::ip::tcp::socket socket)
{
    try {
        for(std::array<char, 2000> data;;)
        {
            if(auto r = co_await (asio::async_read(socket, asio::buffer(data), as_tuple(asio::use_awaitable)) || timeout(2ms));
                r.index() == 1)
            {
                once_in(1000, [&]{ std::cout<< "server session time out.\n"; });
            } else {
                auto [e, n] = std::get<0>(r);
                if(! e) {
                    once_in(1000, [&, n] {
                        std::cout << "server session writing " << n << " bytes to "
                            << socket.remote_endpoint() << "\n";
                    });
                    if(n) {
                        co_await asio::async_write(socket, asio::buffer(data, n), asio::use_awaitable);
                    }
                } else {
                    std::cout << "error: " << e.message() << "\n";
                }
            }
        }
    } catch(std::system_error const& e) {
        std::cout << "server session exception: " <<e.code().message() << "\n";
    } catch(std::exception const& e) {
        std::cout << "server session exception: " << e.what() << "\n";
    }

    std::cout << "server session closed\n";
}

asio::awaitable<void> listener(uint16_t port)
{
    for(asio::ip::tcp::acceptor acceptor(co_await asio::this_coro::executor, {{}, port});;)
    {
        auto strand = asio::make_strand(acceptor.get_executor());
        asio::co_spawn(strand, server_session(co_await acceptor.async_accept(strand, asio::use_awaitable)), asio::detached);
    }
}

asio::awaitable<void> client_session(uint16_t port)
{
    try {
        asio::ip::tcp::socket socket(co_await asio::this_coro::executor);
        co_await socket.async_connect({{}, port}, asio::use_awaitable);

        for(std::array<char, 4024> data{0};;)
        {
            co_await ( asio::async_read(socket, asio::buffer(data), asio::use_awaitable) || timeout(2ms) );
            auto w = co_await asio::async_write(socket, asio::buffer(data, 2000), asio::use_awaitable);

            once_in(1000, [&](size_t counter){
                std::cout << "#" << counter << " wrote " << w << " bytes from " << socket.local_endpoint() << "\n";
            });
        }
    } catch(std::system_error const& e) {
        std::cout << "client session exception: " << e.code().message() << "\n";
    } catch(std::exception const& e) {
        std::cout << "client session exception: " << e.what() << "\n";
    }

    std::cout << "client session closed\n";
}

int main(int argc, char** argv)
{
    auto flags = std::set<std::string_view>(argv + 1, argv + argc);
    bool server = flags.contains("server");
    bool client = flags.contains("client");

    asio::thread_pool pool(server ? 8 : 3);
    try {
        asio::signal_set signals(pool, SIGINT, SIGTERM);
        signals.async_wait([&pool](auto, auto){ pool.stop(); });

        if(server) {
            asio::co_spawn(pool, listener(5555), asio::detached);
        }

        if(client) {
            asio::co_spawn(asio::make_strand(pool), client_session(5555), asio::detached);
            asio::co_spawn(asio::make_strand(pool), client_session(5555), asio::detached);
            asio::co_spawn(asio::make_strand(pool), client_session(5555), asio::detached);
        }

        asio::co_spawn(pool, timeout(30s), asio::use_future).wait();
    } catch (std::exception const& e) {
        std::cout << "main exception: " << e.what() << "\n";
    }
}
