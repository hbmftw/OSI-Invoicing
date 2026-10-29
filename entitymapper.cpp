#include "entitymapper.h"
#include <QDate>
#include <QRegularExpression>
#include <algorithm>

// Models
#include "client.h"
#include "invoice.h"
#include "payment.h"
#include "DAO_Company.h"
#include "clientimportmapper.h"
#include "item.h"

namespace {

QString cell(const QStringList& row, const QHash<QString, int>& columns, const QString& key) {
    const int col = columns.value(key, -1);
    return (col >= 0 && col < row.size()) ? row.at(col).trimmed() : QString();
}

bool isTruthy(const QString& value) {
    static const QStringList yes{QStringLiteral("1"), QStringLiteral("y"),
                                 QStringLiteral("yes"), QStringLiteral("true")};
    return yes.contains(value.trimmed().toLower());
}

int parseCents(const QString& val) {
    QString s = val;
    s.remove('$');
    s.remove(',');
    bool ok = false;
    double d = s.toDouble(&ok);
    return ok ? static_cast<int>(qRound(d * 100.0)) : 0;
}

// Dates are stored and reported as yyyy-MM-dd text (the reports compare them
// as strings), so anything imported has to be converted. Returns an empty
// string when the text isn't a recognizable date. Slash dates are read as
// month/day/year.
QString normalizeDate(const QString& raw)
{
    QString s = raw.trimmed();
    if (s.isEmpty())
        return QString();

    static const QRegularExpression timePart(QStringLiteral("(?:T|\\s+)\\d{1,2}:\\d{2}.*$"));
    s.remove(timePart);
    s = s.trimmed();

    static const QStringList longYear{
        QStringLiteral("yyyy-MM-dd"),  QStringLiteral("yyyy-M-d"),
        QStringLiteral("yyyy/M/d"),    QStringLiteral("M/d/yyyy"),
        QStringLiteral("M-d-yyyy"),    QStringLiteral("d-MMM-yyyy"),
        QStringLiteral("d MMM yyyy"),  QStringLiteral("MMM d, yyyy"),
        QStringLiteral("MMMM d, yyyy")};
    for (const QString& f : longYear) {
        const QDate d = QDate::fromString(s, f);
        if (d.isValid() && d.year() >= 1900 && d.year() <= 2200)
            return d.toString(QStringLiteral("yyyy-MM-dd"));
    }

    static const QStringList shortYear{
        QStringLiteral("M/d/yy"), QStringLiteral("M-d-yy"), QStringLiteral("d-MMM-yy")};
    for (const QString& f : shortYear) {
        QDate d = QDate::fromString(s, f);
        if (d.isValid()) {
            if (d.year() < 1970)
                d = d.addYears(100);   // Qt reads "24" as 1924
            return d.toString(QStringLiteral("yyyy-MM-dd"));
        }
    }
    return QString();
}

// For the older header-only invoice/payment import: convert when we can,
// otherwise keep what the file said.
QString dateOrRaw(const QString& raw)
{
    const QString iso = normalizeDate(raw);
    return iso.isEmpty() ? raw : iso;
}

// "$1,234.50", "(12.00)" and "-5" all work. False when it isn't a number.
bool parseMoneyCents(const QString& text, int& cents)
{
    QString s = text.trimmed();
    bool negative = false;
    if (s.startsWith(QLatin1Char('(')) && s.endsWith(QLatin1Char(')'))) {
        negative = true;
        s = s.mid(1, s.size() - 2);
    }
    s.remove(QLatin1Char('$'));
    s.remove(QLatin1Char(','));
    s.remove(QLatin1Char(' '));
    bool ok = false;
    const double d = s.toDouble(&ok);
    if (!ok)
        return false;
    cents = static_cast<int>(qRound64(d * 100.0));
    if (negative)
        cents = -qAbs(cents);
    return true;
}

bool parseNumber(const QString& text, double& value)
{
    QString s = text.trimmed();
    s.remove(QLatin1Char(','));
    bool ok = false;
    value = s.toDouble(&ok);
    return ok;
}

QString money(int cents)
{
    return QStringLiteral("%1$%2").arg(cents < 0 ? QStringLiteral("-") : QString())
        .arg(qAbs(cents) / 100.0, 0, 'f', 2);
}

bool isTaxableWord(const QString& v)
{
    static const QStringList yes{QStringLiteral("1"), QStringLiteral("y"), QStringLiteral("yes"),
                                 QStringLiteral("true"), QStringLiteral("t"), QStringLiteral("x"),
                                 QStringLiteral("tax"), QStringLiteral("taxable")};
    return yes.contains(v.trimmed().toLower());
}

bool isInactiveWord(const QString& v)
{
    static const QStringList no{QStringLiteral("0"), QStringLiteral("n"), QStringLiteral("no"),
                                QStringLiteral("false"), QStringLiteral("f"),
                                QStringLiteral("inactive"), QStringLiteral("archived"),
                                QStringLiteral("deleted"), QStringLiteral("disabled")};
    return no.contains(v.trimmed().toLower());
}

// Source statuses vary a lot; the app uses Draft/Sent/Paid/Overdue/Void.
// Anything unrecognized (open, unpaid, partial...) becomes Sent. A blank
// stays blank so the caller can apply its own default.
QString normalizeStatus(const QString& raw)
{
    const QString s = raw.trimmed().toLower();
    if (s.isEmpty())
        return QString();
    if (s == QLatin1String("paid"))
        return QStringLiteral("Paid");
    if (s == QLatin1String("draft"))
        return QStringLiteral("Draft");
    if (s == QLatin1String("void") || s == QLatin1String("voided")
        || s == QLatin1String("cancelled") || s == QLatin1String("canceled"))
        return QStringLiteral("Void");
    if (s == QLatin1String("overdue") || s == QLatin1String("past due"))
        return QStringLiteral("Overdue");
    return QStringLiteral("Sent");
}

} // namespace

