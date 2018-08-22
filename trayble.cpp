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

#include "trayble.h"
#include <QDebug>
#include <QInputDialog>
#include <QMetaEnum>

TrayBle::TrayBle() :
    m_discoveryAgent(nullptr), m_controller(nullptr), m_service(nullptr),
    m_influxInsertReq(QUrl("http://localhost:8086/write?db=health")),
    m_netReply(nullptr), m_updated(false)
{
    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    m_discoveryAgent->setLowEnergyDiscoveryTimeout(0); // scan forever
    m_influxInsertReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    connect(m_discoveryAgent, SIGNAL(deviceDiscovered(const QBluetoothDeviceInfo&)),
            this, SLOT(addDevice(const QBluetoothDeviceInfo&)));
    connect(m_discoveryAgent, SIGNAL(error(QBluetoothDeviceDiscoveryAgent::Error)),
            this, SLOT(deviceScanError(QBluetoothDeviceDiscoveryAgent::Error)));
    connect(m_discoveryAgent, SIGNAL(finished()), this, SLOT(scanFinished()));
}

TrayBle::~TrayBle()
{
}

void TrayBle::deviceSearch()
{
    m_discoveryAgent->start();
    setStatus(tr("scanning for devices"));
}

void TrayBle::scanFinished()
{
    if (!m_device.isValid())
        deviceSearch();
}

void TrayBle::addDevice(const QBluetoothDeviceInfo &device)
{
    if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
        qDebug() << "discovered device " << device.name() << device.address().toString();
        if (device.name() == QLatin1String("Electronic Scale")) {
            m_device = device;
            connectService();
        }
    }
}

void TrayBle::deviceScanError(QBluetoothDeviceDiscoveryAgent::Error e)
{
    static QMetaEnum menum = m_discoveryAgent->metaObject()->enumerator(
                m_discoveryAgent->metaObject()->indexOfEnumerator("Error"));
    QString msg;
    switch (e) {
    case QBluetoothDeviceDiscoveryAgent::PoweredOffError:
        msg = tr("Bluetooth adaptor is powered off");
        break;
    case QBluetoothDeviceDiscoveryAgent::InputOutputError:
        msg = tr("no Bluetooth adapter or I/O error");
        break;
    default:
        msg = tr("device scan error: %1").arg(menum.valueToKey(e));
        break;
    }
    emit error(msg);
    setStatus(msg);
}

void TrayBle::setStatus(QString s)
{
    qDebug() << s;
    m_status = s;
    emit statusChanged(m_status);
}

QString TrayBle::status() const
{
    return m_status;
}

void TrayBle::connectService()
{
    qDebug() << m_device.name() << m_device.address();

    if (m_controller) {
        m_controller->disconnectFromDevice();
        delete m_controller;
        m_controller = nullptr;

    }
    m_controller = new QLowEnergyController(m_device, this);
    connect(m_controller, SIGNAL(serviceDiscovered(QBluetoothUuid)),
            this, SLOT(serviceDiscovered(QBluetoothUuid)));
    connect(m_controller, SIGNAL(discoveryFinished()),
            this, SLOT(serviceScanDone()));
    connect(m_controller, SIGNAL(error(QLowEnergyController::Error)),
            this, SLOT(controllerError(QLowEnergyController::Error)));
    connect(m_controller, SIGNAL(connected()),
            this, SLOT(deviceConnected()));
    connect(m_controller, SIGNAL(disconnected()),
            this, SLOT(deviceDisconnected()));

    m_controller->connectToDevice();
}

void TrayBle::deviceConnected()
{
    m_updated = false;
    m_controller->discoverServices();
}

void TrayBle::deviceDisconnected()
{
    QLowEnergyController *ctrl = static_cast<QLowEnergyController *>(sender());
    qDebug() << ctrl->remoteName() << ctrl->state();
    setStatus(tr("%1 disconnected").arg(ctrl->remoteName()));
    deviceSearch();
}

