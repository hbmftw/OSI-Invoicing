#ifndef UNIVERSALIMPORTDIALOG_H
#define UNIVERSALIMPORTDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>

#include "DatabaseManager.h"
#include "DAO_Client.h"
#include "DAO_Invoice.h"
#include "DAO_Payment.h"
#include "DAO_Company.h"
#include "entitymapper.h"

class UniversalImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UniversalImportDialog(DatabaseManager& dbManager, QWidget* parent = nullptr);

    int importedCount() const { return m_imported; }

private slots:
    void onBrowse();
    void onBack();
    void onNext();
    void onEntityChanged(int index);
    void onSaveProblemRows();
    void onOpenCsvPrep();

private:
    enum Page { PageFile = 0, PageMapping, PagePreview, PageDone };

    struct RowResult {
        enum class State { Ok, Duplicate, Error };
        EntityMapper::MappedRecord record;
        int sourceRow{0};
        State state{State::Ok};
        QString message;
    };

    void showPage(int page);
    void loadFile(const QString& path);
    void updateEntityFields();
    void autoMap();
    void buildMappingTable();
    void refreshMappingRow(int field, bool manual);
    int comboColumn(int field) const;
    QHash<QString, int> currentColumnMap() const;
    QString findCsvPrepFile();

    void buildPreview();
    void fillPreviewTable();
    void runImport();
    bool saveRecordToDb(const EntityMapper::MappedRecord& rec, QString& error);

    DatabaseManager& m_dbManager;
    DAO_Client m_clientDao;
    DAO_Invoice m_invoiceDao;
    DAO_Payment m_paymentDao;
    DAO_Company m_companyDao;

    ImportEntity m_selectedEntity{ImportEntity::Client};
    QVector<TargetField> m_fields;

    QStringList m_headers;
    QVector<QStringList> m_rows;
    QVector<FieldMatch> m_matches;
    QVector<QComboBox*> m_combos;
    QVector<RowResult> m_results;

    int m_importableCount{0};
    int m_duplicateCount{0};
    int m_errorCount{0};
    int m_imported{0};
    bool m_loaded{false};

    QStackedWidget* m_pages{nullptr};
    QComboBox* m_entitySelector{nullptr};
    QLineEdit* m_pathEdit{nullptr};
    QLabel* m_fileStatus{nullptr};
    QTableWidget* m_mapTable{nullptr};
    QLabel* m_summary{nullptr};
    QTableWidget* m_previewTable{nullptr};
    QProgressBar* m_progress{nullptr};
    QLabel* m_doneLabel{nullptr};
    QPushButton* m_saveProblemsBtn{nullptr};
    QPushButton* m_backBtn{nullptr};
    QPushButton* m_nextBtn{nullptr};
    QPushButton* m_cancelBtn{nullptr};
    QPushButton* m_openPrepBtn{nullptr};
};

#endif // UNIVERSALIMPORTDIALOG_H