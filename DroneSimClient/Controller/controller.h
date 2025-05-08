#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QDebug>
#include <QQueue>
#include <QTimer>
#include <QSharedPointer>
#include "../ControllDroneServer/DroneRpc.hpp"


/// <summary>
/// Контроллер приложения
/// </summary>
class Controller : public QObject
{
    Q_OBJECT

private:
    int _clientSock = -1;
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
        {drone::DroneMethods::ToBack, "ToBack"}
    };
    QSharedPointer<QTimer> _timer;

    // Параметры запроса из Ui
    bool _yaw_is_rate = true;
    float _yaw_or_rate = 0.0f; // угол рысканья
    float _speed = 5.0f; // скорость
    int _drivetrain = 1;

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

public slots:
    /// <summary>
    /// Запрос на соединение с дроном
    /// </summary>
    void slotConnect();

    /// <summary>
    /// Запрос на включение дрона
    /// </summary>
    void slotArm();

    /// <summary>
    /// Запрос на выключение дрона
    /// </summary>
    void slotDisarm();

    /// <summary>
    /// Запрос на взлёт
    /// </summary>
    void slotTakeoff();

    /// <summary>
    /// Запрос на посадку
    /// </summary>
    void slotLanding();

    /// <summary>
    /// Запрос на тестовый полёт
    /// </summary>
    void slotTestBox();

    /// <summary>
    /// Запрос на полёт вверк
    /// </summary>
    void slotToUpFly();

    /// <summary>
    /// Запрос на полёт вверк
    /// </summary>
    void slotToDownFly();

    /// <summary>
    /// Запрос на полёт вперёд
    /// </summary>
    void slotToForwardFly();

    /// <summary>
    /// Запрос на полёт назад
    /// </summary>
    void slotToBackFly();

    /// <summary>
    /// Запрос на полёт вправо
    /// </summary>
    void slotToRightFly();

    /// <summary>
    /// Запрос на полёт влево
    /// </summary>
    void slotToLeftFly();

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
                       const int &drivetrain);

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
};


#endif // CONTROLLER_H
