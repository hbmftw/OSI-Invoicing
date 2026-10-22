#include "DAO_SalesTaxReport.h"
#include "DatabaseManager.h"
#include "DAO_Company.h"
#include "DAO_Tax.h"
#include "DAO_Items.h"
#include "DAO_Payment.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QHash>
#include <QtGlobal>

DAO_SalesTaxReport::DAO_SalesTaxReport(DatabaseManager &dbManager)
    : m_dbManager(dbManager) {}

SalesTaxSummary DAO_SalesTaxReport::computeSummary(const QDate &startDate, const QDate &endDate) const {
    DAO_Company DAO_Company(m_dbManager);
    const Company company = DAO_Company.getCompany();
    // No company profile saved yet defaults to accrual, since that's the
    // simpler/more common basis and doesn't depend on payment records.
    const bool accrual = !company.exists || company.accrual;

    SalesTaxSummary summary = accrual ? computeAccrualSummary(startDate, endDate)
                                      : computeCashSummary(startDate, endDate);
    summary.startDate = startDate;
    summary.endDate = endDate;
    summary.accrualBasis = accrual;
    return summary;
}

SalesTaxSummary DAO_SalesTaxReport::computeAccrualSummary(const QDate &startDate, const QDate &endDate) const {
    SalesTaxSummary summary;

    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_SalesTaxReport::computeAccrualSummary - Database connection is not open!";
        return summary;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT ii.tax_id AS tax_id, tt.tax_name AS tax_name, tt.tax_rate AS tax_rate, "
        "       SUM(ii.line_total) AS total_cents "
        "FROM invoice_items ii "
        "JOIN invoices i ON i.invoice_id = ii.invoice_id "
        "LEFT JOIN tax_table tt ON tt.tax_id = ii.tax_id "
        "WHERE i.status != 'Void' "
        "  AND i.issue_date BETWEEN :start_date AND :end_date "
        "GROUP BY ii.tax_id, tt.tax_name, tt.tax_rate;"
        );
    query.bindValue(":start_date", startDate.toString("yyyy-MM-dd"));
    query.bindValue(":end_date", endDate.toString("yyyy-MM-dd"));

    if (!query.exec()) {
        qCritical() << "Error computing accrual-basis sales tax summary:" << query.lastError().text();
        return summary;
    }

    while (query.next()) {
        const bool isExempt = query.value("tax_id").isNull();
        const int cents = query.value("total_cents").toInt();

        summary.grossSalesCents += cents;
        if (isExempt) {
            summary.exemptSalesCents += cents;
            continue;
        }

        SalesTaxLine line;
        line.taxId = query.value("tax_id").toInt();
        line.taxName = query.value("tax_name").toString();
        line.ratePercent = query.value("tax_rate").toInt();
        line.taxableSalesCents = cents;
        line.taxOwedCents = qRound(cents * line.ratePercent / 100.0);
        summary.lines.append(line);
        summary.totalTaxOwedCents += line.taxOwedCents;
    }

    summary.taxableSalesCents = summary.grossSalesCents - summary.exemptSalesCents;
    return summary;
}

SalesTaxSummary DAO_SalesTaxReport::computeCashSummary(const QDate &startDate, const QDate &endDate) const {
    SalesTaxSummary summary;

    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_SalesTaxReport::computeCashSummary - Database connection is not open!";
        return summary;
    }

    // Candidate invoices: anything with at least one payment recorded in
    // the window. Whether it actually counts depends on being fully paid
    // off (see class comment).
    QSqlQuery query(db);
    query.prepare(
        "SELECT DISTINCT invoice_id FROM payments WHERE payment_date BETWEEN :start_date AND :end_date;"
        );
    query.bindValue(":start_date", startDate.toString("yyyy-MM-dd"));
    query.bindValue(":end_date", endDate.toString("yyyy-MM-dd"));

    if (!query.exec()) {
        qCritical() << "Error finding candidate invoices for cash-basis summary:" << query.lastError().text();
        return summary;
    }

    QVector<int> candidateInvoiceIds;
    while (query.next()) {
        candidateInvoiceIds.append(query.value("invoice_id").toInt());
    }

    DAO_Tax DAO_Tax(m_dbManager);
    QHash<int, TaxRate> ratesById;
    for (const TaxRate &rate : DAO_Tax.getActiveTaxRates()) {
        ratesById.insert(rate.id, rate);
    }

    DAO_Items itemDao(m_dbManager);
    DAO_Payment DAO_Payment(m_dbManager);

    QHash<int, int> taxableCentsByTaxId; // tax_id -> cents (0 key = exempt)

    for (int invoiceId : candidateInvoiceIds) {
        const QVector<Payment> payments = DAO_Payment.getPaymentsForInvoice(invoiceId);
        if (payments.isEmpty()) {
            continue;
        }

        int totalPaidCents = 0;
        QString latestPaymentDate;
        for (const Payment &payment : payments) {
            totalPaidCents += payment.amountCents;
            if (latestPaymentDate.isEmpty() || payment.paymentDate > latestPaymentDate) {
                latestPaymentDate = payment.paymentDate;
            }
        }

        // Only count invoices completed within the window -- if the
        // completing payment falls outside [start, end], this invoice
        // belongs to a different period's report.
        const QDate completedOn = QDate::fromString(latestPaymentDate, "yyyy-MM-dd");
        if (completedOn < startDate || completedOn > endDate) {
            continue;
        }

        const QVector<InvoiceItem> items = itemDao.getItemsForInvoice(invoiceId);

        int invoiceTotalCents = 0;
        for (const InvoiceItem &item : items) {
            invoiceTotalCents += item.lineTotalCents;
            if (item.taxId != 0 && ratesById.contains(item.taxId)) {
                invoiceTotalCents += qRound(item.lineTotalCents * ratesById.value(item.taxId).ratePercent / 100.0);
            }
        }

        // Not fully paid off yet -- doesn't count as recognized cash-basis
        // revenue in this simplified model. Partial payments toward it
        // will show up once/if a later payment finishes paying it off.
        if (totalPaidCents < invoiceTotalCents) {
            continue;
        }

        for (const InvoiceItem &item : items) {
            taxableCentsByTaxId[item.taxId] += item.lineTotalCents;
        }
    }

    for (auto it = taxableCentsByTaxId.constBegin(); it != taxableCentsByTaxId.constEnd(); ++it) {
        const int taxId = it.key();
        const int cents = it.value();

        summary.grossSalesCents += cents;
        if (taxId == 0 || !ratesById.contains(taxId)) {
            summary.exemptSalesCents += cents;
            continue;
        }

        const TaxRate &rate = ratesById.value(taxId);
        SalesTaxLine line;
        line.taxId = taxId;
        line.taxName = rate.name;
        line.ratePercent = rate.ratePercent;
        line.taxableSalesCents = cents;
        line.taxOwedCents = qRound(cents * rate.ratePercent / 100.0);
        summary.lines.append(line);
        summary.totalTaxOwedCents += line.taxOwedCents;
    }

    summary.taxableSalesCents = summary.grossSalesCents - summary.exemptSalesCents;
    return summary;
}
