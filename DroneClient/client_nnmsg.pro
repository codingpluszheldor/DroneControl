TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp


INCLUDEPATH += c:/msys64/ucrt64/include/nng
LIBS += c:/msys64/ucrt64/lib/libnng.dll.a

