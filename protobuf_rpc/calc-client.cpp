/*
 * calc-client.cpp
 *
 *  Created on: Nov 12, 2018
 *      Author: frank
 */
#include <iostream>
#include "pbrpc.h"
#include "calc.pb.h"

using namespace std;

int main()
{
    asio::io_context ioc;
    SyncChannel channel(ioc);
    channel.Init("127.0.0.1", 30000);

    CalcRequest request;
    CalcResponse response;
    request.set_a(3);
    request.set_b(2);

    MinimalController ctrl;
    CalcService_Stub stub(&channel);
    stub.Add(&ctrl, &request, &response, nullptr);
    cout<<"ret="<<response.ret()<<"\n";

    return 0;
}





