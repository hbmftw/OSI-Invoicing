#ifndef DAO_PAYMENT_H
#define DAO_PAYMENT_H

#include <QVector>
#include <QString>

#include "payment.h"


// Forward declaration
class DatabaseManager;

struct PaymentListItem {
    int id{0};
    int invoiceId{0};
    QString invoiceNumber;
    QString clientDisplayName;
    QString paymentDate;
    int amountCents{0};
    QString method;
    QString notes;
};

class DAO_Payment
{
public:
    explicit DAO_Payment(DatabaseManager& dbManager);

    // Insert a new payment. Returns the new payment_id, or 0 on failure.
    int insertPayment(const Payment& payment);
    // All payments recorded against a given invoice, oldest first.
    QVector<Payment> getPaymentsForInvoice(int invoiceId) const;

    // Sum of payments recorded against a given invoice, in cents.
    int getTotalPaidForInvoice(int invoiceId) const;

    // New methods for payment_window
    QVector<PaymentListItem> searchPayments(const QString& filterText, const QString& methodFilter = QString()) const;
    bool deletePayment(int paymentId);

    // Paged version of searchPayments() for the Payments list: same matching
    // and ordering, plus the total number of matches for the same filters.
    int countPayments(const QString& filterText, const QString& methodFilter = QString()) const;
    QVector<PaymentListItem> searchPaymentsPage(const QString& filterText, const QString& methodFilter,
                                                int limit, int offset) const;

private:
    DatabaseManager& m_dbManager;
};

#endif // DAO_PAYMENT_H
