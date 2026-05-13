#include <QApplication>
#include "tray_icon.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    TrayIcon trayIcon;
    trayIcon.show();

    return app.exec();
}