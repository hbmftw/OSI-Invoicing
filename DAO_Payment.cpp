#include "DAO_Payment.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

DAO_Payment::DAO_Payment(DatabaseManager &dbManager)
    : m_dbManager(dbManager) {}

int DAO_Payment::insertPayment(const Payment &payment) {
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Payment::insertPayment - Database connection is not open!";
        return 0;
    }

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO payments (invoice_id, payment_date, amount, method, notes) "
        "VALUES (:invoice_id, :payment_date, :amount, :method, :notes);"
        );
    query.bindValue(":invoice_id", payment.invoiceId);
    query.bindValue(":payment_date", payment.paymentDate);
    query.bindValue(":amount", payment.amountCents);
    query.bindValue(":method", payment.method);
    query.bindValue(":notes", payment.notes);

    if (!query.exec()) {
        qCritical() << "DAO_Payment::insertPayment - Error inserting payment:" << query.lastError().text();
        return 0;
    }

    const QVariant paymentIdVariant = query.lastInsertId();
    if (!paymentIdVariant.isValid()) {
        qCritical() << "DAO_Payment::insertPayment - Failed to retrieve inserted payment_id";
        return 0;
    }

    return paymentIdVariant.toInt();
}

QVector<Payment> DAO_Payment::getPaymentsForInvoice(int invoiceId) const {
    QVector<Payment> results;

    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Payment::getPaymentsForInvoice - Database connection is not open!";
        return results;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT payment_id, invoice_id, payment_date, amount, method, notes "
        "FROM payments "
        "WHERE invoice_id = :invoice_id "
        "ORDER BY payment_date ASC, payment_id ASC;"
        );
    query.bindValue(":invoice_id", invoiceId);

    if (!query.exec()) {
        qCritical() << "DAO_Payment::getPaymentsForInvoice - Error fetching payments for invoice:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        Payment payment;
        payment.id = query.value("payment_id").toInt();
        payment.invoiceId = query.value("invoice_id").toInt();
        payment.paymentDate = query.value("payment_date").toString();
        payment.amountCents = query.value("amount").toInt();
        payment.method = query.value("method").toString();
        payment.notes = query.value("notes").toString();
        results.append(payment);
    }

    return results;
}

int DAO_Payment::getTotalPaidForInvoice(int invoiceId) const {
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Payment::getTotalPaidForInvoice - Database connection is not open!";
        return 0;
    }

    QSqlQuery query(db);
    query.prepare("SELECT COALESCE(SUM(amount), 0) AS total FROM payments WHERE invoice_id = :invoice_id;");
    query.bindValue(":invoice_id", invoiceId);

    if (!query.exec() || !query.next()) {
        qCritical() << "DAO_Payment::getTotalPaidForInvoice - Error summing payments for invoice:" << query.lastError().text();
        return 0;
    }

    return query.value("total").toInt();
}
