#include <QApplication>
#include "tray_icon.h"
#include "main_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    MainWindow mainWindow;
    TrayIcon trayIcon(&mainWindow);
    trayIcon.show();

    return app.exec();
}