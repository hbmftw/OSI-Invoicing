#include "universalimportdialog.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlError>
#include <QStackedWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextStream>
#include <utility>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QUrl>

#include "csvreader.h"
#include "DAO_Items.h"
#include "DAO_Services.h"
#include "DAO_Tax.h"

#include <QSet>

namespace {

// Gathers the existing clients, services, tax rates and invoice numbers the
// invoice-history builder needs to match CSV rows against.
EntityMapper::HistoryContext buildHistoryContext(DatabaseManager& dbManager)
{
    EntityMapper::HistoryContext ctx;

    auto addClientKey = [&ctx](const QString& text, int id) {
        const QString key = EntityMapper::lookupKey(text);
        if (key.isEmpty())
            return;
        QVector<int>& ids = ctx.clientIdsByKey[key];
        if (!ids.contains(id))
            ids.append(id);
    };
    DAO_Client clientDao(dbManager);
    const QVector<Client> clients = clientDao.searchClients(QString());
    for (const Client& c : clients) {
        addClientKey(QStringLiteral("%1 %2").arg(c.firstName, c.lastName), c.id);
        addClientKey(QStringLiteral("%1, %2").arg(c.lastName, c.firstName), c.id);
        addClientKey(c.businessName, c.id);
        addClientKey(c.email, c.id);
    }

    DAO_Services serviceDao(dbManager);
    const QVector<Item> services = serviceDao.getAllServices();
    for (const Item& svc : services)
        ctx.serviceIdByName.insert(EntityMapper::lookupKey(svc.name), svc.id);

    // -1 marks a key that matches more than one tax rate.
    DAO_Tax taxDao(dbManager);
    const QVector<TaxRate> rates = taxDao.getAllTaxRates();
    for (const TaxRate& rate : rates) {
        const QString nameKey = EntityMapper::lookupKey(rate.name);
        if (!nameKey.isEmpty())
            ctx.taxIdByKey[nameKey] = ctx.taxIdByKey.contains(nameKey) ? -1 : rate.id;
        if (rate.active) {
            const QString pctKey = QStringLiteral("pct:%1").arg(rate.ratePercent);
            ctx.taxIdByKey[pctKey] = ctx.taxIdByKey.contains(pctKey) ? -1 : rate.id;
        }
    }

    DAO_Invoice invoiceDao(dbManager);
    const QVector<InvoiceListItem> invoices = invoiceDao.searchInvoices(QString());
    for (const InvoiceListItem& inv : invoices)
        ctx.existingInvoiceNumbers.insert(EntityMapper::lookupKey(inv.invoiceNumber));

    return ctx;
}

// Existing invoices (to match payments to) and payments (to skip repeats).
EntityMapper::PaymentContext buildPaymentContext(DatabaseManager& dbManager)
{
    EntityMapper::PaymentContext ctx;

    auto compact = [](const QString& text) {
        QString out;
        for (const QChar c : text)
            if (c.isLetterOrNumber())
                out += c.toLower();
        return out;
    };

    DAO_Invoice invoiceDao(dbManager);
    const QVector<InvoiceListItem> invoices = invoiceDao.searchInvoices(QString());
    for (const InvoiceListItem& inv : invoices) {
        const QString key = EntityMapper::lookupKey(inv.invoiceNumber);
        ctx.invoiceIdByNumber[key] = ctx.invoiceIdByNumber.contains(key) ? -1 : inv.id;
        const QString ck = compact(inv.invoiceNumber);
        ctx.invoiceIdByCompact[ck] = ctx.invoiceIdByCompact.contains(ck) ? -1 : inv.id;
        ctx.numberById.insert(inv.id, inv.invoiceNumber);
    }

    DAO_Payment paymentDao(dbManager);
    const QVector<PaymentListItem> payments = paymentDao.searchPayments(QString(), QString());
    for (const PaymentListItem& p : payments) {
        ctx.existingPayments.insert(QStringLiteral("%1|%2|%3|%4")
                                        .arg(p.invoiceId).arg(p.paymentDate).arg(p.amountCents)
                                        .arg(EntityMapper::lookupKey(p.method)));
    }
    return ctx;
}

} // namespace

