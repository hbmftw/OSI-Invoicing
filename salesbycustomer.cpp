#include "salesbycustomer.h"
#include "ui_salesbycustomer.h"
#include "DatabaseManager.h"
#include "DAO_Reports.h"

#include <QTableWidgetItem>

SalesByCustomer::SalesByCustomer(DatabaseManager &dbManager, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SalesByCustomer)
    , m_dbManager(dbManager)
{
    ui->setupUi(this);
    const QDate today = QDate::currentDate();
    ui->StartDate_DateEdit->setDate(QDate(today.year(), today.month(), 1));
    ui->EndDate_DateEdit->setDate(today);

    runReport();
}

SalesByCustomer::~SalesByCustomer() { delete ui; }

void SalesByCustomer::on_RunReport_PushButton_clicked() { runReport(); }

void SalesByCustomer::runReport()
{
    DAO_Reports reportsDao(m_dbManager);
    QVector<SalesByCustomerRow> rows = reportsDao.getSalesByCustomer(
        ui->StartDate_DateEdit->date(),
        ui->EndDate_DateEdit->date()
        );

    ui->ReportTable_TableWidget->clearContents();
    ui->ReportTable_TableWidget->setRowCount(rows.size());

    for (int r = 0; r < rows.size(); ++r) {
        const SalesByCustomerRow &row = rows[r];
        ui->ReportTable_TableWidget->setItem(r, 0, new QTableWidgetItem(row.clientName));
        ui->ReportTable_TableWidget->setItem(r, 1, new QTableWidgetItem(row.businessName));
        ui->ReportTable_TableWidget->setItem(r, 2, new QTableWidgetItem(QString::number(row.invoiceCount)));

        auto *amountItem = new QTableWidgetItem(QString("$%1").arg(row.totalSalesCents / 100.0, 0, 'f', 2));
        amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ui->ReportTable_TableWidget->setItem(r, 3, amountItem);
    }
}