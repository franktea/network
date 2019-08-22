/*
 * pbrpc.cpp
 *
 *  Created on: Oct 24, 2018
 *      Author: frank
 */
#include "asio/experimental.hpp"
#include "pbrpc.h"
#include "rpc_pkg.pb.h"

using namespace std;
using asio::ip::tcp;
using asio::ip::address;

void SyncChannel::CallMethod(const ::google::protobuf::MethodDescriptor* method,
        ::google::protobuf::RpcController* controller,
        const ::google::protobuf::Message* request,
        ::google::protobuf::Message* response,
        ::google::protobuf::Closure* done)
{
    std::string binary_request = request->SerializeAsString();

    // 构造完整的请求包，为RpcPackage类型的message
    RpcPackage req_pkg;
    req_pkg.set_service_name(method->service()->name());
    req_pkg.set_method_name(method->name());
    req_pkg.set_pkg_size(binary_request.size());
    request->SerializeToString(req_pkg.mutable_pkg());

    // 序列化整个请求包
    std::string binary_req_pkg = req_pkg.SerializeAsString();
    // reqeust请求包的长度为4字节整数，放在包的最前面
    int length = binary_req_pkg.size();
    std::cout << "send length: " << length << "\n";
    length = htonl(length);
    binary_req_pkg.insert(0,
            std::string((const char*) (&length), sizeof(length)));

    // 为了简单起见，采用同步来实现
    socket_.send(asio::buffer(binary_req_pkg));

    // 接收回包，先接收4字节的长度
    int recv_len;
    socket_.receive(asio::buffer(&recv_len, sizeof(recv_len)));
    recv_len = ntohl(recv_len);
    std::cout << "recv length: " << recv_len << "\n";

    // 再接收回包
    std::string binary_resp_pkg;
    binary_resp_pkg.resize(recv_len);
    socket_.receive(
            asio::buffer(&binary_resp_pkg[0], binary_resp_pkg.length()));

    // 解包
    RpcPackage resp_msg;
    resp_msg.ParseFromString(binary_resp_pkg);
    // 解出response结构
    response->ParseFromString(resp_msg.pkg());
}

void RpcServer::RegisterService(::google::protobuf::Service* service)
{
    ServiceItem item;
    item.service = service;
    item.service_desc = service->GetDescriptor();
    for (int i = 0; i < item.service_desc->method_count(); ++i)
    {
        item.method_descs.insert(
                std::make_pair(item.service_desc->method(i)->name(),
                        item.service_desc->method(i)));
    }
    services_.insert(std::make_pair(item.service_desc->name(), item));
}

void RpcServer::Start()
{
    StartAccept();
    ioc_.run();
}

void RpcServer::ProcessRequest(std::shared_ptr<RpcPackage> pkg_request,
        std::shared_ptr<Session> session)
{
    auto it = services_.find(pkg_request->service_name());
    if (it == services_.end())
    {
        cout << "unregistered serivce name: " << pkg_request->service_name()
                << "\n";
        return;
    }

    ServiceItem& item = it->second;
    auto it2 = item.method_descs.find(pkg_request->method_name());
    if (it2 == item.method_descs.end())
    {
        cout << "no method name = " << pkg_request->method_name() << "\n";
        return;
    }
    const ::google::protobuf::MethodDescriptor* pmd = it2->second;

    cout << "process service name: " << pkg_request->service_name() << "\n";
    cout << "process method name: " << pkg_request->method_name() << "\n";
    cout << "process req type: " << pmd->input_type()->name() << "\n";
    cout << "process resp type: " << pmd->output_type()->name() << "\n";

    ::google::protobuf::Message* request = item.service->GetRequestPrototype(
            pmd).New();
    request->ParseFromString(pkg_request->pkg());
    ::google::protobuf::Message* response = item.service->GetResponsePrototype(
            pmd).New();

    auto done = ::google::protobuf::NewCallback(this, &RpcServer::OnResponse,
            std::make_pair(request, response),
            std::make_pair(pkg_request, session));
    MinimalController controller;
    item.service->CallMethod(pmd, &controller, request, response, done);
}

void RpcServer::OnResponse(
        std::pair<::google::protobuf::Message*, ::google::protobuf::Message*> msgs,
        std::pair<std::shared_ptr<RpcPackage>, std::shared_ptr<Session>> args)
{
    args.second->Response(msgs.second, args.first);
    delete msgs.first;
    delete msgs.second;
}

void RpcServer::StartAccept()
{
    auto ps = std::make_shared<Session>(ioc_, *this);
    acceptor_.async_accept(ps->socket_, [ps, this](const asio::error_code& err)
    {
        if(err)
        {
            std::cout<<"accept err: "<<err.message()<<"\n";
            return;
        }
        ps->Start();
        StartAccept();
    });
}
