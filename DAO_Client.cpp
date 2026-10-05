#include "DAO_Client.h"

#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

// Client Data Access Object for searching clients name or business
DAO_Client::DAO_Client(DatabaseManager &dbManager)
    : m_dbManager(dbManager) {}

QVector<Client> DAO_Client::searchClients(const QString& searchTerm) const {
    //qWarning() << ">>>> RUNNING FRESH BUILD - marker 001 - ClientDao <<<<"; // Testing purpose
    QVector<Client> results;

    // Fetch the thread-safe connection from DatabaseManager
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Client::searchClients - Database connection is not open!";
        return results;
    }

    // Prepare parameterized SQL query
    // Utilize idx_clients_names (last_name, first_name, business_name)
    QSqlQuery query(db);
    query.prepare(
        "SELECT client_id, first_name, last_name, business_name, email, phone_number, created_at "
        "FROM clients "
        "WHERE first_name LIKE :term1 "
        "   OR last_name LIKE :term2 "
        "   OR business_name LIKE :term3 "
        "ORDER BY last_name ASC, first_name ASC, business_name ASC;"
        );

    // Format wildcards for substring matching ("Acme" -> "%Acme%)
    QString formattedPattern = "%" + searchTerm.trimmed() + "%";
    query.bindValue(":term1", formattedPattern);
    query.bindValue(":term2", formattedPattern);
    query.bindValue(":term3", formattedPattern);

    // Execute query
    if (!query.exec()) {
        qCritical() << "Error executing client search:" << query.lastError().text();
        return results;
    }

    // Extract fows into Clients structs
    while (query.next()) {
        Client client;
        client.id = query.value("client_id").toInt();
        client.firstName = query.value("first_name").toString();
        client.lastName = query.value("last_name").toString();
        client.businessName = query.value("business_name").toString();
        client.email = query.value("email").toString();
        client.phoneNumber = query.value("phone_number").toString();
        client.createdAt = query.value("created_at").toString();

        results.append(client);
    }

    return results;
}

bool DAO_Client::insertClient(const Client& client) {
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "ClientDao::insertClient - Database connection is not open!";
        return false;
    }

    db.transaction();

    // Insert the core client record
    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO clients (first_name, last_name, business_name, email, phone_number, tax_exempt) "
        "VALUES (:first_name, :last_name, :business_name, :email, :phone_number, :tax_exempt);"
        );
    query.bindValue(":first_name", client.firstName);
    query.bindValue(":last_name", client.lastName);
    query.bindValue(":business_name", client.businessName);
    query.bindValue(":email", client.email);
    query.bindValue(":phone_number", client.phoneNumber);
    query.bindValue(":tax_exempt", client.taxExempt);

    if (!query.exec()) {
        qCritical() << "Error inserting client:" << query.lastError().text();
        db.rollback();
        return false;
    }

    const QVariant clientIdVariant = query.lastInsertId();
    if (!clientIdVariant.isValid()) {
        qCritical() << "ClientDao::insertClient - Failed to retrieve inserted client_id";
        db.rollback();
        return false;
    }
    const int clientId = clientIdVariant.toInt();

    // Only insert an address row if the caller actually supplied address data
    const bool hasAddressData = !client.address1.isEmpty() || !client.city.isEmpty()
                                || !client.state.isEmpty() || !client.postalCode.isEmpty() || !client.country.isEmpty();

    if (hasAddressData) {
        QSqlQuery addressQuery(db);
        addressQuery.prepare(
            "INSERT INTO addresses (client_id, address_type, street_address, address_line2, city, state, postal_code, country) "
            "VALUES (:client_id, :address_type, :street_address, :address_line2, :city, :state, :postal_code, :country);"
            );
        addressQuery.bindValue(":client_id", clientId);
        addressQuery.bindValue(":address_type", "Billing Address");
        addressQuery.bindValue(":street_address", client.address1);
        addressQuery.bindValue(":address_line2", client.address2);
        addressQuery.bindValue(":city", client.city);
        addressQuery.bindValue(":state", client.state);
        addressQuery.bindValue(":postal_code", client.postalCode);
        addressQuery.bindValue(":country", client.country);

        if (!addressQuery.exec()) {
            qCritical() << "Error inserting address:" << addressQuery.lastError().text();
            db.rollback();
            return false;
        }
    }

    if (!db.commit()) {
        qCritical() << "ClientDao::insertClient - Failed to commit transaction:" << db.lastError().text();
        db.rollback();
        return false;
    }

    return true;
}

