#include "DAO_Items.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

DAO_Items::DAO_Items(DatabaseManager &dbManager)
    : m_dbManager(dbManager) {}

QVector<InvoiceItem> DAO_Items::getItemsForInvoice(int invoiceId) const {
    QVector<InvoiceItem> results;

    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Items::getItemsForInvoice - Database connection is not open!";
        return results;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT item_id, invoice_id, service_id, tax_id, description, quantity, snapshot_unit_price, line_total "
        "FROM invoice_items "
        "WHERE invoice_id = :invoice_id "
        "ORDER BY item_id ASC;"
        );
    query.bindValue(":invoice_id", invoiceId);

    if (!query.exec()) {
        qCritical() << "DAO_Items::getItemsForInvoice - Error fetching invoice items:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        InvoiceItem item;
        item.id = query.value("item_id").toInt();
        item.invoiceId = query.value("invoice_id").toInt();
        item.serviceId = query.value("service_id").toInt();
        item.taxId = query.value("tax_id").toInt();
        item.description = query.value("description").toString();
        item.quantity = query.value("quantity").toDouble();
        item.snapshotUnitPriceCents = query.value("snapshot_unit_price").toInt();
        item.lineTotalCents = query.value("line_total").toInt();
        results.append(item);
    }

    return results;
}

bool DAO_Items::replaceItemsForInvoice(int invoiceId, const QVector<InvoiceItem> &items) {
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Items::replaceItemsForInvoice - Database connection is not open!";
        return false;
    }

    db.transaction();

    QSqlQuery deleteQuery(db);
    deleteQuery.prepare("DELETE FROM invoice_items WHERE invoice_id = :invoice_id;");
    deleteQuery.bindValue(":invoice_id", invoiceId);
    if (!deleteQuery.exec()) {
        qCritical() << "DAO_Items::replaceItemsForInvoice - Error clearing existing invoice items:" << deleteQuery.lastError().text();
        db.rollback();
        return false;
    }

    QSqlQuery insertQuery(db);
    insertQuery.prepare(
        "INSERT INTO invoice_items (invoice_id, service_id, tax_id, description, quantity, snapshot_unit_price) "
        "VALUES (:invoice_id, :service_id, :tax_id, :description, :quantity, :snapshot_unit_price);"
        );

    for (const InvoiceItem &item : items) {
        insertQuery.bindValue(":invoice_id", invoiceId);
        // service_id and tax_id are nullable FKs; 0 means "not set" in our
        // struct, so bind SQL NULL instead of a literal 0 (which would
        // otherwise dangle as a foreign key to a row that doesn't exist).
        insertQuery.bindValue(":service_id", item.serviceId == 0 ? QVariant(QMetaType(QMetaType::Int)) : QVariant(item.serviceId));
        insertQuery.bindValue(":tax_id", item.taxId == 0 ? QVariant(QMetaType(QMetaType::Int)) : QVariant(item.taxId));
        insertQuery.bindValue(":description", item.description);
        insertQuery.bindValue(":quantity", item.quantity);
        insertQuery.bindValue(":snapshot_unit_price", item.snapshotUnitPriceCents);

        if (!insertQuery.exec()) {
            qCritical() << "DAO_Items::replaceItemsForInvoice - Error inserting invoice item:" << insertQuery.lastError().text();
            db.rollback();
            return false;
        }
    }

    if (!db.commit()) {
        qCritical() << "DAO_Items::replaceItemsForInvoice - Failed to commit transaction:" << db.lastError().text();
        db.rollback();
        return false;
    }

    return true;
}

int DAO_Items::getInvoiceTotalCents(int invoiceId) const {
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Items::getInvoiceTotalCents - Database connection is not open!";
        return 0;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT COALESCE(SUM("
        "  ii.line_total + CASE WHEN tt.tax_rate IS NOT NULL "
        "    THEN CAST(ROUND(ii.line_total * tt.tax_rate / 100.0) AS INTEGER) "
        "    ELSE 0 END"
        "), 0) AS total_cents "
        "FROM invoice_items ii "
        "LEFT JOIN tax_table tt ON tt.tax_id = ii.tax_id "
        "WHERE ii.invoice_id = :invoice_id;"
        );
    query.bindValue(":invoice_id", invoiceId);

    if (!query.exec() || !query.next()) {
        qCritical() << "DAO_Items::getInvoiceTotalCents - Error computing invoice total:" << query.lastError().text();
        return 0;
    }

    return query.value("total_cents").toInt();
}
