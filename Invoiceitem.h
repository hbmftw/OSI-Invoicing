#ifndef INVOICEITEM_H
#define INVOICEITEM_H

#include <QString>

// A single line on an invoice. Mirrors invoice_items; line_total is DB-generated
// (quantity * snapshotUnitPriceCents), so it's read-only here -- set by the DAO
// on read, ignored on write.
struct InvoiceItem {
    int id{0};
    int invoiceId{0};
    int serviceId{0};             // 0 = no linked service (freeform line item)
    int taxId{0};                 // 0 = No Tax / exempt on this line
    QString description;
    double quantity{1.0};
    int snapshotUnitPriceCents{0};
    int lineTotalCents{0};        // read-only; DB-generated
};

#endif // INVOICEITEM_H
