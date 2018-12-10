/*
    one shot redis client. connect to server, send cmd, recv response, and then disconnect.
    the client should be destroyed after each time.
                      frank Jul 1, 2018
*/
#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <map>
#include "asio.hpp"
#include "response_parser.h"

using namespace std;
using std::placeholders::_1;
using std::placeholders::_2;

// one shot async client, exe one cmd, get the result, then destroy
class OneShotClient: public std::enable_shared_from_this<OneShotClient>
{
public:
	OneShotClient(asio::io_context& io, asio::ip::tcp::endpoint ep): ep_(ep), socket_(io) {}

	void ExecuteCmd(const string& cmd)
	{
		send_buff_ = cmd; // copy and save
		socket_.async_connect(ep_, [this](const asio::error_code& err) {
			if(err)
			{
				cout<<"connect err: "<<err.message()<<"\n";
				return;
			}

			asio::async_write(socket_, asio::buffer(send_buff_), [this](const asio::error_code& err, size_t len) {
				if(err)
				{
					cout<<"send err: "<<err.message()<<"\n";
					return;
				}

				socket_.async_read_some(asio::buffer(recv_buff_), std::bind(&OneShotClient::ReceiveResponse, this, _1, _2));
			});
		});
	}
private:
	void ReceiveResponse(const asio::error_code& err, size_t len)
	{
		if(err)
		{
			cout<<"recv err: "<<err.message()<<"\n";
			return;
		}

	    //cout<<"got result: "<<string(recv_buff_, len)<<"\n";

		for(char* p = recv_buff_; p < recv_buff_ + len; ++p)
		{
			if(! parse_item_)
			{
				parse_item_ = AbstractReplyItem::CreateItem(*p);
				continue;
			}

			ParseResult pr = parse_item_->Feed(*p);
			if(pr == ParseResult::PR_FINISHED)
			{
				cout<<"get parse result: "<<parse_item_->ToString()<<"\n";
				return;
			}
			else if(pr == ParseResult::PR_ERROR)
			{
				cout<<"parse error at "<<*p<<", pos="<<(p-recv_buff_)<<"\n";
				return;
			}
		}

		// not finished, recv again
		socket_.async_read_some(asio::buffer(recv_buff_), std::bind(&OneShotClient::ReceiveResponse, this, _1, _2));
	}
private:
//	asio::io_context& io_;
	asio::ip::tcp::endpoint ep_; // server address
	asio::ip::tcp::socket socket_;
	string send_buff_;
	char recv_buff_[1024];
	std::shared_ptr<AbstractReplyItem> parse_item_;
};

int main()
{
	asio::io_context io;
	asio::ip::tcp::endpoint ep(asio::ip::address::from_string("127.0.0.1"), 6379);

	auto p = std::make_shared<OneShotClient>(io, ep);
	//p->ExecuteCmd("*3\r\n$3\r\nset\r\n$3\r\naaa\r\n$3\r\nbbb\r\n");
	//p->ExecuteCmd("*2\r\n$3\r\nget\r\n$3\r\naaa\r\n");
	p->ExecuteCmd("*2\r\n$4\r\nkeys\r\n$1\r\n*\r\n");

	io.run();
	return 0;
}


