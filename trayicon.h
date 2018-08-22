#ifndef TRAYICON_H
#define TRAYICON_H

#include <QBluetoothDeviceInfo>
#include <QMenu>
#include <QSystemTrayIcon>

class QAction;

class TrayIcon : public QSystemTrayIcon
{
    Q_OBJECT
public:
    TrayIcon();

public slots:
    void showTooltip(const QString &message);
    void showError(const QString &message);
    void showReading(QString context, QString values);

private:
    QIcon m_normalIcon;
    QMenu m_menu;
    QAction *m_separator;
    QHash<QString, QMenu*> m_deviceMenus;
};

#endif // TRAYICON_H
