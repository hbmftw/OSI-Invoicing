#ifndef CONTACTS_CUSTOMER_H
#define CONTACTS_CUSTOMER_H

#include <QWidget>
#include <QVector>
#include "client.h"

// Forward Declaration
class DatabaseManager;

namespace Ui {
class Contacts_Customer;
}

class Contacts_Customer : public QWidget
{
    Q_OBJECT

public:
    // Accept DatabaseManager reference in constructor
    explicit Contacts_Customer(DatabaseManager& dbManager, QWidget *parent = nullptr);
    ~Contacts_Customer();

public slots:
    void refresh(); // re-query the DB and repopulate the table/model

private slots:
    void on_NewCustomer_Button_clicked();

    void on_CsvImport_Button_clicked();

    void on_CustomerSearch_Field_returnPressed();

    void on_CustomerSearch_Field_textChanged(const QString &arg1);

private:
    // Helper to render search results in the clients table
    void populateCustomersTable(const QVector<Client> &clients);

    void openCustomerCard(int clientID);

    Ui::Contacts_Customer *ui;
    DatabaseManager& m_dbManager; // Store reference to manager

};

#endif // CONTACTS_CUSTOMER_H
