#ifndef PAYMENT_WINDOW_H
#define PAYMENT_WINDOW_H

#include <QWidget>
#include <QVector>
#include "DAO_Payment.h"

class PageBar;

namespace Ui {
class Payment_Window;
}

class Payment_Window : public QWidget
{
    Q_OBJECT

public:
    explicit Payment_Window(DatabaseManager& dbManager, QWidget *parent = nullptr);
    ~Payment_Window();

public slots:
    void refresh();

private slots:
    void on_NewPayment_Button_clicked();
    void on_PaymentSearch_Field_textChanged(const QString &arg1); //[cite: 4]
    void on_MethodFilter_ComboBox_currentIndexChanged(int index);

private:
    void populatePaymentsTable(const QVector<PaymentListItem> &payments);
    void refreshPaymentsTable();
    void deletePayment(int paymentId);

    Ui::Payment_Window *ui;
    DatabaseManager& m_dbManager;
    PageBar* m_pageBar{nullptr};  // "Rows per page" + Prev/Next bar under the table
};

#endif // PAYMENT_WINDOW_H
