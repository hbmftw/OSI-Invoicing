#ifndef ENTITYMAPPER_H
#define ENTITYMAPPER_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QHash>
#include <QVariant>
#include <QSet>
#include "columnmatcher.h"
#include "invoice.h"
#include "Invoiceitem.h"
#include "payment.h"

// Forward declarations for structs/classes stored inside MappedRecord's QVariant
struct Client;
struct Invoice;
struct Payment;
struct Company;

enum class ImportEntity {
    Unknown = 0,
    Client,
    Invoice,
    Payment,
    Company,
    Item,            // services / products catalog (services table)
    InvoiceHistory   // invoices + line items, one CSV row per line item
};

QString entityToString(ImportEntity entity);
ImportEntity stringToEntity(const QString& name);

namespace EntityMapper {

// Field schemas for matcher
QVector<TargetField> fieldsForEntity(ImportEntity entity);

// Auto-detect entity type by comparing headers & sample values against field schemas
ImportEntity detectEntity(const QStringList& headers, const QVector<QStringList>& sampleRows);

// Unified variant map out structure
struct MappedRecord {
    ImportEntity entity{ImportEntity::Unknown};
    QVariant data; // Holds Client, Invoice, Payment, or Company
};

// Generic mapping function for a given entity type
bool mapRow(ImportEntity entity,
            const QStringList& row,
            const QHash<QString, int>& columns,
            MappedRecord& out,
            QString& error);

// ---------------------------------------------------------------------------
// Invoice history import (one CSV row per invoice line item)
// ---------------------------------------------------------------------------

// One invoice with all of its line items, ready for DAO_Invoice::insertInvoice
// followed by DAO_Items::insertItemsForInvoice.
struct InvoiceHistoryRecord {
    Invoice invoice;
    QVector<InvoiceItem> items;
};

// Everything the builder needs to know about the database. The dialog fills
// this in so the mapper itself stays free of database access. All keys are
// produced with lookupKey().
struct HistoryContext {
    QHash<QString, QVector<int>> clientIdsByKey;  // name / business / email -> client ids
    QHash<QString, int> serviceIdByName;          // services.service_name -> service_id
    QHash<QString, int> taxIdByKey;               // tax name, or "pct:<n>" -> tax_id (-1 = ambiguous)
    QSet<QString> existingInvoiceNumbers;         // invoices already in the database
};

struct HistoryResult {
    enum class State { Ok, Duplicate, Error };
    State state{State::Ok};
    int firstRow{0};                // 0-based index of the first source row involved
    QString message;
    InvoiceHistoryRecord record;    // only meaningful when state == Ok
};

// Trim, drop quotes, collapse whitespace, lower-case.
QString lookupKey(const QString& text);

// ---------------------------------------------------------------------------
// Payment import: matching a payment to an existing invoice
// ---------------------------------------------------------------------------

// What mapRow() produces for a payment row. The invoice is NOT chosen yet:
// the file's invoice number / ID are kept as written, and resolvePayment()
// picks the real invoice_id once it can look at the database.
struct PaymentImportRecord {
    Payment payment;            // payment.invoiceId is filled in by resolvePayment()
    QString invoiceNumber;      // "Invoice #" column as written ("" if not mapped)
    QString sourceInvoiceId;    // "Invoice ID" column as written ("" if not mapped)
};

struct PaymentContext {
    QHash<QString, int> invoiceIdByNumber;    // lookupKey(number) -> invoice_id (-1 = ambiguous)
    QHash<QString, int> invoiceIdByCompact;   // letters+digits only (INV-12 == INV12)
    QHash<int, QString> numberById;           // invoice_id -> invoice_number
    QSet<QString> existingPayments;           // "invoiceId|date|cents|method" already recorded
};

enum class PaymentMatch { Ok, Duplicate, Error };

// Finds the invoice for a payment and fills 'out' (a Payment ready to insert).
// The invoice NUMBER is the match: invoice IDs from another system don't carry
// over, because this app gives every imported invoice a new ID. An ID column is
// only used when the file has no invoice number, and only if that ID exists here.
// 'message' says what it matched, or why it couldn't.
PaymentMatch resolvePayment(const PaymentImportRecord& in, const PaymentContext& ctx,
                            Payment& out, QString& message);

// Groups line-item rows into invoices and validates them. Rows sharing an
// invoice number form one invoice. Rows with a blank invoice number form one
// invoice per customer + date, and get a generated number (IMP-yyyyMMdd-<clientId>).
// An invoice with any bad line is reported as one error and not imported, so
// no invoice is ever imported with missing lines.
QVector<HistoryResult> buildInvoiceHistory(const QVector<QStringList>& rows,
                                           const QHash<QString, int>& columns,
                                           const HistoryContext& ctx);

} // namespace EntityMapper

#endif // ENTITYMAPPER_H