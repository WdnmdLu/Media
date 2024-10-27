QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    player.cpp \
    queue.cpp \
    widget.cpp

HEADERS += \
    player.h \
    queue.h \
    widget.h

FORMS += \
    widget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


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
