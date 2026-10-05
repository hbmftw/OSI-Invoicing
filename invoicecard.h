#ifndef INVOICECARD_H
#define INVOICECARD_H

#include <QWidget>
#include "invoice.h"

class DatabaseManager;

namespace Ui {
class InvoiceCard;
}

class InvoiceCard : public QWidget
{
    Q_OBJECT

public:
    explicit InvoiceCard(DatabaseManager& dbManager, int invoiceId, QWidget *parent = nullptr);
    ~InvoiceCard();

signals:
    void invoiceUpdated(int invoiceId);

private slots:
    void on_Edit_PushButton_clicked();
    void on_Save_PushButton_clicked();
    void on_Cancel_PushButton_clicked();
    void on_Close_PushButton_clicked();

private:
    void loadInvoice();
    void populateFields(const Invoice &invoice);
    void setEditingEnabled(bool enabled);

    Ui::InvoiceCard *ui;
    DatabaseManager& m_dbManager;
    int m_invoiceId;
    Invoice m_originalInvoice; // used to restore fields on Cancel
};

#endif // INVOICECARD_H
