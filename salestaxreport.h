#ifndef SALESTAXREPORT_H
#define SALESTAXREPORT_H

#include <QWidget>

// Forward declaration
class DatabaseManager;

namespace Ui {
class SalesTaxReport;
}

class SalesTaxReport : public QWidget
{
    Q_OBJECT

public:
    explicit SalesTaxReport(DatabaseManager& dbManager, QWidget *parent = nullptr);
    ~SalesTaxReport();

private slots:
    void on_RunReport_PushButton_clicked();

private:
    void runReport();

    Ui::SalesTaxReport *ui;
    DatabaseManager& m_dbManager;
};

#endif // SALESTAXREPORT_H
