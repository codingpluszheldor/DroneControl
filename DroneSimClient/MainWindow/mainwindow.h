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
                         const int &drivetrain);
};

#endif // MAINWINDOW_H
