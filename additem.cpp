#include "additem.h"
#include "ui_additem.h"
#include "DatabaseManager.h"
#include "DAO_Services.h"
#include <QMessageBox>

AddItem::AddItem(DatabaseManager& dbManager, int itemId, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddItem)
    , m_dbManager(dbManager)
    , m_itemId(itemId)
{
    ui->setupUi(this);

    ui->Name_LineEdit->setPlaceholderText("Item or Service Name");
    ui->Description_LineEdit->setPlaceholderText("Description");

    if (m_itemId > 0) {
        setWindowTitle("Edit Item");
        DAO_Services serviceDao(m_dbManager);
        Item existing = serviceDao.getServiceById(m_itemId);

        ui->Name_LineEdit->setText(existing.name);
        ui->Description_LineEdit->setText(existing.description);
        ui->Price_SpinBox->setValue(existing.priceCents / 100.0);
        ui->Taxable_CheckBox->setChecked(existing.taxable);
    } else {
        setWindowTitle("New Item");
    }
}

AddItem::~AddItem()
{
    delete ui;
}

void AddItem::on_Cancel_PushButton_clicked()
{
    reject();
}

void AddItem::on_Save_PushButton_clicked()
{
    Item item;
    item.id = m_itemId;
    item.name = ui->Name_LineEdit->text().trimmed();
    item.description = ui->Description_LineEdit->text().trimmed();
    item.priceCents = qRound(ui->Price_SpinBox->value() * 100.0);
    item.taxable = ui->Taxable_CheckBox->isChecked();
    item.isActive = true;

    if (item.name.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Item name is required.");
        return;
    }

    DAO_Services serviceDao(m_dbManager);
    bool success = (m_itemId > 0) ? serviceDao.updateService(item) : serviceDao.insertService(item);

    if (success) {
        accept();
    } else {
        QMessageBox::critical(this, "Error", "Failed to save item to database.");
    }
}