UniversalImportDialog::UniversalImportDialog(DatabaseManager& dbManager, QWidget* parent)
    : QDialog(parent)
    , m_dbManager(dbManager)
    , m_clientDao(dbManager)
    , m_invoiceDao(dbManager)
    , m_paymentDao(dbManager)
    , m_companyDao(dbManager)
    , m_selectedEntity(ImportEntity::Client) // Default target entity strictly set to Client
{
    setWindowTitle(tr("Universal CSV Import Wizard"));
    resize(950, 650);

    auto* root = new QVBoxLayout(this);

    // ---------------- Import-order banner ----------------
    auto* banner = new QFrame;
    banner->setFrameShape(QFrame::StyledPanel);
    banner->setStyleSheet(QStringLiteral(
        "QFrame { background: #fff8e1; border: 1px solid #f0c36d; border-radius: 4px; }"
        "QLabel { border: none; background: transparent; color: #5d4300; }"
        "QPushButton { background: #ffffff; color: #1a1a1a; border: 1px solid #c9a24a; border-radius: 4px; padding: 6px 14px; }"));

    auto* bannerLayout = new QHBoxLayout(banner);

    auto* bannerText = new QLabel(tr(
        "<b>Import in this order:</b> "
        "<b>1.</b> Customers &rarr; <b>2.</b> Items &rarr; <b>3.</b> Invoices &rarr; <b>4.</b> Payments"
        "<br>Each step relies on the one before it: invoices are matched to existing customers "
        "and items, and payments are matched to existing invoices."));
    bannerText->setWordWrap(true);

    m_openPrepBtn = new QPushButton(tr("Open CSV Prep Tool"));
    m_openPrepBtn->setToolTip(tr("Opens the CSV prep page in your web browser so you can fix up "
                                 "your files before importing."));

    bannerLayout->addWidget(bannerText, 1);
    bannerLayout->addWidget(m_openPrepBtn, 0, Qt::AlignVCenter);
    root->addWidget(banner);

    m_pages = new QStackedWidget(this);
    root->addWidget(m_pages, 1);

    // ---------------- Page 0: File & Target Entity Selection ----------------
    auto* filePage = new QWidget;
    auto* fileLayout = new QVBoxLayout(filePage);

    auto* selectorBox = new QHBoxLayout;
    auto* selectorLabel = new QLabel(tr("Import Data Into Table:"));
    selectorLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));

    m_entitySelector = new QComboBox;
    m_entitySelector->addItem(tr("Clients (Customers)"), static_cast<int>(ImportEntity::Client));
    m_entitySelector->addItem(tr("Services / Products"), static_cast<int>(ImportEntity::Item));
    m_entitySelector->addItem(tr("Invoice History (one row per line item)"), static_cast<int>(ImportEntity::InvoiceHistory));
    m_entitySelector->addItem(tr("Invoices (headers only, no line items)"), static_cast<int>(ImportEntity::Invoice));
    m_entitySelector->addItem(tr("Payments"), static_cast<int>(ImportEntity::Payment));

    // Only allow Company Profile import if no company profile exists in DB
    const Company existingCompany = m_companyDao.getCompany();
    if (!existingCompany.exists) {
        m_entitySelector->addItem(tr("Company Profile (One-time Setup)"), static_cast<int>(ImportEntity::Company));
    }

    selectorBox->addWidget(selectorLabel);
    selectorBox->addWidget(m_entitySelector, 1);

    auto* pickRow = new QHBoxLayout;
    m_pathEdit = new QLineEdit;
    m_pathEdit->setReadOnly(true);
    m_pathEdit->setPlaceholderText(tr("No file chosen"));
    auto* browse = new QPushButton(tr("Browse..."));
    pickRow->addWidget(m_pathEdit, 1);
    pickRow->addWidget(browse);

    m_fileStatus = new QLabel;
    fileLayout->addLayout(selectorBox);
    fileLayout->addSpacing(10);
    fileLayout->addLayout(pickRow);
    fileLayout->addWidget(m_fileStatus);
    fileLayout->addStretch();
    m_pages->addWidget(filePage);

    // ---------------- Page 1: Column Mapping ----------------
    auto* mapPage = new QWidget;
    auto* mapLayout = new QVBoxLayout(mapPage);
    auto* mapHint = new QLabel(tr("Match the target database fields on the left to the columns from your CSV file on the right."));
    mapHint->setWordWrap(true);

    m_mapTable = new QTableWidget(0, 4);
    m_mapTable->setHorizontalHeaderLabels({tr("Target Field"), tr("CSV Header"), tr("Match Confidence"), tr("Sample Value")});
    m_mapTable->verticalHeader()->setVisible(false);
    m_mapTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_mapTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_mapTable->horizontalHeader()->setStretchLastSection(true);
    m_mapTable->setColumnWidth(0, 200);
    m_mapTable->setColumnWidth(1, 250);
    m_mapTable->setColumnWidth(2, 130);

    mapLayout->addWidget(mapHint);
    mapLayout->addWidget(m_mapTable, 1);
    m_pages->addWidget(mapPage);

    // ---------------- Page 2: Preview ----------------
    auto* previewPage = new QWidget;
    auto* previewLayout = new QVBoxLayout(previewPage);
    m_summary = new QLabel;
    m_summary->setWordWrap(true);

    m_previewTable = new QTableWidget(0, 5);
    m_previewTable->setHorizontalHeaderLabels({tr("Status"), tr("Source Row"), tr("Target Entity"), tr("Validation Details"), tr("Status Message")});
    m_previewTable->verticalHeader()->setVisible(false);
    m_previewTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_previewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_previewTable->horizontalHeader()->setStretchLastSection(true);

    m_progress = new QProgressBar;
    m_progress->setVisible(false);

    previewLayout->addWidget(m_summary);
    previewLayout->addWidget(m_previewTable, 1);
    previewLayout->addWidget(m_progress);
    m_pages->addWidget(previewPage);

    // ---------------- Page 3: Import Complete ----------------
    auto* donePage = new QWidget;
    auto* doneLayout = new QVBoxLayout(donePage);
    m_doneLabel = new QLabel;
    m_doneLabel->setWordWrap(true);
    m_saveProblemsBtn = new QPushButton(tr("Save problem rows to a file..."));
    m_saveProblemsBtn->setVisible(false);

    doneLayout->addWidget(m_doneLabel);
    doneLayout->addWidget(m_saveProblemsBtn, 0, Qt::AlignLeft);
    doneLayout->addStretch();
    m_pages->addWidget(donePage);

    // ---------------- Buttons ----------------
    auto* buttons = new QHBoxLayout;
    m_backBtn = new QPushButton(tr("Back"));
    m_nextBtn = new QPushButton(tr("Next"));
    m_cancelBtn = new QPushButton(tr("Cancel"));
    m_nextBtn->setDefault(true);

    buttons->addStretch();
    buttons->addWidget(m_backBtn);
    buttons->addWidget(m_nextBtn);
    buttons->addWidget(m_cancelBtn);
    root->addLayout(buttons);

    // Signals
    connect(browse, &QPushButton::clicked, this, &UniversalImportDialog::onBrowse);
    connect(m_backBtn, &QPushButton::clicked, this, &UniversalImportDialog::onBack);
    connect(m_nextBtn, &QPushButton::clicked, this, &UniversalImportDialog::onNext);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_entitySelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &UniversalImportDialog::onEntityChanged);
    connect(m_openPrepBtn, &QPushButton::clicked, this, &UniversalImportDialog::onOpenCsvPrep);

    showPage(PageFile);
}

