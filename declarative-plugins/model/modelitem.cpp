/*
    Copyright 2013 Jan Grulich <jgrulich@redhat.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "modelitem.h"

#include <QtNetworkManager/wireddevice.h>
#include <QtNetworkManager/wirelessdevice.h>
#include <QtNetworkManager/settings/802-11-wireless.h>

#include <KLocalizedString>

#include "debug.h"

ModelItem::ModelItem(NetworkManager::Device * device, QObject * parent):
    QObject(parent),
    m_active(0),
    m_connection(0),
    m_device(0),
    m_network(0),
    m_vpn(0),
    m_connected(false),
    m_conecting(false),
    m_previousSignal(0),
    m_signal(0),
    m_secure(false),
    m_type(NetworkManager::Settings::ConnectionSettings::Unknown)
{
    setDevice(device);
}

ModelItem::~ModelItem()
{
    delete m_vpn;
}

QString ModelItem::name() const
{
    return m_name;
}

QString ModelItem::uuid() const
{
    return m_uuid;
}

bool ModelItem::connected() const
{
    return m_connected;
}

bool ModelItem::connecting() const
{
    return m_conecting;
}

QString ModelItem::deviceUdi() const
{
    return m_deviceUdi;
}

QString ModelItem::detailInformations() const
{
    return m_details;
}

QString ModelItem::icon() const
{
    return m_icon;
}

QString ModelItem::ssid() const
{
    return m_ssid;
}

int ModelItem::signal() const
{
    return m_signal;
}

bool ModelItem::secure() const
{
    return m_secure;
}

NetworkManager::Settings::ConnectionSettings::ConnectionType ModelItem::type() const
{
    return m_type;
}

void ModelItem::updateDetails()
{
    m_details = "<qt><table>";
    QString format = "<tr><td align=\"right\"><b>%1</b></td><td align=\"left\">&nbsp;%2</td></tr>";

    if (m_type != NetworkManager::Settings::ConnectionSettings::Unknown) {
        m_details += QString(format).arg(i18nc("type of network device", "Type:"), NetworkManager::Settings::ConnectionSettings::typeAsString(m_type));
    }

    if (m_device) {

        if (device()->ipV4Config().isValid() && connected()) {
            QHostAddress addr(device()->ipV4Config().addresses().first().address());
            m_details += QString(format).arg(i18n("IPv4 Address:"), addr.toString());
        }

        if (device()->ipV6Config().isValid() && connected()) {
            QHostAddress addr(device()->ipV6Config().addresses().first().address());
            m_details += QString(format).arg(i18n("IPv6 Address:"), addr.toString());
        }

        if (device()->type() == NetworkManager::Device::Ethernet) {
            NetworkManager::WiredDevice * wired = qobject_cast<NetworkManager::WiredDevice*>(device());
            if (connected()) {
                if (wired->bitRate() < 1000000) {
                    m_details += QString(format).arg(i18n("Connection speed:"), i18n("%1 Mb/s", wired->bitRate()/1000));
                } else {
                    m_details += QString(format).arg(i18n("Connection speed:"), i18n("%1 Gb/s", wired->bitRate()/1000000));
                }
            }
            m_details += QString(format).arg(i18n("MAC Address:"), wired->permanentHardwareAddress());

        } else if (device()->type() == NetworkManager::Device::Wifi) {
            NetworkManager::WirelessDevice * wireless = qobject_cast<NetworkManager::WirelessDevice*>(device());
            if (connected()) {
                if (wireless->bitRate() && wireless->bitRate() < 1000) {
                    m_details += QString(format).arg(i18n("Connection speed:"), i18n("%1 Kb/s", wireless->bitRate()));
                } else if (wireless->bitRate()) {
                    m_details += QString(format).arg(i18n("Connection speed:"), i18n("%1 Mb/s", wireless->bitRate()/1000));
                }
            }
            m_details += QString(format).arg(i18n("MAC Address:"), wireless->permanentHardwareAddress());
        }

        QString name;
        if (device()->ipInterfaceName().isEmpty()) {
            name = device()->interfaceName();
        } else {
            name = device()->ipInterfaceName();
        }
    }

    if (m_network) {
        NetworkManager::WirelessDevice * wifiDev = qobject_cast<NetworkManager::WirelessDevice*>(m_device);
        NetworkManager::AccessPoint * ap = wifiDev->findAccessPoint(m_network->referenceAccessPoint());

        m_details += QString(format).arg(i18n("Access point (SSID):"), m_network->ssid());
        m_details += QString(format).arg(i18n("Access point (BSSID):"), ap->hardwareAddress());
        m_details += QString(format).arg(i18nc("Wifi AP frequency", "Frequency:"), i18n("%1 Mhz", ap->frequency()));
    }

    m_details += QString("</table></qt>");
}

bool ModelItem::operator==(ModelItem* item)
{
    if (((item->uuid() == this->uuid()) && !item->name().isEmpty() && !this->name().isEmpty()) ||
        item->name() == this->name()) {
        return true;
    }

    return false;
}

void ModelItem::setWirelessNetwork(NetworkManager::WirelessNetwork * network)
{
    m_network = network;

    if (m_network) {
        m_ssid = network->ssid();
        m_previousSignal = m_signal;
        m_signal = m_network->signalStrength();
        m_type = NetworkManager::Settings::ConnectionSettings::Wireless;

        if (m_device) {
            NetworkManager::WirelessDevice * wifiDev = qobject_cast<NetworkManager::WirelessDevice*>(m_device);
            NetworkManager::AccessPoint * ap = wifiDev->findAccessPoint(m_network->referenceAccessPoint());

            if (ap->capabilities() & NetworkManager::AccessPoint::Privacy) {
                m_secure = true;
            }
        }

        if (!m_connection) {
            m_name = ssid();
        }

        connect(m_network, SIGNAL(signalStrengthChanged(int)),
                SLOT(onSignalStrengthChanged(int)), Qt::UniqueConnection);
        connect(m_network, SIGNAL(referenceAccessPointChanged(QString)),
                SLOT(onAccessPointChanged(QString)), Qt::UniqueConnection);
    } else {
        m_ssid.clear();
        m_signal = 0;
        m_type = NetworkManager::Settings::ConnectionSettings::Unknown;
        m_secure = false;
    }

    setWirelessIcon();
    updateDetails();
}

NetworkManager::WirelessNetwork* ModelItem::wirelessNetwork() const
{
    return m_network;
}

void ModelItem::setActiveConnection(NetworkManager::ActiveConnection* active)
{
    m_active = active;

    if (m_active) {
        if (m_active->state() == NetworkManager::ActiveConnection::Activating) {
            m_conecting = true;
            NMItemDebug() << name() << ": activating";
        } else if (m_active->state() == NetworkManager::ActiveConnection::Activated) {
            NMItemDebug() << name() << ": activated";
            m_connected = true;
        }

        if (m_active->vpn()) {
            m_vpn = new NetworkManager::VpnConnection(m_active->path());

            connect(m_vpn, SIGNAL(stateChanged(NetworkManager::VpnConnection::State)),
                    SLOT(onVpnConnectionStateChanged(NetworkManager::VpnConnection::State)), Qt::UniqueConnection);
        } else {
            connect(m_active, SIGNAL(default4Changed(bool)),
                    SLOT(onDefaultRouteChanged(bool)), Qt::UniqueConnection);
            connect(m_active, SIGNAL(default6Changed(bool)),
                    SLOT(onDefaultRouteChanged(bool)), Qt::UniqueConnection);
        }

        connect(m_active, SIGNAL(stateChanged(NetworkManager::ActiveConnection::State)),
                SLOT(onActiveConnectionStateChanged(NetworkManager::ActiveConnection::State)));

    } else {
        m_conecting = false;
        m_connected = false;
    }

    updateDetails();
}

NetworkManager::ActiveConnection* ModelItem::activeConnection() const
{
    return m_active;
}

void ModelItem::setDevice(NetworkManager::Device* device)
{
    m_device = device;

    if (m_device) {
        m_deviceUdi = m_device->udi();
    } else {
        m_deviceUdi.clear();
    }

    updateDetails();
}

NetworkManager::Device* ModelItem::device() const
{
    return m_device;
}

void ModelItem::setConnection(NetworkManager::Settings::Connection* connection)
{
    m_connection = connection;

    if (m_connection) {
        setConnectionSettings(connection->settings());

        connect(connection, SIGNAL(updated(QVariantMapMap)),
                SLOT(onConnectionUpdated(QVariantMapMap)), Qt::UniqueConnection);
    } else {
        m_uuid.clear();
        if (m_network) {
            m_name = m_network->ssid();
            m_type = NetworkManager::Settings::ConnectionSettings::Wireless;
        } else {
            m_name.clear();
            m_type = NetworkManager::Settings::ConnectionSettings::Unknown;
            m_ssid.clear();
            m_secure = false;
        }
        m_active = 0;
    }
}

void ModelItem::setConnectionSettings(const QVariantMapMap& map)
{
    NetworkManager::Settings::ConnectionSettings settings;
    settings.fromMap(map);

    m_uuid = settings.uuid();
    m_name = settings.id();
    m_type = settings.connectionType();

    if (settings.connectionType() == NetworkManager::Settings::ConnectionSettings::Wireless) {
        NetworkManager::Settings::WirelessSetting * wirelessSetting = static_cast<NetworkManager::Settings::WirelessSetting*>(settings.setting(NetworkManager::Settings::Setting::Wireless));
        m_ssid = wirelessSetting->ssid();
        if (!wirelessSetting->security().isEmpty()) {
            m_secure = true;
        }
    }

    setConnectionIcon();
    updateDetails();
}

NetworkManager::Settings::Connection* ModelItem::connection() const
{
    return m_connection;
}

QString ModelItem::connectionPath() const
{
    if (m_connection) {
        return m_connection->path();
    }

    return QString();
}

QString ModelItem::devicePath() const
{
    if (m_device) {
        return m_device->uni();
    }

    return QString();
}

QString ModelItem::specificPath() const
{
    if (m_network) {
        return m_network->referenceAccessPoint();
    }

    return QString();
}

void ModelItem::onActiveConnectionStateChanged(NetworkManager::ActiveConnection::State state)
{
    if (state == NetworkManager::ActiveConnection::Deactivated ||
        state == NetworkManager::ActiveConnection::Deactivating) {
        NMItemDebug() << name() << ": disconnected";
        m_active = 0;
        m_conecting = false;
        m_connected = false;
    } else if (state == NetworkManager::ActiveConnection::Activated) {
        NMItemDebug() << name() << ": activated";
        m_conecting = false;
        m_connected = true;
    } else if (state == NetworkManager::ActiveConnection::Activating) {
        NMItemDebug() << name() << ": activating";
        m_conecting = true;
        m_connected = false;
    }

    updateDetails();

    NMItemDebug() << name() << ": state has been changed to " << state;
    emit stateChanged();
}

void ModelItem::onConnectionUpdated(const QVariantMapMap& map)
{
    setConnectionSettings(map);

    emit connectionChanged();

    NMItemDebug() << name() << ": connection changed";
}

void ModelItem::onDefaultRouteChanged(bool defaultRoute)
{
    Q_UNUSED(defaultRoute);

    updateDetails();

    emit defaultRouteChanged();

    NMItemDebug() << name() << ": default route changed";
}

void ModelItem::onSignalStrengthChanged(int strength)
{
    m_previousSignal = m_signal;
    m_signal = strength;

    if (m_previousSignal - m_signal >= 25 ||
        m_previousSignal - m_signal <= 25) {
        setWirelessIcon();
        emit signalChanged();
    }

    NMItemDebug() << name() << ": strength changed to " << strength;
}

void ModelItem::onAccessPointChanged(const QString& accessPoint)
{
    updateDetails();

    emit accessPointChanged();

    NMItemDebug() << name() << ": access point changed to " << accessPoint;
}

void ModelItem::onVpnConnectionStateChanged(NetworkManager::VpnConnection::State state)
{
    if (state == NetworkManager::VpnConnection::Disconnected) {
        m_conecting = false;
        m_connected = false;
        delete m_vpn;
        m_vpn = 0;
        NMItemDebug() << name() << ": disconnected";
        m_active = 0;
    }
}

void ModelItem::setConnectionIcon()
{
    switch (m_type) {
        case NetworkManager::Settings::ConnectionSettings::Adsl:
            m_icon = "modem";
            break;
        case NetworkManager::Settings::ConnectionSettings::Bluetooth:
            m_icon = "bluetooth";
            break;
        case NetworkManager::Settings::ConnectionSettings::Bond:
            break;
        case NetworkManager::Settings::ConnectionSettings::Bridge:
            break;
        case NetworkManager::Settings::ConnectionSettings::Cdma:
            m_icon = "phone";
            break;
        case NetworkManager::Settings::ConnectionSettings::Gsm:
            m_icon = "phone";
            break;
        case NetworkManager::Settings::ConnectionSettings::Infiniband:
            break;
        case NetworkManager::Settings::ConnectionSettings::OLPCMesh:
            break;
        case NetworkManager::Settings::ConnectionSettings::Pppoe:
            m_icon = "modem";
            break;
        case NetworkManager::Settings::ConnectionSettings::Vlan:
            break;
        case NetworkManager::Settings::ConnectionSettings::Vpn:
            m_icon = "secure-card";
            break;
        case NetworkManager::Settings::ConnectionSettings::Wimax:
            break;
        case NetworkManager::Settings::ConnectionSettings::Wired:
            m_icon = "network-wired";
            break;
        case NetworkManager::Settings::ConnectionSettings::Wireless:
            setWirelessIcon();
            break;
        default:
            m_icon = "network-wired";
            break;
    }
}

void ModelItem::setWirelessIcon()
{
    if (m_signal <= 5) {
        m_icon = "network-wireless-connected-0";
    } else if (m_signal <= 25) {
        m_icon = "network-wireless-connected-25";
    } else if (m_signal <= 50) {
        m_icon = "network-wireless-connected-50";
    } else if (m_signal <= 75) {
        m_icon = "network-wireless-connected-75";
    } else {
        m_icon = "network-wireless-connected-100";
    }
}
