#ifndef DAO_CLIENT_H
#define DAO_CLIENT_H

#include <QVector>
#include <QString>
#include "client.h"

// Forward declaration
class DatabaseManager;


class DAO_Client
{
public:
    explicit DAO_Client(DatabaseManager& dbManager);

    // Search clients across first_name, last_name, or business_name
    QVector<Client> searchClients(const QString& searchTerm) const;

    // Insert a new client, address
    bool insertClient(const Client& client);

    // Fetch a single client by id (returned Client.id == 0 if not found)
    Client getClientById(int clientId) const;

    // Update an existing client's core fields + billing address
    bool updateClient(const Client& client);

private:
    DatabaseManager& m_dbManager;

};

#endif // DAO_CLIENT_H
