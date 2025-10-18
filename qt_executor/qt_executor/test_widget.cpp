#include "test_widget.h"
#include "qt_executor.hpp"
#include <QString>
#include <string>
#include <utility>

void test_widget::closeEvent(QCloseEvent *event)
{
    stop_all();
    QWidget::closeEvent(event);
}

void test_widget::showEvent(QShowEvent* event)
{
    stopped_ = false;

    asio::co_spawn(qt_executor(), [this]{ return run_demo(); }, asio::detached);

    QTextEdit::showEvent(event);
}

void test_widget::hideEvent(QHideEvent* event)
{
    stop_all();
    QTextEdit::hideEvent(event);
}

void test_widget::listen_for_stop(std::function<void ()> slot)
{
    if(stopped_)
        return slot();

    stop_signals_.push_back(std::move(slot));
}

void test_widget::stop_all()
{
    stopped_ = true;
    auto copy = std::exchange(stop_signals_, {});
    for(auto&& f: copy)
    {
        f();
    }
}

asio::awaitable<void> test_widget::run_demo()
{
    using namespace std::literals;
    auto timer = asio::high_resolution_timer(co_await asio::this_coro::executor);

    bool done = false;

    listen_for_stop([&]{
        done = true;
        timer.cancel();
    });

    while(! done)
    {
        for(int i = 0; i < 10; ++i)
        {
            timer.expires_after(1s);
            auto ec = std::error_code();
            co_await timer.async_wait(asio::redirect_error(asio::use_awaitable, ec));
            if(ec)
            {
                done = true;
                break;
            }
            this->setText(QString::fromStdString(std::to_string(i + 1) + " seconds"));
        }

        // 如果没下面的判断明显是bug
        if(done) break;

        for(int i = 10; i > 0; --i)
        {
            timer.expires_after(250ms);
            auto ec = std::error_code();
            co_await timer.async_wait(asio::redirect_error(asio::use_awaitable, ec));
            if(ec)
            {
                done = true;
                break;
            }
            this->setText(QString::fromStdString(std::to_string(i+1) + " seconds"));
        }
    }

    co_return;
}


