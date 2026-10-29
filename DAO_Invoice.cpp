#include "DAO_Invoice.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

DAO_Invoice::DAO_Invoice(DatabaseManager& dbManager)
    : m_dbManager(dbManager) {}

int DAO_Invoice::insertInvoice(const Invoice& invoice) {
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Invoice::insertInvoice - Database connection is not open!";
        return 0;
    }

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO invoices (client_id, invoice_number, issue_date, due_date, status, tax_rate, taxable, discount_amount, po_number, notes) "
        "VALUES (:client_id, :invoice_number, :issue_date, :due_date, :status, :tax_rate, :taxable, :discount_amount, :po_number, :notes);"
        );
    query.bindValue(":client_id", invoice.clientId);
    query.bindValue(":invoice_number", invoice.invoiceNumber);
    query.bindValue(":issue_date", invoice.issueDate);
    query.bindValue(":due_date", invoice.dueDate);
    query.bindValue(":status", invoice.status);
    query.bindValue(":tax_rate", invoice.taxRate);
    query.bindValue(":taxable", invoice.taxable);
    query.bindValue(":discount_amount", invoice.discountAmount);
    query.bindValue(":po_number", invoice.poNumber);
    query.bindValue(":notes", invoice.notes);

    if (!query.exec()) {
        qCritical() << "Error inserting invoice:" << query.lastError().text();
        return 0;
    }

    const QVariant invoiceIdVariant = query.lastInsertId();
    if (!invoiceIdVariant.isValid()) {
        qCritical() << "DAO_Invoice::insertInvoice - Failed to retrieve inserted invoice_id";
        return 0;
    }

    return invoiceIdVariant.toInt();
}

Invoice DAO_Invoice::getInvoiceById(int invoiceId) const {
    Invoice invoice; // id defaults to 0 -> acts as "not found" sentinel

    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Invoice::getInvoiceById - Database connection is not open!";
        return invoice;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT invoice_id, client_id, invoice_number, issue_date, due_date, status, tax_rate, taxable, discount_amount, po_number, notes "
        "FROM invoices "
        "WHERE invoice_id = :id;"
        );
    query.bindValue(":id", invoiceId);

    if (!query.exec()) {
        qCritical() << "Error fetching invoice by id:" << query.lastError().text();
        return invoice;
    }

    if (query.next()) {
        invoice.id = query.value("invoice_id").toInt();
        invoice.clientId = query.value("client_id").toInt();
        invoice.invoiceNumber = query.value("invoice_number").toString();
        invoice.issueDate = query.value("issue_date").toString();
        invoice.dueDate = query.value("due_date").toString();
        invoice.status = query.value("status").toString();
        invoice.taxRate = query.value("tax_rate").toInt();
        invoice.taxable = query.value("taxable").toBool();
        invoice.discountAmount = query.value("discount_amount").toInt();
        invoice.poNumber = query.value("po_number").toString();
        invoice.notes = query.value("notes").toString();
    }

    return invoice;
}

bool DAO_Invoice::updateInvoice(const Invoice& invoice) {
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Invoice::updateInvoice - Database connection is not open!";
        return false;
    }

    // Note: invoice_number and client_id are treated as immutable once created,
    // so they are intentionally left out of this UPDATE.
    QSqlQuery query(db);
    query.prepare(
        "UPDATE invoices SET issue_date = :issue_date, due_date = :due_date, status = :status, "
        "tax_rate = :tax_rate, taxable = :taxable, discount_amount = :discount_amount, po_number = :po_number, notes = :notes "
        "WHERE invoice_id = :invoice_id;"
        );
    query.bindValue(":issue_date", invoice.issueDate);
    query.bindValue(":due_date", invoice.dueDate);
    query.bindValue(":status", invoice.status);
    query.bindValue(":tax_rate", invoice.taxRate);
    query.bindValue(":taxable", invoice.taxable);
    query.bindValue(":discount_amount", invoice.discountAmount);
    query.bindValue(":po_number", invoice.poNumber);
    query.bindValue(":notes", invoice.notes);
    query.bindValue(":invoice_id", invoice.id);

    if (!query.exec()) {
        qCritical() << "DAO_Invoice::updateInvoice - Error updating invoice:" << query.lastError().text();
        return false;
    }

    return true;
}

