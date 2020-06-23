executor在asio库中是一个很重要的概念，将asio的各种组件功能串起来，起到穿针引线的作用。ISO C++从2011年开始就希望引入一套统一的executor库，到2020已经有了很大的进展，但是因为covid-19的原因，进度停止了，重启的时间估计要很久以后。趁这段时间，简单介绍一下asio的executor。

对于executor的基础用法我打算分3篇，每篇尽量简短，阅读起来更轻松，分别为：executor的基本概念和用法、asio中提供的几个execution context、自定义executor的实现。

其实关于executor的内容还有很多，asio从1.14开始对于io_object（如socket）增加了默认为的executor参数，某些应用更方便了，但是性能降低了；对于coroutine的封装，也使用了executor。对于这些内容，可能没机会写了，因为executor本身就很小众，写多了没人看。

在asio中，所有的function object都会被放到一个execution context中去执行，是一个比较重量级的对象，一般来说声明周期较长、不能拷贝。网络编程的核心部件io_context就是一个execution context。

executor是对execution context的一个指针的封装（asio中一般用引用），可以这样理解：

```
struct my_executor {
    // ...
    my_execution_context& context_;
    
    // 代理一大堆executor的成员函数
    void post(...) { context_.post(...); }
};
```

如此一来，executor就可以廉价拷贝了。也就是说，在使用asio的时候，对于任何的executor，不需要使用引用、指针，直接拷贝即可。（虽然如此，但是asio的源码中很多地方还是用了引用）。

另外，executor提供了execution context的全部重要的成员函数，将其代理给内部的context_。

asio通过一些函数来使用它提供的executor。例如asio::post，用来执行一个callable对象。先来看几种基本的用法。

```
#include <iostream>
#include <chrono>
#include "asio.hpp"

int main()
{
    // 用法1：直接post
    asio::post([](){ std::cout<<"hello world\n"; });

    // 用法2：post到thread pool中去
    asio::thread_pool pool;
    asio::post(pool.get_executor(), [](){ std::cout<<"hello from pool\n"; });
    pool.join();

    // 用法3：使用future
    std::future<int> f = asio::post(
            asio::use_future([](){
                return 99;
            }));
    std::cout<<"future get "<<f.get()<<"\n";

    return 0;
}
```

在上面的三种用法中，只有pool.get_executor()看起来和executor的概念有关系。其实另外两种用法是省略了第一个参数，post到一个默认的executor中去了(默认的叫`system_executor`)。

```
asio::post([](){ std::cout<<"hello world\n"; });
```
其实等价于
```
asio::post([](){asio::system_executor(), std::cout<<"hello world\n"; });
```
也等价于
```
asio::system_executor().post([](){ std::cout<<"hello world\n"; }, some_allocator);
```

