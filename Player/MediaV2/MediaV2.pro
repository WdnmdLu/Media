QT += core gui widgets opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += openglwidgets widgets

CONFIG += c++11

QT += opengl

SOURCES += \
    main.cpp \
    openglwidget.cpp \
    player.cpp \
    queue.cpp \
    widget.cpp

HEADERS += \
    openglwidget.h \
    player.h \
    queue.h \
    widget.h

FORMS += \
    widget.ui

INCLUDEPATH += ../SDL2-2.30.5/include
LIBS += ../SDL2-2.30.5/lib/x64/SDL2.lib

LIBS += -lopengl32 -lglu32

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

DISTFILES += \
    ../MediaV2-Build/fragment.frag \
    ../MediaV2-Build/vertex.vert \
    ../build-MediaV2/fragment.frag
