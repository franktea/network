#ifndef QT_GUARDED_EXECUTOR_HPP
#define QT_GUARDED_EXECUTOR_HPP

#include <memory>
#include "qt_execution_context.hpp"

struct qt_guarded_executor
{
    qt_guarded_executor(
        std::weak_ptr<void> guard,
        qt_execution_context& context = qt_execution_context::instance()) noexcept
        : context_(std::addressof(context))
        , guard_(std::move(guard)) {}

    qt_execution_context& query(asio::execution::context_t) noexcept
    {
        return *context_;
    }

    static constexpr asio::execution::blocking_t query(asio::execution::blocking_t) noexcept
    {
        return asio::execution::blocking.never;
    }

    static constexpr asio::execution::relationship_t query(asio::execution::relationship_t) noexcept
    {
        return asio::execution::relationship.fork;
    }

    static constexpr asio::execution::outstanding_work_t query(asio::execution::outstanding_work_t) noexcept
    {
        return asio::execution::outstanding_work.tracked;
    }

    template<typename OtherAllocator>
    static constexpr auto query(asio::execution::allocator_t<OtherAllocator>) noexcept
    {
        return std::allocator<void>();
    }

    static constexpr auto query(asio::execution::allocator_t<void>) noexcept
    {
        return std::allocator<void>();
    }

    template<class F>
    void execute(F f) const
    {
        if(auto lock1 = guard_.lock())
        {
            context_->post([guard = guard_, f = std::move(f)](){
                if(auto lock2 = guard.lock())
                {
                    f();
                }
            });
        }
    }

    bool operator==(qt_guarded_executor const& other) const noexcept
    {
        return context_ == other.context_ && !guard_.owner_before(other.guard_)
               && !other.guard_.owner_before(guard_);
    }

    bool operator!=(qt_guarded_executor const& other) const noexcept
    {
        return ! (*this == other);
    }
private:
    qt_execution_context* context_;
    std::weak_ptr<void> guard_;
};

static_assert(asio::execution::is_executor_v<qt_guarded_executor>);

#endif // QT_GUARDED_EXECUTOR_HPP
