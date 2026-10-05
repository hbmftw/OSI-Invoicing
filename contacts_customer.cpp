#include "contacts_customer.h"
#include "ui_contacts_customer.h"
#include "DatabaseManager.h"
#include "DAO_Client.h"
#include "addclient.h" //"addclientdialog.h"
#include "customercard.h"

#include <QMessageBox>
#include <QPushButton>

/*
 * This is the page that shows when Customers button is clicked in main window
 */
Contacts_Customer::Contacts_Customer(DatabaseManager& dbManager, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Contacts_Customer)
    , m_dbManager(dbManager) // Bind reference in initializer list
{
    ui->setupUi(this);

    // Set up table headers (also defined in the .ui file; kept here in case columns change at runtime)
    ui->clientsTableWidget->setColumnCount(5);
    ui->clientsTableWidget->setHorizontalHeaderLabels({"Name", "Business", "Email", "Phone", ""});

    // Line Edit for Customer Search Field
    // Place Holder Text
    ui->CustomerSearch_Field->setPlaceholderText("Search Customers, Contact Information...");
    // Add "Clear" button
    ui->CustomerSearch_Field->setClearButtonEnabled(true);
    // Add magnifing glass icon to the left side of text box
    QAction *searchAction = new QAction(this);
    searchAction->setIcon(QIcon(":/new/prefix1/search.png"));// Path to search icon (Resources file)
    ui->CustomerSearch_Field->addAction(searchAction, QLineEdit::LeadingPosition);

    // Populate the table with all clients on first open
    on_CustomerSearch_Field_textChanged(QString());
}

Contacts_Customer::~Contacts_Customer()
{
    delete ui;
}

// Re-run the search with whatever is currently typed in the search box
void Contacts_Customer::refresh() { on_CustomerSearch_Field_textChanged(ui->CustomerSearch_Field->text()); }

void Contacts_Customer::on_CustomerSearch_Field_textChanged(const QString &arg1)
{
    // Create DAO instance passing m_dbManager reference
    DAO_Client clientDao(m_dbManager);

    // Fetch matched results from DAO (returns QVector<Client>)
    QVector<Client> results = clientDao.searchClients(arg1);

    // Render results in table
    populateCustomersTable(results);
}

void Contacts_Customer::populateCustomersTable(const QVector<Client> &clients) {
    ui->clientsTableWidget->clearContents();
    ui->clientsTableWidget->setRowCount(clients.size());

    for (int row = 0; row < clients.size(); ++row) {
        const Client &client = clients[row];

        QString fullName = QString("%1 %2").arg(client.firstName, client.lastName).trimmed();
        QTableWidgetItem *nameItem = new QTableWidgetItem(fullName);
        nameItem->setData(Qt::UserRole, client.id);

        ui->clientsTableWidget->setItem(row, 0, nameItem);
        ui->clientsTableWidget->setItem(row, 1, new QTableWidgetItem(client.businessName));
        ui->clientsTableWidget->setItem(row, 2, new QTableWidgetItem(client.email));
        ui->clientsTableWidget->setItem(row, 3, new QTableWidgetItem(client.phoneNumber));

        QPushButton *editButton = new QPushButton("Edit", this);
        const int clientId = client.id; // captured by value, not by row index
        connect(editButton, &QPushButton::clicked, this, [this, clientId]() {
            openCustomerCard(clientId);
        });
        ui->clientsTableWidget->setCellWidget(row, 4, editButton);
    }
}

void Contacts_Customer::openCustomerCard(int clientId)
{
    CustomerCard *card = new CustomerCard(m_dbManager, clientId, this);

    // Refresh the results table whenever the card saves a change
    connect(card, &CustomerCard::customerUpdated, this, [this](int) {
        on_CustomerSearch_Field_textChanged(ui->CustomerSearch_Field->text());
    });

    // Walk up to the top-level MainWindow's mdiArea instead of showing as a free Window
    //QMdiArea *mdiArea = window()->findChild<QMdiArea*>("mdiArea");
    //if (mdiArea) {
    //    QMdiSubWindow *sub = mdiArea->addSubWindow(card);
    //    sub->setAttribute(Qt::WA_DeleteOnClose);
    //    sub->show();
    //} else {
    //    card->show(); // fallback if not embedded in an MDI-based parent
    //}

    card->show();
    card->raise();
    card->activateWindow();
}

// Add New Customer
void Contacts_Customer::on_NewCustomer_Button_clicked()
{
    // Pass m_dbManager down to child addclientdialog
    AddClient dialog(m_dbManager, this);

    // Show modal dialog
    if (dialog.exec() == QDialog::Accepted) {
        // Refresh the client table after saving
        on_CustomerSearch_Field_textChanged(ui->CustomerSearch_Field->text());
    }
}


// CSV Import
void Contacts_Customer::on_CsvImport_Button_clicked()
{

}


// Customer Search Field
void Contacts_Customer::on_CustomerSearch_Field_returnPressed()
{
    // Should be the same as on_CustomerSearch_Field_textChanged
}
