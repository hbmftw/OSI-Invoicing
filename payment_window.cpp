#include "payment_window.h"
#include "ui_payment_window.h"
#include "DatabaseManager.h"
#include "DAO_Payment.h"
#include "addPayment.h"

#include <QMessageBox>
#include <QPushButton>
#include <QAction>

Payment_Window::Payment_Window(DatabaseManager& dbManager, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Payment_Window)
    ,m_dbManager(dbManager)
{
    ui->setupUi(this);

    ui->paymentsTableWidget->setColumnCount(7);
    ui->paymentsTableWidget->setHorizontalHeaderLabels({"Date", "Invoice #", "Client", "Method", "Amount", "Notes", ""});

    ui->PaymentSearch_Field->setPlaceholderText("Search Invoices, Clients, Notes...");
    ui->PaymentSearch_Field->setClearButtonEnabled(true);

    QAction *searchAction = new QAction(this);
    searchAction->setIcon(QIcon(":/new/prefix1/search.png"));
    ui->PaymentSearch_Field->addAction(searchAction, QLineEdit::LeadingPosition);

    refreshPaymentsTable();
}

Payment_Window::~Payment_Window()
{
    delete ui;
}

void Payment_Window::refresh() {
    refreshPaymentsTable();
}

void Payment_Window::on_PaymentSearch_Field_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    refreshPaymentsTable();
}

void Payment_Window::on_MethodFilter_ComboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    refreshPaymentsTable();
}

void Payment_Window::refreshPaymentsTable()
{
    DAO_Payment paymentDao(m_dbManager);
    const QString searchText = ui->PaymentSearch_Field->text();
    const QString methodFilter = ui->MethodFilter_ComboBox->currentText();

    QVector<PaymentListItem> results = paymentDao.searchPayments(searchText, methodFilter);
    populatePaymentsTable(results);
}

void Payment_Window::populatePaymentsTable(const QVector<PaymentListItem> &payments)
{
    ui->paymentsTableWidget->clearContents();
    ui->paymentsTableWidget->setRowCount(payments.size());

    for (int row = 0; row < payments.size(); ++row) {
        const PaymentListItem &payment = payments[row];

        ui->paymentsTableWidget->setItem(row, 0, new QTableWidgetItem(payment.paymentDate));
        ui->paymentsTableWidget->setItem(row, 1, new QTableWidgetItem(payment.invoiceNumber));
        ui->paymentsTableWidget->setItem(row, 2, new QTableWidgetItem(payment.clientDisplayName));
        ui->paymentsTableWidget->setItem(row, 3, new QTableWidgetItem(payment.method));

        auto *amountItem = new QTableWidgetItem(QString("$%1").arg(payment.amountCents / 100.0, 0, 'f', 2));
        amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter); //[cite: 3]
        ui->paymentsTableWidget->setItem(row, 4, amountItem); //[cite: 3]

        ui->paymentsTableWidget->setItem(row, 5, new QTableWidgetItem(payment.notes));

        const int paymentId = payment.id;
        QPushButton *deleteBtn = new QPushButton("Delete", this);
        connect(deleteBtn, &QPushButton::clicked, this, [this, paymentId]() {
            deletePayment(paymentId);
        });
        ui->paymentsTableWidget->setCellWidget(row, 6, deleteBtn);
    }
}

void Payment_Window::deletePayment(int paymentId)
{
    if (QMessageBox::question(this, "Confirm Delete", "Are you sure you want to delete this payment?",
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        DAO_Payment paymentDao(m_dbManager);
        if (paymentDao.deletePayment(paymentId)) {
            refreshPaymentsTable();
        }
    }
}

void Payment_Window::on_NewPayment_Button_clicked()
{
    AddPayment dialog(m_dbManager, 0, this);
    if (dialog.exec() == QDialog::Accepted) {
        refreshPaymentsTable();
    }
}
