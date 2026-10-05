#include "DAO_Tax.h"
#include "DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

DAO_Tax::DAO_Tax(DatabaseManager &dbManager)
    : m_dbManager(dbManager) {}

QVector<TaxRate> DAO_Tax::getActiveTaxRates() const {
    QVector<TaxRate> results;

    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Tax::getActiveTaxRates - Database connection is not open!";
        return results;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT tax_id, tax_name, tax_rate, tax_is_active "
        "FROM tax_table "
        "WHERE tax_is_active = 1 "
        "ORDER BY tax_name ASC;"
        );

    if (!query.exec()) {
        qCritical() << "Error fetching active tax rates:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        TaxRate rate;
        rate.id = query.value("tax_id").toInt();
        rate.name = query.value("tax_name").toString();
        rate.ratePercent = query.value("tax_rate").toInt();
        rate.active = query.value("tax_is_active").toBool();
        results.append(rate);
    }

    return results;
}

QVector<TaxRate> DAO_Tax::getAllTaxRates() const {
    QVector<TaxRate> results;

    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Tax::getAllTaxRates - Database connection is not open!";
        return results;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT tax_id, tax_name, tax_rate, tax_is_active "
        "FROM tax_table "
        "ORDER BY tax_name ASC;"
        );

    if (!query.exec()) {
        qCritical() << "Error fetching all tax rates:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        TaxRate rate;
        rate.id = query.value("tax_id").toInt();
        rate.name = query.value("tax_name").toString();
        rate.ratePercent = query.value("tax_rate").toInt();
        rate.active = query.value("tax_is_active").toBool();
        results.append(rate);
    }

    return results;
}

int DAO_Tax::insertTaxRate(const QString &name, int ratePercent, bool active) {
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Tax::insertTaxRate - Database connection is not open!";
        return 0;
    }

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO tax_table (tax_name, tax_rate, tax_is_active) "
        "VALUES (:tax_name, :tax_rate, :tax_is_active);"
        );
    query.bindValue(":tax_name", name);
    query.bindValue(":tax_rate", ratePercent);
    query.bindValue(":tax_is_active", active);

    if (!query.exec()) {
        qCritical() << "Error inserting tax rate:" << query.lastError().text();
        return 0;
    }

    const QVariant taxIdVariant = query.lastInsertId();
    if (!taxIdVariant.isValid()) {
        qCritical() << "DAO_Tax::insertTaxRate - Failed to retrieve inserted tax_id";
        return 0;
    }

    return taxIdVariant.toInt();
}

bool DAO_Tax::updateTaxRate(const TaxRate &rate) {
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Tax::updateTaxRate - Database connection is not open!";
        return false;
    }

    QSqlQuery query(db);
    query.prepare(
        "UPDATE tax_table SET tax_name = :tax_name, tax_rate = :tax_rate, tax_is_active = :tax_is_active "
        "WHERE tax_id = :tax_id;"
        );
    query.bindValue(":tax_name", rate.name);
    query.bindValue(":tax_rate", rate.ratePercent);
    query.bindValue(":tax_is_active", rate.active);
    query.bindValue(":tax_id", rate.id);

    if (!query.exec()) {
        qCritical() << "Error updating tax rate:" << query.lastError().text();
        return false;
    }

    return true;
}
