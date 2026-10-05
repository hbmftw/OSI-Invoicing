#ifndef INVOICESTATUSGUARD_H
#define INVOICESTATUSGUARD_H

#include <QWidget>
#include <QMessageBox>
#include <QString>

// Shared confirmation guardrail for invoice status. "Draft" carries no
// consequence and needs no confirmation. Anything else does: "Sent"/
// "Overdue"/"Void" mark something as actually issued rather than a
// scratch invoice, and "Paid" specifically feeds the Sales Tax Report's
// cash-basis math and the invoice list's Paid/Due filter -- so leaving
// Draft is a deliberate confirmed step, not a stray combo-box click.
//
// Used by both AddInvoiceDialog (on creation, since new invoices default
// to "Sent") and InvoiceCard (on edit, only when status is actually being
// changed away from Draft in that edit -- see call sites).
inline bool confirmNonDraftInvoiceStatus(QWidget *parent, const QString &status)
{
    if (status == "Draft") {
        return true; // nothing to confirm
    }

    const QString message = (status == "Paid")
                                ? "Mark this invoice as Paid? This affects payment tracking and tax reporting."
                                : QString("Save this invoice with status \"%1\"?").arg(status);

    return QMessageBox::question(parent, "Confirm Invoice Status", message,
                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
           == QMessageBox::Yes;
}

#endif // INVOICESTATUSGUARD_H
