#include "customercard.h"
#include "ui_customercard.h"

#include "DatabaseManager.h"
#include "DAO_Client.h"

#include <QMessageBox>

CustomerCard::CustomerCard(DatabaseManager& dbManager, int clientId, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CustomerCard)
    , m_dbManager(dbManager)
    , m_clientId(clientId)
{
    ui->setupUi(this);

    // Open as its own top-level window rather than embedded in the parent's layout
    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose);

    loadClient();
    setEditingEnabled(false); // opens read-only
}

CustomerCard::~CustomerCard()
{
    delete ui;
}

void CustomerCard::loadClient()
{
    DAO_Client clientDao(m_dbManager);
    m_originalClient = clientDao.getClientById(m_clientId);

    if (m_originalClient.id == 0) {
        QMessageBox::warning(this, "Not Found", "Could not load this customer record.");
        close();
        return;
    }

    populateFields(m_originalClient);
    setWindowTitle(QString("Customer: %1 %2").arg(m_originalClient.firstName, m_originalClient.lastName));
}

void CustomerCard::populateFields(const Client &client)
{
    ui->FirstName_LineEdit->setText(client.firstName);
    ui->LastName_LineEdit->setText(client.lastName);
    ui->BusinessName_LineEdit->setText(client.businessName);
    ui->Email_LineEdit->setText(client.email);
    ui->PhoneNumber_LineEdit->setText(client.phoneNumber);
    ui->Address1_LineEdit->setText(client.address1);
    ui->Address2_LineEdit->setText(client.address2);
    ui->City_LineEdit->setText(client.city);
    ui->State_LineEdit->setText(client.state);
    ui->PostalCode_LineEdit->setText(client.postalCode);
    ui->CreatedAt_Label->setText(client.createdAt);
    ui->TaxExempt_CheckBox->setChecked(client.taxExempt);
}

void CustomerCard::setEditingEnabled(bool enabled)
{
    ui->FirstName_LineEdit->setReadOnly(!enabled);
    ui->LastName_LineEdit->setReadOnly(!enabled);
    ui->BusinessName_LineEdit->setReadOnly(!enabled);
    ui->Email_LineEdit->setReadOnly(!enabled);
    ui->PhoneNumber_LineEdit->setReadOnly(!enabled);
    ui->Address1_LineEdit->setReadOnly(!enabled);
    ui->Address2_LineEdit->setReadOnly(!enabled);
    ui->City_LineEdit->setReadOnly(!enabled);
    ui->State_LineEdit->setReadOnly(!enabled);
    ui->PostalCode_LineEdit->setReadOnly(!enabled);
    ui->TaxExempt_CheckBox->setEnabled(enabled);

    ui->Edit_PushButton->setVisible(!enabled);
    ui->Save_PushButton->setVisible(enabled);
    ui->Cancel_PushButton->setVisible(enabled);
}

void CustomerCard::on_Edit_PushButton_clicked()
{
    setEditingEnabled(true);
}

void CustomerCard::on_Cancel_PushButton_clicked()
{
    populateFields(m_originalClient); // discard unsaved edits
    setEditingEnabled(false);
}

void CustomerCard::on_Save_PushButton_clicked()
{
    Client updated = m_originalClient; // keeps the id
    updated.firstName = ui->FirstName_LineEdit->text().trimmed();
    updated.lastName = ui->LastName_LineEdit->text().trimmed();
    updated.businessName = ui->BusinessName_LineEdit->text().trimmed();
    updated.email = ui->Email_LineEdit->text().trimmed();
    updated.phoneNumber = ui->PhoneNumber_LineEdit->text().trimmed();
    updated.address1 = ui->Address1_LineEdit->text().trimmed();
    updated.address2 = ui->Address2_LineEdit->text().trimmed();
    updated.city = ui->City_LineEdit->text().trimmed();
    updated.state = ui->State_LineEdit->text().trimmed();
    updated.postalCode = ui->PostalCode_LineEdit->text().trimmed();
    updated.taxExempt = ui->TaxExempt_CheckBox->isChecked();

    if (updated.firstName.isEmpty() || updated.lastName.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "First and Last Name are required.");
        return;
    }

    DAO_Client clientDao(m_dbManager);
    if (clientDao.updateClient(updated)) {
        m_originalClient = updated;
        setWindowTitle(QString("Customer: %1 %2").arg(updated.firstName, updated.lastName));
        setEditingEnabled(false);
        emit customerUpdated(m_clientId);
    } else {
        QMessageBox::critical(this, "Error", "Failed to update client in database.");
    }
}

void CustomerCard::on_Close_PushButton_clicked()
{
    close();
}
