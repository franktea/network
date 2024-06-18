/*
 * pbrpc.h
 *
 *  Created on: Oct 24, 2018
 *      Author: frank
 */

#ifndef PROTOBUF_RPC_PBRPC_H_
#define PROTOBUF_RPC_PBRPC_H_

#include <iostream>
#include <memory>
#include <map>
#include "asio.hpp"
#include "google/protobuf/service.h"
#include "google/protobuf/stubs/common.h"
#include "rpc_pkg.pb.h"

using asio::ip::tcp;
using asio::ip::address;

class MinimalController: public ::google::protobuf::RpcController
{
public:
    virtual void Reset() override
    {
    }
    ;
    virtual bool Failed() const override
    {
        return false;
    }
    ;
    virtual std::string ErrorText() const override
    {
        return "";
    }
    ;
    virtual void StartCancel() override
    {
    }
    ;
    virtual void SetFailed(const std::string&) override
    {
    }
    ;
    virtual bool IsCanceled() const override
    {
        return false;
    }
    ;
    virtual void NotifyOnCancel(::google::protobuf::Closure*) override
    {
    }
    ;
};

class SyncChannel: public ::google::protobuf::RpcChannel
{
public:
    SyncChannel(asio::io_context& ioc) :
            socket_(ioc)
    {
    }
    void Init(const std::string& ip, int port)
    {
        socket_.connect(
                tcp::endpoint(asio::ip::address::from_string(ip), port));
    }

    virtual void CallMethod(const ::google::protobuf::MethodDescriptor* method,
            ::google::protobuf::RpcController* controller,
            const ::google::protobuf::Message* request,
            ::google::protobuf::Message* response,
            ::google::protobuf::Closure* done) override;
private:
    asio::ip::tcp::socket socket_;
};

class RpcServer;

class RpcServer
{
    friend struct Session;
    class Session: public std::enable_shared_from_this<Session>
    {
    public:
        Session(asio::io_context& ioc, RpcServer& server) :
                socket_(ioc), server_(server)
        {
        }

        void Start()
        {
            auto p = shared_from_this();
            // 接收请求，先接收长度
            buffer_.resize(4);
            asio::async_read(socket_, asio::buffer(buffer_),
                    [this, p](const asio::error_code& err, size_t len)
                    {
                        if(err)
                        {
                            std::cout<<"recv len err: "<<err.message()<<"\n";
                            return;
                        }
                        int* plen = (int*)(&buffer_[0]);
                        int pkg_len = ntohl(*plen);
                        std::cout<<"request pkg len:"<<pkg_len<<"\n";
                        // 收取请求数据包
                        buffer_.resize(pkg_len);
                        asio::async_read(socket_, asio::buffer(buffer_), [this, p](const asio::error_code& err, size_t len)
                                {
                                    if(err)
                                    {
                                        std::cout<<"recv request pkg err: "<<err.message()<<"\n";
                                        return;
                                    }
                                    // 解包
                                    auto rpc_pkg = std::make_shared<RpcPackage>();
                                    rpc_pkg->ParseFromString(buffer_);
                                    // 处理请求

                                    server_.ProcessRequest(rpc_pkg, p);
                                });
                    });
        }

        void Response(::google::protobuf::Message* response,
                std::shared_ptr<RpcPackage> rpc_pkg)
        {
            auto p = shared_from_this();
            std::cout << "got response: " << response->ShortDebugString()
                    << "\n";

            std::string binary_response = response->SerializeAsString();
            rpc_pkg->set_pkg_size(binary_response.size());
            rpc_pkg->set_pkg(binary_response);

            // 序列化整个请求包
            buffer_ = rpc_pkg->SerializeAsString();
            // reqeust请求包的长度为4字节整数，放在包的最前面
            int length = buffer_.size();
            std::cout << "send length: " << length << "\n";
            length = htonl(length);
            buffer_.insert(0,
                    std::string((const char*) (&length), sizeof(length)));

            asio::async_write(socket_, asio::buffer(buffer_),
                    [p, this](const asio::error_code& err, size_t len)
                    {
                        if(err)
                        {
                            std::cout<<"send response err: "<<err.message()<<"\n";
                            return;
                        }

                        Start(); // 接收下一个请求
                    });
        }

        tcp::socket socket_;
        std::string buffer_;
        RpcServer& server_;
    };

    struct ServiceItem
    {
        ::google::protobuf::Service* service;
        const ::google::protobuf::ServiceDescriptor* service_desc;
        std::map<std::string, const ::google::protobuf::MethodDescriptor*> method_descs;
    };
public:
    RpcServer(const std::string& ip, int port) :
            ep_(address::from_string(ip), port), acceptor_(ioc_, ep_)
    {
    }
    void RegisterService(::google::protobuf::Service* service);
    void Start();

    asio::io_context& Context() { return ioc_; }
private:
    void StartAccept();
    void ProcessRequest(std::shared_ptr<RpcPackage> pkg_request,
            std::shared_ptr<Session> session);
    void OnResponse(
            std::pair<::google::protobuf::Message*, ::google::protobuf::Message*> msgs,
            std::pair<std::shared_ptr<RpcPackage>, std::shared_ptr<Session>> args);
private:
    tcp::endpoint ep_;
    asio::io_context ioc_;
    tcp::acceptor acceptor_;
    std::map<std::string, ServiceItem> services_;
};

#endif /* PROTOBUF_RPC_PBRPC_H_ */
