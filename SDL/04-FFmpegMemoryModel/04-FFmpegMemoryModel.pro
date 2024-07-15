TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        avpacket.c \
        main.c
INCLUDEPATH += ../ffmpeg-master-latest-win64-gpl-shared/include

LIBS += -L../ffmpeg-master-latest-win64-gpl-shared/lib
LIBS += -lavformat -lavcodec -lavutil

HEADERS += \
    avpacket.h
