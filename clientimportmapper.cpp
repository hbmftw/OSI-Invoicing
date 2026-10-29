#include "clientimportmapper.h"
#include "client.h"
#include <QRegularExpression>

namespace {

QString cell(const QStringList& row, const QHash<QString, int>& columns, const QString& key)
{
    const int col = columns.value(key, -1);
    return (col >= 0 && col < row.size()) ? row.at(col).trimmed() : QString();
}

bool isTruthy(const QString& value)
{
    static const QStringList yes{QStringLiteral("1"), QStringLiteral("y"),
                                 QStringLiteral("yes"), QStringLiteral("true")};
    return yes.contains(value.trimmed().toLower());
}

} // namespace

namespace ClientImport {

QVector<TargetField> fields()
{
    using T = TargetField::Type;
    return {
             {"displayName", "Name (Full / Combined)",
              {"name", "customer", "customer name", "client name", "display name", "full name"}, T::Text},
             {"firstName", "First Name",
              {"first name", "given name", "fname", "first", "contact first name"}, T::Text},
             {"lastName", "Last Name",
              {"last name", "surname", "lname", "last", "contact last name"}, T::Text},
             {"businessName", "Business Name / Company",
              {"company", "company name", "business name", "organization"}, T::Text},
             {"email", "Email",
              {"email", "e-mail", "email address", "contact email"}, T::Email},
             {"phoneNumber", "Phone",
              {"phone", "phone number", "contact phone", "client phone", "telephone", "mobile"},
              T::Phone},
             {"address1", "Address Line 1",
              {"street", "address", "address1", "address line 1", "billto line1", "street address"},
              T::Text},
             {"address2", "Address Line 2",
              {"apt/suite", "address2", "address line 2", "billto line2", "suite"}, T::Text},
             {"city", "City", {"billto city", "town", "city"}, T::Text},
             {"state", "State", {"state/province", "billto state", "province", "region", "state"}, T::Text},
             {"postalCode", "Postal Code",
              {"zip", "zip code", "billto zip", "postcode", "postal"}, T::Text},
             {"country", "Country", {"country name", "country"}, T::Text},
             {"taxExempt", "Tax Exempt",
              {"is tax exempt", "tax exempt", "taxexempt", "exempt"}, T::Bool},
             {"shippingAddress1", "Shipping Street",
              {"shipping street", "shipping address", "shipping address1",
               "shipping address line 1"}, T::Text},
             };
}

bool mapRow(const QStringList& row,
            const QHash<QString, int>& columns,
            Client& out,
            QString& error)
{
    Client c;

    c.firstName = cell(row, columns, "firstName");
    c.lastName = cell(row, columns, "lastName");
    c.businessName = cell(row, columns, "businessName");

    // Clean leading/trailing double-quotes that CSV exports wrap around strings
    QString display = cell(row, columns, "displayName");
    if (display.startsWith(QLatin1Char('"')) && display.endsWith(QLatin1Char('"')) && display.length() > 1) {
        display = display.mid(1, display.length() - 2).trimmed();
    }

    // 1. If First & Last name are empty, split combined Name
    if (c.firstName.isEmpty() && c.lastName.isEmpty() && !display.isEmpty()) {
        // Strip out embedded nickname quotes like "Barbie" -> Barbie
        QString cleanName = display;
        cleanName = cleanName.remove(QLatin1Char('"')).remove(QLatin1Char('\'')).trimmed();

        const QStringList nameParts = cleanName.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (nameParts.size() == 1) {
            c.firstName = nameParts.first();
            c.lastName = QStringLiteral(" "); // Space fallback for single-word names
        } else if (nameParts.size() >= 2) {
            c.firstName = nameParts.first();
            c.lastName = nameParts.mid(1).join(QLatin1Char(' '));
        }
    }

    // 2. If business name was provided but First/Last name are empty
    if (c.firstName.isEmpty() && c.lastName.isEmpty() && !c.businessName.isEmpty()) {
        c.firstName = c.businessName;
        c.lastName = QStringLiteral("N/A");
    }

    // 3. Other fields
    c.email = cell(row, columns, "email");
    c.phoneNumber = cell(row, columns, "phoneNumber");
    c.address1 = cell(row, columns, "address1");
    if (c.address1.isEmpty())
        c.address1 = cell(row, columns, "shippingAddress1");
    c.address2 = cell(row, columns, "address2");
    c.city = cell(row, columns, "city");
    c.state = cell(row, columns, "state");
    c.postalCode = cell(row, columns, "postalCode");
    c.country = cell(row, columns, "country");
    c.taxExempt = isTruthy(cell(row, columns, "taxExempt"));

    // 4. Validate that a name exists
    if (c.businessName.isEmpty() && c.firstName.trimmed().isEmpty() && c.lastName.trimmed().isEmpty() && display.trimmed().isEmpty()) {
        error = QStringLiteral("Row has no name");
        return false;
    }

    // 5. HARDENED NOT NULL GUARANTEE: Never pass empty string to SQLite for first_name or last_name
    if (c.firstName.trimmed().isEmpty()) {
        c.firstName = !display.isEmpty() ? display : QStringLiteral(" ");
    }
    if (c.lastName.trimmed().isEmpty()) {
        c.lastName = QStringLiteral(" ");
    }

    out = c;
    return true;
}

} // namespace ClientImport