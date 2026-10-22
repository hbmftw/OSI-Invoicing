#ifndef DAO_SALESTAXREPORT_H
#define DAO_SALESTAXREPORT_H

#include <QDate>
#include <QString>
#include <QVector>

// Forward declaration
class DatabaseManager;

// One row per user-defined tax rate that had taxable sales in the period.
struct SalesTaxLine {
    int taxId{0};
    QString taxName;
    int ratePercent{0};
    int taxableSalesCents{0};   // sales at this specific rate
    int taxOwedCents{0};        // taxableSalesCents * ratePercent / 100
};

struct SalesTaxSummary {
    QDate startDate;
    QDate endDate;
    bool accrualBasis{true};    // mirrors company_profile.accrual at the time the report ran

    int grossSalesCents{0};     // every line item in the period, taxed or exempt
    int exemptSalesCents{0};    // lines with No Tax (tax_id NULL), or on a Tax Exempt invoice
    int taxableSalesCents{0};   // grossSalesCents - exemptSalesCents

    QVector<SalesTaxLine> lines; // one per tax rate with taxable sales; excludes the exempt bucket
    int totalTaxOwedCents{0};    // sum of lines[].taxOwedCents
};

// Aggregates invoice_items into a sales tax report for a date range.
//
// Basis is read from CompanyDAO::getCompany().accrual at call time:
//   - Accrual: sales are dated by invoices.issue_date. All non-Void
//     invoices in range count, regardless of payment status.
//   - Cash: sales are dated by when an invoice was actually paid off.
//     SIMPLIFICATION (v1): only invoices that are FULLY paid count, dated
//     on their completing (most recent) payment. An invoice with partial
//     payments recorded in-range but not yet fully paid off contributes
//     nothing until it's fully paid. Proportionally allocating a partial
//     payment across an invoice's line items is not implemented.
//     This could change, but more research is required on the laws....
class DAO_SalesTaxReport
{
public:
    explicit DAO_SalesTaxReport(DatabaseManager& dbManager);

    SalesTaxSummary computeSummary(const QDate& startDate, const QDate& endDate) const;

private:
    DatabaseManager& m_dbManager;

    SalesTaxSummary computeAccrualSummary(const QDate& startDate, const QDate& endDate) const;
    SalesTaxSummary computeCashSummary(const QDate& startDate, const QDate& endDate) const;
};

#endif // DAO_SALESTAXREPORT_H
