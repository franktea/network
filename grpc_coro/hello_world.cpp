/**
 * @file hello_world.cpp
 * @author frank
 * @brief 
 * @version 0.1
 * @date 2023-01-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <iostream>
#include <list>
#include <memory>
#include "helloworld.grpc.pb.h"
#include "asio.hpp"
#include "grpcpp/alarm.h"
#include "grpcpp/grpcpp.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloResponse;
using helloworld::HelloRequest;

template<class T, class... Args>
auto allocate(Args&&... args)
{
    return new T{std::forward<Args>(args)...};
}

template<class T>
void deallocate(T* t)
{
    delete t;
}

struct TypeErasedOperation
{
    using OnCompleteFunction = void (*)(TypeErasedOperation*, bool);
    OnCompleteFunction on_complete = nullptr;

    void Complete(bool ok) { 
        this->on_complete(this, ok); 
    }

    TypeErasedOperation(OnCompleteFunction on_complete) : on_complete(on_complete) {}
};

using QueuedOperations = std::list<TypeErasedOperation*>;

template<class Function>
struct Operation : TypeErasedOperation
{
    Function function;
    Operation(Function f) : TypeErasedOperation{&Operation::DoComplete}, function(std::move(f)) {}
    
    static void DoComplete(TypeErasedOperation* base, bool ok)
    {
        auto* self = static_cast<Operation*>(base);
        auto func = std::move(self->function);
        deallocate(self);
        func(ok);
    }
};

struct GrpcContext : asio::execution_context
{
    struct executor_type;

    std::unique_ptr<grpc::ServerCompletionQueue> queue;
    QueuedOperations queued_operations;
    grpc::Alarm alarm;

    explicit GrpcContext(std::unique_ptr<grpc::ServerCompletionQueue> queue) : queue(std::move(queue)) {}

    ~GrpcContext()
    {
        queue->Shutdown();
        asio::execution_context::shutdown();
        asio::execution_context::destroy();
    }

    executor_type get_executor() noexcept;

    static constexpr void* MAKER_TAG = nullptr;

    void run()
    {
        void* tag;
        bool ok;
        while(queue->Next(&tag, &ok))
        {
            if(MAKER_TAG == tag)
            {
                while(!queued_operations.empty())
                {
                    auto op = queued_operations.front();
                    queued_operations.pop_front();
                    op->Complete(ok);
                }
            } else {
                static_cast<TypeErasedOperation*>(tag)->Complete(ok);
            }
        }
    }
};

struct GrpcContext::executor_type
{
    GrpcContext* grpc_context {};

    template<class Function>
    void execute(Function function) const
    {
        auto* op = allocate<Operation<Function>>(std::move(function));
        grpc_context->queued_operations.push_front(op);
        grpc_context->alarm.Set(grpc_context->queue.get(), gpr_time_0(GPR_CLOCK_REALTIME), GrpcContext::MAKER_TAG);
    }

    GrpcContext& query(asio::execution::context_t) const noexcept
    {
      return *grpc_context;
    }

    static constexpr asio::execution::blocking_t query(asio::execution::blocking_t) noexcept
    {
        return asio::execution::blocking.never;
    }

    friend bool operator==(const GrpcContext::executor_type& a,
        const GrpcContext::executor_type& b) noexcept
    {
      return &a.grpc_context == &b.grpc_context;
    }

    friend bool operator!=(const GrpcContext::executor_type& a,
        const GrpcContext::executor_type& b) noexcept
    {
      return &a.grpc_context != &b.grpc_context;
    }
};

GrpcContext::executor_type GrpcContext::get_executor() noexcept
{
    return executor_type{this};
}

// Read改成小写的read就编不过了，应该是跟某个名字叫read的函数冲突
template<class CompletionToken>
auto asnyc_hello(grpc::ClientAsyncReader<helloworld::HelloResponse>& reader,
    helloworld::HelloResponse& response, CompletionToken token)
{
    return asio::async_initiate<CompletionToken, void(bool)>(
        [&](auto completion_handler)
        {
            void* tag = allocate<Operation<decltype(completion_handler)>>(std::move(completion_handler));
            reader.Read(&response, tag);
        }, token);
}

asio::awaitable<void> ProcessRpc()
{
    Greeter::AsyncService service;
    //ServerAsyncResponseWriter<HelloResponse> responder;
    HelloRequest request;
    helloworld::HelloResponse response;
    co_await asnyc_hello(reader, response, asio::use_awaitable);
}

static_assert(asio::execution::is_executor<GrpcContext::executor_type>::value);
//static_assert(asio::execution::is_scheduler<GrpcContext>::value);

int main() {
    GrpcContext grpc_context{std::make_unique<grpc::ServerCompletionQueue>()};
    GrpcContext::executor_type exec{&grpc_context};
    exec.execute([](bool){ std::cout<<"hello wolrd\n"; });
    //asio::co_spawn(exec, ProcessRpc(), asio::detached);
    grpc_context.run();
}
