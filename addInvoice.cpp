#include "addInvoice.h"
#include "ui_addInvoice.h"
#include "DatabaseManager.h"
#include "DAO_Invoice.h"
#include "DAO_Client.h"
#include "customercard.h"
#include "DAO_Tax.h"
#include "Invoiceitem.h"
#include "DAO_Items.h"
#include "invoicestatusguard.h"

#include <QMessageBox>
#include <QDate>
#include <QCompleter>
#include <QPushButton>
#include <QComboBox>
#include <QStandardItemModel>
#include <QTableWidgetItem>
#include <QSignalBlocker>
#include <utility>

namespace {
// Index in NetTerms_ComboBox -> days until due. Kept in one place so the
// combo box population order and this lookup can't drift apart silently.
const int kNetTermsDays[] = {0, 15, 30, 45, 60};
const int kDefaultNetTermsIndex = 2; // "Net 30"
}

AddInvoice::AddInvoice(DatabaseManager& dbManager, int preselectedClientId, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddInvoice)
    , m_dbManager(dbManager) // Bind reference in initializer list
{
    ui->setupUi(this);

    // Loaded once so every item row's TAX dropdown offers the same options
    // without re-querying the DB per row.
    DAO_Tax taxDao(m_dbManager);
    m_taxRates = taxDao.getActiveTaxRates();

    populateClientCompleter(preselectedClientId);

    // Tax Exempt disables every row's TAX dropdown (their selections no
    // longer matter) and zeroes out tax on the totals; Taxable re-enables
    // them. Defaults to Taxable; refreshClientSummary() flips this to
    // Tax Exempt automatically when the selected client is tax-exempt, but
    // it stays freely changeable by hand after that.
    connect(ui->TaxStatus_ComboBox, &QComboBox::currentIndexChanged, this, [this](int) {
        const bool taxable = isInvoiceTaxable();
        for (int row = 0; row < ui->Items_TableWidget->rowCount(); ++row) {
            if (auto *taxCombo = qobject_cast<QComboBox *>(ui->Items_TableWidget->cellWidget(row, 5))) {
                taxCombo->setEnabled(taxable);
            }
        }
        recalculateInvoiceTotals();
    });
    ui->TaxStatus_ComboBox->setCurrentIndex(0); // Taxable by default

    // Live-refresh the client summary panel whenever the typed client
    // resolves to a real client, whether via picking a completer entry or
    // tabbing away after typing an exact match.
    connect(ui->Client_LineEdit->completer(), QOverload<const QString &>::of(&QCompleter::activated),
            this, [this](const QString &) { onClientSelectionChanged(); });
    connect(ui->Client_LineEdit, &QLineEdit::editingFinished, this, &AddInvoice::onClientSelectionChanged);
    onClientSelectionChanged(); // picks up the preselected client, if any (and its tax-exempt status)

    // Net Terms recomputes Due Date from the current Issue Date. Manual
    // edits to Due Date afterward stick until Net Terms is changed again.
    connect(ui->NetTerms_ComboBox, &QComboBox::currentIndexChanged, this, [this](int index) {
        constexpr int kCount = sizeof(kNetTermsDays) / sizeof(kNetTermsDays[0]);
        if (index >= 0 && index < kCount) {
            ui->DueDate_DateEdit->setDate(ui->IssueDate_DateEdit->date().addDays(kNetTermsDays[index]));
        }
    });

    // TOTAL (per row) and taxTotal_lineEdit/GrandTotal_lineEdit (invoice-wide)
    // are all derived from QUANTITY, AMOUNT, and TAX, so any edit to those
    // needs to ripple through.
    connect(ui->Items_TableWidget, &QTableWidget::cellChanged, this, &AddInvoice::onItemsTableCellChanged);

    // Sensible defaults for a brand-new invoice
    ui->IssueDate_DateEdit->setDate(QDate::currentDate());
    ui->NetTerms_ComboBox->setCurrentIndex(kDefaultNetTermsIndex); // also sets Due Date via the connection above
    populateStatusComboBox();
    ui->InvoiceStatus_ComboBox->setCurrentIndex(1); // "Sent" -- an invoice you save is treated as sent by default; "Draft" is still one click away for the rare case that's wrong

    ui->Client_LineEdit->setPlaceholderText("Type client name...");
    ui->InvoiceNumber_LineEdit->setPlaceholderText("Invoice Number");
    ui->PONumber_LineEdit->setPlaceholderText("PO Number");
    ui->Notes_PlainTextEdit->setPlaceholderText("Notes");
}