Client DAO_Client::getClientById(int clientId) const {
    Client client; // id defaults to 0 -> acts as "not found" sentinel

    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Client::getClientById - Database connection is not open!";
        return client;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT c.client_id, c.first_name, c.last_name, c.business_name, c.email, c.phone_number, c.created_at, c.tax_exempt, "
        "       a.street_address, a.address_line2, a.city, a.state, a.postal_code, a.country "
        "FROM clients c "
        "LEFT JOIN addresses a ON a.client_id = c.client_id AND a.address_type = 'Billing Address' "
        "WHERE c.client_id = :id;"
        );
    query.bindValue(":id", clientId);

    if (!query.exec()) {
        qCritical() << "Error fetching client by id:" << query.lastError().text();
        return client;
    }

    if (query.next()) {
        client.id = query.value("client_id").toInt();
        client.firstName = query.value("first_name").toString();
        client.lastName = query.value("last_name").toString();
        client.businessName = query.value("business_name").toString();
        client.email = query.value("email").toString();
        client.phoneNumber = query.value("phone_number").toString();
        client.createdAt = query.value("created_at").toString();
        client.taxExempt = query.value("tax_exempt").toBool();
        client.address1 = query.value("street_address").toString();
        client.address2 = query.value("address_line2").toString();
        client.city = query.value("city").toString();
        client.state = query.value("state").toString();
        client.postalCode = query.value("postal_code").toString();
        client.country = query.value("country").toString();
    }

    return client;
}

bool DAO_Client::updateClient(const Client& client) {
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        qCritical() << "DAO_Client::updateClient - Database connection is not open!";
        return false;
    }

    db.transaction();

    QSqlQuery query(db);
    query.prepare(
        "UPDATE clients SET first_name = :first_name, last_name = :last_name, "
        "business_name = :business_name, email = :email, phone_number = :phone_number, tax_exempt = :tax_exempt "
        "WHERE client_id = :client_id;"
        );
    query.bindValue(":first_name", client.firstName);
    query.bindValue(":last_name", client.lastName);
    query.bindValue(":business_name", client.businessName);
    query.bindValue(":email", client.email);
    query.bindValue(":phone_number", client.phoneNumber);
    query.bindValue(":tax_exempt", client.taxExempt);
    query.bindValue(":client_id", client.id);

    if (!query.exec()) {
        qCritical() << "Error updating client:" << query.lastError().text();
        db.rollback();
        return false;
    }

    // Upsert the billing address row
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT address_id FROM addresses WHERE client_id = :client_id AND address_type = 'Billing Address';");
    checkQuery.bindValue(":client_id", client.id);
    if (!checkQuery.exec()) {
        qCritical() << "Error checking existing address:" << checkQuery.lastError().text();
        db.rollback();
        return false;
    }

    if (checkQuery.next()) {
        QSqlQuery updateAddress(db);
        updateAddress.prepare(
            "UPDATE addresses SET street_address = :street_address, address_line2 = :address_line2, "
            "city = :city, state = :state, postal_code = :postal_code, country = :country "
            "WHERE client_id = :client_id AND address_type = 'Billing Address';"
            );
        updateAddress.bindValue(":street_address", client.address1);
        updateAddress.bindValue(":address_line2", client.address2);
        updateAddress.bindValue(":city", client.city);
        updateAddress.bindValue(":state", client.state);
        updateAddress.bindValue(":postal_code", client.postalCode);
        updateAddress.bindValue(":country", client.country);
        updateAddress.bindValue(":client_id", client.id);

        if (!updateAddress.exec()) {
            qCritical() << "Error updating address:" << updateAddress.lastError().text();
            db.rollback();
            return false;
        }
    } else {
        QSqlQuery insertAddress(db);
        insertAddress.prepare(
            "INSERT INTO addresses (client_id, address_type, street_address, address_line2, city, state, postal_code, country) "
            "VALUES (:client_id, :address_type, :street_address, :address_line2, :city, :state, :postal_code, :country);"
            );
        insertAddress.bindValue(":client_id", client.id);
        insertAddress.bindValue(":address_type", "Billing Address");
        insertAddress.bindValue(":street_address", client.address1);
        insertAddress.bindValue(":address_line2", client.address2);
        insertAddress.bindValue(":city", client.city);
        insertAddress.bindValue(":state", client.state);
        insertAddress.bindValue(":postal_code", client.postalCode);
        insertAddress.bindValue(":country", client.country);

        if (!insertAddress.exec()) {
            qCritical() << "Error inserting address:" << insertAddress.lastError().text();
            db.rollback();
            return false;
        }
    }

    if (!db.commit()) {
        qCritical() << "ClientDao::updateClient - Failed to commit transaction:" << db.lastError().text();
        db.rollback();
        return false;
    }

    return true;
}

