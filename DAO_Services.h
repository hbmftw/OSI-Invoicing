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

    // Paged search for the Products & Items list: same matching as
    // searchServices() (active items only), ordered by name with
    // service_id as a tie-break. countServices() gives the total matches.
    int countServices(const QString& searchTerm) const;
    QVector<Item> searchServicesPage(const QString& searchTerm, int limit, int offset) const;

    // Every service, including deactivated ones, ordered by name. Used by the
    // CSV import to spot duplicates and to link old invoice lines to items.
    QVector<Item> getAllServices() const;

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