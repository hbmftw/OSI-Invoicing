#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMdiSubWindow>

//Forward Declaration
class DatabaseManager;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(DatabaseManager& m_dbManager, QWidget *parent = nullptr);
    ~MainWindow();

private slots:


    void on_addCustomerButton_clicked();

    void on_addInvoiceButton_clicked();

    void on_addPaymentsButton_clicked();

    void on_addProductsButton_clicked();

    void on_Invoices_Button_clicked();

    void on_Customers_Button_clicked();

    void onSubWindowActivated(QMdiSubWindow *sub);

private:

    // Shared by both the Receive Payment button and the Received Payments
    // menu action: asks for an invoice number, resolves it, and opens
    // RecordPaymentDialog. No invoice picker UI yet -- typed exact match only.
    // This is in the process of being resolved
    void openReceivePaymentFlow();

    template <typename T>
    void openOrActivateTab();

    Ui::MainWindow *ui;
    DatabaseManager& m_dbManager; // Reference to main.cpp's instance
};

#endif // MAINWINDOW_H
