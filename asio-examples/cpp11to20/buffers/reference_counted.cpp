

#include <iostream>
#include <memory>
#include <utility>
#include <vector>
#include <ctime>
#include "asio.hpp"

using asio::ip::tcp;

// a reference-counted non-modifiable buffer class
class SharedConstBuffer {
public:
    // construct from a std::string
    explicit SharedConstBuffer(const std::string& data) :
        data_(std::make_shared<std::vector<char>>(data.begin(), data.end())),
        buffer_(asio::buffer(*data_))
    {
    }

    // implement the ConstBufferSequence requirements
    typedef asio::const_buffer value_type;
    typedef const asio::const_buffer* const_iterator;
    const asio::const_buffer* begin() const { return &buffer_; }
    const asio::const_buffer* end() const { return &buffer_ + 1; }
private:
    std::shared_ptr<std::vector<char>> data_;
    asio::const_buffer buffer_;
};

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket) : socket_(std::move(socket)) {}

    asio::awaitable<void> Start() {
        std::time_t now = std::time(0);
        SharedConstBuffer buffer(std::ctime(&now));

        auto self(shared_from_this());
        co_await asio::async_write(socket_, buffer, asio::use_awaitable);
    }
private:
    tcp::socket socket_;
};

class Server {
public:
    Server(asio::io_context& io_context, short port):
        acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        
    }

    asio::awaitable<void> Start() {
        for(;;) {
            tcp::socket socket = co_await acceptor_.async_accept(asio::use_awaitable);
            auto session = std::make_shared<Session>(std::move(socket));
            asio::co_spawn(acceptor_.get_executor(), session->Start(), asio::detached);
        }
    }
private:
    tcp::acceptor acceptor_;
};

int main(int argc, char* argv[]) {
    try {
        if(argc != 2) {
            std::cerr << "usage: refrence_counted <port>\n";
            return 1;
        }

        asio::io_context io_context;

        Server server(io_context, std::atoi(argv[1]));
        std::cout<<"listening on port " << std::atoi(argv[1]) << "\n";
        asio::co_spawn(io_context, server.Start(), asio::detached);

        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "exception: " << e.what() << "\n";
    }
}
