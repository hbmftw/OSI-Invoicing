#include "addPayment.h"
#include "ui_addPayment.h"

#include "DatabaseManager.h"
#include "DAO_Invoice.h"
#include "DAO_Items.h"
#include "DAO_Payment.h"

#include <QMessageBox>
#include <QDate>

AddPayment::AddPayment(DatabaseManager &dbManager, int invoiceId, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddPayment)
    , m_dbManager(dbManager)
    , m_invoiceId(invoiceId)
{
    ui->setupUi(this);

    ui->PaymentDate_DateEdit->setDate(QDate::currentDate());

    DAO_Invoice invoiceDao(m_dbManager);
    Invoice invoice = invoiceDao.getInvoiceById(invoiceId);
    if (invoice.id != 0) {
        ui->InvoiceSummary_Label->setText(QString("Invoice: %1").arg(invoice.invoiceNumber));
    }

    DAO_Items itemDao(m_dbManager);
    DAO_Payment paymentDao(m_dbManager);
    const int totalCents = itemDao.getInvoiceTotalCents(invoiceId);
    const int paidCents = paymentDao.getTotalPaidForInvoice(invoiceId);
    const int balanceCents = totalCents - paidCents;

    ui->InvoiceTotal_Label->setText(QString("Invoice Total: $%1").arg(totalCents / 100.0, 0, 'f', 2));
    ui->BalanceDue_Label->setText(QString("Amount Due: $%1").arg(balanceCents / 100.0, 0, 'f', 2));

    // Default the amount to whatever's left owing -- still just a starting
    // point, editable for a partial payment.
    if (balanceCents > 0) {
        ui->Amount_DoubleSpinBox->setValue(balanceCents / 100.0);
    }

}

AddPayment::~AddPayment()
{
    delete ui;
}

void AddPayment::on_Save_PushButton_clicked()
{
    if (ui->Amount_DoubleSpinBox->value() <= 0.0) {
        QMessageBox::warning(this, "Validation Error", "Enter a payment amount greater than $0.00.");
        return;
    }

    Payment payment;
    payment.invoiceId = m_invoiceId;
    payment.paymentDate = ui->PaymentDate_DateEdit->date().toString("yyyy-MM-dd");
    payment.amountCents = qRound(ui->Amount_DoubleSpinBox->value() * 100.0);
    payment.method = ui->Method_ComboBox->currentText();
    payment.notes = ui->Notes_LineEdit->text().trimmed();

    DAO_Payment paymentDao(m_dbManager);
    if (paymentDao.insertPayment(payment) != 0) {
        accept();
    } else {
        QMessageBox::critical(this, "Error", "Failed to save payment to database.");
    }
}

void AddPayment::on_Cancel_PushButton_clicked()
{
    reject();
}