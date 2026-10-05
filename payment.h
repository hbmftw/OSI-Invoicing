#ifndef PAYMENT_H
#define PAYMENT_H

#include <QString>

struct Payment {
    int id{0};
    int invoiceId{0};
    QString paymentDate;   // "yyyy-MM-dd"
    int amountCents{0};
    QString method;        // e.g. "Check", "Card", "Cash", "ACH" -- freeform for now
    QString notes;
};

#endif // PAYMENT_H
