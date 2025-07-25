#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QDebug>
#include <QQueue>
#include <QTimer>
#include <QFuture>
#include <QSharedPointer>
#include "../ControllDroneServer/DroneRpc.hpp"

using namespace drone;

/// <summary>
/// Контроллер приложения
/// </summary>
class Controller : public QObject
{
    Q_OBJECT

private:
    int _clientSock = -1;
    int _serverSock = -1;
    QString _errorText;
    QString _requestText;
    QString _replyText;
    QQueue<drone::DroneMethodReq*> _cmqQueue;
    drone::DroneMethods _lastCmd = drone::DroneMethods::Wait;
    QMap<drone::DroneMethods, QString> _methodNames = {
        {drone::DroneMethods::Connection, "Connection"},
        {drone::DroneMethods::Arm, "Arm"},
        {drone::DroneMethods::Disarm, "Disarm"},
        {drone::DroneMethods::Takeoff, "Takeoff"},
        {drone::DroneMethods::Landing, "Landing"},
        {drone::DroneMethods::TestFlyBox, "TestFlyBox"},
        {drone::DroneMethods::BarometerData, "BarometerData"},
        {drone::DroneMethods::ImuData, "ImuData"},
        {drone::DroneMethods::GpsData, "GpsData"},
        {drone::DroneMethods::MagnetometerData, "MagnetometerData"},
        {drone::DroneMethods::ToUp, "ToUp"},
        {drone::DroneMethods::ToDown, "ToDown"},
        {drone::DroneMethods::ToRight, "ToRight"},
        {drone::DroneMethods::ToLeft, "ToLeft"},
        {drone::DroneMethods::ToForward, "ToForward"},
        {drone::DroneMethods::ToBack, "ToBack"},
        {drone::DroneMethods::RotateLeft, "RotateLeft"},
        {drone::DroneMethods::RotateRight, "RotateRight"}
    };
    QSharedPointer<QTimer> _timer;
    QFuture<void> _future;      // результат работы потока
    std::atomic<bool> _isStarted {true};

    // Параметры запроса из Ui
    bool _yaw_is_rate = true;
    float _yaw_or_rate = 0.0f; // угол рысканья
    float _speed = 5.0f; // скорость
    int _drivetrain = 1;
    bool _get_image = false;
    DroneCamera _camera = DroneCamera::front_center;
    // Сохранеие  данных с дрона
    bool _save_images = false;
    bool _save_sensors_data = false;    

public:
    explicit Controller(QObject *parent = nullptr);
    ~Controller();

    /// <summary>
    /// Инициализация
    /// </summary>
    /// <returns>Результат инициализации</returns>
    bool setInit();

private:
    /// <summary>
    /// Создание структуры запроса к дрону и постановка в очередь
    /// </summary>
    void makeRequest(const drone::DroneMethods &method);

    /// <summary>
    /// Отправка запроса и удаление из очереди
    /// </summary>
    /// <param name="request">Указатель на запрос к дрону</param>
    /// <returns>Результат выполнения отправки по сети</returns>
    bool sendRequest(drone::DroneMethodReq *request);

    /// <summary>
    /// Цикл приёма изображения от камеры
    /// </summary>
    void cameraImageLoop();

public slots:
    /// <summary>
    /// Создание запросов к дрону
    /// </summary>
    /// <param name="cmd">Команда</param>
    void slotBtnCmd(const int &cmd);

    /// <summary>
    /// Обработка нажатий клавишь
    /// </summary>
    /// <param name="key">Код нажатой клавиши</param>
    void slotKeyPressed(const int &key);

    /// <summary>
    /// Установка параметров запроса
    /// </summary>
    void slotSetParams(const bool &yaw_is_rate,
                       const float &yaw_or_rate,
                       const float &speed,
                       const int &drivetrain,
                       const bool &get_image,
                       const int &camera);

    /// <summary>
    /// Установка параметров сохранения
    /// </summary>
    void slotSetSaveParams(const bool &save_images, const bool &save_sensors_data);

    /// <summary>
    /// Получение данных от AI
    /// </summary>
    void slotAiDataResponse(const QPoint &obj,
                            const QPoint &center,
                            const QSize  &size,
                            const double &polar_r,
                            const double &polar_theta);

private slots:
    /// <summary>
    /// Обработка событый таймера
    /// </summary>
    void slotTimeOut();

signals:
    /// <summary>
    /// Отправка в UI результата выполнения сетевой посылки запроса
    /// </summary>
    /// param name="isError">Флаг ошибки</param>
    /// <param name="text">Строка лога</param>
    void signalSendRequest(const bool &isError, const QString &text);

    /// <summary>
    /// Отправка в UI данных барометра
    /// </summary>
    void signalBarometerSensorData(const BarometerSensorDataRep &data);

    /// <summary>
    /// Отправка в UI данных ИНС
    /// </summary>
    void signalImuSensorData(const ImuSensorDataRep &data);

    /// <summary>
    /// Отправка в UI данных GPS
    /// </summary>
    void signalGpsSensorData(const GpsSensorDataRep &data);

    /// <summary>
    /// Отправка в UI данных компаса
    /// </summary>
    void signalMagnetometerSensorData(const MagnetometerSensorDataRep &data);

    /// <summary>
    /// Сигнал отправляет изображение, принятое через nanomsg
    /// </summary>
    void signalReceivedImageData(const QByteArray &buffer);

    /// <summary>
    /// Сигнал отправляет изображение для сохранения или отображения
    /// </summary>
    void signalSaveImage(const QByteArray &buffer);

};


#endif // CONTROLLER_H
