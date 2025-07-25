#include <QDateTime>
#include <QtConcurrent>
#include <compat/nanomsg/nn.h>
#include <compat/nanomsg/reqrep.h>
#include <compat/nanomsg/pipeline.h>
#include "controller.h"

Controller::Controller(QObject *parent)
    : QObject(parent)
{
    using namespace drone;
    qRegisterMetaType<BarometerSensorDataRep>("BarometerSensorDataRep");
    qRegisterMetaType<ImuSensorDataRep>("ImuSensorDataRep");
    qRegisterMetaType<GpsSensorDataRep>("GpsSensorDataRep");
    qRegisterMetaType<MagnetometerSensorDataRep>("MagnetometerSensorDataRep");

    _timer = QSharedPointer<QTimer>(new QTimer(this));
    connect(_timer.data(), &QTimer::timeout, this, &Controller::slotTimeOut);
    _timer->start(50);
}

Controller::~Controller()
{
     _isStarted = false;
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

    if (!_future.isRunning()) {
        _future = QtConcurrent::run(this, &Controller::cameraImageLoop);
        _isStarted = true;
        qDebug() << "Start cameraImageLoop";
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
        char buffer[drone::MSG_BUFFER_SIZE] = { 0 };
        int recvResult = nn_recv(_clientSock, buffer, sizeof(buffer), 0);
        drone::DroneReply *reply = reinterpret_cast<drone::DroneReply*>(buffer);
        if (recvResult > 0 && reply != nullptr) {
            _replyText = QString("<-- [%1] Получен ответ от сервера").arg(_methodNames.value(reply->method));
            emit signalBarometerSensorData(reply->barometer);
            emit signalImuSensorData(reply->imu);
            if (reply->gps.is_valid) {
                emit signalGpsSensorData(reply->gps);
            }
            emit signalMagnetometerSensorData(reply->magnetometer);
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
    request->get_camera_image = _get_image;
    request->camera = _camera;

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

void Controller::slotBtnCmd(const int &cmd)
{
    drone::DroneMethods method = static_cast<drone::DroneMethods>(cmd);
    makeRequest(method);
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
    case Qt::Key_Insert:
        makeRequest(drone::DroneMethods::RotateLeft);
        break;
    case Qt::Key_Delete:
        makeRequest(drone::DroneMethods::RotateRight);
        break;
    default:
        break;
    }
}

void Controller::slotSetParams(const bool &yaw_is_rate,
                               const float &yaw_or_rate,
                               const float &speed,
                               const int &drivetrain,
                               const bool &get_image,
                               const int &camera)
{
    _yaw_is_rate = yaw_is_rate;
    _yaw_or_rate = yaw_or_rate;
    _speed = speed;
    _drivetrain = drivetrain;
    _get_image = get_image;
    _camera = static_cast<DroneCamera>(camera);
}

void Controller::cameraImageLoop()
{
    _serverSock = nn_socket(AF_SP, NN_PULL);
    if (_serverSock < 0) {
        qDebug() << "Ошибка инициализации socket";
        return;
    }

    if (nn_bind(_serverSock, "tcp://127.0.0.1:20002") < 0) {
        qDebug() << "Ошибка bind socket";
        return;
    }

    int to = 100;
    if (nn_setsockopt(_serverSock, NN_SOL_SOCKET, NN_RCVTIMEO, &to, sizeof(to)) < 0) {
        qDebug() << "Ошибка set socket options";
    }

    qDebug() << "Приём от камеры.........";
    while (_isStarted)
    {
        char *buf = NULL;
        int bytes = nn_recv(_serverSock, &buf, NN_MSG, 0);
        if (bytes > 0) {
            if (_isStarted) {
                QByteArray buffer(buf, bytes);

                // В GUI
                emit signalReceivedImageData(buffer);
                // В vlc stream
                if (_save_images) {
                    emit signalSaveImage(buffer);
                }
            }
            nn_freemsg(buf);
        }
        else if (bytes == 0) {
            nn_freemsg(buf);
        }
    }
    qDebug() << "Окончание приёма от камеры.........";
}

void Controller::slotSetSaveParams(const bool &save_images, const bool &save_sensors_data)
{
    _save_images = save_images;
    _save_sensors_data = save_sensors_data;
}

void Controller::slotAiDataResponse(const QPoint &obj,
                                    const QPoint &center,
                                    const QSize  &size,
                                    const double &polar_r,
                                    const double &polar_theta)
{
    qDebug() << "Коррекция на центр < > /\\ \\/...., размер:" << size;

    Q_UNUSED(polar_r);
    Q_UNUSED(polar_theta);

    constexpr int delta_x = 40;
    constexpr int delta_y = 20;

    int res_x = center.x() - obj.x();
    int res_y = center.y() - obj.y();

    if (std::abs(res_x) > delta_x) {
        // Нужен корректирующий поворот
        _yaw_is_rate = true;
        std::abs(res_x) > (delta_x * 3) ? _yaw_or_rate = 10.0f : _yaw_or_rate = 4.0f;
        if (res_x < 0) {
            // Объект слева, поворот влево
            makeRequest(drone::DroneMethods::RotateLeft);
            qDebug() << "<=== Влево, delta:" << std::abs(res_x);
        } else if (res_x > 0) {
            // Объект справа, поворот вправо
            makeRequest(drone::DroneMethods::RotateRight);
            qDebug() << "===> Вправо, delta:" << std::abs(res_x);
        }
    } else if (std::abs(res_y) > delta_y) {
        // Нужен корректирующий спуск/подъём
        if (res_y > 0) {
            // Объект сверху, подъём
            makeRequest(drone::DroneMethods::ToUp);
            qDebug() << "/ \\";
            qDebug() << " |";
            qDebug() << " | Вверх";
        } else if (res_y < 0) {
            // Объект снизу, спуск
            makeRequest(drone::DroneMethods::ToDown);
            qDebug() << " |";
            qDebug() << " |";
            qDebug() << "\\ / Вниз";
        }
    }
}
