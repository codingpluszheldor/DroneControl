#include <QDateTime>
#include <QKeyEvent>
#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(this);

    // Параметры по умолчанию
    sbSpeed->setValue(5.0);
    rbYaw->setChecked(true);
    sbYaw->setValue(0.0);
    cBoxDrivetrainType->setCurrentIndex(1);

    setFocusPolicy(Qt::StrongFocus);

    installEventFilter(this);

    _timer = QSharedPointer<QTimer>(new QTimer(this));
    connect(_timer.data(), &QTimer::timeout, this, &MainWindow::slotTimeOut);
    _timer->start(500);
}

MainWindow::~MainWindow()
{
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Up){
        qDebug() << "Клавиша 'вверх' нажата";
        emit signalKeyPressed(Qt::Key_Up);
    } else if(event->key() == Qt::Key_Down){
        qDebug() << "Клавиша 'вниз' нажата";
        emit signalKeyPressed(Qt::Key_Down);
    } else if(event->key() == Qt::Key_Right){
        qDebug() << "Клавиша 'впаво' нажата";
        emit signalKeyPressed(Qt::Key_Right);
    } else if(event->key() == Qt::Key_Left){
        qDebug() << "Клавиша 'влево' нажата";
        emit signalKeyPressed(Qt::Key_Left);
    } else if(event->key() == Qt::Key_PageUp){
        qDebug() << "Клавиша 'влево' нажата";
        emit signalKeyPressed(Qt::Key_PageUp);
    } else if(event->key() == Qt::Key_PageDown){
        qDebug() << "Клавиша 'влево' нажата";
        emit signalKeyPressed(Qt::Key_PageDown);
    }
}

void MainWindow::slotTimeOut()
{
    emit signalSetParams(rbYaw->isChecked(),
                         sbYaw->value(),
                         sbSpeed->value(),
                         cBoxDrivetrainType->currentIndex());
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
    connect(this, &MainWindow::signalSetParams,
            controller, &Controller::slotSetParams, Qt::QueuedConnection);
    // Проверка дрона
    connect(pBtnArm, &QPushButton::clicked,
            controller, &Controller::slotArm, Qt::QueuedConnection);
    connect(pBtnDisArm, &QPushButton::clicked,
            controller, &Controller::slotDisarm, Qt::QueuedConnection);
    connect(pBtnTakeOff, &QPushButton::clicked,
            controller, &Controller::slotTakeoff, Qt::QueuedConnection);
    connect(pBtnLanding, &QPushButton::clicked,
            controller, &Controller::slotLanding, Qt::QueuedConnection);
    connect(pBtnTestBox, &QPushButton::clicked,
            controller, &Controller::slotTestBox, Qt::QueuedConnection);
    // Пульт управления
    connect(this, &MainWindow::signalKeyPressed,
            controller, &Controller::slotKeyPressed, Qt::QueuedConnection);
    connect(pBtnUp, &QPushButton::clicked,
            controller, &Controller::slotToUpFly, Qt::QueuedConnection);
    connect(pBtnDown, &QPushButton::clicked,
            controller, &Controller::slotToDownFly, Qt::QueuedConnection);
    connect(pBtnForward, &QPushButton::clicked,
            controller, &Controller::slotToForwardFly, Qt::QueuedConnection);
    connect(pBtnBack, &QPushButton::clicked,
            controller, &Controller::slotToBackFly, Qt::QueuedConnection);

}

void MainWindow::slotAddLogText(const bool &isError, const QString &text)
{
    Q_UNUSED(isError)

    QDateTime dt = QDateTime::currentDateTime();
    teLog->append(text + QString(" - [%1]").arg(dt.toString("hh:mm:ss.z")));
}

