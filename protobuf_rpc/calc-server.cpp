/*
 * calc-server.cpp
 *
 *  Created on: Nov 12, 2018
 *      Author: frank
 */
#include "pbrpc.h"
#include "calc.pb.h"

class ConcreateCalcService: public CalcService
{
public:
    void Add(::google::protobuf::RpcController* controller,
            const ::CalcRequest* request, ::CalcResponse* response,
            ::google::protobuf::Closure* done) override
    {
        response->set_ret(request->a() + request->b());
        done->Run();
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
};

int main()
{
    RpcServer server("127.0.0.1", 30000);
    ConcreateCalcService service;
    server.RegisterService(&service);
    server.Start();
    return 0;
}

