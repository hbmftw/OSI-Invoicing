#include "invoice_window.h"
#include "ui_invoice_window.h"

#include "DatabaseManager.h"
#include "DAO_Invoice.h"
#include "DAO_Items.h" // Items will correspond with products, changed from invoiceitemdao.h
#include "DAO_Payment.h"
#include "addInvoice.h" //Changed from addinvoicedialog.h
#include "invoicecard.h"
#include "addPayment.h" //Changed from recordpaymentdialog.h
#include "pagebar.h"

#include <QMessageBox>
#include <QPushButton>

Invoice_Window::Invoice_Window(DatabaseManager& dbManager, QWidget *parent) // Add: DatabaseManager& dbManager
    : QWidget(parent)
    , ui(new Ui::Invoice_Window)
    , m_dbManager(dbManager) // Bind reference in initializer list
{
    ui->setupUi(this);

    // Paging bar under the table: 25 / 50 / 100 rows per page
    m_pageBar = new PageBar([this]() { refreshInvoicesTable(); }, this);
    ui->verticalLayout->addWidget(m_pageBar);

    // Set up table headers (also defined in the .ui file; kept here in case columns change at runtime)
    ui->invoicesTableWidget->setColumnCount(9);
    ui->invoicesTableWidget->setHorizontalHeaderLabels({"Invoice #", "Client", "Issue Date", "Due Date", "Amount", "Payment", "Status", "", ""});

    // Line Edit for Invoice Search Field
    // Place Holder Text
    ui->InvoiceSearch_Field->setPlaceholderText("Search Invoices, Client Name...");
    // Add "Clear" button
    ui->InvoiceSearch_Field->setClearButtonEnabled(true);
    // Add magnifing glass icon to the left side of text box
    QAction *searchAction = new QAction(this);
    searchAction->setIcon(QIcon(":/new/prefix1/search.png"));// Path to search icon (Resources file)
    ui->InvoiceSearch_Field->addAction(searchAction, QLineEdit::LeadingPosition);

    // Populate the table with all invoices on first open
    refreshInvoicesTable();
}

Invoice_Window::~Invoice_Window()
{
    delete ui;
}

// Re-runs the search using the current text and the status filter
void Invoice_Window::refresh() { refreshInvoicesTable();}

void Invoice_Window::on_InvoiceSearch_Field_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    if (m_pageBar)
        m_pageBar->resetPage();
    refreshInvoicesTable();
}

void Invoice_Window::on_StatusFilter_ComboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    if (m_pageBar)
        m_pageBar->resetPage();
    refreshInvoicesTable();
}

void Invoice_Window::refreshInvoicesTable()
{
    if (!m_pageBar)
        return;

    DAO_Invoice invoiceDao(m_dbManager);
    const QString term = ui->InvoiceSearch_Field->text();

    // Paid/Due filtering is based on actual recorded payments -- the same
    // computation the Payment column uses -- rather than the manually-set
    // Status field. It is done in SQL (index 0 = All, 1 = Paid, 2 = Due) so
    // the total and each page respect the filter.
    const int filterIndex = ui->StatusFilter_ComboBox->currentIndex();

    // Total first: it can move the page back if the current one no longer exists
    m_pageBar->setTotal(invoiceDao.countInvoices(term, filterIndex));
    populateInvoicesTable(invoiceDao.searchInvoicesPage(term, filterIndex, m_pageBar->limit(), m_pageBar->offset()));
}

void Invoice_Window::populateInvoicesTable(const QVector<InvoiceListItem> &invoices) {
    ui->invoicesTableWidget->clearContents();
    ui->invoicesTableWidget->setRowCount(invoices.size());

    DAO_Items itemDao(m_dbManager); // reused across rows rather than re-constructed per row
    DAO_Payment paymentDao(m_dbManager);

    for (int row = 0; row < invoices.size(); ++row) {
        const InvoiceListItem &invoice = invoices[row];

        QTableWidgetItem *numberItem = new QTableWidgetItem(invoice.invoiceNumber);
        numberItem->setData(Qt::UserRole, invoice.id);

        ui->invoicesTableWidget->setItem(row, 0, numberItem);
        ui->invoicesTableWidget->setItem(row, 1, new QTableWidgetItem(invoice.clientDisplayName));
        ui->invoicesTableWidget->setItem(row, 2, new QTableWidgetItem(invoice.issueDate));
        ui->invoicesTableWidget->setItem(row, 3, new QTableWidgetItem(invoice.dueDate));

        const int totalCents = itemDao.getInvoiceTotalCents(invoice.id);
        auto *amountItem = new QTableWidgetItem(QString("$%1").arg(totalCents / 100.0, 0, 'f', 2));
        amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ui->invoicesTableWidget->setItem(row, 4, amountItem);

        // Computed from actual recorded payments, independent of the
        // manually-set Status field (an invoice can be "Sent" and fully
        // paid, or "Paid" on paper with no payments actually recorded --
        // this column reflects the real payment data, that one doesn't).
        const int paidCents = paymentDao.getTotalPaidForInvoice(invoice.id);
        QString paymentText;
        if (totalCents > 0 && paidCents >= totalCents) {
            paymentText = "Paid";
        } else if (paidCents > 0) {
            paymentText = "Partial";
        } else {
            paymentText = "Unpaid";
        }
        ui->invoicesTableWidget->setItem(row, 5, new QTableWidgetItem(paymentText));

        ui->invoicesTableWidget->setItem(row, 6, new QTableWidgetItem(invoice.status));

        const int invoiceId = invoice.id; // captured by value, not by row index

        QPushButton *editButton = new QPushButton("Edit", this);
        connect(editButton, &QPushButton::clicked, this, [this, invoiceId]() {
            openInvoiceCard(invoiceId);
        });
        ui->invoicesTableWidget->setCellWidget(row, 7, editButton);

        QPushButton *receivePaymentButton = new QPushButton("Add Payment", this);
        connect(receivePaymentButton, &QPushButton::clicked, this, [this, invoiceId]() {
            openReceivePayment(invoiceId);
        });
        ui->invoicesTableWidget->setCellWidget(row, 8, receivePaymentButton);
    }
}

void Invoice_Window::openInvoiceCard(int invoiceId)
{
    InvoiceCard *card = new InvoiceCard(m_dbManager, invoiceId, this);

    // Refresh the results table whenever the card saves a change
    connect(card, &InvoiceCard::invoiceUpdated, this, [this](int) {
        refreshInvoicesTable();
    });

    card->show();
    card->raise();
    card->activateWindow();
}

void Invoice_Window::openReceivePayment(int invoiceId)
{
    AddPayment dialog(m_dbManager, invoiceId, this);
    if (dialog.exec() == QDialog::Accepted) {
        // Doesn't change invoice.status by itself, but refreshing keeps
        // the row's numbers current with anything else that might have
        // changed, and is cheap enough not to bother checking.
        refreshInvoicesTable();
    }
}

// Add New Invoice
void Invoice_Window::on_NewInvoice_Button_clicked()
{
    // No client is known ahead of time from this entry point,
    // so the dialog opens with its client picker unset (0).
    AddInvoice dialog(m_dbManager, 0, this);

    // Save/New keeps the dialog open for another invoice, so refresh on
    // every save rather than only once the dialog finally closes.
    connect(&dialog, &AddInvoice::invoiceSaved, this, [this](int) {
        refreshInvoicesTable();
    });

    dialog.exec();
}
