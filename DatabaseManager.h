#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QString>
#include <QSqlDatabase>

class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();

    // Prevent copying to avoid duplicate database connection handles
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    // Prevent moving to enforce strict single-instance identity per database connection
    DatabaseManager(DatabaseManager&&) = delete;
    DatabaseManager& operator=(DatabaseManager&&) = delete;

    // Called during app startup (e.g., in main.cpp)
    bool initDatabase();

    // Explicitly closes connection and unregisters handle from Qt's pool
    void closeDatabase();

    // Thread-safe accessor for active QSqlDatabase connection pool handle
    QSqlDatabase database() const;

private:
    bool createTables();

    QSqlDatabase m_db;
    QString m_connectionName;
};

#endif // DATABASEMANAGER_H
