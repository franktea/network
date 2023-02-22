#include "qt_net_application.hpp"

bool qt_net_application::event(QEvent* event)
{
    if(event->type() == qt_work_event_base::generated_type())
    {
        auto p = static_cast<qt_work_event_base*>(event);
        p->accept();
        p->invoke();
        return true;
    }

    return QApplication::event(event);
}
