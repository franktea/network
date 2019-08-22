/*
 * client.cpp
 *
 *  Created on: Jun 21, 2018
 *      Author: frank
 */

#include <iostream>
#include <string>
#include <functional>
#include <memory>
#include <random>
#include <map>
#include "asio.hpp"
#include "package.h"

using namespace std;

// simulate a queue and simulate a situation where the queue may be empty.
class PersudoQueue
{
public:
    // get a pkg, if the queue is empty, return false
    bool Recv(Package& pkg)
    {
        std::uniform_int_distribution<int> d(0, 100);
        if (d(dre_) < 50)
        {
            return false;
        }

        std::uniform_int_distribution<uint32_t> d2(0, MAX_BODY_LEN);
        uint32_t random_len = d2(dre_);
        pkg.header.body_len = random_len;
        return true;
    }
public:
    static PersudoQueue& Instance() // singleton
    {
        static PersudoQueue q;
        return q;
    }
private:
    std::default_random_engine dre_;
    PersudoQueue() = default;
    PersudoQueue(const PersudoQueue&) = delete;
    PersudoQueue& operator=(const PersudoQueue&) = delete;
};

class Session;
using close_callback = std::function<void(std::shared_ptr<Session> ptr, const asio::error_code& err)>;

// a connection to server
class Session: public std::enable_shared_from_this<Session>
{
public:
    Session(int id, asio::io_context& io, const asio::ip::tcp::endpoint ep,
            close_callback cb) :
            id_(id), socket_(io), server_endpoint_(ep), wait_timer_(io), after_close_(
                    cb)
    {
    }
    void Start()
    {
        auto self = shared_from_this();
        socket_.async_connect(server_endpoint_,
                [self, this](const asio::error_code& err)
                {
                    if(err) // connect err
                    {
                        std::cerr<<"connect err:"<<err.message()<<"\n";
                        Close(err);
                        return;
                    }

                    asio::error_code ec;
                    socket_.set_option(asio::socket_base::reuse_address(true), ec);
                    if(ec)
                    {
                        std::cerr<<"client["<<id_<<"] set reuse error, message:"<<ec.message()<<"\n";
                        return;
                    }

                    // receive pkg from queue, and then send it
                    SendQueuePkg();
                });
    }

    void SendQueuePkg()
    {
        auto self = shared_from_this();
        bool has_pkg = PersudoQueue::Instance().Recv(request_);
        if (!has_pkg) // the queue is empty now , sleep for a while
        {
            std::cout << "client[" << id_ << "] sleep...\n";
            wait_timer_.expires_after(std::chrono::milliseconds(100));
            wait_timer_.async_wait(std::bind(&Session::SendQueuePkg, this));
        }
        else // send request to server, and then recv response
        {
            asio::async_write(socket_,
                    asio::buffer(&request_,
                            sizeof(PkgHeader) + request_.header.body_len),
                    [self, this](const asio::error_code& err, size_t len)
                    {
                        if(err) // send error, maybe the server is down
                        {
                            std::cerr<<"send request err:"<<err.message()<<"\n";
                            Close(err);
                            return;
                        }

                        // now recv from server.
                        // recv header first
                        asio::async_read(socket_, asio::buffer(&response_, sizeof(PkgHeader)),
                                [self, this](const asio::error_code& err, size_t len)
                                {
                                    if(err) // read err
                                    {
                                        std::cerr<<"recv header err: "<<err.message()<<"\n";
                                        Close(err);
                                        return;
                                    }

                                    // recv body
                                    if(response_.header.body_len > MAX_BODY_LEN)
                                    {
                                        std::cerr<<"invalid body len: "<<response_.header.body_len<<"\n";
                                        Close(err);
                                        return;
                                    }

                                    asio::async_read(socket_, asio::buffer(&response_.body, response_.header.body_len),
                                            [self, this](const asio::error_code& err, size_t len)
                                            {
                                                if(err)
                                                {
                                                    std::cerr<<"recv body err: "<<err.message()<<"\n";
                                                    Close(err);
                                                    return;
                                                }

                                                // recv response ok, procece the response
                                                std::cout<<"client["<<id_<<"] got response, body len: "<<response_.header.body_len<<"\n";

                                                // now get next pkg from queue
                                                SendQueuePkg();
                                            });
                                });
                    });
        }
    }

    int ID() const
    {
        return id_;
    }

    void Close(const asio::error_code& err)
    {
        socket_.close();
        after_close_(shared_from_this(), err);
    }
private:
    int id_;
//    asio::io_context& io_context_;
    asio::ip::tcp::socket socket_;
    asio::ip::tcp::endpoint server_endpoint_;
    Package request_;
    Package response_;
    asio::steady_timer wait_timer_;
    close_callback after_close_; // when this session is close, maybe a new session should start, do it in this function
};

int main()
{
    asio::io_context io(1);
    asio::ip::tcp::endpoint ep(asio::ip::address::from_string("127.0.0.1"),
            12345);

    // all live clients, save them in a map
    std::map<int, std::shared_ptr<Session>> clients;

    // close callback
    std::function<void(std::shared_ptr<Session>, const asio::error_code&)> f;
    f =
            [&f, &io, &ep, &clients](std::shared_ptr<Session> p, const asio::error_code& err)
            {
                // some client is closed. get the id and create a new one.
                int id = p->ID();
                clients.erase(id);// erase the old one
                std::shared_ptr<Session> new_ptr = std::make_shared<Session>(id, io, ep, f);
                clients.insert(std::make_pair(id, new_ptr));
                new_ptr->Start();
            };

    for (int i = 1; i <= 10000; ++i)
    {
        std::shared_ptr<Session> p = std::make_shared<Session>(i, io, ep, f);
        clients.insert(std::make_pair(i, p));
        p->Start();
    }

    io.run();
    return 0;
}