void UniversalImportDialog::showPage(int page)
{
    m_pages->setCurrentIndex(page);

    m_backBtn->setVisible(page != PageDone);
    m_cancelBtn->setVisible(page != PageDone);
    m_backBtn->setEnabled(page != PageFile);

    switch (page) {
    case PageFile:
        m_nextBtn->setText(tr("Next"));
        m_nextBtn->setEnabled(m_loaded);
        break;
    case PageMapping:
        m_nextBtn->setText(tr("Next"));
        m_nextBtn->setEnabled(true);
        break;
    case PagePreview:
        m_nextBtn->setText(tr("Execute Import (%1 records)").arg(m_importableCount));
        m_nextBtn->setEnabled(m_importableCount > 0);
        break;
    case PageDone:
        m_nextBtn->setText(tr("Close"));
        m_nextBtn->setEnabled(true);
        break;
    }
}

void UniversalImportDialog::onBrowse()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Choose CSV file"), QString(),
        tr("CSV files (*.csv *.txt);;All files (*)"));
    if (!path.isEmpty()) {
        loadFile(path);
    }
}

void UniversalImportDialog::loadFile(const QString& path)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const CsvData data = CsvReader::readFile(path);
    QApplication::restoreOverrideCursor();

    if (!data.ok()) {
        QMessageBox::warning(this, tr("Import Error"), data.error);
        return;
    }
    if (data.rows.isEmpty()) {
        QMessageBox::warning(this, tr("Import Error"), tr("The file has a header row but no data rows."));
        return;
    }

    m_headers = data.headers;
    m_rows = data.rows;
    m_pathEdit->setText(path);
    m_loaded = true;

    // Default to the entity selected in dropdown (defaults to Client)
    m_selectedEntity = static_cast<ImportEntity>(m_entitySelector->currentData().toInt());

    m_fileStatus->setText(tr("%1 rows and %2 columns loaded. Target table: %3.")
                              .arg(m_rows.size())
                              .arg(m_headers.size())
                              .arg(entityToString(m_selectedEntity)));

    updateEntityFields();
    showPage(PageFile);
}