void TrayBle::serviceDiscovered(const QBluetoothUuid &svc)
{
    QLowEnergyController *ctrl = static_cast<QLowEnergyController *>(sender());
    qDebug() << ctrl->remoteName() << ": discovered service" << svc << hex << svc.toUInt16();
    if (svc.toUInt16() == 0xfff0)
        m_serviceUuid = svc;
}

void TrayBle::serviceScanDone()
{
    QLowEnergyController *ctrl = static_cast<QLowEnergyController *>(sender());
    qDebug() << ctrl->remoteName();

    delete m_service;
    m_service = nullptr;

    if (m_serviceUuid.isNull()) {
        setStatus(tr("no known service on ") + ctrl->remoteName());
        return;
    }

    setStatus(tr("connecting..."));
    m_service = ctrl->createServiceObject(m_serviceUuid, this);

    if (!m_service) {
        setStatus(tr("failed to connect to ") + ctrl->remoteName());
        return;
    }

    connect(m_service, SIGNAL(stateChanged(QLowEnergyService::ServiceState)),
            this, SLOT(serviceStateChanged(QLowEnergyService::ServiceState)));
    connect(m_service, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)),
            this, SLOT(updateBodyComp(QLowEnergyCharacteristic,QByteArray)));
    connect(m_service, SIGNAL(error(QLowEnergyService::ServiceError)),
            this, SLOT(serviceError(QLowEnergyService::ServiceError)));

    m_service->discoverDetails();
}

void TrayBle::disconnectService()
{
    // disable notifications before disconnecting
    if (m_notification.isValid() && m_service
            && m_notification.value() == QByteArray::fromHex("0100")) {
        m_service->writeDescriptor(m_notification, QByteArray::fromHex("0000"));
    } else {
        m_controller->disconnectFromDevice();
        delete m_service;
        m_service = nullptr;
    }
}

void TrayBle::sendRequest()
{
    for (const QLowEnergyCharacteristic &characteristic : m_service->characteristics()) {
        qDebug() << "   characteristic " << hex << characteristic.handle() << characteristic.name() << characteristic.properties();

        switch (characteristic.properties()) {
        case QLowEnergyCharacteristic::Write:
            // TODO figure out if this is the preferences or what.  But merely subscribing for notifications
            // without writing to this characteristic seems not to be enough.
            m_service->writeCharacteristic(characteristic, QByteArray::fromHex("fe010100aa2d0285"));
            break;
        case QLowEnergyCharacteristic::Notify: {
            m_notification = characteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
            if (!m_notification.isValid()) {
                qWarning() << "invalid notification descriptor";
                return;
            }

            // enable notification
            m_service->writeDescriptor(m_notification, QByteArray::fromHex("0100"));
        }
            break;
        default:
            break;
        }
    }
}

void TrayBle::controllerError(QLowEnergyController::Error e)
{
    static QMetaEnum menum = static_cast<QLowEnergyController *>(sender())
            ->metaObject()->enumerator(static_cast<QLowEnergyController *>(sender())
                                       ->metaObject()->indexOfEnumerator("Error"));
    setStatus(tr("controller error: %1").arg(menum.valueToKey(e)));
}

void TrayBle::serviceStateChanged(QLowEnergyService::ServiceState s)
{
    switch (s) {
    case QLowEnergyService::ServiceDiscovered:
        sendRequest();
        break;
    default:
        break;
    }
}

void TrayBle::serviceError(QLowEnergyService::ServiceError e)
{
    static QMetaEnum menum = static_cast<QLowEnergyService *>(sender())
            ->metaObject()->enumerator(static_cast<QLowEnergyService *>(sender())
                                       ->metaObject()->indexOfEnumerator("ServiceError"));
    setStatus(tr("service error: %1").arg(menum.valueToKey(e)));
}

void TrayBle::networkFinished()
{
    qDebug() << "influxDB says: " << m_netReply->readAll();
    m_netReply->disconnect();
    m_netReply->deleteLater();
    m_netReply = nullptr;
}

void TrayBle::networkError(QNetworkReply::NetworkError e)
{
    static QMetaEnum menum = m_netReply->metaObject()->enumerator(
                m_netReply->metaObject()->indexOfEnumerator("NetworkError"));
    setStatus(tr("network error: ") + menum.key(e));
    m_netReply->disconnect();
    m_netReply->deleteLater();
    m_netReply = nullptr;
}

