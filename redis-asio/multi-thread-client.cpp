/*
 client.cpp
 frank Jun 20, 2018
 */
#include <iostream>
#include <functional>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <deque>
#include <asio.hpp>
#include "input_composer.h"
#include "response_parser.h"

using namespace std;
using namespace asio;

// asio async read from console, see :
// https://www.boost.org/doc/libs/1_67_0/doc/html/boost_asio/example/cpp03/chat/posix_chat_client.cpp

class RedisClient: public std::enable_shared_from_this<RedisClient>
{
public:
    RedisClient(asio::io_context& io, asio::ip::tcp::endpoint server_endpoint) :
            io_context_(io), socket_(io), server_endpoint_(server_endpoint)
    {
    }

    void Start()
    {
        auto self = shared_from_this();
        socket_.async_connect(server_endpoint_,
                [self, this](const asio::error_code& err)
                {
                    if(err)
                    {
                        std::cout<<"failed to connect\n";
                        return;
                    }

                    //std::cout<<"connected, is_open: "<<socket_.is_open()<<"\n";

                    // read data from server
                    auto self = shared_from_this();
                    socket_.async_receive(asio::buffer(recv_buffer_), std::bind(&RedisClient::RecvFromServer, this, std::placeholders::_1, std::placeholders::_2));
                });
    }

    void RecvFromServer(const asio::error_code& err, size_t len)
    {
        if (err)
        {
            std::cout << "recv error: " << err.message() << "\n";
            socket_.close();
            return;
        }

        for (char* p = recv_buffer_; p < recv_buffer_ + len; ++p)
        {
            if (!parse_item_)
            {
                parse_item_ = AbstractReplyItem::CreateItem(*p);
                continue;
            }

            ParseResult pr = parse_item_->Feed(*p);
            if (pr == ParseResult::PR_FINISHED)
            {
                cout << parse_item_->ToString() << "\n";
                cout << server_endpoint_.address().to_string() << ":"
                        << server_endpoint_.port() << "> " << std::flush;
                parse_item_.reset();
            }
            else if (pr == ParseResult::PR_ERROR)
            {
                cout << "parse error at " << *p << ", pos="
                        << (p - recv_buffer_) << "\n";
                cout << server_endpoint_.address().to_string() << ":"
                        << server_endpoint_.port() << "> " << std::flush;
                parse_item_.reset();
            }
        }

        // continue receiving data
        socket_.async_receive(asio::buffer(recv_buffer_),
                std::bind(&RedisClient::RecvFromServer, this,
                        std::placeholders::_1, std::placeholders::_2));
    }

    void Send(const string& line)
    {
        //std::cout<<"send thread:"<<std::this_thread::get_id()<<"\n";
        auto self = shared_from_this();
        asio::post(io_context_, [self, this, line]()
        {
            //std::cout<<"process line thread:"<<std::this_thread::get_id()<<"\n";
                requests_.push_back(line);
                SendQueue();
            });
    }

    void SendQueue()
    {
        //std::cout<<"sendqueue thread:"<<std::this_thread::get_id()<<"\n";
        auto self = shared_from_this();
        if (!requests_.empty())
        {
            send_buffer_ = requests_.front();
            requests_.pop_front();
            asio::async_write(socket_,
                    asio::buffer(&send_buffer_[0], send_buffer_.length()),
                    [this, self](const asio::error_code& err, std::size_t bytes_transferred)
                    {
                        if(err)
                        {
                            socket_.close();
                            return;
                        }

                        asio::post(io_context_, [self, this]()
                                {
                                    SendQueue();
                                });
                    });
        }
    }
private:
    std::string send_buffer_; // the content send to server
    asio::io_context& io_context_;
    asio::ip::tcp::socket socket_;
    asio::ip::tcp::endpoint server_endpoint_;
    std::deque<string> requests_;
    char recv_buffer_[1024];
    std::shared_ptr<AbstractReplyItem> parse_item_;
};

int main()
{
    asio::io_context io_context;
    asio::ip::tcp::endpoint server_endpoint(
            asio::ip::address::from_string("127.0.0.1"), 6379);
    std::shared_ptr<RedisClient> client = std::make_shared<RedisClient>(
            io_context, server_endpoint);
    client->Start();
    std::thread t([&io_context]()
    {
        io_context.run();
    });

    //std::cout<<"main thread:"<<std::this_thread::get_id()<<"\n";

    cout << server_endpoint.address().to_string() << ":"
            << server_endpoint.port() << "> " << std::flush;
    std::string line;
    while (std::getline(std::cin, line))
    {
        std::string bulk_string = InputComposer::ComposeInputToBulk(line);
        client->Send(bulk_string);
    }

    t.join();
    return 0;
}
