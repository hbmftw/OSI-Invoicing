#include "DAO_Services.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

DAO_Services::DAO_Services(DatabaseManager &dbManager)
    : m_dbManager(dbManager) {}

QVector<Item> DAO_Services::searchServices(const QString& searchTerm) const {
    QVector<Item> results;

    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Services::searchServices - Database connection is not open!";
        return results;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT service_id, service_name, service_description, service_price, taxable, is_active "
        "FROM services "
        "WHERE (service_name LIKE :term1 OR service_description LIKE :term2) "
        "  AND is_active = 1 "
        "ORDER BY service_name ASC;"
        );

    QString pattern = "%" + searchTerm.trimmed() + "%";
    query.bindValue(":term1", pattern);
    query.bindValue(":term2", pattern);

    if (!query.exec()) {
        qCritical() << "DAO_Services::searchServices - Error executing query:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        Item item;
        item.id = query.value("service_id").toInt();
        item.name = query.value("service_name").toString();
        item.description = query.value("service_description").toString();
        item.priceCents = query.value("service_price").toInt();
        item.taxable = query.value("taxable").toBool();
        item.isActive = query.value("is_active").toBool();
        results.append(item);
    }

    return results;
}

int DAO_Services::countServices(const QString& searchTerm) const {
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Services::countServices - Database connection is not open!";
        return 0;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT COUNT(*) FROM services "
        "WHERE (service_name LIKE :term1 OR service_description LIKE :term2) "
        "  AND is_active = 1;"
        );
    const QString pattern = "%" + searchTerm.trimmed() + "%";
    query.bindValue(":term1", pattern);
    query.bindValue(":term2", pattern);

    if (!query.exec() || !query.next()) {
        qCritical() << "DAO_Services::countServices - Error:" << query.lastError().text();
        return 0;
    }
    return query.value(0).toInt();
}

QVector<Item> DAO_Services::searchServicesPage(const QString& searchTerm, int limit, int offset) const {
    QVector<Item> results;

    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Services::searchServicesPage - Database connection is not open!";
        return results;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT service_id, service_name, service_description, service_price, taxable, is_active "
        "FROM services "
        "WHERE (service_name LIKE :term1 OR service_description LIKE :term2) "
        "  AND is_active = 1 "
        "ORDER BY service_name ASC, service_id ASC "
        "LIMIT :limit OFFSET :offset;"
        );
    const QString pattern = "%" + searchTerm.trimmed() + "%";
    query.bindValue(":term1", pattern);
    query.bindValue(":term2", pattern);
    query.bindValue(":limit", limit);
    query.bindValue(":offset", offset);

    if (!query.exec()) {
        qCritical() << "DAO_Services::searchServicesPage - Error:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        Item item;
        item.id = query.value("service_id").toInt();
        item.name = query.value("service_name").toString();
        item.description = query.value("service_description").toString();
        item.priceCents = query.value("service_price").toInt();
        item.taxable = query.value("taxable").toBool();
        item.isActive = query.value("is_active").toBool();
        results.append(item);
    }
    return results;
}

QVector<Item> DAO_Services::getAllServices() const {
    QVector<Item> results;

    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Services::getAllServices - Database connection is not open!";
        return results;
    }

    QSqlQuery query(db);
    if (!query.exec("SELECT service_id, service_name, service_description, service_price, taxable, is_active "
                    "FROM services ORDER BY service_name ASC;")) {
        qCritical() << "DAO_Services::getAllServices - Error executing query:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        Item item;
        item.id = query.value("service_id").toInt();
        item.name = query.value("service_name").toString();
        item.description = query.value("service_description").toString();
        item.priceCents = query.value("service_price").toInt();
        item.taxable = query.value("taxable").toBool();
        item.isActive = query.value("is_active").toBool();
        results.append(item);
    }

    return results;
}

bool DAO_Services::insertService(const Item& service) {
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO services (service_name, service_description, service_price, taxable, is_active) "
        "VALUES (:name, :desc, :price, :taxable, :is_active);"
        );
    query.bindValue(":name", service.name);
    query.bindValue(":desc", service.description);
    query.bindValue(":price", service.priceCents);
    query.bindValue(":taxable", service.taxable);
    query.bindValue(":is_active", service.isActive ? 1 : 0);

    if (!query.exec()) {
        qCritical() << "DAO_Services::insertService - Error inserting service:" << query.lastError().text();
        return false;
    }
    return true;
}

Item DAO_Services::getServiceById(int serviceId) const {
    Item item;
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) return item;

    QSqlQuery query(db);
    query.prepare(
        "SELECT service_id, service_name, service_description, service_price, taxable, is_active "
        "FROM services WHERE service_id = :id;"
        );
    query.bindValue(":id", serviceId);

    if (query.exec() && query.next()) {
        item.id = query.value("service_id").toInt();
        item.name = query.value("service_name").toString();
        item.description = query.value("service_description").toString();
        item.priceCents = query.value("service_price").toInt();
        item.taxable = query.value("taxable").toBool();
        item.isActive = query.value("is_active").toBool();
    }
    return item;
}

bool DAO_Services::updateService(const Item& service) {
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare(
        "UPDATE services SET service_name = :name, service_description = :desc, "
        "service_price = :price, taxable = :taxable, is_active = :is_active "
        "WHERE service_id = :id;"
        );
    query.bindValue(":name", service.name);
    query.bindValue(":desc", service.description);
    query.bindValue(":price", service.priceCents);
    query.bindValue(":taxable", service.taxable);
    query.bindValue(":is_active", service.isActive ? 1 : 0);
    query.bindValue(":id", service.id);

    if (!query.exec()) {
        qCritical() << "DAO_Services::updateService - Error updating service:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DAO_Services::deactivateService(int serviceId) {
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("UPDATE services SET is_active = 0 WHERE service_id = :id;");
    query.bindValue(":id", serviceId);

    if (!query.exec()) {
        qCritical() << "DAO_Services::deactivateService - Error deactivating service:" << query.lastError().text();
        return false;
    }
    return true;
}