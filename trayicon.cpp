#include "trayicon.h"
#include <QApplication>
#include <QBluetoothAddress>
#include <QDebug>
#include <QMenu>

TrayIcon::TrayIcon() :
    m_normalIcon(":/icons/bathroom-scale-dial.svg")
{
    setIcon(m_normalIcon);
    m_separator = m_menu.addSeparator();
    QObject::connect(m_menu.addAction(tr("Quit")), &QAction::triggered,
                     qApp, &QApplication::quit);
    setContextMenu(&m_menu);
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
    } else {
        QMenu *sub = *it;
        Q_ASSERT(sub);
        Q_ASSERT(sub->children().count() > 1);
        QAction *subAction = static_cast<QAction*>(sub->children().at(1));
        subAction->setText(values);
    }
}
