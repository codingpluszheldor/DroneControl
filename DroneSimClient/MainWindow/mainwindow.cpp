#include <QDateTime>
#include <QKeyEvent>
#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(this);

    twBarometer->resizeColumnsToContents();
    twAngularVel->resizeColumnsToContents();
    twLinearAccel->resizeColumnsToContents();

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
    } else if(event->key() == Qt::Key_Insert){
        qDebug() << "Клавиша 'insert' нажата";
        emit signalKeyPressed(Qt::Key_Insert);
    } else if(event->key() == Qt::Key_Delete){
        qDebug() << "Клавиша 'delete' нажата";
        emit signalKeyPressed(Qt::Key_Delete);
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
    if (controller == nullptr) {
        return;
    }

    // Соединение UI с контролером
    connect(controller, &Controller::signalSendRequest,
            this, &MainWindow::slotAddLogText, Qt::QueuedConnection);
    connect(this, &MainWindow::signalBtnCmd,
            controller, &Controller::slotBtnCmd, Qt::QueuedConnection);
    connect(pBtnConnect, &QPushButton::clicked, this, [this]() {
        emit signalBtnCmd(static_cast<int>(drone::DroneMethods::Connection));
    }, Qt::QueuedConnection);
    connect(this, &MainWindow::signalSetParams,
            controller, &Controller::slotSetParams, Qt::QueuedConnection);
    // Проверка дрона
    connect(pBtnArm, &QPushButton::clicked, this, [this]() {
        emit signalBtnCmd(static_cast<int>(drone::DroneMethods::Arm));
    }, Qt::QueuedConnection);
    connect(pBtnDisArm, &QPushButton::clicked, this, [this]() {
        emit signalBtnCmd(static_cast<int>(drone::DroneMethods::Disarm));
    }, Qt::QueuedConnection);
    connect(pBtnTakeOff, &QPushButton::clicked, this, [this]() {
        emit signalBtnCmd(static_cast<int>(drone::DroneMethods::Takeoff));
    }, Qt::QueuedConnection);
    connect(pBtnLanding, &QPushButton::clicked, this, [this]() {
        emit signalBtnCmd(static_cast<int>(drone::DroneMethods::Landing));
    }, Qt::QueuedConnection);
    connect(pBtnTestBox, &QPushButton::clicked, this, [this]() {
        emit signalBtnCmd(static_cast<int>(drone::DroneMethods::TestFlyBox));
    }, Qt::QueuedConnection);
    // Пульт управления
    connect(pBtnUp, &QPushButton::clicked, this, [this]() {
        emit signalBtnCmd(static_cast<int>(drone::DroneMethods::ToUp));
    }, Qt::QueuedConnection);
    connect(pBtnDown, &QPushButton::clicked, this, [this]() {
        emit signalBtnCmd(static_cast<int>(drone::DroneMethods::ToDown));
    }, Qt::QueuedConnection);
    connect(pBtnForward, &QPushButton::clicked, this, [this]() {
        emit signalBtnCmd(static_cast<int>(drone::DroneMethods::ToForward));
    }, Qt::QueuedConnection);
    connect(pBtnBack, &QPushButton::clicked, this, [this]() {
        emit signalBtnCmd(static_cast<int>(drone::DroneMethods::ToBack));
    }, Qt::QueuedConnection);
    connect(pBtnRight, &QPushButton::clicked, this, [this]() {
        emit signalBtnCmd(static_cast<int>(drone::DroneMethods::ToRight));
    }, Qt::QueuedConnection);
    connect(pBtnLeft, &QPushButton::clicked, this, [this]() {
        emit signalBtnCmd(static_cast<int>(drone::DroneMethods::ToLeft));
    }, Qt::QueuedConnection);
    connect(this, &MainWindow::signalKeyPressed,
            controller, &Controller::slotKeyPressed, Qt::QueuedConnection);
    // Сенсоры
    connect(controller, &Controller::signalBarometerSensorData,
            this, &MainWindow::slotBarometerSensorData, Qt::QueuedConnection);
    connect(controller, &Controller::signalImuSensorData,
            this, &MainWindow::slotImuSensorData, Qt::QueuedConnection);
}

void MainWindow::slotAddLogText(const bool &isError, const QString &text)
{
    Q_UNUSED(isError)

    QDateTime dt = QDateTime::currentDateTime();
    teLog->append(text + QString(" - [%1]").arg(dt.toString("hh:mm:ss.z")));
}

void MainWindow::slotBarometerSensorData(const drone::BarometerSensorDataRep &data)
{
    for (int i = 0; i < 3; i++) {
        if (twBarometer->item(i, 1) == nullptr) {
            QTableWidgetItem *item = new QTableWidgetItem();
            twBarometer->setItem(i, 1, item);
        }
    }
    twBarometer->item(0, 1)->setText(QString::number(data.altitude, 'f', 2));
    twBarometer->item(1, 1)->setText(QString::number(data.pressure, 'f', 2));
    twBarometer->item(2, 1)->setText(QString::number(data.qnh, 'f', 2));
    twBarometer->resizeColumnsToContents();
}

void  MainWindow::slotImuSensorData(const ImuSensorDataRep &data)
{
    for (int i = 0; i < 3; i++) {
        if (twAngularVel->item(i, 1) == nullptr) {
            QTableWidgetItem *item = new QTableWidgetItem();
            twAngularVel->setItem(i, 1, item);
        }
        if (twLinearAccel->item(i, 1) == nullptr) {
            QTableWidgetItem *item = new QTableWidgetItem();
            twLinearAccel->setItem(i, 1, item);
        }
    }

    twAngularVel->item(0, 1)->setText(QString::number(data.angular_velocity_x, 'f', 2));
    twAngularVel->item(1, 1)->setText(QString::number(data.angular_velocity_y, 'f', 2));
    twAngularVel->item(2, 1)->setText(QString::number(data.angular_velocity_z, 'f', 2));
    twAngularVel->resizeColumnsToContents();

    twLinearAccel->item(0, 1)->setText(QString::number(data.linear_acceleration_x, 'f', 2));
    twLinearAccel->item(1, 1)->setText(QString::number(data.linear_acceleration_y, 'f', 2));
    twLinearAccel->item(2, 1)->setText(QString::number(data.linear_acceleration_z, 'f', 2));
    twLinearAccel->resizeColumnsToContents();
}
