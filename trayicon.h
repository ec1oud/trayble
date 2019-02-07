#ifndef TRAYICON_H
#define TRAYICON_H

#include <QBluetoothDeviceInfo>
#include <QMenu>
#include <QSettings>
#include <QSystemTrayIcon>

class QAction;

class TrayIcon : public QSystemTrayIcon
{
    Q_OBJECT
public:
    TrayIcon(QSettings &settings);

public slots:
    void showTooltip(const QString &message);
    void showError(const QString &message);
    void showReading(QString context, QString values);
    void openSettings();

private:
    QSettings &m_settings;
    QIcon m_normalIcon;
    QMenu m_menu;
    QAction *m_separator;
    QHash<QString, QMenu*> m_deviceMenus;
};

#endif // TRAYICON_H
