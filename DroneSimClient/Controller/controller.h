#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QDebug>
#include "../DroneServer/DroneRpc.hpp"


/// <summary>
/// Контроллер приложения
/// </summary>
class Controller : public QObject
{
    Q_OBJECT

private:
    int _clientSock = -1;
    QString _errorText;
    QString _replyText;

public:
    explicit Controller(QObject *parent = nullptr);
    ~Controller();

    /// <summary>
    /// Инициализация
    /// </summary>
    /// <returns>Результат инициализации</returns>
    bool setInit();

public slots:
    /// <summary>
    /// Запрос на соединение с дроном
    /// </summary>
    void slotConnect();

private:
    /// <summary>
    /// Отправка запроса
    /// </summary>
    /// <param name="request">Указатель на запрос к дрону</param>
    /// <returns>Результат выполнения отправки по сети</returns>
    bool sendRequest(drone::DroneMethodReq *request);

signals:
    /// <summary>
    /// Отправка в UI результата выполнения сетевой посылки запроса
    /// </summary>
    /// param name="isError">Флаг ошибки</param>
    /// <param name="text">Строка лога</param>
    void signalSendRequest(const bool &isError, const QString &text);
};


#endif // CONTROLLER_H