void UniversalImportDialog::onEntityChanged(int index)
{
    m_selectedEntity = static_cast<ImportEntity>(m_entitySelector->itemData(index).toInt());
    if (m_loaded) {
        updateEntityFields();
    }
}

void UniversalImportDialog::updateEntityFields()
{
    m_fields = EntityMapper::fieldsForEntity(m_selectedEntity);
    autoMap();
    buildMappingTable();
}

void UniversalImportDialog::autoMap()
{
    const QVector<QStringList> samples = m_rows.mid(0, 50);
    m_matches = ColumnMatcher().match(m_headers, samples, m_fields);
}

void UniversalImportDialog::buildMappingTable()
{
    m_mapTable->setRowCount(0);
    m_mapTable->setRowCount(m_fields.size());
    m_combos.clear();

    for (int f = 0; f < m_fields.size(); ++f) {
        auto* label = new QTableWidgetItem(m_fields.at(f).label);
        label->setFlags(Qt::ItemIsEnabled);
        m_mapTable->setItem(f, 0, label);

        auto* combo = new QComboBox;
        combo->addItem(tr("(don't import)"), -1);
        for (int c = 0; c < m_headers.size(); ++c) {
            combo->addItem(m_headers.at(c), c);
        }

        const int matchedCol = m_matches.at(f).column;
        const int index = combo->findData(matchedCol);
        combo->setCurrentIndex(index >= 0 ? index : 0);

        m_mapTable->setCellWidget(f, 1, combo);
        m_combos.push_back(combo);

        auto* match = new QTableWidgetItem;
        match->setFlags(Qt::ItemIsEnabled);
        m_mapTable->setItem(f, 2, match);

        auto* example = new QTableWidgetItem;
        example->setFlags(Qt::ItemIsEnabled);
        m_mapTable->setItem(f, 3, example);

        refreshMappingRow(f, false);
        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this, f]() { refreshMappingRow(f, true); });
    }
}

int UniversalImportDialog::comboColumn(int field) const
{
    return (field >= 0 && field < m_combos.size()) ? m_combos.at(field)->currentData().toInt() : -1;
}