AddInvoice::~AddInvoice()
{
    delete ui;
}

void AddInvoice::populateClientCompleter(int preselectedClientId)
{
    m_clientDisplayNameToId.clear();

    // Empty search term matches every client (LIKE '%%')
    DAO_Client clientDao(m_dbManager);
    QVector<Client> clients = clientDao.searchClients(QString());

    QStringList displayNames;
    QString preselectedDisplayName;

    for (const Client &client : std::as_const(clients)) { // Wrap clients in std::as_const() c++17 to force the loop to call const iterators without detaching container
        QString display = QString("%1 %2").arg(client.firstName, client.lastName).trimmed();
        if (!client.businessName.isEmpty()) {
            display += QString(" (%1)").arg(client.businessName);
        }

        // Note: if two clients share an identical display name, the later
        // one wins the id lookup below. Good enough for a typeahead field;
        // revisit if that collision turns out to matter in practice.
        m_clientDisplayNameToId.insert(display, client.id);
        displayNames << display;

        if (preselectedClientId != 0 && client.id == preselectedClientId) {
            preselectedDisplayName = display;
        }
    }

    QCompleter *completer = new QCompleter(displayNames, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains); // matches anywhere in the name, not just the start
    completer->setCompletionMode(QCompleter::PopupCompletion); // live dropdown as you type
    ui->Client_LineEdit->setCompleter(completer);

    if (!preselectedDisplayName.isEmpty()) {
        ui->Client_LineEdit->setText(preselectedDisplayName);
    }
}

void AddInvoice::populateStatusComboBox()
{
    // "Draft" and "Sent" are what every ordinary invoice uses and sit
    // together at the top; "Paid" carries real consequence (it's what the
    // Sales Tax Report's cash-basis math and the invoice filter treat as
    // settled) and sits below a separator, one deliberate step away from
    // the common ones rather than immediately adjacent to them.
    ui->InvoiceStatus_ComboBox->addItem("Draft");
    ui->InvoiceStatus_ComboBox->addItem("Sent");

    const int separatorIndex = ui->InvoiceStatus_ComboBox->count();
    ui->InvoiceStatus_ComboBox->insertSeparator(separatorIndex);

    ui->InvoiceStatus_ComboBox->addItem("Paid");
    ui->InvoiceStatus_ComboBox->addItem("Overdue");
    ui->InvoiceStatus_ComboBox->addItem("Void");

    // Belt-and-suspenders: insertSeparator() renders a visual divider, but
    // doesn't stop every style from letting keyboard navigation land on it.
    // Explicitly strip its selectable/enabled flags so it truly can't be
    // the current item, accidentally or otherwise.
    if (auto *model = qobject_cast<QStandardItemModel *>(ui->InvoiceStatus_ComboBox->model())) {
        if (QStandardItem *separatorItem = model->item(separatorIndex)) {
            separatorItem->setFlags(separatorItem->flags() & ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled));
        }
    }
}

int AddInvoice::resolveSelectedClientId() const
{
    return m_clientDisplayNameToId.value(ui->Client_LineEdit->text().trimmed(), 0);
}

void AddInvoice::onClientSelectionChanged()
{
    refreshClientSummary(resolveSelectedClientId());
}

void AddInvoice::refreshClientSummary(int clientId)
{
    if (clientId == 0) {
        ui->ClientSummary_TextBrowser->clear();
        return;
    }

    DAO_Client clientDao(m_dbManager);
    Client client = clientDao.getClientById(clientId);
    if (client.id == 0) {
        ui->ClientSummary_TextBrowser->clear();
        return;
    }

    // Reflect this client's tax-exempt flag on the invoice-wide toggle.
    // Still just a starting point: the user can flip it back afterward
    // (e.g. a normally-exempt client buying something taxable this once).
    ui->TaxStatus_ComboBox->setCurrentIndex(client.taxExempt ? 1 : 0);

    QStringList lines;
    QString name = QString("%1 %2").arg(client.firstName, client.lastName).trimmed();
    lines << name;
    if (!client.businessName.isEmpty()) {
        lines << client.businessName;
    }
    if (!client.email.isEmpty()) {
        lines << client.email;
    }
    if (!client.phoneNumber.isEmpty()) {
        lines << client.phoneNumber;
    }
    if (!client.address1.isEmpty()) {
        lines << client.address1;
    }
    if (!client.address2.isEmpty()) {
        lines << client.address2;
    }
    QString cityStateZip = QString("%1%2%3")
                               .arg(client.city, client.state.isEmpty() ? "" : ", " + client.state, client.postalCode.isEmpty() ? "" : " " + client.postalCode);
    if (!cityStateZip.trimmed().isEmpty()) {
        lines << cityStateZip;
    }

    ui->ClientSummary_TextBrowser->setPlainText(lines.join("\n"));
}

