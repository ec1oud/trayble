TEMPLATE = app
TARGET = trayble

QT += widgets bluetooth svg network
CONFIG += debug

HEADERS += trayble.h \
    trayicon.h \
    userdialog.h

SOURCES += trayble.cpp \
    main.cpp \
    trayicon.cpp \
    userdialog.cpp

RESOURCES += \
    resources.qrc

FORMS += \
    userdialog.ui