void UniversalImportDialog::refreshMappingRow(int field, bool manual)
{
    const int col = comboColumn(field);
    QTableWidgetItem* matchItem = m_mapTable->item(field, 2);
    QTableWidgetItem* exampleItem = m_mapTable->item(field, 3);

    QString text = col < 0 ? QStringLiteral("-") : (manual ? tr("Manual") : tr("%1%").arg(qRound(m_matches.at(field).score * 100)));
    matchItem->setText(text);

    QString sample;
    if (col >= 0) {
        for (const QStringList& row : std::as_const(m_rows)) {
            if (col < row.size() && !row.at(col).trimmed().isEmpty()) {
                sample = row.at(col).trimmed();
                break;
            }
        }
    }
    exampleItem->setText(sample);
}

QHash<QString, int> UniversalImportDialog::currentColumnMap() const
{
    QHash<QString, int> map;
    for (int f = 0; f < m_fields.size(); ++f) {
        map.insert(m_fields.at(f).key, comboColumn(f));
    }
    return map;
}

void UniversalImportDialog::onNext()
{
    const int page = m_pages->currentIndex();
    if (page == PageFile) {
        if (!m_loaded) return;
        showPage(PageMapping);
    } else if (page == PageMapping) {
        buildPreview();
        showPage(PagePreview);
    } else if (page == PagePreview) {
        runImport();
    } else if (page == PageDone) {
        accept();
    }
}

void UniversalImportDialog::onBack()
{
    const int page = m_pages->currentIndex();
    if (page > PageFile && page < PageDone) {
        showPage(page - 1);
    }
}

void UniversalImportDialog::buildPreview()
{
    const QHash<QString, int> columns = currentColumnMap();

    m_results.clear();
    m_importableCount = 0;
    m_duplicateCount = 0;
    m_errorCount = 0;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    if (m_selectedEntity == ImportEntity::InvoiceHistory) {
        // One CSV row per line item: rows are grouped into invoices first, so
        // each result below is one invoice (with all of its lines).
        const EntityMapper::HistoryContext ctx = buildHistoryContext(m_dbManager);
        const QVector<EntityMapper::HistoryResult> groups =
            EntityMapper::buildInvoiceHistory(m_rows, columns, ctx);

        m_results.reserve(groups.size());
        for (const EntityMapper::HistoryResult& g : groups) {
            RowResult result;
            result.sourceRow = g.firstRow;
            result.message = g.message;

            switch (g.state) {
            case EntityMapper::HistoryResult::State::Ok:
                result.state = RowResult::State::Ok;
                result.record.entity = ImportEntity::InvoiceHistory;
                result.record.data = QVariant::fromValue(g.record);
                ++m_importableCount;
                break;
            case EntityMapper::HistoryResult::State::Duplicate:
                result.state = RowResult::State::Duplicate;
                ++m_duplicateCount;
                break;
            case EntityMapper::HistoryResult::State::Error:
                result.state = RowResult::State::Error;
                ++m_errorCount;
                break;
            }
            m_results.push_back(result);
        }
    } else {
        // Items already in the catalog (or repeated in the file) are skipped.
        QSet<QString> knownItems;
        if (m_selectedEntity == ImportEntity::Item) {
            DAO_Services serviceDao(m_dbManager);
            const QVector<Item> existing = serviceDao.getAllServices();
            for (const Item& svc : existing)
                knownItems.insert(EntityMapper::lookupKey(svc.name));
        }

        // Payments are matched to existing invoices up front, so a payment that
        // can't be placed shows up here instead of rolling back the import.
        EntityMapper::PaymentContext paymentCtx;
        if (m_selectedEntity == ImportEntity::Payment)
            paymentCtx = buildPaymentContext(m_dbManager);

        // Enforce single-row import limit for Company Profile
        const int limit = (m_selectedEntity == ImportEntity::Company) ? qMin(1, static_cast<int>(m_rows.size())) : m_rows.size();

        m_results.reserve(limit);
        for (int r = 0; r < limit; ++r) {
            RowResult result;
            result.sourceRow = r;

            QString error;
            if (!EntityMapper::mapRow(m_selectedEntity, m_rows.at(r), columns, result.record, error)) {
                result.state = RowResult::State::Error;
                result.message = error;
                ++m_errorCount;
            } else if (m_selectedEntity == ImportEntity::Payment) {
                Payment resolved;
                QString message;
                const EntityMapper::PaymentMatch match = EntityMapper::resolvePayment(
                    result.record.data.value<EntityMapper::PaymentImportRecord>(), paymentCtx, resolved, message);
                result.message = message;
                if (match == EntityMapper::PaymentMatch::Ok) {
                    result.record.data = QVariant::fromValue(resolved);   // now carries the real invoice_id
                    result.state = RowResult::State::Ok;
                    ++m_importableCount;
                } else if (match == EntityMapper::PaymentMatch::Duplicate) {
                    result.state = RowResult::State::Duplicate;
                    ++m_duplicateCount;
                } else {
                    result.state = RowResult::State::Error;
                    ++m_errorCount;
                }
            } else if (m_selectedEntity == ImportEntity::Item
                       && knownItems.contains(EntityMapper::lookupKey(result.record.data.value<Item>().name))) {
                result.state = RowResult::State::Duplicate;
                result.message = tr("An item with this name already exists");
                ++m_duplicateCount;
            } else {
                if (m_selectedEntity == ImportEntity::Item)
                    knownItems.insert(EntityMapper::lookupKey(result.record.data.value<Item>().name));
                result.state = RowResult::State::Ok;
                result.message = tr("Valid");
                ++m_importableCount;
            }
            m_results.push_back(result);
        }
    }

    QApplication::restoreOverrideCursor();

    m_summary->setText(tr("%1 record(s) ready for import into '%2'. %3 duplicate(s) will be skipped. %4 problem row(s) found.")
                           .arg(m_importableCount)
                           .arg(entityToString(m_selectedEntity))
                           .arg(m_duplicateCount)
                           .arg(m_errorCount));

    fillPreviewTable();
}

