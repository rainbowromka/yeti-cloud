#include <QApplication>
#include <QThread>
#include "client.h"
#include "tray_icon.h"
#include "main_window.h"

static QThread* startClientInThread(Client& client)
{
    auto* thread = new QThread();
    thread->setObjectName("ClientThread");
    client.moveToThread(thread);
    thread->start();
    return thread;
}

static void stopClientInThread(QThread* thread)
{
    thread->quit();
    thread->wait();
    delete thread;
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    Client client;
    QThread* clientThread = startClientInThread(client);

    TrayIcon trayIcon(nullptr);
    MainWindow mainWindow(&client, &trayIcon);
    trayIcon.setMainWindow(&mainWindow);
    trayIcon.show();

    int result = app.exec();

    stopClientInThread(clientThread);
    return result;
}