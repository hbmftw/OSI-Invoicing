#ifndef INVOICE_WINDOW_H
#define INVOICE_WINDOW_H

#include <QWidget>
#include <QVector>
#include "DAO_Invoice.h" // Name Changed from invoiceDao.h

/*
 * invoice_window.h used to be called contacts_invoices.h
 */

// Forward Declaration
class DatabaseManager;


namespace Ui {
class Invoice_Window;
}

class Invoice_Window : public QWidget
{
    Q_OBJECT

public:
    explicit Invoice_Window(DatabaseManager& dbManager, QWidget *parent = nullptr);
    ~Invoice_Window();

public slots:
    void refresh(); // re-query the DB and repopulates the table/model

private slots:
    void on_NewInvoice_Button_clicked();

    //void on_InvoiceSearch_Field_returnPressed();

    void on_InvoiceSearch_Field_textChanged(const QString &arg1);

    void on_StatusFilter_ComboBox_currentIndexChanged(int index);

private:

    // Helper to render search results in the invoices table
    void populateInvoicesTable(const QVector<InvoiceListItem> &invoices);

    // Re-runs the search with the current search text + status filter combo
    // selection ("All" -> no filter). Called on any change to either.
    void refreshInvoicesTable();

    // InvoiceCard that shows invoice information and allows quick changes
    void openInvoiceCard(int invoiceId);

    // Open ReceivePayment window
    void openReceivePayment(int invoiceId);

    Ui::Invoice_Window *ui;
    DatabaseManager& m_dbManager; // Store reference to manager
};

#endif // INVOICE_WINDOW_H
