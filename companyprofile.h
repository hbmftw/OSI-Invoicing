#ifndef COMPANYPROFILE_H
#define COMPANYPROFILE_H

#include <QWidget>
#include "DAO_Company.h"

namespace Ui {
class CompanyProfile;
}

class CompanyProfile : public QWidget
{
    Q_OBJECT

public:
    explicit CompanyProfile(DatabaseManager& dbManager, QWidget *parent = nullptr);
    ~CompanyProfile();

signals:
    void companyProfileUpdated();

private slots:
    void on_Edit_PushButton_clicked();
    void on_Save_PushButton_clicked();
    void on_Cancel_PushButton_clicked();
    void on_Close_PushButton_clicked();

private:
    void loadCompany();
    void populateFields(const Company &company);
    void setEditingEnabled(bool enabled);

    Ui::CompanyProfile *ui;
    DatabaseManager& m_dbManager;
    Company m_originalCompany; // used to restore fields on Cancel
};

#endif // COMPANYPROFILE_H