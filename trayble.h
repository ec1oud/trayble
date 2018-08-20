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

#ifndef TRAYBLE_H
#define TRAYBLE_H

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSettings>

class TrayBle : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)

public:
    TrayBle();
    ~TrayBle();

    QString status() const;
    void setStatus(QString s);

    void deviceSearch();
    void connectService();
    void disconnectService();
    void sendRequest();

private slots:
    void addDevice(const QBluetoothDeviceInfo&);
    void scanFinished();
    void deviceScanError(QBluetoothDeviceDiscoveryAgent::Error);

    void serviceDiscovered(const QBluetoothUuid &svc);
    void serviceScanDone();
    void controllerError(QLowEnergyController::Error e);
    void deviceConnected();
    void deviceDisconnected();

    void serviceStateChanged(QLowEnergyService::ServiceState s);
    void updateBodyComp(const QLowEnergyCharacteristic &c,
                              const QByteArray &value);
    void serviceError(QLowEnergyService::ServiceError e);

    void networkFinished();
    void networkError(QNetworkReply::NetworkError e);

signals:
    void error(QString message);
    void statusChanged(QString message);
    void weightUpdated(QString person, QString message);

private:
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent;
    QBluetoothDeviceInfo m_device;
    QLowEnergyDescriptor m_notification;
    QLowEnergyController *m_controller;
    QBluetoothUuid m_serviceUuid;
    QLowEnergyService *m_service;
    QString m_status;

    QSettings m_settings;

    QNetworkAccessManager m_nam;
    QNetworkRequest m_influxInsertReq;
    QNetworkReply *m_netReply;

    qreal m_weight;
    qreal m_fat;
    qreal m_bone;
    qreal m_muscle;
    qreal m_vfat;
    qreal m_water;
    int m_bmr;
    bool m_updated;
};

#endif // TRAYBLE_H
