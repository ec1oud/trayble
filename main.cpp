/****************************************************************************
**
** Copyright (C) 2018 Shawn Rutledge
**
** This file is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** version 3 as published by the Free Software Foundation
** and appearing in the file LICENSE included in the packaging
** of this file.
**
** This code is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
****************************************************************************/

#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include "trayicon.h"
#include "trayble.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationDomain(QLatin1String("ecloud.org"));
    app.setApplicationName(QLatin1String("TrayBLE"));

    // TODO maybe #ifdef QT_NO_SYSTEMTRAYICON ...
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, QApplication::applicationName(),
                              TrayIcon::tr("System tray unavailable"));
        return 1;
    }
    QApplication::setQuitOnLastWindowClosed(false);

    TrayBle trayBle;
    TrayIcon trayIcon(trayBle.settings());

    QObject::connect(&trayBle, &TrayBle::readingUpdated,
                     &trayIcon, &TrayIcon::showReading);
    QObject::connect(&trayBle, &TrayBle::error,
                     &trayIcon, &TrayIcon::showError);
    QObject::connect(&trayBle, &TrayBle::statusChanged,
                     &trayIcon, &TrayIcon::showTooltip);
    QObject::connect(&trayBle, SIGNAL(notify(QString,QString)),
                     &trayIcon, SLOT(showMessage(QString,QString)));

//    connect(trayIcon, &QSystemTrayIcon::messageClicked, &trayBle, &trayBle::messageClicked);
//    connect(trayIcon, &QSystemTrayIcon::activated, &trayBle, &trayBle::iconActivated);

    trayIcon.show();
    trayBle.deviceSearch();
    return app.exec();
}
