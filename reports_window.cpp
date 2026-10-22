#include "reports_window.h"
#include "ui_reports_window.h"
#include "salestaxreport.h"
#include "salesbyitem.h"
#include "salesbycustomer.h"
#include "arsummary.h"
#include "DatabaseManager.h"

#include <QIcon>
#include <QSize>

Reports_Window::Reports_Window(DatabaseManager& dbManager, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Reports_Window)
    , m_dbManager(dbManager)
{
    ui->setupUi(this);

    // Sidebar button icons
    ui->SalesTax_Button->setIcon(QIcon(":/new/prefix1/tax_report.png"));
    ui->SalesTax_Button->setIconSize(QSize(24, 24));

    ui->SalesByItem_Button->setIcon(QIcon(":/new/prefix1/item_report.png"));
    ui->SalesByItem_Button->setIconSize(QSize(24, 24));

    ui->SalesByCustomer_Button->setIcon(QIcon(":/new/prefix1/customer_report.png"));
    ui->SalesByCustomer_Button->setIconSize(QSize(24, 24));

    ui->ArSummary_Button->setIcon(QIcon(":/new/prefix1/ar_report.png"));
    ui->ArSummary_Button->setIconSize(QSize(24, 24));

    // Instantiate reports
    m_salesTaxReportWidget = new SalesTaxReport(m_dbManager, this);
    m_salesByItemWidget = new SalesByItem(m_dbManager, this);
    m_salesByCustomerWidget = new SalesByCustomer(m_dbManager, this);
    m_arSummaryWidget = new ArSummary(m_dbManager, this);

    // Stack container layout
    ui->reportsStackedWidget->addWidget(m_salesTaxReportWidget);
    ui->reportsStackedWidget->addWidget(m_salesByItemWidget);
    ui->reportsStackedWidget->addWidget(m_salesByCustomerWidget);
    ui->reportsStackedWidget->addWidget(m_arSummaryWidget);

    on_SalesTax_Button_clicked();
}

Reports_Window::~Reports_Window()
{
    delete ui;
}

void Reports_Window::on_SalesTax_Button_clicked()
{
    ui->reportsStackedWidget->setCurrentWidget(m_salesTaxReportWidget);
}

void Reports_Window::on_SalesByItem_Button_clicked()
{
    ui->reportsStackedWidget->setCurrentWidget(m_salesByItemWidget);
}

void Reports_Window::on_SalesByCustomer_Button_clicked()
{
    ui->reportsStackedWidget->setCurrentWidget(m_salesByCustomerWidget);
}

void Reports_Window::on_ArSummary_Button_clicked()
{
    ui->reportsStackedWidget->setCurrentWidget(m_arSummaryWidget);
}