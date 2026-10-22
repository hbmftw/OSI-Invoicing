#ifndef PRODUCTS_ITEM_H
#define PRODUCTS_ITEM_H

#include <QWidget>
#include <QVector>
#include "item.h"

class DatabaseManager;

namespace Ui {
class Products_Item;
}

class Products_Item : public QWidget
{
    Q_OBJECT

public:
    explicit Products_Item(DatabaseManager& dbManager, QWidget *parent = nullptr);
    ~Products_Item();

public slots:
    void refresh();

private slots:
    void on_NewItem_Button_clicked();
    void on_ItemSearch_Field_textChanged(const QString &arg1);

private:
    void populateItemsTable(const QVector<Item> &items);
    void openEditItemDialog(int itemId);

    Ui::Products_Item *ui;
    DatabaseManager& m_dbManager;
};

#endif // PRODUCTS_ITEM_H