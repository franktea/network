/*
 AsyncHttpsClient.cpp
 frank Jun 7, 2018
 */
#include <regex>
#include "AsyncHttpsClient.h"

void AsyncHttpsClient::Start(ahc_callback cb)
{
    finish_callback_ = cb;

    // parse url
    std::regex re("(https?)://([^/]+)((/.*)*)");
    std::smatch sm;
    if (!std::regex_match(url_, sm, re))
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
    auto self = shared_from_this();
    resolver_.async_resolve(host_, protocol_,
            [this, self](const asio::error_code& error,
                    tcp::resolver::results_type results)
            {
                if(! error)
                {
                    //std::cout<<url_<<"finished resolving"<<"\n";
                    // connect to the server
                    socket_.set_verify_mode(asio::ssl::verify_peer);
                    socket_.set_verify_callback(std::bind(&AsyncHttpsClient::verify_certificate, this, _1, _2));

                    asio::async_connect(socket_.lowest_layer(), results, [this, self](const asio::error_code& error, const tcp::endpoint& endpoint)
                            {
                                if(! error)
                                {
                                    socket_.async_handshake(asio::ssl::stream_base::client, [this, self](const asio::error_code& error)
                                            {
                                                if(! error)
                                                {
                                                    // succeeded to handshake, now send http request and receive response
                                                    request_.append("GET ").append(path_).append(" HTTP/1.1\r\n");
                                                    request_.append("Host: ").append(host_).append("\r\n");
                                                    request_.append("Accept: */*\r\n");
                                                    request_.append("Connection: Close\r\n\r\n");

                                                    asio::async_write(socket_, asio::buffer(&request_[0], request_.size()), [this, self](const asio::error_code& error, size_t len)
                                                            {
                                                                if(! error)
                                                                {
                                                                    // read all data, until the server finish sending and close the socket
                                                                    asio::async_read(socket_, asio::dynamic_buffer(response_), [this, self](const asio::error_code& error, size_t len)
                                                                            {
                                                                                if(! error)
                                                                                {

                                                                                }
                                                                                else if(error.value() != asio::error::eof)
                                                                                {
                                                                                    err_ = error.message();
                                                                                }
                                                                                std::cout<<response_<<"\r\n\r\n";
                                                                                finish_callback_(shared_from_this());
                                                                            });
                                                                }
                                                                else
                                                                {
                                                                    err_ = error.message();
                                                                    finish_callback_(shared_from_this());
                                                                }
                                                            });
                                                }
                                                else
                                                {
                                                    err_ = error.message();
                                                    finish_callback_(shared_from_this());
                                                }
                                            });
                                }
                                else
                                {
                                    err_ = error.message();
                                    finish_callback_(shared_from_this());
                                }
                            });
                }
                else
                {
                    err_ = error.message();
                    finish_callback_(shared_from_this());
                }
            });
}

int AsyncHttpsClient::Result()
{
}

bool AsyncHttpsClient::verify_certificate(bool preverified,
        asio::ssl::verify_context& ctx)
{
    // The verify callback can be used to check whether the certificate that is
    // being presented is valid for the peer. For example, RFC 2818 describes
    // the steps involved in doing this for HTTPS. Consult the OpenSSL
    // documentation for more details. Note that the callback is called once
    // for each certificate in the certificate chain, starting from the root
    // certificate authority.

    // In this example we will simply print the certificate's subject name.
    char subject_name[256];
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    std::cout << "Verifying " << subject_name << "\n";

    return preverified;
}
