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

static const QStringList supportedDeviceNamePrefixes = { "Electronic Scale", "aplant" };

TrayBle::TrayBle() :
    m_influxHealthInsertReq(QUrl("http://localhost:8086/write?db=health")),
    m_influxPlantsInsertReq(QUrl("http://localhost:8086/write?db=weather"))
{
    m_settings.beginGroup(QLatin1String("General"));
    m_lastUser = m_settings.value(QLatin1String("lastUser")).toString();
    m_settings.endGroup();

    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    m_discoveryAgent->setLowEnergyDiscoveryTimeout(0); // scan forever
    m_influxHealthInsertReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    m_influxPlantsInsertReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

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
    setStatus(tr("scanning stopped unexpectedly"));
    deviceSearch();
}

void TrayBle::addDevice(const QBluetoothDeviceInfo &device)
{
    QString addrString = device.address().toString();
    if (m_discoveredDevices.contains(addrString)) {
        qDebug() << "rediscovered" << device.name() << device.address();
        return;
    }
    if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
        qDebug() << "discovered device " << device.name() << device.address().toString()
                 << "manufacturer IDs" << hex << device.manufacturerIds();
        for (const QString &pfx : supportedDeviceNamePrefixes) {
            if (device.name().startsWith(pfx)) {
                m_discoveredDevices.insert(addrString);
                updateDevice(device, QBluetoothDeviceInfo::Field::All);
                //updateDevice(device, QBluetoothDeviceInfo::Field(0x7fff));
                connectService(device);
                connect(m_discoveryAgent, SIGNAL(deviceUpdated(const QBluetoothDeviceInfo&, QBluetoothDeviceInfo::Fields)),
                        this, SLOT(updateDevice(const QBluetoothDeviceInfo&, QBluetoothDeviceInfo::Fields)));
            }
        }
    }
}

void TrayBle::updateDevice(const QBluetoothDeviceInfo &device, QBluetoothDeviceInfo::Fields updatedFields)
{
    if (updatedFields.testFlag(QBluetoothDeviceInfo::Field::ManufacturerData))
        for (auto id : device.manufacturerIds()) {
            qDebug() << device.name() << device.address() << hex << "ID" << id << "data" << dec << device.manufacturerData(id).count() << hex << "bytes:" << device.manufacturerData(id).toHex();
            if (id == 0x4c)
                decodeIBeaconData(device, device.manufacturerData(id));
        }

    if (device.name() == supportedDeviceNamePrefixes.first()) // Electronic Scale
        connectService(device);
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

void TrayBle::connectService(const QBluetoothDeviceInfo &device)
{
    QLowEnergyController *ctrl = QLowEnergyController::createCentral(device, this);
    m_connectedDevices.insert(device.address().toString(), DeviceInfo { device, ctrl });
    connect(ctrl, SIGNAL(serviceDiscovered(QBluetoothUuid)),
            this, SLOT(serviceDiscovered(QBluetoothUuid)));
    connect(ctrl, SIGNAL(discoveryFinished()),
            this, SLOT(serviceScanDone()));
    connect(ctrl, SIGNAL(error(QLowEnergyController::Error)),
            this, SLOT(controllerError(QLowEnergyController::Error)));
    connect(ctrl, SIGNAL(connected()),
            this, SLOT(deviceConnected()));
    connect(ctrl, SIGNAL(disconnected()),
            this, SLOT(deviceDisconnected()));

    if (device.name() == supportedDeviceNamePrefixes.first()) // Electronic Scale
        ctrl->connectToDevice();
    else
        ctrl->discoverServices();
}

void TrayBle::deviceConnected()
{
    QLowEnergyController *ctrl = static_cast<QLowEnergyController *>(sender());
    qDebug() << ctrl->remoteName();
    m_updatedBodyComp = false;
    ctrl->discoverServices();
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
//        m_controller->disconnectFromDevice(); // TODO
        delete m_service;
        m_service = nullptr;
    }
}

QByteArray TrayBle::userCharacteristic(QString user)
{
    m_settings.beginGroup(QLatin1String("UserID"));
    uint8_t id = m_settings.value(user, 99).toInt();
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("UserGender"));
    uint8_t gender = 0; // female
    if (m_settings.value(user, "m") == "m")
        gender = 1; // male
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("UserExerciseLevel"));
    uint8_t exerciseLevel = m_settings.value(user, 0).toInt();
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("UserHeight"));
    uint8_t height = m_settings.value(user, 170).toInt();
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("UserBirthday"));
    uint8_t age = int(m_settings.value(user, QDate(1990, 1, 1)).toDate().daysTo(QDate::currentDate()) / 365.25);
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("UserUnits"));
    uint8_t units = 1; // kg
    QString unitsString = m_settings.value(user, 0).toString();
    if (!unitsString.isEmpty()) {
        switch (unitsString.at(0).toLatin1()) {
        case 's':
            units = 0;
            break;
        case 'p':
            units = 2;
            break;
        }
    }
    m_settings.endGroup();

    uint8_t checksum = id ^ gender ^ exerciseLevel ^ height ^ age ^ units;

    QByteArray ret;
    ret.append(0xfe);
    ret.append(id);
    ret.append(gender);
    ret.append(exerciseLevel);
    ret.append(height);
    ret.append(age);
    ret.append(units);
    ret.append(checksum);

    qDebug() << user << id << gender << exerciseLevel << height << age << units << checksum << ret.toHex();

    return ret;
}