void UniversalImportDialog::fillPreviewTable()
{
    m_previewTable->setRowCount(m_results.size());

    for (int row = 0; row < m_results.size(); ++row) {
        const RowResult& r = m_results.at(row);
        const bool ok = (r.state == RowResult::State::Ok);
        const bool duplicate = (r.state == RowResult::State::Duplicate);

        const QString details = (ok && r.message == tr("Valid")) ? tr("Passed validation") : r.message;
        const QStringList cells{
            ok ? tr("Ready") : (duplicate ? tr("Duplicate") : tr("Error")),
            tr("Row %1").arg(r.sourceRow + 1),
            entityToString(m_selectedEntity),
            details,
            ok ? tr("Will import") : tr("Skipped")
        };

        for (int col = 0; col < cells.size(); ++col) {
            auto* item = new QTableWidgetItem(cells.at(col));
            if (!ok) {
                item->setBackground(QBrush(duplicate ? QColor(255, 244, 200) : QColor(255, 224, 224)));
                item->setForeground(QBrush(Qt::black));
            }
            m_previewTable->setItem(row, col, item);
        }
    }
}

bool UniversalImportDialog::saveRecordToDb(const EntityMapper::MappedRecord& rec, QString& error)
{
    switch (rec.entity) {
    case ImportEntity::Client:
        return m_clientDao.insertClient(rec.data.value<Client>());

    case ImportEntity::Invoice:
        return m_invoiceDao.insertInvoice(rec.data.value<Invoice>()) > 0;

    case ImportEntity::Payment: {
        // The invoice was matched in the preview (see resolvePayment), so
        // invoiceId is already this database's own invoice_id.
        const Payment p = rec.data.value<Payment>();
        if (p.invoiceId <= 0) {
            error = QStringLiteral("Payment was not matched to an invoice");
            return false;
        }
        if (m_paymentDao.insertPayment(p) <= 0) {
            error = QStringLiteral("Could not save the payment");
            return false;
        }
        return true;
    }

    case ImportEntity::Company:
        return m_companyDao.saveCompany(rec.data.value<Company>());

    case ImportEntity::Item: {
        DAO_Services serviceDao(m_dbManager);
        if (!serviceDao.insertService(rec.data.value<Item>())) {
            error = QStringLiteral("Could not save the item");
            return false;
        }
        return true;
    }

    case ImportEntity::InvoiceHistory: {
        const EntityMapper::InvoiceHistoryRecord history = rec.data.value<EntityMapper::InvoiceHistoryRecord>();

        const int invoiceId = m_invoiceDao.insertInvoice(history.invoice);
        if (invoiceId <= 0) {
            error = QStringLiteral("Could not save invoice %1").arg(history.invoice.invoiceNumber);
            return false;
        }

        DAO_Items itemsDao(m_dbManager);
        if (!itemsDao.insertItemsForInvoice(invoiceId, history.items)) {
            error = QStringLiteral("Could not save the line items for invoice %1").arg(history.invoice.invoiceNumber);
            return false;
        }
        return true;
    }

    default:
        error = QStringLiteral("Unknown import entity type");
        return false;
    }
}

