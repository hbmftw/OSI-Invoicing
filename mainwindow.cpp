#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "invoice_window.h"
#include "contacts_customer.h"
#include "addInvoice.h"
#include "addPayment.h"
#include "addclient.h"

#include <QPushButton>
#include <QList>
#include <QMessageBox>
#include <QMdiSubWindow>
#include <QInputDialog>
#include <QLineEdit>
#include <QSqlQuery>
#include <QIcon>
#include <QSize>

MainWindow::MainWindow(DatabaseManager& dbManager, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)

    , m_dbManager(dbManager) // Bind reference passed from main.cpp
{
    ui->setupUi(this);

    // Enable tabbed view for MDIArea
    ui->mdiArea_main->setViewMode(QMdiArea::TabbedView);

    // Add 'X' button to tab to close
    ui->mdiArea_main->setTabsClosable(true);
    // Allow tabs to be reordered
    ui->mdiArea_main->setTabsMovable(true);
    // Make tabs rounded
    ui->mdiArea_main->setTabShape(QTabWidget::Rounded);

    connect(ui->mdiArea_main, &QMdiArea::subWindowActivated, this, &MainWindow::onSubWindowActivated);

    //-----------Push Button Icons--------------
    const QIcon addIcon(":/new/prefix1/add_circle.svg");
    const QSize iconSize(16, 16);   // adjust to taste

    const QList<QPushButton*> addButtons = {
        ui->addCustomerButton,
        ui->addInvoiceButton,
        ui->addPaymentsButton,
        ui->addProductsButton
    };

    for (QPushButton *btn : addButtons) {
        btn->setIcon(addIcon);
        btn->setIconSize(iconSize);
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { background: transparent; border: none; }"
            "QPushButton:hover { background: rgba(0, 0, 0, 30); border-radius: 6px; }"
            "QPushButton:pressed { background: rgba(0, 0, 0, 60); border-radius: 6px; }"
            );
    }

}

MainWindow::~MainWindow()
{
    delete ui;

}

template <typename T>
void MainWindow::openOrActivateTab()
{
    // if a tab of this type already exists, just switch to it
    const auto subs = ui->mdiArea_main->subWindowList();
    for (QMdiSubWindow *sub : subs){
        if (qobject_cast<T*>(sub->widget())) {
            ui->mdiArea_main->setActiveSubWindow(sub);
            return;
        }
    }

    // Otherwise create a new one
    T *window = new T(m_dbManager, this);
    QMdiSubWindow *sub = ui->mdiArea_main->addSubWindow(window);
    sub->setAttribute(Qt::WA_DeleteOnClose);
    sub->showMaximized();
}

void MainWindow::onSubWindowActivated(QMdiSubWindow *sub)
{
    if (!sub || !sub->widget()) {return; } // Null when the last tab closes

    // Calls refresh() on whichever window in now active (no-op if it has none)
    QMetaObject::invokeMethod(sub->widget(), "refresh", Qt::DirectConnection);
}

void MainWindow::openReceivePaymentFlow() // Called in ReceivePayment_Button
{
    bool ok = false;
    QString invoiceNumber = QInputDialog::getText(this, "Receive Payment", "Invoice Number:",
                                                  QLineEdit::Normal, QString(),&ok);
    if (!ok || invoiceNumber.trimmed().isEmpty()){ return; }

    DAO_Invoice invoiceDao(m_dbManager);
    int invoiceId = invoiceDao.getInvoiceIdByNumber(invoiceNumber.trimmed());
    if (invoiceId == 0) {
        QMessageBox::warning(this, "Not Found", QString("No invoice found with number \"%1\".").arg(invoiceNumber.trimmed()));
        return;
    }

    AddPayment dialog(m_dbManager, invoiceId, this);
    dialog.exec();

}

void MainWindow::on_Customers_Button_clicked() {openOrActivateTab<Contacts_Customer>(); }

void MainWindow::on_addCustomerButton_clicked()
{


}

void MainWindow::on_Invoices_Button_clicked() { openOrActivateTab<Invoice_Window>(); }

void MainWindow::on_addInvoiceButton_clicked()
{

    // No client is known ahead of time from this entry point,
    // so the dialog opens with its client picker unset (0).
    AddInvoice dialog(m_dbManager, 0, this);

    dialog.exec();

}


void MainWindow::on_addPaymentsButton_clicked()
{

    openReceivePaymentFlow(); // function is above!

}


void MainWindow::on_addProductsButton_clicked()
{

}



