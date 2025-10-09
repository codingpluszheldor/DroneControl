TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp


win32 {
    INCLUDEPATH += $$PWD/../../../msys64/ucrt64/include
    INCLUDEPATH += $$PWD/../../../msys64/ucrt64/include/opencv4

    LIBS += -Lc:/msys64/ucrt64/bin \
    -lopencv_core-412 \
    -lopencv_imgproc-412 \
    -lopencv_highgui-412 \
    -lopencv_videoio-412 \
    -lopencv_calib3d-412 \
    -lopencv_imgcodecs-412

} unix {

}