void UniversalImportDialog::runImport()
{
    QSqlDatabase db = m_dbManager.database();
    if (!db.isOpen()) {
        QMessageBox::warning(this, tr("Import Error"), tr("Database is not open."));
        return;
    }

    if (!db.transaction()) {
        QMessageBox::warning(this, tr("Import Error"), tr("Failed to initiate database transaction: %1").arg(db.lastError().text()));
        return;
    }

    m_imported = 0;
    m_progress->setRange(0, m_importableCount);
    m_progress->setValue(0);
    m_progress->setVisible(true);

    int processed = 0;
    for (const RowResult& result : std::as_const(m_results)) {
        if (result.state != RowResult::State::Ok) continue;

        QString error;
        if (saveRecordToDb(result.record, error)) {
            ++m_imported;
        } else {
            db.rollback();
            m_progress->setVisible(false);
            QMessageBox::warning(this, tr("Import Failed"), tr("Error on row %1: %2. All changes rolled back.").arg(result.sourceRow + 1).arg(error));
            return;
        }

        if (++processed % 25 == 0) {
            m_progress->setValue(processed);
            QCoreApplication::processEvents();
        }
    }

    m_progress->setValue(processed);

    if (!db.commit()) {
        db.rollback();
        m_progress->setVisible(false);
        QMessageBox::warning(this, tr("Import Error"), tr("Failed to commit database transaction: %1").arg(db.lastError().text()));
        return;
    }

    m_doneLabel->setText(tr("Import complete!\n\nSuccessfully added %1 record(s) to table '%2'.")
                             .arg(m_imported)
                             .arg(entityToString(m_selectedEntity)));
    showPage(PageDone);
}

void UniversalImportDialog::onSaveProblemRows()
{
    // Implementation reserved for exporting problem rows to CSV
}

QString UniversalImportDialog::findCsvPrepFile()
{
    // Browsers can't read qrc: paths, so write a real copy out each time.
    // Overwriting on every click guarantees it matches this build.
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString target = QDir(dir).filePath(QStringLiteral("csv-prep-3.html"));

    QFile::remove(target);
    if (!QFile::copy(QStringLiteral(":/new/prefix1/csv-prep-3.html"), target))
        return QString();

    // Files copied from resources are read-only; make it normal.
    QFile::setPermissions(target, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser);
    return target;
}

void UniversalImportDialog::onOpenCsvPrep()
{
    const QString path = findCsvPrepFile();
    if (path.isEmpty()) {
        QMessageBox::warning(this, tr("Open CSV Prep Tool"),
                             tr("Could not prepare the CSV prep tool for opening."));
        return;
    }
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        QMessageBox::warning(this, tr("Open CSV Prep Tool"),
                             tr("Could not open the file in your browser:\n%1").arg(path));
    }
}