#ifndef ADDCLIENT_H
#define ADDCLIENT_H

#include <QDialog>

// Forward declaration
class DatabaseManager;

namespace Ui {
class AddClient;
}

class AddClient : public QDialog
{
    Q_OBJECT

public:
    explicit AddClient(DatabaseManager& dbManager, QWidget *parent = nullptr);
    ~AddClient();

private slots:
    void on_Cancel_PushButton_clicked();

    void on_Save_PushButton_clicked();

private:
    Ui::AddClient *ui;
    DatabaseManager& m_dbManager; // Store reference to manager
};

#endif // ADDCLIENT_H
