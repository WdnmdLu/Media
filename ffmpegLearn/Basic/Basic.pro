TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
CONFIG+=console
SOURCES += \
        Decoder.c \
        main.c

INCLUDEPATH += ../SDL2-2.30.5/include
INCLUDEPATH += ../ffmpeg-master-latest-win64-gpl-shared/include


LIBS += -L../SDL2-2.30.5/lib/x64 -lSDL2
LIBS += -L../ffmpeg-master-latest-win64-gpl-shared/lib -lavcodec -lavformat -lavutil -lswresample -lswscale -lavdevice

HEADERS += \
    Decoder.h
