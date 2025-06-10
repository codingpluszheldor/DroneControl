#ifndef MJPEGSTREAMER_H
#define MJPEGSTREAMER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QImage>
#include <QBuffer>
#include <QTimer>
#include <QDebug>
#include <QList>

/// <summary>
/// Класс для обработки MJPEG потока
/// </summary>
class MjpegStreamer : public QObject
{
    Q_OBJECT

private:
    QTcpServer *_tcpServer;              // TCP сервер для прослушивания подключений
    QList<QTcpSocket*> _clientSockets;   // Список всех подключенных клиентов
    quint16 _port { 8000 };              // Порт сервера

    // Уникальная строка-разделитель для MJPEG потока. Должна быть сложной, чтобы не встречаться в данных.
    const QString _boundary = "----QtMjpegBoundaryString123456789ABCDEF----";

public:
    MjpegStreamer(quint16 port, QObject* parent = nullptr);

    ~MjpegStreamer();

    /// <summary>
    /// Запуск TCP сервера для прослушки входящих подключений
    /// </summary>
    bool startServer();

public slots:
    /// <summary>
    /// Отправка следующего кадра jpg
    /// </summary>
    void slotNextFrame(const QByteArray &jpegData);

private slots:
    /// <summary>
    /// Обработка нового входящего соединения
    /// </summary>
    void slotNewConnection();

    /// <summary>
    /// Обработка отключения клиента
    /// </summary>
    void slotClientDisconnected();

    /// <summary>
    /// Обработка HTTP GET запрос от клиента (например, VLC)
    /// </summary>
    void slotClientReadyRead();

private:
    /// <summary>
    /// Отправка начальных HTTP заголовков MJPEG потока клиенту
    /// </summary>
    void sendInitialHeaders(QTcpSocket* socket);

    /// <summary>
    /// Отправка JPEG-кадра клиенту, включая его собственные заголовки
    /// </summary>
    void sendJpegFrame(QTcpSocket* socket, const QByteArray& jpegData);
};


#endif // MJPEGSTREAMER_H
