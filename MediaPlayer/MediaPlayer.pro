QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

SOURCES += \
    Test.cpp \
    main.cpp \
    mediacore.cpp \
    pktQueue.cpp \
    sdlshow.cpp \
    widget.cpp

HEADERS += \
    mediacore.h \
    sdlshow.h \
    widget.h

FORMS += \
    widget.ui
INCLUDEPATH += ../SDL2-2.30.5/include
LIBS += ../SDL2-2.30.5/lib/x64/SDL2.lib


INCLUDEPATH += ../ffmpeg-master-latest-win64-gpl-shared/include
LIBS += -L../ffmpeg-master-latest-win64-gpl-shared/lib
LIBS += ../ffmpeg-master-latest-win64-gpl-shared/lib/avcodec.lib \
        ../ffmpeg-master-latest-win64-gpl-shared/lib/avfilter.lib \
        ../ffmpeg-master-latest-win64-gpl-shared/lib/avformat.lib \
        ../ffmpeg-master-latest-win64-gpl-shared/lib/avutil.lib \
        ../ffmpeg-master-latest-win64-gpl-shared/lib/postproc.lib \
        ../ffmpeg-master-latest-win64-gpl-shared/lib/swresample.lib \
        ../ffmpeg-master-latest-win64-gpl-shared/lib/swscale.lib \
        ../ffmpeg-master-latest-win64-gpl-shared/lib/avdevice.lib
