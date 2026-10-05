#include "invoicecard.h"
#include "ui_invoicecard.h"
#include "DatabaseManager.h"
#include "DAO_Invoice.h"
#include "DAO_Client.h"
#include "invoicestatusguard.h"

#include <QMessageBox>
#include <QDate>

InvoiceCard::InvoiceCard(DatabaseManager& dbManager, int invoiceId, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::InvoiceCard)
    , m_dbManager(dbManager)
    , m_invoiceId(invoiceId)
{
    ui->setupUi(this);

    // Open as its own top-level window rather than embedded in the parent's layout
    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose);

    loadInvoice();
    setEditingEnabled(false); // opens read-only
}

InvoiceCard::~InvoiceCard()
{
    delete ui;
}

void InvoiceCard::loadInvoice()
{
    DAO_Invoice invoiceDao(m_dbManager);
    m_originalInvoice = invoiceDao.getInvoiceById(m_invoiceId);

    if (m_originalInvoice.id == 0) {
        QMessageBox::warning(this, "Not Found", "Could not load this invoice record.");
        close();
        return;
    }

    populateFields(m_originalInvoice);
    setWindowTitle(QString("Invoice: %1").arg(m_originalInvoice.invoiceNumber));
}

void InvoiceCard::populateFields(const Invoice &invoice)
{
    ui->InvoiceNumber_Label->setText(invoice.invoiceNumber);

    // Client name/business is looked up for display only; the invoice's
    // client association is not editable from this card.
    DAO_Client clientDao(m_dbManager);
    Client client = clientDao.getClientById(invoice.clientId);
    QString clientDisplay = QString("%1 %2").arg(client.firstName, client.lastName).trimmed();
    if (!client.businessName.isEmpty()) {
        clientDisplay += QString(" (%1)").arg(client.businessName);
    }
    ui->Client_Label->setText(clientDisplay);

    ui->IssueDate_DateEdit->setDate(QDate::fromString(invoice.issueDate, "yyyy-MM-dd"));
    ui->DueDate_DateEdit->setDate(QDate::fromString(invoice.dueDate, "yyyy-MM-dd"));

    int statusIndex = ui->Status_ComboBox->findText(invoice.status);
    ui->Status_ComboBox->setCurrentIndex(statusIndex >= 0 ? statusIndex : 0);

    ui->TaxRate_SpinBox->setValue(invoice.taxRate);
    ui->Discount_DoubleSpinBox->setValue(invoice.discountAmount / 100.0);
    ui->Notes_PlainTextEdit->setPlainText(invoice.notes);
}

void InvoiceCard::setEditingEnabled(bool enabled)
{
    // Invoice number and client are never editable from this card
    ui->IssueDate_DateEdit->setReadOnly(!enabled);
    ui->DueDate_DateEdit->setReadOnly(!enabled);
    ui->Status_ComboBox->setEnabled(enabled);
    ui->TaxRate_SpinBox->setReadOnly(!enabled);
    ui->Discount_DoubleSpinBox->setReadOnly(!enabled);
    ui->Notes_PlainTextEdit->setReadOnly(!enabled);

    ui->Edit_PushButton->setVisible(!enabled);
    ui->Save_PushButton->setVisible(enabled);
    ui->Cancel_PushButton->setVisible(enabled);
}

void InvoiceCard::on_Edit_PushButton_clicked()
{
    setEditingEnabled(true);
}

void InvoiceCard::on_Cancel_PushButton_clicked()
{
    populateFields(m_originalInvoice); // discard unsaved edits
    setEditingEnabled(false);
}

void InvoiceCard::on_Save_PushButton_clicked()
{
    Invoice updated = m_originalInvoice; // keeps id, clientId, invoiceNumber
    updated.issueDate = ui->IssueDate_DateEdit->date().toString("yyyy-MM-dd");
    updated.dueDate = ui->DueDate_DateEdit->date().toString("yyyy-MM-dd");
    updated.status = ui->Status_ComboBox->currentText();
    updated.taxRate = ui->TaxRate_SpinBox->value();
    updated.discountAmount = qRound(ui->Discount_DoubleSpinBox->value() * 100.0);
    updated.notes = ui->Notes_PlainTextEdit->toPlainText().trimmed();

    if (updated.dueDate < updated.issueDate) {
        QMessageBox::warning(this, "Validation Error", "Due date cannot be before the issue date.");
        return;
    }

    // Only confirm when status is actually being changed to something
    // non-Draft in this edit -- an invoice that was already "Sent" and
    // stays "Sent" while you just fix a typo in Notes shouldn't re-prompt
    // every time you hit Save.
    if (updated.status != m_originalInvoice.status && !confirmNonDraftInvoiceStatus(this, updated.status)) {
        return;
    }

    DAO_Invoice invoiceDao(m_dbManager);
    if (invoiceDao.updateInvoice(updated)) {
        m_originalInvoice = updated;
        setEditingEnabled(false);
        emit invoiceUpdated(m_invoiceId);
    } else {
        QMessageBox::critical(this, "Error", "Failed to update invoice in database.");
    }
}

void InvoiceCard::on_Close_PushButton_clicked()
{
    close();
}
