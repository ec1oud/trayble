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

struct DeviceInfo {
    QBluetoothDeviceInfo info;
    QLowEnergyController *controller;
};

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
    void connectService(const QBluetoothDeviceInfo &device);
    void disconnectService();
    void sendRequest();
    QSettings &settings() { return m_settings; }

private slots:
    void addDevice(const QBluetoothDeviceInfo&);
    void updateDevice(const QBluetoothDeviceInfo &device, QBluetoothDeviceInfo::Fields updatedFields);
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

    void decodeIBeaconData(const QBluetoothDeviceInfo &dev, QByteArray data);

    void networkFinished();
    void networkError(QNetworkReply::NetworkError e);

signals:
    void error(QString message);
    void statusChanged(QString message);
    void notify(QString title, QString message);
    void readingUpdated(QString context, QString values);

private:
    QByteArray userCharacteristic(QString user);

private:
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    QSet<QString> m_discoveredDevices;
    QHash<QString, DeviceInfo> m_connectedDevices; // by QBluetoothAddress.toString(), because QBluetoothAddress qHash impl is missing
    QLowEnergyDescriptor m_notification;
    QBluetoothUuid m_serviceUuid;
    QLowEnergyService *m_service = nullptr; // TODO nix
    QString m_status;
    QString m_lastUser;

    QSettings m_settings;

    QNetworkAccessManager m_nam;
    QNetworkRequest m_influxHealthInsertReq;
    QNetworkRequest m_influxPlantsInsertReq;
    QNetworkReply *m_netReply = nullptr;

    qreal m_weight = 0;
    qreal m_fat = 0;
    qreal m_bone = 0;
    qreal m_muscle = 0;
    qreal m_vfat = 0;
    qreal m_water = 0;
    int m_bmr = 0;
    bool m_updatedBodyComp = false;
};

#endif // TRAYBLE_H
