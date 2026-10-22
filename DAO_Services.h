#ifndef DAO_SERVICES_H
#define DAO_SERVICES_H

#include <QVector>
#include <QString>
#include "item.h"

class DatabaseManager;

class DAO_Services
{
public:
    explicit DAO_Services(DatabaseManager& dbManager);

    // Search catalog items across service_name or service_description in the 'services' table
    QVector<Item> searchServices(const QString& searchTerm) const;

    // Insert a new service/product into the 'services' table
    bool insertService(const Item& service);

    // Fetch single service by service_id
    Item getServiceById(int serviceId) const;

    // Update an existing service record
    bool updateService(const Item& service);

    // Soft-delete/deactivate a service (is_active = 0)
    bool deactivateService(int serviceId);

private:
    DatabaseManager& m_dbManager;
};

#endif // DAO_SERVICES_H