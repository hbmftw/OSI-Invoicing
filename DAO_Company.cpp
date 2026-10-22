#include "DAO_Company.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

DAO_Company::DAO_Company(DatabaseManager &dbManager)
    : m_dbManager(dbManager) {}

Company DAO_Company::getCompany() const {
    Company company; // exists defaults to false -> acts as "not found" sentinel

    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Company::getCompany - Database connection is not open!";
        return company;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT company_name, company_address, company_city, company_state, company_zip, "
        "       company_phone, company_email, accrual, late_fees, taxid_ein "
        "FROM company_profile "
        "WHERE company_id = 1;"
        );

    if (!query.exec()) {
        qCritical() << "Error fetching company profile:" << query.lastError().text();
        return company;
    }

    if (query.next()) {
        company.exists = true;
        company.name = query.value("company_name").toString();
        company.address = query.value("company_address").toString();
        company.city = query.value("company_city").toString();
        company.state = query.value("company_state").toString();
        company.zip = query.value("company_zip").toString();
        company.phone = query.value("company_phone").toString();
        company.email = query.value("company_email").toString();
        company.accrual = query.value("accrual").toBool();
        company.lateFeePercent = query.value("late_fees").toInt();
        company.taxIdEin = query.value("taxid_ein").toString();
    }

    return company;
}

bool DAO_Company::saveCompany(const Company &company) {
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Company::saveCompany - Database connection is not open!";
        return false;
    }

    // Cheap existence check to decide insert vs. update; company_profile
    // only ever has the single company_id = 1 row.
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT company_id FROM company_profile WHERE company_id = 1;");
    if (!checkQuery.exec()) {
        qCritical() << "Error checking existing company profile:" << checkQuery.lastError().text();
        return false;
    }
    const bool rowExists = checkQuery.next();

    QSqlQuery query(db);
    if (rowExists) {
        query.prepare(
            "UPDATE company_profile SET "
            "company_name = :company_name, company_address = :company_address, "
            "company_city = :company_city, company_state = :company_state, company_zip = :company_zip, "
            "company_phone = :company_phone, company_email = :company_email, "
            "accrual = :accrual, late_fees = :late_fees, taxid_ein = :taxid_ein "
            "WHERE company_id = 1;"
            );
    } else {
        query.prepare(
            "INSERT INTO company_profile "
            "(company_id, company_name, company_address, company_city, company_state, company_zip, "
            " company_phone, company_email, accrual, late_fees, taxid_ein) "
            "VALUES (1, :company_name, :company_address, :company_city, :company_state, :company_zip, "
            "        :company_phone, :company_email, :accrual, :late_fees, :taxid_ein);"
            );
    }

    query.bindValue(":company_name", company.name);
    query.bindValue(":company_address", company.address);
    query.bindValue(":company_city", company.city);
    query.bindValue(":company_state", company.state);
    query.bindValue(":company_zip", company.zip);
    query.bindValue(":company_phone", company.phone);
    query.bindValue(":company_email", company.email);
    query.bindValue(":accrual", company.accrual);
    query.bindValue(":late_fees", company.lateFeePercent);
    query.bindValue(":taxid_ein", company.taxIdEin);

    if (!query.exec()) {
        qCritical() << "Error saving company profile:" << query.lastError().text();
        return false;
    }

    return true;
}