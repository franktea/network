/**
 AsyncHttpsClient.h
 frank Jun 7, 2018
 */

#ifndef HTTPCLIENT_HTTPS_ASYNCHTTPSCLIENT_H_
#define HTTPCLIENT_HTTPS_ASYNCHTTPSCLIENT_H_

#include <iostream>
#include <string>
#include <functional>
#include <memory>
#include <asio.hpp>
#include <asio/ssl.hpp>

using std::vector;
using std::string;
using std::pair;
using asio::ip::tcp;
using std::placeholders::_1;
using std::placeholders::_2;

class AsyncHttpsClient: public std::enable_shared_from_this<AsyncHttpsClient>
{
    using ahc_callback = std::function<void(std::shared_ptr<AsyncHttpsClient>)>;
public:
    AsyncHttpsClient(asio::io_context& io_context,
            asio::ssl::context& ssl_context, string url) :
            resolver_(io_context), socket_(io_context, ssl_context), url_(url)
    {
    }
public:
    void Start(ahc_callback cb);
    int Result(); // 0 for ok, otherwise failed
    string Err()
    {
        return err_;
    } // if Result returns non zero, Err return detail message
private:
    bool verify_certificate(bool preverified, asio::ssl::verify_context& ctx);
private:
    string protocol_; // http or https
    string host_; // such as www.qq.com, 129.168.0.1
    string path_; // url path, such as /, /index.html
private:
    //HCStatus status_;
    //asio::io_context& io_context_;
    tcp::resolver resolver_;
    asio::ssl::stream<tcp::socket> socket_;
    string url_;
    vector<pair<string, string>> request_headers_;
    string request_;
    vector<pair<string, string>> response_headers_;
    string response_;
    ahc_callback finish_callback_;
    string err_;
};

#endif /* HTTPCLIENT_HTTPS_ASYNCHTTPSCLIENT_H_ */
