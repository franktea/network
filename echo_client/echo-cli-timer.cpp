/*
 * echo_cli.cpp
 *
 *  Created on: Nov 6, 2018
 *      Author: frank
 */
#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include "asio.hpp"

using namespace std;
using asio::ip::tcp;

//clang++ -std=c++14 -DASIO_STANDALONE -I../asio/asio/include echo-cli-timer.cpp -o ech-cli-timer

class Session : public std::enable_shared_from_this<Session>
{
public:
	Session(asio::io_context& ioc, tcp::endpoint ep):socket_(ioc), timer_(ioc)
	{
		socket_.async_connect(ep, [this](const asio::error_code& err) {
			if(err) {
				cout<<"connect err:"<<err.message()<<"\n";
				return;
			}
			SendAndRecv();
		});
	}
private:
	void SendAndRecv();
	void OnTimeOut(const asio::error_code& err)
	{
		if(err != asio::error::operation_aborted)
		{
			std::cout<<"time out, session will be ended\n";
			asio::error_code ignore;
			socket_.close(ignore);
		}
		else
		{
			std::cout<<"timer canceled\n";
		}
	}
private:
	tcp::socket socket_;
	char buff_[1024];
	asio::steady_timer timer_;
};

int main(int argc, const char* argv[])
{
	asio::io_context ioc;
	tcp::endpoint ep(asio::ip::address::from_string("127.0.0.1"), std::stoi(argv[1]));
	auto p = std::make_shared<Session>(ioc, ep);
	ioc.run();
	return 0;
}

inline void Session::SendAndRecv() {
	auto p = shared_from_this();
	timer_.expires_after(std::chrono::milliseconds(1));
	timer_.async_wait([p, this](const asio::error_code& err){
        OnTimeOut(err);
	});
	static int i = 0;
	++i;
	if(i > 10) return;
	snprintf(buff_, sizeof(buff_), "this is message %d", i);
	asio::async_write(socket_, asio::buffer(buff_, strlen(buff_)),
			[p, this](const asio::error_code& err, size_t len) {
		if(err) return;
		timer_.expires_after(std::chrono::milliseconds(1));
		timer_.async_wait([p, this](const asio::error_code& err){
	        OnTimeOut(err);
		});
		socket_.async_read_some(asio::buffer(buff_, sizeof(buff_)),
				[p, this](const asio::error_code& err, size_t len){
			if(err) return;
			cout<<"recved: "<<std::string(buff_, len)<<"\n";
			SendAndRecv();
		});
	});
}