QVector<Invoice> DAO_Invoice::getInvoicesForClient(int clientId) const {
    QVector<Invoice> results;

    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Invoice::getInvoicesForClient - Database connection is not open!";
        return results;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT invoice_id, client_id, invoice_number, issue_date, due_date, status, tax_rate, taxable, discount_amount, po_number, notes "
        "FROM invoices "
        "WHERE client_id = :client_id "
        "ORDER BY issue_date DESC;"
        );
    query.bindValue(":client_id", clientId);

    if (!query.exec()) {
        qCritical() << "Error fetching invoices for client:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        Invoice invoice;
        invoice.id = query.value("invoice_id").toInt();
        invoice.clientId = query.value("client_id").toInt();
        invoice.invoiceNumber = query.value("invoice_number").toString();
        invoice.issueDate = query.value("issue_date").toString();
        invoice.dueDate = query.value("due_date").toString();
        invoice.status = query.value("status").toString();
        invoice.taxRate = query.value("tax_rate").toInt();
        invoice.taxable = query.value("taxable").toBool();
        invoice.discountAmount = query.value("discount_amount").toInt();
        invoice.poNumber = query.value("po_number").toString();
        invoice.notes = query.value("notes").toString();

        results.append(invoice);
    }

    return results;
}

QVector<InvoiceListItem> DAO_Invoice::searchInvoices(const QString& searchTerm, const QString& statusFilter) const {
    QVector<InvoiceListItem> results;

    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Invoice::searchInvoices - Database connection is not open!";
        return results;
    }

    QString sql =
        "SELECT i.invoice_id, i.invoice_number, i.issue_date, i.due_date, i.status, "
        "       c.first_name, c.last_name, c.business_name "
        "FROM invoices i "
        "LEFT JOIN clients c ON c.client_id = i.client_id "
        "WHERE (i.invoice_number LIKE :term1 "
        "   OR c.first_name LIKE :term2 "
        "   OR c.last_name LIKE :term3 "
        "   OR c.business_name LIKE :term4) ";

    // Status filter values are fixed constants below, never interpolated
    // from caller input, so this is safe to branch on without binding it.
    if (statusFilter == "Paid") {
        sql += "AND i.status = 'Paid' ";
    } else if (statusFilter == "Due") {
        sql += "AND i.status NOT IN ('Paid', 'Void') ";
    }
    sql += "ORDER BY i.issue_date DESC;";

    QSqlQuery query(db);
    query.prepare(sql);

    QString formattedPattern = "%" + searchTerm.trimmed() + "%";
    query.bindValue(":term1", formattedPattern);
    query.bindValue(":term2", formattedPattern);
    query.bindValue(":term3", formattedPattern);
    query.bindValue(":term4", formattedPattern);

    if (!query.exec()) {
        qCritical() << "DAO_Invoice::searchInvoices - Error executing invoice search:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        InvoiceListItem item;
        item.id = query.value("invoice_id").toInt();
        item.invoiceNumber = query.value("invoice_number").toString();
        item.issueDate = query.value("issue_date").toString();
        item.dueDate = query.value("due_date").toString();
        item.status = query.value("status").toString();

        QString clientDisplayName = QString("%1 %2")
                                        .arg(query.value("first_name").toString(), query.value("last_name").toString())
                                        .trimmed();
        QString businessName = query.value("business_name").toString();
        if (!businessName.isEmpty()) {
            clientDisplayName += QString(" (%1)").arg(businessName);
        }
        item.clientDisplayName = clientDisplayName;

        results.append(item);
    }

    return results;
}

