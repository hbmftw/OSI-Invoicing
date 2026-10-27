#ifndef TAXSETTINGS_H
#define TAXSETTINGS_H

#include <QWidget>

// Forward declaration
class DatabaseManager;

namespace Ui {
class TaxSettings;
}

class TaxSettings : public QWidget
{
    Q_OBJECT

public:
    explicit TaxSettings(DatabaseManager& dbManager, QWidget *parent = nullptr);
    ~TaxSettings();

private slots:
    void on_AddRate_PushButton_clicked();

private:
    void refreshTable();

    Ui::TaxSettings *ui;
    DatabaseManager& m_dbManager;
};
#endif // TAXSETTINGS_H
