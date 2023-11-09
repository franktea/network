
入门asio最好的资料还是它的examples。所以我打算把所有的examples代码全部加上注释。

最新的asio已经取消了c++11以下的支持，所以examples的cpp03的目录我删掉了。

另外，cpp17的功能cpp20都有，所以也懒得注释了。

# cpp11/allocation

主要就是演示`socket_.async_read_some`这样的函数所发起的一部操作本身是支持自定义的allocator的，传入allocator的方式是用`asio::bind_allocator`，`asio::bind_allocator`的第一个参数是`const allocator&`类型，它会拷贝一个allocator保存起来。

另外，`asio::dispatch`系列函数也支持用`asio::bind_allocator`传入自定义的allocator。

