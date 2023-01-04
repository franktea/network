#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include <random>
#include <iostream>
#include "asio.hpp"

using namespace std::chrono;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;

awaitable<void> quicksort(asio::io_context& ctx,
    std::vector<int>::iterator begin,
    std::vector<int>::iterator end) 
{
    //std::cout<<"thread: "<<std::this_thread::get_id()<<"\n";

    if(std::distance(begin, end) <= 32) {
        std::sort(begin, end);
    } else {
        auto pivot = begin + std::distance(begin, end) - 1;
        auto i = std::partition(begin, pivot, [=](int x){ return x <= *pivot; });
        std::swap(*i, *pivot);

        auto quicksortwrapper = [&](auto&& begin, auto&& end) {
            quicksort(ctx, begin, end);
        };

        co_await quicksort(ctx, begin, i);
        co_await quicksort(ctx, i + 1, end);
    }
}

int main() {
    std::vector<int> arr(10000000);

    std::cout<<"filling"<<"\n";
    std::iota(std::begin(arr), std::end(arr), 0);
    auto shuffle = [](std::vector<int>& arr) {
        std::mt19937 rng(std::random_device{}());
        std::shuffle(std::begin(arr), std::end(arr), rng);
    };
    shuffle(arr);

    std::cout<<"running"<<"\n";

    const int num_threads = std::thread::hardware_concurrency();
    asio::io_context ctx{num_threads};
    const auto start = high_resolution_clock::now();

    co_spawn(ctx,
        [&]()->awaitable<void> {
            co_await quicksort(ctx, std::begin(arr), std::end(arr));
        },
        detached);
    
    ctx.run();

    const auto elapsed = duration_cast<milliseconds>(high_resolution_clock::now() - start);
    std::cout<<"took "<<elapsed.count()<<"\n";

    std::cout<<"sorted: "<<std::is_sorted(std::begin(arr), std::end(arr))<<"\n";
}
