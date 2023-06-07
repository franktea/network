/**
 * @file timer_execudtor.cpp
 * @author frankt (could.net@gmail.com)
 * @brief modified from priority scheduler example, added asio::post and asio::co_spawn support.
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <map>
#include <chrono>
#include "asio.hpp"

class timer_scheduler : public asio::execution_context
{
public:
    class executor_type
    {
    public:
        executor_type(timer_scheduler& ctx, std::chrono::time_point<std::chrono::steady_clock> deadline):
            context_(ctx), deadline_(deadline) {}
        
        timer_scheduler& query(asio::execution::context_t) const noexcept
        {
            return context_;
        }

        // 2023-06-07 下面这个函数必不可少，否则既不能用post，又不能用co_spawn。
        // 只要加了这个函数就都可以用。
        static constexpr asio::execution::blocking_t query(asio::execution::blocking_t) noexcept
        {
            return asio::execution::blocking.never;
        }

        template<class Func>
        void execute(Func f) const
        {
            auto p(std::make_shared<item<Func>>(deadline_, std::move(f)));
            std::lock_guard<std::mutex> lock(context_.mutex_);
            context_.queue_.push(p);
            context_.condition_.notify_one();
        }

        friend bool operator==(const executor_type& a,
            const executor_type& b) noexcept
        {
            return &a.context_ == &b.context_;
        }

        friend bool operator!=(const executor_type& a,
            const executor_type& b) noexcept
        {
            return ! (a == b);
        }
    private:
        timer_scheduler& context_;
        std::chrono::time_point<std::chrono::steady_clock> deadline_;
    };

    ~timer_scheduler()
    {
        shutdown();
        destroy();
    }

    executor_type get_executor(std::chrono::time_point<std::chrono::steady_clock> deadline) noexcept
    {
        return executor_type(*this, deadline);
    }

    executor_type get_executor() noexcept
    {
        return executor_type(*this, std::chrono::steady_clock::now());
    }

    void run()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        for(;;)
        {
            condition_.wait(lock, [&]{ return stopped_ || !queue_.empty(); });
            if(stopped_)
            {
                return;
            }

            // 模拟timer manager，查询是否有定时器超时
            if(queue_.top()->deadline_ > std::chrono::steady_clock::now()) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                continue;
            }

            auto p(queue_.top());
            queue_.pop();
            lock.unlock();
            p->execute_(p);
            lock.lock();
        }
    }

    void stop()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped_ = true;
        condition_.notify_all();
    }
private:
    struct item_base
    {
        std::chrono::time_point<std::chrono::steady_clock> deadline_;
        void (*execute_)(std::shared_ptr<item_base>&);
    };

    template<class Func>
    struct item: item_base
    {
        item(std::chrono::time_point<std::chrono::steady_clock> deadline,
            Func f): function_(std::move(f))
        {
            deadline_ = deadline;
            execute_ = [](std::shared_ptr<item_base>& p)
            {
                Func tmp(std::move(static_cast<item*>(p.get())->function_));
                p.reset();
                tmp();
            };
        }

        Func function_;
    };

    struct item_compare
    {
        bool operator()(const std::shared_ptr<item_base>& a,
            const std::shared_ptr<item_base>& b)
        {
            return a->deadline_ > b->deadline_;
        }
    };

    std::mutex mutex_;
    std::condition_variable condition_;
    std::priority_queue<
        std::shared_ptr<item_base>,
        std::vector<std::shared_ptr<item_base>>,
        item_compare> queue_;
    bool stopped_ = false;
};


static_assert(asio::execution::is_executor_v<timer_scheduler::executor_type>);

// implement async sleep
template<asio::completion_token_for<void(std::error_code)> CompletionToken>
auto async_sleep_for(timer_scheduler& scheduler,  std::chrono::milliseconds time, CompletionToken&& token)
{
    return asio::async_initiate<CompletionToken, void(std::error_code)>(
        [&scheduler, time](auto handler){
            auto executor = scheduler.get_executor(std::chrono::steady_clock::now() + time);
            auto tracked = asio::prefer(executor, asio::execution::outstanding_work.tracked);
            asio::post(tracked, [h=std::move(handler)]() mutable { h(std::error_code{}); });
        },
        token
    );
}

// test coroutine
asio::awaitable<void> Sleep5(timer_scheduler& scheduler)
{
    std::cout<<"==> going to sleep for 5 seconds in coroutine\n";
    co_await async_sleep_for(scheduler, std::chrono::milliseconds(5000), asio::use_awaitable);
    std::cout<<"==> after sleep for 5 seconds in coroutine\n";
    co_return;
}

int main()
{
    timer_scheduler scheduler;
    auto e1 = scheduler.get_executor(std::chrono::steady_clock::now() + std::chrono::milliseconds(1000));
    auto e2 = scheduler.get_executor(std::chrono::steady_clock::now() + std::chrono::milliseconds(2000));
    auto e3 = scheduler.get_executor(std::chrono::steady_clock::now() + std::chrono::milliseconds(3000));
    auto e6 = scheduler.get_executor(std::chrono::steady_clock::now() + std::chrono::milliseconds(6000));
    asio::dispatch(e1, [](){ std::cout<<"hello after 1s\n"; });
    asio::dispatch(e2, [](){ std::cout<<"hello after 2s\n"; });
    asio::post(e3, [](){ std::cout<<"hello after 3s\n"; });
    asio::post(e6, [&scheduler](){ scheduler.stop(); std::cout<<"stopped after 6s\n"; }); // 6s后停止
    asio::co_spawn(scheduler, Sleep5(scheduler), asio::detached);
    scheduler.run();
}
