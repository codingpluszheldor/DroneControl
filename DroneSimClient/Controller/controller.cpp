#include <QDateTime>
#include <compat/nanomsg/nn.h>
#include <compat/nanomsg/reqrep.h>
#include "controller.h"

Controller::Controller(QObject *parent)
    : QObject(parent)
{
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
        _errorText = "Ошибка отправки сообщения";
        return false;
    } else {
        _replyText = "Сообщение успешно отправлено!";

        // Ожидание ответа от сервера
        char buffer[drone::MSG_BUFFER_SIZE];
        int recvResult = nn_recv(_clientSock, buffer, sizeof(buffer), 0);
        if (recvResult > 0){
            _replyText = "Полученный ответ от сервера";
        } else {
            _errorText = "Ошибка приема ответа";
            return false;
        }
    }

    return true;
}

void Controller::slotConnect()
{
    drone::DroneMethodReq *request = new drone::DroneMethodReq;
    request->method = drone::DroneMethods::Connection;
    request->time_point = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();

    if (sendRequest(request)) {
        emit signalSendRequest(false, _replyText);
    } else {
        emit signalSendRequest(true, _errorText);
    }

    delete request;
}
