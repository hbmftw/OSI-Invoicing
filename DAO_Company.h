#ifndef DAO_COMPANY_H
#define DAO_COMPANY_H

#include <QString>

// Forward declaration
class DatabaseManager;

struct Company {
    bool exists{false}; // false until a row has actually been loaded/saved

    QString name;
    QString address;
    QString city;
    QString state;
    QString zip;
    QString phone;
    QString email;
    bool accrual{false};   // true = accrual accounting, false = cash
    int lateFeePercent{0};
    QString taxIdEin;
    // NOTE: logo (BLOB) intentionally left out of this pass -- no logo
    // upload UI yet. DAO_Company::saveCompany() never touches the logo
    // column, so any existing logo already in the table is preserved.
};

// DAO for the single-row company_profile table (company_id is always 1).
class DAO_Company
{
public:
    explicit DAO_Company(DatabaseManager& dbManager);

    // Returns a default-constructed Company (exists == false) if no row exists yet.
    Company getCompany() const;

    // Upsert: inserts the company_id = 1 row if missing, otherwise updates it.
    bool saveCompany(const Company& company);

private:
    DatabaseManager& m_dbManager;
};

#endif // DAO_COMPANY_H