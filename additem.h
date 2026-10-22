#ifndef ADDITEM_H
#define ADDITEM_H

#include <QDialog>

class DatabaseManager;

namespace Ui {
class AddItem;
}

class AddItem : public QDialog
{
    Q_OBJECT

public:
    // If itemId > 0, dialog enters "Edit" mode; otherwise "Add" mode
    explicit AddItem(DatabaseManager& dbManager, int itemId = 0, QWidget *parent = nullptr);
    ~AddItem();

private slots:
    void on_Cancel_PushButton_clicked();
    void on_Save_PushButton_clicked();

private:
    Ui::AddItem *ui;
    DatabaseManager& m_dbManager;
    int m_itemId{0};
};

#endif // ADDITEM_H