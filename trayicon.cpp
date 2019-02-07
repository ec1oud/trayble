#include "trayicon.h"
#include "userdialog.h"
#include <QApplication>
#include <QBluetoothAddress>
#include <QDebug>
#include <QMenu>

TrayIcon::TrayIcon(QSettings &settings) :
    m_settings(settings),
    m_normalIcon(":/icons/bathroom-scale-dial.svg")
{
    setIcon(m_normalIcon);
    m_separator = m_menu.addSeparator();
    QObject::connect(m_menu.addAction(tr("Quit")), &QAction::triggered,
                     qApp, &QApplication::quit);
    setContextMenu(&m_menu);
    // populate the menu with last-known weights
    m_settings.beginGroup(QLatin1String("UserWeights"));
    for (const QString &user : m_settings.childKeys()) {
        QMenu *sub = new QMenu(user);
        m_deviceMenus.insert(user, sub);
        m_menu.insertMenu(m_separator, sub);
        sub->addAction(m_settings.value(user).toString());
        sub->addAction(tr("Settings"), this, &TrayIcon::openSettings)->setData(user);
    }
    m_settings.endGroup();
}

void TrayIcon::showTooltip(const QString &message)
{
    setToolTip(message);
}

void TrayIcon::showError(const QString &message)
{
    showMessage(QApplication::applicationName(), message, QSystemTrayIcon::Critical);
}

void TrayIcon::showReading(QString context, QString values)
{
    auto it = m_deviceMenus.find(context);
    if (it == m_deviceMenus.end()) {
        QMenu *sub = new QMenu(context);
        m_deviceMenus.insert(context, sub);
        m_menu.insertMenu(m_separator, sub);
        sub->addAction(values);
        m_settings.beginGroup(QLatin1String("UserWeights"));
        if (m_settings.value(context).isValid()) { // it's a user, not a plant
            sub->addAction(tr("Settings"), this, &TrayIcon::openSettings)->setData(context);
        }
        m_settings.endGroup();
    } else {
        QMenu *sub = *it;
        Q_ASSERT(sub);
        Q_ASSERT(sub->children().count() > 1);
        QAction *subAction = static_cast<QAction*>(sub->children().at(1));
        subAction->setText(values);
    }
}

void TrayIcon::openSettings()
{
    UserDialog *dlg = new UserDialog(nullptr, m_settings, static_cast<QAction *>(sender())->data().toString());
    // it has to delete itself when closed
    dlg->show();
}
