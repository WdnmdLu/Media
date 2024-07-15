TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.c


INCLUDEPATH += $$PWD/SDL2-2.30.5/include
LIBS += $$PWD/SDL2-2.30.5/lib/x64/SDL2.lib