void TrayBle::updateBodyComp(const QLowEnergyCharacteristic &c,
                                 const QByteArray &value)
{
    if (m_updated)
        return;
    QByteArray hexValue = value.toHex();
    qDebug() << c.name() << hexValue;

    // example cf01adaa 0405 015a 1b 0269 13 01cd 0599
    //         not sure, weight, fat, bone, muscle, visceral fat, water, BMR
    if (hexValue.length() != 32)
        setStatus(tr("reading has unexpected length"));
    else {
        // Decode values from the hex string
        bool ok = false;
        int val = hexValue.mid(8, 4).toInt(&ok, 16);
        if (!ok) {
            setStatus(tr("failed to decode weight reading "));
            return;
        }
        m_weight = val / 10.0;

        val = hexValue.mid(12, 4).toInt(&ok, 16);
        if (!ok) {
            setStatus(tr("failed to decode fat reading"));
            return;
        }
        m_fat = val / 10.0;

        val = hexValue.mid(16, 2).toInt(&ok, 16);
        if (!ok) {
            setStatus(tr("failed to decode bone mass reading"));
            return;
        }
        m_bone = val / 10.0;

        val = hexValue.mid(18, 4).toInt(&ok, 16);
        if (!ok) {
            setStatus(tr("failed to decode muscle mass reading"));
            return;
        }
        m_muscle = val / 10.0;

        val = hexValue.mid(22, 2).toInt(&ok, 16);
        if (!ok) {
            setStatus(tr("failed to decode visceral fat reading"));
            return;
        }
        m_vfat = val / 10.0;

        val = hexValue.mid(24, 4).toInt(&ok, 16);
        if (!ok) {
            setStatus(tr("failed to decode water reading"));
            return;
        }
        m_water = val / 10.0;

        val = hexValue.mid(28, 4).toInt(&ok, 16);
        if (!ok) {
            setStatus(tr("failed to decode BMR reading"));
            return;
        }
        m_bmr = val;
        m_updated = true;

        // figure out which user this might be
        m_settings.beginGroup(QLatin1String("UserWeights"));
        QStringList users = m_settings.childKeys();
        QString nearestUser;
        qreal nearestUserDelta = 1000;
        for (const QString &key : users) {
            qreal weight = m_settings.value(key).toReal();
            qreal delta = weight - m_weight;
            if (qAbs(delta) < qAbs(nearestUserDelta)) {
                nearestUser = key;
                nearestUserDelta = delta;
            }
        }

        if (users.isEmpty() || nearestUserDelta > 5) {
            nearestUser = QInputDialog::getText(nullptr, tr("New user?"), tr("user name"));
        }

        // update user's last-known weight for comparison next time
        m_settings.setValue(nearestUser, m_weight);
        m_settings.endGroup();

        // update the UI
        QString message = tr("%1 %2 (delta %8), %3% fat, %4% water, %5 %2 muscle, %6 %2 bone, BMR %7 kcal")
                .arg(m_weight).arg(tr("kg")).arg(m_fat).arg(m_water).arg(m_muscle).arg(m_bone).arg(m_bmr).arg(nearestUserDelta);

        setStatus(message);
        emit weightUpdated(nearestUser, message);

        // update influxDB
        if (!m_netReply) {
            QString reqData = QLatin1String("bodycomp,username=%1 weight=%2,unit=\"%3\",fat=%4,water=%5,muscle=%6,bone=%7,bmr=%8,vfat=%9");
            reqData = reqData.arg(nearestUser).arg(m_weight).arg(tr("kg")).arg(m_fat).arg(m_water).arg(m_muscle).arg(m_bone).arg(m_bmr).arg(m_vfat);
            m_netReply = m_nam.post(m_influxInsertReq, reqData.toLatin1());
            connect(m_netReply, &QNetworkReply::finished, this, &TrayBle::networkFinished);
            connect(m_netReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkError(QNetworkReply::NetworkError)));
        }
    }
}
