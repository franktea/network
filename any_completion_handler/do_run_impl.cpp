#include <chrono>
#include <iostream>
#include "asio.hpp"
#include "asio/any_completion_handler.hpp"
#include "asio/consign.hpp"

using namespace std::literals::chrono_literals;
using std::error_code;

void do_run_impl(asio::any_completion_handler<void(error_code)> handler,
    asio::any_io_executor ex, int run)
{
    auto timer = std::make_shared<asio::steady_timer>(ex, 300ms - run * 100ms);
    timer->async_wait(asio::consign(std::move(handler), timer));
}

template<typename Token>
inline auto async_do_run(asio::any_io_executor ex, int run, Token&& token)
{
    return asio::async_initiate<Token, void(error_code)>(
        do_run_impl, 
        token, // 这里不能用std::move(token)，因为这个参数的类型是CompletionToken &
        ex, 
        run
    );
}

asio::awaitable<void> coro(int run)
{
    try {
        co_await async_do_run(co_await asio::this_coro::executor, run, asio::use_awaitable);
        std::cout<<"async operation using co_await completed successfully\n";
    } catch(std::system_error const& e) {
        std::cout<<"async operation resulted in error: "<<e.code().message()<<"\n";
    }
}

template<typename T>
bool is_ready(std::future<T> const& f)
{
    return f.wait_for(0ms) == std::future_status::ready;
}

int main()
{
    asio::thread_pool pool;
    auto ex = asio::make_strand(pool);

    std::future<void> f = async_do_run(ex, 0, asio::use_future);
    asio::co_spawn(ex, coro(1), asio::detached);

    async_do_run(ex, 2, [](error_code ec){
        std::cout<<"callback completed: "<< (ec ? ec.message() : "successful")<<"\n";
    });

    std::cout<<"ready? "<<std::boolalpha<<is_ready(f)<<"\n";

    f.wait();
    std::cout<<"ready? "<<std::boolalpha<<is_ready(f)<<"\n";

    pool.join();
}
