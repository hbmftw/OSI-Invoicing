#ifndef DAO_TAX_H
#define DAO_TAX_H

#include <QVector>
#include <QString>

// Forward declaration
class DatabaseManager;

struct TaxRate {
    int id{0};
    QString name;
    int ratePercent{0};    // Whole percentage, e.g. 7 for 7% (tax_table.tax_rate)
    bool active{true};
};

class DAO_Tax
{
public:
    explicit DAO_Tax(DatabaseManager& dbManager);

    // Returns only rows where tax_is_active = 1, ordered by tax_name.
    // Used to populate tax-type selectors (e.g. the invoice items TAX column).
    QVector<TaxRate> getActiveTaxRates() const;

    // Returns every row regardless of active status, ordered by tax_name.
    // Used by the Tax Settings screen, which needs to show and let you
    // reactivate inactive rates too.
    QVector<TaxRate> getAllTaxRates() const;

    // Insert a new tax rate. Returns the new tax_id, or 0 on failure.
    int insertTaxRate(const QString& name, int ratePercent, bool active);

    // Update an existing tax rate's name, rate, and active status.
    // Rates are never deleted (existing invoice_items may reference one
    // via tax_id) -- deactivate instead of removing.
    bool updateTaxRate(const TaxRate& rate);

private:
    DatabaseManager& m_dbManager;

};

#endif // DAO_TAX_H
