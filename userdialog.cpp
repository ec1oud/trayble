#include "userdialog.h"
#include "ui_userdialog.h"

UserDialog::UserDialog(QWidget *parent, QSettings &settings, QString name) :
    QDialog(parent),
    ui(new Ui::UserDialog),
    m_settings(settings),
    m_name(name)
{
    ui->setupUi(this);
    ui->name->setText(name);

    m_settings.beginGroup(QLatin1String("UserID"));
    ui->userID->setValue(m_settings.value(name, 99).toInt());
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("UserGender"));
    if (m_settings.value(name, "m") == "m")
        ui->genderMale->setChecked(true);
    else
        ui->genderFemale->setChecked(true);
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("UserExerciseLevel"));
    switch (m_settings.value(name, 0).toInt()) {
    case 0:
        ui->athleticNot->setChecked(true);
        break;
    case 1:
        ui->athleticAmateur->setChecked(true);
        break;
    case 2:
        ui->athleticProfessional->setChecked(true);
        break;
    }
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("UserHeight"));
    ui->height->setValue(m_settings.value(name, 170).toInt());
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("UserBirthday"));
    ui->birthday->setDate(m_settings.value(name, QDate(1990, 1, 1)).toDate());
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("UserUnits"));
    QString unitsString = m_settings.value(name, 0).toString();
    if (!unitsString.isEmpty()) {
        switch (unitsString.at(0).toLatin1()) {
        case 's':
            ui->unitsStones->setChecked(true);
            break;
        case 'k':
            ui->unitsKG->setChecked(true);
            break;
        case 'p':
            ui->unitsPounds->setChecked(true);
            break;
        }
    }
    m_settings.endGroup();
}

UserDialog::~UserDialog()
{
    delete ui;
}

void UserDialog::on_buttonBox_accepted()
{
    m_settings.beginGroup(QLatin1String("UserID"));
    m_settings.setValue(m_name, ui->userID->value());
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("UserGender"));
    if (ui->genderMale->isChecked())
        m_settings.setValue(m_name, "m");
    if (ui->genderFemale->isChecked())
        m_settings.setValue(m_name, "f");
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("UserExerciseLevel"));
    if (ui->athleticNot->isChecked())
        m_settings.setValue(m_name, 0);
    if (ui->athleticAmateur->isChecked())
        m_settings.setValue(m_name, 1);
    if (ui->athleticProfessional->isChecked())
        m_settings.setValue(m_name, 2);
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("UserHeight"));
    m_settings.setValue(m_name, ui->height->value());
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("UserBirthday"));
    m_settings.setValue(m_name, ui->birthday->date());
    m_settings.endGroup();

    m_settings.beginGroup(QLatin1String("UserUnits"));
    if (ui->unitsStones->isChecked())
        m_settings.setValue(m_name, "stones");
    if (ui->unitsKG->isChecked())
        m_settings.setValue(m_name, "kilograms");
    if (ui->unitsPounds->isChecked())
        m_settings.setValue(m_name, "pounds");
    m_settings.endGroup();

    delete this;
}

void UserDialog::on_buttonBox_rejected()
{
    delete this;
}
