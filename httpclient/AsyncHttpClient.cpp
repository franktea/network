/*
 simple async http client, retrieve html from a web server,
 without considering redirection.

                      frank May 24, 2018
*/
#include <regex>
#include "AsyncHttpClient.h"

void AsyncHttpClient::Start(ahc_callback cb)
{
	finish_callback_ = cb;

	// parse url
	std::regex re("(https?)://([^/]+)((/.*)*)");
	std::smatch sm;
	if(! std::regex_match(url_, sm, re))
	{
		err_ = "invalid url";
		finish_callback_(shared_from_this());
		return;
	}

	protocol_ = sm[1];
	host_ = sm[2];
	path_ = sm[3].str() == "" ? "/" : sm[3].str();

	//std::cout<<"parsed, protocol="<<protocol_<<", host="<<host_<<", path="<<path_<<"\n";

    // asynchronous resolve host name to ip address
    resolver_.async_resolve(host_, protocol_,
        std::bind(&AsyncHttpClient::ResolveCallback, this,
        		_1, _2));
}

void AsyncHttpClient::ResolveCallback(asio::error_code const & err,
		tcp::resolver::results_type const & endpoints)
{
	if(! err)
	{
		//std::cout<<url_<<"finished resolving"<<"\n";
		// connect to the server
		asio::async_connect(socket_, endpoints,
	          std::bind(&AsyncHttpClient::ConnectCallback, this, _1));
	}
	else
	{
		err_ = err.message();
		finish_callback_(shared_from_this());
	}
}

void AsyncHttpClient::ConnectCallback(asio::error_code const & err)
{
	if(! err)
	{
		std::cout<<"connected"<<"\n";

		request_.append("GET ").append(path_).append(" HTTP/1.1\r\n");
		request_.append("Host: ").append(host_).append("\r\n");
		request_.append("Accept: */*\r\n");
		request_.append("Connection: Close\r\n\r\n");

		asio::async_write(socket_, asio::buffer(&request_[0], request_.size()),
				std::bind(&AsyncHttpClient::SendRequestCallback, this, _1));
	}
	else
	{
		err_ = err.message();
		finish_callback_(shared_from_this());
	}
}

void AsyncHttpClient::SendRequestCallback(asio::error_code const & err)
{
	if(! err)
	{
		std::cout<<"finished sending request\n";

		// read all data, until the server finish sending and close the socket
		asio::async_read(socket_, asio::dynamic_buffer(response_),
				std::bind(&AsyncHttpClient::ServerResponseCallback, this, _1, _2));
	}
	else
	{
		err_ = err.message();
		finish_callback_(shared_from_this());
	}
}

void AsyncHttpClient::ServerResponseCallback(asio::error_code const & err, std::size_t bytes_transferred)
{
	if(! err)
	{
		std::cout<<response_.substr(0, bytes_transferred)<<"\n";
	}
	else if(err.value() == 2) // close by server
	{
		// now the whole html content is stored in response_, including response headers.
	}
	else
	{
		err_ = err.message();
	}

	finish_callback_(shared_from_this());
}
