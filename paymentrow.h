#ifndef PAYMENTROW_H
#define PAYMENTROW_H

#include <QDate>
#include <QString>

// One row of the Payments window: a payment joined with its invoice and client.
struct PaymentRow {
    int paymentId{0};
    int invoiceId{0};
    QString invoiceNumber;
    QString clientName;
    QString businessName;
    QDate paymentDate;
    qint64 amountCents{0};
    QString method;
    QString notes;
};

#endif // PAYMENTROW_H