void AddInvoice::on_EditCustomer_PushButton_clicked()
{
    int clientId = resolveSelectedClientId();
    if (clientId == 0) {
        QMessageBox::warning(this, "No Client Selected", "Please select a client from the dropdown first.");
        return;
    }

    CustomerCard *card = new CustomerCard(m_dbManager, clientId, this);

    // If the name/business changes, the completer list and summary need to
    // catch up so this dialog isn't showing stale client info.
    connect(card, &CustomerCard::customerUpdated, this, [this](int updatedClientId) {
        populateClientCompleter(updatedClientId);
        refreshClientSummary(updatedClientId);
    });

    card->show();
    card->raise();
    card->activateWindow();
}

void AddInvoice::on_AddItem_PushButton_clicked()
{
    addBlankItemRow();
}

void AddInvoice::addBlankItemRow()
{
    int row = ui->Items_TableWidget->rowCount();

    // Setting these items fires cellChanged for each one, and TOTAL isn't
    // in place yet while ITEM/QUANTITY/DESCRIPTION/AMOUNT are being added.
    // Block signals for the initial population and recalc once at the end
    // instead of reacting to every intermediate setItem call.
    const QSignalBlocker blocker(ui->Items_TableWidget);

    ui->Items_TableWidget->insertRow(row);

    ui->Items_TableWidget->setItem(row, 0, new QTableWidgetItem());        // ITEM
    ui->Items_TableWidget->setItem(row, 1, new QTableWidgetItem("1"));     // QUANTITY
    ui->Items_TableWidget->setItem(row, 2, new QTableWidgetItem());        // DESCRIPTION
    ui->Items_TableWidget->setItem(row, 3, new QTableWidgetItem("0.00"));  // AMOUNT

    QTableWidgetItem *totalItem = new QTableWidgetItem("0.00");            // TOTAL (computed)
    totalItem->setFlags(totalItem->flags() & ~Qt::ItemIsEditable);
    ui->Items_TableWidget->setItem(row, 4, totalItem);

    // TAX (5): dropdown of tax types loaded from tax_table, rather than a
    // freeform rate. Each entry's Qt::UserRole data is the tax rate as a
    // fraction (e.g. 7% -> 0.07) so recalculation can use it directly;
    // Qt::UserRole+1 holds the actual tax_id, used when persisting items on
    // save. "No Tax" has no UserRole+1 set, so currentData(UserRole+1)
    // comes back invalid -> .toInt() defaults to 0, matching InvoiceItem's
    // "0 = No Tax/exempt" convention. Disabled whenever the invoice as a
    // whole is Tax Exempt.
    QComboBox *taxCombo = new QComboBox(this);
    taxCombo->addItem("No Tax", 0.0);
    for (const TaxRate &rate : std::as_const(m_taxRates)) {
        QString label = QString("%1 (%2%)").arg(rate.name).arg(rate.ratePercent);
        taxCombo->addItem(label, rate.ratePercent / 100.0);
        taxCombo->setItemData(taxCombo->count() - 1, rate.id, Qt::UserRole + 1);
    }
    taxCombo->setEnabled(isInvoiceTaxable());
    connect(taxCombo, &QComboBox::currentIndexChanged, this, [this]() {
        recalculateInvoiceTotals();
    });
    ui->Items_TableWidget->setCellWidget(row, 5, taxCombo);

    QPushButton *removeButton = new QPushButton("Remove", this);
    connect(removeButton, &QPushButton::clicked, this, [this, removeButton]() {
        for (int r = 0; r < ui->Items_TableWidget->rowCount(); ++r) {
            if (ui->Items_TableWidget->cellWidget(r, 6) == removeButton) {
                ui->Items_TableWidget->removeRow(r);
                break;
            }
        }
        recalculateInvoiceTotals();
    });
    ui->Items_TableWidget->setCellWidget(row, 6, removeButton);
}

