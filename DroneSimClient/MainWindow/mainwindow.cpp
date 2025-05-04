#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(this);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setController(Controller *controller)
{
//    _controller = controller;
    if (controller == nullptr) {
        return;
    }

    // Соединение UI с контролером
    connect(controller, &Controller::signalSendRequest,
            this, &MainWindow::slotAddLogText, Qt::QueuedConnection);
    connect(pBtnConnect, &QPushButton::clicked,
            controller, &Controller::slotConnect, Qt::QueuedConnection);

}

void MainWindow::slotAddLogText(const bool &isError, const QString &text)
{
    teLog->append(text);
    teLog->append("\n");
}

