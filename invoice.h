#ifndef INVOICE_H
#define INVOICE_H

#include <QString>

// A lightweight struct for passing invoice data between the database layer
// and the UI
struct Invoice {
    int id{0};
    int clientId{0};
    QString invoiceNumber;
    QString issueDate;       // "yyyy-MM-dd"
    QString dueDate;         // "yyyy-MM-dd"
    QString status;          // e.g. "Draft", "Sent", "Paid", "Overdue", "Void"
    int taxRate{0};          // whole percentage, e.g. 7 == 7%
    bool taxable{true};      // Is taxable
    int discountAmount{0};   // stored in cents
    QString poNumber;
    QString notes;
};

#endif // INVOICE_H
