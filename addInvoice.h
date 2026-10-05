#ifndef ADDINVOICE_H
#define ADDINVOICE_H

#include <QDialog>
#include <QMap>
#include <QString>
#include <QVector>
#include "DAO_Tax.h" // Changed from taxdao.h

// Forward declaration
class DatabaseManager;


namespace Ui {
class AddInvoice;
}

class AddInvoice : public QDialog
{
    Q_OBJECT

public:

    // preselectedClientId lets a caller that already knows the client
    // (e.g. a "New Invoice" button on that client's card) open the dialog
    // with the client field already filled in. Pass 0 when the caller
    // doesn't have a client in context and the user should pick one.
    explicit AddInvoice(DatabaseManager& dbManager, int preselectedClientId = 0, QWidget *parent = nullptr);
    ~AddInvoice();

signals:
    // Emitted after every successful save (both Save/Close and Save/New),
    // so a list view can refresh without waiting for the dialog to close.
    void invoiceSaved(int invoiceId);

private slots:
    void on_Cancel_PushButton_clicked();
    void on_SaveClose_PushButton_clicked();
    void on_SaveNew_PushButton_clicked();
    void on_EditCustomer_PushButton_clicked();
    void on_AddItem_PushButton_clicked();

    // Connected manually (not auto-connect) to the completer's activated
    // signal and the line edit's editingFinished, since both should trigger
    // the same "client selection changed" refresh.
    void onClientSelectionChanged();

    // Connected to Items_TableWidget::cellChanged. Recomputes that row's
    // TOTAL when QUANTITY or AMOUNT is edited, then always refreshes the
    // invoice-level tax/grand totals (since a TAX edit alone still needs
    // to flow through).
    void onItemsTableCellChanged(int row, int column);

private:

    // Loads all clients into a QCompleter attached to Client_LineEdit,
    // and builds m_clientDisplayNameToId for resolving the typed text back
    // to a client_id on save.
    void populateClientCompleter(int preselectedClientId);

    // Builds Status_ComboBox's items in code rather than in the .ui, since
    // Designer's static item list can't express the separator between
    // "Sent" and "Paid" -- that gap exists on purpose, to put a click of
    // real consequence (marking something Paid) a deliberate step away from
    // the two statuses used on every ordinary invoice.
    void populateStatusComboBox();

    // Resolves the currently typed client text to a client_id, or 0 if it
    // doesn't match anything in the completer's list.
    int resolveSelectedClientId() const;

    // Fills ClientSummary_TextBrowser with the given client's contact info,
    // or clears it if clientId is 0.
    void refreshClientSummary(int clientId);

    // Builds an Invoice from the current form state, validates it, and
    // inserts it. Returns the new invoice_id, or 0 if validation or the
    // insert failed (a message box is shown internally in that case).
    int saveCurrentInvoice();

    // Clears invoice-specific fields (number, dates, PO number, net terms,
    // items, notes) while leaving the selected client in place, for
    // Save/New.
    void resetFormForNewInvoice();

    // Appends a blank, editable row to Items_TableWidget with a Remove
    // button in the trailing column.
    void addBlankItemRow();

    // Sets column 4 (TOTAL) of the given row to QUANTITY * AMOUNT. Creates
    // the TOTAL item (read-only) if the row doesn't have one yet. Blocks
    // Items_TableWidget's signals while writing so this doesn't recurse
    // back into onItemsTableCellChanged.
    void recalculateRowTotal(int row);

    // Sums every row's TOTAL into taxTotal_lineEdit's counterpart math:
    // taxTotal_lineEdit = sum(TAX_i * TOTAL_i), GrandTotal_lineEdit =
    // sum(TOTAL_i) + taxTotal_lineEdit. Called after any row's QUANTITY,
    // AMOUNT, or TAX changes, and whenever a row is added or removed.
    // If the invoice is marked Tax Exempt, tax is forced to 0 regardless
    // of what any row's (disabled) TAX dropdown has selected.
    void recalculateInvoiceTotals();

    // True when TaxStatus_ComboBox is set to "Taxable" (index 0), false
    // when "Tax Exempt" (index 1).
    bool isInvoiceTaxable() const;


    Ui::AddInvoice *ui;
    DatabaseManager& m_dbManager; // Store reference to manager
    QMap<QString, int> m_clientDisplayNameToId; // exact display text -> client_id

    // Loaded once per dialog instance from tax_table; used to populate each
    // new row's TAX combo box without re-querying the DB per row.
    QVector<TaxRate> m_taxRates;
};

#endif // ADDINVOICE_H
