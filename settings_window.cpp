#include "settings_window.h"
#include "ui_settings_window.h"
#include "companyprofile.h"
#include "taxsettings.h"
#include "DatabaseManager.h"
#include "universalimportdialog.h"

#include <QIcon>
#include <QSize>
#include <QMessageBox>

Settings_Window::Settings_Window(DatabaseManager& dbManager, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Settings_Window)
    , m_dbManager(dbManager)
{
    ui->setupUi(this);

    // Set icons for the left sidebar buttons
    //ui->CompanyProfile_Button->setIcon(QIcon(":/new/prefix1/company.png")); // Replace with your icon paths
    //ui->CompanyProfile_Button->setIconSize(QSize(24, 24));

    //ui->TaxSettings_Button->setIcon(QIcon(":/new/prefix1/tax.png"));
    //ui->TaxSettings_Button->setIconSize(QSize(24, 24));

    // Instantiate embedded setting views
    m_companyProfileWidget = new CompanyProfile(m_dbManager, this);
    m_taxSettingsWidget = new TaxSettings(m_dbManager, this);

    // Add child widgets to stacked widget container
    ui->settingsStackedWidget->addWidget(m_companyProfileWidget); // Index 0
    ui->settingsStackedWidget->addWidget(m_taxSettingsWidget);    // Index 1

    // Default to Company Profile page
    on_CompanyProfile_Button_clicked();
}

Settings_Window::~Settings_Window()
{
    delete ui;
}

void Settings_Window::on_CompanyProfile_Button_clicked()
{
    ui->settingsStackedWidget->setCurrentWidget(m_companyProfileWidget);
}

void Settings_Window::on_TaxSettings_Button_clicked()
{
    ui->settingsStackedWidget->setCurrentWidget(m_taxSettingsWidget);
}
void Settings_Window::on_CVSImport_Button_clicked()
{
    UniversalImportDialog dlg(m_dbManager, this);
    if (dlg.exec() == QDialog::Accepted) {
        const int count = dlg.importedCount();
        if (count > 0) {
            QMessageBox::information(
                this,
                tr("Import Successful"),
                tr("Successfully imported %1 record(s).").arg(count)
                );
        }
    }
    else {
        QMessageBox::information(
            this,
            tr("Import Completed"),
            tr("Import Completed, but no new records were added.")
            );
    }
}
