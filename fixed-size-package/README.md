## tcp通信分包简单高效的方法

即 定长包头、边长包体

包头中有一个字段说明当前包体长度。

收包时，先按照包头的大小size1，收取size1个字节，从中解出包体size2，然后再收size2个字节的数据，即收到完整的包。

```
for(;;) {
    // 收取包头，因为包头是定长的，用asio的一次调用即可。
    // 注意此处用的是sizeof(PkgHeader)，实际上打包的时候考虑到字节的对齐未必成立，但只要包头是定长即可。
    // 例如包头打包以后是20字节，那么就收取20字节。
    // 还有一个注意事项就是网络通信一般是需要转字节序的，因为整数在不同的cpu里面有大端和小端两种表示方法，
    // 此处未转字节序是假设两边的cpu一样。
    size_t n = co_await asio::async_read(socket_, asio::buffer(&request_, sizeof(PkgHeader)));

    // check if request body len is valid
    if(request_.header.body_len > MAX_BODY_LEN) {
        std::cerr<<"invalid body len: "<<request_.header.body_len<<"\n";
        socket_.close();
        co_return;
    }

    // 从包头中解析出包体的长度，然后再收入整个包体。
    n = co_await asio::async_read(socket_, asio::buffer(&request_.body, request_.header.body_len));
    // receive ok, now process request and send response to client

    std::cout<<"recved pkg, body len="<<n<<"\n";
    response_ = request_;

    // send response
    co_await asio::async_write(socket_, asio::buffer(&response_, sizeof(PkgHeader) + response_.header.body_len));
}
```