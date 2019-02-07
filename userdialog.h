#ifndef USERDIALOG_H
#define USERDIALOG_H

#include <QDialog>
#include <QSettings>

namespace Ui {
class UserDialog;
}

class UserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UserDialog(QWidget *parent, QSettings &settings, QString name);
    ~UserDialog();

private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    Ui::UserDialog *ui;
    QSettings &m_settings;
    QString m_name;
};

#endif // USERDIALOG_H
