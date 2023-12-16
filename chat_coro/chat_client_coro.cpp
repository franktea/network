#include "chat_message.hpp"
#include "awaitable_default.h"

using chat_message_queue = std::deque<chat_message>;

class chat_client {
public:
    chat_client(asio::io_context& io_context,
        const tcp::resolver::results_type& ep) :
        io_context_(io_context),
        socket_(io_context) {

    }

    void close() {
        asio::post(io_context_, [this]() { socket_.close(); });
    }
private:
    asio::io_context& io_context_;
    tcp_socket socket_;
    chat_message read_msg_;
    chat_message_queue write_msgs_; // 发送的是用户输入的内容，用户可能输入很快，所以也需要一个队列保存起来
};

int main(int argc, char* argv[]) {

}