#ifndef QT_WORK_EVENT_HPP
#define QT_WORK_EVENT_HPP

#include <QApplication>

class qt_work_event_base : public QEvent
{
public:
    qt_work_event_base() : QEvent(generated_type()) {}

    virtual void invoke() = 0;

    static QEvent::Type generated_type()
    {
        static int event_type = QEvent::registerEventType();
        return static_cast<QEvent::Type>(event_type);
    }
};

template<class F>
struct basic_qt_work_event : qt_work_event_base
{
    basic_qt_work_event(F f): f_(std::move(f)) {}
    void invoke() override
    {
        f_();
    }
private:
    F f_;
};

#endif // QT_WORK_EVENT_HPP
