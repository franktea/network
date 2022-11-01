/*
 * server.cpp
 *
 *  Created on: Jun 21, 2018
 *      Author: frank
 */

#include <iostream>
#include <string>
#include <functional>
#include <memory>
#include "asio.hpp"
#include "package.h"
#include "awaitable_default.h"

using namespace std;

class Server;

// a connect from client
class Session: public std::enable_shared_from_this<Session>
{
    friend class Server;
public:
    Session(asio::io_context& io_context) : //io_context_(io_context),
            socket_(io_context)
    {
    }

    awaitable<void> Start()
    {
        auto self = shared_from_this();

        try {
            for(;;) {
                // recv package header, since length of header is fixed
                size_t n = co_await asio::async_read(socket_, asio::buffer(&request_, sizeof(PkgHeader)));

                // check if request body len is valid
                if(request_.header.body_len > MAX_BODY_LEN) {
                    std::cerr<<"invalid body len: "<<request_.header.body_len<<"\n";
                    socket_.close();
                    co_return;
                }

                // recv body
                n = co_await asio::async_read(socket_, asio::buffer(&request_.body, request_.header.body_len));
                // receive ok, now process request and send response to client

                std::cout<<"recved pkg, body len="<<n<<"\n";
                response_ = request_;

                // send response
                co_await asio::async_write(socket_, asio::buffer(&response_, sizeof(PkgHeader) + response_.header.body_len));
            }
        } catch (std::exception& ex) {
            std::cerr<<"socket error: "<<ex.what()<<"\n";
            socket_.close();
        }
    }
private:
//    asio::io_context& io_context_;
    tcp_socket socket_;
    Package request_;
    Package response_;
};

class Server
{
public:
    Server(asio::io_context& ioc, const asio::ip::tcp::endpoint ep) :
            io_context_(ioc), acceptor_(ioc, ep)
    {
    }

    awaitable<void> Start()
    {
        try {
            for(;;) {
                auto p = std::make_shared<Session>(io_context_);
                co_await acceptor_.async_accept(p->socket_);

                std::cout<<"new conn, from ip:"<<p->socket_.remote_endpoint().address().to_string()<<"\n";

                asio::error_code ec;
                p->socket_.set_option(asio::socket_base::reuse_address(true), ec);
                if(ec)
                {
                    std::cerr<<"set reuse error, message:"<<ec.message()<<"\n";
                    co_return;
                }

                auto f = [p]()->awaitable<void>{ co_await p->Start(); };
                asio::co_spawn(io_context_, f(), asio::detached);
            }
        } catch (std::exception& ex) {
            // 当打开的文件数超过系统限制时会运行到此处，意味着listen会停止、server在
            // 处理完所有的socket事件以后也会退出。
            // 为了能让server一直正常运行不退出，必须要保证允许建立的最大连接数小于系统的限制。
            std::cerr<<"acceptor error: "<<ex.what()<<"\n";
            co_return;
        }
    }
private:
    asio::io_context& io_context_;
    tcp_acceptor acceptor_;
};

int main()
{
    asio::io_context io(1);
    asio::ip::tcp::endpoint ep(asio::ip::address::from_string("127.0.0.1"),
            12345);
    Server server(io, ep);
    auto f = [&server]()->awaitable<void>{ co_await server.Start(); };
    asio::co_spawn(io, f(), asio::detached);
    asio::error_code ec;
    io.run(ec);
    if (ec)
    {
        std::cerr << "run() met an error, msg:" << ec.message() << "\n";
    }
    return 0;
}

