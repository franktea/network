#ifndef TEST_WIDGET_H
#define TEST_WIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <vector>
#include "asio.hpp"
#include "add_guard_executor.hpp"

class test_widget : public QTextEdit, public add_guarded_executor
{
    Q_OBJECT

public:
    using QTextEdit::QTextEdit;
protected:
    void closeEvent(QCloseEvent* event) override;
private:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

    void listen_for_stop(std::function<void()> slot);
    void stop_all();

    asio::awaitable<void> run_demo();

    std::vector<std::function<void()>> stop_signals_;
    bool stopped_= false;
};
#endif // TEST_WIDGET_H
