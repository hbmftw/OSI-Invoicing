#include "salesbyitem.h"
#include "ui_salesbyitem.h"
#include "DatabaseManager.h"
#include "DAO_Reports.h"

#include <QTableWidgetItem>

SalesByItem::SalesByItem(DatabaseManager &dbManager, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SalesByItem)
    , m_dbManager(dbManager)
{
    ui->setupUi(this);
    const QDate today = QDate::currentDate();
    ui->StartDate_DateEdit->setDate(QDate(today.year(), today.month(), 1));
    ui->EndDate_DateEdit->setDate(today);

    runReport();
}

SalesByItem::~SalesByItem() { delete ui; }

void SalesByItem::on_RunReport_PushButton_clicked() { runReport(); }

void SalesByItem::runReport()
{
    DAO_Reports reportsDao(m_dbManager);
    QVector<SalesByItemRow> rows = reportsDao.getSalesByItem(
        ui->StartDate_DateEdit->date(),
        ui->EndDate_DateEdit->date()
        );

    ui->ReportTable_TableWidget->clearContents();
    ui->ReportTable_TableWidget->setRowCount(rows.size());

    for (int r = 0; r < rows.size(); ++r) {
        const SalesByItemRow &row = rows[r];
        ui->ReportTable_TableWidget->setItem(r, 0, new QTableWidgetItem(row.description));
        ui->ReportTable_TableWidget->setItem(r, 1, new QTableWidgetItem(QString::number(row.totalQuantity, 'f', 2)));

        auto *amountItem = new QTableWidgetItem(QString("$%1").arg(row.totalAmountCents / 100.0, 0, 'f', 2));
        amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ui->ReportTable_TableWidget->setItem(r, 2, amountItem);
    }
}