void TrayBle::sendRequest()
{
    for (const QLowEnergyCharacteristic &characteristic : m_service->characteristics()) {
        qDebug() << "   characteristic " << hex << characteristic.handle() << characteristic.name() << characteristic.properties();

        switch (characteristic.properties()) {
        case QLowEnergyCharacteristic::Write: {
            // Send user preferences (even though we aren't sure which user this is, yet).
            // Merely subscribing for notifications without writing to this characteristic
            // seems not to be enough to get a weight reading.
            m_service->writeCharacteristic(characteristic, userCharacteristic(m_lastUser));
        } break;
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

void TrayBle::decodeIBeaconData(const QBluetoothDeviceInfo &dev, QByteArray data)
{
//qDebug() << dev.name() << dev.address() << data.toHex();
    if (dev.name().startsWith("aplant") && data.length() == 23) { // TODO and some part of some UUID is well-known?
        // figure out which plant this is
        m_settings.beginGroup(QLatin1String("Plants"));
        QStringList plants = m_settings.childKeys();
        QString plantName;
        for (const QString &key : m_settings.childKeys()) {
            if (dev.name() == key)
                plantName = m_settings.value(key).toString();
        }
        if (plantName.isEmpty()) {
            plantName = QInputDialog::getText(nullptr, tr("Which plant has sensor %1?").arg(dev.name()), tr("plant name"));
            m_settings.setValue(dev.name(), plantName);
        }
        m_settings.endGroup();

        // TODO if there's a settable name on the device, we need that
        int temperature = int(data[data.length() - 2]);
        int moisture = int(data[data.length() - 3]);
        QString message = tr("%1 (%2) temperature %3 moisture %4").arg(plantName).arg(dev.name()).arg(temperature).arg(moisture);
        QString reading = tr("%1°C %2%").arg(temperature).arg(moisture);
        emit readingUpdated(plantName, reading);
        setStatus(message);

        // update influxDB
        if (!m_netReply) {
            QString reqData = QLatin1String("plants,plant=%1 temperature=%2,moisture=%3");
            reqData = reqData.arg(plantName).arg(temperature).arg(moisture);
            m_netReply = m_nam.post(m_influxPlantsInsertReq, reqData.toLatin1());
            connect(m_netReply, &QNetworkReply::finished, this, &TrayBle::networkFinished);
            connect(m_netReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkError(QNetworkReply::NetworkError)));
        }
    }
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
    if (m_updatedBodyComp)
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
        m_updatedBodyComp = true;

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

        bool differentUser = true;
        if (nearestUser.isEmpty() || nearestUserDelta > 5) {
            m_lastUser = QInputDialog::getText(nullptr, tr("New user?"), tr("user name"));
        } else {
            if (m_lastUser == nearestUser)
                differentUser = false;
            else
                m_lastUser = nearestUser;
        }

        // update user's last-known weight for comparison next time
        m_settings.setValue(m_lastUser, m_weight);
        m_settings.endGroup();

        m_settings.beginGroup(QLatin1String("General"));
        m_settings.setValue(QLatin1String("lastUser"), m_lastUser);
        m_settings.endGroup();

        // update the UI
        QString message = tr("%1 %2 (delta %8), %3% fat, %4% water, %5 %2 muscle, %6 %2 bone, BMR %7 kcal")
                .arg(m_weight).arg(tr("kg")).arg(m_fat).arg(m_water).arg(m_muscle).arg(m_bone).arg(m_bmr).arg(m_lastUser);

        setStatus(message);
        emit notify(m_lastUser, message);
        emit readingUpdated(m_lastUser, message);

        // update influxDB
        if (!m_netReply) {
            QString reqData = QLatin1String("bodycomp,username=%1 weight=%2,unit=\"%3\",fat=%4,water=%5,muscle=%6,bone=%7,bmr=%8,vfat=%9");
            reqData = reqData.arg(m_lastUser).arg(m_weight).arg(tr("kg")).arg(m_fat).arg(m_water).arg(m_muscle).arg(m_bone).arg(m_bmr).arg(m_vfat);
            m_netReply = m_nam.post(m_influxHealthInsertReq, reqData.toLatin1());
            connect(m_netReply, &QNetworkReply::finished, this, &TrayBle::networkFinished);
            connect(m_netReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkError(QNetworkReply::NetworkError)));
        }

        // if this is a different user than last time, ask the scale to use the user's settings and try again
        if (differentUser)
            sendRequest();
    }
}
