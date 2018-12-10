/*
    sock5 daemon, simple sock5 server.

                      frank May 28, 2018
*/
#include <iostream>
#include <asio.hpp>
#include <string>
#include <memory>
#include <functional>

using namespace std;
using namespace asio;
using std::placeholders::_1;
using std::placeholders::_2;

class Sock5Session : public std::enable_shared_from_this<Sock5Session>
{
public:
	explicit Sock5Session(asio::io_context& ioc): io_context_(ioc), local_socket_(ioc), dest_socket_(ioc),
		resolver_(ioc)
	{
	}
public:
	void Start()
	{
		local_socket_.async_receive(asio::buffer(local_buffer_), std::bind(&Sock5Session::HandleLocalConnect,
				shared_from_this(), _1, _2));
	}

	asio::ip::tcp::socket& LocalSocket()
	{
		return local_socket_;
	}
private:
	void HandleLocalConnect(const asio::error_code& err, std::size_t bytes_transferred) // recv 05 01 00 from client
	{
		if(err)
		{
			cout<<"recv 05 01 00 err: "<<err.message()<<"\n";
			return;
		}

		//cout<<"recv 05 01 00, bytes_transferred="<<bytes_transferred<<"\n";

		if(bytes_transferred < 3)
		{
			cout<<"recv 05 01 00 not enough, bytes_transferred = "<<bytes_transferred<<"\n";
			return;
		}

		if(local_buffer_[0] != 0x05 || local_buffer_[1] != 0x01 || local_buffer_[2] != 0x00)
		{
			cout<<"recv 05 01 00 content error "<<int(local_buffer_[0])<<", "<<int(local_buffer_[1])<<", "<<int(local_buffer_[1])<<"\n";
			return;
		}

		//cout<<"recv 05 01 00 content ok\n";
		// send 0x05 00 to client
		local_buffer_[1] = 0x00;
		asio::async_write(local_socket_, asio::buffer(&local_buffer_[0], 2), std::bind(&Sock5Session::HandleConnectWrite,
				shared_from_this(), _1, _2));
	}

	void HandleConnectWrite(const asio::error_code& err, std::size_t bytes_transferred) // after send 05 00 to client
	{
		if(err)
		{
			cout<<"send 05 00 to client err: "<<err.message()<<"\n";
			return;
		}

		//read command from client, only connect here. should be like 05 01 00 01/03 ...
		local_socket_.async_receive(asio::buffer(local_buffer_), std::bind(&Sock5Session::HandleConnectCmd,
				shared_from_this(), _1, _2));
	}

