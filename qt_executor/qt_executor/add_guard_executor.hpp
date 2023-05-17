#ifndef ADD_GUARD_EXECUTOR_HPP
#define ADD_GUARD_EXECUTOR_HPP

// 从这个类继承就能有一个get_executor()函数返回guard executor

#include "qt_execution_context.hpp"
#include "qt_guarded_executor.hpp"

struct add_guarded_executor
{
    using executor_type = qt_guarded_executor;

    add_guarded_executor(qt_execution_context& ctx = qt_execution_context::instance())
        : context_(std::addressof(ctx))
    {}

    void reset_guard()
    {
        guard_.reset();
    }

    executor_type get_executor() const
    {
        return qt_guarded_executor(guard_, *context_);
    }

private:
    void new_guard()
    {
        static int x = 0;
        guard_ = std::shared_ptr<int>(std::addressof(x),
            // no-op deleter
            [](auto*){});
    }
private:
    qt_execution_context* context_;
    std::shared_ptr<void> guard_;
};


#endif // ADD_GUARD_EXECUTOR_HPP
