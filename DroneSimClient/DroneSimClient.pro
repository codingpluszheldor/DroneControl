QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += c:/msys64/ucrt64/include/nng
INCLUDEPATH += ../AirLib/include

SOURCES += \
    Application/application.cpp \
    Controller/controller.cpp \
    MainWindow/mainwindow.cpp \
    main.cpp

HEADERS += \
    ../ControllDroneServer/DroneRpc.hpp \
    Application/application.h \
    Controller/controller.h \
    MainWindow/mainwindow.h

FORMS += \
    MainWindow/mainwindow.ui

LIBS += c:/msys64/ucrt64/lib/libnng.dll.a
