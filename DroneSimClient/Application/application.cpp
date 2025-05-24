#include <QStyleFactory>
#include <QDebug>
#include <QSharedPointer>
#include <QThread>
#include <windows.h>
#include "application.h"
#include "MainWindow/mainwindow.h"
#include "Controller/controller.h"
#include "ImageSaver/imagesaver.h"


Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
{
    setlocale(LC_ALL, ".UTF-8");
    SetConsoleOutputCP(CP_UTF8);

    setStyle(QStyleFactory::create("Fusion"));
}

Application::~Application()
{
}

int Application::run()
{
    // VAS: здесь QSharedPointer только для RAII
    QSharedPointer<Controller> controller = QSharedPointer<Controller>(new Controller());
    if (!controller->setInit()) {
        return -1;
    }

    QSharedPointer<MainWindow> mainWindow = QSharedPointer<MainWindow>(new MainWindow());
    mainWindow->setController(controller.data());
    mainWindow->show();

    QThread *thread = new QThread();
    Q_CHECK_PTR(thread);
    connect(controller.data(), SIGNAL(destroyed()), thread, SLOT(quit()), Qt::QueuedConnection);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()), Qt::QueuedConnection);
    controller->moveToThread(thread);
    thread->start();

    QSharedPointer<ImageSaver> imageSaver = QSharedPointer<ImageSaver>(new ImageSaver());
    thread = new QThread();
    Q_CHECK_PTR(thread);
    connect(controller.data(), &Controller::signalSaveImage,
            imageSaver.data(), &ImageSaver::slotSave, Qt::QueuedConnection);
    connect(imageSaver.data(), SIGNAL(destroyed()), thread, SLOT(quit()), Qt::QueuedConnection);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()), Qt::QueuedConnection);
    imageSaver->moveToThread(thread);
    thread->start();

    return exec();
}
