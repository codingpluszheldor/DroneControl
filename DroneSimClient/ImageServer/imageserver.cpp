
#include <QFile>
#include <QDebug>
#include <QDateTime>
#include <QImage>
#include <QBuffer>
#include <QPixmap>
#include <QTemporaryFile>
#include <QtConcurrent>
#include <asio.hpp>
#include <nlohmann/json.hpp>
#include "imageserver.h"

using namespace std::literals::chrono_literals;
using json = nlohmann::json;

// ASIO io_context
asio::io_context ctxAsio;
// Сокет на отправку в AI
asio::ip::tcp::socket *socketAsio = nullptr;

const uint8_t MAGIC[] = { 'V', '1' };
const asio::ip::tcp::endpoint endpoint(asio::ip::make_address("192.168.255.12"), 10000);
//const asio::ip::tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 10000);

void sendFrame(const QByteArray &buffer, asio::ip::tcp::socket &socket)
{
    auto len_bytes = htonl(buffer.size());

    // Отправка magic bytes, длина кадра и сам кадр
    asio::write(socket, asio::buffer(MAGIC));
    asio::write(socket, asio::buffer(&len_bytes, sizeof(uint32_t)));
    asio::write(socket, asio::buffer(buffer.data(), buffer.size()));
}

std::pair<bool, json> receiveResponse(asio::ip::tcp::socket &socket)
{
    uint32_t response_length;
    char raw_data[sizeof(response_length)];

    // Чтение 4 bytes поля длины
    asio::read(socket, asio::buffer(raw_data, sizeof(response_length)));
    response_length = ntohl(*reinterpret_cast<const uint32_t*>(raw_data));

    // Буфер под ответ JSON payload
    std::vector<char> response_buffer(response_length);
    asio::read(socket, asio::buffer(response_buffer.data(), response_length));

    return {true, json::parse(response_buffer.begin(), response_buffer.end())};
}

ImageServer::ImageServer(QObject *parent)
    : QObject(parent)
{
    // Порт для видео сервера
    const quint16 streamPort = 8000;
    _mjpegStreamer = QSharedPointer<MjpegStreamer>(new MjpegStreamer(streamPort));
    connect(this, &ImageServer::signalShowImage, _mjpegStreamer.data(), &MjpegStreamer::slotNextFrame);
    _mjpegStreamer-> startServer();
}

ImageServer::~ImageServer()
{
    _isStarted = false;
    if (socketAsio != nullptr) {
        socketAsio->close();
        delete socketAsio;
    }
}

void ImageServer::slotSave(const QByteArray &buffer)
{
    if (!_futureConnect.isRunning() && !_isConnectedToAi) {
        _futureConnect = QtConcurrent::run(this, &ImageServer::connectToAi);
        qDebug() << "Попытка соединения с AI сервисом";
    }

    if (_isConnectedToAi && !_futureSendImage.isRunning()) {
        _futureSendImage = QtConcurrent::run(this, &ImageServer::sendImageToAi, buffer);
        _isStarted = true;
        qDebug() << "Отправка изображения в AI сервис";
    }

    if (_isConnectedToAi && !_futureResponse.isRunning()) {
        _futureResponse = QtConcurrent::run(this, &ImageServer::responseFromAi);
        qDebug() << "Запуск приёма json от AI сервиса";
    }

    QPixmap pixmap;
    pixmap.loadFromData(buffer);
    QImage image = pixmap.toImage();

    // Создаем массив байтов для хранения результата в формате JPEG
    QByteArray baJpeg;
    QBuffer qbuf(&baJpeg);
    qbuf.open(QIODevice::WriteOnly);

    // Сохраняем изображение в буфер в формате JPEG
    bool success = image.save(&qbuf, "JPEG");
    if (!success) {
        qWarning() << "Ошибка сохранения изображения в JPEG";
        return;
    }

    // Отправка кадра в видео поток
    emit signalShowImage(baJpeg);

#ifdef SAVE_IMAGES
    QFile file(_fileImagesPath + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".jpg");
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Ошибка открытия файла изображения для сохранения";
        return;
    }

    file.write(baJpeg);
    file.flush();
    file.close();
#endif
}

void ImageServer::connectToAi()
{
    _isConnectedToAi = false;
    socketAsio = new asio::ip::tcp::socket(ctxAsio);
    socketAsio->connect(endpoint);
    _isConnectedToAi = true;
    qDebug() << "Успешное соединение с AI сервисом";
}

void ImageServer::sendImageToAi(const QByteArray &buffer)
{
    // В сокет AI
    sendFrame(buffer, *socketAsio);    
}

void ImageServer::responseFromAi()
{
    while (_isStarted) {
        // Ожидание ответа от AI
        auto [ok, data] = receiveResponse(*socketAsio);
        if (ok && !data.empty()) {
            // int width = data["image_size"]["width_px"];
            // int height = data["image_size"]["height_px"];
            double center_x = data["center_px"]["x"];
            double center_y = data["center_px"]["y"];
            double object_x = data["object_px"]["x"];
            double object_y = data["object_px"]["y"];
            double polar_r = data["polar_coordinates"]["r_px"];
            double polar_theta = data["polar_coordinates"]["theta_deg"];

            qDebug() << "Ответ от AI";
            emit signalAiDataResponse(QPoint(object_x, object_y),
                                      QPoint(center_x, center_y),
                                      polar_r,
                                      polar_theta);
        }
    }
}
