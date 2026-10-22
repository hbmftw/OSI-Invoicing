#ifndef SALESBYCUSTOMER_H
#define SALESBYCUSTOMER_H

#include <QWidget>

class DatabaseManager;

namespace Ui {
class SalesByCustomer;
}

class SalesByCustomer : public QWidget
{
    Q_OBJECT

public:
    explicit SalesByCustomer(DatabaseManager& dbManager, QWidget *parent = nullptr);
    ~SalesByCustomer();

private slots:
    void on_RunReport_PushButton_clicked();

private:
    void runReport();

    Ui::SalesByCustomer *ui;
    DatabaseManager& m_dbManager;
};

#endif // SALESBYCUSTOMER_H