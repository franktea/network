#include "test_widget.h"

#include <QApplication>
#include "qt_net_application.hpp"
#include "qt_execution_context.hpp"

int main(int argc, char *argv[])
{
    qt_net_application app(argc, argv);
    qt_execution_context context;

    test_widget w;
    w.setText("Hello, World!");
    w.show();

    return app.exec();
}