void AddInvoice::recalculateRowTotal(int row)
{
    QTableWidgetItem *quantityItem = ui->Items_TableWidget->item(row, 1);
    QTableWidgetItem *amountItem = ui->Items_TableWidget->item(row, 3);
    double quantity = quantityItem ? quantityItem->text().toDouble() : 0.0;
    double amount = amountItem ? amountItem->text().toDouble() : 0.0;

    QTableWidgetItem *totalItem = ui->Items_TableWidget->item(row, 4);
    if (!totalItem) {
        totalItem = new QTableWidgetItem();
        totalItem->setFlags(totalItem->flags() & ~Qt::ItemIsEditable);
        ui->Items_TableWidget->setItem(row, 4, totalItem);
    }

    // Writing the TOTAL item would otherwise re-fire cellChanged and loop
    // back into this same recalculation.
    const QSignalBlocker blocker(ui->Items_TableWidget);
    totalItem->setText(QString::number(quantity * amount, 'f', 2));
}

void AddInvoice::recalculateInvoiceTotals()
{
    double invoiceTotal = 0.0;
    double taxTotal = 0.0;
    const bool taxable = isInvoiceTaxable();

    for (int row = 0; row < ui->Items_TableWidget->rowCount(); ++row) {
        QTableWidgetItem *totalItem = ui->Items_TableWidget->item(row, 4);
        double total = totalItem ? totalItem->text().toDouble() : 0.0;
        invoiceTotal += total;

        // A Tax Exempt invoice charges no tax at all, regardless of what's
        // selected in any row's (disabled) TAX dropdown.
        if (taxable) {
            auto *taxCombo = qobject_cast<QComboBox *>(ui->Items_TableWidget->cellWidget(row, 5));
            double tax = taxCombo ? taxCombo->currentData().toDouble() : 0.0;
            taxTotal += tax * total;
        }
    }

    ui->taxTotal_lineEdit->setText(QString::number(taxTotal, 'f', 2));
    ui->GrandTotal_lineEdit->setText(QString::number(invoiceTotal + taxTotal, 'f', 2));
}

bool AddInvoice::isInvoiceTaxable() const
{
    return ui->TaxStatus_ComboBox->currentIndex() == 0; // 0 = "Taxable", 1 = "Tax Exempt"
}

void AddInvoice::onItemsTableCellChanged(int row, int column)
{
    // QUANTITY (1) or AMOUNT (3) changed: that row's TOTAL is stale.
    if (column == 1 || column == 3) {
        recalculateRowTotal(row);
    }

    // Every path here (including a bare TAX edit) affects the invoice-wide
    // totals.
    recalculateInvoiceTotals();
}

