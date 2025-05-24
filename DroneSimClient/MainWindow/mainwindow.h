#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"
#include "Controller/controller.h"


class MainWindow : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT

private:
    QSharedPointer<QTimer> _timer;
    int _image_counter = 0;
    QString _fileImagesPath = "D:/Documents/AirSim/ClientRecording/image_";

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    /// <summary>
    /// Передача в UI указателя на контроллер
    /// </summary>
    /// <param name="controller">Указатель на контролер</param>
    void setController(Controller *controller);

public slots:
    /// <summary>
    /// Добавление в UI строки выполнения команды
    /// </summary>
    /// <param name="isError">Флаг ошибки</param>
    /// <param name="text">Строка лога</param>
    void slotAddLogText(const bool &isError, const QString &text);

    /// <summary>
    /// Отображение данных барометра
    /// </summary>
    void slotBarometerSensorData(const BarometerSensorDataRep &data);

    /// <summary>
    /// Отображение данных ИНС
    /// </summary>
    void slotImuSensorData(const ImuSensorDataRep &data);

    /// <summary>
    /// Отображение данных GPS
    /// </summary>
    void slotGpsSensorData(const GpsSensorDataRep &data);

    /// <summary>
    /// Отображение данных компаса
    /// </summary>
    void slotMagnetometerSensorData(const MagnetometerSensorDataRep &data);

    /// <summary>
    /// Сигнал отправляет изображение, принятое через nanomsg
    /// </summary>
    void slotReceivedImageData(const QByteArray &buffer);

private slots:
    /// <summary>
    /// Обновление параметров в контроллере
    /// </summary>
    void slotTimeOut();

protected:
    /// <summary>
    /// События от клавиатуры
    /// </summary>
    void keyPressEvent(QKeyEvent* event) override;

signals:
    /// <summary>
    /// Обработка нажатий кнопок на форме
    /// </summary>
    /// <param name="cmd">Команда по нажатой кнопке</param>
    void signalBtnCmd(const int &cmd);

    /// <summary>
    /// Обработка нажатий клавишь
    /// </summary>
    /// <param name="key">Код нажатой клавиши</param>
    void signalKeyPressed(const int &key);

    /// <summary>
    /// Установка параметров запроса
    /// </summary>
    void signalSetParams(const bool &yaw_is_rate,
                         const float &yaw_or_rate,
                         const float &speed,
                         const int &drivetrain,
                         const bool &get_image,
                         const int &camera);

    /// <summary>
    /// Установка параметров сохранения
    /// </summary>
    void signalSetSaveParams(const bool &save_images, const bool &save_sensors_data);
};

#endif // MAINWINDOW_H
