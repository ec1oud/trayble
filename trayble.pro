TEMPLATE = app
TARGET = trayble

QT += widgets bluetooth svg network
CONFIG += debug

HEADERS += trayble.h \
    trayicon.h

SOURCES += trayble.cpp \
    main.cpp \
    trayicon.cpp

RESOURCES += \
    resources.qrc
