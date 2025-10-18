#ifndef QT_NET_APPLICATION_HPP
#define QT_NET_APPLICATION_HPP

#include "qt_work_event.hpp"
#include <QApplication>

class qt_net_application : public QApplication
{
    using QApplication::QApplication;
protected:
    bool event(QEvent* event) override;
};

#endif // QT_NET_APPLICATION_HPP
