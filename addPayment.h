#ifndef ADDPAYMENT_H
#define ADDPAYMENT_H

#include <QDialog>

// Forward declaration
class DatabaseManager;


namespace Ui {
class AddPayment;
}

// Minimal payment-entry dialog for a single, already-known invoice.
// Currently invoked from MainWindow's Receive Payment button/menu action,
// which resolves an invoice number to an id first (InvoiceDao::getInvoiceIdByNumber)
// -- there's no invoice picker UI here.

class AddPayment : public QDialog
{
    Q_OBJECT

public:
    explicit AddPayment(DatabaseManager& dbManager, int invoiceId, QWidget *parent = nullptr);
    ~AddPayment();

private slots:
    void on_Save_PushButton_clicked();
    void on_Cancel_PushButton_clicked();

private:
    Ui::AddPayment *ui;
    DatabaseManager& m_dbManager;
    int m_invoiceId;
};

#endif // ADDPAYMENT_H
