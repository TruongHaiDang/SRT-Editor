#include "main.h"

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    QCoreApplication::setOrganizationName("haidanghth910");
    QCoreApplication::setApplicationName("srteditor");

    MainWindow window;
    window.show();

    return application.exec();
}
