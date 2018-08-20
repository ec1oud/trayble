TEMPLATE = app
TARGET = trayble

QT += widgets bluetooth svg network

HEADERS += trayble.h \
    trayicon.h

SOURCES += trayble.cpp \
    main.cpp \
    trayicon.cpp

RESOURCES += \
    resources.qrc
