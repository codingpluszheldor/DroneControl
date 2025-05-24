#include <fstream>
#include <QFile>
#include <QDateTime>
#include "imagesaver.h"


ImageSaver::ImageSaver(QObject *parent)
    : QObject(parent)
{
}

ImageSaver::~ImageSaver()
{
}

void ImageSaver::slotSave(const QByteArray &buffer)
{
    // QFile file(fileImagesPath + QString::number(QDateTime::currentSecsSinceEpoch()) + ".png");
    // if (!file.open(QIODevice::WriteOnly)) {
    //     qWarning() << "Failed to open the image file.";
    //     continue;
    // }

    // file.write(buffer);
    // file.flush();
    // file.close();
    std::string file_path = (_fileImagesPath + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".png").toStdString();
    std::ofstream file(file_path, std::ios::binary);
    file.write(buffer.data(), buffer.size());
    file.close();
}
