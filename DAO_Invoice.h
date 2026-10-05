#ifndef DAO_INVOICE_H
#define DAO_INVOICE_H

#include <QVector>
#include <QString>
#include "invoice.h"

// Forward declaration
class DatabaseManager;

// Lightweight, read-only row shape for list/search views. Joins in the
// client's display name so callers don't need a separate ClientDao lookup
// per row.
struct InvoiceListItem {
    int id{0};
    QString invoiceNumber;
    QString clientDisplayName;
    QString issueDate;
    QString dueDate;
    QString status;
};

// Invoice Data Access Object
class DAO_Invoice
{
public:
    explicit DAO_Invoice(DatabaseManager& dbManager);

    // Insert a new invoice. Returns the new invoice_id, or 0 on failure.
    int insertInvoice(const Invoice& invoice);

    // Fetch a single invoice by id (returned Invoice.id == 0 if not found)
    Invoice getInvoiceById(int invoiceId) const;

    // Update an existing invoice's editable fields (does not touch invoice_number)
    bool updateInvoice(const Invoice& invoice);

    // Fetch invoices belonging to a given client, most recently issued first
    QVector<Invoice> getInvoicesForClient(int clientId) const;

    // Search invoices by invoice number or client name/business, most recently issued first.
    // statusFilter: "" (default) = no filter, "Paid" = status == "Paid",
    // "Due" = status NOT IN ("Paid", "Void") -- i.e. still outstanding,
    // whatever its exact status (Draft/Sent/Overdue). Void is excluded from
    // "Due" since a voided invoice was cancelled, not outstanding.
    QVector<InvoiceListItem> searchInvoices(const QString& searchTerm, const QString& statusFilter = QString()) const;

    // Exact invoice_number lookup. Returns 0 if not found. Used by the
    // Receive Payment flow to resolve a typed invoice number to an id.
    int getInvoiceIdByNumber(const QString& invoiceNumber) const;

private:
    DatabaseManager& m_dbManager;
};

#endif // DAO_INVOICE_H
