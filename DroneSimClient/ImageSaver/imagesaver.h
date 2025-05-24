#ifndef IMAGESAVER_H
#define IMAGESAVER_H

#include <QObject>
#include <QString>


/// <summary>
/// Обработка и сохранения изображений с камеры
/// </summary>
class ImageSaver : public QObject
{
    Q_OBJECT

private:
    QString _fileImagesPath = "D:/Documents/AirSim/ClientRecording/image_";

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
