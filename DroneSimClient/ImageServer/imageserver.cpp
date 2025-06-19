
#include <QFile>
#include <QDebug>
#include <QDateTime>
#include <QImage>
#include <QBuffer>
#include <QPixmap>
#include <QTemporaryFile>
#include "imageserver.h"


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
}

void ImageServer::slotSave(const QByteArray &buffer)
{
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

    QFile file(_fileImagesPath + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".jpg");
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open the image file.";
        return;
    }

    file.write(baJpeg);
    file.flush();
    file.close();
}

