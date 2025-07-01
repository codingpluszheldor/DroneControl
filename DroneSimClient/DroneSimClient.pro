QT       += core gui concurrent
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ../AirLib/include

SOURCES += \
    Application/application.cpp \
    Controller/controller.cpp \
    ImageServer/imageserver.cpp \
    MainWindow/mainwindow.cpp \
    MjpegStreamer/mjpegstreamer.cpp \
    main.cpp

HEADERS += \
    ../ControllDroneServer/DroneRpc.hpp \
    Application/application.h \
    Controller/controller.h \
    ImageServer/imageserver.h \
    MainWindow/mainwindow.h \
    MjpegStreamer/mjpegstreamer.h

FORMS += \
    MainWindow/mainwindow.ui

win32 {
    INCLUDEPATH += $$PWD/../../../msys64/ucrt64/include/nng
    INCLUDEPATH += $$PWD/../../../msys64/ucrt64/include

    LIBS += c:/msys64/ucrt64/lib/libnng.dll.a
    LIBS += c:/Windows/System32/msvcrt.dll
    LIBS += -lws2_32

} unix {
    LIBS += -lnanomsg
}
