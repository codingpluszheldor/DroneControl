#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"
#include "Controller/controller.h"


class MainWindow : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT

private:
//    Controller *_controller = nullptr;

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

};

#endif // MAINWINDOW_H
