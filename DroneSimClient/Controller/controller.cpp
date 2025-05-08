#include <QDateTime>
#include <compat/nanomsg/nn.h>
#include <compat/nanomsg/reqrep.h>
#include "controller.h"

Controller::Controller(QObject *parent)
    : QObject(parent)
{
    _timer = QSharedPointer<QTimer>(new QTimer(this));
    connect(_timer.data(), &QTimer::timeout, this, &Controller::slotTimeOut);
    _timer->start(50);
}

Controller::~Controller()
{
    if (_clientSock > -1) {
        nn_shutdown(_clientSock, 0);
        nn_close(_clientSock);
    }
}

bool Controller::setInit()
{
    _clientSock = nn_socket(AF_SP, NN_REQ);
    if (_clientSock < 0) {
        qDebug() << "Ошибка инициализации socket";
        return false;
    }

    const char* endpoint = "tcp://127.0.0.1:20001";
    if (nn_connect(_clientSock, endpoint) < 0) {
        qDebug() << "Ошибка соединения с сервером";
        nn_close(_clientSock);
        return false;
    }

    return true;
}

bool Controller::sendRequest(drone::DroneMethodReq *request)
{
    int sendResult = nn_send(_clientSock, request, sizeof(drone::DroneMethodReq), 0);
    if (sendResult < 0) {
        _errorText = QString("[%1] Ошибка отправки команды").arg(_methodNames.value(request->method));
        qDebug() << _errorText;
        return false;
    } else {
        _requestText = QString("--> [%1] Команда отправлена").arg(_methodNames.value(request->method));
         emit signalSendRequest(false, _requestText);

        // Ожидание ответа от сервера
        char buffer[drone::MSG_BUFFER_SIZE];
        int recvResult = nn_recv(_clientSock, buffer, sizeof(buffer), 0);
        if (recvResult > 0){
            _replyText = QString("<-- [%1] Получен ответ от сервера").arg(_methodNames.value(request->method));
        } else {
            _errorText = QString("[%1] Ошибка приема ответа").arg(_methodNames.value(request->method));
            qDebug() << _errorText;
            return false;
        }
    }

    return true;
}

void Controller::makeRequest(const drone::DroneMethods &method)
{
    drone::DroneMethodReq *request = new drone::DroneMethodReq;
    request->method = method;
    request->speed = _speed;
    request->yaw_is_rate = _yaw_is_rate;
    request->yaw_or_rate = _yaw_or_rate;
    request->drivetrain = _drivetrain;
    request->time_point = QDateTime::currentSecsSinceEpoch();

    if (_lastCmd != method) {
        while (!_cmqQueue.isEmpty()) {
            delete _cmqQueue.dequeue();
        }
        _cmqQueue.append(request);
    }
}

void Controller::slotTimeOut()
{
    if (_cmqQueue.isEmpty()) {
        return;
    }

     drone::DroneMethodReq *request = _cmqQueue.dequeue();
    if (sendRequest(request)) {
        emit signalSendRequest(false, _replyText);
    } else {
        emit signalSendRequest(true, _errorText);
    }

    delete request;
}

void Controller::slotConnect()
{    
    makeRequest(drone::DroneMethods::Connection);
}

void Controller::slotArm()
{
    makeRequest(drone::DroneMethods::Arm);
}

void Controller::slotDisarm()
{
    makeRequest(drone::DroneMethods::Disarm);
}

void Controller::slotTakeoff()
{
    makeRequest(drone::DroneMethods::Takeoff);
}

void Controller::slotLanding()
{
    makeRequest(drone::DroneMethods::Landing);
}

void Controller::slotTestBox()
{
    makeRequest(drone::DroneMethods::TestFlyBox);
}

void Controller::slotToUpFly()
{
    makeRequest(drone::DroneMethods::ToUp);
}

void Controller::slotToDownFly()
{
    makeRequest(drone::DroneMethods::ToDown);
}

void Controller::slotToForwardFly()
{
    makeRequest(drone::DroneMethods::ToForward);
}

void Controller::slotToBackFly()
{
    makeRequest(drone::DroneMethods::ToBack);
}

void Controller::slotToRightFly()
{
    makeRequest(drone::DroneMethods::ToRight);
}

void Controller::slotToLeftFly()
{
    makeRequest(drone::DroneMethods::ToLeft);
}

void Controller::slotKeyPressed(const int &key)
{
    switch (key) {
    case Qt::Key_Up:
        makeRequest(drone::DroneMethods::ToForward);
        break;
    case Qt::Key_Down:
        makeRequest(drone::DroneMethods::ToBack);
        break;
    case Qt::Key_Right:
        makeRequest(drone::DroneMethods::ToRight);
        break;
    case Qt::Key_Left:
        makeRequest(drone::DroneMethods::ToLeft);
        break;
    case Qt::Key_PageUp:
        makeRequest(drone::DroneMethods::ToUp);
        break;
    case Qt::Key_PageDown:
        makeRequest(drone::DroneMethods::ToDown);
        break;
    default:
        break;
    }
}

void Controller::slotSetParams(const bool &yaw_is_rate,
                               const float &yaw_or_rate,
                               const float &speed,
                               const int &drivetrain)
{
    _yaw_is_rate = yaw_is_rate;
    _yaw_or_rate = yaw_or_rate;
    _speed = speed;
    _drivetrain = drivetrain;
}