QString entityToString(ImportEntity entity) {
    switch (entity) {
    case ImportEntity::Client: return QStringLiteral("Clients");
    case ImportEntity::Invoice: return QStringLiteral("Invoices");
    case ImportEntity::Payment: return QStringLiteral("Payments");
    case ImportEntity::Company: return QStringLiteral("Company Profile");
    case ImportEntity::Item: return QStringLiteral("Services / Products");
    case ImportEntity::InvoiceHistory: return QStringLiteral("Invoice History");
    default: return QStringLiteral("Unknown");
    }
}

ImportEntity stringToEntity(const QString& name) {
    if (name == QStringLiteral("Clients")) return ImportEntity::Client;
    if (name == QStringLiteral("Invoices")) return ImportEntity::Invoice;
    if (name == QStringLiteral("Payments")) return ImportEntity::Payment;
    if (name == QStringLiteral("Company Profile")) return ImportEntity::Company;
    if (name == QStringLiteral("Services / Products")) return ImportEntity::Item;
    if (name == QStringLiteral("Invoice History")) return ImportEntity::InvoiceHistory;
    return ImportEntity::Unknown;
}

namespace EntityMapper {

QVector<TargetField> fieldsForEntity(ImportEntity entity) {
    using T = TargetField::Type;
    switch (entity) {
    case ImportEntity::Client:
        return ClientImport::fields();

    case ImportEntity::Invoice:
        return {
            {"invoiceNumber", "Invoice #", {"invoice", "invoice_number", "inv #", "invoice no"}, T::Text},
            {"clientId", "Client ID", {"client_id", "customer_id", "client id"}, T::Integer},
            {"issueDate", "Issue Date", {"issue_date", "date", "invoice date", "created_at"}, T::Date},
            {"dueDate", "Due Date", {"due_date", "payment_due", "due"}, T::Date},
            {"status", "Status", {"status", "invoice_status", "state"}, T::Text},
            {"taxRate", "Tax Rate (%)", {"tax_rate", "tax rate", "tax %"}, T::Integer},
            {"taxable", "Taxable", {"taxable", "is_taxable"}, T::Bool},
            {"discountAmount", "Discount ($)", {"discount", "discount_amount"}, T::Money},
            {"poNumber", "PO Number", {"po_number", "po #", "purchase order"}, T::Text},
            {"notes", "Notes", {"notes", "memo", "comments", "description"}, T::Text}
        };

    case ImportEntity::Payment:
        return {
            {"invoiceNumber", "Invoice # (matched to existing invoices)",
             {"invoice", "invoice #", "invoice no", "invoice number", "invoice_number", "invoice num",
              "inv #", "inv no", "inv number", "applied to invoice"}, T::Text},
            {"invoiceId", "Invoice ID (only if it's this app's own ID)", {"invoice_id", "inv_id"}, T::Integer},
            {"paymentDate", "Payment Date", {"payment_date", "date", "paid_on"}, T::Date},
            {"amount", "Amount", {"amount", "paid_amount", "payment_amount"}, T::Money},
            {"method", "Payment Method", {"method", "payment_method", "type", "pay_mode"}, T::Text},
            {"notes", "Notes / Reference", {"notes", "memo", "reference", "ref_num"}, T::Text}
        };

    case ImportEntity::Company:
        return {
            {"name", "Company Name", {"company_name", "business_name", "company"}, T::Text},
            {"address", "Street Address", {"address", "company_address", "street"}, T::Text},
            {"city", "City", {"city", "company_city"}, T::Text},
            {"state", "State", {"state", "company_state"}, T::Text},
            {"zip", "Zip Code", {"zip", "postal", "company_zip"}, T::Text},
            {"phone", "Phone Number", {"phone", "company_phone"}, T::Phone},
            {"email", "Email", {"email", "company_email"}, T::Email},
            {"taxIdEin", "Tax ID / EIN", {"taxid_ein", "ein", "tax_id"}, T::Text}
        };

    case ImportEntity::Item:
        return {
            {"name", "Item / Service Name",
             {"name", "item", "item name", "product", "product name", "service", "service name",
              "product/service", "product/service name", "item name/number", "product key", "sku"},
             T::Text},
            {"description", "Description",
             {"description", "sales description", "item description", "notes", "details"}, T::Text},
            {"price", "Price / Rate",
             {"price", "sales price", "unit price", "rate", "cost", "sales price/rate"}, T::Money},
            {"taxable", "Taxable", {"taxable", "is taxable", "tax", "sales tax"}, T::Bool},
            {"active", "Active", {"active", "is active", "active status", "status"}, T::Text}
        };

    case ImportEntity::InvoiceHistory:
        return {
            {"invoiceNumber", "Invoice # (blank = auto)",
             {"invoice", "invoice #", "invoice no", "invoice number", "invoice_number", "num",
              "number", "doc number", "transaction #", "ref no"}, T::Text},
            {"issueDate", "Invoice Date",
             {"date", "invoice date", "issue date", "issue_date", "transaction date", "txn date",
              "created"}, T::Date},
            {"dueDate", "Due Date", {"due date", "due_date", "due", "payment due"}, T::Date},
            {"customer", "Customer / Client",
             {"customer", "customer name", "client", "client name", "name", "bill to",
              "customer:job", "full name"}, T::Text},
            {"customerEmail", "Customer Email (optional)",
             {"customer email", "client email", "email", "email address"}, T::Email},
            {"item", "Item / Service",
             {"item", "product/service", "product", "service", "item name",
              "product/service full name", "item name/number", "product key", "sku"}, T::Text},
            {"description", "Line Description",
             {"description", "memo/description", "memo", "line description", "item description",
              "notes", "details"}, T::Text},
            {"quantity", "Quantity", {"qty", "quantity", "hours", "units"}, T::Money},
            {"rate", "Unit Price / Rate",
             {"rate", "unit price", "price", "sales price", "unit cost", "cost", "price each"},
             T::Money},
            {"amount", "Line Amount",
             {"amount", "line amount", "line total", "total", "extended amount"}, T::Money},
            {"tax", "Tax (name or %)",
             {"tax", "tax name", "tax rate", "tax %", "sales tax rate", "tax code"}, T::Text},
            {"status", "Status", {"status", "invoice status", "payment status"}, T::Text}
        };

    default:
        return {};
    }
}

ImportEntity detectEntity(const QStringList& headers, const QVector<QStringList>& sampleRows) {
    const QVector<ImportEntity> entities = {
        ImportEntity::Client,
        ImportEntity::Invoice,
        ImportEntity::Payment,
        ImportEntity::Company
    };

    ColumnMatcher matcher;
    ImportEntity bestEntity = ImportEntity::Client;
    double highestAvgScore = -1.0;

    for (ImportEntity entity : entities) {
        auto fields = fieldsForEntity(entity);
        auto matches = matcher.match(headers, sampleRows, fields);

        double totalScore = 0.0;
        int matchedCount = 0;
        for (const auto& m : std::as_const(matches)) {
            if (m.column >= 0) {
                totalScore += m.score;
                matchedCount++;
            }
        }

        double scoreAvg = fields.isEmpty() ? 0.0 : (totalScore / fields.size());
        if (scoreAvg > highestAvgScore) {
            highestAvgScore = scoreAvg;
            bestEntity = entity;
        }
    }

    return bestEntity;
}

bool mapRow(ImportEntity entity,
            const QStringList& row,
            const QHash<QString, int>& columns,
            MappedRecord& out,
            QString& error)
{
    out.entity = entity;

    switch (entity) {
    case ImportEntity::Client: {
        Client c;
        if (!ClientImport::mapRow(row, columns, c, error))
            return false;
        out.data = QVariant::fromValue(c);
        return true;
    }

    case ImportEntity::Invoice: {
        Invoice inv;
        inv.invoiceNumber = cell(row, columns, "invoiceNumber");
        inv.clientId = cell(row, columns, "clientId").toInt();
        inv.issueDate = dateOrRaw(cell(row, columns, "issueDate"));
        inv.dueDate = dateOrRaw(cell(row, columns, "dueDate"));
        inv.status = cell(row, columns, "status");
        if (inv.status.isEmpty()) inv.status = QStringLiteral("Draft");
        inv.taxRate = cell(row, columns, "taxRate").toInt();
        inv.taxable = isTruthy(cell(row, columns, "taxable"));
        inv.discountAmount = parseCents(cell(row, columns, "discountAmount"));
        inv.poNumber = cell(row, columns, "poNumber");
        inv.notes = cell(row, columns, "notes");

        if (inv.invoiceNumber.isEmpty()) {
            error = QStringLiteral("Invoice row is missing an invoice number");
            return false;
        }
        out.data = QVariant::fromValue(inv);
        return true;
    }

    case ImportEntity::Payment: {
        PaymentImportRecord rec;
        rec.invoiceNumber = cell(row, columns, "invoiceNumber");
        rec.sourceInvoiceId = cell(row, columns, "invoiceId");

        Payment& p = rec.payment;
        p.invoiceId = 0;    // chosen later by resolvePayment()
        p.method = cell(row, columns, "method");
        p.notes = cell(row, columns, "notes");

        const QString dateText = cell(row, columns, "paymentDate");
        p.paymentDate = normalizeDate(dateText);
        if (p.paymentDate.isEmpty()) {
            error = dateText.isEmpty() ? QStringLiteral("Payment has no date")
                                       : QStringLiteral("Unrecognized payment date '%1'").arg(dateText);
            return false;
        }

        const QString amountText = cell(row, columns, "amount");
        int amountCents = 0;
        if (!parseMoneyCents(amountText, amountCents) || amountCents <= 0) {
            error = QStringLiteral("Payment amount '%1' must be a number greater than zero").arg(amountText);
            return false;
        }
        p.amountCents = amountCents;

        if (rec.invoiceNumber.isEmpty() && rec.sourceInvoiceId.isEmpty()) {
            error = QStringLiteral("Payment has no invoice number or invoice ID");
            return false;
        }

        out.data = QVariant::fromValue(rec);
        return true;
    }

    case ImportEntity::Company: {
        Company comp;
        comp.exists = true;
        comp.name = cell(row, columns, "name");
        comp.address = cell(row, columns, "address");
        comp.city = cell(row, columns, "city");
        comp.state = cell(row, columns, "state");
        comp.zip = cell(row, columns, "zip");
        comp.phone = cell(row, columns, "phone");
        comp.email = cell(row, columns, "email");
        comp.taxIdEin = cell(row, columns, "taxIdEin");

        if (comp.name.isEmpty()) {
            error = QStringLiteral("Company profile requires a company name");
            return false;
        }
        out.data = QVariant::fromValue(comp);
        return true;
    }

    case ImportEntity::Item: {
        Item it;
        it.name = cell(row, columns, "name");
        if (it.name.isEmpty()) {
            error = QStringLiteral("Item row has no name");
            return false;
        }
        it.description = cell(row, columns, "description");

        const QString priceText = cell(row, columns, "price");
        int priceCents = 0;
        if (!priceText.isEmpty() && !parseMoneyCents(priceText, priceCents)) {
            error = QStringLiteral("Price '%1' is not a number").arg(priceText);
            return false;
        }
        it.priceCents = priceCents;

        it.taxable = isTaxableWord(cell(row, columns, "taxable"));
        const QString active = cell(row, columns, "active");
        it.isActive = active.isEmpty() || !isInactiveWord(active);

        out.data = QVariant::fromValue(it);
        return true;
    }

    case ImportEntity::InvoiceHistory:
        // Needs every row at once (rows are grouped into invoices); use
        // buildInvoiceHistory() instead.
        error = QStringLiteral("Invoice history is imported with buildInvoiceHistory()");
        return false;

    default:
        error = QStringLiteral("Unsupported entity type");
        return false;
    }
}

QString lookupKey(const QString& text)
{
    QString k = text.trimmed();
    k.remove(QLatin1Char('"'));
    return k.simplified().toLower();
}

namespace {

QString compactKey(const QString& text)
{
    QString out;
    for (const QChar c : text)
        if (c.isLetterOrNumber())
            out += c.toLower();
    return out;
}

} // namespace

PaymentMatch resolvePayment(const PaymentImportRecord& in, const PaymentContext& ctx,
                            Payment& out, QString& message)
{
    out = in.payment;
    int invoiceId = 0;
    QString note;

    const QString number = in.invoiceNumber.trimmed();
    if (!number.isEmpty()) {
        QString key = lookupKey(number);
        if (key.startsWith(QLatin1Char('#')))
            key = key.mid(1).trimmed();

        int found = ctx.invoiceIdByNumber.value(key, 0);
        if (found == 0)
            found = ctx.invoiceIdByCompact.value(compactKey(key), 0);

        if (found < 0) {
            message = QStringLiteral("invoice number '%1' matches more than one invoice").arg(number);
            return PaymentMatch::Error;
        }
        if (found == 0) {
            message = QStringLiteral("no invoice numbered '%1' - import the invoices first").arg(number);
            return PaymentMatch::Error;
        }
        invoiceId = found;
    } else {
        const QString idText = in.sourceInvoiceId.trimmed();
        bool ok = false;
        const int id = idText.toInt(&ok);
        if (!ok || !ctx.numberById.contains(id)) {
            message = QStringLiteral("invoice ID '%1' does not exist here. IDs from another system don't carry over - "
                                     "map the invoice number column instead").arg(idText);
            return PaymentMatch::Error;
        }
        invoiceId = id;
        note = QStringLiteral(" (matched by invoice ID - correct only if the file uses this app's IDs)");
    }

    out.invoiceId = invoiceId;

    const QString key = QStringLiteral("%1|%2|%3|%4")
                            .arg(invoiceId).arg(out.paymentDate).arg(out.amountCents).arg(lookupKey(out.method));
    const QString target = QStringLiteral("invoice %1").arg(ctx.numberById.value(invoiceId));
    if (ctx.existingPayments.contains(key)) {
        message = QStringLiteral("same payment already recorded on %1 - skipped").arg(target);
        return PaymentMatch::Duplicate;
    }

    message = QStringLiteral("%1 -> %2%3")
                  .arg(money(out.amountCents), target, note);
    return PaymentMatch::Ok;
}

namespace {

struct HistLine {
    int row{0};
    QString number;
    QString isoDate;
    QString isoDue;
    QString customer;
    QString email;
    QString status;
    int clientId{0};         // 0 = no customer given or not resolved
    QString clientError;     // set when a customer was given but not resolved
    QString problem;         // first thing wrong with this line, if anything
    InvoiceItem item;
};

struct HistGroup {
    QString number;          // explicit number from the file; empty = generate
    QVector<HistLine> lines;
};

// Client id, 0 = not found, -1 = more than one client matches.
int resolveClient(const EntityMapper::HistoryContext& ctx, const QString& customer, const QString& email)
{
    auto lookup = [&ctx](const QString& text) -> int {
        const QString k = EntityMapper::lookupKey(text);
        if (k.isEmpty())
            return 0;
        const QVector<int> ids = ctx.clientIdsByKey.value(k);
        if (ids.isEmpty())
            return 0;
        return ids.size() == 1 ? ids.first() : -1;
    };

    int id = lookup(customer);
    if (id == 0 && customer.contains(QLatin1Char(':')))          // "Customer:Job"
        id = lookup(customer.section(QLatin1Char(':'), 0, 0));
    if (id <= 0) {                                                // email can settle it
        const int byEmail = lookup(email);
        if (byEmail > 0)
            id = byEmail;
    }
    return id;
}

// tax_id, 0 = no tax. ok=false when a value was given that doesn't match
// exactly one rate in Tax Settings.
int resolveTax(const EntityMapper::HistoryContext& ctx, const QString& text, bool& ok)
{
    ok = true;
    const QString k = EntityMapper::lookupKey(text);
    if (k.isEmpty())
        return 0;

    static const QStringList none{QStringLiteral("none"), QStringLiteral("no"), QStringLiteral("non"),
                                  QStringLiteral("n"), QStringLiteral("exempt"),
                                  QStringLiteral("tax exempt"), QStringLiteral("no tax"),
                                  QStringLiteral("-"), QStringLiteral("0"), QStringLiteral("0%"),
                                  QStringLiteral("0.00"), QStringLiteral("false")};
    if (none.contains(k))
        return 0;

    const int byName = ctx.taxIdByKey.value(k, 0);
    if (byName > 0)
        return byName;
    if (byName < 0) {
        ok = false;
        return 0;
    }

    QString pct = k;
    pct.remove(QLatin1Char('%'));
    bool isNumber = false;
    const double d = pct.trimmed().toDouble(&isNumber);
    if (isNumber && qAbs(d - qRound(d)) < 1e-9) {
        const int byPct = ctx.taxIdByKey.value(QStringLiteral("pct:%1").arg(qRound(d)), 0);
        if (byPct > 0)
            return byPct;
    }
    ok = false;
    return 0;
}

QString rowList(const QVector<int>& rows)
{
    QStringList parts;
    for (int i = 0; i < rows.size() && i < 6; ++i)
        parts << QString::number(rows.at(i) + 1);
    QString text = parts.join(QStringLiteral(", "));
    if (rows.size() > 6)
        text += QStringLiteral(", ...");
    return (rows.size() == 1 ? QStringLiteral("row ") : QStringLiteral("rows ")) + text;
}

} // namespace

QVector<HistoryResult> buildInvoiceHistory(const QVector<QStringList>& rows,
                                           const QHash<QString, int>& columns,
                                           const HistoryContext& ctx)
{
    using State = HistoryResult::State;

    QVector<HistoryResult> results;
    QVector<HistGroup> groups;
    QHash<QString, int> groupIndex;

    auto addError = [&results](int row, const QString& message) {
        HistoryResult e;
        e.state = State::Error;
        e.firstRow = row;
        e.message = message;
        results.push_back(e);
    };

    // ---- Pass 1: read every row into a line and file it under an invoice ----
    for (int r = 0; r < rows.size(); ++r) {
        const QStringList& row = rows.at(r);

        const QString itemName = cell(row, columns, "item");
        const QString desc = cell(row, columns, "description");
        const QString qtyText = cell(row, columns, "quantity");
        const QString rateText = cell(row, columns, "rate");
        const QString amountText = cell(row, columns, "amount");
        const QString taxText = cell(row, columns, "tax");

        if (itemName.isEmpty() && desc.isEmpty() && rateText.isEmpty() && amountText.isEmpty()) {
            addError(r, QStringLiteral("Row %1: no item, description or amount - not an invoice line").arg(r + 1));
            continue;
        }

        HistLine ln;
        ln.row = r;
        ln.number = cell(row, columns, "invoiceNumber");
        ln.customer = cell(row, columns, "customer");
        ln.email = cell(row, columns, "customerEmail");
        ln.status = normalizeStatus(cell(row, columns, "status"));

        auto problem = [&ln](const QString& message) {
            if (ln.problem.isEmpty())
                ln.problem = message;
        };

        const QString dateRaw = cell(row, columns, "issueDate");
        if (!dateRaw.isEmpty()) {
            ln.isoDate = normalizeDate(dateRaw);
            if (ln.isoDate.isEmpty())
                problem(QStringLiteral("unrecognized date '%1'").arg(dateRaw));
        }
        const QString dueRaw = cell(row, columns, "dueDate");
        if (!dueRaw.isEmpty()) {
            ln.isoDue = normalizeDate(dueRaw);
            if (ln.isoDue.isEmpty())
                problem(QStringLiteral("unrecognized due date '%1'").arg(dueRaw));
        }

        if (ln.number.isEmpty() && ln.isoDate.isEmpty()) {
            addError(r, QStringLiteral("Row %1: %2").arg(r + 1).arg(
                         ln.problem.isEmpty()
                             ? QStringLiteral("no invoice number and no date, so the line can't be placed on an invoice")
                             : ln.problem));
            continue;
        }

        if (!ln.customer.isEmpty() || !ln.email.isEmpty()) {
            const int id = resolveClient(ctx, ln.customer, ln.email);
            if (id > 0) {
                ln.clientId = id;
            } else {
                const QString who = ln.customer.isEmpty() ? ln.email : ln.customer;
                ln.clientError = (id < 0)
                                     ? QStringLiteral("customer '%1' matches more than one client (duplicate clients?)").arg(who)
                                     : QStringLiteral("customer '%1' was not found - import clients first").arg(who);
            }
        }

        double qty = 1.0;
        if (!qtyText.isEmpty()) {
            if (!parseNumber(qtyText, qty))
                problem(QStringLiteral("quantity '%1' is not a number").arg(qtyText));
            else if (qty == 0.0)
                problem(QStringLiteral("quantity is 0"));
        }

        int rateCents = 0;
        int amountCents = 0;
        bool hasRate = false;
        bool hasAmount = false;
        if (!rateText.isEmpty()) {
            hasRate = parseMoneyCents(rateText, rateCents);
            if (!hasRate)
                problem(QStringLiteral("rate '%1' is not a number").arg(rateText));
        }
        if (!amountText.isEmpty()) {
            hasAmount = parseMoneyCents(amountText, amountCents);
            if (!hasAmount)
                problem(QStringLiteral("amount '%1' is not a number").arg(amountText));
        }

        int unitCents = 0;
        if (hasRate)
            unitCents = rateCents;
        else if (hasAmount && qty != 0.0)
            unitCents = qRound(amountCents / qty);

        if (hasRate && hasAmount && qty != 0.0 && qAbs(qRound(qty * rateCents) - amountCents) > 1)
            problem(QStringLiteral("amount %1 does not equal quantity x rate (%2)")
                        .arg(money(amountCents), money(qRound(qty * rateCents))));

        bool taxOk = true;
        const int taxId = resolveTax(ctx, taxText, taxOk);
        if (!taxOk)
            problem(QStringLiteral("tax '%1' does not match exactly one rate in Tax Settings").arg(taxText));

        ln.item.description = desc.isEmpty() ? itemName : desc;
        ln.item.quantity = qty;
        ln.item.snapshotUnitPriceCents = unitCents;
        ln.item.lineTotalCents = qRound(qty * unitCents);
        ln.item.taxId = taxId;
        ln.item.serviceId = ctx.serviceIdByName.value(lookupKey(itemName), 0);

        QString key;
        if (!ln.number.isEmpty()) {
            key = QStringLiteral("n:") + lookupKey(ln.number);
        } else {
            const QString who = ln.clientId > 0 ? QString::number(ln.clientId)
                                                : QStringLiteral("?") + lookupKey(ln.customer + QLatin1Char('|') + ln.email);
            key = QStringLiteral("d:") + who + QLatin1Char('|') + ln.isoDate;
        }

        int index = groupIndex.value(key, -1);
        if (index < 0) {
            index = groups.size();
            groupIndex.insert(key, index);
            HistGroup g;
            g.number = ln.number;
            groups.push_back(g);
        }
        groups[index].lines.push_back(ln);
    }

    // Numbers written in the file must never be handed out as generated ones.
    QSet<QString> fileNumbers;
    for (const HistGroup& g : std::as_const(groups))
        if (!g.number.isEmpty())
            fileNumbers.insert(lookupKey(g.number));
    QSet<QString> generated;

    // ---- Pass 2: validate each invoice as a whole ----
    for (const HistGroup& g : std::as_const(groups)) {
        HistoryResult res;
        res.firstRow = g.lines.first().row;

        QVector<int> rowNums;
        for (const HistLine& ln : g.lines)
            rowNums.push_back(ln.row);
        const QString where = rowList(rowNums);

        auto fail = [&](const QString& why) {
            res.state = State::Error;
            res.message = QStringLiteral("%1: %2 - invoice skipped").arg(where, why);
            results.push_back(res);
        };

        QString lineProblem;
        QSet<int> clientIds;
        QString clientError;
        QSet<QString> dates;
        QString due;
        QString status;
        QString customerName;
        for (const HistLine& ln : g.lines) {
            if (lineProblem.isEmpty() && !ln.problem.isEmpty())
                lineProblem = g.lines.size() > 1
                                  ? QStringLiteral("row %1: %2").arg(ln.row + 1).arg(ln.problem)
                                  : ln.problem;
            if (ln.clientId > 0)
                clientIds.insert(ln.clientId);
            else if (clientError.isEmpty() && !ln.clientError.isEmpty())
                clientError = ln.clientError;
            if (!ln.isoDate.isEmpty())
                dates.insert(ln.isoDate);
            if (due.isEmpty() && !ln.isoDue.isEmpty())
                due = ln.isoDue;
            if (status.isEmpty() && !ln.status.isEmpty())
                status = ln.status;
            if (customerName.isEmpty() && !ln.customer.isEmpty())
                customerName = ln.customer;
        }

        if (!lineProblem.isEmpty()) { fail(lineProblem); continue; }
        if (clientIds.size() > 1)   { fail(QStringLiteral("lines belong to different customers")); continue; }
        if (clientIds.isEmpty())    { fail(clientError.isEmpty() ? QStringLiteral("no customer on any line") : clientError); continue; }
        if (!clientError.isEmpty()) { fail(clientError); continue; }
        if (dates.isEmpty())        { fail(QStringLiteral("no date on any line")); continue; }
        if (dates.size() > 1) {
            QStringList list = QStringList(dates.begin(), dates.end());
            list.sort();
            fail(QStringLiteral("lines have different dates (%1)").arg(list.join(QStringLiteral(", "))));
            continue;
        }

        const int clientId = *clientIds.begin();
        const QString issue = *dates.begin();

        QString number = g.number;
        const bool wasGenerated = number.isEmpty();
        if (wasGenerated) {
            QString digits = issue;
            digits.remove(QLatin1Char('-'));
            const QString base = QStringLiteral("IMP-%1-%2").arg(digits).arg(clientId);
            number = base;
            int n = 2;
            while (fileNumbers.contains(lookupKey(number)) || generated.contains(lookupKey(number)))
                number = base + QLatin1Char('-') + QString::number(n++);
            generated.insert(lookupKey(number));
        }

        if (ctx.existingInvoiceNumbers.contains(lookupKey(number))) {
            res.state = State::Duplicate;
            res.message = QStringLiteral("%1: invoice %2 already exists - skipped").arg(where, number);
            results.push_back(res);
            continue;
        }

        Invoice inv;
        inv.clientId = clientId;
        inv.invoiceNumber = number;
        inv.issueDate = issue;
        inv.dueDate = due.isEmpty() ? issue : due;
        inv.status = status.isEmpty() ? QStringLiteral("Sent") : status;
        inv.taxRate = 0;
        inv.discountAmount = 0;

        int totalCents = 0;
        bool anyTax = false;
        for (const HistLine& ln : g.lines) {
            res.record.items.push_back(ln.item);
            totalCents += ln.item.lineTotalCents;
            if (ln.item.taxId != 0)
                anyTax = true;
        }
        inv.taxable = anyTax;
        res.record.invoice = inv;

        res.state = State::Ok;
        res.message = QStringLiteral("%1: invoice %2%3, %4%5 line(s), %6 before tax")
                          .arg(where, number, wasGenerated ? QStringLiteral(" (number generated)") : QString(),
                               customerName.isEmpty() ? QString() : customerName + QStringLiteral(", "))
                          .arg(g.lines.size())
                          .arg(money(totalCents));
        results.push_back(res);
    }

    std::stable_sort(results.begin(), results.end(),
                     [](const HistoryResult& a, const HistoryResult& b) { return a.firstRow < b.firstRow; });
    return results;
}

} // namespace EntityMapper