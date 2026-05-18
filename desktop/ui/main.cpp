#include <QApplication>
#include "tray_icon.h"
#include "main_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    TrayIcon trayIcon(nullptr);          // сначала TrayIcon, временно без окна
    MainWindow mainWindow(&trayIcon);    // MainWindow получает указатель на TrayIcon
    trayIcon.setMainWindow(&mainWindow); // теперь TrayIcon знает о MainWindow

    trayIcon.show();

    return app.exec();
}