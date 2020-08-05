/*
 * example.cpp
 *
 *  Created on: Nov 1, 2018
 *      Author: frank
 */
#include <functional>
#include <iostream>
#include <random>
#include <chrono>
#include "asio.hpp"

using namespace std::placeholders;

struct TimerData {
    
};

void print(const asio::error_code& ec, std::shared_ptr<asio::steady_timer> t, int index)
{
    if(!ec) {
        std::cout << "time out index "<<index<<"...\n";
        t->expires_at(t->expiry() + std::chrono::seconds(1));
        t->async_wait(std::bind(print, _1, t, index)); // add again
    } else if(ec == asio::error::operation_aborted) {
        std::cout<<index<<" aborted\n";
    }
}

int main()
{
    asio::io_context ioc;

    // 添加n个循环的timer
    std::vector<std::shared_ptr<asio::steady_timer>> v;
    for(int i = 0; i < 10000; ++i) {
        auto t = std::make_shared<asio::steady_timer>(ioc, std::chrono::milliseconds(1000));
        v.push_back(t);
        t->async_wait(std::bind(print, _1, t, i));
    }

    // 180秒内随机cancel掉全部timer
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 180000);
    for(auto t: v) {
        int time_out = distrib(gen); 
        auto t2 = std::make_shared<asio::steady_timer>(ioc, std::chrono::milliseconds(time_out));
        t2->async_wait([t, t2](const asio::error_code& ec){
            t->cancel();
        });
    }
    v.clear();

    // 当所有timer都被cancel以后退出
    ioc.run();
}

