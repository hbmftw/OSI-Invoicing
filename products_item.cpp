#include "products_item.h"
#include "ui_products_item.h"
#include "DatabaseManager.h"
#include "DAO_Services.h"
#include "additem.h"
#include "pagebar.h"

#include <QPushButton>
#include <QAction>

Products_Item::Products_Item(DatabaseManager& dbManager, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Products_Item)
    , m_dbManager(dbManager)
{
    ui->setupUi(this);

    // Paging bar under the table: 25 / 50 / 100 rows per page
    m_pageBar = new PageBar([this]() { reload(); }, this);
    ui->verticalLayout->addWidget(m_pageBar);

    ui->itemsTableWidget->setColumnCount(5);
    ui->itemsTableWidget->setHorizontalHeaderLabels({"Name", "Description", "Price", "Taxable", ""});

    ui->ItemSearch_Field->setPlaceholderText("Search Items or Services...");
    ui->ItemSearch_Field->setClearButtonEnabled(true);

    QAction *searchAction = new QAction(this);
    searchAction->setIcon(QIcon(":/new/prefix1/search.png"));
    ui->ItemSearch_Field->addAction(searchAction, QLineEdit::LeadingPosition);

    refresh();
}

Products_Item::~Products_Item()
{
    delete ui;
}

void Products_Item::refresh() {
    reload();
}

void Products_Item::on_ItemSearch_Field_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    // A new search always starts on the first page
    if (m_pageBar)
        m_pageBar->resetPage();
    reload();
}

void Products_Item::reload()
{
    if (!m_pageBar)
        return;

    DAO_Services itemDao(m_dbManager);
    const QString term = ui->ItemSearch_Field->text();

    // Total first: it can move the page back if the current one no longer exists
    m_pageBar->setTotal(itemDao.countServices(term));
    populateItemsTable(itemDao.searchServicesPage(term, m_pageBar->limit(), m_pageBar->offset()));
}

void Products_Item::populateItemsTable(const QVector<Item> &items) {
    ui->itemsTableWidget->clearContents();
    ui->itemsTableWidget->setRowCount(items.size());

    for (int row = 0; row < items.size(); ++row) {
        const Item &item = items[row];

        ui->itemsTableWidget->setItem(row, 0, new QTableWidgetItem(item.name));
        ui->itemsTableWidget->setItem(row, 1, new QTableWidgetItem(item.description));

        QString priceStr = QString("$%1").arg(item.priceCents / 100.0, 0, 'f', 2);
        auto *priceItem = new QTableWidgetItem(priceStr);
        priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ui->itemsTableWidget->setItem(row, 2, priceItem);

        ui->itemsTableWidget->setItem(row, 3, new QTableWidgetItem(item.taxable ? "Yes" : "No"));

        QPushButton *editButton = new QPushButton("Edit", this);
        const int itemId = item.id;
        connect(editButton, &QPushButton::clicked, this, [this, itemId]() {
            openEditItemDialog(itemId);
        });
        ui->itemsTableWidget->setCellWidget(row, 4, editButton);
    }
}

void Products_Item::openEditItemDialog(int itemId) {
    AddItem dialog(m_dbManager, itemId, this);
    if (dialog.exec() == QDialog::Accepted) {
        refresh();
    }
}

void Products_Item::on_NewItem_Button_clicked()
{
    AddItem dialog(m_dbManager, 0, this);
    if (dialog.exec() == QDialog::Accepted) {
        refresh();
    }
}