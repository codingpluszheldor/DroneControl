#ifndef IMAGESAVER_H
#define IMAGESAVER_H

#include <QObject>
#include <QString>
#include <vlc/vlc.h>

/// <summary>
/// Обработка и сохранения изображений с камеры
/// </summary>
class ImageSaver : public QObject
{
    Q_OBJECT

private:
    QString _fileImagesPath { "D:/Documents/AirSim/ClientRecording/image_" };
    libvlc_instance_t *_vlc_inst { nullptr };
    libvlc_media_t *_media { nullptr };
    libvlc_media_player_t *_mediaPlayer { nullptr };

public:
    explicit ImageSaver(QObject *parent = nullptr);
    ~ImageSaver();

private:

public slots:
    /// <summary>
    /// Создание запросов к дрону
    /// </summary>
    /// <param name="buffer">Кадр</param>
    void slotSave(const QByteArray &buffer);
};


#endif // IMAGESAVER_H
