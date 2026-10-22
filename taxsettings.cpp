#include "taxsettings.h"
#include "ui_taxsettings.h"
#include "DatabaseManager.h"
#include "DAO_Tax.h"

#include <QMessageBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QWidget>

TaxSettings::TaxSettings(DatabaseManager &dbManager, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TaxSettings)
    , m_dbManager(dbManager)
{
    ui->setupUi(this);
    refreshTable();
}

TaxSettings::~TaxSettings()
{
    delete ui;
}

void TaxSettings::refreshTable()
{
    DAO_Tax taxDao(m_dbManager);
    QVector<TaxRate> rates = taxDao.getAllTaxRates();

    ui->TaxRates_TableWidget->clearContents();
    ui->TaxRates_TableWidget->setRowCount(rates.size());

    for (int row = 0; row < rates.size(); ++row) {
        const TaxRate rate = rates[row]; // captured by value for the row's lambdas below
        const int taxId = rate.id;

        auto *nameEdit = new QLineEdit(rate.name, this);
        ui->TaxRates_TableWidget->setCellWidget(row, 0, nameEdit);

        auto *rateSpin = new QSpinBox(this);
        rateSpin->setRange(0, 100);
        rateSpin->setValue(rate.ratePercent);
        ui->TaxRates_TableWidget->setCellWidget(row, 1, rateSpin);

        auto *activeCheck = new QCheckBox(this);
        activeCheck->setChecked(rate.active);
        // Center the checkbox in its cell rather than having it hug the left edge
        auto *activeContainer = new QWidget(this);
        auto *activeLayout = new QHBoxLayout(activeContainer);
        activeLayout->addWidget(activeCheck);
        activeLayout->setAlignment(Qt::AlignCenter);
        activeLayout->setContentsMargins(0, 0, 0, 0);
        ui->TaxRates_TableWidget->setCellWidget(row, 2, activeContainer);

        auto *saveButton = new QPushButton("Save", this);
        connect(saveButton, &QPushButton::clicked, this, [this, taxId, nameEdit, rateSpin, activeCheck]() {
            QString name = nameEdit->text().trimmed();
            if (name.isEmpty()) {
                QMessageBox::warning(this, "Validation Error", "Tax name is required.");
                return;
            }

            TaxRate updated;
            updated.id = taxId;
            updated.name = name;
            updated.ratePercent = rateSpin->value();
            updated.active = activeCheck->isChecked();

            DAO_Tax taxDao(m_dbManager);
            if (!taxDao.updateTaxRate(updated)) {
                QMessageBox::critical(this, "Error", "Failed to save tax rate to database.");
            }
        });
        ui->TaxRates_TableWidget->setCellWidget(row, 3, saveButton);
    }
}

void TaxSettings::on_AddRate_PushButton_clicked()
{
    QString name = ui->NewName_LineEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Enter a name for the new tax rate.");
        return;
    }

    DAO_Tax taxDao(m_dbManager);
    if (taxDao.insertTaxRate(name, ui->NewRate_SpinBox->value(), true) == 0) {
        QMessageBox::critical(this, "Error", "Failed to save tax rate to database.");
        return;
    }

    ui->NewName_LineEdit->clear();
    ui->NewRate_SpinBox->setValue(0);
    refreshTable();
}

void TaxSettings::on_Close_PushButton_clicked()
{
    // Embedded in a QMdiSubWindow (see MainWindow::on_actionTax_Settings_triggered).
    // close() on just this widget hides/destroys the widget itself but
    // leaves the QMdiSubWindow behind as an empty frame in the MDI area --
    // close the wrapper instead when there is one.
    if (QWidget *container = parentWidget()) {
        container->close();
    } else {
        close();
    }
}