int AddInvoice::saveCurrentInvoice()
{
    QString clientText = ui->Client_LineEdit->text().trimmed();
    int clientId = resolveSelectedClientId();

    // Build Invoice structure from UI inputs.
    // Note: this layout doesn't collect an invoice-level tax rate or
    // discount, so new invoices always start with tax_rate and
    // discount_amount at 0. Status now comes from Status_ComboBox (defaults
    // to "Sent" but is editable before save, including down to "Draft") rather than being hardcoded.
    // Line items ARE persisted (see below, via InvoiceItemDao) now that
    // tax_id exists on invoice_items.
    Invoice newInvoice;
    newInvoice.clientId = clientId;
    newInvoice.invoiceNumber = ui->InvoiceNumber_LineEdit->text().trimmed();
    newInvoice.issueDate = ui->IssueDate_DateEdit->date().toString("yyyy-MM-dd");
    newInvoice.dueDate = ui->DueDate_DateEdit->date().toString("yyyy-MM-dd");
    newInvoice.status = ui->InvoiceStatus_ComboBox->currentText();
    newInvoice.taxRate = 0;
    newInvoice.taxable = isInvoiceTaxable();
    newInvoice.discountAmount = 0;
    newInvoice.poNumber = ui->PONumber_LineEdit->text().trimmed();
    newInvoice.notes = ui->Notes_PlainTextEdit->toPlainText().trimmed();

    // Validation
    if (clientText.isEmpty() || clientId == 0) {
        QMessageBox::warning(this, "Validation Error", "Please select a client from the dropdown.");
        return 0;
    }
    if (newInvoice.invoiceNumber.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Invoice Number is required.");
        return 0;
    }
    if (newInvoice.dueDate < newInvoice.issueDate) {
        QMessageBox::warning(this, "Validation Error", "Due date cannot be before the issue date.");
        return 0;
    }

    // Confirm before this invoice leaves Draft status. Comes after the
    // validation checks above (no point confirming a status the invoice
    // won't even get saved with) but before the actual insert.
    if (!confirmNonDraftInvoiceStatus(this, newInvoice.status)) {
        return 0;
    }

    DAO_Invoice invoiceDao(m_dbManager);
    int newId = invoiceDao.insertInvoice(newInvoice);
    if (newId == 0) {
        QMessageBox::critical(this, "Error", "Failed to save invoice to database. The invoice number may already be in use.");
        return 0;
    }

    // Persist line items. If the invoice as a whole is Tax Exempt, every
    // item is forced to taxId 0 regardless of what's selected in its
    // (disabled) dropdown -- the disabled state alone doesn't stop a
    // previously-chosen rate from still being what the combo reports.
    const bool taxable = isInvoiceTaxable();
    QVector<InvoiceItem> items;
    for (int row = 0; row < ui->Items_TableWidget->rowCount(); ++row) {
        QTableWidgetItem *itemNameCell = ui->Items_TableWidget->item(row, 0);
        QTableWidgetItem *quantityCell = ui->Items_TableWidget->item(row, 1);
        QTableWidgetItem *descriptionCell = ui->Items_TableWidget->item(row, 2);
        QTableWidgetItem *amountCell = ui->Items_TableWidget->item(row, 3);
        auto *taxCombo = qobject_cast<QComboBox *>(ui->Items_TableWidget->cellWidget(row, 5));

        InvoiceItem item;
        item.invoiceId = newId;
        // invoice_items has a single description column; ITEM is the short
        // name, DESCRIPTION is the optional longer text, so fold both in.
        QString itemName = itemNameCell ? itemNameCell->text().trimmed() : QString();
        QString itemDescription = descriptionCell ? descriptionCell->text().trimmed() : QString();
        item.description = itemDescription.isEmpty() ? itemName : QString("%1 - %2").arg(itemName, itemDescription);
        item.quantity = quantityCell ? quantityCell->text().toDouble() : 0.0;
        item.snapshotUnitPriceCents = qRound((amountCell ? amountCell->text().toDouble() : 0.0) * 100.0);
        item.taxId = (taxable && taxCombo) ? taxCombo->currentData(Qt::UserRole + 1).toInt() : 0;

        items.append(item);
    }

    DAO_Items itemDao(m_dbManager);
    if (!items.isEmpty() && !itemDao.replaceItemsForInvoice(newId, items)) {
        QMessageBox::warning(this, "Partial Save",
                             "The invoice was saved, but its line items failed to save. You can re-open and re-save it to retry.");
    }

    return newId;
}

void AddInvoice::resetFormForNewInvoice()
{
    ui->InvoiceNumber_LineEdit->clear();
    ui->IssueDate_DateEdit->setDate(QDate::currentDate());
    ui->NetTerms_ComboBox->setCurrentIndex(kDefaultNetTermsIndex); // also resets Due Date
    ui->InvoiceStatus_ComboBox->setCurrentIndex(1); // back to "Sent" for the next invoice
    ui->PONumber_LineEdit->clear();
    ui->Items_TableWidget->setRowCount(0);
    ui->taxTotal_lineEdit->clear();
    ui->GrandTotal_lineEdit->clear();
    ui->Notes_PlainTextEdit->clear();

    // Client field and its summary are intentionally left as-is, so
    // creating several invoices for the same client in one sitting doesn't
    // require re-picking them each time.
}

void AddInvoice::on_Cancel_PushButton_clicked()
{
    reject();
}

void AddInvoice::on_SaveClose_PushButton_clicked()
{
    int newId = saveCurrentInvoice();
    if (newId != 0) {
        emit invoiceSaved(newId);
        accept(); // Close dialog with QDialog::Accepted
    }
}

void AddInvoice::on_SaveNew_PushButton_clicked()
{
    int newId = saveCurrentInvoice();
    if (newId != 0) {
        emit invoiceSaved(newId);
        resetFormForNewInvoice();
    }
}