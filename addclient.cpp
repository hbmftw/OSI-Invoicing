#include "addclient.h"
#include "ui_addclient.h"
#include "DatabaseManager.h"
#include "DAO_Client.h"

#include <QMessageBox>


AddClient::AddClient(DatabaseManager& dbManager, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddClient)
    , m_dbManager(dbManager) // Bind reference in initializer list
{
    ui->setupUi(this);

    ui->FirstName_LineEdit->setPlaceholderText("First Name");
    ui->LastName_LineEdit->setPlaceholderText("Last Name");
    ui->BusinessName_LineEdit->setPlaceholderText("business Name");
    ui->Email_LineEdit->setPlaceholderText("Email");
    ui->PhoneNumber_LineEdit->setPlaceholderText("Phone Number");
    ui->Address1_LineEdit->setPlaceholderText("Street Address");
    ui->Address2_LineEdit->setPlaceholderText("Address Line 2");
    ui->City_LineEdit->setPlaceholderText("City");
    ui->State_LineEdit->setPlaceholderText("State");
    ui->PostalCode_LineEdit->setPlaceholderText("Postal Code");
}

AddClient::~AddClient()
{
    delete ui;
}

void AddClient::on_Cancel_PushButton_clicked()
{
    reject();
}


void AddClient::on_Save_PushButton_clicked()
{
    // Build Client Structure from UI inputs
    Client newClient;
    // newClient.addressType = ui->AddressType_SelectionBox;
    newClient.firstName = ui->FirstName_LineEdit->text().trimmed();
    newClient.lastName = ui->LastName_LineEdit->text().trimmed();
    newClient.businessName = ui->BusinessName_LineEdit->text().trimmed();
    newClient.email = ui->Email_LineEdit->text().trimmed();
    newClient.phoneNumber = ui->PhoneNumber_LineEdit->text().trimmed();
    newClient.address1 = ui->Address1_LineEdit->text().trimmed();
    newClient.address2 = ui->Address2_LineEdit->text().trimmed();
    newClient.city = ui->City_LineEdit->text().trimmed();
    newClient.state = ui->State_LineEdit->text().trimmed();
    newClient.postalCode = ui->PostalCode_LineEdit->text().trimmed();
    newClient.taxExempt = ui->TaxExempt_CheckBox->isChecked();
    //newClient.country = ui->County_SelectionBox;

    // Validation that Line Edit Box is not empty for First and Last Name
    if (newClient.firstName.isEmpty() || newClient.lastName.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "First and Last Name are required.");
        return;
    }

    // Pass m_dbManager reference to DAO
    DAO_Client clientDao(m_dbManager);

    // Save to database
    if (clientDao.insertClient(newClient)) {
        accept(); // Close dialog with QDialog::Accepted
    } else {
        QMessageBox::critical(this, "Error", "Failed to save client to database.");
    }
}

