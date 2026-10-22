#ifndef ARSUMMARY_H
#define ARSUMMARY_H

#include <QWidget>

class DatabaseManager;

namespace Ui {
class ArSummary;
}

class ArSummary : public QWidget
{
    Q_OBJECT

public:
    explicit ArSummary(DatabaseManager& dbManager, QWidget *parent = nullptr);
    ~ArSummary();

private slots:
    void on_RunReport_PushButton_clicked();

private:
    void runReport();

    Ui::ArSummary *ui;
    DatabaseManager& m_dbManager;
};

#endif // ARSUMMARY_H