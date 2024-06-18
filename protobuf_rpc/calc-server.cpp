/*
 * calc-server.cpp
 *
 *  Created on: Nov 12, 2018
 *      Author: frank
 */
#include <chrono>
#include "pbrpc.h"
#include "calc.pb.h"
#include "asio.hpp"

class ConcreateCalcService: public CalcService
{
public:
    void Add(::google::protobuf::RpcController* controller,
            const ::CalcRequest* request, ::CalcResponse* response,
            ::google::protobuf::Closure* done) override
    {
        asio::co_spawn(io_context, [request, response, done, this]()->asio::awaitable<void> {
            asio::steady_timer timer(io_context);
            timer.expires_after(std::chrono::seconds(5));
            co_await timer.async_wait(asio::use_awaitable);
            std::cout<<"after 5 seconds\n";
            response->set_ret(request->a() + request->b());
            done->Run();
            co_return;
        }, asio::detached);
    }
    void Subtract(::google::protobuf::RpcController* controller,
            const ::CalcRequest* request, ::CalcResponse* response,
            ::google::protobuf::Closure* done) override
    {
        response->set_ret(request->a() - request->b());
        done->Run();
    }
    void Multiply(::google::protobuf::RpcController* controller,
            const ::CalcRequest* request, ::CalcResponse* response,
            ::google::protobuf::Closure* done) override
    {
        response->set_ret(request->a() * request->b());
        done->Run();
    }

    ConcreateCalcService(asio::io_context& ioc):
        io_context(ioc) {}

    asio::io_context& io_context;
};

int main()
{
    RpcServer server("127.0.0.1", 30000);
    ConcreateCalcService service{server.Context()};
    server.RegisterService(&service);
    server.Start();
    return 0;
}

