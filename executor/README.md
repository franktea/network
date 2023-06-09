自定义的executor，关键是要定义两个函数:

```
your_scheduler & query(asio::execution::context_t) const noexcept
{
    return context_;
}

static constexpr asio::execution::blocking_t query(asio::execution::blocking_t) noexcept
{
    return asio::execution::blocking.never;
}
```

另外==和!==也必不可少。这个在asio的文档里面已经说得很清楚：
https://www.boost.org/doc/libs/1_82_0/doc/html/boost_asio/std_executors.html

只要定义了以上4个函数就可以支持各种常规操作了，包括post、co_spawn等，asio的例子中很多都没有定义blocking_t，所以不支持co_spawn。

代码：
timer_executor.cpp 参考asio的example/cpp14/executors/priority_scheduler.cpp实现的至于定时器的executor。

