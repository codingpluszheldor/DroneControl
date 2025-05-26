
#include <QFile>
#include <QDebug>
#include <QDateTime>
#include <QImage>
#include <QBuffer>
#include <QPixmap>
#include <QTemporaryFile>
#include "imagesaver.h"

// Настройки RTSP-сервера
const char* RTSP_URL = "rtsp://127.0.0.1:554/live.sdp";

ImageSaver::ImageSaver(QObject *parent)
    : QObject(parent)
{
    // Инициализация libvlc
    _vlc_inst = libvlc_new(0, nullptr);
    if (!_vlc_inst) {
        qCritical() << "Failed to create libvlc instance.";
        return;
    }
}

ImageSaver::~ImageSaver()
{
    if (_mediaPlayer) libvlc_media_player_release(_mediaPlayer);
    if (_media) libvlc_media_release(_media);
    if (_vlc_inst) libvlc_release(_vlc_inst);
}

void ImageSaver::slotSave(const QByteArray &buffer)
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

    QFile file(_fileImagesPath + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".jpg");
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open the image file.";
        return;
    }

    file.write(baJpeg);
    file.flush();
    file.close();

    QTemporaryFile tempFile;
    tempFile.open();
    tempFile.write(baJpeg);
    tempFile.flush();

    // Создаем медиасущность из временного файла.

    if (_media == nullptr) {
        _media = libvlc_media_new_path(_vlc_inst, "d:/Documents/AirSim/ClientRecording/image_1748212791880.jpg");
        if (!_media) {
            qWarning() << "Error creating media object!";
            return;
        }

        // Настраиваем MRL для RTSP-стриминга.
        QString soutOption(":sout=#rtp{sdp=rtsp://@:8554/example}");
        libvlc_media_add_option(_media, soutOption.toUtf8().constData());

        _mediaPlayer = libvlc_media_player_new_from_media(_media);
        if (!_mediaPlayer) {
            qWarning() << "Error creating media player!";
            return;
        }

        libvlc_media_player_play(_mediaPlayer);
    } else {
        libvlc_media_release(_media);
        _media = libvlc_media_new_path(_vlc_inst, tempFile.fileName().toStdString().c_str());
        libvlc_media_add_option(_media, ":sout=#rtp{sdp=rtsp://@:8554/example}");
        libvlc_media_player_set_media(_mediaPlayer, _media);
        libvlc_media_player_play(_mediaPlayer);
    }
}

