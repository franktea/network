#ifndef QT_EXECUTION_CONTEXT_HPP
#define QT_EXECUTION_CONTEXT_HPP

#include <QApplication>
#include <cassert>
#include "asio.hpp"
#include "qt_work_event.hpp"

struct qt_execution_context : public asio::execution_context
{
    qt_execution_context(QApplication* app = qApp) : app_(app)
    {
        instance_ = this;
    }

    template<class F>
    void post(F f)
    {
        auto event = new basic_qt_work_event(std::move(f));
        QApplication::postEvent(app_, event);
    }

    static qt_execution_context& instance()
    {
        assert(instance_);
        return *instance_;
    }
private:
    inline static qt_execution_context* instance_ = nullptr;
    QApplication* app_;
};


#endif // QT_EXECUTION_CONTEXT_HPP
