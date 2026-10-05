#ifndef DAO_PAYMENT_H
#define DAO_PAYMENT_H

#include <QVector>
#include "payment.h"

// Forward declaration
class DatabaseManager;

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

private:
    DatabaseManager& m_dbManager;
};

#endif // DAO_PAYMENT_H
