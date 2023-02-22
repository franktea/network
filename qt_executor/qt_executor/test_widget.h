#ifndef TEST_WIDGET_H
#define TEST_WIDGET_H

#include <QWidget>
#include <QTextEdit>
#include "asio.hpp"

class test_widget : public QTextEdit
{
    Q_OBJECT

public:
    using QTextEdit::QTextEdit;
private:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

    asio::awaitable<void> run_demo();
};
#endif // TEST_WIDGET_H
