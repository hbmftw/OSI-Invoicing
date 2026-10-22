#ifndef SALESBYITEM_H
#define SALESBYITEM_H

#include <QWidget>

class DatabaseManager;

namespace Ui {
class SalesByItem;
}

class SalesByItem : public QWidget
{
    Q_OBJECT

public:
    explicit SalesByItem(DatabaseManager& dbManager, QWidget *parent = nullptr);
    ~SalesByItem();

private slots:
    void on_RunReport_PushButton_clicked();

private:
    void runReport();

    Ui::SalesByItem *ui;
    DatabaseManager& m_dbManager;
};

#endif // SALESBYITEM_H