	void HandleConnectCmd(const asio::error_code& err, std::size_t bytes_transferred)
	{
		if(err)
		{
			cout<<"recv cmd err: "<<err.message()<<"\n";
			return;
		}

		// expect to be 05 01 00 01/03 ...
		if(bytes_transferred < 5)
		{
			cout<<"recv 05 01 00 01/03 not enough, bytes_transferred = "<<bytes_transferred<<"\n";
			return;
		}

		if(local_buffer_[0] != 0x05 || local_buffer_[2] != 0x00)
		{
			cout<<"byte 0 and 2 error\n";
			return;
		}

		if(local_buffer_[1] != 0x01)// now only 0x01 is supported
		{
			cout<<"unsupported cmd : "<<int(local_buffer_[1])<<"\n";
			return;
		}

		if(local_buffer_[1] == 0x01) // connect as ipv4 address
		{
			if(local_buffer_[3] == 0x01) // 05 01 00 01 ip[4] port[2]
			{
				if(bytes_transferred != 10)
				{
					cout<<"connect with ip length error, not 10, but "<<bytes_transferred<<"\n";
					return;
				}

				uint32_t int_ip = *(uint32_t*)(&local_buffer_[4]);
				dest_host_ = asio::ip::address_v4(ntohl(int_ip)).to_string();
				uint16_t int_port = *(uint16_t*)(&local_buffer_[8]);
				dest_port_ = std::to_string(ntohs(int_port));
				cout<<"connect with ip[4]="<<dest_host_<<", port="<<dest_port_<<"\n";
			}
			else if(local_buffer_[3] == 0x03) // 05 01 00 03 host_len host[host_len] port[2]
			{
				uint8_t host_len = local_buffer_[4];
				if(bytes_transferred != 7 + host_len)
				{
					cout<<"connect with hostname length error, not "<<7+host_len<<", but "<<bytes_transferred<<"\n";
					return;
				}

				dest_host_ = std::string(&local_buffer_[5], host_len);
				uint16_t int_port = *(uint16_t*)(&local_buffer_[bytes_transferred-2]);
				dest_port_ = std::to_string(ntohs(int_port));

				cout<<"connect with host="<<dest_host_<<", port="<<dest_port_<<"\n";
			}

			auto self = shared_from_this();
			// resolve the domain
			asio::ip::tcp::resolver::query q(dest_host_, dest_port_);
			resolver_.async_resolve(q, [self, this](const asio::error_code& err, asio::ip::tcp::resolver::results_type results) {
					if(err)
					{
						cout<<"resolve error: "<<err.message()<<"\n";
						return;
					}

					dest_socket_.async_connect(*results.begin(), [self, this](const asio::error_code& err) {
						if(err)
						{
							cout<<"connect to dest err: "<<err.message()<<"\n";
							return;
						}

						cout<<"connected to host="<<dest_host_<<", port="<<dest_port_<<", now read data\n";

						//sock5 handshake finished, send response to client
						char resp[] = {0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
						memcpy(local_buffer_, resp, sizeof(resp));
						asio::async_write(local_socket_, asio::buffer(local_buffer_, sizeof(resp)), [this, self](const asio::error_code& err, size_t bytes_transferred) {
							if(err)
							{
								cout<<"handshake resp error: "<<err.message()<<"\n";
								return;
							}

							// connected to dest server, now receive data and transfer
							ReadFromLocal();
							ReadFromDest();
						});
					});
				});
		}
		//cout<<"recv connect cmd, host="<<dest_host_<<", port="<<dest_port_<<"\n";
	}

	void ReadFromLocal()
	{
		auto self = shared_from_this();
		local_socket_.async_read_some(asio::buffer(local_buffer_), [self, this](const asio::error_code& err, size_t bytes_transferred) {
				if(! err)
				{
					cout<<"recved from local, length: "<<bytes_transferred<<"\n";
					asio::async_write(dest_socket_, asio::buffer(local_buffer_, bytes_transferred), [self, this](const asio::error_code& err, std::size_t bytes_transferred) {
						if(! err)
						{
							ReadFromLocal();
						}
						else
						{
							cout<<"send to dest error: "<<err.message()<<"\n";
							local_socket_.close();
							dest_socket_.close();
						}
					});
				}
				else
				{
					cout<<"receive from local error: "<<err.message()<<"\n";
					local_socket_.close();
					dest_socket_.close();
				}
			});
	}

	void ReadFromDest()
	{
		auto self = shared_from_this();
		dest_socket_.async_read_some(asio::buffer(dest_buffer_), [self, this](const asio::error_code& err, size_t bytes_transferred) {
				if(! err)
				{
					cout<<"recved from dest, length: "<<bytes_transferred<<"\n";
					asio::async_write(local_socket_, asio::buffer(dest_buffer_, bytes_transferred), [self, this](const asio::error_code& err, std::size_t bytes_transferred) {
						if(! err)
						{
							ReadFromDest();
						}
						else
						{
							cout<<"send to dest error: "<<err.message()<<"\n";
							local_socket_.close();
							dest_socket_.close();
						}
					});
				}
				else
				{
					cout<<"receive from local error: "<<err.message()<<"\n";
					local_socket_.close();
					dest_socket_.close();
				}
			});
	}
private:
	asio::io_context& io_context_;
	asio::ip::tcp::socket local_socket_;
	asio::ip::tcp::socket dest_socket_;
	asio::ip::tcp::resolver resolver_;
private:
	char local_buffer_[1024]; // buffer for local_socket_ to read
	char dest_buffer_[1024]; // buffer for dest socket_ to read
private:
	string dest_host_;
	string dest_port_;
};

using session_ptr = std::shared_ptr<Sock5Session>;

class Sock5Server
{
public:
	explicit Sock5Server(asio::io_context& ioc, const asio::ip::tcp::endpoint& ep): io_context_(ioc), acceptor_(ioc, ep) {}
public:
	void Start()
	{
		session_ptr p = std::make_shared<Sock5Session>(io_context_);
		acceptor_.async_accept(p->LocalSocket(), [p, this](const asio::error_code& err){
				if(! err)
				{
					p->Start();
					Start();
				}
				else
				{
					std::cout<<"accept error, msg:"<<err.message()<<"\n";
				}
			} );
	}
private:
	asio::io_context& io_context_;
	asio::ip::tcp::acceptor acceptor_;
};

// usage: ./server bind_ip port
int main(int argc, const char**argv)
{
	if(argc < 2)
	{
		cout<<"usage: ./server port\n";
		return -1;
	}

	cout<<"listen on port: "<<stoi(argv[1])<<"\n";
	asio::ip::tcp::endpoint ep(asio::ip::tcp::v4(), std::stoi(argv[1]));
	asio::io_context ioc;
	Sock5Server ss(ioc, ep);
	ss.Start();
	ioc.run();

	return 0;
}


