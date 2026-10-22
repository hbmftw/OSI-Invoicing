#ifndef REPORTS_WINDOW_H
#define REPORTS_WINDOW_H

#include <QWidget>

class DatabaseManager;
class SalesTaxReport;
class SalesByItem;
class SalesByCustomer;
class ArSummary;

namespace Ui {
class Reports_Window;
}

class Reports_Window : public QWidget
{
    Q_OBJECT

public:
    explicit Reports_Window(DatabaseManager& dbManager, QWidget *parent = nullptr);
    ~Reports_Window();

private slots:
    void on_SalesTax_Button_clicked();
    void on_SalesByItem_Button_clicked();
    void on_SalesByCustomer_Button_clicked();
    void on_ArSummary_Button_clicked();

private:
    Ui::Reports_Window *ui;
    DatabaseManager& m_dbManager;

    SalesTaxReport *m_salesTaxReportWidget{nullptr};
    SalesByItem *m_salesByItemWidget{nullptr};
    SalesByCustomer *m_salesByCustomerWidget{nullptr};
    ArSummary *m_arSummaryWidget{nullptr};
};

#endif // REPORTS_WINDOW_H