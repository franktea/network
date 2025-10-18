#ifndef QT_EXECUTOR_HPP
#define QT_EXECUTOR_HPP

#include "qt_execution_context.hpp"

struct qt_executor
{
    qt_executor(qt_execution_context& context = qt_execution_context::instance()) noexcept
        : context_(std::addressof(context))
    {}

    qt_execution_context& query(asio::execution::context_t) const noexcept
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
        context_->post(std::move(f));
    }

    bool operator==(qt_executor const& other) const noexcept
    {
        return context_ == other.context_;
    }

    bool operator!=(qt_executor const& other) const noexcept
    {
        return !(*this == other);
    }
private:
    qt_execution_context* context_;
};

static_assert(asio::execution::is_executor_v<qt_executor>);

#endif // QT_EXECUTOR_HPP
