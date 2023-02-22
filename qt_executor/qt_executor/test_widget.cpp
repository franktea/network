#include "test_widget.h"
#include "qt_executor.hpp"
#include <QString>
#include <string>

void test_widget::showEvent(QShowEvent* event)
{
    asio::co_spawn(qt_executor(), [this]{ return run_demo(); }, asio::detached);

    QTextEdit::showEvent(event);
}

void test_widget::hideEvent(QHideEvent* event)
{
    QTextEdit::hideEvent(event);
}

asio::awaitable<void> test_widget::run_demo()
{
    using namespace std::literals;
    auto timer = asio::high_resolution_timer(co_await asio::this_coro::executor);

    for(int i = 0; i < 10; ++i)
    {
        timer.expires_after(1s);
        co_await timer.async_wait(asio::use_awaitable);
        this->setText(QString::fromStdString(std::to_string(i + 1) + " seconds"));
    }

    co_return;
}