namespace {

// Per-invoice totals, computed exactly like DAO_Items::getInvoiceTotalCents()
// (line totals plus each line's tax at the tax table's rate) and the sum of
// recorded payments, so "fully paid" matches what the window displays.
const QString kInvoiceTotalsCte = QStringLiteral(
    "WITH inv_total AS ("
    "  SELECT ii.invoice_id AS invoice_id, "
    "         SUM(ii.line_total + CASE WHEN tt.tax_rate IS NOT NULL "
    "             THEN CAST(ROUND(ii.line_total * tt.tax_rate / 100.0) AS INTEGER) "
    "             ELSE 0 END) AS total_cents "
    "  FROM invoice_items ii "
    "  LEFT JOIN tax_table tt ON tt.tax_id = ii.tax_id "
    "  GROUP BY ii.invoice_id), "
    "inv_paid AS ("
    "  SELECT invoice_id, SUM(amount) AS paid_cents FROM payments GROUP BY invoice_id) ");

QString invoicePageFromWhere(int paymentFilter) {
    QString sql =
        "FROM invoices i "
        "LEFT JOIN clients c ON c.client_id = i.client_id "
        "LEFT JOIN inv_total t ON t.invoice_id = i.invoice_id "
        "LEFT JOIN inv_paid p ON p.invoice_id = i.invoice_id "
        "WHERE (i.invoice_number LIKE :term1 "
        "   OR c.first_name LIKE :term2 "
        "   OR c.last_name LIKE :term3 "
        "   OR c.business_name LIKE :term4) ";

    const QString paid =
        "(COALESCE(t.total_cents, 0) > 0 AND COALESCE(p.paid_cents, 0) >= COALESCE(t.total_cents, 0))";
    if (paymentFilter == 1) {
        sql += "AND " + paid + " ";
    } else if (paymentFilter == 2) {
        sql += "AND NOT " + paid + " ";
    }
    return sql;
}

} // namespace

int DAO_Invoice::countInvoices(const QString& searchTerm, int paymentFilter) const {
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Invoice::countInvoices - Database connection is not open!";
        return 0;
    }

    QSqlQuery query(db);
    query.prepare(kInvoiceTotalsCte + "SELECT COUNT(*) " + invoicePageFromWhere(paymentFilter) + ";");
    const QString pattern = "%" + searchTerm.trimmed() + "%";
    query.bindValue(":term1", pattern);
    query.bindValue(":term2", pattern);
    query.bindValue(":term3", pattern);
    query.bindValue(":term4", pattern);

    if (!query.exec() || !query.next()) {
        qCritical() << "DAO_Invoice::countInvoices - Error:" << query.lastError().text();
        return 0;
    }
    return query.value(0).toInt();
}

QVector<InvoiceListItem> DAO_Invoice::searchInvoicesPage(const QString& searchTerm, int paymentFilter,
                                                         int limit, int offset) const {
    QVector<InvoiceListItem> results;

    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Invoice::searchInvoicesPage - Database connection is not open!";
        return results;
    }

    QSqlQuery query(db);
    query.prepare(
        kInvoiceTotalsCte +
        "SELECT i.invoice_id, i.invoice_number, i.issue_date, i.due_date, i.status, "
        "       c.first_name, c.last_name, c.business_name " +
        invoicePageFromWhere(paymentFilter) +
        "ORDER BY i.issue_date DESC, i.invoice_id DESC "
        "LIMIT :limit OFFSET :offset;"
        );
    const QString pattern = "%" + searchTerm.trimmed() + "%";
    query.bindValue(":term1", pattern);
    query.bindValue(":term2", pattern);
    query.bindValue(":term3", pattern);
    query.bindValue(":term4", pattern);
    query.bindValue(":limit", limit);
    query.bindValue(":offset", offset);

    if (!query.exec()) {
        qCritical() << "DAO_Invoice::searchInvoicesPage - Error:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        InvoiceListItem item;
        item.id = query.value("invoice_id").toInt();
        item.invoiceNumber = query.value("invoice_number").toString();
        item.issueDate = query.value("issue_date").toString();
        item.dueDate = query.value("due_date").toString();
        item.status = query.value("status").toString();

        QString clientDisplayName = QString("%1 %2")
                                        .arg(query.value("first_name").toString(), query.value("last_name").toString())
                                        .trimmed();
        const QString businessName = query.value("business_name").toString();
        if (!businessName.isEmpty()) {
            clientDisplayName += QString(" (%1)").arg(businessName);
        }
        item.clientDisplayName = clientDisplayName;

        results.append(item);
    }
    return results;
}

int DAO_Invoice::getInvoiceIdByNumber(const QString &invoiceNumber) const {
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Invoice::getInvoiceIdByNumber - Database connection is not open!";
        return 0;
    }

    QSqlQuery query(db);
    query.prepare("SELECT invoice_id FROM invoices WHERE invoice_number = :invoice_number;");
    query.bindValue(":invoice_number", invoiceNumber.trimmed());

    if (!query.exec()) {
        qCritical() << "DAO_Invoice::getInvoiceIdByNumber - Error looking up invoice by number:" << query.lastError().text();
        return 0;
    }

    return query.next() ? query.value("invoice_id").toInt() : 0;
}
