#ifndef CUSTOMERCARD_H
#define CUSTOMERCARD_H

#include <QWidget>
#include "client.h"

class DatabaseManager;

namespace Ui {
class CustomerCard;
}

class CustomerCard : public QWidget
{
    Q_OBJECT

public:
    explicit CustomerCard(DatabaseManager& dbManager, int clientId, QWidget *parent = nullptr);
    ~CustomerCard();

signals:
    void customerUpdated(int clientId);

private slots:
    void on_Edit_PushButton_clicked();
    void on_Save_PushButton_clicked();
    void on_Cancel_PushButton_clicked();
    void on_Close_PushButton_clicked();

private:
    void loadClient();
    void populateFields(const Client &client);
    void setEditingEnabled(bool enabled);

    Ui::CustomerCard *ui;
    DatabaseManager& m_dbManager;
    int m_clientId;
    Client m_originalClient; // used to restore fields on Cancel
};

#endif // CUSTOMERCARD_H
