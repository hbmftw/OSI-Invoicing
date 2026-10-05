#ifndef DAO_ITEMS_H
#define DAO_ITEMS_H

#include <QVector>
#include "Invoiceitem.h"

// Forward declaration
class DatabaseManager;

// DAO for invoice_items. Callers (e.g. AddInvoiceDialog) currently save an
// invoice's items as "delete everything for this invoice, then re-insert
// current rows" via replaceItemsForInvoice() rather than diffing row-by-row
// -- simpler, and fine for the size of invoice this app deals with.

class DAO_Items
{

public:
    explicit DAO_Items(DatabaseManager& dbManager);

    // Fetch all line items for an invoice, in insertion order.
    QVector<InvoiceItem> getItemsForInvoice(int invoiceId) const;

    // Deletes every existing item for invoiceId, then inserts items (each
    // item's .invoiceId is overwritten with invoiceId regardless of what's
    // set on it). Runs as a single transaction. Returns false and leaves
    // existing rows untouched if anything fails.
    bool replaceItemsForInvoice(int invoiceId, const QVector<InvoiceItem>& items);

    // Tax-inclusive total for an invoice, in cents: sum of line_total plus
    // each line's tax (looked up by tax_id against tax_table's current
    // rate, regardless of whether that rate is still active -- a rate
    // deactivated later shouldn't retroactively change an existing
    // invoice's total). Returns 0 for an invoice with no items.
    int getInvoiceTotalCents(int invoiceId) const;

private:
    DatabaseManager& m_dbManager;

};

#endif // DAO_ITEMS_H
