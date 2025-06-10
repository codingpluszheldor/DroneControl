#include <QHostAddress>
#include "mjpegstreamer.h"

MjpegStreamer::MjpegStreamer(quint16 port, QObject* parent)
    : QObject(parent),
    _port(port)
{
    _tcpServer = new QTcpServer(this);
    connect(_tcpServer, &QTcpServer::newConnection, this, &MjpegStreamer::slotNewConnection);
}

MjpegStreamer::~MjpegStreamer()
{

}

bool MjpegStreamer::startServer()
{
    if (!_tcpServer->listen(QHostAddress::Any, _port)) {
        qCritical() << "Ошибка: Не удалось запустить сервер на порту" << _port << ":" << _tcpServer->errorString();
        return false;
    }

    return true;
}

void MjpegStreamer::slotNewConnection()
{
    QTcpSocket* clientSocket = _tcpServer->nextPendingConnection();
    _clientSockets.append(clientSocket); // Добавляем сокет клиента в список активных

    qDebug() << "Новое подключение от:" << clientSocket->peerAddress().toString() << ":" << clientSocket->peerPort();

    connect(clientSocket, &QTcpSocket::disconnected, this, &MjpegStreamer::slotClientDisconnected);
    connect(clientSocket, &QTcpSocket::readyRead, this, &MjpegStreamer::slotClientReadyRead);

    // Отправление начальных HTTP заголовков для MJPEG потока
    sendInitialHeaders(clientSocket);
}

void MjpegStreamer::slotClientDisconnected()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket) {
        qDebug() << "Клиент отключен:" << clientSocket->peerAddress().toString() << ":" << clientSocket->peerPort();
        _clientSockets.removeAll(clientSocket); // Удаление сокета из списка активных
        clientSocket->deleteLater();
    }
}

void MjpegStreamer::slotClientReadyRead()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket) {
        QByteArray request = clientSocket->readAll();
        // VLC при подключении отправляет HTTP GET запрос.
        // Заголовки уже отправлены ранее в slotNewConnection, здесь просто логируем.
        if (request.startsWith("GET")) {
            qDebug() << "Получен HTTP GET запрос от клиента:" << clientSocket->peerAddress().toString();
        }
    }
}

void MjpegStreamer::slotNextFrame(const QByteArray &jpegData)
{
    if (jpegData.isEmpty()) {
        return;
    }

    // Отправляем JPEG-кадр всем подключенным клиентам
    foreach (QTcpSocket* clientSocket, _clientSockets) {
        if (clientSocket->state() == QAbstractSocket::ConnectedState) {
            sendJpegFrame(clientSocket, jpegData);
        }
    }
}

void MjpegStreamer::sendInitialHeaders(QTcpSocket* socket)
{
    // Стандартный HTTP 1.0 ответ для MJPEG потока multipart/x-mixed-replace
    // Важно: строка boundary (разделитель) должна совпадать с той, что используется для каждого кадра.
    QString header = "HTTP/1.0 200 OK\r\n"
                     "Server: QtMjpegStreamer/1.0\r\n"
                     "Cache-Control: no-cache\r\n"
                     "Cache-Control: private\r\n"
                     "Pragma: no-cache\r\n"
                     "Content-Type: multipart/x-mixed-replace; boundary=" + _boundary + "\r\n"
                                  "\r\n"; // Конец начальных HTTP заголовков (обязательная пустая строка)
    socket->write(header.toUtf8());
    socket->flush(); // Гарантируем немедленную отправку заголовков
    qDebug() << "Отправлены начальные HTTP заголовки клиенту.";
}

void MjpegStreamer::sendJpegFrame(QTcpSocket* socket, const QByteArray& jpegData)
{
    // Каждый кадр в MJPEG потоке начинается со своего разделителя и заголовков.
    QString frameHeaders;
    frameHeaders += "--" + _boundary + "\r\n"; // Разделитель для следующего кадра
    frameHeaders += "Content-Type: image/jpeg\r\n"; // Тип содержимого, которое следует (JPEG)
    // Размер данных JPEG для клиента чтобы знать сколько читать.
    frameHeaders += QString("Content-Length: %1\r\n").arg(jpegData.size());
    frameHeaders += "\r\n"; // Конец заголовков кадра (обязательная пустая строка)

    socket->write(frameHeaders.toUtf8()); // Отправляем заголовки кадра
    socket->write(jpegData);              // Отправляем бинарные данные JPEG
    socket->flush(); // Гарантируем немедленную отправку данных
}

