#include <iostream>
#include "chat_message.hpp"
#include "awaitable_default.h"

using chat_message_queue = std::deque<chat_message>;

class chat_client {
public:
    chat_client(asio::io_context& io_context,
        const tcp::resolver::results_type& ep) :
        io_context_(io_context),
        socket_(io_context) {
        // 启动读取消息的协程
        asio::co_spawn(io_context_, connect_and_read(ep), asio::detached);
    }

    void write(const chat_message& msg) {
        asio::post(io_context_, [this,msg](){
            bool write_in_process = ! write_msgs_.empty();
            write_msgs_.push_back(msg);
            if(!write_in_process) {
                asio::co_spawn(io_context_, send_mesgs(), asio::detached);
            }
        });
    }

    void close() {
        asio::post(io_context_, [this]() { socket_.close(); });
    }
private:
    asio::awaitable<void> connect_and_read(const tcp::resolver::results_type& ep) {
        co_await asio::async_connect(socket_, ep);

        // 读取来自服务器的信息
        while(1) {
            co_await asio::async_read(socket_, asio::buffer(read_msg_.data(), read_msg_.header_length));
            co_await asio::async_read(socket_, asio::buffer(read_msg_.body(), read_msg_.body_length()));
            std::cout.write(read_msg_.body(), read_msg_.body_length());
            std::cout<<"\n";
        }
    }

    // 发送消息，将队列中的消息逐条发送
    asio::awaitable<void> send_mesgs() {
        // 只要write_msgs_不为空，这个协程就一定在运行
        while(! write_msgs_.empty()) {
            // 利用write_msgs_.front()来当buffer，只要没发送完，队列一定不为空
            co_await asio::async_write(socket_, asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()));
            write_msgs_.pop_front(); // 发送完了才pop出来
        }
    }

    asio::io_context& io_context_;
    tcp_socket socket_;
    chat_message read_msg_;
    chat_message_queue write_msgs_; // 发送的是用户输入的内容，用户可能输入很快，所以也需要一个队列保存起来
};

int main(int argc, char* argv[]) {
    if(argc != 3) {
        std::cerr << "usage: chat_client_coro <host> <port>\n";
        return 1;
    }

    asio::io_context io_context;

    tcp_resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[1], argv[2]);
    chat_client c(io_context, endpoints);

    std::thread t([&io_context](){ io_context.run(); });

    char line[chat_message::max_body_length + 1];
    while(std::cin.getline(line, chat_message::max_body_length + 1)) {
        chat_message msg;
        msg.body_length(std::strlen(line));
        std::memcpy(msg.body(), line, msg.body_length());
        msg.encode_header();
        c.write(msg);
    }

    c.close();
    t.join();
}
