#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H

#include <QWidget>

class DatabaseManager;
class CompanyProfile;
class TaxSettings;

namespace Ui {
class Settings_Window;
}

class Settings_Window : public QWidget
{
    Q_OBJECT

public:
    explicit Settings_Window(DatabaseManager& dbManager, QWidget *parent = nullptr);
    ~Settings_Window();

private slots:
    void on_CompanyProfile_Button_clicked();
    void on_TaxSettings_Button_clicked();

    void on_CVSImport_Button_clicked();

private:
    Ui::Settings_Window *ui;
    DatabaseManager& m_dbManager;

    CompanyProfile *m_companyProfileWidget{nullptr};
    TaxSettings *m_taxSettingsWidget{nullptr};
};

#endif // SETTINGS_WINDOW_H