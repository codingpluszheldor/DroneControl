#ifndef IMAGE_SERVER_H
#define IMAGE_SERVER_H

#include <QObject>
#include <QString>
#include <QSharedPointer>
#include <QFuture>
#include <QPoint>
#include <QSize>
#include <atomic>
#include "MjpegStreamer/mjpegstreamer.h"

// --- Структуры форматов ---

/*
 * ФОРМАТ JPEG (Joint Photographic Experts Group)
 * JPEG - это метод сжатия изображений с потерями. Файлы, использующие JPEG-сжатие,
 * обычно соответствуют стандартам JFIF или Exif.
 *
 * Структура JPEG файла включает маркеры (последовательности байтов, начинающиеся с FF):
 * - SOI (Start of Image)          : FF D8 (начало файла)
 * - APP (Application Segments)    : FF E0 - FF EF (метаданные, например, Exif)
 * - DQT (Define Quantization Table): FF DB (таблицы квантования)
 * - DHT (Define Huffman Table)    : FF C4 (таблицы Хаффмана)
 * - SOF (Start of Frame)          : FF C0 (или другие FF Cx) (параметры изображения: ширина, высота, компоненты)
 * - SOS (Start of Scan)           : FF DA (начало сжатых данных изображения)
 *    [СЖАТЫЕ ДАННЫЕ ИЗОБРАЖЕНИЯ]
 * - EOI (End of Image)            : FF D9 (конец файла)
 *
 * Qt's QImage::load() и QImage::save() абстрагируют эту сложность, позволяя
 * работать с JPEG файлами на высоком уровне.
 */

/*
 * ФОРМАТ MJPEG (Motion JPEG) для HTTP-потока
 * MJPEG - это серия отдельных JPEG-изображений, передаваемых как видеопоток.
 * Часто используется для HTTP-потока типа `multipart/x-mixed-replace`.
 *
 * Структура одного "MJPEG-кадра" в HTTP-потоке:
 * 1. Разделитель (Boundary String):
 *     Например: "--mycustomboundarystring\r\n"
 *     Используется для разделения отдельных JPEG-кадров в потоке.
 *     Это строка, указанная в заголовке Content-Type основного HTTP-ответа.
 *
 * 2. Заголовок Content-Type для кадра:
 *     Например: "Content-Type: image/jpeg\r\n"
 *     Указывает, что следующая часть данных - это JPEG-изображение.
 *
 * 3. Заголовок Content-Length для кадра:
 *     Например: "Content-Length: 12345\r\n"
 *     Указывает точный размер JPEG-данных, которые следуют за заголовками.
 *
 * 4. Пустая строка:
 *     "\r\n"
 *     Разделяет заголовки кадра от его бинарных данных.
 *
 * 5. Бинарные данные JPEG:
 *     Собственно, сам JPEG-файл.
 */


/// <summary>
/// Обработка, сохранения и отдача изображений с камеры
/// </summary>
class ImageServer : public QObject
{
    Q_OBJECT

private:
    QString _fileImagesPath { "D:/Documents/AirSim/ClientRecording/image_" };
    QSharedPointer<MjpegStreamer> _mjpegStreamer;
    std::atomic<bool> _isConnectedToAi {false};
    std::atomic<bool> _isStarted {false};
    QFuture<void> _futureConnect; // результат соединения
    QFuture<void> _futureSendImage; // результат отправки изображения
    QFuture<void> _futureResponse; // результат обработки изображения

public:
    explicit ImageServer(QObject *parent = nullptr);
    ~ImageServer();

private:
    /// <summary>
    /// Соединение с сервисом AI
    /// </summary>
    void connectToAi();

    /// <summary>
    /// Отправка изображения в сервис AI
    /// </summary>
    void sendImageToAi(const QByteArray &buffer);

    /// <summary>
    /// Приём ответов от AI
    /// </summary>
    void responseFromAi();

public slots:
    /// <summary>
    /// Создание запросов к дрону
    /// </summary>
    /// <param name="buffer">Кадр</param>
    void slotSave(const QByteArray &buffer);

signals:
    /// <summary>
    /// Сигнал отправляет изображение для отображения
    /// </summary>
    void signalShowImage(const QByteArray &buffer);

    /// <summary>
    /// Сигнал отправляет данные от AI
    /// </summary>
    void signalAiDataResponse(const QPoint &obj,
                              const QPoint &center,
                              const QSize  &size,
                              const double &polar_r,
                              const double &polar_theta);
};


#endif // IMAGE_SERVER_H
