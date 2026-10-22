#include "DAO_Reports.h"
#include "DatabaseManager.h"
#include "DAO_Items.h"
#include "DAO_Payment.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

DAO_Reports::DAO_Reports(DatabaseManager &dbManager)
    : m_dbManager(dbManager) {}

QVector<SalesByItemRow> DAO_Reports::getSalesByItem(const QDate& startDate, const QDate& endDate) const {
    QVector<SalesByItemRow> results;

    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Reports::getSalesByItem - Database connection is not open!";
        return results;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT ii.description, SUM(ii.quantity) AS total_qty, SUM(ii.line_total) AS total_amount "
        "FROM invoice_items ii "
        "JOIN invoices i ON i.invoice_id = ii.invoice_id "
        "WHERE i.status != 'Void' "
        "  AND i.issue_date BETWEEN :start AND :end "
        "GROUP BY ii.description "
        "ORDER BY total_amount DESC;"
        );
    query.bindValue(":start", startDate.toString("yyyy-MM-dd"));
    query.bindValue(":end", endDate.toString("yyyy-MM-dd"));

    if (!query.exec()) {
        qCritical() << "DAO_Reports::getSalesByItem - Query error:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        SalesByItemRow row;
        row.description = query.value("description").toString();
        row.totalQuantity = query.value("total_qty").toDouble();
        row.totalAmountCents = query.value("total_amount").toInt();
        results.append(row);
    }

    return results;
}

QVector<SalesByCustomerRow> DAO_Reports::getSalesByCustomer(const QDate& startDate, const QDate& endDate) const {
    QVector<SalesByCustomerRow> results;

    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Reports::getSalesByCustomer - Database connection is not open!";
        return results;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT c.first_name || ' ' || c.last_name AS client_name, c.business_name, "
        "       COUNT(DISTINCT i.invoice_id) AS invoice_count, SUM(ii.line_total) AS total_sales "
        "FROM clients c "
        "JOIN invoices i ON i.client_id = c.client_id "
        "JOIN invoice_items ii ON ii.invoice_id = i.invoice_id "
        "WHERE i.status != 'Void' "
        "  AND i.issue_date BETWEEN :start AND :end "
        "GROUP BY c.client_id "
        "ORDER BY total_sales DESC;"
        );
    query.bindValue(":start", startDate.toString("yyyy-MM-dd"));
    query.bindValue(":end", endDate.toString("yyyy-MM-dd"));

    if (!query.exec()) {
        qCritical() << "DAO_Reports::getSalesByCustomer - Query error:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        SalesByCustomerRow row;
        row.clientName = query.value("client_name").toString();
        row.businessName = query.value("business_name").toString();
        row.invoiceCount = query.value("invoice_count").toInt();
        row.totalSalesCents = query.value("total_sales").toInt();
        results.append(row);
    }

    return results;
}

QVector<ArSummaryRow> DAO_Reports::getArSummary() const {
    QVector<ArSummaryRow> results;

    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Reports::getArSummary - Database connection is not open!";
        return results;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT i.invoice_id, i.invoice_number, i.due_date, "
        "       c.first_name || ' ' || c.last_name AS client_name "
        "FROM invoices i "
        "LEFT JOIN clients c ON c.client_id = i.client_id "
        "WHERE i.status NOT IN ('Paid', 'Void') "
        "ORDER BY i.due_date ASC;"
        );

    if (!query.exec()) {
        qCritical() << "DAO_Reports::getArSummary - Query error:" << query.lastError().text();
        return results;
    }

    DAO_Items itemDao(m_dbManager);
    DAO_Payment paymentDao(m_dbManager);

    while (query.next()) {
        int invoiceId = query.value("invoice_id").toInt();
        int totalCents = itemDao.getInvoiceTotalCents(invoiceId);
        int paidCents = paymentDao.getTotalPaidForInvoice(invoiceId);
        int balanceCents = totalCents - paidCents;

        if (balanceCents <= 0) continue; // Fully paid off

        ArSummaryRow row;
        row.invoiceId = invoiceId;
        row.invoiceNumber = query.value("invoice_number").toString();
        row.clientName = query.value("client_name").toString();
        row.dueDate = query.value("due_date").toString();
        row.balanceCents = balanceCents;
        results.append(row);
    }

    return results;
}