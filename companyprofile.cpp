#include "companyprofile.h"
#include "ui_companyprofile.h"
#include "DatabaseManager.h"
#include "DAO_Company.h"

#include <QMessageBox>

CompanyProfile::CompanyProfile(DatabaseManager &dbManager, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CompanyProfile)
    , m_dbManager(dbManager)
{
    ui->setupUi(this);
    // No setWindowFlags(Qt::Window) / WA_DeleteOnClose here -- MainWindow
    // owns this via QMdiSubWindow (see on_actionCompany_Profile_triggered),
    // same as Contacts_Customer / Contacts_Invoices.

    loadCompany();

    // A brand-new install has no company_profile row yet -- open straight
    // into edit mode so there's something to Save, rather than showing a
    // blank read-only form with no way in.
    setEditingEnabled(!m_originalCompany.exists);
}

CompanyProfile::~CompanyProfile()
{
    delete ui;
}

void CompanyProfile::loadCompany()
{
    DAO_Company companyDao(m_dbManager);
    m_originalCompany = companyDao.getCompany();
    populateFields(m_originalCompany);
    setWindowTitle("Company Profile");
}

void CompanyProfile::populateFields(const Company &company)
{
    ui->CompanyName_LineEdit->setText(company.name);
    ui->Address_LineEdit->setText(company.address);
    ui->City_LineEdit->setText(company.city);
    ui->State_LineEdit->setText(company.state);
    ui->Zip_LineEdit->setText(company.zip);
    ui->Phone_LineEdit->setText(company.phone);
    ui->Email_LineEdit->setText(company.email);
    ui->TaxIdEin_LineEdit->setText(company.taxIdEin);
    ui->LateFees_SpinBox->setValue(company.lateFeePercent);
    ui->Accrual_CheckBox->setChecked(company.accrual);
}

void CompanyProfile::setEditingEnabled(bool enabled)
{
    ui->CompanyName_LineEdit->setReadOnly(!enabled);
    ui->Address_LineEdit->setReadOnly(!enabled);
    ui->City_LineEdit->setReadOnly(!enabled);
    ui->State_LineEdit->setReadOnly(!enabled);
    ui->Zip_LineEdit->setReadOnly(!enabled);
    ui->Phone_LineEdit->setReadOnly(!enabled);
    ui->Email_LineEdit->setReadOnly(!enabled);
    ui->TaxIdEin_LineEdit->setReadOnly(!enabled);
    ui->LateFees_SpinBox->setReadOnly(!enabled);
    ui->Accrual_CheckBox->setEnabled(enabled);

    ui->Edit_PushButton->setVisible(!enabled);
    ui->Save_PushButton->setVisible(enabled);
    // Cancel only makes sense once a row already exists to revert to.
    ui->Cancel_PushButton->setVisible(enabled && m_originalCompany.exists);
}

void CompanyProfile::on_Edit_PushButton_clicked()
{
    setEditingEnabled(true);
}

void CompanyProfile::on_Cancel_PushButton_clicked()
{
    populateFields(m_originalCompany); // discard unsaved edits
    setEditingEnabled(false);
}

void CompanyProfile::on_Save_PushButton_clicked()
{
    Company updated = m_originalCompany;
    updated.name = ui->CompanyName_LineEdit->text().trimmed();
    updated.address = ui->Address_LineEdit->text().trimmed();
    updated.city = ui->City_LineEdit->text().trimmed();
    updated.state = ui->State_LineEdit->text().trimmed();
    updated.zip = ui->Zip_LineEdit->text().trimmed();
    updated.phone = ui->Phone_LineEdit->text().trimmed();
    updated.email = ui->Email_LineEdit->text().trimmed();
    updated.taxIdEin = ui->TaxIdEin_LineEdit->text().trimmed();
    updated.lateFeePercent = ui->LateFees_SpinBox->value();
    updated.accrual = ui->Accrual_CheckBox->isChecked();

    if (updated.name.isEmpty() || updated.address.isEmpty() || updated.city.isEmpty()
        || updated.state.isEmpty() || updated.zip.isEmpty()) {
        // Matches the NOT NULL columns in company_profile
        QMessageBox::warning(this, "Validation Error", "Company Name, Address, City, State, and Zip are required.");
        return;
    }

    DAO_Company companyDao(m_dbManager);
    if (companyDao.saveCompany(updated)) {
        updated.exists = true;
        m_originalCompany = updated;
        setEditingEnabled(false);
        emit companyProfileUpdated();
    } else {
        QMessageBox::critical(this, "Error", "Failed to save company profile to database.");
    }
}
