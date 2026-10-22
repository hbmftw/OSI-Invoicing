#include "arsummary.h"
#include "ui_arsummary.h"
#include "DatabaseManager.h"
#include "DAO_Reports.h"

#include <QTableWidgetItem>

ArSummary::ArSummary(DatabaseManager &dbManager, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ArSummary)
    , m_dbManager(dbManager)
{
    ui->setupUi(this);
    runReport();
}

ArSummary::~ArSummary() { delete ui; }

void ArSummary::on_RunReport_PushButton_clicked() { runReport(); }

void ArSummary::runReport()
{
    DAO_Reports reportsDao(m_dbManager);
    QVector<ArSummaryRow> rows = reportsDao.getArSummary();

    ui->ReportTable_TableWidget->clearContents();
    ui->ReportTable_TableWidget->setRowCount(rows.size());

    for (int r = 0; r < rows.size(); ++r) {
        const ArSummaryRow &row = rows[r];
        ui->ReportTable_TableWidget->setItem(r, 0, new QTableWidgetItem(row.invoiceNumber));
        ui->ReportTable_TableWidget->setItem(r, 1, new QTableWidgetItem(row.clientName));
        ui->ReportTable_TableWidget->setItem(r, 2, new QTableWidgetItem(row.dueDate));

        auto *balanceItem = new QTableWidgetItem(QString("$%1").arg(row.balanceCents / 100.0, 0, 'f', 2));
        balanceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ui->ReportTable_TableWidget->setItem(r, 3, balanceItem);
    }
}