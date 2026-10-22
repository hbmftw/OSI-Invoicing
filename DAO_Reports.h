#ifndef DAO_REPORTS_H
#define DAO_REPORTS_H

#include <QVector>
#include <QString>
#include <QDate>

class DatabaseManager;

// Data structures for report rows
struct SalesByItemRow {
    QString description;
    double totalQuantity{0.0};
    int totalAmountCents{0};
};

struct SalesByCustomerRow {
    QString clientName;
    QString businessName;
    int invoiceCount{0};
    int totalSalesCents{0};
};

struct ArSummaryRow {
    int invoiceId{0};
    QString invoiceNumber;
    QString clientName;
    QString dueDate;
    int balanceCents{0};
};

class DAO_Reports
{
public:
    explicit DAO_Reports(DatabaseManager& dbManager);

    // Sales by Item query
    QVector<SalesByItemRow> getSalesByItem(const QDate& startDate, const QDate& endDate) const;

    // Sales by Customer query
    QVector<SalesByCustomerRow> getSalesByCustomer(const QDate& startDate, const QDate& endDate) const;

    // Accounts Receivable Summary query
    QVector<ArSummaryRow> getArSummary() const;

private:
    DatabaseManager& m_dbManager;
};

#endif // DAO_REPORTS_H