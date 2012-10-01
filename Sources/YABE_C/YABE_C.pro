TEMPLATE = app
CONFIG += console
CONFIG -= qt

QMAKE_CFLAGS += -std=c99

SOURCES += main.c \
    yabe.c

HEADERS += \
    yabe.h \
    PrintHex.h

OTHER_FILES +=

