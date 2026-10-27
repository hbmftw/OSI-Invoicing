#include "salestaxreport.h"
#include "ui_salestaxreport.h"
#include "DatabaseManager.h"
#include "DAO_SalesTaxReport.h"

#include <QDate>
#include <QTableWidgetItem>

namespace {
QString formatCents(int cents)
{
    return QString("$%1").arg(cents / 100.0, 0, 'f', 2);
}
}

SalesTaxReport::SalesTaxReport(DatabaseManager &dbManager, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SalesTaxReport)
    , m_dbManager(dbManager)
{
    ui->setupUi(this);

    // Default to the current month; a reasonable starting point for a
    // sales tax filing period without guessing at quarterly/annual cadence.
    const QDate today = QDate::currentDate();
    ui->StartDate_DateEdit->setDate(QDate(today.year(), today.month(), 1));
    ui->EndDate_DateEdit->setDate(today);

    runReport();
}

SalesTaxReport::~SalesTaxReport()
{
    delete ui;
}

void SalesTaxReport::on_RunReport_PushButton_clicked()
{
    runReport();
}

void SalesTaxReport::runReport()
{
    const QDate startDate = ui->StartDate_DateEdit->date();
    const QDate endDate = ui->EndDate_DateEdit->date();

    DAO_SalesTaxReport reportDao(m_dbManager);
    const SalesTaxSummary summary = reportDao.computeSummary(startDate, endDate);

    ui->Basis_Label->setText(QString("Basis: %1").arg(summary.accrualBasis ? "Accrual" : "Cash"));
    ui->GrossSales_Label->setText(formatCents(summary.grossSalesCents));
    ui->ExemptSales_Label->setText(formatCents(summary.exemptSalesCents));
    ui->TaxableSales_Label->setText(formatCents(summary.taxableSalesCents));
    ui->TotalTaxOwed_Label->setText(formatCents(summary.totalTaxOwedCents));

    ui->TaxLines_TableWidget->clearContents();
    ui->TaxLines_TableWidget->setRowCount(summary.lines.size());
    for (int row = 0; row < summary.lines.size(); ++row) {
        const SalesTaxLine &line = summary.lines[row];
        ui->TaxLines_TableWidget->setItem(row, 0, new QTableWidgetItem(line.taxName));
        ui->TaxLines_TableWidget->setItem(row, 1, new QTableWidgetItem(QString("%1%").arg(line.ratePercent)));
        ui->TaxLines_TableWidget->setItem(row, 2, new QTableWidgetItem(formatCents(line.taxableSalesCents)));
        ui->TaxLines_TableWidget->setItem(row, 3, new QTableWidgetItem(formatCents(line.taxOwedCents)));
    }
}
