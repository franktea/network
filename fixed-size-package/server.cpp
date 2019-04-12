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

using namespace std;

class Server;

// a connect from client
class Session: public std::enable_shared_from_this<Session>
{
    friend class Server;
public:
    Session(asio::io_context& io_context): //io_context_(io_context),
        socket_(io_context) {}
    void Start()
    {
        auto self = shared_from_this();
        // recv from client
        asio::async_read(socket_, asio::buffer(&request_, sizeof(PkgHeader)),
                [this, self](const asio::error_code& err, size_t len) {
            if(err)
            {
                std::cerr<<"recv header err:"<<err.message()<<"\n";
                socket_.close();
                return;
            }

            // check if request body len is valid
            if(request_.header.body_len > MAX_BODY_LEN)
            {
                std::cerr<<"body len too large: "<<request_.header.body_len<<"\n";
                socket_.close();
                return;
            }

            // recv body
            asio::async_read(socket_, asio::buffer(&request_.body, request_.header.body_len),
                    [self, this](const asio::error_code& err, size_t len) {
                if(err)
                {
                    std::cerr<<"recv body err:"<<err.message()<<"\n";
                    socket_.close();
                    return;
                }

                //receive ok, now process and send response to client
                std::cout<<"recv pkg, body length: "<<request_.header.body_len<<"\n";
                response_ = request_;
                asio::async_write(socket_, asio::buffer(&response_, response_.header.body_len + sizeof(PkgHeader)),
                        [self, this](const asio::error_code& err, size_t len) {
                    if(err)
                    {
                        std::cerr<<"send err: "<<err.message()<<"\n";
                        socket_.close();
                        return;
                    }

                    // send ok, just read next package
                    Start();
                });
            });
        });
    }
private:
//    asio::io_context& io_context_;
    asio::ip::tcp::socket socket_;
    Package request_;
    Package response_;
};

class Server
{
public:
    Server(asio::io_context& ioc, const asio::ip::tcp::endpoint ep) : io_context_(ioc), acceptor_(ioc, ep) {};
    void Start()
    {
        auto p = std::make_shared<Session>(io_context_);
        acceptor_.async_accept(p->socket_, [p, this](const asio::error_code& err)
        {
            if(! err)
            {
                std::cout<<"new conn, from ip:"<<p->socket_.remote_endpoint().address().to_string()<<"\n";

                asio::error_code ec;
                p->socket_.set_option(asio::socket_base::reuse_address(true), ec);
                if(ec)
                {
                    std::cerr<<"set reuse error, message:"<<ec.message()<<"\n";
                    return;
                }

                p->Start();
                Start(); // listen for the next connection
            }
            else
            {
                std::cerr<<"accept error, msg:"<<err.message()<<"\n";
            }
        });
    }
private:
    asio::io_context& io_context_;
    asio::ip::tcp::acceptor acceptor_;
};

int main()
{
    asio::io_context io(1);
    asio::ip::tcp::endpoint ep(asio::ip::address::from_string("127.0.0.1"), 12345);
    Server server(io, ep);
    server.Start();
    asio::error_code ec;
    io.run(ec);
    if(ec)
    {
        std::cerr<<"run() met an error, msg:"<<ec.message()<<"\n";
    }
    return 0;
}


