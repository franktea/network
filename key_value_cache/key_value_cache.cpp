#include "key_value_cache.h"

asio::awaitable<void> Test(AsyncCache<std::string, std::string>& cache)
{
    auto str = co_await cache.async_get(std::string("hello"), asio::use_awaitable);
    std::cout<<"get str: "<<str<<"\n";
    co_return;
}

int main()
{
    asio::io_context io_context(1);
    AsyncCache<std::string, std::string> cache(io_context);
    asio::co_spawn(io_context, Test(cache), asio::detached);
    io_context.run();
}
