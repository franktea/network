#include <cstdlib>
#include <deque>
#include <iostream>
#include <memory>
#include <set>
#include <list>
#include <utility>
#include "asio.hpp"
#include "chat_message.hpp"
#include "awaitable_default.h"

using chat_message_queue = std::deque<chat_message>;

class chat_participant {
public:
    virtual ~chat_participant() {}
    virtual void deliver(const chat_message& msg) = 0;
};

using chat_participant_ptr = std::shared_ptr<chat_participant>;

// 聊天室，每个聊天室对应多名玩家，每个玩家发送的信息都会广播给其它全部玩家
class chat_room {
public:
    void join(chat_participant_ptr participant) {
        participants_.insert(participant);
        // 刚加入聊天室的玩家，将聊天室最近的历史消息全部发给他
        for(auto& msg: recent_msgs_) {
            participant->deliver(msg);
        }
    }

    // 玩家离开了聊天室
    void leave(chat_participant_ptr participant) {
        participants_.erase(participant);
    }

    // 向聊天室的所有玩家广播消息
    void deliver(const chat_message& msg) {
        recent_msgs_.push_back(msg);
        while(recent_msgs_.size() > max_recent_msgs) {
            recent_msgs_.pop_front();
        }

        // 广播
        for(auto p: participants_) {
            p->deliver(msg);
        }
    }
private:
    std::set<chat_participant_ptr> participants_;
    const int max_recent_msgs = 100;
    chat_message_queue recent_msgs_; // 每个room记录一个最近的消息列表，这个列表主要发送给刚加入room的人
};

class chat_session : public chat_participant,
    public std::enable_shared_from_this<chat_session> {
public:
    chat_session(tcp_socket socket, chat_room& room) : socket_(std::move(socket)), room_(room) {
    }

    void start() {
        room_.join(shared_from_this());
        // 在协程中读取消息
        asio::co_spawn(socket_.get_executor(), read_msg(), asio::detached);
    }

    // 读取消息，并放入当前聊天室的队列中以待广播
    asio::awaitable<void> read_msg() {
        auto self = shared_from_this();
        while(1) {
            // 读取消息，因为消息头部长度固定为4字节，只要先读4字节即可
            co_await asio::async_read(socket_, asio::buffer(read_msg_.data(), read_msg_.header_length));
            if(! read_msg_.decode_header()) { // 解析长度
                std::cerr<<"decode header error, maybe msg too long.\n";
                co_return;
            }
            // 读取到长度以后，再读取对应长度的消息内容
            co_await asio::async_read(socket_, asio::buffer(read_msg_.body(), read_msg_.body_length()));

            // 消息读取完了，放到chat_room里面去广播
            room_.deliver(read_msg_);
        }
    }

    // 发送消息，将队列中的消息逐条发送
    asio::awaitable<void> send_mesgs() {
        auto self = shared_from_this();
        // 只要write_msgs_不为空，这个协程就一定在运行
        while(! write_msgs_.empty()) {
            // 利用write_msgs_.front()来当buffer，只要没发送完，队列一定不为空
            co_await asio::async_write(socket_, asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()));
            write_msgs_.pop_front(); // 发送完了才pop出来
        }
    }

    // 向当前玩家发送一条消息，实现是将该消息放入到消息队列中，按顺序发送，
    // 因为之前可能有消息没发送完
    void deliver(const chat_message& msg) {
        bool write_in_progress = ! write_msgs_.empty();
        write_msgs_.push_back(msg);
        if(! write_in_progress) {
            // 之前队列为空，发送协程一定没有在运行，开启一个发送协程
            asio::co_spawn(socket_.get_executor(), send_mesgs(), asio::detached);
        }
    }
private:
    tcp_socket socket_;
    chat_room& room_;
    chat_message read_msg_; // 需要读取消息缓存，只需要一条
    chat_message_queue write_msgs_; // 需要发送的消息队列
};

class chat_server {
public:
    chat_server(asio::io_context& io_context, const tcp::endpoint& ep) : acceptor_(io_context, ep) {
        asio::co_spawn(io_context, do_accept(), asio::detached);
    }
private:
    asio::awaitable<void> do_accept() {
        while(1) {
            tcp_socket socket = co_await acceptor_.async_accept();
            std::make_shared<chat_session>(std::move(socket), room_)->start();
        }
    }

    tcp_acceptor acceptor_;
    chat_room room_; // 这个例子中是每个server一个chat room，当然也可以按需分配
};

int main(int argc, char* argv[]) {
    if(argc < 2) {
        std::cerr << "usage: chat_server_coro port1 port2 ... portn" << "\n";
        return 1;
    }

    asio::io_context io_context;

    std::list<chat_server> servers;
    for(int i = 1; i < argc; ++i) {
        tcp::endpoint ep(tcp::v4(), std::atoi(argv[i]));
        servers.emplace_back(io_context, ep);
    }

    io_context.run();
}